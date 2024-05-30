/*
 *  DECT USB driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dectusb_audio.c
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



/*
 * Dect audio's interface protocol :
 *
 * 	IN and OUT direction (dect to host) : 
 *
 *
 * 		1ms		2		3	...	10ms		1ms		2
 *
 * 		|---------------|---------------|----------------|--------------|---------------|-----------> time
 *	
 *		64 bytes	18 bytes					64 bytes	18 bytes
 * 
 *
 *
 *
 *	 		|---------------|---------------------------------------|
 * 	64 bytes 	|   report id  	|	data				|
 *	 		|---------------|---------------------------------------|
 *				1 byte		63 bytes
 * 
 *
 * 			|---------------|-----------------------|
 * 	18 bytes 	|   report id  	|	data		|
 *	 		|---------------|-----------------------|
 *				1 byte		17 bytes
 *
 *
 *
 *	report ids : host to dongle : 	64 bytes buffer : 0x7e
 *					18 bytes buffer : 0x50
 * 
 *
 * Driver constraint : playback period size must be a multiple of 80 samples
 *
 * features :
 *
 * 	- Supported encoding formats :
 * 		.	A law	: set encoding_format to 0
 * 		.	PCM	: set encoding_format to 1 (default case)
 *		
 *		Use sysctl to set the encoding format :
 * 			.	echo 1 > /proc/sys/DECT/encoding_format
 * 			.	sysctl -w DECT.encoding_format=1
 * 			.	using sysctl() in a C program
 *
 *
 */

#include <linux/sysctl.h>
#include <asm/hardirq.h>


#include "dectusb_audio.h"
#include "dectusb_audio_priv.h"



/*********************************************************************************/
/*		 G711 Convesion functions    A law <-> PCM 16 bits 		 */
/*********************************************************************************/
/*********************************************************************************/
/*********************************************************************************/
/* 		got from Sun Microsystems, on ftp.iptel.org 			 */
/*********************************************************************************/


static short search(
		short           val,
		short           *table,
		short           size)
{
	short           i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF};

#define SIGN_BIT        (0x80)          /* Sign bit for a A-law byte. */
#define QUANT_MASK      (0xf)           /* Quantization field mask. */
#define SEG_MASK        (0x70)          /* Segment field mask. */
#define SEG_SHIFT       (4)             /* Left shift for segment number. */


/* got from Sun Microsystems, on ftp.iptel.org */
/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *              Linear Input Code       Compressed Code
 *      ------------------------        ---------------
 *      za                    yz
 *      01wxyza                    yz
 *      1wxyzab                    010wxyz
 *      wxyzabc                    011wxyz
 *      xyzabcd                    1z
 *      yzabcde                    101wxyz
 *      01wxyzabcdef                    110wxyz
 *      1wxyzabcdefg                    111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
unsigned char
linear2alaw(
		short           pcm_val)        /* 2's complement (16-bit range) */
{
	short           mask;
	short           seg;
	unsigned char   aval;

	pcm_val = pcm_val >> 3;

	if (pcm_val >= 0) {
		mask = 0xD5;            /* sign (7th) bit = 1 */
	} else {
		mask = 0x55;            /* sign bit = 0 */
		pcm_val = -pcm_val - 1;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_aend, 8);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)           /* out of range, return maximum value. */
		return (unsigned char) (0x7F ^ mask);
	else {
		aval = (unsigned char) seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 1) & QUANT_MASK;
		else
			aval |= (pcm_val >> seg) & QUANT_MASK;
		return (aval ^ mask);
	}
}

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
short
alaw2linear(
		unsigned char   a_val)
{
	short           t;
	short           seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
		case 0:
			t += 8;
			break;
		case 1:
			t += 0x108;
			break;
		default:
			t += 0x108;
			t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}




/*********************************************************************************/
/*					 MISC					 */
/*********************************************************************************/

/** 
 *
 *
 * @param dest OUT The buffer to set
 * @param val IN Value to set
 * @param size IN Size of the buffer
 */
void dectmemset(unsigned char *dest, unsigned int val, unsigned int size){
	if(!dest || val < 0 || size < 0){
		ERROR("EINVAL : dest (%p) val (%d) size (%d)", dest, val, size);
		DECT_BUG();
		return;
	}

	switch(encoding_format){
		case DECT_AUDIO_FORMAT_PCM:
			memset(dest, 0x55, sizeof(short) * size);
			break;
		case DECT_AUDIO_FORMAT_ALAW:
			memset(dest, 0x55, size);
			break;
		default:
			ERROR("Corrupted format");
			DECT_BUG();
			return;
	}

	return;
}

/** 
 * Copy incoming data (from dect audio device) 
 * in alsa's buffer. If encoding_format is set to 1,
 * buffer will be converted to pcm.
 * 
 * @param dest OUT Alsa's buffer.
 * @param offset IN Offset in Alsa's buffer (number of frames).
 * @param src IN Incoming data from dect audio device
 * @param size IN size of data to copy
 */
void dectmemcpy_alaw_to_pcm(unsigned char *dest, int offset, unsigned char *src, int size){
	short *tmp_dest = NULL;
	int i;

	if(!src || !dest || offset < 0 || size < 0 || size > SIZE_OF_FIRST_MESSAGE){
		ERROR("Invalid value : src (%p) dest (%p) offset (%d) size (%d)", src, dest, offset, size);
		DECT_BUG();
		return;
	}
	
	switch(encoding_format){
		 case DECT_AUDIO_FORMAT_PCM:
			 tmp_dest = ((short *)dest);
			 tmp_dest += offset;

			 for(i = 0; i < size; i++){
				 tmp_dest[i] = alaw2linear(src[i]);
			 }
			 break;
		 case DECT_AUDIO_FORMAT_ALAW:
			 memcpy(dest + offset, src, size);
			 break;
		default:
			ERROR("Corrupted format");
			DECT_BUG();
			return;
	}

	return;
}

/** 
 * Copy data from alsa to dect audio device's 
 * outoing buffer. If encoding_format is set to 1,
 * buffer will be converted from pcm to alaw.
 * 
 * @param dest OUT The urb's buffer, data to dongle.
 * @param src IN Incoming data from alsa.
 * @param offset IN Offset in Alsa's buffer (number of frames).
 * @param size IN size of data to copy (number of frames).
 */
