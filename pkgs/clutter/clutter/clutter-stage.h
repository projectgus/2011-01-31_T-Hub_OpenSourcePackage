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

#ifndef __CLUTTER_STAGE_H__
#define __CLUTTER_STAGE_H__

#include <clutter/clutter-types.h>
#include <clutter/clutter-group.h>
#include <clutter/clutter-color.h>
#include <clutter/clutter-event.h>

#ifdef USE_GDKPIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

G_BEGIN_DECLS

#define CLUTTER_TYPE_PERSPECTIVE        (clutter_perspective_get_type ())
#define CLUTTER_TYPE_FOG                (clutter_fog_get_type ())
#define CLUTTER_TYPE_STAGE              (clutter_stage_get_type())

#define CLUTTER_STAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTER_TYPE_STAGE, ClutterStage))

#define CLUTTER_STAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTER_TYPE_STAGE, ClutterStageClass))

#define CLUTTER_IS_STAGE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTER_TYPE_STAGE))

#define CLUTTER_IS_STAGE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTER_TYPE_STAGE))

#define CLUTTER_STAGE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTER_TYPE_STAGE, ClutterStageClass))

/**
 * CLUTTER_STAGE_WIDTH:
 *
 * Macro that evaluates to the current stage width
 *
 * Since: 0.2
 */
#define CLUTTER_STAGE_WIDTH() \
 (clutter_actor_get_width (clutter_stage_get_default ()))

/**
 * CLUTTER_STAGE_HEIGHT:
 *
 * Macro that evaluates to the current stage height
 *
 * Since: 0.2
 */
#define CLUTTER_STAGE_HEIGHT() \
 (clutter_actor_get_height (clutter_stage_get_default ()))

typedef struct _ClutterPerspective  ClutterPerspective;
typedef struct _ClutterFog          ClutterFog;


typedef struct _ClutterStageClass   ClutterStageClass;
typedef struct _ClutterStagePrivate ClutterStagePrivate;

struct _ClutterStage
{
  /*< private >*/
  ClutterGroup parent_instance;

  ClutterStagePrivate *priv;
};

struct _ClutterStageClass
{
  /*< private >*/
  ClutterGroupClass parent_class;

  /*< public >*/
  /* signals */
  void (* fullscreen)   (ClutterStage *stage);
  void (* unfullscreen) (ClutterStage *stage);
  void (* activate)     (ClutterStage *stage);
  void (* deactivate)   (ClutterStage *stage);

  /*< private >*/
  /* padding for future expansion */
  gpointer _padding_dummy[32];
};



/**
 * ClutterPerspective:
 * @fovy: the field of view angle, in degrees, in the y direction
 * @aspect: the aspect ratio that determines the field of view in the x
 *   direction. The aspect ratio is the ratio of x (width) to y (height)
 * @z_near: the distance from the viewer to the near clipping
 *   plane (always positive)
 * @z_far: the distance from the viewer to the far clipping
 *   plane (always positive)
 *
 * Stage perspective definition. #ClutterPerspective is only used by
 * the fixed point version of clutter_stage_set_perspective().
 *
 * Since: 0.4
 */
struct _ClutterPerspective
{
  ClutterFixed fovy;
  ClutterFixed aspect;
  ClutterFixed z_near;
  ClutterFixed z_far;
};

/**
 * ClutterFog:
 * @density: density of the fog
 * @z_near: starting distance from the viewer to the near clipping
 *   plane (always positive)
 * @z_far: final distance from the viewer to the far clipping
 *   plane (always positive)
 *
 * Fog settings used to create the depth cueing effect. #ClutterFog is
 * useful only when using the fixed point API.
 *
 * Since: 0.6
 */
struct _ClutterFog
{
  ClutterFixed density;
  ClutterFixed z_near;
  ClutterFixed z_far;
};

GType         clutter_perspective_get_type    (void) G_GNUC_CONST;
GType         clutter_fog_get_type            (void) G_GNUC_CONST;
GType         clutter_stage_get_type          (void) G_GNUC_CONST;

