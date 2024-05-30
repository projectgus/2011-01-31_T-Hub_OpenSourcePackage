/*
 * Clutter COGL
 *
 * A basic GL/GLES Abstraction/Utility Layer
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2007 OpenedHand
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

/* 
 * COGL
 * ====
 *
 * 'cogl' is a very simple abstraction layer which wraps GL and GLES.
 * 
 *
 * !!!! DO NOT USE THIS API YET OUTSIDE OF CLUTTER CORE !!!!
 *              THE API WILL FLUCTUATE WILDLY
 *
 * TODO:
 *  - Use ClutterReal for fixed/float params.
 *  - Add Perspective/viewport setup
 *  - Add Features..
 */

#ifndef __COGL_H__
#define __COGL_H__

#include <glib.h>
#include <clutter/clutter-color.h>
#include <clutter/clutter-feature.h>
#include <clutter/clutter-fixed.h>
#include <clutter/clutter-types.h>

#include <cogl/cogl-defines-gl.h>

G_BEGIN_DECLS

/* Enum declarations */

#define COGL_PIXEL_FORMAT_24    2
#define COGL_PIXEL_FORMAT_32    3
#define COGL_A_BIT              (1 << 4)
#define COGL_BGR_BIT            (1 << 5)
#define COGL_AFIRST_BIT         (1 << 6)
#define COGL_PREMULT_BIT        (1 << 7)
#define COGL_UNORDERED_MASK     0x0F
#define COGL_UNPREMULT_MASK     0x7F

/*sagem modifs <-internal gles*/
#define COGL_ENABLE_BLEND             (1<<1)
#define COGL_ENABLE_TEXTURE_2D        (1<<2)
#define COGL_ENABLE_ALPHA_TEST        (1<<3)
#define COGL_ENABLE_TEXTURE_RECT      (1<<4)
#define COGL_ENABLE_VERTEX_ARRAY      (1<<5)
#define COGL_ENABLE_TEXCOORD_ARRAY    (1<<6)
#define COGL_ENABLE_COLOR_ARRAY       (1<<7)

/**
 * CoglPixelFormat:
 * @COGL_PIXEL_FORMAT_ANY:
 * @COGL_PIXEL_FORMAT_A_8:
 * @COGL_PIXEL_FORMAT_RGB_888:
 * @COGL_PIXEL_FORMAT_BGR_888:
 * @COGL_PIXEL_FORMAT_RGBA_8888:
 * @COGL_PIXEL_FORMAT_BGRA_8888:
 * @COGL_PIXEL_FORMAT_ARGB_8888:
 * @COGL_PIXEL_FORMAT_ABGR_8888:
 * @COGL_PIXEL_FORMAT_RGBA_8888_PRE:
 * @COGL_PIXEL_FORMAT_BGRA_8888_PRE:
 * @COGL_PIXEL_FORMAT_ARGB_8888_PRE:
 * @COGL_PIXEL_FORMAT_ABGR_8888_PRE:
 * @COGL_PIXEL_FORMAT_RGB_565:
 * @COGL_PIXEL_FORMAT_RGBA_4444:
 * @COGL_PIXEL_FORMAT_RGBA_5551:
 * @COGL_PIXEL_FORMAT_RGBA_4444_PRE:
 * @COGL_PIXEL_FORMAT_RGBA_5551_PRE:
 * @COGL_PIXEL_FORMAT_YUV:
 * @COGL_PIXEL_FORMAT_G_8:
 *
 * Pixel formats used by COGL.
 */
typedef enum
{
  COGL_PIXEL_FORMAT_ANY           = 0,
  COGL_PIXEL_FORMAT_A_8           = 1 | COGL_A_BIT,

  COGL_PIXEL_FORMAT_RGB_565       = 4,
  COGL_PIXEL_FORMAT_RGBA_4444     = 5 | COGL_A_BIT,
  COGL_PIXEL_FORMAT_RGBA_5551     = 6 | COGL_A_BIT,
  COGL_PIXEL_FORMAT_YUV           = 7,
  COGL_PIXEL_FORMAT_G_8           = 8,
  
  COGL_PIXEL_FORMAT_RGB_888       =  COGL_PIXEL_FORMAT_24,

  COGL_PIXEL_FORMAT_BGR_888       = (COGL_PIXEL_FORMAT_24 |
                                     COGL_BGR_BIT),

  COGL_PIXEL_FORMAT_RGBA_8888     =  COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT,

  COGL_PIXEL_FORMAT_BGRA_8888     = (COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT           |
                                     COGL_BGR_BIT),

  COGL_PIXEL_FORMAT_ARGB_8888     = (COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT           |
                                     COGL_AFIRST_BIT),

  COGL_PIXEL_FORMAT_ABGR_8888     = (COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT           |
                                     COGL_BGR_BIT         |
                                     COGL_AFIRST_BIT),

  COGL_PIXEL_FORMAT_RGBA_8888_PRE = (COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT           |
                                     COGL_PREMULT_BIT),

  COGL_PIXEL_FORMAT_BGRA_8888_PRE = (COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT           |
                                     COGL_PREMULT_BIT     |
                                     COGL_BGR_BIT),

  COGL_PIXEL_FORMAT_ARGB_8888_PRE = (COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT           |
                                     COGL_PREMULT_BIT     |
                                     COGL_AFIRST_BIT),

  COGL_PIXEL_FORMAT_ABGR_8888_PRE = (COGL_PIXEL_FORMAT_32 |
                                     COGL_A_BIT           |
                                     COGL_PREMULT_BIT     |
                                     COGL_BGR_BIT         |
                                     COGL_AFIRST_BIT),
  
  COGL_PIXEL_FORMAT_RGBA_4444_PRE = (COGL_PIXEL_FORMAT_RGBA_4444 |
                                     COGL_A_BIT                  |
                                     COGL_PREMULT_BIT),

  COGL_PIXEL_FORMAT_RGBA_5551_PRE = (COGL_PIXEL_FORMAT_RGBA_5551 |
                                     COGL_A_BIT                  |
                                     COGL_PREMULT_BIT),
  
  
} CoglPixelFormat;

