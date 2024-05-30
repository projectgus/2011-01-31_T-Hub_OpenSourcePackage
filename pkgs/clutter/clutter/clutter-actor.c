/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006 OpenedHand
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
 * SECTION:clutter-actor
 * @short_description: Base abstract class for all visual stage actors.
 *
 * #ClutterActor is a base abstract class for all visual elements on the
 * stage. Every object that must appear on the main #ClutterStage must also
 * be a #ClutterActor, either by using one of the classes provided by
 * Clutter, or by implementing a new #ClutterActor subclass.
 *
 * Every actor is a 2D surface positioned and optionally transformed
 * in 3D space. The actor is positioned relative to top left corner of
 * it parent with the childs origin being its anchor point (also top
 * left by default).
 *
 * The actors 2D surface is contained inside its bounding box,
 * described by the #ClutterActorBox structure:
 *
 * <figure id="actor-box">
 *   <title>Bounding box of an Actor</title>
 *   <graphic fileref="actor-box.png" format="PNG"/>
 * </figure>
 *
 * The actor box represents the untransformed area occupied by an
 * actor. Each visible actor that has been put on a #ClutterStage also
 * has a transformed area, depending on the actual transformations
 * applied to it by the developer (scale, rotation). Tranforms will
 * also be applied to any child actors. Also applied to all actors by
 * the #ClutterStage is a perspective transformation. API is provided
 * for both tranformed and untransformed actor geometry information.
 *
 * The 'modelview' transform matrix for the actor is constructed from
 * the actor settings by the following order of operations:
 * <orderedlist>
 *   <listitem><para>Translation by actor x, y coords,</para></listitem>
 *   <listitem><para>Scaling by scale_x, scale_y,</para></listitem>
 *   <listitem><para>Negative translation by anchor point x,
 *   y,</para></listitem>
 *   <listitem><para>Rotation around z axis,</para></listitem>
 *   <listitem><para>Rotation around y axis,</para></listitem>
 *   <listitem><para>Rotation around x axis,</para></listitem>
 *   <listitem><para>Translation by actor depth (z),</para></listitem>
 *   <listitem><para>Rectangular Clip is applied (this is not an operation on
 *   the matrix as such, but it is done as part of the transform set
 *   up).</para></listitem>
 * </orderedlist>
 *
 * An actor can either be explicitly sized and positioned, using the
 * various size and position accessors, like clutter_actor_set_x() or
 * clutter_actor_set_width(); or it can have a preferred width and
 * height, which then allows a layout manager to implicitly size and
 * position it by "allocating" an area for an actor. This allows for
 * actors to be manipulate in both a fixed or static parent container
 * (i.e. children of #ClutterGroup) and a more automatic or dynamic
 * layout based parent container.
 *
 * When accessing the position and size of an actor, the simple accessors
 * like clutter_actor_get_width() and clutter_actor_get_x() will return
 * a value depending on whether the actor has been explicitly sized and
 * positioned by the developer or implicitly by the layout manager.
 *
 * Depending on whether you are querying an actor or implementing a
 * layout manager, you should either use the simple accessors or use the
 * size negotiation API.
 *
 * Clutter actors are also able to receive input events and react to
 * them. Events are handled in the following ways:
 *
 * <orderedlist>
 *   <listitem><para>Actors emit pointer events if set reactive, see
 *   clutter_actor_set_reactive()</para></listitem>
 *   <listitem><para>The stage is always reactive</para></listitem>
 *   <listitem><para>Events are handled by connecting signal handlers to
 *   the numerous event signal types.</para></listitem>
 *   <listitem><para>Event handlers must return %TRUE if they handled
 *   the event and wish to block the event emission chain, or %FALSE
 *   if the emission chain must continue</para></listitem>
 *   <listitem><para>Keyboard events are emitted if actor has focus, see
 *   clutter_stage_set_key_focus()</para></listitem>
 *   <listitem><para>Motion events (motion, enter, leave) are not emitted
 *   if clutter_set_motion_events_enabled() is called with %FALSE.
 *   See clutter_set_motion_events_enabled() documentation for more
 *   information.</para></listitem>
 *   <listitem><para>Once emitted, an event emission chain has two
 *   phases: capture and bubble. An emitted event starts in the capture
 *   phase (see ClutterActor::captured-event) beginning at the stage and
 *   traversing every child actor until the event source actor is reached.
 *   The emission then enters the bubble phase, traversing back up the
 *   chain via parents until it reaches the stage. Any event handler can
 *   abort this chain by returning %TRUE (meaning "event handled").
 *   </para></listitem>
 *   <listitem><para>Pointer events will 'pass through' non reactive
 *   overlapping actors.</para></listitem>
 * </orderedlist>
 *
 * <figure id="event-flow">
 *   <title>Event flow in Clutter</title>
 *   <graphic fileref="event-flow.png" format="PNG"/>
 * </figure>
 *
 * Every '?' box in the diagram above is an entry point for application
 * code.
 *
 * For implementing a new custom actor class, please read <link
 * linkend="clutter-subclassing-ClutterActor">the corresponding section</link>
 * of the API reference.
 */

/**
 * CLUTTER_ACTOR_IS_MAPPED:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the %CLUTTER_ACTOR_MAPPED flag is set.
 *
 * Since: 0.2
 */

/**
 * CLUTTER_ACTOR_IS_REALIZED:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the %CLUTTER_ACTOR_REALIZED flag is set.
 *
 * Since: 0.2
 */

/**
 * CLUTTER_ACTOR_IS_VISIBLE:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the actor is both realized and mapped.
 *
 * Since: 0.2
 */

/**
 * CLUTTER_ACTOR_IS_REACTIVE:
 * @e: a #ClutterActor
 *
 * Evaluates to %TRUE if the %CLUTTER_ACTOR_REACTIVE flag is set.
 *
 * Since: 0.6
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clutter-actor.h"
#include "clutter-container.h"
#include "clutter-main.h"
#include "clutter-enum-types.h"
#include "clutter-scriptable.h"
#include "clutter-script.h"
#include "clutter-marshal.h"
#include "clutter-private.h"
#include "clutter-debug.h"
#include "clutter-units.h"
#include "cogl/cogl.h"

typedef struct _ShaderData ShaderData;

#define CLUTTER_ACTOR_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), CLUTTER_TYPE_ACTOR, ClutterActorPrivate))

struct _ClutterActorPrivate
{
  /* fixed_x, fixed_y, and the allocation box are all in parent
   * coordinates.
   */
  ClutterUnit fixed_x;
  ClutterUnit fixed_y;

  /* request mode */
  ClutterRequestMode request_mode;

  /* our cached request width is for this height */
  ClutterUnit request_width_for_height;
  ClutterUnit request_min_width;
  ClutterUnit request_natural_width;
  /* our cached request height is for this width */
  ClutterUnit request_height_for_width;
  ClutterUnit request_min_height;
  ClutterUnit request_natural_height;

  ClutterActorBox allocation;

  guint position_set         : 1;
  guint min_width_set        : 1;
  guint min_height_set       : 1;
  guint natural_width_set    : 1;
  guint natural_height_set   : 1;
  /* cached request is invalid (implies allocation is too) */
  guint needs_width_request  : 1;
  /* cached request is invalid (implies allocation is too) */
  guint needs_height_request : 1;
  /* cached allocation is invalid (request has changed, probably) */
  guint needs_allocation     : 1;

  guint           has_clip : 1;
  ClutterUnit     clip[4];

  /* Rotation angles */
  ClutterFixed    rxang;
  ClutterFixed    ryang;
  ClutterFixed    rzang;

  /* Rotation center: X axis */
  ClutterUnit     rxy;
  ClutterUnit     rxz;

  /* Rotation center: Y axis */
  ClutterUnit     ryx;
  ClutterUnit     ryz;

  /* Rotation center: Z axis */
  ClutterUnit     rzx;
  ClutterUnit     rzy;

  /* Anchor point coordinates */
  ClutterUnit     anchor_x;
  ClutterUnit     anchor_y;

  /* depth */
  ClutterUnit     z;

  guint8          opacity;

  ClutterActor   *parent_actor;

  gchar          *name;
  guint32         id; /* Unique ID */

  ClutterFixed    scale_x;
  ClutterFixed    scale_y;

  ShaderData     *shader_data;

  gboolean        show_on_set_parent;
};

enum
{
  PROP_0,

  PROP_NAME,

  /* X, Y, WIDTH, HEIGHT are "do what I mean" properties;
   * when set they force a size request, when gotten they
   * get the allocation if the allocation is valid, and the
   * request otherwise. Also, they are in pixels, while
   * all the underlying properties are in ClutterUnit.
   */
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,

  /* Then the rest of these size-related properties are the "actual"
   * underlying properties set or gotten by X, Y, WIDTH, HEIGHT. All
   * of these are in ClutterUnit not in pixels.
   */
  PROP_FIXED_X,
  PROP_FIXED_Y,

  PROP_FIXED_POSITION_SET,

  PROP_MIN_WIDTH,
  PROP_MIN_WIDTH_SET,

  PROP_MIN_HEIGHT,
  PROP_MIN_HEIGHT_SET,

  PROP_NATURAL_WIDTH,
  PROP_NATURAL_WIDTH_SET,

  PROP_NATURAL_HEIGHT,
  PROP_NATURAL_HEIGHT_SET,

  PROP_REQUEST_MODE,

  /* Allocation properties are read-only */
  PROP_ALLOCATION,

  PROP_DEPTH,

  PROP_CLIP,
  PROP_HAS_CLIP,

  PROP_OPACITY,
  PROP_VISIBLE,
  PROP_REACTIVE,

  PROP_SCALE_X,
  PROP_SCALE_Y,

  PROP_ROTATION_ANGLE_X,
  PROP_ROTATION_ANGLE_Y,
  PROP_ROTATION_ANGLE_Z,
  PROP_ROTATION_CENTER_X,
  PROP_ROTATION_CENTER_Y,
  PROP_ROTATION_CENTER_Z,

  PROP_ANCHOR_X,
  PROP_ANCHOR_Y,

  PROP_SHOW_ON_SET_PARENT
};

enum
{
  SHOW,
  HIDE,
  DESTROY,
  PARENT_SET,
  FOCUS_IN,
  FOCUS_OUT,
  PAINT,
  REALIZE,
  UNREALIZE,

  EVENT,
  CAPTURED_EVENT,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SCROLL_EVENT,
  KEY_PRESS_EVENT,
  KEY_RELEASE_EVENT,
  MOTION_EVENT,
  ENTER_EVENT,
  LEAVE_EVENT,

  LAST_SIGNAL
};

static guint actor_signals[LAST_SIGNAL] = { 0, };

static void clutter_scriptable_iface_init (ClutterScriptableIface *iface);

static void _clutter_actor_apply_modelview_transform           (ClutterActor *self);
static void _clutter_actor_apply_modelview_transform_recursive (ClutterActor *self,
                                                                ClutterActor *ancestor);

static void clutter_actor_shader_pre_paint  (ClutterActor *actor,
                                             gboolean      repeat);
static void clutter_actor_shader_post_paint (ClutterActor *actor);

static void destroy_shader_data (ClutterActor *self);

/* These setters are all static for now, maybe they should be in the
 * public API, but they are perhaps obscure enough to leave only as
 * properties
 */
static void clutter_actor_set_min_width          (ClutterActor *self,
                                                  ClutterUnit   min_width);
static void clutter_actor_set_min_height         (ClutterActor *self,
                                                  ClutterUnit   min_height);
static void clutter_actor_set_natural_width      (ClutterActor *self,
                                                  ClutterUnit   natural_width);
static void clutter_actor_set_natural_height     (ClutterActor *self,
                                                  ClutterUnit   natural_height);
static void clutter_actor_set_min_width_set      (ClutterActor *self,
                                                  gboolean      use_min_width);
static void clutter_actor_set_min_height_set     (ClutterActor *self,
                                                  gboolean      use_min_height);
static void clutter_actor_set_natural_width_set  (ClutterActor *self,
                                                  gboolean  use_natural_width);
static void clutter_actor_set_natural_height_set (ClutterActor *self,
                                                  gboolean  use_natural_height);
static void clutter_actor_set_request_mode       (ClutterActor *self,
                                                  ClutterRequestMode mode);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ClutterActor,
                                  clutter_actor,
                                  G_TYPE_INITIALLY_UNOWNED,
                                  G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_SCRIPTABLE,
                                                         clutter_scriptable_iface_init));


static void
clutter_actor_real_show (ClutterActor *self)
{
  if (!CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      if (!CLUTTER_ACTOR_IS_REALIZED (self))
        clutter_actor_realize (self);

      /* the mapped flag on the top-level actors must be set by the
       * per-backend implementation because it might be asynchronous
       */
      if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL))
        CLUTTER_ACTOR_SET_FLAGS (self, CLUTTER_ACTOR_MAPPED);

      if (CLUTTER_ACTOR_IS_VISIBLE (self))
        clutter_actor_queue_redraw (self);

      clutter_actor_queue_relayout (self);
    }
}

/**
 * clutter_actor_show:
 * @self: A #ClutterActor
 *
 * Flags an actor to be displayed. An actor that isn't shown will not
 * be rendered on the stage.
 *
 * Actors are visible by default.
 *
 * If this function is called on an actor without a parent, the
 * #ClutterActor:show-on-set-parent will be set to %TRUE as a side
 * effect.
 */
void
clutter_actor_show (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  g_object_freeze_notify (G_OBJECT (self));

  if (!priv->show_on_set_parent && !priv->parent_actor)
    {
      priv->show_on_set_parent = TRUE;
      g_object_notify (G_OBJECT (self), "show-on-set-parent");
    }

  if (!CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      g_signal_emit (self, actor_signals[SHOW], 0);
      g_object_notify (G_OBJECT (self), "visible");
    }

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_show_all:
 * @self: a #ClutterActor
 *
 * Calls clutter_actor_show() on all children of an actor (if any).
 *
 * Since: 0.2
 */
void
clutter_actor_show_all (ClutterActor *self)
{
  ClutterActorClass *klass;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  klass = CLUTTER_ACTOR_GET_CLASS (self);
  if (klass->show_all)
    klass->show_all (self);
}

void
clutter_actor_real_hide (ClutterActor *self)
{
  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      /* see comment in clutter_actor_real_show() on why we don't set
       * the mapped flag on the top-level actors
       */
      if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL))
        CLUTTER_ACTOR_UNSET_FLAGS (self, CLUTTER_ACTOR_MAPPED);

      clutter_actor_queue_relayout (self);
    }
}

/**
 * clutter_actor_hide:
 * @self: A #ClutterActor
 *
 * Flags an actor to be hidden. A hidden actor will not be
 * rendered on the stage.
 *
 * Actors are visible by default.
 *
 * If this function is called on an actor without a parent, the
 * #ClutterActor:show-on-set-parent property will be set to %FALSE
 * as a side-effect.
 */
void
clutter_actor_hide (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  g_object_freeze_notify (G_OBJECT (self));

  if (priv->show_on_set_parent && !priv->parent_actor)
    {
      priv->show_on_set_parent = FALSE;
      g_object_notify (G_OBJECT (self), "show-on-set-parent");
    }

  if (CLUTTER_ACTOR_IS_MAPPED (self))
    {
      g_signal_emit (self, actor_signals[HIDE], 0);
      g_object_notify (G_OBJECT (self), "visible");
    }

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_hide_all:
 * @self: a #ClutterActor
 *
 * Calls clutter_actor_hide() on all child actors (if any).
 *
 * Since: 0.2
 */
void
clutter_actor_hide_all (ClutterActor *self)
{
  ClutterActorClass *klass;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  klass = CLUTTER_ACTOR_GET_CLASS (self);
  if (klass->hide_all)
    klass->hide_all (self);
}

/**
 * clutter_actor_realize:
 * @self: A #ClutterActor
 *
 * Creates any underlying graphics resources needed by the actor to be
 * displayed.
 **/
void
clutter_actor_realize (ClutterActor *self)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (CLUTTER_ACTOR_IS_REALIZED (self))
    return;

  CLUTTER_ACTOR_SET_FLAGS (self, CLUTTER_ACTOR_REALIZED);

  g_signal_emit (self, actor_signals[REALIZE], 0);
}

/**
 * clutter_actor_unrealize:
 * @self: A #ClutterActor
 *
 * Frees up any underlying graphics resources needed by the actor to be
 * displayed.
 **/
void
clutter_actor_unrealize (ClutterActor *self)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (!CLUTTER_ACTOR_IS_REALIZED (self))
    return;

  /* unrealizing also means hiding a visible actor, exactly
   * like showing implies realization if called on an unrealized
   * actor. this keeps the flags in sync.
   */
  clutter_actor_hide (self);

  CLUTTER_ACTOR_UNSET_FLAGS (self, CLUTTER_ACTOR_REALIZED);

  g_signal_emit (self, actor_signals[UNREALIZE], 0);
}

static void
clutter_actor_real_pick (ClutterActor       *self,
			 const ClutterColor *color)
{
  /* the default implementation is just to paint a rectangle
   * with the same size of the actor using the passed color
   */
  if (clutter_actor_should_pick_paint (self))
    {
      cogl_color (color);
      cogl_rectangle (0, 0,
                      clutter_actor_get_width (self),
                      clutter_actor_get_height (self));
    }
}

/**
 * clutter_actor_pick:
 * @self: A #ClutterActor
 * @color: A #ClutterColor
 *
 * Renders a silhouette of the actor using the supplied color. Used
 * internally for mapping pointer events to actors.
 *
 * This function should never be called directly by applications.
 *
 * Subclasses overiding the ClutterActor::pick() method should call
 * clutter_actor_should_pick_paint() to decide whether to render their
 * silhouette. Containers should always recursively call pick for
 * each child.
 *
 * Since 0.4
 **/
void
clutter_actor_pick (ClutterActor       *self,
		    const ClutterColor *color)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (color != NULL);

  CLUTTER_ACTOR_GET_CLASS (self)->pick (self, color);
}

/**
 * clutter_actor_should_pick_paint:
 * @self: A #ClutterActor
 *
 * Utility call for subclasses overiding the pick method.
 *
 * This function should never be called directly by applications.
 *
 * Return value: %TRUE if the actor should paint its silhouette,
 *   %FALSE otherwise
 */
gboolean
clutter_actor_should_pick_paint (ClutterActor *self)
{
  ClutterMainContext *context;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  context = clutter_context_get_default ();

  if (CLUTTER_ACTOR_IS_MAPPED (self) &&
      (G_UNLIKELY (context->pick_mode == CLUTTER_PICK_ALL) ||
       CLUTTER_ACTOR_IS_REACTIVE (self)))
    return TRUE;

  return FALSE;
}

static void
clutter_actor_real_get_preferred_width (ClutterActor *self,
                                        ClutterUnit   for_height,
                                        ClutterUnit  *min_width_p,
                                        ClutterUnit  *natural_width_p)
{
  /* Default implementation is always 0x0, usually an actor
   * using this default is relying on someone to set the
   * request manually
   */

  if (min_width_p)
    *min_width_p = 0;

  if (natural_width_p)
    *natural_width_p = 0;
}

static void
clutter_actor_real_get_preferred_height (ClutterActor *self,
                                         ClutterUnit   for_width,
                                         ClutterUnit  *min_height_p,
                                         ClutterUnit  *natural_height_p)
{
  /* Default implementation is always 0x0, usually an actor
   * using this default is relying on someone to set the
   * request manually
   */

  if (min_height_p)
    *min_height_p = 0;

  if (natural_height_p)
    *natural_height_p = 0;
}

static void
clutter_actor_store_old_geometry (ClutterActor    *self,
                                  ClutterActorBox *box)
{
  box->x1 = clutter_actor_get_xu (self);
  box->y1 = clutter_actor_get_yu (self);
  box->x2 = box->x1 + clutter_actor_get_widthu (self);
  box->y2 = box->y1 + clutter_actor_get_heightu (self);
}

static inline void
clutter_actor_notify_if_geometry_changed (ClutterActor          *self,
                                          const ClutterActorBox *old)
{
  ClutterUnit xu, yu;
  ClutterUnit widthu, heightu;

  clutter_actor_get_positionu (self, &xu, &yu);
  clutter_actor_get_sizeu (self, &widthu, &heightu);

  g_object_freeze_notify (G_OBJECT (self));

  if (xu != old->x1)
    g_object_notify (G_OBJECT (self), "x");

  if (yu != old->y1)
    g_object_notify (G_OBJECT (self), "y");

  if (widthu != (old->x2 - old->x1))
    g_object_notify (G_OBJECT (self), "width");

  if (heightu != (old->y2 - old->y1))
    g_object_notify (G_OBJECT (self), "height");

  g_object_thaw_notify (G_OBJECT (self));
}

static void
clutter_actor_real_allocate (ClutterActor          *self,
                             const ClutterActorBox *box,
                             gboolean               absolute_origin_changed)
{
  ClutterActorPrivate *priv = self->priv;
  gboolean x1_changed, y1_changed, x2_changed, y2_changed;
  ClutterActorBox old = { 0, };

  clutter_actor_store_old_geometry (self, &old);

  x1_changed = priv->allocation.x1 != box->x1;
  y1_changed = priv->allocation.y1 != box->y1;
  x2_changed = priv->allocation.x2 != box->x2;
  y2_changed = priv->allocation.y2 != box->y2;

  priv->allocation = *box;
  priv->needs_allocation = FALSE;

  g_object_freeze_notify (G_OBJECT (self));

  if (x1_changed || y1_changed || x2_changed || y2_changed)
    g_object_notify (G_OBJECT (self), "allocation");

  clutter_actor_notify_if_geometry_changed (self, &old);

  g_object_thaw_notify (G_OBJECT (self));
}

/*
 * Utility functions for manipulating transformation matrix
 *
 * Matrix: 4x4 of ClutterFixed
 */
#define M(m,row,col)  (m)[(col) * 4 + (row)]

/* Transform point (x,y,z) by matrix */
static void
mtx_transform (ClutterFixed m[16],
	       ClutterFixed *x, ClutterFixed *y, ClutterFixed *z,
	       ClutterFixed *w)
{
    ClutterFixed _x, _y, _z, _w;
    _x = *x;
    _y = *y;
    _z = *z;
    _w = *w;

    /* We care lot about precision here, so have to use QMUL */
    *x = CFX_QMUL (M (m, 0, 0), _x) + CFX_QMUL (M (m, 0, 1), _y) +
	 CFX_QMUL (M (m, 0, 2), _z) + CFX_QMUL (M (m, 0, 3), _w);

    *y = CFX_QMUL (M (m, 1, 0), _x) + CFX_QMUL (M (m, 1, 1), _y) +
	 CFX_QMUL (M (m, 1, 2), _z) + CFX_QMUL (M (m, 1, 3), _w);

    *z = CFX_QMUL (M (m, 2, 0), _x) + CFX_QMUL (M (m, 2, 1), _y) +
	 CFX_QMUL (M (m, 2, 2), _z) + CFX_QMUL (M (m, 2, 3), _w);

    *w = CFX_QMUL (M (m, 3, 0), _x) + CFX_QMUL (M (m, 3, 1), _y) +
	 CFX_QMUL (M (m, 3, 2), _z) + CFX_QMUL (M (m, 3, 3), _w);

    /* Specially for Matthew: was going to put a comment here, but could not
     * think of anything at all to say ;)
     */
}

#undef M

/* Applies the transforms associated with this actor and its ancestors,
 * retrieves the resulting OpenGL modelview matrix, and uses the matrix
 * to transform the supplied point
 */
static void
clutter_actor_transform_point_relative (ClutterActor *actor,
					ClutterActor *ancestor,
					ClutterUnit  *x,
					ClutterUnit  *y,
					ClutterUnit  *z,
					ClutterUnit  *w)
{
  ClutterFixed mtx[16];

  cogl_push_matrix();

  _clutter_actor_apply_modelview_transform_recursive (actor, ancestor);
  cogl_get_modelview_matrix (mtx);
  mtx_transform (mtx, x, y, z, w);

  cogl_pop_matrix();
}

/* Applies the transforms associated with this actor and its ancestors,
 * retrieves the resulting OpenGL modelview matrix, and uses the matrix
 * to transform the supplied point
 */
static void
clutter_actor_transform_point (ClutterActor *actor,
			       ClutterUnit  *x,
			       ClutterUnit  *y,
			       ClutterUnit  *z,
			       ClutterUnit  *w)
{
  ClutterFixed mtx[16];

  cogl_push_matrix();

  _clutter_actor_apply_modelview_transform_recursive (actor, NULL);
  cogl_get_modelview_matrix (mtx);
  mtx_transform (mtx, x, y, z, w);

  cogl_pop_matrix();
}

/* Help macros to scale from OpenGL <-1,1> coordinates system to our
 * X-window based <0,window-size> coordinates
 */
