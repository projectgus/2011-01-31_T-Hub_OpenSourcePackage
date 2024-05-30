#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clutter-backend-egl.h"
#include "clutter-stage-egl.h"
#include "clutter-eglx.h"

#include "../clutter-main.h"
#include "../clutter-feature.h"
#include "../clutter-color.h"
#include "../clutter-util.h"
#include "../clutter-event.h"
#include "../clutter-enum-types.h"
#include "../clutter-private.h"
#include "../clutter-debug.h"
#include "../clutter-units.h"
#include "../clutter-container.h"
#include "../clutter-stage.h"
#include "../clutter-stage-window.h"

static void clutter_stage_window_iface_init (ClutterStageWindowIface *iface);

G_DEFINE_TYPE_WITH_CODE (ClutterStageEGL,
                         clutter_stage_egl,
                         CLUTTER_TYPE_STAGE_X11,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_STAGE_WINDOW,
                                                clutter_stage_window_iface_init));

static void
clutter_stage_egl_unrealize (ClutterActor *actor)
{
  ClutterStageEGL *stage_egl = CLUTTER_STAGE_EGL (actor);
  ClutterStageX11 *stage_x11 = CLUTTER_STAGE_X11 (actor);
  gboolean was_offscreen;

  CLUTTER_MARK();

  g_object_get (stage_x11->wrapper, "offscreen", &was_offscreen, NULL);

  CLUTTER_ACTOR_CLASS (clutter_stage_egl_parent_class)->unrealize (actor);

  clutter_x11_trap_x_errors ();

  if (G_UNLIKELY (was_offscreen))
    {
      /* No support as yet for this */
    }
  else
    {
      if (!stage_x11->is_foreign_xwin && stage_x11->xwin != None)
	{
	  XDestroyWindow (stage_x11->xdpy, stage_x11->xwin);
	  stage_x11->xwin = None;
	}
      else
	stage_x11->xwin = None;
    }

  if (stage_egl->egl_surface)
    {
      eglDestroySurface (clutter_eglx_display (), stage_egl->egl_surface);
      stage_egl->egl_surface = EGL_NO_SURFACE;
    }

  XSync (stage_x11->xdpy, False);

  clutter_x11_untrap_x_errors ();

  CLUTTER_MARK ();
}

