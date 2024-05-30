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
 * @file mxc_keyb.c
 *
 * @brief Driver for the Freescale Semiconductor MXC keypad port.
 *
 * The keypad driver is designed as a standard Input driver which interacts
 * with low level keypad port hardware. Upon opening, the Keypad driver
 * initializes the keypad port. When the keypad interrupt happens the driver
 * calles keypad polling timer and scans the keypad matrix for key
 * press/release. If all key press/release happened it comes out of timer and
 * waits for key press interrupt. The scancode for key press and release events
 * are passed to Input subsytem.
 *
 * @ingroup keypad
 */

/*!
 * Comment KPP_DEBUG to disable debug messages
 */
#define KPP_DEBUG        0

#if defined(KPP_DEBUG) && KPP_DEBUG
#define	DEBUG
#include <linux/kernel.h>
#endif

#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/arch/hardware.h>
#include <linux/kd.h>
#include <linux/fs.h>
#include <linux/kbd_kern.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#define KPP_WITH_WORKQUEUE
#ifdef KPP_WITH_WORKQUEUE
#include <linux/workqueue.h>
#else
#include <linux/timer.h>
#endif
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <asm/mach/keypad.h>
/*
 * Module header file
 */
#include "mxc_keyb.h"

#if defined(CONFIG_EVTPROBE) || defined(CONFIG_EVTPROBE2)
#include <linux/evtprobe_api.h>
#endif

#include <asm-arm/arch-mxc/gpio.h>
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
#define K_ROW_NUMBER_PATCH 1
#else
#define K_ROW_NUMBER_PATCH 0
#endif

/*!
 * Prototype functions for GPIO key
 */
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
int        gpio_buttons_active   (void);
void       gpio_buttons_inactive (void);
static int mxc_kpp_thread        (void * data);
#endif

/*!
 * This structure holds the keypad private data structure.
 */
static struct keypad_priv kpp_dev;

/*! Indicates if the key pad device is enabled. */
static unsigned int key_pad_enabled;

/*! Input device structure. */
static struct input_dev *mxckbd_dev = NULL;

/*! KPP clock handle. */
static struct clk *kpp_clk;

/*! This static variable indicates whether a key event is pressed/released. */
static unsigned short KPress;

/*! cur_rcmap and prev_rcmap array is used to detect key press and release. */
static unsigned short *cur_rcmap;	/* max 64 bits (8x8 matrix) */
static unsigned short *prev_rcmap;

/*!
 * Debounce polling period(10ms) in system ticks.
 */
static unsigned short KScanRate = (10 * HZ) / 1000;

static struct keypad_data *keypad;

static int has_learning_key;
/*!
 * These arrays are used to store press and release scancodes.
 */
static short **press_scancode;
static short **release_scancode;

static const unsigned short *mxckpd_keycodes;
static unsigned short mxckpd_keycodes_size;

#define press_left_code     30
#define press_right_code    29
#define press_up_code       28
#define press_down_code     27

#define rel_left_code       158
#define rel_right_code      157
#define rel_up_code         156
#define rel_down_code       155

#if defined(CONFIG_MACH_MX27MEDIAPHONE)
#define POWER_KEY_PORT    4
#define POWER_KEY_SIGNAL  11
#define MAINTAINER_MOD0      4
#define MAINTAINER_MOD1      1
#define MAINTAINER_MOD2      0

#elif defined(CONFIG_MACH_MX31HSV1)
#define POWER_KEY_GPIO       HOMESCREEN_V1_GPIO_POWER_KEY
#define POWER_KEY_IRQ        IOMUX_TO_IRQ(POWER_KEY_GPIO)
#define MAINTAINER_MOD0      2
#define MAINTAINER_MOD1      1
#define MAINTAINER_MOD2      0
#endif // CONFIG_MACH_MX27HSV1
/*!
 * These functions are used to configure and the GPIO pins for keypad to
 * activate and deactivate it.
 */
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
extern void gpio_keypad_active(void);
extern void gpio_keypad_inactive(void);

/*******************************************************************************
 * Private variables
 * ****************************************************************************/

static spinlock_t detection_lock = SPIN_LOCK_UNLOCKED;

static int    btn_power_pressed  = 0;
static u32    btn_power_irq_type = IRQT_RISING;
static struct task_struct *th2   = NULL;
#endif

/*******************************************************************************
 * Definition functions
 * ****************************************************************************/

/*!
 * This function is called for generating scancodes for key press and
 * release on keypad for the board.
 *
 *  @param   row        Keypad row pressed on the keypad matrix.
 *  @param   col        Keypad col pressed on the keypad matrix.
 *  @param   press      Indicated key press/release.
 *
 *  @return     Key press/release Scancode.
 */
