/*
 * input/touchscreen/ad7877_ts.c
 *
 * touchscreen input device driver for AD7877 on Premium board
 * Copyright (c) 2002 MontaVista Software Inc.
 * Copyright (c) 2004 Texas Instruments, Inc.
 * Copyright (c) 2006 Sagem Comminucations
 * Cleanup and modularization 2004 by Dirk Behme <dirk.behme@de.bosch.com>
 *
 * Assembled using driver code copyright the companies above.
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
 * History:
 * 12/12/2004    Srinath Modified and intergrated code for H2 and H3
 *
 *
 * Copyright (c) 2006 Sagem BDt : adaptation for the AD7877
 *
 */

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/suspend.h>
#include <linux/types.h>
#include <linux/poll.h>

#include <asm/uaccess.h>
#include <asm/mach-types.h>

#include <asm/arch/gpio.h>
#include "../../../arch/arm/mach-mx3/iomux.h"

#include "ad7877_ts.h"

#include "ad7877_spi_inter.h"

#define TS_POLL_PERIOD100  msecs_to_jiffies(100)
#define TS_POLL_PERIOD10   msecs_to_jiffies(10)

#define GPIO_INT2 2
#define GPIO_INT3 3
#define GPIO_INT4 4

/********** PROVI **************/

#ifndef CONFIG_MACH_MX31TABLET
#define CONFIG_MACH_MX31TABLET
#endif

#ifndef CONFIG_DEBUG
#define CONFIG_DEBUG
#endif

/********************************/

