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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _HAVE_CLUTTER_EVENT_H
#define _HAVE_CLUTTER_EVENT_H

#include <glib-object.h>
#include <clutter/clutter-types.h>

#define CLUTTER_TYPE_EVENT	(clutter_event_get_type ())

/**
 * CLUTTER_PRIORITY_EVENTS:
 *
 * Priority for event handling.
 *
 * Since: 0.4
 */
#define CLUTTER_PRIORITY_EVENTS (G_PRIORITY_DEFAULT)

/**
 * CLUTTER_CURRENT_TIME:
 *
 * Default value for "now".
 *
 * Since: 0.4
 */
#define CLUTTER_CURRENT_TIME    0L

G_BEGIN_DECLS

/**
 * ClutterModifierType:
 * @CLUTTER_SHIFT_MASK: Mask applied by the Shift key
 * @CLUTTER_LOCK_MASK: Mask applied by the Caps Lock key
 * @CLUTTER_CONTROL_MASK: Mask applied by the Control key
 * @CLUTTER_MOD1_MASK: Mask applied by the first Mod key
 * @CLUTTER_MOD2_MASK: Mask applied by the second Mod key
 * @CLUTTER_MOD3_MASK: Mask applied by the third Mod key
 * @CLUTTER_MOD4_MASK: Mask applied by the fourth Mod key
 * @CLUTTER_MOD5_MASK: Mask applied by the fifth Mod key
 * @CLUTTER_BUTTON1_MASK: Mask applied by the first pointer button
 * @CLUTTER_BUTTON2_MASK: Mask applied by the second pointer button
 * @CLUTTER_BUTTON3_MASK: Mask applied by the third pointer button
 * @CLUTTER_BUTTON4_MASK: Mask applied by the fourth pointer button
 * @CLUTTER_BUTTON5_MASK: Mask applied by the fifth pointer button
 *
 * Masks applied to a #ClutterEvent by modifiers.
 *
 * Since: 0.4
 */
typedef enum {
  CLUTTER_SHIFT_MASK    = 1 << 0,
  CLUTTER_LOCK_MASK     = 1 << 1,
  CLUTTER_CONTROL_MASK  = 1 << 2,
  CLUTTER_MOD1_MASK     = 1 << 3,
  CLUTTER_MOD2_MASK     = 1 << 4,
  CLUTTER_MOD3_MASK     = 1 << 5,
  CLUTTER_MOD4_MASK     = 1 << 6,
  CLUTTER_MOD5_MASK     = 1 << 7,
  CLUTTER_BUTTON1_MASK  = 1 << 8,
  CLUTTER_BUTTON2_MASK  = 1 << 9,
  CLUTTER_BUTTON3_MASK  = 1 << 10,
  CLUTTER_BUTTON4_MASK  = 1 << 11,
  CLUTTER_BUTTON5_MASK  = 1 << 12
} ClutterModifierType;

/**
 * ClutterEventFlags:
 * @CLUTTER_EVENT_FLAG_SYNTHETIC: Synthetic event
 *
 * Flags for the #ClutterEvent
 *
 * Since: 0.6
 */
typedef enum {
  CLUTTER_EVENT_FLAG_SYNTHETIC = 1 << 0,
} ClutterEventFlags;

/**
 * ClutterEventType:
 * @CLUTTER_NOTHING: Empty event
 * @CLUTTER_KEY_PRESS: Key press event
 * @CLUTTER_KEY_RELEASE: Key release event
 * @CLUTTER_MOTION: Pointer motion event
 * @CLUTTER_ENTER: Actor enter event
 * @CLUTTER_LEAVE: Actor leave event
 * @CLUTTER_BUTTON_PRESS: Pointer button press event
 * @CLUTTER_BUTTON_RELEASE: Pointer button release event
 * @CLUTTER_SCROLL: Pointer scroll event
 * @CLUTTER_STAGE_STATE: Stage stage change event
 * @CLUTTER_DESTROY_NOTIFY: Destroy notification event
 * @CLUTTER_CLIENT_MESSAGE: Client message event
 * @CLUTTER_DELETE: Stage delete event
 *
 * Types of events.
 *
 * Since: 0.4
 */
typedef enum 
{
  CLUTTER_NOTHING = 0,
  CLUTTER_KEY_PRESS,
  CLUTTER_KEY_RELEASE,
  CLUTTER_MOTION,
  CLUTTER_ENTER,
  CLUTTER_LEAVE,
  CLUTTER_BUTTON_PRESS,
  CLUTTER_BUTTON_RELEASE,
  CLUTTER_SCROLL,
  CLUTTER_STAGE_STATE,
  CLUTTER_DESTROY_NOTIFY,
  CLUTTER_CLIENT_MESSAGE,
  CLUTTER_DELETE
} ClutterEventType;