static signed short mxc_scan_matrix_learning_key(int row, int col, int press)
{
	static unsigned first_row;
	static unsigned first_set = 0, flag = 0;
	signed short scancode = -1;

	if (press) {
		if ((3 == col) && ((3 == row) ||
					(4 == row) || (5 == row) || (6 == row))) {
			if (first_set == 0) {
				first_set = 1;
				first_row = row;
			} else {
				first_set = 0;
				if (((first_row == 6) || (first_row == 3))
						&& ((row == 6) || (row == 3)))
					scancode = press_down_code;
				else if (((first_row == 3) || (first_row == 5))
						&& ((row == 3) || (row == 5)))
					scancode = press_left_code;
				else if (((first_row == 6) || (first_row == 4))
						&& ((row == 6) || (row == 4)))
					scancode = press_right_code;
				else if (((first_row == 4) || (first_row == 5))
						&& ((row == 4) || (row == 5)))
					scancode = press_up_code;
				KPress = 1;
				kpp_dev.iKeyState = KStateUp;
				pr_debug("Press (%d, %d) scan=%d Kpress=%d\n",
						row, col, scancode, KPress);
			}
		} else {
			/*
			 * check for other keys only
			 * if the cursor key presses
			 * are not detected may be
			 * this needs better logic
			 */
			if ((0 == (cur_rcmap[3] & BITSET(0, 3))) &&
					(0 == (cur_rcmap[4] & BITSET(0, 3))) &&
					(0 == (cur_rcmap[5] & BITSET(0, 3))) &&
					(0 == (cur_rcmap[6] & BITSET(0, 3)))) {
				scancode = ((col * kpp_dev.kpp_rows) + row);
				KPress = 1;
				kpp_dev.iKeyState = KStateUp;
				flag = 1;
				pr_debug("Press (%d, %d) scan=%d Kpress=%d\n",
						row, col, scancode, KPress);
			}
		}
	} else {
		if ((flag == 0) && (3 == col)
				&& ((3 == row) || (4 == row) || (5 == row)
					|| (6 == row))) {
			if (first_set == 0) {
				first_set = 1;
				first_row = row;
			} else {
				first_set = 0;
				if (((first_row == 6) || (first_row == 3))
						&& ((row == 6) || (row == 3)))
					scancode = rel_down_code;
				else if (((first_row == 3) || (first_row == 5))
						&& ((row == 3) || (row == 5)))
					scancode = rel_left_code;
				else if (((first_row == 6) || (first_row == 4))
						&& ((row == 6) || (row == 4)))
					scancode = rel_right_code;
				else if (((first_row == 4) || (first_row == 5))
						&& ((row == 4) || (row == 5)))
					scancode = rel_up_code;
				KPress = 0;
				kpp_dev.iKeyState = KStateDown;
				pr_debug("Release (%d, %d) scan=%d Kpress=%d\n",
						row, col, scancode, KPress);
			}
		} else {
			/*
			 * check for other keys only
			 * if the cursor key presses
			 * are not detected may be
			 * this needs better logic
			 */
			if ((0 == (prev_rcmap[3] & BITSET(0, 3))) &&
					(0 == (prev_rcmap[4] & BITSET(0, 3))) &&
					(0 == (cur_rcmap[5] & BITSET(0, 3))) &&
					(0 == (cur_rcmap[6] & BITSET(0, 3)))) {
				scancode = ((col * kpp_dev.kpp_rows) + row) +
					MXC_KEYRELEASE;
				KPress = 0;
				flag = 0;
				kpp_dev.iKeyState = KStateDown;
				pr_debug("Release (%d, %d) scan=%d Kpress=%d\n",
						row, col, scancode, KPress);
			}
		}
	}
	return scancode;
}

#define MAX_COL					(8)
#define MAX_ROW					(8)
#define ROW_MASK(nrow)			((1<<(nrow))-1)
#define KEYB_HIST_DEBOUNCE 	(6)
#define KEYB_DEBOUNCE_DEPTH 	(4) /*!< cannot be above (KEYB_HIST_DEBOUNCE-1) */


u16 hist_col[MAX_COL][KEYB_HIST_DEBOUNCE] ;
u16 prev_value[MAX_COL] = {0};
/*
 * To have a transition: 
 * We need: event[n-N+1] MUST have change state from event[n-N] (the oldest) 
 *    with : n=now, N=memory, event="row bit event".
 * AND all event[n] to event[n-N+1] MUST be equal.
 *
 */

void debouncer_init(void)
{
	int col, j;
	for (col=0; col<MAX_COL; col++)
		for (j=0; j<KEYB_HIST_DEBOUNCE; j++)
			hist_col[col][j] = ( j & 1 ) ? 0xAAAA : 0x5555 ; /* broken chain */

	for (col=0; col<MAX_COL; col++)
		prev_value[col] = 0xFFFF ; /* released */ 
}

void debounce_add_tail(u16 hist[KEYB_HIST_DEBOUNCE], u16 new_value) {
	int i;
	
	for(i=0; i<KEYB_HIST_DEBOUNCE-1; i++) {
		hist[i] = hist[i+1];
	}
	hist[KEYB_HIST_DEBOUNCE-1] = new_value;
}

u16 debounce_is_value(u16 hist[KEYB_HIST_DEBOUNCE]) 
{
	int i;
	u16 event_mask = 0;
	for (i=0; i<KEYB_HIST_DEBOUNCE; i++) {
		event_mask |= ( hist[i] & ROW_MASK(kpp_dev.kpp_rows) );
	}
	return event_mask;
}

u16 debounce_getvalue(u16 hist[KEYB_HIST_DEBOUNCE], u16 prev_value) 
{
	int i, power2, row;
	int count ;
	int broken_chain;
	u16 confirmed = 0;
	u16 result = prev_value & ROW_MASK(kpp_dev.kpp_rows);

	/* we need to investigate each row separatly */
	for (row=0, power2=1; row < kpp_dev.kpp_rows; row++, power2=power2<<1) {
		/* the 'oldest' and the 'oldest-1', respectively [0] and [1] MUST differ */
#if 0
		pr_debug("row%d events=(%c %c ", 
				row,
				(hist[0] & power2) ? '1' : '0',
				(hist[1] & power2) ? '1' : '0' 
				);
#endif
		if ( 1 || (( hist[0] ^ hist[1] ) & power2) ) {
			count = 1;
			broken_chain = 0;

			for (i=1; i<KEYB_HIST_DEBOUNCE; i++) {
#if 0
				pr_debug("%c ", (hist[i] & power2) ? '1' : '0');
#endif
				if ( (hist[i] & power2) != (hist[0] & power2) ) 
					broken_chain = 1;

				if ( !broken_chain )
					count++;
			}
#if 0
		   pr_debug("] : chain length=%d\n", count);
#endif
			if ( count >= KEYB_DEBOUNCE_DEPTH ) {
				confirmed |= power2 ;
#if 0
				pr_debug("Confirmed: row%d\n", row);
#endif
			}
		}
	}
	/* clear and set confirmed bits */
	result = (result & ~confirmed) | (hist[0] & confirmed) ;
	return result;
}

u16 debouncer( int col, u16 new_value)
{
	u16 result;
	u16 in = new_value  ;

	// pr_debug("debouncer: in  0x%x\n", in );

	result = debounce_getvalue( hist_col[col], prev_value[col] );

	// pr_debug("debouncer: out 0x%x\n", result);

	prev_value[col] = result ;
	debounce_add_tail( hist_col[col], in );
	return result;
}


/*!
 * This function is called to scan the keypad matrix to find out the key press
 * and key release events. Make scancode and break scancode are generated for
 * key press and key release events.
 *
 * The following scanning sequence are done for
 * keypad row and column scanning,
 * -# Write 1's to KPDR[15:8], setting column data to 1's
 * -# Configure columns as totem pole outputs(for quick discharging of keypad
 * capacitance)
 * -# Configure columns as open-drain
 * -# Write a single column to 0, others to 1.
 * -# Sample row inputs and save data. Multiple key presses can be detected on
 * a single column.
 * -# Repeat steps the above steps for remaining columns.
 * -# Return all columns to 0 in preparation for standby mode.
 * -# Clear KPKD and KPKR status bit(s) by writing to a 1,
 *    Set the KPKR synchronizer chain by writing "1" to KRSS register,
 *    Clear the KPKD synchronizer chain by writing "1" to KDSC register
 *
 * @result    Number of key pressed/released.
 */