#ifdef CONFIG_DEBUG1
#       define CS_WARNING(msg, args...)                         \
                printk("CS_WARNING: " msg "\n",## args);        \

#else
#       define CS_WARNING(msg, args...)
#endif


static int ad7877_read(void);
static irqreturn_t ad7877_pen_irq(int irq, void *handle, struct pt_regs *regs);
static irqreturn_t ad7877_dav_irq(int irq, void *handle, struct pt_regs *regs);
static irqreturn_t ad7877_alert_irq(int irq, void *handle, struct pt_regs *regs);

static struct ad7877 *ad7877;
int hardware_version_info(void);

#ifdef CONFIG_MACH_MX31TABLET
static int ad7877_alert(unsigned long handle);
static int ad7877_monitoring(void);
static struct ad7877_bat *ad7877_bat;
static void ad7877_tempo_reg(void);
int cpteur;
int ad7877_bat_major = AD7877_BAT_MAJOR ;
int ad7877_bat_minor = 0 ;
#endif

extern int ipu_sdc_set_brightness(uint8_t level);
extern int gpio_lcd_state(uint8_t state);
extern struct mxcfb_data mxcfb_drv_data;


enum {
        HARDVERSION_NONE,
        HARDVERSION_1_0,
        HARDVERSION_2_0,
        HARDVERSION_2_1,
        HARDVERSION_TABLET = HARDVERSION_2_1,
} versionHard = HARDVERSION_NONE;

enum {
        TOUCH_STATE_PENUP,
        TOUCH_STATE_PENDEBOUNCE,
        TOUCH_STATE_PENCONVERTING,
        TOUCH_STATE_PENDOWN
} TouchPanelState = TOUCH_STATE_PENUP;

/*
 * Driver name
 */
#define AD7877_NAME      "ad7877"

/*
 * ad7877_bright_ctrl()
 *
 * Manage the DAC for LCD contrast control
 */
int ad7877_bright_ctrl(u8 data)
{
        int reg_value = 0;

        reg_value |= (data << 4) & 0xff0;
        reg_value |= 0x01;  /* DAC on + Voltage + Vref */

        /* Power on DAC for LCD contrast control */
        spi_write_7877reg(AD7877_REG_DAC, reg_value);
	
        return 0;
}

/*
 * board_bright_ctrl()
 *
 * Manage the brightness for LCD contrast control
 */
int board_bright_ctrl(u8 level)
{
        
        mxcfb_drv_data.backlight_level = level;

        if (hardware_version_info() >= 3){
		if (level > 220) {
			ipu_sdc_set_brightness(255);
			ad7877_bright_ctrl(level);
		}
		else if (level > 120) {
			ipu_sdc_set_brightness(255);
			ad7877_bright_ctrl(level-110);
		}
		else if (level > 100) {
			ipu_sdc_set_brightness(255);
			ad7877_bright_ctrl(level-80);
		}
		else {
			ipu_sdc_set_brightness(level+50);
		}
	}
        else {
                ad7877_bright_ctrl(level);
        }

        return 0;
}

/*
 * board_inverter_ctrl()
 *
 * Manage the ON /OFF for LCD screen panel
 */
int board_inverter_ctrl(u8 state)
{
        mxcfb_drv_data.inverter_state = state;

        gpio_lcd_state(state);

        return 0;
}

/*
 * ad7877_read()
 *
 * Retrieve data points from touch controller and send them
 * to framebuffer.
 */
static int ad7877_read(void)
{
        unsigned int buffer = 0;
        unsigned int buffer1 = 0;
        unsigned int resistance;

        /* Read the touch coordinates */

        spi_read_7877reg(AD7877_REG_XPLUS, &buffer);
        spi_read_7877reg(AD7877_REG_YPLUS, &buffer1);

        /* Version Hard 1.0 : old touchscreen + connecteur inversé */
        if (versionHard == HARDVERSION_1_0) {
                ad7877->tc.x = ((4048) -(u16)(buffer)) & MASK_12BITS;
                ad7877->tc.y = (4048 -(u16)(buffer1)) & MASK_12BITS;
        }

        /* Version Hard 2.0 : new touchscreen + connecteur inversé */
        else if (versionHard == HARDVERSION_2_0) {
                ad7877->tc.y = ((u16)buffer) & MASK_12BITS;
                ad7877->tc.x = (4048 -(u16)(buffer1)) & MASK_12BITS;
        }

        /* Version Hard 2.1 : new touchscreen + connecteur normal */
        /* VGA 2.1 est en fait la version 1.3.0 */
        else {
                ad7877->tc.x = (4048 -(u16)(buffer)) & MASK_12BITS;
                ad7877->tc.y = ((u16)buffer1) & MASK_12BITS;
        }

        spi_read_7877reg(AD7877_REG_Z1, &buffer);
        ad7877->tc.z1 = (u16)(buffer) & MASK_12BITS;

        spi_read_7877reg(AD7877_REG_Z2, &buffer);
        ad7877->tc.z2 = (u16)(buffer) & MASK_12BITS;

        /* Calculate the pressure */
        if (ad7877->tc.z1 >10){
                resistance = 10*(((X_PLATE/10) * ad7877->tc.x *
                                  (ad7877->tc.z2 - ad7877->tc.z1)) / ad7877->tc.z1);
                resistance = (resistance + 2048) >> 12;
        } else {
                resistance = 0;
        }

        /* Send the coordinates to Framebuffer */
        if (resistance > 0) {
                input_report_abs(ad7877->input, ABS_X, ad7877->tc.x);

                input_report_abs(ad7877->input, ABS_Y, ad7877->tc.y);

                input_report_abs(ad7877->input, ABS_PRESSURE, resistance);

                if(ad7877->pendown == 0){
                        input_report_key(ad7877->input, BTN_TOUCH, 0);
                        input_sync(ad7877->input);
                        input_report_key(ad7877->input, BTN_TOUCH, 1);
                        ad7877->pendown = 1;
                }

                input_sync(ad7877->input);

                mod_timer(&ad7877->timer_appui, jiffies + TS_POLL_PERIOD100);
        } else {
                if(ad7877->pendown != 0){
                        input_report_key(ad7877->input, BTN_TOUCH, 0);
                        input_sync(ad7877->input);
                        ad7877->pendown = 0;
                }
        }
#ifdef CONFIG_DEBUG1
        printk ("\n ABS_X: = %d ABS_Y: = %d ABS_Z1: = %d ABS_Z2: = %d ABS_PRESS: = %u \n",
                 ad7877->tc.x, ad7877->tc.y, ad7877->tc.z1, ad7877->tc.z2, resistance);
#endif
        /* Start another conversion data will be available next call */
        spi_write_7877reg(AD7877_REG_CTRL1, 0x002);

        return 0;
}

/*
 * ad7877_pen_irq()
 *
 * Management of interrupt PENIRQ : Pen Interrupt : Awake the process
 */
static irqreturn_t ad7877_pen_irq(int irq, void *handle, struct pt_regs *regs)
{
        spin_lock(&ad7877->lock);

        disable_irq(ad7877->pen_irq);

        /* Restart the timer */
        ad7877->timer_saisie.expires = jiffies + TS_POLL_PERIOD10;
        add_timer(&ad7877->timer_saisie);

        spin_unlock(&ad7877->lock);

        return IRQ_HANDLED;
}

/*
 * ad7877_dav_irq()
 *
 * Management of interrupt DAV : Data Available Output
 */
static irqreturn_t ad7877_dav_irq(int irq, void *handle, struct pt_regs *regs)
{

        spin_lock(&ad7877->lock);

        ad7877_read();

        spin_unlock(&ad7877->lock);

        return IRQ_HANDLED;
}

/*
 * ad7877_alert_irq()
 *
 * Management of interrupt ALERT : pb battery or temp
 */
static irqreturn_t ad7877_alert_irq(int irq, void *handle, struct pt_regs *regs)
{
#ifdef CONFIG_MACH_MX31TABLET

        struct ad7877_bat *dev = (struct ad7877_bat*) handle;
        spin_lock(&dev->lock);

        /* Call the alert management function */
        ad7877_alert((unsigned long)handle);

        spin_unlock(&ad7877->lock);
#endif
        return IRQ_HANDLED;
}

/*
 * ad7877_alert()
 *
 * Management of ALERT : routine that is called to determine
 * the Alert reason and to reacte.
 */
#ifdef CONFIG_MACH_MX31TABLET
static int ad7877_alert(unsigned long handle)
{
        unsigned int buffer, buffer2, buffer3;
        struct ad7877_bat  *dev = (void *)handle;
        unsigned int GpioData, trial;

        spi_read_7877reg(AD7877_REG_GPIODATA, &buffer);
        GpioData = (u16)(buffer) & MASK_GPIO_DAT;

        /* Read the Alert status register for TEMP1,AUX1,BAT1,BAT2 */
        spi_read_7877reg(AD7877_REG_ALERT, &buffer);

        buffer &= MASK_8BITS;

        /* Case Temperature limit high exceeded */
        if (buffer & MASK_TEMP1LOW) {
             	/* Take a new measure to check if the default is always present */
                spi_read_7877reg(AD7877_REG_TEMP1, &buffer);
                if (((u16)(buffer) & MASK_12BITS) > ad7877->temp_high_limit) {
                        CS_WARNING("Templow");
//                      board_bright_ctrl(mxcfb_drv_data.backlight_level/2); // Reduce the TFT backlight
//                      board_inverter_ctrl(!mxcfb_drv_data.inverter_state); // Put off the TFT backlight
                }
                buffer ^= MASK_TEMP1LOW;
        }
        /* Case Temperature limit low exceeded */
        if (buffer & MASK_TEMP1HIGH) {
                /* Take a new measure to check if the default is always present */
                spi_read_7877reg(AD7877_REG_TEMP1, &buffer);
                if (((u16)(buffer) & MASK_12BITS) < ad7877->temp_low_limit){
                        CS_WARNING("TempHigh");
                        // Action ???
                }
                buffer ^= MASK_TEMP1HIGH;
        }
        /* Case Battery 1 limit high exceeded */
        if (buffer & MASK_BAT1HIGH) {
                CS_WARNING("Bat1High");
                buffer ^= MASK_BAT1HIGH;
                // Action ???
        }
        /* Case Battery 1 limit low exceeded */
        if (buffer & MASK_BAT1LOW) {
		/* test 3 Vbat values to be sure that we are under 
		   the unloading value */               
		spi_read_7877reg(AD7877_REG_BAT1, &buffer2);
		for (trial = 0; trial < 2; trial++) {
			ad7877_tempo_reg();
			spi_read_7877reg(AD7877_REG_BAT1, &buffer3);
			if (((u16)(buffer3) & MASK_12BITS) > dev->VbatLow) {
				goto bad_value;	
			}
		if (buffer3 > buffer2)
			buffer2 = buffer3;
                }
		CS_WARNING("	Vbat = %x \n", buffer2); 
                /* The Battery is completely discharged so we must
                   inform the system : The limit is 0xCB0 (9.91),
		   10.06v), about 4mn before the screen is off and
		   about 5mn before the system is off
		 */
                if (((u16)(buffer2) & MASK_12BITS) <= 0xCB0){  
                        CS_WARNING("Warning : Battery unloading !!");
                        /* Alert the System */
                        dev->unloaded_level = 1;
                        spi_write_7877reg(AD7877_REG_BAT1LOW,  0x000);
			dev->VbatLow = 0;
                        dev->battery_unloaded = 1;
                        wake_up_interruptible(&dev->poll_wait);
                }
                /* The Battery is 1 or 2 mn before to be completely
                   discharged so we must inform the system : The
		   limit is 0xDOO (10.16v), about 2mn before unloading
		 */
                else if ((((u16)(buffer2) & MASK_12BITS) <= 0xD00) && (!dev->cpteur)) {
                        CS_WARNING("Warning : Battery almost unloaded !!");
                        /* Alert the System */
			dev->cpteur = 1;
                        dev->unloaded_level = 0;
			spi_write_7877reg(AD7877_REG_BAT1LOW,  0xCB0);
			dev->VbatLow = 0xCB0;
                        dev->battery_unloaded = 1;
                        wake_up_interruptible(&dev->poll_wait);
                }
bad_value :
                buffer ^= MASK_BAT1LOW;
        }
        /* Case GPIO4 : Charger connected or not */
        /* We must inform the system when the charger is connected
         */
        /* Test if the charger is or not connected */
        if (GpioData & MASK_GPIO4_DAT) {
		ad7877_monitoring();
                spi_read_7877reg(AD7877_REG_GPIOCTRL2, &buffer2);
                if (((u16)(buffer2) & MASK_12BITS) == 0xC7) {
                        CS_WARNING("Warning : BATTERY BECOMES IN CHARGE !!");
			ad7877_bat->on_the_base = 1;
                        /* Stop the unloading detection */
			dev->VbatLow = 0x000;
                        spi_write_7877reg(AD7877_REG_BAT1LOW,  0x000);
                        /* Alert the System */
                        ad7877_bat->battery_connected = 1;
                        wake_up_interruptible(&dev->poll_wait);
                        /* GPIO4 = remplace par detection de la deconnection */
                        spi_write_7877reg(AD7877_REG_EXTWRITE, 0x1C3);
			dev->cpteur = 0;
                }
                else {
                        CS_WARNING("Warning : BATTERY BECOMES IN DECHARGE !!");
			ad7877_bat->on_the_base = 0;
                        /* Restart the Alert IT at 2mn before unloading */
			dev->VbatLow = 0xD00;	
                        spi_write_7877reg(AD7877_REG_BAT1LOW,  0xD00);
                        /* Alert the System */
			udelay(1000);
                        ad7877_bat->battery_connected = 1;
                        wake_up_interruptible(&dev->poll_wait);
                         /* GPIO4 = remplace par detection de la connection */
                        spi_write_7877reg(AD7877_REG_EXTWRITE, 0x1C7);
               }

                /* Start another conversion data will be available next call */
                spi_write_7877reg(AD7877_REG_CTRL1, 0x002);
        }

        /* Alert status and enable register */
        spi_write_7877reg(AD7877_REG_ALERT, 0x200);

        /* Start another conversion data will be available next call */
        spi_write_7877reg(AD7877_REG_CTRL1, 0x002);

        return 0;
}
#endif
/*
 * TouchIsPenDown()
 *
 * Determine if pen is up or down
 */
static int TouchIsPenDown(void)
{
    if (gpio_get_data(0, GPIO_INT3))
        return 1;
    else
        return 0;
}

/*
 * ad7877_timer_appui()
 *
 * Timer that is called at each time the screen is touched down to wait
 * it is touched up
 */
static void ads7877_timer_appui(unsigned long handle)
{
        spin_lock(&ad7877->lock);

        if(ad7877->pendown != 0){
                input_report_key(ad7877->input, BTN_TOUCH, 0);
                input_sync(ad7877->input);
                ad7877->pendown = 0;
        }

        input_report_key(ad7877->input, BTN_TOUCH, 0);
        input_sync(ad7877->input);

        spin_unlock(&ad7877->lock);
}

/*
 * ad7877_timer_saisie()
 *
 * State machine that is used to read the Touchscreen data :
 * This timer is called by the PEN_IRQ (when the screen is touched)
 */
static void ads7877_timer_saisie(unsigned long handle)
{
        spin_lock(&ad7877->lock);

        switch(TouchPanelState) {
                case TOUCH_STATE_PENUP:
                        if (!TouchIsPenDown()) {
                                /* This is the first sample after detecting pendown.
                                 * This delay is a software debounce. Trigger a conversion,
                                 * and move on to next state.
                                 */
                                TouchPanelState = TOUCH_STATE_PENCONVERTING;
                                spi_write_7877reg(AD7877_REG_CTRL1, 0x002);
                                /* Restart the timer */
                                ad7877->timer_saisie.expires = jiffies + TS_POLL_PERIOD10;
                                add_timer(&ad7877->timer_saisie);
                        }
                        else {
                                /* Re-enable pen down interrupt */
                                enable_irq(ad7877->pen_irq);
                        }
                        break;

                case TOUCH_STATE_PENDEBOUNCE:
                        if (!TouchIsPenDown()) {
                                /* This is the first sample after detecting pendown.
                                 * This delay is a software debounce. Trigger a conversion,
                                 * and move on to next state.
                                 */
                                TouchPanelState = TOUCH_STATE_PENCONVERTING;
                                spi_write_7877reg(AD7877_REG_CTRL1, 0x002);
                                /* Restart the timer */
                                ad7877->timer_saisie.expires = jiffies + TS_POLL_PERIOD10;
                                add_timer(&ad7877->timer_saisie);
                        }
                        else {
                                /* Re-enable pen down interrupt */
                                TouchPanelState = TOUCH_STATE_PENUP;
                                enable_irq(ad7877->pen_irq);
                        }
                        break;

                case TOUCH_STATE_PENCONVERTING:
                        /* Status:
                         * The timer has expired.  It's time to take another sample, if the pen
                         * is still down.  We've not yet indicated a pen down to GWE, so if we detect
                         * pen back up in this state, don't indicate PENUP to GWE.
                         */
                        if (!TouchIsPenDown()) {
                                TouchPanelState = TOUCH_STATE_PENDOWN;
                                ad7877_read();
                                /* Restart the timer */
                                ad7877->timer_saisie.expires = jiffies + TS_POLL_PERIOD10;
                                add_timer(&ad7877->timer_saisie);
                        }
                        else {
                                /* Re-enable pen down interrupt */
                                TouchPanelState = TOUCH_STATE_PENUP;
                                enable_irq(ad7877->pen_irq);
                        }
                        break;

               case TOUCH_STATE_PENDOWN:
                        /* Status:
                         * The MDD timer has expired.  It's time for us to take another sample, if the pen
                         * is still down.  If not, report PENUP. Wait the pen is up !
                         */
                        if (!TouchIsPenDown()) {
                                ad7877_read();
                                /* Restart the timer */
                                ad7877->timer_saisie.expires = jiffies + TS_POLL_PERIOD10;
                                add_timer(&ad7877->timer_saisie);
                        }
                        else {
                                /* Re-enable pen down interrupt */
                                TouchPanelState = TOUCH_STATE_PENUP;
                                enable_irq(ad7877->pen_irq);
                        }
                        break;

                default:
                        /* We shouldn't ever get here...
                         */
                        break;
                }

        spin_unlock(&ad7877->lock);

}

/*
 * ad7877_timer_lecture()
 *
 * Timer that is called all the 10s to read the ad7877 registers :
 * Temperature, Auxilliaries and Batteries registers
  */
static void ads7877_timer_lecture(unsigned long handle) 
{
	spin_lock(&ad7877->lock);

        /* Start another conversion data will be available next call */
        spi_write_7877reg(AD7877_REG_CTRL1, 0x002);

	/* Restart the timer to 10 seconds */
        ad7877->timer_lecture.expires = jiffies + 10*HZ;
        add_timer(&ad7877->timer_lecture);

	spin_unlock(&ad7877->lock);

}

/*
 * ad7877_tempo_reg()
 *
 * tempo to read the as7877 register.
 */
#ifdef CONFIG_MACH_MX31TABLET
int Vbat, Vref, Ibat;
static void ad7877_tempo_reg(void)
{
        unsigned int tempo;
	
	spi_write_7877reg(AD7877_REG_CTRL1, 0x002);	
	for (tempo = 0; tempo <= 10; tempo++) udelay(1000);
}
	
/*
 * ad7877_monitoring()
 *
 * Monitoring the Battery charger.
 */
static int ad7877_monitoring()
{
        unsigned int buffer = 0, buffer1, Vsys;
        unsigned int GpioData;

	ad7877_tempo_reg();
	
	/* Test if the charger is or not connected */
        spi_read_7877reg(AD7877_REG_GPIODATA, &buffer);
        GpioData = (u16)(buffer) & MASK_GPIO_DAT;

        /* Charger connected if Vsys > 2.2 V (13 V / 6)
         * For Vref = 2.5V the LSB per volt is 1638 (2.5V = FFFh so 1V = 4095/2.5 = 1638)
         * Aux1: 2.2 V => 2.2 * 1638 => 0xE13k     (take 0xE00 as reference)
         */
        spi_read_7877reg(AD7877_REG_AUX1, &buffer);
        Vref = (u16)(buffer) & MASK_12BITS; 
	Vsys = (u16)(buffer) & MASK_12BITS;
        spi_read_7877reg(AD7877_REG_GPIOCTRL2, &buffer);
        if (((u16)(buffer) & MASK_12BITS) == 0xC3) {
                ad7877_bat->on_the_base = 1;
                /* Shut down SLEEP Led */
                spi_write_7877reg(AD7877_REG_EXTWRITE, (0x0200 | (GpioData & ~MASK_GPIO3_DAT)));
		/* Read the Vbat */
                spi_read_7877reg(AD7877_REG_BAT1, &buffer); 
		Vbat = (u16)(buffer) & MASK_12BITS; 
                /* Read the Mon_Icharge voltage (take the minor on 3 measures) */
		ad7877_tempo_reg();
                spi_read_7877reg(AD7877_REG_BAT2, &buffer);
		ad7877_tempo_reg();
                spi_read_7877reg(AD7877_REG_BAT2, &buffer1);
		if (buffer1 < buffer)
			buffer = buffer1;
		ad7877_tempo_reg();
                spi_read_7877reg(AD7877_REG_BAT2, &buffer1);
		if (buffer1 < buffer)
			buffer = buffer1;
		Ibat = (u16)(buffer) & MASK_12BITS;		
                /* Measure of the loading of the battery (Mon_Icharge)
                   Battery2 : For Vref = 2.5V the LSB per volt is 819
                   (5V = FFFh so 1V = 4095/5 = 819) 2 A = 1.19v 0 A = 0.309v
                   => n Ampere = n * (1.19 - 0.309)/2 + 0.309
                   Loading = 2h40 => 1 progressive level all the 32 mn (except for 0)
                     5 prog. levels if < 0.55 A (1C3h)
                     4 prog. levels if < 0.95 A (253h)
                     3 prog. levels if < 1.65 A (34Fh)
                     2 prog. levels if < 1.90 A (3A9h)
                     1 prog. levels if < 2 A    (3CDh)
                     0 prog. levels if >= 2 A   (3CDh)
                  */
                if (((u16)(buffer) & MASK_12BITS) < 0x080)
                        ad7877_bat->charge_level = 5;  
                else if (((u16)(buffer) & MASK_12BITS) < 0x090)
                        ad7877_bat->charge_level = 4;
                else if (((u16)(buffer) & MASK_12BITS) < 0x100)
                        ad7877_bat->charge_level = 3;
                else if (((u16)(buffer) & MASK_12BITS) < 0x1e0)
                        ad7877_bat->charge_level = 2;
                else if (((u16)(buffer) & MASK_12BITS) < 0x200)
                        ad7877_bat->charge_level = 1;
                else
                        ad7877_bat->charge_level = 0;

        }
        /* The charger is unconnected : Vsys < 2.2 V (13 V / 6)
         * For Vref = 2.5V the LSB per volt is 1638 (2.5V = FFFh so 1V = 4095/2.5 = 1638)
         * Aux1: 2.2 V => 2.2 * 1638 => 0xE13k
         */
        else {
                //Provi : allume led
//              spi_write_7877reg(AD7877_REG_EXTWRITE, (0x0200 | (GpioData | MASK_GPIO3_DAT)));

                ad7877_bat->on_the_base = 0;
                /* Measure of the unloading of the battery (Mon_Bat = Vbat/2.5)
                   Battery1 : For Vref = 2.5V the LSB per volt is 819
                   (5V = FFFh so 1V = 4095/5 = 819) 12.3=> 12.3 / 2.5 * 819 = 7DEh
                       5 prog. levels = 12.5 to 12.2v (FF5h to F90h)
                       4 prog. levels = 12.2 to 11.7v (F90h to F00h)
                       3 prog. levels = 11.7 to 11.5v (F00h to EB0h)
                       2 prog. levels = 11.5 to 11.3v (EB0h to E70h)
                       1 prog. levels = 11.3 to 10.1v (E70h to D00h)
                       0 prog. levels < 10.1 v           
                   */
                /* Read the Mon_Vcharge voltage (take the major on 3 measures) */
		ad7877_tempo_reg();
		spi_read_7877reg(AD7877_REG_BAT1, &buffer);
		ad7877_tempo_reg();
                spi_read_7877reg(AD7877_REG_BAT1, &buffer1);
		if (buffer1 > buffer)
			buffer = buffer1;
		ad7877_tempo_reg();
                spi_read_7877reg(AD7877_REG_BAT1, &buffer1);
		if (buffer1 > buffer)
			buffer = buffer1;
		Vbat = (u16)(buffer) & MASK_12BITS; //provi
                spi_read_7877reg(AD7877_REG_BAT2, &buffer);
        	Ibat = (u16)(buffer) & MASK_12BITS; //provi
                spi_read_7877reg(AD7877_REG_BAT1, &buffer);
                if (((u16)(buffer) & MASK_12BITS) >  0xF90)
                        ad7877_bat->charge_level = 5;
                else if (((u16)(buffer) & MASK_12BITS) >  0xF00)
                        ad7877_bat->charge_level = 4;
                else if (((u16)(buffer) & MASK_12BITS) >  0xEB0)
                        ad7877_bat->charge_level = 3;
                else if (((u16)(buffer) & MASK_12BITS) >  0xE70)
                        ad7877_bat->charge_level = 2;
                else if (((u16)(buffer) & MASK_12BITS) >  0xD00)
                        ad7877_bat->charge_level = 1;
                else {
                        ad7877_bat->charge_level = 0;

                        /* Shut up SLEEP Led */
                        spi_write_7877reg(AD7877_REG_EXTWRITE, (0x0200 | (GpioData | MASK_GPIO3_DAT)));
                }
        }

        return 0;
}

ssize_t ad7877_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)
{
        int retval = 0;

        struct ad7877_bat *dev =  filp->private_data;

        spin_lock(&dev->lock);

        spin_unlock(&dev->lock);

        return retval;
}