/**
 * ClutterScrollDirection:
 * @CLUTTER_SCROLL_UP: Scroll up
 * @CLUTTER_SCROLL_DOWN: Scroll down
 * @CLUTTER_SCROLL_LEFT: Scroll left
 * @CLUTTER_SCROLL_RIGHT: Scroll right
 *
 * Direction of a pointer scroll event.
 *
 * Since: 0.4
 */
typedef enum
{
  CLUTTER_SCROLL_UP,
  CLUTTER_SCROLL_DOWN,
  CLUTTER_SCROLL_LEFT,
  CLUTTER_SCROLL_RIGHT
} ClutterScrollDirection;

/**
 * ClutterStageState:
 * @CLUTTER_STAGE_STATE_FULLSCREEN: Fullscreen mask
 * @CLUTTER_STAGE_STATE_OFFSCREEN: Offscreen mask
 * @CLUTTER_STAGE_STATE_ACTIVATED: Activated mask
 *
 * Stage state masks
 *
 * Since: 0.4
 */
typedef enum
{
  CLUTTER_STAGE_STATE_FULLSCREEN       = (1<<1),
  CLUTTER_STAGE_STATE_OFFSCREEN        = (1<<2),
  CLUTTER_STAGE_STATE_ACTIVATED        = (1<<3)
} ClutterStageState;

typedef union _ClutterEvent ClutterEvent;

typedef struct _ClutterAnyEvent         ClutterAnyEvent;
typedef struct _ClutterButtonEvent      ClutterButtonEvent;
typedef struct _ClutterKeyEvent         ClutterKeyEvent;
typedef struct _ClutterMotionEvent      ClutterMotionEvent;
typedef struct _ClutterScrollEvent      ClutterScrollEvent;
typedef struct _ClutterStageStateEvent  ClutterStageStateEvent;
typedef struct _ClutterCrossingEvent    ClutterCrossingEvent;

/**
 * ClutterInputDevice:
 *
 * Generic representation of an input device. The
 * actual contents of this structure depend on the
 * backend used.
 */
typedef struct _ClutterInputDevice      ClutterInputDevice;

/**
 * ClutterAnyEvent:
 * @type: event type
 * @time: event time
 * @flags: event flags
 * @source: event source actor
 *
 * Common members for a #ClutterEvent
 *
 * Since: 0.2
 */
struct _ClutterAnyEvent
{
  ClutterEventType  type;
  guint32           time;
  ClutterEventFlags flags;
  ClutterStage *stage;
  ClutterActor *source;
};

/**
 * ClutterKeyEvent:
 * @type: event type
 * @time: event time
 * @flags: event flags
 * @stage: event source stage
 * @source: event source actor
 * @modifier_state: key modifiers
 * @keyval: raw key value
 * @hardware_keycode: raw hardware key value
 * @unicode_value: Unicode representation
 *
 * Key event
 *
 * Since: 0.2
 */
struct _ClutterKeyEvent
{
  ClutterEventType type;
  guint32 time;
  ClutterEventFlags flags;
  ClutterStage *stage;
  ClutterActor *source;
  ClutterModifierType modifier_state;
  guint keyval;
  guint16 hardware_keycode;
  gunichar unicode_value;
};

/**
 * ClutterButtonEvent:
 * @type: event type
 * @time: event time
 * @flags: event flags
 * @stage: event source stage
 * @source: event source actor
 * @x: event X coordinate
 * @y: event Y coordinate
 * @modifier_state: button modifiers
 * @button: event button
 * @click_count: number of button presses within the default time
 *   and radius
 * @axes: reserved for future use
 * @device: reserved for future use
 *
 * Button event
 *
 * Since: 0.2
 */
struct _ClutterButtonEvent
{
  ClutterEventType type;
  guint32 time;
  ClutterEventFlags flags;
  ClutterStage *stage;
  ClutterActor *source;
  gint x;
  gint y;
  ClutterModifierType modifier_state;
  guint32 button;
  guint click_count;
  gdouble *axes; /* Future use */
  ClutterInputDevice *device; /* Future use */
};

/**
 * ClutterCrossingEvent:
 * @type: event type
 * @time: event time
 * @flags: event flags
 * @stage: event source stage
 * @source: event source actor
 * @x: event X coordinate
 * @y: event Y coordinate
 * @related: actor related to the crossing
 *
 * Event for the movement of the pointer across different actors
 *
 * Since: 0.2
 */
