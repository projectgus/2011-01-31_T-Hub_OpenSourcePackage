/*
 * Copyright 2004-2007 Sagem Communications All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

 /*!
  * @defgroup SOUND_DRV MXC Sound Driver for ALSA
  */

 /*!
  * @file       mxc-alsa-dect.c
  * @brief      this fle       mxc-alsa-dect.c
  * @brief      this file implements the dect sound driver interface for ALSA.
  *             
  *      
  *             Recording supports 8000 khz sample rate. 
  *             Playback supports 8000 khz.
  *          
  * 
  * @ingroup    SOUND_DRV
  */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/soundcard.h>

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif				/* CONFIG_PM */

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>

#include "../dect_sagem/dectusb_audio.h"

#define MXC_DECT_USB_VERSION	"v1.0"

//#define TRACE_DMA


#define MXC_DECT_USB_PROG_NAME	"[MXC_DECT_USB] "

#undef pr_debug
#undef pr_error
#undef pr_info
#define pr_info(fmt, args...)		/*printk(MXC_DECT_USB_PROG_NAME "INFO  %d %s()  "fmt"\n", __LINE__, __func__, ## args);*/
#define pr_debug(fmt, args...)		/*printk(MXC_DECT_USB_PROG_NAME "DEBUG  %d %s()  "fmt"\n", __LINE__, __func__, ## args);*/
#define pr_error(fmt, args...)   	printk(MXC_DECT_USB_PROG_NAME "ERROR  %d %s()  "fmt"\n", __LINE__, __func__, ## args);


