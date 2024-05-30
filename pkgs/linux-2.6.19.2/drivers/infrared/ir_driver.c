/*
 *  Infrared driver for remote control.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: ir_driver.c
 *  Creation date: 10/10/2007
 *  Author: Stephane Lapie
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

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm-arm/arch-mxc/gpio.h>
#include <asm-arm/io.h>
#include <asm/arch-mxc/mx31_pins.h>

#include "../../arch/arm/mach-mx3/iomux.h"
#include "ir_driver.h"

/*
** Make a signal period with the given alpha value.
*/
static void		make_wave(int alpha)
{
    mxc_set_gpio_dataout(MX31_PIN_TXD2, 1);
    udelay(alpha);
    mxc_set_gpio_dataout(MX31_PIN_TXD2, 0);
    udelay(alpha);
}

/*
** Device write callback.
** Take the given IR trame durations and make
** make the signal.
*/
static ssize_t		ir_write(struct file *file,
				 const char *buf,
				 size_t count,
				 loff_t *ppos)
{
    int			z_i;
    int			z_j;
    int			*z_pTrame;
    int			z_nPeriod;

    z_pTrame = (int *)buf;
    z_nPeriod = z_pTrame[1] * 2;
    for (z_i = 4; z_i < count; z_i += 2)
    {
	for (z_j = 0; z_j < z_pTrame[z_i]; z_j++)
	    make_wave(z_pTrame[1]);
	mxc_set_gpio_dataout(MX31_PIN_TXD2, 0);
	udelay((z_pTrame[z_i + 1] * z_nPeriod));
    }
    for (z_j = 0; z_j < z_pTrame[z_i - 2]; z_j++)
	make_wave(z_pTrame[1]);
    return (0);
}

/*
** Callbacks assignement structure
*/
struct file_operations	fops =
{
    .write = ir_write,
};

/*
** Module init.
** Request for needed GPIO, set his direction, set his
** data to zero and register IRMOD_DEVICE.
*/
static int __init	ir_init(void)
{
    if (mxc_request_iomux(MX31_PIN_TXD2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE) == -1)
    {
	printk("Infrared: could not register GPIO\n");
	return (0);
    }
    iomux_config_mux(MX31_PIN_TXD2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
    mxc_set_gpio_direction(MX31_PIN_TXD2,GPIO_DIR_OUTPUT);
    mxc_set_gpio_dataout(MX31_PIN_TXD2, 0);
    if (register_chrdev(IRMOD_MAJOR, IRMOD_DEVICE, &fops) == -1)
    {
	printk("Infrared: could not register device\n");
	return (0);
    }
    printk("Infrared: IRMOD device initialized\n");
    return (0);
}

/*
** Module exit.
** Free GPIO mux and unregister IRMOD_DEVICE.
*/
static void __exit	ir_cleanup(void)
{
    mxc_free_iomux(MX31_PIN_TXD2, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
    unregister_chrdev(IRMOD_MAJOR, "irmod");
    printk("Infrared: IRMOD device disabled\n");
}

/*
** Entries asignement
*/
module_init(ir_init);
module_exit(ir_cleanup);

/*
** Module informations
*/
MODULE_AUTHOR(IRMOD_AUTHOR);
MODULE_DESCRIPTION(IRMOD_DESC);
