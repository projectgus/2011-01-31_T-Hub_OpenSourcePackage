/*!
 ***************************************************************************
 * \file pwrmngt_mx27.c
 * \brief summary
 * \ingroup DPM
 *
 * \par Copyright
 \verbatim Copyright 2007 Sagem Communication. All Rights Reserved.
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 \endverbatim
 */

/**************/
/* Prototypes */
/**************/
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/dpm.h>
#include <linux/delay.h>

/* include SAGEM */
#include <asm/io.h>
#include <asm/arch/pmic_power.h>
#include "pwrmngt.h"

extern void board_power_serdes(int);

#define NB_MODE_NORMAL 		1
#define NB_MODE_DOZE 		4
#define NB_MODE_STANDBY 	3
#define NB_MODE_SWITCHOFF 	1

#define LCDC_BRIGHNESS_REG	(IO_ADDRESS(LCDC_BASE_ADDR) + 0x2C)

/* Commented ressources */
#define CAMERA_PWR_MNGT 0
#define LED_PWR_MNGT 0
#define LIFE_PWR_MNGT 1

/********************/
/* local functions  */
/********************/
/*!
 * @brief Set LCD brightness
 * @param level brightness level
 */

static void _pwrmngt_set_brightness(unsigned char level)
{
        /* Set LCDC PWM contract control register */
        __raw_writel(0x00A90100 | level, LCDC_BRIGHNESS_REG);
}

/*!
 * @brief command suspend 
 * @param device  Pointer to device node
 */
static int
match_drvdev(struct device *dev,void *param)
{
//	printk("match_drvdev : %x %x\n",dev,dev->bus_id);
//        return (strncmp("0", dev->bus_id, 1) == 0);
	return 1;
}

static int pwrmngt_device_suspend (struct device_driver *dev_drv)
{
struct device *dev;
	dev = driver_find_device(dev_drv, NULL, NULL,match_drvdev );
	printk("suspend dev %s",dev->driver->name);
	dev_drv->suspend(dev,PMSG_SUSPEND);
return 0;
}

/*!
 * @brief command resume 
 * @param device  Pointer to device node
 */
static int pwrmngt_device_resume (struct device_driver *dev_drv)
{
struct device *dev;
	dev = driver_find_device(dev_drv, NULL, NULL,match_drvdev );
	printk("resume dev %s",dev->driver->name);
	dev_drv->resume(dev);
return 0;
}

/**************/
/* functions  */
/**************/
/*!
 * @brief 	call back for entering normal state 
 * @param mode 	Level off normal state
 * @return 	The function returns 0 on success and a non-zero value on
 *            	failure.
 */
int
pwrmngt_md_normal_in (unsigned short mode)
{
int error = 0;
t_regulator_voltage volt;
PMIC_STATUS l_pmic_test;

	if (mode > NB_MODE_NORMAL) {
		return -EINVAL;
	}
	l_pmic_test = pmic_power_regulator_get_voltage(SW_SW1A,&volt);
	if ((l_pmic_test == PMIC_SUCCESS) && (volt.sw1a == (t_pmic_regulator_voltage_sw1a) SW1A_1_6V)) { 
		error = dpm_set_policy("399");
	}
	else {
		error = -EAGAIN;
	} 
	_pwrmngt_set_brightness(178);
return error;
}

/*!
 * @brief 	call back for entering doze state 
 * @param mode 	Level off doze state
 * @return 	The function returns 0 on success and a non-zero value on
 *            	failure.
 */
int
pwrmngt_md_doze_in (unsigned short mode)
{
int error = 0;
	if (mode > NB_MODE_DOZE) {
		return -EINVAL;
	}
	error = dpm_set_policy("133");
	if (mode == 0) {
	_pwrmngt_set_brightness(89);
	}
	else if (mode == 1) {
	_pwrmngt_set_brightness(89);
	}
	else if (mode == 2) {
	_pwrmngt_set_brightness(89);
	}
	else if (mode == 3) {
	_pwrmngt_set_brightness(44);
	}
return error;
}

/*!
 * @brief 	call back for entering stand by state 
 * @param mode 	Level off stand by state
 * @return 	The function returns 0 on success and a non-zero value on
 *            	failure.
 */