ClutterActor *clutter_stage_get_default       (void);
ClutterActor *clutter_stage_new               (void);

void          clutter_stage_set_color         (ClutterStage       *stage,
                                               const ClutterColor *color);
void          clutter_stage_get_color         (ClutterStage       *stage,
                                               ClutterColor       *color);
void          clutter_stage_set_perspectivex  (ClutterStage       *stage,
			                       ClutterPerspective *perspective);
void          clutter_stage_get_perspectivex  (ClutterStage       *stage,
			                       ClutterPerspective *perspective);
void          clutter_stage_set_perspective   (ClutterStage       *stage,
					       gfloat              fovy,
					       gfloat              aspect,
					       gfloat              z_near,
					       gfloat              z_far);
void          clutter_stage_get_perspective   (ClutterStage       *stage,
					       gfloat             *fovy,
					       gfloat             *aspect,
					       gfloat             *z_near,
					       gfloat             *z_far);
void          clutter_stage_fullscreen        (ClutterStage       *stage);
void          clutter_stage_unfullscreen      (ClutterStage       *stage);
void          clutter_stage_show_cursor       (ClutterStage       *stage);
void          clutter_stage_hide_cursor       (ClutterStage       *stage);

ClutterActor *clutter_stage_get_actor_at_pos  (ClutterStage       *stage,
                                               gint                x,
                                               gint                y);
guchar       *clutter_stage_read_pixels       (ClutterStage       *stage,
                                               gint                x,
                                               gint                y,
                                               gint                width,
                                               gint                height);
gboolean      clutter_stage_event             (ClutterStage       *stage,
                                               ClutterEvent       *event);

void                  clutter_stage_set_title          (ClutterStage *stage,
                                                        const gchar  *title);
G_CONST_RETURN gchar *clutter_stage_get_title          (ClutterStage *stage);
void                  clutter_stage_set_user_resizable (ClutterStage *stage,
						        gboolean      resizable);
gboolean              clutter_stage_get_user_resizable (ClutterStage *stage);
void                  clutter_stage_set_use_fog        (ClutterStage *stage,
                                                        gboolean      fog);
gboolean              clutter_stage_get_use_fog        (ClutterStage *stage);
void                  clutter_stage_set_fog            (ClutterStage *stage,
                                                        gdouble       density,
                                                        gdouble       z_near,
                                                        gdouble       z_far);
void                  clutter_stage_get_fog            (ClutterStage *stage,
                                                        gdouble      *density,
                                                        gdouble      *z_near,
                                                        gdouble      *z_far);
void                  clutter_stage_set_fogx           (ClutterStage *stage,
                                                        ClutterFog   *fog);
void                  clutter_stage_get_fogx           (ClutterStage *stage,
                                                        ClutterFog   *fog);
gdouble               clutter_stage_get_resolution     (ClutterStage *stage);
ClutterFixed          clutter_stage_get_resolutionx    (ClutterStage *stage);

void                  clutter_stage_set_key_focus      (ClutterStage *stage,
                                                        ClutterActor *actor);
ClutterActor *        clutter_stage_get_key_focus      (ClutterStage *stage);

/* New experiental calls */
void                  clutter_stage_ensure_current     (ClutterStage *stage);
void                  clutter_stage_queue_redraw       (ClutterStage *stage);
gboolean              clutter_stage_is_default         (ClutterStage *stage);

/* Commodity macro */
#define clutter_stage_add(stage,actor)                  G_STMT_START {  \
  if (CLUTTER_IS_STAGE ((stage)) && CLUTTER_IS_ACTOR ((actor)))         \
    {                                                                   \
      ClutterContainer *_container = (ClutterContainer *) (stage);      \
      ClutterActor *_actor = (ClutterActor *) (actor);                  \
      clutter_container_add_actor (_container, _actor);                 \
    }                                                   } G_STMT_END

G_END_DECLS

#endif /* __CLUTTER_STAGE_H__ */