/*static*/ int mxc_kpp_scan_matrix(void)
{
	unsigned short reg_val;
	static int running=0;
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	unsigned long reg_val_gpio;
#endif // CONFIG_MACH_MX27MEDIAPHONE
	int col, row;
	short scancode = 0;
	int keycnt = 0;		/* How many keys are still pressed */

	BUG_ON( running );

	running = 1;
	/*
	 * wmb() linux kernel function which guarantees orderings in write
	 * operations
	 */
	wmb();

	/* save cur keypad matrix to prev */

	memcpy(prev_rcmap, cur_rcmap, (kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(prev_rcmap[0]));
	memset(cur_rcmap, 0, (kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(cur_rcmap[0]));

	for (col = 0; col < kpp_dev.kpp_cols; col++) {	/* Col */
		/* 2. Write 1.s to KPDR[15:8] setting column data to 1.s */
		reg_val = __raw_readw(KPDR);
		reg_val |= 0xff00;
		__raw_writew(reg_val, KPDR);


		/*
		 * 3. Configure columns as totem pole outputs(for quick
		 * discharging of keypad capacitance)
		 */
		reg_val = __raw_readw(KPCR);
		reg_val &= 0x00ff;
		__raw_writew(reg_val, KPCR);

#if defined(CONFIG_MACH_MX31HSV1)
		if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)  
			udelay(1000);
		else 
#endif
		udelay(2);

		/*
		 * 4. Configure columns as open-drain
		 */
		reg_val = __raw_readw(KPCR);
		reg_val |= ((1 << MAXCOL) - 1) << 8;
		__raw_writew(reg_val, KPCR);

		/*
		 * 5. Write a single column to 0, others to 1.
		 * 6. Sample row inputs and save data. Multiple key presses
		 * can be detected on a single column.
		 * 7. Repeat steps 2 - 6 for remaining columns.
		 */

		/* Col bit starts at 8th bit in KPDR */
		reg_val = __raw_readw(KPDR);
		reg_val &= ~(1 << (8 + col));
		__raw_writew(reg_val, KPDR); 

		/* Delay added to avoid propagating the 0 from column to row
		 * when scanning. */

#if defined(CONFIG_MACH_MX31HSV1)
		if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC) 
			udelay(1000);
		else
#endif
		udelay(5);


		/* Read row input */
		reg_val = __raw_readw(KPDR);

		reg_val = debouncer( col, reg_val & ((1<<kpp_dev.kpp_rows)-1));

		//pr_debug ("--- col%d: row value=0x%x (0x%x)\n", col, (reg_val^3)&3, reg_val );
		for (row = 0; row < kpp_dev.kpp_rows; row++) {	/* sample row */
			if (TEST_BIT(reg_val, row) == 0) {
				cur_rcmap[row] = BITSET(cur_rcmap[row], col);
				keycnt++;
			}
		}
	}

#if defined(CONFIG_MACH_MX27MEDIAPHONE) 
	/* SAGEM */


	reg_val_gpio = (__raw_readl(GPIOE_SSR) >> POWER_KEY_SIGNAL ) & 1;
	if (reg_val_gpio) {
		//pr_debug("VISIO START pressed\n");
		// set bit in rcmap for 

		cur_rcmap[2] = BITSET(cur_rcmap[2], 0);
		keycnt++;

		//CLD:!!	  gpio_clear_int(POWER_KEY_PORT,POWER_KEY_SIGNAL);
	}
#elif defined(CONFIG_MACH_MX31HSV1)
	/* MODIF CARTE SAGEM */
	reg_val_gpio = mxc_get_gpio_datain(POWER_KEY_GPIO);
	if (reg_val_gpio) {
		if ( get_board_type() >= BOARD_TYPE_V4_HOMESCREEN_GENERIC) {
			// set bit in rcmap for 
			cur_rcmap[1] = BITSET(cur_rcmap[1], 0); /* row: 2, col: 1 */
		}
		else {
			// set bit in rcmap for 
			cur_rcmap[2] = BITSET(cur_rcmap[2], 0);
		}
		keycnt++;
	}
#endif /* CONFIG_MACH_MX31HS */


#if defined(CONFIG_EVTPROBE) || defined(CONFIG_EVTPROBE2)
	if ((cur_rcmap[0] == MAINTAINER_MOD0) && (cur_rcmap[1] == MAINTAINER_MOD1) && (cur_rcmap[2] == MAINTAINER_MOD2))	{
		pr_debug ("MAINTAINER MODE\n");
		send_netlink_msg(SYSM_H1, 0, GFP_ATOMIC);
	}
#endif // CONFIG_EVTPROBE

	/*
	 * 8. Return all columns to 0 in preparation for standby mode.
	 * 9. Clear KPKD and KPKR status bit(s) by writing to a .1.,
	 * set the KPKR synchronizer chain by writing "1" to KRSS register,
	 * clear the KPKD synchronizer chain by writing "1" to KDSC register
	 */
	reg_val = 0x00;
	__raw_writew(reg_val, KPDR);
	reg_val = __raw_readw(KPDR);
	reg_val = __raw_readw(KPSR);
	reg_val |= KBD_STAT_KPKD | KBD_STAT_KPKR | KBD_STAT_KRSS |
		KBD_STAT_KDSC;
	__raw_writew(reg_val, KPSR);

	/* Check key press status change */

	/*
	 * prev_rcmap array will contain the previous status of the keypad
	 * matrix.  cur_rcmap array will contains the present status of the
	 * keypad matrix. If a bit is set in the array, that (row, col) bit is
	 * pressed, else it is not pressed.
	 *
	 * XORing these two variables will give us the change in bit for
	 * particular row and column.  If a bit is set in XOR output, then that
	 * (row, col) has a change of status from the previous state.  From
	 * the diff variable the key press and key release of row and column
	 * are found out.
	 *
	 * If the key press is determined then scancode for key pressed
	 * can be generated using the following statement:
	 *    scancode = ((row * 8) + col);
	 *
	 * If the key release is determined then scancode for key release
	 * can be generated using the following statement:
	 *    scancode = ((row * 8) + col) + MXC_KEYRELEASE;
	 */
	for (row = 0; row < kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH; row++) {
		unsigned char diff;

		/*
		 * Calculate the change in the keypad row status
		 */
		diff = prev_rcmap[row] ^ cur_rcmap[row];

		for (col = 0; col < kpp_dev.kpp_cols; col++) {
			if ((diff >> col) & 0x1) {
				/* There is a status change on col */
				if ((prev_rcmap[row] & BITSET(0, col)) == 0) {
					/*
					 * Previous state is 0, so now
					 * a key is pressed
					 */
					if (has_learning_key) {
						scancode =
							mxc_scan_matrix_learning_key
							(row, col, 1);
					} else {
						scancode =
							((row * kpp_dev.kpp_cols) +
							 col);
						KPress = 1;
						kpp_dev.iKeyState = KStateUp;
					}
					pr_debug("Press   (%d, %d) scan=%d "
							"Kpress=%d\n",
							row, col, scancode, KPress);
					press_scancode[row][col] =
						(short)scancode;
				} else {
					/*
					 * Previous state is not 0, so
					 * now a key is released
					 */
					if (has_learning_key) {
						scancode =
							mxc_scan_matrix_learning_key
							(row, col, 0);
					} else {
						scancode =
							(row * kpp_dev.kpp_cols) +
							col + MXC_KEYRELEASE;
						KPress = 0;
						kpp_dev.iKeyState = KStateDown;
					}

					pr_debug
						("Release (%d, %d) scan=%d Kpress=%d\n",
						 row, col, scancode, KPress);
					release_scancode[row][col] =
						(short)scancode;
					keycnt++;
				}
			}
		}
	}

	/*
	 * This switch case statement is the
	 * implementation of state machine of debounce
	 * logic for key press/release.
	 * The explaination of state machine is as
	 * follows:
	 *
	 * KStateUp State:
	 * This is in intial state of the state machine
	 * this state it checks for any key presses.
	 * The key press can be checked using the
	 * variable KPress. If KPress is set, then key
	 * press is identified and switches the to
	 * KStateFirstDown state for key press to
	 * debounce.
	 *
	 * KStateFirstDown:
	 * After debounce delay(10ms), if the KPress is
	 * still set then pass scancode generated to
	 * input device and change the state to
	 * KStateDown, else key press debounce is not
	 * satisfied so change the state to KStateUp.
	 *
	 * KStateDown:
	 * In this state it checks for any key release.
	 * If KPress variable is cleared, then key
	 * release is indicated and so, switch the
	 * state to KStateFirstUp else to state
	 * KStateDown.
	 *
	 * KStateFirstUp:
	 * After debounce delay(10ms), if the KPress is
	 * still reset then pass the key release
	 * scancode to input device and change
	 * the state to KStateUp else key release is
	 * not satisfied so change the state to
	 * KStateDown.
	 */
	switch (kpp_dev.iKeyState) {
		case KStateUp:
			if (KPress) {
				/* First Down (must debounce). */
				kpp_dev.iKeyState = KStateFirstDown;
			} else {
				/* Still UP.(NO Changes) */
				kpp_dev.iKeyState = KStateUp;
			}
			break;

		case KStateFirstDown:
			if (KPress) {
				for (row = 0; row < kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH; row++) {
					for (col = 0; col < kpp_dev.kpp_cols; col++) {
						if ((press_scancode[row][col] != -1)) {
							/* Still Down, so add scancode */
							scancode =
								press_scancode[row][col];
							input_event(mxckbd_dev, EV_KEY,
									mxckpd_keycodes
									[scancode], 1);
							//printk("scancode=%d  key=%x press\n", scancode, mxckpd_keycodes[scancode]);
#if defined(CONFIG_EVTPROBE) || defined(CONFIG_EVTPROBE2)
							if (mxckpd_keycodes[scancode] == KEY_POWER) {
								send_netlink_msg(SYSM_P1, 1, GFP_ATOMIC);
							}
#endif

#ifndef CONFIG_MACH_MX27MEDIAPHONE
							if (mxckpd_keycodes[scancode] ==
									KEY_LEFTSHIFT) {
								input_event(mxckbd_dev,
										EV_KEY,
										KEY_3, 1);
							}
#endif // CONFIG_MACH_MX27MEDIAPHONE

							kpp_dev.iKeyState = KStateDown;
							press_scancode[row][col] = -1;
						}
					}
				}
			} else {
				/* Just a bounce */
				kpp_dev.iKeyState = KStateUp;
			}
			break;

		case KStateDown:
			if (KPress) {
				/* Still down (no change) */
				kpp_dev.iKeyState = KStateDown;
			} else {
				/* First Up. Must debounce */
				kpp_dev.iKeyState = KStateFirstUp;
			}
			break;

		case KStateFirstUp:
			if (KPress) {
				/* Just a bounce */
				kpp_dev.iKeyState = KStateDown;
			} else {
				for (row = 0; row < kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH; row++) {
					for (col = 0; col < kpp_dev.kpp_cols; col++) {
						if ((release_scancode[row][col] != -1)) {
							scancode =
								release_scancode[row][col];
							scancode =
								scancode - MXC_KEYRELEASE;
							input_event(mxckbd_dev, EV_KEY,
									mxckpd_keycodes
									[scancode], 0);
							//printk("scancode=%d  key=%x release\n", scancode, mxckpd_keycodes[scancode]);
#if defined(CONFIG_EVTPROBE) || defined(CONFIG_EVTPROBE2)
							if (mxckpd_keycodes[scancode] == KEY_POWER) {
								send_netlink_msg(SYSM_P1, 0, GFP_ATOMIC);
							}
#endif

#ifndef CONFIG_MACH_MX27MEDIAPHONE
							if (mxckpd_keycodes[scancode] ==
									KEY_LEFTSHIFT) {
								input_event(mxckbd_dev,
										EV_KEY,
										KEY_3, 0);
							}
#endif // CONFIG_MACH_MX27MEDIAPHONE

							kpp_dev.iKeyState = KStateUp;
							release_scancode[row][col] = -1;
						}
					}
				}
			}
			break;

		default:
			running=0;
			return -EBADRQC;
			break;
	}

	running=0;
	return keycnt;
}

