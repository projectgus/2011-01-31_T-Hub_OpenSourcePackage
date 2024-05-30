/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006 OpenedHand
 *
 * Modified by Sagemcom under LGPL license on 15/07/2008
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
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
/**
 * SECTION:clutter-main
 * @short_description: Various 'global' clutter functions.
 *
 * Functions to retrieve various global Clutter resources and other utility
 * functions for mainloops, events and threads
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <glib/gi18n-lib.h>
#include <locale.h>

#include "clutter-event.h"
#include "clutter-backend.h"
#include "clutter-main.h"
#include "clutter-feature.h"
#include "clutter-actor.h"
#include "clutter-stage.h"
#include "clutter-private.h"
#include "clutter-debug.h"
#include "clutter-version.h" 	/* For flavour define */
#include "clutter-frame-source.h"

#include "cogl/cogl.h"

/* main context */
static ClutterMainContext *ClutterCntx  = NULL;

/* main lock and locking/unlocking functions */
static GMutex *clutter_threads_mutex    = NULL;
static GCallback clutter_threads_lock   = NULL;
static GCallback clutter_threads_unlock = NULL;

static gboolean clutter_is_initialized  = FALSE;
static gboolean clutter_show_fps        = FALSE;
static gboolean clutter_fatal_warnings  = FALSE;

static guint clutter_default_fps        = 60;

static guint clutter_main_loop_level    = 0;
static GSList *main_loops               = NULL;

guint clutter_debug_flags = 0;  /* global clutter debug flag */

#ifdef CLUTTER_ENABLE_DEBUG
static const GDebugKey clutter_debug_keys[] = {
  { "misc", CLUTTER_DEBUG_MISC },
  { "actor", CLUTTER_DEBUG_ACTOR },
  { "texture", CLUTTER_DEBUG_TEXTURE },
  { "event", CLUTTER_DEBUG_EVENT },
  { "paint", CLUTTER_DEBUG_PAINT },
  { "gl", CLUTTER_DEBUG_GL },
  { "alpha", CLUTTER_DEBUG_ALPHA },
  { "behaviour", CLUTTER_DEBUG_BEHAVIOUR },
  { "pango", CLUTTER_DEBUG_PANGO },
  { "backend", CLUTTER_DEBUG_BACKEND },
  { "scheduler", CLUTTER_DEBUG_SCHEDULER },
  { "script", CLUTTER_DEBUG_SCRIPT },
  { "shader", CLUTTER_DEBUG_SHADER },
  { "multistage", CLUTTER_DEBUG_MULTISTAGE },
};
#endif /* CLUTTER_ENABLE_DEBUG */

/**
 * clutter_get_show_fps:
 *
 * Returns whether Clutter should print out the frames per second on the
 * console. You can enable this setting either using the
 * <literal>CLUTTER_SHOW_FPS</literal> environment variable or passing
 * the <literal>--clutter-show-fps</literal> command line argument. *
 *
 * Return value: %TRUE if Clutter should show the FPS.
 *
 * Since: 0.4
 */
gboolean
clutter_get_show_fps (void)
{
  return clutter_show_fps;
}

void
_clutter_stage_maybe_relayout (ClutterActor *stage)
{
  ClutterUnit natural_width, natural_height;
  ClutterActorBox box = { 0, };

  /* avoid reentrancy */
  if (!(CLUTTER_PRIVATE_FLAGS (stage) & CLUTTER_ACTOR_IN_RELAYOUT))
    {
      CLUTTER_NOTE (ACTOR, "Recomputing layout");

      CLUTTER_SET_PRIVATE_FLAGS (stage, CLUTTER_ACTOR_IN_RELAYOUT);

      natural_width = natural_height = 0;
      clutter_actor_get_preferred_size (stage,
                                        NULL, NULL,
                                        &natural_width, &natural_height);

      box.x1 = 0;
      box.y1 = 0;
      box.x2 = natural_width;
      box.y2 = natural_height;

      CLUTTER_NOTE (ACTOR, "Allocating (0, 0 - %d, %d) for the stage",
                    CLUTTER_UNITS_TO_DEVICE (natural_width),
                    CLUTTER_UNITS_TO_DEVICE (natural_height));

      clutter_actor_allocate (stage, &box, FALSE);

      CLUTTER_UNSET_PRIVATE_FLAGS (stage, CLUTTER_ACTOR_IN_RELAYOUT);
    }
}

void
_clutter_stage_maybe_setup_viewport (ClutterStage *stage)
{
  if (CLUTTER_PRIVATE_FLAGS (stage) & CLUTTER_ACTOR_SYNC_MATRICES)
    {
      ClutterPerspective perspective;
      guint width, height;

      clutter_actor_get_size (CLUTTER_ACTOR (stage), &width, &height);
      clutter_stage_get_perspectivex (stage, &perspective);

      CLUTTER_NOTE (PAINT, "Setting up the viewport");

      cogl_setup_viewport (width, height,
			   perspective.fovy,
			   perspective.aspect,
			   perspective.z_near,
			   perspective.z_far);

      CLUTTER_UNSET_PRIVATE_FLAGS (stage, CLUTTER_ACTOR_SYNC_MATRICES);
    }
}

/**
 * clutter_redraw:
 *
 * Forces a redraw of the entire stage. Applications should never use this
 * function, but queue a redraw using clutter_actor_queue_redraw().
 */
void
clutter_redraw (ClutterStage *stage)
{
  ClutterMainContext *ctx;
  static GTimer      *timer = NULL;
  static guint        timer_n_frames = 0;

  ctx  = clutter_context_get_default ();

  CLUTTER_TIMESTAMP (SCHEDULER, "Redraw start for stage:%p", stage);
  CLUTTER_NOTE (PAINT, " Redraw enter for stage:%p", stage);
  CLUTTER_NOTE (MULTISTAGE, "Redraw called for stage:%p", stage);

  /* Before we can paint, we have to be sure we have the latest layout */
  _clutter_stage_maybe_relayout (CLUTTER_ACTOR (stage));

  _clutter_backend_ensure_context (ctx->backend, stage);

  /* Setup FPS count - not currently across *all* stages rather than per */
  if (G_UNLIKELY (clutter_get_show_fps ()))
    {
      if (!timer)
	timer = g_timer_new ();
    }

  /* The code below can't go in stage paint as base actor_paint
   * will get called before it (and break picking, etc)
   */
  _clutter_stage_maybe_setup_viewport (stage);

  /* Call through to the actual backend to do the painting down from
   * the stage. It will likely need to swap buffers, vblank sync etc
   * which will be windowing system dependant.
  */
  _clutter_backend_redraw (ctx->backend, stage);

  /* Complete FPS info */
  if (G_UNLIKELY (clutter_get_show_fps ()))
    {
      timer_n_frames++;

      if (g_timer_elapsed (timer, NULL) >= 1.0)
	{
	  g_print ("*** FPS: %i ***\n", timer_n_frames);
	  timer_n_frames = 0;
	  g_timer_start (timer);
	}
    }

  CLUTTER_NOTE (PAINT, " Redraw leave for stage:%p", stage);
  CLUTTER_TIMESTAMP (SCHEDULER, "Redraw finish for stage:%p", stage);
}

/**
 * clutter_set_motion_events_enabled:
 * @enable: %TRUE to enable per-actor motion events
 *
 * Sets whether per-actor motion events should be enabled or not (the
 * default is to enable them).
 *
 * If @enable is %FALSE the following events will not work:
 * <itemizedlist>
 *   <listitem><para>ClutterActor::motion-event, unless on the
 *     #ClutterStage</para></listitem>
 *   <listitem><para>ClutterActor::enter-event</para></listitem>
 *   <listitem><para>ClutterActor::leave-event</para></listitem>
 * </itemizedlist>
 *
 * Since: 0.6
 */
void
clutter_set_motion_events_enabled (gboolean enable)
{
  ClutterMainContext *context = clutter_context_get_default ();

  context->motion_events_per_actor = enable;
}

/**
 * clutter_get_motion_events_enabled:
 *
 * Gets whether the per-actor motion events are enabled.
 *
 * Return value: %TRUE if the motion events are enabled
 *
 * Since: 0.6
 */
gboolean
clutter_get_motion_events_enabled (void)
{
  ClutterMainContext *context = clutter_context_get_default ();

  return context->motion_events_per_actor;
}

