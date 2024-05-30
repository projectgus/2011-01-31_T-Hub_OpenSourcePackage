/*
 * Copyright 2007-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 04/07/2007
 * Copyright (c) 2010 Sagemcom All rights reserved:
 * - Merged changes of i.MX51 BSP in pmic_adc and mxc_ts
 * - Change sleep duration
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
 * @file mxc_ts.c
 *
 * @brief Driver for the Freescale Semiconductor MXC touchscreen.
 *
 * The touchscreen driver is designed as a standard input driver which is a
 * wrapper over low level PMIC driver. Most of the hardware configuration and
 * touchscreen functionality is implemented in the low level PMIC driver. During
 * initialization, this driver creates a kernel thread. This thread then calls
 * PMIC driver to obtain touchscreen values continously. These values are then
 * passed to the input susbsystem.
 *
 * @ingroup touchscreen
 */
#include <linux/platform_device.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/init.h>
#include <asm/arch/hardware.h>
#include <linux/delay.h>
#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_adc.h>

#define MXC_TS_NAME	"mxc_ts"

#if defined(CONFIG_MACH_MX31HSV1)
	#define MX31HSV1_TOUCH_MIN_X 30
	#define MX31HSV1_TOUCH_MAX_X 1000
	#define MX31HSV1_TOUCH_MIN_Y 60
	#define MX31HSV1_TOUCH_MAX_Y 1000
	#define MX31HSV1_TOUCH_MIN_ABS 0
	#define MX31HSV1_TOUCH_MAX_ABS 1
#endif

static struct input_dev *mxc_inputdev = NULL;
static u32 input_ts_installed;

static int ts_thread(void *arg)
{
	t_touch_screen ts_sample;
	s32 wait = 0;
	daemonize("mxc_ts");
	while (input_ts_installed) {
		try_to_freeze();
		memset(&ts_sample, 0, sizeof(t_touch_screen));
		if (0 != pmic_adc_get_touch_sample(&ts_sample, !wait))
			continue;
		input_report_abs(mxc_inputdev, ABS_X, ts_sample.x_position);
		input_report_abs(mxc_inputdev, ABS_Y, ts_sample.y_position);
		input_report_abs(mxc_inputdev, ABS_PRESSURE,
				 ts_sample.contact_resistance);
		input_sync(mxc_inputdev);

		wait = ts_sample.contact_resistance;
		msleep(10);
	}

	return 0;
}

static int __init mxc_ts_init(void)
{
	if (!is_pmic_adc_ready())
		return -ENODEV;

	mxc_inputdev = input_allocate_device();
	if (!mxc_inputdev) {
		printk(KERN_ERR
		       "mxc_ts_init: not enough memory for input device\n");
		return -ENOMEM;
	}

	mxc_inputdev->name = MXC_TS_NAME;
	mxc_inputdev->evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);
	mxc_inputdev->keybit[LONG(BTN_TOUCH)] |= BIT(BTN_TOUCH);
	mxc_inputdev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
#if defined(CONFIG_MACH_MX31HSV1)
	input_set_abs_params(mxc_inputdev, ABS_X,       MX31HSV1_TOUCH_MIN_X,   MX31HSV1_TOUCH_MAX_X,0,0);
	input_set_abs_params(mxc_inputdev, ABS_Y,       MX31HSV1_TOUCH_MIN_Y,   MX31HSV1_TOUCH_MAX_Y,0,0);
	input_set_abs_params(mxc_inputdev, ABS_PRESSURE,MX31HSV1_TOUCH_MIN_ABS, MX31HSV1_TOUCH_MAX_ABS,0,0);
#endif
	input_register_device(mxc_inputdev);

	input_ts_installed = 1;
	kernel_thread(ts_thread, NULL, CLONE_VM | CLONE_FS);
	printk("mxc input touchscreen loaded\n");
	return 0;
}

static void __exit mxc_ts_exit(void)
{
	input_ts_installed = 0;
	input_unregister_device(mxc_inputdev);

	if (mxc_inputdev) {
		input_free_device(mxc_inputdev);
		mxc_inputdev = NULL;
	}
}

late_initcall(mxc_ts_init);
module_exit(mxc_ts_exit);

MODULE_DESCRIPTION("MXC input touchscreen driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
