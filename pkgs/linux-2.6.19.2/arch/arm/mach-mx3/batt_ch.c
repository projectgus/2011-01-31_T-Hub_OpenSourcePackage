/*
 *  This driver controls the battery.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: batt_ch.c
 *  Creation date: 09/01/2009
 *  Author: Olivier Le Roy, Sagemcom
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

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* copy_from_user, copy_to_user */
#include <asm/arch/gpio.h>
#include <asm/arch/batt_ch.h>
#include <asm/arch/pmic_light.h>
#include <asm/arch/pmic_adc.h>

static int batt_ch_major;

static struct class *batt_ch_class;

extern board_type_t get_board_type(void);

static int batt_ch_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	unsigned char param;
	unsigned short result[8];

	switch (cmd) {
		case SET_CHG_SHDN:
			if (copy_from_user(&param, (unsigned char *) arg, sizeof(param)))
				return -EFAULT;

			if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
				mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_CHG_SHDN, param);
			else
				mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_CHG_SHDN, param);
			break;

		case GET_CHG_SHDN:
			if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
				param = mxc_get_gpio_datain(HOMESCREEN_V3_GPIO_CHG_SHDN);
			else
				param = mxc_get_gpio_datain(HOMESCREEN_V1_GPIO_CHG_SHDN);

			if (copy_to_user((unsigned char *)arg, &param, sizeof(param)))
				return -EFAULT;
			break;


		case SET_CHEM:
			/*		param        Duty Cycle (% On-time over Cycle Time)
					------    ---------------------------------------
					0        0%
					1        6.7%
					2        13.3%
					3        20%
					4        26.7%
					5        33.3%
					6        40%
					7        46.7%
					8        53.3%
					9        60%
					10        66.7%
					11        73.3%
					12        80%
					13        86.7%
					14        93.3%
					15        100%
			 */
			if (copy_from_user(&param, (unsigned char *)arg, sizeof(param)))
				return -EFAULT;

			pmic_bklit_set_dutycycle(BACKLIGHT_LED3, param);
			break;

		case GET_CHEM:
			pmic_bklit_get_dutycycle(BACKLIGHT_LED3, &param);
			if (copy_to_user((unsigned char *)arg, &param, sizeof(param)))
				return -EFAULT;
			break;

		case GET_CHG_MON:
			pmic_adc_convert_8x(GEN_PURPOSE_AD8, &result[0]);
			if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
			{
				if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result)))
					return -EFAULT;
				break;
			}
			else
			{	/* Arithmetic mean filtering on 8 samples */
				result[0] = (result[0] + result[1] + result[2] + result[3] + result[4] + result[5] + result[6] + result[7]) / 8;
				if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result[0])))
					return -EFAULT;

				break;
			}

		case GET_VINBAT:
			pmic_adc_convert_8x(GEN_PURPOSE_AD6, &result[0]);
			if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
			{
				if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result)))
					return -EFAULT;
				break;
			}
			else
			{	/* Arithmetic mean filtering on 8 samples */
				result[0] = (result[0] + result[1] + result[2] + result[3] + result[4] + result[5] + result[6] + result[7]) / 8;
				if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result[0])))
					return -EFAULT;

				break;
			}

		case GET_VOUTBAT:
			pmic_adc_convert_8x(GEN_PURPOSE_AD7, &result[0]);
			if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
			{
				if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result)))
					return -EFAULT;
				break;
			}
			else
			{	/* Arithmetic mean filtering on 8 samples */
				result[0] = (result[0] + result[1] + result[2] + result[3] + result[4] + result[5] + result[6] + result[7]) / 8;
				if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result[0])))
					return -EFAULT;

				break;
			}

		case GET_BP:
			pmic_adc_convert_8x(APPLICATION_SUPPLY, &result[0]);
			if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result)))
					return -EFAULT;
			break;

		case GET_BATT_THERM:
			pmic_adc_convert_8x(GEN_PURPOSE_AD5, &result[0]);
			if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result)))
					return -EFAULT;
			break;

		case GET_3V3:
			pmic_adc_convert_8x(GEN_PURPOSE_AD9, &result[0]);
			if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result)))
					return -EFAULT;
			break;

		case GET_IGEN:
			pmic_adc_convert_8x(GEN_PURPOSE_AD10, &result[0]);
			if (copy_to_user((unsigned short *)arg, &result[0], sizeof(result)))
					return -EFAULT;
			break;

		default:
			pr_debug("batt_ch_ioctl: unsupported ioctl command 0x%x\n", cmd);
			return -EINVAL;
	}
	return 0;
}

struct file_operations batt_ch_fops = {
	.owner = THIS_MODULE,
	.ioctl = batt_ch_ioctl,
};

/*
 * Initialization and Exit
 */

static int __init batt_ch_init(void) {
	int ret = 0;

	struct class_device *temp_class;

	extern void gpio_activate_battery_charger(void);

	pr_debug("batt_ch driver loading...\n");
	batt_ch_major = register_chrdev(0, "batt_ch", &batt_ch_fops);
	if (batt_ch_major < 0) {
		printk(KERN_ERR "Unable to get a major for batt_ch\n");
		return batt_ch_major;
	}
	batt_ch_class = class_create(THIS_MODULE, "batt_ch");
	if (IS_ERR(batt_ch_class)) {
		pr_debug(KERN_ERR "Error creating batt_ch class.\n");
		ret = PTR_ERR(batt_ch_class);
		goto err_out1;
	}
	temp_class = class_device_create(batt_ch_class, NULL, MKDEV(batt_ch_major, 0), NULL, "batt_ch");
	if (IS_ERR(temp_class)) {
		pr_debug(KERN_ERR "Error creating batt_ch class device.\n");
		ret = PTR_ERR(temp_class);
		goto err_out2;
	}
	gpio_activate_battery_charger();
	/* config CHEM
	 * LED3 ATLAS:	100 Hz (period = 0),
	 *		0% : 8.25 V, 100% 8.40 V
	 */
	pmic_bklit_set_cycle_time(0);
	pmic_bklit_set_dutycycle(BACKLIGHT_LED3, 15);
	pmic_bklit_set_mode(BACKLIGHT_LED3, BACKLIGHT_TRIODE_MODE);
	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
		mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_CHG_SHDN, 0);  // Enable charger.
	else
		mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_CHG_SHDN, 0);  // Enable charger.
	printk(KERN_NOTICE "batt_ch loaded successfully\n");
	return ret;

err_out2:
	class_destroy(batt_ch_class);
err_out1:
	unregister_chrdev(batt_ch_major, "batt_ch");
	return ret;
}

static void __exit batt_ch_exit(void) {
	class_device_destroy(batt_ch_class, MKDEV(batt_ch_major, 0));
	class_destroy(batt_ch_class);
	unregister_chrdev(batt_ch_major, "batt_ch");
	pr_debug(KERN_INFO "batt_ch driver successfully unloaded\n");
}


module_init(batt_ch_init);
module_exit(batt_ch_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Olivier Le Roy <olivier.leroy@sagem.com>");
MODULE_DESCRIPTION("Battery charger driver");
