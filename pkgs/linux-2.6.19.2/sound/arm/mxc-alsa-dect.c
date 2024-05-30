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
#include "asm-arm/arch-mxc/gpio.h"

#ifdef CONFIG_PM
#include <linux/pm.h>
#endif				/* CONFIG_PM */

#include <asm/arch/dma.h>
#include <asm/arch/spba.h>
#include <asm/arch/clock.h>
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

//#define TRACE_DMA

#if 0
#undef pr_debug 
#undef pr_error
#define pr_debug(x...) 		{ printk("[MXC_DECT_TRACE] - "); printk(x);}
#define pr_error(x...)    	{ printk("[MXC_DECT_ERROR] - "); printk(x);}
#else
#define pr_debug(x...)	
#define pr_error(x...)      { printk("[MXC_DECT_ERROR] - "); printk(x);}
#endif //TRACE_DMA

#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_AUDIO)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

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

typedef enum
{
	DECT_MASTER=1,
	DECT_SLAVE=2
} e_SyncMode;

#define TX_WATERMARK					0x4
#define RX_WATERMARK					0x6
#define DECT_SAMPLE_RATE_MAX					0x1
/*!
  * ID for this card 
  */
static char *id = NULL;

#define MXC_ALSA_MAX_PCM_DEV 2
#define MXC_ALSA_MAX_PLAYBACK 2
#define MXC_ALSA_MAX_CAPTURE 2

#define DECT_STREAM_PLAYBACK_CHANNEL_0	0
#define DECT_STREAM_CAPTURE_CHANNEL_0	1

#define DECT_STREAM_PLAYBACK_CHANNEL_1	2
#define DECT_STREAM_CAPTURE_CHANNEL_1	3

#if defined(CONFIG_ARCH_MX27)
/*!
 * These macros define the divider used to get correct
 * SSI main clock frequency. This is used only when MCU
 * is the clock provider (dect slave mode) 
 */
#define CCM_DIVIDER_8KHZ                                8

#define PRESCALER_8KHZ                                  142

#endif				/* CONFIG_ARCH_MX27 */

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

	/*!
	 * for locking in DMA operations 
	 */
	spinlock_t dma_lock;

	/*!
	 * Alsa substream pointer
	 */
	snd_pcm_substream_t *stream;
} audio_stream_t;

/*!
  * This structure represents the dect sound card with its
  * 2 streams and its shared parameters
  */
