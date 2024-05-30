/*
 *  Configure audio power amplifier.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: sagem-audio-pwr-amp.c
 *  Creation date: 12/12/2008
 *  Author: Olivier Le Roy, Farid Hammane, Sagemcom
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

#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
#include <asm/arch/gpio.h>
#else
#include <linux/sysdev.h>
#include <mach/gpio.h>
#endif
#include "../../arch/arm/mach-mx3/iomux.h"

#ifdef CONFIG_FACTORY_ZONE
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
#include <asm/arch/factory_conf.h>
#else
#include <mach/factory_conf.h>
#endif
extern int factory_parse_parameter(factory_parameter_t *parameter);
#endif

/* 
 * Number of authorized suspends and resumes.
 */
#define MAX_DEVICE_RESUME	3

/*
 * The max size alowed while getting gain value from
 * user space. 2 is a decimal between 0 and 3, plus the 
 * new line char.
 */
#define MAX_SIZE_DATA_GAIN	2


#define PROG_NAME	"[SAGEM Audio Amplifier] "
#define TRACE(fmt, args...)		/*printk(PROG_NAME "TRACE  %d %s()  "fmt"\n", __LINE__, __func__, ## args);*/
#define INFO(fmt, args...)		printk(KERN_INFO PROG_NAME "INFO  %d %s()  "fmt"\n", __LINE__, __func__, ## args);
#define WARNING(fmt, args...)		printk(KERN_ERR PROG_NAME "WARNING  %d %s()  "fmt"\n", __LINE__, __func__, ## args);
#define ERROR(fmt, args...)		printk(KERN_ERR PROG_NAME "ERROR  %d %s()  "fmt"\n", __LINE__, __func__, ## args);

struct t_sagem_audio_pwr_amp {

	/*
	 * state of amplifier. Set to 1 if init 
	 * was successfull.
	 */
	int state;

	/* 
	 * Number of devices using amplifier.
	 * Amplifier still on while nb_dev is
	 * positive.
	 */
	int nb_using_dev;


	/* Set to 1 if headset is plugged. in
	 * this case , amplifier will always 
	 * be suspended.
	 */
	int headset_state;


	/* Unconditional suspend : if it is set
	 * power amplifier still off.
	 */
	int uncond_susp;

};
struct t_sagem_audio_pwr_amp sapa;


/* GPIO1_0 : \SHUTDOWN_HP , (= 1 to enable HP output) 
 * GPIO1_1 : GAIN0_HP, ( = 1,
 * GPIO1_2 : GAIN1_HP    = 1 for gain of +24 dB )
 *   G1  G0  Gain
 *    0   0   +6 dB
 *    0   1  +12 dB
 *    1   0  +18 dB
 *    1   1  +24 dB
 */


/*!
 * @enum GAIN_GPIO
 * @brief Supported gain by gpio.
 */
typedef enum {
	GAIN_GPIO_6DB,
	GAIN_GPIO_12DB,
	GAIN_GPIO_18DB,
	GAIN_GPIO_24DB,
} GAIN_GPIO;


/** Set audio amp's gain. 
 * @param gain IN : gain to set. Supported gain : GAIN_GPIO 
 * @return 0 if successful. Non-zero otherwise. 
 */
int sagem_audio_pwr_amp_set_gain(GAIN_GPIO gain){
	TRACE("Enter");
	switch(gain){
		case GAIN_GPIO_6DB:
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_1, 0);
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_2, 0);
			break;

		case GAIN_GPIO_12DB:
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_1, 1);
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_2, 0);
			break;

		case GAIN_GPIO_18DB:
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_1, 0);
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_2, 1);
			break;

		case GAIN_GPIO_24DB:
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_1, 1);
			mxc_set_gpio_dataout(MX31_PIN_GPIO1_2, 1);
			break;

		default:
			ERROR("Unsupported gain (%d)", gain);
			return -EINVAL;
	}
	return 0;
};

/** Turn audio amplifier off while headset is plugged, and set headset_state to 1. 
 * @param *dev
 * @param state 
 * @return 
 */
void sagem_audio_pwr_amp_headset_suspend(void){
	if(!sapa.state) panic("Device absent.");
	sapa.headset_state = 1;
	if(sapa.uncond_susp){
		TRACE("Unconditional suspend is set");
		return;
	}
	TRACE("Suspend ok");
	mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 0);
}
EXPORT_SYMBOL(sagem_audio_pwr_amp_headset_suspend); //Called by mxc-alsa-pmic.c

/** Turn audio amplifier off
 * @param dev IN : prototype defined by sysfs. dev is not used.
 * @param state IN : prototype defined by sysfs. state is not used.
 * @return always 0
 */