#define alsa_dectusb_assert(expr)                                                       \
        if (unlikely(!(expr))) {                                                        \
                printk(KERN_ERR "%s %d : BUG ! (%s)\n", __func__, __LINE__, #expr);     \
                dump_stack();                                                           \
                panic("panic");                                                         \
        }




#if 0 //defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_AUDIO)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

#define DECT_SUPPORTED_FORMATS			(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_MU_LAW | SNDRV_PCM_FMTBIT_A_LAW)

/*
 * DECT driver buffer policy. 
 * Customize here if the sound is not correct
 */
#define MAX_BUFFER_SIZE  			(32*1024)
#define DMA_BUF_SIZE				(8*1024)
#define MIN_PERIOD_SIZE				64
#define MIN_PERIOD				2
#define MAX_PERIOD				255

#define AUD_MUX_CONF 				0x0031010
#define MASK_2_TS				0xfffffffc
#define MASK_1_TS				0xfffffffe
#define MASK_4_TS				0xfffffffa
#define SOUND_CARD_NAME				"DECT"


/*!
 * These defines enable DMA chaining for playback
 * and capture respectively.
 */
#define MXC_SOUND_PLAYBACK_CHAIN_DMA_EN 1
#define MXC_SOUND_CAPTURE_CHAIN_DMA_EN 1

#define DECT_SAMPLE_RATE_MAX					0x1
/*!
  * ID for this card 
  */
static char *id = NULL;


#define MXC_ALSA_MAX_PCM_DEV 1
#define MXC_ALSA_MAX_PLAYBACK 1
#define MXC_ALSA_MAX_CAPTURE 1

#define DECT_STREAM_PLAYBACK_CHANNEL_0  0
#define DECT_STREAM_CAPTURE_CHANNEL_0   1

/*!
  * This structure represents an audio stream in term of
  * channel DMA, HW configuration on DECT and on AudioMux/SSI
  */
typedef struct audio_stream {
	/*!
	 * identification string 
	 */
	char *id;

	/*! 
	 * numeric identification 
	 */
	int stream_id;

	/*!
	 * we are using this stream for transfer now 
	 */
	int active:1;

	/*! 
	 * current transfer period 
	 */
	int period;

	/*!
	 * current count of transfered periods 
	 */
	int periods;

	/*!
	 * are we recording - flag used to do DMA trans. for sync 
	 */
	int tx_spin;

	/*!
	 * Previous offset value for resume
	 */
	unsigned int old_offset;

	/*!
	 * for locking in DMA operations 
	 */
	spinlock_t dma_lock;

	/*!
	 * Alsa substream pointer
	 */
	struct snd_pcm_substream *stream;

	/*!
	 * @ of the dect audio driver context
	 */
	void *dect_drv_cntx;

} audio_stream_t;

/*!
  * This structure represents the dect sound card with its
  * 2 streams and its shared parameters
  */
typedef struct snd_card_mxc_audio_dect {
	/*!
	 * ALSA sound card handle
	 */
	struct snd_card *card;

	/*!
	 * ALSA pcm driver type handle
	 */
	struct snd_pcm *pcm[MXC_ALSA_MAX_PCM_DEV];

	/*!
	 * playback & capture streams handle
	 */
	audio_stream_t s[MXC_ALSA_MAX_CAPTURE + MXC_ALSA_MAX_PLAYBACK];

	/* channel rx active*/
	int active_channel_rx ;

	/* channel tx active*/
	int active_channel_tx ;

} mxc_audio_dect_t;


/*!
 * This structure is used to store pre-calculated values
 * to set ccm_clk and prescaler divider correctly.
 *
 * The calculation of these values are entirely based on
 * current MPLL frequency. As this frequency is different
 * for each platform, the values are different for each supported
 * board.
 *
 * IMPORTANT: These structure is used only when AP side is the
 * provider of the audio clocks (BCL and FS) 
 *
 * Currently i300-30  sets MPLL to 240 Mhz or 400 Mhz.
 *
 * For each supported sample rate, there is one structure like this one.
 *
 */
struct freq_coefficients {
	/*!
	 * This variable holds the sample rates supported by our driver.
	 * Supported sample rates are:
	 *
	 * - 8Khz
	 */
	int sample_rate;

	/*!
	 * This variable holds the divider to be applied to the ccm_ssi_clk
	 * clock. This divider is used to get a base frequency from which
	 * the desired sample rate will be calculated. In other words we
	 * apply the following division:
	 *
	 * SERIALPLL/ccm_div
	 *
	 * ccm_ssi_clk clock is currently set 400 Mhz 
	 */
	int ccm_div;

	/*!
	 * This variable holds the prescaler divider to be applied to
	 * SERIALPLL/ccm_div result. Please note that this value is multiplied
	 * by 2 internally when SSI's STCCR register is set.
	 * (see set_audio_clocks and configure_ssi_tx functions)
	 */
	int prescaler;
};


/*!
  * Global variable that represents the DECT soundcard
  */
mxc_audio_dect_t *mxc_audio_dect = NULL;

static unsigned int playback_rates[] = {
	8000
};

/*!
  * Supported capture rates array
  */
static unsigned int capture_rates[] = {
	8000
};

/*!
  * this structure represents the sample rates supported
  * by DECT for playback operations.
  */
static struct snd_pcm_hw_constraint_list hw_playback_rates = {
	.count = ARRAY_SIZE(playback_rates),
	.list = playback_rates,
	.mask = 0,
};

/*!
  * this structure represents the sample rates supported
  * by DECT for capture operations.
  */
static struct snd_pcm_hw_constraint_list hw_capture_rates = {
	.count = ARRAY_SIZE(capture_rates),
	.list = capture_rates,
	.mask = 0,
};

/*! Get the stream ID
 * 
 * @param IN device number
 * @return A stream ID
 */
static int snd_card_mxc_capture_get_stream_id(const char *func, int device){
	switch(device){
		case 0:
		//pr_info("asked by %s(). stream id is %d", func, DECT_STREAM_CAPTURE_CHANNEL_0);
		return DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	
	pr_error("Channel %d is does not exist. (asked by %s)", device, func);
	return -ENODEV;;
}



/*! Get the playback's stream ID
 * 
 * @param IN device number
 * @return A stream ID
 */
static int snd_card_mxc_playback_get_stream_id(const char *func, int device){

	switch(device){
		case 0:
		//pr_info("asked by %s(). stream id is %d", func, DECT_STREAM_PLAYBACK_CHANNEL_0);
		return DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	
	pr_error("Channel %d is does not exist. (asked by %s)", device, func);
	return -ENODEV;
}

/*!
  * This function frees the stream structure
  *
  * @param	s	pointer to the structure of the current stream.
  */
static void audio_dect_dma_free(audio_stream_t * s)
{
	/* 
	 * There is nothing to be done here since the dma channel has been
	 * freed either in the callback or in the stop method
	 */

}

/*!
  * This function gets the dma pointer position during record.
  * Our DMA implementation does not allow to retrieve this position
  * when a transfert is active, so, it answers the middle of 
  * the current period beeing transfered
  *
  * @param	s	pointer to the structure of the current stream.
  *
  */
static u_int audio_dect_get_capture_pos(audio_stream_t * s)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int offset;

	substream = s->stream;
	runtime = substream->runtime;
	offset = 0;

	/* tx_spin value is used here to check if a transfert is active */
	if (s->tx_spin) {
		offset = (runtime->period_size * (s->periods)) + 0;
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("%s - capture audio_dect_get_dma_pos offset  %d",__FUNCTION__,offset);
#endif
	} else {
		offset = (runtime->period_size * (s->periods));
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
 		pr_debug("%s -capture audio_dect_get_dma_pos BIS offset %d, runtime->period_size = %d, (s->periods)) = %d",
				__FUNCTION__, offset, runtime->period_size, (s->periods));
#endif
	}

	return offset;
}

/*!
  * This function gets the dma pointer position during playback.
  * Our DMA implementation does not allow to retrieve this position
  * when a transfert is active, so, it answers the middle of 
  * the current period beeing transfered
  *
  * @param	s	pointer to the structure of the current stream.
  *
  */
static u_int audio_dect_get_playback_pos(audio_stream_t * s)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int offset;

	substream = s->stream;
	runtime = substream->runtime;
	offset = 0;

	/* tx_spin value is used here to check if a transfert is active */
	if (s->tx_spin) {
		offset = (runtime->period_size * (s->periods)) + 0;
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("%s - audio_dect_get_dma_pos offset  %d",__FUNCTION__, offset);
#endif
	} else {
		offset = (runtime->period_size * (s->periods));
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("%s - audio_dect_get_dma_pos BIS offset  %d",__FUNCTION__, offset);
#endif
	}

	return offset;
}

/*!
  * This function stops the current dma transfert for playback
  * and clears the dma pointers.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_dect_playback_stop(audio_stream_t * s)
{
	unsigned long flags;
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int dma_size;
	unsigned int offset;

	substream = s->stream;
	runtime = substream->runtime;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * s->periods;

	spin_lock_irqsave(&s->dma_lock, flags);

	pr_debug("%s - audio_dect_stop_dma active = 0",__FUNCTION__);
	s->active = 0;
	s->period = 0;
	s->periods = 0;

	spin_unlock_irqrestore(&s->dma_lock, flags);
}

/*!
  * This function stops the current dma transfert for capture
  * and clears the dma pointers.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_dect_capture_stop(audio_stream_t * s)
{
	unsigned long flags;
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int dma_size;
	unsigned int offset;

	substream = s->stream;
	runtime = substream->runtime;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * s->periods;

	spin_lock_irqsave(&s->dma_lock, flags);

	pr_debug("%s - capture audio_dect_stop_dma active = 0",__FUNCTION__);
	s->active = 0;
	s->period = 0;
	s->periods = 0;

	spin_unlock_irqrestore(&s->dma_lock, flags);
}

static void audio_dect_playback_callback(void *data, int error, unsigned int count);

/*!
  * This function is called whenever a new audio block needs to be
  * transferred to DECT. The function receives the address and the size 
  * of the new block and start a new DMA transfer.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_dect_playback_start(audio_stream_t * s)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int dma_size;
	unsigned int offset;
	int ret = 0;
	int stream_id = -1;
	int device ;

	substream = s->stream;
	runtime = substream->runtime;
	device = substream->pcm->device;

	stream_id = snd_card_mxc_playback_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0)
		return;
	
#ifdef TRACE_DMA
	pr_debug("DECT: audio_dect_playback_start device (%d) stream (%d)", device, stream_id);
#endif

	if (s->active) {
		dma_size = frames_to_bytes(runtime, runtime->period_size);

		offset = dma_size * s->period;
		snd_assert(dma_size <= DMA_BUF_SIZE,);


#ifdef TRACE_DMA
		pr_debug("%s - s->period (%x) runtime->periods (%d)",__FUNCTION__,s->period, runtime->periods);
 		pr_debug("%s - runtime->period_size (%d) dma_size (%d)",__FUNCTION__,(unsigned int)runtime->period_size,runtime->dma_bytes);
 		pr_debug("%s - Start playback DMA offset (%d) size (%d)",__FUNCTION__ ,offset,runtime->dma_bytes);
#endif


		ret = mxc_dectusb_operations.playback_start(
				s->dect_drv_cntx,
				runtime->dma_area+offset, 
				runtime->period_size,
				s,
				audio_dect_playback_callback);

		if(ret){
			pr_error("dectusb_audio_write_init : error %d", ret);
			return;
		}

		s->tx_spin = 1;	/* FGA little trick to retrieve DMA pos */

		if (ret) {
			pr_error("audio_process_dma: cannot queue DMA buffer (%i)",ret);
			return;
		}
		s->period++;
		s->period %= runtime->periods;

#ifdef MXC_SOUND_PLAYBACK_CHAIN_DMA_EN
		if ((s->period > s->periods) && ((s->period - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: already double buffered");
#endif
			return;
		}

		if ((s->period < s->periods)
		    && ((s->period + runtime->periods - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: already double buffered");
#endif
			return;
		}

		if (s->period == s->periods) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: s->period == s->periods");
#endif
			return;
		}

		if (snd_pcm_playback_hw_avail(runtime) <
		    2 * runtime->period_size) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: available data is not enough");
#endif
			return;
		}
#endif				/* MXC_SOUND_PLAYBACK_CHAIN_DMA_EN */

	}

}


/*! If dectusb_audio needs more data, it call audio_dect_playback_again().
 *
 * 
 * @param s pointer to the structure of the current stream.
 * @return 0 if stream is active. Otherwise, it returns 1.
 *
 * audio_dect_playback_again() is exported. It is called by dectusb_audio
 *
 */
int audio_dect_playback_again(audio_stream_t * s){

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_MXC_WRITE_AGAIN, 0);
#endif
	if(s->active)
		audio_dect_playback_start(s);
	else return 1;

	return 0;
}
EXPORT_SYMBOL(audio_dect_playback_again);


