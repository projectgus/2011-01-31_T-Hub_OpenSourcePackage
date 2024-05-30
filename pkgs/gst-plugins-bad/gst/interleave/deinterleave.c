/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Wim Taymans <wim@fluendo.com>
 *                    2007 Andy Wingo <wingo at pobox.com>
 *
 * deinterleave.c: deinterleave samples, based on interleave.c
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


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>


#define GST_TYPE_DEINTERLEAVE            (gst_deinterleave_get_type())
#define GST_DEINTERLEAVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DEINTERLEAVE,GstDeinterleave))
#define GST_DEINTERLEAVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DEINTERLEAVE,GstDeinterleaveClass))
#define GST_DEINTERLEAVE_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_DEINTERLEAVE,GstDeinterleaveClass))
#define GST_IS_DEINTERLEAVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DEINTERLEAVE))
#define GST_IS_DEINTERLEAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DEINTERLEAVE))

typedef struct _GstDeinterleave GstDeinterleave;
typedef struct _GstDeinterleaveClass GstDeinterleaveClass;


struct _GstDeinterleave
{
  GstElement element;

  /*< private > */
  GList *srcpads;
  GstCaps *sinkcaps;
  gint channels;

  GstPad *sink;
};

struct _GstDeinterleaveClass
{
  GstElementClass parent_class;
};


GST_DEBUG_CATEGORY_STATIC (gst_deinterleave_debug);
#define GST_CAT_DEFAULT gst_deinterleave_debug


static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src%d",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("audio/x-raw-float, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 1, "
        "endianness = (int) BYTE_ORDER, " "width = (int) 32")
    );
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-float, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, MAX ], "
        "endianness = (int) BYTE_ORDER, " "width = (int) 32")
    );


GST_BOILERPLATE (GstDeinterleave, gst_deinterleave, GstElement,
    GST_TYPE_ELEMENT);


static GstFlowReturn gst_deinterleave_chain (GstPad * pad, GstBuffer * buffer);
static gboolean gst_deinterleave_sink_setcaps (GstPad * pad, GstCaps * caps);
static gboolean gst_deinterleave_sink_activate_push (GstPad * pad,
    gboolean active);


static const GstElementDetails details =
GST_ELEMENT_DETAILS ("Audio deinterleaver",
    "Filter/Converter/Audio",
    "Splits one interleaved multichannel audio stream into many mono audio streams",
    "Andy Wingo <wingo at pobox.com>, " "Iain <iain@prettypeople.org>");