/**
 * CoglFeatureFlags:
 * @COGL_FEATURE_TEXTURE_RECTANGLE:
 * @COGL_FEATURE_TEXTURE_NPOT:
 * @COGL_FEATURE_TEXTURE_YUV:
 * @COGL_FEATURE_TEXTURE_READ_PIXELS:
 * @COGL_FEATURE_SHADERS_GLSL:
 * @COGL_FEATURE_OFFSCREEN:
 * @COGL_FEATURE_OFFSCREEN_MULTISAMPLE:
 * @COGL_FEATURE_OFFSCREEN_BLIT:
 * @COGL_FEATURE_FOUR_CLIP_PLANES:
 * @COGL_FEATURE_STENCIL_BUFFER:
 *
 * Flags for the supported features.
 */
typedef enum
{
  COGL_FEATURE_TEXTURE_RECTANGLE      = (1 << 1),
  COGL_FEATURE_TEXTURE_NPOT           = (1 << 2),
  COGL_FEATURE_TEXTURE_YUV            = (1 << 3),
  COGL_FEATURE_TEXTURE_READ_PIXELS    = (1 << 4),
  COGL_FEATURE_SHADERS_GLSL           = (1 << 5),
  COGL_FEATURE_OFFSCREEN              = (1 << 6),
  COGL_FEATURE_OFFSCREEN_MULTISAMPLE  = (1 << 7),
  COGL_FEATURE_OFFSCREEN_BLIT         = (1 << 8),
  COGL_FEATURE_FOUR_CLIP_PLANES       = (1 << 9),
  COGL_FEATURE_STENCIL_BUFFER         = (1 << 10)

} CoglFeatureFlags;

/**
 * CoglBufferTarget:
 * @COGL_WINDOW_BUFFER:
 * @COGL_MASK_BUFFER:
 * @COGL_OFFSCREEN_BUFFER:
 *
 *
 */
typedef enum
{
  COGL_WINDOW_BUFFER      = (1 << 1),
  COGL_MASK_BUFFER        = (1 << 2),
  COGL_OFFSCREEN_BUFFER   = (1 << 3)
  
} CoglBufferTarget;

/**
 * CoglTextureVertex:
 * @x: Model x-coordinate
 * @y: Model y-coordinate
 * @z: Model z-coordinate
 * @tx: Texture x-coordinate
 * @ty: Texture y-coordinate
 * @color: The color to use at this vertex. This is ignored if
 * @use_color is %FALSE when calling cogl_texture_polygon().
 *
 * Used to specify vertex information when calling cogl_texture_polygon().
 */
struct _CoglTextureVertex
{
  ClutterFixed x, y, z;
  ClutterFixed tx, ty;
  ClutterColor color;
};

typedef struct _CoglTextureVertex CoglTextureVertex;

/**
 * SECTION:cogl
 * @short_description: General purpose API
 *
 * General utility functions for COGL.
 */

/* Context manipulation */

/**
 * cogl_create_context:
 *
 * FIXME
 */
gboolean        cogl_create_context           (void);

/**
 * cogl_destroy_context:
 *
 * FIXME
 */
void            cogl_destroy_context          (void);

/* Misc */
/**
 * COGL_INVALID_HANDLE:
 *
 * A COGL handle that is not valid, used for unitialized handles as well as
 * error conditions.
 */
#define COGL_INVALID_HANDLE NULL

/**
 * CoglHandle:
 *
 * Type used for storing references to cogl objects, the CoglHandle is
 * a fully opaque type without any public data members.
 */
typedef gpointer CoglHandle;

/**
 * CoglFuncPtr:
 *
 * The type used by cogl for function pointers, note that this type
 * is used as a generic catch-all cast for function pointers and the
 * actual arguments and return type may be different.
 */
typedef void (* CoglFuncPtr) (void);

/**
 * cogl_get_features:
 *
 * Returns all of the features supported by COGL.
 *
 * Return value: A logical OR of all the supported COGL features.
 *
 * Since: 0.8
 */
ClutterFeatureFlags cogl_get_features         (void);

/**
 * cogl_features_available:
 * @features: A bitmask of features to check for
 *
 * Checks whether the given COGL features are available. Multiple
 * features can be checked for by or-ing them together with the '|'
 * operator. %TRUE is only returned if all of the requested features
 * are available.
 *
 * Return value: %TRUE if the features are available, %FALSE otherwise.
 */
gboolean        cogl_features_available       (CoglFeatureFlags    features);

/**
 * cogl_get_proc_address:
 * @name: the name of the function.
 *
 * Gets a pointer to a given GL or GL ES extension function. This acts
 * as a wrapper around glXGetProcAddress() or whatever is the
 * appropriate function for the current backend.
 *
 * Return value: a pointer to the requested function or %NULL if the
 * function is not available.
 */
CoglFuncPtr     cogl_get_proc_address         (const gchar        *name);

/**
 * cogl_check_extension:
 * @name: extension to check for
 * @ext: list of extensions
 *
 * Check whether @name occurs in list of extensions in @ext.
 *
 * Returns: %TRUE if the extension occurs in the list, %FALSE otherwize.
 */
gboolean        cogl_check_extension          (const gchar        *name,
                                               const gchar        *ext);

/**
 * cogl_get_bitmasks:
 * @red: Return location for the number of red bits or %NULL
 * @green: Return location for the number of green bits or %NULL
 * @blue: Return location for the number of blue bits or %NULL
 * @alpha: Return location for the number of alpha bits or %NULL
 *
 * Gets the number of bitplanes used for each of the color components
 * in the color buffer. Pass %NULL for any of the arguments if the
 * value is not required.
 */
void            cogl_get_bitmasks             (gint               *red,
                                               gint               *green,
                                               gint               *blue,
                                               gint               *alpha);

/**
 * cogl_perspective:
 * @fovy: Vertical of view angle in degrees.
 * @aspect: Aspect ratio of diesplay
 * @z_near: Nearest visible point
 * @z_far: Furthest visible point along the z-axis
 *
 * Multiplies the current set matrix with a projection matrix based
 * on the provided values.
 */
void            cogl_perspective              (ClutterFixed        fovy,
                                               ClutterFixed        aspect,
                                               ClutterFixed        z_near,
                                               ClutterFixed        z_far);

/**
 * cogl_setup_viewport:
 * @width: Width of the viewport
 * @height: Height of the viewport
 * @fovy: Field of view angle in degrees
 * @aspect: Aspect ratio to determine the field of view along the x-axis
 * @z_near: Nearest visible point along the z-axis
 * @z_far: Furthest visible point along the z-axis
 *
 * Replaces the current viewport and projection matrix with the given
 * values. The viewport is placed at the top left corner of the window
 * with the given width and height. The projection matrix is replaced
 * with one that has a viewing angle of @fovy along the y-axis and a
 * view scaled according to @aspect along the x-axis. The view is
 * clipped according to @z_near and @z_far on the z-axis.
 */
