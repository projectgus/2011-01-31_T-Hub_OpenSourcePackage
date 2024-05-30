/*
 * Copyright 2007-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_MMC_H__
#define __ASM_ARCH_MXC_MMC_H__

#include <linux/mmc/host.h>

struct mxc_mmc_platform_data {
	unsigned int ocr_mask;	/* available voltages */
	unsigned int min_clk;
	unsigned int max_clk;
	unsigned int card_inserted_state;
//      u32 (*translate_vdd)(struct device *, unsigned int);
	unsigned int (*status) (struct device *);
};

#endif
