/*
 *  Spi driver for dect module.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: spi_dect.h
 *  Creation date: 17/07/2009
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


#ifndef SPI_DECT_PRIV_H
#define SPI_DECT_PRIV_H
#include <linux/fs.h>
#define DEBUG
#include <linux/platform_device.h>
#include <asm/arch/mxc_spi.h>
#include <linux/delay.h>
#include <asm/arch/gpio.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <asm/arch/mxc_spi.h>

extern struct mxc_spi_unique_def spi_ver_0_7;
extern struct mxc_spi_unique_def spi_ver_0_5;
extern struct mxc_spi_unique_def spi_ver_0_4;
extern struct mxc_spi_unique_def spi_ver_0_0;

extern int gpio_spi_active(int cspi_mod);
extern void gpio_spi_inactive(int cspi_mod);

extern struct file_operations spi_dect_fops;

extern struct list_head spi_dect_dev_list;
extern struct mutex spi_dect_dev_list_lock;

/* taken from spidev */
#define SPI_DECT_MAJOR		153 /* assigned */
#define SPI_DECT_MINORS		32 

extern unsigned long minors[SPI_DECT_MINORS / BITS_PER_LONG];

struct spi_dect_context {
	struct spi_device 	*spi;
	struct spi_master	*master;
	struct list_head	device_entry;
	dev_t			devt;
	int			in_use;

	struct mutex		message_mutex;
	struct list_head 	message_list;
};

#endif
