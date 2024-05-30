/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 06/08/2007 
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
 * @file       mxc-alsa-mixer.c
 * @brief      this file implements the mxc sound driver mixer interface for ALSA.
 *             The mxc sound driver supports mono/stereo recording (there are
 *             some limitations due to hardware), mono/stereo playback and
 *             audio mixing. This file implements output switching, volume/balance controls
 *             mono adder config, I/P dev switching and gain on the PCM streams.
 *             Recording supports 8000 khz and 16000 khz sample rate.
 *             Playback supports 8000, 11025, 16000, 22050, 24000, 32000,
 *             44100 and 48000 khz for mono and stereo.
 *
 * @ingroup SOUND_DRV
 */

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <linux/soundcard.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
#include <asm/arch/pmic_audio.h>
#else
#include <mach/pmic_audio.h>
#endif
#include "mxc-alsa-common.h"
/*!
 * These are the functions implemented in the ALSA PCM driver that
 * are used for mixer operations
 *
 */

#define OUPUT_VOLUME_COUNT   1
#define OUPUT_VOLUME_MIN     0
#define OUPUT_VOLUME_MAX     100
#define OUPUT_VOLUME_STEP    1

#define OUPUT_MONOCONFIG_COUNT    1
#define OUPUT_MONOCONFIG_MIN      0
#define OUPUT_MONOCONFIG_MAX      3
#define OUPUT_MONOCONFIG_STEP     1


#define OUPUT_CAP_INPUT_COUNT     1
#define OUPUT_CAP_INPUT_MIN       0
#define OUPUT_CAP_INPUT_MAX       7
#define OUPUT_CAP_INPUT_STEP      1

#define OUTPUT_SWITCHS_COUNT      1 /* Set 2 for stereo and update get/put callbacks */
#define OUTPUT_SWITCHS_MIN        0
#define OUTPUT_SWITCHS_MAX        1
#define OUTPUT_SWITCHS_STEP       0


#define INPUT_VOLUME_COUNT   1
#define INPUT_VOLUME_MIN     0
#define INPUT_VOLUME_MAX     100
#define INPUT_VOLUME_STEP    1


typedef enum {
	MASTER_PLAYBACK,
	MASTER_PLAYBACK_MUTE,
	MASTER_MONOCONFIG_PLAYBACK,
	MASTER_BALANCE_PLAYBACK,
	STEREO_DAC_PLAYBACK,
	STEREO_DAC_PLAYBACK_MUTE,
	VOICE_CODEC_PLAYBACK,
	VOICE_CODEC_PLAYBACK_MUTE,
	CAPTURE,
	CAPTURE_MUTE,

} MIXER_PRIVATE_VALUES;

#if 0
#define TRACES(fmt, args...) { printk("[MXC_PMIC_DEBUG] %s() %d  "fmt"\n", __func__,  __LINE__, ## args); }
#define ERRORS(fmt, args...) { printk("[MXC_PMIC_DEBUG] %s() %d  "fmt"\n", __func__,  __LINE__, ## args); }
#else
#define TRACES(fmt, args...)
#define ERRORS(fmt, args...) { printk("[MXC_PMIC_DEBUG] %s() %d  "fmt"\n", __func__,  __LINE__, ## args); }
#endif

#define ERROR_INVALID_PRIVATE_VALUE						\
	printk("%s() %d : error : invalid value\n", __func__, __LINE__); BUG();panic("BUG !"); /* debug mode... */

#define SND_KCL_NEW(n, itm, ifc, idx, priv_val)				\
	static struct snd_kcontrol_new pmic_mixer_##n __devinitdata =	{	\
		.iface = SNDRV_CTL_ELEM_IFACE_##ifc,				\
		.name = itm,							\
		.index = idx,							\
		.info = pmic_mixer_info_##n,					\
		.get = pmic_mixer_get_##n,					\
		.put = pmic_mixer_put_##n,					\
		.private_value = priv_val,					\
	};


#define SET_KCL_INFOS(u, t, c, mi, ma, st)	\
	u->type = SNDRV_CTL_ELEM_TYPE_##t;	\
u->count = c;				\
u->value.integer.min = mi;		\
u->value.integer.max = ma;		\
u->value.integer.step = st;


#define SET_KCL_INFOS_SWITCH(u) SET_KCL_INFOS(u, BOOLEAN, OUTPUT_SWITCHS_COUNT,	OUTPUT_SWITCHS_MIN, OUTPUT_SWITCHS_MAX,	OUTPUT_SWITCHS_STEP);


