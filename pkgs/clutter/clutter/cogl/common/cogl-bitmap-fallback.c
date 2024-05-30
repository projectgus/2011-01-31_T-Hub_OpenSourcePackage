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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cogl.h"
#include "cogl-internal.h"
#include "cogl-bitmap.h"

#include <string.h>

/* TO rgba */

inline static void
_cogl_g_to_rgba (const guchar *src, guchar *dst)
{
  dst[0] = src[0];
  dst[1] = src[0];
  dst[2] = src[0];
  dst[3] = 255;
}

inline static void
_cogl_rgb_to_rgba (const guchar *src, guchar *dst)
{
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = 255;
}

inline static void
_cogl_bgr_to_rgba (const guchar *src, guchar *dst)
{
  dst[0] = src[2];
  dst[1] = src[1];
  dst[2] = src[0];
  dst[3] = 255;
}

inline static void
_cogl_bgra_to_rgba (const guchar *src, guchar *dst)
{
  dst[0] = src[2];
  dst[1] = src[1];
  dst[2] = src[0];
  dst[3] = src[3];
}

inline static void
_cogl_argb_to_rgba (const guchar *src, guchar *dst)
{
  dst[0] = src[1];
  dst[1] = src[2];
  dst[2] = src[3];
  dst[3] = src[0];
}

inline static void
_cogl_abgr_to_rgba (const guchar *src, guchar *dst)
{
  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
}

inline static void
_cogl_rgba_to_rgba (const guchar *src, guchar *dst)
{
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = src[3];
}

/* FROM rgba */

inline static void
_cogl_rgba_to_g (const guchar *src, guchar *dst)
{
  dst[0] = (src[0] + src[1] + src[2]) / 3;
}

inline static void
_cogl_rgba_to_rgb (const guchar *src, guchar *dst)
{
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
}

inline static void
_cogl_rgba_to_bgr (const guchar *src, guchar *dst)
{
  dst[0] = src[2];
  dst[1] = src[1];
  dst[2] = src[0];
}

inline static void
_cogl_rgba_to_bgra (const guchar *src, guchar *dst)
{
  dst[0] = src[2];
  dst[1] = src[1];
  dst[2] = src[0];
  dst[3] = src[3];
}

inline static void
_cogl_rgba_to_argb (const guchar *src, guchar *dst)
{
  dst[0] = src[3];
  dst[1] = src[0];
  dst[2] = src[1];
  dst[3] = src[2];
}

inline static void
_cogl_rgba_to_abgr (const guchar *src, guchar *dst)
{
  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
}

/* (Un)Premultiplication */

inline static void
_cogl_unpremult_alpha_0 (const guchar *src, guchar *dst)
{
  dst[0] = 0;
  dst[1] = 0;
  dst[2] = 0;
  dst[3] = 0;
}

inline static void
_cogl_unpremult_alpha_last (const guchar *src, guchar *dst)
{
  guchar alpha = src[3];
  
  dst[0] = ((((gulong) src[0] >> 16) & 0xff) * 255 ) / alpha;
  dst[1] = ((((gulong) src[1] >> 8) & 0xff) * 255 ) / alpha;
  dst[2] = ((((gulong) src[2] >> 0) & 0xff) * 255 ) / alpha;
  dst[3] = alpha;
}

inline static void
_cogl_unpremult_alpha_first (const guchar *src, guchar *dst)
{
  guchar alpha = src[0];
  
  dst[0] = alpha;
  dst[1] = ((((gulong) src[1] >> 16) & 0xff) * 255 ) / alpha;
  dst[2] = ((((gulong) src[2] >> 8) & 0xff) * 255 ) / alpha;
  dst[3] = ((((gulong) src[3] >> 0) & 0xff) * 255 ) / alpha;
}

gboolean
_cogl_bitmap_fallback_can_convert (CoglPixelFormat src, CoglPixelFormat dst)
{
  if (src == dst)
    return FALSE;
  
  switch (src & COGL_UNORDERED_MASK)
    {
    case COGL_PIXEL_FORMAT_G_8:
    case COGL_PIXEL_FORMAT_24:
    case COGL_PIXEL_FORMAT_32:
      
      if ((dst & COGL_UNORDERED_MASK) != COGL_PIXEL_FORMAT_24 &&
	  (dst & COGL_UNORDERED_MASK) != COGL_PIXEL_FORMAT_32 &&
	  (dst & COGL_UNORDERED_MASK) != COGL_PIXEL_FORMAT_G_8)
	return FALSE;
      break;
      
    default:
      return FALSE;
    }
  
  return TRUE;
}

gboolean
_cogl_bitmap_fallback_can_unpremult (CoglPixelFormat format)
{
  return ((format & COGL_UNORDERED_MASK) == COGL_PIXEL_FORMAT_32);
}