#define MTX_GL_SCALE_X(x,w,v1,v2) (CFX_QMUL (((CFX_QDIV ((x), (w)) + CFX_ONE) >> 1), (v1)) + (v2))
#define MTX_GL_SCALE_Y(y,w,v1,v2) ((v1) - CFX_QMUL (((CFX_QDIV ((y), (w)) + CFX_ONE) >> 1), (v1)) + (v2))
#define MTX_GL_SCALE_Z(z,w,v1,v2) (MTX_GL_SCALE_X ((z), (w), (v1), (v2)))

/**
 * clutter_actor_apply_relative_transform_to_point:
 * @self: A #ClutterActor
 * @ancestor: A #ClutterActor ancestor, or %NULL to use the
 *   default #ClutterStage
 * @point: A point as #ClutterVertex
 * @vertex: The translated #ClutterVertex
 *
 * Transforms @point in coordinates relative to the actor into
 * ancestor-relative coordinates using the relevant transform
 * stack (i.e. scale, rotation, etc).
 *
 * If @ancestor is %NULL the ancestor will be the #ClutterStage. In
 * this case, the coordinates returned will be the coordinates on
 * the stage before the projection is applied. This is different from
 * the behaviour of clutter_actor_apply_transform_to_point().
 *
 * Since: 0.6
 */
void
clutter_actor_apply_relative_transform_to_point (ClutterActor  *self,
						 ClutterActor  *ancestor,
						 ClutterVertex *point,
						 ClutterVertex *vertex)
{
  ClutterFixed  v[4];
  ClutterFixed  w = CFX_ONE;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (ancestor == NULL || CLUTTER_IS_ACTOR (ancestor));

  /* First we tranform the point using the OpenGL modelview matrix */
  clutter_actor_transform_point_relative (self, ancestor,
					  &point->x, &point->y, &point->z,
					  &w);

  cogl_get_viewport (v);

  /*
   * The w[3] parameter should always be 1.0 here, so we ignore it; otherwise
   * we would have to divide the original verts with it.
   */
  vertex->x = CFX_QMUL ((point->x + CFX_ONE / 2), v[2]);
  vertex->y = CFX_QMUL ((CFX_ONE / 2 - point->y), v[3]);
  vertex->z = CFX_QMUL ((point->z + CFX_ONE / 2), v[2]);
}

/**
 * clutter_actor_apply_transform_to_point:
 * @self: A #ClutterActor
 * @point: A point as #ClutterVertex
 * @vertex: The translated #ClutterVertex
 *
 * Transforms @point in coordinates relative to the actor
 * into screen-relative coordinates with the current actor
 * transformation (i.e. scale, rotation, etc)
 *
 * Since: 0.4
 **/
void
clutter_actor_apply_transform_to_point (ClutterActor  *self,
					ClutterVertex *point,
					ClutterVertex *vertex)
{
  ClutterFixed  mtx_p[16];
  ClutterFixed  v[4];
  ClutterFixed  w = CFX_ONE;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  /* First we tranform the point using the OpenGL modelview matrix */
  clutter_actor_transform_point (self, &point->x, &point->y, &point->z, &w);

  cogl_get_projection_matrix (mtx_p);
  cogl_get_viewport (v);

  /* Now, transform it again with the projection matrix */
  mtx_transform (mtx_p, &point->x, &point->y, &point->z, &w);

  /* Finaly translate from OpenGL coords to window coords */
  vertex->x = MTX_GL_SCALE_X (point->x, w, v[2], v[0]);
  vertex->y = MTX_GL_SCALE_Y (point->y, w, v[3], v[1]);
  vertex->z = MTX_GL_SCALE_Z (point->z, w, v[2], v[0]);
}

/* Recursively tranform supplied vertices with the tranform for the current
 * actor and up to the ancestor (like clutter_actor_transform_point() but
 * for all the vertices in one go).
 */
static void
clutter_actor_transform_vertices_relative (ClutterActor  *self,
					   ClutterActor  *ancestor,
					   ClutterVertex  verts[4],
					   ClutterFixed   w[4])
{
  ClutterFixed    mtx[16];
  ClutterFixed    _x, _y, _z, _w;

  cogl_push_matrix();

  _clutter_actor_apply_modelview_transform_recursive (self, ancestor);
  cogl_get_modelview_matrix (mtx);

  _x = 0;
  _y = 0;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[0].x = _x;
  verts[0].y = _y;
  verts[0].z = _z;
  w[0] = _w;

  _x = self->priv->allocation.x2 - self->priv->allocation.x1;
  _y = 0;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[1].x = _x;
  verts[1].y = _y;
  verts[1].z = _z;
  w[1] = _w;

  _x = 0;
  _y = self->priv->allocation.y2 - self->priv->allocation.y1;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[2].x = _x;
  verts[2].y = _y;
  verts[2].z = _z;
  w[2] = _w;

  _x = self->priv->allocation.x2 - self->priv->allocation.x1;
  _y = self->priv->allocation.y2 - self->priv->allocation.y1;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[3].x = _x;
  verts[3].y = _y;
  verts[3].z = _z;
  w[3] = _w;

  cogl_pop_matrix();
}

/* Recursively tranform supplied box with the tranform for the current
 * actor and all its ancestors (like clutter_actor_transform_point()
 * but for all the vertices in one go) and project it into screen
 * coordinates
 */
static void
clutter_actor_transform_and_project_box (ClutterActor          *self,
					 const ClutterActorBox *box,
					 ClutterVertex          verts[4])
{
  ClutterActor          *stage;
  ClutterFixed           mtx[16];
  ClutterFixed           mtx_p[16];
  ClutterFixed           _x, _y, _z, _w;
  ClutterFixed           w[4];
  ClutterFixed           v[4];

  /* We essentially have to dupe some code from clutter_redraw() here
   * to make sure GL Matrices etc are initialised if we're called and we
   * havn't yet rendered anything.
   *
   * Simply duping code for now in wait for Cogl cleanup that can hopefully
   * address this in a nicer way.
  */
  stage = clutter_actor_get_stage (self);

  /* FIXME: if were not yet added to a stage, its probably unsafe to
   * return default - idealy the func should fail.
  */
  if (stage == NULL)
    stage = clutter_stage_get_default ();

  clutter_stage_ensure_current (CLUTTER_STAGE (stage));
  _clutter_stage_maybe_setup_viewport (CLUTTER_STAGE (stage));

  cogl_push_matrix();
  _clutter_actor_apply_modelview_transform_recursive (self, NULL);

  cogl_get_modelview_matrix (mtx);

  _x = 0;
  _y = 0;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[0].x = _x;
  verts[0].y = _y;
  verts[0].z = _z;
  w[0] = _w;

  _x = box->x2 - box->x1;
  _y = 0;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[1].x = _x;
  verts[1].y = _y;
  verts[1].z = _z;
  w[1] = _w;

  _x = 0;
  _y = box->y2 - box->y1;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[2].x = _x;
  verts[2].y = _y;
  verts[2].z = _z;
  w[2] = _w;

  _x = box->x2 - box->x1;
  _y = box->y2 - box->y1;
  _z = 0;
  _w = CFX_ONE;

  mtx_transform (mtx, &_x, &_y, &_z, &_w);

  verts[3].x = _x;
  verts[3].y = _y;
  verts[3].z = _z;
  w[3] = _w;

  cogl_pop_matrix();

  cogl_get_projection_matrix (mtx_p);
  cogl_get_viewport (v);

  mtx_transform (mtx_p,
		 &verts[0].x,
		 &verts[0].y,
		 &verts[0].z,
		 &w[0]);

  verts[0].x = MTX_GL_SCALE_X (verts[0].x, w[0], v[2], v[0]);
  verts[0].y = MTX_GL_SCALE_Y (verts[0].y, w[0], v[3], v[1]);
  verts[0].z = MTX_GL_SCALE_Z (verts[0].z, w[0], v[2], v[0]);

  mtx_transform (mtx_p,
		 &verts[1].x,
		 &verts[1].y,
		 &verts[1].z,
		 &w[1]);

  verts[1].x = MTX_GL_SCALE_X (verts[1].x, w[1], v[2], v[0]);
  verts[1].y = MTX_GL_SCALE_Y (verts[1].y, w[1], v[3], v[1]);
  verts[1].z = MTX_GL_SCALE_Z (verts[1].z, w[1], v[2], v[0]);

  mtx_transform (mtx_p,
		 &verts[2].x,
		 &verts[2].y,
		 &verts[2].z,
		 &w[2]);

  verts[2].x = MTX_GL_SCALE_X (verts[2].x, w[2], v[2], v[0]);
  verts[2].y = MTX_GL_SCALE_Y (verts[2].y, w[2], v[3], v[1]);
  verts[2].z = MTX_GL_SCALE_Z (verts[2].z, w[2], v[2], v[0]);

  mtx_transform (mtx_p,
		 &verts[3].x,
		 &verts[3].y,
		 &verts[3].z,
		 &w[3]);

  verts[3].x = MTX_GL_SCALE_X (verts[3].x, w[3], v[2], v[0]);
  verts[3].y = MTX_GL_SCALE_Y (verts[3].y, w[3], v[3], v[1]);
  verts[3].z = MTX_GL_SCALE_Z (verts[3].z, w[3], v[2], v[0]);
}

/**
 * clutter_actor_get_allocation_vertices:
 * @self: A #ClutterActor
 * @ancestor: A #ClutterActor to calculate the vertices against, or %NULL
 *   to use the default #ClutterStage
 * @verts: return location for an array of 4 #ClutterVertex in which
 *   to store the result.
 *
 * Calculates the transformed coordinates of the four corners of the
 * actor in the plane of @ancestor. The returned vertices relate to
 * the #ClutterActorBox coordinates as follows:
 * <itemizedlist>
 *   <listitem><para>v[0] contains (x1, y1)</para></listitem>
 *   <listitem><para>v[1] contains (x2, y1)</para></listitem>
 *   <listitem><para>v[2] contains (x1, y2)</para></listitem>
 *   <listitem><para>v[3] contains (x2, y2)</para></listitem>
 * </itemizedlist>
 *
 * If @ancestor is %NULL the ancestor will be the #ClutterStage. In
 * this case, the coordinates returned will be the coordinates on
 * the stage before the projection is applied. This is different from
 * the behaviour of clutter_actor_get_abs_allocation_vertices().
 *
 * Since: 0.6
 */
void
clutter_actor_get_allocation_vertices (ClutterActor  *self,
                                       ClutterActor  *ancestor,
                                       ClutterVertex  verts[4])
{
  ClutterFixed           v[4];
  ClutterFixed           w[4];
  ClutterActorPrivate   *priv;
  ClutterActor          *stage;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (ancestor == NULL || CLUTTER_IS_ACTOR (ancestor));

  priv = self->priv;

  /* We essentially have to dupe some code from clutter_redraw() here
   * to make sure GL Matrices etc are initialised if we're called and we
   * havn't yet rendered anything.
   *
   * Simply duping code for now in wait for Cogl cleanup that can hopefully
   * address this in a nicer way.
  */
  stage = clutter_actor_get_stage (self);

  /* FIXME: if were not yet added to a stage, its probably unsafe to
   * return default - idealy the func should fail.
  */
  if (stage == NULL)
    stage = clutter_stage_get_default ();

  clutter_stage_ensure_current (CLUTTER_STAGE (stage));
  _clutter_stage_maybe_setup_viewport (CLUTTER_STAGE (stage));

  /* if the actor needs to be allocated we force a relayout, so that
   * clutter_actor_transform_vertices_relative() will have valid values
   * to use in the transformations
   */
  if (priv->needs_allocation)
    _clutter_stage_maybe_relayout (stage);

  clutter_actor_transform_vertices_relative (self, ancestor, verts, w);
  cogl_get_viewport (v);

  /*
   * The w[3] parameter should always be 1.0 here, so we ignore it; otherwise
   * we would have to devide the original verts with it.
   */
  verts[0].x = CFX_QMUL ((verts[0].x + CFX_ONE / 2), v[2]);
  verts[0].y = CFX_QMUL ((CFX_ONE / 2 - verts[0].y), v[3]);
  verts[0].z = CFX_QMUL ((verts[0].z + CFX_ONE / 2), v[2]);

  verts[1].x = CFX_QMUL ((verts[1].x + CFX_ONE / 2), v[2]);
  verts[1].y = CFX_QMUL ((CFX_ONE / 2 - verts[1].y), v[3]);
  verts[1].z = CFX_QMUL ((verts[1].z + CFX_ONE / 2), v[2]);

  verts[2].x = CFX_QMUL ((verts[2].x + CFX_ONE / 2), v[2]);
  verts[2].y = CFX_QMUL ((CFX_ONE / 2 - verts[2].y), v[3]);
  verts[2].z = CFX_QMUL ((verts[2].z + CFX_ONE / 2), v[2]);

  verts[3].x = CFX_QMUL ((verts[3].x + CFX_ONE / 2), v[2]);
  verts[3].y = CFX_QMUL ((CFX_ONE / 2 - verts[3].y), v[3]);
  verts[3].z = CFX_QMUL ((verts[3].z + CFX_ONE / 2), v[2]);
}

/**
 * clutter_actor_get_abs_allocation_vertices:
 * @self: A #ClutterActor
 * @verts: Pointer to a location of an array of 4 #ClutterVertex where to
 * store the result.
 *
 * Calculates the transformed screen coordinates of the four corners of
 * the actor; the returned vertices relate to the #ClutterActorBox
 * coordinates  as follows:
 * <itemizedlist>
 *   <listitem><para>v[0] contains (x1, y1)</para></listitem>
 *   <listitem><para>v[1] contains (x2, y1)</para></listitem>
 *   <listitem><para>v[2] contains (x1, y2)</para></listitem>
 *   <listitem><para>v[3] contains (x2, y2)</para></listitem>
 * </itemizedlist>
 *
 * Since: 0.4
 */
void
clutter_actor_get_abs_allocation_vertices (ClutterActor  *self,
                                           ClutterVertex  verts[4])
{
  ClutterActorPrivate   *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  /* if the actor needs to be allocated we force a relayout, so that
   * the actor allocation box will be valid for
   * clutter_actor_transform_and_project_box()
   */
  if (priv->needs_allocation)
    {
      ClutterActor *stage = clutter_actor_get_stage (self);

      /* FIXME: if were not yet added to a stage, its probably unsafe to
       * return default - idealy the func should fail.
       */
      if (stage == NULL)
	stage = clutter_stage_get_default ();

      _clutter_stage_maybe_relayout (stage);
    }

  clutter_actor_transform_and_project_box (self,
					   &self->priv->allocation,
					   verts);
}

/* Applies the transforms associated with this actor to the
 * OpenGL modelview matrix.
 *
 * This function does not push/pop matrix; it is the responsibility
 * of the caller to do so as appropriate
 */
static void
_clutter_actor_apply_modelview_transform (ClutterActor *self)
{
  ClutterActorPrivate *priv = self->priv;
  gboolean             is_stage = CLUTTER_IS_STAGE (self);

  if (!is_stage)
    cogl_translatex (CLUTTER_UNITS_TO_FIXED (priv->allocation.x1),
		     CLUTTER_UNITS_TO_FIXED (priv->allocation.y1),
		     0);

  /*
   * because the rotation involves translations, we must scale before
   * applying the rotations (if we apply the scale after the rotations,
   * the translations included in the rotation are not scaled and so the
   * entire object will move on the screen as a result of rotating it).
   */
  if (priv->scale_x != CFX_ONE || priv->scale_y != CFX_ONE)
    cogl_scale (priv->scale_x, priv->scale_y);

   if (priv->rzang)
    {
      cogl_translatex (CLUTTER_UNITS_TO_FIXED (priv->rzx),
		       CLUTTER_UNITS_TO_FIXED (priv->rzy),
		       0);

      cogl_rotatex (priv->rzang, 0, 0, CFX_ONE);

      cogl_translatex (CLUTTER_UNITS_TO_FIXED (-priv->rzx),
		       CLUTTER_UNITS_TO_FIXED (-priv->rzy),
		       0);
    }

  if (priv->ryang)
    {
      cogl_translatex (CLUTTER_UNITS_TO_FIXED (priv->ryx),
		       0,
		       CLUTTER_UNITS_TO_FIXED (priv->z + priv->ryz));

      cogl_rotatex (priv->ryang, 0, CFX_ONE, 0);

      cogl_translatex (CLUTTER_UNITS_TO_FIXED (-priv->ryx),
		       0,
		       CLUTTER_UNITS_TO_FIXED (-(priv->z + priv->ryz)));
    }

  if (priv->rxang)
    {
      cogl_translatex (0,
		       CLUTTER_UNITS_TO_FIXED (priv->rxy),
		       CLUTTER_UNITS_TO_FIXED (priv->z + priv->rxz));

      cogl_rotatex (priv->rxang, CFX_ONE, 0, 0);

      cogl_translatex (0,
		       CLUTTER_UNITS_TO_FIXED (-priv->rxy),
		       CLUTTER_UNITS_TO_FIXED (-(priv->z + priv->rxz)));
    }

  if (!is_stage && (priv->anchor_x || priv->anchor_y))
    cogl_translatex (CLUTTER_UNITS_TO_FIXED (-priv->anchor_x),
		     CLUTTER_UNITS_TO_FIXED (-priv->anchor_y),
		     0);

  if (priv->z)
    cogl_translatex (0, 0, priv->z);
}

/* Recursively applies the transforms associated with this actor and
 * its ancestors to the OpenGL modelview matrix. Use NULL if you want this
 * to go all the way down to the stage.
 *
 * This function does not push/pop matrix; it is the responsibility
 * of the caller to do so as appropriate
 */
static void
_clutter_actor_apply_modelview_transform_recursive (ClutterActor *self,
						    ClutterActor *ancestor)
{
  ClutterActor *parent, *stage;

  parent = clutter_actor_get_parent (self);

  /*
   * If we reached the ancestor, quit
   * NB: NULL ancestor means the stage, and this will not trigger
   * (as it should not)
   */
  if (self == ancestor)
    return;

  stage = clutter_actor_get_stage (self);

  /* FIXME: if were not yet added to a stage, its probably unsafe to
   * return default - idealy the func should fail.
  */
  if (stage == NULL)
    stage = clutter_stage_get_default ();

  if (parent)
    _clutter_actor_apply_modelview_transform_recursive (parent, ancestor);
  else if (self != stage)
    _clutter_actor_apply_modelview_transform (stage);

  _clutter_actor_apply_modelview_transform (self);
}

/**
 * clutter_actor_paint:
 * @self: A #ClutterActor
 *
 * Renders the actor to display.
 *
 * This function should not be called directly by applications.
 * Call clutter_actor_queue_redraw() to queue paints, instead.
 */
void
clutter_actor_paint (ClutterActor *self)
{
  ClutterActorPrivate *priv;
  ClutterMainContext *context;
  gboolean clip_set = FALSE;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (!CLUTTER_ACTOR_IS_REALIZED (self))
    {
      CLUTTER_NOTE (PAINT, "Attempting realize via paint()");
      clutter_actor_realize(self);

      if (!CLUTTER_ACTOR_IS_REALIZED (self))
	{
	  CLUTTER_NOTE (PAINT, "Attempt failed, aborting paint");
	  return;
	}
    }

  cogl_push_matrix();

  _clutter_actor_apply_modelview_transform (self);

  if (priv->has_clip)
    {
      cogl_clip_set (CLUTTER_UNITS_TO_FIXED (priv->clip[0]),
		     CLUTTER_UNITS_TO_FIXED (priv->clip[1]),
		     CLUTTER_UNITS_TO_FIXED (priv->clip[2]),
		     CLUTTER_UNITS_TO_FIXED (priv->clip[3]));
      clip_set = TRUE;
    }

  context = clutter_context_get_default ();
  if (G_UNLIKELY (context->pick_mode != CLUTTER_PICK_NONE))
    {
      ClutterColor col = { 0, };

      _clutter_id_to_color (clutter_actor_get_gid (self), &col);

      /* Actor will then paint silhouette of itself in supplied
       * color.  See clutter_stage_get_actor_at_pos() for where
       * picking is enabled.
       */
      clutter_actor_pick (self, &col);
    }
  else
    {
      clutter_actor_shader_pre_paint (self, FALSE);

      g_signal_emit (self, actor_signals[PAINT], 0);

      clutter_actor_shader_post_paint (self);
    }

  if (clip_set)
    cogl_clip_unset();

  cogl_pop_matrix();
}

/* fixed point, unit based rotation setter, to be used by
 * set_property() so that we don't lose precision in the
 * center coordinates by converting them to and from units
 */
static inline void
clutter_actor_set_rotation_internal (ClutterActor      *self,
                                     ClutterRotateAxis  axis,
                                     ClutterFixed       angle,
                                     ClutterUnit        center_x,
                                     ClutterUnit        center_y,
                                     ClutterUnit        center_z)
{
  ClutterActorPrivate *priv = self->priv;

  g_object_ref (self);
  g_object_freeze_notify (G_OBJECT (self));

  switch (axis)
    {
    case CLUTTER_X_AXIS:
      priv->rxang = angle;
      priv->rxy = center_y;
      priv->rxz = center_z;
      g_object_notify (G_OBJECT (self), "rotation-angle-x");
      g_object_notify (G_OBJECT (self), "rotation-center-x");
      break;

    case CLUTTER_Y_AXIS:
      priv->ryang = angle;
      priv->ryx = center_x;
      priv->ryz = center_z;
      g_object_notify (G_OBJECT (self), "rotation-angle-y");
      g_object_notify (G_OBJECT (self), "rotation-center-y");
      break;

    case CLUTTER_Z_AXIS:
      priv->rzang = angle;
      priv->rzx = center_x;
      priv->rzy = center_y;
      g_object_notify (G_OBJECT (self), "rotation-angle-z");
      g_object_notify (G_OBJECT (self), "rotation-center-z");
      break;
    }

  g_object_thaw_notify (G_OBJECT (self));
  g_object_unref (self);

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);
}