int ad7877_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
        int retval = 0, Alert_info = 0;

        struct ad7877_bat *dev = filp->private_data ;

        /*
         * extract the type and number bitfields, and don't decode
         * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
         */
        if (_IOC_TYPE(cmd) != AD7877_IOC_MAGIC)
                return -ENOTTY;

        /*
         * the direction is a bitmask, and VERIFY_WRITE catches R/W
         * transfers. `Type' is user-oriented, while
         * access_ok is kernel-oriented, so the concept of "read" and
         * "write" is reversed
         */
        if (_IOC_DIR(cmd) & _IOC_READ)
                retval = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
        else if (_IOC_DIR(cmd) & _IOC_WRITE)
                retval =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
        if (retval)
                return -EFAULT;

        switch (cmd) {
                case AD7877_IOCTL_GET_ALERT_INFO:
                        /* Get the Alert condition : presence charger : 1 , battery unloaded : 0 */
                        if (dev->battery_connected)
                                Alert_info = 1;
                        put_user(Alert_info, (unsigned char*)arg);
                        dev->battery_connected = 0;
                        dev->battery_unloaded = 0;
                        retval = 1;
                        break;

                case AD7877_IOCTL_GET_PRESENCE_CHARGER:
                        /* Get the charger state : 1 if connected, 0 if unconnected */
		        /* Read the temperature, batteries and auxilliaries measures */
                        put_user(dev->on_the_base, (unsigned char*)arg);
			retval = 1;
			break;

                case AD7877_IOCTL_GET_BATTERY_UNLOADED:
                        /* Get the alarm Battery unloaded : 0 if almost unloaded, 1 if unloaded */
                        put_user(dev->unloaded_level, (unsigned char*)arg);
                        dev->unloaded_level = 0;
                        retval = 1;
                        break;

                case AD7877_IOCTL_GET_BATTERY_LEVEL:
                        /* Get the battery level : number of loading or unloading progressive
                           level, between 0 and 6 levels  */
                        /* Read the temperature, batteries and auxilliaries measures */
		        ad7877_monitoring();
			CS_WARNING("Vbat = %x \n", Vbat); 
			CS_WARNING("Vsys = %x \n", Vref);
			CS_WARNING("Ibat = %x \n", Ibat);
			put_user(dev->charge_level, (unsigned char*)arg);
			retval = 1;
                        break;

                default:
                        retval = -ENOIOCTLCMD;
        }
        return retval;
}