void dectmemcpy_pcm_to_alaw(unsigned char *dest, unsigned char *src, int offset,  int size){
	int i;
	short *src_short = NULL;

	if(!src || !dest || offset < 0 || size < 0 || size > SIZE_OF_FIRST_MESSAGE){
		ERROR("Invalid value : src (%p) dest (%p) offset (%d) size (%d)", src, dest, offset, size);
		DECT_BUG();
		return;
	}
	switch(encoding_format){
		 case DECT_AUDIO_FORMAT_PCM:
			 src_short = (short *)src;
			 src_short += offset;
			 for(i = 0; i < size; i++){
				 dest[i] = linear2alaw(src_short[i]);
			 }
			 break;
		 case DECT_AUDIO_FORMAT_ALAW:
			 memcpy(dest, src + offset, size);
			 break;
		 default:
			 ERROR("Corrupted format");
			 DECT_BUG();
			 return;
	}
	return;
}


/*********************************************************************************/
/*					 SYNCHRO				 */
/*********************************************************************************/



/*! \brief Callback called when a urb is complete
 * @param *urb
 * @param *regs 
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
static void synchro_intr(struct urb *urb, struct pt_regs *regs)
#else
static void synchro_intr(struct urb *urb)
#endif
{
	struct dectusb_audio_t *ldect = NULL;
	dectusb_audio_sync_t *sync = NULL; 
	ulog_ctx_type(SYNCHRO_INTR);
	db_sync_packet++;

	ldect = urb->context;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		goto end;
	}
	sync = &(ldect->sync);
	sync->error = urb->status;
	switch(urb->status){
		case 0:
		case -EOVERFLOW: /* It happens when a URB of 18 bytes is asked and 64 are received */
			break;
		default:
			ERROR("URB error (%d).", urb->status);
			sync->run = 0;
			exec_user_r_callback(ldect, urb->status);
			/* :FIXME:17.10.2009 13:38:34:: Dongle has may be reboot */

			/* :TODO:17.10.2009 20:59:08:: May be we can recover 
			   ret = synchro_request_urb((struct dectusb_audio_t *)urb->context);
			   if(ret){
			   ERROR("Could not recover from error");
			   goto end_stop;
			   }*/
			goto end;
	}

	if(!sync->run){
		WARNING("Driver DECT halted");
		goto end;
	}

	if(ldect->r.transfer_status && urb->actual_length)
		capture_handle_data(urb, ldect);

	synchro_request_urb((struct dectusb_audio_t *)urb->context);

	if(ldect->w.transfer_status && sync->size == SIZE_OF_SECOND_MESSAGE_HID){
		/* :TODO:18.10.2009 23:10:00:: calling playback_request_urb in a workqueue, or waking up a wainting task*/
		playback_request_urb((struct dectusb_audio_t *)urb->context);
	}

end:
	ulog_ctx_type_done(SYNCHRO_INTR);
	return;
}

/*! \brief ask a urb to usb core. 
 * 
 * @param *p_dect IN a pointer to dectusb_audio_t structure
 * @return 0 if successfull. < 0 otherwise:.
 *
 *
 * the callback synchro_intr will call to get a synchronisation signal.
 *
 *
 */
int synchro_request_urb(struct dectusb_audio_t * p_dect){
	int err = -EINVAL;
	dectusb_audio_sync_t *sync = NULL;
	ulog_ctx_type(SYNCHRO_REQUEST_URB);

	
	sync = &(p_dect->sync);

	if(!sync->run){
		WARNING("Driver DECT is not running");
		err = -ESRCH;
		goto fail;
	}

	if(!sync->size || 
			sync->size == SIZE_OF_SECOND_MESSAGE_HID)
		sync->size = SIZE_OF_FIRST_MESSAGE_HID;

	else 	
		if(sync->error == -EOVERFLOW){
			sync->size = SIZE_OF_FIRST_MESSAGE_HID;
		}
		else 	sync->size = SIZE_OF_SECOND_MESSAGE_HID;

	usb_fill_int_urb(sync->interrupt_urb,
			p_dect->usbdev,
			usb_rcvintpipe(p_dect->usbdev, sync->endpoint->bEndpointAddress),
			sync->buff,
			sync->size,
			synchro_intr,
			p_dect,
			sync->endpoint->bInterval);

	err = usb_submit_urb(sync->interrupt_urb, GFP_ATOMIC);
	if (err){	
		ERROR("usb_submit_urb : %d. sync->interrupt_urb->hcpriv(%p), URB status is %d", err, sync->interrupt_urb->hcpriv, sync->interrupt_urb->status);
		if(sync->interrupt_urb->hcpriv){
			usb_kill_urb(sync->interrupt_urb);
		}
		goto fail;
	}

fail:
	if(err){
		sync->run = 0;
		ERROR("error %d", err);
	}
	ulog_ctx_type_done(SYNCHRO_REQUEST_URB);
	return err;
}


/*********************************************************************************/
/*				 CAPTURE FUNCTUNS				 */
/*********************************************************************************/


/*! \brief Open capture audio dect usb driver.
 * @return Returns 0 if successful.
 */
int capture_open(void *private_data){
	struct dectusb_audio_t *ldect = NULL;
	int ret = 0;
	mutex_lock(&dectmutex);
	ulog_ctx_type(CAPTURE_OPEN);

	INFO("Opening DECT audio driver %s, on minor %d. Encoding format %d.", DECTUSB_AUDIO_DRIVER_VERSION, DECT_USB_MINOR_BASE_AUDIO, encoding_format);

	ldect = (struct dectusb_audio_t *)private_data;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		ret = -EFAULT;
		goto fail;
	}

	if(!ldect->sync.run){
		ERROR("Driver DECT : synchro loop is not running");
		ret = -ESRCH;
		goto fail;
	}

	switch(ldect->r.state){
		case DECT_OPEN:
			WARNING("device already open");
			ret = -EBUSY;
			goto fail;
		case DECT_CLOSE:
			lock_irq();
			clear_r_struct(&(ldect->r));
			ldect->r.state = DECT_OPEN;
			unlock_irq();
			break;
		default:
			ERROR("Device is in an unknown state : state is %d", ldect->r.state);
			ret = -EINVAL;
			goto fail;
	}

fail:
	ulog_ctx_type_done(CAPTURE_OPEN);
	mutex_unlock(&dectmutex);
	return ret;
}


/*! \brief Close capture audio dect usb driver.
 *
 * @param handle IN Driver handle to close.
 * @return Returns 0 if successful, < 0 if an error occured.
 */
void capture_close(void * private_data){
	struct dectusb_audio_t *ldect = NULL;
	mutex_lock(&dectmutex);
	ulog_ctx_type(CAPTURE_CLOSE);

	INFO("Closing capture DECT audio driver");

	ldect = (struct dectusb_audio_t *)private_data;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		goto fail;
	}
	lock_irq();
	clear_r_struct(&(ldect->r));
	ldect->r.state = DECT_CLOSE;
	unlock_irq();
fail:
	ulog_ctx_type_done(CAPTURE_CLOSE);
	mutex_unlock(&dectmutex);
	return;
}


