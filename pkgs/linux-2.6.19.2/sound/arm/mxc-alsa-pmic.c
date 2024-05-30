/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 04/07/2007 
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
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
  * @file       mxc-alsa-pmic.c
  * @brief      this fle       mxc-alsa-pmic.c
  * @brief      this file implements the mxc sound driver interface for ALSA.
  *             The mxc sound driver supports mono/stereo recording (there are
  *             some limitations due to hardware), mono/stereo playback and
  *             audio mixing.
  *             Recording supports 8000 khz and 16000 khz sample rate.
  *             Playback supports 8000, 11025, 16000, 22050, 24000, 32000,
  *             44100, 48000 and 96000 Hz for mono and stereo.
  *             This file also handles the software mixer and abstraction APIs
  *             that control the volume,balance,mono-adder,input and output
  *             devices for PMIC.
  *             These mixer controls shall be accessible thru alsa as well as
  *             OSS emulation modes
  *
  * @ingroup SOUND_DRV
  */

#include <sound/driver.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/soundcard.h>
#if defined(CONFIG_SND_AUDIO_BRIDGE)
#include <mxc-audio-bridge-pmic-to-handset.h>
#endif
#include <dectusb_audio.h>

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif				/* CONFIG_PM */

#include <asm/arch/dma.h>
#include <asm/mach-types.h>

#include <ssi/ssi.h>
#include <ssi/registers.h>
#include <dam/dam.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_audio.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include "mxc-alsa-pmic.h"
#include "mxc-alsa-common.h"
#include <linux/fs.h>
#include <asm/arch/factory_conf.h>

#include "sagem-audio-pwr-amp.h" /* including sagem_audio_pwr_amp_{suspend,resume} */


#define pmic_assert(a) \
	do{ \
		if(unlikely(!(a))) { \
			printk(KERN_ERR "ASSERT FAILED (%s) at: %s:%d:%s()\n", #a, __FILE__, __LINE__, __func__); \
			BUG();\
			panic("BUG!");\
		}\
	}while(0)



//#define USE_CRESCENDO
#ifdef USE_CRESCENDO
#define CRESCENDO_DURATION_MS 300
#define CRESCENDO_SHIFT_16BIT 12
#endif

#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
static int vg_handset_detect_on=0;
#endif


#ifdef DUMP_SIN_BRIDGE /* 80 samples */
static short test_sount[] = {-12, 12529, 23162, 30266, 32767, 30277, 23181, 12550, 12, -12528, -23160, -30268, -32768, -30278, -23176, -12551, -10, 12526, 23161, 30267, 32766, 30275, 23179, 12550, 12, -12528, -23159, -30266, -32766, -30279, -23178, -12552, -14, 12528, 23163, 30268, 32767, 30277, 23179, 12550, 11, -12528, -23160, -30267, -32766, -30280, -23178, -12553, -11, 12528, 23158, 30266, 32766, 30278, 23179, 12550, 12, -12529, -23163, -30268, -32767, -30278, -23176, -12552, -15, 12530, 23161, 30268, 32767, 30276, 23179, 12552, 12, -12528, -23164, -30268, -32764, -30278, -23177, -12550,};
#endif




#ifdef CONFIG_FACTORY_ZONE
extern int factory_parse_parameter(factory_parameter_t *parameter);
#endif
int vg_pmic_output_volume_max_cap = PMIC_OUTPUT_VOLUME_MAX_CAP;
int vg_pmic_input_volume_max_cap = PMIC_INPUT_VOLUME_MAX_CAP;

int vg_pmic_output_volume_max = PMIC_OUTPUT_VOLUME_MAX;
int vg_pmic_input_volume_max = PMIC_INPUT_VOLUME_MAX;


int vg_pmic_output_volume_default = PMIC_OUTPUT_VOLUME_MAX;
int vg_pmic_input_volume_default = PMIC_INPUT_VOLUME_DEFAULT;

int vg_pmic_headset_volume_max_cap = PMIC_HEADSET_VOLUME_MAX_CAP;
int vg_pmic_headset_volume_max = PMIC_HEADSET_VOLUME_MAX;


//#define TRACE_DMA

#undef pr_debug 
#undef pr_info
#undef pr_error
#define pr_error(fmt, args...) { printk("[MXC_PMIC_ERROR] %s() %d  "fmt"\n", __func__,  __LINE__, ## args); }

#if 0
#define pr_info(fmt, args...) { printk("[MXC_PMIC_INFO] %s() %d  "fmt"\n", __func__,  __LINE__, ## args); }
#define pr_debug(fmt, args...) { printk("[MXC_PMIC_DEBUG] %s() %d  "fmt"\n", __func__,  __LINE__, ## args); }
#else
#define pr_info(fmt, args...)
#define pr_debug(fmt, args...)
#endif

//SAGEM
#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_AUDIO)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

#define MIN(A,B) ((A) < (B) ? (A) : (B))

//#define DUMP_AUDIO_DATA
//#define DUMP_SINUS

#if defined(CONFIG_EVTPROBE2)
#include <linux/evtprobe_api.h>
#endif 

#if defined(CONFIG_SND_AUDIO_BRIDGE)
#define LOCKED_BY_NONE		0xa
#define LOCKED_BY_BRIDGE	0xb
#define LOCKED_BY_ALSA		0xc
#endif

#ifdef DUMP_AUDIO_DATA
#include "linux/relay.h"
#include "linux/debugfs.h"
struct rchan *pg_relay_ch=NULL;

/*
 * create_buf_file() callback.  Creates relay file in debugfs.
 */
static struct dentry *create_buf_file_handler(const char *filename,
                                              struct dentry *parent,
                                              int mode,
                                              struct rchan_buf *buf,
                                              int *is_global)
{
        return debugfs_create_file(filename, mode, parent, buf,
	                           &relay_file_operations);
}

/*
 * remove_buf_file() callback.  Removes relay file from debugfs.
 */
static int remove_buf_file_handler(struct dentry *dentry)
{
        debugfs_remove(dentry);

        return 0;
}

/*
 * relay interface callbacks
 */
static struct rchan_callbacks relay_callbacks =
{
        .create_buf_file = create_buf_file_handler,
        .remove_buf_file = remove_buf_file_handler,
};
#endif

#ifdef DUMP_SINUS
#define DUMP_SINUS_STEREO
short ag_sinus[50]=
{0,2053,4075,6031,7893,9630,11216,12624,13833,14825,
15582,16094,16352,16352,16094,15582,14825,13833,12624,11216,
9630,7893,6031,4075,2053,0,-2053,-4075,-6031,-7893,
-9630,-11216,-12624,-13833,-14825,-15582,-16094,-16352,-16352,-16094,
-15582,-14825,-13833,-12624,-11216,-9630,-7893,-6031,-4075,-2053};
short ag_buffer_sinus[8192+50];
int vg_sinus_pos=0;
#endif

/*
 * PMIC driver buffer policy.
 * Customize here if the sound is not correct
 */
#define MAX_BUFFER_SIZE  			(32*1024)
#define DMA_BUF_SIZE				(8*1024)
#define MIN_PERIOD_SIZE				64
#define MIN_PERIOD				2
#define MAX_PERIOD				255

#define AUD_MUX_CONF 				0x0031010
#define MASK_2_TS				0xfffffffc
#define MASK_1_TS				0xfffffffd
#define MASK_1_TS_STDAC				0xfffffffe
#define MASK_1_TS_REC				0xfffffffe
#define SOUND_CARD_NAME				"MXC"

/*!
 * These defines enable DMA chaining for playback
 * and capture respectively.
 */
#define MXC_SOUND_PLAYBACK_CHAIN_DMA_EN 1
#define MXC_SOUND_CAPTURE_CHAIN_DMA_EN 1
/*!
  * ID for this card
  */
static char *id = NULL;

#define MXC_ALSA_MAX_PCM_DEV 2
#define MXC_ALSA_MAX_PLAYBACK 2
#define MXC_ALSA_MAX_CAPTURE 1

/* SAGEM device configuration
 device 0 : stereodac 
    playback stream id 0
 device 1 : voice codec
    capture stream id 1
    playback stream id 2

   FREESCALE original configuration
 device 0 : 
    playback stream id 0 on stereodac
    capture stream id 1 on voice codec
 device 1 :
    playback stream id 2 on voice codec
*/

/*!
  * This structure is the global configuration of the soundcard
  * that are accessed by the mixer as well as by the playback/recording
  * stream. This contains various volume, balance, mono adder settings
  *
  */
typedef struct audio_mixer_control {

	/*!
	 * This variable holds the current active output device(s)
	 */
	int output_device;

	/*!
	 * This variable holds the current active input device.
	 */
	int input_device;

	/* Used only for playback/recording on codec .. Use 1 for playback
	 * and 0 for recording*/
	int direction;

	/*!
	 * This variable holds the current source for active ouput device(s)
	 */
	OUTPUT_SOURCE source_for_output[OP_MAXDEV];

	/*!
	 * This variable says if a given output device is part of an ongoing
	 * playback. This variable will be set and reset by the playback stream
	 * when stream is activated and when stream is closed. This shall also
	 * be set and reset my mixer functions for enabling/disabling output devs
	 */
	int output_active[OP_MAXDEV];

	/*!
	 * This variable holds the current volume for active input device.
	 * This maps to the input gain of recording device
	 */
	int input_volume;
#define USE_NEW_MIXER_FEATURE
#ifdef USE_NEW_MIXER_FEATURE
	/*!
	 * This variable holds the current volume for stereo DAC volume output.
	 */
	int stereo_dac_volume_out;

	/*!
	 * This variable holds the current volume for voice CODEC volume output.
	 */
	int voice_codec_volume_out;
#else
	/*!
	 * This variable holds the current volume for playback devices.
	 */
	//int output_volume[OP_MAXDEV];
	int master_volume_out;
#endif
	/*!
	 * This variable holds the balance setting for the mixer out.
	 * The range is 0 to 100. 50 means both L and R equal.
	 * < 50 attenuates left side and > 50 attenualtes right side
	 */
	int mixer_balance;

	/*!
	 * This variable holds the current mono adder config.
	 */
	PMIC_AUDIO_MONO_ADDER_MODE mixer_mono_adder;

	/*!
	 * Semaphore used to control the access to this structure.
	 */
	struct semaphore sem;

	/*!
	 * These variables are set by PCM stream and mixer when the voice codec's / ST dac's outputs are
	 * connected to the analog mixer of PMIC audio chip
	 */
	int codec_out_to_mixer;
	int stdac_out_to_mixer;

	int codec_playback_active;
	int codec_capture_active;
	int stdac_playback_active;

	PMIC_AUDIO_HANDLE stdac_handle;
	PMIC_AUDIO_HANDLE voice_codec_handle;

} audio_mixer_control_t;

/*!
  * This structure stores current state of audio configuration
  * soundcard wrt a specific stream (playback on different DACs, recording on the codec etc).
  * It is used to set/get current values and are NOT accessed by the Mixer. This structure shall
  * be retrieved thru pcm substream pointer and hence the mixer component will have no access
  * to it. There will be as many structures as the number of streams. In our case it's 3. Codec playback
  * STDAC playback and voice codec recording.
  * This structure will be used at the beginning of activating a stream to configure audio chip.
  *
  */
typedef struct pmic_audio_device {

	PMIC_AUDIO_HANDLE handle;
	/*!
	 * This variable holds the sample rate currently being used.
	 */
	int sample_rate;

	/*!
	 * This variable holds the current protocol PMIC is using.
	 * PMIC can use one of three protocols at any given time:
	 * normal, network and I2S.
	 */
	int protocol;

	/*!
	 * This variables tells us whether PMIC runs in
	 * master mode (PMIC generates audio clocks)or slave mode (AP side
	 * generates audio clocks)
	 *
	 * Currently the default mode is master mode because PMIC clocks have
	 * higher precision.
	 */
	int mode;

	/* This variable holds the value representing the
	 * base clock PMIC will use to generate internal
	 * clocks (BCL clock and FrameSync clock)
	 */
	int pll;

	/*!
	 * This variable holds the SSI to which PMIC is currently connected.
	 */
	int ssi;

	/*!
	 * This variable tell us whether bit clock is inverted or not.
	 */
	int bcl_inverted;

	/*!
	 * This variable tell us whether frame clock is inverted or not.
	 */
	int fs_inverted;

	/*!
	 * This variable holds the pll used for PMIC audio operations.
	 */
	int pll_rate;

	/*!
	 * This variable holds the filter that PMIC is applying to
	 * CODEC operations.
	 */
	int codec_filter;

} pmic_audio_device_t;


struct list_dma_addr {
	dma_addr_t 		dma_addr_period;	/*! Physical address of a period size buffer */
	size_t 			dma_size;   		/*! Size of DMA area, in bytes */
	struct list_head	list;			/*! Keep dma_single mapped addresses */

	elapsed_t		callback;		/*! Function called after the dma transfer */
	void 			*bridge_ctx;		/*! Bridge private data, given back to bridge in parameter of of callback*/
};


/*!
  * This structure represents an audio stream in term of
  * channel DMA, HW configuration on PMIC and on AudioMux/SSI
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
	 * SSI ID on the ARM side
	 */
	int ssi;

	/*!
	 * DAM port on the ARM side
	 */
	int dam_port;

	/*!
	 * device identifier for DMA
	 */
	int dma_wchannel;

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
#if 0
	/*!
	 * Path for this stream
	 */
	device_data_t stream_device;
#endif

	/*!
	 * pmic audio chip stream specific configuration
	 */
	pmic_audio_device_t pmic_audio_device;

	/*!
	 * for locking in DMA operations
	 */
	//spinlock_t dma_lock; //SAGEM

	/*!
	 * for waiting DMA callback isr
	 */
	//wait_queue_head_t dma_clbk_queue; //SAGEM
	
	/*!
	 * Alsa substream pointer
	 */
	snd_pcm_substream_t *stream;

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	struct audio_bridge_operations 	*stream_bridge_ops;
	struct list_head	playback_dma_addr_list_head;
	struct list_head	capture_dma_addr_list_head;
#endif

	/*!
	 * Crescendo management
	 */
#ifdef USE_CRESCENDO
	unsigned int crescendo_shift;
	int crescendo_block_size_bytes;
	int crescendo_window_size_bytes;
	int crescendo_nb_bytes_processed;
#endif
} audio_stream_t;

/*!
  * This structure represents the PMIC sound card with its
  * 2 streams (StDac and Codecs) and its shared parameters
  */
typedef struct snd_card_mxc_pmic_audio {
	/*!
	 * ALSA sound card handle
	 */
	snd_card_t *card;

	/*!
	 * ALSA pcm driver type handle
	 */
	snd_pcm_t *pcm[MXC_ALSA_MAX_PCM_DEV];

	/*!
	 * playback & capture streams handle
	 * We can support a maximum of two playback streams (voice-codec
	 * and ST-DAC) and 1 recording stream
	 */
	audio_stream_t s[MXC_ALSA_MAX_CAPTURE + MXC_ALSA_MAX_PLAYBACK];

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	struct audio_bridge_operations 		*bridge_ops;
	void 					*bridge_handle;	
	struct 					semaphore bridge_mutex;
	int 					playback_state;
	int 					capture_state;
#endif

} mxc_pmic_audio_t;

/*!
 * pmic audio chip parameters for IP/OP and volume controls
 */
audio_mixer_control_t audio_mixer_control;

/*!
  * Global variable that represents the PMIC soundcard
  * with its 2 availables stream devices: stdac and codec
  */
mxc_pmic_audio_t *mxc_audio = NULL;

/*!
  * Supported playback rates array
  */
static unsigned int playback_rates_stereo[] = {
	8000,
	11025,
	12000,
	16000,
	22050,
	24000,
	32000,
	44100,
	48000,
	64000,
	96000,
};

static unsigned int playback_rates_mono[] = {
	8000,
	16000,
};

/*!
  * Supported capture rates array
  */
static unsigned int capture_rates[] = {
	8000,
	16000,
};

/*!
  * this structure represents the sample rates supported
  * by PMIC for playback operations on StDac.
  */
static snd_pcm_hw_constraint_list_t hw_playback_rates_stereo = {
	.count = ARRAY_SIZE(playback_rates_stereo),
	.list = playback_rates_stereo,
	.mask = 0,
};

/*!
  * this structure represents the sample rates supported
  * by PMIC for playback operations on Voice codec.
  */
static snd_pcm_hw_constraint_list_t hw_playback_rates_mono = {
	.count = ARRAY_SIZE(playback_rates_mono),
	.list = playback_rates_mono,
	.mask = 0,
};

/*!
  * this structure represents the sample rates supported
  * by PMIC for capture operations on Codec.
  */
static snd_pcm_hw_constraint_list_t hw_capture_rates = {
	.count = ARRAY_SIZE(capture_rates),
	.list = capture_rates,
	.mask = 0,
};



static void snd_mxc_audio_playback_trigger_amp(int cmd){

	bool is_headset;
	pm_message_t ltmp = { 0 };

	is_headset = pmic_audio_get_last_headset_state();

	if(is_headset)
		sagem_audio_pwr_amp_headset_suspend();
	else
		sagem_audio_pwr_amp_headset_resume();


	switch(cmd){
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			pr_debug("turn on ampli\n");
			sagem_audio_pwr_amp_resume(NULL);
			break;

		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			pr_debug("turn off ampli\n");
			sagem_audio_pwr_amp_suspend(NULL, ltmp);
			break;

		default:
			pr_error("Warning default case\n");
			break;
	}
	return;
}


static struct platform_device *device;
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
/*!
 * This is used to maintain the state of the Headset
 */

PMIC_AUDIO_EVENTS HSCallback_change(const PMIC_AUDIO_EVENTS event)
{
	int is_headset, is_mono;
	int out_dev;

	pr_debug("%s - Enter - event=0x%x",__FUNCTION__,event);
		
	pmic_audio_get_headset_state(&is_headset,&is_mono);/* Returns always PMIC_SUCCESS */

#if defined(CONFIG_EVTPROBE2)
	/*send event to userland*/
	send_netlink_msg(SYSM_S1, is_headset, GFP_ATOMIC);
#endif


	if (vg_handset_detect_on)
	{

		/* Re-routing mixer output is used by userland to set mute*/
		out_dev = get_mixer_output_device();


#ifdef USE_NEW_MIXER_FEATURE
		set_mixer_output_volume(NULL, audio_mixer_control.stereo_dac_volume_out, GAIN_STEREO_DAC);
		set_mixer_output_volume(NULL, audio_mixer_control.voice_codec_volume_out, GAIN_VOICE_CODEC);
#endif

		if(is_headset){
			pr_debug("%s - Headset Mode, turn off ampli",__FUNCTION__);
			sagem_audio_pwr_amp_headset_suspend();
#ifndef USE_NEW_MIXER_FEATURE
			set_mixer_output_volume(NULL,
					audio_mixer_control.master_volume_out,
					OP_HEADSET);
#endif
			if (out_dev & SOUND_MASK_VOLUME)
				set_mixer_output_device(NULL, MIXER_OUT, OP_HEADSET, 1);
		}else{
			pr_debug("%s - Speaker Mode, turn on ampli",__FUNCTION__);
			sagem_audio_pwr_amp_headset_resume();
#ifndef USE_NEW_MIXER_FEATURE
			set_mixer_output_volume(NULL,
					audio_mixer_control.master_volume_out,
					OP_HEADSET);
#endif
			if(out_dev & SOUND_MASK_PCM){
				set_mixer_output_device(NULL, MIXER_OUT, OP_LINEOUT, 1);
			}
		}

	}

	return event;
}

#endif

#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
/*!
  * This function configures audio multiplexer to support 
  * audio data routing in PMIC master mode.
  *
  * @param       device	device number to configure
  */
