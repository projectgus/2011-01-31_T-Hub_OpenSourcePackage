/* Clutter.
 * An OpenGL based 'interactive canvas' library.
 * Authored By Matthew Allum  <mallum@openedhand.com>
 * Copyright (C) 2006-2007 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>

#include "clutter-backend-win32.h"
#include "clutter-stage-win32.h"
#include "clutter-win32.h"

#include "../clutter-event.h"
#include "../clutter-main.h"
#include "../clutter-debug.h"
#include "../clutter-private.h"
#include "../clutter-version.h"

#include "cogl/cogl.h"

G_DEFINE_TYPE (ClutterBackendWin32, clutter_backend_win32,
	       CLUTTER_TYPE_BACKEND);

typedef int (WINAPI * SwapIntervalProc) (int interval);

/* singleton object */
static ClutterBackendWin32 *backend_singleton = NULL;

static gchar *clutter_vblank_name = NULL;

gboolean
clutter_backend_win32_pre_parse (ClutterBackend  *backend,
				 GError         **error)
{
  const gchar *env_string;

  if ((env_string = g_getenv ("CLUTTER_VBLANK")))
    clutter_vblank_name = g_strdup (env_string);

  return TRUE;
}

static void
clutter_backend_win32_init_events (ClutterBackend *backend)
{
  CLUTTER_NOTE (EVENT, "initialising the event loop");

  _clutter_backend_win32_events_init (backend);
}

static const GOptionEntry entries[] =
  {
    {
      "vblank", 0, 
      0, 
      G_OPTION_ARG_STRING, &clutter_vblank_name,
      "VBlank method to be used (none, default or wgl)", "METHOD" 
    },
    { NULL }
  };

void
clutter_backend_win32_add_options (ClutterBackend *backend,
				   GOptionGroup   *group)
{
  g_option_group_add_entries (group, entries);
}

static void
clutter_backend_win32_finalize (GObject *gobject)
{
  backend_singleton = NULL;

  timeEndPeriod (1);

  G_OBJECT_CLASS (clutter_backend_win32_parent_class)->finalize (gobject);
}

static void
clutter_backend_win32_dispose (GObject *gobject)
{
  ClutterBackendWin32 *backend_win32 = CLUTTER_BACKEND_WIN32 (gobject);
  ClutterMainContext  *context;
  ClutterStageManager *stage_manager;

  CLUTTER_NOTE (BACKEND, "Disposing the of stages");

  context = clutter_context_get_default ();
  stage_manager = context->stage_manager;

  /* Destroy all of the stages. g_slist_foreach is used because the
     finalizer for the stages will remove the stage from the
     stage_manager's list and g_slist_foreach has some basic
     protection against this */
  g_slist_foreach (stage_manager->stages, (GFunc) clutter_actor_destroy, NULL);

  CLUTTER_NOTE (BACKEND, "Removing the event source");
  _clutter_backend_win32_events_uninit (CLUTTER_BACKEND (backend_win32));

  /* Unrealize all shaders, since the GL context is going away */
  _clutter_shader_release_all ();

  if (backend_win32->gl_context)
    {
      wglDeleteContext (backend_win32->gl_context);
      backend_win32->gl_context = NULL;
    }

  G_OBJECT_CLASS (clutter_backend_win32_parent_class)->dispose (gobject);
}

static GObject *
clutter_backend_win32_constructor (GType                  gtype,
				   guint                  n_params,
				   GObjectConstructParam *params)
{
  GObjectClass *parent_class;
  GObject *retval;

  if (!backend_singleton)
    {
      parent_class = G_OBJECT_CLASS (clutter_backend_win32_parent_class);
      retval = parent_class->constructor (gtype, n_params, params);

      backend_singleton = CLUTTER_BACKEND_WIN32 (retval);

      return retval;
    }

  g_warning ("Attempting to create a new backend object. This should "
             "never happen, so we return the singleton instance.");

  return g_object_ref (backend_singleton);
}