static void audio_dect_capture_callback(void *data, int error, unsigned int count);

/*!
  * This function is called whenever a new audio block needs to be
  * transferred from DECT. The function receives the address and the size 
  * of the block that will store the audio samples and start a new DMA transfer.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_dect_capture_start(audio_stream_t * s)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int dma_size;
	unsigned int offset;
	int ret = 0;
	int device; 
	int stream_id = -1;

	substream = s->stream;
	runtime = substream->runtime;
	
	device = substream->pcm->device;
#ifdef TRACE_DMA
	pr_debug("capture dma device %d", device);
#endif

	stream_id = snd_card_mxc_capture_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0)
		return;


	if (s->active) {
		dma_size = frames_to_bytes(runtime, runtime->period_size);

		offset = dma_size * s->period;
		snd_assert(dma_size <= DMA_BUF_SIZE,);

#ifdef TRACE_DMA
		pr_debug("DECT: Start capture DMA offset (%d) size (%d)", offset,runtime->dma_bytes);
		pr_debug("DECT: Start capture DMA channel (%d)", s->dma_wchannel);
		pr_debug("DECT: Start capture device (%d) stream (%d)", device, stream_id);
#endif


		ret = mxc_dectusb_operations.capture_start(s->dect_drv_cntx, runtime->dma_area+offset, runtime->period_size, s, audio_dect_capture_callback);
		if(ret){
			pr_error("dectusb_audio_read_init : error %d", ret);
			return;
		}

		s->tx_spin = 1;	/* FGA little trick to retrieve DMA pos */

		s->period++;
		s->period %= runtime->periods;

