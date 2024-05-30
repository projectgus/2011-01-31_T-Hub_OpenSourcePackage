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

#include "clutter-backend-glx.h"
#include "clutter-stage-glx.h"
#include "clutter-glx.h"

#include "../clutter-main.h"
#include "../clutter-feature.h"
#include "../clutter-color.h"
#include "../clutter-util.h"
#include "../clutter-event.h"
#include "../clutter-enum-types.h"
#include "../clutter-private.h"
#include "../clutter-debug.h"
#include "../clutter-units.h"
#include "../clutter-shader.h"
#include "../clutter-group.h"
#include "../clutter-container.h"
#include "../clutter-stage.h"
#include "../clutter-stage-window.h"

#include "cogl/cogl.h"

#include <GL/glx.h>
#include <GL/gl.h>

static void clutter_stage_window_iface_init (ClutterStageWindowIface *iface);

G_DEFINE_TYPE_WITH_CODE (ClutterStageGLX,
                         clutter_stage_glx,
                         CLUTTER_TYPE_STAGE_X11,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_STAGE_WINDOW,
                                                clutter_stage_window_iface_init));

static void
clutter_stage_glx_unrealize (ClutterActor *actor)
{
  ClutterStageX11 *stage_x11 = CLUTTER_STAGE_X11 (actor);
  ClutterStageGLX *stage_glx = CLUTTER_STAGE_GLX (actor);
  gboolean was_offscreen;

  /* Note unrealize should free up any backend stage related resources */
  CLUTTER_NOTE (BACKEND, "Unrealizing stage");

  g_object_get (stage_x11->wrapper, "offscreen", &was_offscreen, NULL);

  /* Chain up so all children get unrealized, needed to move texture data
   * across contexts
  */
  CLUTTER_ACTOR_CLASS (clutter_stage_glx_parent_class)->unrealize (actor);

  clutter_x11_trap_x_errors ();

  if (G_UNLIKELY (was_offscreen))
    {
      if (stage_glx->glxpixmap)
        {
          glXDestroyGLXPixmap (stage_x11->xdpy,stage_glx->glxpixmap);
          stage_glx->glxpixmap = None;
        }

      if (stage_x11->xpixmap)
        {
          XFreePixmap (stage_x11->xdpy, stage_x11->xpixmap);
          stage_x11->xpixmap = None;
        }
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

  XSync (stage_x11->xdpy, False);

  clutter_x11_untrap_x_errors ();

  CLUTTER_MARK ();
}

static void
clutter_stage_glx_realize (ClutterActor *actor)
{
  ClutterStageX11   *stage_x11 = CLUTTER_STAGE_X11 (actor);
  ClutterStageGLX   *stage_glx = CLUTTER_STAGE_GLX (actor);
  ClutterBackendGLX *backend_glx;
  ClutterBackendX11 *backend_x11;
  gboolean           is_offscreen;

  CLUTTER_NOTE (MISC, "Realizing main stage");

  g_object_get (stage_x11->wrapper, "offscreen", &is_offscreen, NULL);

  backend_glx = CLUTTER_BACKEND_GLX (clutter_get_default_backend ());
  backend_x11 = CLUTTER_BACKEND_X11 (clutter_get_default_backend ());

  if (G_LIKELY (!is_offscreen))
    {
      int gl_attributes[] = 
        {
          GLX_RGBA, 
          GLX_DOUBLEBUFFER,
          GLX_RED_SIZE, 1,
          GLX_GREEN_SIZE, 1,
          GLX_BLUE_SIZE, 1,
          GLX_STENCIL_SIZE, 1,
          0
        };

      if (stage_x11->xvisinfo)
        {
          XFree (stage_x11->xvisinfo);
          stage_x11->xvisinfo = None;
        }

      /* The following check seems strange */
      if (stage_x11->xvisinfo == None)
        stage_x11->xvisinfo = glXChooseVisual (stage_x11->xdpy,
                                               stage_x11->xscreen,
                                               gl_attributes);
      if (!stage_x11->xvisinfo)
        {
          g_critical ("Unable to find suitable GL visual.");
          goto fail;
        }

      if (stage_x11->xwin == None)
        {
          XSetWindowAttributes xattr;
          unsigned long mask;

          CLUTTER_NOTE (MISC, "Creating stage X window");

          /* window attributes */  
          xattr.background_pixel = WhitePixel (stage_x11->xdpy,
                                               stage_x11->xscreen);
          xattr.border_pixel = 0;
          xattr.colormap = XCreateColormap (stage_x11->xdpy, 
                                            stage_x11->xwin_root,
                                            stage_x11->xvisinfo->visual,
                                            AllocNone);
          mask = CWBackPixel | CWBorderPixel | CWColormap;
          stage_x11->xwin = XCreateWindow (stage_x11->xdpy,
                                           stage_x11->xwin_root,
                                           0, 0,
                                           stage_x11->xwin_width,
                                           stage_x11->xwin_height,
                                           0,
                                           stage_x11->xvisinfo->depth,
                                           InputOutput,
                                           stage_x11->xvisinfo->visual,
                                           mask, &xattr);
        }

      if (clutter_x11_has_event_retrieval())
        {
          if (clutter_x11_has_xinput())
            {
              XSelectInput (stage_x11->xdpy, stage_x11->xwin,
                            StructureNotifyMask |
                            FocusChangeMask |
                            ExposureMask |
                            KeyPressMask | KeyReleaseMask |
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

      /* no user resize.. */
      clutter_stage_x11_fix_window_size (stage_x11);
      clutter_stage_x11_set_wm_protocols (stage_x11);

      if (G_UNLIKELY (backend_glx->gl_context == None))
        {
          CLUTTER_NOTE (GL, "Creating GL Context");
          backend_glx->gl_context = glXCreateContext (stage_x11->xdpy, 
                                                      stage_x11->xvisinfo, 
                                                      0,
                                                      True);

          if (backend_glx->gl_context == None)
            {
              g_critical ("Unable to create suitable GL context.");
              goto fail;
            }
        }

      CLUTTER_NOTE (BACKEND, "Marking stage as realized");
      CLUTTER_ACTOR_SET_FLAGS (stage_x11, CLUTTER_ACTOR_REALIZED);
    }
  else
    {
      int gl_attributes[] = {
        GLX_DEPTH_SIZE,    0,
        GLX_ALPHA_SIZE,    0,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_USE_GL,
        GLX_RGBA,
        0
      };

      if (stage_x11->xvisinfo)
         XFree (stage_x11->xvisinfo);

      stage_x11->xvisinfo = NULL;

      CLUTTER_NOTE (GL, "glXChooseVisual");
      stage_x11->xvisinfo = glXChooseVisual (stage_x11->xdpy,
                                             stage_x11->xscreen,
                                             gl_attributes);
      if (!stage_x11->xvisinfo)
        {
          g_critical ("Unable to find suitable GL visual.");
          goto fail;
        }

     
      stage_x11->xpixmap = XCreatePixmap (stage_x11->xdpy,
                                          stage_x11->xwin_root,
                                          stage_x11->xwin_width, 
                                          stage_x11->xwin_height,
                                          DefaultDepth (stage_x11->xdpy,
                                                        stage_x11->xscreen));

      stage_glx->glxpixmap = glXCreateGLXPixmap (stage_x11->xdpy,
                                                 stage_x11->xvisinfo,
                                                 stage_x11->xpixmap);

      if (backend_glx->gl_context == None)
        {
          CLUTTER_NOTE (GL, "Creating GL Context");

          /* FIXME: we probably need a seperate offscreen context here
           * - though it likely makes most sense to drop offscreen stages
           * and rely on FBO's instead and GLXPixmaps seems mostly broken
           * anyway..
          */
          backend_glx->gl_context =  glXCreateContext (stage_x11->xdpy, 
                                                       stage_x11->xvisinfo, 
                                                       0,
                                                       False);

          if (backend_glx->gl_context == None)
            {
              g_critical ("Unable to create suitable GL context.");
              goto fail;
            }
        }

      CLUTTER_NOTE (BACKEND, "Marking stage as realized");
      CLUTTER_ACTOR_SET_FLAGS (stage_x11, CLUTTER_ACTOR_REALIZED);
    }

  /* we need to chain up to the X11 stage implementation in order to
   * set the window state in case we set it before realizing the stage
   */
  CLUTTER_ACTOR_CLASS (clutter_stage_glx_parent_class)->realize (actor);
  return;
  
fail:
  CLUTTER_ACTOR_UNSET_FLAGS (actor, CLUTTER_ACTOR_REALIZED);
}

static void
clutter_stage_glx_dispose (GObject *gobject)
{
  ClutterStageGLX *stage_glx = CLUTTER_STAGE_GLX (gobject);
  ClutterStageX11 *stage_x11 = CLUTTER_STAGE_X11 (gobject);

  if (stage_x11->xwin)
    clutter_actor_unrealize (CLUTTER_ACTOR (stage_glx));

  G_OBJECT_CLASS (clutter_stage_glx_parent_class)->dispose (gobject);
}

static void
clutter_stage_glx_class_init (ClutterStageGLXClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  gobject_class->dispose = clutter_stage_glx_dispose;
  
  actor_class->realize = clutter_stage_glx_realize;
  actor_class->unrealize = clutter_stage_glx_unrealize;
}

static void
clutter_stage_glx_init (ClutterStageGLX *stage)
{
}

static void
clutter_stage_window_iface_init (ClutterStageWindowIface *iface)
{
  /* the rest is inherited from ClutterStageX11 */
}
