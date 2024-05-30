/*
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 06/08/2007 
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
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
 * @file arch-mxc/hardware.h
 * @brief This file contains the hardware definitions of the board.
 *
 * @ingroup MSL_MX27 MSL_MX31
 */
#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#define __ASM_ARCH_MXC_HARDWARE_H__

#include <asm/sizes.h>

/*!
 * defines PCIO_BASE (not used but needed for compilation)
 */
#define PCIO_BASE		0

/*!
 * This macro is used to get certain bit field from a number
 */
#define MXC_GET_FIELD(val, len, sh)          ((val >> sh) & ((1 << len) - 1))

/*!
 * This macro is used to set certain bit field inside a number
 */
#define MXC_SET_FIELD(val, len, sh, nval)    ((val & ~(((1 << len) - 1) << sh)) | nval << sh)

/*
 * ---------------------------------------------------------------------------
 * Processor specific defines
 * ---------------------------------------------------------------------------
 */
#define CHIP_REV_1_0		0x10
#define CHIP_REV_1_1		0x11
#define CHIP_REV_1_2		0x12
#define CHIP_REV_1_3		0x13
#define CHIP_REV_2_0		0x20
#define CHIP_REV_2_1		0x21
#define CHIP_REV_2_2		0x22
#define CHIP_REV_2_3		0x23
#define CHIP_REV_3_0		0x30
#define CHIP_REV_3_1		0x31
#define CHIP_REV_3_2		0x32

#ifdef CONFIG_ARCH_MX3
#include <asm/arch/mx31.h>
#define cpu_is_mx31()		(1)
#else
#define cpu_is_mx31()		(0)
#endif

#ifdef CONFIG_ARCH_MX27
#include <asm/arch/mx27.h>
#define cpu_is_mx27()		(1)
#else
#define cpu_is_mx27()		(0)
#endif

#if !defined(__ASSEMBLY__) && !defined(__MXC_BOOT_UNCOMPRESS)
extern unsigned int system_rev;
#define _is_rev(rev) ((system_rev == rev) ? 1 : ((system_rev < rev) ? -1 : 2))

#define MXC_REV(type)				\
static inline int type## _rev (int rev)		\
{						\
	return (type() ? _is_rev(rev) : 0);	\
}

/*
 * Create inline functions to test for cpu revision
 * Function name is cpu_is_<cpu name>_rev(rev)
 *
 * Returns:
 *	 0 - not the cpu queried
 *	 1 - cpu and revision match
 *	 2 - cpu matches, but cpu revision is greater than queried rev
 *	-1 - cpu matches, but cpu revision is less than queried rev
 */
MXC_REV(cpu_is_mx27);
MXC_REV(cpu_is_mx31);
#endif

#include <asm/arch/mxc.h>

#define MXC_MAX_GPIO_LINES      (GPIO_NUM_PIN * GPIO_PORT_NUM)

/*
 * ---------------------------------------------------------------------------
 * Board specific defines
 * ---------------------------------------------------------------------------
 */
#define MXC_EXP_IO_BASE         (MXC_GPIO_BASE + MXC_MAX_GPIO_LINES)

#if defined(CONFIG_MACH_MX31ADS)
#include <asm/arch/board-mx31ads.h>
#elif defined(CONFIG_MACH_MX31LSV0)
#include <asm/arch/board-mx31livescreen.h>   // livescreen
#elif defined(CONFIG_MACH_MX31HSV1)
#include <asm/arch/board-mx31homescreen.h>   // homescreen
#elif defined(CONFIG_MACH_MX27ADS)
#include <asm/arch/board-mx27ads.h>
#elif  defined(CONFIG_MACH_MX27MEDIAPHONE)
#include <asm/arch/board-mx27mediaphone.h>   // mediaphone
#else
#error No MACH defined !
#endif

#ifndef MXC_MAX_EXP_IO_LINES
#define MXC_MAX_EXP_IO_LINES 0
#endif

#define MXC_MAX_VIRTUAL_INTS	16
#define MXC_VIRTUAL_INTS_BASE	(MXC_EXP_IO_BASE + MXC_MAX_EXP_IO_LINES)
#define MXC_SDIO1_CARD_IRQ	MXC_VIRTUAL_INTS_BASE
#define MXC_SDIO2_CARD_IRQ	(MXC_VIRTUAL_INTS_BASE + 1)
#define MXC_SDIO3_CARD_IRQ	(MXC_VIRTUAL_INTS_BASE + 2)

#define MXC_MAX_INTS            (MXC_MAX_INT_LINES + \
                                MXC_MAX_GPIO_LINES + \
                                MXC_MAX_EXP_IO_LINES + \
                                MXC_MAX_VIRTUAL_INTS)

#endif				/* __ASM_ARCH_MXC_HARDWARE_H__ */