/*! Read urb's callback 
 * 
 * @param ldect  IN Dectusb audio context
 * @param status IN Status of read urb's transfer 
 */
static void exec_user_r_callback(struct dectusb_audio_t * ldect, int status){
	dectusb_audio_r_t *r = NULL;
	int got_size;
	INFO("Enter");

	r = &(ldect->r);

	
	r->transfer_status = 0;
	r->alsa_buffer = NULL;
	got_size = r->alsa_asked_size;
	r->alsa_asked_size = 0;
	r->alsa_got_size = 0;

	if(r->UserCallback){
		r->UserCallback(r->UserCtx, status, got_size);
	} else {
		ERROR("Capture elapsed callback is NULL");
	}
	return;
}

/*! \brief Callback called when a urb is complete
 * @param *urb IN
 * @param *pdect IN a pointer to driver's context. 
 * Getting data from device and filling alsa's buffer.
 */
static void capture_handle_data(struct urb *urb, struct dectusb_audio_t *pdect)
{
	int s_left_in_alsa_buff;
	int size = 0;
	dectusb_audio_r_t *r = &(pdect->r);
	dectusb_audio_sync_t *sync = &(pdect->sync);

	if(urb->actual_length != urb->transfer_buffer_length){
		assert(!urb->actual_length || urb->actual_length == SIZE_OF_SECOND_MESSAGE_HID);
	}

	size = urb->actual_length-1;
	assert(size > 0); /* If actual_length is 1, it means that urb has only transfer the report id. */

	s_left_in_alsa_buff = r->alsa_asked_size - r->alsa_got_size;

	if(size > s_left_in_alsa_buff){
		r->size_still_in_buff = size - s_left_in_alsa_buff;
		dectmemcpy_alaw_to_pcm(r->alsa_buffer, r->alsa_got_size, sync->buff+1, s_left_in_alsa_buff);  /*+1 to delete the hid report id */
		memcpy(r->buffer, (sync->buff+1) + s_left_in_alsa_buff, r->size_still_in_buff);
		r->alsa_got_size += s_left_in_alsa_buff;
		assert(r->alsa_got_size == r->alsa_asked_size);
		exec_user_r_callback(pdect, urb->status);
		goto end;
	}

	dectmemcpy_alaw_to_pcm(r->alsa_buffer, r->alsa_got_size, sync->buff+1, size);  /*+1 to delete the hid report id */
	r->alsa_got_size += size;
	if(r->alsa_asked_size == r->alsa_got_size){
		exec_user_r_callback(pdect, urb->status);
		goto end;
	}
	assert(r->alsa_got_size < r->alsa_asked_size);

end:
	return;
}


/*! \brief Initialise dect transfer data from dongle to host
 * @param handle IN 
 * @param *bdest OUT A pointer to destination buffer.
 * @param per_size IN Frame periode size.
 * @param *p_UserCtx IN A pointer to user context.
 * @return If successful, dectusb_audio_read_init 0. Otherwise a negative value is returned.
 */
static int capture_start(
		void *dect_private_data, 
		void *bdest, 
		int per_size,
		void *p_UserCtx,
		elapsed_t p_UserCallback){

	struct dectusb_audio_t *ldect = NULL;
	int ret = 0;
	dectusb_audio_r_t *r = NULL;
	ulog_ctx_type(CAPTURE_START);

	if(per_size < SIZE_OF_FIRST_MESSAGE || !bdest){
		ERROR("Invalid param : size (%d) dest buffer (%p)", per_size, bdest);
		ret = -EINVAL;
		goto fail;
	}

	ldect = (struct dectusb_audio_t *)dect_private_data;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		ret = -EFAULT;
		goto fail;
	}

	r = &(ldect->r);
	if(r->state != DECT_OPEN){
		ERROR("Driver not open");
		ret = -EINVAL;
		goto fail;
	}

	if(r->transfer_status){
		WARNING("Device is busy");
		ret = -EBUSY;
		goto fail;
	}
	
	lock_irq();
	r->alsa_buffer = bdest;
	r->alsa_asked_size = per_size;
	r->alsa_got_size = 0;
	dectmemset(r->alsa_buffer, 0x0, r->alsa_asked_size);
	
	if(r->size_still_in_buff){
		if(r->size_still_in_buff > r->alsa_asked_size){
			ERROR("still_in_inc_buff cannot ---- yet ---- be greater than r_alsa_asked_size");
			clear_r_struct(r);
			ret = -EINVAL;
			unlock_irq();
			goto fail;
		}

		dectmemcpy_alaw_to_pcm(r->alsa_buffer, 0, r->buffer, r->size_still_in_buff);
		r->alsa_got_size += r->size_still_in_buff;
		r->size_still_in_buff = 0;
	}

	memset(r->buffer, 0, ldect->dongle_msg_size_max);

	r->UserCtx = p_UserCtx;
	r->UserCallback = p_UserCallback;
	r->transfer_status = 1;
	unlock_irq();
fail:
	ulog_ctx_type_done(CAPTURE_START);
	return ret;
}

int capture_stop(void *private_data){
	struct dectusb_audio_t *ldect = NULL;
	dectusb_audio_r_t *r = NULL;
	int ret = 0;
	mutex_lock(&dectmutex);
	ulog_ctx_type(CAPTURE_STOP);

	ldect = (struct dectusb_audio_t *)private_data;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		ret = -EFAULT;
		goto fail;
	}

	lock_irq();
	r = &(ldect->r);
	if(r->state != DECT_OPEN){
		ERROR("Driver not open");
		ret = -EPERM;
		unlock_irq();
		goto fail;
	}

	r->transfer_status = 0;
	unlock_irq();
fail:
	ulog_ctx_type_done(CAPTURE_STOP);
	mutex_unlock(&dectmutex);
	return ret;
}



/*********************************************************************************/
/*				 PLAYBACK FUNCTUNS				 */
/*********************************************************************************/


/*! \brief Open playback audio dect usb driver.
 * @return: 0 if successful, < 0 otherwise.
 */
static int playback_open(void *ctx){
	int ret = 0;
	struct dectusb_audio_t *ldect = NULL;
	mutex_lock(&dectmutex);
	ulog_ctx_type(PLAYBACK_OPEN);

	INFO("Opening DECT audio driver %s, on minor %d. Encoding format %d", DECTUSB_AUDIO_DRIVER_VERSION, DECT_USB_MINOR_BASE_AUDIO, encoding_format);

	ldect = ctx;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		ret = -EFAULT;
		goto end;
	}

	if(!ldect->sync.run){
		ERROR("Driver DECT : synchro loop is not running");
		ret = -ESRCH;
		goto end;
	}

	switch(ldect->w.state){
		case DECT_OPEN:
			WARNING("Device already open");
			ret = -EBUSY;
			goto end;
		case DECT_CLOSE:
			lock_irq();
			clear_w_struct(&(ldect->w));
			clear_circular_buff(&(ldect->w.ci));	
			ldect->w.state = DECT_OPEN;
			unlock_irq();
			break;
		default:
			ERROR("Device is in an unknown state, close it");
			ret = -EINVAL;
			goto end;
	}