guint _clutter_pix_to_id (guchar pixel[4]);

static inline void init_bits (void)
{
  ClutterMainContext *ctx;

  static gboolean done = FALSE;
  if (G_LIKELY (done))
    return;

  ctx = clutter_context_get_default ();

  done = TRUE;
}

void
_clutter_id_to_color (guint id, ClutterColor *col)
{
  ClutterMainContext *ctx;
  gint                red, green, blue;
  ctx = clutter_context_get_default ();

  /* compute the numbers we'll store in the components */
  red   = (id >> (ctx->fb_g_mask_used+ctx->fb_b_mask_used)) 
                & (0xff >> (8-ctx->fb_r_mask_used));
  green = (id >> ctx->fb_b_mask_used) & (0xff >> (8-ctx->fb_g_mask_used));
  blue  = (id)  & (0xff >> (8-ctx->fb_b_mask_used));

  /* shift left bits a bit and add one, this circumvents
   * at least some potential rounding errors in GL/GLES
   * driver / hw implementation. 
   */
  if (ctx->fb_r_mask_used != ctx->fb_r_mask)
    red = red * 2 + 1;
  if (ctx->fb_g_mask_used != ctx->fb_g_mask)
    green = green * 2 + 1;
  if (ctx->fb_b_mask_used != ctx->fb_b_mask)
    blue  = blue  * 2 + 1;

  /* shift up to be full 8bit values */ 
  red   = red   << (8 - ctx->fb_r_mask);
  green = green << (8 - ctx->fb_g_mask);
  blue  = blue  << (8 - ctx->fb_b_mask);

  col->red   = red;
  col->green = green;
  col->blue  = blue;
  col->alpha = 0xff;
}

guint 
_clutter_pixel_to_id (guchar pixel[4])                 
{
  ClutterMainContext *ctx;
  gint  red, green, blue;
  guint id;

  ctx = clutter_context_get_default ();

  /* reduce the pixel components to the number of bits actually used of the
   * 8bits.
   */
  red   = pixel[0] >> (8 - ctx->fb_r_mask);
  green = pixel[1] >> (8 - ctx->fb_g_mask);
  blue  = pixel[2] >> (8 - ctx->fb_b_mask);

  /* divide potentially by two if 'fuzzy' */
  red   = red   >> (ctx->fb_r_mask - ctx->fb_r_mask_used);
  green = green >> (ctx->fb_g_mask - ctx->fb_g_mask_used);
  blue  = blue  >> (ctx->fb_b_mask - ctx->fb_b_mask_used);  

  /* combine the correct per component values into the final id */
  id =  blue + (green <<  ctx->fb_b_mask_used) 
          + (red << (ctx->fb_b_mask_used + ctx->fb_g_mask_used));
  
  return id;
}

ClutterActor *
_clutter_do_pick (ClutterStage   *stage,
		  gint            x,
		  gint            y,
		  ClutterPickMode mode)
{
  ClutterMainContext *context;
  guchar              pixel[4];
  GLint               viewport[4];
  ClutterColor        white = { 0xff, 0xff, 0xff, 0xff };
  guint32             id;
  
  context = clutter_context_get_default ();

  _clutter_backend_ensure_context (context->backend, stage);

  /* needed for when a context switch happens */
  _clutter_stage_maybe_setup_viewport (stage);

  cogl_paint_init (&white);

  /* Disable dithering (if any) when doing the painting in pick mode */
  glDisable (GL_DITHER);

  /* Render the entire scence in pick mode - just single colored silhouette's
   * are drawn offscreen (as we never swap buffers)
  */
  context->pick_mode = mode;
  clutter_actor_paint (CLUTTER_ACTOR (stage));
  context->pick_mode = CLUTTER_PICK_NONE;

  /* Calls should work under both GL and GLES, note GLES needs RGBA */
  glGetIntegerv(GL_VIEWPORT, viewport);

  /* Below to be safe, particularly on GL ES. an EGL wait call or full
   * could be nicer.
  */
  glFinish();

  /* glEnable (GL_DITHER); we never enabled this originally, so its
     probably not safe to then enable it */

  /* Read the color of the screen co-ords pixel */
  glReadPixels (x, viewport[3] - y -1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

  if (pixel[0] == 0xff && pixel[1] == 0xff && pixel[2] == 0xff)
    return CLUTTER_ACTOR (stage);

  id = _clutter_pixel_to_id (pixel);

  return clutter_get_actor_by_gid (id);
}

static void
clutter_context_free (ClutterMainContext *context)
{
  /* this will take care of destroying the stage */
  g_object_unref (context->backend);
  context->backend = NULL;

  clutter_id_pool_free (context->id_pool);
  context->id_pool = NULL;

  g_object_unref (context->font_map);
  context->font_map = NULL;

  /* XXX: The cleaning up of the event queue should be moved here from
          the backend base class. */

  g_free (context);
}

PangoContext *
_clutter_context_create_pango_context (ClutterMainContext *self)
{
  PangoContext *context;
  gdouble resolution;
  cairo_font_options_t *font_options;

  resolution = clutter_backend_get_resolution (self->backend);
  if (resolution < 0)
    resolution = 96.0; /* fall back */

  context = pango_clutter_font_map_create_context (self->font_map);

  pango_cairo_context_set_resolution (context, resolution);

  font_options = clutter_backend_get_font_options (self->backend);
  pango_cairo_context_set_font_options (context, font_options);

  return context;
}

/**
 * clutter_main_quit:
 *
 * Terminates the Clutter mainloop.
 */
void
clutter_main_quit (void)
{
  g_return_if_fail (main_loops != NULL);

  g_main_loop_quit (main_loops->data);
}

/**
 * clutter_main_level:
 *
 * Retrieves the depth of the Clutter mainloop.
 *
 * Return value: The level of the mainloop.
 */
gint
clutter_main_level (void)
{
  return clutter_main_loop_level;
}

/**
 * clutter_main:
 *
 * Starts the Clutter mainloop.
 */
void
clutter_main (void)
{
  ClutterMainContext *context = CLUTTER_CONTEXT ();
  GMainLoop *loop;

  if (!clutter_is_initialized)
    {
      g_warning ("Called clutter_main() but Clutter wasn't initialised.  "
		 "You must call clutter_init() first.");
      return;
    }

  CLUTTER_MARK ();

  clutter_main_loop_level++;

  loop = g_main_loop_new (NULL, TRUE);
  main_loops = g_slist_prepend (main_loops, loop);

#ifdef HAVE_CLUTTER_FRUITY
  /* clutter fruity creates an application that forwards events and manually
   * spins the mainloop
   */
  clutter_fruity_main ();
#else
  if (g_main_loop_is_running (main_loops->data))
    {
      clutter_threads_leave ();
      g_main_loop_run (loop);
      clutter_threads_enter ();
    }
#endif

  main_loops = g_slist_remove (main_loops, loop);

  g_main_loop_unref (loop);

  clutter_main_loop_level--;

  if (clutter_main_loop_level == 0)
    {
      clutter_context_free (context);
    }

  CLUTTER_MARK ();
}

static void
clutter_threads_impl_lock (void)
{
  if (clutter_threads_mutex)
    g_mutex_lock (clutter_threads_mutex);
}

static void
clutter_threads_impl_unlock (void)
{
  if (clutter_threads_mutex)
    g_mutex_unlock (clutter_threads_mutex);
}

/**
 * clutter_threads_init:
 *
 * Initialises the Clutter threading mechanism, so that Clutter API can be
 * called by multiple threads, using clutter_threads_enter() and
 * clutter_threads_leave() to mark the critical sections.
 *
 * You must call g_thread_init() before this function.
 *
 * This function must be called before clutter_init().
 *
 * Since: 0.4
 */
void
clutter_threads_init (void)
{
  if (!g_thread_supported ())
    g_error ("g_thread_init() must be called before clutter_threads_init()");

  clutter_threads_mutex = g_mutex_new ();

  if (!clutter_threads_lock)
    clutter_threads_lock = clutter_threads_impl_lock;

  if (!clutter_threads_unlock)
    clutter_threads_unlock = clutter_threads_impl_unlock;
}

/**
 * clutter_threads_set_lock_functions:
 * @enter_fn: function called when aquiring the Clutter main lock
 * @leave_fn: function called when releasing the Clutter main lock
 *
 * Allows the application to replace the standard method that
 * Clutter uses to protect its data structures. Normally, Clutter
 * creates a single #GMutex that is locked by clutter_threads_enter(),
 * and released by clutter_threads_leave(); using this function an
 * application provides, instead, a function @enter_fn that is
 * called by clutter_threads_enter() and a function @leave_fn that is
 * called by clutter_threads_leave().
 *
 * The functions must provide at least same locking functionality
 * as the default implementation, but can also do extra application
 * specific processing.
 *
 * As an example, consider an application that has its own recursive
 * lock that when held, holds the Clutter lock as well. When Clutter
 * unlocks the Clutter lock when entering a recursive main loop, the
 * application must temporarily release its lock as well.
 *
 * Most threaded Clutter apps won't need to use this method.
 *
 * This method must be called before clutter_threads_init(), and cannot
 * be called multiple times.
 *
 * Since: 0.4
 */
void
clutter_threads_set_lock_functions (GCallback enter_fn,
                                    GCallback leave_fn)
{
  g_return_if_fail (clutter_threads_lock == NULL &&
                    clutter_threads_unlock == NULL);

  clutter_threads_lock = enter_fn;
  clutter_threads_unlock = leave_fn;
}

typedef struct
{
  GSourceFunc func;
  gpointer data;
  GDestroyNotify notify;
} ClutterThreadsDispatch;

static gboolean
clutter_threads_dispatch (gpointer data)
{
  ClutterThreadsDispatch *dispatch = data;
  gboolean ret = FALSE;

  clutter_threads_enter ();

  if (!g_source_is_destroyed (g_main_current_source ()))
    ret = dispatch->func (dispatch->data);

  clutter_threads_leave ();

  return ret;
}

static void
clutter_threads_dispatch_free (gpointer data)
{
  ClutterThreadsDispatch *dispatch = data;

  /* XXX - we cannot hold the thread lock here because the main loop
   * might destroy a source while still in the dispatcher function; so
   * knowing whether the lock is being held or not is not known a priori.
   *
   * see bug: http://bugzilla.gnome.org/show_bug.cgi?id=459555
   */
  if (dispatch->notify)
    dispatch->notify (dispatch->data);

  g_slice_free (ClutterThreadsDispatch, dispatch);
}

/**
 * clutter_threads_add_idle_full:
 * @priority: the priority of the timeout source. Typically this will be in the
 *    range between #G_PRIORITY_DEFAULT_IDLE and #G_PRIORITY_HIGH_IDLE
 * @func: function to call
 * @data: data to pass to the function
 * @notify: functio to call when the idle source is removed
 *
 * Adds a function to be called whenever there are no higher priority
 * events pending.  If the function returns %FALSE it is automatically
 * removed from the list of event sources and will not be called again.
 *
 * This variant of g_idle_add_full() calls @function with the Clutter lock
 * held. It can be thought of a MT-safe version for Clutter actors for the
 * following use case, where you have to worry about idle_callback()
 * running in thread A and accessing @self after it has been finalized
 * in thread B:
 *
 * <informalexample><programlisting>
 * static gboolean
 * idle_callback (gpointer data)
 * {
 *    // clutter_threads_enter(); would be needed for g_idle_add()
 *
 *    SomeActor *self = data;
 *    /<!-- -->* do stuff with self *<!-- -->/
 *
 *    self->idle_id = 0;
 *
 *    // clutter_threads_leave(); would be needed for g_idle_add()
 *    return FALSE;
 * }
 * static void
 * some_actor_do_stuff_later (SomeActor *self)
 * {
 *    self->idle_id = clutter_threads_add_idle (idle_callback, self)
 *    // using g_idle_add() here would require thread protection in the callback
 * }
 *
 * static void
 * some_actor_finalize (GObject *object)
 * {
 *    SomeActor *self = SOME_ACTOR (object);
 *    if (self->idle_id)
 *      g_source_remove (self->idle_id);
 *    G_OBJECT_CLASS (parent_class)->finalize (object);
 * }
 * </programlisting></informalexample>
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.4
 */
guint
clutter_threads_add_idle_full (gint           priority,
                               GSourceFunc    func,
                               gpointer       data,
                               GDestroyNotify notify)
{
  ClutterThreadsDispatch *dispatch;

  g_return_val_if_fail (func != NULL, 0);

  dispatch = g_slice_new (ClutterThreadsDispatch);
  dispatch->func = func;
  dispatch->data = data;
  dispatch->notify = notify;

  return g_idle_add_full (priority,
                          clutter_threads_dispatch, dispatch,
                          clutter_threads_dispatch_free);
}

/**
 * clutter_threads_add_idle:
 * @func: function to call
 * @data: data to pass to the function
 *
 * Simple wrapper around clutter_threads_add_idle_full()
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.4
 */
guint
clutter_threads_add_idle (GSourceFunc func,
                          gpointer    data)
{
  g_return_val_if_fail (func != NULL, 0);

  return clutter_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
                                        func, data,
                                        NULL);
}