static gboolean
check_vblank_env (const char *name)
{
  return clutter_vblank_name && !g_ascii_strcasecmp (clutter_vblank_name, name);
}

ClutterFeatureFlags
clutter_backend_win32_get_features (ClutterBackend *backend)
{
  ClutterFeatureFlags  flags;
  const gchar         *extensions;
  SwapIntervalProc     swap_interval;
  ClutterBackendWin32 *backend_win32;

  /* FIXME: we really need to check if gl context is set */

  extensions = (const gchar *) glGetString (GL_EXTENSIONS);

  /* this will make sure that the GL context exists and is bound to a
     drawable */
  backend_win32 = CLUTTER_BACKEND_WIN32 (backend);
  g_return_val_if_fail (backend_win32->gl_context != NULL, 0);
  g_return_val_if_fail (wglGetCurrentDC () != NULL, 0);

  CLUTTER_NOTE (BACKEND, "Checking features\n"
                "GL_VENDOR: %s\n"
                "GL_RENDERER: %s\n"
                "GL_VERSION: %s\n"
                "GL_EXTENSIONS: %s\n",
                glGetString (GL_VENDOR),
                glGetString (GL_RENDERER),
                glGetString (GL_VERSION),
                extensions);

  flags = CLUTTER_FEATURE_STAGE_USER_RESIZE
    | CLUTTER_FEATURE_STAGE_CURSOR
    | CLUTTER_FEATURE_STAGE_MULTIPLE;

  /* If the VBlank should be left at the default or it has been
     disabled elsewhere (eg NVIDIA) then don't bother trying to check
     for the swap control extension */
  if (getenv ("__GL_SYNC_TO_VBLANK") || check_vblank_env ("default"))
    CLUTTER_NOTE (BACKEND, "vblank sync: left at default at user request");
  else if (cogl_check_extension ("WGL_EXT_swap_control", extensions)
	   && (swap_interval = (SwapIntervalProc)
	       cogl_get_proc_address ("wglSwapIntervalEXT")))
    {
      /* According to the specification for the WGL_EXT_swap_control
	 extension the default swap interval is 1 anyway, so if no
	 vblank is requested then we should explicitly set it to
	 zero */
      if (check_vblank_env ("none"))
	{
	  if (swap_interval (0))
	    CLUTTER_NOTE (BACKEND, "vblank sync: successfully disabled");
	  else
	    CLUTTER_NOTE (BACKEND, "vblank sync: disabling failed");
	}
      else
	{
	  if (swap_interval (1))
	    {
	      flags |= CLUTTER_FEATURE_SYNC_TO_VBLANK;
	      CLUTTER_NOTE (BACKEND, "vblank sync: wglSwapIntervalEXT "
			    "vblank setup success");
	    }
	  else
	    CLUTTER_NOTE (BACKEND, "vblank sync: wglSwapIntervalEXT "
			  "vblank setup failed");
	}
    }
  else
    CLUTTER_NOTE (BACKEND, "no use-able vblank mechanism found");

  return flags;
}

static void
clutter_backend_win32_ensure_context (ClutterBackend *backend, 
				      ClutterStage   *stage)
{
  if (stage == NULL)
    {
      CLUTTER_NOTE (MULTISTAGE, "Clearing all context");

      wglMakeCurrent (NULL, NULL);
    }
  else
    {
      ClutterBackendWin32 *backend_win32;
      ClutterStageWin32   *stage_win32;
      ClutterStageWindow  *impl;

      impl = _clutter_stage_get_window (stage);
      g_return_if_fail (impl != NULL);
      
      CLUTTER_NOTE (MULTISTAGE, "Setting context for stage of type %s [%p]",
		    g_type_name (G_OBJECT_TYPE (impl)),
		    impl);

      backend_win32 = CLUTTER_BACKEND_WIN32 (backend);
      stage_win32 = CLUTTER_STAGE_WIN32 (impl);
      
      /* no GL context to set */
      if (backend_win32->gl_context == NULL)
	return;

      /* we might get here inside the final dispose cycle, so we
       * need to handle this gracefully
       */
      if (stage_win32->client_dc == NULL)
	{
	  CLUTTER_NOTE (MULTISTAGE,
			"Received a stale stage, clearing all context");
  	 
	  wglMakeCurrent (NULL, NULL);
	}
      else
	{
	  CLUTTER_NOTE (BACKEND,
			"MakeCurrent window %p (%s), context %p",
			stage_win32->hwnd,
			stage_win32->is_foreign_win ? "foreign" : "native",
			backend_win32->gl_context);
	  wglMakeCurrent (stage_win32->client_dc,
			  backend_win32->gl_context);
	}
    }
}