end:
	ulog_ctx_type_done(PLAYBACK_OPEN);
	mutex_unlock(&dectmutex);
	return ret;
}


/*! \brief Close playback audio dect usb driver.
 *
 * @param handle IN Driver handle to close.
 * @return
 */
static void playback_close(void *ctx){
	struct dectusb_audio_t *ldect;
	mutex_lock(&dectmutex);
	ulog_ctx_type(PLAYBACK_CLOSE);
	INFO("Closing playback DECT audio driver");

	ldect = (struct dectusb_audio_t *)ctx;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		goto end;
	}

	lock_irq();
	ldect->w.state = DECT_CLOSE;
	clear_w_struct(&(ldect->w));
	unlock_irq();
end:
	ulog_ctx_type_done(PLAYBACK_CLOSE);
	mutex_unlock(&dectmutex);
	return;
}


/*! \brief call the user callback and reset the w struct  
 * @param ldect IN a pointer to the dectusb_audio_t structure
 * @param status IN status of the last urb transfer 
 */
static void exec_user_write_callback(struct dectusb_audio_t * ldect, int status){
	dectusb_audio_w_t *w = NULL;
	int sent_size;
	w = &(ldect->w);

	ulog_playback_delay_elapsed();
	sent_size = w->app_period_size - w->ci.app_size_left_in_space[w->ci.start_indx];

	w->transfer_status = 0;
	w->ci.app_size_left_in_space[w->ci.start_indx] = 0;
	w->ci.app_ptr[w->ci.start_indx] = NULL;
	w->ci.start_indx = ++w->ci.start_indx % NB_CIRCULAR_DATA_INFO;
	if(w->UserCallback){
		w->UserCallback(w->UserCtx, status, w->app_period_size - sent_size);
	} else {
		ERROR("Playback elapsed not define");
	}
	return;
}


/*! \brief Callback called when a urb is complete
 * @param *urb
 * @param *regs 
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
static void playback_intr(struct urb *urb, struct pt_regs *regs)
#else
static void playback_intr(struct urb *urb)
#endif
{
	struct dectusb_audio_t *ldect = NULL;
	dectusb_audio_w_t *w = NULL;
	ulog_ctx_type(PLAYBACK_INTR);
	db_snd_packet++;

	ldect = urb->context;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		goto end;
	}
	w = &(ldect->w);
	switch(urb->status){
		case 0:	
			break;
		default:
			ERROR("URB error %d", urb->status);
			exec_user_write_callback(ldect, urb->status);
			goto stop;
	}

	if(!w->transfer_status){
		goto end;
	}

	if(urb->actual_length != urb->transfer_buffer_length){
		WARNING("Sent size is different than expected. %d instead of %d", urb->transfer_buffer_length, urb->actual_length);
		assert(!urb->actual_length || urb->actual_length == SIZE_OF_SECOND_MESSAGE_HID);
	} 

	if((urb->actual_length - 1) <= w->ci.app_size_left_in_space[w->ci.start_indx]){
		w->ci.app_size_left_in_space[w->ci.start_indx] -= urb->actual_length - 1;
	} else{

		w->ci.app_size_left_in_space[(w->ci.start_indx + 1) % NB_CIRCULAR_DATA_INFO] -=  (urb->actual_length - 1 - w->ci.app_size_left_in_space[w->ci.start_indx]);
		w->ci.app_size_left_in_space[w->ci.start_indx] = 0;

	}

	w->ci.total_app_size_available -= urb->actual_length - 1;
	
	if(!w->ci.app_size_left_in_space[w->ci.start_indx]){
		exec_user_write_callback(ldect, 0);
	}

	if(w->out_size == SIZE_OF_FIRST_MESSAGE_HID){
		playback_request_urb(ldect);
	}

	ulog_ctx_type_done(PLAYBACK_INTR);
	return;
stop:
	clear_w_struct(&(ldect->w));
	ERROR("Fail. Calling mxc's callback");
end:
	ulog_ctx_type_done(PLAYBACK_INTR);
	return;
}

/*! Prepare the outgoing buffer (host to dongle) 
 * 
 * @param *p_dect 
 * @return 0 if successfull. Otherwise a negative value (if DECT_BUG() is not define. DECT_BUG() call panic()).
 */
static int playback_request_urb_prepare_buffer(dectusb_audio_w_t *w){

	if(!w->ci.app_ptr ||
			w->ci.start_indx < 0 ||
			w->ci.start_indx > NB_CIRCULAR_DATA_INFO ||
			w->ci.end_indx < 0 ||
			w->ci.end_indx > NB_CIRCULAR_DATA_INFO ||
			w->ci.total_app_size_available < (w->out_size-1) ||
			(w->out_size != SIZE_OF_FIRST_MESSAGE_HID && w->out_size != SIZE_OF_SECOND_MESSAGE_HID)){
		ERROR("see values : \n\tapp_ptr %p\n\tstart_indx %d\n\tend_indx %d\n\ttotal_app_size_available %d\n\tout_size %d", 
				w->ci.app_ptr, w->ci.start_indx, w->ci.end_indx, w->ci.total_app_size_available, (w->out_size-1));
		return -EINVAL;
	}


	if((w->out_size-1) <= w->ci.app_size_left_in_space[w->ci.start_indx]){
		dectmemcpy_pcm_to_alaw(w->out_buffer + 1, w->ci.app_ptr[w->ci.start_indx], w->app_period_size - w->ci.app_size_left_in_space[w->ci.start_indx], w->out_size-1);
	}else{
		if(w->ci.start_indx == w->ci.end_indx){
			ERROR("start index == end index");
			DECT_BUG();
			return -EINVAL;
		}
		dectmemcpy_pcm_to_alaw(w->out_buffer + 1, w->ci.app_ptr[w->ci.start_indx], w->app_period_size - w->ci.app_size_left_in_space[w->ci.start_indx], w->ci.app_size_left_in_space[w->ci.start_indx]);

		dectmemcpy_pcm_to_alaw(w->out_buffer + 1 + w->ci.app_size_left_in_space[w->ci.start_indx], w->ci.app_ptr[(w->ci.start_indx + 1)%NB_CIRCULAR_DATA_INFO], 0, w->out_size - 1 - w->ci.app_size_left_in_space[w->ci.start_indx]);
	}
	
	return 0;
}