void            cogl_setup_viewport           (guint               width,
                                               guint               height,
                                               ClutterFixed        fovy,
                                               ClutterFixed        aspect,
                                               ClutterFixed        z_near,
                                               ClutterFixed        z_far);

/**
 * cogl_push_matrix:
 *
 * Store the current model-view matrix on the matrix stack. The matrix
 * can later be restored with cogl_pop_matrix().
 */
void            cogl_push_matrix              (void);

/**
 * cogl_pop_matrix:
 *
 * Restore the current model-view matrix from the matrix stack.
 */
void            cogl_pop_matrix               (void);

/**
 * cogl_scale:
 * @x: Amount to scale along the x-axis
 * @y: Amount to scale along the y-axis
 *
 * Multiplies the current model-view matrix by one that scales the x
 * and y axes by the given values.
 */
void            cogl_scale                    (ClutterFixed        x,
                                               ClutterFixed        y);

/**
 * cogl_translatex:
 * @x: Distance to translate along the x-axis
 * @y: Distance to translate along the y-axis
 * @z: Distance to translate along the z-axis
 *
 * Multiplies the current model-view matrix by one that translates the
 * model along all three axes according to the given values.
 */
void            cogl_translatex               (ClutterFixed        x,
                                               ClutterFixed        y,
                                               ClutterFixed        z);

/**
 * cogl_translate:
 * @x: Distance to translate along the x-axis
 * @y: Distance to translate along the y-axis
 * @z: Distance to translate along the z-axis
 *
 * Integer version of cogl_translatex(). Multiplies the current
 * model-view matrix by one that translates the model along all three
 * axes according to the given values.
 */
void            cogl_translate                (gint                x,
                                               gint                y,
                                               gint                z);

/**
 * cogl_rotatex:
 * @angle: Angle in degrees to rotate.
 * @x: X-component of vertex to rotate around.
 * @y: Y-component of vertex to rotate around.
 * @z: Z-component of vertex to rotate around.
 *
 * Multiplies the current model-view matrix by one that rotates the
 * model around the vertex specified by @x, @y and @z. The rotation
 * follows the right-hand thumb rule so for example rotating by 10
 * degrees about the vertex (0, 0, 1) causes a small counter-clockwise
 * rotation.
 */
void            cogl_rotatex                  (ClutterFixed        angle,
                                               gint                x,
                                               gint                y,
                                               gint                z);

/**
 * cogl_rotate:
 * @angle: Angle in degrees to rotate.
 * @x: X-component of vertex to rotate around.
 * @y: Y-component of vertex to rotate around.
 * @z: Z-component of vertex to rotate around.
 *
 * Integer version of cogl_rotatex(). Multiplies the current
 * model-view matrix by one that rotates the model around the vertex
 * specified by @x, @y and @z.
 */
void            cogl_rotate                   (gint                angle,
                                               gint                x,
                                               gint                y,
                                               gint                z);

/**
 * cogl_get_modelview_matrix:
 * @m: pointer to a 4x4 array of #ClutterFixed<!-- -->s to receive the matrix
 *
 * Stores the current model-view matrix in @m. The matrix is in
 * column-major order.
 */
void            cogl_get_modelview_matrix     (ClutterFixed        m[16]);

/**
 * cogl_get_projection_matrix:
 * @m: pointer to a 4x4 array of #ClutterFixed<!-- -->s to receive the matrix
 *
 * Stores the current projection matrix in @m. The matrix is in
 * column-major order.
 */
void            cogl_get_projection_matrix    (ClutterFixed        m[16]);

/**
 * cogl_get_viewport:
 * @v: pointer to a 4 element array of #ClutterFixed<!-- -->s to
 * receive the viewport dimensions.
 *
 * Stores the current viewport in @v. @v[0] and @v[1] get the x and y
 * position of the viewport and @v[2] and @v[3] get the width and
 * height.
 */
void            cogl_get_viewport             (ClutterFixed        v[4]);

/**
 * cogl_clip_set:
 * @x_offset: left edge of the clip rectangle
 * @y_offset: top edge of the clip rectangle
 * @width: width of the clip rectangle
 * @height: height of the clip rectangle
 *
 * Specifies a rectangular clipping area for all subsequent drawing
 * operations. Any drawing commands that extend outside the rectangle
 * will be clipped so that only the portion inside the rectangle will
 * be displayed. The rectangle dimensions are transformed by the
 * current model-view matrix.
 */
void            cogl_clip_set                 (ClutterFixed        x_offset,
                                               ClutterFixed        y_offset,
                                               ClutterFixed        width,
                                               ClutterFixed        height);

/**
 * cogl_clip_unset:
 *
 * Removes the current clipping rectangle so that all drawing
 * operations extend to full size of the viewport again.
 */
void            cogl_clip_unset               (void);

/**
 * cogl_enable_depth_test:
 * @setting: %TRUE to enable depth testing or %FALSE to disable.
 *
 * Sets whether depth testing is enabled. If it is disabled then the
 * order that actors are layered on the screen depends solely on the
 * order specified using clutter_actor_raise() and
 * clutter_actor_lower(), otherwise it will also take into account the
 * actor's depth. Depth testing is disabled by default.
 */
void            cogl_enable_depth_test        (gboolean            setting);

/**
 * cogl_alpha_func:
 * @func: the comparison function to use, one of CGL_NEVER, CGL_LESS,
 * CGL_EQUAL, CGL_LEQUAL, CGL_GREATER, CGL_NOTEQUAL, CGL_GEQUAL and GL_ALWAYS.
 * @ref: reference value.
 *
 * Changes the alpha test to use the specified function specified in @func,
 * comparing with the value in @ref. The default function is CGL_ALWAYS the
 * initial reference value is 1.0.
 */
void            cogl_alpha_func               (COGLenum            func, 
                                               ClutterFixed        ref);