/*!
 * This function is called to start the timer for scanning the keypad if there
 * is any key press. Currently this interval is  set to 10 ms. When there are
 * no keys pressed on the keypad we return back, waiting for a keypad key
 * press interrupt.
 *
 * @param data  Opaque data passed back by kernel. Not used.
 */
#ifdef KPP_WITH_WORKQUEUE
static void mxc_kpp_handle_workqueue(void *data)
#else
static void mxc_kpp_handle_timer(unsigned long data)
#endif
{
	unsigned short reg_val;
#if defined(CONFIG_MACH_MX27MEDIAPHONE)
	unsigned long  reg_val_gpio;
#endif // CONFIG_MACH_MX27MEDIAPHONE
	int i;

	if (key_pad_enabled == 0) {
		return;
	}
	if (mxc_kpp_scan_matrix() == 0) {
		/*
		 * Stop scanning and wait for interrupt.
		 * Enable press interrupt and disable release interrupt.
		 */
		__raw_writew(0x00FF, KPDR);
		reg_val = __raw_readw(KPSR);
		reg_val |= (KBD_STAT_KPKR | KBD_STAT_KPKD);
		reg_val |= KBD_STAT_KRSS | KBD_STAT_KDSC;
		__raw_writew(reg_val, KPSR);
		reg_val |= KBD_STAT_KDIE;
		reg_val &= ~KBD_STAT_KRIE;
		__raw_writew(reg_val, KPSR);

		/*
		 * No more keys pressed... make sure unwanted key codes are
		 * not given upstairs
		 */
		for (i = 0; i < kpp_dev.kpp_rows; i++) {
			memset(press_scancode[i], -1,
					sizeof(press_scancode[0][0]) * kpp_dev.kpp_cols);
			memset(release_scancode[i], -1,
					sizeof(release_scancode[0][0]) *
					kpp_dev.kpp_cols);
		}
		return;
	}

	/*
	 * There are still some keys pressed, continue to scan.
	 * We shall scan again in 10 ms. This has to be tuned according
	 * to the requirement.
	 */
#ifdef KPP_WITH_WORKQUEUE
	if(!queue_delayed_work(kpp_dev.queue, &(kpp_dev.work_timer), KScanRate))
		printk("%s %d : cannot add work_timer in workqueue\n", __func__, __LINE__);
#else
	kpp_dev.poll_timer.expires = jiffies + KScanRate;
	kpp_dev.poll_timer.function = mxc_kpp_handle_timer;
	add_timer(&kpp_dev.poll_timer);
#endif
}