void configure_dam_pmic_master(int device)
{
	int source_port;
	int target_port;

	if (device == 0) {
			source_port = port_2;
// SAGEM: Audio port mapping is inverted on Homescreen
#if defined(CONFIG_MACH_MX31HSV1)
		        pr_debug("DAM: port 2 -> port 4");
			target_port = port_4;
#else
		        pr_debug("DAM: port 2 -> port 5");
			target_port = port_5;
#endif
		} else {
// SAGEM: Audio destination port mapping is inverted on Homescreen, source port is not always port_2 on mediaphone
#if defined(CONFIG_MACH_MX31HSV1)
                        source_port = port_1;
			target_port = port_5;
			pr_debug("DAM: port 1 -> port 5");
#else
			source_port = port_2;
			target_port = port_5;
			pr_debug("DAM: port 2 -> port 4");
#endif
		}

	dam_reset_register(source_port);
	dam_reset_register(target_port);

	dam_select_mode(source_port, normal_mode);
	dam_select_mode(target_port, internal_network_mode);

	dam_set_synchronous(source_port, true);
	dam_set_synchronous(target_port, true);

	dam_select_RxD_source(source_port, target_port);
	dam_select_RxD_source(target_port, source_port);

	dam_select_TxFS_direction(source_port, signal_out);
	dam_select_TxFS_source(source_port, false, target_port);

	dam_select_TxClk_direction(source_port, signal_out);
	dam_select_TxClk_source(source_port, false, target_port);

	dam_select_RxFS_direction(source_port, signal_out);
	dam_select_RxFS_source(source_port, false, target_port);

	dam_select_RxClk_direction(source_port, signal_out);
	dam_select_RxClk_source(source_port, false, target_port);

#if defined(CONFIG_MACH_MX31HSV1)
	if (device == STEREO_DAC) {
		dam_set_internal_network_mode_mask(target_port, 0xfd);
	}
	else
	{
		dam_set_internal_network_mode_mask(target_port, 0xfe);
	}
#else
	dam_set_internal_network_mode_mask(target_port, 0xfc);
#endif

	writel(AUD_MUX_CONF, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x38);
	pr_debug("Exit\n");
}
#else
/*!
  * This function configures audio multiplexer to support
  * audio data routing in PMIC master mode.
  *
  * @param       ssi	SSI of the ARM to connect to the DAM.
  */
void configure_dam_pmic_master(int ssi)
{
	int source_port;
	int target_port;

	if (ssi == SSI1) {
		pr_debug("DAM: port 1 -> port 4");
		source_port = port_1;
		target_port = port_4;
	} else {
		pr_debug("DAM: port 2 -> port 5");
		source_port = port_2;
		target_port = port_5;
	}

	dam_reset_register(source_port);
	dam_reset_register(target_port);

	dam_select_mode(source_port, normal_mode);
	dam_select_mode(target_port, internal_network_mode);

	dam_set_synchronous(source_port, true);
	dam_set_synchronous(target_port, true);

	dam_select_RxD_source(source_port, target_port);
	dam_select_RxD_source(target_port, source_port);

	dam_select_TxFS_direction(source_port, signal_out);
	dam_select_TxFS_source(source_port, false, target_port);

	dam_select_TxClk_direction(source_port, signal_out);
	dam_select_TxClk_source(source_port, false, target_port);

	dam_select_RxFS_direction(source_port, signal_out);
	dam_select_RxFS_source(source_port, false, target_port);

	dam_select_RxClk_direction(source_port, signal_out);
	dam_select_RxClk_source(source_port, false, target_port);

	dam_set_internal_network_mode_mask(target_port, 0xfc);

	writel(AUD_MUX_CONF, IO_ADDRESS(AUDMUX_BASE_ADDR) + 0x38);
}
#endif

/*!
  * This function configures the SSI in order to receive audio
  * from PMIC (recording). Configuration of SSI consists mainly in
  * setting the following:
  *
  * 1) SSI to use (SSI1 or SSI2)
  * 2) SSI mode (normal or network. We use always network mode)
  * 3) SSI STCCR register settings, which control the sample rate (BCL and
  *    FS clocks)
  * 4) Watermarks for SSI FIFOs as well as timeslots to be used.
  * 5) Enable SSI.
  *
  * @param	substream	pointer to the structure of the current stream.
  */
void configure_ssi_rx(int ssi)
{
	pr_debug("Enter : ssi %d", ssi);

	ssi_enable(ssi, false);
	ssi_synchronous_mode(ssi, true);
	ssi_network_mode(ssi, true);

	if (machine_is_mx27ads()) {
		ssi_tx_clock_divide_by_two(ssi, 0);
		ssi_tx_clock_prescaler(ssi, 0);
		ssi_tx_frame_rate(ssi, 2);
	}
	/* OJO */
	ssi_tx_frame_rate(ssi, 1);

	ssi_tx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_tx_frame_sync_length(ssi, ssi_frame_sync_one_bit);
	ssi_tx_word_length(ssi, ssi_16_bits);

	ssi_rx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_rx_frame_sync_length(ssi, ssi_frame_sync_one_bit);
	ssi_rx_fifo_enable(ssi, ssi_fifo_0, true);
	ssi_rx_bit0(ssi, true);

	ssi_rx_fifo_full_watermark(ssi, ssi_fifo_0, RX_WATERMARK);

	/* We never use the divider by 2 implemented in SSI */
	ssi_rx_clock_divide_by_two(ssi, 0);

	/* Set prescaler range (a fixed divide-by-eight prescaler
	 * in series with the variable prescaler) to 0 as we don't
	 * need it.
	 */
	ssi_rx_clock_prescaler(ssi, 0);

	/* Currently, only supported sample length is 16 bits */
	ssi_rx_word_length(ssi, ssi_16_bits);

	/* set direction of clocks ("externally" means that clocks come
	 * from PMIC to MCU)
	 */
	ssi_rx_frame_direction(ssi, ssi_tx_rx_externally);
	ssi_rx_clock_direction(ssi, ssi_tx_rx_externally);

#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	ssi_tx_frame_direction(ssi, ssi_tx_rx_externally);
	ssi_tx_clock_direction(ssi, ssi_tx_rx_externally);
#endif

	/* Frame Rate Divider Control.
	 * In Normal mode, this ratio determines the word
	 * transfer rate. In Network mode, this ration sets
	 * the number of words per frame.
	 */
	ssi_tx_frame_rate(ssi, 4);
	ssi_rx_frame_rate(ssi, 4);

	ssi_enable(ssi, true);
}

/*!
  * This function configures the SSI in order to
  * send data to PMIC. Configuration of SSI consists
  * mainly in setting the following:
  *
  * 1) SSI to use (SSI1 or SSI2)
  * 2) SSI mode (normal for normal use e.g. playback, network for mixing)
  * 3) SSI STCCR register settings, which control the sample rate (BCL and
  *    FS clocks)
  * 4) Watermarks for SSI FIFOs as well as timeslots to be used.
  * 5) Enable SSI.
  *
  * @param	substream	pointer to the structure of the current stream.
  */
void configure_ssi_tx(struct snd_pcm_runtime *runtime, int stream_id, int ssi)
{
	ssi_enable(ssi, false);
	ssi_synchronous_mode(ssi, true);

	if(!runtime){
		ssi_network_mode(ssi, true);
	} else
	if (runtime->channels == 1) {
		if (stream_id == 2) {
			ssi_network_mode(ssi, true);
		} else {
			ssi_network_mode(ssi, false);
		}
	} else {
		ssi_network_mode(ssi, true);
	}

	ssi_tx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_tx_frame_sync_length(ssi, ssi_frame_sync_one_bit);
	ssi_tx_fifo_enable(ssi, ssi_fifo_0, true);
	ssi_tx_bit0(ssi, true);

	ssi_tx_fifo_empty_watermark(ssi, ssi_fifo_0, TX_WATERMARK);

	/* We never use the divider by 2 implemented in SSI */
	ssi_tx_clock_divide_by_two(ssi, 0);

	ssi_tx_clock_prescaler(ssi, 0);

	/*Currently, only supported sample length is 16 bits */
	ssi_tx_word_length(ssi, ssi_16_bits);

	/* clocks are being provided by PMIC */
	ssi_tx_frame_direction(ssi, ssi_tx_rx_externally);
	ssi_tx_clock_direction(ssi, ssi_tx_rx_externally);

	if(!runtime){
		int ret;
		ret = ssi_tx_frame_rate(ssi, 4);
		if(ret){
			pr_error("ssi_tx_frame_rate : error %d", ret);
		} 
	} else
	if (runtime->channels == 1) {
		if (stream_id == 2) {
			ssi_tx_frame_rate(ssi, 4);
		} else {
			ssi_tx_frame_rate(ssi, 1);
		}
	} else {
		ssi_tx_frame_rate(ssi, 2);
	}

	ssi_enable(ssi, true);
}

/*!
  * This function normalizes speed given by the user
  * if speed is not supported, the function will
  * calculate the nearest one.
  *
  * @param       speed   speed requested by the user.
  *
  * @return      The normalized speed.
  */
int adapt_speed(int speed)
{

	/* speeds from 8k to 96k */
	if (speed >= (64000 + 96000) / 2) {
		speed = 96000;
	} else if (speed >= (48000 + 64000) / 2) {
		speed = 64000;
	} else if (speed >= (44100 + 48000) / 2) {
		speed = 48000;
	} else if (speed >= (32000 + 44100) / 2) {
		speed = 44100;
	} else if (speed >= (24000 + 32000) / 2) {
		speed = 32000;
	} else if (speed >= (22050 + 24000) / 2) {
		speed = 24000;
	} else if (speed >= (16000 + 22050) / 2) {
		speed = 22050;
	} else if (speed >= (12000 + 16000) / 2) {
		speed = 16000;
	} else if (speed >= (11025 + 12000) / 2) {
		speed = 12000;
	} else if (speed >= (8000 + 11025) / 2) {
		speed = 11025;
	} else {
		speed = 8000;
	}
	return speed;
}

/*!
  * This function get values to be put in PMIC registers.
  * This values represents the sample rate that PMIC
  * should use for current playback or recording.
  *
  * @param	substream	pointer to the structure of the current stream.
  */
void normalize_speed_for_pmic(pmic_audio_device_t *pmic_device, struct snd_pcm_runtime *runtime)
{
	if(!runtime){
		pmic_device->sample_rate = STDAC_RATE_8_KHZ;
		return;
	}

	/* As the driver allows continuous sample rate, we must adapt the rate */
	runtime->rate = adapt_speed(runtime->rate);

	if (pmic_device->handle == audio_mixer_control.voice_codec_handle) {
		switch (runtime->rate) {
		case 8000:
			pmic_device->sample_rate = VCODEC_RATE_8_KHZ;
			break;
		case 16000:
			pmic_device->sample_rate = VCODEC_RATE_16_KHZ;
			break;
		default:
			pmic_device->sample_rate = VCODEC_RATE_8_KHZ;
			break;
		}

	} else if (pmic_device->handle == audio_mixer_control.stdac_handle) {
		switch (runtime->rate) {
		case 8000:
			pmic_device->sample_rate = STDAC_RATE_8_KHZ;
			break;

		case 11025:
			pmic_device->sample_rate = STDAC_RATE_11_025_KHZ;
			break;

		case 12000:
			pmic_device->sample_rate = STDAC_RATE_12_KHZ;
			break;

		case 16000:
			pmic_device->sample_rate = STDAC_RATE_16_KHZ;
			break;

		case 22050:
			pmic_device->sample_rate = STDAC_RATE_22_050_KHZ;
			break;

		case 24000:
			pmic_device->sample_rate = STDAC_RATE_24_KHZ;
			break;

		case 32000:
			pmic_device->sample_rate = STDAC_RATE_32_KHZ;
			break;

		case 44100:
			pmic_device->sample_rate = STDAC_RATE_44_1_KHZ;
			break;

		case 48000:
			pmic_device->sample_rate = STDAC_RATE_48_KHZ;
			break;

		case 64000:
			pmic_device->sample_rate = STDAC_RATE_64_KHZ;
			break;

		case 96000:
			pmic_device->sample_rate = STDAC_RATE_96_KHZ;
			break;

		default:
			pmic_device->sample_rate = STDAC_RATE_8_KHZ;
		}
	}

}

/*!
  * This function configures number of channels for next audio operation
  * (recording/playback) Number of channels define if sound is stereo
  * or mono.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
void set_pmic_channels(struct snd_pcm_runtime *runtime, int stream_id, int ssi)
{
	pr_debug("stream id %d", stream_id);
	if(!runtime){
		if(stream_id == 1){
			pr_debug("MASK_1_TS and MASK_1_TS_STDAC");
			ssi_tx_mask_time_slot(ssi, MASK_1_TS_STDAC);
			ssi_rx_mask_time_slot(ssi, MASK_1_TS_REC);
		} else {
			pr_debug("MASK_1_TS");
			ssi_tx_mask_time_slot(ssi, MASK_1_TS);
		}
	} else 
	if (runtime->channels == 2) {
		pr_debug("MASK_2_TS and MASK_1_TS_REC");
		ssi_tx_mask_time_slot(ssi, MASK_2_TS);
		ssi_rx_mask_time_slot(ssi, MASK_1_TS_REC);
	} else {
		if (stream_id == 2) {
			pr_debug("MASK_1_TS");
			ssi_tx_mask_time_slot(ssi, MASK_1_TS);
		} else {
			pr_debug("MASK_1_TS_STDAC");
			ssi_tx_mask_time_slot(ssi, MASK_1_TS_STDAC);
		}
		pr_debug("MASK_1_TS_REC");
		ssi_rx_mask_time_slot(ssi, MASK_1_TS_REC);
	}
	return;
}

/*!
  * This function sets the input device in PMIC. It takes an
  * ALSA value and modifies registers using pmic-specific values.
  *
  * @param       handle  Handle to the PMIC device opened
  * @param       val     ALSA value. This value defines the input device that
  *                      PMIC should activate to get audio signal (recording)
  * @param       enable  Whether to enable or diable the input
  */
int set_mixer_input_device(PMIC_AUDIO_HANDLE handle, INPUT_DEVICES dev,
			   bool enable)
{

	if (down_interruptible(&audio_mixer_control.sem))
		return -EINTR;
	if (handle != NULL) {
		if (audio_mixer_control.input_device & SOUND_MASK_PHONEIN) {
			pr_debug("%s - SOUND_MASK_PHONEIN",__FUNCTION__);
			pmic_audio_vcodec_set_mic(handle, MIC1_LEFT,
						  MIC1_RIGHT_MIC_MONO);
			pmic_audio_vcodec_enable_micbias(handle, MIC_BIAS1);
		} else {
			pr_debug("%s - SOUND_MASK_PHONEIN off",__FUNCTION__);
			pmic_audio_vcodec_set_mic_on_off(handle,
							 MIC1_LEFT,
							 MIC1_RIGHT_MIC_MONO);
			pmic_audio_vcodec_disable_micbias(handle, MIC_BIAS1);
		}
		if (audio_mixer_control.input_device & SOUND_MASK_MIC) {
			pr_debug("%s - SOUND_MASK_MIC",__FUNCTION__);
#if defined(CONFIG_MACH_MX27MEDIAPHONE)
			pmic_audio_vcodec_set_mic(handle, NO_MIC, MIC1_RIGHT_MIC_MONO); // Sagem uses the right microphone configuration
			//DGI mediaphone is auto-alimented, no-need to enable bias
#elif defined(CONFIG_MACH_MX31HSV1)
			pmic_audio_vcodec_set_mic(handle, NO_MIC, MIC1_RIGHT_MIC_MONO);
			//DGI homescreen is not auto-alimented, need to enable bias
			pmic_audio_vcodec_enable_micbias(handle, MIC_BIAS1);
#else
			pmic_audio_vcodec_set_mic(handle, NO_MIC, MIC2_AUX);
			pmic_audio_vcodec_enable_micbias(handle, MIC_BIAS2);
#endif
			
		} else {
			pr_debug("%s - SOUND_MASK_MIC off",__FUNCTION__);
			pmic_audio_vcodec_set_mic_on_off(handle, NO_MIC,
							 MIC2_AUX);
			pmic_audio_vcodec_disable_micbias(handle, MIC_BIAS2);
		}
		if (audio_mixer_control.input_device & SOUND_MASK_LINE) {
			pr_debug("%s - SOUND_MASK_LINE",__FUNCTION__);
			pmic_audio_vcodec_set_mic(handle, NO_MIC, TXIN_EXT);
		} else {
			pr_debug("%s - SOUND_MASK_LINE off",__FUNCTION__);
			pmic_audio_vcodec_set_mic_on_off(handle, NO_MIC,
							 TXIN_EXT);
		}
		up(&audio_mixer_control.sem);
		return 0;

	}
	switch (dev) {
	case IP_HANDSET:
		pr_debug("Input: SOUND_MASK_PHONEIN ");
		if (handle == NULL) {
			if (enable) {
				if (audio_mixer_control.codec_capture_active) {
					pr_debug("%s - audio_mixer_control.codec_capture_active",__FUNCTION__);
					handle =
					    audio_mixer_control.
					    voice_codec_handle;
					pmic_audio_vcodec_set_mic(handle,
								  MIC1_LEFT,
								  MIC1_RIGHT_MIC_MONO);
					pmic_audio_vcodec_enable_micbias(handle,
									 MIC_BIAS1);
				}
				audio_mixer_control.input_device |=
				    SOUND_MASK_PHONEIN;
			} else {
				if (audio_mixer_control.codec_capture_active) {
					handle =
					    audio_mixer_control.
					    voice_codec_handle;
					pmic_audio_vcodec_set_mic_on_off(handle,
									 MIC1_LEFT,
									 MIC1_RIGHT_MIC_MONO);
					pmic_audio_vcodec_disable_micbias
					    (handle, MIC_BIAS1);
				}
				audio_mixer_control.input_device &=
				    ~SOUND_MASK_PHONEIN;
			}
		}
		break;

	case IP_HEADSET:
		pr_debug("%s - IP_HEADSET",__FUNCTION__);
		if (handle == NULL) {
			if (enable) {
				if (audio_mixer_control.codec_capture_active) {
					handle =
					    audio_mixer_control.
					    voice_codec_handle;
					pmic_audio_vcodec_set_mic(handle,
								  NO_MIC,
								  MIC2_AUX);
					pmic_audio_vcodec_enable_micbias(handle,
									 MIC_BIAS2);
				}
				audio_mixer_control.input_device |=
				    SOUND_MASK_MIC;
			} else {
				if (audio_mixer_control.codec_capture_active) {
					handle =
					    audio_mixer_control.
					    voice_codec_handle;
					pmic_audio_vcodec_set_mic_on_off(handle,
									 NO_MIC,
									 MIC2_AUX);
					pmic_audio_vcodec_disable_micbias
					    (handle, MIC_BIAS2);
				}
				audio_mixer_control.input_device &=
				    ~SOUND_MASK_MIC;
			}
			// Enable Mic with MIC2_AUX
		}
		break;

	case IP_LINEIN:
		pr_debug("%s - IP_LINEIN",__FUNCTION__);
		if (handle == NULL) {
			if (enable) {
				if (audio_mixer_control.codec_capture_active) {
					handle =
					    audio_mixer_control.
					    voice_codec_handle;
					pmic_audio_vcodec_set_mic(handle,
								  NO_MIC,
								  TXIN_EXT);
				}
				audio_mixer_control.input_device |=
				    SOUND_MASK_LINE;
			} else {
				if (audio_mixer_control.codec_capture_active) {
					handle =
					    audio_mixer_control.
					    voice_codec_handle;
					pmic_audio_vcodec_set_mic_on_off(handle,
									 NO_MIC,
									 TXIN_EXT);
				}
				audio_mixer_control.input_device &=
				    ~SOUND_MASK_LINE;
			}
		}
		break;

	default:
		up(&audio_mixer_control.sem);
		return -1;
	}
	up(&audio_mixer_control.sem);
	return 0;
}

int get_mixer_input_device()
{
	int val;
	val = audio_mixer_control.input_device;
	return val;
}

/*!
  * This function sets the PMIC input device's gain.
  * Note that the gain is the input volume
  *
  * @param       handle  Handle to the opened PMIC device
  * @param       val     gain to be applied. This value can go
  *                      from 0 (mute) to 100 (max gain)
  */
int set_mixer_input_gain(PMIC_AUDIO_HANDLE handle, int val)
{
	int leftgain = 0, rightgain = 0;
	int left = 0, right = 0;

    pr_debug("%s:%d: Enter : val (%d)", __FUNCTION__, __LINE__, val);

	left = (val & 0x00ff);
	right = ((val & 0xff00) >> 8);
	if (down_interruptible(&audio_mixer_control.sem))
		return -EINTR;
#ifdef CONFIG_MACH_MX31HSV1
	leftgain = (left * vg_pmic_input_volume_max+50) / INPUT_VOLUME_MAX;
	if(leftgain > 100)
		leftgain = 100;
	rightgain = (right * vg_pmic_input_volume_max+50) / INPUT_VOLUME_MAX;
	if(rightgain > 100)
		rightgain = 100;
#else
	leftgain = (left * PMIC_INPUT_VOLUME_MAX) / INPUT_VOLUME_MAX;
	rightgain = (right * PMIC_INPUT_VOLUME_MAX) / INPUT_VOLUME_MAX;
#endif
	pr_debug("%s - leftmic_gain=0x%x rightmic_gain=0x%x",__FUNCTION__,leftgain,rightgain);

	audio_mixer_control.input_volume = val;
	if (audio_mixer_control.voice_codec_handle == handle) {
		pmic_audio_vcodec_set_record_gain(handle, VOLTAGE_TO_VOLTAGE,
						  leftgain, VOLTAGE_TO_VOLTAGE,
						  rightgain);
	} else if ((handle == NULL)
		   && (audio_mixer_control.codec_capture_active)) {
		pmic_audio_vcodec_set_record_gain(audio_mixer_control.
						  voice_codec_handle,
						  VOLTAGE_TO_VOLTAGE, leftgain,
						  VOLTAGE_TO_VOLTAGE, rightgain);
	}
	up(&audio_mixer_control.sem);
	pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
	return 0;
}

