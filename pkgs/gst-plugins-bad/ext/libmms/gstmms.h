/* 
 * gstmms.h: header file for gst-mms plugin
 *
 * Modified by Sagemcom under LGPL license on 03/09/2009 
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
 */

#ifndef __GST_MMS_H__
#define __GST_MMS_H__

#include <gst/gst.h>
#include <libmms/mms.h>
#include <libmms/mmsh.h>
#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

/* #define's don't like whitespacey bits */
#define GST_TYPE_MMS \
  (gst_mms_get_type())
#define GST_MMS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MMS,GstMMS))
#define GST_MMS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MMS,GstMMSClass))
#define GST_IS_MMS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MMS))
#define GST_IS_MMS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MMS))

typedef struct _GstMMS      GstMMS;
typedef struct _GstMMSClass GstMMSClass;

struct _GstMMS
{
  GstPushSrc parent;

  gchar  *uri_name;
  guint  connection_speed;
  guint  max_audio_rate;
  guint  max_video_rate;
  
  mms_t  *connection;
  mmsh_t *connection_h;
};

struct _GstMMSClass 
{
  GstPushSrcClass parent_class;
};

GType gst_mms_get_type (void);

G_END_DECLS

#endif /* __GST_MMS_H__ */
