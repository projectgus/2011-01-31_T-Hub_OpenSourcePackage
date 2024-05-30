/*
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*!
 * @file mach-mx3/cpu.c
 *
 * @brief This file contains the CPU initialization code.
 *
 * @ingroup MSL_MX31
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/hardware.h>
#include <asm/io.h>

const u32 system_rev_tbl[SYSTEM_REV_NUM][2] = {
	/* SREV, own defined ver */
	{0x00, CHIP_REV_1_0},	/* MX31 PASS 1 */
	{0x10, CHIP_REV_1_1},	/* MX31 PASS 1.1 */
	{0x20, CHIP_REV_2_0},	/* MX31 PASS 2 FIXME: confirm */
};

/*!
 * CPU initialization. It is called by fixup_mxc_board()
 */
void __init mxc_cpu_init(void)
{
	/* Setup Peripheral Port Remap register for AVIC */
	asm("ldr r0, =0xC0000015				\n\
	 mcr p15, 0, r0, c15, c2, 4");
}

/*!
 * Post CPU init code
 *
 * @return 0 always
 */
static int __init post_cpu_init(void)
{
	volatile unsigned long aips_reg;

	/*
	 * S/W workaround: Clear the off platform peripheral modules
	 * Supervisor Protect bit for SDMA to access them.
	 */
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x40));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x44));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x48));
	__raw_writel(0x0, IO_ADDRESS(AIPS1_BASE_ADDR + 0x4C));
	aips_reg = __raw_readl(IO_ADDRESS(AIPS1_BASE_ADDR + 0x50));
	aips_reg &= 0x00FFFFFF;
	__raw_writel(aips_reg, IO_ADDRESS(AIPS1_BASE_ADDR + 0x50));

	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x40));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x44));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x48));
	__raw_writel(0x0, IO_ADDRESS(AIPS2_BASE_ADDR + 0x4C));
	aips_reg = __raw_readl(IO_ADDRESS(AIPS2_BASE_ADDR + 0x50));
	aips_reg &= 0x00FFFFFF;
	__raw_writel(aips_reg, IO_ADDRESS(AIPS2_BASE_ADDR + 0x50));

	return 0;
}

postcore_initcall(post_cpu_init);