typedef struct snd_card_mxc_audio_dect {
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
	 */
	audio_stream_t s[MXC_ALSA_MAX_CAPTURE + MXC_ALSA_MAX_PLAYBACK];
	/* card syncho mode*/
	e_SyncMode synchro_mode;

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
static snd_pcm_hw_constraint_list_t hw_playback_rates = {
	.count = ARRAY_SIZE(playback_rates),
	.list = playback_rates,
	.mask = 0,
};

/*!
  * this structure represents the sample rates supported
  * by DECT for capture operations.
  */
static snd_pcm_hw_constraint_list_t hw_capture_rates = {
	.count = ARRAY_SIZE(capture_rates),
	.list = capture_rates,
	.mask = 0,
};
/*!
 * Structure used to store precalculated values to get
 * correct Bit Clock and Frame Sync when playing sound
 * in MCU master mode.
 *
 * Remember that ccm_div field values have
 * been multiplied by 2 for FP accuracy (i.e if we
 * want divide the clock by 10, we must pass
 * value 20 to mxc_set_clocks_div function)
 *
 * MXC driver uses the following equations to calculate FS and BC clocks:
 *
 * bit_clock = ssi_clock / [(div2 + 1) x (7 x psr + 1) x (prescaler + 1) x 2]
 * where
 *
 * ssi_clock = SSIx_BAUD/ccm_divider (x in SSIx_BAUD can be 1 or 2)
 * ccm_divider = defined in struct freq_coefficients.
 * prescaler = defined in struct freq_coefficients.
 * div2 = 0.
 * psr = 0.
 *
 * frame_sync_clock = (bit_clock) / [(frdc + 1) x word_length]
 * where
 * frdc = 1 for stereo, 0 for mono.
 * word_length = 16 (sample bits)
 *
 * Note: SSIx_BAUD is sourced by SERIALPLL.
 */
#if defined(CONFIG_ARCH_MX27)
static struct freq_coefficients freq_values[DECT_SAMPLE_RATE_MAX] = {
	{8000, CCM_DIVIDER_8KHZ, PRESCALER_8KHZ},
};
#endif

/*!
 * This function returns the optimal values to
 * get a desired sample rate (among supported ones)
 * Values stored in freq_coefficients array are valid if
 * the following conditions are honored:
 *
 * 1 - DIV2 bit in SSI STCCR register (SSI Transmit and
 *     Receive Clock Control Registers) is set to 0.
 * 2 - PSR bit in SSI STCCR is set to 0.
 * 3 - DC bits (frame rate divider) in SSI STCCR are set to 0 for mono audio or
 *     1 for stereo audio.
 * 4 - WL bits in SSI STCCR are set to 7 (16 bit word length)
 *
 * @param       sample_rate:    sample rate requested.
 *
 * @return      This function returns a structure holding the correct
 *              values to set BC and FS clocks for this sample rate,
 *              NULL if an unsupported sample rate is requested.
 */

#if defined(CONFIG_ARCH_MX27)
inline struct freq_coefficients *get_ccm_divider(int sample_rate)
{
	int i;

	for (i = 0; i < DECT_SAMPLE_RATE_MAX; i++) {
		if (freq_values[i].sample_rate == sample_rate) {
			return &freq_values[i];
		}
	}
	return NULL;
}
#endif

/*!
  * This function configures audio multiplexer to support 
  * audio data routing in case of slave DECT card (IMX master)
  *
  * @param       ssi	SSI of the ARM to connect to the DAM.
  */
void configure_dam_dect_slave(int ssi)
{
	int source_port;
	int target_port;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	pr_debug("%s - ssi=%d DAM: port 1 -> port 6\n",__FUNCTION__,ssi);
	if (ssi == SSI1)
		source_port = port_1;
	else
		source_port = port_2;

	target_port = port_6;

	dam_reset_register(source_port);
	dam_reset_register(target_port);

	// set port 1 (SSI-1) in internal network mode
	dam_select_mode(source_port, normal_mode);
	dam_select_mode(target_port, normal_mode);

	// tx clock and rx clock are synchronous
	dam_set_synchronous(source_port, true);
	dam_set_synchronous(target_port, true);

	// select sources to receive data
	dam_select_RxD_source(source_port, target_port);
	dam_select_RxD_source(target_port, source_port);

	// port 6 is slave : fs and clock are generated by port1
 	dam_select_TxFS_direction(target_port, signal_out);
 	dam_select_TxFS_direction(source_port, signal_in);
 	dam_select_TxFS_source(target_port, false, source_port);

	dam_select_TxClk_direction(target_port, signal_out);
	dam_select_TxClk_direction(source_port, signal_in);
	dam_select_TxClk_source(target_port, false, source_port);

	dam_select_RxFS_direction(target_port, signal_out);
	dam_select_RxFS_direction(source_port, signal_in);
	dam_select_RxFS_source(target_port, false, source_port);

	dam_select_RxClk_direction(target_port, signal_out);
	dam_select_RxClk_direction(source_port, signal_in);
	dam_select_RxClk_source(target_port, false, source_port);

    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
}

/*!
  * This function configures audio multiplexer to support 
  * audio data routing in case of master DECT card (IMX slave)
  *
  * @param       ssi	SSI of the ARM to connect to the DAM.
  */
void configure_dam_dect_master(int ssi)
{
	int source_port;
	int target_port;
pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	pr_debug("%s - ssi=%d DAM: port 1 -> port 6\n",__FUNCTION__,ssi);
	if (ssi == SSI1)
		source_port = port_1;
	else
		source_port = port_2;

	target_port = port_6;

	dam_reset_register(source_port);
	dam_reset_register(target_port);

	dam_select_mode(source_port, normal_mode);
	dam_select_mode(target_port, normal_mode);

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

    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
}
/*!
 * This function applies precalculated values for
 * each supported audio frequency. This settings
 * are applied only if MCU master mode is selected.
 *
 * @param       device  pointer to the structure of the current device.
 */
/*void set_audio_clocks(snd_pcm_substream_t *substream)
{
	struct freq_coefficients *ccm_div;
	unsigned long clock = 0;
	int ssi_clock;
	unsigned long mcu_pll;

	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	snd_pcm_runtime_t *runtime;
	int ssi;

	int direction = substream->pstr->stream;
	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[direction];
	runtime = substream->runtime;
	ssi = s->ssi;

	// select serial clock

	pr_debug("direction  = %d\n", (int)direction);

	// only tx frame need to be programmed (synchronous mode)

//	if (direction == 0) {
		ssi_tx_frame_rate(ssi, 4); // select 4 slots by frame
//    	} else {
//    		ssi_rx_frame_rate(ssi, 4);
//   	}

	// get the ccm divider structure for 8 kHz clocking
	ccm_div = get_ccm_divider(8000);
	// get the ssi_clock depend on SSI 
	ssi_clock = (ssi == SSI1) ? SSI1_BAUD : SSI2_BAUD;

	// disable ssi clock 
	mxc_clks_disable(ssi_clock);

	mcu_pll = mxc_pll_clock(MCUPLL);
	if (mcu_pll > 300000000)
    {
		//set the clock divider for this audio session 
		mxc_set_clocks_div(ssi_clock, ((ccm_div->ccm_div * 3) / 2));
	}
	else
	{
		mxc_set_clocks_div(ssi_clock, ccm_div->ccm_div);
	}

	// enable clock 
	mxc_clks_enable(ssi_clock);

	  // Set prescaler value. Prescaler value is divided by
	 // 2 because ssi internally multiplies it by 2.
	 //

	// only tx prescaler need to be programmed (synchronous mode)
//	if (direction == 0) {
		ssi_tx_prescaler_modulus(ssi,
					 (unsigned char)(ccm_div->prescaler /
							 2));
//   	} else {
//   		ssi_rx_prescaler_modulus(ssi,
//   					 (unsigned char)(ccm_div->prescaler /
//   							 2));
//  	}

	pr_debug("prescaler value = %d\n", ccm_div->prescaler);

	// clocks are provided by MCU, thus we use "internally" mode 
	// only tx frame and clock direction need to be programmed (synchronous mode)
//	if (direction == 0) {
		ssi_tx_frame_direction(ssi, ssi_tx_rx_internally);
		ssi_tx_clock_direction(ssi, ssi_tx_rx_internally);
//  	} else {
//    		ssi_rx_frame_direction(ssi, ssi_tx_rx_internally);
//    		ssi_rx_clock_direction(ssi, ssi_tx_rx_internally);
//    	}
 
	clock = mxc_get_clocks(ssi_clock);
	pr_debug("Current SSI clock frequency = %d\n", (int)clock);
}*/

/*!
  * This function configures the SSI in order to receive audio 
  * from DECT (recording). Configuration of SSI consists mainly in 
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
void configure_ssi_rx_for_dect(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int ssi;
	int device, stream_id = -1;
	device = substream->pcm->device;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	// get the SSI
	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	ssi = s->ssi;

	pr_debug("%s - DECT configure_ssi_rx: SSI %d\n",__FUNCTION__, ssi + 1);

	ssi_enable(ssi, false);
	ssi_synchronous_mode(ssi, true);
	ssi_network_mode(ssi, true);
	
	/*For mx27,need also to config stcr and stccr */
	ssi_tx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_tx_frame_sync_length(ssi, ssi_frame_sync_one_bit);
	ssi_tx_clock_divide_by_two(ssi, 0);
	ssi_tx_clock_prescaler(ssi, 0);
	ssi_tx_word_length(ssi, ssi_16_bits);
	ssi_tx_frame_rate(ssi, 4);
	
	/*For mx27,need also to config stcr and stccr */
	ssi_rx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_rx_frame_sync_length(ssi, ssi_frame_sync_one_bit);

	// select two channel mode 
	ssi_two_channel_mode(ssi,true) ;

	// enable fifo 0
	ssi_rx_fifo_enable(ssi, ssi_fifo_0, true);
	ssi_rx_fifo_full_watermark(ssi, ssi_fifo_0, RX_WATERMARK) ;

	//enable fifo 1 (can only be used if fifo 0 is active)
	ssi_rx_fifo_enable(ssi, ssi_fifo_1, true);
	ssi_rx_fifo_full_watermark(ssi, ssi_fifo_1, RX_WATERMARK);
		
	ssi_rx_bit0(ssi, true);

	/* We never use the divider by 2 implemented in SSI */
	ssi_rx_clock_divide_by_two(ssi, 0);
	
	/* Set prescaler range (a fixed divide-by-eight prescaler 
	 * in series with the variable prescaler) to 0 as we don't 
	 * need it.
	 */
	ssi_rx_clock_prescaler(ssi, 0);
	
	/* Currently, only supported sample length is 16 bits */
	ssi_rx_word_length(ssi, ssi_16_bits);
	
	
	if (chip->synchro_mode == DECT_SLAVE)
	{
		pr_error("MCU master mode not supported on kernel 2.6.19\n");
		//set_audio_clocks(substream); // set BCL and FS clocks if MCU master mode 
	}
	else
	{
		if (chip->synchro_mode != DECT_MASTER)
		{
			pr_error("synchronization mode is not set, use DECT MASTER\n");
		}

		ssi_rx_frame_direction(ssi, ssi_tx_rx_externally);
		ssi_rx_clock_direction(ssi, ssi_tx_rx_externally);

		ssi_tx_frame_direction(ssi, ssi_tx_rx_externally);
		ssi_tx_clock_direction(ssi, ssi_tx_rx_externally);
		ssi_tx_frame_rate(ssi, 4);
		ssi_rx_frame_rate(ssi, 4);
	}
	
	ssi_enable(ssi, true);	
	pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
}