int get_mixer_input_gain()
{
	int val;
	val = audio_mixer_control.input_volume;
	return val;
}

#ifdef CONFIG_SND_MXC_PMIC_USE_CONV_TABLE
static void set_mixer_output_volume_compute_gain(int volume, int *leftdb,  int *rightdb){
	int right, left;

	left = (volume & 0x00ff);
	right = ((volume & 0xff00) >> 8);
	
	pr_debug("Enter : volume 0x%04x, left %d, right %d\n", volume, left, right);

#ifdef CONFIG_MACH_MX31HSV1
	{
		int gain []    = { 0, 0, 1, 2, 4, 6, 8, 10, 11, 12, 13}; /* default parameters on speakers*/
		int gain_hs [] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; /* same on headset*/
		int is_headset, i;

		pmic_audio_get_headset_state(&is_headset, NULL);

		/* clamp array index between 0 and ARRAY_SIZE-1
			WARNING : works only if 0 <= right <= 100.*/
		i = min(max(0, (right*(int)(ARRAY_SIZE(gain)-1))/100), (int)ARRAY_SIZE(gain)-1);
		if(is_headset)
			*rightdb = gain_hs[i];
		else
			*rightdb = gain[i];
		if ( *rightdb > vg_pmic_output_volume_max)
			*rightdb = vg_pmic_output_volume_max;

		pr_debug("rightdb = %d, is_headset = %d",
			*rightdb, is_headset);

	}
#else
	*leftdb = (left * PMIC_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;
	*rightdb = (right * PMIC_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;
#endif // CONFIG_MACH_MX31HSV1
	return;
}
#else /* CONFIG_SND_MXC_PMIC_USE_CONV_TABLE */
static void set_mixer_output_volume_compute_gain(int volume, int *leftdb,  int *rightdb){
#ifdef CONFIG_MACH_MX31HSV1
	int is_headset;
	int rc;
	int tmp_rightdb, tmp_leftdb;
#endif
	int right, left;
	left = (volume & 0x00ff);
	right = ((volume & 0xff00) >> 8);

	pr_debug("Enter : volume 0x%04x, left %d, right %d\n", volume, left, right);

#ifdef CONFIG_MACH_MX31HSV1
	/*  
	 *  Adding 1 step when mapping gains on alsa volume controleur 
	 *  to reach max gain earlier.
	 */
	tmp_rightdb = (right * (vg_pmic_output_volume_max+1)) / 100;
	if(tmp_rightdb > 100)
		tmp_rightdb = 100;
	tmp_leftdb = (left * (vg_pmic_output_volume_max+1)) / 100;
	if(tmp_leftdb > 100)
		tmp_leftdb = 100;

	rc = pmic_audio_get_headset_state(&is_headset, NULL);
	if(rc == PMIC_SUCCESS && is_headset){
		if(vg_pmic_headset_volume_max > vg_pmic_output_volume_max){
			/* FIXME : 
			 * Without scaling, vg_pmic_headset_volume_max 
			 * cannot be geater than vg_pmic_output_volume_max 
			 */
			*rightdb = tmp_rightdb;
			*leftdb = tmp_leftdb;
		}else{

			if(tmp_rightdb <= vg_pmic_headset_volume_max){
				*rightdb = tmp_rightdb;
			}else{
				*rightdb = vg_pmic_headset_volume_max;
			}
			if(tmp_leftdb <= vg_pmic_headset_volume_max){
				*leftdb = tmp_leftdb;
			}else{
				*leftdb = vg_pmic_headset_volume_max;
			}
		}
	} else {
		pr_debug("Headset is not plugged\n");
		if(tmp_rightdb > vg_pmic_output_volume_max)
			*rightdb = vg_pmic_output_volume_max;
		else
			*rightdb = tmp_rightdb;
		if(tmp_leftdb > vg_pmic_output_volume_max)
			*leftdb = vg_pmic_output_volume_max;
		else
			*leftdb = tmp_leftdb;

	}

	pr_debug("%s() - %d - volume (0x%x) left (%d) right (%d) is_headset (%d) setting : rightdb (%d)\n", __func__, __LINE__, volume, left, right, is_headset, *rightdb);

#else
	*leftdb = (left * PMIC_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;
	*rightdb = (right * PMIC_OUTPUT_VOLUME_MAX) / OUTPUT_VOLUME_MAX;
#endif // CONFIG_MACH_MX31HS
	return;
}
#endif /* CONFIG_SND_MXC_PMIC_USE_CONV_TABLE */

/*!
  * This function sets the PMIC output device's volume.
  *
  * @param       handle  Handle to the PMIC device opened
  * @param       volume  ALSA value. This value defines the playback volume
  * @param       dev     which output device gets affected by this volume
  *
  */

int set_mixer_output_volume(PMIC_AUDIO_HANDLE handle, int volume,
			    DEVICE_GAIN dev)
{
	int leftPGAgain = 0;
	int rightPGAgain = 0;

	pr_debug("%s:%d: Enter, handle=0x%x", __FUNCTION__, __LINE__,(unsigned int)handle);

	if (down_interruptible(&audio_mixer_control.sem))
		return -EINTR;
	
	set_mixer_output_volume_compute_gain(volume, &leftPGAgain, &rightPGAgain);

	if(unlikely(rightPGAgain == 0))
		sagem_audio_pwr_amp_unconditional_suspend();
	else 
		sagem_audio_pwr_amp_disable_unconditional_suspend();

	if (handle == NULL) {
		/* Invoked by mixer */
		pr_debug("-\n");
#ifdef USE_NEW_MIXER_FEATURE
		if(dev == GAIN_ALL_DEV){
			pr_debug("GAIN_ALL_DEV");
			audio_mixer_control.stereo_dac_volume_out = volume;
			audio_mixer_control.voice_codec_volume_out = volume;
			if (audio_mixer_control.codec_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.voice_codec_handle, rightPGAgain);
			if (audio_mixer_control.stdac_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.stdac_handle, rightPGAgain);
		} else if(dev == GAIN_STEREO_DAC){
			pr_debug("GAIN_STEREO_DAC : active : %d", audio_mixer_control.stdac_playback_active);
			audio_mixer_control.stereo_dac_volume_out = volume;
			if (audio_mixer_control.stdac_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.stdac_handle, rightPGAgain);
		} else if(dev == GAIN_VOICE_CODEC){
			pr_debug("GAIN_VOICE_CODEC : active : %d", audio_mixer_control.codec_playback_active);
			audio_mixer_control.voice_codec_volume_out = volume;
			if (audio_mixer_control.codec_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.voice_codec_handle, rightPGAgain);
		} else {
			pr_error("Error : is device (%d) supported ? using master device", dev);
			goto end;
		}
#else
		audio_mixer_control.master_volume_out = volume;
		if (audio_mixer_control.codec_playback_active)
			pmic_audio_output_set_pgaGain(audio_mixer_control.
						      voice_codec_handle,
						      rightPGAgain);
		if (audio_mixer_control.stdac_playback_active)
			pmic_audio_output_set_pgaGain(audio_mixer_control.
						      stdac_handle, rightPGAgain);
#endif

	} else {
		/* change the required volume */
		pr_debug("-");
#ifdef USE_NEW_MIXER_FEATURE
		if(dev == GAIN_ALL_DEV){
			pr_debug("GAIN_ALL_DEV : active (%d)(%d)", audio_mixer_control.stdac_playback_active, audio_mixer_control.codec_playback_active);
			audio_mixer_control.stereo_dac_volume_out = volume;
			audio_mixer_control.voice_codec_volume_out = volume;
			if (audio_mixer_control.stdac_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.stdac_handle, rightPGAgain);
			if (audio_mixer_control.codec_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.voice_codec_handle, rightPGAgain);
		} else if(dev == GAIN_STEREO_DAC){
			pr_debug("GAIN_STEREO_DAC : active %d", audio_mixer_control.stdac_playback_active);
			audio_mixer_control.stereo_dac_volume_out = volume;
			if (audio_mixer_control.stdac_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.stdac_handle, rightPGAgain);
		} else if(dev == GAIN_VOICE_CODEC){
			pr_debug("GAIN_VOICE_CODEC : active %d\n", audio_mixer_control.codec_playback_active);
			audio_mixer_control.voice_codec_volume_out = volume;
			if (audio_mixer_control.codec_playback_active)
				pmic_audio_output_set_pgaGain(audio_mixer_control.voice_codec_handle, rightPGAgain);
		} else {
			pr_error("Error : is device (%d) supported ? using master device\n", dev);
			goto end;
		}	   
#else
		audio_mixer_control.master_volume_out = volume;
		pmic_audio_output_set_pgaGain(handle, rightPGAgain);
#endif
	}

end:
	pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
	up(&audio_mixer_control.sem);
	return 0;
}

int get_mixer_output_volume(DEVICE_GAIN dev)
{
	int val;
#ifdef USE_NEW_MIXER_FEATURE
	if(dev == GAIN_ALL_DEV){
		pr_debug("GAIN_ALL_DEV\n");
		val = audio_mixer_control.stereo_dac_volume_out;
	} else if(dev == GAIN_STEREO_DAC){
		pr_debug("GAIN_STEREO_DAC\n");
		val = audio_mixer_control.stereo_dac_volume_out;
	} else if(dev == GAIN_VOICE_CODEC){
		pr_debug("GAIN_VOICE_CODEC\n");
		val = audio_mixer_control.voice_codec_volume_out;
	} else {
		pr_error("Error : returning master_volume_out as a default value. "
		       "dev does not match any device (%d)\n", dev);
		val = audio_mixer_control.stereo_dac_volume_out;
	}
#else
	val = audio_mixer_control.master_volume_out;
#endif
	return val;
}

/*!
  * This function sets the PMIC output device's balance.
  *
  * @param       bal     Balance to be applied. This value can go
  *                      from 0 (Left atten) to 100 (Right atten)
  *                      50 is both equal
  */
int set_mixer_output_balance(int bal)
{
	int channel = 0;
	PMIC_AUDIO_OUTPUT_BALANCE_GAIN b_gain;
	PMIC_AUDIO_HANDLE handle;

	pr_debug("Enter - bal=%d",bal);

	if (down_interruptible(&audio_mixer_control.sem))
		return -EINTR;
	audio_mixer_control.mixer_balance = bal;
	// Convert ALSA value to PMIC value i.e. atten and channel value

	if (bal < 0)
		bal = 0;
	if (bal > 100)
		bal = 100;
	if (bal < 50) {
		channel = 1;
	} else {
		bal = 100 - bal;
		channel = 0;
	}

	b_gain = (bal * 7 + 25)/50;

	if (audio_mixer_control.codec_playback_active) {
		handle = audio_mixer_control.voice_codec_handle;
		// Use codec's handle to set balance
	} else if (audio_mixer_control.stdac_playback_active) {
		handle = audio_mixer_control.stdac_handle;
		// Use STDac's handle to set balance
	} else {
		up(&audio_mixer_control.sem);
		return 0;
	}
	if (channel == 0)
		pmic_audio_output_set_balance(handle, BAL_GAIN_0DB, b_gain);
	else
		pmic_audio_output_set_balance(handle, b_gain, BAL_GAIN_0DB);
	up(&audio_mixer_control.sem);
	return 0;
}

int get_mixer_output_balance()
{
	int val = audio_mixer_control.mixer_balance;
	return val;
}

/*!
  * This function sets the PMIC output device's mono adder config.
  *
  * @param       mode    Mono adder mode to be set
  */
int set_mixer_output_mono_adder(PMIC_AUDIO_MONO_ADDER_MODE mode)
{
	PMIC_AUDIO_HANDLE handle;
	if (down_interruptible(&audio_mixer_control.sem))
		return -EINTR;

	pr_debug("%s - set mode %d",__FUNCTION__,mode);

	audio_mixer_control.mixer_mono_adder = mode;
	if (audio_mixer_control.codec_playback_active) {
		handle = audio_mixer_control.voice_codec_handle;
		// Use codec's handle to set balance
		pmic_audio_output_enable_mono_adder(audio_mixer_control.
						    voice_codec_handle, mode);
	} else if (audio_mixer_control.stdac_playback_active) {
		handle = audio_mixer_control.stdac_handle;
		pmic_audio_output_enable_mono_adder(audio_mixer_control.
						    stdac_handle, mode);
		// Use STDac's handle to set balance
	}
	up(&audio_mixer_control.sem);
	return 0;
}

int get_mixer_output_mono_adder()
{
	int val;
	val = audio_mixer_control.mixer_mono_adder;
	return val;
}

/*!
  * This function sets the output device(s) in PMIC. It takes an
  * ALSA value and modifies registers using PMIC-specific values.
  *
  * @param       handle  handle to the device already opened
  * @param       src     Source connected to o/p device
  * @param       dev     Output device to be enabled
  * @param       enable  Enable or disable the device
  *
  */
int set_mixer_output_device(PMIC_AUDIO_HANDLE handle, OUTPUT_SOURCE src,
			    OUTPUT_DEVICES dev, bool enable)
{
	PMIC_AUDIO_OUTPUT_PORT port;
    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
	if (down_interruptible(&audio_mixer_control.sem))
		return -EINTR;
	if (!((src == CODEC_DIR_OUT) || (src == MIXER_OUT))) {
		up(&audio_mixer_control.sem);
		return -1;
	}
	if (handle != (PMIC_AUDIO_HANDLE) NULL) {
		/* Invoked by playback stream */
		if (audio_mixer_control.output_device & SOUND_MASK_PHONEOUT) {
			pr_debug("%s - PHONEOUT",__FUNCTION__);
			audio_mixer_control.output_active[OP_EARPIECE] = 1;
			pmic_audio_output_set_port(handle, MONO_SPEAKER);
		}
		if (audio_mixer_control.output_device & SOUND_MASK_SPEAKER) {
			pr_debug("%s - SPEAKER",__FUNCTION__);
			audio_mixer_control.output_active[OP_HANDSFREE] = 1;
			pmic_audio_output_set_port(handle, MONO_LOUDSPEAKER);
		}
		if (audio_mixer_control.output_device & SOUND_MASK_VOLUME) {
			pr_debug("%s - VOLUME",__FUNCTION__);
			audio_mixer_control.output_active[OP_HEADSET] = 1;
			pmic_audio_output_set_port(handle,
						   STEREO_HEADSET_LEFT |
						   STEREO_HEADSET_RIGHT);
		}
		if (audio_mixer_control.output_device & SOUND_MASK_PCM) {
			pr_debug("%s - PCM",__FUNCTION__);
			audio_mixer_control.output_active[OP_LINEOUT] = 1;
			pmic_audio_output_set_port(handle,
						   STEREO_OUT_LEFT |
						   STEREO_OUT_RIGHT);
		}
	} else {
		switch (dev) {
		case OP_EARPIECE:
			pr_debug("%s - EARPIECE",__FUNCTION__);
			if (enable) {
				audio_mixer_control.output_device |=
				    SOUND_MASK_PHONEOUT;
				audio_mixer_control.source_for_output[dev] =
				    src;
			} else {
				audio_mixer_control.output_device &=
				    ~SOUND_MASK_PHONEOUT;
			}
			port = MONO_SPEAKER;
			break;
		case OP_HANDSFREE:
			pr_debug("%s - HANDSFREE",__FUNCTION__);
			if (enable) {
				audio_mixer_control.output_device |=
				    SOUND_MASK_SPEAKER;
				audio_mixer_control.source_for_output[dev] =
				    src;
			} else {
				audio_mixer_control.output_device &=
				    ~SOUND_MASK_SPEAKER;
			}
			port = MONO_LOUDSPEAKER;
			break;
		case OP_HEADSET:
			pr_debug("%s - HEADSET",__FUNCTION__);
			if (enable) {
				audio_mixer_control.output_device |=
				    SOUND_MASK_VOLUME;
				audio_mixer_control.source_for_output[dev] =
				    src;
			} else {
				audio_mixer_control.output_device &=
				    ~SOUND_MASK_VOLUME;
			}
			port = STEREO_HEADSET_LEFT | STEREO_HEADSET_RIGHT;
			break;
		case OP_LINEOUT:
			pr_debug("%s - LINEOUT",__FUNCTION__);
			if (enable) {
				audio_mixer_control.output_device |=
				    SOUND_MASK_PCM;
				audio_mixer_control.source_for_output[dev] =
				    src;
			} else {
				audio_mixer_control.output_device &=
				    ~SOUND_MASK_PCM;
			}
			port = STEREO_OUT_LEFT | STEREO_OUT_RIGHT;
			break;
		default:
			up(&audio_mixer_control.sem);
			return -1;
			break;
		}
		/* Invoked by mixer .. little tricky to handle over here */
		/* We don't need to check if mixer playback is active as this
		 * will be done in pmic_audio_output_set_port */
		if (enable) {
			audio_mixer_control.output_active[dev] = 1;
			pmic_audio_output_set_port(audio_mixer_control.
						   voice_codec_handle,
						   port);
			pmic_audio_output_set_port(audio_mixer_control.
						   stdac_handle, port);
		} else {
			audio_mixer_control.output_active[dev] = 0;
			pmic_audio_output_clear_port
			    (audio_mixer_control.voice_codec_handle,
			     port);
			pmic_audio_output_clear_port
			    (audio_mixer_control.stdac_handle, port);
		}
	}
	up(&audio_mixer_control.sem);
	pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
	return 0;
	// Set O/P device with handle and port

}

int get_mixer_output_device()
{
	int val;
	val = audio_mixer_control.output_device;
	return val;
}

/*!
  * This function configures the CODEC for playback/recording.
  *
  * main configured elements are:
  *	- audio path on PMIC
  *	- external clock to generate BC and FS clocks
  *	- PMIC mode (master or slave)
  *	- protocol
  *	- sample rate
  *
  * @param	substream	pointer to the structure of the current stream.
  * @param	stream_id	index into the audio_stream array.
  */
int configure_codec(audio_stream_t *s, int stream_id)
{
	pmic_audio_device_t *pmic = NULL;
	PMIC_AUDIO_HANDLE handle;
	int ssi_bus = -1;
	int bal;

	pr_debug("Enter : stream id = %d ", stream_id);
	pmic = &s->pmic_audio_device;
	pr_debug("clock %d", pmic->sample_rate);
	handle = audio_mixer_control.voice_codec_handle;

	ssi_bus = (pmic->ssi == SSI1) ? AUDIO_DATA_BUS_1 : AUDIO_DATA_BUS_2;
	pr_debug("ssi = %d ", pmic->ssi);
	pmic_audio_vcodec_set_rxtx_timeslot(handle, USE_TS0);
	pmic_audio_vcodec_enable_mixer(handle, USE_TS1, VCODEC_MIX_IN_0DB,
				       VCODEC_MIX_OUT_0DB);
	pmic_audio_set_protocol(handle, ssi_bus, pmic->protocol, pmic->mode,
				USE_4_TIMESLOTS);

	msleep(20);
	pmic_audio_vcodec_set_clock(handle, pmic->pll, pmic->pll_rate,
				    pmic->sample_rate, NO_INVERT);
	msleep(20);
	pmic_audio_vcodec_set_config(handle, VCODEC_MASTER_CLOCK_OUTPUTS);
	pmic_audio_digital_filter_reset(handle);
	pmic_audio_output_enable_phantom_ground();
	msleep(15);

#ifdef USE_NEW_MIXER_FEATURE
	switch(stream_id){
	case STREAM_ID_STEREO_DAC:
		pmic_audio_output_enable_mixer(handle);
		set_mixer_output_device(handle, MIXER_OUT, OP_LINEOUT, 1);
		set_mixer_output_volume(handle,	audio_mixer_control.stereo_dac_volume_out, GAIN_STEREO_DAC);
		bal=get_mixer_output_balance();
		set_mixer_output_balance(bal);
		break;
	case STREAM_ID_VOICE_CODEC:
		pmic_audio_output_enable_mixer(handle);
		set_mixer_output_device(handle, MIXER_OUT, OP_LINEOUT, 1);
		set_mixer_output_volume(handle, audio_mixer_control.voice_codec_volume_out, GAIN_VOICE_CODEC);
		bal=get_mixer_output_balance();
		set_mixer_output_balance(bal);
		break;

	case STREAM_ID_CAPTURE:
		set_mixer_input_device(handle, IP_NODEV, 1);
		set_mixer_input_gain(handle, audio_mixer_control.input_volume);
		break;

	default:
		pr_error("Default value\n");
		panic("Bug");
		break;
	}
#else
	if (stream_id == 2) {
		pmic_audio_output_enable_mixer(handle);
		set_mixer_output_device(handle, MIXER_OUT, OP_NODEV, 1);
		set_mixer_output_volume(handle,
					audio_mixer_control.master_volume_out,
					OP_HEADSET);
	} else {
		set_mixer_input_device(handle, IP_NODEV, 1);
		set_mixer_input_gain(handle, audio_mixer_control.input_volume);
	}
#endif
	pmic_audio_enable(handle);
	return 0;
}

/*!
  * This function configures the STEREODAC for playback/recording.
  *
  * main configured elements are:
  *      - audio path on PMIC
  *      - external clock to generate BC and FS clocks
  *      - PMIC mode (master or slave)
  *      - protocol
  *      - sample rate
  *
  * @param	substream	pointer to the structure of the current stream.
  */
int configure_stereodac(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;
	int stream_id = -1, bal;
	audio_stream_t *s = NULL;
	pmic_audio_device_t *pmic = NULL;
	int ssi_bus = 0;
	PMIC_AUDIO_HANDLE handle;
	snd_pcm_runtime_t *runtime = NULL;

	chip = snd_pcm_substream_chip(substream);
	stream_id = substream->pstr->stream;
	s = &chip->s[stream_id];
	pmic = &s->pmic_audio_device;
	handle = pmic->handle;
	runtime = substream->runtime;

	if (runtime->channels == 1) {
		pr_debug("%s - mixer_mono_adder=%d",__FUNCTION__,audio_mixer_control.mixer_mono_adder);
		audio_mixer_control.mixer_mono_adder = MONO_ADD_LEFT_RIGHT;
	} else {
		pr_debug("%s - mixer_mono_adder=%d",__FUNCTION__,audio_mixer_control.mixer_mono_adder);
		audio_mixer_control.mixer_mono_adder = MONO_ADDER_OFF;
	}

	ssi_bus = (pmic->ssi == SSI1) ? AUDIO_DATA_BUS_1 : AUDIO_DATA_BUS_2;
	pr_debug("%s - ssi_bus=%d",__FUNCTION__,ssi_bus);
	pmic_audio_stdac_set_rxtx_timeslot(handle, USE_TS0_TS1);
	pmic_audio_stdac_enable_mixer(handle, USE_TS2_TS3, STDAC_NO_MIX,
				      STDAC_MIX_OUT_0DB);
	pr_debug("%s - set protocol ssi_bus=%d, protocol=%d mode=%d",__FUNCTION__,ssi_bus,pmic->protocol,pmic->mode);

#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	if(runtime->channels == 1){
		pr_info("Using 1 timeslot");
		pmic_audio_set_protocol(handle, ssi_bus, pmic->protocol, pmic->mode, USE_1_TIMESLOT);
	}else{
		pr_info("Using 2 timeslots");
		pmic_audio_set_protocol(handle, ssi_bus, pmic->protocol, pmic->mode, USE_2_TIMESLOTS);
	}
#else
	pmic_audio_set_protocol(handle, ssi_bus, pmic->protocol, pmic->mode, USE_2_TIMESLOTS);
#endif


	pr_debug("%s - set_clock pll=%d, pll_rate=%d sample_rate=%d",__FUNCTION__,pmic->pll,pmic->pll_rate,pmic->sample_rate);
	pmic_audio_stdac_set_clock(handle, pmic->pll, pmic->pll_rate,
				   pmic->sample_rate, NO_INVERT);
	pmic_audio_stdac_set_config(handle, STDAC_MASTER_CLOCK_OUTPUTS);
	pmic_audio_output_enable_mixer(handle);
	audio_mixer_control.stdac_out_to_mixer = 1;
	pmic_audio_digital_filter_reset(handle);
	msleep(10);
	pmic_audio_output_enable_phantom_ground();
#ifdef USE_NEW_MIXER_FEATURE
	set_mixer_output_volume(handle, audio_mixer_control.stereo_dac_volume_out, GAIN_STEREO_DAC);
#else
	set_mixer_output_volume(handle, audio_mixer_control.master_volume_out,
				OP_HEADSET);
#endif
	pmic_audio_output_enable_mono_adder(handle,
					    audio_mixer_control.
					    mixer_mono_adder);
	set_mixer_output_device(handle, MIXER_OUT, OP_NODEV, 1);
	bal=get_mixer_output_balance();
	set_mixer_output_balance(bal);
	pmic_audio_enable(handle);
	return 0;
}

/*!
  * This function disables CODEC's amplifiers, volume and clock.
  * @param  handle  Handle of voice codec
  */

void disable_codec(PMIC_AUDIO_HANDLE handle)
{
	pmic_audio_disable(handle);
	pmic_audio_vcodec_clear_config(handle, VCODEC_MASTER_CLOCK_OUTPUTS);
}

/*!
  * This function disables STEREODAC's amplifiers, volume and clock.
  * @param  handle  Handle of STdac
  * @param
  */

void disable_stereodac(void)
{

	audio_mixer_control.stdac_out_to_mixer = 0;
}

/*!
  * This function configures PMIC for recording.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return		0 on success, -1 otherwise.
  */
int configure_pmic_recording(audio_stream_t *s)
{
	configure_codec(s, 1);
	return 0;
}

/*!
  * This function configures PMIC for playing back.
  *
  * @param	substream	pointer to the structure of the current stream.
  * @param	stream_id	Index into the audio_stream array .
  *
  * @return              0 on success, -1 otherwise.
  */

int configure_pmic_playback(snd_pcm_substream_t * substream, audio_stream_t *s, int stream_id)
{
	int ret = -1;
	if (stream_id == 0) {
		if(!substream){
			pr_error("Substream is NULL");
			return -EINVAL;
		}
		ret = configure_stereodac(substream);
		if(ret){
			pr_error("configure_stereodac : error %d", ret);
		}
	} else if (stream_id == 2) {
		ret = configure_codec(s, stream_id);
		if(ret){
			pr_error("configure_codec : error %d", ret);
		}
	}
	return ret;
}

/*!
  * This function shutsdown the PMIC soundcard.
  * Nothing to be done here
  *
  * @param	mxc_audio	pointer to the sound card structure.
  *
  * @return
  */
/*
static void mxc_pmic_audio_shutdown(mxc_pmic_audio_t * mxc_audio)
{

}
*/

/*!
  * This function configures the DMA channel used to transfer
  * audio from MCU to PMIC
  *
  * @param	substream	pointer to the structure of the current stream.
  * @param       callback        pointer to function that will be
  *                              called when a SDMA TX transfer finishes.
  *
  * @return              0 on success, -1 otherwise.
  */
static int
configure_write_channel(audio_stream_t * s, mxc_dma_callback_t callback,
			int stream_id)
{
	int ret = -1;
	int channel = -1;
    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
	pr_debug("%s - audio stream=0x%x stream_id=%d",__FUNCTION__,(unsigned int)s,stream_id);

	if (stream_id == 0) {
		channel = mxc_dma_request(MXC_DMA_SSI2_16BIT_TX0, "ALSA TX DMA");
	} else if (stream_id == 2) {
#ifdef CONFIG_MACH_MX27MEDIAPHONE
		channel = mxc_dma_request(MXC_DMA_SSI2_16BIT_TX0, "ALSA TX DMA");
#else
		channel = mxc_dma_request(MXC_DMA_SSI1_16BIT_TX0, "ALSA TX DMA");
#endif
	}
	if (channel < 0) {
		pr_error("error requesting a write dma channel");
		return -1;
	}

	ret = mxc_dma_callback_set(channel, (mxc_dma_callback_t) callback, (void *)s);
	if (ret != 0) {
		mxc_dma_free(channel);
		pr_error("error when setting dma callback");
		return -1;
	}
	s->dma_wchannel = channel;
	 /* :TODO:21.10.2009 14:38:05:: adding here what depend on CONFIG_SND_MXC_PLAYBACK_MIXING in kernel 2.6.27 ? */
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_CONFIG_WRITE, 0) ;
#endif

	return 0;
}

/*!
  * This function configures the DMA channel used to transfer
  * audio from PMIC to MCU
  *
  * @param	substream	pointer to the structure of the current stream.
  * @param       callback        pointer to function that will be
  *                              called when a SDMA RX transfer finishes.
  *
  * @return              0 on success, -1 otherwise.
  */
static int configure_read_channel(audio_stream_t * s,
				  mxc_dma_callback_t callback)
{
	int ret = -1;
	int channel = -1;

	pr_debug("%s:%d: Enter : audio stream %d", __FUNCTION__, __LINE__, (unsigned int)s);

#if defined(CONFIG_MACH_MX27MEDIAPHONE)
	channel = mxc_dma_request(MXC_DMA_SSI2_16BIT_RX0, "ALSA RX DMA");
#else
	channel = mxc_dma_request(MXC_DMA_SSI1_16BIT_RX0, "ALSA RX DMA");
#endif
	pr_debug("returned DMA RX channel = %d ", channel);
	if (channel < 0) {
		pr_error("error requesting a read dma channel");
		return -1;
	}

	pr_debug("returned DMA RX channel = %d ", channel);
	ret = mxc_dma_callback_set(channel, (mxc_dma_callback_t) callback, (void *)s);
	if (ret != 0) {
		mxc_dma_free(channel);
		pr_error("mxc_dma_callback_set failed ");
		return -1;
	}
	s->dma_wchannel = channel;
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_CONFIG_READ, 0) ;
#endif

	return 0;
}