unsigned int ad7877_poll(struct file *filp, struct poll_table_struct *poll_data)
{

    unsigned int mask = 0;

    struct ad7877_bat *dev = filp->private_data ;

    spin_lock(&dev->lock);

    /* adding a wait queue to the poll table*/
    poll_wait(filp, &dev->poll_wait, poll_data);

    if ((dev->battery_connected) || (dev->battery_unloaded))
        mask |= (POLLIN | POLLRDNORM);

    spin_unlock(&dev->lock);

    return mask;

}

int ad7877_open(struct inode *inode, struct file *filp)
{
        int retval = 0;
        unsigned int buffer = 0;

        struct ad7877_bat *dev; /* device information */

        dev = container_of(inode->i_cdev, struct ad7877_bat, cdev);
        filp->private_data = dev; /* for other methods */

        spin_lock(&dev->lock);

        (dev->opened)++ ;

        if((dev->opened)==1) {
                /* Initialization Temperature limits high and low :
                   Temperature : between +5°C and +45°C
                   For Vref = 2.5V the LSB per degree is -3.44
                   If the calibration reading Dcal at 25°C is x
                   Dhigh = (45 - 25) * -3.44 + x = -68.8 + x
                   Dlow  = (05 - 25) * -3.44 + x = +68.8 + x
                 */
		spi_read_7877reg(AD7877_REG_TEMP1, &buffer);

                ad7877->temp_high_limit = ((u16)(buffer) & MASK_12BITS) + 0x44;
                ad7877->temp_low_limit  = ((u16)(buffer) & MASK_12BITS) - 0x44;

                spi_write_7877reg(AD7877_REG_TEMP1HIGH, (ad7877->temp_high_limit));
                spi_write_7877reg(AD7877_REG_TEMP1LOW,  (ad7877->temp_low_limit));

                /* TEMP1,BAT1 and BAT2 can generate IT ALERT */
                spi_write_7877reg(AD7877_REG_ALERT, 0x200);

                /* GPIO4 = If charger connected, init the unconnected detection */
                spi_read_7877reg(AD7877_REG_GPIODATA, &buffer);
                if (((u16)(buffer) & MASK_GPIO_DAT) & MASK_GPIO4_DAT) {
			ad7877_bat->on_the_base = 1;
                        spi_write_7877reg(AD7877_REG_EXTWRITE, 0x1C3);
			dev->VbatLow = 0x000;
                        spi_write_7877reg(AD7877_REG_BAT1LOW,  0x000);
		}
                /* else, init the connected detection */
                else {
			ad7877_bat->on_the_base = 0;
			spi_write_7877reg(AD7877_REG_EXTWRITE, 0x1C7);
			dev->VbatLow = 0xD00; 
                        spi_write_7877reg(AD7877_REG_BAT1LOW,  0xD00);
		}
          
		/* Init the 2mn before the unloading test */
		dev->cpteur = 0;
                
                /* Enable Interrupts */
        	enable_irq (ad7877->alert_irq);
                
		/* Start conversion data will be available next call */
                spi_write_7877reg(AD7877_REG_CTRL1, 0x002);
        }
        spin_unlock(&dev->lock);

        return retval;
}

