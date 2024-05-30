/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/init.h>
#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/setup.h>
#include <linux/delay.h>

extern const u32 system_rev_tbl[SYSTEM_REV_NUM][2];

/*!
 * @file plat-mxc/cpu_common.c
 *
 * @brief This file contains the common CPU initialization code.
 *
 * @ingroup MSL_MX31
 */

static int system_rev_updated = 0;

static void __init system_rev_setup(char **p)
{
	system_rev = simple_strtoul(*p, NULL, 16);
	system_rev_updated = 1;
}

/* system_rev=0x10 for pass 1; 0x20 for pass 2.0; 0x21 for pass 2.1; etc */
__early_param("system_rev=", system_rev_setup);

/*
 * This functions reads the IIM module and returns the system revision number.
 * It does the translation from the IIM silicon revision reg value to our own
 * convention so that it is more readable.
 */
static u32 read_system_rev(void)
{
	u32 val;
	u32 i;

	val = __raw_readl(SYSTEM_PREV_REG);

	/* If the IIM doesn't contain valid product signature, return
	 * the lowest revision number */
	if (MXC_GET_FIELD(val, IIM_PROD_REV_LEN, IIM_PROD_REV_SH) !=
	    PROD_SIGNATURE) {
		printk(KERN_ALERT
		       "Can't find valid PROD_REV. Default Rev=0x%x\n",
		       SYSTEM_REV_MIN);
		return SYSTEM_REV_MIN;
	}

	/* Now trying to retrieve the silicon rev from IIM's SREV register */
	val = __raw_readl(SYSTEM_SREV_REG);

	/* ckeck SILICON_REV[7:0] first with ROM ver at [3:2] masked off */
	for (i = 0; i < SYSTEM_REV_NUM; i++) {
		if ((val & 0xF3) == system_rev_tbl[i][0]) {
			return system_rev_tbl[i][1];
		}
	}
	/* check again with SILICON_REV[3:0] masked off */
	for (i = 0; i < SYSTEM_REV_NUM; i++) {
		if ((val & 0xF0) == system_rev_tbl[i][0]) {
			return system_rev_tbl[i][1];
		}
	}
	/* Reach here only the SREV value unknown. Maybe due to new tapeout? */
	if (i == SYSTEM_REV_NUM) {
		val = system_rev_tbl[SYSTEM_REV_NUM - 1][1];
		printk(KERN_ALERT "WARNING: Can't find valid system rev\n");
		printk(KERN_ALERT "Assuming last known system_rev=0x%x\n", val);
		return val;
	}
	return 0;
}

/*
 * Update the system_rev value.
 * If no system_rev is passed in through the command line, it gets the value
 * from the IIM module. Otherwise, it uses the pass-in value.
 */
static void system_rev_update(void)
{
	int i;

	if (!system_rev_updated) {
		/* means NO value passed-in through command line */
		system_rev = read_system_rev();
		pr_info("system_rev is: 0x%x\n", system_rev);
	} else {
		pr_info("Command line passed system_rev: 0x%x\n", system_rev);
		for (i = 0; i < SYSTEM_REV_NUM; i++) {
			if (system_rev == system_rev_tbl[i][1]) {
				break;
			}
		}
		/* Reach here only the SREV value unknown. Maybe due to new tapeout? */
		if (i == SYSTEM_REV_NUM) {
			panic("Command system_rev is unknown\n");
		}
	}
}

int mxc_jtag_enabled;		/* OFF: 0 (default), ON: 1 */

/*
 * Here are the JTAG options from the command line. By default JTAG
 * is OFF which means JTAG is not connected and WFI is enabled
 *
 *       "on" --  JTAG is connected, so WFI is disabled
 *       "off" -- JTAG is disconnected, so WFI is enabled
 */

static void __init jtag_wfi_setup(char **p)
{
	if (memcmp(*p, "on", 2) == 0) {
		mxc_jtag_enabled = 1;
		*p += 2;
	} else if (memcmp(*p, "off", 3) == 0) {
		mxc_jtag_enabled = 0;
		*p += 3;
	}
}

__early_param("jtag=", jtag_wfi_setup);

void mxc_cpu_common_init(void)
{
	system_rev_update();
}