int sagem_audio_pwr_amp_suspend(struct platform_device *dev, pm_message_t state)
{
	 /* :TODO:10.11.2009 17:08:02:: update this function */
	if(!sapa.state) panic("Device absent.");

	sapa.nb_using_dev--;

	if(sapa.uncond_susp){
		TRACE("Unconditional suspend is set");
		return 0;
	}

	TRACE("Amplifier state (%d)", sapa.nb_using_dev);

	if(sapa.nb_using_dev <= 0){
		TRACE("Suspend ok");
		mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 0);
	}

	return 0;
};
EXPORT_SYMBOL(sagem_audio_pwr_amp_suspend); //Called by mxc-alsa-pmic.c

/** Set headset_state to 0, and turn amplifier on if some devices need it 
 * @param struct platform_device *pdev 
 * @return 
 */
void sagem_audio_pwr_amp_headset_resume(void){
	if(!sapa.state) panic("Device absent.");
	sapa.headset_state = 0;
	if(sapa.uncond_susp){
		TRACE("Unconditional suspend is set");
		return;
	}
	if(sapa.nb_using_dev > 0){
		TRACE("Resume ok");
		mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 1);
	}
}
EXPORT_SYMBOL(sagem_audio_pwr_amp_headset_resume); //Called by mxc-alsa-pmic.c

/** Turn audio amplifier on
 * @param dev IN : prototype defined by sysfs. dev is not used.
 * @return always 0
 */
int sagem_audio_pwr_amp_resume(struct platform_device *pdev)
{
	if(!sapa.state) panic("Device absent.");
	sapa.nb_using_dev++;
	if(sapa.uncond_susp){
		TRACE("Unconditional suspend is set (nb of suspends (%d))", sapa.nb_using_dev);
		return 0;
	}
	if(sapa.headset_state){
		TRACE("Headset is plugged, will not turn amplifier on (ampl. nb of suspend (%d))", sapa.nb_using_dev);
		return 0;
	}

	TRACE("Amplifier state (%d)", sapa.nb_using_dev);

	if(sapa.nb_using_dev > 0){
		TRACE("Resume ok");
		mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 1);
	}

	return 0;
}
EXPORT_SYMBOL(sagem_audio_pwr_amp_resume); //Called by mxc-alsa-pmic.c


/** Turn audio amplifier off with no condition
 */
void sagem_audio_pwr_amp_unconditional_suspend(void)
{
	if(!sapa.state) panic("Device absent.");
	sapa.uncond_susp = 1;
	TRACE("Setting Unconditional suspend");
	TRACE("Suspend ok");
	mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 0);
	return;
}
EXPORT_SYMBOL(sagem_audio_pwr_amp_unconditional_suspend);


/** Disable unconditional suspend and resume
 * if necessary
 */
void sagem_audio_pwr_amp_disable_unconditional_suspend(void)
{
	if(!sapa.state) panic("Device absent.");
	
	if(!sapa.uncond_susp)
		return;

	TRACE("Disabling Unconditional suspend");
	sapa.uncond_susp = 0;
	
	if(sapa.headset_state){
		TRACE("Headset is plugged, will not turn amplifier on");
		return;
	}
	if(sapa.nb_using_dev > 0){
		TRACE("Resume ok");
		mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 1);
	}
	return;
}
EXPORT_SYMBOL(sagem_audio_pwr_amp_disable_unconditional_suspend);


/** Geting amplifier state 
 * @param void 
 * @return The number of devices using audio amp.
 */
int sagem_audio_pwr_amp_get_state(void)
{
	if(!sapa.state){
		ERROR("No devices");
		return -ENODEV;
	}

	TRACE("Amplifier state (%d)", sapa.nb_using_dev);

	if(mxc_get_gpio_datain(MX31_PIN_GPIO1_0) &&
			sapa.nb_using_dev < 1){
		ERROR("Amp cannot be active if no devices use it");
		return -EINVAL;
	}
	
	return sapa.nb_using_dev;
}
EXPORT_SYMBOL(sagem_audio_pwr_amp_get_state);




