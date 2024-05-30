/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006 OpenedHand
 *
 * Modified by Sagemcom under LGPL license on 20/08/2008
 * Copyright (c) 2010 Sagemcom All rights reserved. 
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

#ifndef _HAVE_CLUTTER_TEXTURE_H
#define _HAVE_CLUTTER_TEXTURE_H

#include <clutter/clutter-actor.h>
#include <cogl/cogl.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_TEXTURE            (clutter_texture_get_type ())
#define CLUTTER_TEXTURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLUTTER_TYPE_TEXTURE, ClutterTexture))
#define CLUTTER_TEXTURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLUTTER_TYPE_TEXTURE, ClutterTextureClass))
#define CLUTTER_IS_TEXTURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLUTTER_TYPE_TEXTURE))
#define CLUTTER_IS_TEXTURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLUTTER_TYPE_TEXTURE))
#define CLUTTER_TEXTURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLUTTER_TYPE_TEXTURE, ClutterTextureClass))

#define CLUTTER_TYPE_TEXTURE_HANDLE     (clutter_texture_handle_get_type ())

/**
 * ClutterTextureError:
 * @CLUTTER_TEXTURE_ERROR_OUT_OF_MEMORY: OOM condition
 * @CLUTTER_TEXTURE_ERROR_NO_YUV: YUV operation attempted but no YUV support
 *   found
 * @CLUTTER_TEXTURE_ERROR_BAD_FORMAT: The requested format for
 * clutter_texture_set_from_rgb_data or
 * clutter_texture_set_from_yuv_data is unsupported.
 *
 * Error enumeration for #ClutterTexture
 *
 * Since: 0.4
 */
typedef enum {
  CLUTTER_TEXTURE_ERROR_OUT_OF_MEMORY,
  CLUTTER_TEXTURE_ERROR_NO_YUV,
  CLUTTER_TEXTURE_ERROR_BAD_FORMAT
} ClutterTextureError;

#define CLUTTER_TEXTURE_ERROR   (clutter_texture_error_quark ())
GQuark clutter_texture_error_quark (void);

typedef struct _ClutterTexture        ClutterTexture;
typedef struct _ClutterTextureClass   ClutterTextureClass;
typedef struct _ClutterTexturePrivate ClutterTexturePrivate;

struct _ClutterTexture
{
  /*< private >*/
  ClutterActor         parent;

  ClutterTexturePrivate *priv;
};

struct _ClutterTextureClass
{
  ClutterActorClass parent_class;

  void (*size_change)   (ClutterTexture *texture, 
		         gint            width, 
		         gint            height);
  void (*pixbuf_change) (ClutterTexture *texture);

  /*< private >*/
  /* padding, for future expansion */
  void (*_clutter_texture1) (void);
  void (*_clutter_texture2) (void);
  void (*_clutter_texture3) (void);
  void (*_clutter_texture4) (void);
  void (*_clutter_texture5) (void);
  void (*_clutter_texture6) (void);
};

/**
 * ClutterTextureFlags:
 * @CLUTTER_TEXTURE_RGB_FLAG_BGR: FIXME
 * @CLUTTER_TEXTURE_RGB_FLAG_PREMULT: FIXME
 * @CLUTTER_TEXTURE_YUV_FLAG_YUV2: FIXME
 *
 * Flags for clutter_texture_set_from_rgb_data() and
 * clutter_texture_set_from_yuv_data().
 *
 * Since: 0.4
 */
typedef enum { /*< prefix=CLUTTER_TEXTURE >*/
    CLUTTER_TEXTURE_RGB_FLAG_BGR     = 1 << 1,
    CLUTTER_TEXTURE_RGB_FLAG_PREMULT = 1 << 2, /* FIXME: not handled */
    CLUTTER_TEXTURE_YUV_FLAG_YUV2    = 1 << 3

    /* FIXME: add compressed types ? */
} ClutterTextureFlags;

