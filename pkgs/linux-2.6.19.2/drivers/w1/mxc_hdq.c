/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @defgroup MXC_OWIRE MXC Driver for owire interface
 */

/*!
 * @file mxc_hdq.c
 *
 * @brief Driver for the Freescale Semiconductor MXC owire interface.
 *
 *
 * @ingroup MXC_OWIRE
 */

/*!
 * Include Files
 */

#include <asm/atomic.h>
#include <asm/types.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/pci_ids.h>
#include <linux/pci.h>
#include <linux/timer.h>
#include <linux/config.h>
#include <linux/init.h>
#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/arch/clock.h>

#include "hdq.h"
#include "hdq_int.h"
#include "hdq_log.h"

/*
 * mxc function declarations
 */

static int __devinit mxc_hdq_probe(struct device *pdev);
static int __devexit mxc_hdq_remove(struct device *dev);

extern void gpio_owire_active(void);
extern void gpio_owire_inactive(void);

/*
 * MXC HDQ Register offsets
 */
#define MXC_HDQ_CONTROL          0x00
#define MXC_HDQ_TIME_DIVIDER     0x02
#define MXC_HDQ_RESET            0x04

DEFINE_RAW_SPINLOCK(hdq_lock);

/*!
 * This structure contains pointers to  callback functions.
 */

/// SAGEM: Need to keep the name mxcw1 in order to match with the bus_name defined in /arc/arm/mach-mx27/devices.c

static struct device_driver mxc_hdq_driver = {
	.name = "mxcw1",
	.bus = &platform_bus_type,
	.probe = mxc_hdq_probe,
	.remove = mxc_hdq_remove,
};

/*!
 * This structure is used to store
 * information specific to hdq module.
 */

struct mxc_hdq_device {
	char *base_address;
	unsigned long found;
	unsigned int clkdiv;
	struct hdq_bus_master *bus_master;
};

/*
 * one-wire function declarations
 */

static u8 mxc_hdq_touch_bit(unsigned long, u8);
static u8 mxc_hdq_reset_bus(unsigned long);

/*
 * this is the low level routine to
 * reset the device on the One Wire interface
 * on the hardware
 * @param data  the data field of the hdq device structure
 * @return the function returns 0 when the reset pulse has
 *  been generated
 */
static u8 mxc_hdq_reset_bus(unsigned long data)
{
	volatile u8 reg_val;
	u8 ret;
	struct mxc_hdq_device *dev = (struct mxc_hdq_device *)data;

	__raw_writeb(0x80, (dev->base_address + MXC_HDQ_CONTROL));

	do {
		reg_val = __raw_readb(dev->base_address + MXC_HDQ_CONTROL);
	} while (((reg_val >> 7) & 0x1) != 0);

	ret = ((reg_val >> 6) & 0x1);

	return ret;
}

/*!
 * this is the low level routine to read/write a bit on the One Wire 
 * interface on the hardware
 * @param data  the data field of the hdq device structure
 * @param bit  0 = write-0 cycle, 1 = write-1/read cycle
 * @return the function returns the bit read (0 or 1)
 */
static u8 mxc_hdq_touch_bit(unsigned long data, u8 bit)
{

	volatile u8 reg_val;
	struct mxc_hdq_device *dev = (struct mxc_hdq_device *)data;
	u8 ret = 0;

	if (0 == bit) {
		__raw_writeb((1 << 5), (dev->base_address + MXC_HDQ_CONTROL));

		do {
			reg_val =
			    __raw_readb(dev->base_address + MXC_HDQ_CONTROL);
		} while (0 != ((reg_val >> 5) & 0x1));
	}

	else {
		__raw_writeb((1 << 4), dev->base_address + MXC_HDQ_CONTROL);
		do {
			reg_val =
			    __raw_readb(dev->base_address + MXC_HDQ_CONTROL);
		} while (0 != ((reg_val >> 4) & 0x1));

		reg_val =
		    (((__raw_readb(dev->base_address + MXC_HDQ_CONTROL)) >> 3) &
		     0x1);
		ret = (u8) (reg_val);
	}

	return ret;
}

/*!
 * this routine sets the One Wire clock
 * to a value of 1 Mhz, as required by
 * hardware.
 * @param   dev   the device structure for hdq
 * @return  The function returns void
 */
static void mxc_hdq_hw_init(struct mxc_hdq_device *dev)
{
	mxc_clks_enable(OWIRE_CLK);

	/* set the timer divider clock to divide by 65 */
	/* as the clock to the One Wire is at 66.5MHz */
	__raw_writeb(dev->clkdiv, dev->base_address + MXC_HDQ_TIME_DIVIDER);

	return;
}

static int mxc_hdq_getdiv(void)
{
	return ((mxc_get_clocks(OWIRE_CLK) / 1000000) - 1);
}

/*!
 * this is the probe routine for the One Wire driver.
 * It is called during the driver initilaization.
 * @param   pdev   the platform device structure for hdq
 * @return The function returns 0 on success
 * and a non-zero value on failure
 *
 */
static int __devinit mxc_hdq_probe(struct device *pdev)
{
	struct mxc_hdq_device *dev;
	int err = 0;

	dev = kmalloc(sizeof(struct mxc_hdq_device) +
		      sizeof(struct hdq_bus_master), GFP_KERNEL);
	if (!dev) {
		return -ENOMEM;
	}

	memset(dev, 0,
	       sizeof(struct mxc_hdq_device) + sizeof(struct hdq_bus_master));
	dev->bus_master = (struct hdq_bus_master *)(dev + 1);
	dev->found = 1;
	dev->clkdiv = mxc_hdq_getdiv();
	dev->base_address = (void *)IO_ADDRESS(OWIRE_BASE_ADDR);

	mxc_hdq_hw_init(dev);
	dev->bus_master->data = (unsigned long)dev;
	dev->bus_master->touch_bit = &mxc_hdq_touch_bit;
	dev->bus_master->reset_bus = &mxc_hdq_reset_bus;

	err = hdq_add_master_device(dev->bus_master);

	if (err)
	  {
	    goto err_out_free_device;
	  }
	dev_set_drvdata(pdev, dev);
	return 0;

      err_out_free_device:

	kfree(dev);
	return err;
}

/*
 * disassociate the hdq device from the driver
 * @param   dev   the device structure for hdq
 * @return  The function returns void
 */
static int mxc_hdq_remove(struct device *dev)
{
	struct mxc_hdq_device *pdev = dev_get_drvdata(dev);

	if (pdev->found) {
		hdq_remove_master_device(pdev->bus_master);
	}
	dev_set_drvdata(dev, NULL);

	return 0;
}

/*
 * initialize the driver
 * @return The function returns 0 on success
 * and a non-zero value on failure
 */

static int __init mxc_hdq_init(void)
{
	int ret;

	printk(KERN_INFO "Serial: MXC OWire driver Test\n");
	gpio_owire_active();
	ret = driver_register(&mxc_hdq_driver);
	if (0 != ret) {
		return ret;
	}

	return ret;
}

/*
 * cleanup before the driver exits
 */
static void mxc_hdq_exit(void)
{
	gpio_owire_inactive();
	driver_unregister(&mxc_hdq_driver);
}

module_init(mxc_hdq_init);
module_exit(mxc_hdq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laurent Isenegger");
MODULE_DESCRIPTION("Driver for HDQ on MXC");