#ifdef MXC_SOUND_CAPTURE_CHAIN_DMA_EN
		if ((s->period > s->periods) && ((s->period - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio capture chain dma: already double buffered");
#endif
			return;
		}

		if ((s->period < s->periods)
		    && ((s->period + runtime->periods - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio capture chain dma: already double buffered");
#endif
			return;
		}

		if (s->period == s->periods) {
#ifdef TRACE_DMA
			pr_debug("audio capture chain dma: s->period == s->periods");
#endif
			return;
		}

		if (snd_pcm_capture_hw_avail(runtime) < 2 * runtime->period_size) {
#ifdef TRACE_DMA
			pr_debug("audio capture chain dma: available data is not enough");
#endif
			return;
		}
#endif

	}
}

/*! 
 * This is a callback which will be called
 * when a TX transfer finishes. The call occurs
 * in interrupt context.
 * 
 * @param IN handle 
 * @param IN error 
 * @param IN *data 
 * @param IN count 
 */
static void audio_dect_playback_callback(void *data, int error, unsigned int count){
	audio_stream_t *s;
	

	pr_debug("Enter");

	s = data;
	if(!s || !s->stream){
		pr_error("data(%p) or stream(%p) in NULL", s, s->stream);
		return;
	}

	if(error == -EAGAIN){
		/*
		 * Trig next DMA transfer 
		 */
		audio_dect_playback_start(s);
		return;
	}


#if defined(DO_ULOG)
	{
		struct snd_pcm_substream *substream;
		struct snd_pcm_runtime *runtime;
		unsigned int dma_size;
		unsigned int previous_period;
		unsigned int offset;
		substream = s->stream;
		runtime = substream->runtime;
		previous_period = s->periods;
		dma_size = frames_to_bytes(runtime, runtime->period_size);
		offset = dma_size * previous_period;
		ulog(ULOG_ALSA_DECT_DMA_WRITE_CALLBACK,(unsigned int)(runtime->dma_area + offset)) ;
	}
#endif

	s->tx_spin = 0;
	s->periods++;
	s->periods %= s->stream->runtime->periods;


#ifdef TRACE_DMA
	pr_debug("-");
#endif
	
	/* 
	 * If we are getting a callback for an active stream then we inform
	 * the PCM middle layer we've finished a period
	 */
	if (s->active)
		snd_pcm_period_elapsed(s->stream);

	spin_lock(&s->dma_lock);

	/*
	 * Trig next DMA transfer 
	 */
	audio_dect_playback_start(s);

	spin_unlock(&s->dma_lock);
}

/*!
  * This is a callback which will be called
  * when a RX transfer finishes. The call occurs
  * in interrupt context.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */

/*!  
 * This is a callback which will be called
 * when a RX transfer finishes. The call occurs
 * in interrupt context.
 *
 * @param error  IN
 * @param error  IN 
 * @param count  IN
 *
 */
static void audio_dect_capture_callback(void *data, int error, unsigned int count){
	audio_stream_t *s;
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	unsigned int dma_size;
	unsigned int previous_period;
	unsigned int offset;

	pr_debug("Enter");
	
	s = data;
	if(!s || !s->stream){
		pr_error("data(%p) or stream(%p) in NULL", s, s->stream);
		return;
	}

	substream = s->stream;
	runtime = substream->runtime;
	previous_period = s->periods;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * previous_period;

	s->tx_spin = 0;
	s->periods++;
	s->periods %= runtime->periods;

#ifdef TRACE_DMA
	pr_debug("-");
#endif

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_DMA_READ_CALLBACK,(unsigned int)(runtime->dma_area + offset)) ;
#endif

	/* 
	 * If we are getting a callback for an active stream then we inform
	 * the PCM middle layer we've finished a period
	 */
	if (s->active)
		snd_pcm_period_elapsed(s->stream);

	spin_lock(&s->dma_lock);

	/*
	 * Trig next DMA transfer 
	 */
	audio_dect_capture_start(s);

	pr_debug("Exit");
	spin_unlock(&s->dma_lock);
}

/*!
  * This function is a dispatcher of command to be executed
  * by the driver for playback.
  *
  * @param	substream	pointer to the structure of the current stream.
  * @param	cmd		command to be executed
  *
  * @return              0 on success, -1 otherwise.
  */
static int
snd_mxc_audio_dect_playback_trigger(struct snd_pcm_substream * substream, int cmd)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int err;
	int device;
	int stream_id = -1;

	pr_debug("Enter");
	device = substream->pcm->device;
	
	stream_id = snd_card_mxc_playback_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0)
		return stream_id;

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	err = 0;
	

	/* note local interrupts are already disabled in the midlevel code */
	spin_lock(&s->dma_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_START");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_WRITE_TRIGGER_START,0) ;
#endif
		s->tx_spin = 0;
		/* requested stream startup */
		s->active = 1;
		audio_dect_playback_start(s);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_STOP");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_WRITE_TRIGGER_STOP,0) ;
#endif
		/* requested stream shutdown */
		audio_dect_playback_stop(s);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		pr_debug("DECT : SNDRV_PCM_TRIGGER_SUSPEND active = 0");
		pr_error("DECT : SNDRV_PCM_TRIGGER_SUSPEND is not yet implemented");
#if 0
		s->active = 0;
		s->periods = 0;
#endif
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_RESUME");
		pr_error("DECT : SNDRV_PCM_TRIGGER_RESUME is not yet implemented");
#if 0
		s->active = 1;
		s->tx_spin = 0;
		audio_dect_playback_start(s);
#endif
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_PUSH");
		pr_error("DECT : SNDRV_PCM_TRIGGER_PAUSE_PUSH is not yet implemented");
#if 0
		s->active = 0;
#endif
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_RELEASE");
		pr_error("DECT : SNDRV_PCM_TRIGGER_PAUSE_RELEASE is not yet implemented");
#if 0
		s->active = 1;
		if (s->old_offset) {
			s->tx_spin = 0;
			audio_dect_playback_start(s);
			break;
		}
#endif
		break;
	default:
		pr_error("unknown command 0x%x",cmd);
		err = -EINVAL;
		break;
	}
	spin_unlock(&s->dma_lock);
	return err;
}

