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

#include "clutter-stage-x11.h"
#include "clutter-backend-x11.h"
#include "clutter-x11.h"

#include "../clutter-backend.h"
#include "../clutter-event.h"
#include "../clutter-private.h"
#include "../clutter-debug.h"
#include "../clutter-main.h"

#include <string.h>

#include <glib.h>

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include <X11/Xatom.h>

#ifdef USE_XINPUT
#include <X11/extensions/XInput.h>
#endif

/* XEMBED protocol support for toolkit embedding */
#define XEMBED_MAPPED                   (1 << 0)
#define MAX_SUPPORTED_XEMBED_VERSION    1

#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON              10
#define XEMBED_MODALITY_OFF             11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

static Window ParentEmbedderWin = None;

typedef struct _ClutterEventSource      ClutterEventSource;

struct _ClutterEventSource
{
  GSource source;

  ClutterBackend *backend;
  GPollFD event_poll_fd;
};

static gboolean clutter_event_prepare  (GSource     *source,
                                        gint        *timeout);
static gboolean clutter_event_check    (GSource     *source);
static gboolean clutter_event_dispatch (GSource     *source,
                                        GSourceFunc  callback,
                                        gpointer     user_data);

static GList *event_sources = NULL;

static GSourceFuncs event_funcs = {
  clutter_event_prepare,
  clutter_event_check,
  clutter_event_dispatch,
  NULL
};

static GSource *
clutter_event_source_new (ClutterBackend *backend)
{
  GSource *source = g_source_new (&event_funcs, sizeof (ClutterEventSource));
  ClutterEventSource *event_source = (ClutterEventSource *) source;

  event_source->backend = backend;

  return source;
}

static gboolean
check_xpending (ClutterBackend *backend)
{
  return XPending (CLUTTER_BACKEND_X11 (backend)->xdpy);
}

static gboolean
xembed_send_message (ClutterBackendX11 *backend_x11,
                     Window             window,
                     long               message,
                     long               detail,
                     long               data1,
                     long               data2)
{
  XEvent ev;

  memset (&ev, 0, sizeof (ev));

  ev.xclient.type = ClientMessage;
  ev.xclient.window = window;
  ev.xclient.message_type = backend_x11->atom_XEMBED;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = message;
  ev.xclient.data.l[2] = detail;
  ev.xclient.data.l[3] = data1;
  ev.xclient.data.l[4] = data2;

  clutter_x11_trap_x_errors ();

  XSendEvent (backend_x11->xdpy, window, False, NoEventMask, &ev);
  XSync (backend_x11->xdpy, False);

  if (clutter_x11_untrap_x_errors ())
    return False;

  return True;
}

static void
xembed_set_info (ClutterBackendX11 *backend_x11,
                 Window             window,
                 gint               flags)
{
  gint32 list[2];

  list[0] = MAX_SUPPORTED_XEMBED_VERSION;
  list[1] = XEMBED_MAPPED;

  XChangeProperty (backend_x11->xdpy, window,
                   backend_x11->atom_XEMBED_INFO,
                   backend_x11->atom_XEMBED_INFO, 32,
                   PropModeReplace, (unsigned char *) list, 2);
}

void
_clutter_backend_x11_events_init (ClutterBackend *backend)
{
  ClutterBackendX11 *backend_x11 = CLUTTER_BACKEND_X11 (backend);
  GSource *source;
  ClutterEventSource *event_source;
  int connection_number;

  connection_number = ConnectionNumber (backend_x11->xdpy);
  CLUTTER_NOTE (EVENT, "Connection number: %d", connection_number);

  source = backend_x11->event_source = clutter_event_source_new (backend);
  event_source = (ClutterEventSource *) source;
  g_source_set_priority (source, CLUTTER_PRIORITY_EVENTS);

  event_source->event_poll_fd.fd = connection_number;
  event_source->event_poll_fd.events = G_IO_IN;

  event_sources = g_list_prepend (event_sources, event_source);

  g_source_add_poll (source, &event_source->event_poll_fd);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

}

