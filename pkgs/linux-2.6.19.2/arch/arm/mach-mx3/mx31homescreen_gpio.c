/*
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright 2004-2007 Sagem Communications. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pmic_power.h>
#include <linux/mmc/host.h>
#include "iomux.h"

static int gpio_dect_usb_requested = 0;

void gpio_serviceled_active(void);
void gpio_serviceled_inactive(void);
void gpio_serviceled_pulse(void);
int gpio_serviceled_get(void);
void gpio_serviceled_set(int value);
int gpio_usbh2_active(void);
void gpio_usbh2_inactive(void);
int gpio_usbotg_hs_active(void);
void gpio_usbotg_hs_inactive(void);
void gpio_dect_usb_active(void);

/*!
 * @file mach-mx3/mx31ads_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO_MX31
 */

void gpio_activate_audio_ports(void);

/*!
 * This system-wise GPIO function initializes the pins during system startup.
 * All the statically linked device drivers should put the proper GPIO initialization
 * code inside this function. It is called by \b fixup_mx31xxx() during
 * system startup. This function is board specific.
 */
void mx31hsv1_gpio_init(void)
{
	/* Init the SOFT_STOP mecanism: this pin must be kept to 1, and set down to 0 to switch off the machine */
	mxc_request_iomux(HOMESCREEN_V1_GPIO_SOFT_STOP, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_SOFT_STOP, GPIO_DIR_OUTPUT);
	mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_SOFT_STOP, 1);

	/* craddle detector */
	mxc_request_iomux(HOMESCREEN_V1_GPIO_CRADDLE_DETECT, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
#define CRADDLE_DETECT_CFG   (PAD_CTL_DRV_NORMAL | PAD_CTL_PKE_NONE | PAD_CTL_SRE_SLOW | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS)
	mxc_iomux_set_pad(HOMESCREEN_V1_GPIO_CRADDLE_DETECT,CRADDLE_DETECT_CFG);
	mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_CRADDLE_DETECT, GPIO_DIR_INPUT);

	/* host2 and OTG host USB power supply shutdown */
	/* Power supplies will re-enabled later while the fsl-ehci driver is probed if they are needed */
	gpio_usbh2_active();
	gpio_usbh2_inactive();
	gpio_usbotg_hs_active();
	gpio_usbotg_hs_inactive();
}

int gpio_buttons_active(void)
{
   mxc_request_iomux(MX31_PIN_SCK6, OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);
   mxc_set_gpio_direction(MX31_PIN_SCK6,GPIO_DIR_INPUT);
   return IOMUX_TO_IRQ(MX31_PIN_SCK6);
}