/*! Fill the output buffer and ask a urb transfer.
 * 
 * @param *urb
 * @return 0 if successful. Otherwise, a negative value. 
 */
static int playback_request_urb(struct dectusb_audio_t *p_dect){
	int err = -EINVAL;
	dectusb_audio_w_t *w = NULL;
	ulog_ctx_type(PLAYBACK_REQUEST_URB);

	w = &(p_dect->w);

	if(!w->transfer_status)
		goto retok;

	if(w->interrupt_urb->status == -EINPROGRESS)
		goto retok;
	
	if(w->interrupt_urb->status == -EPROTO){
		ERROR("Proto error. Is retrofit running ?");
		goto fail;
	}

	if(!w->out_size || w->out_size == SIZE_OF_SECOND_MESSAGE_HID){
		w->out_size = SIZE_OF_FIRST_MESSAGE_HID;
	}
	else if(w->out_size == SIZE_OF_FIRST_MESSAGE_HID){
		w->out_size = SIZE_OF_SECOND_MESSAGE_HID;
	} else{
		ERROR("w->out_size(%d) is different than %d and %d", w->out_size, SIZE_OF_FIRST_MESSAGE_HID, SIZE_OF_SECOND_MESSAGE_HID);
		err = -EINVAL;
		goto fail;
	}

	/* If there is not enought data available ? otherwize, call egain */
	if(w->ci.total_app_size_available < w->out_size-1){
		ERROR("Not enougth space to send data : space left (%d) size needed for next transfer (%d)", 
				w->ci.total_app_size_available, w->out_size);
		/* Because period size is a multiple of DECT's 
		 * hw constraint, this case should not happen.
		 */
		assert(w->ci.total_app_size_available >= w->out_size-1);
	}

	w->out_buffer[0] = (w->out_size-1) + SND_REPORID;
			
	err = playback_request_urb_prepare_buffer(w);
	if(err)
		goto fail;
	
	usb_fill_int_urb(w->interrupt_urb,
				p_dect->usbdev,
				usb_sndintpipe(p_dect->usbdev, w->endpoint->bEndpointAddress),
				w->out_buffer,
				w->out_size,
				playback_intr,
				p_dect,
				w->endpoint->bInterval);


	err = usb_submit_urb(w->interrupt_urb, GFP_ATOMIC);
	if (err){	
		ERROR("usb_submit_urb() : error %d. see value : app_ptr[%d] %p, out_size %d", err, w->ci.start_indx, &(w->ci.app_ptr[w->ci.start_indx]), w->out_size);
		if(w->interrupt_urb->hcpriv){
			usb_kill_urb(w->interrupt_urb);
		}
		/* Do not a goto fail, because we may be can recover... */
		goto fail;
	}

retok:
	ulog_ctx_type_done(PLAYBACK_REQUEST_URB);
	return 0;

fail:
	ERROR("Fail");
	w->transfer_status = 0;
	ulog_ctx_type_done(PLAYBACK_REQUEST_URB);
	return err;
}


/*! \brief Initialise dect transfer data from host to dongle
 * @param handle IN
 * @param *src IN A pointer to source buffer.
 * @param size IN Frame periode size.
 * @param *p_UserCtx IN A pointer to user context.
 * @return 0 if successful. Otherwise a negative value is returned.
 */
int playback_start(
		void *handle, 
		void *src, 
		int size,
		void *p_UserCtx,
		elapsed_t p_UserCallback){

	int err = 0;
	struct dectusb_audio_t *ldect;
	dectusb_audio_w_t *w = NULL;
	ulog_ctx_type(PLAYBACK_START);

	if(!src){
		ERROR("Source buffer is NULL");
		err = -EINVAL;
		goto fail;
	}
	
	if(size < SIZE_OF_DONGLE_DECT_MESSAGE-2 &&
			(!size%(SIZE_OF_DONGLE_DECT_MESSAGE-2))/* HW constraint */){
		ERROR("The size of a period must be greater than %d bytes", SIZE_OF_FIRST_MESSAGE_HID);
		err = -EINVAL;
		goto fail;
	}

	ldect = (struct dectusb_audio_t *)handle;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		err = -EFAULT;
		goto fail;
	}

	w = &(ldect->w);
	if(w->state != DECT_OPEN){
		ERROR("Playback device is not open.");
		err = -EPERM;
		goto fail;
	}

	if(w->transfer_status){
		WARNING("Device is busy");
		err = -EBUSY;
		goto fail;
	}


	if(!w->app_period_size)
		w->app_period_size = size;
	else if(w->app_period_size != size){
		ERROR("Period size has changed during playback ! It is not allowed");
		err = -EINVAL;
		goto fail;
	}

	lock_irq();
	ulog_playback_delay();
	w->ci.app_ptr[w->ci.end_indx] = (unsigned char *)src;
	w->ci.app_size_left_in_space[w->ci.end_indx] = w->app_period_size;
	w->ci.total_app_size_available += w->app_period_size;
	w->ci.end_indx = ++w->ci.end_indx % NB_CIRCULAR_DATA_INFO;
	w->UserCtx = p_UserCtx;
	w->UserCallback = p_UserCallback;
	unlock_irq();

	w->transfer_status = 1;

fail:
	ulog_ctx_type_done(PLAYBACK_START);
	return err;
}


/*! Close the current stream. 
 * 
 * @param handle IN The dectusb driver contex
 * @return void 
 */
int playback_stop(void *private_data){
	struct dectusb_audio_t *ldect = NULL;
	dectusb_audio_w_t *w = NULL;
	int ret = 0;
	mutex_lock(&dectmutex);
	ulog_ctx_type(PLAYBACK_STOP);

	ldect = (struct dectusb_audio_t *)private_data;
	if(bad_ctx(ldect)){
		warning_bad_ctx(ldect);
		ret = -EFAULT;
		goto fail;
	}
	w = &(ldect->w);
	lock_irq();
	clear_w_struct(w);
	unlock_irq();
fail:
	ulog_ctx_type_done(PLAYBACK_STOP);
	mutex_unlock(&dectmutex);
	return 0;
}


/*********************************************************************************/
/*				 PROBE / DISCONNECT				 */
/*********************************************************************************/


#if defined(CONFIG_SND_AUDIO_BRIDGE)
struct audio_bridge_operations dectusb_operation = {

	.name = "dectusb",
	.period_size = 80,
	.playback_open = playback_open,
	.playback_close = playback_close,


	.playback_start = playback_start,
	.playback_stop = playback_stop,

	.capture_open = capture_open,
	.capture_close = capture_close,
	.capture_start = capture_start,
	.capture_stop = capture_stop,
};
#endif