/**
 * cogl_fog_set:
 * @fog_color: The color of the fog
 * @density: Ignored
 * @z_near: Position along z-axis where no fogging should be applied
 * @z_far: Position along z-axes where full fogging should be applied
 *
 * Enables fogging. Fogging causes vertices that are further away from
 * the eye to be rendered with a different color. The color is
 * linearly interpolated so that vertices at @z_near are drawn fully
 * with their original color and vertices at @z_far are drawn fully
 * with @fog_color. Fogging will remain enabled until the next call to
 * cogl_paint_init().
 */
void            cogl_fog_set                  (const ClutterColor *fog_color,
                                               ClutterFixed        density,
                                               ClutterFixed        z_near,
                                               ClutterFixed        z_far);

/**
 * cogl_paint_init:
 * @color: Background color to clear to
 *
 * Clears the color buffer to @color. The depth buffer and stencil
 * buffers are also cleared and fogging and lighting are disabled.
 */
void            cogl_paint_init               (const ClutterColor *color);

/**
 * SECTION:cogl-texture
 * @short_description: Fuctions for creating and manipulating textures
 *
 * COGL allows creating and manipulating GL textures using a uniform
 * API that tries to hide all the various complexities of creating,
 * loading and manipulating textures.
 */

/* Textures api */

/**
 * cogl_texture_new_with_size:
 * @width: width of texture in pixels.
 * @height: height of texture in pixels.
 * @max_waste: maximum extra horizontal and|or vertical margin pixels to make
 * texture fit GPU limitations.
 * @auto_mipmap: enable or disable automatic generation of mipmap pyramid
 * from the base level image whenever it is updated.
 * @internal_format: the #CoglPixelFormat to use for the GPU storage of the
 * texture.
 *
 * Create a new texture with specified dimensions and pixel format.
 *
 * Returns: a #CoglHandle to the newly created texture or COGL_INVALID_HANDLE
 * if texture creation failed.
 */
CoglHandle      cogl_texture_new_with_size    (guint           width,
                                               guint           height,
                                               gint            max_waste,
                                               gboolean        auto_mipmap,
                                               CoglPixelFormat internal_format);

/**
 * cogl_texture_new_from_file:
 * @filename: the file to load
 * @max_waste: maximum extra horizontal and|or vertical margin pixels to make
 * texture fit GPU limitations.
 * @auto_mipmap: enable or disable automatic generation of mipmap pyramid
 * from the base level image whenever it is updated.
 * @internal_format: the #CoglPixelFormat to use for the GPU storage of the
 * texture.
 * @error: a #GError or NULL.
 *
 * Load an image file from disk.
 *
 * Returns: a #CoglHandle to the newly created texture or COGL_INVALID_HANDLE
 * if creating the texture failed.
 */
CoglHandle      cogl_texture_new_from_file    (const gchar    *filename,
                                               gint            max_waste,
                                               gboolean        auto_mipmap,
                                               CoglPixelFormat internal_format,
                                               GError        **error);

/**
 * cogl_texture_new_from_data:
 * @width: width of texture in pixels.
 * @height: height of texture in pixels.
 * @max_waste: maximum extra horizontal and|or vertical margin pixels to make
 * @auto_mipmap: enable or disable automatic generation of mipmap pyramid
 * from the base level image whenever it is updated.
 * @format: the #CoglPixelFormat the buffer is stored in in RAM
 * @internal_format: the #CoglPixelFormat that will be used for storing the
 * buffer on the GPU.
 * @rowstride: the memory offset in bytes between the starts of scanlines in
 * @data.
 * @data: pointer the memory region where the source buffer resides.
 *
 * Create a new cogl texture based on data residing in memory.
 *
 * Returns: a #CoglHandle to the newly created texture or COGL_INVALID_HANDLE
 * if creating the texture failed.
 */
CoglHandle      cogl_texture_new_from_data    (guint            width,
                                               guint            height,
                                               gint             max_waste,
                                               gboolean         auto_mipmap,
                                               CoglPixelFormat  format,
                                               CoglPixelFormat  internal_format,
                                               guint            rowstride,
                                               const guchar    *data);

/**
 * cogl_texture_new_from_foreign:
 * @gl_handle: opengl target type of foreign texture
 * @gl_target: opengl handle of foreign texture.
 * @width: width of foreign texture
 * @height: height of foreign texture.
 * @x_pot_waste: maximum horizontal waste.
 * @y_pot_waste: maximum vertical waste.
 * @format: format of the foreign texture.
 *
 * Create a cogl texture based on an existing OpenGL texture, the width, height
 * and format are passed along since it is not possible to query this from a
 * handle with GLES 1.0.
 *
 * Returns: a #CoglHandle to the newly created texture or COGL_INVALID_HANDLE
 * if creating the texture failed.
 */
CoglHandle      cogl_texture_new_from_foreign (GLuint              gl_handle,
                                               GLenum              gl_target,
                                               GLuint              width,
                                               GLuint              height,
                                               GLuint              x_pot_waste,
                                               GLuint              y_pot_waste,
                                               CoglPixelFormat     format);

/**
 * cogl_is_texture:
 * @handle: A CoglHandle
 *
 * Gets whether the given handle references an existing texture object.
 *
 * Returns: %TRUE if the handle references a texture,
 *   %FALSE otherwise
 */
gboolean        cogl_is_texture               (CoglHandle          handle);

/**
 * cogl_texture_get_width:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query the width of a cogl texture.
 *
 * Returns: the width of the GPU side texture in pixels:
 */
guint           cogl_texture_get_width        (CoglHandle          handle);

/**
 * cogl_texture_get_height:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query the height of a cogl texture.
 *
 * Returns: the height of the GPU side texture in pixels:
 */
guint           cogl_texture_get_height       (CoglHandle          handle);

/**
 * cogl_texture_get_format:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query the #CoglPixelFormat of a cogl texture.
 *
 * Returns: the #CoglPixelFormat of the GPU side texture.
 */
CoglPixelFormat cogl_texture_get_format       (CoglHandle          handle);


/**
 * cogl_texture_get_rowstride:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query the rowstride of a cogl texture.
 *
 * Returns: the offset in bytes between each consequetive row of pixels.
 */
guint           cogl_texture_get_rowstride    (CoglHandle          handle);

/**
 * cogl_texture_get_max_waste:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query the maximum wasted (unused) pixels in one dimension of a GPU side
 * texture.
 *
 * Returns: the maximum waste.
 */
gint            cogl_texture_get_max_waste    (CoglHandle          handle);