void gpio_buttons_inactive(void)
{
   mxc_free_iomux(MX31_PIN_SCK6,OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
}



/*!
 * Setup GPIO for a UART port to be active
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_active(int port, int no_irda)
{
   /*
    * Configure the IOMUX control registers for the UART signals
    */
   switch (port) {
      /* UART 1 IOMUX Configs */
      case 0:
         mxc_request_iomux(MX31_PIN_RXD1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_TXD1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_RTS1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_CTS1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_DTR_DCE1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_RI_DCE1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_DCD_DCE1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         /* UART 2 IOMUX Configs */
      case 1:
         if (no_irda == 1) {
            mxc_request_iomux(MX31_PIN_RTS2, OUTPUTCONFIG_FUNC,
                  INPUTCONFIG_FUNC);
            mxc_request_iomux(MX31_PIN_CTS2, OUTPUTCONFIG_FUNC,
                  INPUTCONFIG_FUNC);
            mxc_request_iomux(MX31_PIN_DTR_DCE2, OUTPUTCONFIG_FUNC,
                  INPUTCONFIG_FUNC);
         }
         break;
         /* UART 3 IOMUX Configs */
      case 2:
         mxc_request_iomux(MX31_PIN_CSPI3_MOSI, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_CSPI3_MISO, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_CSPI3_SCLK, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_CSPI3_SPI_RDY, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         break;
         /* UART 4 IOMUX Configs */
      case 3:
         mxc_request_iomux(MX31_PIN_ATA_CS0, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_ATA_CS1, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_ATA_DIOR, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_ATA_DIOW, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         break;
         /* UART 5 IOMUX Configs */
      case 4:
         mxc_request_iomux(MX31_PIN_PC_VS2, OUTPUTCONFIG_ALT2,
               INPUTCONFIG_ALT2);
         mxc_request_iomux(MX31_PIN_PC_RST, OUTPUTCONFIG_ALT2,
               INPUTCONFIG_ALT2);
         mxc_request_iomux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_ALT2,
               INPUTCONFIG_ALT2);
         mxc_request_iomux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_ALT2,
               INPUTCONFIG_ALT2);
         break;
      default:
         break;
   }
   /*
    * TODO: Configure the Pad registers for the UART pins
    */
}

/*!
 * Setup GPIO for a UART port to be inactive
 *
 * @param  port         a UART port
 * @param  no_irda      indicates if the port is used for SIR
 */
void gpio_uart_inactive(int port, int no_irda)
{
   switch (port) {
      case 0:
         /* SAGEM: Disable this avoids a warning stack when the kernel
          * closes the console when starting </sbin/init>
          */
#if !defined(CONFIG_MACH_MX31HSV1)
         mxc_free_iomux(MX31_PIN_RXD1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_TXD1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_RTS1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_CTS1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_DTR_DCE1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_RI_DCE1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_DCD_DCE1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
#endif // CONFIG_MACH_MX31HSV1
         break;
      case 1:
         if (no_irda == 1) {
            mxc_free_iomux(MX31_PIN_RTS2, OUTPUTCONFIG_GPIO,
                  INPUTCONFIG_GPIO);
            mxc_free_iomux(MX31_PIN_CTS2, OUTPUTCONFIG_GPIO,
                  INPUTCONFIG_GPIO);
            mxc_free_iomux(MX31_PIN_DTR_DCE2, OUTPUTCONFIG_GPIO,
                  INPUTCONFIG_GPIO);
         }
         break;
      case 2:
         mxc_free_iomux(MX31_PIN_CSPI3_MOSI, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_CSPI3_MISO, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_CSPI3_SCLK, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_CSPI3_SPI_RDY, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         break;
      case 3:
         mxc_free_iomux(MX31_PIN_ATA_CS0, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_ATA_CS1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_ATA_DIOR, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_ATA_DIOW, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         break;
      case 4:
         mxc_free_iomux(MX31_PIN_PC_VS2, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_PC_RST, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_PC_BVD1, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         mxc_free_iomux(MX31_PIN_PC_BVD2, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_GPIO);
         break;
      default:
         break;
   }
}

/*!
 * Configure the IOMUX GPR register to receive shared SDMA UART events
 *
 * @param  port         a UART port
 */
void config_uartdma_event(int port)
{
   switch (port) {
      case 1:
         /* Configure to receive UART 2 SDMA events */
         mxc_iomux_set_gpr(MUX_PGP_FIRI, false);
         break;
      case 2:
         /* Configure to receive UART 3 SDMA events */
         mxc_iomux_set_gpr(MUX_CSPI1_UART3, true);
         break;
      case 4:
         /* Configure to receive UART 5 SDMA events */
         mxc_iomux_set_gpr(MUX_CSPI3_UART5_SEL, true);
         break;
      default:
         break;
   }
}

EXPORT_SYMBOL(gpio_uart_active);
EXPORT_SYMBOL(gpio_uart_inactive);
EXPORT_SYMBOL(config_uartdma_event);


#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)

/*!
 * Setup GPIO for Keypad  to be active
 *
 * \todo check requests
 */
void gpio_keypad_active(void)
{
   /*
    * Configure the IOMUX control register for keypad signals.
    */
   mxc_request_iomux(MX31_PIN_KEY_COL0, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
   mxc_request_iomux(MX31_PIN_KEY_COL1, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
   mxc_request_iomux(MX31_PIN_KEY_COL2, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);

   /*! \TODO Check if this is needed. Schematic shows this is connected to GND */
   /*   mxc_request_iomux(MX31_PIN_KEY_COL3, OUTPUTCONFIG_FUNC,
        INPUTCONFIG_FUNC);
        */
   mxc_request_iomux(MX31_PIN_KEY_ROW0, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
   mxc_request_iomux(MX31_PIN_KEY_ROW1, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
   if ( get_board_type() < BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      /*CLD: FIXME those were there on HSV1 and HSV2 but it seems to be wrong */
      mxc_request_iomux(MX31_PIN_KEY_COL4, OUTPUTCONFIG_FUNC,
            INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_KEY_COL5, OUTPUTCONFIG_FUNC,
            INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_KEY_COL6, OUTPUTCONFIG_FUNC,
            INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_KEY_COL7, OUTPUTCONFIG_FUNC,
            INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_KEY_ROW2, OUTPUTCONFIG_FUNC,
            INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_KEY_ROW3, OUTPUTCONFIG_FUNC,
            INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_KEY_ROW4, OUTPUTCONFIG_FUNC,
            INPUTCONFIG_FUNC);
   }

#if !defined(CONFIG_MACH_MX31HSV1)
   /* CLD: ROW5 and ROW7 are used as Board identifier, ROW6 is kept reserved as well */
   mxc_request_iomux(MX31_PIN_KEY_ROW5, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
   mxc_request_iomux(MX31_PIN_KEY_ROW6, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
   mxc_request_iomux(MX31_PIN_KEY_ROW7, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
#endif // CONFIG_MACH_MX31HSV1
}

EXPORT_SYMBOL(gpio_keypad_active);

/*!
 * Setup GPIO for Keypad to be inactive
 *
 * \todo check requests
 * \warning why gpio mode ?
 */
void gpio_keypad_inactive(void)
{
   mxc_request_iomux(MX31_PIN_KEY_COL0, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);
   mxc_request_iomux(MX31_PIN_KEY_COL1, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);
   mxc_request_iomux(MX31_PIN_KEY_COL2, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);

   /*! \TODO Check if this is needed. Schematic shows this is connected to GND */
   /*   mxc_request_iomux(MX31_PIN_KEY_COL3, OUTPUTCONFIG_GPIO,
        INPUTCONFIG_GPIO);
        */
   mxc_request_iomux(MX31_PIN_KEY_ROW0, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);
   mxc_request_iomux(MX31_PIN_KEY_ROW1, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);

   if ( get_board_type() < BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      /*CLD: FIXME it was like this on HSV1 and HSV2 but it seems to be wrong*/
      mxc_request_iomux(MX31_PIN_KEY_COL4, OUTPUTCONFIG_GPIO,
            INPUTCONFIG_GPIO);
      mxc_request_iomux(MX31_PIN_KEY_COL5, OUTPUTCONFIG_GPIO,
            INPUTCONFIG_GPIO);
      mxc_request_iomux(MX31_PIN_KEY_COL6, OUTPUTCONFIG_GPIO,
            INPUTCONFIG_GPIO);
      mxc_request_iomux(MX31_PIN_KEY_COL7, OUTPUTCONFIG_GPIO,
            INPUTCONFIG_GPIO);
      mxc_request_iomux(MX31_PIN_KEY_ROW2, OUTPUTCONFIG_GPIO,
            INPUTCONFIG_GPIO);
      mxc_request_iomux(MX31_PIN_KEY_ROW3, OUTPUTCONFIG_GPIO,
            INPUTCONFIG_GPIO);
      mxc_request_iomux(MX31_PIN_KEY_ROW4, OUTPUTCONFIG_GPIO,
            INPUTCONFIG_GPIO);
   }
#if !defined(CONFIG_MACH_MX31HSV1) 
   /* CLD: ROW5 and ROW7 are used as Board identifier, ROW6 is kept reserved as well */   
   mxc_request_iomux(MX31_PIN_KEY_ROW5, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);
   mxc_request_iomux(MX31_PIN_KEY_ROW6, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);
   mxc_request_iomux(MX31_PIN_KEY_ROW7, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);
#endif // CONFIG_MACH_MX31HSV1
}

EXPORT_SYMBOL(gpio_keypad_inactive);

#endif
/*!
 * Setup GPIO for a CSPI device to be active
 *
 * @param  cspi_mod         an CSPI device
 * \todo check requests
 */
int gpio_spi_active(int cspi_mod)
{
   switch (cspi_mod) {
      case 0:
         /* SPI1 - Used for PMIC, Line \SS0 as chip select, interrupt is MX31_PIN_CSPI1_SPI_RDY */
			if(mxc_request_iomux(MX31_PIN_CSPI1_MISO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
					mxc_request_iomux(MX31_PIN_CSPI1_MOSI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
					mxc_request_iomux(MX31_PIN_CSPI1_SCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
					/*mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC) ||*/
					mxc_request_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)){
				printk("%s %d: Could not init cspi\n", __func__, __LINE__);
				return -EIO;
			}

         // mxc_request_iomux(MX31_PIN_CSPI1_SS1, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC);
         // mxc_request_iomux(MX31_PIN_CSPI1_SS2, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC);
         break;
      case 1:
         /* SPI2  - Used since Homescreen V3 to converse with PMIC (aka ATLAS)*/
			if(mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC) ||
					mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC) ||
					mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC) ||
					mxc_request_iomux(MX31_PIN_CSPI2_SPI_RDY, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC) ||
					mxc_request_iomux(MX31_PIN_CSPI2_SS0, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC)){
				printk("%s %d: Could not init cspi\n", __func__, __LINE__);
				return -EIO;
			}

         //mxc_request_iomux(MX31_PIN_CSPI2_SS1, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC);
         //mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_FUNC,INPUTCONFIG_FUNC);
         break;
      case 2:
         /* SPI3 - Unused on Homescreen */
			if(mxc_request_iomux(MX31_PIN_CSPI3_MISO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
			mxc_request_iomux(MX31_PIN_CSPI3_MOSI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
			mxc_request_iomux(MX31_PIN_CSPI3_SCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
			mxc_request_iomux(MX31_PIN_CSPI3_SPI_RDY, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)){
				printk("%s %d: Could not init cspi\n", __func__, __LINE__);
				return -EIO;
			}
	

#if !defined(CONFIG_MACH_MX31HSV1)
         /* SPI3 steals its SSn pins to other functions, this this is board-dependant
          * and must be set for each one with Alternate Function XX.
          */
			if(mxc_request_iomux(MX31_PIN_CSPI2_SS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
			mxc_request_iomux(MX31_PIN_CSPI2_SS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
			mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)){
				printk("%s %d: Could not init cspi\n", __func__, __LINE__);
				return -EIO;
			}
#endif // CONFIG_MACH_MX31HSV1
         break;
      default:
         break;
   }
	return 0;
}

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_active(void)
{
   /*
    * Configure the IOMUX control register for 1-wire signals.
    */
   iomux_config_mux(MX31_PIN_BATT_LINE, OUTPUTCONFIG_FUNC,
         INPUTCONFIG_FUNC);
   iomux_config_pad(MX31_PIN_BATT_LINE, PAD_CTL_LOOPBACK);
}

/*!
 * Setup 1-Wire to be active
 */
void gpio_owire_inactive(void)
{
   /*
    * Configure the IOMUX control register for 1-wire signals.
    */
   iomux_config_mux(MX31_PIN_BATT_LINE, OUTPUTCONFIG_GPIO,
         INPUTCONFIG_GPIO);
}

EXPORT_SYMBOL(gpio_owire_active);
EXPORT_SYMBOL(gpio_owire_inactive);

/*!
 * Setup GPIO for a CSPI device to be inactive
 *
 * @param  cspi_mod         a CSPI device
 */
void gpio_spi_inactive(int cspi_mod)
{
   /* Do nothing as CSPI pins doesn't have/support GPIO mode */
}

/*!
 * Setup GPIO for an I2C device to be active
 *
 * @param  i2c_num         an I2C device
 * \todo check requests
 */
void gpio_i2c_active(int i2c_num)
{
   switch (i2c_num) {
      case 0:
         mxc_request_iomux(MX31_PIN_I2C_CLK, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_I2C_DAT, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         break;
      case 1:
         mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         break;
      case 2:
         mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_ALT1,
               INPUTCONFIG_ALT1);
         break;
      default:
         break;
   }

}

/*!
 * Setup GPIO for an I2C device to be inactive
 *
 * @param  i2c_num         an I2C device
 */
void gpio_i2c_inactive(int i2c_num)
{
   switch (i2c_num) {
      case 0:
         mxc_request_iomux(MX31_PIN_I2C_CLK, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_I2C_DAT, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_FUNC);
         break;
      case 1:
         mxc_request_iomux(MX31_PIN_CSPI2_MOSI, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_CSPI2_MISO, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_ALT1);
         break;
      case 2:
         mxc_request_iomux(MX31_PIN_CSPI2_SS2, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_ALT1);
         mxc_request_iomux(MX31_PIN_CSPI2_SCLK, OUTPUTCONFIG_GPIO,
               INPUTCONFIG_ALT1);
         break;
      default:
         break;
   }
}

/*!
 * This function configures the IOMux block for PMIC standard operations.
 *
 */
void gpio_pmic_active(void)
{
   /* Homescreen V1: No GPIO in use for PMIC */
   //mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY, OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);
   //mxc_set_gpio_direction(MX31_PIN_CSPI1_SPI_RDY, 1);
   //mxc_set_gpio_edge_ctrl(MX31_PIN_CSPI1_SPI_RDY, GPIO_INT_RISE_EDGE);
}

EXPORT_SYMBOL(gpio_pmic_active);

/*!
 * This function activates DAM ports 4 & 5 to enable
 * audio I/O. Thsi function is called from mx31ads_gpio_init
 * function, which is board-specific.
 */
void gpio_activate_audio_ports(void)
{
   static int _initialized=0;

   if (! _initialized)
   {
      /* config Audio ports (4 & 5) */
      mxc_request_iomux(MX31_PIN_SCK4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_SRXD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_STXD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_SFS4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_SCK5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_SRXD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_STXD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
      mxc_request_iomux(MX31_PIN_SFS5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
   }
   _initialized++;
   printk(KERN_INFO "%s : called %d times\n",__FUNCTION__,_initialized);
}

/*!
 * Setup GPIO for SDHC to be active
 *
 * @param module SDHC module number
 */
int gpio_sdhc_active(int module)
{
   switch (module) {
      case 0:
         mxc_request_iomux(MX31_PIN_SD1_CLK, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_SD1_CMD, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_SD1_DATA0, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_SD1_DATA1, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_SD1_DATA2, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);
         mxc_request_iomux(MX31_PIN_SD1_DATA3, OUTPUTCONFIG_FUNC,
               INPUTCONFIG_FUNC);

#define SDHC_PADS (PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST | PAD_CTL_100K_PU | PAD_CTL_HYS_CMOS )
         mxc_iomux_set_pad(MX31_PIN_SD1_CLK,  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);
         mxc_iomux_set_pad(MX31_PIN_SD1_CMD,  SDHC_PADS);
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA0,SDHC_PADS);
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA1,SDHC_PADS);
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA2,SDHC_PADS);
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA3,SDHC_PADS);
#undef SDHC_PADS
         break;
      case 1:
         /*****************************************************************************
          *                              SD / MMC CARD
          * WORKAROUND level shifting : Initialize pin (CMD, DATA0, DATA1, DATA2, DATA3)    
          * to high level in order to detected a new inserted SD/MMC card.
          * Solution : Drive GPIO MCU2_14 
          *
           *****************************************************************************/ 
         gpio_serviceled_active();
         gpio_serviceled_pulse();

         mxc_request_iomux (HOMESCREEN_V3_SD_CLK, OUTPUTCONFIG_ALT1,INPUTCONFIG_ALT1); 
         mxc_request_iomux (HOMESCREEN_V3_SD_CMD, OUTPUTCONFIG_ALT1,INPUTCONFIG_ALT1);   
         mxc_request_iomux (HOMESCREEN_V3_SD_DATA3, OUTPUTCONFIG_ALT1,INPUTCONFIG_ALT1);
         mxc_request_iomux (HOMESCREEN_V3_SD_DATA2, OUTPUTCONFIG_ALT1,INPUTCONFIG_ALT1);
         mxc_request_iomux (HOMESCREEN_V3_SD_DATA1, OUTPUTCONFIG_ALT1,INPUTCONFIG_ALT1);
         mxc_request_iomux (HOMESCREEN_V3_SD_DATA0, OUTPUTCONFIG_ALT1,INPUTCONFIG_ALT1);

         mxc_iomux_set_pad (HOMESCREEN_V3_SD_CLK,
               (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST | PAD_CTL_PKE_ENABLE |
                PAD_CTL_PUE_PUD | PAD_CTL_100K_PU | 
                PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_LOOPBACK
               ));
         mxc_iomux_set_pad (HOMESCREEN_V3_SD_CMD,
               (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST | PAD_CTL_PKE_ENABLE |
                PAD_CTL_PUE_PUD | PAD_CTL_100K_PU |
                PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_LOOPBACK
               ));
         mxc_iomux_set_pad (HOMESCREEN_V3_SD_DATA3,
               (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST | PAD_CTL_PKE_ENABLE |
                PAD_CTL_PUE_PUD | PAD_CTL_100K_PU |
                PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_LOOPBACK
               ));
         mxc_iomux_set_pad (HOMESCREEN_V3_SD_DATA2,
               (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST | PAD_CTL_PKE_ENABLE |
                PAD_CTL_PUE_PUD | PAD_CTL_100K_PU |
                PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_LOOPBACK
               ));
         mxc_iomux_set_pad (HOMESCREEN_V3_SD_DATA1,
               (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST | PAD_CTL_PKE_ENABLE |
                PAD_CTL_PUE_PUD | PAD_CTL_100K_PU |
                PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_LOOPBACK
               ));
         mxc_iomux_set_pad (HOMESCREEN_V3_SD_DATA0,
               (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST | PAD_CTL_PKE_ENABLE |
                PAD_CTL_PUE_PUD | PAD_CTL_100K_PU |
                PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_LOOPBACK
               ));

         break;
      default:
         return -ENODEV;	break;
   }
   return 0;
}

EXPORT_SYMBOL(gpio_sdhc_active);

/*!
 * Setup GPIO for SDHC1 to be inactive
 *
 * @param module SDHC module number
 */
void gpio_sdhc_inactive(int module)
{
   switch (module) {
      case 0:
         mxc_request_iomux(MX31_PIN_SD1_CLK, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_SD1_CMD, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_SD1_DATA0, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_SD1_DATA1, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_SD1_DATA2, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_SD1_DATA3, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);

         mxc_iomux_set_pad(MX31_PIN_SD1_CLK,(PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
         mxc_iomux_set_pad(MX31_PIN_SD1_CMD,(PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA0,(PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA1,(PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA2,(PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
         mxc_iomux_set_pad(MX31_PIN_SD1_DATA3,(PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW));
         break;
      case 1:
         /* TODO:what are the pins for SDHC2? */
         mxc_request_iomux(MX31_PIN_PC_CD2_B, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_PC_CD1_B, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_PC_WAIT_B, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_PC_READY, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_PC_VS1, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         mxc_request_iomux(MX31_PIN_PC_PWRON, OUTPUTCONFIG_GPIO,INPUTCONFIG_NONE);
         gpio_serviceled_inactive();
         break;
      default:
         break;
   }
}

EXPORT_SYMBOL(gpio_sdhc_inactive);

/*
 * Probe for the card. If present the GPIO data would be set.
 */
int sdhc_get_card_det_status(struct device *dev)
{
   switch (to_platform_device(dev)->id)
   {
      case 0:
         return 0;break;
      case 1:
         return mxc_get_gpio_datain(MX31_PIN_SIMPD0);
         break;
      default:
         return -ENODEV;break;
   }
}
EXPORT_SYMBOL(sdhc_get_card_det_status);

/*
 * Return the card detect pin.
 */
int sdhc_init_card_det(int id)
{
   switch (id)
   {
      case 0:
         return 0;
         break;
      case 1:
         iomux_config_mux(MX31_PIN_SIMPD0, OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);
         return IOMUX_TO_IRQ(MX31_PIN_SIMPD0);
         break;
      default:
         return -ENODEV;	break;
   }
}
EXPORT_SYMBOL(sdhc_init_card_det);

/*
 * Probe the Write Protect slider. If protected the GPIO data would be set.
 */
int sdhc_get_card_write_protect_status(struct mmc_host *host)
{
   switch (to_platform_device(host->parent)->id)
   {
      case 0:
         return 0;break;
      case 1:
         return mxc_get_gpio_datain(MX31_PIN_SCLK0);
         break;
      default:
         return -ENODEV;break;
   }
}

EXPORT_SYMBOL(sdhc_get_card_write_protect_status);

/*
 * Initialise the write-protect detection pin
 */
int sdhc_init_card_write_protect(int id)
{
   switch (id)
   {
      case 0:
         return 0;
         break;
      case 1:
         return iomux_config_mux(MX31_PIN_SCLK0, OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);
         break;
      default:
         return -ENODEV;	break;
   }
}
EXPORT_SYMBOL(sdhc_init_card_write_protect);

/*!
 * Setup GPIO for LCD to be active
 *
 */

void gpio_lcd_active(void)
{
	static int gpio_already_requested = 0;

	if (!gpio_already_requested)
	{
		mxc_request_iomux(MX31_PIN_LD0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
		mxc_request_iomux(MX31_PIN_LD16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);		// LD16
		mxc_request_iomux(MX31_PIN_LD17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);		// LD17
		mxc_request_iomux(MX31_PIN_VSYNC3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// VSYNC
		mxc_request_iomux(MX31_PIN_HSYNC, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);		// HSYNC
		mxc_request_iomux(MX31_PIN_FPSHIFT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// CLK
		mxc_request_iomux(MX31_PIN_DRDY0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);		// DRDY
		mxc_request_iomux(MX31_PIN_D3_REV, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// REV
		mxc_request_iomux(MX31_PIN_CONTRAST, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// CONTR
		mxc_request_iomux(MX31_PIN_D3_SPL, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// SPL
		mxc_request_iomux(MX31_PIN_D3_CLS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);	// CLS

		iomux_config_mux(HOMESCREEN_V2_TFT_SHDN_GPIO, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		mxc_set_gpio_direction(HOMESCREEN_V2_TFT_SHDN_GPIO, GPIO_DIR_OUTPUT);

		if (get_board_type() < BOARD_TYPE_V3_HOMESCREEN_GENERIC)
		{
			iomux_config_mux(HOMESCREEN_V2_BKLT_ANALOG_EN_GPIO, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
			mxc_set_gpio_direction(HOMESCREEN_V2_BKLT_ANALOG_EN_GPIO, GPIO_DIR_OUTPUT);
			mxc_set_gpio_dataout(HOMESCREEN_V2_BKLT_ANALOG_EN_GPIO, 1 /* 0 = Digital Dimmer mode */ );
		}
		else if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
		{
			iomux_config_mux(HOMESCREEN_V4_TFT_EN_GPIO, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
			mxc_set_gpio_direction(HOMESCREEN_V4_TFT_EN_GPIO, GPIO_DIR_OUTPUT);
		}
		mdelay(500); /* Nasty hack to fix a bug preventing the Tabbee from booting */
	} // gpio_already_requested

	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
	{
		mxc_set_gpio_dataout(HOMESCREEN_V4_TFT_EN_GPIO, 1 /* 1 = ENABLE TFT */ );
		mdelay(50); /* 50 ms between TFT is switched on and buffers in Hi Z state */
		mxc_set_gpio_dataout(HOMESCREEN_V2_TFT_SHDN_GPIO, 1); /* Enable TFT screen */
	}
	else
		mxc_set_gpio_dataout(HOMESCREEN_V2_TFT_SHDN_GPIO, 0); /* Enable TFT screen */
	if (gpio_already_requested)
		mdelay(300); /* 300 ms before bklt PWM can be switched on again */
	gpio_already_requested = 1;
}

EXPORT_SYMBOL(gpio_lcd_active);

/*!
 * Setup GPIO for LCD to be inactive
 *
 */
void gpio_lcd_inactive(void)
{
	mdelay(2); /* 2 ms after bklt PWM is switched off */
	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
	{
		mxc_set_gpio_dataout(HOMESCREEN_V2_TFT_SHDN_GPIO, 0); /* disable TFT screen */
		mdelay(100); /* 100 ms between buffers in Hi Z state and TFT is switched off */
		mxc_set_gpio_dataout(HOMESCREEN_V4_TFT_EN_GPIO, 0 /* 0 = DISABLE TFT */ );
	}
	else
		mxc_set_gpio_dataout(HOMESCREEN_V2_TFT_SHDN_GPIO, 1); /* disable TFT screen */
}
EXPORT_SYMBOL(gpio_lcd_inactive);

#if defined(CONFIG_MACH_MX31HSV1)
/*!
 * Setup GPIO for LCD to be on/off
 *
 */
int gpio_lcd_state(uint8_t state)
{
   // mxc_set_gpio_dataout(MX31_PIN_STX0, state);
   return 0;
}

EXPORT_SYMBOL(gpio_lcd_state);
#endif


/*!
 * Setup GPIO for ATA interface
 *
 */
void gpio_ata_active(void)
{
}

EXPORT_SYMBOL(gpio_ata_active);

/*!
 * Restore ATA interface pins to reset values
 *
 */
void gpio_ata_inactive(void)
{
}

EXPORT_SYMBOL(gpio_ata_inactive);

/*!
 * Setup EDIO/IOMUX for external UART.
 *
 * @param port UART port
 * @param irq Interrupt line to allocate
 * @param handler Function to be called when the IRQ occurs
 * @param irq_flags Interrupt type flags
 * @param devname An ascii name for the claiming device
 * @param dev_id A cookie passed back to the handler function
 * @return  Returns 0 if the interrupt was successfully requested,
 *          otherwise returns an error code.
 */
int extuart_intr_setup(unsigned int port, unsigned int irq,
      irqreturn_t(*handler) (int, void *),
      unsigned long irq_flags, const char *devname,
      void *dev_id)
{
   return 0;
}

/*!
 * Get the EDIO interrupt, clear if set.
 *
 * @param port UART port
 */
void extuart_intr_clear(unsigned int port)
{
}

/*!
 * Do IOMUX configs required to put the
 * pin back in low power mode.
 *
 * @param port UART port
 * @param irq Interrupt line to free
 * @param dev_id Device identity to free
 * @return  Returns 0 if the interrupt was successfully freed,
 *          otherwise returns an error code.
 */
int extuart_intr_cleanup(unsigned int port, unsigned int irq, void *dev_id)
{
   return 0;
}

/* *INDENT-OFF* */
/*!
 * USB Host 1 activation
 * \warning pins conflict with SPI1, ATA, UART3
 */
int gpio_usbh1_active(void)
{
	static unsigned char gpio_already_requested = 0;

	if (!gpio_already_requested)
	{
		if (mxc_request_iomux(MX31_PIN_CSPI1_MOSI,	/* USBH1_RXDM */
			OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1) ||
		mxc_request_iomux(MX31_PIN_CSPI1_MISO,	/* USBH1_RXDP */
			OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1) ||
		mxc_request_iomux(MX31_PIN_CSPI1_SS0,	/* USBH1_TXDM */
			OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1) ||
		mxc_request_iomux(MX31_PIN_CSPI1_SS1,	/* USBH1_TXDP */
			OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1) ||
		mxc_request_iomux(MX31_PIN_CSPI1_SS2,	/* USBH1_RCV  */
			OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1) ||
		mxc_request_iomux(MX31_PIN_CSPI1_SCLK,	/* USBH1_OEB (_TXOE) */
			OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1) ||
		mxc_request_iomux(MX31_PIN_CSPI1_SPI_RDY,	/* USBH1_FS   */
			OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1))
			return -EINVAL;

		mxc_iomux_set_pad(MX31_PIN_CSPI1_MOSI,		/* USBH1_RXDM */
			(PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

		mxc_iomux_set_pad(MX31_PIN_CSPI1_MISO,		/* USBH1_RXDP */
			(PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

		mxc_iomux_set_pad(MX31_PIN_CSPI1_SS0,		/* USBH1_TXDM */
			(PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

		mxc_iomux_set_pad(MX31_PIN_CSPI1_SS1,		/* USBH1_TXDP */
			(PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

		mxc_iomux_set_pad(MX31_PIN_CSPI1_SS2,		/* USBH1_RCV  */
			(PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

		mxc_iomux_set_pad(MX31_PIN_CSPI1_SCLK,		/* USBH1_OEB (_TXOE) */
			(PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

		mxc_iomux_set_pad(MX31_PIN_CSPI1_SPI_RDY,	/* USBH1_FS   */
			(PAD_CTL_DRV_MAX | PAD_CTL_SRE_FAST));

		gpio_already_requested = 1;
	}

	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
		/* SFS6 pin is used as GPIO */
		mxc_iomux_set_gpr(MUX_PGP_USB_SUSPEND, false);
	}
	else
		mxc_iomux_set_gpr(MUX_PGP_USB_SUSPEND, true);

	return 0;
}

EXPORT_SYMBOL(gpio_usbh1_active);

/*!
 * USB Host 1 deactivation
 */
void gpio_usbh1_inactive(void)
{
	return;
}

EXPORT_SYMBOL(gpio_usbh1_inactive);

#define	H2_PAD_CFG	PAD_CTL_LOOPBACK | \
   PAD_CTL_PKE_ENABLE | \
PAD_CTL_PUE_PUD | \
PAD_CTL_100K_PU | \
PAD_CTL_HYS_CMOS | \
PAD_CTL_ODE_OpenDrain | \
PAD_CTL_DRV_HIGH /* PAD_CTL_DRV_MAX */ |\
PAD_CTL_SRE_FAST /* PAD_CTL_SRE_SLOW */

#define	H2_PAD_CFG_CMOS	PAD_CTL_LOOPBACK | \
   PAD_CTL_PKE_ENABLE | \
PAD_CTL_PUE_PUD | \
PAD_CTL_100K_PU | \
PAD_CTL_HYS_CMOS | \
PAD_CTL_ODE_CMOS | \
PAD_CTL_DRV_HIGH /* PAD_CTL_DRV_MAX */ |\
PAD_CTL_SRE_SLOW /* PAD_CTL_SRE_FAST */


#define OTG_PAD_CFG H2_PAD_CFG
#define OTG_PAD_CFG_CMOS H2_PAD_CFG_CMOS

static int _usb_bypass = 0;
static int _usb_host_activated = 0;
static int _usb_otg_activated = 0;

int gpio_usbh2_ulpi_reset(int active)
{
   if ( ! _usb_host_activated )
      return -EBUSY ;

   if (active) {
      /* activate the phy : release the reset */
      mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_USBHOST_RST_N, 0);
      mdelay(1);
   } 
   else {
      if ( _usb_bypass & 1 ) {
         return -EBUSY;
      }
      else {
         /* get out of reset state */
         mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_USBHOST_RST_N, 1);
         mdelay(3); // 2,3 ms mandatory
      }
   }
   return 0;
}
EXPORT_SYMBOL(gpio_usbh2_ulpi_reset);

int gpio_usbotg_ulpi_reset(int active)
{
   unsigned int reset_pin_otg;

   if ( ! _usb_otg_activated )
      return -EBUSY ;

   if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      reset_pin_otg = HOMESCREEN_V3_GPIO_USBOTG_RST_N;
   }
   else {
      reset_pin_otg = HOMESCREEN_V1_GPIO_USBOTG_RST_N;
   }

   if (active) {
      /* activate the phy : release the reset */
      mxc_set_gpio_dataout(reset_pin_otg, 0);
      mdelay(1);
   } 
   else {
      if ( _usb_bypass & 2 ) {
         return -EBUSY;
      }
      else {
         /* get out of reset state */
         mxc_set_gpio_dataout(reset_pin_otg, 1);
         mdelay(3); // 2,3 ms mandatory
      }
   }
   return 0;
}
EXPORT_SYMBOL(gpio_usbotg_ulpi_reset);


void gpio_usb_set_bypass(int bypass)
{
   _usb_bypass = bypass & 3;

   if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      /* bypass only from HSV3 */
      gpio_usbh2_ulpi_reset((bypass & 1) ? 1 : 0);
      gpio_usbotg_ulpi_reset((bypass & 2) ? 1 : 0);
   }
}
EXPORT_SYMBOL(gpio_usb_set_bypass);

int gpio_usb_get_bypass(void)
{
   return _usb_bypass ;
}
EXPORT_SYMBOL(gpio_usb_get_bypass);

/*!
 * USB Host ULPI active
 */
int gpio_usb_host_ulpi_active(void)
{
	static unsigned char gpio_already_requested = 0;

	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
	{
		if (!gpio_already_requested)
		{
			mxc_request_iomux(HOMESCREEN_V3_GPIO_USBHOST_STBY_N, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
			mxc_iomux_set_pad(HOMESCREEN_V3_GPIO_USBHOST_STBY_N, H2_PAD_CFG_CMOS);
			/* configure as output */
			mxc_set_gpio_direction(HOMESCREEN_V3_GPIO_USBHOST_STBY_N, GPIO_DIR_OUTPUT);
			gpio_already_requested = 1;
		}

		mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_USBHOST_STBY_N, 1);
	}

	return 0;
}

EXPORT_SYMBOL(gpio_usb_host_ulpi_active);

/*!
 * USB Host ULPI inactive
 */
int gpio_usb_host_ulpi_inactive(void)
{
	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
		mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_USBHOST_STBY_N, 0);

	return 0;
}

EXPORT_SYMBOL(gpio_usb_host_ulpi_inactive);

/*!
 * USB OTG ULPI active
 */
int gpio_usb_otg_ulpi_active(void)
{
	static unsigned char gpio_already_requested = 0;

	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
	{
		if (!gpio_already_requested)
		{
			mxc_request_iomux(HOMESCREEN_V3_GPIO_USBOTG_STBY_N, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
			mxc_iomux_set_pad(HOMESCREEN_V3_GPIO_USBOTG_STBY_N, OTG_PAD_CFG_CMOS);
			/* configure as output */
			mxc_set_gpio_direction(HOMESCREEN_V3_GPIO_USBOTG_STBY_N, GPIO_DIR_OUTPUT);
			gpio_already_requested = 1;
		}

		mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_USBOTG_STBY_N,  1 );
	}

	return 0;
}

EXPORT_SYMBOL(gpio_usb_otg_ulpi_active);

/*!
 * USB OTG ULPI inactive
 */
int gpio_usb_otg_ulpi_inactive(void)
{
	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC)
		mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_USBOTG_STBY_N, 0);

	return 0;
}

EXPORT_SYMBOL(gpio_usb_otg_ulpi_inactive);

/*!
 * USB Host 2 activation
 * \warning pin conflicts with UART5, PCMCIA in primary functions, NAND on ALT3
 */
int gpio_usbh2_active(void)
{
	static unsigned char gpio_already_requested = 0;

	if (!gpio_already_requested)
	{
		if (mxc_request_iomux(MX31_PIN_USBH2_CLK,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)	||
		mxc_request_iomux(MX31_PIN_USBH2_DIR,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)	||
		mxc_request_iomux(MX31_PIN_USBH2_NXT,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)	||
		mxc_request_iomux(MX31_PIN_USBH2_STP,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)	||
		mxc_request_iomux(MX31_PIN_USBH2_DATA0,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)	||
		mxc_request_iomux(MX31_PIN_USBH2_DATA1,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)	||
		mxc_request_iomux(MX31_PIN_STXD3,OUTPUTCONFIG_ALT2, OUTPUTCONFIG_ALT2)	||		/* USBH2_DATA2 */
		mxc_request_iomux(MX31_PIN_SRXD3,OUTPUTCONFIG_ALT2, OUTPUTCONFIG_ALT2)	||		/* USBH2_DATA3 */
		mxc_request_iomux(MX31_PIN_SCK3,OUTPUTCONFIG_ALT2, OUTPUTCONFIG_ALT2)	||		/* USBH2_DATA4 */
		mxc_request_iomux(MX31_PIN_SFS3,OUTPUTCONFIG_ALT2, OUTPUTCONFIG_ALT2)	||		/* USBH2_DATA5 */
		mxc_request_iomux(MX31_PIN_STXD6,OUTPUTCONFIG_ALT2, OUTPUTCONFIG_ALT2)	||		/* USBH2_DATA6 */
		mxc_request_iomux(MX31_PIN_SRXD6,OUTPUTCONFIG_ALT2, OUTPUTCONFIG_ALT2))			/* USBH2_DATA7 */
			return -EINVAL;

		mxc_iomux_set_pad(MX31_PIN_USBH2_CLK, H2_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBH2_DIR, H2_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBH2_NXT, H2_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBH2_STP, H2_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBH2_DATA0, H2_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBH2_DATA1, H2_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_STXD3, H2_PAD_CFG);	/* USBH2_DATA2 */
		mxc_iomux_set_pad(MX31_PIN_SRXD3, H2_PAD_CFG);	/* USBH2_DATA3 */
		mxc_iomux_set_pad(MX31_PIN_SCK3, H2_PAD_CFG);	/* USBH2_DATA4 */
		mxc_iomux_set_pad(MX31_PIN_SFS3, H2_PAD_CFG);	/* USBH2_DATA5 */
		mxc_iomux_set_pad(MX31_PIN_STXD6, H2_PAD_CFG);	/* USBH2_DATA6 */
		mxc_iomux_set_pad(MX31_PIN_SRXD6, H2_PAD_CFG);	/* USBH2_DATA7 */
#ifdef CONFIG_MACH_MX31HSV1
		/* GPIO of the \reset line of tranceiver */
		mxc_request_iomux(HOMESCREEN_V1_GPIO_USBHOST_RST_N, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		mxc_iomux_set_pad(HOMESCREEN_V1_GPIO_USBHOST_RST_N, H2_PAD_CFG_CMOS);
		/* configure GPIO MX31_PIN_GPIO3_0 as output */
		mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_USBHOST_RST_N, GPIO_DIR_OUTPUT);
		gpio_already_requested = 1;
	}
	_usb_host_activated = 1;
	/* phy activate: reset release */
	gpio_usbh2_ulpi_reset(1);
	gpio_usbh2_ulpi_reset(0);
#else
	}
#endif /* CONFIG_MACH_MX31HSV1 */

	mxc_iomux_set_gpr(MUX_PGP_UH2, true);

	return 0;
}

EXPORT_SYMBOL(gpio_usbh2_active);

void gpio_usbh2_inactive(void)
{
	iomux_config_gpr(MUX_PGP_UH2, false);

	/* configure GPIO HOMESCREEN_V1_GPIO_USBHOST_RST_N at down by default (PHY in reset) */
	gpio_usbh2_ulpi_reset(1);
	_usb_host_activated = 0;
}
EXPORT_SYMBOL(gpio_usbh2_inactive);

/*
 * USB OTG HS port
 */
int gpio_usbotg_hs_active(void)
{
	static unsigned char gpio_already_requested = 0;
	unsigned int reset_pin_otg;

	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC )
		reset_pin_otg = HOMESCREEN_V3_GPIO_USBOTG_RST_N;
	else
		reset_pin_otg = HOMESCREEN_V1_GPIO_USBOTG_RST_N;

	if (!gpio_already_requested)
	{
		if (mxc_request_iomux(MX31_PIN_USBOTG_DATA0,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DATA1,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DATA2,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DATA3,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DATA4,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DATA5,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DATA6,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DATA7,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_CLK,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_DIR,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_NXT,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC) ||
		mxc_request_iomux(MX31_PIN_USBOTG_STP,OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC))
			return -EINVAL;

		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA0, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA1, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA2, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA3, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA4, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA5, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA6, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DATA7, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_CLK, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_DIR, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_NXT, OTG_PAD_CFG);
		mxc_iomux_set_pad(MX31_PIN_USBOTG_STP, OTG_PAD_CFG);

		/* enable GPIO mode of the \reset line of tranceiver */
		mxc_request_iomux(reset_pin_otg, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);

		mxc_iomux_set_pad(reset_pin_otg, OTG_PAD_CFG_CMOS);

		/* configure GPIO SVEN0 as output */
		mxc_set_gpio_direction(reset_pin_otg, GPIO_DIR_OUTPUT);
	}
	_usb_otg_activated = 1;
	gpio_already_requested = 1;

	/* phy activate : reset release */
	gpio_usbotg_ulpi_reset(1);
	gpio_usbotg_ulpi_reset(0);

	return 0;
}

EXPORT_SYMBOL(gpio_usbotg_hs_active);

void gpio_usbotg_hs_inactive(void)
{
	unsigned int reset_pin_otg;

	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC) 
		reset_pin_otg = HOMESCREEN_V3_GPIO_USBOTG_RST_N;
	else
		reset_pin_otg = HOMESCREEN_V1_GPIO_USBOTG_RST_N;

	/* configure reset at down by default (PHY in reset) */
	gpio_usbotg_ulpi_reset(1);
	_usb_otg_activated = 0;
}

EXPORT_SYMBOL(gpio_usbotg_hs_inactive);

/*!
 * Setup GPIO for PCMCIA interface
 *
 */
void gpio_pcmcia_active(void)
{
}

EXPORT_SYMBOL(gpio_pcmcia_active);

/*!
 * Setup GPIO for pcmcia to be inactive
 */
void gpio_pcmcia_inactive(void)
{
}

EXPORT_SYMBOL(gpio_pcmcia_inactive);
/*!
 * Setup IR to be used by UART and FIRI
 */
void gpio_firi_init(void)
{
   //gpio_uart_active(1, 0);
}

EXPORT_SYMBOL(gpio_firi_init);

/*!
 * Setup IR to be used by UART
 */
void gpio_firi_inactive(void)
{

}

EXPORT_SYMBOL(gpio_firi_inactive);

/*!
 * Setup IR to be used by FIRI
 */
void gpio_firi_active(void *fir_cong_reg_base, unsigned int tpp_mask)
{

}
EXPORT_SYMBOL(gpio_firi_active);

void gpio_power_off(void)
{
   PMIC_STATUS pmic_res;
   int value;

   /* PMIC interrupt cause wake up of processor : \FIXME this should be more  
    * fine-tuned to allow Touchscreen interrupt or Alarm to wake up only.
    */

   /* mask all PMIC events */
   pmic_res = pmic_write_reg(REG_INTERRUPT_MASK_0, 0xFFFFFF, 0xFFFFFF);
   pmic_res = pmic_write_reg(REG_INTERRUPT_MASK_1, 0xFFFFFF, 0xFFFFFF);

   /* clear all PMIC pending events */
   value = mxc_get_gpio_datain(HOMESCREEN_V2_GPIO_PMIC_IRQ);

   if ( value ) {
      printk(KERN_EMERG "System won't be able to halt by itself; user action needed.");
   }
 
	if ( get_board_type() < BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
		/* Not for board from V3 */
		/* switch off bucks to avoid a restart */
		pmic_res = pmic_power_switcher_set_mode(SW_SW2A,SYNC_RECT,false);
		pmic_res = pmic_power_switcher_set_mode(SW_SW2B,SYNC_RECT,false);
	}
   
	/* cut processor's power supply*/
   mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_SOFT_STOP,0);
}

EXPORT_SYMBOL(gpio_power_off);

/*!
 * This function activates the battery charger I/Os.
 * This function is called from batt_ch_init
 * function in batt_ch.c.
 */
void gpio_activate_battery_charger(void)
{
   static int _initialized = 0;

   if (! _initialized) {
      if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
         mxc_request_iomux(HOMESCREEN_V3_GPIO_CHG_SHDN, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
         mxc_set_gpio_direction(HOMESCREEN_V3_GPIO_CHG_SHDN, GPIO_DIR_OUTPUT);
         mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_CHG_SHDN, 1);
      } else {
         /* config CHG_SHDN
          * GPIO1_3: CHG_SHDN, (=0 to enable charger shutdown)
          */
         mxc_request_iomux(HOMESCREEN_V1_GPIO_CHG_SHDN, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
         mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_CHG_SHDN, GPIO_DIR_OUTPUT);
         mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_CHG_SHDN, 1);
      }
   } else {
      printk(KERN_INFO "%s: called %d times\n", __FUNCTION__, _initialized);
   }

   _initialized++;
}

EXPORT_SYMBOL(gpio_activate_battery_charger);

/*!
 * This function enables I2C E2PROM writes.
 * This function is called from factory_conf.c
 */
void homescreen_v1_eeprom_write_unprotect(void)
{
   mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_EEPROM_WP, 0);
}

EXPORT_SYMBOL(homescreen_v1_eeprom_write_unprotect);

/*!
 * This function disables all I2C EEprom writes.
 * This function is called from factory_conf.c
 */
void homescreen_v1_eeprom_write_protect(void)
{
   mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_EEPROM_WP, 1);
}

EXPORT_SYMBOL(homescreen_v1_eeprom_write_protect);


/*!
 * Grab gpio for service LED.
 */
void gpio_serviceled_active(void)
{
   if ( get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC  ) {
      mxc_request_iomux(HOMESCREEN_V3_GPIO_LED_SERVICE, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);         
      mxc_set_gpio_direction(HOMESCREEN_V3_GPIO_LED_SERVICE, GPIO_DIR_OUTPUT);
      mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_LED_SERVICE, 0); /* switch off */

   } 
}

/*!
 * Release gpio for service LED.
 */
void gpio_serviceled_inactive(void)
{
   if ( get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      /* it is *sometimes* a good idea to fall back to input (\TODO revalidate hardware) */
      mxc_set_gpio_direction(HOMESCREEN_V3_GPIO_LED_SERVICE, GPIO_DIR_INPUT);
      mxc_free_iomux(HOMESCREEN_V3_GPIO_LED_SERVICE, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);         
   } 
}

/*! 
 * Get LED value
 */
int gpio_serviceled_get (void)
{
   if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      return mxc_get_gpio_datain (HOMESCREEN_V3_GPIO_LED_SERVICE);
   }

   return -EPERM;
}

void gpio_serviceled_pulse (void)
{
   if ( get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
         gpio_serviceled_set (1);
         mdelay (1);
         gpio_serviceled_set (0);
         mdelay (1);
   }
}

/*! 
 * Set LED value
 */
void gpio_serviceled_set (int value)
{
   if ( get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_LED_SERVICE, value & 1);
   }
}

/*!
* Setup GPIO for Dect USB Device
*
*/
void gpio_dect_usb_active(void)
{
#define ETHRESET_PAD_CFG (PAD_CTL_DRV_NORMAL| PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_100K_PU)
   if (!gpio_dect_usb_requested)
   {
	mxc_request_iomux(HOMESCREEN_GPIO_DECT_USB_RESET, OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE);
  	mxc_iomux_set_pad(HOMESCREEN_GPIO_DECT_USB_RESET, ETHRESET_PAD_CFG);
	mxc_set_gpio_direction(HOMESCREEN_GPIO_DECT_USB_RESET, GPIO_DIR_OUTPUT);
	mxc_set_gpio_dataout(HOMESCREEN_GPIO_DECT_USB_RESET, 1);
   	gpio_dect_usb_requested = 1;
    printk ("MCU_1_27 set as output logic 1"); 
  }
   else
   {
	printk("Pin DECT USB (MCU1_27) already requested\n");
   }
}

EXPORT_SYMBOL(gpio_dect_usb_active);
EXPORT_SYMBOL(gpio_serviceled_active);
EXPORT_SYMBOL(gpio_serviceled_inactive);
EXPORT_SYMBOL(gpio_serviceled_set);
EXPORT_SYMBOL(gpio_serviceled_get);
EXPORT_SYMBOL(gpio_serviceled_pulse);