int
pwrmngt_md_standby_in (unsigned short mode)
{
int error = 0;
struct device_driver *mx2fb_device;
struct device_driver *led_device;
struct device_driver *camera_device;
t_regulator_voltage volt;
PMIC_STATUS l_pmic_test;

	if (mode > NB_MODE_STANDBY) {
		return -EINVAL;
	}
	error = dpm_set_policy("133");
	/* life time saving */
#if LIFE_PWR_MNGT
	volt.sw1a = (t_pmic_regulator_voltage_sw1a) SW1A_1_2V;
	l_pmic_test = pmic_power_regulator_set_voltage(SW_SW1A,volt);
	if (l_pmic_test != PMIC_SUCCESS) {
		printk("pmic error 1_2V\n");
		error = -EINVAL;
	};
	mdelay(100);
#endif

	if (mode <=1) /* 0 or 1 */
	{
		/* LCD off */
//		mx2fb_device = driver_find("mxc_sdc_fb.0",&platform_bus_type);
		mx2fb_device = driver_find("mxc_sdc_fb",&platform_bus_type);
		if (mx2fb_device != NULL) {
			pwrmngt_device_suspend(mx2fb_device);
		}
		else {
			printk("Unknow drivers mxc_sdc_fb\n");
			error = -EINVAL;
		}

		/* LED off */
#if LED_PWR_MNGT
		led_device = driver_find("pmic_light",&platform_bus_type);
		if (led_device != NULL) {
			pwrmngt_device_suspend(led_device);
		}
		else {
			printk("Unknow drivers pmic_light\n");
			error = -EINVAL;
		}
#endif

		/* Camera off */
#if CAMERA_PWR_MNGT
		camera_device = driver_find("mxc_v4l2",&platform_bus_type);
		if (camera_device != NULL) {
			pwrmngt_device_suspend(camera_device);
		}
		else {
			printk("Unknow drivers mxc_v4l2\n");
			error = -EINVAL;
		}
#endif
	
		/* SerDes off */
		board_power_serdes(0);
		}
	else {
		_pwrmngt_set_brightness(44);
	}

	


return error;
}

/*!
 * @brief 	call back for outing stand by state 
 * @param mode 	Level off stand by state
 * @return 	The function returns 0 on success and a non-zero value on
 *            	failure.
 */
int
pwrmngt_md_standby_out (unsigned short mode)
{
int error = 0;
struct device_driver *mx2fb_device;
struct device_driver *led_device;
struct device_driver *camera_device;
t_regulator_voltage volt;
PMIC_STATUS l_pmic_test;

	/* end life saving */
#if LIFE_PWR_MNGT
	volt.sw1a = (t_pmic_regulator_voltage_sw1a) SW1A_1_6V;
	l_pmic_test = pmic_power_regulator_set_voltage(SW_SW1A,volt);
	if (l_pmic_test != PMIC_SUCCESS) {
		printk("pmic error 1_6V\n");
		error = -EINVAL;
	};
	mdelay(100);
#endif

	/* LCD on */
//	mx2fb_device = driver_find("mxc_sdc_fb.0",&platform_bus_type);
	mx2fb_device = driver_find("mxc_sdc_fb",&platform_bus_type);
	if (mx2fb_device != NULL) {
		pwrmngt_device_resume(mx2fb_device);
	}
	else {
		printk("Unknow drivers mxc_sdc_fb\n");
		error = -EINVAL;
	}

	/* LED on */
#if LED_PWR_MNGT
	led_device = driver_find("pmic_light",&platform_bus_type);
	if (led_device != NULL) {
		pwrmngt_device_resume(led_device);
	}
	else {
		printk("Unknow drivers pmic_light\n");
		error = -EINVAL;
	}
#endif

	/* Camera on */
#if CAMERA_PWR_MNGT
	camera_device = driver_find("mxc_v4l2",&platform_bus_type);
	if (camera_device != NULL) {
		pwrmngt_device_resume(camera_device);
	}
	else {
		printk("Unknow drivers mxc_v4l2\n");
		error = -EINVAL;
	}
#endif


	/* SerDes on */
	board_power_serdes(1);

return error;
}

/*!
 * @brief 	call back for switch off the system 
 * @param mode 	Level off switch off by state
 * @return 	The function returns 0 on success and a non-zero value on
 *            	failure.
 */
int
pwrmngt_md_switchoff_in (unsigned short mode)
{
int error = 0;

return error;
}