#define SND_CKL_ADD(s)											\
	err = snd_ctl_add(card, snd_ctl_new1(s, p_value));						\
if(err) {											\
	ERRORS("Error - snd_ctl_add has return error %d\n", err);	\
	return err;										\
}


/**********************************
 *      Master Playback Volume    *
 **********************************/

static int pmic_mixer_info_master_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	TRACES("-");
	SET_KCL_INFOS(uinfo, 
			INTEGER, 
			OUPUT_VOLUME_COUNT, 
			OUPUT_VOLUME_MIN,
			OUPUT_VOLUME_MAX,
			OUPUT_VOLUME_STEP);
	return 0;
}

static int pmic_mixer_get_master_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int val;
	TRACES("-");
	val = get_mixer_output_volume(GAIN_ALL_DEV);
	val = val & 0xFF;
	uvalue->value.integer.value[0] = val;
	return 0;
}

static int pmic_mixer_put_master_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int volume;
	TRACES("-");
	volume = uvalue->value.integer.value[0];
	volume = volume | (volume << 8);
	return set_mixer_output_volume(NULL, volume, GAIN_ALL_DEV);
}


static int pmic_mixer_info_master_playback_mute(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	TRACES("-");
	SET_KCL_INFOS_SWITCH(uinfo);
	return 0;
}

static int pmic_mixer_get_master_playback_mute(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	uvalue->value.integer.value[0] = get_mixer_output_device()?1:0;
	TRACES("value[0] = %ld", uvalue->value.integer.value[0]);
	return 0;
}

static int pmic_mixer_put_master_playback_mute(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int ret, i;
	int state = get_mixer_output_device()?1:0;
	int enable = uvalue->value.integer.value[0];
	TRACES("state (%d) enable (%d)", state, enable);
	if(enable != state){
		if(enable){
			TRACES("Turning devices ON");
		} else {
			TRACES("Turning devices OFF");
		}
		
		for(i = OP_EARPIECE; i < OP_MAXDEV; i++){
			ret = set_mixer_output_device(NULL, MIXER_OUT, i, enable);
			if(ret){
				ERRORS("Error while setting device %d '%s' : error %d", i, enable?"enable":"disable", ret);
				return ret;
			}
		}
		return 1;
	}
	return 0;	
}


SND_KCL_NEW(master_playback, "Master Playback Volume", MIXER, 0x0, MASTER_PLAYBACK);
SND_KCL_NEW(master_playback_mute, "Master Playback Switch", MIXER, 0x0, MASTER_PLAYBACK_MUTE);

/***************************************
 *  Master Monoconfig Playback Volume  *
 ***************************************/
static int pmic_mixer_info_master_monoconfig_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	TRACES("-");
	SET_KCL_INFOS(uinfo,
			INTEGER,
			OUPUT_MONOCONFIG_COUNT,
			OUPUT_MONOCONFIG_MIN,
			OUPUT_MONOCONFIG_MAX,
			OUPUT_MONOCONFIG_STEP);
	return 0;
}

static int pmic_mixer_get_master_monoconfig_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	TRACES("-");
	uvalue->value.integer.value[0] = get_mixer_output_mono_adder();
	return 0;
}

static int pmic_mixer_put_master_monoconfig_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int mono = uvalue->value.integer.value[0];
	TRACES("-");
	return set_mixer_output_mono_adder(mono);
}
SND_KCL_NEW(master_monoconfig_playback, "Master Monoconfig Playback Volume", MIXER, 0x0, MASTER_MONOCONFIG_PLAYBACK);


/************************************
 *  Master Balance Playback Volume  *
 ************************************/
static int pmic_mixer_info_master_balance_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	TRACES("-");
	SET_KCL_INFOS(uinfo,
			INTEGER, 
			OUPUT_VOLUME_COUNT, 
			OUPUT_VOLUME_MIN,
			OUPUT_VOLUME_MAX,
			OUPUT_VOLUME_STEP);
	return 0;
}

static int pmic_mixer_get_master_balance_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	TRACES("-");
	uvalue->value.integer.value[0] = get_mixer_output_balance();
	return 0;

}
static int pmic_mixer_put_master_balance_playback(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int bal = uvalue->value.integer.value[0];
	TRACES("-");
	return set_mixer_output_balance(bal);
}


SND_KCL_NEW(master_balance_playback, "Master Balance Playback Volume", MIXER, 0x0, MASTER_BALANCE_PLAYBACK);