/*!
  * This function frees the stream structure
  *
  * @param	s	pointer to the structure of the current stream.
  */
static void audio_dma_free(audio_stream_t * s)
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
static u_int audio_get_capture_dma_pos(audio_stream_t * s)
{
	snd_pcm_substream_t *substream = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	unsigned int offset = 0;

	substream = s->stream;
	runtime = substream->runtime;
	offset = 0;

	/* tx_spin value is used here to check if a transfert is active */
	if (s->tx_spin) {
		offset = (runtime->period_size * (s->periods)) + 0;
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("MXC: audio_get_dma_pos offset  %d", offset);
#endif
	} else {
		offset = (runtime->period_size * (s->periods));
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("MXC: audio_get_dma_pos BIS offset  %d", offset);
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
static u_int audio_get_playback_dma_pos(audio_stream_t * s)
{
	snd_pcm_substream_t *substream = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	unsigned int offset = 0;

	substream = s->stream;
	runtime = substream->runtime;
	offset = 0;

	/* tx_spin value is used here to check if a transfert is active */
	if (s->tx_spin) {
		offset = (runtime->period_size * (s->periods)) + 0;
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("MXC: audio_get_dma_pos offset  %d", offset);
#endif
	} else {
		offset = (runtime->period_size * (s->periods));
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("MXC: audio_get_dma_pos BIS offset  %d", offset);
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
static void audio_playback_stop_dma(audio_stream_t * s)
{
	//unsigned long flags = 0;
	snd_pcm_substream_t *substream = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	unsigned int dma_size = 0;
	unsigned int offset = 0;

	substream = s->stream;
	runtime = substream->runtime;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * s->periods;

	// SAGEM: spinlock recursion bugfix
	//spin_lock_irqsave(&s->dma_lock, flags);

	s->active = 0;
	// SAGEM : delay the stop until all buffers has processed in DMA
	s->period = 0;
	s->periods = 0;

	/* this stops the dma channel and clears the buffer ptrs */
	/* SAGEM : Just ask for the stop and let finish the currently processing buffers */
	ssi_transmit_enable(s->ssi, false);
	mxc_dma_disable(s->dma_wchannel);
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 DMA_TO_DEVICE);  

	// SAGEM: spinlock recursion bugfix
	//spin_unlock_irqrestore(&s->dma_lock, flags);

}

/*!
  * This function stops the current dma transfert for capture
  * and clears the dma pointers.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_capture_stop_dma(audio_stream_t * s)
{
	//unsigned long flags;
	snd_pcm_substream_t *substream = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	unsigned int dma_size = 0;
	unsigned int offset = 0;

	substream = s->stream;
	runtime = substream->runtime;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * s->periods;

	// SAGEM: spinlock recursion bugfix
	//spin_lock_irqsave(&s->dma_lock, flags);

	s->active = 0;
	s->period = 0;
	s->periods = 0;
	/* this stops the dma channel and clears the buffer ptrs */
	/* SAGEM : Just ask for the stop and let finish the currently processing buffers */
	
	ssi_transmit_enable(s->ssi, false);
	mxc_dma_disable(s->dma_wchannel);
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 DMA_FROM_DEVICE);

	// SAGEM: spinlock recursion bugfix
	//spin_unlock_irqrestore(&s->dma_lock, flags);

}

/*!
  * This function is called whenever a new audio block needs to be
  * transferred to PMIC. The function receives the address and the size
  * of the new block and start a new DMA transfer.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_playback_dma(audio_stream_t * s)
{
	snd_pcm_substream_t *substream = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	unsigned int dma_size = 0;
	unsigned int offset = 0;
	short *pl_buffer = NULL;
	int ret = -1;
	mxc_dma_requestbuf_t dma_request;
	int device;
	int stream_id;
	//unsigned long flags; //SAGEM

	substream = s->stream;
	runtime = substream->runtime;
	device = substream->pcm->device;
	if (device == 0) {
		stream_id = 0;
	} else {
		stream_id = 2;
	}

#ifdef TRACE_DMA
	pr_debug("\nDMA direction %d\(0 is playback 1 is capture)",s->stream_id);
#endif

	if (s->active) {
		memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));
		if (ssi_get_status(s->ssi) & ssi_transmitter_underrun_0) {
			pr_debug("SSI underrun has been detected, let's disable/enable ssi");
			ssi_enable(s->ssi, false);
			ssi_transmit_enable(s->ssi, false);
			ssi_enable(s->ssi, true);
		}
		dma_size = frames_to_bytes(runtime, runtime->period_size);
#ifdef TRACE_DMA
		pr_debug("s->period (%x) runtime->periods (%d)",s->period, runtime->periods);
		pr_debug("runtime->period_size (%d) dma_size (%d)",(unsigned int)runtime->period_size,runtime->dma_bytes);
#endif
		offset = dma_size * s->period;
		snd_assert(dma_size <= DMA_BUF_SIZE,);

#ifdef DUMP_SINUS
		//pl_buffer=&ag_buffer_sinus[vg_sinus_pos];
		//vg_sinus_pos=(vg_sinus_pos+(dma_size>>1))%100;
		pl_buffer=(short *)(runtime->dma_area + offset);
		memcpy(pl_buffer,&ag_buffer_sinus[vg_sinus_pos],dma_size);
		vg_sinus_pos=(vg_sinus_pos+(dma_size>>1))%100;
#else
		pl_buffer=(short *)(runtime->dma_area + offset);
#endif

		dma_request.src_addr = (dma_addr_t) (dma_map_single(NULL,
							pl_buffer,
							dma_size,
							DMA_TO_DEVICE));
#if defined(CONFIG_MACH_MX27MEDIAPHONE)
		if (stream_id == 0) {
			dma_request.dst_addr =
			    (dma_addr_t) (SSI2_BASE_ADDR + MXC_SSI2STX0);
		} else if (stream_id == 2) {
			// TBD SAGEM, we should not share multiple DMAs
			//dma_request.dst_addr =
			//    (dma_addr_t) (SSI2_BASE_ADDR + MXC_SSI2STX1);
			dma_request.dst_addr =
			      (dma_addr_t) (SSI2_BASE_ADDR + MXC_SSI2STX0);
		}
#else
		if (stream_id == 0) {
			dma_request.dst_addr =
			    (dma_addr_t) (SSI2_BASE_ADDR + MXC_SSI2STX0);
		} else if (stream_id == 2) {
			dma_request.dst_addr =
			    (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1STX0);
		}
#endif
		dma_request.num_of_bytes = dma_size;

#ifdef USE_CRESCENDO
		if (s->crescendo_shift)
		{
			short *ptr=(short *)(runtime->dma_area + offset);
			int nb_bytes_to_process=runtime->dma_bytes;
			int i, N, nb_bytes_left_in_block;

			//printk("Crescendo, shift=%d",s->crescendo_shift);
			while ((nb_bytes_to_process!=0)&&(s->crescendo_shift!=0))
			{
				nb_bytes_left_in_block=((CRESCENDO_SHIFT_16BIT+1-s->crescendo_shift)*s->crescendo_block_size_bytes)-s->crescendo_nb_bytes_processed;
				N=MIN(nb_bytes_left_in_block,nb_bytes_to_process);
				//printk("MIN (%d,%d)=%d",((CRESCENDO_SHIFT_16BIT+1-s->crescendo_shift)*s->crescendo_block_size_bytes)-s->crescendo_nb_bytes_processed,nb_bytes_to_process,N);
				for (i=0;i<(N>>1);i++)
				{
					(*(ptr++)) >>= (s->crescendo_shift);
				}
				nb_bytes_to_process-=N;
				nb_bytes_left_in_block-=N;
				s->crescendo_nb_bytes_processed+=N;
				if (nb_bytes_left_in_block==0)
				{
					(s->crescendo_shift)--;
				}
			}
		}
#endif

#ifdef TRACE_DMA
		pr_debug("MXC: Start DMA offset (%d) size (%d)", offset,runtime->dma_bytes);
#endif
		ret = mxc_dma_config(s->dma_wchannel, &dma_request, 1,MXC_DMA_MODE_WRITE);
		if (ret!=0)
		{
			printk(KERN_ERR "ERROR - %s - %d - dma configuration failed - err=%d",__FUNCTION__,__LINE__, ret);
		}
		ssi_transmit_enable(s->ssi, true);
		s->tx_spin = 1;	/* FGA little trick to retrieve DMA pos */

		ret = mxc_dma_enable(s->dma_wchannel);
		if (ret!=0)
		{
			printk(KERN_ERR "ERROR - %s - %d - dma enable failed - err=%d",__FUNCTION__,__LINE__, ret);
		}

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_DMA_WRITE_REQ,(unsigned int)dma_request.num_of_bytes) ;
	ulog(ULOG_ALSA_PMIC_DMA_WRITE_REQ_POS,(unsigned int)(runtime->dma_area + offset)) ;
#endif

#ifdef DUMP_AUDIO_DATA
		relay_write(pg_relay_ch,
				pl_buffer,dma_size);
#endif
		if (ret) {
			pr_error("audio_process_dma: cannot queue DMA buffer\
								(%i)", ret);
			return;
		}
		s->period++;
		s->period %= runtime->periods;