/*! \brief Check if this type of endpoint is expected
 * @param *ep : a pointer to the endpoint to check
 * @param *direction : enpoint in ou out
 * @return 0 if successful, < 0 otherwise
 */
static int check_endpoint(struct usb_endpoint_descriptor *ep, int direction){
	if(!ep){
		ERROR("Endpoint %d is NULL", direction);
		return -ENODEV;
	}
#if 0
	if(vl > 2){
		INFO("Endpoint %d :", DECT_USB_INTERF_AUDIO_IN);
		seek_endpoints_type(ep);
	}
#endif

	if(SIZE_OF_FIRST_MESSAGE_HID != le16_to_cpu(ep->wMaxPacketSize)){
		WARNING("SIZE_OF_FIRST_MESSAGE_HID is different than the wMaxPacketSize ! does this driver works whith your dongle ?");
	}

	switch(direction){
		case DECT_USB_INTERF_AUDIO_IN:
			if(!usb_endpoint_is_int_in(ep)){
				ERROR("Endpoint %d not interrupt IN", direction);
				return -ENODEV;
			}
			break;

		case DECT_USB_INTERF_AUDIO_OUT:
			if (!usb_endpoint_is_int_out(ep)) {
				ERROR("Endpoint %d not interrupt OUT", direction);
				return -ENODEV;
			}
	}
	return 0;
}


/*! \brief Test if this interface is expected
 * @param *intf : The struct usb_interface given to the probe function 
 * @return If this interface is expected, test_intf() returns 0 
 */
static inline int test_intf(struct usb_interface *intf){
	struct usb_host_interface *linterface = intf->cur_altsetting;
	struct usb_device *udev = interface_to_usbdev(intf);
	char string[DECT_INTERFACE_STRING_NAME_SIZE_MAX];
	int ret = -EINVAL;

	/* TODO
	 * Tester le idProduct
	 *
	 */

	if (linterface->desc.bNumEndpoints != 2){
		ERROR("Interface %d has not 2 Endpoints", linterface->desc.iInterface);
		return -ENODEV;
	}
	memset(string, 0x0, DECT_INTERFACE_STRING_NAME_SIZE_MAX);
	ret = usb_string(udev, linterface->desc.iInterface, string, DECT_INTERFACE_STRING_NAME_SIZE_MAX);
	if(ret < 0){
		ERROR("Could not get string of usb interface (%d). String is used to determinate wich interface it is", linterface->desc.iInterface);
		return ret;
	}

	INFO("Interface name = %s, interface no=%d",string,linterface->desc.iInterface);

	if(strlen(string) != strlen(DECT_INTERFACE_STRING_NAME_AUDIO) ||
			memcmp(string, DECT_INTERFACE_STRING_NAME_AUDIO, strlen(DECT_INTERFACE_STRING_NAME_AUDIO))){ 
		INFO("Interface %d is not an Audio interface ('%s' (%d bytes) != '%s' (%d bytes))", 
				linterface->desc.iInterface, 
				string,
				strlen(string),
				DECT_INTERFACE_STRING_NAME_AUDIO,
				strlen(DECT_INTERFACE_STRING_NAME_AUDIO));
		return -ENODEV;
	}

	
	INFO("Will compare %s (%d bytes)  and %s (%d bytes)", string, strlen(string), DECT_INTERFACE_STRING_NAME_AUDIO, strlen(string));
	if(memcmp(string, DECT_INTERFACE_STRING_NAME_AUDIO, strlen(DECT_INTERFACE_STRING_NAME_AUDIO))){ 
		WARNING("Interface %d is not an audio interface", linterface->desc.iInterface);
		return -ENODEV;
	}

	INFO("Device AUDIO %s (%d) found", string, linterface->desc.iInterface);
	return 0;
}

static struct file_operations fops = {
	.owner		=	THIS_MODULE,
#if 0
	.ioctl		=	ioctl,
#endif
};

static struct usb_class_driver class = {
	.name		=       DECTUSB_AUDIO_NODE_NAME"%d",
	.fops		=       &fops,
	.minor_base	=   	DECT_USB_MINOR_BASE_AUDIO,
}; 


const struct usb_device_id dectusb_audio_id_tab [] = {
	{ USB_VENDOR_DEVICE(DONGLE_D45_USB_VENDOR_ID) },
	{ USB_INTERFACE_INFO(3, 0, 0) },
	{ USB_DEVICE_INFO(0, 0, 0) },
	{} /* Fin de la liste */
};
MODULE_DEVICE_TABLE(usb, dectusb_audio_id_tab);


/*! \brief Called by the usb core subsystem 
 * @param *intf : The struct usb_interface
 * @param *id : The struct usb_device_id
 * @return Returns 0 if successful.
 */
static int probe(struct usb_interface *intf, const struct usb_device_id *id){
	int ret = -ENODEV;
	struct dectusb_audio_t *dect = NULL;
	char path[SIZE_PATH];

	INFO("Probe DECT audio interface : Version %s", DECTUSB_AUDIO_DRIVER_VERSION);

	if(Dectusb.ctx){
		return -ENODEV;
	}

	db_sync_packet = db_snd_packet = 0;

	ret = test_intf(intf);
	if(ret)
		goto fail;

	ret = set_struct(&dect, intf);
	if(ret)
		goto fail;

	if(!dect){
		ERROR("Error : dectusb_audio is NULL");
		ret = -ENODEV;
		goto fail;
	}

	ret = usb_register_dev(intf, &class);
	if (ret) {
		ERROR("Error usb_register_dev : Error num %d", ret);
		goto fail;
	}

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	INFO("Registering dectusb (%p) in bridge", dect);
	dect->dectusb_ops = &dectusb_operation;
	dect->dectusb_ops->private_data = dect;
	dect->bridge_handle = register_audio_bridge(dect->dectusb_ops);
	if(!dect->bridge_handle){
		ERROR("Could not register driver");
		ret = -ENODEV;
		goto fail;
	}
	INFO("bridge ctx (%p)", dect->bridge_handle)


#endif

	usb_set_intfdata(intf, dect);
	Dectusb.ctx = dect;
	usb_make_path(interface_to_usbdev(intf), path, SIZE_PATH-1);
	INFO("Dect AUDIO attached on %s. Minor is %d. vl = %d",	path, intf->minor, vl);


	dect->sync.run = 1;
	ret = synchro_request_urb(dect);
	if(ret)
		goto fail;

	return 0;
fail:
	usb_set_intfdata(intf, NULL);
	Dectusb.ctx = NULL;

	if(dect){
		dect->sync.run = 0;
#if defined(CONFIG_SND_AUDIO_BRIDGE)
		unregister_audio_bridge(dect->bridge_handle);
#endif
		free_r_struct(dect);	
		free_sync_struct(dect);	
		free_w_struct(dect);
		kfree(dect);
	}
	return ret?ret:-ENODEV;
}


