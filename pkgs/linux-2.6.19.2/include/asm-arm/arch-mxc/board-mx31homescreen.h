/*
 * Copyright 2005-2007 Sagem Communications. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ASM_ARCH_MXC_BOARD_MX31HOMESCREEN_H__
#define __ASM_ARCH_MXC_BOARD_MX31HOMESCREEN_H__

/*!
 * @defgroup BRDCFG_MX31 Board Configuration Options
 * @ingroup MSL_MX31
 */

/*!
 * @file arch-mxc/board-mx31homescreen.h
 *
 * @brief This file contains all the board level configuration options.
 *
 * It currently hold the options defined for MX31 ADS Platform.
 *
 * @ingroup BRDCFG_MX31
 */

/*
 * Include Files
 */
#include <asm/arch/board.h>

/* Start of physical RAM */
#define PHYS_OFFSET	        UL(0x80000000)

/* Size of contiguous memory for DMA and other h/w blocks */
#define CONSISTENT_DMA_SIZE	(SZ_16M) /* SAGEM: Use 16M instead of 8M for consistent DMA */

/*!
 * @name UART configurations
 */
/*! @{ */
/*!
 * Specify the max baudrate for the MXC UARTs for your board, do not specify a max
 * baudrate greater than 1500000. This is used while specifying the UART Power
 * management constraints.
 */
#define MAX_UART_BAUDRATE       1500000
/*!
 * Specifies if the Irda transmit path is inverting
 */
#define MXC_IRDA_TX_INV         0
/*!
 * Specifies if the Irda receive path is inverting
 */
#define MXC_IRDA_RX_INV         0
/* UART 1 configuration */
/*!
 * This define specifies if the UART port is configured to be in DTE or
 * DCE mode. There exists a define like this for each UART port. Valid
 * values that can be used are \b MODE_DTE or \b MODE_DCE.
 */
#define UART1_MODE              MODE_DCE
/*!
 * This define specifies if the UART is to be used for IRDA. There exists a
 * define like this for each UART port. Valid values that can be used are
 * \b IRDA or \b NO_IRDA.
 */
#define UART1_IR                NO_IRDA
/*!
 * This define is used to enable or disable a particular UART port. If
 * disabled, the UART will not be registered in the file system and the user
 * will not be able to access it. There exists a define like this for each UART
 * port. Specify a value of 1 to enable the UART and 0 to disable it.
 */
#define UART1_ENABLED           1

/* UART 2 configuration */
#define UART2_MODE              MODE_DCE
#define UART2_IR                IRDA

#define UART2_ENABLED           0

/* UART 3 configuration */
#define UART3_MODE              MODE_DTE
#define UART3_IR                NO_IRDA

#define UART3_ENABLED           0

/* UART 4 configuration */
#define UART4_MODE              MODE_DTE
#define UART4_IR                NO_IRDA
#define UART4_ENABLED           0	/* Disable UART 4 as its pins are shared with ATA */

/* UART 5 configuration */
#define UART5_MODE              MODE_DTE
#define UART5_IR                NO_IRDA
#define UART5_ENABLED           0

#define MXC_LL_UART_PADDR	UART1_BASE_ADDR
#define MXC_LL_UART_VADDR	AIPS1_IO_ADDRESS(UART1_BASE_ADDR)
/*! @} */


/*!
 * @name Memory Size parameters
 */
/*! @{ */
/*!
 * Size of SDRAM memory
 */
#define SDRAM_MEM_SIZE          SZ_128M
/*!
 * Size of MBX buffer memory
 */
#if !defined(CONFIG_MXC_MBX) || !defined(CONFIG_MXC_MBX_SIZE)
# define MXC_MBX_MEM_SIZE	0 /* SAGEM: HomeScreen doesn't use graphics accelerator */
#else
# if (CONFIG_MXC_MBX_SIZE == 4)
#  define MXC_MBX_MEM_SIZE	SZ_4M
# endif
# if (CONFIG_MXC_MBX_SIZE == 8)
#  define MXC_MBX_MEM_SIZE	SZ_8M
# endif
# if (CONFIG_MXC_MBX_SIZE == 16)
#  define MXC_MBX_MEM_SIZE	SZ_16M
# endif
# if (CONFIG_MXC_MBX_SIZE == 32)
#  define MXC_MBX_MEM_SIZE	SZ_32M
# endif
# if (CONFIG_MXC_MBX_SIZE == 64)
#  define MXC_MBX_MEM_SIZE	SZ_64M
# endif
#endif
/*!
 * Size of memory available to kernel
 */
#define MEM_SIZE                (SDRAM_MEM_SIZE - MXC_MBX_MEM_SIZE)
/*! @} */

/*!
 * @name Keypad Configurations FIXME
 */