/**
 * ClutterTextureQuality:
 * @CLUTTER_TEXTURE_QUALITY_LOW: fastest rendering will use nearest neighbour
 *   interpolation when rendering. good setting.
 * @CLUTTER_TEXTURE_QUALITY_MEDIUM: higher quality rendering without using
 *   extra resources.
 * @CLUTTER_TEXTURE_QUALITY_HIGH: render the texture with the best quality
 *   available using extra memory.
 *
 * Enumaration controlling the texture quality.
 *
 * Since: 0.8
 */
typedef enum { /*< prefix=CLUTTER_TEXTURE_QUALITY >*/
  CLUTTER_TEXTURE_QUALITY_LOW = 0,
  CLUTTER_TEXTURE_QUALITY_MEDIUM,
  CLUTTER_TEXTURE_QUALITY_HIGH
} ClutterTextureQuality;

GType clutter_texture_get_type (void) G_GNUC_CONST;
GType clutter_texture_handle_get_type (void) G_GNUC_CONST;

ClutterActor *       clutter_texture_new                    (void);
ClutterActor *       clutter_texture_new_from_file          (const gchar            *filename,
                                                             GError                **error);
ClutterActor *       clutter_texture_new_from_actor         (ClutterActor           *actor);
gboolean             clutter_texture_set_from_file          (ClutterTexture         *texture,
                                                             const gchar            *filename,
                                                             GError                **error);
gboolean             clutter_texture_set_from_rgb_data      (ClutterTexture         *texture,
                                                             const guchar           *data,
                                                             gboolean                has_alpha,
                                                             gint                    width,
                                                             gint                    height,
                                                             gint                    rowstride,
                                                             gint                    bpp,
                                                             ClutterTextureFlags     flags,
                                                             GError                **error);
gboolean              clutter_texture_set_from_yuv_data     (ClutterTexture         *texture,
                                                             const guchar           *data,
                                                             gint                    width,
                                                             gint                    height,
                                                             ClutterTextureFlags     flags,
                                                             GError                **error);
gboolean             clutter_texture_set_area_from_rgb_data (ClutterTexture         *texture,
                                                             const guchar           *data,
                                                             gboolean                has_alpha,
                                                             gint                    x,
                                                             gint                    y,
                                                             gint                    width,
                                                             gint                    height,
                                                             gint                    rowstride,
                                                             gint                    bpp,
                                                             ClutterTextureFlags     flags,
                                                             GError                **error);
void                  clutter_texture_get_base_size         (ClutterTexture         *texture,
                                                             gint                   *width,
                                                             gint                   *height);
void                  clutter_texture_set_filter_quality    (ClutterTexture         *texture,
                                                             ClutterTextureQuality   filter_quality);
ClutterTextureQuality clutter_texture_get_filter_quality    (ClutterTexture         *texture);
void                  clutter_texture_set_max_tile_waste    (ClutterTexture         *texture,
                                                             gint                    max_tile_waste);
gint                  clutter_texture_get_max_tile_waste    (ClutterTexture         *texture);
CoglHandle            clutter_texture_get_cogl_texture      (ClutterTexture         *texture);
void                  clutter_texture_set_cogl_texture      (ClutterTexture         *texture,
                                                             CoglHandle              cogl_tex);
/*sagem*/
void     clutter_texture_bind_tile           (ClutterTexture *texture,gint index_);
gboolean clutter_texture_has_generated_tiles (ClutterTexture *texture);
gboolean clutter_texture_is_tiled            (ClutterTexture *texture);
void     clutter_texture_get_n_tiles         (ClutterTexture *texture,gint *n_x_tiles,gint *n_y_tiles);
void     clutter_texture_get_x_tile_detail   (ClutterTexture *texture,gint x_index,gint *pos,gint *size,gint *waste);
void     clutter_texture_get_y_tile_detail   (ClutterTexture *texture, gint y_index,gint *pos,gint *size,gint *waste);

G_END_DECLS

#endif /* _HAVE_CLUTTER_TEXTURE_H */
