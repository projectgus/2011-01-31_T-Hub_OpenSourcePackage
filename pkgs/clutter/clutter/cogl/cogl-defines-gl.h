/*
 * Clutter COGL
 *
 * A basic GL/GLES Abstraction/Utility Layer
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2007 OpenedHand
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

#ifndef __COGL_DEFINES_H__
#define __COGL_DEFINES_H__

#include <GL/gl.h>

G_BEGIN_DECLS

typedef GLenum COGLenum;
typedef GLint COGLint;
typedef GLuint COGLuint;

/* FIXME + DOCUMENT */

#define COPENSTEP OPENSTEP
#define CGLAPI GLAPI
#define CGLAPI GLAPI
#define CGLAPI GLAPI
#define CGLAPIENTRY GLAPIENTRY
#define CGLAPI GLAPI
#define CGLAPIENTRY GLAPIENTRY
#define CGLAPI GLAPI
#define CGLAPIENTRY GLAPIENTRY
#define CPRAGMA_EXPORT_SUPPORTED PRAGMA_EXPORT_SUPPORTED
#define CGLAPI GLAPI
#define CGLAPIENTRY GLAPIENTRY
#define CAPIENTRY APIENTRY
#define CAPIENTRYP APIENTRYP
#define CGLAPIENTRYP GLAPIENTRYP
#define CGL_FALSE GL_FALSE
#define CGL_TRUE GL_TRUE
#define CGL_BYTE GL_BYTE
#define CGL_UNSIGNED_BYTE GL_UNSIGNED_BYTE
#define CGL_SHORT GL_SHORT
#define CGL_UNSIGNED_SHORT GL_UNSIGNED_SHORT
#define CGL_INT GL_INT
#define CGL_UNSIGNED_INT GL_UNSIGNED_INT
#define CGL_FLOAT GL_FLOAT
#define CGL_DOUBLE GL_DOUBLE
#define CGL_POINTS GL_POINTS
#define CGL_LINES GL_LINES
#define CGL_LINE_LOOP GL_LINE_LOOP
#define CGL_LINE_STRIP GL_LINE_STRIP
#define CGL_TRIANGLES GL_TRIANGLES
#define CGL_TRIANGLE_STRIP GL_TRIANGLE_STRIP
#define CGL_TRIANGLE_FAN GL_TRIANGLE_FAN
#define CGL_QUADS GL_QUADS
#define CGL_QUAD_STRIP GL_QUAD_STRIP
#define CGL_POLYGON GL_POLYGON
#define CGL_VERTEX_ARRAY GL_VERTEX_ARRAY
#define CGL_NORMAL_ARRAY GL_NORMAL_ARRAY
#define CGL_COLOR_ARRAY GL_COLOR_ARRAY
#define CGL_INDEX_ARRAY GL_INDEX_ARRAY
#define CGL_TEXTURE_COORD_ARRAY GL_TEXTURE_COORD_ARRAY
#define CGL_EDGE_FLAG_ARRAY GL_EDGE_FLAG_ARRAY
#define CGL_VERTEX_ARRAY_SIZE GL_VERTEX_ARRAY_SIZE
#define CGL_VERTEX_ARRAY_TYPE GL_VERTEX_ARRAY_TYPE
#define CGL_VERTEX_ARRAY_STRIDE GL_VERTEX_ARRAY_STRIDE
#define CGL_NORMAL_ARRAY_TYPE GL_NORMAL_ARRAY_TYPE
#define CGL_NORMAL_ARRAY_STRIDE GL_NORMAL_ARRAY_STRIDE
#define CGL_COLOR_ARRAY_SIZE GL_COLOR_ARRAY_SIZE
#define CGL_COLOR_ARRAY_TYPE GL_COLOR_ARRAY_TYPE
#define CGL_COLOR_ARRAY_STRIDE GL_COLOR_ARRAY_STRIDE
#define CGL_INDEX_ARRAY_TYPE GL_INDEX_ARRAY_TYPE
#define CGL_INDEX_ARRAY_STRIDE GL_INDEX_ARRAY_STRIDE
#define CGL_TEXTURE_COORD_ARRAY_SIZE GL_TEXTURE_COORD_ARRAY_SIZE
#define CGL_TEXTURE_COORD_ARRAY_TYPE GL_TEXTURE_COORD_ARRAY_TYPE
#define CGL_TEXTURE_COORD_ARRAY_STRIDE GL_TEXTURE_COORD_ARRAY_STRIDE
#define CGL_EDGE_FLAG_ARRAY_STRIDE GL_EDGE_FLAG_ARRAY_STRIDE
#define CGL_VERTEX_ARRAY_POINTER GL_VERTEX_ARRAY_POINTER
#define CGL_NORMAL_ARRAY_POINTER GL_NORMAL_ARRAY_POINTER
#define CGL_COLOR_ARRAY_POINTER GL_COLOR_ARRAY_POINTER
#define CGL_INDEX_ARRAY_POINTER GL_INDEX_ARRAY_POINTER
#define CGL_TEXTURE_COORD_ARRAY_POINTER GL_TEXTURE_COORD_ARRAY_POINTER
#define CGL_EDGE_FLAG_ARRAY_POINTER GL_EDGE_FLAG_ARRAY_POINTER
#define CGL_MATRIX_MODE GL_MATRIX_MODE
#define CGL_MODELVIEW GL_MODELVIEW
#define CGL_PROJECTION GL_PROJECTION
#define CGL_TEXTURE GL_TEXTURE
#define CGL_POINT_SMOOTH GL_POINT_SMOOTH
#define CGL_POINT_SIZE GL_POINT_SIZE
#define CGL_POINT_SIZE_GRANULARITY GL_POINT_SIZE_GRANULARITY
#define CGL_POINT_SIZE_RANGE GL_POINT_SIZE_RANGE
#define CGL_LINE_SMOOTH GL_LINE_SMOOTH
#define CGL_LINE_STIPPLE GL_LINE_STIPPLE
#define CGL_LINE_STIPPLE_PATTERN GL_LINE_STIPPLE_PATTERN
#define CGL_LINE_STIPPLE_REPEAT GL_LINE_STIPPLE_REPEAT
#define CGL_LINE_WIDTH GL_LINE_WIDTH
#define CGL_LINE_WIDTH_GRANULARITY GL_LINE_WIDTH_GRANULARITY
#define CGL_LINE_WIDTH_RANGE GL_LINE_WIDTH_RANGE
#define CGL_POINT GL_POINT
#define CGL_LINE GL_LINE
#define CGL_FILL GL_FILL
#define CGL_CW GL_CW
#define CGL_CCW GL_CCW
#define CGL_FRONT GL_FRONT
#define CGL_BACK GL_BACK
#define CGL_POLYGON_MODE GL_POLYGON_MODE
#define CGL_POLYGON_SMOOTH GL_POLYGON_SMOOTH
#define CGL_POLYGON_STIPPLE GL_POLYGON_STIPPLE
#define CGL_EDGE_FLAG GL_EDGE_FLAG
#define CGL_CULL_FACE GL_CULL_FACE
#define CGL_CULL_FACE_MODE GL_CULL_FACE_MODE
#define CGL_FRONT_FACE GL_FRONT_FACE
#define CGL_POLYGON_OFFSET_FACTOR GL_POLYGON_OFFSET_FACTOR
#define CGL_POLYGON_OFFSET_UNITS GL_POLYGON_OFFSET_UNITS
#define CGL_POLYGON_OFFSET_POINT GL_POLYGON_OFFSET_POINT
#define CGL_POLYGON_OFFSET_LINE GL_POLYGON_OFFSET_LINE
#define CGL_POLYGON_OFFSET_FILL GL_POLYGON_OFFSET_FILL
#define CGL_COMPILE GL_COMPILE
#define CGL_COMPILE_AND_EXECUTE GL_COMPILE_AND_EXECUTE
#define CGL_LIST_BASE GL_LIST_BASE
#define CGL_LIST_INDEX GL_LIST_INDEX
#define CGL_LIST_MODE GL_LIST_MODE
#define CGL_NEVER GL_NEVER
#define CGL_LESS GL_LESS
#define CGL_EQUAL GL_EQUAL
#define CGL_LEQUAL GL_LEQUAL
#define CGL_GREATER GL_GREATER
#define CGL_NOTEQUAL GL_NOTEQUAL
#define CGL_GEQUAL GL_GEQUAL
#define CGL_ALWAYS GL_ALWAYS
#define CGL_DEPTH_TEST GL_DEPTH_TEST
#define CGL_DEPTH_BITS GL_DEPTH_BITS
#define CGL_DEPTH_CLEAR_VALUE GL_DEPTH_CLEAR_VALUE
#define CGL_DEPTH_FUNC GL_DEPTH_FUNC
#define CGL_DEPTH_RANGE GL_DEPTH_RANGE
#define CGL_DEPTH_WRITEMASK GL_DEPTH_WRITEMASK
#define CGL_DEPTH_COMPONENT GL_DEPTH_COMPONENT
#define CGL_LIGHTING GL_LIGHTING
#define CGL_SPOT_EXPONENT GL_SPOT_EXPONENT
#define CGL_SPOT_CUTOFF GL_SPOT_CUTOFF
#define CGL_CONSTANT_ATTENUATION GL_CONSTANT_ATTENUATION
#define CGL_LINEAR_ATTENUATION GL_LINEAR_ATTENUATION
#define CGL_QUADRATIC_ATTENUATION GL_QUADRATIC_ATTENUATION
#define CGL_AMBIENT GL_AMBIENT
#define CGL_DIFFUSE GL_DIFFUSE
#define CGL_SPECULAR GL_SPECULAR
#define CGL_SHININESS GL_SHININESS
#define CGL_EMISSION GL_EMISSION
#define CGL_POSITION GL_POSITION
#define CGL_SPOT_DIRECTION GL_SPOT_DIRECTION
#define CGL_AMBIENT_AND_DIFFUSE GL_AMBIENT_AND_DIFFUSE
#define CGL_COLOR_INDEXES GL_COLOR_INDEXES
#define CGL_LIGHT_MODEL_TWO_SIDE GL_LIGHT_MODEL_TWO_SIDE
#define CGL_LIGHT_MODEL_LOCAL_VIEWER GL_LIGHT_MODEL_LOCAL_VIEWER
#define CGL_LIGHT_MODEL_AMBIENT GL_LIGHT_MODEL_AMBIENT
#define CGL_FRONT_AND_BACK GL_FRONT_AND_BACK
#define CGL_SHADE_MODEL GL_SHADE_MODEL
#define CGL_FLAT GL_FLAT
#define CGL_SMOOTH GL_SMOOTH
#define CGL_COLOR_MATERIAL GL_COLOR_MATERIAL
#define CGL_COLOR_MATERIAL_FACE GL_COLOR_MATERIAL_FACE
#define CGL_COLOR_MATERIAL_PARAMETER GL_COLOR_MATERIAL_PARAMETER
#define CGL_NORMALIZE GL_NORMALIZE
#define CGL_ACCUM_RED_BITS GL_ACCUM_RED_BITS
#define CGL_ACCUM_GREEN_BITS GL_ACCUM_GREEN_BITS
#define CGL_ACCUM_BLUE_BITS GL_ACCUM_BLUE_BITS
#define CGL_ACCUM_ALPHA_BITS GL_ACCUM_ALPHA_BITS
#define CGL_ACCUM_CLEAR_VALUE GL_ACCUM_CLEAR_VALUE
#define CGL_ACCUM GL_ACCUM
#define CGL_ADD GL_ADD
#define CGL_LOAD GL_LOAD
#define CGL_MULT GL_MULT
#define CGL_RETURN GL_RETURN
#define CGL_ALPHA_TEST GL_ALPHA_TEST
#define CGL_ALPHA_TEST_REF GL_ALPHA_TEST_REF
#define CGL_ALPHA_TEST_FUNC GL_ALPHA_TEST_FUNC
#define CGL_BLEND GL_BLEND
#define CGL_BLEND_SRC GL_BLEND_SRC
#define CGL_BLEND_DST GL_BLEND_DST
#define CGL_ZERO GL_ZERO
#define CGL_ONE GL_ONE
#define CGL_SRC_COLOR GL_SRC_COLOR
#define CGL_ONE_MINUS_SRC_COLOR GL_ONE_MINUS_SRC_COLOR
#define CGL_SRC_ALPHA GL_SRC_ALPHA
#define CGL_ONE_MINUS_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
#define CGL_DST_ALPHA GL_DST_ALPHA
#define CGL_ONE_MINUS_DST_ALPHA GL_ONE_MINUS_DST_ALPHA
#define CGL_DST_COLOR GL_DST_COLOR
#define CGL_ONE_MINUS_DST_COLOR GL_ONE_MINUS_DST_COLOR
#define CGL_SRC_ALPHA_SATURATE GL_SRC_ALPHA_SATURATE
#define CGL_FEEDBACK GL_FEEDBACK
#define CGL_RENDER GL_RENDER
#define CGL_SELECT GL_SELECT
#define CGL_POINT_TOKEN GL_POINT_TOKEN
#define CGL_LINE_TOKEN GL_LINE_TOKEN
#define CGL_LINE_RESET_TOKEN GL_LINE_RESET_TOKEN
#define CGL_POLYGON_TOKEN GL_POLYGON_TOKEN
#define CGL_BITMAP_TOKEN GL_BITMAP_TOKEN
#define CGL_DRAW_PIXEL_TOKEN GL_DRAW_PIXEL_TOKEN
#define CGL_COPY_PIXEL_TOKEN GL_COPY_PIXEL_TOKEN
#define CGL_PASS_THROUGH_TOKEN GL_PASS_THROUGH_TOKEN
#define CGL_FEEDBACK_BUFFER_POINTER GL_FEEDBACK_BUFFER_POINTER
#define CGL_FEEDBACK_BUFFER_SIZE GL_FEEDBACK_BUFFER_SIZE
#define CGL_FEEDBACK_BUFFER_TYPE GL_FEEDBACK_BUFFER_TYPE
#define CGL_SELECTION_BUFFER_POINTER GL_SELECTION_BUFFER_POINTER
#define CGL_SELECTION_BUFFER_SIZE GL_SELECTION_BUFFER_SIZE
#define CGL_FOG GL_FOG
#define CGL_FOG_MODE GL_FOG_MODE
#define CGL_FOG_DENSITY GL_FOG_DENSITY
#define CGL_FOG_COLOR GL_FOG_COLOR
#define CGL_FOG_INDEX GL_FOG_INDEX
#define CGL_FOG_START GL_FOG_START
#define CGL_FOG_END GL_FOG_END
#define CGL_LINEAR GL_LINEAR
#define CGL_EXP GL_EXP
#define CGL_LOGIC_OP GL_LOGIC_OP
#define CGL_INDEX_LOGIC_OP GL_INDEX_LOGIC_OP
#define CGL_COLOR_LOGIC_OP GL_COLOR_LOGIC_OP
#define CGL_LOGIC_OP_MODE GL_LOGIC_OP_MODE
#define CGL_CLEAR GL_CLEAR
#define CGL_SET GL_SET
#define CGL_COPY GL_COPY
#define CGL_COPY_INVERTED GL_COPY_INVERTED
#define CGL_NOOP GL_NOOP
#define CGL_INVERT GL_INVERT
#define CGL_AND GL_AND
#define CGL_NAND GL_NAND
#define CGL_OR GL_OR
#define CGL_NOR GL_NOR
#define CGL_XOR GL_XOR
#define CGL_EQUIV GL_EQUIV
#define CGL_AND_REVERSE GL_AND_REVERSE
#define CGL_AND_INVERTED GL_AND_INVERTED
#define CGL_OR_REVERSE GL_OR_REVERSE
#define CGL_OR_INVERTED GL_OR_INVERTED
#define CGL_STENCIL_BITS GL_STENCIL_BITS
#define CGL_STENCIL_TEST GL_STENCIL_TEST
#define CGL_STENCIL_CLEAR_VALUE GL_STENCIL_CLEAR_VALUE
#define CGL_STENCIL_FUNC GL_STENCIL_FUNC
#define CGL_STENCIL_VALUE_MASK GL_STENCIL_VALUE_MASK
#define CGL_STENCIL_FAIL GL_STENCIL_FAIL
#define CGL_STENCIL_PASS_DEPTH_FAIL GL_STENCIL_PASS_DEPTH_FAIL
#define CGL_STENCIL_PASS_DEPTH_PASS GL_STENCIL_PASS_DEPTH_PASS
#define CGL_STENCIL_REF GL_STENCIL_REF
#define CGL_STENCIL_WRITEMASK GL_STENCIL_WRITEMASK
#define CGL_STENCIL_INDEX GL_STENCIL_INDEX
#define CGL_KEEP GL_KEEP
#define CGL_REPLACE GL_REPLACE
#define CGL_INCR GL_INCR
#define CGL_DECR GL_DECR
#define CGL_NONE GL_NONE
#define CGL_LEFT GL_LEFT
#define CGL_RIGHT GL_RIGHT
#define CGL_FRONT_LEFT GL_FRONT_LEFT
#define CGL_FRONT_RIGHT GL_FRONT_RIGHT
#define CGL_BACK_LEFT GL_BACK_LEFT
#define CGL_BACK_RIGHT GL_BACK_RIGHT
#define CGL_COLOR_INDEX GL_COLOR_INDEX
#define CGL_RED GL_RED
#define CGL_GREEN GL_GREEN
#define CGL_BLUE GL_BLUE
#define CGL_ALPHA GL_ALPHA
#define CGL_LUMINANCE GL_LUMINANCE
#define CGL_LUMINANCE_ALPHA GL_LUMINANCE_ALPHA
#define CGL_ALPHA_BITS GL_ALPHA_BITS
#define CGL_RED_BITS GL_RED_BITS
#define CGL_GREEN_BITS GL_GREEN_BITS
#define CGL_BLUE_BITS GL_BLUE_BITS
#define CGL_INDEX_BITS GL_INDEX_BITS
#define CGL_SUBPIXEL_BITS GL_SUBPIXEL_BITS
#define CGL_AUX_BUFFERS GL_AUX_BUFFERS
#define CGL_READ_BUFFER GL_READ_BUFFER
#define CGL_DRAW_BUFFER GL_DRAW_BUFFER
#define CGL_DOUBLEBUFFER GL_DOUBLEBUFFER
#define CGL_STEREO GL_STEREO
#define CGL_BITMAP GL_BITMAP
#define CGL_COLOR GL_COLOR
#define CGL_DEPTH GL_DEPTH
#define CGL_STENCIL GL_STENCIL
#define CGL_DITHER GL_DITHER
#define CGL_RGB GL_RGB
#define CGL_RGBA GL_RGBA
#define CGL_MAX_LIST_NESTING GL_MAX_LIST_NESTING
#define CGL_MAX_EVAL_ORDER GL_MAX_EVAL_ORDER
#define CGL_MAX_LIGHTS GL_MAX_LIGHTS
#define CGL_MAX_CLIP_PLANES GL_MAX_CLIP_PLANES
#define CGL_MAX_TEXTURE_SIZE GL_MAX_TEXTURE_SIZE
#define CGL_MAX_PIXEL_MAP_TABLE GL_MAX_PIXEL_MAP_TABLE
#define CGL_MAX_ATTRIB_STACK_DEPTH GL_MAX_ATTRIB_STACK_DEPTH
#define CGL_MAX_MODELVIEW_STACK_DEPTH GL_MAX_MODELVIEW_STACK_DEPTH
#define CGL_MAX_NAME_STACK_DEPTH GL_MAX_NAME_STACK_DEPTH
#define CGL_MAX_PROJECTION_STACK_DEPTH GL_MAX_PROJECTION_STACK_DEPTH
#define CGL_MAX_TEXTURE_STACK_DEPTH GL_MAX_TEXTURE_STACK_DEPTH
#define CGL_MAX_VIEWPORT_DIMS GL_MAX_VIEWPORT_DIMS
#define CGL_MAX_CLIENT_ATTRIB_STACK_DEPTH GL_MAX_CLIENT_ATTRIB_STACK_DEPTH
#define CGL_ATTRIB_STACK_DEPTH GL_ATTRIB_STACK_DEPTH
#define CGL_CLIENT_ATTRIB_STACK_DEPTH GL_CLIENT_ATTRIB_STACK_DEPTH
#define CGL_COLOR_CLEAR_VALUE GL_COLOR_CLEAR_VALUE
#define CGL_COLOR_WRITEMASK GL_COLOR_WRITEMASK
#define CGL_CURRENT_INDEX GL_CURRENT_INDEX
#define CGL_CURRENT_COLOR GL_CURRENT_COLOR
#define CGL_CURRENT_NORMAL GL_CURRENT_NORMAL
#define CGL_CURRENT_RASTER_COLOR GL_CURRENT_RASTER_COLOR
#define CGL_CURRENT_RASTER_DISTANCE GL_CURRENT_RASTER_DISTANCE
#define CGL_CURRENT_RASTER_INDEX GL_CURRENT_RASTER_INDEX
#define CGL_CURRENT_RASTER_POSITION GL_CURRENT_RASTER_POSITION
#define CGL_CURRENT_RASTER_TEXTURE_COORDS GL_CURRENT_RASTER_TEXTURE_COORDS
#define CGL_CURRENT_RASTER_POSITION_VALID GL_CURRENT_RASTER_POSITION_VALID
#define CGL_CURRENT_TEXTURE_COORDS GL_CURRENT_TEXTURE_COORDS
#define CGL_INDEX_CLEAR_VALUE GL_INDEX_CLEAR_VALUE
#define CGL_INDEX_MODE GL_INDEX_MODE
#define CGL_INDEX_WRITEMASK GL_INDEX_WRITEMASK
#define CGL_MODELVIEW_MATRIX GL_MODELVIEW_MATRIX
#define CGL_MODELVIEW_STACK_DEPTH GL_MODELVIEW_STACK_DEPTH
#define CGL_NAME_STACK_DEPTH GL_NAME_STACK_DEPTH
#define CGL_PROJECTION_MATRIX GL_PROJECTION_MATRIX
#define CGL_PROJECTION_STACK_DEPTH GL_PROJECTION_STACK_DEPTH
#define CGL_RENDER_MODE GL_RENDER_MODE
#define CGL_RGBA_MODE GL_RGBA_MODE
#define CGL_TEXTURE_MATRIX GL_TEXTURE_MATRIX
#define CGL_TEXTURE_STACK_DEPTH GL_TEXTURE_STACK_DEPTH
#define CGL_VIEWPORT GL_VIEWPORT
#define CGL_AUTO_NORMAL GL_AUTO_NORMAL
#define CGL_COEFF GL_COEFF
#define CGL_ORDER GL_ORDER
#define CGL_DOMAIN GL_DOMAIN
#define CGL_PERSPECTIVE_CORRECTION_HINT GL_PERSPECTIVE_CORRECTION_HINT
#define CGL_POINT_SMOOTH_HINT GL_POINT_SMOOTH_HINT
#define CGL_LINE_SMOOTH_HINT GL_LINE_SMOOTH_HINT
#define CGL_POLYGON_SMOOTH_HINT GL_POLYGON_SMOOTH_HINT
#define CGL_FOG_HINT GL_FOG_HINT
#define CGL_DONT_CARE GL_DONT_CARE
#define CGL_FASTEST GL_FASTEST
#define CGL_NICEST GL_NICEST
#define CGL_SCISSOR_BOX GL_SCISSOR_BOX
#define CGL_SCISSOR_TEST GL_SCISSOR_TEST
#define CGL_MAP_COLOR GL_MAP_COLOR
#define CGL_MAP_STENCIL GL_MAP_STENCIL
#define CGL_INDEX_SHIFT GL_INDEX_SHIFT
#define CGL_INDEX_OFFSET GL_INDEX_OFFSET
#define CGL_RED_SCALE GL_RED_SCALE
#define CGL_RED_BIAS GL_RED_BIAS
#define CGL_GREEN_SCALE GL_GREEN_SCALE
#define CGL_GREEN_BIAS GL_GREEN_BIAS
#define CGL_BLUE_SCALE GL_BLUE_SCALE
#define CGL_BLUE_BIAS GL_BLUE_BIAS
#define CGL_ALPHA_SCALE GL_ALPHA_SCALE
#define CGL_ALPHA_BIAS GL_ALPHA_BIAS
#define CGL_DEPTH_SCALE GL_DEPTH_SCALE
#define CGL_DEPTH_BIAS GL_DEPTH_BIAS
#define CGL_PIXEL_MAP_S_TO_S_SIZE GL_PIXEL_MAP_S_TO_S_SIZE
#define CGL_PIXEL_MAP_I_TO_I_SIZE GL_PIXEL_MAP_I_TO_I_SIZE
#define CGL_PIXEL_MAP_I_TO_R_SIZE GL_PIXEL_MAP_I_TO_R_SIZE
#define CGL_PIXEL_MAP_I_TO_G_SIZE GL_PIXEL_MAP_I_TO_G_SIZE
#define CGL_PIXEL_MAP_I_TO_B_SIZE GL_PIXEL_MAP_I_TO_B_SIZE
#define CGL_PIXEL_MAP_I_TO_A_SIZE GL_PIXEL_MAP_I_TO_A_SIZE
#define CGL_PIXEL_MAP_R_TO_R_SIZE GL_PIXEL_MAP_R_TO_R_SIZE
#define CGL_PIXEL_MAP_G_TO_G_SIZE GL_PIXEL_MAP_G_TO_G_SIZE
#define CGL_PIXEL_MAP_B_TO_B_SIZE GL_PIXEL_MAP_B_TO_B_SIZE
#define CGL_PIXEL_MAP_A_TO_A_SIZE GL_PIXEL_MAP_A_TO_A_SIZE
#define CGL_PIXEL_MAP_S_TO_S GL_PIXEL_MAP_S_TO_S
#define CGL_PIXEL_MAP_I_TO_I GL_PIXEL_MAP_I_TO_I
#define CGL_PIXEL_MAP_I_TO_R GL_PIXEL_MAP_I_TO_R
#define CGL_PIXEL_MAP_I_TO_G GL_PIXEL_MAP_I_TO_G
#define CGL_PIXEL_MAP_I_TO_B GL_PIXEL_MAP_I_TO_B
#define CGL_PIXEL_MAP_I_TO_A GL_PIXEL_MAP_I_TO_A
#define CGL_PIXEL_MAP_R_TO_R GL_PIXEL_MAP_R_TO_R
#define CGL_PIXEL_MAP_G_TO_G GL_PIXEL_MAP_G_TO_G
#define CGL_PIXEL_MAP_B_TO_B GL_PIXEL_MAP_B_TO_B
#define CGL_PIXEL_MAP_A_TO_A GL_PIXEL_MAP_A_TO_A
#define CGL_PACK_ALIGNMENT GL_PACK_ALIGNMENT
#define CGL_PACK_LSB_FIRST GL_PACK_LSB_FIRST
#define CGL_PACK_ROW_LENGTH GL_PACK_ROW_LENGTH
#define CGL_PACK_SKIP_PIXELS GL_PACK_SKIP_PIXELS
#define CGL_PACK_SKIP_ROWS GL_PACK_SKIP_ROWS
#define CGL_PACK_SWAP_BYTES GL_PACK_SWAP_BYTES
#define CGL_UNPACK_ALIGNMENT GL_UNPACK_ALIGNMENT
#define CGL_UNPACK_LSB_FIRST GL_UNPACK_LSB_FIRST
#define CGL_UNPACK_ROW_LENGTH GL_UNPACK_ROW_LENGTH
#define CGL_UNPACK_SKIP_PIXELS GL_UNPACK_SKIP_PIXELS
#define CGL_UNPACK_SKIP_ROWS GL_UNPACK_SKIP_ROWS
#define CGL_UNPACK_SWAP_BYTES GL_UNPACK_SWAP_BYTES
#define CGL_ZOOM_X GL_ZOOM_X
#define CGL_ZOOM_Y GL_ZOOM_Y
#define CGL_TEXTURE_ENV GL_TEXTURE_ENV
#define CGL_TEXTURE_ENV_MODE GL_TEXTURE_ENV_MODE
#define CGL_TEXTURE_WRAP_S GL_TEXTURE_WRAP_S
#define CGL_TEXTURE_WRAP_T GL_TEXTURE_WRAP_T
#define CGL_TEXTURE_MAG_FILTER GL_TEXTURE_MAG_FILTER
#define CGL_TEXTURE_MIN_FILTER GL_TEXTURE_MIN_FILTER
#define CGL_TEXTURE_ENV_COLOR GL_TEXTURE_ENV_COLOR
#define CGL_TEXTURE_GEN_S GL_TEXTURE_GEN_S
#define CGL_TEXTURE_GEN_T GL_TEXTURE_GEN_T
#define CGL_TEXTURE_GEN_MODE GL_TEXTURE_GEN_MODE
#define CGL_TEXTURE_BORDER_COLOR GL_TEXTURE_BORDER_COLOR
#define CGL_TEXTURE_WIDTH GL_TEXTURE_WIDTH
#define CGL_TEXTURE_HEIGHT GL_TEXTURE_HEIGHT
#define CGL_TEXTURE_BORDER GL_TEXTURE_BORDER
#define CGL_TEXTURE_COMPONENTS GL_TEXTURE_COMPONENTS
#define CGL_TEXTURE_RED_SIZE GL_TEXTURE_RED_SIZE
#define CGL_TEXTURE_GREEN_SIZE GL_TEXTURE_GREEN_SIZE
#define CGL_TEXTURE_BLUE_SIZE GL_TEXTURE_BLUE_SIZE
#define CGL_TEXTURE_ALPHA_SIZE GL_TEXTURE_ALPHA_SIZE
#define CGL_TEXTURE_LUMINANCE_SIZE GL_TEXTURE_LUMINANCE_SIZE
#define CGL_TEXTURE_INTENSITY_SIZE GL_TEXTURE_INTENSITY_SIZE
#define CGL_NEAREST_MIPMAP_NEAREST GL_NEAREST_MIPMAP_NEAREST
#define CGL_NEAREST_MIPMAP_LINEAR GL_NEAREST_MIPMAP_LINEAR
#define CGL_LINEAR_MIPMAP_NEAREST GL_LINEAR_MIPMAP_NEAREST
#define CGL_LINEAR_MIPMAP_LINEAR GL_LINEAR_MIPMAP_LINEAR
#define CGL_OBJECT_LINEAR GL_OBJECT_LINEAR
#define CGL_OBJECT_PLANE GL_OBJECT_PLANE
#define CGL_EYE_LINEAR GL_EYE_LINEAR
#define CGL_EYE_PLANE GL_EYE_PLANE
#define CGL_SPHERE_MAP GL_SPHERE_MAP
#define CGL_DECAL GL_DECAL
#define CGL_MODULATE GL_MODULATE
#define CGL_NEAREST GL_NEAREST
#define CGL_REPEAT GL_REPEAT
#define CGL_CLAMP GL_CLAMP
#define CGL_S GL_S
#define CGL_T GL_T
#define CGL_R GL_R
#define CGL_Q GL_Q
#define CGL_TEXTURE_GEN_R GL_TEXTURE_GEN_R
#define CGL_TEXTURE_GEN_Q GL_TEXTURE_GEN_Q
#define CGL_VENDOR GL_VENDOR
#define CGL_RENDERER GL_RENDERER
#define CGL_VERSION GL_VERSION
#define CGL_EXTENSIONS GL_EXTENSIONS
#define CGL_NO_ERROR GL_NO_ERROR
#define CGL_INVALID_ENUM GL_INVALID_ENUM
#define CGL_INVALID_VALUE GL_INVALID_VALUE
#define CGL_INVALID_OPERATION GL_INVALID_OPERATION
#define CGL_STACK_OVERFLOW GL_STACK_OVERFLOW
#define CGL_STACK_UNDERFLOW GL_STACK_UNDERFLOW
#define CGL_OUT_OF_MEMORY GL_OUT_OF_MEMORY
#define CGL_CURRENT_BIT GL_CURRENT_BIT
#define CGL_POINT_BIT GL_POINT_BIT
#define CGL_LINE_BIT GL_LINE_BIT
#define CGL_POLYGON_BIT GL_POLYGON_BIT
#define CGL_POLYGON_STIPPLE_BIT GL_POLYGON_STIPPLE_BIT
#define CGL_PIXEL_MODE_BIT GL_PIXEL_MODE_BIT
#define CGL_LIGHTING_BIT GL_LIGHTING_BIT
#define CGL_FOG_BIT GL_FOG_BIT
#define CGL_DEPTH_BUFFER_BIT GL_DEPTH_BUFFER_BIT
#define CGL_ACCUM_BUFFER_BIT GL_ACCUM_BUFFER_BIT
#define CGL_STENCIL_BUFFER_BIT GL_STENCIL_BUFFER_BIT
#define CGL_VIEWPORT_BIT GL_VIEWPORT_BIT
#define CGL_TRANSFORM_BIT GL_TRANSFORM_BIT
#define CGL_ENABLE_BIT GL_ENABLE_BIT
#define CGL_COLOR_BUFFER_BIT GL_COLOR_BUFFER_BIT
#define CGL_HINT_BIT GL_HINT_BIT
#define CGL_EVAL_BIT GL_EVAL_BIT
#define CGL_LIST_BIT GL_LIST_BIT
#define CGL_TEXTURE_BIT GL_TEXTURE_BIT
#define CGL_SCISSOR_BIT GL_SCISSOR_BIT
#define CGL_ALL_ATTRIB_BITS GL_ALL_ATTRIB_BITS
#define CGL_TEXTURE_PRIORITY GL_TEXTURE_PRIORITY
#define CGL_TEXTURE_RESIDENT GL_TEXTURE_RESIDENT
#define CGL_TEXTURE_INTERNAL_FORMAT GL_TEXTURE_INTERNAL_FORMAT
#define CGL_INTENSITY GL_INTENSITY
#define CGL_CLIENT_PIXEL_STORE_BIT GL_CLIENT_PIXEL_STORE_BIT
#define CGL_CLIENT_VERTEX_ARRAY_BIT GL_CLIENT_VERTEX_ARRAY_BIT
#define CGL_ALL_CLIENT_ATTRIB_BITS GL_ALL_CLIENT_ATTRIB_BITS
#define CGL_CLIENT_ALL_ATTRIB_BITS GL_CLIENT_ALL_ATTRIB_BITS
#define CGL_RESCALE_NORMAL GL_RESCALE_NORMAL
#define CGL_CLAMP_TO_EDGE GL_CLAMP_TO_EDGE
#define CGL_MAX_ELEMENTS_VERTICES GL_MAX_ELEMENTS_VERTICES
#define CGL_MAX_ELEMENTS_INDICES GL_MAX_ELEMENTS_INDICES
#define CGL_BGR GL_BGR
#define CGL_BGRA GL_BGRA
#define CGL_LIGHT_MODEL_COLOR_CONTROL GL_LIGHT_MODEL_COLOR_CONTROL
#define CGL_SINGLE_COLOR GL_SINGLE_COLOR
#define CGL_SEPARATE_SPECULAR_COLOR GL_SEPARATE_SPECULAR_COLOR
#define CGL_TEXTURE_MIN_LOD GL_TEXTURE_MIN_LOD
#define CGL_TEXTURE_MAX_LOD GL_TEXTURE_MAX_LOD
#define CGL_TEXTURE_BASE_LEVEL GL_TEXTURE_BASE_LEVEL
#define CGL_TEXTURE_MAX_LEVEL GL_TEXTURE_MAX_LEVEL
#define CGL_SMOOTH_POINT_SIZE_RANGE GL_SMOOTH_POINT_SIZE_RANGE
#define CGL_SMOOTH_POINT_SIZE_GRANULARITY GL_SMOOTH_POINT_SIZE_GRANULARITY
#define CGL_SMOOTH_LINE_WIDTH_RANGE GL_SMOOTH_LINE_WIDTH_RANGE
#define CGL_SMOOTH_LINE_WIDTH_GRANULARITY GL_SMOOTH_LINE_WIDTH_GRANULARITY
#define CGL_ALIASED_POINT_SIZE_RANGE GL_ALIASED_POINT_SIZE_RANGE
#define CGL_ALIASED_LINE_WIDTH_RANGE GL_ALIASED_LINE_WIDTH_RANGE
#define CGL_PACK_SKIP_IMAGES GL_PACK_SKIP_IMAGES
#define CGL_PACK_IMAGE_HEIGHT GL_PACK_IMAGE_HEIGHT
#define CGL_UNPACK_SKIP_IMAGES GL_UNPACK_SKIP_IMAGES
#define CGL_UNPACK_IMAGE_HEIGHT GL_UNPACK_IMAGE_HEIGHT
#define CGL_TEXTURE_DEPTH GL_TEXTURE_DEPTH
#define CGL_TEXTURE_WRAP_R GL_TEXTURE_WRAP_R
#define CGL_CONSTANT_COLOR GL_CONSTANT_COLOR
#define CGL_ONE_MINUS_CONSTANT_COLOR GL_ONE_MINUS_CONSTANT_COLOR
#define CGL_CONSTANT_ALPHA GL_CONSTANT_ALPHA
#define CGL_ONE_MINUS_CONSTANT_ALPHA GL_ONE_MINUS_CONSTANT_ALPHA
#define CGL_COLOR_TABLE GL_COLOR_TABLE
#define CGL_POST_CONVOLUTION_COLOR_TABLE GL_POST_CONVOLUTION_COLOR_TABLE
#define CGL_POST_COLOR_MATRIX_COLOR_TABLE GL_POST_COLOR_MATRIX_COLOR_TABLE
#define CGL_PROXY_COLOR_TABLE GL_PROXY_COLOR_TABLE
#define CGL_PROXY_POST_CONVOLUTION_COLOR_TABLE GL_PROXY_POST_CONVOLUTION_COLOR_TABLE
#define CGL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE
#define CGL_COLOR_TABLE_SCALE GL_COLOR_TABLE_SCALE
#define CGL_COLOR_TABLE_BIAS GL_COLOR_TABLE_BIAS
#define CGL_COLOR_TABLE_FORMAT GL_COLOR_TABLE_FORMAT
#define CGL_COLOR_TABLE_WIDTH GL_COLOR_TABLE_WIDTH
#define CGL_COLOR_TABLE_RED_SIZE GL_COLOR_TABLE_RED_SIZE
#define CGL_COLOR_TABLE_GREEN_SIZE GL_COLOR_TABLE_GREEN_SIZE
#define CGL_COLOR_TABLE_BLUE_SIZE GL_COLOR_TABLE_BLUE_SIZE
#define CGL_COLOR_TABLE_ALPHA_SIZE GL_COLOR_TABLE_ALPHA_SIZE
#define CGL_COLOR_TABLE_LUMINANCE_SIZE GL_COLOR_TABLE_LUMINANCE_SIZE
#define CGL_COLOR_TABLE_INTENSITY_SIZE GL_COLOR_TABLE_INTENSITY_SIZE
#define CGL_CONVOLUTION_BORDER_MODE GL_CONVOLUTION_BORDER_MODE
#define CGL_CONVOLUTION_FILTER_SCALE GL_CONVOLUTION_FILTER_SCALE
#define CGL_CONVOLUTION_FILTER_BIAS GL_CONVOLUTION_FILTER_BIAS
#define CGL_REDUCE GL_REDUCE
#define CGL_CONVOLUTION_FORMAT GL_CONVOLUTION_FORMAT
#define CGL_CONVOLUTION_WIDTH GL_CONVOLUTION_WIDTH
#define CGL_CONVOLUTION_HEIGHT GL_CONVOLUTION_HEIGHT
#define CGL_MAX_CONVOLUTION_WIDTH GL_MAX_CONVOLUTION_WIDTH
#define CGL_MAX_CONVOLUTION_HEIGHT GL_MAX_CONVOLUTION_HEIGHT
#define CGL_POST_CONVOLUTION_RED_SCALE GL_POST_CONVOLUTION_RED_SCALE
#define CGL_POST_CONVOLUTION_GREEN_SCALE GL_POST_CONVOLUTION_GREEN_SCALE
#define CGL_POST_CONVOLUTION_BLUE_SCALE GL_POST_CONVOLUTION_BLUE_SCALE
#define CGL_POST_CONVOLUTION_ALPHA_SCALE GL_POST_CONVOLUTION_ALPHA_SCALE
#define CGL_POST_CONVOLUTION_RED_BIAS GL_POST_CONVOLUTION_RED_BIAS
#define CGL_POST_CONVOLUTION_GREEN_BIAS GL_POST_CONVOLUTION_GREEN_BIAS
#define CGL_POST_CONVOLUTION_BLUE_BIAS GL_POST_CONVOLUTION_BLUE_BIAS
#define CGL_POST_CONVOLUTION_ALPHA_BIAS GL_POST_CONVOLUTION_ALPHA_BIAS
#define CGL_CONSTANT_BORDER GL_CONSTANT_BORDER
#define CGL_REPLICATE_BORDER GL_REPLICATE_BORDER
#define CGL_CONVOLUTION_BORDER_COLOR GL_CONVOLUTION_BORDER_COLOR
#define CGL_COLOR_MATRIX GL_COLOR_MATRIX
#define CGL_COLOR_MATRIX_STACK_DEPTH GL_COLOR_MATRIX_STACK_DEPTH
#define CGL_MAX_COLOR_MATRIX_STACK_DEPTH GL_MAX_COLOR_MATRIX_STACK_DEPTH
#define CGL_POST_COLOR_MATRIX_RED_SCALE GL_POST_COLOR_MATRIX_RED_SCALE
#define CGL_POST_COLOR_MATRIX_GREEN_SCALE GL_POST_COLOR_MATRIX_GREEN_SCALE
#define CGL_POST_COLOR_MATRIX_BLUE_SCALE GL_POST_COLOR_MATRIX_BLUE_SCALE
#define CGL_POST_COLOR_MATRIX_ALPHA_SCALE GL_POST_COLOR_MATRIX_ALPHA_SCALE
#define CGL_POST_COLOR_MATRIX_RED_BIAS GL_POST_COLOR_MATRIX_RED_BIAS
#define CGL_POST_COLOR_MATRIX_GREEN_BIAS GL_POST_COLOR_MATRIX_GREEN_BIAS
#define CGL_POST_COLOR_MATRIX_BLUE_BIAS GL_POST_COLOR_MATRIX_BLUE_BIAS
#define CGL_POST_COLOR_MATRIX_ALPHA_BIAS GL_POST_COLOR_MATRIX_ALPHA_BIAS
#define CGL_HISTOGRAM GL_HISTOGRAM
#define CGL_PROXY_HISTOGRAM GL_PROXY_HISTOGRAM
#define CGL_HISTOGRAM_WIDTH GL_HISTOGRAM_WIDTH
#define CGL_HISTOGRAM_FORMAT GL_HISTOGRAM_FORMAT
#define CGL_HISTOGRAM_RED_SIZE GL_HISTOGRAM_RED_SIZE
#define CGL_HISTOGRAM_GREEN_SIZE GL_HISTOGRAM_GREEN_SIZE
#define CGL_HISTOGRAM_BLUE_SIZE GL_HISTOGRAM_BLUE_SIZE
#define CGL_HISTOGRAM_ALPHA_SIZE GL_HISTOGRAM_ALPHA_SIZE
#define CGL_HISTOGRAM_LUMINANCE_SIZE GL_HISTOGRAM_LUMINANCE_SIZE
#define CGL_HISTOGRAM_SINK GL_HISTOGRAM_SINK
#define CGL_MINMAX GL_MINMAX
#define CGL_MINMAX_FORMAT GL_MINMAX_FORMAT
#define CGL_MINMAX_SINK GL_MINMAX_SINK
#define CGL_TABLE_TOO_LARGE GL_TABLE_TOO_LARGE
#define CGL_BLEND_EQUATION GL_BLEND_EQUATION
#define CGL_MIN GL_MIN
#define CGL_MAX GL_MAX
#define CGL_FUNC_ADD GL_FUNC_ADD
#define CGL_FUNC_SUBTRACT GL_FUNC_SUBTRACT
#define CGL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT
#define CGL_BLEND_COLOR GL_BLEND_COLOR
#define CGL_ACTIVE_TEXTURE GL_ACTIVE_TEXTURE
#define CGL_CLIENT_ACTIVE_TEXTURE GL_CLIENT_ACTIVE_TEXTURE
#define CGL_MAX_TEXTURE_UNITS GL_MAX_TEXTURE_UNITS
#define CGL_NORMAL_MAP GL_NORMAL_MAP
#define CGL_REFLECTION_MAP GL_REFLECTION_MAP
#define CGL_TEXTURE_CUBE_MAP GL_TEXTURE_CUBE_MAP
#define CGL_TEXTURE_BINDING_CUBE_MAP GL_TEXTURE_BINDING_CUBE_MAP
#define CGL_TEXTURE_CUBE_MAP_POSITIVE_X GL_TEXTURE_CUBE_MAP_POSITIVE_X
#define CGL_TEXTURE_CUBE_MAP_NEGATIVE_X GL_TEXTURE_CUBE_MAP_NEGATIVE_X
#define CGL_TEXTURE_CUBE_MAP_POSITIVE_Y GL_TEXTURE_CUBE_MAP_POSITIVE_Y
#define CGL_TEXTURE_CUBE_MAP_NEGATIVE_Y GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
#define CGL_TEXTURE_CUBE_MAP_POSITIVE_Z GL_TEXTURE_CUBE_MAP_POSITIVE_Z
#define CGL_TEXTURE_CUBE_MAP_NEGATIVE_Z GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
#define CGL_PROXY_TEXTURE_CUBE_MAP GL_PROXY_TEXTURE_CUBE_MAP
#define CGL_MAX_CUBE_MAP_TEXTURE_SIZE GL_MAX_CUBE_MAP_TEXTURE_SIZE
#define CGL_COMPRESSED_ALPHA GL_COMPRESSED_ALPHA
#define CGL_COMPRESSED_LUMINANCE GL_COMPRESSED_LUMINANCE
#define CGL_COMPRESSED_LUMINANCE_ALPHA GL_COMPRESSED_LUMINANCE_ALPHA
#define CGL_COMPRESSED_INTENSITY GL_COMPRESSED_INTENSITY
#define CGL_COMPRESSED_RGB GL_COMPRESSED_RGB
#define CGL_COMPRESSED_RGBA GL_COMPRESSED_RGBA
#define CGL_TEXTURE_COMPRESSION_HINT GL_TEXTURE_COMPRESSION_HINT
#define CGL_TEXTURE_COMPRESSED_IMAGE_SIZE GL_TEXTURE_COMPRESSED_IMAGE_SIZE
#define CGL_TEXTURE_COMPRESSED GL_TEXTURE_COMPRESSED
#define CGL_NUM_COMPRESSED_TEXTURE_FORMATS GL_NUM_COMPRESSED_TEXTURE_FORMATS
#define CGL_COMPRESSED_TEXTURE_FORMATS GL_COMPRESSED_TEXTURE_FORMATS
#define CGL_MULTISAMPLE GL_MULTISAMPLE
#define CGL_SAMPLE_ALPHA_TO_COVERAGE GL_SAMPLE_ALPHA_TO_COVERAGE
#define CGL_SAMPLE_ALPHA_TO_ONE GL_SAMPLE_ALPHA_TO_ONE
#define CGL_SAMPLE_COVERAGE GL_SAMPLE_COVERAGE
#define CGL_SAMPLE_BUFFERS GL_SAMPLE_BUFFERS
#define CGL_SAMPLES GL_SAMPLES
#define CGL_SAMPLE_COVERAGE_VALUE GL_SAMPLE_COVERAGE_VALUE
#define CGL_SAMPLE_COVERAGE_INVERT GL_SAMPLE_COVERAGE_INVERT
#define CGL_MULTISAMPLE_BIT GL_MULTISAMPLE_BIT
#define CGL_TRANSPOSE_MODELVIEW_MATRIX GL_TRANSPOSE_MODELVIEW_MATRIX
#define CGL_TRANSPOSE_PROJECTION_MATRIX GL_TRANSPOSE_PROJECTION_MATRIX
#define CGL_TRANSPOSE_TEXTURE_MATRIX GL_TRANSPOSE_TEXTURE_MATRIX
#define CGL_TRANSPOSE_COLOR_MATRIX GL_TRANSPOSE_COLOR_MATRIX
#define CGL_COMBINE GL_COMBINE
#define CGL_COMBINE_RGB GL_COMBINE_RGB
#define CGL_COMBINE_ALPHA GL_COMBINE_ALPHA
#define CGL_RGB_SCALE GL_RGB_SCALE
#define CGL_ADD_SIGNED GL_ADD_SIGNED
#define CGL_INTERPOLATE GL_INTERPOLATE
#define CGL_SUBTRACT GL_SUBTRACT
#define CGL_CONSTANT GL_CONSTANT
#define CGL_PRIMARY_COLOR GL_PRIMARY_COLOR
#define CGL_PREVIOUS GL_PREVIOUS
#define CGL_CLAMP_TO_BORDER GL_CLAMP_TO_BORDER
#define CGL_ACTIVE_TEXTURE_ARB GL_ACTIVE_TEXTURE_ARB
#define CGL_CLIENT_ACTIVE_TEXTURE_ARB GL_CLIENT_ACTIVE_TEXTURE_ARB
#define CGL_MAX_TEXTURE_UNITS_ARB GL_MAX_TEXTURE_UNITS_ARB
#define CGL_DEBUG_OBJECT_MESA GL_DEBUG_OBJECT_MESA
#define CGL_DEBUG_PRINT_MESA GL_DEBUG_PRINT_MESA
#define CGL_DEBUG_ASSERT_MESA GL_DEBUG_ASSERT_MESA
#define CGL_TRACE_ALL_BITS_MESA GL_TRACE_ALL_BITS_MESA
#define CGL_TRACE_OPERATIONS_BIT_MESA GL_TRACE_OPERATIONS_BIT_MESA
#define CGL_TRACE_PRIMITIVES_BIT_MESA GL_TRACE_PRIMITIVES_BIT_MESA
#define CGL_TRACE_ARRAYS_BIT_MESA GL_TRACE_ARRAYS_BIT_MESA
#define CGL_TRACE_TEXTURES_BIT_MESA GL_TRACE_TEXTURES_BIT_MESA
#define CGL_TRACE_PIXELS_BIT_MESA GL_TRACE_PIXELS_BIT_MESA
#define CGL_TRACE_ERRORS_BIT_MESA GL_TRACE_ERRORS_BIT_MESA
#define CGL_TRACE_MASK_MESA GL_TRACE_MASK_MESA
#define CGL_TRACE_NAME_MESA GL_TRACE_NAME_MESA
#define CGL_DEPTH_STENCIL_MESA GL_DEPTH_STENCIL_MESA
#define CGL_FRAGMENT_PROGRAM_POSITION_MESA GL_FRAGMENT_PROGRAM_POSITION_MESA
#define CGL_FRAGMENT_PROGRAM_CALLBACK_MESA GL_FRAGMENT_PROGRAM_CALLBACK_MESA
#define CGL_FRAGMENT_PROGRAM_CALLBACK_FUNC_MESA GL_FRAGMENT_PROGRAM_CALLBACK_FUNC_MESA
#define CGL_FRAGMENT_PROGRAM_CALLBACK_DATA_MESA GL_FRAGMENT_PROGRAM_CALLBACK_DATA_MESA
#define CGL_VERTEX_PROGRAM_POSITION_MESA GL_VERTEX_PROGRAM_POSITION_MESA
#define CGL_VERTEX_PROGRAM_CALLBACK_MESA GL_VERTEX_PROGRAM_CALLBACK_MESA
#define CGL_VERTEX_PROGRAM_CALLBACK_FUNC_MESA GL_VERTEX_PROGRAM_CALLBACK_FUNC_MESA
#define CGL_VERTEX_PROGRAM_CALLBACK_DATA_MESA GL_VERTEX_PROGRAM_CALLBACK_DATA_MESA
#define CGL_ALPHA_BLEND_EQUATION_ATI GL_ALPHA_BLEND_EQUATION_ATI
#define CGL_TIME_ELAPSED_EXT GL_TIME_ELAPSED_EXT
#define CGL_READ_FRAMEBUFFER_EXT GL_READ_FRAMEBUFFER_EXT
#define CGL_DRAW_FRAMEBUFFER_EXT GL_DRAW_FRAMEBUFFER_EXT
#define CGL_DRAW_FRAMEBUFFER_BINDING_EXT GL_DRAW_FRAMEBUFFER_BINDING_EXT
#define CGL_READ_FRAMEBUFFER_BINDING_EXT GL_READ_FRAMEBUFFER_BINDING_EXT
#define CGL_DEPTH_STENCIL_EXT GL_DEPTH_STENCIL_EXT
#define CGL_TEXTURE_STENCIL_SIZE_EXT GL_TEXTURE_STENCIL_SIZE_EXT
#define CGL_SRGB_EXT GL_SRGB_EXT
#define CGL_SRGB_ALPHA_EXT GL_SRGB_ALPHA_EXT
#define CGL_SLUMINANCE_ALPHA_EXT GL_SLUMINANCE_ALPHA_EXT
#define CGL_SLUMINANCE_EXT GL_SLUMINANCE_EXT
#define CGL_COMPRESSED_SRGB_EXT GL_COMPRESSED_SRGB_EXT
#define CGL_COMPRESSED_SRGB_ALPHA_EXT GL_COMPRESSED_SRGB_ALPHA_EXT
#define CGL_COMPRESSED_SLUMINANCE_EXT GL_COMPRESSED_SLUMINANCE_EXT
#define CGL_COMPRESSED_SLUMINANCE_ALPHA_EXT GL_COMPRESSED_SLUMINANCE_ALPHA_EXT

