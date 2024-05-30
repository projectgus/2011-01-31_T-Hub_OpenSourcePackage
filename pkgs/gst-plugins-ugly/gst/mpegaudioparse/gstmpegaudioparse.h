/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __MP3PARSE_H__
#define __MP3PARSE_H__


#include <gst/gst.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

#define GST_TYPE_MP3PARSE \
  (gst_mp3parse_get_type())
#define GST_MP3PARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MP3PARSE,GstMPEGAudioParse))
#define GST_MP3PARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MP3PARSE,GstMPEGAudioParseClass))
#define GST_IS_MP3PARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MP3PARSE))
#define GST_IS_MP3PARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MP3PARSE))

typedef struct _GstMPEGAudioParse GstMPEGAudioParse;
typedef struct _GstMPEGAudioParseClass GstMPEGAudioParseClass;

struct _GstMPEGAudioParse {
  GstElement element;

  GstPad *sinkpad, *srcpad;

  GstClockTime next_ts;
  /* Offset as supplied by incoming buffers */
  gint64 cur_offset;

  /* Upcoming timestamp given on an incoming buffer and
   * the offset at which it becomes active */
  GstClockTime pending_ts;
  gint64 pending_offset;
  /* Offset since the last newseg */
  gint64 tracked_offset;

  GstAdapter *adapter;

  guint skip; /* number of frames to skip */
  guint bit_rate; /* in kbps */
  gint channels, rate, layer, version;
  gint spf; /* Samples per frame */

  gboolean resyncing; /* True when attempting to resync (stricter checks are
                         performed) */
  gboolean sent_codec_tag;

  /* VBR tracking */
  guint   avg_bitrate;
  guint64 bitrate_sum;
  guint   frame_count;
  guint   last_posted_bitrate;

  /* Xing info */
  guint32 xing_flags;
  guint32 xing_frames;
  GstClockTime xing_total_time;
  guint32 xing_bytes;
  guchar xing_seek_table[100];
  guint32 xing_vbr_scale;
  guint   xing_bitrate;
};

struct _GstMPEGAudioParseClass {
  GstElementClass parent_class;
};

GType gst_mp3parse_get_type(void);

#ifdef __GCC__
#define ATTR_UNUSED __attribute__ ((unused)
#else
#define ATTR_UNUSED 
#endif

G_END_DECLS

#endif /* __MP3PARSE_H__ */