static void
clutter_actor_set_property (GObject      *object,
			    guint         prop_id,
			    const GValue *value,
			    GParamSpec   *pspec)
{

  ClutterActor        *actor;
  ClutterActorPrivate *priv;

  actor = CLUTTER_ACTOR(object);
  priv = actor->priv;

  switch (prop_id)
    {
    case PROP_X:
      clutter_actor_set_x (actor, g_value_get_int (value));
      break;
    case PROP_Y:
      clutter_actor_set_y (actor, g_value_get_int (value));
      break;
    case PROP_WIDTH:
      clutter_actor_set_width (actor, g_value_get_int (value));
      break;
    case PROP_HEIGHT:
      clutter_actor_set_height (actor, g_value_get_int (value));
      break;
    case PROP_FIXED_X:
      clutter_actor_set_xu (actor, clutter_value_get_unit (value));
      break;
    case PROP_FIXED_Y:
      clutter_actor_set_yu (actor, clutter_value_get_unit (value));
      break;
    case PROP_FIXED_POSITION_SET:
      clutter_actor_set_fixed_position_set (actor, g_value_get_boolean (value));
      break;
    case PROP_MIN_WIDTH:
      clutter_actor_set_min_width (actor, clutter_value_get_unit (value));
      break;
    case PROP_MIN_HEIGHT:
      clutter_actor_set_min_height (actor, clutter_value_get_unit (value));
      break;
    case PROP_NATURAL_WIDTH:
      clutter_actor_set_natural_width (actor, clutter_value_get_unit (value));
      break;
    case PROP_NATURAL_HEIGHT:
      clutter_actor_set_natural_height (actor, clutter_value_get_unit (value));
      break;
    case PROP_MIN_WIDTH_SET:
      clutter_actor_set_min_width_set (actor, g_value_get_boolean (value));
      break;
    case PROP_MIN_HEIGHT_SET:
      clutter_actor_set_min_height_set (actor, g_value_get_boolean (value));
      break;
    case PROP_NATURAL_WIDTH_SET:
      clutter_actor_set_natural_width_set (actor, g_value_get_boolean (value));
      break;
    case PROP_NATURAL_HEIGHT_SET:
      clutter_actor_set_natural_height_set (actor, g_value_get_boolean (value));
      break;
    case PROP_REQUEST_MODE:
      clutter_actor_set_request_mode (actor, g_value_get_enum (value));
      break;
    case PROP_DEPTH:
      clutter_actor_set_depth (actor, g_value_get_int (value));
      break;
    case PROP_OPACITY:
      clutter_actor_set_opacity (actor, g_value_get_uchar (value));
      break;
    case PROP_NAME:
      clutter_actor_set_name (actor, g_value_get_string (value));
      break;
    case PROP_VISIBLE:
      if (g_value_get_boolean (value) == TRUE)
	clutter_actor_show (actor);
      else
	clutter_actor_hide (actor);
      break;
    case PROP_SCALE_X:
      clutter_actor_set_scalex
                         (actor,
			  CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value)),
			  priv->scale_y);
      break;
    case PROP_SCALE_Y:
      clutter_actor_set_scalex
                         (actor,
			  priv->scale_x,
			  CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value)));
      break;
    case PROP_CLIP:
      {
        ClutterGeometry *geom = g_value_get_boxed (value);

	clutter_actor_set_clip (actor,
				geom->x, geom->y,
				geom->width, geom->height);
      }
      break;
    case PROP_REACTIVE:
      clutter_actor_set_reactive (actor, g_value_get_boolean (value));
      break;
    case PROP_ROTATION_ANGLE_X:
      {
        ClutterFixed angle;

        angle = CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value));
        clutter_actor_set_rotation_internal (actor,
                                             CLUTTER_X_AXIS,
                                             angle,
                                             0,
                                             priv->rxy,
                                             priv->rxz);
      }
      break;
    case PROP_ROTATION_ANGLE_Y:
      {
        ClutterFixed angle;

        angle = CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value));
        clutter_actor_set_rotation_internal (actor,
                                             CLUTTER_Y_AXIS,
                                             angle,
                                             priv->ryx,
                                             0,
                                             priv->ryz);
      }
      break;
    case PROP_ROTATION_ANGLE_Z:
      {
        ClutterFixed angle;

        angle = CLUTTER_FLOAT_TO_FIXED (g_value_get_double (value));
        clutter_actor_set_rotation_internal (actor,
                                             CLUTTER_Z_AXIS,
                                             angle,
                                             priv->rzx,
                                             priv->rzy,
                                             0);
      }
      break;
    case PROP_ROTATION_CENTER_X:
      {
        ClutterVertex *center;

        center = g_value_get_boxed (value);
        clutter_actor_set_rotation_internal (actor,
                                             CLUTTER_X_AXIS,
                                             priv->rxang,
                                             0,
                                             center->y,
                                             center->z);
      }
      break;
    case PROP_ROTATION_CENTER_Y:
      {
        ClutterVertex *center;

        center = g_value_get_boxed (value);
        clutter_actor_set_rotation_internal (actor,
                                             CLUTTER_Y_AXIS,
                                             priv->ryang,
                                             center->x,
                                             0,
                                             center->z);
      }
      break;
    case PROP_ROTATION_CENTER_Z:
      {
        ClutterVertex *center;

        center = g_value_get_boxed (value);
        clutter_actor_set_rotation_internal (actor,
                                             CLUTTER_Z_AXIS,
                                             priv->rzang,
                                             center->x,
                                             center->y,
                                             0);
      }
      break;
    case PROP_ANCHOR_X:
      priv->anchor_x = CLUTTER_UNITS_FROM_DEVICE (g_value_get_int (value));
      break;
    case PROP_ANCHOR_Y:
      priv->anchor_y = CLUTTER_UNITS_FROM_DEVICE (g_value_get_int (value));
      break;
    case PROP_SHOW_ON_SET_PARENT:
      priv->show_on_set_parent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clutter_actor_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  ClutterActor        *actor;
  ClutterActorPrivate *priv;

  actor = CLUTTER_ACTOR(object);
  priv = actor->priv;

  switch (prop_id)
    {
    case PROP_X:
      g_value_set_int (value, clutter_actor_get_x (actor));
      break;
    case PROP_Y:
      g_value_set_int (value, clutter_actor_get_y (actor));
      break;
    case PROP_WIDTH:
      g_value_set_int (value, clutter_actor_get_width (actor));
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, clutter_actor_get_height (actor));
      break;
    case PROP_FIXED_X:
      clutter_value_set_unit (value, priv->fixed_x);
      break;
    case PROP_FIXED_Y:
      clutter_value_set_unit (value, priv->fixed_y);
      break;
    case PROP_FIXED_POSITION_SET:
      g_value_set_boolean (value, priv->position_set);
      break;
    case PROP_MIN_WIDTH:
      clutter_value_set_unit (value, priv->request_min_width);
      break;
    case PROP_MIN_HEIGHT:
      clutter_value_set_unit (value, priv->request_min_height);
      break;
    case PROP_NATURAL_WIDTH:
      clutter_value_set_unit (value, priv->request_natural_width);
      break;
    case PROP_NATURAL_HEIGHT:
      clutter_value_set_unit (value, priv->request_natural_height);
      break;
    case PROP_MIN_WIDTH_SET:
      g_value_set_boolean (value, priv->min_width_set);
      break;
    case PROP_MIN_HEIGHT_SET:
      g_value_set_boolean (value, priv->min_height_set);
      break;
    case PROP_NATURAL_WIDTH_SET:
      g_value_set_boolean (value, priv->natural_width_set);
      break;
    case PROP_NATURAL_HEIGHT_SET:
      g_value_set_boolean (value, priv->natural_height_set);
      break;
    case PROP_REQUEST_MODE:
      g_value_set_enum (value, priv->request_mode);
      break;
    case PROP_ALLOCATION:
      g_value_set_boxed (value, &priv->allocation);
      break;
    case PROP_DEPTH:
      g_value_set_int (value, clutter_actor_get_depth (actor));
      break;
    case PROP_OPACITY:
      g_value_set_uchar (value, priv->opacity);
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value,
		           (CLUTTER_ACTOR_IS_VISIBLE (actor) != FALSE));
      break;
    case PROP_HAS_CLIP:
      g_value_set_boolean (value, priv->has_clip);
      break;
    case PROP_CLIP:
      {
        ClutterGeometry clip = { 0, };

        clip.x      = CLUTTER_UNITS_TO_DEVICE (priv->clip[0]);
        clip.y      = CLUTTER_UNITS_TO_DEVICE (priv->clip[1]);
        clip.width  = CLUTTER_UNITS_TO_DEVICE (priv->clip[2]);
        clip.height = CLUTTER_UNITS_TO_DEVICE (priv->clip[3]);

        g_value_set_boxed (value, &clip);
      }
      break;
    case PROP_SCALE_X:
      g_value_set_double (value, CLUTTER_FIXED_TO_DOUBLE (priv->scale_x));
      break;
    case PROP_SCALE_Y:
      g_value_set_double (value, CLUTTER_FIXED_TO_DOUBLE (priv->scale_y));
      break;
    case PROP_REACTIVE:
      g_value_set_boolean (value, clutter_actor_get_reactive (actor));
      break;
    case PROP_ROTATION_ANGLE_X:
      g_value_set_double (value, CLUTTER_FIXED_TO_DOUBLE (priv->rxang));
      break;
    case PROP_ROTATION_ANGLE_Y:
      g_value_set_double (value, CLUTTER_FIXED_TO_DOUBLE (priv->ryang));
      break;
    case PROP_ROTATION_ANGLE_Z:
      g_value_set_double (value, CLUTTER_FIXED_TO_DOUBLE (priv->rzang));
      break;
    case PROP_ROTATION_CENTER_X:
      {
        ClutterVertex center = { 0, };

        center.y = priv->rxy;
        center.z = priv->rxz;

        g_value_set_boxed (value, &center);
      }
      break;
    case PROP_ROTATION_CENTER_Y:
      {
        ClutterVertex center = { 0, };

        center.x = priv->ryx;
        center.z = priv->ryz;

        g_value_set_boxed (value, &center);
      }
      break;
    case PROP_ROTATION_CENTER_Z:
      {
        ClutterVertex center = { 0, };

        center.x = priv->rzx;
        center.y = priv->rzy;

        g_value_set_boxed (value, &center);
      }
      break;
    case PROP_ANCHOR_X:
      g_value_set_int (value, CLUTTER_UNITS_TO_DEVICE (priv->anchor_x));
      break;
    case PROP_ANCHOR_Y:
      g_value_set_int (value, CLUTTER_UNITS_TO_DEVICE (priv->anchor_y));
      break;
    case PROP_SHOW_ON_SET_PARENT:
      g_value_set_boolean (value, priv->show_on_set_parent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clutter_actor_dispose (GObject *object)
{
  ClutterActor *self = CLUTTER_ACTOR (object);
  ClutterActorPrivate *priv = self->priv;

  CLUTTER_NOTE (MISC, "Disposing of object (id=%d) of type `%s' (ref_count:%d)",
		self->priv->id,
		g_type_name (G_OBJECT_TYPE (self)),
                object->ref_count);

  /* avoid recursing when called from clutter_actor_destroy() */
  if (priv->parent_actor)
    {
      ClutterActor *parent = priv->parent_actor;

      if (CLUTTER_IS_CONTAINER (parent))
        clutter_container_remove_actor (CLUTTER_CONTAINER (parent), self);
      else
        priv->parent_actor = NULL;
    }

  clutter_actor_unrealize (self);

  destroy_shader_data (self);

  g_signal_emit (self, actor_signals[DESTROY], 0);

  G_OBJECT_CLASS (clutter_actor_parent_class)->dispose (object);
}

static void
clutter_actor_finalize (GObject *object)
{
  ClutterActor *actor = CLUTTER_ACTOR (object);

  CLUTTER_NOTE (MISC, "Finalize object (id=%d) of type `%s'",
		actor->priv->id,
		g_type_name (G_OBJECT_TYPE (actor)));

  g_free (actor->priv->name);
  clutter_id_pool_remove (CLUTTER_CONTEXT()->id_pool, actor->priv->id);

  G_OBJECT_CLASS (clutter_actor_parent_class)->finalize (object);
}

static void
clutter_actor_class_init (ClutterActorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clutter_actor_set_property;
  object_class->get_property = clutter_actor_get_property;
  object_class->dispose      = clutter_actor_dispose;
  object_class->finalize     = clutter_actor_finalize;

  g_type_class_add_private (klass, sizeof (ClutterActorPrivate));

  /**
   * ClutterActor:x:
   *
   * X coordinate of the actor in pixels. If written, forces a fixed
   * position for the actor. If read, returns the fixed position if any,
   * otherwise the allocation if available, otherwise 0.
   */
  g_object_class_install_property (object_class,
    PROP_X,
    g_param_spec_int ("x",
                      "X coordinate",
                      "X coordinate of the actor",
                      -G_MAXINT, G_MAXINT,
                      0,
                      CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:y:
   *
   * Y coordinate of the actor in pixels. If written, forces a fixed
   * position for the actor.  If read, returns the fixed position if
   * any, otherwise the allocation if available, otherwise 0.
   */
  g_object_class_install_property (object_class,
    PROP_Y,
    g_param_spec_int ("y",
                      "Y coordinate",
                      "Y coordinate of the actor",
                      -G_MAXINT, G_MAXINT,
                      0,
                      CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:width:
   *
   * Width of the actor (in pixels). If written, forces the minimum and
   * natural size request of the actor to the given width. If read, returns
   * the allocated width if available, otherwise the width request.
   */
  g_object_class_install_property (object_class,
    PROP_WIDTH,
    g_param_spec_int ("width",
                      "Width",
                      "Width of the actor",
                      0, G_MAXINT,
                      0,
                      CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:height:
   *
   * Height of the actor (in pixels).  If written, forces the minimum and
   * natural size request of the actor to the given height. If read, returns
   * the allocated height if available, otherwise the height request.
   */
  g_object_class_install_property (object_class,
    PROP_HEIGHT,
    g_param_spec_int ("height",
                      "Height",
                      "Height of the actor",
                      0, G_MAXINT,
                      0,
                      CLUTTER_PARAM_READWRITE));

  /**
   * ClutterActor:fixed-x
   *
   * The fixed X position of the actor in ClutterUnit<!-- -->s. Writing this
   * property sets the fixed-position-set property as well, as a side effect.
   *
   * Since: 0.8
   */
  g_object_class_install_property
                   (object_class,
                    PROP_FIXED_X,
                    clutter_param_spec_unit ("fixed-x",
                                             "Fixed X",
                                             "Forced X position of the actor",
                                             CLUTTER_MINUNIT, CLUTTER_MAXUNIT,
                                             0,
                                             CLUTTER_PARAM_READWRITE));

  /**
   * ClutterActor:fixed-y
   *
   * The fixed Y position of the actor in ClutterUnit<!-- -->s. Writing
   * this property sets the fixed-position-set property as well, as a side
   * effect.
   *
   * Since: 0.8
   */
  g_object_class_install_property
                   (object_class,
                    PROP_FIXED_Y,
                    clutter_param_spec_unit ("fixed-y",
                                             "Fixed Y",
                                             "Forced Y position of the actor",
                                             CLUTTER_MINUNIT, CLUTTER_MAXUNIT,
                                             0,
                                             CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:fixed-position-set
   *
   * This flag controls whether the fixed-x and fixed-y properties are used.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_FIXED_POSITION_SET,
         g_param_spec_boolean ("fixed-position-set",
                               "Fixed position set",
                               "Whether to use fixed positioning for the actor",
                               FALSE,
                               CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:min-width
   *
   * A forced minimum width request for the actor, in ClutterUnit<!-- -->s.
   * Writing this property sets the min-width-set property as well, as a side
   * effect. This property overrides the usual width request of the actor.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_MIN_WIDTH,
         clutter_param_spec_unit ("min-width",
                                  "Min Width",
                                  "Forced minimum width request for the actor",
                                  0, CLUTTER_MAXUNIT,
                                  0,
                                  CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:min-height
   *
   * A forced minimum height request for the actor, in
   * ClutterUnit<!-- -->s. Writing this property sets the min-height-set
   * property as well, as a side effect. This property overrides the usual
   * height request of the actor.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_MIN_HEIGHT,
         clutter_param_spec_unit ("min-height",
                                  "Min Height",
                                  "Forced minimum height request for the actor",
                                  0, CLUTTER_MAXUNIT,
                                  0,
                                  CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:natural-width
   *
   * A forced natural width request for the actor, in ClutterUnit<!-- -->s.
   * Writing this property sets the natural-width-set property as
   * well, as a side effect. This property overrides the usual width request
   * of the actor.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_NATURAL_WIDTH,
         clutter_param_spec_unit ("natural-width",
                                  "Natural Width",
                                  "Forced natural width request for the actor",
                                  0, CLUTTER_MAXUNIT,
                                  0,
                                  CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:natural-height
   *
   * A forced natural height request for the actor, in ClutterUnit<!-- -->s.
   * Writing this property sets the natural-height-set property as well, as
   * a side effect. This property overrides the usual height request
   * of the actor.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_NATURAL_HEIGHT,
         clutter_param_spec_unit ("natural-height",
                                  "Natural Height",
                                  "Forced natural height request for the actor",
                                  0, CLUTTER_MAXUNIT,
                                  0,
                                  CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:min-width-set
   *
   * This flag controls whether the min-width property is used.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_MIN_WIDTH_SET,
         g_param_spec_boolean ("min-width-set",
                               "Minimum width set",
                               "Whether to use the min-width property",
                               FALSE,
                               CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:min-height-set
   *
   * This flag controls whether the min-height property is used.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_MIN_HEIGHT_SET,
         g_param_spec_boolean ("min-height-set",
                               "Minimum height set",
                               "Whether to use the min-height property",
                               FALSE,
                               CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:natural-width-set
   *
   * This flag controls whether the natural-width property is used.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_NATURAL_WIDTH_SET,
         g_param_spec_boolean ("natural-width-set",
                               "Natural width set",
                               "Whether to use the natural-width property",
                               FALSE,
                               CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:natural-height-set
   *
   * This flag controls whether the natural-height property is used.
   *
   * Since: 0.8
   */
  g_object_class_install_property
        (object_class,
         PROP_NATURAL_HEIGHT_SET,
         g_param_spec_boolean ("natural-height-set",
                               "Natural height set",
                               "Whether to use the natural-height property",
                               FALSE,
                               CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:allocation:
   *
   * The allocation for the actor, in ClutterUnit<!-- -->s. This is
   * read-only, but you might monitor this property to know when an
   * actor moves or resizes.
   *
   * Since: 0.8
   */
  g_object_class_install_property (object_class,
                                   PROP_ALLOCATION,
                                   g_param_spec_boxed ("allocation",
                                                       "Allocation",
                                                       "The actor's allocation",
                                                       CLUTTER_TYPE_ACTOR_BOX,
                                                       CLUTTER_PARAM_READABLE));
  /**
   * ClutterActor:request-mode:
   *
   * Request mode for the #ClutterActor. The request mode determines the
   * type of geometry management used by the actor, either height for width
   * (the default) or width for height.
   *
   * For actors implementing height for width, the parent container should get
   * the preferred width first, and then the preferred height for that width.
   *
   * For actors implementing width for height, the parent container should get
   * the preferred height first, and then the preferred width for that height.
   *
   * For instance:
   *
   * |[
   *   ClutterRequestMode mode;
   *   ClutterUnit natural_width, min_width;
   *   ClutterUnit natural_height, min_height;
   *
   *   g_object_get (G_OBJECT (child), "request-mode", &amp;mode, NULL);
   *   if (mode == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
   *     {
   *       clutter_actor_get_preferred_width (child, -1,
   *                                          &amp;min_width,
   *                                          &amp;natural_width);
   *       clutter_actor_get_preferred_height (child, natural_width,
   *                                           &amp;min_height,
   *                                           &amp;natural_height);
   *     }
   *   else
   *     {
   *       clutter_actor_get_preferred_height (child, -1,
   *                                           &amp;min_height,
   *                                           &amp;natural_height);
   *       clutter_actor_get_preferred_width (child, natural_height,
   *                                          &amp;min_width,
   *                                          &amp;natural_width);
   *     }
   * ]|
   *
   * will retrieve the minimum and natural width and height depending on the
   * preferred request mode of the #ClutterActor "child".
   *
   * The clutter_actor_get_preferred_size() function will implement this
   * check for you.
   *
   * Since: 0.8
   */
  g_object_class_install_property (object_class,
    PROP_REQUEST_MODE,
    g_param_spec_enum ("request-mode",
                       "Request Mode",
                       "The actor's request mode",
                       CLUTTER_TYPE_REQUEST_MODE,
                       CLUTTER_REQUEST_HEIGHT_FOR_WIDTH,
                       CLUTTER_PARAM_READWRITE));

  /**
   * ClutterActor:depth:
   *
   * Depth of the actor.
   *
   * Since: 0.6
   */
  g_object_class_install_property (object_class,
                                   PROP_DEPTH,
                                   g_param_spec_int ("depth",
                                                     "Depth",
                                                     "Depth of actor",
                                                     -G_MAXINT, G_MAXINT,
                                                     0,
                                                     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:opacity:
   *
   * Opacity of the actor.
   */
  g_object_class_install_property (object_class,
                                   PROP_OPACITY,
                                   g_param_spec_uchar ("opacity",
                                                       "Opacity",
                                                       "Opacity of actor",
                                                       0, 0xff,
                                                       0xff,
                                                       CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:visible:
   *
   * Whether the actor is visible or not.
   */
  g_object_class_install_property (object_class,
                                   PROP_VISIBLE,
                                   g_param_spec_boolean ("visible",
                                                         "Visible",
                                                         "Whether the actor is visible or not",
                                                         FALSE,
                                                         CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:reactive:
   *
   * Whether the actor is reactive to events or not.
   *
   * Since: 0.6
   */
  g_object_class_install_property (object_class,
                                   PROP_REACTIVE,
                                   g_param_spec_boolean ("reactive",
                                                         "Reactive",
                                                         "Whether the actor "
                                                         "is reactive to "
                                                         "events or not",
                                                         FALSE,
                                                         CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:has-clip:
   *
   * Whether the actor has the clip property set or not.
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_CLIP,
                                   g_param_spec_boolean ("has-clip",
                                                         "Has Clip",
                                                         "Whether the actor "
                                                         "has a clip set or "
                                                         "not",
                                                         FALSE,
                                                         CLUTTER_PARAM_READABLE));
  /**
   * ClutterActor:clip:
   *
   * The clip region for the actor.
   */
  g_object_class_install_property (object_class,
                                   PROP_CLIP,
                                   g_param_spec_boxed ("clip",
                                                       "Clip",
                                                       "The clip region for "
                                                       "the actor",
                                                       CLUTTER_TYPE_GEOMETRY,
                                                       CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor:name:
   *
   * The name of the actor.
   *
   * Since: 0.2
   */
  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Name of the actor",
                                                        NULL,
                                                        CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::scale-x:
   *
   * The horizontal scale of the actor
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_SCALE_X,
			g_param_spec_double ("scale-x",
					     "Scale-X",
					     "Scale factor on the X axis",
					     0.0,
					     G_MAXDOUBLE,
					     1.0,
					     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::scale-y:
   *
   * The vertical scale of the actor
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_SCALE_Y,
			g_param_spec_double ("scale-y",
					     "Scale-Y",
					     "Scale factor on the Y axis",
					     0.0,
					     G_MAXDOUBLE,
					     1.0,
					     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::rotation-angle-x:
   *
   * The rotation angle on the X axis.
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_ROTATION_ANGLE_X,
			g_param_spec_double ("rotation-angle-x",
					     "Rotation Angle X",
					     "The rotation angle on the X axis",
					     0.0,
					     G_MAXDOUBLE,
					     0.0,
					     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::rotation-angle-y:
   *
   * The rotation angle on the Y axis.
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_ROTATION_ANGLE_Y,
			g_param_spec_double ("rotation-angle-y",
					     "Rotation Angle Y",
					     "The rotation angle on the Y axis",
					     0.0,
					     G_MAXDOUBLE,
					     0.0,
					     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::rotation-angle-z:
   *
   * The rotation angle on the Z axis.
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_ROTATION_ANGLE_Z,
			g_param_spec_double ("rotation-angle-z",
					     "Rotation Angle Z",
					     "The rotation angle on the Z axis",
					     0.0,
					     G_MAXDOUBLE,
					     0.0,
					     CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::rotation-center-x:
   *
   * The rotation center on the X axis.
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_ROTATION_CENTER_X,
			g_param_spec_boxed ("rotation-center-x",
					    "Rotation Center X",
					    "The rotation center on the X axis",
					    CLUTTER_TYPE_VERTEX,
					    CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::rotation-center-y:
   *
   * The rotation center on the Y axis.
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_ROTATION_CENTER_Y,
			g_param_spec_boxed ("rotation-center-y",
					    "Rotation Center Y",
					    "The rotation center on the Y axis",
					    CLUTTER_TYPE_VERTEX,
					    CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::rotation-center-z:
   *
   * The rotation center on the Z axis.
   *
   * Since: 0.6
   */
  g_object_class_install_property
                       (object_class,
			PROP_ROTATION_CENTER_Z,
			g_param_spec_boxed ("rotation-center-z",
					    "Rotation Center Z",
					    "The rotation center on the Z axis",
					    CLUTTER_TYPE_VERTEX,
					    CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::anchor-x:
   *
   * The X coordinate of an actor's anchor point, relative to
   * the actor coordinate space, in pixels.
   *
   * Since: 0.8
   */
  g_object_class_install_property
                       (object_class,
			PROP_ANCHOR_X,
			g_param_spec_int ("anchor-x",
					  "Anchor X",
					  "X coordinate of the anchor point",
					  -G_MAXINT, G_MAXINT,
                                          0,
					  CLUTTER_PARAM_READWRITE));
  /**
   * ClutterActor::anchor-y:
   *
   * The Y coordinate of an actor's anchor point, relative to
   * the actor coordinate space, in pixels.
   *
   * Since: 0.8
   */
  g_object_class_install_property
                       (object_class,
			PROP_ANCHOR_Y,
			g_param_spec_int ("anchor-y",
					  "Anchor Y",
					  "Y coordinate of the anchor point",
					  -G_MAXINT, G_MAXINT,
                                          0,
					  CLUTTER_PARAM_READWRITE));

  /**
   * ClutterActor:show-on-set-parent:
   *
   * If %TRUE, the actor is automatically shown when parented.
   *
   * Calling clutter_actor_hide() on an actor which has not been
   * parented will set this property to %FALSE as a side effect.
   *
   * Since: 0.8
   */
  g_object_class_install_property
                       (object_class,
                        PROP_SHOW_ON_SET_PARENT,
                        g_param_spec_boolean ("show-on-set-parent",
                                              "Show on set parent",
                                              "Whether the actor is shown"
                                              " when parented",
                                              TRUE,
                                              CLUTTER_PARAM_READWRITE));


  /**
   * ClutterActor::destroy:
   * @actor: the object which received the signal
   *
   * The ::destroy signal is emitted when an actor is destroyed,
   * either by direct invocation of clutter_actor_destroy() or
   * when the #ClutterGroup that contains the actor is destroyed.
   *
   * Since: 0.2
   */
  actor_signals[DESTROY] =
    g_signal_new ("destroy",
		  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		  G_STRUCT_OFFSET (ClutterActorClass, destroy),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /**
   * ClutterActor::show:
   * @actor: the object which received the signal
   *
   * The ::show signal is emitted when an actor is visible and
   * rendered on the stage.
   *
   * Since: 0.2
   */
  actor_signals[SHOW] =
    g_signal_new ("show",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (ClutterActorClass, show),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /**
   * ClutterActor::hide:
   * @actor: the object which received the signal
   *
   * The ::hide signal is emitted when an actor is no longer rendered
   * on the stage.
   *
   * Since: 0.2
   */
  actor_signals[HIDE] =
    g_signal_new ("hide",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (ClutterActorClass, hide),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /**
   * ClutterActor::parent-set:
   * @actor: the object which received the signal
   * @old_parent: the previous parent of the actor, or %NULL
   *
   * This signal is emitted when the parent of the actor changes.
   *
   * Since: 0.2
   */
  actor_signals[PARENT_SET] =
    g_signal_new ("parent-set",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClutterActorClass, parent_set),
                  NULL, NULL,
                  clutter_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  CLUTTER_TYPE_ACTOR);

  /**
   * ClutterActor::event:
   * @actor: the actor which received the event
   * @event: a #ClutterEvent
   *
   * The ::event signal is emitted each time an event is received
   * by the @actor. This signal will be emitted on every actor,
   * following the hierarchy chain, until it reaches the top-level
   * container (the #ClutterStage).
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[EVENT] =
    g_signal_new ("event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::button-press-event:
   * @actor: the actor which received the event
   * @event: a #ClutterButtonEvent
   *
   * The ::button-press-event signal is emitted each time a mouse button
   * is pressed on @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[BUTTON_PRESS_EVENT] =
    g_signal_new ("button-press-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, button_press_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::button-release-event:
   * @actor: the actor which received the event
   * @event: a #ClutterButtonEvent
   *
   * The ::button-release-event signal is emitted each time a mouse button
   * is released on @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new ("button-release-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, button_release_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::scroll-event:
   * @actor: the actor which received the event
   * @event: a #ClutterScrollEvent
   *
   * The ::scroll-event signal is emitted each time the mouse is
   * scrolled on @actor
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[SCROLL_EVENT] =
    g_signal_new ("scroll-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, scroll_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::key-press-event:
   * @actor: the actor which received the event
   * @event: a #ClutterKeyEvent
   *
   * The ::key-press-event signal is emitted each time a keyboard button
   * is pressed while @actor has key focus (see clutter_stage_set_key_focus()).
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[KEY_PRESS_EVENT] =
    g_signal_new ("key-press-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, key_press_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::key-release-event:
   * @actor: the actor which received the event
   * @event: a #ClutterKeyEvent
   *
   * The ::key-release-event signal is emitted each time a keyboard button
   * is released while @actor has key focus (see
   * clutter_stage_set_key_focus()).
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[KEY_RELEASE_EVENT] =
    g_signal_new ("key-release-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, key_release_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * ClutterActor::motion-event:
   * @actor: the actor which received the event
   * @event: a #ClutterMotionEvent
   *
   * The ::motion-event signal is emitted each time the mouse pointer is
   * moved over @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[MOTION_EVENT] =
    g_signal_new ("motion-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, motion_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ClutterActor::focus-in:
   * @actor: the actor which now has key focus
   *
   * The ::focus-in signal is emitted when @actor recieves key focus.
   *
   * Since: 0.6
   */
  actor_signals[FOCUS_IN] =
    g_signal_new ("focus-in",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, focus_in),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * ClutterActor::focus-out:
   * @actor: the actor which now has key focus
   *
   * The ::focus-out signal is emitted when @actor loses key focus.
   *
   * Since: 0.6
   */
  actor_signals[FOCUS_OUT] =
    g_signal_new ("focus-out",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, focus_out),
		  NULL, NULL,
		  clutter_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * ClutterActor::enter-event:
   * @actor: the actor which the pointer has entered.
   * @event: a #ClutterCrossingEvent
   *
   * The ::enter-event signal is emitted when the pointer enters the @actor
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[ENTER_EVENT] =
    g_signal_new ("enter-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, enter_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ClutterActor::leave-event:
   * @actor: the actor which the pointer has left
   * @event: a #ClutterCrossingEvent
   *
   * The ::leave-event signal is emitted when the pointer leaves the @actor.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[LEAVE_EVENT] =
    g_signal_new ("leave-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, leave_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ClutterActor::captured-event:
   * @actor: the actor which received the signal
   * @event: a #ClutterEvent
   *
   * The ::captured-event signal is emitted when an event is captured
   * by Clutter. This signal will be emitted starting from the top-level
   * container (the #ClutterStage) to the actor which received the event
   * going down the hierarchy. This signal can be used to intercept every
   * event before the specialized events (like
   * ClutterActor::button-press-event or ::key-released-event) are
   * emitted.
   *
   * Return value: %TRUE if the event has been handled by the actor,
   *   or %FALSE to continue the emission.
   *
   * Since: 0.6
   */
  actor_signals[CAPTURED_EVENT] =
    g_signal_new ("captured-event",
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (ClutterActorClass, captured_event),
		  _clutter_boolean_handled_accumulator, NULL,
		  clutter_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  CLUTTER_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * ClutterActor::paint:
   * @actor: the #ClutterActor that received the signal
   *
   * The ::paint signal is emitted each time an actor is being painted.
   *
   * Subclasses of #ClutterActor should override the class signal handler
   * and paint themselves in that function.
   *
   * It is possible to connect a handler to the ::paint signal in order
   * to set up some custom aspect of a paint.
   *
   * Since: 0.8
   */
  actor_signals[PAINT] =
    g_signal_new (I_("paint"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClutterActorClass, paint),
                  NULL, NULL,
                  clutter_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  /**
   * ClutterActor::realize:
   * @actor: the #ClutterActor that received the signal
   *
   * The ::realize signal is emitted each time an actor is being
   * realized.
   *
   * Since: 0.8
   */
  actor_signals[REALIZE] =
    g_signal_new (I_("realize"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClutterActorClass, realize),
                  NULL, NULL,
                  clutter_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  /**
   * ClutterActor::unrealize:
   * @actor: the #ClutterActor that received the signal
   *
   * The ::unrealize signal is emitted each time an actor is being
   * unrealized.
   *
   * Since: 0.8
   */
  actor_signals[UNREALIZE] =
    g_signal_new (I_("unrealize"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClutterActorClass, unrealize),
                  NULL, NULL,
                  clutter_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->show = clutter_actor_real_show;
  klass->show_all = clutter_actor_show;
  klass->hide = clutter_actor_real_hide;
  klass->hide_all = clutter_actor_hide;
  klass->pick = clutter_actor_real_pick;
  klass->get_preferred_width = clutter_actor_real_get_preferred_width;
  klass->get_preferred_height = clutter_actor_real_get_preferred_height;
  klass->allocate = clutter_actor_real_allocate;
}

static void
clutter_actor_init (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  self->priv = priv = CLUTTER_ACTOR_GET_PRIVATE (self);

  priv->parent_actor = NULL;
  priv->has_clip     = FALSE;
  priv->opacity      = 0xff;
  priv->id           = clutter_id_pool_add (CLUTTER_CONTEXT()->id_pool, self);
  priv->scale_x      = CFX_ONE;
  priv->scale_y      = CFX_ONE;
  priv->shader_data  = NULL;
  priv->show_on_set_parent = TRUE;

  priv->needs_width_request  = TRUE;
  priv->needs_height_request = TRUE;
  priv->needs_allocation     = TRUE;

  memset (priv->clip, 0, sizeof (ClutterUnit) * 4);
}

/**
 * clutter_actor_destroy:
 * @self: a #ClutterActor
 *
 * Destroys an actor.  When an actor is destroyed, it will break any
 * references it holds to other objects.  If the actor is inside a
 * container, the actor will be removed.
 *
 * When you destroy a container, its children will be destroyed as well.
 *
 * Note: you cannot destroy the #ClutterStage returned by
 * clutter_stage_get_default().
 */
void
clutter_actor_destroy (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  g_object_ref (self);

  /* avoid recursion while destroying */
  if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_DESTRUCTION))
    {
      CLUTTER_SET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_DESTRUCTION);

      g_object_run_dispose (G_OBJECT (self));

      CLUTTER_SET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_DESTRUCTION);
    }

  g_object_unref (self);
}

/**
 * clutter_actor_queue_redraw:
 * @self: A #ClutterActor
 *
 * Queues up a redraw of an actor and any children. The redraw occurs
 * once the main loop becomes idle (after the current batch of events
 * has been processed, roughly).
 *
 * Applications rarely need to call this, as redraws are handled
 * automatically by modification functions.
 */
void
clutter_actor_queue_redraw (ClutterActor *self)
{
  ClutterActor *stage;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  /* FIXME: should we check we're visible here? */
  if ((stage = clutter_actor_get_stage (self)) != NULL)
    clutter_stage_queue_redraw (CLUTTER_STAGE (stage));
}

/**
 * clutter_actor_queue_relayout:
 * @self: A #ClutterActor
 *
 * Indicates that the actor's size request or other layout-affecting
 * properties may have changed. This function is used inside #ClutterActor
 * subclass implementations, not by applications directly.
 *
 * Queueing a new layout automatically queues a redraw as well.
 *
 * Since: 0.8
 */
void
clutter_actor_queue_relayout (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  priv = self->priv;

  if (priv->needs_width_request &&
      priv->needs_height_request &&
      priv->needs_allocation)
    return; /* save some cpu cycles */

  priv->needs_width_request  = TRUE;
  priv->needs_height_request = TRUE;
  priv->needs_allocation     = TRUE;

  /* always repaint also */
  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);

  /* We need to go all the way up the hierarchy */
  if (priv->parent_actor)
    clutter_actor_queue_relayout (priv->parent_actor);
}

/**
 * clutter_actor_get_preferred_size:
 * @self: a #ClutterActor
 * @min_width_p: return location for the minimum width, or %NULL
 * @min_height_p: return location for the minimum height, or %NULL
 * @natural_width_p: return location for the natural width, or %NULL
 * @natural_height_p: return location for the natural height, or %NULL
 *
 * Computes the preferred minimum and natural size of an actor, taking into
 * account the actor's geometry management (either height-for-width
 * or width-for-height).
 *
 * The width and height used to compute the preferred height and preferred
 * width are the actor's natural ones.
 *
 * If you need to control the height for the preferred width, or the width for
 * the preferred height, you should use clutter_actor_get_preferred_width()
 * and clutter_actor_get_preferred_height(), and check the actor's preferred
 * geometry management using the #ClutterActor:request-mode property.
 *
 * Since: 0.8
 */
void
clutter_actor_get_preferred_size (ClutterActor *self,
                                  ClutterUnit  *min_width_p,
                                  ClutterUnit  *min_height_p,
                                  ClutterUnit  *natural_width_p,
                                  ClutterUnit  *natural_height_p)
{
  ClutterActorPrivate *priv;
  ClutterUnit for_width, for_height;
  ClutterUnit min_width, min_height;
  ClutterUnit natural_width, natural_height;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  for_width = for_height = 0;
  min_width = min_height = 0;
  natural_width = natural_height = 0;

  if (priv->request_mode == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
    {
      clutter_actor_get_preferred_width (self, -1,
                                         &min_width,
                                         &natural_width);
      clutter_actor_get_preferred_height (self, natural_width,
                                          &min_height,
                                          &natural_height);
    }
  else
    {
      clutter_actor_get_preferred_height (self, -1,
                                          &min_height,
                                          &natural_height);
      clutter_actor_get_preferred_width (self, natural_height,
                                         &min_width,
                                         &natural_width);
    }

  if (min_width_p)
    *min_width_p = min_width;

  if (min_height_p)
    *min_height_p = min_height;

  if (natural_width_p)
    *natural_width_p = natural_width;

  if (natural_height_p)
    *natural_height_p = natural_height;
}

/**
 * clutter_actor_get_preferred_width:
 * @self: A #ClutterActor
 * @for_height: available height when computing the preferred width,
 *   or a negative value to indicate that no height is defined
 * @min_width_p: return location for minimum width, or %NULL
 * @natural_width_p: return location for the natural width, or %NULL
 *
 * Computes the requested minimum and natural widths for an actor,
 * optionally depending on the specified height, or if they are
 * already computed, returns the cached values.
 *
 * An actor may not get its request - depending on the layout
 * manager that's in effect.
 *
 * A request should not incorporate the actor's scale or anchor point;
 * those transformations do not affect layout, only rendering.
 *
 * Since: 0.8
 */
void
clutter_actor_get_preferred_width (ClutterActor *self,
                                   ClutterUnit   for_height,
                                   ClutterUnit  *min_width_p,
                                   ClutterUnit  *natural_width_p)
{
  ClutterActorClass *klass;
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  klass = CLUTTER_ACTOR_GET_CLASS (self);
  priv = self->priv;

  if (priv->needs_width_request ||
      priv->request_width_for_height != for_height)
    {
      ClutterUnit min_width, natural_width;

      min_width = natural_width = 0;

      klass->get_preferred_width (self, for_height,
                                  &min_width,
                                  &natural_width);

      if (natural_width < min_width)
        {
          g_warning ("Actor of type %s reported a natural width of %d (%d px) "
                     "lower than min width %d (%d px)",
                     G_OBJECT_TYPE_NAME (self),
                     natural_width, CLUTTER_UNITS_TO_DEVICE (natural_width),
                     min_width, CLUTTER_UNITS_TO_DEVICE (min_width));
        }

      if (!priv->min_width_set)
        priv->request_min_width = min_width;

      if (!priv->natural_width_set)
        priv->request_natural_width = natural_width;

      priv->request_width_for_height = for_height;
      priv->needs_width_request = FALSE;
    }

  if (min_width_p)
    *min_width_p = priv->request_min_width;

  if (natural_width_p)
    *natural_width_p = priv->request_natural_width;
}

/**
 * clutter_actor_get_preferred_height:
 * @self: A #ClutterActor
 * @for_width: available width to assume in computing desired height,
 *   or a negative value to indicate that no width is defined
 * @min_height_p: return location for minimum height, or %NULL
 * @natural_height_p: return location for natural height, or %NULL
 *
 * Computes the requested minimum and natural heights for an actor,
 * or if they are already computed, returns the cached values.
 *
 * An actor may not get its request - depending on the layout
 * manager that's in effect.
 *
 * A request should not incorporate the actor's scale or anchor point;
 * those transformations do not affect layout, only rendering.
 *
 * Since: 0.8
 */
void
clutter_actor_get_preferred_height (ClutterActor *self,
                                    ClutterUnit   for_width,
                                    ClutterUnit  *min_height_p,
                                    ClutterUnit  *natural_height_p)
{
  ClutterActorClass *klass;
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  klass = CLUTTER_ACTOR_GET_CLASS (self);
  priv = self->priv;

  if (priv->needs_height_request ||
      priv->request_height_for_width != for_width)
    {
      ClutterUnit min_height, natural_height;

      min_height = natural_height = 0;

      klass->get_preferred_height (self, for_width,
                                   &min_height,
                                   &natural_height);

      if (natural_height < min_height)
        {
          g_warning ("Actor of type %s reported a natural height of %d "
                     "(%d px) lower than min height %d (%d px)",
                     G_OBJECT_TYPE_NAME (self),
                     natural_height, CLUTTER_UNITS_TO_DEVICE (natural_height),
                     min_height, CLUTTER_UNITS_TO_DEVICE (min_height));
        }

      if (!priv->min_height_set)
        priv->request_min_height = min_height;

      if (!priv->natural_height_set)
        priv->request_natural_height = natural_height;

      priv->request_height_for_width = for_width;
      priv->needs_height_request = FALSE;
    }

  if (min_height_p)
    *min_height_p = priv->request_min_height;

  if (natural_height_p)
    *natural_height_p = priv->request_natural_height;
}

/**
 * clutter_actor_get_allocation_coords:
 * @self: A #ClutterActor
 * @x_1: x1 coordinate
 * @y_1: y1 coordinate
 * @x_2: x2 coordinate
 * @y_2: y2 coordinate
 *
 * Gets the layout box an actor has been assigned.  The allocation can
 * only be assumed valid inside a paint() method; anywhere else, it
 * may be out-of-date.
 *
 * An allocation does not incorporate the actor's scale or anchor point;
 * those transformations do not affect layout, only rendering.
 *
 * The returned coordinates are in pixels.
 *
 * Since: 0.8
 */
void
clutter_actor_get_allocation_coords (ClutterActor  *self,
                                     gint          *x_1,
                                     gint          *y_1,
                                     gint          *x_2,
                                     gint          *y_2)
{
  ClutterActorBox allocation = { 0, };

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_get_allocation_box (self, &allocation);

  if (x_1)
    *x_1 = CLUTTER_UNITS_TO_DEVICE (allocation.x1);

  if (y_1)
    *y_1 = CLUTTER_UNITS_TO_DEVICE (allocation.y1);

  if (x_2)
    *x_2 = CLUTTER_UNITS_TO_DEVICE (allocation.x2);

  if (y_2)
    *y_2 = CLUTTER_UNITS_TO_DEVICE (allocation.y2);
}

/**
 * clutter_actor_get_allocation_box:
 * @self: A #ClutterActor
 * @box: the function fills this in with the actor's allocation
 *
 * Gets the layout box an actor has been assigned. The allocation can
 * only be assumed valid inside a paint() method; anywhere else, it
 * may be out-of-date.
 *
 * An allocation does not incorporate the actor's scale or anchor point;
 * those transformations do not affect layout, only rendering.
 *
 * <note>Do not call any of the clutter_actor_get_allocation_*() family
 * of functions inside the implementation of the get_preferred_width()
 * or get_preferred_height() virtual functions.</note>
 *
 * Since: 0.8
 */
void
clutter_actor_get_allocation_box (ClutterActor    *self,
                                  ClutterActorBox *box)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  /* FIXME - if needs_allocation=TRUE, we can either 1)
   * g_return_if_fail, which limits calling get_allocation to inside
   * paint() basically; or we can 2) force a layout, which could be
   * expensive if someone calls get_allocation somewhere silly; or we
   * can 3) just return the latest value, allowing it to be
   * out-of-date, and assume people know what they are doing.
   *
   * The least-surprises approach that keeps existing code working is
   * likely to be 2). People can end up doing some inefficient things,
   * though, and in general code that requires 2) is probably broken.
   */

  /* this implements 2) */
  if (G_UNLIKELY (self->priv->needs_allocation))
    {
      ClutterActor *stage = clutter_actor_get_stage (self);

      /* do not queue a relayout on an unparented actor */
      if (stage)
        _clutter_stage_maybe_relayout (stage);
    }

  /* commenting out the code above and just keeping this assigment
   * implements 3)
   */
  *box = self->priv->allocation;
}

/**
 * clutter_actor_get_allocation_geometry:
 * @self: A #ClutterActor
 * @geom: allocation geometry in pixels
 *
 * Gets the layout box an actor has been assigned.  The allocation can
 * only be assumed valid inside a paint() method; anywhere else, it
 * may be out-of-date.
 *
 * An allocation does not incorporate the actor's scale or anchor point;
 * those transformations do not affect layout, only rendering.
 *
 * The returned rectangle is in pixels.
 *
 * Since: 0.8
 */
void
clutter_actor_get_allocation_geometry (ClutterActor    *self,
                                       ClutterGeometry *geom)
{
  int x2, y2;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_get_allocation_coords (self, &geom->x, &geom->y, &x2, &y2);
  geom->width = x2 - geom->x;
  geom->height = y2 - geom->y;
}

/**
 * clutter_actor_allocate:
 * @self: A #ClutterActor
 * @box: new allocation of the actor, in parent-relative coordinates
 * @absolute_origin_changed: whether the position of the parent has
 *   changed in stage coordinates
 *
 * Called by the parent of an actor to assign the actor its size.
 * Should never be called by applications (except when implementing
 * a container or layout manager).
 *
 * Actors can know from their allocation box whether they have moved
 * with respect to their parent actor. The absolute_origin_changed
 * parameter additionally indicates whether the parent has moved with
 * respect to the stage, for example because a grandparent's origin
 * has moved.
 *
 * Since: 0.8
 */
void
clutter_actor_allocate (ClutterActor          *self,
                        const ClutterActorBox *box,
                        gboolean               absolute_origin_changed)
{
  ClutterActorPrivate *priv;
  ClutterActorClass *klass;
  gboolean child_moved;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;
  klass = CLUTTER_ACTOR_GET_CLASS (self);

  child_moved = (box->x1 != priv->allocation.x1 ||
                 box->y1 != priv->allocation.y1);

  /* If we get an allocation "out of the blue"
   * (we did not queue relayout), then we want to
   * ignore it. But if we have needs_allocation set,
   * we want to guarantee that allocate() virtual
   * method is always called, i.e. that queue_relayout()
   * always results in an allocate() invocation on
   * an actor.
   *
   * The optimization here is to avoid re-allocating
   * actors that did not queue relayout and were
   * not moved.
   */

  if (!priv->needs_allocation &&
      !absolute_origin_changed &&
      !child_moved &&
      box->x2 == priv->allocation.x2 &&
      box->y2 == priv->allocation.y2)
    {
      CLUTTER_NOTE (ACTOR, "No allocation needed");
      return;
    }

  /* When absolute_origin_changed is passed in to
   * clutter_actor_allocate(), it indicates whether the parent has its
   * absolute origin moved; when passed in to ClutterActor::allocate()
   * virtual method though, it indicates whether the child has its
   * absolute origin moved.  So we set it to TRUE if child_moved.
   */
  klass->allocate (self, box, absolute_origin_changed || child_moved);
}

/**
 * clutter_actor_set_geometry:
 * @self: A #ClutterActor
 * @geometry: A #ClutterGeometry
 *
 * Sets the actor's fixed position and forces its minimum and natural
 * size, in pixels. This means the untransformed actor will have the
 * given geometry. This is the same as calling clutter_actor_set_position()
 * and clutter_actor_set_size().
 */
void
clutter_actor_set_geometry (ClutterActor          *self,
			    const ClutterGeometry *geometry)
{
  g_object_freeze_notify (G_OBJECT (self));
  clutter_actor_set_position (self, geometry->x, geometry->y);
  clutter_actor_set_size (self, geometry->width, geometry->height);
  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_get_geometry:
 * @self: A #ClutterActor
 * @geometry: A location to store actors #ClutterGeometry
 *
 * Gets the size and position of an actor relative to its parent
 * actor. This is the same as calling clutter_actor_get_position() and
 * clutter_actor_get_size(). It tries to "do what you mean" and get the
 * requested size and position if the actor's allocation is invalid.
 */
void
clutter_actor_get_geometry (ClutterActor    *self,
			    ClutterGeometry *geometry)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_get_position (self, &geometry->x, &geometry->y);
  clutter_actor_get_size (self, &geometry->width, &geometry->height);
}

/**
 * clutter_actor_set_position
 * @self: A #ClutterActor
 * @x: New left position of actor in pixels.
 * @y: New top position of actor in pixels.
 *
 * Sets the actor's fixed position in pixels relative to any parent
 * actor.
 *
 * If a layout manager is in use, this position will override the
 * layout manager and force a fixed position.
 */
void
clutter_actor_set_position (ClutterActor *self,
			    gint          x,
			    gint          y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_freeze_notify (G_OBJECT (self));
  clutter_actor_set_x (self, x);
  clutter_actor_set_y (self, y);
  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_set_positionu
 * @self: A #ClutterActor
 * @x: New left position of actor in #ClutterUnit<!-- -->s
 * @y: New top position of actor in #ClutterUnit<!-- -->s
 *
 * Sets the actor's position in #ClutterUnit<!-- -->s relative to any
 * parent actor.
 *
 * If a layout manager is in use, this position will override the
 * layout manager and force a fixed position.
 *
 * Since: 0.6
 */
void
clutter_actor_set_positionu (ClutterActor *self,
			     ClutterUnit   x,
			     ClutterUnit   y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_freeze_notify (G_OBJECT (self));
  clutter_actor_set_xu (self, x);
  clutter_actor_set_yu (self, y);
  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_get_fixed_position_set:
 * @self: A #ClutterActor
 *
 * Checks whether an actor has a fixed position set (and will thus be
 * unaffected by any layout manager).
 *
 * Return value: %TRUE if the fixed position is set on the actor
 *
 * Since: 0.8
 */
gboolean
clutter_actor_get_fixed_position_set (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  return self->priv->position_set;
}

/**
 * clutter_actor_set_fixed_position_set:
 * @self: A #ClutterActor
 * @is_set: whether to use fixed position
 *
 * Sets whether an actor has a fixed position set (and will thus be
 * unaffected by any layout manager).
 *
 * Since: 0.8
 */
void
clutter_actor_set_fixed_position_set (ClutterActor *self,
                                      gboolean      is_set)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (self->priv->position_set == (is_set != FALSE))
    return;

  self->priv->position_set = is_set != FALSE;
  g_object_notify (G_OBJECT (self), "fixed-position-set");
  clutter_actor_queue_relayout (self);
}

/**
 * clutter_actor_move_by:
 * @self: A #ClutterActor
 * @dx: Distance to move Actor on X axis.
 * @dy: Distance to move Actor on Y axis.
 *
 * Moves an actor by the specified distance relative to its current
 * position in pixels. This function modifies the fixed position of an
 * actor and thus removes it from any layout management. Another way
 * to move an actor is with an anchor point, see
 * clutter_actor_set_anchor_point().
 *
 * Since: 0.2
 */
void
clutter_actor_move_by (ClutterActor *self,
		       gint          dx,
		       gint          dy)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_move_byu (self,
                          CLUTTER_UNITS_FROM_DEVICE (dx),
                          CLUTTER_UNITS_FROM_DEVICE (dy));
}

/**
 * clutter_actor_move_byu:
 * @self: A #ClutterActor
 * @dx: Distance to move Actor on X axis, in #ClutterUnit<!-- -->s.
 * @dy: Distance to move Actor on Y axis, in #ClutterUnit<!-- -->s.
 *
 * Moves an actor by the specified distance relative to its current
 * position.
 *
 * The move is accomplished by setting a fixed position, overriding
 * any layout manager, see clutter_actor_set_positionu().
 *
 * Since: 0.6
 */
void
clutter_actor_move_byu (ClutterActor *self,
                        ClutterUnit   dx,
                        ClutterUnit   dy)
{
  ClutterUnit x, y;
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  x = self->priv->fixed_x;
  y = self->priv->fixed_y;

  clutter_actor_set_positionu (self, x + dx, y + dy);
}

static void
clutter_actor_set_min_width (ClutterActor *self,
                             ClutterUnit   min_width)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  /* if we are setting the size on a top-level actor and the
   * backend only supports static top-levels (e.g. framebuffers)
   * then we ignore the passed value and we override it with
   * the stage implementation's preferred size.
   */
  if ((CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL) &&
      clutter_feature_available (CLUTTER_FEATURE_STAGE_STATIC))
    return;

  if (priv->min_width_set && min_width == priv->request_min_width)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  clutter_actor_store_old_geometry (self, &old);

  priv->request_min_width = min_width;
  g_object_notify (G_OBJECT (self), "min-width");
  clutter_actor_set_min_width_set (self, TRUE);

  clutter_actor_notify_if_geometry_changed (self, &old);

  g_object_thaw_notify (G_OBJECT (self));

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_min_height (ClutterActor *self,
                              ClutterUnit   min_height)

{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  /* if we are setting the size on a top-level actor and the
   * backend only supports static top-levels (e.g. framebuffers)
   * then we ignore the passed value and we override it with
   * the stage implementation's preferred size.
   */
  if ((CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL) &&
      clutter_feature_available (CLUTTER_FEATURE_STAGE_STATIC))
    return;

  if (priv->min_height_set && min_height == priv->request_min_height)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  clutter_actor_store_old_geometry (self, &old);

  priv->request_min_height = min_height;
  g_object_notify (G_OBJECT (self), "min-height");
  clutter_actor_set_min_height_set (self, TRUE);

  clutter_actor_notify_if_geometry_changed (self, &old);

  g_object_thaw_notify (G_OBJECT (self));

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_natural_width (ClutterActor *self,
                                 ClutterUnit   natural_width)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  /* if we are setting the size on a top-level actor and the
   * backend only supports static top-levels (e.g. framebuffers)
   * then we ignore the passed value and we override it with
   * the stage implementation's preferred size.
   */
  if ((CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL) &&
      clutter_feature_available (CLUTTER_FEATURE_STAGE_STATIC))
    return;

  if (priv->natural_width_set &&
      natural_width == priv->request_natural_width)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  clutter_actor_store_old_geometry (self, &old);

  priv->request_natural_width = natural_width;
  g_object_notify (G_OBJECT (self), "natural-width");
  clutter_actor_set_natural_width_set (self, TRUE);

  clutter_actor_notify_if_geometry_changed (self, &old);

  g_object_thaw_notify (G_OBJECT (self));

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_natural_height (ClutterActor *self,
                                  ClutterUnit   natural_height)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  /* if we are setting the size on a top-level actor and the
   * backend only supports static top-levels (e.g. framebuffers)
   * then we ignore the passed value and we override it with
   * the stage implementation's preferred size.
   */
  if ((CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL) &&
      clutter_feature_available (CLUTTER_FEATURE_STAGE_STATIC))
    return;

  if (priv->natural_height_set &&
      natural_height == priv->request_natural_height)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  clutter_actor_store_old_geometry (self, &old);

  priv->request_natural_height = natural_height;
  g_object_notify (G_OBJECT (self), "natural-height");
  clutter_actor_set_natural_height_set (self, TRUE);

  clutter_actor_notify_if_geometry_changed (self, &old);

  g_object_thaw_notify (G_OBJECT (self));

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_min_width_set (ClutterActor *self,
                                 gboolean      use_min_width)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  if (priv->min_width_set == (use_min_width != FALSE))
    return;

  clutter_actor_store_old_geometry (self, &old);

  priv->min_width_set = use_min_width != FALSE;
  g_object_notify (G_OBJECT (self), "min-width-set");

  clutter_actor_notify_if_geometry_changed (self, &old);

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_min_height_set (ClutterActor *self,
                                  gboolean      use_min_height)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  if (priv->min_height_set == (use_min_height != FALSE))
    return;

  clutter_actor_store_old_geometry (self, &old);

  priv->min_height_set = use_min_height != FALSE;
  g_object_notify (G_OBJECT (self), "min-height-set");

  clutter_actor_notify_if_geometry_changed (self, &old);

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_natural_width_set (ClutterActor *self,
                                     gboolean      use_natural_width)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  if (priv->natural_width_set == (use_natural_width != FALSE))
    return;

  clutter_actor_store_old_geometry (self, &old);

  priv->natural_width_set = use_natural_width != FALSE;
  g_object_notify (G_OBJECT (self), "natural-width-set");

  clutter_actor_notify_if_geometry_changed (self, &old);

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_natural_height_set (ClutterActor *self,
                                      gboolean      use_natural_height)
{
  ClutterActorPrivate *priv = self->priv;
  ClutterActorBox old = { 0, };

  if (priv->natural_height_set == (use_natural_height != FALSE))
    return;

  clutter_actor_store_old_geometry (self, &old);

  priv->natural_height_set = use_natural_height != FALSE;
  g_object_notify (G_OBJECT (self), "natural-height-set");

  clutter_actor_notify_if_geometry_changed (self, &old);

  clutter_actor_queue_relayout (self);
}

static void
clutter_actor_set_request_mode (ClutterActor *self,
                                ClutterRequestMode mode)
{
  ClutterActorPrivate *priv = self->priv;

  if (priv->request_mode == mode)
    return;

  priv->request_mode = mode;

  priv->needs_width_request = TRUE;
  priv->needs_height_request = TRUE;

  g_object_notify (G_OBJECT (self), "request-mode");

  clutter_actor_queue_relayout (self);
}

/**
 * clutter_actor_set_size
 * @self: A #ClutterActor
 * @width: New width of actor in pixels, or -1
 * @height: New height of actor in pixels, or -1
 *
 * Sets the actor's size request in pixels. This overrides any
 * "normal" size request the actor would have. For example
 * a text actor might normally request the size of the text;
 * this function would force a specific size instead.
 *
 * If @width and/or @height are -1 the actor will use its
 * "normal" size request instead of overriding it, i.e.
 * you can "unset" the size with -1.
 *
 * This function sets or unsets both the minimum and natural size.
 */
void
clutter_actor_set_size (ClutterActor *self,
			gint          width,
			gint          height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_sizeu (self,
                           CLUTTER_UNITS_FROM_DEVICE (width),
                           CLUTTER_UNITS_FROM_DEVICE (height));
}

/**
 * clutter_actor_set_sizeu
 * @self: A #ClutterActor
 * @width: New width of actor in #ClutterUnit<!-- -->s, or -1
 * @height: New height of actor in #ClutterUnit<!-- -->s, or -1
 *
 * Overrides the actor's size request in #ClutterUnit<!-- -->s. If @width
 * and/or @height are -1 the actor will use its normal size request (the
 * override is removed).
 *
 * This function sets or unsets both the minimum and natural size.
 *
 * Since: 0.6
 */
void
clutter_actor_set_sizeu (ClutterActor *self,
			 ClutterUnit   width,
			 ClutterUnit   height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_freeze_notify (G_OBJECT (self));

  if (width >= 0)
    {
      clutter_actor_set_min_width (self, width);
      clutter_actor_set_natural_width (self, width);
    }
  else
    {
      clutter_actor_set_min_width_set (self, FALSE);
      clutter_actor_set_natural_width_set (self, FALSE);
    }

  if (height >= 0)
    {
      clutter_actor_set_min_height (self, height);
      clutter_actor_set_natural_height (self, height);
    }
  else
    {
      clutter_actor_set_min_height_set (self, FALSE);
      clutter_actor_set_natural_height_set (self, FALSE);
    }

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_get_size:
 * @self: A #ClutterActor
 * @width: return location for the width, or %NULL.
 * @height: return location for the height, or %NULL.
 *
 * This function tries to "do what you mean" and return
 * the size an actor will have. If the actor has a valid
 * allocation, the allocation will be returned; otherwise,
 * the actors natural size request will be returned.
 *
 * If you care whether you get the request vs. the allocation, you
 * should probably call a different function like
 * clutter_actor_get_allocation_coords() or
 * clutter_actor_get_preferred_width().
 *
 * Since: 0.2
 */
void
clutter_actor_get_size (ClutterActor *self,
			guint        *width,
			guint        *height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (width)
    *width = clutter_actor_get_width (self);

  if (height)
    *height = clutter_actor_get_height (self);
}

/**
 * clutter_actor_get_sizeu:
 * @self: A #ClutterActor
 * @width: return location for the width, or %NULL
 * @height: return location for the height, or %NULL
 *
 * This function tries to "do what you mean" and return
 * the size an actor will have. If the actor has a valid
 * allocation, the allocation will be returned; otherwise,
 * the actors natural size request will be returned.
 *
 * If you care whether you get the request vs. the allocation, you
 * should probably call a different function like
 * clutter_actor_get_allocation_coords() or
 * clutter_actor_get_preferred_width().
 *
 * Since: 0.6
 */
void
clutter_actor_get_sizeu (ClutterActor *self,
                         ClutterUnit  *width,
                         ClutterUnit  *height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (width)
    *width = clutter_actor_get_widthu (self);

  if (height)
    *height = clutter_actor_get_heightu (self);
}

/**
 * clutter_actor_get_position:
 * @self: a #ClutterActor
 * @x: return location for the X coordinate, or %NULL
 * @y: return location for the Y coordinate, or %NULL
 *
 * This function tries to "do what you mean" and tell you where the
 * actor is, prior to any transformations. Retrieves the fixed
 * position of an actor in pixels, if one has been set; otherwise, if
 * the allocation is valid, returns the actor's allocated position;
 * otherwise, returns 0,0.
 *
 * The returned position is in pixels.
 *
 * Since: 0.6
 */
void
clutter_actor_get_position (ClutterActor *self,
                            gint         *x,
                            gint         *y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (x)
    *x = clutter_actor_get_x (self);

  if (y)
    *y = clutter_actor_get_y (self);
}

/**
 * clutter_actor_get_positionu:
 * @self: a #ClutterActor
 * @x: return location for the X coordinate, or %NULL
 * @y: return location for the Y coordinate, or %NULL
 *
 * This function tries to "do what you mean" and tell you where the
 * actor is, prior to any transformations. Retrieves the fixed
 * position of an actor in pixels, if one has been set; otherwise, if
 * the allocation is valid, returns the actor's allocated position;
 * otherwise, returns 0,0.
 *
 * The returned position is in #ClutterUnit<!-- -->s.
 *
 * Since: 0.6
 */
void
clutter_actor_get_positionu (ClutterActor *self,
                             ClutterUnit  *x,
                             ClutterUnit  *y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (x)
    *x = clutter_actor_get_xu (self);

  if (y)
    *y = clutter_actor_get_yu (self);
}

/**
 * clutter_actor_get_transformed_positionu
 * @self: A #ClutterActor
 * @x: return location for the X coordinate, or %NULL
 * @y: return location for the Y coordinate, or %NULL
 *
 * Gets the absolute position of an actor, in #ClutterUnit<!-- -->s,
 * relative to the stage.
 *
 * Since: 0.8
 */
void
clutter_actor_get_transformed_positionu (ClutterActor *self,
                                         ClutterUnit  *x,
                                         ClutterUnit  *y)
{
  ClutterVertex v1 = { 0, };
  ClutterVertex v2 = { 0, };

  clutter_actor_apply_transform_to_point (self, &v1, &v2);

  if (x)
    *x = v2.x;
  if (y)
    *y = v2.y;
}

/**
 * clutter_actor_get_transformed_position
 * @self: A #ClutterActor
 * @x: return location for the X coordinate, or %NULL
 * @y: return location for the Y coordinate, or %NULL
 *
 * Gets the absolute position of an actor, in pixels, relative
 * to the stage.
 *
 * Since: 0.8
 */
void
clutter_actor_get_transformed_position (ClutterActor *self,
                                        gint         *x,
                                        gint         *y)
{
  ClutterUnit xu, yu;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  xu = yu = 0;
  clutter_actor_get_transformed_positionu (self, &xu, &yu);

  if (x)
    *x = CLUTTER_UNITS_TO_DEVICE (xu);
  if (y)
    *y = CLUTTER_UNITS_TO_DEVICE (yu);
}

/**
 * clutter_actor_get_transformed_sizeu:
 * @self: A #ClutterActor
 * @width: return location for the width, or %NULL
 * @height: return location for the height, or %NULL
 *
 * Gets the absolute size of an actor in #ClutterUnits<!-- -->s, taking
 * into account the scaling factors.
 *
 * If the actor has a valid allocation, the allocated size will be used.
 * If the actor has not a valid allocation then the preferred size will
 * be transformed and returned.
 *
 * If you want the transformed allocation, see
 * clutter_actor_get_abs_allocation_vertices() instead.
 *
 * <note>When the actor (or one of its ancestors) is rotated around the
 * X or Y axis, it no longer appears as on the stage as a rectangle, but
 * as a generic quadrangle; in that case this function returns the size
 * of the smallest rectangle that encapsulates the entire quad. Please
 * note that in this case no assumptions can be made about the relative
 * position of this envelope to the absolute position of the actor, as
 * returned by clutter_actor_get_transformed_position(); if you need this
 * information, you need to use clutter_actor_get_abs_allocation_vertices()
 * to get the coords of the actual quadrangle.</note>
 *
 * Since: 0.8
 */
void
clutter_actor_get_transformed_sizeu (ClutterActor *self,
                                     ClutterUnit  *width,
                                     ClutterUnit  *height)
{
  ClutterActorPrivate *priv;
  ClutterVertex v[4];
  ClutterFixed  x_min, x_max, y_min, y_max;
  gint i;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  /* if the actor hasn't been allocated yet, get the preferred
   * size and transform that
   */
  if (priv->needs_allocation)
    {
      ClutterUnit natural_width, natural_height;
      ClutterActorBox box;

      /* make a fake allocation to transform */
      clutter_actor_get_positionu (self, &box.x1, &box.y1);

      natural_width = natural_height = 0;
      clutter_actor_get_preferred_size (self, NULL, NULL,
                                        &natural_width,
                                        &natural_height);

      box.x2 = box.x1 + natural_width;
      box.y2 = box.y1 + natural_height;
      
      clutter_actor_transform_and_project_box (self, &box, v);
    }
  else
    clutter_actor_get_abs_allocation_vertices (self, v);

  x_min = x_max = v[0].x;
  y_min = y_max = v[0].y;

  for (i = 1; i < G_N_ELEMENTS (v); ++i)
    {
      if (v[i].x < x_min)
	x_min = v[i].x;

      if (v[i].x > x_max)
	x_max = v[i].x;

      if (v[i].y < y_min)
	y_min = v[i].y;

      if (v[i].y > y_max)
	y_max = v[i].y;
    }

  if (width)
    *width  = x_max - x_min;

  if (height)
    *height = y_max - y_min;
}

/**
 * clutter_actor_get_transformed_size:
 * @self: A #ClutterActor
 * @width: return location for the width, or %NULL
 * @height: return location for the height, or %NULL
 *
 * Gets the absolute size of an actor taking into account
 * any scaling factors
 *
 * Since: 0.8
 */
void
clutter_actor_get_transformed_size (ClutterActor *self,
                                    guint        *width,
                                    guint        *height)
{
  ClutterUnit wu, hu;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  wu = hu = 0;
  clutter_actor_get_transformed_sizeu (self, &wu, &hu);

  if (width)
    *width  = CLUTTER_UNITS_TO_DEVICE (wu);

  if (height)
    *height = CLUTTER_UNITS_TO_DEVICE (hu);
}


/**
 * clutter_actor_get_width
 * @self: A #ClutterActor
 *
 * Retrieves the width of a #ClutterActor.
 *
 * If the actor has a valid allocation, this function will return the
 * width of the allocated area given to the actor.
 *
 * If the actor does not have a valid allocation, this function will
 * return the actor's natural width, that is the preferred width of
 * the actor.
 *
 * If you care whether you get the preferred width or the width that
 * has been assigned to the actor, you should probably call a different
 * function like clutter_actor_get_allocation_coords() to retrieve the
 * allocated size or clutter_actor_get_preferred_width() to retrieve the
 * preferred width.
 *
 * If an actor has a fixed width, for instance a width that has been
 * assigned using clutter_actor_set_width(), the width returned will
 * be the same value.
 *
 * Return value: the width of the actor, in pixels
 */
guint
clutter_actor_get_width (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  return CLUTTER_UNITS_TO_DEVICE (clutter_actor_get_widthu (self));
}

/**
 * clutter_actor_get_widthu
 * @self: A #ClutterActor
 *
 * Retrieves the width of a #ClutterActor, in #ClutterUnit<!-- -->s.
 *
 * If the actor has a valid allocation, this function will return the
 * width of the allocated area given to the actor.
 *
 * If the actor does not have a valid allocation, this function will
 * return the actor's natural width, that is the preferred width of
 * the actor.
 *
 * If you care whether you get the preferred width or the width that
 * has been assigned to the actor, you should probably call a different
 * function like clutter_actor_get_allocation_coords() to retrieve the
 * allocated size or clutter_actor_get_preferred_width() to retrieve the
 * preferred width.
 *
 * If an actor has a fixed width, for instance a width that has been
 * assigned using clutter_actor_set_width(), the width returned will
 * be the same value.
 *
 * Return value: the width of the actor, in #ClutterUnit<!-- -->s
 *
 * since: 0.6
 */
ClutterUnit
clutter_actor_get_widthu (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  if (self->priv->needs_allocation)
    {
      ClutterUnit natural_width = 0;

      if (self->priv->request_mode == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
        clutter_actor_get_preferred_width (self, -1, NULL, &natural_width);
      else
        {
          ClutterUnit natural_height = 0;

          clutter_actor_get_preferred_height (self, -1, NULL, &natural_height);
          clutter_actor_get_preferred_width (self, natural_height,
                                             NULL,
                                             &natural_width);
        }

      return natural_width;
    }
  else
    return self->priv->allocation.x2 - self->priv->allocation.x1;
}

/**
 * clutter_actor_get_height
 * @self: A #ClutterActor
 *
 * Retrieves the height of a #ClutterActor.
 *
 * If the actor has a valid allocation, this function will return the
 * height of the allocated area given to the actor.
 *
 * If the actor does not have a valid allocation, this function will
 * return the actor's natural height, that is the preferred height of
 * the actor.
 *
 * If you care whether you get the preferred height or the height that
 * has been assigned to the actor, you should probably call a different
 * function like clutter_actor_get_allocation_coords() to retrieve the
 * allocated size or clutter_actor_get_preferred_height() to retrieve the
 * preferred height.
 *
 * If an actor has a fixed height, for instance a height that has been
 * assigned using clutter_actor_set_height(), the height returned will
 * be the same value.
 *
 * Return value: the height of the actor, in pixels
 */
guint
clutter_actor_get_height (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  return CLUTTER_UNITS_TO_DEVICE (clutter_actor_get_heightu (self));
}

/**
 * clutter_actor_get_heightu
 * @self: A #ClutterActor
 *
 * Retrieves the height of a #ClutterActor, in #ClutterUnit<!-- -->s.
 *
 * If the actor has a valid allocation, this function will return the
 * height of the allocated area given to the actor.
 *
 * If the actor does not have a valid allocation, this function will
 * return the actor's natural height, that is the preferred height of
 * the actor.
 *
 * If you care whether you get the preferred height or the height that
 * has been assigned to the actor, you should probably call a different
 * function like clutter_actor_get_allocation_coords() to retrieve the
 * allocated size or clutter_actor_get_preferred_height() to retrieve the
 * preferred height.
 *
 * If an actor has a fixed height, for instance a height that has been
 * assigned using clutter_actor_set_height(), the height returned will
 * be the same value.
 *
 * Return value: the height of the actor, in #ClutterUnit<!-- -->s
 *
 * since: 0.6
 */
ClutterUnit
clutter_actor_get_heightu (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  if (self->priv->needs_allocation)
    {
      ClutterUnit natural_height = 0;

      if (self->priv->request_mode == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
        {
          ClutterUnit natural_width = 0;

          clutter_actor_get_preferred_width (self, -1, NULL, &natural_width);
          clutter_actor_get_preferred_height (self, natural_width,
                                              NULL, &natural_height);
        }
      else
        clutter_actor_get_preferred_height (self, -1, NULL, &natural_height);

      return natural_height;
    }
  else
    return self->priv->allocation.y2 - self->priv->allocation.y1;
}

/**
 * clutter_actor_set_width
 * @self: A #ClutterActor
 * @width: Requested new width for the actor, in pixels
 *
 * Forces a width on an actor, causing the actor's preferred width
 * and height (if any) to be ignored.
 *
 * This function sets both the minimum and natural size of the actor.
 *
 * since: 0.2
 **/
void
clutter_actor_set_width (ClutterActor *self,
                         guint         width)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_widthu (self, CLUTTER_UNITS_FROM_DEVICE (width));
}

/**
 * clutter_actor_set_widthu
 * @self: A #ClutterActor
 * @width: Requested new width for the actor, in #ClutterUnit<!-- -->s
 *
 * Forces a width on an actor, causing the actor's preferred width
 * and height (if any) to be ignored.
 *
 * This function sets both the minimum and natural size of the actor.
 *
 * since: 0.6
 **/
void
clutter_actor_set_widthu (ClutterActor *self,
                          ClutterUnit   width)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_freeze_notify (G_OBJECT (self));

  clutter_actor_set_min_width (self, width);
  clutter_actor_set_natural_width (self, width);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_set_height
 * @self: A #ClutterActor
 * @height: Requested new height for the actor, in pixels
 *
 * Forces a height on an actor, causing the actor's preferred width
 * and height (if any) to be ignored.
 *
 * This function sets both the minimum and natural size of the actor.
 *
 * since: 0.2
 **/
void
clutter_actor_set_height (ClutterActor *self,
                          guint         height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_heightu (self, CLUTTER_UNITS_FROM_DEVICE (height));
}

/**
 * clutter_actor_set_heightu
 * @self: A #ClutterActor
 * @height: Requested new height for the actor, in #ClutterUnit<!-- -->s
 *
 * Forces a height on an actor, causing the actor's preferred width
 * and height (if any) to be ignored.
 *
 * This function sets both the minimum and natural size of the actor.
 *
 * since: 0.6
 **/
void
clutter_actor_set_heightu (ClutterActor *self,
                          ClutterUnit   height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_freeze_notify (G_OBJECT (self));

  clutter_actor_set_min_height (self, height);
  clutter_actor_set_natural_height (self, height);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_set_x:
 * @self: a #ClutterActor
 * @x: the actor's position on the X axis
 *
 * Sets the actor's X coordinate, relative to its parent, in pixels.
 *
 * Overrides any layout manager and forces a fixed position for
 * the actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_x (ClutterActor *self,
                     gint          x)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_xu (self, CLUTTER_UNITS_FROM_DEVICE (x));
}

/**
 * clutter_actor_set_xu:
 * @self: a #ClutterActor
 * @x: the actor's position on the X axis, in #ClutterUnit<!-- -->s
 *
 * Sets the actor's X coordinate, relative to its parent.
 *
 * Overrides any layout manager and forces a fixed position for
 * the actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_xu (ClutterActor *self,
		      ClutterUnit   x)
{
  ClutterActorBox old = { 0, };

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (self->priv->position_set &&
      self->priv->fixed_x == x)
    return;

  clutter_actor_store_old_geometry (self, &old);

  self->priv->fixed_x = x;
  clutter_actor_set_fixed_position_set (self, TRUE);

  clutter_actor_notify_if_geometry_changed (self, &old);

  clutter_actor_queue_relayout (self);
}

/**
 * clutter_actor_set_y:
 * @self: a #ClutterActor
 * @y: the actor's position on the Y axis
 *
 * Sets the actor's Y coordinate, relative to its parent, in pixels.#
 *
 * Overrides any layout manager and forces a fixed position for
 * the actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_y (ClutterActor *self,
                     gint          y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_yu (self, CLUTTER_UNITS_FROM_DEVICE (y));
}

/**
 * clutter_actor_set_yu:
 * @self: a #ClutterActor
 * @y: the actor's position on the Y axis, in #ClutterUnit<!-- -->s
 *
 * Sets the actor's Y coordinate, relative to its parent.
 *
 * Overrides any layout manager and forces a fixed position for
 * the actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_yu (ClutterActor *self,
		      ClutterUnit   y)
{
  ClutterActorBox old = { 0, };

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (self->priv->position_set &&
      self->priv->fixed_y == y)
    return;

  clutter_actor_store_old_geometry (self, &old);

  self->priv->fixed_y = y;
  clutter_actor_set_fixed_position_set (self, TRUE);

  clutter_actor_notify_if_geometry_changed (self, &old);

  clutter_actor_queue_relayout (self);
}

/**
 * clutter_actor_get_x
 * @self: A #ClutterActor
 *
 * Retrieves the X coordinate of a #ClutterActor.
 *
 * This function tries to "do what you mean", by returning the
 * correct value depending on the actor's state.
 *
 * If the actor has a valid allocation, this function will return
 * the X coordinate of the origin of the allocation box.
 *
 * If the actor has any fixed coordinate set using clutter_actor_set_x(),
 * clutter_actor_set_position() or clutter_actor_set_geometry(), this
 * function will return that coordinate.
 *
 * If both the allocation and a fixed position are missing, this function
 * will return 0.
 *
 * Return value: the X coordinate, in pixels, ignoring any
 *   transformation (i.e. scaling, rotation)
 */
gint
clutter_actor_get_x (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  return CLUTTER_UNITS_TO_DEVICE (clutter_actor_get_xu (self));
}

/**
 * clutter_actor_get_xu
 * @self: A #ClutterActor
 *
 * Retrieves the X coordinate of a #ClutterActor, in #ClutterUnit<!-- -->s.
 *
 * This function tries to "do what you mean", by returning the
 * correct value depending on the actor's state.
 *
 * If the actor has a valid allocation, this function will return
 * the X coordinate of the origin of the allocation box.
 *
 * If the actor has any fixed coordinate set using clutter_actor_set_x(),
 * clutter_actor_set_position() or clutter_actor_set_geometry(), this
 * function will return that coordinate.
 *
 * If both the allocation and a fixed position are missing, this function
 * will return 0.
 *
 * Return value: the X coordinate, in #ClutterUnit<!-- -->s, ignoring
 *   any transformation (i.e. scaling, rotation)
 *
 * Since: 0.6
 */
ClutterUnit
clutter_actor_get_xu (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  if (self->priv->needs_allocation)
    {
      if (self->priv->position_set)
        return self->priv->fixed_x;
      else
        return 0;
    }
  else
    return self->priv->allocation.x1;
}

/**
 * clutter_actor_get_y
 * @self: A #ClutterActor
 *
 * Retrieves the Y coordinate of a #ClutterActor.
 *
 * This function tries to "do what you mean", by returning the
 * correct value depending on the actor's state.
 *
 * If the actor has a valid allocation, this function will return
 * the Y coordinate of the origin of the allocation box.
 *
 * If the actor has any fixed coordinate set using clutter_actor_set_y(),
 * clutter_actor_set_position() or clutter_actor_set_geometry(), this
 * function will return that coordinate.
 *
 * If both the allocation and a fixed position are missing, this function
 * will return 0.
 *
 * Return value: the Y coordinate, in pixels, ignoring any
 *   transformation (i.e. scaling, rotation)
 */
gint
clutter_actor_get_y (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  return CLUTTER_UNITS_TO_DEVICE (clutter_actor_get_yu (self));
}

/**
 * clutter_actor_get_yu
 * @self: A #ClutterActor
 *
 * Retrieves the Y coordinate of a #ClutterActor, in #ClutterUnit<!-- -->s.
 *
 * This function tries to "do what you mean", by returning the
 * correct value depending on the actor's state.
 *
 * If the actor has a valid allocation, this function will return
 * the Y coordinate of the origin of the allocation box.
 *
 * If the actor has any fixed coordinate set using clutter_actor_set_y(),
 * clutter_actor_set_position() or clutter_actor_set_geometry(), this
 * function will return that coordinate.
 *
 * If both the allocation and a fixed position are missing, this function
 * will return 0.
 *
 * Return value: the Y coordinate, in #ClutterUnit<!-- -->s, ignoring
 *   any transformation (i.e. scaling, rotation)
 *
 * Since: 0.6
 */
ClutterUnit
clutter_actor_get_yu (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  if (self->priv->needs_allocation)
    {
      if (self->priv->position_set)
        return self->priv->fixed_y;
      else
        return 0;
    }
  else
    return self->priv->allocation.y1;
}

/**
 * clutter_actor_set_scalex:
 * @self: A #ClutterActor
 * @scale_x: #ClutterFixed factor to scale actor by horizontally.
 * @scale_y: #ClutterFixed factor to scale actor by vertically.
 *
 * Fixed point version of clutter_actor_set_scale().
 *
 * Scales an actor with the given factors. The scaling is always
 * relative to the anchor point.
 */
void
clutter_actor_set_scalex (ClutterActor *self,
			  ClutterFixed  scale_x,
			  ClutterFixed  scale_y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_object_ref (self);
  g_object_freeze_notify (G_OBJECT (self));

  self->priv->scale_x = scale_x;
  g_object_notify (G_OBJECT (self), "scale-x");

  self->priv->scale_y = scale_y;
  g_object_notify (G_OBJECT (self), "scale-y");

  g_object_thaw_notify (G_OBJECT (self));
  g_object_unref (self);

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);
}

/**
 * clutter_actor_set_scale:
 * @self: A #ClutterActor
 * @scale_x: double factor to scale actor by horizontally.
 * @scale_y: double factor to scale actor by vertically.
 *
 * Scales an actor with the given factors. The scaling is always
 * relative to the anchor point.
 *
 * Since: 0.2
 */
void
clutter_actor_set_scale (ClutterActor *self,
			 double        scale_x,
			 double        scale_y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_scalex (self,
			    CLUTTER_FLOAT_TO_FIXED (scale_x),
			    CLUTTER_FLOAT_TO_FIXED (scale_y));
}

/**
 * clutter_actor_get_scalex:
 * @self: A #ClutterActor
 * @scale_x: Location to store horizonal scale factor, or  %NULL.
 * @scale_y: Location to store vertical scale factor, or %NULL.
 *
 * Fixed point version of clutter_actor_get_scale().
 *
 * Retrieves the scale factors of an actor.
 *
 * Since: 0.2
 */
void
clutter_actor_get_scalex (ClutterActor *self,
			  ClutterFixed *scale_x,
			  ClutterFixed *scale_y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (scale_x)
    *scale_x = self->priv->scale_x;

  if (scale_y)
    *scale_y = self->priv->scale_y;
}

/**
 * clutter_actor_get_scale:
 * @self: A #ClutterActor
 * @scale_x: Location to store horizonal float scale factor, or %NULL.
 * @scale_y: Location to store vertical float scale factor, or %NULL.
 *
 * Retrieves an actors scale in floating point.
 *
 * Since: 0.2
 */
void
clutter_actor_get_scale (ClutterActor *self,
			 gdouble      *scale_x,
			 gdouble      *scale_y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  if (scale_x)
    *scale_x = CLUTTER_FIXED_TO_FLOAT (self->priv->scale_x);

  if (scale_y)
    *scale_y = CLUTTER_FIXED_TO_FLOAT (self->priv->scale_y);
}

/**
 * clutter_actor_set_opacity:
 * @self: A #ClutterActor
 * @opacity: New opacity value for the actor.
 *
 * Sets the actor's opacity, with zero being completely transparent and
 * 255 (0xff) being fully opaque.
 */
void
clutter_actor_set_opacity (ClutterActor *self,
			   guint8        opacity)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  self->priv->opacity = opacity;

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);
}

/**
 * clutter_actor_get_paint_opacity:
 * @self: A #ClutterActor
 *
 * Retrieves the absolute opacity of the actor, as it appears on the stage.
 *
 * This function traverses the hierarchy chain and composites the opacity of
 * the actor with that of its parents.
 *
 * This function is intended for subclasses to use in the paint virtual
 * function, to paint themselves with the correct opacity.
 *
 * Return value: The actor opacity value.
 *
 * Since: 0.8
 */
guint8
clutter_actor_get_paint_opacity (ClutterActor *self)
{
  ClutterActorPrivate *priv;
  ClutterActor *parent;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  priv = self->priv;

  parent = priv->parent_actor;

  /* Factor in the actual actors opacity with parents */
  if (G_LIKELY (parent))
    {
      guint8 opacity = clutter_actor_get_paint_opacity (parent);

      if (opacity != 0xff)
        return (opacity * priv->opacity) / 0xff;
    }

  return clutter_actor_get_opacity (self);
}

/**
 * clutter_actor_get_opacity:
 * @self: a #ClutterActor
 *
 * Retrieves the opacity value of an actor, as set by
 * clutter_actor_set_opacity().
 *
 * For retrieving the absolute opacity of the actor inside a paint
 * virtual function, see clutter_actor_get_paint_opacity().
 *
 * Return value: the opacity of the actor
 */
guint8
clutter_actor_get_opacity (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  return self->priv->opacity;
}

/**
 * clutter_actor_set_name:
 * @self: A #ClutterActor
 * @name: Textual tag to apply to actor
 *
 * Sets the given name to @self. The name can be used to identify
 * a #ClutterActor.
 */
void
clutter_actor_set_name (ClutterActor *self,
			const gchar  *name)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  g_free (self->priv->name);
  self->priv->name = g_strdup (name);

  g_object_notify (G_OBJECT (self), "name");
}

/**
 * clutter_actor_get_name:
 * @self: A #ClutterActor
 *
 * Retrieves the name of @self.
 *
 * Return value: the name of the actor, or %NULL. The returned string is
 *   owned by the actor and should not be modified or freed.
 */
G_CONST_RETURN gchar *
clutter_actor_get_name (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), NULL);

  return self->priv->name;
}

/**
 * clutter_actor_get_gid:
 * @self: A #ClutterActor
 *
 * Retrieves the unique id for @self.
 *
 * Return value: Globally unique value for this object instance.
 *
 * Since: 0.6
 */
guint32
clutter_actor_get_gid (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  return self->priv->id;
}

/**
 * clutter_actor_set_depth:
 * @self: a #ClutterActor
 * @depth: Z co-ord
 *
 * Sets the Z co-ordinate of @self to @depth. The Units of which are dependant
 * on the perspective setup.
 */
void
clutter_actor_set_depth (ClutterActor *self,
                         gint          depth)
{
  clutter_actor_set_depthu (self, CLUTTER_UNITS_FROM_DEVICE (depth));
}

/**
 * clutter_actor_set_depthu:
 * @self: a #ClutterActor
 * @depth: Z co-ordinate, in #ClutterUnit<!-- -->s
 *
 * Sets the Z co-ordinate of @self to @depth in #ClutterUnit<!-- -->s, the
 * units of which are dependant on the perspective setup.
 */
void
clutter_actor_set_depthu (ClutterActor *self,
			  ClutterUnit   depth)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (priv->z != depth)
    {
      /* Sets Z value. - FIXME: should invert ?*/
      priv->z = depth;

      if (priv->parent_actor && CLUTTER_IS_CONTAINER (priv->parent_actor))
        {
          ClutterContainer *parent;

          /* We need to resort the container stacking order as to
           * correctly render alpha values.
           *
           * FIXME: This is sub optimal. maybe queue the the sort
           *        before stacking
           */
          parent = CLUTTER_CONTAINER (priv->parent_actor);
          clutter_container_sort_depth_order (parent);
        }

      if (CLUTTER_ACTOR_IS_VISIBLE (self))
        clutter_actor_queue_redraw (self);

      g_object_notify (G_OBJECT (self), "depth");
    }
}

/**
 * clutter_actor_get_depth:
 * @self: a #ClutterActor
 *
 * Retrieves the depth of @self.
 *
 * Return value: the depth of the actor
 */
gint
clutter_actor_get_depth (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), -1);

  return CLUTTER_UNITS_TO_DEVICE (self->priv->z);
}

/**
 * clutter_actor_get_depthu:
 * @self: a #ClutterActor
 *
 * Retrieves the depth of @self.
 *
 * Return value: the depth of the actor, in #ClutterUnit<!-- -->s
 *
 * Since: 0.6
 */
ClutterUnit
clutter_actor_get_depthu (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), -1);

  return self->priv->z;
}

/**
 * clutter_actor_set_rotationu:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @angle: the angle of rotation
 * @x: X coordinate of the rotation center, in #ClutterUnit<!-- -->s
 * @y: Y coordinate of the rotation center, in #ClutterUnit<!-- -->s
 * @z: Z coordinate of the rotation center, in #ClutterUnit<!-- -->s
 *
 * Sets the rotation angle of @self around the given axis.
 *
 * This function is the units based variant of clutter_actor_set_rotation().
 *
 * Since: 0.8
 */
void
clutter_actor_set_rotationu (ClutterActor      *self,
                             ClutterRotateAxis  axis,
                             gdouble            angle,
                             ClutterUnit        x,
                             ClutterUnit        y,
                             ClutterUnit        z)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_rotation_internal (self, axis,
                                       CLUTTER_FLOAT_TO_FIXED (angle),
                                       x, y, z);
}

/**
 * clutter_actor_set_rotationx:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @angle: the angle of rotation
 * @x: X coordinate of the rotation center
 * @y: Y coordinate of the rotation center
 * @z: Z coordinate of the rotation center
 *
 * Sets the rotation angle of @self around the given axis.
 *
 * This function is the fixed point variant of clutter_actor_set_rotation().
 *
 * Since: 0.6
 */
void
clutter_actor_set_rotationx (ClutterActor      *self,
                             ClutterRotateAxis  axis,
                             ClutterFixed       angle,
                             gint               x,
                             gint               y,
                             gint               z)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_rotation_internal (self, axis, angle,
                                       CLUTTER_UNITS_FROM_DEVICE (x),
                                       CLUTTER_UNITS_FROM_DEVICE (y),
                                       CLUTTER_UNITS_FROM_DEVICE (z));
}

/**
 * clutter_actor_set_rotation:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @angle: the angle of rotation
 * @x: X coordinate of the rotation center
 * @y: Y coordinate of the rotation center
 * @z: Z coordinate of the rotation center
 *
 * Sets the rotation angle of @self around the given axis.
 *
 * The rotation center coordinates used depend on the value of @axis:
 * <itemizedlist>
 *   <listitem><para>%CLUTTER_X_AXIS requires @y and @z</para></listitem>
 *   <listitem><para>%CLUTTER_Y_AXIS requires @x and @z</para></listitem>
 *   <listitem><para>%CLUTTER_Z_AXIS requires @x and @y</para></listitem>
 * </itemizedlist>
 *
 * The rotation coordinates are relative to the anchor point of the
 * actor, set using clutter_actor_set_anchor_point(). If no anchor
 * point is set, the upper left corner is assumed as the origin.
 *
 * Since: 0.6
 */
void
clutter_actor_set_rotation (ClutterActor      *self,
                            ClutterRotateAxis  axis,
                            gdouble            angle,
                            gint               x,
                            gint               y,
                            gint               z)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_rotationx (self, axis,
                               CLUTTER_FLOAT_TO_FIXED (angle),
                               x, y, z);
}

/**
 * clutter_actor_get_rotationu:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @x: return value for the X coordinate of the center of rotation,
 *   in #ClutterUnit<!-- -->s
 * @y: return value for the Y coordinate of the center of rotation,
 *   in #ClutterUnit<!-- -->s
 * @z: return value for the Z coordinate of the center of rotation,
 *   in #ClutterUnit<!-- -->s
 *
 * Retrieves the angle and center of rotation on the given axis,
 * set using clutter_actor_set_rotation().
 *
 * This function is the units based variant of clutter_actor_get_rotation().
 *
 * Return value: the angle of rotation
 *
 * Since: 0.8
 */
gdouble
clutter_actor_get_rotationu (ClutterActor      *self,
                             ClutterRotateAxis  axis,
                             ClutterUnit       *x,
                             ClutterUnit       *y,
                             ClutterUnit       *z)
{
  ClutterActorPrivate *priv;
  gdouble retval = 0;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  priv = self->priv;

  switch (axis)
    {
    case CLUTTER_X_AXIS:
      retval = CLUTTER_FIXED_TO_DOUBLE (priv->rxang);
      if (y)
        *y = priv->rxy;
      if (z)
        *z = priv->rxz;
      break;

    case CLUTTER_Y_AXIS:
      retval = CLUTTER_FIXED_TO_DOUBLE (priv->ryang);
      if (x)
        *x = priv->ryx;
      if (z)
        *z = priv->ryz;
      break;

    case CLUTTER_Z_AXIS:
      retval = CLUTTER_FIXED_TO_DOUBLE (priv->rzang);
      if (x)
        *x = priv->rzx;
      if (y)
        *y = priv->rzy;
      break;
    }

  return retval;
}

/**
 * clutter_actor_get_rotationx:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @x: return value for the X coordinate of the center of rotation
 * @y: return value for the Y coordinate of the center of rotation
 * @z: return value for the Z coordinate of the center of rotation
 *
 * Retrieves the angle and center of rotation on the given axis,
 * set using clutter_actor_set_rotation().
 *
 * This function is the fixed point variant of clutter_actor_get_rotation().
 *
 * Return value: the angle of rotation as a fixed point value.
 *
 * Since: 0.6
 */
ClutterFixed
clutter_actor_get_rotationx (ClutterActor      *self,
                             ClutterRotateAxis  axis,
                             gint              *x,
                             gint              *y,
                             gint              *z)
{
  ClutterActorPrivate *priv;
  ClutterFixed retval = 0;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0);

  priv = self->priv;

  switch (axis)
    {
    case CLUTTER_X_AXIS:
      retval = priv->rxang;
      if (y)
        *y = CLUTTER_UNITS_TO_DEVICE (priv->rxy);
      if (z)
        *z = CLUTTER_UNITS_TO_DEVICE (priv->rxz);
      break;

    case CLUTTER_Y_AXIS:
      retval = priv->ryang;
      if (x)
        *x = CLUTTER_UNITS_TO_DEVICE (priv->ryx);
      if (z)
        *z = CLUTTER_UNITS_TO_DEVICE (priv->ryz);
      break;

    case CLUTTER_Z_AXIS:
      retval = priv->rzang;
      if (x)
        *x = CLUTTER_UNITS_TO_DEVICE (priv->rzx);
      if (y)
        *y = CLUTTER_UNITS_TO_DEVICE (priv->rzy);
      break;
    }

  return retval;
}

/**
 * clutter_actor_get_rotation:
 * @self: a #ClutterActor
 * @axis: the axis of rotation
 * @x: return value for the X coordinate of the center of rotation
 * @y: return value for the Y coordinate of the center of rotation
 * @z: return value for the Z coordinate of the center of rotation
 *
 * Retrieves the angle and center of rotation on the given axis,
 * set using clutter_actor_set_angle().
 *
 * The coordinates of the center returned by this function depend on
 * the axis passed.
 *
 * Return value: the angle of rotation.
 *
 * Since: 0.6
 */
gdouble
clutter_actor_get_rotation (ClutterActor      *self,
                            ClutterRotateAxis  axis,
                            gint              *x,
                            gint              *y,
                            gint              *z)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), 0.0);

  return CLUTTER_FIXED_TO_FLOAT (clutter_actor_get_rotationx (self,
                                                              axis,
                                                              x, y, z));
}

/**
 * clutter_actor_set_clipu:
 * @self: A #ClutterActor
 * @xoff: X offset of the clip rectangle, in #ClutterUnit<!-- -->s
 * @yoff: Y offset of the clip rectangle, in #ClutterUnit<!-- -->s
 * @width: Width of the clip rectangle, in #ClutterUnit<!-- -->s
 * @height: Height of the clip rectangle, in #ClutterUnit<!-- -->s
 *
 * Unit-based variant of clutter_actor_set_clip()
 *
 * Sets clip area for @self. The clip area is always computed from the
 * upper left corner of the actor, even if the anchor point is set
 * otherwise.
 *
 * Since: 0.6
 */
void
clutter_actor_set_clipu (ClutterActor *self,
			 ClutterUnit   xoff,
			 ClutterUnit   yoff,
			 ClutterUnit   width,
			 ClutterUnit   height)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  priv->clip[0] = xoff;
  priv->clip[1] = yoff;
  priv->clip[2] = width;
  priv->clip[3] = height;

  priv->has_clip = TRUE;

  g_object_notify (G_OBJECT (self), "has-clip");
  g_object_notify (G_OBJECT (self), "clip");
}

/**
 * clutter_actor_set_clip:
 * @self: A #ClutterActor
 * @xoff: X offset of the clip rectangle, in pixels
 * @yoff: Y offset of the clip rectangle, in pixels
 * @width: Width of the clip rectangle, in pixels
 * @height: Height of the clip rectangle, in pixels
 *
 * Sets clip area in pixels for @self. The clip area is always computed
 * from the upper left corner of the actor, even if the anchor point is
 * set otherwise.
 */
void
clutter_actor_set_clip (ClutterActor *self,
                        gint          xoff,
                        gint          yoff,
                        gint          width,
                        gint          height)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_clipu (self,
                           CLUTTER_UNITS_FROM_DEVICE (xoff),
                           CLUTTER_UNITS_FROM_DEVICE (yoff),
                           CLUTTER_UNITS_FROM_DEVICE (width),
                           CLUTTER_UNITS_FROM_DEVICE (height));
}

/**
 * clutter_actor_remove_clip
 * @self: A #ClutterActor
 *
 * Removes clip area from @self.
 */
void
clutter_actor_remove_clip (ClutterActor *self)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  self->priv->has_clip = FALSE;

  g_object_notify (G_OBJECT (self), "has-clip");
}

/**
 * clutter_actor_has_clip:
 * @self: a #ClutterActor
 *
 * Determines whether the actor has a clip area set or not.
 *
 * Return value: %TRUE if the actor has a clip area set.
 *
 * Since: 0.1.1
 */
gboolean
clutter_actor_has_clip (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  return self->priv->has_clip;
}

/**
 * clutter_actor_get_clipu:
 * @self: a #ClutterActor
 * @xoff: return location for the X offset of the clip rectangle, or %NULL
 * @yoff: return location for the Y offset of the clip rectangle, or %NULL
 * @width: return location for the width of the clip rectangle, or %NULL
 * @height: return location for the height of the clip rectangle, or %NULL
 *
 * Unit-based variant of clutter_actor_get_clip().
 *
 * Gets the clip area for @self, in #ClutterUnit<!-- -->s.
 *
 * Since: 0.6
 */
void
clutter_actor_get_clipu (ClutterActor *self,
                         ClutterUnit  *xoff,
			 ClutterUnit  *yoff,
			 ClutterUnit  *width,
			 ClutterUnit  *height)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (!priv->has_clip)
    return;

  if (xoff)
    *xoff = priv->clip[0];

  if (yoff)
    *yoff = priv->clip[1];

  if (width)
    *width = priv->clip[2];

  if (height)
    *height = priv->clip[3];
}

/**
 * clutter_actor_get_clip:
 * @self: a #ClutterActor
 * @xoff: return location for the X offset of the clip rectangle, or %NULL
 * @yoff: return location for the Y offset of the clip rectangle, or %NULL
 * @width: return location for the width of the clip rectangle, or %NULL
 * @height: return location for the height of the clip rectangle, or %NULL
 *
 * Gets the clip area for @self, in pixels.
 *
 * Since: 0.6
 */
void
clutter_actor_get_clip (ClutterActor *self,
                        gint         *xoff,
                        gint         *yoff,
                        gint         *width,
                        gint         *height)
{
  struct clipu { ClutterUnit x, y, width, height; } c = { 0, };

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_get_clipu (self, &c.x, &c.y, &c.width, &c.height);

  if (xoff)
    *xoff = CLUTTER_UNITS_TO_DEVICE (c.x);

  if (yoff)
    *yoff = CLUTTER_UNITS_TO_DEVICE (c.y);

  if (width)
    *width = CLUTTER_UNITS_TO_DEVICE (c.width);

  if (height)
    *height = CLUTTER_UNITS_TO_DEVICE (c.height);
}

/**
 * clutter_actor_set_parent:
 * @self: A #ClutterActor
 * @parent: A new #ClutterActor parent
 *
 * Sets the parent of @self to @parent.  The opposite function is
 * clutter_actor_unparent().
 *
 * This function should not be used by applications, but by custom
 * container actor subclasses.
 */
void
clutter_actor_set_parent (ClutterActor *self,
		          ClutterActor *parent)
{
  ClutterMainContext *clutter_context;
  ClutterActorPrivate *priv;

  clutter_context = clutter_context_get_default ();

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (CLUTTER_IS_ACTOR (parent));
  g_return_if_fail (self != parent);
  g_return_if_fail (clutter_context != NULL);

  priv = self->priv;

  if (priv->parent_actor != NULL)
    {
      g_warning ("Cannot set a parent on an actor which has a parent.\n"
		 "You must use clutter_actor_unparent() first.\n");
      return;
    }

  if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL)
    {
      g_warning ("Cannot set a parent on a toplevel actor\n");
      return;
    }

  g_object_ref_sink (self);
  priv->parent_actor = parent;

  /* clutter_actor_reparent() will emit ::parent-set for us */
  if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_REPARENT))
    g_signal_emit (self, actor_signals[PARENT_SET], 0, NULL);

  /* the invariant is: if the parent is realized, the we must be
   * realized after set_parent(). the call to clutter_actor_show()
   * will cause this anyway, but we need to maintain the invariant
   * even for actors that have :show-on-set-parent set to FALSE
   */
  if (CLUTTER_ACTOR_IS_REALIZED (priv->parent_actor))
    clutter_actor_realize (self);

  if (priv->show_on_set_parent)
    clutter_actor_show (self);

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->parent_actor) &&
      CLUTTER_ACTOR_IS_VISIBLE (self))
    {
      clutter_actor_queue_redraw (self);
    }

  /* maintain the invariant that if an actor needs layout,
   * its parents do as well
   */
  if (priv->needs_width_request ||
      priv->needs_height_request ||
      priv->needs_allocation)
    {
      clutter_actor_queue_relayout (self);
    }
}

/**
 * clutter_actor_get_parent:
 * @self: A #ClutterActor
 *
 * Retrieves the parent of @self.
 *
 * Return Value: The #ClutterActor parent, or %NULL if no parent is set
 */
ClutterActor *
clutter_actor_get_parent (ClutterActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), NULL);

  return self->priv->parent_actor;
}

/**
 * clutter_actor_unparent:
 * @self: a #ClutterActor
 *
 * Removes the parent of @self.
 *
 * This function should not be used in applications.  It should be called by
 * implementations of container actors, to dissociate a child from the
 * container.
 *
 * Since: 0.1.1
 */
void
clutter_actor_unparent (ClutterActor *self)
{
  ClutterActorPrivate *priv;
  ClutterActor *old_parent;

  gboolean show_on_set_parent_enabled = TRUE;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (priv->parent_actor == NULL)
    return;

  show_on_set_parent_enabled = priv->show_on_set_parent;

  old_parent = priv->parent_actor;
  priv->parent_actor = NULL;

  /* if we are uparenting we hide ourselves; if we are just reparenting
   * there's no need to do that, as the paint is fast enough.
   */
  if (CLUTTER_ACTOR_IS_REALIZED (self))
    {
      if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_REPARENT))
        clutter_actor_hide (self);
    }

  /* clutter_actor_hide() will set the :show-on-set-parent property
   * to FALSE because the actor doesn't have a parent anymore; but
   * we need to return the actor to its initial state, so we force
   * the state of the :show-on-set-parent property to its value
   * previous the unparenting
   */
  priv->show_on_set_parent = show_on_set_parent_enabled;

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);

  /* clutter_actor_reparent() will emit ::parent-set for us */
  if (!(CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IN_REPARENT))
    g_signal_emit (self, actor_signals[PARENT_SET], 0, old_parent);

  /* Queue a redraw on old_parent */
  if (CLUTTER_ACTOR_IS_VISIBLE (old_parent))
    clutter_actor_queue_redraw (old_parent);

  /* Could also need to relayout */
  if (old_parent->priv->needs_width_request ||
      old_parent->priv->needs_height_request ||
      old_parent->priv->needs_allocation)
    {
      clutter_actor_queue_relayout (old_parent);
    }

  /* remove the reference we acquired in clutter_actor_set_parent() */
  g_object_unref (self);
}

/**
 * clutter_actor_reparent:
 * @self: a #ClutterActor
 * @new_parent: the new #ClutterActor parent
 *
 * This function resets the parent actor of @self.  It is
 * logically equivalent to calling clutter_actor_unparent()
 * and clutter_actor_set_parent().
 *
 * Since: 0.2
 */
void
clutter_actor_reparent (ClutterActor *self,
                        ClutterActor *new_parent)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (CLUTTER_IS_ACTOR (new_parent));
  g_return_if_fail (self != new_parent);

  if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL)
    {
      g_warning ("Cannot set a parent on a toplevel actor\n");
      return;
    }

  priv = self->priv;

  if (priv->parent_actor != new_parent)
    {
      ClutterActor *old_parent;

      CLUTTER_SET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_REPARENT);

      old_parent = priv->parent_actor;

      g_object_ref (self);

      if (CLUTTER_IS_CONTAINER (priv->parent_actor))
        {
          ClutterContainer *parent = CLUTTER_CONTAINER (priv->parent_actor);
          /* Note, will call unparent() */
          clutter_container_remove_actor (parent, self);
        }
      else
        clutter_actor_unparent (self);

      if (CLUTTER_IS_CONTAINER (new_parent))
          /* Note, will call parent() */
        clutter_container_add_actor (CLUTTER_CONTAINER (new_parent), self);
      else
        clutter_actor_set_parent (self, new_parent);

      /* we emit the ::parent-set signal once */
      g_signal_emit (self, actor_signals[PARENT_SET], 0, old_parent);

      g_object_unref (self);

      CLUTTER_UNSET_PRIVATE_FLAGS (self, CLUTTER_ACTOR_IN_REPARENT);
   }
}
/**
 * clutter_actor_raise:
 * @self: A #ClutterActor
 * @below: A #ClutterActor to raise above.
 *
 * Puts @self above @below.
 *
 * Both actors must have the same parent.
 *
 * This function is the equivalent of clutter_container_raise_child().
 */
void
clutter_actor_raise (ClutterActor *self,
                     ClutterActor *below)
{
  ClutterActor *parent;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  parent = clutter_actor_get_parent (self);
  if (!parent)
    {
      g_warning ("Actor of type %s is not inside a container",
                 g_type_name (G_OBJECT_TYPE (self)));
      return;
    }

  if (below)
    {
      if (parent != clutter_actor_get_parent (below))
        {
          g_warning ("Actor of type %s is not in the same "
                     "container of actor of type %s",
                     g_type_name (G_OBJECT_TYPE (self)),
                     g_type_name (G_OBJECT_TYPE (below)));
          return;
        }
    }

  clutter_container_raise_child (CLUTTER_CONTAINER (parent), self, below);
}

/**
 * clutter_actor_lower:
 * @self: A #ClutterActor
 * @above: A #ClutterActor to lower below
 *
 * Puts @self below @above.
 *
 * Both actors must have the same parent.
 *
 * This function is the equivalent of clutter_container_lower_child().
 */
void
clutter_actor_lower (ClutterActor *self,
                     ClutterActor *above)
{
  ClutterActor *parent;

  g_return_if_fail (CLUTTER_IS_ACTOR(self));

  parent = clutter_actor_get_parent (self);
  if (!parent)
    {
      g_warning ("Actor of type %s is not inside a container",
                 g_type_name (G_OBJECT_TYPE (self)));
      return;
    }

  if (above)
    {
      if (parent != clutter_actor_get_parent (above))
        {
          g_warning ("Actor of type %s is not in the same "
                     "container of actor of type %s",
                     g_type_name (G_OBJECT_TYPE (self)),
                     g_type_name (G_OBJECT_TYPE (above)));
          return;
        }
    }

  clutter_container_lower_child (CLUTTER_CONTAINER (parent), self, above);
}

/**
 * clutter_actor_raise_top:
 * @self: A #ClutterActor
 *
 * Raises @self to the top.
 *
 * This function calls clutter_actor_raise() internally.
 */
void
clutter_actor_raise_top (ClutterActor *self)
{
  clutter_actor_raise (self, NULL);
}

/**
 * clutter_actor_lower_bottom:
 * @self: A #ClutterActor
 *
 * Lowers @self to the bottom.
 *
 * This function calls clutter_actor_lower() internally.
 */
void
clutter_actor_lower_bottom (ClutterActor *self)
{
  clutter_actor_lower (self, NULL);
}

/*
 * Event handling
 */

/**
 * clutter_actor_event:
 * @actor: a #ClutterActor
 * @event: a #ClutterEvent
 * @capture: TRUE if event in in capture phase, FALSE otherwise.
 *
 * This function is used to emit an event on the main stage.
 * You should rarely need to use this function, except for
 * synthetising events.
 *
 * Return value: the return value from the signal emission: %TRUE
 *   if the actor handled the event, or %FALSE if the event was
 *   not handled
 *
 * Since: 0.6
 */
gboolean
clutter_actor_event (ClutterActor *actor,
                     ClutterEvent *event,
		     gboolean      capture)
{
  gboolean retval = FALSE;
  gint signal_num = -1;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (actor), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  g_object_ref (actor);

  if (capture)
    {
      g_signal_emit (actor, actor_signals[CAPTURED_EVENT], 0,
		     event,
                     &retval);
      goto out;
    }

  g_signal_emit (actor, actor_signals[EVENT], 0, event, &retval);

  if (!retval)
    {
      switch (event->type)
	{
	case CLUTTER_NOTHING:
	  break;
	case CLUTTER_BUTTON_PRESS:
	  signal_num = BUTTON_PRESS_EVENT;
	  break;
	case CLUTTER_BUTTON_RELEASE:
	  signal_num = BUTTON_RELEASE_EVENT;
	  break;
	case CLUTTER_SCROLL:
	  signal_num = SCROLL_EVENT;
	  break;
	case CLUTTER_KEY_PRESS:
	  signal_num = KEY_PRESS_EVENT;
	  break;
	case CLUTTER_KEY_RELEASE:
	  signal_num = KEY_RELEASE_EVENT;
	  break;
	case CLUTTER_MOTION:
	  signal_num = MOTION_EVENT;
	  break;
	case CLUTTER_ENTER:
	  signal_num = ENTER_EVENT;
	  break;
	case CLUTTER_LEAVE:
	  signal_num = LEAVE_EVENT;
	  break;
	case CLUTTER_DELETE:
	case CLUTTER_DESTROY_NOTIFY:
	case CLUTTER_CLIENT_MESSAGE:
	default:
	  signal_num = -1;
	  break;
	}

      if (signal_num != -1)
	g_signal_emit (actor, actor_signals[signal_num], 0,
		       event, &retval);
    }

out:
  g_object_unref (actor);

  return retval;
}

/**
 * clutter_actor_set_reactive:
 * @actor: a #ClutterActor
 * @reactive: whether the actor should be reactive to events
 *
 * Sets @actor as reactive. Reactive actors will receive events.
 *
 * Since: 0.6
 */
void
clutter_actor_set_reactive (ClutterActor *actor,
                            gboolean      reactive)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (actor));

  if (reactive == CLUTTER_ACTOR_IS_REACTIVE (actor))
    return;

  if (reactive)
    CLUTTER_ACTOR_SET_FLAGS (actor, CLUTTER_ACTOR_REACTIVE);
  else
    CLUTTER_ACTOR_UNSET_FLAGS (actor, CLUTTER_ACTOR_REACTIVE);

  g_object_notify (G_OBJECT (actor), "reactive");
}

/**
 * clutter_actor_get_reactive:
 * @actor: a #ClutterActor
 *
 * Checks whether @actor is marked as reactive.
 *
 * Return value: %TRUE if the actor is reactive
 *
 * Since: 0.6
 */
gboolean
clutter_actor_get_reactive (ClutterActor *actor)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (actor), FALSE);

  return CLUTTER_ACTOR_IS_REACTIVE (actor);
}

/**
 * clutter_actor_set_anchor_point:
 * @self: a #ClutterActor
 * @anchor_x: X coordinate of the anchor point
 * @anchor_y: Y coordinate of the anchor point
 *
 * Sets an anchor point for the @actor. The anchor point is a point in the
 * coordinate space of an actor to which the actor position within its
 * parent is relative; the default is (0, 0), i.e. the top-left corner of
 * the actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_anchor_point (ClutterActor *self,
				gint          anchor_x,
                                gint          anchor_y)
{
  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  clutter_actor_set_anchor_pointu (self,
                                   CLUTTER_UNITS_FROM_DEVICE (anchor_x),
                                   CLUTTER_UNITS_FROM_DEVICE (anchor_y));
}

/**
 * clutter_actor_move_anchor_point:
 * @self: a #ClutterActor
 * @anchor_x: X coordinate of the anchor point
 * @anchor_y: Y coordinate of the anchor point
 *
 * Sets an anchor point for the @actor, and adjusts the actor postion so
 * that the relative position of the actor toward its parent remains the
 * same.
 *
 * Since: 0.6
 */
void
clutter_actor_move_anchor_point (ClutterActor *self,
				 gint          anchor_x,
				 gint          anchor_y)
{
  ClutterActorPrivate *priv;
  ClutterUnit ax = CLUTTER_UNITS_FROM_DEVICE (anchor_x);
  ClutterUnit ay = CLUTTER_UNITS_FROM_DEVICE (anchor_y);
  ClutterUnit dx;
  ClutterUnit dy;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  dx = ax - priv->anchor_x;
  dy = ay - priv->anchor_y;

  priv->anchor_x = ax;
  priv->anchor_y = ay;

  if (priv->position_set)
    clutter_actor_move_byu (self, dx, dy);
}

/**
 * clutter_actor_get_anchor_point:
 * @self: a #ClutterActor
 * @anchor_x: return location for the X coordinate of the anchor point
 * @anchor_y: return location for the y coordinate of the anchor point
 *
 * Gets the current anchor point of the @actor in pixels.
 *
 * Since: 0.6
 */
void
clutter_actor_get_anchor_point (ClutterActor *self,
				gint         *anchor_x,
                                gint         *anchor_y)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (anchor_x)
    *anchor_x = CLUTTER_UNITS_TO_DEVICE (priv->anchor_x);

  if (anchor_y)
    *anchor_y = CLUTTER_UNITS_TO_DEVICE (priv->anchor_y);
}

/**
 * clutter_actor_set_anchor_pointu:
 * @self: a #ClutterActor
 * @anchor_x: X coordinate of the anchor point, in #ClutterUnit<!-- -->s
 * @anchor_y: Y coordinate of the anchor point, in #ClutterUnit<!-- -->s
 *
 * Sets an anchor point for @self. The anchor point is a point in the
 * coordinate space of an actor to which the actor position within its
 * parent is relative; the default is (0, 0), i.e. the top-left corner
 * of the actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_anchor_pointu (ClutterActor *self,
				 ClutterUnit   anchor_x,
                                 ClutterUnit   anchor_y)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  g_object_freeze_notify (G_OBJECT (self));

  if (priv->anchor_x != anchor_x)
    {
      priv->anchor_x = anchor_x;
      g_object_notify (G_OBJECT (self), "anchor-x");
    }

  if (priv->anchor_y != anchor_y)
    {
      priv->anchor_y = anchor_y;
      g_object_notify (G_OBJECT (self), "anchor-y");
    }

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * clutter_actor_move_anchor_pointu:
 * @self: a #ClutterActor
 * @anchor_x: X coordinate of the anchor point
 * @anchor_y: Y coordinate of the anchor point
 *
 * Sets an anchor point for the actor, and adjusts the actor postion so that
 * the relative position of the actor toward its parent remains the same.
 *
 * Since: 0.6
 */
void
clutter_actor_move_anchor_pointu (ClutterActor *self,
				  ClutterUnit   anchor_x,
				  ClutterUnit   anchor_y)
{
  ClutterActorPrivate *priv;
  ClutterUnit dx;
  ClutterUnit dy;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  dx = anchor_x - priv->anchor_x;
  dy = anchor_y - priv->anchor_y;

  priv->anchor_x = anchor_x;
  priv->anchor_y = anchor_y;

  if (priv->position_set)
    clutter_actor_move_byu (self, dx, dy);
}

/**
 * clutter_actor_get_anchor_pointu:
 * @self: a #ClutterActor
 * @anchor_x: return location for the X coordinace of the anchor point
 * @anchor_y: return location for the X coordinace of the anchor point
 *
 * Gets the current anchor point of the @actor in #ClutterUnit<!-- -->s.
 *
 * Since: 0.6
 */
void
clutter_actor_get_anchor_pointu (ClutterActor *self,
				 ClutterUnit  *anchor_x,
                                 ClutterUnit  *anchor_y)
{
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  if (anchor_x)
    *anchor_x = priv->anchor_x;

  if (anchor_y)
    *anchor_y = priv->anchor_y;
}

/**
 * clutter_actor_move_anchor_point_from_gravity:
 * @self: a #ClutterActor
 * @gravity: #ClutterGravity.
 *
 * Sets an anchor point on the actor based on the given gravity, adjusting the
 * actor postion so that its relative position within its parent remains
 * unchanged.
 *
 * Since: 0.6
 */
void
clutter_actor_move_anchor_point_from_gravity (ClutterActor   *self,
					      ClutterGravity  gravity)
{
  ClutterUnit ax, ay, dx, dy;
  ClutterActorPrivate *priv;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  ax = priv->anchor_x;
  ay = priv->anchor_y;

  clutter_actor_set_anchor_point_from_gravity (self, gravity);

  dx = priv->anchor_x - ax;
  dy = priv->anchor_y - ay;

  if (priv->position_set)
    {
      clutter_actor_move_byu (self, dx, dy);
    }
}

/**
 * clutter_actor_set_anchor_point_from_gravity:
 * @self: a #ClutterActor
 * @gravity: #ClutterGravity.
 *
 * Sets an anchor point on the actor, based on the given gravity (this is a
 * convenience function wrapping clutter_actor_set_anchor_point()).
 *
 * Since: 0.6
 */
void
clutter_actor_set_anchor_point_from_gravity (ClutterActor   *self,
					     ClutterGravity  gravity)
{
  ClutterActorPrivate *priv;
  ClutterUnit w, h, x, y;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));

  priv = self->priv;

  x = 0;
  y = 0;
  clutter_actor_get_sizeu (self, &w, &h);

  switch (gravity)
    {
    case CLUTTER_GRAVITY_NORTH:
      x = w / 2;
      break;
    case CLUTTER_GRAVITY_SOUTH:
      x = w / 2;
      y = h;
      break;
    case CLUTTER_GRAVITY_EAST:
      x = w;
      y = h / 2;
      break;
    case CLUTTER_GRAVITY_NORTH_EAST:
      x = w;
      break;
    case CLUTTER_GRAVITY_SOUTH_EAST:
      x = w;
      y = h;
      break;
    case CLUTTER_GRAVITY_SOUTH_WEST:
      y = h;
      break;
    case CLUTTER_GRAVITY_WEST:
      y = h / 2;
      break;
    case CLUTTER_GRAVITY_CENTER:
      x = w / 2;
      y = h / 2;
      break;
    case CLUTTER_GRAVITY_NONE:
    case CLUTTER_GRAVITY_NORTH_WEST:
      break;
    }

  clutter_actor_set_anchor_pointu (self, x, y);
}

typedef enum
{
  PARSE_X,
  PARSE_Y,
  PARSE_WIDTH,
  PARSE_HEIGHT,
  PARSE_ANCHOR_X,
  PARSE_ANCHOR_Y
} ParseDimension;

static ClutterUnit
parse_units (ClutterActor   *self,
             ParseDimension  dimension,
             JsonNode       *node)
{
  GValue value = { 0, };
  ClutterUnit retval = 0;

  if (JSON_NODE_TYPE (node) != JSON_NODE_VALUE)
    return 0;

  json_node_get_value (node, &value);

  if (G_VALUE_HOLDS (&value, G_TYPE_INT))
    {
      gint pixels = g_value_get_int (&value);

      retval = CLUTTER_UNITS_FROM_DEVICE (pixels);
    }
  else if (G_VALUE_HOLDS (&value, G_TYPE_STRING))
    {
      gint64 val;
      gchar *end;

      val = g_ascii_strtoll (g_value_get_string (&value), &end, 10);

      /* skip whitespace */
      while (g_ascii_isspace (*end))
        end++;

      /* assume pixels */
      if (*end == '\0')
        {
          retval = CLUTTER_UNITS_FROM_DEVICE (val);
          goto out;
        }

      if (strcmp (end, "px") == 0)
        {
          retval = CLUTTER_UNITS_FROM_DEVICE (val);
          goto out;
        }

      if (strcmp (end, "mm") == 0)
        {
          retval = CLUTTER_UNITS_FROM_MM (val);
          goto out;
        }

      if (strcmp (end, "pt") == 0)
        {
          retval = CLUTTER_UNITS_FROM_POINTS (val);
          goto out;
        }

      if (end[0] == '%' && end[1] == '\0')
        {
          if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL)
            {
              g_warning ("Unable to set percentage of %s on a top-level "
                         "actor of type `%s'",
                         (dimension == PARSE_X ||
                          dimension == PARSE_WIDTH ||
                          dimension == PARSE_ANCHOR_X) ? "width" : "height",
                         g_type_name (G_OBJECT_TYPE (self)));
              retval = 0;
              goto out;
            }

          if (dimension == PARSE_X ||
              dimension == PARSE_WIDTH ||
              dimension == PARSE_ANCHOR_X)
            retval = CLUTTER_UNITS_FROM_STAGE_WIDTH_PERCENTAGE (val);
          else
            retval = CLUTTER_UNITS_FROM_STAGE_HEIGHT_PERCENTAGE (val);

          goto out;
        }

      g_warning ("Invalid value `%s': integers, strings or floating point "
                 "values can be used for the x, y, width and height "
                 "properties. Valid modifiers for strings are `px', 'mm' "
                 "and '%%'.",
                 g_value_get_string (&value));

      retval = 0;
    }
  else if (G_VALUE_HOLDS (&value, G_TYPE_DOUBLE))
    {
      gint val;

      if (CLUTTER_PRIVATE_FLAGS (self) & CLUTTER_ACTOR_IS_TOPLEVEL)
        {
          g_warning ("Unable to set percentage of %s on a top-level "
                     "actor of type `%s'",
                     (dimension == PARSE_X || dimension == PARSE_WIDTH) ? "width"
                                                                        : "height",
                     g_type_name (G_OBJECT_TYPE (self)));
          retval = 0;
          goto out;
        }

      val = CLAMP (g_value_get_double (&value) * 100, 0, 100);

      if (dimension == PARSE_X ||
          dimension == PARSE_WIDTH ||
          dimension == PARSE_ANCHOR_X)
        retval = CLUTTER_UNITS_FROM_STAGE_WIDTH_PERCENTAGE (val);
      else
        retval = CLUTTER_UNITS_FROM_STAGE_HEIGHT_PERCENTAGE (val);
    }
  else
    {
      g_warning ("Invalid value of type `%s': integers, strings of floating "
                 "point values can be used for the x, y, width, height "
                 "anchor-x and anchor-y properties.",
                 g_type_name (G_VALUE_TYPE (&value)));
    }

out:
  g_value_unset (&value);

  return retval;
}

typedef struct {
  ClutterRotateAxis axis;

  ClutterFixed angle;

  ClutterUnit center_x;
  ClutterUnit center_y;
  ClutterUnit center_z;
} RotationInfo;

static inline gboolean
parse_rotation_array (ClutterActor *actor,
                      JsonArray    *array,
                      RotationInfo *info)
{
  JsonNode *element;

  if (json_array_get_length (array) != 2)
    return FALSE;

  /* angle */
  element = json_array_get_element (array, 0);
  if (JSON_NODE_TYPE (element) == JSON_NODE_VALUE)
    info->angle = CLUTTER_FLOAT_TO_FIXED (json_node_get_double (element));
  else
    return FALSE;

  /* center */
  element = json_array_get_element (array, 1);
  if (JSON_NODE_TYPE (element) == JSON_NODE_ARRAY)
    {
      JsonArray *center = json_node_get_array (element);

      if (json_array_get_length (center) != 2)
        return FALSE;

      switch (info->axis)
        {
        case CLUTTER_X_AXIS:
          info->center_y = parse_units (actor, PARSE_Y,
                                        json_array_get_element (center, 0));
          info->center_z = parse_units (actor, PARSE_Y,
                                        json_array_get_element (center, 1));
          return TRUE;

        case CLUTTER_Y_AXIS:
          info->center_x = parse_units (actor, PARSE_X,
                                        json_array_get_element (center, 0));
          info->center_z = parse_units (actor, PARSE_X,
                                        json_array_get_element (center, 1));
          return TRUE;

        case CLUTTER_Z_AXIS:
          info->center_x = parse_units (actor, PARSE_X,
                                        json_array_get_element (center, 0));
          info->center_y = parse_units (actor, PARSE_Y,
                                        json_array_get_element (center, 1));
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
parse_rotation (ClutterActor *actor,
                JsonNode     *node,
                RotationInfo *info)
{
  JsonArray *array;
  guint len, i;
  gboolean retval = FALSE;

  if (JSON_NODE_TYPE (node) != JSON_NODE_ARRAY)
    {
      g_warning ("Invalid node of type `%s' found, expecting an array",
                 json_node_type_name (node));
      return FALSE;
    }

  array = json_node_get_array (node);
  len = json_array_get_length (array);

  for (i = 0; i < len; i++)
    {
      JsonNode *element = json_array_get_element (array, i);
      JsonObject *object;
      JsonNode *member;

      if (JSON_NODE_TYPE (element) != JSON_NODE_OBJECT)
        {
          g_warning ("Invalid node of type `%s' found, expecting an object",
                     json_node_type_name (element));
          return FALSE;
        }

      object = json_node_get_object (element);

      if (json_object_has_member (object, "x-axis"))
        {
          member = json_object_get_member (object, "x-axis");

          info->axis = CLUTTER_X_AXIS;

          if (JSON_NODE_TYPE (member) == JSON_NODE_VALUE)
            {
              info->angle = json_node_get_double (member);
              retval = TRUE;
            }
          else if (JSON_NODE_TYPE (member) == JSON_NODE_ARRAY)
            retval = parse_rotation_array (actor,
                                           json_node_get_array (member),
                                           info);
          else
            retval = FALSE;
        }
      else if (json_object_has_member (object, "y-axis"))
        {
          member = json_object_get_member (object, "y-axis");

          info->axis = CLUTTER_Y_AXIS;

          if (JSON_NODE_TYPE (member) == JSON_NODE_VALUE)
            {
              info->angle = json_node_get_double (member);
              retval = TRUE;
            }
          else if (JSON_NODE_TYPE (member) == JSON_NODE_ARRAY)
            retval = parse_rotation_array (actor,
                                           json_node_get_array (member),
                                           info);
          else
            retval = FALSE;
        }
      else if (json_object_has_member (object, "z-axis"))
        {
          member = json_object_get_member (object, "z-axis");

          info->axis = CLUTTER_Z_AXIS;

          if (JSON_NODE_TYPE (member) == JSON_NODE_VALUE)
            {
              info->angle = json_node_get_double (member);
              retval = TRUE;
            }
          else if (JSON_NODE_TYPE (member) == JSON_NODE_ARRAY)
            retval = parse_rotation_array (actor,
                                           json_node_get_array (member),
                                           info);
          else
            retval = FALSE;
        }
    }

  return retval;
}

static gboolean
clutter_actor_parse_custom_node (ClutterScriptable *scriptable,
                                 ClutterScript     *script,
                                 GValue            *value,
                                 const gchar       *name,
                                 JsonNode          *node)
{
  ClutterActor *actor = CLUTTER_ACTOR (scriptable);
  gboolean retval = FALSE;

  if ((name[0] == 'x' && name[1] == '\0') ||
      (name[0] == 'y' && name[1] == '\0') ||
      (strcmp (name, "width") == 0) ||
      (strcmp (name, "height") == 0) ||
      (strcmp (name, "anchor_x") == 0) ||
      (strcmp (name, "anchor_y") == 0))
    {
      ClutterUnit units;
      ParseDimension dimension;

      if (name[0] == 'x')
        dimension = PARSE_X;
      else if (name[0] == 'y')
        dimension = PARSE_Y;
      else if (name[0] == 'w')
        dimension = PARSE_WIDTH;
      else if (name[0] == 'h')
        dimension = PARSE_HEIGHT;
      else if (name[0] == 'a' && name[7] == 'x')
        dimension = PARSE_ANCHOR_X;
      else if (name[0] == 'a' && name[7] == 'y')
        dimension = PARSE_ANCHOR_Y;
      else
        return FALSE;

      units = parse_units (actor, dimension, node);

      /* convert back to pixels: all properties are pixel-based */
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, CLUTTER_UNITS_TO_DEVICE (units));

      retval = TRUE;
    }
  else if (strcmp (name, "rotation") == 0)
    {
      RotationInfo *info;

      info = g_slice_new0 (RotationInfo);
      retval = parse_rotation (actor, node, info);

      if (retval)
        {
          g_value_init (value, G_TYPE_POINTER);
          g_value_set_pointer (value, info);
        }
      else
        g_slice_free (RotationInfo, info);
    }

  return retval;
}

static void
clutter_actor_set_custom_property (ClutterScriptable *scriptable,
                                   ClutterScript     *script,
                                   const gchar       *name,
                                   const GValue      *value)
{
  CLUTTER_NOTE (SCRIPT, "in ClutterActor::set_custom_property('%s')", name);

  if (strcmp (name, "rotation") == 0)
    {
      RotationInfo *info;

      if (!G_VALUE_HOLDS (value, G_TYPE_POINTER))
        return;

      info = g_value_get_pointer (value);

      clutter_actor_set_rotation_internal (CLUTTER_ACTOR (scriptable),
                                           info->axis, info->angle,
                                           info->center_x,
                                           info->center_y,
                                           info->center_z);

      g_slice_free (RotationInfo, info);
    }
  else
    g_object_set_property (G_OBJECT (scriptable), name, value);
}

static void
clutter_scriptable_iface_init (ClutterScriptableIface *iface)
{
  iface->parse_custom_node = clutter_actor_parse_custom_node;
  iface->set_custom_property = clutter_actor_set_custom_property;
}

/**
 * clutter_actor_transform_stage_point
 * @self: A #ClutterActor
 * @x: x screen coordinate of the point to unproject, in #ClutterUnit<!-- -->s
 * @y: y screen coordinate of the point to unproject, in #ClutterUnit<!-- -->s
 * @x_out: return location for the unprojected x coordinance, in
 *   #ClutterUnit<!-- -->s
 * @y_out: return location for the unprojected y coordinance, in
 *   #ClutterUnit<!-- -->s
 *
 * This function translates screen coordinates (@x, @y) to
 * coordinates relative to the actor. For example, it can be used to translate
 * screen events from global screen coordinates into actor-local coordinates.
 *
 * The conversion can fail, notably if the transform stack results in the
 * actor being projected on the screen as a mere line.
 *
 * The conversion should not be expected to be pixel-perfect due to the
 * nature of the operation. In general the error grows when the skewing
 * of the actor rectangle on screen increases.
 *
 * Note: This function is fairly computationally intensive.
 *
 * Note: This function only works when the allocation is up-to-date, i.e. inside of paint()
 *
 * Return value: %TRUE if conversion was successful.
 *
 * Since: 0.6
 */
gboolean
clutter_actor_transform_stage_point (ClutterActor  *self,
				     ClutterUnit    x,
				     ClutterUnit    y,
				     ClutterUnit   *x_out,
				     ClutterUnit   *y_out)
{
  ClutterVertex v[4];
  ClutterFixed  ST[3][3];
  ClutterFixed  RQ[3][3];
  int du, dv, xi, yi;
  ClutterFixed xf, yf, wf, px, py, det;
  ClutterActorPrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  priv = self->priv;

  /*
   * This implementation is based on the quad -> quad projection algorithm
   * described by Paul Heckbert in
   *
   * http://www.cs.cmu.edu/~ph/texfund/texfund.pdf
   *
   * and the sample implementaion at http://www.cs.cmu.edu/~ph/src/texfund/.
   *
   * Our texture is a rectangle with origin [0,0], so we are mapping from quad
   * to rectangle only, which significantly simplifies things; the function
   * calls have been unrolled, and most of the math is done in fixed point.
   */

  clutter_actor_get_abs_allocation_vertices (self, v);

  /*
   * Keeping these as ints simplifies the multiplication (no significant loss
   * of precision here).
   */
  du = CLUTTER_UNITS_TO_DEVICE (priv->allocation.x2 - priv->allocation.x1);
  dv = CLUTTER_UNITS_TO_DEVICE (priv->allocation.y2 - priv->allocation.y1);

  if (!du || !dv)
    return FALSE;

#define FP2FX CLUTTER_FLOAT_TO_FIXED
#define FX2FP CLUTTER_FIXED_TO_DOUBLE
#define FP2INT CLUTTER_FLOAT_TO_INT
#define DET2X(a,b, c,d) (CFX_QMUL(a,d) - CFX_QMUL(b,c))
#define DET2FP(a,b, c,d) (a*d - b*c)

  /*
   * First, find mapping from unit uv square to xy quadrilateral; this
   * equivalent to the pmap_square_quad() functions in the sample
   * implementation, which we can simplify, since our target is always
   * a rectangle.
   */
  px = v[0].x - v[1].x + v[3].x - v[2].x;
  py = v[0].y - v[1].y + v[3].y - v[2].y;

  if (!px && !py)
    { /* affine transform */
      RQ[0][0] = v[1].x - v[0].x;
      RQ[1][0] = v[3].x - v[1].x;
      RQ[2][0] = v[0].x;
      RQ[0][1] = v[1].y - v[0].y;
      RQ[1][1] = v[3].y - v[1].y;
      RQ[2][1] = v[0].y;
      RQ[0][2] = 0;
      RQ[1][2] = 0;
      RQ[2][2] = CFX_ONE;
    }
  else
    { /*
       * projective transform
       *
       * Must do this in floating point, as the del value can overflow the
       * range of ClutterFixed for large actors.
       *
       * TODO -- see if we could do this with sufficient precision in 26.8
       * fixed.
       */
      double dx1, dx2, dy1, dy2, del;

      dx1 = FX2FP (v[1].x - v[3].x);
      dx2 = FX2FP (v[2].x - v[3].x);
      dy1 = FX2FP (v[1].y - v[3].y);
      dy2 = FX2FP (v[2].y - v[3].y);

      del = DET2FP (dx1,dx2, dy1,dy2);

      if (!del)
	return FALSE;

      /*
       * The division here needs to be done in floating point for
       * precisions reasons.
       */
      RQ[0][2] = FP2FX (DET2FP (FX2FP(px),dx2, FX2FP(py),dy2) / del);
      RQ[1][2] = FP2FX (DET2FP (dx1,FX2FP(px), dy1,FX2FP(py)) / del);
      RQ[1][2] = FP2FX (DET2FP(dx1,FX2FP(px), dy1,FX2FP(py))/del);
      RQ[2][2] = CFX_ONE;
      RQ[0][0] = v[1].x - v[0].x + CFX_QMUL (RQ[0][2], v[1].x);
      RQ[1][0] = v[2].x - v[0].x + CFX_QMUL (RQ[1][2], v[2].x);
      RQ[2][0] = v[0].x;
      RQ[0][1] = v[1].y - v[0].y + CFX_QMUL (RQ[0][2], v[1].y);
      RQ[1][1] = v[2].y - v[0].y + CFX_QMUL (RQ[1][2], v[2].y);
      RQ[2][1] = v[0].y;
    }

  /*
   * Now combine with transform from our rectangle (u0,v0,u1,v1) to unit
   * square. Since our rectangle is based at 0,0 we only need to scale.
   */
  RQ[0][0] /= du;
  RQ[1][0] /= dv;
  RQ[0][1] /= du;
  RQ[1][1] /= dv;
  RQ[0][2] /= du;
  RQ[1][2] /= dv;

  /*
   * Now RQ is transform from uv rectangle to xy quadrilateral; we need an
   * inverse of that.
   */
  ST[0][0] = DET2X(RQ[1][1], RQ[1][2], RQ[2][1], RQ[2][2]);
  ST[1][0] = DET2X(RQ[1][2], RQ[1][0], RQ[2][2], RQ[2][0]);
  ST[2][0] = DET2X(RQ[1][0], RQ[1][1], RQ[2][0], RQ[2][1]);
  ST[0][1] = DET2X(RQ[2][1], RQ[2][2], RQ[0][1], RQ[0][2]);
  ST[1][1] = DET2X(RQ[2][2], RQ[2][0], RQ[0][2], RQ[0][0]);
  ST[2][1] = DET2X(RQ[2][0], RQ[2][1], RQ[0][0], RQ[0][1]);
  ST[0][2] = DET2X(RQ[0][1], RQ[0][2], RQ[1][1], RQ[1][2]);
  ST[1][2] = DET2X(RQ[0][2], RQ[0][0], RQ[1][2], RQ[1][0]);
  ST[2][2] = DET2X(RQ[0][0], RQ[0][1], RQ[1][0], RQ[1][1]);

  /*
   * Check the resutling martix is OK.
   */
  det = CFX_QMUL (RQ[0][0], ST[0][0]) + CFX_QMUL (RQ[0][1], ST[0][1]) +
    CFX_QMUL (RQ[0][2], ST[0][2]);

  if (!det)
    return FALSE;

  /*
   * Now transform our point with the ST matrix; the notional w coordiance
   * is 1, hence the last part is simply added.
   */
  xi = CLUTTER_UNITS_TO_DEVICE (x);
  yi = CLUTTER_UNITS_TO_DEVICE (y);

  xf = xi*ST[0][0] + yi*ST[1][0] + ST[2][0];
  yf = xi*ST[0][1] + yi*ST[1][1] + ST[2][1];
  wf = xi*ST[0][2] + yi*ST[1][2] + ST[2][2];

  /*
   * The division needs to be done in floating point for precision reasons.
   */
  if (x_out)
    *x_out = CLUTTER_UNITS_FROM_FLOAT (FX2FP (xf) / FX2FP (wf));
  if (y_out)
    *y_out = CLUTTER_UNITS_FROM_FLOAT (FX2FP (yf) / FX2FP (wf));

#undef FP2FX
#undef FX2FP
#undef FP2INT
#undef DET2X

  return TRUE;
}

/*
 * ClutterGeometry
 */

static ClutterGeometry*
clutter_geometry_copy (const ClutterGeometry *geometry)
{
  return g_slice_dup (ClutterGeometry, geometry);
}

static void
clutter_geometry_free (ClutterGeometry *geometry)
{
  if (G_LIKELY (geometry))
    g_slice_free (ClutterGeometry, geometry);
}

GType
clutter_geometry_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type =
      g_boxed_type_register_static (I_("ClutterGeometry"),
                                    (GBoxedCopyFunc) clutter_geometry_copy,
                                    (GBoxedFreeFunc) clutter_geometry_free);

  return our_type;
}

/*
 * ClutterVertices
 */

static ClutterVertex *
clutter_vertex_copy (const ClutterVertex *vertex)
{
  return g_slice_dup (ClutterVertex, vertex);
}

static void
clutter_vertex_free (ClutterVertex *vertex)
{
  if (G_UNLIKELY (vertex))
    g_slice_free (ClutterVertex, vertex);
}

GType
clutter_vertex_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type =
      g_boxed_type_register_static (I_("ClutterVertex"),
                                    (GBoxedCopyFunc) clutter_vertex_copy,
                                    (GBoxedFreeFunc) clutter_vertex_free);

  return our_type;
}

/*
 * ClutterActorBox
 */
static ClutterActorBox *
clutter_actor_box_copy (const ClutterActorBox *box)
{
  return g_slice_dup (ClutterActorBox, box);
}

static void
clutter_actor_box_free (ClutterActorBox *box)
{
  if (G_LIKELY (box))
    g_slice_free (ClutterActorBox, box);
}

GType
clutter_actor_box_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type =
      g_boxed_type_register_static (I_("ClutterActorBox"),
                                    (GBoxedCopyFunc) clutter_actor_box_copy,
                                    (GBoxedFreeFunc) clutter_actor_box_free);
  return our_type;
}

