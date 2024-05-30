/*
 * ad7877_ts.h - header file for OMAP touchscreen support
 *
 * Copyright (c) 2002 MontaVista Software Inc.
 * Copyright (c) 2004 Texas Instruments, Inc.
 *
 * Assembled using driver code copyright the companies above.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include <linux/cdev.h>

/*
 * Touchscreen Module Name :
 */

#define MOD_NAME "ad7877"

/*
 * AD7877 Registers :
 */

//#include <linux/types.h>

#define AD7877_REG_ZEROS                        0
#define AD7877_REG_CTRL1                        1
#define AD7877_REG_CTRL2                        2
#define AD7877_REG_ALERT                        3
#define AD7877_REG_AUX1HIGH                     4
#define AD7877_REG_AUX1LOW                      5
#define AD7877_REG_BAT1HIGH                     6
#define AD7877_REG_BAT1LOW                      7
#define AD7877_REG_BAT2HIGH                     8
#define AD7877_REG_BAT2LOW                      9
#define AD7877_REG_TEMP1HIGH                    10
#define AD7877_REG_TEMP1LOW                     11
#define AD7877_REG_SEQ0                         12
#define AD7877_REG_SEQ1                         13
#define AD7877_REG_DAC                          14
#define AD7877_REG_NONE1                        15
#define AD7877_REG_EXTWRITE                     15
#define AD7877_REG_XPLUS                        16
#define AD7877_REG_YPLUS                        17
#define AD7877_REG_Z2                           18
#define AD7877_REG_AUX1                         19
#define AD7877_REG_AUX2                         20
#define AD7877_REG_AUX3                         21
#define AD7877_REG_BAT1                         22
#define AD7877_REG_BAT2                         23
#define AD7877_REG_TEMP1                        24
#define AD7877_REG_TEMP2                        25
#define AD7877_REG_Z1                           26
#define AD7877_REG_GPIOCTRL1                    27
#define AD7877_REG_GPIOCTRL2                    28
#define AD7877_REG_GPIODATA                     29
#define AD7877_REG_NONE2                        30
#define AD7877_REG_NONE3                        31

/*
 * Screen features
 */

/*
 * Define read mask register
 */
#define MASK_12BITS     0x0FFF
#define MASK_8BITS      0x00FF
#define MASK_AUX1LOW    0x0001
#define MASK_BAT1LOW    0x0002
#define MASK_BAT2LOW    0x0004
#define MASK_TEMP1HIGH  0x0008
#define MASK_AUX1HIGH   0x0010
#define MASK_BAT1HIGH   0x0020
#define MASK_BAT2HIGH   0x0040
#define MASK_TEMP1LOW   0x0080
#define MASK_GPIO1_DAT  0x0080
#define MASK_GPIO2_DAT  0x0040
#define MASK_GPIO3_DAT  0x0020
#define MASK_GPIO4_DAT  0x0010
#define MASK_GPIO_DAT   0x00F0

/* Specify the value above which data points will be ignored.*/
#define X_PLATE         550   /* 200 ohms < X < 900 ohms */
#define Y_PLATE         550   /* 200 ohms < Y < 900 ohms */

#define X_MIN           -500  /* Dalle 1.5 mm before screen */
#define X_MAX           4500  /* Dalle 1.5 mm after screen  */

#define Y_MIN          -1100  /* Dalle 1.5 mm before screen */
#define Y_MAX           5250  /* Dalle 1.5 mm after screen  */

#define PRESS_MIN       0     /* to be determinated ?? */
#define PRESS_MAX       10000 /* to be determinated ?? */

/*
 * Define time between samples while pen is down
 * This value will drive the overall sample rate
 */
#define SAMPLE_PERIOD_MS     jiffies + HZ / 100;    // Tempo 10 ms (tempo min) (9 ms)

/*
 * Define debounce time after pen has gone down
 */
#define DEBOUNCE_PERIOD_MS    jiffies + HZ / 100;    // Tempo 10 ms (tempo min) (5 ms)

/*
 * Battery Device
 */
#ifndef AD7877_BAT_MAJOR
#define AD7877_BAT_MAJOR 17
#endif

/* nb of minor to reserve */
#ifndef AD7877_BAT_NR_DEVS
#define AD7877_BAT_NR_DEVS 1
#endif

/*
 * Ioctl definitions
*/

/* Use 'K' as magic number */
#define AD7877_IOC_MAGIC  'K'

/* Please use a different 8-bit number in your code */

#define AD7877_IOCTL_GET_ALERT_INFO             _IOW(AD7877_IOC_MAGIC,  0, int)
#define AD7877_IOCTL_GET_PRESENCE_CHARGER       _IOW(AD7877_IOC_MAGIC,  1, int)
#define AD7877_IOCTL_GET_BATTERY_LEVEL          _IOW(AD7877_IOC_MAGIC,  2, int)
#define AD7877_IOCTL_GET_BATTERY_UNLOADED       _IOW(AD7877_IOC_MAGIC,  3, int)
#define AD7877_POLL                             _IOW(AD7877_IOC_MAGIC,  4, int)
#define AD7877_IOC_MAXNR                        5

#define BUF_LEN         500

/*
 * Inverter Ctrl
 */

#define SCREEN_ON  0
#define SCREEN_OFF 1

/*
 * Structures
 */

struct ts_event {
        /* we read 12 bit values using SPI, and expect the controller
         * driver to deliver them in native byteorder with msbs zeroed.
         */
        u16 x;
        u16 y;
        u16 z1, z2;
};

struct ad7877{
        struct input_dev *input;
        char phys[32];

        /* Timer for triggering release */
        struct timer_list timer_appui;
        struct timer_list timer_lecture;
        struct timer_list timer_saisie;

        /* Structure de gestion des events */
        struct ts_event tc;

        spinlock_t lock;

        /* Temperatures limites */
        u16 temp_high_limit;
        u16 temp_low_limit;

        /* Compteur */
        int cpteur;

        /* Interruptions */
        int pen_irq;
        int pen_irq_enabled:1;
        int dav_irq;
        int dav_irq_enabled:1;
        int alert_irq;
        int alert_irq_enabled:1;
        int pendown:1;
};

struct mxcfb_data {
        struct fb_info *fbi;
        struct fb_info *fbi_ovl;
        volatile int32_t vsync_flag;
        wait_queue_head_t vsync_wq;
        wait_queue_head_t suspend_wq;
        bool suspended;
        int backlight_level;
        int inverter_state;
};

struct ad7877_bat {
        /* Char device structure */
        struct cdev cdev;
        wait_queue_head_t write_queue;
        wait_queue_head_t poll_wait;
        char buffer[BUF_LEN];
        struct timer_list timer_monitor;
        int nb_bytes;
        int ready;
        int opened; /* count how many times driver has been opened */
        /* Compteur */
        int cpteur;
        bool suspended;
        spinlock_t lock;
        int battery_connected;
        int on_the_base;
        int battery_unloaded;
        int unloaded_level;
        int charge_level;
	int VbatLow;
};

/*
 * functions
 *
 */
int ad7877_bright_ctrl(u8 data);


 /* __ad7877 _ts_h */