/**
 * clutter_threads_add_timeout_full:
 * @priority: the priority of the timeout source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT and #G_PRIORITY_HIGH.
 * @interval: the time between calls to the function, in milliseconds
 * @func: function to call
 * @data: data to pass to the function
 * @notify: function to call when the timeout source is removed
 *
 * Sets a function to be called at regular intervals holding the Clutter lock,
 * with the given priority.  The function is called repeatedly until it
 * returns %FALSE, at which point the timeout is automatically destroyed
 * and the function will not be called again.  The @notify function is
 * called when the timeout is destroyed.  The first call to the
 * function will be at the end of the first @interval.
 *
 * Note that timeout functions may be delayed, due to the processing of other
 * event sources. Thus they should not be relied on for precise timing.
 * After each call to the timeout function, the time of the next
 * timeout is recalculated based on the current time and the given interval
 * (it does not try to 'catch up' time lost in delays).
 *
 * This variant of g_timeout_add_full() can be thought of a MT-safe version
 * for Clutter actors. See also clutter_threads_add_idle_full().
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.4
 */
guint
clutter_threads_add_timeout_full (gint           priority,
                                  guint          interval,
                                  GSourceFunc    func,
                                  gpointer       data,
                                  GDestroyNotify notify)
{
  ClutterThreadsDispatch *dispatch;

  g_return_val_if_fail (func != NULL, 0);

  dispatch = g_slice_new (ClutterThreadsDispatch);
  dispatch->func = func;
  dispatch->data = data;
  dispatch->notify = notify;

  return g_timeout_add_full (priority,
                             interval,
                             clutter_threads_dispatch, dispatch,
                             clutter_threads_dispatch_free);
}

/**
 * clutter_threads_add_timeout:
 * @interval: the time between calls to the function, in milliseconds
 * @func: function to call
 * @data: data to pass to the function
 *
 * Simple wrapper around clutter_threads_add_timeout_full().
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.4
 */
guint
clutter_threads_add_timeout (guint       interval,
                             GSourceFunc func,
                             gpointer    data)
{
  g_return_val_if_fail (func != NULL, 0);

  return clutter_threads_add_timeout_full (G_PRIORITY_DEFAULT,
                                           interval,
                                           func, data,
                                           NULL);
}

/**
 * clutter_threads_add_frame_source_full:
 * @priority: the priority of the frame source. Typically this will be in the
 *            range between #G_PRIORITY_DEFAULT and #G_PRIORITY_HIGH.
 * @interval: the time between calls to the function, in milliseconds
 * @func: function to call
 * @data: data to pass to the function
 * @notify: function to call when the timeout source is removed
 *
 * Sets a function to be called at regular intervals holding the Clutter lock,
 * with the given priority.  The function is called repeatedly until it
 * returns %FALSE, at which point the timeout is automatically destroyed
 * and the function will not be called again.  The @notify function is
 * called when the timeout is destroyed.  The first call to the
 * function will be at the end of the first @interval.
 *
 * This function is similar to clutter_threads_add_timeout_full()
 * except that it will try to compensate for delays. For example, if
 * @func takes half the interval time to execute then the function
 * will be called again half the interval time after it finished. In
 * contrast clutter_threads_add_timeout_full() would not fire until a
 * full interval after the function completes so the delay between
 * calls would be @interval * 1.5. This function does not however try
 * to invoke the function multiple times to catch up missing frames if
 * @func takes more than @interval ms to execute.
 *
 * This variant of clutter_frame_source_add_full() can be thought of a
 * MT-safe version for Clutter actors.

 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.8
 */