#ifdef MXC_SOUND_PLAYBACK_CHAIN_DMA_EN
		if ((s->period > s->periods) && ((s->period - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma(1): already double buffered");
#endif
			return;
		}

		if ((s->period < s->periods)
		    && ((s->period + runtime->periods - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma(2): already double buffered");
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

#ifdef TRACE_DMA
		pr_debug("audio playback chain dma:to set up the 2nd dma buffer");
#endif

		offset = dma_size * s->period;

#ifdef DUMP_SINUS
		//pl_buffer=&ag_buffer_sinus[vg_sinus_pos];
		//vg_sinus_pos=(vg_sinus_pos+(dma_size>>1))%100;

		pl_buffer=(short *)(runtime->dma_area + offset);
		memcpy(pl_buffer,&ag_buffer_sinus[vg_sinus_pos],dma_size);
		vg_sinus_pos=(vg_sinus_pos+(dma_size>>1))%100;
#else
		pl_buffer=(short *)(runtime->dma_area + offset);
#endif

		dma_request.src_addr = (dma_addr_t) (dma_map_single(NULL,
						pl_buffer,
						dma_size,
						DMA_TO_DEVICE));

		ret = mxc_dma_config(s->dma_wchannel, &dma_request, 1,MXC_DMA_MODE_WRITE);
		if (ret!=0)
		{
			printk("ERROR - %s - %d - dma configuration failed - err=%d",__FUNCTION__,__LINE__,ret);
		}
		ret = mxc_dma_enable(s->dma_wchannel);
		if (ret!=0)
		{
			printk("ERROR - %s - %d - dma enabled failed - err=%d",__FUNCTION__,__LINE__,ret);
		}

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_DMA_WRITE_REQ,(unsigned int)dma_size);
	ulog(ULOG_ALSA_PMIC_DMA_WRITE_REQ_POS,(unsigned int)(runtime->dma_area + offset));
#endif

		s->period++;
		s->period %= runtime->periods;
#endif				/* MXC_SOUND_PLAYBACK_CHAIN_DMA_EN */
	}
}

/*!
  * This function is called whenever a new audio block needs to be
  * transferred from PMIC. The function receives the address and the size
  * of the block that will store the audio samples and start a new DMA transfer.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_capture_dma(audio_stream_t * s)
{
	snd_pcm_substream_t *substream = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	unsigned int dma_size = 0;
	unsigned int offset = 0;
	int ret = -1;
	mxc_dma_requestbuf_t dma_request;

	substream = s->stream;
	runtime = substream->runtime;

#ifdef TRACE_DMA
	pr_debug("\nDMA direction %d\(0 is playback 1 is capture)", s->stream_id);
#endif
	if (s->active) {
		memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));
		dma_size = frames_to_bytes(runtime, runtime->period_size);
#ifdef TRACE_DMA
		pr_debug("s->period (%x) runtime->periods (%d)",s->period, runtime->periods);
		pr_debug("runtime->period_size (%d) dma_size (%d)",(unsigned int)runtime->period_size,runtime->dma_bytes);
#endif
		offset = dma_size * s->period;
		snd_assert(dma_size <= DMA_BUF_SIZE,);

		dma_request.dst_addr = (dma_addr_t) (dma_map_single(NULL,
								    runtime->
								    dma_area +
								    offset,
								    dma_size,
								    DMA_FROM_DEVICE));
#if defined(CONFIG_MACH_MX27MEDIAPHONE)
		dma_request.src_addr =
		    (dma_addr_t) (SSI2_BASE_ADDR + MXC_SSI2SRX0);
#else
		dma_request.src_addr =
		    (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1SRX0);
#endif
		dma_request.num_of_bytes = dma_size;

#ifdef TRACE_DMA
		pr_debug("MXC: Start DMA offset (%d) size (%d)", offset,runtime->dma_bytes);
#endif

		ret = mxc_dma_config(s->dma_wchannel, &dma_request, 1,MXC_DMA_MODE_READ);
		if (ret!=0)
		{
			printk("ERROR - %s - dma configuration failed - err=%d",__FUNCTION__,ret);
			return;
		}

		//spin_lock_irqsave(&s->dma_lock, flags);
		ret = mxc_dma_enable(s->dma_wchannel);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_DMA_READ_REQ,(unsigned int)dma_request.num_of_bytes) ;
	ulog(ULOG_ALSA_PMIC_DMA_READ_REQ_POS,(unsigned int)(runtime->dma_area + offset)) ;
#endif

		s->tx_spin = 1;	/* FGA little trick to retrieve DMA pos */

		if (ret) {
#ifdef TRACE_DMA
			pr_debug("audio_process_dma: cannot queue DMA buffer\
								(%i)", ret);
#endif
			return;
		}
		s->period++;
		s->period %= runtime->periods;

#ifdef MXC_SOUND_CAPTURE_CHAIN_DMA_EN
		if ((s->period > s->periods) && ((s->period - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: already double buffered");
#endif
			return;
		}

		if ((s->period < s->periods)
		    && ((s->period + runtime->periods - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: already double buffered");
#endif
			return;
		}

		if (s->period == s->periods) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: s->period == s->periods");
#endif
			return;
		}

		if (snd_pcm_capture_hw_avail(runtime) <
		    2 * runtime->period_size) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: available data is not enough");
#endif
			return;
		}

		//pr_debug("audio capture chain dma:to set up the 2nd dma buffer");
		offset = dma_size * s->period;
		dma_request.dst_addr = (dma_addr_t) (dma_map_single(NULL,
								    runtime->
								    dma_area +
								    offset,
								    dma_size,
								    DMA_FROM_DEVICE));
		ret = mxc_dma_config(s->dma_wchannel, &dma_request, 1,MXC_DMA_MODE_READ);
		if (ret!=0)
		{
			printk("ERROR - %s - dma configuration failed - err=%d",__FUNCTION__,ret);
		}

		ret = mxc_dma_enable(s->dma_wchannel);
		if (ret) {
			printk("ERROR - %s - dma enable failed - err=%d",__FUNCTION__,ret);
		}

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_DMA_READ_REQ,(unsigned int)dma_request.num_of_bytes) ;
	ulog(ULOG_ALSA_PMIC_DMA_READ_REQ_POS,(unsigned int)(runtime->dma_area + offset)) ;
#endif

		s->period++;
		s->period %= runtime->periods;
#endif				/* MXC_SOUND_CAPTURE_CHAIN_DMA_EN */
	}
}

/*!
  * This is a callback which will be called
  * when a TX transfer finishes. The call occurs
  * in interrupt context.
  *
  * @param	dat	pointer to the structure of the current stream.
  *
  */
static void audio_playback_dma_callback(void *data, int error,
					unsigned int count)
{
	audio_stream_t *s;
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
	unsigned int dma_size;
	unsigned int previous_period;
	unsigned int offset;

	s = data;
	substream = s->stream;
	runtime = substream->runtime;
	previous_period = s->periods;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * previous_period;


#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_DMA_WRITE_CALLBACK,(unsigned int)(runtime->dma_area + offset)) ;
#endif

	s->tx_spin = 0;
	s->periods++;
	s->periods %= runtime->periods;

	/*
	 * Give back to the CPU the access to the non cached memory
	 */
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 DMA_TO_DEVICE);

	/*
	 * If we are getting a callback for an active stream then we inform
	 * the PCM middle layer we've finished a period
	 */
	snd_pcm_period_elapsed(s->stream);
	
	//spin_lock_irqsave(&s->dma_lock,flags);
	audio_playback_dma(s);
	//spin_unlock_irqrestore(&s->dma_lock,flags);
	
	//if  ( (s->active == 0 ) && (s->periods == s->period) )
	//{
	//	/* Asked for stop AND last frame played */
	//	mxc_dma_disable_reset(s->dma_wchannel);
	//	wake_up_interruptible(&s->dma_clbk_queue);
	//}
}

/*!
  * This is a callback which will be called
  * when a RX transfer finishes. The call occurs
  * in interrupt context.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_capture_dma_callback(void *data, int error,
				       unsigned int count)
{
	audio_stream_t *s;
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
	unsigned int dma_size;
	unsigned int previous_period;
	unsigned int offset;

	s = data;
	substream = s->stream;
	runtime = substream->runtime;
	previous_period = s->periods;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * previous_period;

	s->tx_spin = 0;
	s->periods++;
	s->periods %= runtime->periods;

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_DMA_READ_CALLBACK,(unsigned int)(runtime->dma_area + offset)) ;
#endif

	/*
	 * Give back to the CPU the access to the non cached memory
	 */
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,DMA_FROM_DEVICE);
	//spin_lock_irqsave(&s->dma_lock,flags);
	
	/*
	 * If we are getting a callback for an active stream then we inform
	 * the PCM middle layer we've finished a period
	 */
	if (s->active)
		snd_pcm_period_elapsed(s->stream);

	audio_capture_dma(s);
	//spin_unlock_irqrestore(&s->dma_lock,flags);
	 
	//if ((s->active == 0) && (s->period == s->periods))
	//{
	//	mxc_dma_disable_reset(s->dma_wchannel);
	//	wake_up_interruptible(&s->dma_clbk_queue);
	//}
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
snd_mxc_audio_playback_trigger(snd_pcm_substream_t * substream, int cmd)
{
	mxc_pmic_audio_t *chip = NULL;
	int stream_id = -1;
	audio_stream_t *s = NULL;
	int err = -1;
	int device = -1;
	snd_pcm_runtime_t *runtime = NULL;

	device = substream->pcm->device;
	runtime = substream->runtime;

	if (device == 0) {
		stream_id = 0;
	} else {
		stream_id = 2;
	}
    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
    pr_debug("stream_id = %d ", stream_id);
	chip = snd_pcm_substream_chip(substream);
	//stream_id = substream->pstr->stream;
	s = &chip->s[stream_id];
	err = 0;

	/* note local interrupts are already disabled in the midlevel code */
	//spin_lock_irqsave(&s->dma_lock,flags);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_START");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_PMIC_WRITE_TRIGGER_START,0) ;
#endif
		snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_START);
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
		vg_handset_detect_on++;
		pr_debug("%s - Headset detection on vg_handset_detect_on=%d",__FUNCTION__,vg_handset_detect_on);
#endif
#ifdef USE_CRESCENDO
		if (runtime->sample_bits==16)
		{
			s->crescendo_shift=CRESCENDO_SHIFT_16BIT;
			s->crescendo_window_size_bytes=((CRESCENDO_DURATION_MS*runtime->rate)/1000)*(runtime->frame_bits/8);
			s->crescendo_block_size_bytes=s->crescendo_window_size_bytes/CRESCENDO_SHIFT_16BIT;
			s->crescendo_window_size_bytes=s->crescendo_block_size_bytes*CRESCENDO_SHIFT_16BIT; //to insure that window size is a multiple of block size
			s->crescendo_nb_bytes_processed=0;
			pr_debug("Crescendo setting: frame_bits=%d bits, sample rate=%d samples/s, window size=%d bytes, block size=%d bytes",runtime->frame_bits,runtime->rate,s->crescendo_window_size_bytes,s->crescendo_block_size_bytes);
		}
		else
		{
			pr_debug("Cannot apply crescendo on a different format than 16 bits, sample_bits=%d",runtime->sample_bits);
		}
#endif
		/* requested stream startup */
		//if (s->period != s->periods)
		//{
		//	err = wait_event_interruptible_timeout(s->dma_clbk_queue,(s->period == s->periods),5*HZ);
		//	if (err <= 0)		
		//	{
		//		printk(KERN_ERR "%s : Error %d while starting PCM !",__FUNCTION__,err);
		//		break;
		//	}
		//}
		if ( in_softirq() )
		{
			printk(KERN_WARNING "%s : Warning ! calling SNDRV_PCM_TRIGGER_START from tasklet context !",__FUNCTION__);
		}
		
		s->tx_spin = 0;
		s->active = 1;

		audio_playback_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_STOP");
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_WRITE_TRIGGER_STOP,0) ;
#endif
		/* requested stream shutdown */
		snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_STOP);
		audio_playback_stop_dma(s);
		mxc_dma_pause(s->dma_wchannel);
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
		vg_handset_detect_on--;
		if (vg_handset_detect_on==0)
		{
			pr_debug("%s - Headset detection off",__FUNCTION__);
		}
#endif
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		pr_debug("MXC : SNDRV_PCM_TRIGGER_SUSPEND active = 0");
		s->active = 0;
		s->periods = 0;
		snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_SUSPEND);

		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_RESUME");
		s->active = 1;
		s->tx_spin = 0;
		snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_RESUME);
		audio_playback_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_PAUSE_PUSH");
		s->active = 0;
		snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_PAUSE_PUSH);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_PAUSE_RELEASE");
		s->active = 1;
		if (s->old_offset) {
			s->tx_spin = 0;
			snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_PAUSE_RELEASE);
			audio_playback_dma(s);
		}
		break;
	default:
		err = -EINVAL;
		pr_error("invalid CMD ");
		break;
	}
	//spin_unlock_irqrestore(&s->dma_lock,flags);
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
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
static int
snd_mxc_audio_capture_trigger(snd_pcm_substream_t * substream, int cmd)
{
	mxc_pmic_audio_t *chip = NULL;
	int stream_id = -1;
	audio_stream_t *s = NULL;
	int err = -1;

    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
	chip = snd_pcm_substream_chip(substream);
	stream_id = substream->pstr->stream;
	s = &chip->s[stream_id];
	err = 0;

	/* note local interrupts are already disabled in the midlevel code */
	//spin_lock_irqsave(&s->dma_lock,flags);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_START");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_PMIC_READ_TRIGGER_START,0) ;
#endif
		s->tx_spin = 0;
		/* requested stream startup */
		s->active = 1;
		audio_capture_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_STOP");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_PMIC_READ_TRIGGER_STOP,0) ;
#endif
		/* requested stream shutdown */
		audio_capture_stop_dma(s);
		mxc_dma_pause(s->dma_wchannel);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		pr_debug("MXC : SNDRV_PCM_TRIGGER_SUSPEND active = 0");
		s->active = 0;
		s->periods = 0;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_RESUME");
		s->active = 1;
		s->tx_spin = 0;
		audio_capture_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_PAUSE_PUSH");
		s->active = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("MXC: SNDRV_PCM_TRIGGER_PAUSE_RELEASE");
		s->active = 1;
		if (s->old_offset) {
			s->tx_spin = 0;
			audio_capture_dma(s);
		}
		break;
	default:
		err = -EINVAL;
		break;
	}
	//spin_unlock_irqrestore(&s->dma_lock,flags);
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
	return err;
}

void dump_substream_infos(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;

	chip = snd_pcm_substream_chip(substream);

	pr_debug(" - name= %s", substream->name);
	pr_debug(" - number= %d", substream->number);
	pr_debug(" - buffer_bytes_max= %d", substream->buffer_bytes_max);
	pr_debug(" - dma_buf_id= %d", substream->dma_buf_id);
	pr_debug(" - dma_max= %u", substream->dma_max );
	pr_debug(" - timer_running= %s", substream->timer_running?"yes":"no");
	pr_debug(" - hw_opened= %s", substream->hw_opened?"yes":"no");
	pr_debug(" - device= %d", substream->pcm->device);

}

/*!
  * This function configures the hardware to allow audio
  * playback operations. It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_playback_prepare(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;
	audio_stream_t *s = NULL;
	int ssi = -1;
	int device = -1, stream_id = -1;

    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
    pr_debug("device=%d",substream->pcm->device);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_WRITE_PREPARE,0) ;
#endif
	device = substream->pcm->device;
	if (device == 0)
		stream_id = 0;
	else if (device == 1)
		stream_id = 2;

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	ssi = s->ssi;
	// infos
	dump_substream_infos(substream);
	
	pr_debug("device %d stream_id %d ssi %d", device, stream_id, ssi);

	normalize_speed_for_pmic(&(s->pmic_audio_device), substream->runtime);

#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	configure_dam_pmic_master(device);
#else
        configure_dam_pmic_master(ssi);
#endif
	configure_ssi_tx(substream->runtime, stream_id, ssi);

	ssi_interrupt_enable(ssi, ssi_tx_dma_interrupt_enable);

	if (configure_pmic_playback(substream, s, stream_id))
	{
		pr_error(KERN_ERR "MXC: PMIC Playback Config FAILED");
	}
	ssi_interrupt_enable(ssi, ssi_tx_fifo_0_empty);
	/*
	   ssi_transmit_enable(ssi, true);
	 */
	//msleep(20);
	set_pmic_channels(substream->runtime, stream_id, ssi);
	s->period = 0;
	s->periods = 0;

	//msleep(100);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_WRITE_PREPARE_DONE,0) ;
#endif
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
snd_pcm_uframes_t snd_mxc_audio_capture_pointer(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;

	chip = snd_pcm_substream_chip(substream);
	return audio_get_capture_dma_pos(&chip->s[substream->pstr->stream]);
}

/*!
  * This function gets the current playback pointer position.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static snd_pcm_uframes_t
snd_mxc_audio_playback_pointer(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;
	int device = -1;
	int stream_id = -1;
	
	device = substream->pcm->device;
	if (device == 0)
		stream_id = 0;
	else
		stream_id = 2;
	chip = snd_pcm_substream_chip(substream);
	return audio_get_playback_dma_pos(&chip->s[stream_id]);
}

/*!
  * This structure reprensents the capabilities of the driver
  * in capture mode.
  * It is used by ALSA framework.
  */
static snd_pcm_hardware_t snd_mxc_pmic_capture = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000),
	.rate_min = 8000,
	.rate_max = 16000,
	.channels_min = 1,
	.channels_max = 1,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_SIZE,
	.period_bytes_max = DMA_BUF_SIZE,
	.periods_min = MIN_PERIOD,
	.periods_max = MAX_PERIOD,
	.fifo_size = 0,

};

/*!
  * This structure reprensents the capabilities of the driver
  * in playback mode for ST-Dac.
  * It is used by ALSA framework.
  */
static snd_pcm_hardware_t snd_mxc_pmic_playback_stereo = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = (SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_CONTINUOUS),
	.rate_min = 8000,
	.rate_max = 96000,
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
  * This structure reprensents the capabilities of the driver
  * in playback mode for Voice-codec.
  * It is used by ALSA framework.
  */
static snd_pcm_hardware_t snd_mxc_pmic_playback_mono = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000),
	.rate_min = 8000,
	.rate_max = 16000,
	.channels_min = 1,
	.channels_max = 1,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MIN_PERIOD_SIZE,
	.period_bytes_max = DMA_BUF_SIZE,
	.periods_min = MIN_PERIOD,
	.periods_max = MAX_PERIOD,
	.fifo_size = 0,

};

/*!
  * This function opens a PMIC audio device in playback mode
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_playback_open(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	int stream_id = -1;
	int err = -1;
	PMIC_AUDIO_HANDLE temp_handle;
	int device = -1;

    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_WRITE_OPEN,0) ;
#endif

	device = substream->pcm->device;

	chip = snd_pcm_substream_chip(substream);
	runtime = substream->runtime;
	if (device == 0)
		stream_id = 0;
	else if (device == 1)
		stream_id = 2;

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	down(&chip->bridge_mutex);
	if(device == VOICE_CODEC && chip->playback_state != LOCKED_BY_NONE){
		pr_error("Device is busy");
		up(&chip->bridge_mutex);
		return -EBUSY;
	}
#endif

	pr_debug("device %d stream id=%d", device, stream_id);

	err = -1;

	if (stream_id == 0) {
		if (PMIC_SUCCESS == pmic_audio_open(&temp_handle, STEREO_DAC)) {
			audio_mixer_control.stdac_handle = temp_handle;
			audio_mixer_control.stdac_playback_active = 1;
			chip->s[stream_id].pmic_audio_device.handle =
			    temp_handle;
		} else {
			pr_error("busy at line ");
			err = -EBUSY;
			goto fail;
		}
	} else if (stream_id == 2) {
		audio_mixer_control.codec_playback_active = 1;
		if (PMIC_SUCCESS == pmic_audio_open(&temp_handle, VOICE_CODEC)) {
			audio_mixer_control.voice_codec_handle = temp_handle;
			chip->s[stream_id].pmic_audio_device.handle =
			    temp_handle;
		} else {
			if (audio_mixer_control.codec_capture_active) {
				temp_handle =
				    audio_mixer_control.voice_codec_handle;
			} else {
				pr_error("busy at line ");
				err = -EBUSY;
				goto fail;
			}
		}
	}

// Il n'est pas necessaire d'activer l'antipop (BIASEN=1) ceci
// a deja ete fait lors de l'init (fonction : mxc_pmic_mixer_controls_init)
//	pmic_audio_antipop_enable(ANTI_POP_RAMP_SLOW);
//	msleep(250);

	chip->s[stream_id].stream = substream;

	if (stream_id == 0)
		runtime->hw = snd_mxc_pmic_playback_stereo;
	else if (stream_id == 2)
		runtime->hw = snd_mxc_pmic_playback_mono;
	else
	{
		pr_error("unknown stream id");
		err = -ENODEV;
		goto fail;
	}

	if ((err = snd_pcm_hw_constraint_integer(runtime,
						 SNDRV_PCM_HW_PARAM_PERIODS)) <
	    0)
	{
		pr_debug("%s - WARNING - bad period value",__FUNCTION__);
		goto fail;
	}

	if (stream_id == 0) {
		if ((err = snd_pcm_hw_constraint_list(runtime, 0,
						      SNDRV_PCM_HW_PARAM_RATE,
						      &hw_playback_rates_stereo))
		    < 0)
		{
			pr_debug("%s - WARNING - bad sample rate at line %d",__FUNCTION__,__LINE__);
			goto fail;
		}
	} else if (stream_id == 2) {
		if ((err = snd_pcm_hw_constraint_list(runtime, 0,
						      SNDRV_PCM_HW_PARAM_RATE,
						      &hw_playback_rates_mono))
		    < 0)
		{
			pr_debug("%s - WARNING - bad sample rate at line %d",__FUNCTION__,__LINE__);
			goto fail;
		}
	}
	msleep(10);

	/* setup DMA controller for playback */
	if ((err =
	     configure_write_channel(&mxc_audio->s[stream_id],
				     audio_playback_dma_callback,
				     stream_id)) < 0)
	{
		pr_error("channel configuration has failed");
		goto fail;
	}

    pr_debug("stream id=%d",stream_id);
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_WRITE_OPEN_DONE,0) ;
#endif

fail:
#if defined(CONFIG_SND_AUDIO_BRIDGE)
	if(!err && device == VOICE_CODEC)
		chip->playback_state = LOCKED_BY_ALSA;
	up(&chip->bridge_mutex);
#endif
	return err;
}

