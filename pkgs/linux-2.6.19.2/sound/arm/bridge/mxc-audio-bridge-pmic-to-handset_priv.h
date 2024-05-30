/*
 *  DECT audio bridge driver private header file.
 *
 *  Copyright (C) 2006 - 2009 Sagemcom All rights reserved
 *
 *  File name: mxc-audio-bridge-pmic-to-handset.c
 *  Creation date: 17/04/2009
 *  Author: Farid Hammane, Sagemcom
 *
 *  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General
 *  Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *  Write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA to
 *  receive a copy of the GNU General Public License.
 *  This Copyright notice should not be removed
 */

#ifndef AUDIO_BRIDGE_PMIC_TO_HANDSET_PRIV
#define AUDIO_BRIDGE_PMIC_TO_HANDSET_PRIV


#ifdef CONFIG_ULOG_AUDIO_BRIDGE
#include <ulog/ulog.h>

static int mask_traces = 0;
#define SET_BIT(x)	(mask_traces |= (1 << x))
#define UNSET_BIT(x)	(mask_traces &= ~(1 << x))
#define IS_SET(x)	((mask_traces & (1 << x))?1:0)

enum {
	BIT_4ULOG_TRACE,
	BIT_4ULOG_TRACE_VALUE,
	BIT_4ULOG_TRACE_STREAM,
	BIT_4ULOG_TRACE_ERROR,
	BIT_4ULOG_DELAY_UPDATE,
	BIT_4ULOG_CAPTURE_OFFSET_UPDATE,
	BIT_4ULOG_PLAYBACK_OFFSET_UPDATE,
	BIT_4ULOG_DELAY_COMPENSATION,
	BIT_4ULOG_CAPTURE_JITTER,
	BIT_4ULOG_PLAYBACK_JITTER,
	BIT_4ULOG_MAX,
};

#define ulog_trace(event_id)					\
	do{							\
		if(IS_SET(BIT_4ULOG_TRACE))			\
			ulog(event_id,0);			\
	}while(0)


/* ---------------------------- */
#define ulog_capture_jitter(stream_id, val)\
	do{\
		if(IS_SET(BIT_4ULOG_CAPTURE_JITTER)){\
			if(stream_id == 0){\
				ulog(ULOG_AUDIO_BRIDGE_CAPTURE_JITTER_STREAM_0, val);\
			}\
			else{\
				ulog(ULOG_AUDIO_BRIDGE_CAPTURE_JITTER_STREAM_1, val);\
			}\
		}\
	}while(0)

#define ulog_capture_jitter_done(stream_id, val)\
	do{\
		if(IS_SET(BIT_4ULOG_CAPTURE_JITTER)){\
			if(stream_id == 0){\
				ulog(ULOG_AUDIO_BRIDGE_CAPTURE_JITTER_STREAM_0_DONE, val);\
			}\
			else{\
				ulog(ULOG_AUDIO_BRIDGE_CAPTURE_JITTER_STREAM_1_DONE, val);\
			}\
		}\
	}while(0)
/* ---------------------------- */

/* ---------------------------- */
#define ulog_playback_jitter(stream_id, val)\
	do{\
		if(IS_SET(BIT_4ULOG_PLAYBACK_JITTER)){\
			if(stream_id == 0){\
				ulog(ULOG_AUDIO_BRIDGE_PLAYBACK_JITTER_STREAM_0, val);\
			}\
			else{\
				ulog(ULOG_AUDIO_BRIDGE_PLAYBACK_JITTER_STREAM_1, val);\
			}\
		}\
	}while(0)

#define ulog_playback_jitter_done(stream_id, val)\
	do{\
		if(IS_SET(BIT_4ULOG_PLAYBACK_JITTER)){\
			if(stream_id == 0){\
				ulog(ULOG_AUDIO_BRIDGE_PLAYBACK_JITTER_STREAM_0_DONE, val);\
			}\
			else{\
				ulog(ULOG_AUDIO_BRIDGE_PLAYBACK_JITTER_STREAM_1_DONE, val);\
			}\
		}\
	}while(0)


/* ---------------------------- */




#define ulog_trace_value(event_id,value)			\
	do{							\
		if(IS_SET(BIT_4ULOG_TRACE_VALUE))		\
			ulog(event_id,value);			\
	}while(0)

#define ulog_trace_stream(event_id,stream)			\
	do{							\
		if(IS_SET(BIT_4ULOG_TRACE_STREAM)) 		\
			ulog(event_id,(stream&0xFF)<< 24 );	\
	}while(0)

#define ulog_trace_error(error_id,value)					\
	do{									\
	 	if(IS_SET(BIT_4ULOG_TRACE_ERROR))				\
		  	ulog(ULOG_AUDIO_BRIDGE_TRACE_ERRORS,			\
				(error_id&0xFF)<< 24 | (value&0xFFFFFF));	\
	}while(0)