static void
clutter_backend_win32_redraw (ClutterBackend *backend,
			      ClutterStage   *stage)
{
  ClutterStageWin32  *stage_win32;
  ClutterStageWindow *impl;

  impl = _clutter_stage_get_window (stage);
  if (impl == NULL)
    return;

  g_return_if_fail (CLUTTER_IS_STAGE_WIN32 (impl));

  stage_win32 = CLUTTER_STAGE_WIN32 (impl);

  /* this will cause the stage implementation to be painted */
  clutter_actor_paint (CLUTTER_ACTOR (stage));

  if (stage_win32->client_dc)
    SwapBuffers (stage_win32->client_dc);
}

static ClutterActor *
clutter_backend_win32_create_stage (ClutterBackend  *backend,
				    ClutterStage    *wrapper,
				    GError         **error)
{
  ClutterBackendWin32 *backend_win32 = CLUTTER_BACKEND_WIN32 (backend);
  ClutterStageWin32 *stage_win32;
  ClutterActor *stage;

  CLUTTER_NOTE (BACKEND, "Creating stage of type `%s'",
		g_type_name (CLUTTER_STAGE_TYPE));

  stage = g_object_new (CLUTTER_TYPE_STAGE_WIN32, NULL);

  /* copy backend data into the stage */
  stage_win32 = CLUTTER_STAGE_WIN32 (stage);
  stage_win32->backend = backend_win32;
  stage_win32->wrapper = wrapper;

  return stage;
}

static void
clutter_backend_win32_class_init (ClutterBackendWin32Class *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterBackendClass *backend_class = CLUTTER_BACKEND_CLASS (klass);

  gobject_class->constructor = clutter_backend_win32_constructor;
  gobject_class->dispose = clutter_backend_win32_dispose;
  gobject_class->finalize = clutter_backend_win32_finalize;

  backend_class->pre_parse        = clutter_backend_win32_pre_parse;
  backend_class->init_events      = clutter_backend_win32_init_events;
  backend_class->create_stage     = clutter_backend_win32_create_stage;
  backend_class->add_options      = clutter_backend_win32_add_options;
  backend_class->get_features     = clutter_backend_win32_get_features;
  backend_class->redraw           = clutter_backend_win32_redraw;
  backend_class->ensure_context   = clutter_backend_win32_ensure_context;
}

static void
clutter_backend_win32_init (ClutterBackendWin32 *backend_win32)
{
  ClutterBackend *backend = CLUTTER_BACKEND (backend_win32);

  backend_win32->gl_context         = NULL;
  backend_win32->no_event_retrieval = FALSE;

  /* FIXME: get from GetSystemMetric? */
  clutter_backend_set_double_click_time (backend, 250);
  clutter_backend_set_double_click_distance (backend, 5);
  clutter_backend_set_resolution (backend, 96.0);

  /* Set the maximum precision for Windows time functions. Without
     this glib will not be able to sleep accurately enough to give a
     reasonable frame rate */
  timeBeginPeriod (1);
}

GType
_clutter_backend_impl_get_type (void)
{
  return clutter_backend_win32_get_type ();
}