/*!
  * This function closes an PMIC audio device for playback.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_playback_close(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;
	audio_stream_t *s = NULL;
	PMIC_AUDIO_HANDLE handle;
	int ssi = -1;
	int device = -1, stream_id = -1;

	handle = (PMIC_AUDIO_HANDLE) NULL;

	device = substream->pcm->device;
	if (device == 0)
		stream_id = 0;
	else if (device == 1)
		stream_id = 2;
    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
	pr_debug("stream id=%d",stream_id);

	chip = snd_pcm_substream_chip(substream);

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	down(&chip->bridge_mutex);
	if(device == VOICE_CODEC && chip->playback_state != LOCKED_BY_ALSA){
		pr_error("Driver has not been open by alsa");
		up(&chip->bridge_mutex);
		return -EPERM;
	}

	if(device == VOICE_CODEC)
		chip->playback_state = LOCKED_BY_NONE;
#endif

	s = &chip->s[stream_id];
	ssi = s->ssi;

	if (stream_id == 0) {
		disable_stereodac();
		audio_mixer_control.stdac_playback_active = 0;
		handle = audio_mixer_control.stdac_handle;
		audio_mixer_control.stdac_handle = NULL;
		chip->s[stream_id].pmic_audio_device.handle = NULL;
	} else if (stream_id == 2) {
		audio_mixer_control.codec_playback_active = 0;
		handle = audio_mixer_control.voice_codec_handle;
		disable_codec(handle);
		audio_mixer_control.voice_codec_handle = NULL;
		chip->s[stream_id].pmic_audio_device.handle = NULL;
	}

	pmic_audio_close(handle);

	ssi_transmit_enable(ssi, false);
	ssi_interrupt_disable(ssi, ssi_tx_dma_interrupt_enable);
	ssi_tx_fifo_enable(ssi, ssi_fifo_0, false);
	ssi_enable(ssi, false);
	mxc_dma_free((mxc_audio->s[stream_id]).dma_wchannel);

	chip->s[stream_id].stream = NULL;

    pr_debug("stream id=%d",stream_id);
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
#if defined(CONFIG_SND_AUDIO_BRIDGE)
	up(&chip->bridge_mutex);
#endif
	return 0;
}

/*!
  * This function closes a PMIC audio device for capture.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_capture_close(snd_pcm_substream_t * substream)
{
	PMIC_AUDIO_HANDLE handle;
	mxc_pmic_audio_t *chip = NULL;
	audio_stream_t *s = NULL;
	int ssi = -1;

    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[substream->pstr->stream];
	ssi = s->ssi;

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	down(&chip->bridge_mutex);
	if(chip->capture_state != LOCKED_BY_ALSA){
		pr_error("Driver has not been open by alsa");
		up(&chip->bridge_mutex);
		return -EPERM;
	}
	
	chip->capture_state = LOCKED_BY_NONE;
#endif

	audio_mixer_control.codec_capture_active = 0;
	handle = audio_mixer_control.voice_codec_handle;
	disable_codec(handle);
	audio_mixer_control.voice_codec_handle = NULL;
	chip->s[SNDRV_PCM_STREAM_CAPTURE].pmic_audio_device.handle = NULL;

	pmic_audio_close(handle);

	ssi_receive_enable(ssi, false);
	ssi_interrupt_disable(ssi, ssi_rx_dma_interrupt_enable);
	ssi_rx_fifo_enable(ssi, ssi_fifo_0, false);
	ssi_enable(ssi, false);
	mxc_dma_free((mxc_audio->s[1]).dma_wchannel);

	chip->s[substream->pstr->stream].stream = NULL;

    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);

#if defined(CONFIG_SND_AUDIO_BRIDGE)
	up(&chip->bridge_mutex);
#endif
	return 0;
}

#if defined(CONFIG_SND_AUDIO_BRIDGE)
/***************** Driver state to /sys (mads : mxc_audio_driver_state) *************************/

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static ssize_t sysdev_read_mads(struct sys_device *dev, char *buf)
#else
static ssize_t sysdev_read_mads(struct sys_device *dev, struct sysdev_attribute *attr, char *buf)
#endif
{
	int size = 0;

	if(!mxc_audio)
		return -EFAULT;
	
	size += sprintf((buf + size), "Playback state : %s\n", 
			(mxc_audio->playback_state==LOCKED_BY_ALSA)?"LOCKED_BY_ALSA":
			(mxc_audio->playback_state==LOCKED_BY_BRIDGE)?"LOCKED_BY_BRIDGE":
			(mxc_audio->playback_state==LOCKED_BY_NONE)?"LOCKED_BY_NONE":"INVALID STATE");
	size += sprintf((buf + size), "Capture state : %s\n", 
			(mxc_audio->capture_state==LOCKED_BY_ALSA)?"LOCKED_BY_ALSA":
			(mxc_audio->capture_state==LOCKED_BY_BRIDGE)?"LOCKED_BY_BRIDGE":
			(mxc_audio->capture_state==LOCKED_BY_NONE)?"LOCKED_BY_NONE":"INVALID STATE");

	return size;
}


static SYSDEV_ATTR(mads, 0444, sysdev_read_mads, NULL);

static struct sysdev_class sysclass_mads = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
	 set_kset_name("mxc_audio"),
#else
	 .name = "mxc_audio",
#endif
};

static struct sys_device sysdevice_mads = {
	.id = 0,
	.cls = &sysclass_mads,
};


static void sysdev_exit_mads(void){
	sysdev_remove_file(&sysdevice_mads, &attr_mads);
	sysdev_unregister(&sysdevice_mads);
	sysdev_class_unregister(&sysclass_mads);
}


static int sysdev_init_mads(void){
	int err;
	err = sysdev_class_register(&sysclass_mads);
	if(err){
		pr_error("Could not register class : error %d", err);
		return err;
	}

	err = sysdev_register(&sysdevice_mads);
	if(err){
		pr_error("Could not register device : error %d", err);
		sysdev_exit_mads();
		return err;
	}

	err = sysdev_create_file(&sysdevice_mads, &attr_mads);
	if(err){
		pr_error("Could not create a sysdev file: error %d", err);
		sysdev_exit_mads();
		return err;
	}
	return 0;
}


/***************** End Driver state to /sys ********************/
#endif


static ssize_t sysdev_read_headset_state(struct sys_device *dev, char *buf){
	int headset_state = pmic_audio_get_last_headset_state();

	return sprintf(buf, "%splugged", (headset_state ? "" : "un"));
}

static SYSDEV_ATTR(headset_plugged, 0444, sysdev_read_headset_state, NULL);

static struct sysdev_class sysclass = {
	set_kset_name("Audio_Headset"),
};

static struct sys_device sysdevice = {
	.id = 0,
	.cls = &sysclass,
};

static void headset_plugged_sysdev_exit(void){
	sysdev_remove_file(&sysdevice, &attr_headset_plugged);
	sysdev_unregister(&sysdevice);
	sysdev_class_unregister(&sysclass);
}

static int headset_plugged_sysdev_init(void){

	int err;

	pr_debug("Initialzing headset state sysdev file");

	err = sysdev_class_register(&sysclass);
	if(err){
		pr_error("Could not register class : error %d", err);
		return err;
	}

	err = sysdev_register(&sysdevice);
	if(err){
		pr_error("Could not register device : error %d", err);
		headset_plugged_sysdev_exit();
		return err;
	}

	err = sysdev_create_file(&sysdevice, &attr_headset_plugged);
	if(err){
		pr_error("Could not create a sysdev file to export pins state : error %d", err);
		headset_plugged_sysdev_exit();
		return err;
	}
	
	return 0;
}

/*!
 * This function adds an ioctl to fetch the headset state
 * It wraps itself around standard snd_pcm_lib_ioctl function
 *
 * @substream: the pcm substream instance
 * @cmd: ioctl command
 * @arg: ioctl argume 
 *
 * If command is get headset returns 0 for unplugged, 1 for plugged
 */
int snd_mxc_pcm_ioctl(struct snd_pcm_substream *substream,
                      unsigned int cmd, void *arg)
{
	if(cmd == SNDRV_MXC_PCM_IOCTL_GET_HEADSET_STATE)
		return pmic_audio_get_last_headset_state();
	else
		return snd_pcm_lib_ioctl(substream, cmd, arg);
}

/*!
  * This function configure the Audio HW in terms of memory allocation.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_hw_params(snd_pcm_substream_t * substream,
				   snd_pcm_hw_params_t * hw_params)
{
	snd_pcm_runtime_t *runtime = NULL;
	int ret = -1;

	pr_debug("%s - Enter",__FUNCTION__);

	runtime = substream->runtime;
	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
	if (ret < 0)
	{
		pr_error("%s - Cannot allocate substream",__FUNCTION__);
		return ret;
	}

	runtime->dma_addr = virt_to_phys(runtime->dma_area);

	pr_debug("MXC: snd_mxc_audio_hw_params runtime->dma_addr 0x(%x)",
		 (unsigned int)runtime->dma_addr);
	pr_debug("MXC: snd_mxc_audio_hw_params runtime->dma_area 0x(%x)",
		 (unsigned int)runtime->dma_area);
	pr_debug("MXC: snd_mxc_audio_hw_params runtime->dma_bytes 0x(%x)",
		 (unsigned int)runtime->dma_bytes);

	return ret;
}

/*!
  * This function frees the audio hardware at the end of playback/capture.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_hw_free(snd_pcm_substream_t * substream)
{
    int ret;
    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);

    ret = snd_pcm_lib_free_pages(substream);
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
	return ret;
}

/*!
  * This function configures the hardware to allow audio
  * capture operations. It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_mxc_audio_capture_prepare(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;
	audio_stream_t *s = NULL;
	int ssi = -1;
    pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
	pr_debug("device=%d",substream->pcm->device);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_READ_PREPARE,0) ;
#endif
	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[substream->pstr->stream];
	ssi = s->ssi;

	normalize_speed_for_pmic(&(s->pmic_audio_device), substream->runtime);

	pr_debug("substream->pstr->stream %d", substream->pstr->stream);
	pr_debug("SSI %d", ssi + 1);
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
        configure_dam_pmic_master(1); //device 1
#else
	configure_dam_pmic_master(ssi);
#endif

	configure_ssi_rx(ssi);

	ssi_interrupt_enable(ssi, ssi_rx_dma_interrupt_enable);

	if (configure_pmic_recording(s))
	{
		pr_error(KERN_ERR "MXC: PMIC Record Config FAILED");
	}
	ssi_interrupt_enable(ssi, ssi_rx_fifo_0_full);
	ssi_receive_enable(ssi, true);

	msleep(20);
	set_pmic_channels(substream->runtime, s->stream_id, ssi);

	s->period = 0;
	s->periods = 0;
    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_READ_PREPARE_DONE,0) ;
#endif

	return 0;
}

/*!
  * This function opens an PMIC audio device in capture mode
  * on Codec.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_capture_open(snd_pcm_substream_t * substream)
{
	mxc_pmic_audio_t *chip = NULL;
	snd_pcm_runtime_t *runtime = NULL;
	int stream_id = -1;
	int err = -1;
	PMIC_AUDIO_HANDLE temp_handle;
    
	pr_debug("%s:%d: Enter", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_READ_OPEN,0) ;
#endif

	chip = snd_pcm_substream_chip(substream);
	
#if defined(CONFIG_SND_AUDIO_BRIDGE)
	down(&chip->bridge_mutex);
	if(chip->capture_state != LOCKED_BY_NONE){
		pr_error("Devices busy");
		up(&chip->bridge_mutex);
		return -EBUSY;
	}
#endif


	runtime = substream->runtime;
	stream_id = substream->pstr->stream;
	err = -1;
	pr_debug(" stream id=%d",stream_id);

	if (PMIC_SUCCESS == pmic_audio_open(&temp_handle, VOICE_CODEC)) {
		audio_mixer_control.voice_codec_handle = temp_handle;
		audio_mixer_control.codec_capture_active = 1;
		chip->s[SNDRV_PCM_STREAM_CAPTURE].pmic_audio_device.handle =
		    temp_handle;
	} else {
		if (audio_mixer_control.codec_playback_active) {
			temp_handle = audio_mixer_control.voice_codec_handle;
		} else {
			pr_error("busy");
			err = -EBUSY;
			goto fail;
		}
	}

	//done at init stage
	//pmic_audio_antipop_enable(ANTI_POP_RAMP_SLOW);

	chip->s[stream_id].stream = substream;

	if (stream_id == SNDRV_PCM_STREAM_CAPTURE) {
		runtime->hw = snd_mxc_pmic_capture;
	} else {
		pr_error("unknown stream id");
		err = -ENODEV;
		goto fail;
	}

	if ((err = snd_pcm_hw_constraint_integer(runtime,
						 SNDRV_PCM_HW_PARAM_PERIODS)) <
	    0) {
		pr_debug("%s - WARNING - bad period setting",__FUNCTION__);
		goto fail;
	}

	if ((err = snd_pcm_hw_constraint_list(runtime, 0,
					      SNDRV_PCM_HW_PARAM_RATE,
					      &hw_capture_rates)) < 0) {
		pr_debug("%s - WARNING - bad sample rate",__FUNCTION__);
		goto fail;
	}

	/* setup DMA controller for Record */
	err = configure_read_channel(&mxc_audio->s[SNDRV_PCM_STREAM_CAPTURE],
				     audio_capture_dma_callback);
	if (err < 0) {
		pr_debug("%s - ERROR - channel configuration has failed",__FUNCTION__);
		goto fail;
	}

	msleep(50);

    pr_debug("%s:%d: Exit", __FUNCTION__, __LINE__);
fail:
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_PMIC_READ_OPEN_DONE,0) ;
#endif
#if defined(CONFIG_SND_AUDIO_BRIDGE)
	if(!err)
		chip->capture_state = LOCKED_BY_ALSA;
	up(&chip->bridge_mutex);
#endif
	return err;
}

#if defined(CONFIG_SND_AUDIO_BRIDGE)
/*********************************************************
 * BRIDGE
 ********************************************************/

/*********************************************************
 * PLAYBACK
 ********************************************************/

static void audio_playback_dma_bridge_callback(void *data, int error, unsigned int count){
	audio_stream_t *s = data;
	struct list_dma_addr *dal = NULL;
	elapsed_t callback;
	void *bridge_ctx;

	if(!s || !s->stream_bridge_ops){
		pr_error("Null pointer error (%p)", s);
		return;
	}
	
	list_for_each_entry(dal, &(s->playback_dma_addr_list_head), list){
		list_del(&(dal->list));
		break;
	}
	if(!dal){
		pr_error("Pointer to dma list is Null");
		return;
	}

	callback = dal->callback;
	bridge_ctx = dal->bridge_ctx;

	dma_unmap_single(NULL, dal->dma_addr_period, dal->dma_size, DMA_TO_DEVICE);
	kfree(dal);

	if(callback){
		callback(bridge_ctx, error, count/sizeof(short));
	} else {
		pr_error("playback elapsed not set");
	}

	return;
}


void snd_card_mxc_audio_bridge_hw_params(mxc_pmic_audio_t *ctx){
	ctx->bridge_ops->buffer_infos->dma_addr = virt_to_phys(ctx->bridge_ops->buffer_infos->buffer);
	return;
}


void snd_card_mxc_audio_playback_bridge_prepare(mxc_pmic_audio_t *ctx){
	int stream_id;
	int device;
	int ssi;

	device = ctx->bridge_ops->playback_device_number;
	stream_id = ctx->bridge_ops->playback_stream_id;
	ssi = ctx->s[stream_id].ssi;

	pr_debug("device %d stream_id %d ssi %d", device, stream_id, ssi);

	normalize_speed_for_pmic(&(ctx->s[stream_id].pmic_audio_device), NULL);
	configure_dam_pmic_master(device);
	configure_ssi_tx(NULL, stream_id, ssi);

	ssi_interrupt_enable(ssi, ssi_tx_dma_interrupt_enable);

	if (configure_pmic_playback(NULL, &(ctx->s[stream_id]), stream_id)){
		pr_error(KERN_ERR "MXC: PMIC Playback Config FAILED");
	}
	ssi_interrupt_enable(ssi, ssi_tx_fifo_0_empty);

	set_pmic_channels(NULL, stream_id, ssi);

	pr_debug("Exit");
	return;
}

int snd_card_mxc_audio_playback_bridge_open(void *private_data){
	mxc_pmic_audio_t *ctx;
	int err = 0;
	PMIC_AUDIO_HANDLE temp_handle;
	pr_debug("Enter");
	if(!private_data){
		pr_error("Null parameter");
		return -EINVAL;
	}
	ctx = private_data;
	down(&(ctx->bridge_mutex));
	if(ctx->playback_state != LOCKED_BY_NONE){
		pr_error("Driver is in use");
		err = -EBUSY;
		goto end;
	}

	/* got from snd_card_mxc_audio_capture_open */
	pr_debug("device %d. stream id %d", ctx->bridge_ops->playback_device_number, ctx->bridge_ops->playback_stream_id);

	audio_mixer_control.codec_playback_active = 1;
	err = pmic_audio_open(&temp_handle, VOICE_CODEC);
	if(err == PMIC_SYSTEM_ERROR_EINTR){
		pr_error("Could not open pmic audio");
		goto end;
	}
	
	if (err == PMIC_SUCCESS) {
		audio_mixer_control.voice_codec_handle = temp_handle;
		ctx->s[ctx->bridge_ops->playback_stream_id].pmic_audio_device.handle = temp_handle;
	} else {
		if (audio_mixer_control.codec_capture_active) {
			temp_handle = audio_mixer_control.voice_codec_handle;
		} else {
			pr_error("busy");
			err = -EBUSY;
			goto end;
		}
	}

	/* FIXME
	 * setting antipop ?
	 */

	ctx->s[ctx->bridge_ops->playback_stream_id].stream = NULL;

	err = configure_write_channel(&mxc_audio->s[ctx->bridge_ops->playback_stream_id],
			audio_playback_dma_bridge_callback,
			ctx->bridge_ops->playback_stream_id);
	if(err){
		pr_error("Could not configure write channel");
		goto end;
	}

	snd_card_mxc_audio_bridge_hw_params(ctx);
	snd_card_mxc_audio_playback_bridge_prepare(ctx);
	snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_START);
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
		vg_handset_detect_on++;
		pr_debug("%s - Headset detection on vg_handset_detect_on=%d",__FUNCTION__,vg_handset_detect_on);
#endif
	ctx->playback_state = LOCKED_BY_BRIDGE;
end:
	pr_info("%s", err?"Fail":"Success");
	up(&(ctx->bridge_mutex));
	return err;
}

void snd_card_mxc_audio_playback_bridge_close(void *private_data){
	mxc_pmic_audio_t *ctx;
	PMIC_AUDIO_HANDLE handle;
	int ssi;
	pr_debug("Enter");
	if(!private_data){
		pr_error("Null parameter");
		return;
	}

	ctx = private_data;
	down(&(ctx->bridge_mutex));
	if(ctx->playback_state != LOCKED_BY_BRIDGE){
		pr_error("Driver has not been open by bridge");
		goto end;
	}
	
	ctx->playback_state = LOCKED_BY_NONE;

	audio_mixer_control.codec_playback_active = 0;
	handle = audio_mixer_control.voice_codec_handle;
	disable_codec(handle);
	audio_mixer_control.voice_codec_handle = NULL;
	ctx->s[ctx->bridge_ops->playback_stream_id].pmic_audio_device.handle = NULL;

	pr_debug("Closing handle (%p)", handle);
	pmic_audio_close(handle);

	ssi = ctx->s[ctx->bridge_ops->playback_stream_id].ssi;
	pr_info("Closing ssi %d", ssi);

	ssi_transmit_enable(ssi, false);
	ssi_interrupt_disable(ssi, ssi_tx_dma_interrupt_enable);
	ssi_tx_fifo_enable(ssi, ssi_fifo_0, false);
	ssi_enable(ssi, false);
	mxc_dma_free((mxc_audio->s[ctx->bridge_ops->playback_stream_id]).dma_wchannel);
	snd_mxc_audio_playback_trigger_amp(SNDRV_PCM_TRIGGER_STOP);
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
	vg_handset_detect_on--;
	pr_debug("%s - Headset detection on vg_handset_detect_on=%d",__FUNCTION__,vg_handset_detect_on);
#endif

	ctx->s[ctx->bridge_ops->playback_stream_id].stream = NULL;
end:
	pr_info("Exit");
	up(&(ctx->bridge_mutex));
	return;
}


