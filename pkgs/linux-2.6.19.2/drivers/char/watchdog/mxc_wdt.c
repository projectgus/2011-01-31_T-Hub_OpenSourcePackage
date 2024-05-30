/*
 * linux/drivers/char/watchdog/mxc_wdt.c
 *
 * Watchdog driver for FSL MXC. It is based on omap1610_wdt.c
 *
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * 2005 (c) MontaVista Software, Inc.  All Rights Reserved.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 *
 * 20051207: <AKuster@mvista.com>
 *	     	Full rewrite based on
 *		linux-2.6.15-rc5/drivers/char/watchdog/omap_wdt.c
 *	     	Add platform resource support
 *
 */

/*!
 * @file mxc_wdt.c
 *
 * @brief watch driver
 *
 * @ingroup Timers
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/bitops.h>

#include <asm/hardware.h>
#include "mxc_wdt.h"
#define DVR_VER "2.0"

#define WDOG_SEC_TO_COUNT(s)  ((s * 2) << 8)
#define WDOG_COUNT_TO_SEC(c)  ((c >> 8) / 2)

static int mxc_wdt_users;
static struct clk *mxc_wdt_clk;

static unsigned int timer_margin = TIMER_MARGIN_DEFAULT;
module_param(timer_margin, uint, 0);
MODULE_PARM_DESC(timer_margin, "initial watchdog timeout (in seconds)");

static void mxc_wdt_ping(void)
{
	/* issue the service sequence instructions */
	__raw_writew(WDT_MAGIC_1, MXC_WDT_WSR);
	__raw_writew(WDT_MAGIC_2, MXC_WDT_WSR);
}

static void mxc_wdt_config(void)
{
	u16 val;

	val = WDOG_SEC_TO_COUNT(TIMER_MARGIN_MAX) |
	      WCR_WDA_BIT | WCR_SRS_BIT | WCR_WDBG_BIT;
	__raw_writew(val, MXC_WDT_WCR);
}

static void mxc_wdt_enable(void)
{
	u16 val;

	val = __raw_readw(MXC_WDT_WCR);
	val |= WCR_WDE_BIT;
	__raw_writew(val, MXC_WDT_WCR);
}

static void mxc_wdt_disable(void)
{
	/* disable not supported by this chip */
}

static void mxc_wdt_adjust_timeout(unsigned int new_timeout)
{
	if (new_timeout < TIMER_MARGIN_MIN)
		new_timeout = TIMER_MARGIN_DEFAULT;
	if (new_timeout > TIMER_MARGIN_MAX)
		new_timeout = TIMER_MARGIN_MAX;
	timer_margin = new_timeout;
}

static unsigned int mxc_wdt_get_timeout(void)
{
	u16 val;

	val = __raw_readw(MXC_WDT_WCR);
	return WDOG_COUNT_TO_SEC(val);
}

static unsigned int mxc_wdt_get_boot_reason(void)
{
	u16 val;

	val = __raw_readw(MXC_WDT_WRSR);
	return val;
}

static void mxc_wdt_set_timeout(void)
{
	u16 val;
	val = __raw_readw(MXC_WDT_WCR);
	val = (val & 0x00FF) | WDOG_SEC_TO_COUNT(timer_margin);
	__raw_writew(val, MXC_WDT_WCR);
	val = __raw_readw(MXC_WDT_WCR);
	timer_margin = WDOG_COUNT_TO_SEC(val);
}

/*
 *	Allow only one task to hold it open
 */

static int mxc_wdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(1, (unsigned long *)&mxc_wdt_users))
		return -EBUSY;

	mxc_wdt_config();
	mxc_wdt_set_timeout();
	mxc_wdt_enable();
	mxc_wdt_ping();

	return 0;
}

static int mxc_wdt_release(struct inode *inode, struct file *file)
{
	/*
	 *      Shut off the timer unless NOWAYOUT is defined.
	 */
#ifndef CONFIG_WATCHDOG_NOWAYOUT
	mxc_wdt_disable();
#else
	printk(KERN_CRIT "mxc_wdt: Unexpected close, not stopping!\n");
#endif
	mxc_wdt_users = 0;
	return 0;
}

static ssize_t
mxc_wdt_write(struct file *file, const char __user * data,
	      size_t len, loff_t * ppos)
{
	/* Refresh LOAD_TIME. */
	if (len)
		mxc_wdt_ping();
	return len;
}

