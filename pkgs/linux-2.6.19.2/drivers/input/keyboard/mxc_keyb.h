/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 07/01/2009 
 * Copyright (c) 2010 Sagemcom All rights reserved:
 * 27/10/2009: Configure KPP module for hardware version (V5)
 * 07/01/2009: Fix pr 2660. Kernel panic happened when kernel timer's callback (soft interrupt) was interrupted by gpio's interrupt callback (hard interrupt). BUG_ON() was used to test if mxc_kpp_scan_matrix() was called again before finishing. 
 *	-	removing kernel timer.
 *	-	using workqueue to get sequential call of {kernel timer,gpio,kpp}'s callback (no concurrency between them of calling mxc_kpp_scan_matrix()).
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
 * @defgroup keypad Keypad Driver
 */

/*!
 * @file mxc_keyb.h
 *
 * @brief MXC keypad header file.
 *
 * @ingroup keypad
 */
#ifndef __MXC_KEYB_H__
#define __MXC_KEYB_H__

/*!
 * Keypad Module Name
 */
#define MOD_NAME  "mxckpd"

/*!
 * Keypad irq number
 */
#define KPP_IRQ  INT_KPP

/*!
 * XLATE mode selection
 */
#define KEYPAD_XLATE        0

/*!
 * RAW mode selection
 */
#define KEYPAD_RAW          1

/*!
 * Maximum number of keys.
 */
#define MXC_MAXKEY        (MAXROW * MAXCOL)

/*!
 * This define indicates break scancode for every key release. A constant
 * of 128 is added to the key press scancode.
 */
#define  MXC_KEYRELEASE   128

/*
 * _reg_KPP_KPCR   _reg_KPP_KPSR _reg_KPP_KDDR _reg_KPP_KPDR
 * Keypad Control Register Address
 */
#define KPCR    IO_ADDRESS(KPP_BASE_ADDR + 0x00)

/*
 * Keypad Status Register Address
 */
#define KPSR    IO_ADDRESS(KPP_BASE_ADDR + 0x02)

/*
 * Keypad Data Direction Address
 */
#define KDDR    IO_ADDRESS(KPP_BASE_ADDR + 0x04)

/*
 * Keypad Data Register
 */
#define KPDR    IO_ADDRESS(KPP_BASE_ADDR + 0x06)

#ifdef CONFIG_MACH_MX27MEDIAPHONE
// SAGEM 
#define GPIOE_DDIR IO_ADDRESS(GPIO_BASE_ADDR + 0x400)
#define GPIOE_ICRA IO_ADDRESS(GPIO_BASE_ADDR + 0x40C)
#define GPIOE_GIUS IO_ADDRESS(GPIO_BASE_ADDR + 0x420)
#define GPIOE_SSR  IO_ADDRESS(GPIO_BASE_ADDR + 0x424)
#define GPIOE_ICR1 IO_ADDRESS(GPIO_BASE_ADDR + 0x428)
#define GPIOE_MASK IO_ADDRESS(GPIO_BASE_ADDR + 0x430)
#define GPIOE_ISR1 IO_ADDRESS(GPIO_BASE_ADDR + 0x434)
#define GPIO_VISIO_START 0x800
#endif // CONFIG_MACH_MX27MEDIAPHONE

#ifdef CONFIG_MACH_MX31HSV1
// SAGEM 
#define GPIOE_MASK IO_ADDRESS(GPIO1_BASE_ADDR + 0x14)
#define GPIOE_SSR  IO_ADDRESS(GPIO1_BASE_ADDR + 0x08) //Regsiter PSR1
#define GPIOE_ISR1 IO_ADDRESS(GPIO1_BASE_ADDR + 0x18)
#define GPIO_VISIO_START 0x2000000
#endif // CONFIG_MACH_MX31MEDIAPHONE
/*
 * Key Press Interrupt Status bit
 */
#define KBD_STAT_KPKD        0x01

/*
 * Key Release Interrupt Status bit
 */
#define KBD_STAT_KPKR        0x02

/*
 * Key Depress Synchronizer Chain Status bit
 */
#define KBD_STAT_KDSC        0x04

/*
 * Key Release Synchronizer Status bit
 */
#define KBD_STAT_KRSS        0x08

/*
 * Key Depress Interrupt Enable Status bit
 */
#define KBD_STAT_KDIE        0x100

/*
 * Key Release Interrupt Enable
 */
#define KBD_STAT_KRIE        0x200

/*
 * Keypad Clock Enable
 */
#define KBD_STAT_KPPEN       0x400

/*!
 * Buffer size of keypad queue. Should be a power of 2.
 */
#define KPP_BUF_SIZE    128

/*!
 * Test whether bit is set for integer c
 */
#define TEST_BIT(c, n) ((c) & (0x1 << (n)))

/*!
 * Set nth bit in the integer c
 */
#define BITSET(c, n)   ((c) | (1 << (n)))

/*!
 * Reset nth bit in the integer c
 */
#define BITRESET(c, n) ((c) & ~(1 << (n)))

/*!
 * This enum represents the keypad state machine to maintain debounce logic
 * for key press/release.
 */
enum KeyState {

	/*!
	 * Key press state.
	 */
	KStateUp,

	/*!
	 * Key press debounce state.
	 */
	KStateFirstDown,

	/*!
	 * Key release state.
	 */
	KStateDown,

	/*!
	 * Key release debounce state.
	 */
	KStateFirstUp
};

/*!
 * Keypad Private Data Structure
 */
typedef struct keypad_priv {

	/*!
	 * Keypad state machine.
	 */
	enum KeyState iKeyState;

	/*!
	 * Number of rows configured in the keypad matrix
	 */
	unsigned long kpp_rows;

	/*!
	 * Number of Columns configured in the keypad matrix
	 */
	unsigned long kpp_cols;

#ifdef KPP_WITH_WORKQUEUE
	struct workqueue_struct	*queue;
	struct work_struct	work_timer;
	struct work_struct	work_gpio;
	struct work_struct	work_kpp;
#else
	/*!
	 * Timer used for Keypad polling.
	 */
	struct timer_list poll_timer;
#endif

} keypad_priv;

#endif				/* __MXC_KEYB_H__ */