/*! \brief Called by the usb core subsystem when the device is disconnect.
 * @param *intf : The struct usb_interface
 */
static void dect_audio_disconnect(struct usb_interface *intf) {
	struct dectusb_audio_t *dectusb_audio;

	INFO("Enter");
	ulog_ctx_type(DISCONNECT);

	mutex_lock(&dectmutex);
	lock_kernel();
	lock_irq(DISCONNECT);
	Dectusb.ctx = NULL;
	dectusb_audio = usb_get_intfdata(intf);
	usb_set_intfdata(intf, NULL);
	unlock_irq(DISCONNECT);
	usb_deregister_dev(intf, &class);
	unlock_kernel();

	if(dectusb_audio){

#if defined(CONFIG_SND_AUDIO_BRIDGE)
		INFO("Unregister driver from bridge");
		unregister_audio_bridge(dectusb_audio->bridge_handle);
#endif

		if(dectusb_audio->sync.interrupt_urb->status == -EINPROGRESS)
			WARNING("Synchro URB is in progress");

		if(dectusb_audio->w.interrupt_urb->status == -EINPROGRESS)
			WARNING("Output URB is in progress");

		free_r_struct(dectusb_audio);	
		free_sync_struct(dectusb_audio);	
		free_w_struct(dectusb_audio);	
		kfree(dectusb_audio);
	}
	INFO("Interface disconnected");
	ulog_ctx_type_done(DISCONNECT);
	mutex_unlock(&dectmutex);
	return;
}


static struct usb_driver driver = {
	.name		=	"dectusb_audio",
	.probe		=	probe,
	.disconnect	=	dect_audio_disconnect,
	.id_table	=	dectusb_audio_id_tab,
};


static int set_struct_sync(struct usb_endpoint_descriptor *ep, struct dectusb_audio_t *pdect){

	dectusb_audio_sync_t *sync = &(pdect->sync);

	sync->interrupt_urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!sync->interrupt_urb){
		ERROR("Could not allocate r.interrupt_urb");
		return -ENOMEM;
	}

	sync->endpoint = ep;

	sync->buff = kmalloc(pdect->dongle_msg_size_max, GFP_KERNEL);
	if(!sync->buff){
		ERROR("Could not allocate memory for sync buffer");
		return-ENOMEM;
	}
	
	clear_sync_struct(sync);

	return 0;
}


/*! \brief Initialize the incoming struct and the sync struct 
 * 
 * @param *ep  IN the IN endpoint
 * @param *r, IN the dectusb_audio_r_t to initialize 
 * @param *pdect IN a pointer to dectusb_audio_t structure 
 * @return 
 */
static int set_struct_r(struct usb_endpoint_descriptor *ep, struct dectusb_audio_t *pdect){
	dectusb_audio_r_t *r = &(pdect->r);

	r->alsa_buffer = NULL;
	r->alsa_asked_size = 0;
	r->alsa_got_size = 0;


	r->buffer = kmalloc(pdect->dongle_msg_size_max, GFP_KERNEL);
	if(!r->buffer){
		ERROR("Could not allocate memory for incoming data buffer");
		return-ENOMEM;
	}
	
	r->size_still_in_buff = 0;
	r->transfer_status = 0;
	r->state = DECT_CLOSE;
	return 0;
}

static void free_sync_struct(struct dectusb_audio_t *p_dect){

	if(p_dect){
		if(p_dect->sync.buff){
			kfree(p_dect->sync.buff);
			p_dect->sync.interrupt_urb = NULL;
		}

		if(p_dect->sync.interrupt_urb){
			usb_free_urb(p_dect->sync.interrupt_urb);
			p_dect->sync.interrupt_urb = NULL;
		}
	}
}

/*! \brief free the incoming struct and the sync struct field
 * 
 * @param *r, IN the dectusb_audio_r_t to initialize 
 * @return 
 */
static void free_r_struct(struct dectusb_audio_t *p_dect){
	if(p_dect){
		if(p_dect->r.buffer)
			kfree(p_dect->r.buffer);
	}
}


/*! \brief Initialize the outgoing structure 
 * 
 * @param *ep  IN the IN endpoint
 * @param *w, IN the dectusb_audio_r_t to initialize 
 * @param *pdect IN a pointer to dectusb_audio_t structure 
 * @return 
 */
static int set_struct_w(struct usb_endpoint_descriptor *ep, struct dectusb_audio_t *pdect){
	dectusb_audio_w_t *w = &(pdect->w);

	w->out_buffer = kmalloc(pdect->dongle_msg_size_max, GFP_KERNEL);
	if(!w->out_buffer){
		ERROR("Could not allocate memory for the outging data buffer");
		return -ENOMEM;
	}
	w->out_size = 0;

	w->interrupt_urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!w->interrupt_urb){
		ERROR("Could not allocate memory for outgoing interrupt urb");
		return -ENOMEM;
	}

	w->endpoint = ep;
	w->transfer_status = 0;
	w->state = DECT_CLOSE;
	w->app_period_size = 0;
	memset(&(w->ci), 0x0, sizeof(struct app_circular_data_info_t));
	return 0;
}

/*! Free allocated memory in dectusb_audio_w_t's field 
 * 
 * @param *p_dect IN A pointer to (dectusb_audio_t *)
 */
static void free_w_struct(struct dectusb_audio_t *p_dect){
	dectusb_audio_w_t *w = &(p_dect->w);

	if(w->interrupt_urb)
		usb_free_urb(w->interrupt_urb);

	if(w->out_buffer)
		kfree(w->out_buffer);

}

/*! \brief Allocate memory for struct dectusb_audio_t and set it.
 * @param **ptr : A pointer to struct dectusb_audio_t
 * @return Returns 0 if successful. < 0 otherwise 
 */