/******************************************************************************/

typedef struct _BoxedFloat BoxedFloat;

struct _BoxedFloat
{
  gfloat value;
};

static void
boxed_float_free (gpointer data)
{
  if (G_LIKELY (data))
    g_slice_free (BoxedFloat, data);
}

struct _ShaderData
{
  ClutterShader *shader;
  GHashTable    *float1f_hash; /*< list of values that should be set
                                *  on the shader before each paint cycle
                                */
};

static void
destroy_shader_data (ClutterActor *self)
{
  ClutterActorPrivate *actor_priv = self->priv;
  ShaderData          *shader_data   = actor_priv->shader_data;

  if (!shader_data)
    return;

  if (shader_data->shader)
    {
      g_object_unref (shader_data->shader);
      shader_data->shader = NULL;
    }

  if (shader_data->float1f_hash)
    {
      g_hash_table_destroy (shader_data->float1f_hash);
      shader_data->float1f_hash = NULL;
    }

  g_free (shader_data);
  actor_priv->shader_data = NULL;
}


/**
 * clutter_actor_get_shader:
 * @self: a #ClutterActor
 *
 * Queries the currently set #ClutterShader on @self.
 *
 * Return value: The currently set #ClutterShader or %NULL if no
 *   shader is set.
 *
 * Since: 0.6
 */