guint
clutter_threads_add_frame_source_full (gint           priority,
				       guint          interval,
				       GSourceFunc    func,
				       gpointer       data,
				       GDestroyNotify notify)
{
  ClutterThreadsDispatch *dispatch;

  g_return_val_if_fail (func != NULL, 0);

  dispatch = g_slice_new (ClutterThreadsDispatch);
  dispatch->func = func;
  dispatch->data = data;
  dispatch->notify = notify;

  return clutter_frame_source_add_full (priority,
					interval,
					clutter_threads_dispatch, dispatch,
					clutter_threads_dispatch_free);
}

/**
 * clutter_threads_add_frame_source:
 * @interval: the time between calls to the function, in milliseconds
 * @func: function to call
 * @data: data to pass to the function
 *
 * Simple wrapper around clutter_threads_add_frame_source_full().
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.8
 */
guint
clutter_threads_add_frame_source (guint       interval,
				  GSourceFunc func,
				  gpointer    data)
{
  g_return_val_if_fail (func != NULL, 0);

  return clutter_threads_add_frame_source_full (G_PRIORITY_DEFAULT,
						interval,
						func, data,
						NULL);
}

/**
 * clutter_threads_enter:
 *
 * Locks the Clutter thread lock.
 *
 * Since: 0.4
 */
void
clutter_threads_enter (void)
{
  if (clutter_threads_lock)
    (* clutter_threads_lock) ();
}

/**
 * clutter_threads_leave:
 *
 * Unlocks the Clutter thread lock.
 *
 * Since: 0.4
 */
void
clutter_threads_leave (void)
{
  if (clutter_threads_unlock)
    (* clutter_threads_unlock) ();
}


/**
 * clutter_get_debug_enabled:
 *
 * Check if clutter has debugging turned on.
 *
 * Return value: TRUE if debugging is turned on, FALSE otherwise.
 */
gboolean
clutter_get_debug_enabled (void)
{
#ifdef CLUTTER_ENABLE_DEBUG
  return clutter_debug_flags != 0;
#else
  return FALSE;
#endif
}

ClutterMainContext *
clutter_context_get_default (void)
{
  if (G_UNLIKELY(!ClutterCntx))
    {
      ClutterMainContext *ctx;
      gdouble resolution;

      ClutterCntx = ctx = g_new0 (ClutterMainContext, 1);
      ctx->backend = g_object_new (_clutter_backend_impl_get_type (), NULL);

      ctx->is_initialized = FALSE;
      ctx->motion_events_per_actor = TRUE;

#ifdef CLUTTER_ENABLE_DEBUG
      ctx->timer          =  g_timer_new ();
      g_timer_start (ctx->timer);
#endif

      ctx->font_map = PANGO_CLUTTER_FONT_MAP (pango_clutter_font_map_new ());

      resolution = clutter_backend_get_resolution (ctx->backend);
      pango_clutter_font_map_set_resolution (ctx->font_map, resolution);

      pango_clutter_font_map_set_use_mipmapping (ctx->font_map, TRUE);
    }

  return ClutterCntx;
}

/**
 * clutter_get_timestamp:
 *
 * Returns the approximate number of microseconds passed since clutter was
 * intialised.
 *
 * Return value: Number of microseconds since clutter_init() was called.
 */
gulong
clutter_get_timestamp (void)
{
#ifdef CLUTTER_ENABLE_DEBUG
  ClutterMainContext *ctx;
  gdouble seconds;

  ctx = clutter_context_get_default ();

  /* FIXME: may need a custom timer for embedded setups */
  seconds = g_timer_elapsed (ctx->timer, NULL);

  return (gulong)(seconds / 1.0e-6);
#else
  return 0;
#endif
}


#ifdef CLUTTER_ENABLE_DEBUG
static gboolean
clutter_arg_debug_cb (const char *key,
                      const char *value,
                      gpointer    user_data)
{
  clutter_debug_flags |=
    g_parse_debug_string (value,
                          clutter_debug_keys,
                          G_N_ELEMENTS (clutter_debug_keys));
  return TRUE;
}

static gboolean
clutter_arg_no_debug_cb (const char *key,
                         const char *value,
                         gpointer    user_data)
{
  clutter_debug_flags &=
    ~g_parse_debug_string (value,
                           clutter_debug_keys,
                           G_N_ELEMENTS (clutter_debug_keys));
  return TRUE;
}
#endif /* CLUTTER_ENABLE_DEBUG */

static GOptionEntry clutter_args[] = {
  { "clutter-show-fps", 0, 0, G_OPTION_ARG_NONE, &clutter_show_fps,
    "Show frames per second", NULL },
  { "clutter-default-fps", 0, 0, G_OPTION_ARG_INT, &clutter_default_fps,
    "Default frame rate", "FPS" },
  { "g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &clutter_fatal_warnings,
    "Make all warnings fatal", NULL },
#ifdef CLUTTER_ENABLE_DEBUG
  { "clutter-debug", 0, 0, G_OPTION_ARG_CALLBACK, clutter_arg_debug_cb,
    "Clutter debugging flags to set", "FLAGS" },
  { "clutter-no-debug", 0, 0, G_OPTION_ARG_CALLBACK, clutter_arg_no_debug_cb,
    "Clutter debugging flags to unset", "FLAGS" },
#endif /* CLUTTER_ENABLE_DEBUG */
  { NULL, },
};

/* pre_parse_hook: initialise variables depending on environment
 * variables; these variables might be overridden by the command
 * line arguments that are going to be parsed after.
 */
static gboolean
pre_parse_hook (GOptionContext  *context,
                GOptionGroup    *group,
                gpointer         data,
                GError         **error)
{
  ClutterMainContext *clutter_context;
  ClutterBackend *backend;
  const char *env_string;

  if (clutter_is_initialized)
    return TRUE;
  
  if (setlocale (LC_ALL, "") == NULL)
    g_warning ("Locale not supported by C library.\n"
               "Using the fallback 'C' locale.");

  clutter_context = clutter_context_get_default ();

  clutter_context->id_pool = clutter_id_pool_new (256);

  backend = clutter_context->backend;
  g_assert (CLUTTER_IS_BACKEND (backend));

#ifdef CLUTTER_ENABLE_DEBUG
  env_string = g_getenv ("CLUTTER_DEBUG");
  if (env_string != NULL)
    {
      clutter_debug_flags =
        g_parse_debug_string (env_string,
                              clutter_debug_keys,
                              G_N_ELEMENTS (clutter_debug_keys));
      env_string = NULL;
    }
#endif /* CLUTTER_ENABLE_DEBUG */

  env_string = g_getenv ("CLUTTER_SHOW_FPS");
  if (env_string)
    clutter_show_fps = TRUE;

  env_string = g_getenv ("CLUTTER_DEFAULT_FPS");
  if (env_string)
    {
      gint default_fps = g_ascii_strtoll (env_string, NULL, 10);

      clutter_default_fps = CLAMP (default_fps, 1, 1000);
    }

  if (CLUTTER_BACKEND_GET_CLASS (backend)->pre_parse)
    return CLUTTER_BACKEND_GET_CLASS (backend)->pre_parse (backend, error);

  return TRUE;
}

/* post_parse_hook: initialise the context and data structures
 * and opens the X display
 */
static gboolean
post_parse_hook (GOptionContext  *context,
                 GOptionGroup    *group,
                 gpointer         data,
                 GError         **error)
{
  ClutterMainContext *clutter_context;
  ClutterBackend *backend;
  gboolean retval = FALSE;

  if (clutter_is_initialized)
    return TRUE;

  clutter_context = clutter_context_get_default ();
  backend = clutter_context->backend;
  g_assert (CLUTTER_IS_BACKEND (backend));

  if (clutter_fatal_warnings)
    {
      GLogLevelFlags fatal_mask;

      fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
      fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
      g_log_set_always_fatal (fatal_mask);
    }

  clutter_context->frame_rate = clutter_default_fps;

  if (CLUTTER_BACKEND_GET_CLASS (backend)->post_parse)
    retval = CLUTTER_BACKEND_GET_CLASS (backend)->post_parse (backend, error);
  else
    retval = TRUE;

  return retval;
}