/**
 * cogl_texture_get_min_filter:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query the currently set downscaling filter for a cogl texture.
 *
 * Returns: the current downscaling filter for a cogl texture.
 */
COGLenum        cogl_texture_get_min_filter   (CoglHandle          handle);

/**
 * cogl_texture_get_mag_filter:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query the currently set downscaling filter for a cogl texture.
 *
 * Returns: the current downscaling filter for a cogl texture.
 */
COGLenum        cogl_texture_get_mag_filter   (CoglHandle          handle);

/**
 * cogl_texture_is_sliced:
 * @handle: a #CoglHandle for a texture.
 * 
 * Query if a texture is sliced (stored as multiple GPU side tecture
 * objects).
 *
 * Returns: %TRUE if the texture is sliced, %FALSE if the texture
 * is stored as a single GPU texture.
 */
gboolean        cogl_texture_is_sliced        (CoglHandle          handle);

/*sagem*/
gboolean cogl_texture_has_generated_slices (CoglHandle handle);
void cogl_texture_bind(CoglHandle handle,gint index_);
void     cogl_texture_get_n_slices         (CoglHandle handle,gint *n_x_tiles,gint *n_y_tiles);
void cogl_texture_get_x_tile_detail   (CoglHandle handle,gint x_index,gint *pos,gint *size,gint *waste);
void cogl_texture_get_y_tile_detail   (CoglHandle handle,gint y_index,gint *pos,gint *size,gint *waste);
void cogl_enable (gulong flags);

/**
 * cogl_texture_get_gl_texture:
 * @handle: a #CoglHandle for a texture.
 * @out_gl_handle: pointer to return location for the textures GL handle, or
 * NULL.
 * @out_gl_target: pointer to return location for the GL target type, or NULL.
 * 
 * Query the GL handles for a GPU side texture through it's #CoglHandle,
 * if the texture is spliced the data for the first sub texture will be
 * queried.
 *
 * Returns: %TRUE if the handle was successfully retrieved %FALSE
 * if the handle was invalid.
 */
gboolean        cogl_texture_get_gl_texture   (CoglHandle         handle,
                                               GLuint            *out_gl_handle,
                                               GLenum            *out_gl_target);

/**
 * cogl_texture_get_data:
 * @handle: a #CoglHandle for a texture.
 * @format: the #CoglPixelFormat to store the texture as.
 * @rowstride: the rowstride of @data or retrieved from texture if none is
 * specified.
 * @data: memory location to write contents of buffer, or %NULL if we're
 * only querying the data size through the return value.
 * 
 * Copy the pixel data from a cogl texture to system memory.
 *
 * Returns: the size of the texture data in bytes (or 0 if the texture
 * is not valid.)
 */
gint            cogl_texture_get_data         (CoglHandle          handle,
                                               CoglPixelFormat     format,
                                               guint               rowstride,
                                               guchar             *data);

/**
 * cogl_texture_set_filters:
 * @handle: a #CoglHandle.
 * @min_filter: the filter used when scaling the texture down.
 * @mag_filter: the filter used when magnifying the texture.
 * 
 * Changes the decimation and interpolation filters used when the texture is
 * drawn at other scales than 100%.
 */
void            cogl_texture_set_filters      (CoglHandle          handle,
                                               COGLenum            min_filter,
                                               COGLenum            mag_filter);


/**
 * cogl_texture_set_region:
 * @handle: a #CoglHandle.
 * @src_x: upper left coordinate to use from source data.
 * @src_y: upper left coordinate to use from source data.
 * @dst_x: upper left destination horizontal coordinate.
 * @dst_y: upper left destination vertical coordinate.
 * @dst_width: width of destination region to write.
 * @dst_height: height of destination region to write.
 * @width: width of source data buffer.
 * @height: height of source data buffer.
 * @format: the #CoglPixelFormat used in the source buffer.
 * @rowstride: rowstride of source buffer (computed from width if none
 * specified)
 * @data: the actual pixel data.
 *
 * Sets the pixels in a rectangular subregion of @handle from an in-memory
 * buffer containing pixel data. 
 *
 * Returns: %TRUE if the subregion upload was successful, otherwise %FALSE.
 */
gboolean        cogl_texture_set_region       (CoglHandle          handle,
                                               gint                src_x,
                                               gint                src_y,
                                               gint                dst_x,
                                               gint                dst_y,
                                               guint               dst_width,
                                               guint               dst_height,
                                               gint                width,
                                               gint                height,
                                               CoglPixelFormat     format,
                                               guint               rowstride,
                                               const guchar       *data);

/**
 * cogl_texture_ref:
 * @handle: a @CoglHandle.
 *
 * Increment the reference count for a cogl texture.
 *
 * Returns: the @handle.
 */
CoglHandle      cogl_texture_ref              (CoglHandle          handle);

/**
 * cogl_texture_unref:
 * @handle: a @CoglHandle.
 *
 * Deccrement the reference count for a cogl texture.
 */
void            cogl_texture_unref            (CoglHandle          handle);

/**
 * cogl_texture_rectangle:
 * @handle: a @CoglHandle.
 * @x1: x coordinate upper left on screen.
 * @y1: y coordinate upper left on screen.
 * @x2: x coordinate lower right on screen.
 * @y2: y coordinate lower right on screen.
 * @tx1: x part of texture coordinate to use for upper left pixel
 * @ty1: y part of texture coordinate to use for upper left pixel
 * @tx2: x part of texture coordinate to use for lower right pixel
 * @ty2: y part of texture coordinate to use for left pixel
 *
 * Draw a rectangle from a texture to the display, to draw the entire
 * texture pass in @tx1=0.0 @ty1=0.0 @tx2=1.0 @ty2=1.0.
 */
void            cogl_texture_rectangle        (CoglHandle          handle,
                                               ClutterFixed        x1,
                                               ClutterFixed        y1,
                                               ClutterFixed        x2,
                                               ClutterFixed        y2,
                                               ClutterFixed        tx1,
                                               ClutterFixed        ty1,
                                               ClutterFixed        tx2,
                                               ClutterFixed        ty2);