/*!
  * This function configures the SSI in order to
  * send data to DECT. Configuration of SSI consists
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
void configure_ssi_tx_for_dect(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	snd_pcm_runtime_t *runtime;
	int ssi;
	int device, stream_id = -1;
	device = substream->pcm->device;
    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	runtime = substream->runtime;
	ssi = s->ssi;
	
	pr_debug("%s - DECT configure_ssi_tx: SSI %d\n",__FUNCTION__, ssi + 1);

	ssi_enable(ssi, false);
	ssi_synchronous_mode(ssi, true);
	ssi_network_mode(ssi, true);

	ssi_tx_early_frame_sync(ssi, ssi_frame_sync_one_bit_before);
	ssi_tx_frame_sync_length(ssi, ssi_frame_sync_one_bit);

	// select two channel mode 
	ssi_two_channel_mode(ssi,true) ;

	ssi_tx_fifo_enable(ssi, ssi_fifo_0, true);
	ssi_tx_fifo_empty_watermark(ssi, ssi_fifo_0, TX_WATERMARK);
	ssi_tx_fifo_enable(ssi, ssi_fifo_1, true);
	ssi_tx_fifo_empty_watermark(ssi, ssi_fifo_1, TX_WATERMARK);
	
	ssi_tx_bit0(ssi, true);
		
	/* We never use the divider by 2 implemented in SSI */
	ssi_tx_clock_divide_by_two(ssi, 0);
	
	ssi_tx_clock_prescaler(ssi, 0);
	
	/*Currently, only supported sample length is 16 bits */
	ssi_tx_word_length(ssi, ssi_16_bits);
		
	if (chip->synchro_mode == DECT_SLAVE)
	{
		pr_error("MCU master mode not supported on kernel 2.6.19\n");
		//set_audio_clocks(substream); // set BCL and FS clocks if MCU master mode 
	}
	else
	{
		if (chip->synchro_mode != DECT_MASTER)
		{
			pr_error("synchronization mode is not set, use DECT MASTER\n");
		}
		ssi_rx_frame_direction(ssi, ssi_tx_rx_externally);
		ssi_rx_clock_direction(ssi, ssi_tx_rx_externally);

		ssi_tx_frame_direction(ssi, ssi_tx_rx_externally);
		ssi_tx_clock_direction(ssi, ssi_tx_rx_externally);

		ssi_tx_frame_rate(ssi, 4);
	}
	
	ssi_enable(ssi, true);
	pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
}

/*!
  * This function configures number of channels for next audio operation 
  * (recording/playback) Number of channels define if sound is stereo
  * or mono.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
void set_dect_channels(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	snd_pcm_runtime_t *runtime;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[substream->pstr->stream];
	runtime = substream->runtime;

	pr_debug("%s - nb of channels : %d stream=%d\n",__FUNCTION__, runtime->channels,substream->pstr->stream);

	if (runtime->channels == 2) {
		ssi_tx_mask_time_slot(s->ssi, MASK_4_TS);
		ssi_rx_mask_time_slot(s->ssi, MASK_4_TS);
	} else {		
		ssi_tx_mask_time_slot(s->ssi, MASK_2_TS);
		ssi_rx_mask_time_slot(s->ssi, MASK_2_TS);
	}

    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
}

/*!
  * This function configures the DMA channel used to transfer 
  * audio from MCU to DECT
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

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	pr_debug("DECT: configure_write_channel device (%d)\n", stream_id);

	/* channel 0 dma on register TX0
	   channel 1 dma on register TX1*/

	if (stream_id == DECT_STREAM_PLAYBACK_CHANNEL_0)
		channel = mxc_dma_request(MXC_DMA_SSI1_16BIT_TX0, "ALSA DECT TX0 DMA");
	else
		channel = mxc_dma_request(MXC_DMA_SSI1_16BIT_TX1, "ALSA DECT TX1 DMA");

	if (channel < 0) {
		pr_error(" when DECT requesting a write dma channel\n");
		return -1;
	}

	ret = mxc_dma_callback_set(channel, (mxc_dma_callback_t) callback,
				 (void *)s);

	if (ret != 0) {
		mxc_dma_free(channel);
		pr_error("mxc_dma_callback_set\n");
		return -1;
	}
	s->dma_wchannel = channel;

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_CONFIG_WRITE, 0) ;
#endif

	return 0;
}

/*!
  * This function configures the DMA channel used to transfer 
  * audio from DECT to MCU
  *
  * @param	substream	pointer to the structure of the current stream.
  * @param       callback        pointer to function that will be 
  *                              called when a SDMA RX transfer finishes.
  *
  * @return              0 on success, -1 otherwise.
  */
