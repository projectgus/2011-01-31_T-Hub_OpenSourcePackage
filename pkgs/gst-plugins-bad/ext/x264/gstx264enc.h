/* GStreamer H264 encoder plugin
 * Copyright (C) 2005 Michal Benes <michal.benes@itonis.tv>
 * Copyright (C) 2005 Josef Zlomek <josef.zlomek@itonis.tv>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_X264_ENC_H__
#define __GST_X264_ENC_H__

#include <gst/gst.h>
#include <_stdint.h>
#include <x264.h>

G_BEGIN_DECLS

#define GST_TYPE_X264_ENC \
  (gst_x264_enc_get_type())
#define GST_X264_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_X264_ENC,GstX264Enc))
#define GST_X264_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_X264_ENC,GstX264EncClass))
#define GST_IS_X264_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_X264_ENC))
#define GST_IS_X264_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_X264_ENC))

typedef struct _GstX264Enc GstX264Enc;
typedef struct _GstX264EncClass GstX264EncClass;
 
struct _GstX264Enc
{
  GstElement element;

  /*< private >*/
  GstPad *sinkpad;
  GstPad *srcpad;

  x264_t *x264enc;
  x264_param_t x264param;

  guint threads;
  guint pass;
  gchar *stats_file;
  gboolean byte_stream;
  guint bitrate;
  gint me;
  guint subme;
  guint analyse;
  gboolean dct8x8;
  guint ref;
  guint bframes;
  gboolean b_pyramid;
  gboolean weightb;
  guint sps_id;
  gboolean trellis;
  guint vbv_buf_capacity;
  guint keyint_max;
  gboolean cabac;

  gint width, height;
  guint stride, luma_plane_size;
  gint framerate_num, framerate_den;
  gint par_num, par_den;

  GstClockTime last_timestamp;
  GstClockTime *timestamp_queue;
  GstClockTime *timestamp_queue_dur;
  guint timestamp_queue_size;
  guint timestamp_queue_head;
  guint timestamp_queue_tail;

  guint8 *buffer;
  gulong buffer_size;
};

struct _GstX264EncClass
{
  GstElementClass parent_class;
};

G_END_DECLS

#endif /* __GST_X264_ENC_H__ */