/**
 * cogl_texture_polygon:
 * @handle: A CoglHandle for a texture
 * @n_vertices: The length of the vertices array
 * @vertices: An array of #CoglTextureVertex structs
 * @use_color: %TRUE if the color member of #CoglTextureVertex should be used
 *
 * Draws a polygon from a texture with the given model and texture
 * coordinates. This can be used to draw arbitrary shapes textured
 * with a COGL texture. If @use_color is %TRUE then the current COGL
 * color will be changed for each vertex using the value specified in
 * the color member of #CoglTextureVertex. This can be used for
 * example to make the texture fade out by setting the alpha value of
 * the color.
 *
 * All of the texture coordinates must be in the range [0,1] and
 * repeating the texture is not supported.
 *
 * Because of the way this function is implemented it will currently
 * only work if either the texture is not sliced or the backend is not
 * OpenGL ES and the minifying and magnifying functions are both set
 * to CGL_NEAREST.
 */
void            cogl_texture_polygon          (CoglHandle          handle,
                                               guint               n_vertices,
                                               CoglTextureVertex  *vertices,
                                               gboolean            use_color);

/* Primitives API */

/**
 * SECTION:cogl-primitives
 * @short_description: Functions that draw various primitive shapes and
 * allow for construction of more complex paths.
 *
 * There are three levels on which drawing with cogl can be used. The
 * highest level functions construct various simple primitive shapes
 * to be either filled or stroked. Using a lower-level set of functions
 * more complex and arbitrary paths can be constructed by concatenating
 * straight line, bezier curve and arc segments. Additionally there
 * are utility functions that draw the most common primitives - rectangles
 * and trapezoids - in a maximaly optimized fashion.
 *
 * When constructing arbitrary paths, the current pen location is
 * initialized using the move_to command. The subsequent path segments
 * implicitly use the last pen location as their first vertex and move
 * the pen location to the last vertex they produce at the end. Also
 * there are special versions of functions that allow specifying the
 * vertices of the path segments relative to the last pen location
 * rather then in the absolute coordinates.
 */



/**
 * cogl_color:
 * @color: new current @ClutterColor.
 *
 * Changes the color of cogl's current paint, which is used for filling and stroking
 * primitives.
 */
void            cogl_color                    (const ClutterColor *color);


/**
 * cogl_rectangle:
 * @x: X coordinate of the top-left corner
 * @y: Y coordinate of the top-left corner
 * @width: Width of the rectangle
 * @height: Height of the rectangle
 *
 * Fills a rectangle at the given coordinates with the current
 * drawing color in a highly optimizied fashion.
 **/
void            cogl_rectangle                (gint                x,
                                               gint                y,
                                               guint               width,
                                               guint               height);

/**
 * cogl_rectanglex:
 * @x: X coordinate of the top-left corner
 * @y: Y coordinate of the top-left corner
 * @width: Width of the rectangle
 * @height: Height of the rectangle
 *
 * A fixed-point version of cogl_fast_fill_rectangle.
 **/
void            cogl_rectanglex               (ClutterFixed        x,
                                               ClutterFixed        y,
                                               ClutterFixed        width,
                                               ClutterFixed        height);

/**
 * cogl_path_fill:
 *
 * Fills the constructed shape using the current drawing color.
 **/
void            cogl_path_fill            (void);

/**
 * cogl_path_stroke:
 *
 * Strokes the constructed shape using the current drawing color
 * and a width of 1 pixel (regardless of the current transformation
 * matrix).
 **/
void            cogl_path_stroke          (void);


/**
 * cogl_path_move_to:
 * @x: X coordinate of the pen location to move to.
 * @y: Y coordinate of the pen location to move to.
 *
 * Clears the previously constructed shape and begins a new path
 * contour by moving the pen to the given coordinates.
 **/
void            cogl_path_move_to        (ClutterFixed        x,
                                          ClutterFixed        y);


/**
 * cogl_path_rel_move_to:
 * @x: X offset from the current pen location to move the pen to.
 * @y: Y offset from the current pen location to move the pen to.
 *
 * Clears the previously constructed shape and begins a new path
 * contour by moving the pen to the given coordinates relative
 * to the current pen location.
 **/
void            cogl_path_rel_move_to    (ClutterFixed        x,
                                          ClutterFixed        y);

/**
 * cogl_path_line_to:
 * @x: X coordinate of the end line vertex
 * @y: Y coordinate of the end line vertex
 *
 * Adds a straight line segment to the current path that ends at the
 * given coordinates.
 **/
void            cogl_path_line_to        (ClutterFixed        x,
                                          ClutterFixed        y);

/**
 * cogl_path_rel_line_to:
 * @x: X offset from the current pen location of the end line vertex
 * @y: Y offset from the current pen location of the end line vertex
 *
 * Adds a straight line segment to the current path that ends at the
 * given coordinates relative to the current pen location.
 **/
void            cogl_path_rel_line_to    (ClutterFixed        x,
                                          ClutterFixed        y);


/**
 * cogl_path_arc:
 * @center_x: X coordinate of the elliptical arc center
 * @center_y: Y coordinate of the elliptical arc center
 * @radius_x: X radius of the elliptical arc
 * @radius_y: Y radious of the elliptical arc
 * @angle_1: Angle in the unit-circle at which the arc begin
 * @angle_2: Angle in the unit-circle at which the arc ends
 *
 * Adds an elliptical arc segment to the current path. A straight line
 * segment will link the current pen location with the first vertex
 * of the arc. If you perform a move_to to the arcs start just before
 * drawing it you create a free standing arc.
 **/
void            cogl_path_arc                 (ClutterFixed        center_x,
                                               ClutterFixed        center_y,
                                               ClutterFixed        radius_x,
                                               ClutterFixed        radius_y,
                                               ClutterAngle        angle_1,
                                               ClutterAngle        angle_2);



/**
 * cogl_path_curve_to:
 * @x1: X coordinate of the second bezier control point
 * @y1: Y coordinate of the second bezier control point
 * @x2: X coordinate of the third bezier control point
 * @y2: Y coordinate of the third bezier control point
 * @x3: X coordinate of the fourth bezier control point
 * @y3: Y coordinate of the fourth bezier control point
 *
 * Adds a cubic bezier curve segment to the current path with the given
 * second, third and fourth control points and using current pen location
 * as the first control point.
 **/
void            cogl_path_curve_to            (ClutterFixed        x1,
                                               ClutterFixed        y1,
                                               ClutterFixed        x2,
                                               ClutterFixed        y2,
                                               ClutterFixed        x3,
                                               ClutterFixed        y3);