#define ulog_delay_update(stream,actual_delay)				\
	do{								\
		if(IS_SET(BIT_4ULOG_DELAY_UPDATE))			\
			ulog(ULOG_AUDIO_BRIDGE_ACTUAL_DELAY_UPDATE,	\
		     	(stream&0xFF)<< 24 | (actual_delay&0xFFFFFF));	\
	}while(0)


#define ulog_capture_offset_update(stream,capture_offset)			\
	do{									\
		if(IS_SET(BIT_4ULOG_CAPTURE_OFFSET_UPDATE))			\
			ulog(ULOG_AUDIO_BRIDGE_CAPTURE_OFFSET_UPDATE,		\
			     (stream&0xFF)<< 24 | (capture_offset&0xFFFFFF));	\
	}while(0)


#define ulog_playback_offset_update(stream,playback_offset)			\
	do{									\
		if(IS_SET(BIT_4ULOG_PLAYBACK_OFFSET_UPDATE))			\
			ulog(ULOG_AUDIO_BRIDGE_PLAYBACK_OFFSET_UPDATE,		\
			     (stream&0xFF)<<24 | (playback_offset&0xFFFFFF));	\
	}while(0)


#define ulog_delay_compensation(stream,delay_offset)				\
	do{									\
		if(IS_SET(BIT_4ULOG_DELAY_COMPENSATION))			\
			ulog(ULOG_AUDIO_BRIDGE_DELAY_COMPENSATION,		\
			     (stream&0xFF)<<24 | (delay_offset&0xFFFFFF));	\
	}while(0)


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
/* ------------ */
static ssize_t ulog_trace_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_TRACE (%d)\n", IS_SET(BIT_4ULOG_TRACE));
	return size;
}
static ssize_t ulog_trace_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_TRACE);
	else
		UNSET_BIT(BIT_4ULOG_TRACE);
	return size;
}

/* ------------ */
static ssize_t ulog_capture_jitter_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_CAPTURE_JITTER (%d)\n", IS_SET(BIT_4ULOG_CAPTURE_JITTER));
	return size;
}
static ssize_t ulog_capture_jitter_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_CAPTURE_JITTER);
	else
		UNSET_BIT(BIT_4ULOG_CAPTURE_JITTER);
	return size;
}

/* ------------ */

static ssize_t ulog_playback_jitter_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_PLAYBACK_JITTER (%d)\n", IS_SET(BIT_4ULOG_PLAYBACK_JITTER));
	return size;
}
static ssize_t ulog_playback_jitter_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_PLAYBACK_JITTER);
	else
		UNSET_BIT(BIT_4ULOG_PLAYBACK_JITTER);
	return size;
}

/* ------------ */


static ssize_t ulog_trace_value_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_TRACE_VALUE (%d)\n", IS_SET(BIT_4ULOG_TRACE_VALUE));
	return size;

}
static ssize_t ulog_trace_value_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_TRACE_VALUE);
	else
		UNSET_BIT(BIT_4ULOG_TRACE_VALUE);
	return size;
}

/* ------------ */

static ssize_t ulog_trace_stream_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_TRACE_STREAM (%d)\n", IS_SET(BIT_4ULOG_TRACE_STREAM));
	return size;
}
static ssize_t ulog_trace_stream_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_TRACE_STREAM);
	else
		UNSET_BIT(BIT_4ULOG_TRACE_STREAM);
	return size;
}

/* ------------ */

static ssize_t ulog_trace_error_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_TRACE_ERROR (%d)\n", IS_SET(BIT_4ULOG_TRACE_ERROR));
	return size;
}
static ssize_t ulog_trace_error_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_TRACE_ERROR);
	else
		UNSET_BIT(BIT_4ULOG_TRACE_ERROR);
	return size;
}
/* ------------ */




static ssize_t ulog_delay_update_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_DELAY_UPDATE (%d)\n", IS_SET(BIT_4ULOG_DELAY_UPDATE));
	return size;
}
static ssize_t ulog_delay_update_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_DELAY_UPDATE);
	else
		UNSET_BIT(BIT_4ULOG_DELAY_UPDATE);
	return size;
}

/* ------------ */

static ssize_t ulog_capture_offset_update_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_CAPTURE_OFFSET_UPDATE (%d)\n", IS_SET(BIT_4ULOG_CAPTURE_OFFSET_UPDATE));
	return size;
}
static ssize_t ulog_capture_offset_update_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_CAPTURE_OFFSET_UPDATE);
	else
		UNSET_BIT(BIT_4ULOG_CAPTURE_OFFSET_UPDATE);
	return size;
}

/* ------------ */

static ssize_t ulog_playback_offset_update_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_PLAYBACK_OFFSET_UPDATE (%d)\n", IS_SET(BIT_4ULOG_PLAYBACK_OFFSET_UPDATE));
	return size;
}
static ssize_t ulog_playback_offset_update_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_PLAYBACK_OFFSET_UPDATE);
	else
		UNSET_BIT(BIT_4ULOG_PLAYBACK_OFFSET_UPDATE);
	return size;
}