#ifdef KPP_WITH_WORKQUEUE
static void mxc_kpp_interrupt_delayed(void *data){
	/*
	 * Check if any keys are pressed, if so start polling.
	 */
	mxc_kpp_handle_workqueue(0);
	return;
}
#endif



/*!
 * This function is the keypad Interrupt handler.
 * This function checks for keypad status register (KPSR) for key press
 * and interrupt. If key press interrupt has occurred, then the key
 * press interrupt in the KPSR are disabled.
 * It then calls mxc_kpp_scan_matrix to check for any key pressed/released.
 * If any key is found to be pressed, then a timer is set to call
 * mxc_kpp_scan_matrix function for every 10 ms.
 *
 * @param   irq      The Interrupt number
 * @param   dev_id   Driver private data
 *
 * @result    The function returns \b IRQ_RETVAL(1) if interrupt was handled,
 *            returns \b IRQ_RETVAL(0) if the interrupt was not handled.
 *            \b IRQ_RETVAL is defined in include/linux/interrupt.h.
 */
static irqreturn_t mxc_kpp_interrupt(int irq, void *dev_id)
{
	unsigned short reg_val;

#ifdef KPP_WITH_WORKQUEUE
	cancel_delayed_work(&(kpp_dev.work_timer));
#else
	/* Delete the polling timer */
	del_timer(&kpp_dev.poll_timer);
#endif
	reg_val = __raw_readw(KPSR);

	/* Check if it is key press interrupt */
	if (reg_val & KBD_STAT_KPKD) {
		/*
		 * Disable key press(KDIE status bit) interrupt
		 */
		reg_val &= ~KBD_STAT_KDIE;
		__raw_writew(reg_val, KPSR);

#if 0 && defined(CONFIG_PM) //CLD: 0
	} else if (reg_val & KBD_STAT_KPKR) {
		/*
		 * Disable key release(KRIE status bit) interrupt - only caused
		 * by _suspend setting the bit IF a key is down while the system
		 * is being suspended.
		 */
		reg_val &= ~KBD_STAT_KRIE;
		__raw_writew(reg_val, KPSR);
#endif
	} else {
		/* spurious interrupt */
		return IRQ_RETVAL(0);
	}
#ifdef KPP_WITH_WORKQUEUE
	if(!queue_work(kpp_dev.queue, &(kpp_dev.work_kpp)))
		printk("%s %d : cannot add work_kpp in workqueue\n", __func__, __LINE__);
#else
	/*
	 * Check if any keys are pressed, if so start polling.
	 */
	mxc_kpp_handle_timer(0);
#endif
	return IRQ_RETVAL(1);
}

#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)

#ifdef KPP_WITH_WORKQUEUE
static void mxc_gpio_key_interrupt_delayed(void *data){
#if defined(CONFIG_MACH_MX27MEDIAPHONE)
	unsigned long  reg_val_gpio;
#endif
	/*
	 * Check if any keys are pressed, if so start polling.
	 */
	mxc_kpp_handle_workqueue(0);

#if defined(CONFIG_MACH_MX27MEDIAPHONE)
	reg_val_gpio = __raw_readl (GPIOE_MASK);
	reg_val_gpio |= 1 << (POWER_KEY_SIGNAL);
	__raw_writel (reg_val_gpio, GPIOE_MASK);
#elif defined(CONFIG_MACH_MX31HSV1)
	enable_irq(POWER_KEY_IRQ);
#endif // CONFIG_MACH_MX27MEDIAPHONE
	return;
}
#endif