gboolean
_cogl_bitmap_fallback_convert (const CoglBitmap *bmp,
			       CoglBitmap       *dst_bmp,
			       CoglPixelFormat   dst_format)
{
  guchar  *src;
  guchar  *dst;
  gint     src_bpp;
  gint     dst_bpp;
  gint     x,y;
  guchar   temp_rgba[4] = {0,0,0,0};
  
  /* Make sure conversion supported */
  if (!_cogl_bitmap_fallback_can_convert (bmp->format, dst_format))
    return FALSE;
  
  src_bpp = _cogl_get_format_bpp (bmp->format);
  dst_bpp = _cogl_get_format_bpp (dst_format);

  /* Initialize destination bitmap */
  *dst_bmp = *bmp;
  dst_bmp->rowstride = sizeof(guchar) * dst_bpp * dst_bmp->width;
  dst_bmp->format = ((bmp->format & COGL_PREMULT_BIT) |
		     (dst_format & COGL_UNPREMULT_MASK));
  
  /* Allocate a new buffer to hold converted data */
  dst_bmp->data = g_malloc (sizeof(guchar)
			    * dst_bmp->height
			    * dst_bmp->rowstride);
  
  /* FIXME: Optimize */
  for (y = 0; y < bmp->height; y++)
    {
      src = (guchar*)bmp->data      + y * bmp->rowstride;
      dst = (guchar*)dst_bmp->data  + y * dst_bmp->rowstride;
      
      for (x = 0; x < bmp->width; x++)
	{
	  /* FIXME: Would be nice to at least remove this inner
           * branching, but not sure it can be done without
           * rewriting of the whole loop */
	  switch (bmp->format & COGL_UNPREMULT_MASK)
	    {
	    case COGL_PIXEL_FORMAT_G_8:
	      _cogl_g_to_rgba (src, temp_rgba); break;
	    case COGL_PIXEL_FORMAT_RGB_888:
	      _cogl_rgb_to_rgba (src, temp_rgba); break;
	    case COGL_PIXEL_FORMAT_BGR_888:
	      _cogl_bgr_to_rgba (src, temp_rgba); break;
	    case COGL_PIXEL_FORMAT_RGBA_8888:
	      _cogl_rgba_to_rgba (src, temp_rgba); break;
	    case COGL_PIXEL_FORMAT_BGRA_8888:
	      _cogl_bgra_to_rgba (src, temp_rgba); break;
	    case COGL_PIXEL_FORMAT_ARGB_8888:
	      _cogl_argb_to_rgba (src, temp_rgba); break;
	    case COGL_PIXEL_FORMAT_ABGR_8888:
	      _cogl_abgr_to_rgba (src, temp_rgba); break;
	    default:
	      break;
	    }
	  
	  switch (dst_format & COGL_UNPREMULT_MASK)
	    {
	    case COGL_PIXEL_FORMAT_G_8:
	      _cogl_rgba_to_g (temp_rgba, dst); break;
	    case COGL_PIXEL_FORMAT_RGB_888:
	      _cogl_rgba_to_rgb (temp_rgba, dst); break;
	    case COGL_PIXEL_FORMAT_BGR_888:
	      _cogl_rgba_to_bgr (temp_rgba, dst); break;
	    case COGL_PIXEL_FORMAT_RGBA_8888:
	      _cogl_rgba_to_rgba (temp_rgba, dst); break;
	    case COGL_PIXEL_FORMAT_BGRA_8888:
	      _cogl_rgba_to_bgra (temp_rgba, dst); break;
	    case COGL_PIXEL_FORMAT_ARGB_8888:
	      _cogl_rgba_to_argb (temp_rgba, dst); break;
	    case COGL_PIXEL_FORMAT_ABGR_8888:
	      _cogl_rgba_to_abgr (temp_rgba, dst); break;
	    default:
	      break;
	    }
	  
	  src += src_bpp;
	  dst += dst_bpp;
	}
    }
  
  return TRUE;
}

gboolean
_cogl_bitmap_fallback_unpremult (const CoglBitmap *bmp,
				 CoglBitmap       *dst_bmp)
{
  guchar  *src;
  guchar  *dst;
  gint     bpp;
  gint     x,y;
  
  /* Make sure format supported for un-premultiplication */
  if (!_cogl_bitmap_fallback_can_unpremult (bmp->format))
    return FALSE;
  
  bpp = _cogl_get_format_bpp (bmp->format);
  
  /* Initialize destination bitmap */
  *dst_bmp = *bmp;
  dst_bmp->format = (bmp->format & COGL_UNPREMULT_MASK);
  
  /* Allocate a new buffer to hold converted data */
  dst_bmp->data = g_malloc (sizeof(guchar)
			    * dst_bmp->height
			    * dst_bmp->rowstride);
  
  /* FIXME: Optimize */
  for (y = 0; y < bmp->height; y++)
    {
      src = (guchar*)bmp->data      + y * bmp->rowstride;
      dst = (guchar*)dst_bmp->data  + y * dst_bmp->rowstride;
      
      for (x = 0; x < bmp->width; x++)
	{
	  /* FIXME: Would be nice to at least remove this inner
           * branching, but not sure it can be done without
           * rewriting of the whole loop */
	  if (bmp->format & COGL_AFIRST_BIT)
	    {
	      if (src[0] == 0)
		_cogl_unpremult_alpha_0 (src, dst);
	      else
		_cogl_unpremult_alpha_first (src, dst);
	    }
	  else
	    {
	      if (src[3] == 0)
		_cogl_unpremult_alpha_0 (src, dst);
	      else
		_cogl_unpremult_alpha_last (src, dst);
	    }
	  
	  src += bpp;
	  dst += bpp;
	}
    }
  
  return TRUE;
}

gboolean
_cogl_bitmap_fallback_from_file (CoglBitmap  *bmp,
				 const gchar *filename)
{
  /* FIXME: use jpeglib, libpng, etc. manually maybe */
  return FALSE;
}