void
_clutter_backend_x11_events_uninit (ClutterBackend *backend)
{
  ClutterBackendX11 *backend_x11 = CLUTTER_BACKEND_X11 (backend);

  if (backend_x11->event_source)
    {
      CLUTTER_NOTE (EVENT, "Destroying the event source");

      event_sources = g_list_remove (event_sources,
                                     backend_x11->event_source);

      g_source_destroy (backend_x11->event_source);
      g_source_unref (backend_x11->event_source);
      backend_x11->event_source = NULL;
    }
}

static void
set_user_time (ClutterBackendX11 *backend_x11,
               Window            *xwindow,
               long               timestamp)
{
  if (timestamp != CLUTTER_CURRENT_TIME)
    {
      XChangeProperty (backend_x11->xdpy, *xwindow,
                       backend_x11->atom_NET_WM_USER_TIME,
                       XA_CARDINAL, 32, PropModeReplace,
                       (unsigned char *) &timestamp, 1);
    }
}

#if 0 /* See XInput keyboard comment below USE_XINPUT */
static void
convert_xdevicekey_to_xkey (XDeviceKeyEvent *xkev, XEvent *xevent)
{
  xevent->xany.type = xevent->xkey.type = xkev->type;
  xevent->xkey.serial = xkev->serial;
  xevent->xkey.display = xkev->display;
  xevent->xkey.window = xkev->window;
  xevent->xkey.root = xkev->root;
  xevent->xkey.subwindow = xkev->subwindow;
  xevent->xkey.time = xkev->time;
  xevent->xkey.x = xkev->x;
  xevent->xkey.y = xkev->y;
  xevent->xkey.x_root = xkev->x_root;
  xevent->xkey.y_root = xkev->y_root;
  xevent->xkey.state = xkev->state;
  xevent->xkey.keycode = xkev->keycode;
  xevent->xkey.same_screen = xkev->same_screen;
}
#endif /* USE_XINPUT */

static void
translate_key_event (ClutterBackend   *backend,
                     ClutterEvent     *event,
                     XEvent           *xevent)
{
  char buffer[256+1];
  int n;
  
  CLUTTER_NOTE (EVENT, "Translating key %s event",
                xevent->xany.type == KeyPress ? "press" : "release");

  event->key.time = xevent->xkey.time;
  event->key.modifier_state = (ClutterModifierType) xevent->xkey.state;
  event->key.hardware_keycode = xevent->xkey.keycode;

  /* keyval is the key ignoring all modifiers ('1' vs. '!') */
  event->key.keyval =
    XKeycodeToKeysym (xevent->xkey.display,
                      xevent->xkey.keycode,
                      0);

  /* unicode_value is the printable representation */
  n = XLookupString (&xevent->xkey, buffer, sizeof (buffer) - 1, NULL, NULL);

  if (n != NoSymbol)
    {
      event->key.unicode_value = g_utf8_get_char_validated (buffer, n);
      if ((event->key.unicode_value != -1) &&
          (event->key.unicode_value != -2))
        return;
    }
  
  event->key.unicode_value = (gunichar)'\0';
}

static gboolean
handle_wm_protocols_event (ClutterBackendX11 *backend_x11,
                           Window             window,
                           XEvent            *xevent)
{
  Atom atom = (Atom) xevent->xclient.data.l[0];

  if (atom == backend_x11->atom_WM_DELETE_WINDOW &&
      xevent->xany.window == window)
    {
      /* the WM_DELETE_WINDOW is a request: we do not destroy
       * the window right away, as it might contain vital data;
       * we relay the event to the application and we let it
       * handle the request
       */
      CLUTTER_NOTE (EVENT, "delete window:\txid: %ld",
                    xevent->xclient.window);

      set_user_time (backend_x11,
                     &window,
                     xevent->xclient.data.l[1]);

      return TRUE;
    }
  else if (atom == backend_x11->atom_NET_WM_PING &&
           xevent->xany.window == window)
    {
      XClientMessageEvent xclient = xevent->xclient;

      xclient.window = backend_x11->xwin_root;
      XSendEvent (backend_x11->xdpy, xclient.window,
                  False,
                  SubstructureRedirectMask | SubstructureNotifyMask,
                  (XEvent *) &xclient);
      return FALSE;
    }

  /* do not send any of the WM_PROTOCOLS events to the queue */
  return FALSE;
}