ClutterShader *
clutter_actor_get_shader (ClutterActor *self)
{
  ClutterActorPrivate *actor_priv;
  ShaderData     *shader_data;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  actor_priv = self->priv;
  shader_data = actor_priv->shader_data;

  if (!shader_data)
    return NULL;

  return shader_data->shader;
}

/**
 * clutter_actor_set_shader:
 * @self: a #ClutterActor
 * @shader: a #ClutterShader or %NULL to unset the shader.
 *
 * Sets the #ClutterShader to be used when rendering @self.
 * If @shader is %NULL it will unset any currently set shader
 * for the actor.
 *
 * Return value: %TRUE if the shader was successfully applied
 *
 * Since: 0.6
 */
gboolean
clutter_actor_set_shader (ClutterActor  *self,
                          ClutterShader *shader)
{
  ClutterActorPrivate *actor_priv;
  ShaderData          *shader_data;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);
  g_return_val_if_fail (shader == NULL || CLUTTER_IS_SHADER (shader), FALSE);

  /* if shader passed in is NULL we destroy the shader */
  if (shader == NULL)
    {
      destroy_shader_data (self);
    }

  actor_priv = self->priv;
  shader_data = actor_priv->shader_data;

  if (!shader_data)
    {
      actor_priv->shader_data = shader_data = g_new0 (ShaderData, 1);
      shader_data->float1f_hash =
        g_hash_table_new_full (g_str_hash, g_str_equal,
                               g_free,
                               boxed_float_free);
    }
  if (shader_data->shader)
    {
      g_object_unref (shader_data->shader);
      shader_data->shader = NULL;
    }

  if (shader)
    {
      shader_data->shader = g_object_ref (shader);
    }

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);

  return TRUE;
}