/* ------------ */
static ssize_t ulog_delay_compensation_read(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_DELAY_COMPENSATION (%d)\n", IS_SET(BIT_4ULOG_DELAY_COMPENSATION));
	return size;
}
static ssize_t ulog_delay_compensation_write(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	if(vl)
		SET_BIT(BIT_4ULOG_DELAY_COMPENSATION);
	else
		UNSET_BIT(BIT_4ULOG_DELAY_COMPENSATION);
	return size;
}
/* ------------ */

static ssize_t ulog_check_all(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "BIT_4ULOG_TRACE                    (%d)\n", IS_SET(BIT_4ULOG_TRACE));
	size += sprintf((buf + size), "BIT_4ULOG_TRACE_VALUE              (%d)\n", IS_SET(BIT_4ULOG_TRACE_VALUE));
	size += sprintf((buf + size), "BIT_4ULOG_TRACE_STREAM             (%d)\n", IS_SET(BIT_4ULOG_TRACE_STREAM));
	size += sprintf((buf + size), "BIT_4ULOG_TRACE_ERROR              (%d)\n", IS_SET(BIT_4ULOG_TRACE_ERROR));
	size += sprintf((buf + size), "BIT_4ULOG_DELAY_UPDATE             (%d)\n", IS_SET(BIT_4ULOG_DELAY_UPDATE));
	size += sprintf((buf + size), "BIT_4ULOG_CAPTURE_OFFSET_UPDATE    (%d)\n", IS_SET(BIT_4ULOG_CAPTURE_OFFSET_UPDATE));
	size += sprintf((buf + size), "BIT_4ULOG_PLAYBACK_OFFSET_UPDATE   (%d)\n", IS_SET(BIT_4ULOG_PLAYBACK_OFFSET_UPDATE));
	size += sprintf((buf + size), "BIT_4ULOG_DELAY_COMPENSATION       (%d)\n", IS_SET(BIT_4ULOG_DELAY_COMPENSATION));
	size += sprintf((buf + size), "BIT_4ULOG_CAPTURE_JITTER           (%d)\n", IS_SET(BIT_4ULOG_CAPTURE_JITTER));
	size += sprintf((buf + size), "BIT_4ULOG_PLAYBACK_JITTER          (%d)\n", IS_SET(BIT_4ULOG_PLAYBACK_JITTER));
	return size;
}

#else
/* TODO */
#endif


static SYSDEV_ATTR(ulog_trace, 0666, ulog_trace_read, ulog_trace_write);
static SYSDEV_ATTR(ulog_trace_value, 0666, ulog_trace_value_read, ulog_trace_value_write);
static SYSDEV_ATTR(ulog_trace_stream, 0666, ulog_trace_stream_read, ulog_trace_stream_write);
static SYSDEV_ATTR(ulog_trace_error, 0666, ulog_trace_error_read, ulog_trace_error_write);
static SYSDEV_ATTR(ulog_delay_update, 0666, ulog_delay_update_read, ulog_delay_update_write);
static SYSDEV_ATTR(ulog_capture_offset_update, 0666, ulog_capture_offset_update_read, ulog_capture_offset_update_write);
static SYSDEV_ATTR(ulog_playback_offset_update, 0666, ulog_playback_offset_update_read, ulog_playback_offset_update_write);
static SYSDEV_ATTR(ulog_delay_compensation, 0666, ulog_delay_compensation_read, ulog_delay_compensation_write);
static SYSDEV_ATTR(ulog_capture_jitter, 0666, ulog_capture_jitter_read, ulog_capture_jitter_write);
static SYSDEV_ATTR(ulog_playback_jitter, 0666, ulog_playback_jitter_read, ulog_playback_jitter_write);
static SYSDEV_ATTR(ulog_check_all, 0444, ulog_check_all, NULL);

#else

#define ulog_trace(event_id)					do{}while(0)
#define ulog_trace_value(event_id,value)			do{}while(0)
#define ulog_trace_stream(event_id, stream)			do{}while(0)
#define ulog_trace_error(error_id,value)			do{}while(0)
#define ulog_delay_update(stream,actual_delay)			do{}while(0)
#define ulog_capture_offset_update(stream,capture_offset)	do{}while(0)
#define ulog_playback_offset_update(stream,playback_offset)	do{}while(0)
#define ulog_delay_compensation(stream,delay_offset)		do{}while(0)
#define ulog_playback_jitter(stream_id, val)			do{}while(0)
#define ulog_playback_jitter_done(stream_id, val)		do{}while(0)
#define ulog_capture_jitter(stream_id, val)			do{}while(0)
#define ulog_capture_jitter_done(stream_id, val)		do{}while(0)


#define SET_BIN(x)
#define UNSET_BIN(x)
#define IS_SET(x)


#endif /*CONFIG_ULOG_AUDIO_BRIDGE*/





#endif