/*******************************************
 *      Stereo DAC control and voice codec *
 *******************************************/

#define CALLBACKS_VOLUME(n, gainname)											\
	static int pmic_mixer_info_##n(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)			\
	{														\
		TRACES("-");												\
		SET_KCL_INFOS(uinfo,INTEGER,OUPUT_VOLUME_COUNT,OUPUT_VOLUME_MIN,OUPUT_VOLUME_MAX,OUPUT_VOLUME_STEP);	\
		return 0;												\
	}														\
	static int pmic_mixer_get_##n(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uvalue)			\
	{														\
		int val;												\
		TRACES("-");												\
		val = get_mixer_output_volume(GAIN_##gainname);								\
		val = val & 0xFF;											\
		uvalue->value.integer.value[0] = val;									\
		return 0;												\
	}														\
	static int pmic_mixer_put_##n(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uvalue)			\
	{														\
		int volume;												\
		TRACES("-");												\
		volume = uvalue->value.integer.value[0];								\
		volume = volume | (volume << 8);									\
		return set_mixer_output_volume(NULL, volume, GAIN_##gainname);						\
	}



CALLBACKS_VOLUME(stereo_dac, STEREO_DAC);
SND_KCL_NEW(stereo_dac, "STEREO_DAC Playback Volume", MIXER, 0x0, STEREO_DAC_PLAYBACK);


CALLBACKS_VOLUME(voice_codec, VOICE_CODEC);
SND_KCL_NEW(voice_codec, "VOICE_CODEC Playback Volume", MIXER, 0x0, VOICE_CODEC_PLAYBACK);



#if ! (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))

#define CALLBACKS_MUTE_VOLUME(n, gainname)										\
	static int pmic_mixer_info_##n(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)			\
	{														\
		TRACES("-");												\
		SET_KCL_INFOS_SWITCH(uinfo);										\
		return 0;												\
	}														\
	static int pmic_mixer_get_##n(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uvalue)			\
	{														\
		uvalue->value.integer.value[0] = get_##n();								\
		TRACES(#n " : is active ? (%ld)", uvalue->value.integer.value[0]);							\
		return 0;												\
	}														\
	static int pmic_mixer_put_##n(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uvalue)			\
	{														\
		int act = uvalue->value.integer.value[0];								\
		int state = get_##n();											\
		if(act != state){											\
			TRACES(#n " : setting active to (%d)", act);								\
			set_##n(act);											\
			set_mixer_output_volume(NULL, get_mixer_output_volume(GAIN_##gainname), GAIN_##gainname);	\
			return 1;											\
		}													\
		return 0;												\
	}

CALLBACKS_MUTE_VOLUME(stereo_dac_mute, STEREO_DAC);
SND_KCL_NEW(stereo_dac_mute, "STEREO_DAC Playback Switch", MIXER, 0x0, STEREO_DAC_PLAYBACK_MUTE);

CALLBACKS_MUTE_VOLUME(voice_codec_mute, VOICE_CODEC);
SND_KCL_NEW(voice_codec_mute, "VOICE_CODEC Playback Switch", MIXER, 0x0, VOICE_CODEC_PLAYBACK_MUTE);

#endif

/***************************************
 *          Hardware switchs           *
 ***************************************/

#define CALLBACKS_HW_SWITCH(name, devname, idev)								\
	static int pmic_mixer_info_##name(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)	\
	{													\
		TRACES("-");											\
		SET_KCL_INFOS_SWITCH(uinfo);									\
		return 0;											\
	}													\
	static int pmic_mixer_get_##name (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uvalue)	\
	{													\
		int state = (get_mixer_output_device() &  SOUND_MASK_##devname)?1:0;				\
		TRACES("state (%d)", state);									\
		uvalue->value.integer.value[0] = state;								\
		return 0;											\
	}													\
	static int pmic_mixer_put_##name (struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *uvalue)	\
	{													\
		int ret;											\
		int state = (get_mixer_output_device() & SOUND_MASK_##devname)?1:0;				\
		int enable = uvalue->value.integer.value[0]; 							\
		if(state != enable){										\
			if(enable){										\
				TRACES("Turning on" #idev);							\
			} else {										\
				TRACES("Turning off" #idev);							\
			}											\
			return (ret = set_mixer_output_device(NULL, MIXER_OUT, OP_##idev, enable))?ret:1;	\
		}												\
		return 0;											\
	}

CALLBACKS_HW_SWITCH(phoneout_mute, PHONEOUT, EARPIECE);
SND_KCL_NEW(phoneout_mute, "Phoneout Playback Switch", MIXER, 0x0, 0x0);

CALLBACKS_HW_SWITCH(speaker_mute, SPEAKER, HANDSFREE);
SND_KCL_NEW(speaker_mute, "Handsfree Playback Switch", MIXER, 0x0, 0x0);

CALLBACKS_HW_SWITCH(volume_mute, VOLUME, HEADSET);
SND_KCL_NEW(volume_mute, "Volume Playback Switch", MIXER, 0x0, 0x0);

CALLBACKS_HW_SWITCH(pcm_mute, PCM, LINEOUT);
SND_KCL_NEW(pcm_mute, "PCM Playback Switch", MIXER, 0x0, 0x0);


/***************************************
 *          Master Capture Volume      *
 ***************************************/
static int pmic_mixer_info_capture_volume(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	TRACES("-");
	SET_KCL_INFOS(uinfo, 
			INTEGER, 
			INPUT_VOLUME_COUNT, 
			INPUT_VOLUME_MIN,
			INPUT_VOLUME_MAX,
			INPUT_VOLUME_STEP);
	return 0;
}

static int pmic_mixer_get_capture_volume(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int val;
	TRACES("-");
	val = get_mixer_input_gain();
	val = val & 0xFF;
	uvalue->value.integer.value[0] = val;
	return 0;
}

static int pmic_mixer_put_capture_volume(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{

	int vol;
	TRACES("-");
	vol = uvalue->value.integer.value[0];
	vol = vol | (vol << 8);
	return set_mixer_input_gain(NULL, vol);
}
static int pmic_mixer_info_capture_volume_mute(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	TRACES("-");
	SET_KCL_INFOS_SWITCH(uinfo);
	return 0;
}

static int pmic_mixer_get_capture_volume_mute(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	uvalue->value.integer.value[0] = (get_mixer_input_device() & SOUND_MASK_MIC)?1:0;
	TRACES("State is %ld", uvalue->value.integer.value[0]);
	return 0;
}

static int pmic_mixer_put_capture_volume_mute(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *uvalue)
{
	int ret;
	int state = (get_mixer_input_device() & SOUND_MASK_MIC)?1:0;
	int enable = uvalue->value.integer.value[0];	
	TRACES("State (%d) enable (%d)", state, enable);
	if(enable != state){
		if(enable){
			TRACES("Turning device IP_HEADSET ON ");
		} else {
			TRACES("Turning devices IP_HEADSET OFF");
		}

		return (ret = set_mixer_input_device(NULL, IP_HEADSET, enable))?ret:1;
	}

	return 0;
}



SND_KCL_NEW(capture_volume, "Microphone Capture Volume", MIXER, 0x0, CAPTURE);
SND_KCL_NEW(capture_volume_mute, "Microphone Capture Switch", MIXER, 0x0, CAPTURE_MUTE);/* Amc1R */


/*!
 * This function registers the control components of ALSA Mixer
 * It is called by ALSA PCM init.
 *
 * @param	card pointer to the ALSA sound card structure.
 *
 * @return              0 on success, -ve otherwise.
 */
int mxc_alsa_create_ctl(struct snd_card *card, void *p_value)
{
	int err = 0;

	/* Volumes */
	SND_CKL_ADD(&pmic_mixer_master_playback);
	SND_CKL_ADD(&pmic_mixer_master_playback_mute);
	SND_CKL_ADD(&pmic_mixer_master_monoconfig_playback);
	SND_CKL_ADD(&pmic_mixer_master_balance_playback);
	SND_CKL_ADD(&pmic_mixer_stereo_dac);
	SND_CKL_ADD(&pmic_mixer_voice_codec);

	/* Switch */
#if ! (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	SND_CKL_ADD(&pmic_mixer_stereo_dac_mute);
	SND_CKL_ADD(&pmic_mixer_voice_codec_mute);
#endif
	SND_CKL_ADD(&pmic_mixer_phoneout_mute);
	SND_CKL_ADD(&pmic_mixer_speaker_mute);
	SND_CKL_ADD(&pmic_mixer_volume_mute);
	SND_CKL_ADD(&pmic_mixer_pcm_mute);

	/* Captures */
	SND_CKL_ADD(&pmic_mixer_capture_volume);
	SND_CKL_ADD(&pmic_mixer_capture_volume_mute);
	return 0;
}