static int configure_read_channel(audio_stream_t * s,
				  mxc_dma_callback_t callback, int stream_id)
{
	int ret = -1;
	int channel = -1;

	/* channel 0 dma on register RX0
	   channel 1 dma on register RX1*/

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
    pr_debug("stream_id = %d \n", stream_id);
	if (stream_id == DECT_STREAM_CAPTURE_CHANNEL_0)
		channel = mxc_dma_request(MXC_DMA_SSI1_16BIT_RX0, "ALSA DECT RX0 DMA");
	else
		channel = mxc_dma_request(MXC_DMA_SSI1_16BIT_RX1, "ALSA DECT RX1 DMA");

	if (channel < 0) {
		pr_error(" requesting a read dma channel\n");
		return -1;
	}

	ret =
	    mxc_dma_callback_set(channel, (mxc_dma_callback_t) callback,
				 (void *)s);
	if (ret != 0) {
		mxc_dma_free(channel);
		pr_error("requesting a read dma channel\n");
		return -1;
	}
	s->dma_wchannel = channel;

    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_CONFIG_READ, 0) ;
#endif

	return 0;
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
static u_int audio_dect_get_capture_dma_pos(audio_stream_t * s)
{
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
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
		pr_debug("%s - capture audio_dect_get_dma_pos offset  %d\n",__FUNCTION__,offset);
#endif
	} else {
		offset = (runtime->period_size * (s->periods));
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
 		pr_debug("%s -capture audio_dect_get_dma_pos BIS offset  %d\n",__FUNCTION__, offset);
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
static u_int audio_dect_get_playback_dma_pos(audio_stream_t * s)
{
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
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
		pr_debug("%s - audio_dect_get_dma_pos offset  %d\n",__FUNCTION__, offset);
#endif
	} else {
		offset = (runtime->period_size * (s->periods));
		if (offset >= runtime->buffer_size)
			offset = 0;
#ifdef TRACE_DMA
		pr_debug("%s - audio_dect_get_dma_pos BIS offset  %d\n",__FUNCTION__, offset);
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
static void audio_dect_playback_stop_dma(audio_stream_t * s)
{
	unsigned long flags;
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
	unsigned int dma_size;
	unsigned int offset;

	substream = s->stream;
	runtime = substream->runtime;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * s->periods;

	spin_lock_irqsave(&s->dma_lock, flags);

	pr_debug("%s - audio_dect_stop_dma active = 0\n",__FUNCTION__);
	s->active = 0;
	s->period = 0;
	s->periods = 0;

	/* this stops the dma channel and clears the buffer ptrs */
	mxc_dma_disable(s->dma_wchannel);
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 DMA_TO_DEVICE);

	spin_unlock_irqrestore(&s->dma_lock, flags);
}

/*!
  * This function stops the current dma transfert for capture
  * and clears the dma pointers.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_dect_capture_stop_dma(audio_stream_t * s)
{
	unsigned long flags;
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
	unsigned int dma_size;
	unsigned int offset;

	substream = s->stream;
	runtime = substream->runtime;
	dma_size = frames_to_bytes(runtime, runtime->period_size);
	offset = dma_size * s->periods;

	spin_lock_irqsave(&s->dma_lock, flags);

	pr_debug("%s - capture audio_dect_stop_dma active = 0\n",__FUNCTION__);
	s->active = 0;
	s->period = 0;
	s->periods = 0;

	/* this stops the dma channel and clears the buffer ptrs */
	mxc_dma_disable(s->dma_wchannel);
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 DMA_FROM_DEVICE);

	spin_unlock_irqrestore(&s->dma_lock, flags);
}

/*!
  * This function is called whenever a new audio block needs to be
  * transferred to DECT. The function receives the address and the size 
  * of the new block and start a new DMA transfer.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_dect_playback_dma(audio_stream_t * s)
{
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
	unsigned int dma_size;
	unsigned int offset;
	int ret = 0;
	mxc_dma_requestbuf_t dma_request;
	int stream_id = -1;
	int device ;

	substream = s->stream;
	runtime = substream->runtime;
	
	device = substream->pcm->device;
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));
#ifdef TRACE_DMA
	pr_debug("DECT: audio_dect_playback_dma device (%d) stream (%d)\n", device, stream_id);
#endif

	if (s->active) {
		dma_size = frames_to_bytes(runtime, runtime->period_size);
#ifdef TRACE_DMA
		pr_debug("%s - s->period (%x) runtime->periods (%d)\n",__FUNCTION__,s->period, runtime->periods);
 		pr_debug("%s - runtime->period_size (%d) dma_size (%d)\n",__FUNCTION__,(unsigned int)runtime->period_size,runtime->dma_bytes);
#endif

		offset = dma_size * s->period;
		snd_assert(dma_size <= DMA_BUF_SIZE,);

		dma_request.src_addr = (dma_addr_t) (dma_map_single(NULL,
								    runtime->
								    dma_area +
								    offset,
								    dma_size,
								    DMA_TO_DEVICE));

		if (stream_id == DECT_STREAM_PLAYBACK_CHANNEL_1)
			dma_request.dst_addr = (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1STX1);
		else
			dma_request.dst_addr = (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1STX0);

		dma_request.num_of_bytes = dma_size;

#ifdef TRACE_DMA
 		pr_debug("%s - Start playback DMA offset (%d) size (%d)\n",__FUNCTION__ ,offset,runtime->dma_bytes);
#endif

		mxc_dma_config(s->dma_wchannel, &dma_request, 1,
			       MXC_DMA_MODE_WRITE);
		ret = mxc_dma_enable(s->dma_wchannel);
		//pr_debug("%s - Start playback DMA channel (%d)\n",__FUNCTION__, s->dma_wchannel);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_DMA_WRITE_REQ,(unsigned int)dma_request.num_of_bytes) ;
	ulog(ULOG_ALSA_DECT_DMA_WRITE_REQ_POS,(unsigned int)(runtime->dma_area + offset)) ;
#endif

		s->tx_spin = 1;	/* FGA little trick to retrieve DMA pos */

		if (ret) {
			pr_error("audio_process_dma: cannot queue DMA buffer (%i)\n",ret);
			return;
		}
		s->period++;
		s->period %= runtime->periods;