int ad7877_release(struct inode *inode, struct file *filp)
{
        int retval = 0;

        struct ad7877_bat *dev = filp->private_data;

        spin_lock(&dev->lock);

        dev->opened-- ;

        if(dev->opened == 0) {
		/* Disable Interrupts */
        	disable_irq (ad7877->alert_irq);
        }

        spin_unlock(&dev->lock);

        return retval;
}

struct file_operations ad7877_bat_fops = {
        .owner =    THIS_MODULE,
        .write =    ad7877_write,
        .poll =      ad7877_poll,
        .ioctl =    ad7877_ioctl,
        .open =     ad7877_open,
        .release =  ad7877_release,
};
#endif
/*
 * ad7877_probe()
 *
 * routine that is called when someone adds a
 * new SPI device that supports protocol and registers it as
 * an input device.
 */
static int __init ad7877_probe(struct platform_device *dev)
{

        if (!(ad7877 = kzalloc(sizeof(struct ad7877), GFP_KERNEL))){
                return 1;
        }

        spin_lock_init(&ad7877->lock);

        platform_set_drvdata(dev, ad7877); 

        /*** Initialisation du timer appui ***/

        init_timer(&ad7877->timer_appui);
        ad7877->timer_appui.data = (unsigned long) ad7877;
        ad7877->timer_appui.function = ads7877_timer_appui;

        /*** Initialisation du timer saisie ***/

        init_timer(&ad7877->timer_saisie);
        ad7877->timer_saisie.data = (unsigned long) ad7877;
        ad7877->timer_saisie.function = ads7877_timer_saisie;

        /**** Initialisation du timer de lecture batterie apres 30 s ****/

        init_timer(&ad7877->timer_lecture);
        ad7877->timer_lecture.data = (unsigned long) ad7877;
        ad7877->timer_lecture.function = ads7877_timer_lecture;

	/* Start the reading timer */
	ad7877->timer_lecture.expires = jiffies + 10*HZ;
	add_timer(&ad7877->timer_lecture); 
	
#ifdef  AD7877_SMOOTHER
        ad7877->smooth_samples = 0;
        ad7877->smooth_current = 0;
#endif

        /*** Initialisation de la structure Input ***/

        ad7877->input = input_allocate_device();

        ad7877->input->cdev.dev = &dev->dev;
        ad7877->input->name = "AD7877 Touchscreen";
        snprintf(ad7877->phys, sizeof ad7877->phys, "%s/input0", MOD_NAME);
        ad7877->input->phys = ad7877->phys;
        ad7877->pendown = 0;

        ad7877->input->evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);
        ad7877->input->keybit[LONG(BTN_TOUCH)] = BIT(BTN_TOUCH);

        /**** Calibration ****/

        input_set_abs_params(ad7877->input, ABS_X, X_MIN, X_MAX, 0, 0);
        input_set_abs_params(ad7877->input, ABS_Y, Y_MIN, Y_MAX, 0, 0);
        input_set_abs_params(ad7877->input, ABS_PRESSURE, PRESS_MIN, PRESS_MAX, 0, 0);

        input_register_device(ad7877->input);

        /**** Request irq ***/

        /* PENIRQ */
        if (mxc_request_iomux(MX31_PIN_GPIO1_3, OUTPUTCONFIG_GPIO,
                          INPUTCONFIG_GPIO)) {
                return -EBUSY;
        }

        /* configure GPIO GPIO1_3 as input */
        mxc_set_gpio_direction(MX31_PIN_GPIO1_3, 1);

        /* save the IRQ number (IRQ nb 64+3) */
        ad7877->pen_irq = MXC_GPIO_TO_IRQ(GPIO_INT3);

        /* configure interrupt level for GPIO GPIO1_3 */
        set_irq_type(ad7877->pen_irq, IRQT_FALLING);

        if (request_irq(ad7877->pen_irq, ad7877_pen_irq,
                        0, MOD_NAME " pen", ad7877)) {

                input_unregister_device(ad7877->input);
                kfree(ad7877);
                gpio_free_irq(0, GPIO_INT3, GPIO_HIGH_PRIO);
                return -EBUSY;
        }

        ad7877->pen_irq_enabled = 1;

        /* DAV */
        if (mxc_request_iomux(MX31_PIN_GPIO1_2, OUTPUTCONFIG_GPIO,
                          INPUTCONFIG_GPIO)) {
                return -EBUSY;
        }

        /* configure GPIO GPIO1_2 as input */
        mxc_set_gpio_direction(MX31_PIN_GPIO1_2, 1);

        /* save the IRQ number (IRQ nb 64+2) */
        ad7877->dav_irq = MXC_GPIO_TO_IRQ(GPIO_INT2);

        /* configure interrupt level for GPIO GPIO1_2 */
        set_irq_type(ad7877->dav_irq, IRQT_FALLING);

        if (request_irq(ad7877->dav_irq, ad7877_dav_irq,      
                        0, MOD_NAME " dav", ad7877)) {
                input_unregister_device(ad7877->input);
                kfree(ad7877);
                gpio_free_irq(0, GPIO_INT2, GPIO_HIGH_PRIO);
                return -EBUSY;
        }
        ad7877->dav_irq_enabled = 1;

        /* ALERT IT initialization */
        mxc_request_iomux(MX31_PIN_GPIO1_4, OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);

        /* configure GPIO GPIO1_4 as input */
        mxc_set_gpio_direction(MX31_PIN_GPIO1_4, 1);

        /* save the IRQ number (IRQ nb 64+4) */
        ad7877->alert_irq = MXC_GPIO_TO_IRQ(GPIO_INT4);

        /* configure interrupt level for GPIO GPIO1_4 */
        set_irq_type(ad7877->alert_irq, IRQT_LOW);

        request_irq(ad7877->alert_irq, ad7877_alert_irq,0, MOD_NAME " alert", ad7877_bat);

        ad7877->alert_irq_enabled = 1;

	/*** Screen saver off :
             - DAV IRQ not used
             - PEN IRQ enable
	     - ALERT IRQ enable later
        ***/
        enable_irq (ad7877->pen_irq);
        disable_irq (ad7877->dav_irq);
        disable_irq (ad7877->alert_irq);
	
        TouchPanelState = TOUCH_STATE_PENUP;

        /*** Start the touchscreen controller :
             - in slave mode
             - in radiometric mode
        ***/

        spi_write_7877reg(AD7877_REG_CTRL1, 0x002);

        return 0;
}