/*!
  * This function is a dispatcher of command to be executed
  * by the driver for capture.
  *
  * @param	substream	pointer to the structure of the current stream.
  * @param	cmd		command to be executed
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_dect_capture_trigger(struct snd_pcm_substream * substream, int cmd)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int err;
	int device, stream_id;

	device = substream->pcm->device;

	pr_debug("Enter");


	stream_id = snd_card_mxc_capture_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0)
		return stream_id;


	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	err = 0;


	/* note local interrupts are already disabled in the midlevel code */
	spin_lock(&s->dma_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_START");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_READ_TRIGGER_START,0) ;
#endif

		s->tx_spin = 0;
		/* requested stream startup */
		s->active = 1;
		audio_dect_capture_start(s);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_STOP");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_READ_TRIGGER_STOP,0) ;
#endif
		/* requested stream shutdown */
		audio_dect_capture_stop(s);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		pr_debug("DECT : SNDRV_PCM_TRIGGER_SUSPEND active = 0");
		s->active = 0;
		s->periods = 0;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_RESUME");
		s->active = 1;
		s->tx_spin = 0;
		audio_dect_capture_start(s);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_PUSH");
		s->active = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_RELEASE");
		s->active = 1;
		if (s->old_offset) {
			s->tx_spin = 0;
			audio_dect_capture_start(s);
			break;
		}
		break;
	default:
		pr_error("unknown command 0x%x",cmd);
		err = -EINVAL;
		break;
	}
	spin_unlock(&s->dma_lock);
	return err;
}

/*!
  * This function configures the hardware to allow audio 
  * playback operations. It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_dect_playback_prepare(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int stream_id = -1;
	int device ;
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_PREPARE,0) ;
#endif
	pr_debug("Enter");

	device = substream->pcm->device;
	stream_id = snd_card_mxc_playback_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0)
		return stream_id;

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];

	s->period = 0;
	s->periods = 0;
	
	//msleep(100);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_PREPARE_DONE,0) ;
#endif
	pr_debug("Exit");
	return 0;
}

/*!
  * This function gets the current capture pointer position.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static
snd_pcm_uframes_t snd_mxc_audio_dect_capture_pointer(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t * chip;
	int device;
	int stream_id;
	device = substream->pcm->device;

	stream_id = snd_card_mxc_capture_get_stream_id(__FUNCTION__, device); 
	/* What TODO : returning 0 or -EINVAL ? does alsa test offset returned by audio_dect_get_capture_pos() ? */
	if(stream_id < 0)
		return 0;

	chip = snd_pcm_substream_chip(substream);

	return audio_dect_get_capture_pos(&chip->s[stream_id]);
}

/*!
  * This function gets the current playback pointer position.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static snd_pcm_uframes_t
snd_mxc_audio_dect_playback_pointer(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t *chip;

	int device;
	int stream_id;
	device = substream->pcm->device;

	stream_id = snd_card_mxc_playback_get_stream_id(__FUNCTION__, device);
	/* What TODO : returning 0 or -EINVAL ? does alsa test offset returned by audio_dect_get_capture_pos() ? */
	if(stream_id < 0)
		return  0;

	chip = snd_pcm_substream_chip(substream);
	return audio_dect_get_playback_pos(&chip->s[stream_id]);
}