static void
set_each_param (gpointer key,
                gpointer value,
                gpointer user_data)
{
  ClutterShader *shader = CLUTTER_SHADER (user_data);
  BoxedFloat *box = value;

  clutter_shader_set_uniform_1f (shader, key, box->value);
}

static void
clutter_actor_shader_pre_paint (ClutterActor *actor,
                                gboolean      repeat)
{
  ClutterActorPrivate *priv;
  ShaderData          *shader_data;
  ClutterShader       *shader;
  ClutterMainContext  *context;

  priv = actor->priv;
  shader_data = priv->shader_data;

  if (!shader_data)
    return;

  context = clutter_context_get_default ();
  shader = shader_data->shader;

  if (shader)
    {
      clutter_shader_set_is_enabled (shader, TRUE);

      g_hash_table_foreach (shader_data->float1f_hash, set_each_param, shader);

      if (!repeat)
        context->shaders = g_slist_prepend (context->shaders, actor);
    }
}

static void
clutter_actor_shader_post_paint (ClutterActor *actor)
{
  ClutterActorPrivate *priv;
  ShaderData          *shader_data;
  ClutterShader       *shader;
  ClutterMainContext  *context;

  priv = actor->priv;
  shader_data = priv->shader_data;

  if (!shader_data)
    return;

  context = clutter_context_get_default ();
  shader = shader_data->shader;

  if (shader)
    {
      clutter_shader_set_is_enabled (shader, FALSE);

      context->shaders = g_slist_remove (context->shaders, actor);
      if (context->shaders)
        {
          /* call pre-paint again, this time with the second argument being
           * TRUE, indicating that we are reapplying the shader and thus
           * should not be prepended to the stack
           */
          clutter_actor_shader_pre_paint (context->shaders->data, TRUE);
        }
    }
}