/*! @{ */
/*!
 * Maximum number of rows (0 to 7)
 */
#define MAXROW                  8
/*!
 * Maximum number of columns (0 to 7)
 */
#define MAXCOL                  8
/*! @} */

/*!
 * @name  GPIO alias for Homescreens
 */
/*! @{*/

#define AHB_FREQ                133000000
#define IPG_FREQ                66500000

/*!
 * @name  GPIO alias for Homescreens
 */
/*! @{*/
#define HOMESCREEN_V1_GPIO_SOFT_STOP          MX31_PIN_STX0   /*!< for all */
#define HOMESCREEN_V1_GPIO_CRADDLE_DETECT     MX31_PIN_SVEN0  /*!< for all */
#define HOMESCREEN_V1_GPIO_ETHERNET_IRQ       MX31_PIN_RXD2   /*!< only for Homescren V1  */
#define HOMESCREEN_GPIO_DECT_USB_RESET        MX31_PIN_RXD2   /* Dect USB reset Pin */
#define HOMESCREEN_V1_GPIO_POWER_KEY          MX31_PIN_SCK6
#define HOMESCREEN_V1_GPIO_BOARD_ID1          MX31_PIN_KEY_ROW5 /*!< attention: negative logic */
#define HOMESCREEN_V1_GPIO_BOARD_ID2          MX31_PIN_KEY_ROW7 /*!< attention: negative logic */
#define HOMESCREEN_V1_GPIO_BOARD_ID3          MX31_PIN_USB_BYP /*!< attention: negative logic */
#define HOMESCREEN_V1_GPIO_EEPROM_WP          MX31_PIN_DSR_DTE1 /*!< for Homescreen V1 & V2 */
/* USB OTG */
#define HOMESCREEN_V1_GPIO_USBOTG_RST_N       MX31_PIN_GPIO1_5
#define HOMESCREEN_V1_GPIO_USBHOST_RST_N      MX31_PIN_GPIO1_6
#define HOMESCREEN_V1_GPIO_ETHERNET_RESET_N   MX31_PIN_SRST0   /*!< MCU3_3 */

/*
 * Changes since Homescreen V2
 */
#define HOMESCREEN_V2_GPIO_PMIC_IRQ           MX31_PIN_LCS1    /*!< only from Homescren V2 (and so for V3) */
#define HOMESCREEN_V2_GPIO_ETHERNET_IRQ       MX31_PIN_SER_RS  /*!< only from Homescren V2 */
#define HOMESCREEN_V2_BKLT_ANALOG_EN_GPIO     MX31_PIN_SD_D_I  /* MCU3_20 */
#define HOMESCREEN_V2_TFT_SHDN_GPIO           MX31_PIN_SD_D_IO /* MCU3_21 */

/*
 * Changes since Homescreen V3
 */
#define HOMESCREEN_V3_GPIO_LED_SERVICE        MX31_PIN_RI_DTE1 /*!< MCU2_14, only from Homescren V3 */
#define HOMESCREEN_V3_GPIO_ETHERNET_STBY_N    MX31_PIN_USB_PWR /*!< MCU1_29, only from Homescren V3 */
#define HOMESCREEN_V3_GPIO_ETHERNET_FIFO_SEL  MX31_PIN_CAPTURE /*!< MCU1_7, only from Homescren V3 */

/*
 * Changes since Homescreen V4
 */
#define HOMESCREEN_V4_TFT_EN_GPIO             MX31_PIN_SD_D_I /* MCU3_20 */

/* USB Host */
#define HOMESCREEN_V3_GPIO_USBOTG_STBY_N      MX31_PIN_GPIO1_3 /*!< only from V3 */
#define HOMESCREEN_V3_GPIO_USBHOST_STBY_N     MX31_PIN_GPIO1_4 /*!< only from V3 */
#define HOMESCREEN_V3_GPIO_USBOTG_RST_N       MX31_PIN_COMPARE /*!< MCU1_8, only from Homescren V3 */

#define HOMESCREEN_V3_GPIO_LED_SERVICE        MX31_PIN_RI_DTE1 /*!< MCU2_14, only from Homescren V3 */
#define HOMESCREEN_V3_GPIO_ETHERNET_STBY_N    MX31_PIN_USB_PWR /*!< MCU1_29, only from Homescren V3 */

#define HOMESCREEN_V1_GPIO_CHG_SHDN           MX31_PIN_GPIO1_3 /*!< MCU1_26 (only from Homescren V1&V2) : Charger Shutdown */
#define HOMESCREEN_V3_GPIO_CHG_SHDN           MX31_PIN_SFS6    /*!< MCU1_26 (only from Homescren V3) : Charger Shutdown */


