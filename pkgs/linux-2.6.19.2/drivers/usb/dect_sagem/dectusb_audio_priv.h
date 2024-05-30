/*
 *  DECT USB driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dectusb_audio_priv.h
 *  Creation date: 10/07/2008
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

#ifndef DECT_AUDIO_H
#define DECT_AUDIO_H

#include <linux/timer.h>
#include <dectusb_commun.h>
#if defined(CONFIG_SND_AUDIO_BRIDGE)
#include <mxc-audio-bridge-pmic-to-handset.h>
#endif

#define DECTUSB_AUDIO_DRIVER_VERSION	"v2.0.1"

/* Verbose level */
static int vl;
module_param(vl, int, 0);
MODULE_PARM_DESC(vl, "Verbose level");

MODULE_AUTHOR("Farid Hammane <fhammane@gmail.com>");
MODULE_DESCRIPTION("Driver supporting audio interface of dongle DECT usb")
MODULE_LICENSE("GPL");

#define DECT_AUDIO_NAME			"[DECT USB AUDIO]"
#define INFO(fmt, args...)		if(vl > 2) printk(DECT_AUDIO_NAME " INFO  %d %s()  "fmt"\n", __LINE__, __func__, ## args);
#define ERROR(fmt, args...)		printk(DECT_AUDIO_NAME " ERROR  %d %s()  "fmt"\n", __LINE__, __func__, ## args);
#define WARNING(fmt, args...)		printk(DECT_AUDIO_NAME " WARNING  %d %s()  "fmt"\n", __LINE__, __func__, ## args);

struct dectusb_ctx {
	struct dectusb_audio_t *ctx;
};
static struct dectusb_ctx Dectusb = {
	.ctx = NULL,
};
#define bad_ctx(dect)	(!Dectusb.ctx || dect != Dectusb.ctx)
#define warning_bad_ctx(dect) WARNING("Bad context : Dectusb.ctx (%p) dectusb (%p)", Dectusb.ctx, dect)

#define DECT_USB_INTERF_AUDIO_IN	0
#define DECT_USB_INTERF_AUDIO_OUT	1

#define SND_REPORID			0x3f

enum { 
	DECT_AUDIO_FORMAT_ALAW = 0,
	DECT_AUDIO_FORMAT_PCM,
	DECT_AUDIO_MAX_FORMAT,
};

#define DECT_USB_MINOR_BASE_AUDIO	180

typedef struct{
	void					*UserCtx;
	elapsed_t				UserCallback;
	int					transfer_status; 	/*! the current state of transfer : 1 -> active, 0 not active */
	int					state;		 	/*! capture device can be open or close */
	
	unsigned char				*buffer;		/*! Used to send data, and to keep data that could not been send */
	int					size_still_in_buff;	/*! Number of bytes kept in *buffer */

	unsigned char				*alsa_buffer; 		/*! A pointer to the destination buffer */
	int					alsa_asked_size;	/*! Number of bytes to send */
	int					alsa_got_size;		/*! Number of bytes already sent */

}dectusb_audio_r_t;

typedef struct{

	struct usb_endpoint_descriptor 		*endpoint;		/*! A pointer to (dectusb_audio_w_t*)->endpoint */
	struct urb				*interrupt_urb;		/*! A pointer to (dectusb_audio_w_t*)->interrupt_urb */
	
	unsigned char				*buff;			/*! The buffer containing outgoing data and given to URB */
	int					size;			/*! Number of bytes to send in *buff */ 
	int					error;			/*! Used to save URB error */
	int					run;			/*! Driver will ask for urb while run == 1 */

}dectusb_audio_sync_t;

#define NB_CIRCULAR_DATA_INFO		4

struct app_circular_data_info_t {

	int					start_indx;
	int					end_indx;
	int					total_app_size_available;

	int                                     app_size_left_in_space[NB_CIRCULAR_DATA_INFO];
	unsigned char                           *app_ptr[NB_CIRCULAR_DATA_INFO];
#ifdef STRAT_END_DEBUG
	int					start_nb;
	int					end_nb;
#endif

};

typedef struct{
	
	struct usb_endpoint_descriptor 		*endpoint;		/*! A pointer to INTR out endpoint struct */
	struct urb				*interrupt_urb;		/*! A pointer to the URB used for outgoing data */
	void					*UserCtx;		/*! User's context */
	elapsed_t				UserCallback;
	int					transfer_status;	/*! the current state of transfer : 1 -> active, 0 not active */	
	int					state;		 	/*! capture device can be open or close */

	unsigned char				*out_buffer;		/*! Outgoing data given for the URB */
	int					out_size;		/*! Size of outgoing data */
	int					app_period_size;	/*! The periode size, or size buffer, given by ALSA */
	
	struct app_circular_data_info_t		ci;			

}dectusb_audio_w_t;


struct dectusb_audio_t {
	struct usb_device			*usbdev;		/*! A pointer to the struct usb_device */

	int					dongle_msg_size_max; 	/*! The max size of a dongle dect frame */