static void
gst_deinterleave_base_init (gpointer g_class)
{
  GST_DEBUG_CATEGORY_INIT (gst_deinterleave_debug, "interleave", 0,
      "interleave element");

  gst_element_class_set_details (g_class, &details);

  gst_element_class_add_pad_template (g_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (g_class,
      gst_static_pad_template_get (&src_template));
}

static void
gst_deinterleave_class_init (GstDeinterleaveClass * klass)
{
  /* pass */
}

static void
gst_deinterleave_init (GstDeinterleave * self, GstDeinterleaveClass * klass)
{
  self->sink = gst_pad_new_from_static_template (&sink_template, "sink");

  gst_pad_set_chain_function (self->sink,
      GST_DEBUG_FUNCPTR (gst_deinterleave_chain));
  gst_pad_set_setcaps_function (self->sink,
      GST_DEBUG_FUNCPTR (gst_deinterleave_sink_setcaps));
  gst_pad_set_activatepush_function (self->sink,
      GST_DEBUG_FUNCPTR (gst_deinterleave_sink_activate_push));

  gst_element_add_pad (GST_ELEMENT (self), self->sink);
}

static void
gst_deinterleave_add_new_pads (GstDeinterleave * self, GstCaps * caps)
{
  GstPad *pad;
  guint i;

  for (i = 0; i < self->channels; i++) {
    gchar *name = g_strdup_printf ("src%d", i);

    pad = gst_pad_new_from_static_template (&src_template, name);
    g_free (name);
    gst_pad_set_caps (pad, caps);
    gst_pad_use_fixed_caps (pad);
    gst_pad_set_active (pad, TRUE);
    gst_element_add_pad (GST_ELEMENT (self), pad);
    self->srcpads = g_list_prepend (self->srcpads, gst_object_ref (pad));
  }

  gst_element_no_more_pads (GST_ELEMENT (self));
  self->srcpads = g_list_reverse (self->srcpads);
}

static void
gst_deinterleave_remove_pads (GstDeinterleave * self)
{
  GList *l;

  GST_INFO_OBJECT (self, "removing pads");

  for (l = self->srcpads; l; l = l->next) {
    GstPad *pad = GST_PAD (l->data);

    gst_element_remove_pad (GST_ELEMENT_CAST (self), pad);
    gst_object_unref (pad);
  }
  g_list_free (self->srcpads);
  self->srcpads = NULL;

  gst_pad_set_caps (self->sink, NULL);
  gst_caps_replace (&self->sinkcaps, NULL);
}

static gboolean
gst_deinterleave_sink_setcaps (GstPad * pad, GstCaps * caps)
{
  GstDeinterleave *self;

  self = GST_DEINTERLEAVE (gst_pad_get_parent (pad));

  if (self->sinkcaps && !gst_caps_is_equal (caps, self->sinkcaps)) {
    goto cannot_change_caps_dog;
  } else {
    GST_DEBUG_OBJECT (self, "got caps: %" GST_PTR_FORMAT, caps);
    gst_caps_replace (&self->sinkcaps, caps);
  }

  {
    GstCaps *srccaps;
    GstStructure *s;

    srccaps = gst_caps_copy (caps);
    s = gst_caps_get_structure (srccaps, 0);
    if (!gst_structure_get_int (s, "channels", &self->channels))
      goto no_channels;
    gst_structure_set (s, "channels", G_TYPE_INT, 1, NULL);
    gst_structure_remove_field (s, "channel-positions");
    gst_deinterleave_add_new_pads (self, srccaps);
    gst_caps_unref (srccaps);
  }

  gst_object_unref (self);

  return TRUE;

cannot_change_caps_dog:
  {
    gst_object_unref (self);
    return FALSE;
  }
no_channels:
  {
    g_warning ("yarr, shiver me timbers");
    gst_object_unref (self);
    return FALSE;
  }
}

static GstFlowReturn
gst_deinterleave_process (GstDeinterleave * self, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;      /* initialized to silence a warning */
  GList *srcs;
  guint bufsize, i, j, channels, pads_pushed, nframes;
  GstBuffer **buffers_out;
  gfloat *in, *out;

  channels = self->channels;
  buffers_out = g_alloca (sizeof (GstBuffer *) * channels);
  nframes = GST_BUFFER_SIZE (buf) / channels / sizeof (gfloat);
  bufsize = nframes * sizeof (gfloat);
  pads_pushed = 0;

  for (i = 0; i < channels; i++)
    buffers_out[i] = NULL;

  for (srcs = self->srcpads, i = 0; srcs; srcs = srcs->next, i++) {
    GstPad *pad = (GstPad *) srcs->data;

    buffers_out[i] = NULL;
    ret = gst_pad_alloc_buffer (pad, -1, bufsize, GST_PAD_CAPS (pad),
        &buffers_out[i]);

    if (ret != GST_FLOW_OK && ret != GST_FLOW_NOT_LINKED)
      goto alloc_buffer_failed;
    if (buffers_out[i] && GST_BUFFER_SIZE (buffers_out[i]) != bufsize)
      goto alloc_buffer_bad_size;

    if (buffers_out[i])
      gst_buffer_copy_metadata (buffers_out[i], buf,
          GST_BUFFER_COPY_TIMESTAMPS);
  }

  /* do the thing */
  for (srcs = self->srcpads, i = 0; srcs; srcs = srcs->next, i++) {
    GstPad *pad = (GstPad *) srcs->data;

    in = (gfloat *) GST_BUFFER_DATA (buf);
    in += i;                    /* gfloat * arith */
    if (buffers_out[i]) {
      out = (gfloat *) GST_BUFFER_DATA (buffers_out[i]);
      for (j = 0; j < nframes; j++)
        out[j] = in[j * channels];

      ret = gst_pad_push (pad, buffers_out[i]);
      buffers_out[i] = NULL;
      if (ret == GST_FLOW_OK)
        pads_pushed++;
      else if (ret == GST_FLOW_NOT_LINKED)
        ret = GST_FLOW_OK;
      else
        goto push_failed;
    }
  }

  if (!pads_pushed)
    ret = GST_FLOW_NOT_LINKED;

  gst_buffer_unref (buf);
  return ret;

alloc_buffer_failed:
  {
    GST_WARNING ("gst_pad_alloc_buffer() returned %s", gst_flow_get_name (ret));
    goto clean_buffers;

  }
alloc_buffer_bad_size:
  {
    GST_WARNING ("called alloc_buffer(), but didn't get requested bytes");
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto clean_buffers;
  }
push_failed:
  {
    GST_DEBUG ("push() failed, flow = %s", gst_flow_get_name (ret));
    goto clean_buffers;
  }
clean_buffers:
  {
    for (i = 0; i < channels; i++) {
      if (buffers_out[i])
        gst_buffer_unref (buffers_out[i]);
    }
    gst_buffer_unref (buf);
    return ret;
  }
}

static GstFlowReturn
gst_deinterleave_chain (GstPad * pad, GstBuffer * buffer)
{
  GstDeinterleave *self;
  GstFlowReturn ret;

  self = GST_DEINTERLEAVE (GST_PAD_PARENT (pad));

  ret = gst_deinterleave_process (self, buffer);

  if (ret != GST_FLOW_OK)
    GST_DEBUG_OBJECT (self, "flow: %s", gst_flow_get_name (ret));

  return ret;
}

static gboolean
gst_deinterleave_sink_activate_push (GstPad * pad, gboolean active)
{
  GstDeinterleave *self;

  self = GST_DEINTERLEAVE (gst_pad_get_parent (pad));

  if (!active)
    gst_deinterleave_remove_pads (self);

  gst_object_unref (self);

  return TRUE;
}