/*!
  * This structure reprensents the capabilities of the driver
  * in capture mode.
  * It is used by ALSA framework.
  */
static struct snd_pcm_hardware snd_mxc_dect_capture = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = DECT_SUPPORTED_FORMATS,
	.rates = SNDRV_PCM_RATE_8000,
	.rate_min = 8000,
	.rate_max = 8000,
	.channels_min = 1,
	.channels_max = 2,//??
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_SIZE,
	.period_bytes_max = DMA_BUF_SIZE,
	.periods_min = MIN_PERIOD,
	.periods_max = MAX_PERIOD,
	.fifo_size = 0,

};

/*!
  * This structure reprensents the capabilities of the driver
  * in playback mode 
  * It is used by ALSA framework.
  */
static struct snd_pcm_hardware snd_mxc_dect_playback = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = DECT_SUPPORTED_FORMATS,
	.rates = SNDRV_PCM_RATE_8000,
	.rate_min = 8000,
	.rate_max = 8000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_SIZE,
	.period_bytes_max = DMA_BUF_SIZE,
	.periods_min = MIN_PERIOD,
	.periods_max = MAX_PERIOD,
	.fifo_size = 0,

};

/*!
  * This function opens a DECT audio device in playback mode
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_dect_playback_open(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t *chip;
	struct snd_pcm_runtime *runtime;
	int stream_id = -1;
	int err = -1;
	int device;
	pr_info("Enter");
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_OPEN,0) ;
#endif

	chip = snd_pcm_substream_chip(substream);
	device = substream->pcm->device;
	stream_id = snd_card_mxc_playback_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0){
		return stream_id;
	}

	pr_info("chip (%p) device id  (%d) stream_id (%d)\n", chip, device, stream_id);

	runtime = substream->runtime;

	chip->s[stream_id].stream = substream;
	runtime->hw = snd_mxc_dect_playback;

	err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if(err < 0){
		pr_error("bad period");
		goto end;
	}
	
	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_playback_rates);
	if(err < 0){
		pr_error("bad sample rate");
		goto end;
	}

	msleep(10);

	chip->s[stream_id].dect_drv_cntx = mxc_dectusb_get_handle();
	if(!chip->s[stream_id].dect_drv_cntx){
		pr_error("Could not get driver dect handle");
		err = -ENODEV;
		goto end;
	}

	err = mxc_dectusb_operations.playback_open(chip->s[stream_id].dect_drv_cntx);
	if(err < 0){
		pr_error("Cannot open dectusb. Error %d", err);
		return err;
	}
	
	chip->active_channel_tx ++ ;

end:
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_OPEN_DONE,0) ;
#endif

	if(unlikely(err)){
		pr_error("Fail");
	} else {
		pr_info("Exit");
	}

	return err;
}

/*!
  * This function closes an DECT audio device for playback.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_dect_playback_close(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int device;
	int stream_id;

	pr_info("Enter");

	chip = snd_pcm_substream_chip(substream);
	device = substream->pcm->device;
	stream_id = snd_card_mxc_playback_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0){
		return stream_id;
	}

	pr_info("chip (%p) device id  (%d) stream_id (%d)\n", chip, device, stream_id);

	s = &chip->s[stream_id];

	chip->active_channel_tx -- ;

	chip->s[stream_id].stream = NULL;

	mxc_dectusb_operations.playback_close(chip->s[stream_id].dect_drv_cntx);

	pr_debug("stream_id=%d", stream_id);
	pr_info("Exit");
	return 0;
}


/*!
  * This function configure the Audio HW in terms of memory allocation.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_dect_hw_params(struct snd_pcm_substream * substream,
				   struct snd_pcm_hw_params * hw_params)
{
	pr_debug("Enter");
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

/*!
  * This function frees the audio hardware at the end of playback/capture.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_dect_hw_free(struct snd_pcm_substream * substream)
{
	pr_debug("Enter");
	return snd_pcm_lib_free_pages(substream);
}

/*!
  * This function configures the hardware to allow audio 
  * capture operations. It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_dect_capture_prepare(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int device;
	int stream_id;
	pr_debug("Enter");
	device = substream->pcm->device;

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_PREPARE,0) ;
#endif

	stream_id = snd_card_mxc_capture_get_stream_id(__FUNCTION__, device); 
	if(stream_id < 0)
		return stream_id;

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];

	pr_debug(" DECT substream->pstr->stream %d", substream->pstr->stream);
	
	chip->active_channel_rx ++ ;

	s->period = 0;
	s->periods = 0;

	
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_PREPARE_DONE,0) ;
#endif
	pr_debug("Exit");
	return 0;
}

/*!
 * This function opens the dect USB audio device
 * It is called by ALSA framework.
 *
 * @param	substream	pointer to the structure of the current stream.
 *
 * @return              0 on success, -1 otherwise.
 */