	dectusb_audio_r_t			r;			/*! A poiter to dectusb_audio_r_t */
	dectusb_audio_w_t			w;			/*! A poiter to dectusb_audio_w_t */
	dectusb_audio_sync_t			sync;			/*! A poiter to dectusb_audio_sync_t */

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	void 					*bridge_handle;
	struct audio_bridge_operations 		*dectusb_ops;
#endif

};



static struct usb_driver driver;

static int playback_request_urb(struct dectusb_audio_t *p_dect);

static void clear_circular_buff(struct app_circular_data_info_t *cb);
static inline void clear_w_struct(dectusb_audio_w_t *w);
static inline void clear_r_struct(dectusb_audio_r_t *r);
static inline void clear_sync_struct(dectusb_audio_sync_t *sync);

static int set_struct_r(struct usb_endpoint_descriptor *ep, struct dectusb_audio_t *pdect);
static int set_struct_w(struct usb_endpoint_descriptor *ep, struct dectusb_audio_t *pdect);
static int set_struct(struct dectusb_audio_t **ptr, struct usb_interface *intf);

static void free_sync_struct(struct dectusb_audio_t *pdect);
static void free_r_struct(struct dectusb_audio_t *pdect);
static void free_w_struct(struct dectusb_audio_t *p_dect);

static void exec_user_r_callback(struct dectusb_audio_t * ldect, int status);
static void exec_user_write_callback(struct dectusb_audio_t * ldect, int status);

static void capture_handle_data(struct urb *urb, struct dectusb_audio_t *pdect);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
static void playback_intr(struct urb *urb, struct pt_regs *regs);
#else
static void playback_intr(struct urb *urb);
#endif
int synchro_request_urb(struct dectusb_audio_t * p_dect);

/* Exported variables (/proc) used for debug */
static unsigned int db_sync_packet;
static unsigned int db_snd_packet;

/* 
 * Used to convert data from alaw to pcm for alsa
 * 0 for alaw, 1 for pcm
 */
int encoding_format = DECT_AUDIO_FORMAT_PCM;

#define DECTUSB_AUDIO_NODE_NAME		"dectusb_aud"

#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_DECT)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

static spinlock_t sl_irq;
#if defined (DO_ULOG) && defined(CONFIG_ULOG_DECTUSB_AUDIO_SPINLOCK)
#warning "Audio interface : compiling ulog traces for spin locks"
#define ulog_spin(x,y)	ulog(x,y)
#else
#define ulog_spin(x,y)
#endif

#if defined(CONFIG_ULOG_DECTUSB_AUDIO_PLAYBACK_DELAY)
static int playback_packet_number = 0;
static int playback_packet_number_elapsed = 0;
#define ulog_playback_delay() 		ulog(ULOG_DECTUSB_AUDIO_PLAYBACK_DELAY, 	playback_packet_number++);
#define ulog_playback_delay_elapsed()	ulog(ULOG_DECTUSB_AUDIO_PLAYBACK_DELAY_DONE,	playback_packet_number_elapsed++);
#else
#define ulog_playback_delay()
#define ulog_playback_delay_elapsed()
#endif

#define lock_irq(id)										\
		ulog_spin(ULOG_DECTUSB_AUDIO_SPIN_LOCK,__LINE__<<16 | id);			\
		spin_lock_irq(&sl_irq);

#define unlock_irq(id)										\
		ulog_spin(ULOG_DECTUSB_AUDIO_SPIN_LOCK_DONE,__LINE__<<16 | id);			\
		spin_unlock_irq(&sl_irq);


static struct mutex dectmutex;

#if defined (DO_ULOG) && defined(CONFIG_ULOG_DECTUSB_FUNC_CTX)
#define ulog_ctx_type(x) \
	if(in_irq()){\
		ulog(ULOG_DECTUSB_HARDIRQ_CTX, x);\
	}else if(in_softirq()){\
		ulog(ULOG_DECTUSB_SOFTIRQ_CTX, x);\
	}else{\
		ulog(ULOG_DECTUSB_NOT_ATOMIC, x);\
	}

#define ulog_ctx_type_done(x) \
	if(in_irq()){\
		ulog(ULOG_DECTUSB_HARDIRQ_CTX_DONE, x);\
	}else if(in_softirq()){\
		ulog(ULOG_DECTUSB_SOFTIRQ_CTX_DONE, x);\
	}else{\
		ulog(ULOG_DECTUSB_NOT_ATOMIC_DONE, x);\
	}
#else
#define ulog_ctx_type(x)
#define ulog_ctx_type_done(x)
#endif

#define ulog_traces_at_line() ulog(ULOG_DECTUSB_TRACES_AT_LINE, __LINE__)

enum FUNC_IDS {
	DISCONNECT = 0x80,
	SYNCHRO_INTR,
	PLAYBACK_INTR,
	CAPTURE_OPEN,
	CAPTURE_CLOSE,
	PLAYBACK_OPEN,
	PLAYBACK_CLOSE,
	PLAYBACK_REQUEST_URB,
	SYNCHRO_REQUEST_URB,
	CAPTURE_START,
	CAPTURE_STOP,
	PLAYBACK_START,
	PLAYBACK_STOP,
};

typedef enum FUNC_IDS FUNC_IDS;

#endif