static int set_struct(struct dectusb_audio_t **ptr, struct usb_interface *intf) {
	struct dectusb_audio_t *dect = NULL;
	struct usb_endpoint_descriptor *ep;
	int ret = -ENODEV;

	*ptr = kzalloc(sizeof(struct dectusb_audio_t) , GFP_KERNEL);
	dect = *ptr;
	if(!dect) {
		ERROR("kzalloc on dectusb_audio");
		return -ENOMEM;
	}

	dect->usbdev = usb_get_dev(interface_to_usbdev(intf));
	if(!dect->usbdev){
		ERROR("When getting usbdev");
		goto fail;
	}

	INFO("number of interface : %d, number of endpoints : %d", intf->num_altsetting, intf->cur_altsetting->desc.bNumEndpoints);

	dect->dongle_msg_size_max = SIZE_OF_DONGLE_DECT_MESSAGE;
	
	ep = &intf->cur_altsetting->endpoint[DECT_USB_INTERF_AUDIO_IN].desc;
	ret = check_endpoint(ep, DECT_USB_INTERF_AUDIO_IN);
	if(ret)
		goto fail;

	ret = set_struct_r(ep, dect);
	if(ret)
		goto fail;

	ret = set_struct_sync(ep, dect);
	if(ret)
		goto fail;


	ep = &intf->cur_altsetting->endpoint[DECT_USB_INTERF_AUDIO_OUT].desc;
	ret = check_endpoint(ep, DECT_USB_INTERF_AUDIO_OUT);
	if(ret)
		goto fail;


	ret = set_struct_w(ep, dect);
	if(ret)
		goto fail;

	return 0;
fail:
	if(dect){
		free_r_struct(dect);
		free_sync_struct(dect);	
		free_w_struct(dect);
		kfree(dect);
	}
	ERROR("While setting dectusb_audio : error %d", ret);
	return ret;
}

static void clear_circular_buff(struct app_circular_data_info_t *cb){
	cb->start_indx = cb->end_indx = cb->total_app_size_available = 0;
	memset(cb->app_size_left_in_space, 0x0, sizeof(int)*NB_CIRCULAR_DATA_INFO);
}

/*! Set some dectusb_audio_r_t's field to 0 
 * @param *r IN A pointer to (dectusb_audio_r_t *)
 */
static inline void clear_r_struct(dectusb_audio_r_t *r){

	r->transfer_status = 0;
	r->alsa_buffer = NULL;
	r->alsa_asked_size = 0;
	r->alsa_got_size = 0;
	r->size_still_in_buff = 0;
	r->UserCtx = NULL;
	r->UserCallback = NULL;

}

/*! Set some dectusb_audio_sync_t's field to 0 
 * 
 * @param *sync IN A pointer to (dectusb_audio_sync_t *)
 */
static inline void clear_sync_struct(dectusb_audio_sync_t *sync){
	sync->error = 0;
	sync->size = 0;
	sync->run = 0;
}


/*! Set some dectusb_audio_w_t's field to 0 
 * 
 * @param *w IN A pointer to (dectusb_audio_w_t *)
 */
static inline void clear_w_struct(dectusb_audio_w_t *w){
	w->transfer_status = 0;
	
	w->app_period_size = 0;
	w->UserCtx = NULL;
	w->UserCallback = NULL;
	memset(&(w->ci), 0x0, sizeof(struct app_circular_data_info_t));
}



/*********************************************************************************/
/*				DECTUSB OPERATIONS 				 */
/*********************************************************************************/

struct dectusb_audio_ops mxc_dectusb_operations = {

	.playback_open = playback_open,
	.playback_close = playback_close,
	.playback_start = playback_start,
	.playback_stop = playback_stop,

	.capture_open = capture_open,
	.capture_close = capture_close,
	.capture_start = capture_start,
	.capture_stop = capture_stop,
};
EXPORT_SYMBOL(mxc_dectusb_operations);

void *mxc_dectusb_get_handle(void){
	struct dectusb_audio_t *ldect = NULL;
	struct usb_interface *lui = usb_find_interface(&driver, DECT_USB_MINOR_BASE_AUDIO);
	if (!lui){
		ERROR("Can't find device for minor %d", DECT_USB_MINOR_BASE_AUDIO);
		return NULL;
	}
	ldect = usb_get_intfdata(lui);
	if(!ldect){
		ERROR("Could not get interface data");
		return NULL;
	}
	return (void *)ldect;
}
EXPORT_SYMBOL(mxc_dectusb_get_handle);


/*********************************************************************************/
/*					proc info 				 */
/*********************************************************************************/

#define DECTUSB_AUDIO_VERSION		10
static char dectusb_audio_version[DECTUSB_AUDIO_VERSION];

struct ctl_table_header *proc_th;

static ctl_table proc_t[] = {
	{
		CTL_UNNUMBERED,
		"encoding_format",
		&encoding_format,
		sizeof(int),
		0666,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		NULL,
#else
		(struct ctl_table *)NULL,
		(struct ctl_table *)NULL,
#endif
		&proc_dointvec,
		&sysctl_intvec,
	},
	{
		CTL_UNNUMBERED,
		"version",
		dectusb_audio_version,
		sizeof(char) * DECTUSB_AUDIO_VERSION,
		0444,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		NULL,
#else
		(struct ctl_table *)NULL,
		(struct ctl_table *)NULL,
#endif
		&proc_dostring,
		&sysctl_string,
	},
	{
		CTL_UNNUMBERED,
		"Received_packets",
		&db_sync_packet,
		sizeof(unsigned int),
		0444,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		NULL,
#else
		(struct ctl_table *)NULL,
		(struct ctl_table *)NULL,
#endif
		&proc_dointvec,
		&sysctl_intvec,
	},
	{
		CTL_UNNUMBERED,
		"Sent_packets",
		&db_snd_packet,
		sizeof(unsigned int),
		0444,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		NULL,
#else
		(struct ctl_table *)NULL,
		(struct ctl_table *)NULL,
#endif
		&proc_dointvec,
		&sysctl_intvec,
	},
	{
		0
	}
};


static ctl_table proc_dirt[] = {
	{
		CTL_UNNUMBERED,
		"dectusb_audio",
		NULL,
		0,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		0,
#else
		0555,
#endif
		proc_t,
	},
	{0},
};




/*********************************************************************************/
/*				__INIT / EXIT__ 				 */
/*********************************************************************************/


/**
 * @return Returns 0 if successful.
 */
static int __init dectusb_audio_init(void) {

	INFO("insmod of dectusb audio");
	spin_lock_init(&sl_irq);
	mutex_init(&dectmutex);
	memcpy(dectusb_audio_version, DECTUSB_AUDIO_DRIVER_VERSION, strlen(DECTUSB_AUDIO_DRIVER_VERSION));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
	proc_th = register_sysctl_table(proc_dirt, 0);
#else
	proc_th = register_sysctl_table(proc_dirt);
#endif
	if(!proc_th)
	{
		ERROR("Could not open sysctl table");
		return -EPERM;
	}
	return usb_register(&driver);
}

/**
 * @return Returns 0 if successful.
 */
static void __exit dectusb_audio_exit(void) {
	INFO("rmmod of dectusb audio");
	if(proc_th)
		unregister_sysctl_table(proc_th);

	usb_deregister(&driver);
}


module_init(dectusb_audio_init);
module_exit(dectusb_audio_exit);
MODULE_LICENSE("GPL");