/* extras */

#define CGL_TEXTURE_2D GL_TEXTURE_2D
#define CGL_ARGB GL_ARGB
#define CGL_UNSIGNED_INT_8_8_8_8_REV GL_UNSIGNED_INT_8_8_8_8_REV

#ifdef GL_TEXTURE_RECTANGLE_ARB
#define CGL_TEXTURE_RECTANGLE_ARB GL_TEXTURE_RECTANGLE_ARB
#else
#define CGL_TEXTURE_RECTANGLE_ARB 0
#endif

#ifdef GL_YCBCR_MESA
#define CGL_YCBCR_MESA GL_YCBCR_MESA
#define CGL_UNSIGNED_SHORT_8_8_REV_MESA GL_UNSIGNED_SHORT_8_8_REV_MESA
#define CGL_UNSIGNED_SHORT_8_8_MESA GL_UNSIGNED_SHORT_8_8_MESA
#else
#define CGL_YCBCR_MESA 0
#define CGL_UNSIGNED_SHORT_8_8_REV_MESA 0
#define CGL_UNSIGNED_SHORT_8_8_MESA 0
#endif

#define CGL_FRAGMENT_SHADER GL_FRAGMENT_SHADER_ARB
#define CGL_VERTEX_SHADER   GL_VERTEX_SHADER_ARB