/*
 * ad7877_suspend()
 *
 */
static int ad7877_suspend(struct platform_device *dev, pm_message_t state)
{
        unsigned long   flags;

        spin_lock_irqsave(&ad7877->lock, flags);

        /* Disable the IRQ except the PEN IRQ */

        if (ad7877->dav_irq_enabled) {
                ad7877->dav_irq_enabled = 0;
                disable_irq(ad7877->dav_irq);
        }

        if (ad7877->alert_irq_enabled) {
                ad7877->alert_irq_enabled = 0;
                disable_irq(ad7877->alert_irq);
        }

        /* we know the chip's in lowpower mode since we always
         * leave it that way after every request
         */

        spin_unlock_irqrestore(&ad7877->lock, flags);


        return 0;
}

/*
 * ad7877_resume()
 *
 */
static int ad7877_resume(struct platform_device *dev)
{
        /* Enable the IRQ */
        if (!ad7877->dav_irq_enabled) {
                ad7877->dav_irq_enabled = 1;
                enable_irq(ad7877->dav_irq);
        }

        if (!ad7877->alert_irq_enabled) {
                ad7877->alert_irq_enabled = 1;
                enable_irq(ad7877->alert_irq);
        }

        return 0;
}

/*
 * ad7877_remove()
 *
 */