static int
mxc_wdt_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{
	unsigned int new_margin;
	int boot_reason;

	static struct watchdog_info ident = {
		.identity = "MXC Watchdog",
		.options = WDIOF_SETTIMEOUT,
		.firmware_version = 0,
	};

	switch (cmd) {
	default:
		return -ENOIOCTLCMD;
	case WDIOC_GETSUPPORT:
		return copy_to_user((struct watchdog_info __user *)arg, &ident,
				    sizeof(ident));
	case WDIOC_GETSTATUS:
		return put_user(0, (int __user *)arg);

	case WDIOC_GETBOOTSTATUS:
		boot_reason = mxc_wdt_get_boot_reason();
		return put_user(boot_reason, (unsigned int __user *)arg);

	case WDIOC_KEEPALIVE:
		mxc_wdt_ping();
		return 0;

	case WDIOC_SETTIMEOUT:
		if (get_user(new_margin, (unsigned int __user *)arg))
			return -EFAULT;

		mxc_wdt_adjust_timeout(new_margin);
		mxc_wdt_disable();
		mxc_wdt_set_timeout();
		mxc_wdt_enable();
		mxc_wdt_ping();
		return 0;

	case WDIOC_GETTIMEOUT:
		mxc_wdt_ping();
		new_margin = mxc_wdt_get_timeout();
		return put_user(new_margin, (unsigned int __user *)arg);
	}
}

static struct file_operations mxc_wdt_fops = {
	.owner = THIS_MODULE,
	.write = mxc_wdt_write,
	.ioctl = mxc_wdt_ioctl,
	.open = mxc_wdt_open,
	.release = mxc_wdt_release,
};

static struct miscdevice mxc_wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &mxc_wdt_fops
};

static int __init mxc_wdt_probe(struct platform_device *pdev)
{
	int ret;

	mxc_wdt_disable();
	mxc_wdt_adjust_timeout(timer_margin);

	mxc_wdt_users = 0;

	mxc_wdt_miscdev.dev = &pdev->dev;

	mxc_wdt_clk = clk_get(NULL, "wdog_clk");
	clk_enable(mxc_wdt_clk);

	ret = misc_register(&mxc_wdt_miscdev);
	if (ret)
		goto fail;

	pr_info("MXC Watchdog Timer: initial timeout %d sec\n",
		timer_margin);

	return 0;

      fail:
	pr_info("MXC Watchdog Probe failed\n");
	return ret;
}

static void mxc_wdt_shutdown(struct platform_device *pdev)
{
	mxc_wdt_disable();
	pr_info("MXC Watchdog shutdown\n");
}

static int __exit mxc_wdt_remove(struct platform_device *pdev)
{
	misc_deregister(&mxc_wdt_miscdev);
	pr_info("MXC Watchdog removed\n");
	return 0;
}

#ifdef	CONFIG_PM

/* REVISIT ... not clear this is the best way to handle system suspend; and
 * it's very inappropriate for selective device suspend (e.g. suspending this
 * through sysfs rather than by stopping the watchdog daemon).  Also, this
 * may not play well enough with NOWAYOUT...
 */

static int mxc_wdt_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (mxc_wdt_users) {
		mxc_wdt_disable();
	}
	return 0;
}

static int mxc_wdt_resume(struct platform_device *pdev)
{
	if (mxc_wdt_users) {
		mxc_wdt_enable();
		mxc_wdt_ping();
	}
	return 0;
}

#else
#define	mxc_wdt_suspend NULL
#define	mxc_wdt_resume  NULL
#endif

static struct platform_driver mxc_wdt_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "mxc_wdt",
		   },
	.probe = mxc_wdt_probe,
	.shutdown = mxc_wdt_shutdown,
	.remove = __exit_p(mxc_wdt_remove),
	.suspend = mxc_wdt_suspend,
	.resume = mxc_wdt_resume,
};

static int __init mxc_wdt_init(void)
{
	pr_info("MXC WatchDog Driver %s\n", DVR_VER);

	if ((timer_margin < TIMER_MARGIN_MIN) ||
	    (timer_margin > TIMER_MARGIN_MAX)) {
		pr_info("MXC watchdog error. wrong timer_margin %d\n",
			timer_margin);
		pr_info("    Range: %d to %d seconds\n", TIMER_MARGIN_MIN,
			TIMER_MARGIN_MAX);
		return -EINVAL;
	}

	return platform_driver_register(&mxc_wdt_driver);
}

static void __exit mxc_wdt_exit(void)
{
	platform_driver_unregister(&mxc_wdt_driver);
	pr_info("MXC WatchDog Driver removed\n");
}

module_init(mxc_wdt_init);
module_exit(mxc_wdt_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