/**
 * clutter_get_option_group:
 *
 * Returns a #GOptionGroup for the command line arguments recognized
 * by Clutter. You should add this group to your #GOptionContext with
 * g_option_context_add_group(), if you are using g_option_context_parse()
 * to parse your commandline arguments.
 *
 * Return value: a GOptionGroup for the commandline arguments
 *   recognized by Clutter
 *
 * Since: 0.2
 */
GOptionGroup *
clutter_get_option_group (void)
{
  ClutterMainContext *context;
  GOptionGroup *group;

  context = clutter_context_get_default ();

  group = g_option_group_new ("clutter",
                              "Clutter Options",
                              "Show Clutter Options",
                              NULL,
                              NULL);

  g_option_group_set_parse_hooks (group, pre_parse_hook, post_parse_hook);
  g_option_group_add_entries (group, clutter_args);

  /* add backend-specific options */
  _clutter_backend_add_options (context->backend, group);

  return group;
}

GQuark
clutter_init_error_quark (void)
{
  return g_quark_from_static_string ("clutter-init-error-quark");
}

static ClutterInitError
clutter_init_real (GError **error)
{
  ClutterMainContext *ctx;
  ClutterActor *stage;

  /* Note, creates backend if not already existing, though parse args will
   * have likely created it
   */
  ctx = clutter_context_get_default ();

  /* Stage will give us a GL Context etc */
  stage = clutter_stage_get_default ();
  if (!stage)
    {
      if (error)
        g_set_error (error, CLUTTER_INIT_ERROR,
                     CLUTTER_INIT_ERROR_INTERNAL,
                     "Unable to create the default stage");
      else
        g_critical ("Unable to create the default stage");
      return CLUTTER_INIT_ERROR_INTERNAL;
    }

  clutter_actor_realize (stage);

  if (!CLUTTER_ACTOR_IS_REALIZED (stage))
    {
      if (error)
        g_set_error (error, CLUTTER_INIT_ERROR,
                     CLUTTER_INIT_ERROR_INTERNAL,
                     "Unable to realize the default stage");
      else
        g_critical ("Unable to realize the default stage");

      return CLUTTER_INIT_ERROR_INTERNAL;
    }

  /* Now we can safely assume we have a valid GL context and can 
   * start issueing cogl commands
  */

  /* Figure out framebuffer masks used for pick */
  cogl_get_bitmasks (&ctx->fb_r_mask, &ctx->fb_g_mask, &ctx->fb_b_mask, NULL);

  ctx->fb_r_mask_used = ctx->fb_r_mask;
  ctx->fb_g_mask_used = ctx->fb_g_mask;
  ctx->fb_b_mask_used = ctx->fb_b_mask;

#ifndef HAVE_CLUTTER_FRUITY
  /* We always do fuzzy picking for the fruity backend */
  if (g_getenv ("CLUTTER_FUZZY_PICK") != NULL)
#endif
    {
      ctx->fb_r_mask_used--;
      ctx->fb_g_mask_used--;
      ctx->fb_b_mask_used--;
    }

  /* Initiate event collection */
  _clutter_backend_init_events (ctx->backend);

  /* finally features - will call to backend and cogl */
  _clutter_feature_init ();

  clutter_stage_set_title (CLUTTER_STAGE (stage), g_get_prgname ());

  clutter_is_initialized = TRUE;

  return CLUTTER_INIT_SUCCESS;
}

/**
 * clutter_init_with_args:
 * @argc: a pointer to the number of command line arguments
 * @argv: a pointer to the array of comman line arguments
 * @parameter_string: a string which is displayed in the
 *   first line of <option>--help</option> output, after
 *   <literal><replaceable>programname</replaceable> [OPTION...]</literal>
 * @entries: a %NULL terminated array of #GOptionEntry<!-- -->s
 *   describing the options of your program
 * @translation_domain: a translation domain to use for translating
 *   the <option>--help</option> output for the options in @entries
 *   with gettext(), or %NULL
 * @error: a return location for a #GError
 *
 * This function does the same work as clutter_init(). Additionally,
 * it allows you to add your own command line options, and it
 * automatically generates nicely formatted <option>--help</option>
 * output. Note that your program will be terminated after writing
 * out the help output. Also note that, in case of error, the
 * error message will be placed inside @error instead of being
 * printed on the display.
 *
 * Return value: %CLUTTER_INIT_SUCCESS if Clutter has been successfully
 *   initialised, or other values or #ClutterInitError in case of
 *   error.
 *
 * Since: 0.2
 */
ClutterInitError
clutter_init_with_args (int            *argc,
                        char         ***argv,
                        char           *parameter_string,
                        GOptionEntry   *entries,
                        char           *translation_domain,
                        GError        **error)
{
  GOptionContext *context;
  GOptionGroup *group;
  gboolean res;

  if (clutter_is_initialized)
    return CLUTTER_INIT_SUCCESS;

  clutter_base_init ();

  if (argc && *argc > 0 && *argv)
    g_set_prgname ((*argv)[0]);

  group   = clutter_get_option_group ();
  context = g_option_context_new (parameter_string);

  g_option_context_add_group (context, group);

  if (entries)
    g_option_context_add_main_entries (context, entries, translation_domain);

  res = g_option_context_parse (context, argc, argv, error);
  g_option_context_free (context);

  /* if res is FALSE, the error is filled for
   * us by g_option_context_parse()
   */
  if (!res)
    return CLUTTER_INIT_ERROR_INTERNAL;

  /* Do the real work.. */
  return clutter_init_real (error);
}