static int sagem_audio_pwr_amp_remove(struct platform_device *pdev){
	TRACE("Enter");
	mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 0);
	memset(&sapa, 0x0, sizeof(struct t_sagem_audio_pwr_amp));
	mxc_free_iomux(MX31_PIN_GPIO1_0, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	mxc_free_iomux(MX31_PIN_GPIO1_1, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	mxc_free_iomux(MX31_PIN_GPIO1_2, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	INFO("SAGEM Audio Power Amp disconnected removed");
	return 0;
}

static int sagem_audio_pwr_amp_probe(struct platform_device *pdev)
{
	int err;
	int ampli_gain=0x3;
	
	memset(&sapa, 0x0, sizeof(struct t_sagem_audio_pwr_amp));

	err = mxc_request_iomux(MX31_PIN_GPIO1_0, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	if(err)
		goto fail;
	err = mxc_request_iomux(MX31_PIN_GPIO1_1, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	if(err)
		goto fail;
	err = mxc_request_iomux(MX31_PIN_GPIO1_2, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	if(err)
		goto fail;

#ifdef CONFIG_FACTORY_ZONE
	{
		static factory_parameter_t parameter;
		int result;

		strcpy(&parameter.name[0], "AMPLI_GAIN");
		strcpy(&parameter.application[0], "mxc_alsa_pmic");
		strcpy(&parameter.version[0], "");
		result = factory_parse_parameter(&parameter);
		if(result){
			printk("%s - WARNING - Could not get factory value for AMPLI_GAIN\n",__FUNCTION__);
		} else {
			ampli_gain = (int)simple_strtoul(&parameter.value[0], NULL, 0);
		}
		if ((ampli_gain>3)||(ampli_gain<0))
		{
			printk("%s - ERROR - Invalid value for AMPLI_GAIN\n",__FUNCTION__);
			ampli_gain=0x3; /* Default valume */
		}
	}
#endif

	mxc_set_gpio_direction(MX31_PIN_GPIO1_0, GPIO_DIR_OUTPUT);
	mxc_set_gpio_direction(MX31_PIN_GPIO1_1, GPIO_DIR_OUTPUT);
	mxc_set_gpio_direction(MX31_PIN_GPIO1_2, GPIO_DIR_OUTPUT);
	mxc_set_gpio_dataout(MX31_PIN_GPIO1_0, 0);//Seting to off at startup to save power.
	mxc_set_gpio_dataout(MX31_PIN_GPIO1_1, ampli_gain&1);
	mxc_set_gpio_dataout(MX31_PIN_GPIO1_2, (ampli_gain>>1)&1);

	sapa.state = 1;
	INFO("SAGEM Audio Power Amp successfully loaded");
	return err;
fail:
	ERROR("Probe fail : error %d. Freeing gpios", err);
	mxc_free_iomux(MX31_PIN_GPIO1_0, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	mxc_free_iomux(MX31_PIN_GPIO1_1, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	mxc_free_iomux(MX31_PIN_GPIO1_2, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);

	return err;
}

static struct platform_driver sagem_audio_pwr_amp_driver_ldm = {
	.driver = {
		   .name = "sagem_audio_pwr_amp",
		   },
	.suspend = sagem_audio_pwr_amp_suspend,
	.resume = sagem_audio_pwr_amp_resume,
	.probe = sagem_audio_pwr_amp_probe,
	.remove = sagem_audio_pwr_amp_remove,
};


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static ssize_t sysdev_read_pins(struct sys_device *dev, char *buf)
#else
static ssize_t sysdev_read_pins(struct sys_device *dev, struct sysdev_attribute *att, char *buf)
#endif
{
	int size = 0;
	int data = 0;
	TRACE("Enter");

	data = mxc_get_gpio_datain(MX31_PIN_GPIO1_0);
	size += sprintf((buf + size), "MX31_PIN_GPIO1_0 (%d)\n", data);

	data = mxc_get_gpio_datain(MX31_PIN_GPIO1_1);
	size += sprintf((buf + size), "MX31_PIN_GPIO1_1 (%d)\n", data);

	data = mxc_get_gpio_datain(MX31_PIN_GPIO1_2);
	size += sprintf((buf + size), "MX31_PIN_GPIO1_2 (%d)\n",  data);

	return size;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static ssize_t sysdev_write_gain_value(struct sys_device *dev, const char *buf, size_t size)
#else
static ssize_t sysdev_write_gain_value(struct sys_device *dev, struct sysdev_attribute *att, const char *buf, size_t size)
#endif
{
	int data = 0;
	int err;
	TRACE("Enter");

	if(size > MAX_SIZE_DATA_GAIN){
		ERROR("Invalid value : expected numeric value between 0 and 3");
		return -EINVAL;
	}
	
	data = simple_strtol(buf, NULL, 10);

	if(data < 0 || data > 3){
		ERROR("Invalid value : got %d, expected value between 0 and 3", data);
		return -EINVAL;
	}

	TRACE("Setting %d", data);
	
	err = sagem_audio_pwr_amp_set_gain((GAIN_GPIO)data);
	if(err)
		return err;
	
	return size;
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static ssize_t sysdev_read_gain_value(struct sys_device *dev, char *buf)
#else
static ssize_t sysdev_read_gain_value(struct sys_device *dev, struct sysdev_attribute *att, char *buf)
#endif
{
	int size = 0;
	int data = 0;
	TRACE("Enter");

	data |= mxc_get_gpio_datain(MX31_PIN_GPIO1_2);
	data |= (mxc_get_gpio_datain(MX31_PIN_GPIO1_1) << 1);

	size += sprintf(buf, "Amplifier value : %d\n", data);

	return size;
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
static ssize_t sysdev_read_infos(struct sys_device *dev, char *buf)
#else
static ssize_t sysdev_read_infos(struct sys_device *dev, struct sysdev_attribute *att, char *buf)
#endif
{
	int size = 0;
	size += sprintf(buf + size, "nb_using_dev :          %d\n", sapa.nb_using_dev);
	size += sprintf(buf + size, "state :                 %d\n", sapa.state);
	size += sprintf(buf + size, "headset state :         %d\n", sapa.headset_state);
	size += sprintf(buf + size, "Unconditional suspend : %d\n", sapa.uncond_susp);
	return size;
}


static SYSDEV_ATTR(amp_audio_pins, 0444, sysdev_read_pins, NULL);
static SYSDEV_ATTR(amp_audio_gain_value, 0644, sysdev_read_gain_value, sysdev_write_gain_value);
static SYSDEV_ATTR(amp_audio_infos, 0644, sysdev_read_infos, NULL);

static struct sysdev_class sysclass = {
	
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27))
	set_kset_name("Audio_Amplifier")
#else
	/* 
	 * FIXME : macro set_kset_name has been removed in lin 2.6.27. 
	 * Do we need to initialize something else here ?
	 * */
	.name = "Audio_Amplifier",
#endif
};

static struct sys_device sysdevice = {
	.id = 0,
	.cls = &sysclass,
};

static void sagem_audio_pwr_amp_sysdev_exit(void){
	sysdev_remove_file(&sysdevice, &attr_amp_audio_infos);
	sysdev_remove_file(&sysdevice, &attr_amp_audio_pins);
	sysdev_remove_file(&sysdevice, &attr_amp_audio_gain_value);
	sysdev_unregister(&sysdevice);
	sysdev_class_unregister(&sysclass);
}

static int sagem_audio_pwr_amp_sysdev_init(void){

	int err;
	err = sysdev_class_register(&sysclass);
	if(err){
		ERROR("Could not register class : error %d", err);
		return err;
	}

	err = sysdev_register(&sysdevice);
	if(err){
		ERROR("Could not register device : error %d", err);
		sagem_audio_pwr_amp_sysdev_exit();
		return err;
	}

	err = sysdev_create_file(&sysdevice, &attr_amp_audio_pins);
	if(err){
		ERROR("Could not create a sysdev file to export pins state : error %d", err);
		sagem_audio_pwr_amp_sysdev_exit();
		return err;
	}
	
	err = sysdev_create_file(&sysdevice, &attr_amp_audio_gain_value);
	if(err){
		ERROR("Could not create a sysdev file to export gain value : error %d", err);
		sagem_audio_pwr_amp_sysdev_exit();
		return err;
	}

	err = sysdev_create_file(&sysdevice, &attr_amp_audio_infos);
	if(err){
		ERROR("Could not create a sysdev file to export driver informations : error %d", err);
		sagem_audio_pwr_amp_sysdev_exit();
		return err;
	}
	
	return 0;
}


/*
 * Initialization and Exit
 */

static int __init sagem_audio_pwr_amp_init(void)
{
	int err;
	INFO("SAGEM Audio Power Amp driver loading...");
	err = sagem_audio_pwr_amp_sysdev_init();
	if(err){
		ERROR("Fail to load SAGEM Audio Power Amp driver");
		return err;
	}
	if (platform_driver_register(&sagem_audio_pwr_amp_driver_ldm) != 0) {
		       ERROR("Driver register failed for sagem_audio_pwr_amp_driver");
		return -ENODEV;
	}

	return 0;
}

static void __exit sagem_audio_pwr_amp_exit(void)
{
	sagem_audio_pwr_amp_sysdev_exit();
	platform_driver_unregister(&sagem_audio_pwr_amp_driver_ldm);
	INFO("SAGEM Audio Power Amp driver successfully unloaded");
}

/*
 * Module entry points
 */

module_init(sagem_audio_pwr_amp_init);
module_exit(sagem_audio_pwr_amp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Olivier Le Roy <olivier.leroy@sagem.com>");
MODULE_DESCRIPTION("SAGEM Audio Power Amp driver");