static irqreturn_t mxc_gpio_key_interrupt(int irq, void *dev_id)
{
	unsigned long  reg_val_gpio;

	if ( irq == POWER_KEY_IRQ ) {
#ifdef KPP_WITH_WORKQUEUE
		/* Delete timer work */
		cancel_delayed_work(&(kpp_dev.work_timer));
#else
		/* Delete the polling timer */
		del_timer(&kpp_dev.poll_timer);
#endif

#if defined(CONFIG_MACH_MX27MEDIAPHONE) 
		reg_val_gpio = (__raw_readl (GPIOE_SSR) >> POWER_KEY_SIGNAL) & 1 ;
#elif defined(CONFIG_MACH_MX31HSV1)
		reg_val_gpio = mxc_get_gpio_datain(POWER_KEY_GPIO); 
#endif

#if defined(CONFIG_MACH_MX27MEDIAPHONE)
		/*
		 * Disable GPIO key press interrupt
		 */  
		reg_val_gpio = __raw_readl(GPIOE_MASK);
		reg_val_gpio &= ~( 1 << (POWER_KEY_SIGNAL));
		__raw_writel (reg_val_gpio, GPIOE_MASK);
#elif defined(CONFIG_MACH_MX31HSV1)
		/*
		 * Disable GPIO key press interrupt
		 */  
		disable_irq(POWER_KEY_IRQ);
#endif // CONFIG_MACH_MX27MEDIAPHONE
#ifdef KPP_WITH_WORKQUEUE
		if(!queue_work(kpp_dev.queue, &(kpp_dev.work_gpio)))
			printk("%s %d : cannot add work_gpio in workqueue\n", __func__, __LINE__);
	
		return IRQ_RETVAL(1);
#endif

	} else {
		/* spurious interrupt */
		return IRQ_RETVAL(0);
	}
#ifndef KPP_WITH_WORKQUEUE
	/*
	 * Check if any keys are pressed, if so start polling.
	 */
	mxc_kpp_handle_timer(0);

#if defined(CONFIG_MACH_MX27MEDIAPHONE)
	reg_val_gpio = __raw_readl (GPIOE_MASK);
	reg_val_gpio |= 1 << (POWER_KEY_SIGNAL);
	__raw_writel (reg_val_gpio, GPIOE_MASK);
#elif defined(CONFIG_MACH_MX31HSV1)
	enable_irq(POWER_KEY_IRQ);
#endif // CONFIG_MACH_MX27MEDIAPHONE
#endif

	return IRQ_RETVAL(1);
}
#endif /* CONFIG_MACH_MX27MEDIAPHONE || CONFIG_MACH_MX31HSV1 */


/*!
 * This function is called when the keypad driver is opened.
 * Since keypad initialization is done in __init, nothing is done in open.
 *
 * @param    dev    Pointer to device inode
 *
 * @result    The function always return 0
 */
static int mxc_kpp_open(struct input_dev *dev)
{
	return 0;
}

/*!
 * This function is called close the keypad device.
 * Nothing is done in this function, since every thing is taken care in
 * __exit function.
 *
 * @param    dev    Pointer to device inode
 *
 */
static void mxc_kpp_close(struct input_dev *dev)
{
}

#if 0 && defined(CONFIG_PM)
/*!
 * This function puts the Keypad controller in low-power mode/state.
 * If Keypad is enabled as a wake source(i.e. it can resume the system
 * from suspend mode), the Keypad controller doesn't enter low-power state.
 *
 * @param   pdev  the device structure used to give information on Keypad
 *                to suspend
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxc_kpp_suspend(struct platform_device *pdev, pm_message_t state)
{
	unsigned short reg_val;

	del_timer(&kpp_dev.poll_timer);

	if (device_may_wakeup(&pdev->dev)) {
		reg_val = __raw_readw(KPSR);
		if ((reg_val & KBD_STAT_KDIE) == 0) {
			/* if no depress interrupt enable the release interrupt */
			reg_val |= KBD_STAT_KRIE;
			__raw_writew(reg_val, KPSR);
		}
		enable_irq_wake(keypad->irq);
	} else {
		disable_irq(keypad->irq);
		key_pad_enabled = 0;
		clk_disable(kpp_clk);
	}

	return 0;
}

/*!
 * This function brings the Keypad controller back from low-power state.
 * If Keypad is enabled as a wake source(i.e. it can resume the system
 * from suspend mode), the Keypad controller doesn't enter low-power state.
 *
 * @param   pdev  the device structure used to give information on Keypad
 *                to resume
 *
 * @return  The function always returns 0.
 */
static int mxc_kpp_resume(struct platform_device *pdev)
{
	if (device_may_wakeup(&pdev->dev)) {
		/* the irq routine already cleared KRIE if it was set */
		disable_irq_wake(keypad->irq);
	} else {
		clk_enable(kpp_clk);
		key_pad_enabled = 1;
		enable_irq(keypad->irq);
	}

	init_timer(&kpp_dev.poll_timer);

	return 0;
}

#else
#define mxc_kpp_suspend  NULL
#define mxc_kpp_resume   NULL
#endif				/* CONFIG_PM */

/*!
 * This function is called to free the allocated memory for local arrays
 */
static void mxc_kpp_free_allocated(void)
{

	int i;
#ifdef KPP_WITH_WORKQUEUE
	if(kpp_dev.queue){
		cancel_delayed_work(&(kpp_dev.work_timer));

		/* FIXME do we have or can we flush here ? */
		flush_workqueue(kpp_dev.queue);
		destroy_workqueue(kpp_dev.queue);
	}
#endif
	if (press_scancode) {
		for (i = 0; i < kpp_dev.kpp_rows; i++) {
			if (press_scancode[i])
				kfree(press_scancode[i]);
		}
		kfree(press_scancode);
	}

	if (release_scancode) {
		for (i = 0; i < kpp_dev.kpp_rows; i++) {
			if (release_scancode[i])
				kfree(release_scancode[i]);
		}
		kfree(release_scancode);
	}

	if (cur_rcmap)
		kfree(cur_rcmap);

	if (prev_rcmap)
		kfree(prev_rcmap);

	if (mxckbd_dev)
		input_free_device(mxckbd_dev);
}

/*!
 * This function is called during the driver binding process.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions.
 *
 * @return  The function returns 0 on successful registration. Otherwise returns
 *          specific error code.
 */