static gboolean
handle_xembed_event (ClutterBackendX11 *backend_x11,
                     XEvent            *xevent)
{
  ClutterActor *stage;

  stage = clutter_stage_get_default ();

  switch (xevent->xclient.data.l[1])
    {
    case XEMBED_EMBEDDED_NOTIFY:
      CLUTTER_NOTE (EVENT, "got XEMBED_EMBEDDED_NOTIFY from %lx",
                    xevent->xclient.data.l[3]);

      ParentEmbedderWin = xevent->xclient.data.l[3];

      clutter_actor_realize (stage);
      clutter_actor_show (stage);

      xembed_set_info (backend_x11,
                       clutter_x11_get_stage_window (CLUTTER_STAGE (stage)),
                       XEMBED_MAPPED);
      break;
    case XEMBED_WINDOW_ACTIVATE:
      CLUTTER_NOTE (EVENT, "got XEMBED_WINDOW_ACTIVATE");
      break;
    case XEMBED_WINDOW_DEACTIVATE:
      CLUTTER_NOTE (EVENT, "got XEMBED_WINDOW_DEACTIVATE");
      break;
    case XEMBED_FOCUS_IN:
      CLUTTER_NOTE (EVENT, "got XEMBED_FOCUS_IN");
      if (ParentEmbedderWin)
        xembed_send_message (backend_x11, ParentEmbedderWin,
                             XEMBED_FOCUS_NEXT,
                             0, 0, 0);
      break;
    default:
      CLUTTER_NOTE (EVENT, "got unknown XEMBED message");
      break;
    }

  /* do not propagate the XEMBED events to the stage */
  return FALSE;
}