#ifdef MXC_SOUND_PLAYBACK_CHAIN_DMA_EN
		if ((s->period > s->periods) && ((s->period - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: already double buffered\n");
#endif
			return;
		}

		if ((s->period < s->periods)
		    && ((s->period + runtime->periods - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: already double buffered\n");
#endif
			return;
		}

		if (s->period == s->periods) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: s->period == s->periods\n");
#endif
			return;
		}

		if (snd_pcm_playback_hw_avail(runtime) <
		    2 * runtime->period_size) {
#ifdef TRACE_DMA
			pr_debug("audio playback chain dma: available data is not enough\n");
#endif
			return;
		}

#ifdef TRACE_DMA
		pr_debug("audio playback chain dma:to set up the 2nd dma buffer\n");
#endif
		offset = dma_size * s->period;
		dma_request.src_addr = (dma_addr_t) (dma_map_single(NULL,
								    runtime->
								    dma_area +
								    offset,
								    dma_size,
								    DMA_TO_DEVICE));
		mxc_dma_config(s->dma_wchannel, &dma_request, 1,
			       MXC_DMA_MODE_WRITE);
		ret = mxc_dma_enable(s->dma_wchannel);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_DMA_WRITE_REQ,(unsigned int)dma_request.num_of_bytes) ;
	ulog(ULOG_ALSA_DECT_DMA_WRITE_REQ_POS,(unsigned int)(runtime->dma_area + offset)) ;
#endif

		s->period++;
		s->period %= runtime->periods;
#endif				/* MXC_SOUND_PLAYBACK_CHAIN_DMA_EN */

	}

}

/*!
  * This function is called whenever a new audio block needs to be
  * transferred from DECT. The function receives the address and the size 
  * of the block that will store the audio samples and start a new DMA transfer.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static void audio_dect_capture_dma(audio_stream_t * s)
{
	snd_pcm_substream_t *substream;
	snd_pcm_runtime_t *runtime;
	unsigned int dma_size;
	unsigned int offset;
	int ret = 0;
	mxc_dma_requestbuf_t dma_request;
	int device; 
	int stream_id = -1;

	substream = s->stream;
	runtime = substream->runtime;
	
	device = substream->pcm->device;
#ifdef TRACE_DMA
	pr_debug("capture dma device %d\n", device);
#endif
	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));

	if (s->active) {
		dma_size = frames_to_bytes(runtime, runtime->period_size);

		offset = dma_size * s->period;
		snd_assert(dma_size <= DMA_BUF_SIZE,);

		dma_request.dst_addr = (dma_addr_t) (dma_map_single(NULL,
								    runtime->
								    dma_area +
								    offset,
								    dma_size,
								    DMA_FROM_DEVICE));
		/* channel 0 dma on register TX0
		   channel 1 dma on register TX1*/

		if (stream_id == DECT_STREAM_CAPTURE_CHANNEL_0)
			dma_request.src_addr = (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1SRX0);
		else
			dma_request.src_addr = (dma_addr_t) (SSI1_BASE_ADDR + MXC_SSI1SRX1);

		dma_request.num_of_bytes = dma_size;

#ifdef TRACE_DMA
		pr_debug("DECT: Start capture DMA offset (%d) size (%d)\n", offset,runtime->dma_bytes);
#endif

		mxc_dma_config(s->dma_wchannel, &dma_request, 1,
			       MXC_DMA_MODE_READ);
		ret = mxc_dma_enable(s->dma_wchannel);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_DMA_READ_REQ,(unsigned int)dma_request.num_of_bytes) ;
	ulog(ULOG_ALSA_DECT_DMA_READ_REQ_POS,(unsigned int)(runtime->dma_area + offset)) ;
#endif

#ifdef TRACE_DMA
		pr_debug("DECT: Start capture DMA channel (%d)\n", s->dma_wchannel);
		pr_debug("DECT: Start capture device (%d) stream (%d)\n", device, stream_id);
#endif
		s->tx_spin = 1;	/* FGA little trick to retrieve DMA pos */

		if (ret) {
			pr_error("DECT audio_process_dma: cannot queue DMA buffer (%i)\n",ret);
			return;
		}
		s->period++;
		s->period %= runtime->periods;

#ifdef MXC_SOUND_CAPTURE_CHAIN_DMA_EN
		if ((s->period > s->periods) && ((s->period - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: already double buffered\n");
#endif
			return;
		}

		if ((s->period < s->periods)
		    && ((s->period + runtime->periods - s->periods) > 1)) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: already double buffered\n");
#endif
			return;
		}

		if (s->period == s->periods) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: s->period == s->periods\n");
#endif
			return;
		}

		if (snd_pcm_capture_hw_avail(runtime) <
		    2 * runtime->period_size) {
#ifdef TRACE_DMA
			pr_debug
			    ("audio capture chain dma: available data is not enough\n");
#endif
			return;
		}

		//pr_debug("audio capture chain dma:to set up the 2nd dma buffer\n");
		offset = dma_size * s->period;
		dma_request.dst_addr = (dma_addr_t) (dma_map_single(NULL,
								    runtime->
								    dma_area +
								    offset,
								    dma_size,
								    DMA_FROM_DEVICE));
		mxc_dma_config(s->dma_wchannel, &dma_request, 1,
			       MXC_DMA_MODE_READ);
		ret = mxc_dma_enable(s->dma_wchannel);

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_DMA_READ_REQ,(unsigned int)dma_request.num_of_bytes) ;
	ulog(ULOG_ALSA_DECT_DMA_READ_REQ_POS,(unsigned int)(runtime->dma_area + offset)) ;
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
static void audio_dect_playback_dma_callback(void *data, int error,
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
	ulog(ULOG_ALSA_DECT_DMA_WRITE_CALLBACK,(unsigned int)(runtime->dma_area + offset)) ;
#endif

#ifdef TRACE_DMA
	pr_debug("%s\n",__FUNCTION__);
#endif
	/*
	 * Give back to the CPU the access to the non cached memory
	 */
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 DMA_TO_DEVICE);

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
	audio_dect_playback_dma(s);

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
static void audio_dect_capture_dma_callback(void *data, int error,
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

#ifdef TRACE_DMA
	pr_debug("%s\n",__FUNCTION__);
#endif

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_DMA_READ_CALLBACK,(unsigned int)(runtime->dma_area + offset)) ;
#endif

	/*
	 * Give back to the CPU the access to the non cached memory
	 */
	dma_unmap_single(NULL, runtime->dma_addr + offset, dma_size,
			 DMA_FROM_DEVICE);

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
	audio_dect_capture_dma(s);

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
snd_mxc_audio_dect_playback_trigger(snd_pcm_substream_t * substream, int cmd)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int err;
	int device;
	int stream_id = -1;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	device = substream->pcm->device;
	if (device == 0)
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	else if (device == 1)
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	else
	{
		pr_error(" unknown device, select 0\n");
		stream_id=0;
	}

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	err = 0;
	

	/* note local interrupts are already disabled in the midlevel code */
	spin_lock(&s->dma_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_START\n");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_WRITE_TRIGGER_START,0) ;
#endif
		s->tx_spin = 0;
		/* requested stream startup */
		s->active = 1;
		audio_dect_playback_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_STOP\n");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_WRITE_TRIGGER_STOP,0) ;
#endif
		/* requested stream shutdown */
		audio_dect_playback_stop_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		pr_debug("DECT : SNDRV_PCM_TRIGGER_SUSPEND active = 0\n");
		s->active = 0;
		s->periods = 0;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_RESUME\n");
		s->active = 1;
		s->tx_spin = 0;
		audio_dect_playback_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_PUSH\n");
		s->active = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_RELEASE\n");
		s->active = 1;
		if (s->old_offset) {
			s->tx_spin = 0;
			audio_dect_playback_dma(s);
			break;
		}
		break;
	default:
		pr_error("unknown command 0x%x\n",cmd);
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
static int
snd_mxc_audio_dect_capture_trigger(snd_pcm_substream_t * substream, int cmd)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int err;
	int device, stream_id;

	device = substream->pcm->device;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);

	if (device == 0)
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	else if (device == 1)
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	else
	{
		pr_error("wrong device (%d)\n", device);
		return -1;
	}

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	err = 0;


	/* note local interrupts are already disabled in the midlevel code */
	spin_lock(&s->dma_lock);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		pr_debug("DECT: capture SNDRV_PCM_TRIGGER_START\n");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_READ_TRIGGER_START,0) ;