/**
 * cogl_path_rel_curve_to:
 * @x1: X coordinate of the second bezier control point
 * @y1: Y coordinate of the second bezier control point
 * @x2: X coordinate of the third bezier control point
 * @y2: Y coordinate of the third bezier control point
 * @x3: X coordinate of the fourth bezier control point
 * @y3: Y coordinate of the fourth bezier control point
 *
 * Adds a cubic bezier curve segment to the current path with the given
 * second, third and fourth control points and using current pen location
 * as the first control point. The given coordinates are relative to the
 * current pen location.
 */
void            cogl_path_rel_curve_to        (ClutterFixed        x1,
                                               ClutterFixed        y1,
                                               ClutterFixed        x2,
                                               ClutterFixed        y2,
                                               ClutterFixed        x3,
                                               ClutterFixed        y3);

/**
 * cogl_path_close:
 *
 * Closes the path being constructed by adding a straight line segment
 * to it that ends at the first vertex of the path.
 **/
void            cogl_path_close               (void);

/**
 * cogl_path_line:
 * @x1: X coordinate of the start line vertex
 * @y1: Y coordinate of the start line vertex
 * @x2: X coordinate of the end line vertex
 * @y2: Y coordinate of the end line vertex
 *
 * Clears the previously constructed shape and constructs a straight
 * line shape start and ending at the given coordinates.
 **/
void            cogl_path_line                (ClutterFixed        x1,
                                               ClutterFixed        y1,
                                               ClutterFixed        x2,
                                               ClutterFixed        y2);

/**
 * cogl_path_polyline:
 * @coords: A pointer to the first element of an array of fixed-point
 * values that specify the vertex coordinates.
 * @num_points: The total number of vertices.
 *
 * Clears the previously constructed shape and constructs a series of straight
 * line segments, starting from the first given vertex coordinate. Each
 * subsequent segment stars where the previous one ended and ends at the next
 * given vertex coordinate.
 *
 * The coords array must contain 2 * num_points values. The first value
 * represents the X coordinate of the first vertex, the second value
 * represents the Y coordinate of the first vertex, continuing in the same
 * fashion for the rest of the vertices. (num_points - 1) segments will
 * be constructed.
 **/
void            cogl_path_polyline            (ClutterFixed       *coords,
                                               gint                num_points);


/**
 * cogl_path_polygon:
 * @coords: A pointer to the first element of an array of fixed-point
 * values that specify the vertex coordinates.
 * @num_points: The total number of vertices.
 *
 * Clears the previously constructed shape and constructs a polygonal
 * shape of the given number of vertices.
 *
 * The coords array must contain 2 * num_points values. The first value
 * represents the X coordinate of the first vertex, the second value
 * represents the Y coordinate of the first vertex, continuing in the same
 * fashion for the rest of the vertices.
 **/
void            cogl_path_polygon             (ClutterFixed       *coords,
                                               gint                num_points);


/**
 * cogl_path_rectangle:
 * @x: X coordinate of the top-left corner.
 * @y: Y coordinate of the top-left corner.
 * @width: Rectangle width.
 * @height: Rectangle height.
 *
 * Clears the previously constructed shape and constructs a rectangular
 * shape at the given coordinates.
 **/
void            cogl_path_rectangle           (ClutterFixed        x,
                                               ClutterFixed        y,
                                               ClutterFixed        width,
                                               ClutterFixed        height);

/**
 * cogl_path_ellipse:
 * @center_x: X coordinate of the ellipse center
 * @center_y: Y coordinate of the ellipse center
 * @radius_x: X radius of the ellipse
 * @radius_y: Y radius of the ellipse
 *
 * Clears the previously constructed shape and constructs an ellipse
 * shape.
 **/
void            cogl_path_ellipse             (ClutterFixed        center_x,
                                               ClutterFixed        center_y,
                                               ClutterFixed        radius_x,
                                               ClutterFixed        radius_y);

/**
 * cogl_path_round_rectangle:
 * @x: X coordinate of the top-left corner
 * @y: Y coordinate of the top-left corner
 * @width: Width of the rectangle
 * @height: Height of the rectangle
 * @radius: Radius of the corner arcs.
 * @arc_step: Angle increment resolution for subdivision of
 * the corner arcs.
 *
 * Clears the previously constructed shape and constructs a rectangular
 * shape with rounded corners.
 **/
void            cogl_path_round_rectangle     (ClutterFixed        x,
                                               ClutterFixed        y,
                                               ClutterFixed        width,
                                               ClutterFixed        height,
                                               ClutterFixed        radius,
                                               ClutterAngle        arc_step);

/**
 * SECTION:cogl-shaders
 * @short_description: Fuctions for accessing the programmable GL pipeline
 *
 * COGL allows accessing the GL programmable pipeline in order to create
 * vertex and fragment shaders.
 *
 * The only supported format is GLSL shaders.
 */

/**
 * cogl_create_shader:
 * @shader_type: CGL_VERTEX_SHADER or CGL_FRAGMENT_SHADER.
 *
 * Create a new shader handle, use #cogl_shader_source to set the source code
 * to be used on it.
 *
 * Returns: a new shader handle.
 */
CoglHandle      cogl_create_shader            (COGLenum            shader_type);

/**
 * cogl_shader_ref:
 * @handle: A #CoglHandle to a shader.
 *
 * Add an extra reference to a shader.
 *
 * Returns: @handle
 */
CoglHandle      cogl_shader_ref               (CoglHandle          handle);

/**
 * cogl_shader_unref:
 * @handle: A #CoglHandle to a shader.
 *
 * Removes a reference to a shader. If it was the last reference the
 * shader object will be destroyed.
 */
void            cogl_shader_unref             (CoglHandle          handle);

/**
 * cogl_is_shader:
 * @handle: A CoglHandle
 *
 * Gets whether the given handle references an existing shader object.
 *
 * Returns: %TRUE if the handle references a shader,
 *   %FALSE otherwise
 */
gboolean        cogl_is_shader               (CoglHandle          handle);

/**
 * cogl_shader_source:
 * @shader: #CoglHandle for a shader.
 * @source: GLSL shader source.
 *
 * Replaces the current GLSL source associated with a shader with a new
 * one.
 */
void            cogl_shader_source            (CoglHandle          shader,
                                               const gchar        *source);
/**
 * cogl_shader_compile:
 * @handle: #CoglHandle for a shader.
 *
 * Compiles the shader, no return value, but the shader is now ready for
 * linking into a program.
 */
void            cogl_shader_compile           (CoglHandle        handle);