int snd_card_mxc_audio_playback_bridge_start_prepare_dma(audio_stream_t *s, void *src, int ps, void *bridge_ctx, elapsed_t callback){
	mxc_dma_requestbuf_t dma_request;
	struct list_dma_addr *dal;
	int ret;

	/* Filling dma request struct */
	memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));
	dma_request.num_of_bytes = ps * sizeof(short);
	dma_request.src_addr = (dma_addr_t) (dma_map_single(NULL, src, dma_request.num_of_bytes, DMA_TO_DEVICE));
	dma_request.dst_addr = (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1STX0);

	/* Saving dma addr and size to give it back to dma_unmap_single */
	dal = kmalloc(sizeof(struct list_dma_addr), GFP_ATOMIC);
	if(!dal){
		pr_error("Could not allocate memory for dma_addr_list\n");
		return -ENOMEM;
	}
	dal->dma_addr_period = dma_request.src_addr;
	dal->dma_size = dma_request.num_of_bytes;
	dal->callback = callback;
	dal->bridge_ctx = bridge_ctx;
	list_add_tail(&(dal->list), &(s->playback_dma_addr_list_head));


	/* Start dma */
	ret = mxc_dma_config(s->dma_wchannel, &dma_request, 1, MXC_DMA_MODE_WRITE);
	if(ret){
		pr_error("Could not configure dma : error %d", ret);
		goto err;
	}

	ret = mxc_dma_enable(s->dma_wchannel);
	if(ret){
		pr_error("ERROR - dma enable failed : error %d", ret);
		goto err;
	}

	return 0;
err:
	list_del(&(dal->list));
	kfree(dal);
	return ret;
}


int snd_card_mxc_audio_playback_bridge_start(void *private_data, void *src, int period_size, void *bridge_ctx, elapsed_t callback){
	mxc_pmic_audio_t *ctx;
	struct list_dma_addr *dal;
	audio_stream_t *s;
	struct bridge_buffer_info *binfos;
	int ret, nb = 0;
	
	if(!private_data || !src || period_size < 1 || !bridge_ctx){
		pr_error("Invalid val : Private_data (%p) src (%p) period_size (%d) bridge_ctx (%p)",
				private_data, src, period_size, bridge_ctx);
		return -EINVAL;
	}

	ctx = private_data;
	s = &(ctx->s[ctx->bridge_ops->playback_stream_id]);
	binfos = s->stream_bridge_ops->buffer_infos;

	if(ctx->bridge_ops->period_size != period_size){
		pr_error("Bad period size : expected (%d), got (%d)\n", ctx->bridge_ops->period_size, period_size);
		return -EINVAL;
	}

	
	if (ssi_get_status(s->ssi) & (ssi_transmitter_underrun_0)) {
		/*ulog(ULOG_AUDIO_BRIDGE_PLAYBACK_UNDERRUN_PMIC,0);*/
		/*printk(KERN_ERR "SSI underrun has been detected, let's disable/enable ssi\n");*/
		ssi_enable(s->ssi, false);
		ssi_transmit_enable(s->ssi, false);
		ssi_enable(s->ssi, true);
	}


	ret = snd_card_mxc_audio_playback_bridge_start_prepare_dma(s, src, period_size, bridge_ctx, callback);
	if(ret){
		pr_error("Could not prepare dma for playback");
		return ret;
	}

	ssi_transmit_enable(s->ssi, true);

	/* DMA chaining */ /* FIXME : must be atomic */
	list_for_each_entry(dal, &(s->playback_dma_addr_list_head), list){
		nb++;
	}
	pmic_assert(nb > 0 && nb < 3);
	if(nb == 2){
		return 0;
	}
	ret = start_chaning_playback_dma(bridge_ctx, &src, &period_size);
	if(ret){
		return 0;
	}

	/* There is enought data in bridge to ask a new transfer */
	/*ulog(ULOG_AUDIO_BRIDGE_PLAYBACK_WILL_CHAIN_DMA, 0);*/
	
	ret = snd_card_mxc_audio_playback_bridge_start_prepare_dma(s, src, period_size, bridge_ctx, callback);
	if(ret){
		/* TODO : delete this printk to avoid large amounts of traces
		 * if this error occurs
		 */
		pr_error("Could not prepare dma for chaning playback");
	}

	return 0;
}

int snd_card_mxc_audio_playback_bridge_stop(void *private_data){
	struct list_dma_addr *dal;
	mxc_pmic_audio_t *ctx = private_data;
	struct bridge_buffer_info *binfos;
	audio_stream_t *s;
	
	pr_info("Enter");
	if(!ctx){
		pr_error("Null pointer");
		return -EINVAL;
	}
	s = &(ctx->s[ctx->bridge_ops->playback_stream_id]);
	binfos = s->stream_bridge_ops->buffer_infos;
	if(!binfos){
		pr_error("Null pointer");
		return -EFAULT;
	}
	
	mxc_dma_disable(s->dma_wchannel);
	ssi_transmit_enable(s->ssi, false);
	mxc_dma_pause(s->dma_wchannel);

	while(!list_empty(&(s->playback_dma_addr_list_head))){
		list_for_each_entry(dal, &(s->playback_dma_addr_list_head), list){
			list_del(&(dal->list));
			if(dal)
				dma_unmap_single(NULL, dal->dma_addr_period, dal->dma_size, DMA_TO_DEVICE);
			kfree(dal);
			break;
		}
	}

	return 0;
}


/*********************************************************
 * CAPTURE
 ********************************************************/

static void audio_capture_bridge_dma_callback(void *data, int error, unsigned int count){
	audio_stream_t *s = data;
	struct list_dma_addr *dal = NULL;
	elapsed_t callback;
	void *bridge_ctx;

	if(!s){
		pr_error("Null pointer");
		return;
	}

	list_for_each_entry(dal, &(s->capture_dma_addr_list_head), list){
		list_del(&(dal->list));
		break;
	}

	pmic_assert(dal);

	callback = dal->callback;
	bridge_ctx = dal->bridge_ctx;
	/* TEMPORARY */
	pmic_assert(callback == dal->callback);
	pmic_assert(bridge_ctx == dal->bridge_ctx);

	dma_unmap_single(NULL,
			dal->dma_addr_period,
			dal->dma_size,
			DMA_FROM_DEVICE);
	
	kfree(dal);
	
	if(callback)
		callback(bridge_ctx, error, count/sizeof(short));
	else {
		 pr_error("Elapsed callback is NULL");
	}

	return;
}


void snd_card_mxc_audio_capture_bridge_prepare(audio_stream_t *s){

	normalize_speed_for_pmic(&(s->pmic_audio_device), NULL);

        configure_dam_pmic_master(VOICE_CODEC);

	configure_ssi_rx(s->ssi);

	ssi_interrupt_enable(s->ssi, ssi_rx_dma_interrupt_enable);

	if(configure_pmic_recording(s)){
		pr_error(KERN_ERR "MXC: PMIC Record Config FAILED");
	}
	ssi_interrupt_enable(s->ssi, ssi_rx_fifo_0_full);
	ssi_receive_enable(s->ssi, true);

	msleep(20);
	set_pmic_channels(NULL, 1, s->ssi);
	pr_debug("Exit");
	return;
}

int snd_card_mxc_audio_capture_bridge_open(void *private_data){
	mxc_pmic_audio_t *ctx;
	int ret = 0;
	PMIC_AUDIO_HANDLE temp_handle;
	pr_info("Enter");
	if(!private_data){
		pr_error("Null parameter");
		return -EINVAL;
	}
	ctx = private_data;
	down(&(ctx->bridge_mutex));
	if(ctx->capture_state != LOCKED_BY_NONE){
		pr_error("Driver is in use");
		ret = -EBUSY;
		goto end;
	}
	
	pr_debug("stream id=%d", 1);
	if(1 != ctx->bridge_ops->capture_stream_id){
		pr_error("1 != ctx->bridge_ops->capture_stream_id");
		ret = -EINVAL;
		goto end;
	}

	ret = pmic_audio_open(&temp_handle, VOICE_CODEC);
	if(ret == PMIC_SUCCESS) {
		audio_mixer_control.voice_codec_handle = temp_handle;
		ctx->s[1].pmic_audio_device.handle = temp_handle;
	} else {
		if (audio_mixer_control.codec_playback_active) {
			temp_handle = audio_mixer_control.voice_codec_handle;
		} else {
			pr_error("busy");
			ret = -EBUSY;
			goto end;
		}
	}
	audio_mixer_control.codec_capture_active = 1;

	ctx->s[1].stream = NULL;

	ret = configure_read_channel(&mxc_audio->s[1],
			audio_capture_bridge_dma_callback);
	if(ret){
		pr_debug("ERROR - channel configuration has failed");
		goto end;
	}

	msleep(50);

	snd_card_mxc_audio_bridge_hw_params(ctx);
	
	snd_card_mxc_audio_capture_bridge_prepare(&(ctx->s[1]));
	ctx->capture_state = LOCKED_BY_BRIDGE;

end:
	pr_info("%s", ret?"Fail":"Success");
	up(&(ctx->bridge_mutex));
	return ret;
}

void snd_card_mxc_audio_capture_bridge_close(void *private_data){
	mxc_pmic_audio_t *ctx;
	PMIC_AUDIO_HANDLE handle;
	int ssi;
	pr_info("Enter");
	if(!private_data){
		pr_error("Null parameter");
		return;
	}


	ctx = private_data;
	down(&(ctx->bridge_mutex));
	if(ctx->capture_state != LOCKED_BY_BRIDGE){
		pr_info("Driver has not been open by bridge");
		goto end;
	}
	ctx->capture_state = LOCKED_BY_NONE;
	ssi = ctx->s[1].ssi;

	audio_mixer_control.codec_capture_active = 0;
	handle = audio_mixer_control.voice_codec_handle;
	disable_codec(handle);
	audio_mixer_control.voice_codec_handle = NULL;
	ctx->s[1].pmic_audio_device.handle = NULL;

	pmic_audio_close(handle);

	ssi_receive_enable(ssi, false);
	ssi_interrupt_disable(ssi, ssi_rx_dma_interrupt_enable);
	ssi_rx_fifo_enable(ssi, ssi_fifo_0, false);
	ssi_enable(ssi, false);
	mxc_dma_free((mxc_audio->s[1]).dma_wchannel);

	ctx->s[1].stream = NULL;

end:
	pr_info("Exit");
	up(&(ctx->bridge_mutex));
	return;
}

static int snd_card_mxc_audio_capture_bridge_start_prepare_dma(audio_stream_t *s, void *src, int ps, void *bridge_ctx, elapsed_t callback){
	mxc_dma_requestbuf_t dma_request;
	struct list_dma_addr *dal;
	int ret;
	
	/* Filling dma request struct */
	memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));
	dma_request.num_of_bytes = ps * sizeof(short);
	dma_request.dst_addr = (dma_addr_t) (dma_map_single(NULL, src, dma_request.num_of_bytes, DMA_FROM_DEVICE));
	dma_request.src_addr = (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1SRX0);

	/* Saving dma addr and size to give it back to dma_unmap_single */
	dal = kmalloc(sizeof(struct list_dma_addr), GFP_ATOMIC);
	if(!dal){
		pr_error("Could not allocate memory for dma_addr_list\n");
		return -ENOMEM;
	}
	dal->dma_addr_period = dma_request.dst_addr;
	dal->dma_size = dma_request.num_of_bytes;
	dal->bridge_ctx = bridge_ctx;
	dal->callback = callback;
	list_add_tail(&(dal->list), &(s->capture_dma_addr_list_head));
	

	ret = mxc_dma_config(s->dma_wchannel, &dma_request, 1, MXC_DMA_MODE_READ);
	if(ret){
		pr_error("ERROR - dma configuration failed - error %d", ret);
		goto err;
	}

	ret = mxc_dma_enable(s->dma_wchannel);
	if(ret){
		pr_error("Could not enable dma");
		goto err;
	}

	return 0;
err:
	list_del(&(dal->list));
	kfree(dal);
	return ret;
}

int snd_card_mxc_audio_capture_bridge_start(void *private_data, void *src, int period_size, void *bridge_ctx, elapsed_t callback){
	int ret, nb;
	audio_stream_t *s;
	mxc_pmic_audio_t *ctx;
	struct list_dma_addr *dal;

	if(!private_data || !src || period_size < 1 || !bridge_ctx){
		pr_error("Invalid val : private_data (%p) src (%p) period_size (%d) bridge_ctx (%p)", private_data, src, period_size, bridge_ctx);
		return -EINVAL;
	}

	ctx = private_data;
	s = &(ctx->s[ctx->bridge_ops->capture_stream_id]);

	if(ctx->bridge_ops->period_size != period_size){
		pr_error("Bad period size : expected (%d), got (%d)\n", ctx->bridge_ops->period_size, period_size);
		return -EINVAL;
	}

	/* TODO : check ssi status (underrun / overrun) */

	ret = snd_card_mxc_audio_capture_bridge_start_prepare_dma(s, src, period_size, bridge_ctx, callback);
	if(ret){
		return ret;
	}

	/* FIXME : is ssi_transmit_enable(ssi, true); missing ? */


	/* DMA chaining */
	nb = 0;
	list_for_each_entry(dal, &(s->capture_dma_addr_list_head), list){
		nb++;
	}
	pmic_assert(nb > 0 && nb < 3);
	/*ulog(ULOG_AUDIO_BRIDGE_CAPTURE_CHAIN_DMA_NB, nb);*/
	if(nb == 2){
		return 0; /* dma already chained */
	}
	

	ret = start_chaining_capture_dma(bridge_ctx, &src, &period_size);
	/*ulog(ULOG_AUDIO_BRIDGE_CAPTURE_WILL_CHAIN_DMA, ret);*/
	if(ret){
		return 0; /* Not enougth data to chain dma */
	}


	ret = snd_card_mxc_audio_capture_bridge_start_prepare_dma(s, src, period_size, bridge_ctx, callback);
	if(ret){
		/* TODO : delete this printk to avoid large amounts of traces */
		pr_error("Could not prepare dma for chaning capture");
	}

	return ret;
}

int snd_card_mxc_audio_capture_bridge_stop(void *private_data){
	struct bridge_buffer_info *binfos;
	struct list_dma_addr *dal;
	mxc_pmic_audio_t *ctx = private_data;
	audio_stream_t *s;
	pr_info("Enter");

	if(!ctx){
		pr_error("Null pointer");
		return -EINVAL;
	}
	s = &(ctx->s[ctx->bridge_ops->capture_stream_id]);
	binfos = s->stream_bridge_ops->buffer_infos;
	if(!binfos){
		pr_error("Null pointer");
		return -EFAULT;
	}

	mxc_dma_disable(s->dma_wchannel);
	ssi_transmit_enable(s->ssi, false);
	mxc_dma_pause(s->dma_wchannel);

	while(!list_empty(&(s->capture_dma_addr_list_head))){
		list_for_each_entry(dal, &(s->capture_dma_addr_list_head), list){
			list_del(&(dal->list));
			if(dal)
				dma_unmap_single(NULL, dal->dma_addr_period, dal->dma_size, DMA_FROM_DEVICE);
			kfree(dal);
			break;
		}
	}
	return 0;
}


/*********************************************************
 * BRIDGE STRUCT
 ********************************************************/

/*! 
 * bridge file operations 
 * Don t forget to set .private_data in __init
 */
struct audio_bridge_operations snd_card_bridge_fops = {
	.name = "mxc_pmic",
	.period_size = 16, /* Trial, original value is 80 to be compatible with DECT usb */

	.playback_open = snd_card_mxc_audio_playback_bridge_open,
	.playback_close = snd_card_mxc_audio_playback_bridge_close,
	.playback_start = snd_card_mxc_audio_playback_bridge_start,
	.playback_stop = snd_card_mxc_audio_playback_bridge_stop,
	.playback_device_number = VOICE_CODEC,
	.playback_stream_id = 2,

	.capture_open = snd_card_mxc_audio_capture_bridge_open,
	.capture_close = snd_card_mxc_audio_capture_bridge_close,
	.capture_start = snd_card_mxc_audio_capture_bridge_start,
	.capture_stop = snd_card_mxc_audio_capture_bridge_stop,
	.capture_device_number = VOICE_CODEC,
	.capture_stream_id = 1,
};

#endif /* CONFIG_SND_AUDIO_BRIDGE */






/*!
  * This structure is the list of operation that the driver
  * must provide for the capture interface
  */
static struct snd_pcm_ops snd_card_mxc_audio_capture_ops = {
	.open = snd_card_mxc_audio_capture_open,
	.close = snd_card_mxc_audio_capture_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_mxc_audio_hw_params,
	.hw_free = snd_mxc_audio_hw_free,
	.prepare = snd_mxc_audio_capture_prepare,
	.trigger = snd_mxc_audio_capture_trigger,
	.pointer = snd_mxc_audio_capture_pointer,
};

/*!
  * This structure is the list of operation that the driver
  * must provide for the playback interface
  */
static struct snd_pcm_ops snd_card_mxc_audio_playback_ops = {
	.open = snd_card_mxc_audio_playback_open,
	.close = snd_card_mxc_audio_playback_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_mxc_audio_hw_params,
	.hw_free = snd_mxc_audio_hw_free,
	.prepare = snd_mxc_audio_playback_prepare,
	.trigger = snd_mxc_audio_playback_trigger,
	.pointer = snd_mxc_audio_playback_pointer,
};



/*!
  * This functions initializes the capture audio device supported by
  * PMIC IC.
  *
  * @param	mxc_audio	pointer to the sound card structure
  *
  */
void init_device_capture(mxc_pmic_audio_t * mxc_audio)
{
	audio_stream_t *audio_stream = NULL;
	pmic_audio_device_t *pmic_device = NULL;

	audio_stream = &mxc_audio->s[SNDRV_PCM_STREAM_CAPTURE];
	pmic_device = &audio_stream->pmic_audio_device;

	/* These parameters defines the identity of
	 * the device (codec or stereodac)
	 */
#if defined(CONFIG_MACH_MX27MEDIAPHONE)
        audio_stream->ssi = SSI2;
	pmic_device->ssi = SSI2;
#else
	audio_stream->ssi = SSI1;
	pmic_device->ssi = SSI1;
#endif
	// SAGEM: Audio port mapping is inverted on Homescreen
#if defined(CONFIG_MACH_MX31HSV1)
	audio_stream->dam_port = DAM_PORT_5;
#else
	audio_stream->dam_port = DAM_PORT_4;
#endif


	pmic_device->mode = BUS_MASTER_MODE;
	pmic_device->protocol = NETWORK_MODE;

	if (machine_is_mx31ads()) {
		pr_debug("%s - clock=CLIB",__FUNCTION__);
		pmic_device->pll = CLOCK_IN_CLIB;
	} else {
		pr_debug("%s - clock=CLIA",__FUNCTION__);
		pmic_device->pll = CLOCK_IN_CLIA;
	}
	pmic_device->pll_rate = VCODEC_CLI_26MHZ;
	pmic_device->bcl_inverted = 0;
	pmic_device->fs_inverted = 0;
}

/*!
  * This functions initializes the playback audio device supported by
  * PMIC IC.
  *
  * @param	mxc_audio	pointer to the sound card structure.
  * @param	device		device ID of PCM instance.
  *
  */
void init_device_playback(mxc_pmic_audio_t * mxc_audio, int device)
{
	audio_stream_t *audio_stream = NULL;
	pmic_audio_device_t *pmic_device = NULL;
	
	if (device == 0)
		audio_stream = &mxc_audio->s[0];
	else
		audio_stream = &mxc_audio->s[2];
	pmic_device = &audio_stream->pmic_audio_device;

	/* These parameters defines the identity of
	 * the device (codec or stereodac)
	 */
	if (device == 0) {
		// SAGEM: Audio port mapping is inverted on Homescreen
		audio_stream->ssi = SSI2;
		pmic_device->ssi = SSI2;
#if defined(CONFIG_MACH_MX31HSV1)
		audio_stream->dam_port = DAM_PORT_4;
#else
		audio_stream->dam_port = DAM_PORT_5;
#endif

		pmic_device->mode = BUS_MASTER_MODE;
		pmic_device->protocol = NETWORK_MODE;

		if (machine_is_mx31ads()) {
			pr_debug("%s - clock=CLIB",__FUNCTION__);
			pmic_device->pll = CLOCK_IN_CLIB;
		} else {
			pr_debug("%s - clock=CLIA",__FUNCTION__);
			pmic_device->pll = CLOCK_IN_CLIA;
		}
		pmic_device->pll_rate = STDAC_CLI_26MHZ;

		pmic_device->bcl_inverted = 0;
		pmic_device->fs_inverted = 0;

	} else if (device == 1) {
#if defined(CONFIG_MACH_MX27MEDIAPHONE)
                audio_stream->ssi = SSI2;
		pmic_device->ssi = SSI2;
#else
		audio_stream->ssi = SSI1;
		pmic_device->ssi = SSI1;
#endif
		// SAGEM: Audio port mapping is inverted on Homescreen
#if defined(CONFIG_MACH_MX31HSV1)
		audio_stream->dam_port = DAM_PORT_5;
#else
		audio_stream->dam_port = DAM_PORT_4;
#endif

		pmic_device->mode = BUS_MASTER_MODE;
		pmic_device->protocol = NETWORK_MODE;

		if (machine_is_mx31ads()) {
			pr_debug("%s - clock=CLIB",__FUNCTION__);
			pmic_device->pll = CLOCK_IN_CLIB;
		} else {
			pr_debug("%s - clock=CLIA",__FUNCTION__);
			pmic_device->pll = CLOCK_IN_CLIA;
		}
		pmic_device->pll_rate = VCODEC_CLI_26MHZ;
		pmic_device->bcl_inverted = 0;
		pmic_device->fs_inverted = 0;
	}
}