static int snd_card_mxc_audio_dect_capture_open(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t *chip;
	struct snd_pcm_runtime *runtime;
	int err;
	int device;
	int stream_id;
	pr_info("Enter");

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_OPEN,0) ;
#endif

	device = substream->pcm->device;
	stream_id = snd_card_mxc_capture_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0){
		return stream_id;
	}
	chip = snd_pcm_substream_chip(substream);

	pr_info("device id = %d stream_id %d\n", device, stream_id);

	runtime = substream->runtime;
	err = -1;
	chip->s[stream_id].stream = substream;
	runtime->hw = snd_mxc_dect_capture;

	err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS); 
	if(err < 0){
		pr_error("periods DECT audio stream capture");
		goto end;
	}

	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_capture_rates);
	if(err < 0){
		pr_error("snd_pcm_hw_constraint_list : rate") ;
		goto end;
	}

	chip->s[stream_id].dect_drv_cntx = mxc_dectusb_get_handle();
	if(!chip->s[stream_id].dect_drv_cntx){
		pr_error("Could not get driver dect handle");
		err = -ENODEV;
		goto end;
	}

	err = mxc_dectusb_operations.capture_open(chip->s[stream_id].dect_drv_cntx);
	if(err < 0){
		pr_error("dectusb_audio_open : error %d", err);
		goto end;
	}

end:
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_OPEN_DONE, err);
#endif
	if(unlikely(err)){
		printk("Fail (%d)", err);
	} else {
		pr_debug("Exit");
	}
	return err;
}


/*!
  * This function closes a DECT audio device for capture.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_dect_capture_close(struct snd_pcm_substream * substream)
{
	mxc_audio_dect_t *chip;
	int stream_id ;
	pr_info("Enter");

	stream_id = snd_card_mxc_capture_get_stream_id(__FUNCTION__, substream->pcm->device);
	if(stream_id < 0){
		return  stream_id;
	}
	chip = snd_pcm_substream_chip(substream);
	chip->active_channel_rx -- ;

	chip->s[stream_id].stream = NULL;
	
	mxc_dectusb_operations.capture_close(chip->s[stream_id].dect_drv_cntx);

	pr_debug("Exit");
	return 0;
}


/*!
 * This structure is the list of operation that the driver
 * must provide for the capture interface
 */
static struct snd_pcm_ops snd_card_mxc_audio_dect_capture_ops = {
	.open = snd_card_mxc_audio_dect_capture_open,
	.close = snd_card_mxc_audio_dect_capture_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_mxc_audio_dect_hw_params,
	.hw_free = snd_mxc_audio_dect_hw_free,
	.prepare = snd_mxc_audio_dect_capture_prepare,
	.trigger = snd_mxc_audio_dect_capture_trigger,
	.pointer = snd_mxc_audio_dect_capture_pointer,
};

/*!
 * This structure is the list of operation that the driver
 * must provide for the playback interface
 */
static struct snd_pcm_ops snd_card_mxc_audio_dect_playback_ops = {
	.open = snd_card_mxc_audio_dect_playback_open,
	.close = snd_card_mxc_audio_dect_playback_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_mxc_audio_dect_hw_params,
	.hw_free = snd_mxc_audio_dect_hw_free,
	.prepare = snd_mxc_audio_dect_playback_prepare,
	.trigger = snd_mxc_audio_dect_playback_trigger,
	.pointer = snd_mxc_audio_dect_playback_pointer,
};




/*!
 * This functions initializes the 2 audio devices supported by 
 * DECT IC. 
 *
 * @param	mxc_audio_dect	pointer to the sound card structure.
 * @param	device	        device id of the PCM stream.
 *
 */
void mxc_audio_dect_init(mxc_audio_dect_t * mxc_audio_dect, int device)
{
	int stream_id;
	pr_debug("Enter");
	pr_debug("device = %d", device);

	stream_id = snd_card_mxc_playback_get_stream_id(__FUNCTION__, device);
	if(stream_id < 0)
		return;

	mxc_audio_dect->s[stream_id].id = "Audio out";
	mxc_audio_dect->s[stream_id].stream_id =stream_id;

	stream_id = snd_card_mxc_capture_get_stream_id(__FUNCTION__, device); 
	if(stream_id < 0)
		return;

	mxc_audio_dect->s[stream_id].id = "Audio in";
	mxc_audio_dect->s[stream_id].stream_id = stream_id;

	pr_debug("Exit");
}

/*!
 * This function the soundcard structure.
 *
 * @param	mxc_audio	pointer to the sound card structure.
 * @param	device		the device index (zero based)	
 *
 * @return              0 on success, -1 otherwise.
 */
static int __init snd_card_mxc_audio_dect_pcm(mxc_audio_dect_t * mxc_audio_dect,
		int device)
{
	struct snd_pcm *pcm;
	int err;

	pr_debug("Enter");

	/* 
	 * Create a new PCM instance with 1 capture stream and 1 playback substream
	 */
	if ((err = snd_pcm_new(mxc_audio_dect->card, "DECT", device, 1, 1, &pcm)) < 0) {
		pr_error("snd_pcm_new err = 0x%x", err) ;
		return err;
	}

	/*
	 * this sets up our initial buffers and sets the dma_type to isa.
	 * isa works but I'm not sure why (or if) it's the right choice
	 * this may be too large, trying it for now
	 */
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
			snd_dma_continuous_data
			(GFP_KERNEL), MAX_BUFFER_SIZE * 2,
			MAX_BUFFER_SIZE * 2);

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_card_mxc_audio_dect_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			&snd_card_mxc_audio_dect_capture_ops);

	pcm->private_data = mxc_audio_dect;
	pcm->info_flags = 0;
	strncpy(pcm->name, SOUND_CARD_NAME, sizeof(pcm->name));
	mxc_audio_dect->pcm[device] = pcm;
	mxc_audio_dect_init(mxc_audio_dect, device);
	pr_debug("Exit");
	return 0;
}