#define CGL_OBJECT_COMPILE_STATUS  GL_OBJECT_COMPILE_STATUS_ARB

#define CLUTTER_COGL_HAS_GL 1

/* Extension function prototypes */

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

typedef void
  (APIENTRYP             COGL_PFNGLGENRENDERBUFFERSEXTPROC)
  (GLsizei               n,
   GLuint               *renderbuffers);

typedef void
  (APIENTRYP             COGL_PFNGLBINDRENDERBUFFEREXTPROC)
  (GLenum                target,
   GLuint                renderbuffer);

typedef void
  (APIENTRYP             COGL_PFNGLRENDERBUFFERSTORAGEEXTPROC)
  (GLenum                target,
   GLenum                internalformat,
   GLsizei               width,
   GLsizei               height);

typedef void
  (APIENTRYP             COGL_PFNGLGENFRAMEBUFFERSEXTPROC)
  (GLsizei               n,
   GLuint               *framebuffers);

typedef void
  (APIENTRYP             COGL_PFNGLBINDFRAMEBUFFEREXTPROC)
  (GLenum                target,
   GLuint                framebuffer);

typedef void
  (APIENTRYP             COGL_PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)
  (GLenum                target,
   GLenum                attachment,
   GLenum                textarget,
   GLuint                texture,
   GLint                 level);