static void
clutter_stage_egl_realize (ClutterActor *actor)
{
  ClutterStageEGL   *stage_egl = CLUTTER_STAGE_EGL (actor);
  ClutterStageX11   *stage_x11 = CLUTTER_STAGE_X11 (actor);
  ClutterBackendEGL *backend_egl;
  ClutterBackendX11 *backend_x11;
  EGLConfig          configs[2];
  EGLint             config_count;
  EGLBoolean         status;
  gboolean           is_offscreen = FALSE;

  CLUTTER_NOTE (BACKEND, "Realizing main stage");

  g_object_get (stage_x11->wrapper, "offscreen", &is_offscreen, NULL);

  backend_egl = CLUTTER_BACKEND_EGL (clutter_get_default_backend ());
  backend_x11 = CLUTTER_BACKEND_X11 (clutter_get_default_backend ());

  if (G_LIKELY (!is_offscreen))
    {
      int c;
      int num_configs;
      EGLConfig *all_configs;

      EGLint cfg_attribs[] = {
        EGL_BUFFER_SIZE,    EGL_DONT_CARE,
	EGL_RED_SIZE,       5,
	EGL_GREEN_SIZE,     6,
	EGL_BLUE_SIZE,      5,

#ifdef HAVE_COGL_GLES2
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#else /* HAVE_COGL_GLES2 */
	EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
#endif /* HAVE_COGL_GLES2 */

	EGL_NONE
      };

      status = eglGetConfigs (backend_egl->edpy,
			      configs,
			      2,
			      &config_count);

      eglGetConfigs (clutter_eglx_display (), NULL, 0, &num_configs);

      all_configs = g_malloc (num_configs * sizeof (EGLConfig));
      eglGetConfigs (clutter_eglx_display (),
		     all_configs,
		     num_configs,
		     &num_configs);

      for (c = 0; c < num_configs; ++c)
	{
	  EGLint red = -1, green = -1, blue = -1, alpha = -1, stencil = -1;

	  eglGetConfigAttrib (clutter_eglx_display (),
			      all_configs[c],
			      EGL_RED_SIZE, &red);
	  eglGetConfigAttrib (clutter_eglx_display (),
			      all_configs[c],
			      EGL_GREEN_SIZE, &green);
	  eglGetConfigAttrib (clutter_eglx_display (),
			      all_configs[c],
			      EGL_BLUE_SIZE, &blue);
	  eglGetConfigAttrib (clutter_eglx_display (),
			      all_configs[c],
			      EGL_ALPHA_SIZE, &alpha);
	  eglGetConfigAttrib (clutter_eglx_display (),
			      all_configs[c],
			      EGL_STENCIL_SIZE, &stencil);
	  CLUTTER_NOTE (BACKEND, "EGLConfig == R:%d G:%d B:%d A:%d S:%d \n",
		        red, green, blue, alpha, stencil);
	}

      g_free (all_configs);

      if (status != EGL_TRUE)
	{
          g_critical ("eglGetConfigs failed");
          goto fail;
        }

      status = eglChooseConfig (backend_egl->edpy,
				cfg_attribs,
				configs,
                                G_N_ELEMENTS (configs),
				&config_count);

      if (status != EGL_TRUE)
        {
          g_critical ("eglChooseConfig failed");
          goto fail;
        }

      if (stage_x11->xwin == None)
	stage_x11->xwin =
	  XCreateSimpleWindow (stage_x11->xdpy,
                               stage_x11->xwin_root,
                               0, 0,
                               stage_x11->xwin_width,
                               stage_x11->xwin_height,
                               0, 0,
                               WhitePixel (stage_x11->xdpy,
                                           stage_x11->xscreen));

      if (clutter_x11_has_event_retrieval ())
        {
          if (clutter_x11_has_xinput ())
            {
              XSelectInput (stage_x11->xdpy, stage_x11->xwin,
                            StructureNotifyMask |
                            FocusChangeMask |
                            ExposureMask |
                            PropertyChangeMask);
#ifdef USE_XINPUT          
              _clutter_x11_select_events (stage_x11->xwin);
#endif
            }
          else
            XSelectInput (stage_x11->xdpy, stage_x11->xwin,
                          StructureNotifyMask |
                          FocusChangeMask |
                          ExposureMask |
                          PointerMotionMask |
                          KeyPressMask | KeyReleaseMask |
                          ButtonPressMask | ButtonReleaseMask |
                          PropertyChangeMask);
        }

      /* FIXME, do these in a clutterstage_x11_realise? */
      clutter_stage_x11_fix_window_size (stage_x11);
      clutter_stage_x11_set_wm_protocols (stage_x11);

      if (stage_egl->egl_surface != EGL_NO_SURFACE)
        {
	  eglDestroySurface (backend_egl->edpy, stage_egl->egl_surface);
          stage_egl->egl_surface = EGL_NO_SURFACE;
        }

      stage_egl->egl_surface =
        eglCreateWindowSurface (backend_egl->edpy,
                                configs[0],
                                (NativeWindowType) stage_x11->xwin,
                                NULL);

      if (stage_egl->egl_surface == EGL_NO_SURFACE)
        {
          g_critical ("Unable to create an EGL surface");
          goto fail;
        }

      if (G_UNLIKELY (backend_egl->egl_context == None))
        {
#ifdef HAVE_COGL_GLES2
	  static const EGLint attribs[3]
	    = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

          backend_egl->egl_context = eglCreateContext (backend_egl->edpy,
						       configs[0],
                                                       EGL_NO_CONTEXT,
                                                       attribs);
#else
          /* Seems some GLES implementations 1.x do not like attribs... */
          backend_egl->egl_context = eglCreateContext (backend_egl->edpy,
						       configs[0],
                                                       EGL_NO_CONTEXT,
                                                       NULL);
#endif
          if (backend_egl->egl_context == EGL_NO_CONTEXT)
            {
              g_critical ("Unable to create a suitable EGL context");
              goto fail;
            }

          CLUTTER_NOTE (GL, "Created EGL Context");
        }

      CLUTTER_ACTOR_SET_FLAGS (actor, CLUTTER_ACTOR_REALIZED);
    }
  else
    {
      g_critical ("EGLX Backend does not support offscreen rendering");
      goto fail;
    }

  /* we need to chain up to the X11 stage implementation in order to
   * set the window state in case we set it before realizing the stage
   */
  CLUTTER_ACTOR_CLASS (clutter_stage_egl_parent_class)->realize (actor);
  return;

fail:
  CLUTTER_ACTOR_UNSET_FLAGS (actor, CLUTTER_ACTOR_REALIZED);
}

static void
clutter_stage_egl_dispose (GObject *gobject)
{
  ClutterStageEGL *stage_egl = CLUTTER_STAGE_EGL (gobject);
  ClutterStageX11 *stage_x11 = CLUTTER_STAGE_X11 (gobject);

  if (stage_x11->xwin)
    clutter_actor_unrealize (CLUTTER_ACTOR (stage_egl));

  G_OBJECT_CLASS (clutter_stage_egl_parent_class)->dispose (gobject);
}

static void
clutter_stage_window_iface_init (ClutterStageWindowIface *iface)
{
  /* the rest is inherited from ClutterStageX11 */
}

static void
clutter_stage_egl_class_init (ClutterStageEGLClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  gobject_class->dispose = clutter_stage_egl_dispose;

  actor_class->realize = clutter_stage_egl_realize;
  actor_class->unrealize = clutter_stage_egl_unrealize;
}

static void
clutter_stage_egl_init (ClutterStageEGL *stage)
{
}