/**
 * clutter_actor_set_shader_param:
 * @self: a #ClutterActor
 * @param: the name of the parameter
 * @value: the value of the parameter
 *
 * Sets the value for a named parameter of the shader applied
 * to @actor.
 *
 * Since: 0.6
 */
void
clutter_actor_set_shader_param (ClutterActor *self,
                                const gchar  *param,
                                gfloat        value)
{
  ClutterActorPrivate *priv;
  ShaderData *shader_data;
  BoxedFloat *box;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  g_return_if_fail (param != NULL);

  priv = self->priv;
  shader_data = priv->shader_data;

  if (!shader_data)
    return;

  box = g_slice_new (BoxedFloat);
  box->value = value;
  g_hash_table_insert (shader_data->float1f_hash, g_strdup (param), box);

  if (CLUTTER_ACTOR_IS_VISIBLE (self))
    clutter_actor_queue_redraw (self);
}

/**
 * clutter_actor_is_rotated:
 * @self: a #ClutterActor
 *
 * Checks whether any rotation is applied to the actor.
 *
 * Return value: %TRUE if the actor is rotated.
 *
 * Since: 0.6
 */
gboolean
clutter_actor_is_rotated (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  priv = self->priv;

  if (priv->rxang || priv->ryang || priv->rzang)
    return TRUE;

  return FALSE;
}