typedef void
  (APIENTRYP             COGL_PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)
  (GLenum                target,
   GLenum                attachment,
   GLenum                renderbuffertarget,
   GLuint                renderbuffer);

typedef GLenum
  (APIENTRYP             COGL_PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)
  (GLenum                target);

typedef void
  (APIENTRYP             COGL_PFNGLDELETEFRAMEBUFFERSEXTPROC)
  (GLsizei               n,
   const                 GLuint *framebuffers);

typedef void
  (APIENTRYP             COGL_PFNGLDELETERENDERBUFFERSEXTPROC)
  (GLsizei               n,
   const GLuint         *renderbuffers);

typedef void
  (APIENTRYP             COGL_PFNGLBLITFRAMEBUFFEREXTPROC)
  (GLint                 srcX0,
   GLint                 srcY0,
   GLint                 srcX1,
   GLint                 srcY1,
   GLint                 dstX0,
   GLint                 dstY0,
   GLint                 dstX1,
   GLint                 dstY1,
   GLbitfield            mask,
   GLenum                filter);

typedef void
  (APIENTRYP             COGL_PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)
  (GLenum                target,
   GLsizei               samples,
   GLenum                internalformat,
   GLsizei               width,
   GLsizei               height);

typedef GLhandleARB
  (APIENTRYP             COGL_PFNGLCREATEPROGRAMOBJECTARBPROC)
  (void);