static gboolean
event_translate (ClutterBackend *backend,
                 ClutterEvent   *event,
                 XEvent         *xevent)
{
  ClutterBackendX11  *backend_x11;
  ClutterStageX11    *stage_x11;
  ClutterStage       *stage;
  ClutterStageWindow *impl;
  gboolean            res, not_yet_handled = FALSE;
  Window              xwindow, stage_xwindow;

  backend_x11    = CLUTTER_BACKEND_X11 (backend);

  xwindow = xevent->xany.window;

  if (backend_x11->event_filters)
    {
      GSList                *node;
      ClutterX11EventFilter *filter;

      node = backend_x11->event_filters;

      while (node)
        {
          filter = node->data;

          switch (filter->func (xevent, event, filter->data))
            {
            case CLUTTER_X11_FILTER_CONTINUE:
              break;
            case CLUTTER_X11_FILTER_TRANSLATE:
              return TRUE;
            case CLUTTER_X11_FILTER_REMOVE:
              return FALSE;
            default:
              break;
            }

          node = node->next;
        }
    }

  /*
   * Do further processing only on events for the stage window
   * (the x11 filters might be getting events for other windows, so do not
   * mess them about.
   */
  stage = clutter_x11_get_stage_from_window (xwindow);

  if (stage == NULL)
      return FALSE;

  impl           = _clutter_stage_get_window (stage);
  stage_x11      = CLUTTER_STAGE_X11 (impl);
  stage_xwindow  = xwindow; /* clutter_x11_get_stage_window (stage); */

  event->any.stage = stage;


  res = TRUE;

  switch (xevent->type)
    {
    case ConfigureNotify:
      if (!stage_x11->is_foreign_xwin)
        {
	  /* Set a flag so that the stage will know the actor is being
	     resized in response to the window size changing as
	     opposed to a request from the application. This prevents
	     it from trying to resize the window again */
	  stage_x11->handling_configure = TRUE;

	  clutter_actor_set_size (CLUTTER_ACTOR (stage),
				  xevent->xconfigure.width,
				  xevent->xconfigure.height);

	  stage_x11->handling_configure = FALSE;
        }
      res = FALSE;
      break;

    case PropertyNotify:
      if (xevent->xproperty.atom == backend_x11->atom_NET_WM_STATE &&
          xevent->xproperty.window == stage_xwindow &&
          !stage_x11->is_foreign_xwin)
        {
          Atom     type;
          gint     format;
          gulong   n_items, bytes_after;
          guchar  *data = NULL;
          gboolean fullscreen_set = FALSE;

          clutter_x11_trap_x_errors ();
          XGetWindowProperty (backend_x11->xdpy, stage_xwindow,
                              backend_x11->atom_NET_WM_STATE,
                              0, G_MAXLONG,
                              False, XA_ATOM,
                              &type, &format, &n_items,
                              &bytes_after, &data);
          clutter_x11_untrap_x_errors ();

          if (type != None && data != NULL)
            {
              Atom *atoms = (Atom *) data;
              gulong i;
              gboolean is_fullscreen = FALSE;

              for (i = 0; i < n_items; i++)
                {
                  if (atoms[i] == backend_x11->atom_NET_WM_STATE_FULLSCREEN)
                    fullscreen_set = TRUE;
                }

              is_fullscreen =
                (stage_x11->state & CLUTTER_STAGE_STATE_FULLSCREEN);

              if (fullscreen_set != is_fullscreen)
                {
                  if (fullscreen_set)
                    stage_x11->state |= CLUTTER_STAGE_STATE_FULLSCREEN;
                  else
                    stage_x11->state &= ~CLUTTER_STAGE_STATE_FULLSCREEN;

                  event->type = CLUTTER_STAGE_STATE;
                  event->stage_state.changed_mask =
                    CLUTTER_STAGE_STATE_FULLSCREEN;
                  event->stage_state.new_state = stage_x11->state;
                }
              else
                res = FALSE;

              XFree (data);
            }
          else
            res = FALSE;
        }
      else
        res = FALSE;
      break;

    case MapNotify:
      clutter_stage_x11_map (stage_x11);
      res = FALSE;
      break;

    case UnmapNotify:
      clutter_stage_x11_unmap (stage_x11);
      res = FALSE;
      break;

    case FocusIn:
      if (!(stage_x11->state & CLUTTER_STAGE_STATE_ACTIVATED))
        {
          /* TODO: check xevent->xfocus.detail ? */
          stage_x11->state |= CLUTTER_STAGE_STATE_ACTIVATED;

          event->type = CLUTTER_STAGE_STATE;
          event->stage_state.changed_mask = CLUTTER_STAGE_STATE_ACTIVATED;
          event->stage_state.new_state = stage_x11->state;
        }
      else
        res = FALSE;
      break;

    case FocusOut:
      if (stage_x11->state & CLUTTER_STAGE_STATE_ACTIVATED)
        {
          stage_x11->state &= ~CLUTTER_STAGE_STATE_ACTIVATED;

          event->type = CLUTTER_STAGE_STATE;
          event->stage_state.changed_mask = CLUTTER_STAGE_STATE_ACTIVATED;
          event->stage_state.new_state = stage_x11->state;
        }
      else
        res = FALSE;
      break;

    case Expose:
      {
        CLUTTER_NOTE (MULTISTAGE, "expose for stage: %p, redrawing", stage);
        clutter_actor_queue_redraw (CLUTTER_ACTOR (stage));
        res = FALSE;
      }
      break;
    case DestroyNotify:
      CLUTTER_NOTE (EVENT, "destroy notify:\txid: %ld",
                    xevent->xdestroywindow.window);
      if (xevent->xdestroywindow.window == stage_xwindow &&
          !stage_x11->is_foreign_xwin)
        event->type = event->any.type = CLUTTER_DESTROY_NOTIFY;
      else
        res = FALSE;
      break;

    case ClientMessage:
      CLUTTER_NOTE (EVENT, "client message");

      event->type = event->any.type = CLUTTER_CLIENT_MESSAGE;

      if (xevent->xclient.message_type == backend_x11->atom_XEMBED)
        res = handle_xembed_event (backend_x11, xevent);
      else if (xevent->xclient.message_type == backend_x11->atom_WM_PROTOCOLS)
        {
          res = handle_wm_protocols_event (backend_x11, stage_xwindow, xevent);
          event->type = event->any.type = CLUTTER_DELETE;
        }
      break;

    case KeyPress:
      event->key.type = event->type = CLUTTER_KEY_PRESS;
      translate_key_event (backend, event, xevent);

      set_user_time (backend_x11, &xwindow, xevent->xkey.time);
      break;
              
    case KeyRelease:
      event->key.type = event->type = CLUTTER_KEY_RELEASE;
      translate_key_event (backend, event, xevent);
      break;

    default:
      /* ignore every other event */
      not_yet_handled = TRUE;
      break;
    }

  /* Input device event handling.. */
  if (not_yet_handled)
    {
      if (!clutter_x11_has_xinput ())
        {
          /* Regular X event */
          switch (xevent->type)
            {
			/* KeyPress / KeyRelease should reside here if XInput
             * worked properly
             */
            case ButtonPress:
              switch (xevent->xbutton.button)
                {
                case 4: /* up */
                case 5: /* down */
                case 6: /* left */
                case 7: /* right */
                  event->scroll.type = event->type = CLUTTER_SCROLL;
                  
                  if (xevent->xbutton.button == 4)
                    event->scroll.direction = CLUTTER_SCROLL_UP;
                  else if (xevent->xbutton.button == 5)
                    event->scroll.direction = CLUTTER_SCROLL_DOWN;
                  else if (xevent->xbutton.button == 6)
                    event->scroll.direction = CLUTTER_SCROLL_LEFT;
                  else
                    event->scroll.direction = CLUTTER_SCROLL_RIGHT;
                  
                  event->scroll.time = xevent->xbutton.time;
                  event->scroll.x = xevent->xbutton.x;
                  event->scroll.y = xevent->xbutton.y;
                  event->scroll.modifier_state = xevent->xbutton.state;
                  
                  break;
                default:
                  event->button.type = event->type = CLUTTER_BUTTON_PRESS;
                  event->button.time = xevent->xbutton.time;
                  event->button.x = xevent->xbutton.x;
                  event->button.y = xevent->xbutton.y;
                  event->button.modifier_state = xevent->xbutton.state;
                  event->button.button = xevent->xbutton.button;
                  
                  break;
                }
              
              set_user_time (backend_x11, &xwindow, event->button.time);
              break;
              
            case ButtonRelease:
              /* scroll events don't have a corresponding release */
              if (xevent->xbutton.button == 4 ||
                  xevent->xbutton.button == 5 ||
                  xevent->xbutton.button == 6 ||
                  xevent->xbutton.button == 7)
                {
                  res = FALSE;
                  break;
                }
              
              event->button.type = event->type = CLUTTER_BUTTON_RELEASE;
              event->button.time = xevent->xbutton.time;
              event->button.x = xevent->xbutton.x;
              event->button.y = xevent->xbutton.y;
              event->button.modifier_state = xevent->xbutton.state;
              event->button.button = xevent->xbutton.button;
              break;
              
            case MotionNotify:
              event->motion.type = event->type = CLUTTER_MOTION;
              event->motion.time = xevent->xmotion.time;
              event->motion.x = xevent->xmotion.x;
              event->motion.y = xevent->xmotion.y;
              event->motion.modifier_state = xevent->xmotion.state;
              break;
            default:
              /* ignore every other event */
              res = FALSE;
              break;
            }
        }
      else
        {  /* XInput fun.. Needs clean up. */
#ifdef USE_XINPUT
          int *ev_types = backend_x11->event_types;

          CLUTTER_NOTE (EVENT, "XInput event type: %d", xevent->type);

          if (xevent->type == ev_types [CLUTTER_X11_XINPUT_BUTTON_PRESS_EVENT])
            {
              XDeviceButtonEvent *xbev = (XDeviceButtonEvent *)xevent;

              switch (xbev->button)
                {
                case 4:
                case 5:
                case 6:
                case 7:
                  event->scroll.type = event->type = CLUTTER_SCROLL;
                  
                  if (xbev->button == 4)
                    event->scroll.direction = CLUTTER_SCROLL_UP;
                  else if (xbev->button == 5)
                    event->scroll.direction = CLUTTER_SCROLL_DOWN;
                  else if (xbev->button == 6)
                    event->scroll.direction = CLUTTER_SCROLL_LEFT;
                  else
                    event->scroll.direction = CLUTTER_SCROLL_RIGHT;
                  
                  event->scroll.time = xbev->time;
                  event->scroll.x = xbev->x;
                  event->scroll.y = xbev->y;
                  event->scroll.modifier_state = xbev->state;
                  event->scroll.device 
                    = (ClutterInputDevice *)_clutter_x11_get_device_for_xid (xbev->deviceid);
                  break;
                default:
                  event->button.type = event->type = CLUTTER_BUTTON_PRESS;
                  event->button.time = xbev->time;
                  event->button.x = xbev->x;
                  event->button.y = xbev->y;
                  event->button.modifier_state = xbev->state;
                  event->button.button = xbev->button;
                  event->button.device 
                    = (ClutterInputDevice *)_clutter_x11_get_device_for_xid (xbev->deviceid);
                  break;
                }

              set_user_time (backend_x11, &xwindow, xbev->time);

              CLUTTER_NOTE(EVENT, "XINPUT Button press event for %li %d %d", 
                           xbev->deviceid, xbev->x, xbev->y);
            } 
          else if (xevent->type 
                        == ev_types[CLUTTER_X11_XINPUT_BUTTON_RELEASE_EVENT])
            {
              XDeviceButtonEvent *xbev = (XDeviceButtonEvent *)xevent;

              /* scroll events don't have a corresponding release */
              if (xbev->button == 4 ||
                  xbev->button == 5 ||
                  xbev->button == 6 ||
                  xbev->button == 7)
                {
                  res = FALSE;
                  goto out;
                }

              event->button.type = event->type = CLUTTER_BUTTON_RELEASE;
              event->button.time = xbev->time;
              event->button.x = xbev->x;
              event->button.y = xbev->y;
              event->button.modifier_state = xbev->state;
              event->button.button = xbev->button;
              event->button.device 
                    = (ClutterInputDevice *)_clutter_x11_get_device_for_xid (xbev->deviceid);
              CLUTTER_NOTE(EVENT, "XINPUT Button release event for %li %d %d", 
                           xbev->deviceid, xbev->x, xbev->y);
            } 
          else if (xevent->type 
                       == ev_types [CLUTTER_X11_XINPUT_MOTION_NOTIFY_EVENT]) 
            {
              XDeviceMotionEvent *xmev = (XDeviceMotionEvent *)xevent;

              event->motion.type = event->type = CLUTTER_MOTION;
              event->motion.time = xmev->time;
              event->motion.x = xmev->x;
              event->motion.y = xmev->y;
              event->motion.modifier_state = xmev->state;
              event->motion.device 
                    = (ClutterInputDevice *) _clutter_x11_get_device_for_xid (xmev->deviceid);
              CLUTTER_NOTE(EVENT, "XINPUT Motion event for %li %d %d", 
                           xmev->deviceid, 
                           xmev->x, 
                           xmev->y);
            } 
#if 0
        /* the Xinput handling of key presses/releases disabled for now since
         * it makes keyrepeat, and key presses and releases outside the window
         * not generate events even when the window has focus
         */

          else if (xevent->type 
                        == ev_types [CLUTTER_X11_XINPUT_KEY_PRESS_EVENT]) 
            {
              XEvent xevent_converted;
              XDeviceKeyEvent *xkev = (XDeviceKeyEvent *)xevent;
              
              convert_xdevicekey_to_xkey (xkev, &xevent_converted);

              event->key.type = event->type = CLUTTER_KEY_PRESS;
              translate_key_event (backend, event, &xevent_converted);

              set_user_time (backend_x11, &xwindow, xkev->time);
            } 
          else if (xevent->type 
                   == ev_types [CLUTTER_X11_XINPUT_KEY_RELEASE_EVENT]) 
            {
              XEvent xevent_converted;
              XDeviceKeyEvent *xkev = (XDeviceKeyEvent *)xevent;
              
              convert_xdevicekey_to_xkey (xkev, &xevent_converted);

              event->key.type = event->type = CLUTTER_KEY_RELEASE;
              translate_key_event (backend, event, &xevent_converted);
            }
#endif
          else 
#endif
            {
              CLUTTER_NOTE (EVENT, "Uknown Event");
              res = FALSE;
            }
        }
    }

 out:  
  return res;
}