#endif

		s->tx_spin = 0;
		/* requested stream startup */
		s->active = 1;
		audio_dect_capture_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_STOP\n");
#if defined(DO_ULOG)
		ulog(ULOG_ALSA_DECT_READ_TRIGGER_STOP,0) ;
#endif
		/* requested stream shutdown */
		audio_dect_capture_stop_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
		pr_debug("DECT : SNDRV_PCM_TRIGGER_SUSPEND active = 0\n");
		s->active = 0;
		s->periods = 0;
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_RESUME\n");
		s->active = 1;
		s->tx_spin = 0;
		audio_dect_capture_dma(s);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_PUSH\n");
		s->active = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("DECT: SNDRV_PCM_TRIGGER_PAUSE_RELEASE\n");
		s->active = 1;
		if (s->old_offset) {
			s->tx_spin = 0;
			audio_dect_capture_dma(s);
			break;
		}
		break;
	default:
		pr_error("unknown command 0x%x\n",cmd);
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
static int snd_mxc_audio_dect_playback_prepare(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int ssi;
	int stream_id = -1;
	int device ;

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_PREPARE,0) ;
#endif
	pr_debug("%s:%d - Enter\n",__FUNCTION__,__LINE__);


	device = substream->pcm->device;
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error(" wrong device (%d)\n",device);
		return -1;
	}

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	ssi = s->ssi;
	set_dect_channels(substream);

	if (chip->synchro_mode == DECT_MASTER)
	{
		configure_dam_dect_master(ssi);
	}
	else if (chip->synchro_mode == DECT_SLAVE)
	{
		configure_dam_dect_slave(ssi);
	}
	else
	{
		pr_error("synchro mode is not set\n");
		return -1;
	}
	
	configure_ssi_tx_for_dect(substream);	

	ssi_interrupt_enable(ssi, ssi_tx_dma_interrupt_enable);

	ssi_interrupt_enable(ssi, ssi_tx_fifo_1_empty);
	ssi_interrupt_enable(ssi, ssi_tx_fifo_0_empty);
	ssi_transmit_enable(ssi, true);
	
	s->period = 0;
	s->periods = 0;

	//msleep(100);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_PREPARE_DONE,0) ;
#endif
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
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
snd_pcm_uframes_t snd_mxc_audio_dect_capture_pointer(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t * chip;
	int device;
	int stream_id;
	device = substream->pcm->device;
	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	chip = snd_pcm_substream_chip(substream);

	return audio_dect_get_capture_dma_pos(&chip->s[stream_id]);
}

/*!
  * This function gets the current playback pointer position.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  */
static snd_pcm_uframes_t
snd_mxc_audio_dect_playback_pointer(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;

	int device;
	int stream_id;
	device = substream->pcm->device;
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	chip = snd_pcm_substream_chip(substream);
	return audio_dect_get_playback_dma_pos(&chip->s[stream_id]);
}

/*!
  * This structure reprensents the capabilities of the driver
  * in capture mode.
  * It is used by ALSA framework.
  */
static snd_pcm_hardware_t snd_mxc_dect_capture = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = (SNDRV_PCM_RATE_8000 ),
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
  * This structure reprensents the capabilities of the driver
  * in playback mode 
  * It is used by ALSA framework.
  */