#if 0 // no power managment is done with DECT card
#ifdef CONFIG_PM
/*!
 * This function suspends all active streams.
 *
 * TBD
 *
 * @param	card	pointer to the sound card structure.
 * @param	state	requested state
 *
 * @return              0 on success, -1 otherwise.
 */

static int snd_mxc_audio_dect_suspend(struct platform_device *dev,
		pm_message_t state)
{
	struct snd_card *card = platform_get_drvdata(dev);
	mxc_audio_dect_t *chip = card->private_data;

	//no power managment DECT yet

	//snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);
	snd_pcm_suspend_all(chip->pcm[0]);
	//mxc_alsa_audio_shutdown(chip);

	return 0;
}

/*!
 * This function resumes all suspended streams.
 *
 * TBD
 *
 * @param	card	pointer to the sound card structure.
 * @param	state	requested state
 *
 * @return              0 on success, -1 otherwise.
 */
static int snd_mxc_audio_dect_resume(struct snd_card * card, unsigned int state)
{

	snd_power_change_state(card, SNDRV_CTL_POWER_D0);

	return 0;
}
#endif				/* COMFIG_PM */
#endif

/*!
 * This function frees the sound card structure
 *
 * @param	card	pointer to the sound card structure.
 *
 * @return              0 on success, -1 otherwise.
 */
void snd_mxc_audio_dect_free(struct snd_card * card)
{
	mxc_audio_dect_t *chip;
	pr_debug("Enter");
	chip = card->private_data;
	audio_dect_dma_free(&chip->s[SNDRV_PCM_STREAM_PLAYBACK]);
	audio_dect_dma_free(&chip->s[SNDRV_PCM_STREAM_CAPTURE]);
	mxc_audio_dect = NULL;
	card->private_data = NULL;
	kfree(chip);
	pr_debug("Exit");
}

/*!
 * This function initializes the driver in terms of memory of the soundcard
 * and some basic HW clock settings.
 *
 * @return              0 on success, -1 otherwise.
 */
static int __init mxc_alsa_audio_dect_init(void)
{
	int err;
	int i = 0;
	struct snd_card *card;

	pr_info("Enter. Driver mxc for dect usb audio device, version %s", MXC_DECT_USB_VERSION);

	/* register the soundcard */
	card = snd_card_new(-1, id, THIS_MODULE, sizeof(mxc_audio_dect_t));
	if (card == NULL) {
		pr_error("snd_card_new, no memory") ;
		return -ENOMEM;
	}

	mxc_audio_dect = kcalloc(1, sizeof(*mxc_audio_dect), GFP_KERNEL);
	if (mxc_audio_dect == NULL) {
		pr_error("kalloc, no memory") ;
		return -ENOMEM;
	}

	card->private_data = (void *)mxc_audio_dect;
	card->private_free = snd_mxc_audio_dect_free;

	mxc_audio_dect->card = card;

	/* TODO : how to free this devices ? */
	for(i = 0; i < MXC_ALSA_MAX_PCM_DEV; i++){
		err = snd_card_mxc_audio_dect_pcm(mxc_audio_dect, i);
		if(err < 0){
			pr_error("snd_card_mxc_audio_dect_pcm has failed") ;
	                goto nodev;
		}

	}


	for(i = 0; i < MXC_ALSA_MAX_CAPTURE + MXC_ALSA_MAX_PLAYBACK; i++){
		spin_lock_init(&(mxc_audio_dect->s[i].dma_lock));
	}

	strcpy(card->driver, "DECT");
	strcpy(card->shortname, "DECT-audio");
	sprintf(card->longname, "MXC Freescale with DECT");

	if ((err = snd_card_register(card)) == 0) {
		pr_debug("audio dect support initialized");
		return 0;
	}

nodev:
	snd_card_free(card);
	pr_debug("Exit");
	return err;
}

/*!
 * This function frees the sound driver structure.
 *
 */
static void __exit mxc_alsa_audio_dect_exit(void)
{
	snd_card_free(mxc_audio_dect->card);

	/* SPBA configuration for SSI2 - SDMA is released */
	//spba_rel_ownership(SPBA_SSI2, SPBA_MASTER_C);
}

module_init(mxc_alsa_audio_dect_init);
module_exit(mxc_alsa_audio_dect_exit);

MODULE_AUTHOR("FREESCALE SEMICONDUCTOR");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DECT driver for ALSA");
MODULE_SUPPORTED_DEVICE("{{DECT}}");

module_param(id, charp, 0444);
MODULE_PARM_DESC(id, "ID string for MXC  + DECT soundcard.");
