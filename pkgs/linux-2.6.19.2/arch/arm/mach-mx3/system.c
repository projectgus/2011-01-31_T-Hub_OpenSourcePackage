/*
 * Copyright (C) 1999 ARM Limited
 * Copyright (C) 2000 Deep Blue Solutions Ltd
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
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
 */

#include <linux/clk.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/proc-fns.h>
#include <asm/system.h>
#include "crm_regs.h"

#include <asm/arch/pmic_external.h>
/*!
 * @defgroup MSL_MX31 i.MX31 Machine Specific Layer (MSL)
 */

/*!
 * @file mach-mx3/system.c
 * @brief This file contains idle and reset functions.
 *
 * @ingroup MSL_MX31
 */

extern int mxc_jtag_enabled;

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks.
	 */
/* TODO remove the #if 0 and find the cause of screen blinking */
#if 0
	if (!mxc_jtag_enabled) {
		cpu_do_idle();
	}
#endif
}

#define WDOG_WCR_REG                    IO_ADDRESS(WDOG_BASE_ADDR)
#define WDOG_WCR_SRS                    (1 << 4)

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_reset(char mode)
{
	struct clk *clk;

   /* mask irq to not treat interrupt */
   pmic_irq_mask();

   /* unmask maximum events to keep irq signal active */
   pmic_write_reg(REG_INTERRUPT_MASK_0, 0, 0xFFFFFF);
   pmic_write_reg(REG_INTERRUPT_MASK_1, 0, 0xFFFFFF);

	clk = clk_get(NULL, "wdog_clk");
	clk_enable(clk);

	/* Assert SRS signal */
	__raw_writew(__raw_readw(WDOG_WCR_REG) & ~WDOG_WCR_SRS, WDOG_WCR_REG);
}

/*
 * This function resets the system. It is called by soft_machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_soft_reset(char mode)
{
	struct clk *clk;

   /* mask irq to not treat interrupt */
   pmic_irq_mask();

   /* unmask maximum events to keep irq signal active */
   /* we can not write to spi in soft interrupt context */
   //pmic_write_reg(REG_INTERRUPT_MASK_0, 0, 0xFFFFFF);
   //pmic_write_reg(REG_INTERRUPT_MASK_1, 0, 0xFFFFFF);

	clk = clk_get(NULL, "wdog_clk");
	clk_enable(clk);

	/* Assert SRS signal */
	__raw_writew(__raw_readw(WDOG_WCR_REG) & ~WDOG_WCR_SRS, WDOG_WCR_REG);
}