static void
events_queue (ClutterBackend *backend)
{
  ClutterBackendX11 *backend_x11 = CLUTTER_BACKEND_X11 (backend);
  ClutterEvent      *event;
  Display           *xdisplay = backend_x11->xdpy;
  XEvent             xevent;
  ClutterMainContext  *clutter_context;

  clutter_context = clutter_context_get_default ();

  while (!clutter_events_pending () && XPending (xdisplay))
    {
      XNextEvent (xdisplay, &xevent);

      event = clutter_event_new (CLUTTER_NOTHING);

      if (event_translate (backend, event, &xevent))
        {
	  /* push directly here to avoid copy of queue_put */
	  g_queue_push_head (clutter_context->events_queue, event);
        }
      else
        {
          clutter_event_free (event);
        }
    }
}

/**
 * clutter_x11_handle_event:
 * @xevent: pointer to XEvent structure
 *
 * This function processes a single X event; it can be used to hook
 * into external X event retrieval (for example that done by GDK).
 *
 * Return value: #ClutterX11FilterReturn indicating what the caller
 *   should do with the original event.
 *
 * Since:  0.8
 */
ClutterX11FilterReturn
clutter_x11_handle_event (XEvent *xevent)
{
  ClutterBackend      *backend;
  ClutterEvent        *event;
  ClutterMainContext  *clutter_context;

  clutter_threads_enter ();

  clutter_context = clutter_context_get_default ();
  backend = clutter_context->backend;

  event = clutter_event_new (CLUTTER_NOTHING);

  if (event_translate (backend, event, xevent))
    {
      /* push directly here to avoid copy of queue_put */
      g_queue_push_head (clutter_context->events_queue, event);
    }
  else
    {
      clutter_event_free (event);
    }

  event = clutter_event_get ();

  if (event)
    {
      /* forward the event into clutter for emission etc. */
      clutter_do_event (event);
      clutter_event_free (event);
    }

  clutter_threads_leave ();

  return CLUTTER_X11_FILTER_CONTINUE;
}