static int __exit ad7877_remove(struct platform_device *dev)
{

        struct ad7877       *ad7877_t = platform_get_drvdata(dev);

        gpio_free_irq(0, GPIO_INT2, GPIO_HIGH_PRIO);
        gpio_free_irq(0, GPIO_INT3, GPIO_HIGH_PRIO);
 
        input_unregister_device(ad7877_t->input);

        kfree(ad7877_t);

        return 0;
}

/*!
 * Device definition for the AD7877
 */
static struct platform_driver ad7877_driver = {
        .probe       = ad7877_probe,
        .remove      = __exit_p(ad7877_remove),
        .suspend     = ad7877_suspend,
        .resume      = ad7877_resume,
        .driver      = {
                .name    = AD7877_NAME,
                .owner   = THIS_MODULE,
        },
};

/*!
 * Device definition for the AD7877
 */
static struct platform_device ad7877_device = {
        .name                   = AD7877_NAME,
        .id                     = 0,
};

/*
 * Return Hardware Version
 *
 */
int hardware_version_info(void)
{

        return versionHard;
}

/*
 * Parse Hardware Version passed on the kernel command line
 *
 */
static int __init hardware_version_setup(char *vers)
{

        if(strcmp(vers, "1.0") == 0){
                versionHard = HARDVERSION_1_0;
        } else  if(strcmp(vers, "2.0") == 0){
                versionHard = HARDVERSION_2_0;
        } else  if(strcmp(vers, "2.1") == 0){
                versionHard = HARDVERSION_2_1;
        } else  {
                versionHard = HARDVERSION_TABLET;
        }

        return 0;
}

__setup("hardversion=", hardware_version_setup);


/*
 * The functions for inserting/removing us as a module.
 */

/*
 * ad7877_init()
 *
 */