static int mxc_kpp_probe(struct platform_device *pdev)
{
	int i, irq;
	int retval;
	unsigned int reg_val;

	keypad = (struct keypad_data *)pdev->dev.platform_data;

	kpp_dev.kpp_cols = keypad->colmax;
	kpp_dev.kpp_rows = keypad->rowmax;
	key_pad_enabled = 0;

	debouncer_init();

	/*
	 * Request for IRQ number for keypad port. The Interrupt handler
	 * function (mxc_kpp_interrupt) is called when ever interrupt occurs on
	 * keypad port.
	 */
	irq = platform_get_irq(pdev, 0);
	keypad->irq = irq;
#if 0 // defined(CONFIG_MACH_MX31HSV1)
	retval = request_irq(irq, mxc_kpp_interrupt, IRQF_TRIGGER_RISING, MOD_NAME, MOD_NAME);
#else
	retval = request_irq(irq, mxc_kpp_interrupt, 0, MOD_NAME, MOD_NAME);
#endif
	if (retval) {
		pr_debug("KPP: request_irq(%d) returned error %d\n", irq,
				retval);
		return -1;
	}
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	// SAGEM: Creation of interrupts when the VISO_START_KEY is pressed or released
	{
		int retval_visio_start;

		th2 = kthread_run(mxc_kpp_thread, NULL, MOD_NAME);
		retval_visio_start = gpio_buttons_active();
		if ( retval_visio_start < 0 )
		{
			printk(KERN_WARNING "KPP_power: unable to hook GPIO_IRQ, retcode =%d\n",retval_visio_start);
			return -ENODEV;
		}
		retval_visio_start = request_irq (POWER_KEY_IRQ, mxc_gpio_key_interrupt, IRQF_TRIGGER_RISING, "power_key", MOD_NAME);

		if (retval_visio_start) {
			printk ("KPP_power: request_irq(%d) returned error %d\n", POWER_KEY_IRQ, retval);
			return -1;
		} 

		else {
			pr_debug ("KPP_power: IRQ set properly for GPIO - SAGEM\n");
		}
	}
#endif // CONFIG_MACH_MX27MEDIAPHONE
	/* Enable keypad clock */
	kpp_clk = clk_get(&pdev->dev, "kpp_clk");
	clk_enable(kpp_clk);

	/* IOMUX configuration for keypad */
	gpio_keypad_active();

	/* Configure keypad */

	/* Enable number of rows in keypad (KPCR[7:0])
	 * Configure keypad columns as open-drain (KPCR[15:8])
	 *
	 * Configure the rows/cols in KPP
	 * LSB nibble in KPP is for 8 rows
	 * MSB nibble in KPP is for 8 cols
	 */
	reg_val = __raw_readw(KPCR);
	reg_val |= (1 << keypad->rowmax) - 1;	/* LSB */
	reg_val |= ((1 << keypad->colmax) - 1) << 8;	/* MSB */
	__raw_writew(reg_val, KPCR);

	/* Write 0's to KPDR[15:8] */
	reg_val = __raw_readw(KPDR);
	reg_val &= 0x00ff;
	__raw_writew(reg_val, KPDR);

	/* Configure columns as output, rows as input (KDDR[15:0]) */
	reg_val = __raw_readw(KDDR);
	reg_val |= 0xff00;
	reg_val &= 0xff00;
	__raw_writew(reg_val, KDDR);

	reg_val = __raw_readw(KPSR);
	reg_val &= ~(KBD_STAT_KPKR | KBD_STAT_KPKD);
	reg_val |= KBD_STAT_KPKD;
	reg_val |= KBD_STAT_KRSS | KBD_STAT_KDSC;
	__raw_writew(reg_val, KPSR);
	reg_val |= KBD_STAT_KDIE;
	reg_val &= ~KBD_STAT_KRIE;
	__raw_writew(reg_val, KPSR);

	has_learning_key = keypad->learning;
	mxckpd_keycodes = keypad->matrix;
	mxckpd_keycodes_size = keypad->rowmax * keypad->colmax;

	if ((keypad->matrix == (void *)0)
			|| (mxckpd_keycodes_size == 0)) {
		free_irq(irq, MOD_NAME);
		return -ENODEV;
	}

	mxckbd_dev = input_allocate_device();
	if (!mxckbd_dev) {
		pr_debug(KERN_ERR
				"mxckbd_dev: not enough memory for input device\n");
		free_irq(irq, MOD_NAME);
		return -ENOMEM;
	}

#if defined(CONFIG_MACH_MX27MEDIAPHONE)
	// Unmask IT from GPIO VISIO START
	reg_val = __raw_readl(GPIOE_MASK);
	reg_val |= 1 << (POWER_KEY_SIGNAL);
	__raw_writew(reg_val,GPIOE_MASK);
#elif defined(CONFIG_MACH_MX31HSV1)
	//CLD: done by request_irq() :    enable_irq(POWER_KEY_IRQ);
#endif // CONFIG_MACH_MX27MEDIAPHONE

	mxckbd_dev->keycode = &mxckpd_keycodes;
	mxckbd_dev->keycodesize = sizeof(unsigned char);
	mxckbd_dev->keycodemax = mxckpd_keycodes_size;
	mxckbd_dev->name = "mxckpd";
	mxckbd_dev->id.bustype = BUS_HOST;
	mxckbd_dev->open = mxc_kpp_open;
	mxckbd_dev->close = mxc_kpp_close;

	/* allocate required memory */
	press_scancode = kmalloc((kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(press_scancode[0]),
			GFP_KERNEL);
	release_scancode =
		kmalloc((kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(release_scancode[0]), GFP_KERNEL);

	if (!press_scancode || !release_scancode) {
		free_irq(irq, MOD_NAME);
		mxc_kpp_free_allocated();
		return -1;
	}

	for (i = 0; i < kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH; i++) {
		press_scancode[i] = kmalloc(kpp_dev.kpp_cols
				* sizeof(press_scancode[0][0]),
				GFP_KERNEL);
		release_scancode[i] =
			kmalloc(kpp_dev.kpp_cols * sizeof(release_scancode[0][0]),
					GFP_KERNEL);

		if (!press_scancode[i] || !release_scancode[i]) {
			free_irq(irq, MOD_NAME);
			mxc_kpp_free_allocated();
			return -1;
		}
	}

	cur_rcmap =
		kmalloc((kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(cur_rcmap[0]), GFP_KERNEL);
	prev_rcmap =
		kmalloc((kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(prev_rcmap[0]), GFP_KERNEL);

	if (!cur_rcmap || !prev_rcmap) {
		free_irq(irq, MOD_NAME);
		mxc_kpp_free_allocated();
		return -1;
	}

	__set_bit(EV_KEY, mxckbd_dev->evbit);

	for (i = 0; i < mxckpd_keycodes_size; i++)
		__set_bit(mxckpd_keycodes[i], mxckbd_dev->keybit);

	for (i = 0; i < (kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH); i++) {
		memset(press_scancode[i], -1,
				sizeof(press_scancode[0][0]) * kpp_dev.kpp_cols);
		memset(release_scancode[i], -1,
				sizeof(release_scancode[0][0]) * kpp_dev.kpp_cols);
	}
	memset(cur_rcmap, 0, (kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(cur_rcmap[0]));
	memset(prev_rcmap, 0, (kpp_dev.kpp_rows+K_ROW_NUMBER_PATCH) * sizeof(prev_rcmap[0]));

	key_pad_enabled = 1;
#ifdef KPP_WITH_WORKQUEUE
	kpp_dev.queue = create_workqueue("kpp_workqueue");
	if(!kpp_dev.queue){
		printk(KERN_ERR "Cannot allocate memory for kpp_dev.queue\n");
		free_irq(irq, MOD_NAME);
		mxc_kpp_free_allocated();
		return -ENOMEM;
	}
	INIT_WORK(&(kpp_dev.work_timer), mxc_kpp_handle_workqueue, NULL);
	INIT_WORK(&(kpp_dev.work_kpp), mxc_kpp_interrupt_delayed, NULL);
	INIT_WORK(&(kpp_dev.work_gpio), mxc_gpio_key_interrupt_delayed, NULL); 
#else
	/* Initialize the polling timer */
	init_timer(&kpp_dev.poll_timer);
#endif

	if (input_register_device(mxckbd_dev)) {
		printk(KERN_ERR "mxckbd_dev: failed to register input device\n");
		input_free_device(mxckbd_dev);
		free_irq(irq, MOD_NAME);
		mxc_kpp_free_allocated();
		return -ENODEV;
	}

	/* By default, devices should wakeup if they can */
	/* So keypad is set as "should wakeup" as it can */
	device_init_wakeup(&pdev->dev, 1);

	return 0;
}

/*!
 * Dissociates the driver from the kpp device.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to remove
 *
 * @return  The function always returns 0.
 */
static int mxc_kpp_remove(struct platform_device *pdev)
{
	unsigned short reg_val;

	/*
	 * Clear the KPKD status flag (write 1 to it) and synchronizer chain.
	 * Set KDIE control bit, clear KRIE control bit (avoid false release
	 * events. Disable the keypad GPIO pins.
	 */
	__raw_writew(0x00, KPCR);
	__raw_writew(0x00, KPDR);
	__raw_writew(0x00, KDDR);

	reg_val = __raw_readw(KPSR);
	reg_val |= KBD_STAT_KPKD;
	reg_val &= ~KBD_STAT_KRSS;
	reg_val |= KBD_STAT_KDIE;
	reg_val &= ~KBD_STAT_KRIE;
	__raw_writew(reg_val, KPSR);

	gpio_keypad_inactive();
	clk_disable(kpp_clk);
	clk_put(kpp_clk);

	KPress = 0;

#ifndef KPP_WITH_WORKQUEUE
	del_timer(&kpp_dev.poll_timer);
#endif

	free_irq(keypad->irq, MOD_NAME);
	input_unregister_device(mxckbd_dev);

	mxc_kpp_free_allocated();
	return 0;
}

/*!
 * .
 *
 * @param  thread data. 
 *
 * @return  The function always returns 0.
 */
static int mxc_kpp_thread (void * data)
{
	int cpt = 0;
	ulong flags;
	// thread loop

	// IRQ edge reinitialization
	// @{
	spin_lock_irqsave(&detection_lock, flags);

	btn_power_pressed = mxc_get_gpio_datain(POWER_KEY_GPIO);
	btn_power_irq_type = (btn_power_pressed)?IRQT_FALLING:IRQT_RISING ;
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	set_irq_type (POWER_KEY_IRQ, btn_power_irq_type );
#endif   

	spin_unlock_irqrestore(&detection_lock, flags);
	// @}

	do {
		spin_lock_irqsave(&detection_lock, flags);

		if( mxc_get_gpio_datain(POWER_KEY_GPIO)) {

			if ( btn_power_irq_type == IRQT_RISING ) {
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
				set_irq_type (POWER_KEY_IRQ, IRQT_FALLING);
#endif 
				btn_power_irq_type = IRQT_FALLING;
			}


			if ( ! btn_power_pressed ) {
				cpt=0;
			}
			btn_power_pressed=1;
			cpt++;
		} else {
			if ( btn_power_irq_type == IRQT_FALLING ) {
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
				set_irq_type (POWER_KEY_IRQ, IRQT_RISING);
#endif 
				btn_power_irq_type = IRQT_RISING;
			}

			if ( btn_power_pressed ) {
				cpt=0;
			}
			btn_power_pressed=0;
		}


		spin_unlock_irqrestore(&detection_lock, flags);

		//
		// DO WHATEVER YOU WANT
		//

		// wait X ms before checking again
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout_interruptible(HZ/2);
	} while (!kthread_should_stop());

	printk(KERN_INFO "evtprobe2: end of thread NL\n");

	return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxc_kpd_driver = {
	.driver = {
		.name = "mxc_keypad",
		.bus = &platform_bus_type,
	},
	.suspend = mxc_kpp_suspend,
	.resume = mxc_kpp_resume,
	.probe = mxc_kpp_probe,
	.remove = mxc_kpp_remove
};

/*!
 * This function is called for module initialization.
 * It registers keypad char driver and requests for KPP irq number. This
 * function does the initialization of the keypad device.
 *
 * The following steps are used for keypad configuration,\n
 * -# Enable number of rows in the keypad control register (KPCR[7:0}).\n
 * -# Write 0's to KPDR[15:8]\n
 * -# Configure keypad columns as open-drain (KPCR[15:8])\n
 * -# Configure columns as output, rows as input (KDDR[15:0])\n
 * -# Clear the KPKD status flag (write 1 to it) and synchronizer chain\n
 * -# Set KDIE control bit, clear KRIE control bit\n
 * In this function the keypad queue initialization is done.
 * The keypad IOMUX configuration are done here.*

 *
 * @return      0 on success and a non-zero value on failure.
 */
static int __init mxc_kpp_init(void)
{
	pr_debug(KERN_INFO "MXC keypad loaded\n");
	platform_driver_register(&mxc_kpd_driver);
	return 0;
}

/*!
 * This function is called whenever the module is removed from the kernel. It
 * unregisters the keypad driver from kernel and frees the irq number.
 * This function puts the keypad to standby mode. The keypad interrupts are
 * disabled. It calls gpio_keypad_inactive function to switch gpio
 * configuration into default state.
 *
 */
static void __exit mxc_kpp_cleanup(void)
{
#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
	free_irq(POWER_KEY_IRQ, mxc_kpp_interrupt);
	gpio_buttons_inactive();

	kthread_stop(th2);
	th2 = NULL;
#endif 

	platform_driver_unregister(&mxc_kpd_driver);
}

module_init(mxc_kpp_init);
module_exit(mxc_kpp_cleanup);

#if defined(CONFIG_MACH_MX27MEDIAPHONE) || defined(CONFIG_MACH_MX31HSV1)
MODULE_AUTHOR("Sagem Communications / Freescale Semiconductor, Inc..");
#else
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
#endif // CONFIG_MACH_MX27MEDIAPHONE

MODULE_DESCRIPTION("MXC Keypad Controller Driver");
MODULE_LICENSE("GPL");