static gboolean
clutter_event_prepare (GSource *source,
                       gint    *timeout)
{
  ClutterBackend *backend = ((ClutterEventSource *) source)->backend;
  gboolean retval;

  clutter_threads_enter ();

  *timeout = -1;
  retval = (clutter_events_pending () || check_xpending (backend));

  clutter_threads_leave ();

  return retval;
}

static gboolean
clutter_event_check (GSource *source)
{
  ClutterEventSource *event_source = (ClutterEventSource *) source;
  ClutterBackend *backend = event_source->backend;
  gboolean retval;

  clutter_threads_enter ();

  if (event_source->event_poll_fd.revents & G_IO_IN)
    retval = (clutter_events_pending () || check_xpending (backend));
  else
    retval = FALSE;

  clutter_threads_leave ();

  return retval;
}

static gboolean
clutter_event_dispatch (GSource     *source,
                        GSourceFunc  callback,
                        gpointer     user_data)
{
  ClutterBackend *backend = ((ClutterEventSource *) source)->backend;
  ClutterEvent *event;

  clutter_threads_enter ();

  /*  Grab the event(s), translate and figure out double click.
   *  The push onto queue (stack) if valid.
  */
  events_queue (backend);

  /* Pop an event off the queue if any */
  event = clutter_event_get ();

  if (event)
    {
      /* forward the event into clutter for emission etc. */
      clutter_do_event (event);
      clutter_event_free (event);
    }

  clutter_threads_leave ();

  return TRUE;
}