static gboolean
clutter_parse_args (int    *argc,
                    char ***argv)
{
  GOptionContext *option_context;
  GOptionGroup   *clutter_group;
  GError         *error = NULL;
  gboolean        ret = TRUE;

  if (clutter_is_initialized)
    return TRUE;

  option_context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  g_option_context_set_help_enabled (option_context, FALSE);

  /* Initiate any command line options from the backend */

  clutter_group = clutter_get_option_group ();
  g_option_context_set_main_group (option_context, clutter_group);

  if (!g_option_context_parse (option_context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      ret = FALSE;
    }

  g_option_context_free (option_context);

  return ret;
}

/**
 * clutter_init:
 * @argc: The number of arguments in @argv
 * @argv: A pointer to an array of arguments.
 *
 * It will initialise everything needed to operate with Clutter and
 * parses some standard command line options. @argc and @argv are
 * adjusted accordingly so your own code will never see those standard
 * arguments.
 *
 * Return value: 1 on success, < 0 on failure.
 */
ClutterInitError
clutter_init (int    *argc,
              char ***argv)
{
  if (clutter_is_initialized)
    return CLUTTER_INIT_SUCCESS;

  clutter_base_init ();

  if (argc && *argc > 0 && *argv)
    g_set_prgname ((*argv)[0]);

  /* parse_args will trigger backend creation and things like
   * DISPLAY connection etc.
  */
  if (clutter_parse_args (argc, argv) == FALSE)
    {
      CLUTTER_NOTE (MISC, "failed to parse arguments.");
      return CLUTTER_INIT_ERROR_INTERNAL;
    }

  return clutter_init_real (NULL);
}

gboolean
_clutter_boolean_handled_accumulator (GSignalInvocationHint *ihint,
                                      GValue                *return_accu,
                                      const GValue          *handler_return,
                                      gpointer               dummy)
{
  gboolean continue_emission;
  gboolean signal_handled;

  signal_handled = g_value_get_boolean (handler_return);
  g_value_set_boolean (return_accu, signal_handled);
  continue_emission = !signal_handled;

  return continue_emission;
}

static void
event_click_count_generate (ClutterEvent *event)
{
  /* multiple button click detection */
  static gint    click_count            = 0;
  static gint    previous_x             = -1;
  static gint    previous_y             = -1;
  static guint32 previous_time          = 0;
  static gint    previous_button_number = -1;

  ClutterBackend *backend;
  guint           double_click_time;
  guint           double_click_distance;

  backend = clutter_context_get_default ()->backend;
  double_click_distance = clutter_backend_get_double_click_distance (backend);
  double_click_time = clutter_backend_get_double_click_time (backend);

  if (event->button.device != NULL)
    {
      click_count = event->button.device->click_count;
      previous_x = event->button.device->previous_x;
      previous_y = event->button.device->previous_y;
      previous_time = event->button.device->previous_time;
      previous_button_number = event->button.device->previous_button_number;
    }

  switch (event->type)
    {
      case CLUTTER_BUTTON_PRESS:
      case CLUTTER_SCROLL:
        /* check if we are in time and within distance to increment an
         * existing click count
         */
        if (event->button.time < previous_time + double_click_time &&
            (ABS (event->button.x - previous_x) <= double_click_distance) &&
            (ABS (event->button.y - previous_y) <= double_click_distance)
            && event->button.button == previous_button_number)
          {
            click_count ++;
          }
        else /* start a new click count*/
          {
            click_count=1;
            previous_button_number = event->button.button;
          }

        /* store time and position for this click for comparison with
         * next event 
         */
        previous_time = event->button.time;
        previous_x    = event->button.x;
        previous_y    = event->button.y;

        /* fallthrough */
      case CLUTTER_BUTTON_RELEASE:
        event->button.click_count=click_count;
        break;
      default:
        g_assert (NULL);
    }

  if (event->button.device != NULL)
    {
      event->button.device->click_count = click_count;
      event->button.device->previous_x = previous_x;
      event->button.device->previous_y = previous_y;
      event->button.device->previous_time = previous_time;
      event->button.device->previous_button_number = previous_button_number;
    }
}


static inline void
emit_event (ClutterEvent *event,
            gboolean      is_key_event)
{
#define MAX_EVENT_DEPTH 512

  static ClutterActor **event_tree = NULL;
  static gboolean       lock = FALSE;

  ClutterActor         *actor;
  gint                  i = 0, n_tree_events = 0;

  if (!event->any.source)
    {
      g_warning ("No event source set, discarding event");
      return;
    }

  /* reentrancy check */
  if (lock != FALSE)
    return;

  lock = TRUE;

  /* Sorry Mr Bassi. */
  if (G_UNLIKELY (event_tree == NULL))
    event_tree = g_new0 (ClutterActor *, MAX_EVENT_DEPTH);

  actor = event->any.source;

  /* Build 'tree' of emitters for the event */
  while (actor && n_tree_events < MAX_EVENT_DEPTH)
    {
      ClutterActor *parent;

      parent = clutter_actor_get_parent (actor);

      if (clutter_actor_get_reactive (actor) ||
          parent == NULL ||         /* stage gets all events */
          is_key_event)             /* keyboard events are always emitted */
        {
          event_tree[n_tree_events++] = g_object_ref (actor);
        }

      actor = parent;
    }

  /* Capture */
  for (i = n_tree_events-1; i >= 0; i--)
    if (clutter_actor_event (event_tree[i], event, TRUE))
      goto done;

  /* Bubble */
  for (i = 0; i < n_tree_events; i++)
    if (clutter_actor_event (event_tree[i], event, FALSE))
      goto done;

done:

  for (i = 0; i < n_tree_events; i++)
    g_object_unref (event_tree[i]);

  lock = FALSE;

#undef MAX_EVENT_DEPTH
}

/*
 * Emits a pointer event after having prepared the event for delivery (setting
 * source, computing click_count, generating enter/leave etc.).
 */

static inline void
emit_pointer_event (ClutterEvent       *event,
                    ClutterInputDevice *device)
{
  /* Using the global variable directly, since it has to be initialized
   * at this point
   */
  ClutterMainContext *context = ClutterCntx;

  if (G_UNLIKELY (context->pointer_grab_actor != NULL &&
                  device == NULL))
    {
      /* global grab */
      clutter_actor_event (context->pointer_grab_actor, event, FALSE);
    }
  else if (G_UNLIKELY (device != NULL &&
                       device->pointer_grab_actor != NULL))
    {
      /* per device grab */
      clutter_actor_event (device->pointer_grab_actor, event, FALSE);
    }
  else
    {
      /* no grab, time to capture and bubble */
      emit_event (event, FALSE);
    }
}

static inline void
emit_keyboard_event (ClutterEvent *event)
{
  ClutterMainContext *context = ClutterCntx;

  if (G_UNLIKELY (context->keyboard_grab_actor != NULL))
    clutter_actor_event (context->keyboard_grab_actor, event, FALSE);
  else
    emit_event (event, TRUE);
}

static void
unset_motion_last_actor (ClutterActor *actor, ClutterInputDevice *dev)
{
  ClutterMainContext *context = ClutterCntx;

  if (dev == NULL)
    context->motion_last_actor = NULL;
  else
    dev->motion_last_actor = NULL;
}

static inline void
generate_enter_leave_events (ClutterEvent *event)
{
  ClutterMainContext *context              = ClutterCntx;
  ClutterActor       *motion_current_actor = event->motion.source;
  ClutterActor       *last_actor           = context->motion_last_actor;

  if (event->motion.device != NULL)
    last_actor = event->motion.device->motion_last_actor;

  if (last_actor != motion_current_actor)
    {
      if (motion_current_actor)
        {
          ClutterEvent cev;

          cev.crossing.device  = event->motion.device;

          if (context->motion_last_actor)
            {
              cev.crossing.type    = CLUTTER_LEAVE;
              cev.crossing.time    = event->any.time;
              cev.crossing.flags   = 0;
              cev.crossing.x       = event->motion.x;
              cev.crossing.y       = event->motion.y;
              cev.crossing.source  = last_actor;
              cev.crossing.stage   = event->any.stage;

              /* unref in free  */
              cev.crossing.related = motion_current_actor;

              g_queue_push_head (context->events_queue,
                                 clutter_event_copy (&cev));
            }

          cev.crossing.type    = CLUTTER_ENTER;
          cev.crossing.time    = event->any.time;
          cev.crossing.flags   = 0;
          cev.crossing.x       = event->motion.x;
          cev.crossing.y       = event->motion.y;
          cev.crossing.source  = motion_current_actor;
          cev.crossing.stage   = event->any.stage;

          if (context->motion_last_actor)
            cev.crossing.related = last_actor;
          else
            {
              /* the previous actor we were getting events from seems to have
               * vanished
               */
              cev.crossing.related = NULL;
            }

          g_queue_push_head (context->events_queue,
			     clutter_event_copy (&cev));
        }
    }

  if (last_actor && last_actor != motion_current_actor)
    {
      g_signal_handlers_disconnect_by_func 
                       (last_actor,
                        G_CALLBACK (unset_motion_last_actor),
                        event->motion.device);
    }

  if (motion_current_actor && last_actor != motion_current_actor)
    {
      g_signal_connect (motion_current_actor, "destroy",
                        G_CALLBACK (unset_motion_last_actor),
                        event->motion.device);
    }

  if (event->motion.device != NULL)
    event->motion.device->motion_last_actor = motion_current_actor;
  else
    context->motion_last_actor = motion_current_actor;
}

/**
 * clutter_do_event
 * @event: a #ClutterEvent.
 *
 * Processes an event. This function should never be called by applications.
 *
 * Since: 0.4
 */
void
clutter_do_event (ClutterEvent *event)
{
  /* FIXME: This should probably be clutter_cook_event() - it would
   * take a raw event from the backend and 'cook' it so its more tasty.
   *
  */
  ClutterMainContext  *context;
  ClutterBackend      *backend;
  ClutterActor        *stage;
  ClutterInputDevice  *device = NULL;
  static gint32        motion_last_time = 0L;
  gint32               local_motion_time;

  context = clutter_context_get_default ();
  backend = context->backend;
  stage   = CLUTTER_ACTOR(event->any.stage);

  if (!stage)
    return;

  CLUTTER_TIMESTAMP (EVENT, "Event received");

  switch (event->type)
    {
      case CLUTTER_NOTHING:
        event->any.source = stage;
        break;

      case CLUTTER_ENTER:
      case CLUTTER_LEAVE:
        emit_pointer_event (event, event->crossing.device);
        break;

      case CLUTTER_DESTROY_NOTIFY:
      case CLUTTER_DELETE:
        event->any.source = stage;
        /* the stage did not handle the event, so we just quit */
        if (!clutter_stage_event (CLUTTER_STAGE (stage), event))
          {
            if (stage == clutter_stage_get_default())
              clutter_main_quit ();
            else
              clutter_actor_destroy (stage);
          }
        
        break;

      case CLUTTER_KEY_PRESS:
      case CLUTTER_KEY_RELEASE:
        {
          ClutterActor *actor = NULL;

          /* check that we're not a synthetic event with source set */
          if (event->any.source == NULL)
            {
              actor = clutter_stage_get_key_focus (CLUTTER_STAGE (stage));
              event->any.source = actor;
              if (G_UNLIKELY (actor == NULL))
                {
                  g_warning ("No key focus set, discarding");
                  return;
                }
            }

          emit_keyboard_event (event);
        }
        break;

      case CLUTTER_MOTION:
        device = event->motion.device;

        if (device)
          local_motion_time = device->motion_last_time;
        else
          local_motion_time = motion_last_time;

        /* avoid rate throttling for synthetic motion events or if
         * the per-actor events are disabled
         */
        if (!(event->any.flags & CLUTTER_EVENT_FLAG_SYNTHETIC) ||
            !context->motion_events_per_actor)
          {
            gint32 frame_rate, delta;

            /* avoid issuing too many motion events, which leads to many
             * redraws in pick mode (performance penalty)
             */
            frame_rate = clutter_get_motion_events_frequency ();
            delta = 1000 / frame_rate;

            CLUTTER_NOTE (EVENT,
                  "skip motion event: %s (last:%d, delta:%d, time:%d)",
                  (event->any.time < (local_motion_time + delta) ? "yes" : "no"),
                  local_motion_time,
                  delta,
                  event->any.time);

            if (event->any.time < (local_motion_time + delta))
              break;
            else
              local_motion_time = event->any.time;
          }

        if (device)
          device->motion_last_time = local_motion_time;
        else
          motion_last_time = local_motion_time;

        /* Only stage gets motion events if clutter_set_motion_events is TRUE,
         * and the event is not a synthetic event with source set.
         */
        if (context->motion_events_per_actor &&
            event->any.source == NULL)
          {
            /* Only stage gets motion events */
            event->any.source = stage;

            /* global grabs */
            if (context->pointer_grab_actor != NULL)
              {
                clutter_actor_event (context->pointer_grab_actor, 
                                     event, FALSE);
                break;
              }
            else if (device != NULL && device->pointer_grab_actor != NULL)
              {
                clutter_actor_event (device->pointer_grab_actor, 
                                     event, FALSE);
                break;
              }

            /* Trigger handlers on stage in both capture .. */
            if (!clutter_actor_event (stage, event, TRUE))
              {
                /* and bubbling phase */
                clutter_actor_event (stage, event, FALSE);
              }
            break;
          }

        /* fallthrough */

      case CLUTTER_BUTTON_PRESS:
      case CLUTTER_BUTTON_RELEASE:
      case CLUTTER_SCROLL:
        {
          ClutterActor *actor;
          gint          x,y;

          clutter_event_get_coords (event, &x, &y);

          /* Only do a pick to find the source if source is not already set
           * (as it could be in a synthetic event)
           */
          if (event->any.source == NULL)
            {
              /* Handle release off stage */
              if ((x >= clutter_actor_get_width (stage) ||
                   y >= clutter_actor_get_height (stage) ||
                   x < 0 || y < 0))
                {
                  if (event->type == CLUTTER_BUTTON_RELEASE)
                    {
                      CLUTTER_NOTE (EVENT,
                                    "Release off stage received at %i, %i",
                                    x, y);

                      event->button.source = stage;
                      emit_pointer_event (event, event->button.device);
                    }
                  break;
                }

              /* Map the event to a reactive actor */
              actor = _clutter_do_pick (CLUTTER_STAGE (stage),
                                        x, y,
                                        CLUTTER_PICK_REACTIVE);

              event->any.source = actor;
              if (!actor)
                break;
            }
          else
            {
              /* use the source already set in the synthetic event */
              actor = event->any.source;
            }


          /* FIXME: for an optimisation should check if there are
           * actually any reactive actors and avoid the pick all togeather
           * (signalling just the stage). Should be big help for gles.
           */

          CLUTTER_NOTE (EVENT, "Reactive event received at %i, %i - actor: %p",
                        x, y, actor);

          if (event->type == CLUTTER_MOTION)
            {
              /* Generate enter leave events (if any) */
              generate_enter_leave_events (event);
            }
          else /* (button event) */
            {
              /* Generate click count */
              event_click_count_generate (event);
            }

          if (device == NULL)
            {
              switch (event->type)
                {
                  case CLUTTER_BUTTON_PRESS:
                  case CLUTTER_BUTTON_RELEASE:
                    device = event->button.device;
                    break;
                  case CLUTTER_SCROLL:
                    device = event->scroll.device;
                    break;
                  case CLUTTER_MOTION:
                    /* already handled in the MOTION case of the switch */
                  default:
                    break;
                }
            }

          emit_pointer_event (event, device);
          break;
        }

      case CLUTTER_STAGE_STATE:
        /* fullscreen / focus - forward to stage */
        event->any.source = stage;
        clutter_stage_event (CLUTTER_STAGE (stage), event);
        break;

      case CLUTTER_CLIENT_MESSAGE:
        break;
    }
}

/**
 * clutter_get_actor_by_gid
 * @id: a #ClutterActor ID.
 *
 * Retrieves the #ClutterActor with @id.
 *
 * Return value: the actor with the passed id or %NULL. The returned
 *   actor does not have its reference count increased.
 *
 * Since: 0.6
 */
ClutterActor*
clutter_get_actor_by_gid (guint32 id)
{
  ClutterMainContext *context;

  context = clutter_context_get_default ();

  g_return_val_if_fail (context != NULL, NULL);

  return CLUTTER_ACTOR (clutter_id_pool_lookup (context->id_pool, id));
}

void
clutter_base_init (void)
{
  static gboolean initialised = FALSE;

  if (!initialised)
    {
      GType foo; /* Quiet gcc */

      initialised = TRUE;

      /* initialise GLib type system */
      g_type_init ();

      /* CLUTTER_TYPE_ACTOR */
      foo = clutter_actor_get_type ();
    }
}

/**
 * clutter_get_default_frame_rate:
 *
 * Retrieves the default frame rate used when creating #ClutterTimeline<!--
 * -->s.
 *
 * This value is also used to compute the default frequency of motion
 * events.
 *
 * Return value: the default frame rate
 *
 * Since: 0.6
 */
guint
clutter_get_default_frame_rate (void)
{
  ClutterMainContext *context;

  context = clutter_context_get_default ();

  return context->frame_rate;
}

/**
 * clutter_set_default_frame_rate:
 * @frames_per_sec: the new default frame rate
 *
 * Sets the default frame rate to be used when creating #ClutterTimeline<!--
 * -->s
 *
 * Since: 0.6
 */
void
clutter_set_default_frame_rate (guint frames_per_sec)
{
  ClutterMainContext *context;

  context = clutter_context_get_default ();

  if (context->frame_rate != frames_per_sec)
    context->frame_rate = frames_per_sec;
}


static void
on_pointer_grab_weak_notify (gpointer data,
                             GObject *where_the_object_was)
{
  ClutterInputDevice *dev = (ClutterInputDevice *)data;
  ClutterMainContext *context;
  
  context = clutter_context_get_default ();

  if (dev)
    {
      dev->pointer_grab_actor = NULL;
      clutter_ungrab_pointer_for_device (dev->id);
    }
  else
    {
      context->pointer_grab_actor = NULL;
      clutter_ungrab_pointer ();
    }
}

/**
 * clutter_grab_pointer:
 * @actor: a #ClutterActor
 *
 * Grabs pointer events, after the grab is done all pointer related events
 * (press, motion, release, enter, leave and scroll) are delivered to this
 * actor directly. The source set in the event will be the actor that would
 * have received the event if the pointer grab was not in effect.
 *
 * If you wish to grab all the pointer events for a specific input device,
 * you should use clutter_grab_pointer_for_device().
 *
 * Since: 0.6
 */
void
clutter_grab_pointer (ClutterActor *actor)
{
  ClutterMainContext *context;

  g_return_if_fail (actor == NULL || CLUTTER_IS_ACTOR (actor));

  context = clutter_context_get_default ();

  if (context->pointer_grab_actor == actor)
    return;

  if (context->pointer_grab_actor)
    {
      g_object_weak_unref (G_OBJECT (context->pointer_grab_actor),
			   on_pointer_grab_weak_notify,
			   NULL);
      context->pointer_grab_actor = NULL;
    }

  if (actor)
    {
      context->pointer_grab_actor = actor;

      g_object_weak_ref (G_OBJECT (actor),
			 on_pointer_grab_weak_notify,
			 NULL);
    }
}

/**
 * clutter_grab_pointer_for_device:
 * @actor: a #ClutterActor
 * @id: a device id, or -1
 *
 * Grabs all the pointer events coming from the device @id for @actor.
 *
 * If @id is -1 then this function is equivalent to clutter_grab_pointer().
 *
 * Since: 0.8
 */
void
clutter_grab_pointer_for_device (ClutterActor *actor,
                                 gint          id)
{
  ClutterInputDevice *dev;

  g_return_if_fail (actor == NULL || CLUTTER_IS_ACTOR (actor));

  /* essentially a global grab */
  if (id == -1)
    {
      clutter_grab_pointer (actor);
      return;
    }

  dev = clutter_get_input_device_for_id (id);

  if (!dev)
    return;

  if (dev->pointer_grab_actor == actor)
    return;

  if (dev->pointer_grab_actor)
    {
      g_object_weak_unref (G_OBJECT (dev->pointer_grab_actor),
                          on_pointer_grab_weak_notify,
                          dev);
      dev->pointer_grab_actor = NULL;
    }

  if (actor)
    {
      dev->pointer_grab_actor = actor;

      g_object_weak_ref (G_OBJECT (actor),
                        on_pointer_grab_weak_notify,
                        dev);
    }
}


/**
 * clutter_ungrab_pointer:
 *
 * Removes an existing grab of the pointer.
 *
 * Since: 0.6
 */
void
clutter_ungrab_pointer (void)
{
  clutter_grab_pointer (NULL);
}

/**
 * clutter_ungrab_pointer_for_device:
 * @id: a device id
 *
 * Removes an existing grab of the pointer events for device @id.
 *
 * Since: 0.8
 */
void
clutter_ungrab_pointer_for_device (gint id)
{
  clutter_grab_pointer_for_device (NULL, id);
}


/**
 * clutter_get_pointer_grab:
 *
 * Queries the current pointer grab of clutter.
 *
 * Return value: the actor currently holding the pointer grab, or NULL if there is no grab.
 *
 * Since: 0.6
 */
ClutterActor *
clutter_get_pointer_grab (void)
{
  ClutterMainContext *context;
  context = clutter_context_get_default ();

  return context->pointer_grab_actor;
}


static void
on_keyboard_grab_weak_notify (gpointer data,
                              GObject *where_the_object_was)
{
  ClutterMainContext *context;

  context = clutter_context_get_default ();
  context->keyboard_grab_actor = NULL;

  clutter_ungrab_keyboard ();
}

/**
 * clutter_grab_keyboard:
 * @actor: a #ClutterActor
 *
 * Grabs keyboard events, after the grab is done keyboard events ("key-press-event"
 * and "key-release-event") are delivered to this actor directly. The source
 * set in the event will be the actor that would have received the event if the
 * keyboard grab was not in effect.
 *
 * Since: 0.6
 */
void
clutter_grab_keyboard (ClutterActor *actor)
{
  ClutterMainContext *context;

  g_return_if_fail (actor == NULL || CLUTTER_IS_ACTOR (actor));

  context = clutter_context_get_default ();

  if (context->keyboard_grab_actor == actor)
    return;

  if (context->keyboard_grab_actor)
    {
      g_object_weak_unref (G_OBJECT (context->keyboard_grab_actor),
			   on_keyboard_grab_weak_notify,
			   NULL);
      context->keyboard_grab_actor = NULL;
    }

  if (actor)
    {
      context->keyboard_grab_actor = actor;

      g_object_weak_ref (G_OBJECT (actor),
			 on_keyboard_grab_weak_notify,
			 NULL);
    }
}

/**
 * clutter_ungrab_keyboard:
 *
 * Removes an existing grab of the keyboard.
 *
 * Since: 0.6
 */
void
clutter_ungrab_keyboard (void)
{
  clutter_grab_keyboard (NULL);
}

/**
 * clutter_get_keyboard_grab:
 *
 * Queries the current keyboard grab of clutter.
 *
 * Return value: the actor currently holding the keyboard grab, or NULL if there is no grab.
 *
 * Since: 0.6
 */
ClutterActor *
clutter_get_keyboard_grab (void)
{
  ClutterMainContext *context;
  context = clutter_context_get_default ();

  return context->keyboard_grab_actor;
}

/**
 * clutter_get_motion_events_frequency:
 *
 * Retrieves the number of motion events per second that are delivered
 * to the stage.
 *
 * See clutter_set_motion_events_frequency().
 *
 * Return value: the number of motion events per second
 *
 * Since: 0.6
 */
guint
clutter_get_motion_events_frequency (void)
{
  ClutterMainContext *context = clutter_context_get_default ();

  if (G_LIKELY (context->motion_frequency == 0))
    {
      guint frequency;

      frequency = clutter_default_fps / 4;
      frequency = CLAMP (frequency, 20, 45);

      return frequency;
    }
  else
    return context->motion_frequency;
}

/**
 * clutter_set_motion_events_frequency:
 * @frequency: the number of motion events per second, or 0 for the
 *   default value
 *
 * Sets the motion events frequency. Setting this to a non-zero value
 * will override the default setting, so it should be rarely used.
 *
 * Motion events are delivered from the default backend to the stage
 * and are used to generate the enter/leave events pair. This might lead
 * to a performance penalty due to the way the actors are identified.
 * Using this function is possible to reduce the frequency of the motion
 * events delivery to the stage.
 *
 * Since: 0.6
 */
void
clutter_set_motion_events_frequency (guint frequency)
{
  ClutterMainContext *context = clutter_context_get_default ();

  /* never allow the motion events to exceed the default frame rate */
  context->motion_frequency = CLAMP (frequency, 1, clutter_default_fps);
}

/**
 * clutter_clear_glyph_cache:
 *
 * Clears the internal cache of glyphs used by the Pango
 * renderer. This will free up some memory and GL texture
 * resources. The cache will be automatically refilled as more text is
 * drawn.
 * 
 * Since: 0.8
 */
void
clutter_clear_glyph_cache (void)
{
  if (CLUTTER_CONTEXT ()->font_map)
    pango_clutter_font_map_clear_glyph_cache (CLUTTER_CONTEXT ()->font_map);
}

/**
 * clutter_set_use_mipmapped_text:
 * @value: %TRUE to enable mipmapping or %FALSE to disable.
 *
 * Sets whether subsequent text rendering operations will use
 * mipmapped textures or not. Using mipmapped textures will improve
 * the quality for scaled down text but will use more texture memory.
 *
 * Since: 0.8
 */
void
clutter_set_use_mipmapped_text (gboolean value)
{
  if (CLUTTER_CONTEXT ()->font_map)
    pango_clutter_font_map_set_use_mipmapping (CLUTTER_CONTEXT ()->font_map,
					       value);
}

/**
 * clutter_get_use_mipmapped_text:
 *
 * Gets whether mipmapped textures are used in text operations.
 * See clutter_set_use_mipmapped_text().
 *
 * Return value: %TRUE if text operations should use mipmapped
 *   textures
 *
 * Since: 0.8
 */
gboolean
clutter_get_use_mipmapped_text (void)
{
  PangoClutterFontMap *font_map = NULL;

  font_map = CLUTTER_CONTEXT ()->font_map;

  if (font_map)
    return pango_clutter_font_map_get_use_mipmapping (font_map);

  return FALSE;    
}

/**
 * clutter_get_input_device_for_id:
 * @id: a device id
 *
 * Retrieves the #ClutterInputDevice from its id.
 *
 * Return value: a #ClutterInputDevice, or %NULL
 *
 * Since: 0.8
 */
ClutterInputDevice *
clutter_get_input_device_for_id (gint id)
{
  GSList *item;
  ClutterInputDevice *device = NULL;
  ClutterMainContext  *context;

  context = clutter_context_get_default ();

  for (item = context->input_devices; 
       item != NULL; 
       item = item->next)
  {
    device = item->data;

    if (device->id == id)
      return device;
  }

  return NULL;
}