/**
 * cogl_shader_get_info_log:
 * @handle: #CoglHandle for a shader.
 * @size: maximum number of bytes to retrieve.
 * @buffer: location for info log.
 *
 * Retrieves the information log for a coglobject, can be used in conjunction
 * with #cogl_shader_get_parameteriv to retrieve the compiler warnings/error
 * messages that caused a shader to not compile correctly, mainly useful for
 * debugging purposes.
 */
void            cogl_shader_get_info_log      (CoglHandle          handle,
                                               guint               size,
                                               gchar              *buffer);

/**
 * cogl_shader_get_parameteriv:
 * @handle: #CoglHandle for a shader.
 * @pname: the named COGL parameter to retrieve.
 * @dest: storage location for COGLint return value.
 *
 * Retrieve a named parameter from a shader can be used to query to compile
 * satus of a shader by passing in CGL_OBJECT_COMPILE_STATUS for @pname.
 */
void            cogl_shader_get_parameteriv   (CoglHandle          handle,
                                               COGLenum            pname,
                                               COGLint            *dest);

/**
 * cogl_create_program:
 *
 * Create a new cogl program object that can be used to replace parts of the GL
 * rendering pipeline with custom code.
 *
 * Returns: a new cogl program.
 */
CoglHandle      cogl_create_program           (void);

/**
 * cogl_program_ref:
 * @handle: A #CoglHandle to a program.
 *
 * Add an extra reference to a program.
 *
 * Returns: @handle
 */
CoglHandle      cogl_program_ref              (CoglHandle        handle);

/**
 * cogl_program_unref:
 * @handle: A #CoglHandle to a program.
 *
 * Removes a reference to a program. If it was the last reference the
 * program object will be destroyed.
 */
void            cogl_program_unref            (CoglHandle        handle);

/**
 * cogl_is_program:
 * @handle: A CoglHandle
 *
 * Gets whether the given handle references an existing program object.
 *
 * Returns: %TRUE if the handle references a program,
 *   %FALSE otherwise
 */
gboolean        cogl_is_program               (CoglHandle          handle);

/**
 * cogl_program_attach_shader:
 * @program_handle: a #CoglHandle for a shdaer program.
 * @shader_handle: a #CoglHandle for a vertex of fragment shader.
 *
 * Attaches a shader to a program object, a program can have one vertex shader
 * and one fragment shader attached.
 */
void            cogl_program_attach_shader    (CoglHandle        program_handle,
                                               CoglHandle        shader_handle);


/**
 * cogl_program_link:
 * @handle: a #CoglHandle for a shader program.
 *
 * Links a program making it ready for use.
 */
void            cogl_program_link             (CoglHandle        handle);

/**
 * cogl_program_use:
 * @handle: a #CoglHandle for a shader program or COGL_INVALID_HANDLE.
 *
 * Activate a specific shader program replacing that part of the GL
 * rendering pipeline, if passed in COGL_INVALID_HANDLE the default
 * behavior of GL is reinstated.
 */
void            cogl_program_use              (CoglHandle        handle);

/**
 * cogl_program_get_uniform_location:
 * @handle: a #CoglHandle for a shader program.
 * @uniform_name: the name of a uniform.
 *
 * Retrieve the location (offset) of a uniform variable in a shader program, a
 * uniform is a variable that is constant for all vertices/fragments for a
 * shader object and is possible to modify as an external parameter.
 *
 * Returns: the offset of a uniform in a specified program, this uniform can be
 * set using #cogl_program_uniform_1f when the program is in use.
 */
COGLint         cogl_program_get_uniform_location
                                              (CoglHandle        handle,
                                               const gchar      *uniform_name);


/**
 * cogl_program_uniform_1f:
 * @uniform_no: the unform to set.
 * @value: the new value of the uniform.
 *
 * Changes the value of a uniform in the currently used (see #cogl_program_use)
 * shader program.
 */
void            cogl_program_uniform_1f       (COGLint           uniform_no,
                                               gfloat            value);

/**
 * SECTION:cogl-offscreen
 * @short_description: Fuctions for creating and manipulating offscreen
 *   frame buffer objects
 *
 * COGL allows creating and operating on FBOs (Framebuffer Objects).
 */

/* Offscreen api */

/**
 * cogl_offscreen_new_to_texture:
 * @texhandle:
 *
 * Returns:
 */
CoglHandle      cogl_offscreen_new_to_texture (CoglHandle         texhandle);

/**
 * cogl_offscreen_new_multisample:
 * 
 *
 * Returns:
 */
CoglHandle      cogl_offscreen_new_multisample (void);

/**
 * cogl_offscreen_ref:
 * @handle:
 *
 * Returns:
 */
CoglHandle      cogl_offscreen_ref            (CoglHandle          handle);

/**
 * cogl_is_offscreen:
 * @handle: A CoglHandle
 *
 * Gets whether the given handle references an existing offscreen
 * buffer object.
 *
 * Returns: %TRUE if the handle references an offscreen buffer,
 *   %FALSE otherwise
 */
gboolean        cogl_is_offscreen             (CoglHandle          handle);

/**
 * cogl_offscreen_unref:
 * @handle:
 *
 */
void            cogl_offscreen_unref          (CoglHandle          handle);

/**
 * cogl_offscreen_blit:
 * @src_buffer:
 * @dst_buffer:
 *
 */
void            cogl_offscreen_blit           (CoglHandle          src_buffer,
                                               CoglHandle          dst_buffer);

/**
 * cogl_offscreen_blit_region:
 * @src_buffer:
 * @dst_buffer:
 * @src_x:
 * @src_y:
 * @src_w:
 * @src_h:
 * @dst_x:
 * @dst_y:
 * @dst_w:
 * @dst_h:
 *
 */
void            cogl_offscreen_blit_region    (CoglHandle          src_buffer,
                                               CoglHandle          dst_buffer,
                                               gint                src_x,
                                               gint                src_y,
                                               gint                src_w,
                                               gint                src_h,
                                               gint                dst_x,
                                               gint                dst_y,
                                               gint                dst_w,
                                               gint                dst_h);

/**
 * cogl_draw_buffer:
 * @target:
 * @offscreen:
 *
 */
void            cogl_draw_buffer              (CoglBufferTarget    target,
                                               CoglHandle          offscreen);

G_END_DECLS

#endif /* __COGL_H__ */
