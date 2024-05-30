/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *             Neil Jagdish Patel <njp@o-hand.com>
 *             Emmanuele Bassi <ebassi@openedhand.com>
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
 *
 * NB: Inspiration for column storage taken from GtkListStore
 */

#ifndef __CLUTTER_LIST_MODEL_H__
#define __CLUTTER_LIST_MODEL_H__

#include <clutter/clutter-model.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_LIST_MODEL         (clutter_list_model_get_type ())
#define CLUTTER_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLUTTER_TYPE_LIST_MODEL, ClutterListModel))
#define CLUTTER_IS_LIST_MODEL(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLUTTER_TYPE_LIST_MODEL))

typedef struct _ClutterListModel        ClutterListModel;

GType         clutter_list_model_get_type (void) G_GNUC_CONST;

ClutterModel *clutter_list_model_new      (guint                n_columns,
                                              ...);
ClutterModel *clutter_list_model_newv     (guint                n_columns,
                                           GType               *types,
                                           const gchar * const  names[]);

G_END_DECLS

#endif /* __CLUTTER_LIST_MODEL_H__ */