/**
 * clutter_actor_is_scaled:
 * @self: a #ClutterActor
 *
 * Checks whether the actor is scaled in either dimension.
 *
 * Return value: %TRUE if the actor is scaled.
 *
 * Since: 0.6
 */
gboolean
clutter_actor_is_scaled (ClutterActor *self)
{
  ClutterActorPrivate *priv;

  g_return_val_if_fail (CLUTTER_IS_ACTOR (self), FALSE);

  priv = self->priv;

  if (priv->scale_x != CFX_ONE || priv->scale_y != CFX_ONE)
    return TRUE;

  return FALSE;
}

/**
 * clutter_actor_box_get_from_vertices:
 * @vtx: array of four #ClutterVertex
 * @box: return location for a #ClutterActorBox
 *
 * Calculates the bounding box represented by the four vertices; for details
 * of the vertex array see clutter_actor_get_abs_allocation_vertices().
 *
 * Since: 0.6
 */
void
clutter_actor_get_box_from_vertices (ClutterVertex    vtx[4],
				     ClutterActorBox *box)
{
  ClutterUnit x_1, x_2, y_1, y_2;

  /* 4-way min/max */
  x_1 = vtx[0].x;
  y_1 = vtx[0].y;
  if (vtx[1].x < x_1)
    x_1 = vtx[1].x;
  if (vtx[2].x < x_1)
    x_1 = vtx[2].x;
  if (vtx[3].x < x_1)
    x_1 = vtx[3].x;
  if (vtx[1].y < y_1)
    y_1 = vtx[1].y;
  if (vtx[2].y < y_1)
    y_1 = vtx[2].y;
  if (vtx[3].y < y_1)
    y_1 = vtx[3].y;

  x_2 = vtx[0].x;
  y_2 = vtx[0].y;
  if (vtx[1].x > x_2)
    x_2 = vtx[1].x;
  if (vtx[2].x > x_2)
    x_2 = vtx[2].x;
  if (vtx[3].x > x_2)
    x_2 = vtx[3].x;
  if (vtx[1].y > y_2)
    y_2 = vtx[1].y;
  if (vtx[2].y > y_2)
    y_2 = vtx[2].y;
  if (vtx[3].y > y_2)
    y_2 = vtx[3].y;

  box->x1 = x_1;
  box->x2 = x_2;
  box->y1 = y_1;
  box->y2 = y_2;
}

/**
 * clutter_actor_get_stage:
 * @actor: a #ClutterActor
 *
 * Retrieves the #ClutterStage where @actor is contained.
 *
 * Return value: the stage containing the actor, or %NULL
 *
 * Since: 0.8
 */
ClutterActor *
clutter_actor_get_stage (ClutterActor *actor)
{
  g_return_val_if_fail (CLUTTER_IS_ACTOR (actor), NULL);

  while (actor && !(CLUTTER_PRIVATE_FLAGS (actor) & CLUTTER_ACTOR_IS_TOPLEVEL))
    actor = clutter_actor_get_parent (actor);

  return actor;
}

/**
 * clutter_actor_allocate_preferred_size:
 * @self: a #ClutterActor
 * @absolute_origin_changed: whether the position of the parent has
 *   changed in stage coordinates
 *
 * Allocates the natural size of @self.
 *
 * This function is a utility call for #ClutterActor implementations
 * that allocates the actor's preferred natural size. It can be used
 * by fixed layout managers (like #ClutterGroup or so called
 * 'composite actors') inside the ClutterActor::allocate
 * implementation to give each child exactly how much space it
 * requires.
 *
 * This function is not meant to be used by applications. It is also
 * not meant to be used outside the implementation of the
 * ClutterActor::allocate virtual function.
 *
 * Since: 0.8
 */
void
clutter_actor_allocate_preferred_size (ClutterActor *self,
                                       gboolean      absolute_origin_changed)
{
  ClutterUnit actor_x, actor_y;
  ClutterUnit natural_width, natural_height;
  ClutterActorBox actor_box;

  g_return_if_fail (CLUTTER_IS_ACTOR (self));
  
  actor_x = clutter_actor_get_xu (self);
  actor_y = clutter_actor_get_yu (self);

  clutter_actor_get_preferred_size (self,
                                    NULL, NULL,
                                    &natural_width,
                                    &natural_height);
  
  actor_box.x1 = actor_x;
  actor_box.y1 = actor_y;
  actor_box.x2 = actor_box.x1 + natural_width;
  actor_box.y2 = actor_box.y1 + natural_height;

  clutter_actor_allocate (self, &actor_box, absolute_origin_changed);
}