typedef GLhandleARB
  (APIENTRYP             COGL_PFNGLCREATESHADEROBJECTARBPROC)
  (GLenum                shaderType);

typedef void
  (APIENTRYP             COGL_PFNGLSHADERSOURCEARBPROC)
  (GLhandleARB           shaderObj,
   GLsizei               count,
   const GLcharARB*     *string,
   const GLint          *length);

typedef void
  (APIENTRYP             COGL_PFNGLCOMPILESHADERARBPROC)
  (GLhandleARB           shaderObj);

typedef void
  (APIENTRYP             COGL_PFNGLATTACHOBJECTARBPROC)
  (GLhandleARB           containerObj,
   GLhandleARB           obj);

typedef void
  (APIENTRYP             COGL_PFNGLLINKPROGRAMARBPROC)
  (GLhandleARB           programObj);

typedef void
  (APIENTRYP             COGL_PFNGLUSEPROGRAMOBJECTARBPROC)
  (GLhandleARB           programObj);

typedef GLint
  (APIENTRYP             COGL_PFNGLGETUNIFORMLOCATIONARBPROC)
  (GLhandleARB           programObj,
   const GLcharARB      *name);

typedef void
  (APIENTRYP             COGL_PFNGLDELETEOBJECTARBPROC)
  (GLhandleARB           obj);

typedef void
  (APIENTRYP             COGL_PFNGLGETINFOLOGARBPROC)
  (GLhandleARB           obj,
   GLsizei               maxLength,
   GLsizei              *length,
   GLcharARB            *infoLog);

typedef void
  (APIENTRYP             COGL_PFNGLGETOBJECTPARAMETERIVARBPROC)
  (GLhandleARB           obj,
   GLenum                pname,
   GLint                *params);

typedef void
  (APIENTRYP             COGL_PFNGLUNIFORM1FARBPROC)
  (GLint                 location,
   GLfloat               v0);

G_END_DECLS

#endif