struct _ClutterCrossingEvent
{
  ClutterEventType type;
  guint32 time;
  ClutterEventFlags flags;
  ClutterStage *stage;
  ClutterActor *source;
  gint x;
  gint y;
  ClutterInputDevice *device; /* future use */
  ClutterActor *related;
};

/**
 * ClutterMotionEvent:
 * @type: event type
 * @time: event time
 * @flags: event flags
 * @stage: event source stage
 * @source: event source actor
 * @x: event X coordinate
 * @y: event Y coordinate
 * @modifier_state: button modifiers
 * @axes: reserved for future use
 * @device: reserved for future use
 *
 * Event for the pointer motion
 *
 * Since: 0.2
 */
struct _ClutterMotionEvent
{
  ClutterEventType type;
  guint32 time;
  ClutterEventFlags flags;
  ClutterStage *stage;
  ClutterActor *source;
  gint x;
  gint y;
  ClutterModifierType modifier_state;
  gdouble *axes; /* Future use */
  ClutterInputDevice *device; /* Future use */
};

/**
 * ClutterScrollEvent:
 * @type: event type
 * @time: event time
 * @flags: event flags
 * @stage: event source stage
 * @source: event source actor
 * @x: event X coordinate
 * @y: event Y coordinate
 * @direction: direction of the scrolling
 * @modifier_state: button modifiers
 * @axes: reserved for future use
 * @device: reserved for future use
 *
 * Scroll wheel (or similar device) event
 *
 * Since: 0.2
 */
struct _ClutterScrollEvent
{
  ClutterEventType type;
  guint32 time;
  ClutterEventFlags flags;
  ClutterStage *stage;
  ClutterActor *source;
  gint x;
  gint y;
  ClutterScrollDirection direction;
  ClutterModifierType modifier_state;
  gdouble *axes; /* future use */
  ClutterInputDevice *device; /* future use */
};

/**
 * ClutterStageStateEvent:
 * @type: event type
 * @time: event time
 * @flags: event flags
 * @stage: event source stage
 * @source: event source actor (unused)
 * @changed_mask: bitwise OR of the changed flags
 * @new_state: bitwise OR of the current state flags
 *
 * Event signalling a change in the #ClutterStage state.
 *
 * Since: 0.2
 */
struct _ClutterStageStateEvent
{
  ClutterEventType type;
  guint32 time;
  ClutterEventFlags flags;
  ClutterStage *stage;
  ClutterActor *source; /* unused XXX: should probably be the stage itself */
  ClutterStageState changed_mask;
  ClutterStageState new_state;
};

/**
 * ClutterEvent:
 * @type: event type
 *
 * Generic event wrapper.
 *
 * Since: 0.2
 */
union _ClutterEvent
{
  ClutterEventType type;

  ClutterAnyEvent any;
  ClutterButtonEvent button;
  ClutterKeyEvent key;
  ClutterMotionEvent motion;
  ClutterScrollEvent scroll;
  ClutterStageStateEvent stage_state;
  ClutterCrossingEvent crossing;
};

GType clutter_event_get_type (void) G_GNUC_CONST;

gboolean            clutter_events_pending      (void);
ClutterEvent *      clutter_event_get           (void);
ClutterEvent *      clutter_event_peek          (void);
void                clutter_event_put           (ClutterEvent       *event);
ClutterEvent *      clutter_event_new           (ClutterEventType    type);
ClutterEvent *      clutter_event_copy          (ClutterEvent       *event);
void                clutter_event_free          (ClutterEvent       *event);
ClutterEventType    clutter_event_type          (ClutterEvent       *event);
guint32             clutter_event_get_time      (ClutterEvent       *event);
ClutterModifierType clutter_event_get_state     (ClutterEvent       *event);
void                clutter_event_get_coords    (ClutterEvent       *event,
                                                 gint               *x,
                                                 gint               *y);
gint                clutter_event_get_device_id (ClutterEvent *event);
ClutterActor*       clutter_event_get_source    (ClutterEvent       *event);

guint               clutter_key_event_symbol    (ClutterKeyEvent    *keyev);
guint16             clutter_key_event_code      (ClutterKeyEvent    *keyev);
guint32             clutter_key_event_unicode   (ClutterKeyEvent    *keyev);

guint32             clutter_button_event_button (ClutterButtonEvent *buttev);

guint32             clutter_keysym_to_unicode   (guint               keyval);

ClutterStage*       clutter_event_get_stage     (ClutterEvent *event);

G_END_DECLS

#endif