static int __init ad7877_init(void)
{
        int ret = 0;
        int buffer=0;
#ifdef CONFIG_MACH_MX31TABLET
        dev_t dev = 0;
#endif
        /* If the Hardware is unknown */
        if (!versionHard) {
                printk ("Touchscreen Driver not installed : Hardware version unknowned \n");
                return 1;
        }

        /* Configure the SPI controller */
        spi_7877_init();

        /* Configure touch controller
           - Disable TMR (single shot)
           - Internal reference
           - STOPACK is active hight
           - 1.024 ms first conversion delay
           - continously power ADC, reference
           - 2 us acquisition time
           - 4 sample average
        */
        spi_write_7877reg(AD7877_REG_CTRL2, 0x4a8);

        /* Verify the ad7877 configuration */
        spi_read_7877reg(AD7877_REG_CTRL2, &buffer);
        if (((u16)(buffer) & MASK_12BITS) != 0x4a8) {
                printk ("Touchscreen Driver not installed : The ad7877 doesn't answer \n");
                return 1;
        }

#ifdef CONFIG_MACH_MX31TABLET
        /* Battery :
           - Bat1 : MON_BAT : Battery supply voltage (Vbat / 5)
            = Out charge : detect the voltage available
                Between 2.5 v and 1.8 v (12.6 v and 9 V / 5)
                < 1.8 v (9 v / 6) discharged battery
            = In charge : charge indicator
           - Bat2 : MON_ICHARGE : Current monitoring Output
            = Linear indication of charging current
                Between 0.9 v and 1.19 V

            An Alert IT is available 1 or 2 mn before the
            complete discharged battery so a about 9.5v (614h)
        */
        spi_write_7877reg(AD7877_REG_BAT1HIGH, 0xFFF);

        /* Auxiliaries + GPIO:
           - Aux1 : MON_VSYST : System Voltage Input (= Vsys / 6)
            = 2.25 v (13.5 / 6) if the bord is on the base (ACP = 1)
            = 2.1 V to 1.5 V (12.6 / 6 to 9 / 6) or < 1.5 V (ACP = 0)
           - GPIO2 : CHG_N : Charge Indicator (IO input)
            = 0 : in charge
            = 1 : End of charge
           - GPIO3 : SLEEP : Charge Led Control (IO output)
            Indicator if Vsys if < 9 V (discharged battery)
           - GPIO4 : ACP : Battery charger indicator (IO input)
            = 0 : the charger is present
            = 1 : the charger is not present
         */

        /* GPIO1 = AUX1, GPIO2 = CHG_N input */
        spi_write_7877reg(AD7877_REG_EXTWRITE, 0x00E);

        /* GPIO3 = SLEEP output, GPIO4 = SHDN input */
        spi_write_7877reg(AD7877_REG_EXTWRITE, 0x1C6);

        /* Auxiliaries 1 : MON_VSYST : System Voltage Input (= Vsys / 6)
         * Charger connected if Vsys > 2.2 V (13 V / 6)
         * For Vref = 2.5V the LSB per volt is 1638 (2.5V = FFFh so 1V = 4095/2.5 = 1638)
         * Aux1: 2.2 V => 2.2 * 1638 => 0xE13k (take E00 as reference)
         */
        spi_write_7877reg(AD7877_REG_AUX1HIGH, 0xFFF);
        spi_write_7877reg(AD7877_REG_AUX1LOW,  0x000);

        /* Put X, Y, Z, Temperature, Batteries and auxilliaries measurement
           in slave mode
         */
        spi_write_7877reg(AD7877_REG_SEQ0, 0xffe);

       /* Register the device for the battery */
        if(ad7877_bat_major){
                dev = MKDEV(ad7877_bat_major,ad7877_bat_minor);
                ret = register_chrdev_region(dev, AD7877_BAT_NR_DEVS , "ad7877bat");
        }
        else
                ret = alloc_chrdev_region(&dev, ad7877_bat_minor ,AD7877_BAT_NR_DEVS ,"ad7877bat");

        /* Battery Device registration */
        ad7877_bat = kmalloc(AD7877_BAT_NR_DEVS * sizeof(struct ad7877_bat), GFP_KERNEL );
        if(!ad7877_bat){

                unregister_chrdev_region(dev, AD7877_BAT_NR_DEVS);
                return -ENOMEM; /* Make this more graceful */
        }

        memset(ad7877_bat, 0 , AD7877_BAT_NR_DEVS *  sizeof(struct ad7877_bat));

        spin_lock_init(&ad7877_bat->lock);

        init_waitqueue_head(&ad7877_bat->write_queue);
        init_waitqueue_head(&ad7877_bat->poll_wait);

        cdev_init(&ad7877_bat->cdev, &ad7877_bat_fops );
        ad7877_bat->cdev.owner = THIS_MODULE;
        ad7877_bat->cdev.ops = &ad7877_bat_fops;
        ret |= cdev_add(&ad7877_bat->cdev, dev, AD7877_BAT_NR_DEVS);
        if (ret){
                printk ("Touchscreen Driver not installed : Error adding Battery Device \n");
                cdev_del(&ad7877_bat->cdev);
                return ret;
        }
#endif
        /* Register the driver */
        ret = platform_driver_register(&ad7877_driver);
        if (ret == 0) {
                ret = platform_device_register(&ad7877_device);
                if (ret != 0) {
                        printk ("Touchscreen Driver not installed : Error registering the driver \n");
                        platform_driver_unregister(&ad7877_driver);
                }
        }
        return ret;
}

/*
 * ad7877_exit()
 *
 */
static void __exit ad7877_exit(void)
{
#ifdef CONFIG_MACH_MX31TABLET
        /* Unregister the battery device */
        dev_t devno = MKDEV(ad7877_bat_major, ad7877_bat_minor);
        if (ad7877_bat) {
                cdev_del(&ad7877_bat->cdev);
                kfree(ad7877_bat);
        }
        unregister_chrdev_region(devno,AD7877_BAT_NR_DEVS);
#endif
        /* Unregister the driver */
        platform_device_unregister(&ad7877_device);
        platform_driver_unregister(&ad7877_driver);

        /* Disable the touch controller */
        spi_write_7877reg(AD7877_REG_CTRL1, 0x000);
        spi_write_7877reg(AD7877_REG_CTRL2, 0x000);
}

module_init(ad7877_init);
module_exit(ad7877_exit);

MODULE_AUTHOR("Bruno Delcourt");
MODULE_DESCRIPTION("AD7877 TouchScreen Driver");
MODULE_LICENSE("GPL");