static snd_pcm_hardware_t snd_mxc_dect_playback = {
	.info = (SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.rates = (SNDRV_PCM_RATE_8000 ),
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
static int snd_card_mxc_audio_dect_playback_open(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	snd_pcm_runtime_t *runtime;
	int stream_id = -1;
	int err;
	int device;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_OPEN,0) ;
#endif

	device = substream->pcm->device;
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	else if (device ==1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	pr_debug("stream_id=%d\n",stream_id);

	chip = snd_pcm_substream_chip(substream);
	runtime = substream->runtime;
	err = -1;

	chip->s[stream_id].stream = substream;
	runtime->hw = snd_mxc_dect_playback;

	if ((err = snd_pcm_hw_constraint_integer(runtime,
						 SNDRV_PCM_HW_PARAM_PERIODS)) <
	    0)
	{
		pr_debug("%s - ERROR - bad period\n",__FUNCTION__);
		return err;
	}

	if ((err = snd_pcm_hw_constraint_list(runtime, 0,
						      SNDRV_PCM_HW_PARAM_RATE,
						      &hw_playback_rates))
		    < 0)
	{
		pr_debug("%s - ERROR - bad sample rate\n",__FUNCTION__);
		return err;
	}

	msleep(10);

	/* setup DMA controller for playback */
	if ((err =
	     configure_write_channel(&mxc_audio_dect->s[stream_id],
				     audio_dect_playback_dma_callback,
				     stream_id)) < 0)
	{
		pr_error("channel configuration has failed\n");
		return err;
        }

	chip->active_channel_tx ++ ;

	//mxc_clks_enable(SSI1_BAUD);
	pr_debug(" stream_id=%d\n",stream_id);
	pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_WRITE_OPEN_DONE,0) ;
#endif
	
	return 0;
}

/*!
  * This function closes an DECT audio device for playback.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_dect_playback_close(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int ssi;
	int device;
	int stream_id;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);

	device = substream->pcm->device;
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}	
	else if (device == 1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

    pr_debug("stream_id=%d\n",stream_id);

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];

	ssi = s->ssi;

	chip->active_channel_tx -- ;

	// if no channel tx active disable transmission
	if (chip->active_channel_tx == 0)
	{
		ssi_tx_fifo_enable(ssi, ssi_fifo_1, false);
		ssi_tx_fifo_enable(ssi, ssi_fifo_0, false);

		ssi_transmit_enable(ssi, false);
		ssi_interrupt_disable(ssi, ssi_tx_dma_interrupt_enable);

		if (chip->active_channel_rx == 0)
		{
			ssi_enable(ssi, false);
		}
	}
		
	mxc_dma_free((mxc_audio_dect->s[stream_id]).dma_wchannel);

	chip->s[stream_id].stream = NULL;

    pr_debug("stream_id=%d\n",stream_id);
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
	return 0;
}

/*!
  * This function closes a DECT audio device for capture.
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_dect_capture_close(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int ssi;
	int device;
	int stream_id ;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	device = substream->pcm->device;
	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	pr_debug(" stream_id=%d\n",stream_id);

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	ssi = s->ssi;

	chip->active_channel_rx -- ;

	/*if no rx channel active, disable reception*/
	if (chip->active_channel_rx == 0)
	{
		ssi_receive_enable(ssi, false);
		ssi_interrupt_disable(ssi, ssi_rx_dma_interrupt_enable);
		ssi_rx_fifo_enable(ssi, ssi_fifo_0, false);
		ssi_rx_fifo_enable(ssi, ssi_fifo_1, false);
	
		if (chip->active_channel_tx == 0)
		{
			ssi_enable(ssi, false);
		}
	}
	
	mxc_dma_free((mxc_audio_dect->s[stream_id]).dma_wchannel);

	chip->s[stream_id].stream = NULL;

	pr_debug("stream_id=%d\n",stream_id);
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
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
static int snd_mxc_audio_dect_hw_params(snd_pcm_substream_t * substream,
				   snd_pcm_hw_params_t * hw_params)
{
	snd_pcm_runtime_t *runtime;
	int ret;

	runtime = substream->runtime;
	ret =
	    snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
	if (ret < 0)
		return ret;

	runtime->dma_addr = virt_to_phys(runtime->dma_area);

	pr_debug("%s - runtime->dma_addr 0x(%x)\n",__FUNCTION__,
		 (unsigned int)runtime->dma_addr);
	pr_debug("%s - runtime->dma_area 0x(%x)\n",__FUNCTION__,
		 (unsigned int)runtime->dma_area);
	pr_debug("%s - runtime->dma_bytes 0x(%x)\n",__FUNCTION__,
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
static int snd_mxc_audio_dect_hw_free(snd_pcm_substream_t * substream)
{
    int ret;
    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
    ret = snd_pcm_lib_free_pages(substream);
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
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
static int snd_mxc_audio_dect_capture_prepare(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	audio_stream_t *s;
	int ssi;
	int device;
	int stream_id;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	device = substream->pcm->device;

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_PREPARE,0) ;
#endif

	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	chip = snd_pcm_substream_chip(substream);
	s = &chip->s[stream_id];
	ssi = s->ssi;

	set_dect_channels(substream);

	pr_debug(" DECT substream->pstr->stream %d\n", substream->pstr->stream);
	pr_debug("SSI capture %d\n", ssi + 1);
	if (chip->synchro_mode == DECT_MASTER)
	{
		configure_dam_dect_master(ssi);
	}
	else if (chip->synchro_mode == DECT_SLAVE)
	{
		configure_dam_dect_slave(ssi);
	}
	else
	{
		pr_error("unkwnown synchro mode, select DECT MASTER\n");
		configure_dam_dect_master(ssi);
	}

	configure_ssi_rx_for_dect(substream);
	ssi_interrupt_enable(ssi, ssi_rx_dma_interrupt_enable);

	ssi_interrupt_enable(ssi, ssi_rx_fifo_0_full);
	ssi_interrupt_enable(ssi, ssi_rx_fifo_1_full);
	ssi_receive_enable(ssi, true);

	chip->active_channel_rx ++ ;

	s->period = 0;
	s->periods = 0;

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_PREPARE_DONE,0) ;
#endif
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
	return 0;
}

/*!
  * This function opens an DECT audio device in capture mode
  * It is called by ALSA framework.
  *
  * @param	substream	pointer to the structure of the current stream.
  *
  * @return              0 on success, -1 otherwise.
  */
static int snd_card_mxc_audio_dect_capture_open(snd_pcm_substream_t * substream)
{
	mxc_audio_dect_t *chip;
	snd_pcm_runtime_t *runtime;
	int err;
	int device;
	int stream_id;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	device = substream->pcm->device;

#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_OPEN,0) ;
#endif

	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	chip = snd_pcm_substream_chip(substream);
	runtime = substream->runtime;
	err = -1;

	chip->s[stream_id].stream = substream;
	runtime->hw = snd_mxc_dect_capture;

	if ((err = snd_pcm_hw_constraint_integer(runtime,
						 SNDRV_PCM_HW_PARAM_PERIODS)) <
	    0) {
		printk(" error periods DECT audio stream capture\n") ;
		return err;
	}

	if ((err = snd_pcm_hw_constraint_list(runtime, 0,
					      SNDRV_PCM_HW_PARAM_RATE,
					      &hw_capture_rates)) < 0) {
		pr_error("snd_pcm_hw_constraint_list : rate\n") ;
		return err;
	}

	/* setup DMA controller for Record */
	err = configure_read_channel(&mxc_audio_dect->s[stream_id],
				     audio_dect_capture_dma_callback,stream_id);
	if (err < 0) {
		pr_error("configure failure\n") ;
		return err;
	}

	//mxc_clks_enable(SSI1_BAUD);
	//msleep(50);
#if defined(DO_ULOG)
	ulog(ULOG_ALSA_DECT_READ_OPEN_DONE,0) ;
#endif
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
	
	return 0;
}

/*!
  * This structure is the list of operation that the driver
  * must provide for the capture interface
  */
static snd_pcm_ops_t snd_card_mxc_audio_dect_capture_ops = {
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
static snd_pcm_ops_t snd_card_mxc_audio_dect_playback_ops = {
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
  * This functions initializes the capture audio device supported by 
  * DECT IC.
  *
  * @param	mxc_audio	pointer to the sound card structure
  *
  */
void init_audio_stream_capture(mxc_audio_dect_t * mxc_audio_dect, int device)
{
	audio_stream_t *audio_stream;
	int stream_id;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	audio_stream = &mxc_audio_dect->s[stream_id];
	
	audio_stream->ssi = SSI1;
	audio_stream->dam_port = port_6;
	pr_debug("%s - DECT audio stream capture SSI : 0x%x\n" ,__FUNCTION__,audio_stream->ssi) ;
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
}

/*!
  * This functions initializes the playback audio device supported by 
  * DECT IC. 
  *
  * @param	mxc_audio	pointer to the sound card structure.
  * @param	device		device ID of PCM instance.
  *
  */
void init_audio_stream_playback(mxc_audio_dect_t * mxc_audio_dect, int device)
{
	audio_stream_t *audio_stream;
	int stream_id;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	audio_stream = &mxc_audio_dect->s[stream_id];
	audio_stream->ssi = SSI1;
	audio_stream->dam_port = port_6;
	pr_debug("%s - DECT audio stream playback SSI : 0x%x\n" ,__FUNCTION__,audio_stream->ssi) ;
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
}


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
    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
    pr_debug("device = %d\n", device);
	if (device == 0)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_PLAYBACK_CHANNEL_1;
	}
	else
	{
		pr_error("unknown device, select 0\n");
		stream_id=0;
	}

	mxc_audio_dect->s[stream_id].id = "Audio out";
	mxc_audio_dect->s[stream_id].stream_id =stream_id;
	
	if (device == 0)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_0;
	}
	else if (device == 1)
	{
		stream_id = DECT_STREAM_CAPTURE_CHANNEL_1;
	}
	else
	{
		pr_error(" unknown device, select 0\n");
		stream_id=0;
	}
	
	mxc_audio_dect->s[stream_id].id = "Audio in";
	mxc_audio_dect->s[stream_id].stream_id = stream_id;

	init_audio_stream_playback(mxc_audio_dect,device);
	init_audio_stream_capture(mxc_audio_dect,device);
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
		
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
	snd_pcm_t *pcm;
	int err;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);

	/* 
	 * Create a new PCM instance with 1 capture stream and 1 playback substream
	 */
	if ((err = snd_pcm_new(mxc_audio_dect->card, "DECT", device, 1, 1, &pcm)) < 0) {
		pr_error("snd_pcm_new err=0x%x\n",err) ;
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
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
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
static int snd_mxc_audio_dect_resume(snd_card_t * card, unsigned int state)
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
void snd_mxc_audio_dect_free(snd_card_t * card)
{
	mxc_audio_dect_t *chip;
    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);
	chip = card->private_data;
	audio_dect_dma_free(&chip->s[SNDRV_PCM_STREAM_PLAYBACK]);
	audio_dect_dma_free(&chip->s[SNDRV_PCM_STREAM_CAPTURE]);
	mxc_audio_dect = NULL;
	card->private_data = NULL;
	kfree(chip);
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
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
	snd_card_t *card;

    pr_debug("%s:%d: Enter\n", __FUNCTION__, __LINE__);

	/* register the soundcard */
	card = snd_card_new(-1, id, THIS_MODULE, sizeof(mxc_audio_dect_t));
	if (card == NULL) {
		pr_error("snd_card_new, no memory\n") ;
		return -ENOMEM;
	}

	mxc_audio_dect = kcalloc(1, sizeof(*mxc_audio_dect), GFP_KERNEL);
	if (mxc_audio_dect == NULL) {
		pr_error("kalloc, no memory\n") ;
		return -ENOMEM;
	}

	// initialise master or slave type (depends on board type)
	{ 
		//board_type_t zz_board_type = get_board_type() ;

		//if (  M_SAGEMBOARD_COMPMASK_IS_GA2(zz_board_type) )
		{
			// GA2 specific
			pr_debug("%s - GA2 detection, set DECT as master\n",__FUNCTION__);
			mxc_audio_dect->synchro_mode = DECT_MASTER;
		}
		/*else
		{
			pr_debug("%s - set DECT as slave\n",__FUNCTION__);
			mxc_audio_dect->synchro_mode = DECT_SLAVE ;
		}*/
	}

	card->private_data = (void *)mxc_audio_dect;
	card->private_free = snd_mxc_audio_dect_free;

	mxc_audio_dect->card = card;

	if ((err = snd_card_mxc_audio_dect_pcm(mxc_audio_dect, 0)) < 0) {
		pr_error("snd_card_mxc_audio_dect_pcm has failed\n") ;
		goto nodev;
	}

	if ((err = snd_card_mxc_audio_dect_pcm(mxc_audio_dect, 1)) < 0) {
		goto nodev;
	}

#if 0
	snd_card_set_dev_pm_callback(card, PM_SYS_DEV, snd_mxc_audio_dect_suspend,
				     snd_mxc_audio_dect_resume, mxc_audio_dect);
#endif

	/* Set autodetect feature in order to allow audio operations */

	spin_lock_init(&(mxc_audio_dect->s[0].dma_lock));
	spin_lock_init(&(mxc_audio_dect->s[1].dma_lock));
	spin_lock_init(&(mxc_audio_dect->s[2].dma_lock));
	spin_lock_init(&(mxc_audio_dect->s[3].dma_lock));


	strcpy(card->driver, "DECT");
	strcpy(card->shortname, "DECT-audio");
	sprintf(card->longname, "MXC Freescale with DECT");

	if ((err = snd_card_register(card)) == 0) {
		pr_debug("audio dect support initialized\n");
		return 0;
	}

      nodev:
	snd_card_free(card);
    pr_debug("%s:%d: Exit\n", __FUNCTION__, __LINE__);
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
