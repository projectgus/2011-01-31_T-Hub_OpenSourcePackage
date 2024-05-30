/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *             Jorn Baayen  <jorn@openedhand.com>
 *             Emmanuele Bassi  <ebassi@openedhand.com>
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

#ifndef __CLUTTER_BEHAVIOUR_PATH_H__
#define __CLUTTER_BEHAVIOUR_PATH_H__

#include <clutter/clutter-alpha.h>
#include <clutter/clutter-behaviour.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_BEHAVIOUR_PATH (clutter_behaviour_path_get_type ())

#define CLUTTER_BEHAVIOUR_PATH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CLUTTER_TYPE_BEHAVIOUR_PATH, ClutterBehaviourPath))

#define CLUTTER_BEHAVIOUR_PATH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CLUTTER_TYPE_BEHAVIOUR_PATH, ClutterBehaviourPathClass))

#define CLUTTER_IS_BEHAVIOUR_PATH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CLUTTER_TYPE_BEHAVIOUR_PATH))

#define CLUTTER_IS_BEHAVIOUR_PATH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CLUTTER_TYPE_BEHAVIOUR_PATH))

#define CLUTTER_BEHAVIOUR_PATH_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CLUTTER_TYPE_BEHAVIOUR_PATH, ClutterBehaviourPathClass))

typedef struct _ClutterBehaviourPath        ClutterBehaviourPath;
typedef struct _ClutterBehaviourPathPrivate ClutterBehaviourPathPrivate;
typedef struct _ClutterBehaviourPathClass   ClutterBehaviourPathClass;
 
struct _ClutterBehaviourPath
{
  /*< private >*/
  ClutterBehaviour             parent;
  ClutterBehaviourPathPrivate *priv;
};

/**
 * ClutterBehaviourPathClass
 * @knot_reached: signal class handler for the
 *   ClutterBehaviourPath::knot_reached signal
 *
 * Since 0.2
 */
struct _ClutterBehaviourPathClass
{
  /*< private >*/
  ClutterBehaviourClass   parent_class;

  /*< public >*/
  void (*knot_reached) (ClutterBehaviourPath *pathb,
                        const ClutterKnot    *knot);

  /*< private >*/
  void (*_clutter_path_1) (void);
  void (*_clutter_path_2) (void);
  void (*_clutter_path_3) (void);
  void (*_clutter_path_4) (void);
};

GType clutter_behaviour_path_get_type (void) G_GNUC_CONST;

ClutterBehaviour *clutter_behaviour_path_new          (ClutterAlpha         *alpha,
                                                       const ClutterKnot    *knots,
                                                       guint                 n_knots);
GSList *          clutter_behaviour_path_get_knots    (ClutterBehaviourPath *pathb);
void              clutter_behaviour_path_append_knot  (ClutterBehaviourPath *pathb,
                                                       const ClutterKnot    *knot);
void              clutter_behaviour_path_append_knots (ClutterBehaviourPath *pathb,
                                                       const ClutterKnot    *first_knot,
                                                       ...) G_GNUC_NULL_TERMINATED;
void              clutter_behaviour_path_insert_knot  (ClutterBehaviourPath  *pathb,
				                       guint                  offset,
				                       const ClutterKnot     *knot);
void              clutter_behaviour_path_remove_knot  (ClutterBehaviourPath  *pathb,
				                       guint                  offset);

void              clutter_behaviour_path_clear        (ClutterBehaviourPath  *pathb);

G_END_DECLS

#endif /* __CLUTTER_BEHAVIOUR_PATH_H__ */