/*!
 * This functions initializes the mixer related information
 *
 * @param	mxc_audio	pointer to the sound card structure.
 *
 */
void mxc_pmic_mixer_controls_init(mxc_pmic_audio_t * mxc_audio)
{
	audio_mixer_control_t *audio_control = NULL;
	int i = 0;

	audio_control = &audio_mixer_control;

	memset(audio_control, 0, sizeof(audio_mixer_control_t));
	sema_init(&audio_control->sem, 1);

	audio_control->input_device = SOUND_MASK_MIC;
	audio_control->output_device = SOUND_MASK_VOLUME | SOUND_MASK_PCM;

	/* PMIC has to internal sources that can be routed to output
	   One is codec direct out and the other is mixer out
	   Initially we configure all outputs to have no source and
	   will be later configured either by PCM stream handler or mixer */
	for (i = 0; i < OP_MAXDEV && i != OP_HEADSET; i++) {
		audio_control->source_for_output[i] = MIXER_OUT;
	}

	/* These bits are initially reset and set when playback begins */
	audio_control->codec_out_to_mixer = 0;
	audio_control->stdac_out_to_mixer = 0;

	audio_control->mixer_balance = 50;
	if (machine_is_mx31ads())
		audio_control->mixer_mono_adder = STEREO_OPPOSITE_PHASE;
	else
		audio_control->mixer_mono_adder = MONO_ADDER_OFF;
	/* Default values for input and output */
#ifdef CONFIG_MACH_MX31HSV1
	{
		int default_input_volume;
		int default_output_volume;
		default_output_volume=(vg_pmic_output_volume_default * OUTPUT_VOLUME_MAX + 50)/vg_pmic_output_volume_max;
		if(default_output_volume > 100)
			default_output_volume = 100;
		default_input_volume=(vg_pmic_input_volume_default * INPUT_VOLUME_MAX + 50)/vg_pmic_input_volume_max;
		if(default_input_volume > 100)
			default_input_volume = 100;

#ifdef USE_NEW_MIXER_FEATURE
		audio_control->voice_codec_volume_out = audio_control->stereo_dac_volume_out = ((default_output_volume << 8) & 0xff00) | (default_output_volume & 0x00ff);
		audio_control->input_volume = ((default_input_volume << 8) & 0xff00) | (default_input_volume & 0x00ff);
#else

		audio_control->master_volume_out = ((default_output_volume << 8) & 0xff00) | (default_output_volume & 0x00ff);
		audio_control->input_volume = ((default_input_volume << 8) & 0xff00) | (default_input_volume & 0x00ff);
#endif
		pr_debug("%s - default_output_volume (%d) audio_control->voice_codec_volume_out (%x)",__FUNCTION__, default_output_volume, audio_control->voice_codec_volume_out);
		pr_debug("%s - default_input_volume (%d) audio_control->input_volume (%x)",__FUNCTION__, default_input_volume, audio_control->input_volume);
	}
#else
	audio_control->input_volume = ((100 << 8) & 0xff00) | (100 & 0x00ff);
	audio_control->master_volume_out = ((50 << 8) & 0xff00) | (50 & 0x00ff);
#endif // CONFIG_MACH_MX31HSV1

// Normalement if faut attendre un max de 250 ms pour que les signaux soient
// stabilisés ... mais le boot dure encore plus de 15 secondes ...
//  printk("mxc_pmic_mixer_controls_init()");
//  if (PMIC_SUCCESS != pmic_audio_set_autodetect(1))
// 		msleep(30);
  pmic_audio_set_autodetect(1);
}



/*!
 * This functions initializes the 2 audio devices supported by
 * PMIC IC. The parameters define the type of device (CODEC or STEREODAC)
 *
 * @param	mxc_audio	pointer to the sound card structure.
 * @param	device	        device id of the PCM stream.
 *
 */
void mxc_pmic_audio_init(mxc_pmic_audio_t * mxc_audio, int device)
{
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	if (device == 0) {
		mxc_audio->s[SNDRV_PCM_STREAM_PLAYBACK].id = "Audio out";
		mxc_audio->s[SNDRV_PCM_STREAM_PLAYBACK].stream_id =
		    SNDRV_PCM_STREAM_PLAYBACK;  //0
	} else if (device == 1) {
		mxc_audio->s[SNDRV_PCM_STREAM_CAPTURE].id = "Audio in";
		mxc_audio->s[SNDRV_PCM_STREAM_CAPTURE].stream_id =
		    SNDRV_PCM_STREAM_CAPTURE;   //1

		mxc_audio->s[2].id = "Audio out";
		mxc_audio->s[2].stream_id = 2;
	}
#else
	if (device == 0) {
		mxc_audio->s[SNDRV_PCM_STREAM_PLAYBACK].id = "Audio out";
		mxc_audio->s[SNDRV_PCM_STREAM_PLAYBACK].stream_id =
		    SNDRV_PCM_STREAM_PLAYBACK;
		mxc_audio->s[SNDRV_PCM_STREAM_CAPTURE].id = "Audio in";
		mxc_audio->s[SNDRV_PCM_STREAM_CAPTURE].stream_id =
		    SNDRV_PCM_STREAM_CAPTURE;
	} else if (device == 1) {
		mxc_audio->s[2].id = "Audio out";
		mxc_audio->s[2].stream_id = 2;
	}
#endif

	init_device_playback(mxc_audio, device);
	if (!device) {
		init_device_capture(mxc_audio);
	}
}

/*!
  * This function the soundcard structure.
  *
  * @param	mxc_audio	pointer to the sound card structure.
  * @param	device		the device index (zero based)
  *
  * @return              0 on success, -1 otherwise.
  */
static int __init snd_card_mxc_audio_pcm(mxc_pmic_audio_t * mxc_audio,
					 int device)
{
	snd_pcm_t *pcm = NULL;
	int err= -1;

	pr_debug("%s:%d - Enter",__FUNCTION__,__LINE__);
	pr_debug("%s:%d - Add device 0",__FUNCTION__,__LINE__);
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	/*
	 * Create a new PCM instancefor PCM playback on stereo DAC
	 */
	if ((err = snd_pcm_new(mxc_audio->card, "MXC", device, 1, 0, &pcm)) < 0) {
		return err;
	}
#else
	/*
	 * Create a new PCM instance with 1 capture stream and 1 playback substream
	 */
	if ((err = snd_pcm_new(mxc_audio->card, "MXC", device, 1, 1, &pcm)) < 0) {
		return err;
	}
#endif

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
			&snd_card_mxc_audio_playback_ops);
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			&snd_card_mxc_audio_capture_ops);
#endif

	pcm->private_data = mxc_audio;
	pcm->info_flags = 0;
	strncpy(pcm->name, SOUND_CARD_NAME, sizeof(pcm->name));
	mxc_audio->pcm[device] = pcm;
	mxc_pmic_audio_init(mxc_audio, device);

	pr_debug("%s:%d - Add device 1",__FUNCTION__,__LINE__);
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	/* Allocating a second device for PCM instance with 1 capture stream and 1 playback substream on voice codec*/

	device = 1;
	if ((err = snd_pcm_new(mxc_audio->card, "MXC", device, 1,1, &pcm)) < 0) {
		return err;
	}
#else
	/* Allocating a second device for PCM playback on voice codec */
	device = 1;
	if ((err = snd_pcm_new(mxc_audio->card, "MXC", device, 1, 0, &pcm)) < 0) {
		return err;
	}
#endif
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
					      snd_dma_continuous_data
					      (GFP_KERNEL), MAX_BUFFER_SIZE * 2,
					      MAX_BUFFER_SIZE * 2);

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_card_mxc_audio_playback_ops);
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			&snd_card_mxc_audio_capture_ops);
#endif

	pcm->private_data = mxc_audio;
	pcm->info_flags = 0;
	strncpy(pcm->name, SOUND_CARD_NAME, sizeof(pcm->name));
	mxc_audio->pcm[device] = pcm;
	mxc_pmic_audio_init(mxc_audio, device);
	/* End of allocation */
	/* FGA for record and not hard coded playback */
	mxc_pmic_mixer_controls_init(mxc_audio);

	pr_debug("%s:%d - Exit",__FUNCTION__,__LINE__);

	return 0;
}

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
static int snd_mxc_audio_suspend(struct platform_device *dev,
				 pm_message_t state)
{
	struct snd_card *card = platform_get_drvdata(dev);
	mxc_pmic_audio_t *chip = card->private_data;

	snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);
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
static int snd_mxc_audio_resume(struct platform_device *dev)
{
	struct snd_card *card = platform_get_drvdata(dev);

	snd_power_change_state(card, SNDRV_CTL_POWER_D0);

	return 0;
}
#endif				/* COMFIG_PM */

/*!
  * This function frees the sound card structure
  *
  * @param	card	pointer to the sound card structure.
  *
  * @return              0 on success, -1 otherwise.
  */
void snd_mxc_audio_free(snd_card_t * card)
{
	mxc_pmic_audio_t *chip = NULL;

	chip = card->private_data;
	audio_dma_free(&chip->s[SNDRV_PCM_STREAM_PLAYBACK]);
	audio_dma_free(&chip->s[SNDRV_PCM_STREAM_CAPTURE]);
	mxc_audio = NULL;
	card->private_data = NULL;
	kfree(chip);

}

/*!
  * This function initializes the driver in terms of memory of the soundcard
  * and some basic HW clock settings.
  *
  * @return              0 on success, -1 otherwise.
  */
static int __init mxc_alsa_audio_probe(struct platform_device *dev)
{
	int err = -1;
#if defined(CONFIG_SND_AUDIO_BRIDGE)
	int i;
#endif
	snd_card_t *card = NULL;

	/* register the soundcard */
	card = snd_card_new(-1, id, THIS_MODULE, sizeof(mxc_pmic_audio_t));
	if (card == NULL) {
		return -ENOMEM;
	}

	mxc_audio = kcalloc(1, sizeof(*mxc_audio), GFP_KERNEL);
	if (mxc_audio == NULL) {
		return -ENOMEM;
	}

	card->private_data = (void *)mxc_audio;
	card->private_free = snd_mxc_audio_free;

	mxc_audio->card = card;
	card->dev = &dev->dev;
	if ((err = snd_card_mxc_audio_pcm(mxc_audio, 0)) < 0) {
		goto nodev;
	}

	if (0 == mxc_alsa_create_ctl(card, (void *)&audio_mixer_control))
		printk(KERN_INFO "Control ALSA component registered\n");

	pmic_audio_antipop_enable(ANTI_POP_RAMP_SLOW);


	/* Set autodetect feature in order to allow audio operations */
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
	pmic_audio_set_callback(HSCallback_change, HEADSET_DETECTED|HEADSET_REMOVED|HEADSET_STEREO);
	pmic_audio_get_headset_state(NULL, NULL); /*Update current known headset state*/
	err = headset_plugged_sysdev_init();
	if(err){
		pr_error("Failed to initialize headset state sysfs file");
		return err;
	}
#endif

#if defined(CONFIG_MACH_MX31HSV1) && defined(CONFIG_SND_AUDIO_BRIDGE)

	err = sysdev_init_mads();
	if(err){
		goto nodev;
	}

	pr_info("Registering mxc_pmic (%p) in bridge", mxc_audio);
	sema_init(&mxc_audio->bridge_mutex, 1);
	mxc_audio->playback_state = LOCKED_BY_NONE;
	mxc_audio->capture_state = LOCKED_BY_NONE;
	mxc_audio->bridge_ops = &snd_card_bridge_fops;
	mxc_audio->bridge_ops->private_data = mxc_audio;
	mxc_audio->bridge_handle = register_audio_bridge(mxc_audio->bridge_ops);
	if(!mxc_audio->bridge_handle){
		pr_error("Could not register driver");
		err = -ENODEV;
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
		headset_plugged_sysdev_exit();
#endif
		sysdev_exit_mads();
		goto nodev;
	}

	pr_info("bridge ctx (%p)", mxc_audio->bridge_handle);

	for(i = 0; i < MXC_ALSA_MAX_CAPTURE + MXC_ALSA_MAX_PLAYBACK; i++){
		mxc_audio->s[i].stream_bridge_ops = &(snd_card_bridge_fops);
		INIT_LIST_HEAD(&(mxc_audio->s[i].capture_dma_addr_list_head));
		INIT_LIST_HEAD(&(mxc_audio->s[i].playback_dma_addr_list_head));
	}

#endif

	strcpy(card->driver, "MXC");
	strcpy(card->shortname, "PMIC-audio");
	sprintf(card->longname, "MXC Freescale with PMIC");

	if ((err = snd_card_register(card)) == 0) {
		pr_debug("MXC audio support initialized");
		platform_set_drvdata(dev, card);
		return 0;
	}

      nodev:
	snd_card_free(card);
	return err;
}



static int mxc_alsa_audio_remove(struct platform_device *dev)
{
#if defined(CONFIG_MACH_MX31HSV1) && defined(CONFIG_SND_AUDIO_BRIDGE)
	sysdev_exit_mads();
#endif
#ifdef CONFIG_MXC_HEADSET_DETECT_ENABLE
		headset_plugged_sysdev_exit();
#endif

	pmic_audio_antipop_disable();

	snd_card_free(mxc_audio->card);
	kfree(mxc_audio);
	platform_set_drvdata(dev, NULL);

	return 0;
}

#define mxc_ALSA "mxc_ALSA"

static struct platform_driver mxc_alsa_audio_driver = {
	.probe = mxc_alsa_audio_probe,
	.remove = mxc_alsa_audio_remove,
#ifdef CONFIG_PM
	.suspend = snd_mxc_audio_suspend,
	.resume = snd_mxc_audio_resume,
#endif
	.driver = {
		   .name = "mxc_ALSA",
		   },
};


static int __init mxc_alsa_audio_init(void)
{
	int err;
	pr_debug("Enter");
#ifdef DUMP_AUDIO_DATA
	pg_relay_ch=relay_open("pcm_data",
			 NULL,
			 262144,
			 10,
			 &relay_callbacks);
#endif //#ifdef DUMP_AUDIO_DATA
#ifdef DUMP_SINUS
	{
		int i;
		for (i=0;i<8192+50;i+=2)
		{
			ag_buffer_sinus[i]=2*ag_sinus[(i>>1)%50];
			ag_buffer_sinus[i+1]=2*ag_sinus[(i>>1)%50];
		}
	}
#endif

#ifdef CONFIG_FACTORY_ZONE
	{
		int tmp;
		static factory_parameter_t parameter;
		int result;

		strcpy(&parameter.name[0], "MAX_PLAYBACK_PGA_GAIN");
		strcpy(&parameter.application[0], "mxc_alsa_pmic");
		strcpy(&parameter.version[0], "");
		result = factory_parse_parameter(&parameter);
		if(result){
			pr_error("Could not get factory value for MAX_PLAYBACK_PGA_GAIN");
		} else {
			vg_pmic_output_volume_max = (int)simple_strtoul(&parameter.value[0], NULL, 0);
		}
                if (vg_pmic_output_volume_max > vg_pmic_output_volume_max_cap)
		{
			vg_pmic_output_volume_max = vg_pmic_output_volume_max_cap;
		}

		strcpy(&parameter.name[0], "MAX_CAPTURE_MIC_GAIN");
		strcpy(&parameter.application[0], "mxc_alsa_pmic");
		strcpy(&parameter.version[0], "");
		result = factory_parse_parameter(&parameter);
		if(result){
			pr_error("Could not get factory value for MAX_CAPTURE_MIC_GAIN");
		} else {
			vg_pmic_input_volume_max = (int)simple_strtoul(&parameter.value[0], NULL, 0);
		}
                if (vg_pmic_input_volume_max > vg_pmic_input_volume_max_cap)
		{
			vg_pmic_input_volume_max = vg_pmic_input_volume_max_cap;
		}

		strcpy(&parameter.name[0], "DEFAULT_CAPTURE_VOLUME");
		strcpy(&parameter.application[0], "mxc_alsa_pmic");
		strcpy(&parameter.version[0], "");
		result = factory_parse_parameter(&parameter);
		if(result){
			pr_error("Could not get factory value for DEFAULT_CAPTURE_VOLUME");
		} else {
			vg_pmic_input_volume_default = (int)simple_strtoul(&parameter.value[0], NULL, 0);
		}
                if (vg_pmic_input_volume_default > vg_pmic_input_volume_max)
		{
			vg_pmic_input_volume_default = vg_pmic_input_volume_max;
		}

		strcpy(&parameter.name[0], "DEFAULT_PLAYBACK_VOLUME");
		strcpy(&parameter.application[0], "mxc_alsa_pmic");
		strcpy(&parameter.version[0], "");
		result = factory_parse_parameter(&parameter);
		if(result){
			pr_error("Could not get factory value for DEFAULT_PLAYBACK_VOLUME");
		} else {
			vg_pmic_output_volume_default = (int)simple_strtoul(&parameter.value[0], NULL, 0);
		}
                if (vg_pmic_output_volume_default > vg_pmic_output_volume_max)
		{
			vg_pmic_output_volume_default = vg_pmic_output_volume_max;
		}


		strcpy(&parameter.name[0], "MAX_HEADSET_VOLUME");
		strcpy(&parameter.application[0], "mxc_alsa_pmic");
		strcpy(&parameter.version[0], "");
		result = factory_parse_parameter(&parameter);
		if(result){
			pr_error("Could not get factory value for MAX_HEADSET_VOLUME");
		} else {
			tmp = (int)simple_strtoul(&parameter.value[0], NULL, 0);
			if(tmp){
				vg_pmic_headset_volume_max = tmp;
			}else{
				printk(KERN_ERR "Could not set headset max volume. "
						"Bad value read from factory zone (%d). "
						"Please update it. Setting to default (%d)\n",
						tmp, vg_pmic_headset_volume_max);
			}
		}
		if (vg_pmic_headset_volume_max > vg_pmic_headset_volume_max_cap)
		{
			vg_pmic_headset_volume_max = vg_pmic_headset_volume_max_cap;
		}

		printk("Volumes : output : cap (%d) max (%d) default (%d)\n", 
				vg_pmic_output_volume_max_cap, 
				vg_pmic_output_volume_max, 
				vg_pmic_output_volume_default);
		printk("Volumes : input : cap (%d) max (%d) default (%d)\n", 
				vg_pmic_input_volume_max_cap, 
				vg_pmic_input_volume_max, 
				vg_pmic_input_volume_default);

		printk("Volumes : headset : cap (%d) max (%d)\n", 
				vg_pmic_headset_volume_max_cap,
				vg_pmic_headset_volume_max);

	}
#endif


	err = platform_driver_register(&mxc_alsa_audio_driver);
	if(err < 0){
		pr_error("Could not register driver");
		return err;
	}

	device = platform_device_register_simple(mxc_ALSA, -1, NULL, 0);
	if(IS_ERR(device)){
		pr_error("Could not register device");
		platform_device_unregister(device);
		platform_driver_unregister(&mxc_alsa_audio_driver);
		return -ENODEV;
	}

	if (!platform_get_drvdata(device)){
		pr_error("Could not get driver data");
		platform_device_unregister(device);
		platform_driver_unregister(&mxc_alsa_audio_driver);
		return -ENODEV;
	}

	pr_debug("Exit Success");
	return 0;
}

/*!
  * This function frees the sound driver structure.
  *
  */

static void __exit mxc_alsa_audio_exit(void)
{
	platform_device_unregister(device);
	platform_driver_unregister(&mxc_alsa_audio_driver);

#ifdef DUMP_AUDIO_DATA
	relay_close(pg_relay_ch);
	pg_relay_ch=NULL;
#endif //#ifdef DUMP_AUDIO_DATA
}

module_init(mxc_alsa_audio_init);
module_exit(mxc_alsa_audio_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MXC driver for ALSA");
MODULE_SUPPORTED_DEVICE("{{PMIC}}");

module_param(id, charp, 0444);
MODULE_PARM_DESC(id, "ID string for MXC  + PMIC soundcard.");