/*SD/MMC Host  */
#define HOMESCREEN_V3_SD_CMD                 MX31_PIN_PC_CD1_B
#define HOMESCREEN_V3_SD_CLK                 MX31_PIN_PC_CD2_B
#define HOMESCREEN_V3_SD_DATA3               MX31_PIN_PC_PWRON
#define HOMESCREEN_V3_SD_DATA2               MX31_PIN_PC_VS1
#define HOMESCREEN_V3_SD_DATA1               MX31_PIN_PC_READY
#define HOMESCREEN_V3_SD_DATA0               MX31_PIN_PC_WAIT_B

/*! @}*/




/*!
 * @name  Defines Base address and IRQ used for SMSC Ethernet Controller
 */
/*! @{*/

/*! This is I/O Base address used to access registers of SMSC9215  */
#define SMSC9215_INT                IOMUX_TO_IRQ(HOMESCREEN_V1_GPIO_ETHERNET_IRQ)
#define SMSC9215_INT_V2             IOMUX_TO_IRQ(HOMESCREEN_V2_GPIO_ETHERNET_IRQ)
#define SMSC9215_BASE_ADDRESS       (CS1_BASE_ADDR)
#define SMSC9215_IOBASE_ADDRESS     (IO_CS1_BASE_ADDRESS)
/*! @}*/

/*!
 * @name  Sagem Board Identifiers
 */
/*! @{*/
#if !defined(__ASSEMBLER__)
enum {
   BOARD_DESIGNER_FREESCALE='F',
   BOARD_DESIGNER_SAGEM_VZY='V',
   BOARD_DESIGNER_SAGEM_OSN='O',
};
enum {
   BOARD_PRODUCT_ADS='A',
   BOARD_PRODUCT_LIVESCREEN='L',
   BOARD_HOMESCREEN='H',
   BOARD_MEDIAPHONE='M',
};

#define BOARD_MK_VER(d,product,major,minor) (unsigned int)( (((d)&0xff)<<24) \
      | (((product)&0xff)<<16) \
      | (((major)&0xff)<<8) \
      | (((minor)&0xff)<<0) )

typedef enum {
    BOARD_TYPE_UNKNOWN=0,
    BOARD_TYPE_ADS=BOARD_MK_VER(BOARD_DESIGNER_FREESCALE,BOARD_PRODUCT_ADS,0,0),
    BOARD_TYPE_V0_LIVESCREEN=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_VZY,BOARD_PRODUCT_LIVESCREEN,0,0),
    BOARD_TYPE_V1_HOMESCREEN_GENERIC=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,1,0),
    BOARD_TYPE_V2_HOMESCREEN_GENERIC=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,2,0),
    BOARD_TYPE_V2_HOMESCREEN_ETH=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,2,1),
    BOARD_TYPE_V2_HOMESCREEN_NOETH=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,2,2),
    BOARD_TYPE_V2_HOMESCREEN_ETHSWITCH=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,2,3),
	/* unused*/
    BOARD_TYPE_V3_HOMESCREEN_GENERIC=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,3,0),
    BOARD_TYPE_V4_HOMESCREEN_GENERIC=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,4,0),
    BOARD_TYPE_V5_HOMESCREEN_GENERIC=BOARD_MK_VER(BOARD_DESIGNER_SAGEM_OSN,BOARD_HOMESCREEN,5,0),
} board_type_t ;
extern board_type_t get_board_type(void);
#define M_BOARD_GET_MANUFACTURER()	((get_board_type() >> 24) & 0xff )
#define M_BOARD_GET_PRODUCT()			((get_board_type() >> 16) & 0xff )
#define M_BOARD_GET_MAJOR()			((get_board_type() >> 8) & 0xff )
#define M_BOARD_GET_MINOR()			((get_board_type()) & 0xff )

#define M_BOARD_IS_HOMESCREEN_V(x) 	((get_board_type() & 0xFFFFFF00) == (BOARD_TYPE_V##x##_HOMESCREEN_GENERIC & 0xFFFFFF00))
#define M_BOARD_IS_HOMESCREEN_V1() 	M_BOARD_IS_HOMESCREEN_V(1)
#define M_BOARD_IS_HOMESCREEN_V2() 	M_BOARD_IS_HOMESCREEN_V(2)
#define M_BOARD_IS_HOMESCREEN_V3() 	M_BOARD_IS_HOMESCREEN_V(3)
#define M_BOARD_IS_HOMESCREEN_V4() 	M_BOARD_IS_HOMESCREEN_V(4)
#define M_BOARD_IS_HOMESCREEN_V5() 	M_BOARD_IS_HOMESCREEN_V(5)
#endif
/*! @}*/



#endif   /* __ASM_ARCH_MXC_BOARD_MX31HOMESCREEN_H__ */

