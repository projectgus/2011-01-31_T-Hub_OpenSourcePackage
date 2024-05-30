/*
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright 2006-2007 Sagem Communications. All Rights Reserved.
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

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/serial_8250.h>
#ifdef CONFIG_KGDB_8250
#include <linux/kgdb.h>
#endif
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/clk.h>
#include <linux/spi/spi.h>
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/flash.h>
#endif

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/keypad.h>
#include <asm/arch/memory.h>
#include <asm/arch/gpio.h>

#include <asm/arch/mxc_spi.h>
#include <asm/arch/pmic_power.h>

#include "crm_regs.h"
#include "iomux.h"

/*!
 * @file mach-mx3/mx31ads.c
 *
 * @brief This file contains the board specific initialization routines.
 *
 * @ingroup MSL_MX31
 */

extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_cpu_init(void) __init;
extern void mx31ads_gpio_init(void) __init;
extern struct sys_timer mxc_timer;
extern void mxc_cpu_common_init(void);
extern int mxc_clocks_init(void);
extern void gpio_power_off(void);
extern void gpio_activate_audio_ports(void);
// extern void gpio_unmask_irq(u32);
// extern int gpio_set_irq_type(u32, u32);

static void mxc_nop_release(struct device *dev)
{
   /* Nothing */
}

board_type_t board_type = BOARD_TYPE_UNKNOWN;

/*!
 * Identify Sagem board using dedicated GPIOs.
 *
 * @return Id  
 * @see board_type_t
 */
static board_type_t board_identifier(void)
{
   unsigned char hw_version = 0;
   board_type = BOARD_TYPE_UNKNOWN;
#define BOARD_ID_PAD_CFG   (PAD_CTL_DRV_NORMAL | PAD_CTL_100K_PU | PAD_CTL_SRE_SLOW | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS)
   mxc_request_iomux(HOMESCREEN_V1_GPIO_BOARD_ID1, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
   mxc_iomux_set_pad(HOMESCREEN_V1_GPIO_BOARD_ID1, BOARD_ID_PAD_CFG);
   mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_BOARD_ID1, GPIO_DIR_INPUT);

   mxc_request_iomux(HOMESCREEN_V1_GPIO_BOARD_ID2, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
   mxc_iomux_set_pad(HOMESCREEN_V1_GPIO_BOARD_ID2, BOARD_ID_PAD_CFG);
   mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_BOARD_ID2, GPIO_DIR_INPUT);

   hw_version |= mxc_get_gpio_datain(HOMESCREEN_V1_GPIO_BOARD_ID1) ? 0 : 1 ;
   hw_version |= mxc_get_gpio_datain(HOMESCREEN_V1_GPIO_BOARD_ID2) ? 0 : 2 ;
   hw_version |= mxc_get_gpio_datain(HOMESCREEN_V1_GPIO_BOARD_ID3) ? 0 : 4 ;

   switch(hw_version)
   {
      case 0:
         board_type =  BOARD_TYPE_V1_HOMESCREEN_GENERIC;
         printk("HSV1\n");
		 if (!M_BOARD_IS_HOMESCREEN_V(1)) {
			BUG();
		 }
         break;
      case 1:
         board_type =  BOARD_TYPE_V2_HOMESCREEN_GENERIC;
         printk("HSV2\n");
		 if (!M_BOARD_IS_HOMESCREEN_V(2)) {
			BUG();
		 }
         break;         
      case 2:  
         board_type =  BOARD_TYPE_V3_HOMESCREEN_GENERIC;
         printk("HSV3\n");
		 if (!M_BOARD_IS_HOMESCREEN_V(3)) {
			BUG();
		 }
         break;               
      case 3:
         board_type =  BOARD_TYPE_V4_HOMESCREEN_GENERIC;
         printk("HSV4\n");
		 if (!M_BOARD_IS_HOMESCREEN_V(4)) {
			BUG();
		 }
         break;               
      case 4:
         board_type =  BOARD_TYPE_V5_HOMESCREEN_GENERIC;
         printk("HSV5\n");
		 if (!M_BOARD_IS_HOMESCREEN_V(5)) {
			BUG();
		 }
         break;               
      default:
         printk("Unidentified board  : 0x%x!\n", hw_version);
         break;
   }
   return board_type;
}


board_type_t get_board_type(void)
{
   return board_type;
}


#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)

/* Keypad keycodes for the EVB 8x8
 * keypad.  POWER and PTT keys don't generate
 * any interrupts via this driver so they are
 * not support. Change any keys as u like!
 */
static u16 keymapping_hsv3[7] = {
   /* This is for i.MX31 keypad, for reference */ 
      KEY_HOME, KEY_ADDRESSBOOK, KEY_PHONE, KEY_PLAY,
      KEY_MUTE, KEY_SOUND, KEY_POWER,
};

static u16 keymapping_hsv4[] = {
   /* This is for i.MX31 keypad, for reference */ 
      KEY_VOLUMEUP,     KEY_VOLUMEDOWN,
      KEY_POWER,        0
};

static u16 keymapping_hsv1[64] = {
   /* This is for Homescreen */
   KEY_SCROLLDOWN, KEY_PAGEDOWN, KEY_PAGEUP, KEY_SCROLLUP,
   KEY_ENTER, KEY_BACK, KEY_END, KEY_BACK,
   KEY_F1, KEY_SENDFILE, KEY_HOME, KEY_F6,
   KEY_VOLUMEUP, KEY_F8, KEY_F9, KEY_F10,
   KEY_POWER, KEY_2, KEY_1, KEY_4,
   KEY_VOLUMEDOWN, KEY_7, KEY_5, KEY_6,
   KEY_9, KEY_LEFTSHIFT, KEY_8, KEY_0,
   KEY_KPASTERISK, KEY_RECORD, KEY_Q, KEY_W,
   KEY_A, KEY_S, KEY_D, KEY_E,
   KEY_F, KEY_R, KEY_T, KEY_Y,
   KEY_TAB, KEY_F7, KEY_CAPSLOCK, KEY_Z,
   KEY_X, KEY_C, KEY_V, KEY_G,
   KEY_B, KEY_H, KEY_N, KEY_M,
   KEY_J, KEY_K, KEY_U, KEY_I,
   KEY_SPACE, KEY_F2, KEY_DOT, KEY_ENTER,
   KEY_L, KEY_BACKSPACE, KEY_P, KEY_O,
};
static struct resource mxc_kpp_resources[] = {
   [0] = {
      .start = INT_KPP,
      .end = INT_KPP,
      .flags = IORESOURCE_IRQ,
   }
};

static struct keypad_data evb_2_by_3_keypad = {
   .rowmax = 2,
   .colmax = 3,
   .irq = INT_KPP,
   .learning = 0,
   .delay = 2,
   .matrix = keymapping_hsv3,
};

static struct keypad_data hsv4_1x2_keypad = {
   .rowmax = 1,
   .colmax = 2,
   .irq = INT_KPP,
   .learning = 0,
   .delay = 2,
   .matrix = keymapping_hsv4,
};

static struct keypad_data evb_8_by_8_keypad = {
   .rowmax = 8,
   .colmax = 8,
   .irq = INT_KPP,
   .learning = 0,
   .delay = 2,
   .matrix = keymapping_hsv1,
};

/* mxc keypad driver */
static struct platform_device mxc_keypad_device_hsv4 = {
   .name = "mxc_keypad",
   .id = 0,
   .num_resources = ARRAY_SIZE(mxc_kpp_resources),
   .resource = mxc_kpp_resources,
   .dev = {
      .release = mxc_nop_release,
      .platform_data = &hsv4_1x2_keypad,
   },
};
static struct platform_device mxc_keypad_device_hsv3 = {
   .name = "mxc_keypad",
   .id = 0,
   .num_resources = ARRAY_SIZE(mxc_kpp_resources),
   .resource = mxc_kpp_resources,
   .dev = {
      .release = mxc_nop_release,
      .platform_data = &evb_2_by_3_keypad,
   },
};

static struct platform_device mxc_keypad_device_hsv1 = {
   .name = "mxc_keypad",
   .id = 0,
   .num_resources = ARRAY_SIZE(mxc_kpp_resources),
   .resource = mxc_kpp_resources,
   .dev = {
      .release = mxc_nop_release,
      .platform_data = &evb_8_by_8_keypad,
   },
};

static void mxc_init_keypad(void)
{
   if ( get_board_type() >= BOARD_TYPE_V4_HOMESCREEN_GENERIC)
      (void)platform_device_register(&mxc_keypad_device_hsv4);
   
   else if ( M_BOARD_IS_HOMESCREEN_V3() )
      (void)platform_device_register(&mxc_keypad_device_hsv3);
   
   else 
      (void)platform_device_register(&mxc_keypad_device_hsv1);
}
#else
static inline void mxc_init_keypad(void)
{
}
#endif

#if defined CONFIG_SERIAL_8250_CONSOLE && (defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE))
/*!
 * The serial port definition structure. The fields contain:
 * {UART, CLK, PORT, IRQ, FLAGS}
 */
static struct plat_serial8250_port serial_platform_data[] = {
   {
      .membase = (void __iomem *)(PBC_BASE_ADDRESS + PBC_SC16C652_UARTA),
      .mapbase = (unsigned long)(CS4_BASE_ADDR + PBC_SC16C652_UARTA),
      .irq = EXPIO_INT_XUART_INTA,
      .uartclk = 14745600,
      .regshift = 0,
      .iotype = UPIO_MEM,
      .flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_AUTO_IRQ,
   },
   {
      .membase = (void __iomem *)(PBC_BASE_ADDRESS + PBC_SC16C652_UARTB),
      .mapbase = (unsigned long)(CS4_BASE_ADDR + PBC_SC16C652_UARTB),
      .irq = EXPIO_INT_XUART_INTB,
      .uartclk = 14745600,
      .regshift = 0,
      .iotype = UPIO_MEM,
      .flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_AUTO_IRQ,
   },
   {},
};

/*!
 * REVISIT: document me
 */
static struct platform_device serial_device = {
   .name = "serial8250",
   .id = 0,
   .dev = {
      .platform_data = serial_platform_data,
   },
};

/*!
 * REVISIT: document me
 */
static int __init mxc_init_extuart(void)
{
   return platform_device_register(&serial_device);
}
#else
static inline int mxc_init_extuart(void)
{
   return 0;
}
#endif
/* MTD NOR flash */

#if defined(CONFIG_MTD_MXC) || defined(CONFIG_MTD_MXC_MODULE)

static struct mtd_partition mxc_nor_partitions[] = {
   {
      .name = "Bootloader",
      .size = 0x20000 , /*128 * 1024,*/
      .offset = 0x00000000,
      .mask_flags = MTD_WRITEABLE	/* force read-only */
   },
   {
      .name = "Kernel",
      .size = 0x1E0000, /*2 * 1024 * 1024,*/
      .offset = MTDPART_OFS_APPEND,
      .mask_flags = 0},
   {
      .name = "rootfs",
      .size = 209 * 128 * 1024,
      .offset = MTDPART_OFS_APPEND,
      .mask_flags = MTD_WRITEABLE},
   {
      .name = "rescue",
      .size = 3 * 1024 * 1024,
      .offset = 0x01C20000,
      .mask_flags = MTD_WRITEABLE	/* force read-only */
   },
   {
      .name = "persistent",
      .size = MTDPART_SIZ_FULL,
      .offset = 0x01F20000,
      .mask_flags = MTD_WRITEABLE	/* force read-only */
   },
};

static struct flash_platform_data mxc_flash_data = {
   .map_name = "cfi_probe",
   .width = 2,
   .parts = mxc_nor_partitions,
   .nr_parts = ARRAY_SIZE(mxc_nor_partitions),
};

static struct resource mxc_flash_resource = {
   .start = 0xa0000000,
   .end = 0xa0000000 + 0x02000000 - 1,
   .flags = IORESOURCE_MEM,

};

static struct platform_device mxc_nor_mtd_device = {
   .name = "mxc_nor_flash",
   .id = 0,
   .dev = {
      .release = mxc_nop_release,
      .platform_data = &mxc_flash_data,
   },
   .num_resources = 1,
   .resource = &mxc_flash_resource,
};

static void mxc_init_nor_mtd(void)
{
   (void)platform_device_register(&mxc_nor_mtd_device);
}
#else
static void mxc_init_nor_mtd(void)
{
}
#endif

/* MTD NAND flash */

#if defined(CONFIG_MTD_NAND_MXC) || defined(CONFIG_MTD_NAND_MXC_MODULE)

static struct mtd_partition mxc_nand_partitions[4] = {
   {
      .name = "IPL-SPL",
      .offset = 0,
      .size = 128 * 1024},
   {
      .name = "nand.kernel",
      .offset = MTDPART_OFS_APPEND,
      .size = 4 * 1024 * 1024},
   {
      .name = "nand.rootfs",
      .offset = MTDPART_OFS_APPEND,
      .size = 22 * 1024 * 1024},
   {
      .name = "nand.userfs",
      .offset = MTDPART_OFS_APPEND,
      .size = MTDPART_SIZ_FULL},
};

static struct flash_platform_data mxc_nand_data = {
   .parts = mxc_nand_partitions,
   .nr_parts = ARRAY_SIZE(mxc_nand_partitions),
   .width = 1,
};

static struct platform_device mxc_nand_mtd_device = {
   .name = "mxc_nand_flash",
   .id = 0,
   .dev = {
      .release = mxc_nop_release,
      .platform_data = &mxc_nand_data,
   },
};

static void mxc_init_nand_mtd(void)
{
   if (__raw_readl(MXC_CCM_RCSR) & MXC_CCM_RCSR_NF16B) {
      mxc_nand_data.width = 2;
   }
   (void)platform_device_register(&mxc_nand_mtd_device);
}
#else
static inline void mxc_init_nand_mtd(void)
{
}
#endif //defined(CONFIG_MTD_NAND_MXC) || defined(CONFIG_MTD_NAND_MXC_MODULE)



#if defined(CONFIG_MACH_MX31HSV1)
static struct mxc_spi_chip_info homescreen_spi_chip_info = {
#ifdef CONFIG_SPI_MXC_TEST_LOOPBACK
   .lb_enable = 1,
#else
   .lb_enable = 0,
#endif
};

static struct spi_board_info mxc_spi_board_info[] __initdata = {
   {
      .modalias = "pmic_spi",
      .irq = -1, //NO irq yet on PMIC (IOMUX_TO_IRQ(MX31_PIN_CSPI1_SPI_RDY))
      .max_speed_hz = 4000000,
      .bus_num = 1,
      .chip_select = 0,
      .controller_data = &homescreen_spi_chip_info,
   },
};


#if defined(CONFIG_SPI_DECT)
struct spi_bus_info spi_dect_info = {
	.spi_loopback = 0,
};

/*!
 * Specs of spi dect, 21/05/09 says :
 * SPI clock freq : 470 KHz
 * Mode : 3
 * Frame size : 16 bits
 * Chip select : 0
 */
static struct spi_board_info spi_dect_board_info[] __initdata = {
   {
      .modalias = 		SPI_DECT_DEVICE_NAME,
      .irq = 			SPI_DECT_IRQ, 
      .max_speed_hz = 		SPI_DECT_CLOCK_FREQ,
      .bus_num = 		1,
      .chip_select = 		SPI_DECT_CHIP_SELECT,
      .mode = 			SPI_DECT_MODE,
      .controller_data = 	&spi_dect_info,
   },
};
#endif

#endif

#if defined(CONFIG_FB_MXC_SYNC_PANEL) || defined(CONFIG_FB_MXC_SYNC_PANEL_MODULE)

#if defined(CONFIG_MACH_MX31LSV0) || defined(CONFIG_MACH_MX27MEDIAPHONE)
static const char fb_screen_default_model[] = "Samsung-WVGA";
#endif

#if defined(CONFIG_MACH_MX31HSV1)
static const char fb_screen_model_samsung[] = "Samsung-WVGA-slim";
static const char fb_screen_model_dataimage[] = "DataImage-WVGA-slim";
#endif

#if defined(CONFIG_MACH_MX27ADS) ||  defined(CONFIG_MACH_MX31ADS)
static const char fb_screen_default_model[] = "Sharp-QVGA";
#endif

/* mxc lcd driver */
static struct platform_device mxc_fb_device = {
   .name = "mxc_sdc_fb",
   .id = 0,
   .dev = {
      .release = mxc_nop_release,
      .platform_data = &fb_screen_model_samsung,
      .coherent_dma_mask = 0xFFFFFFFF,
   },
};

void mxc_init_fb(void)
{
   if ( get_board_type() >= BOARD_TYPE_V4_HOMESCREEN_GENERIC ) {
      mxc_fb_device.dev.platform_data = &fb_screen_model_dataimage ; 
   }
   (void)platform_device_register(&mxc_fb_device);
}
#else
static inline void mxc_init_fb(void)
{
}
#endif

/*
 * Power Off
 */
static void mxc_power_off(void)
{
   gpio_power_off();
}

/*!
 * Board specific fixup function. It is called by \b setup_arch() in
 * setup.c file very early on during kernel starts. It allows the user to
 * statically fill in the proper values for the passed-in parameters. None of
 * the parameters is used currently.
 *
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line
 * @param  mi           pointer to \b struct \b meminfo
 */
static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
      char **cmdline, struct meminfo *mi)
{
   struct tag *t;
#ifdef CONFIG_KGDB_8250
   int i;
   for (i = 0;
         i <
         (sizeof(serial_platform_data) / sizeof(serial_platform_data[0]));
         i += 1)
      kgdb8250_add_platform_port(i, &serial_platform_data[i]);
#endif

   mxc_cpu_init();
#ifdef CONFIG_DISCONTIGMEM
   do {
      int nid;
      mi->nr_banks = MXC_NUMNODES;
      for (nid = 0; nid < mi->nr_banks; nid++) {
         SET_NODE(mi, nid);
      }
   } while (0);
#endif
}

/* ethernet driver initialization */

#if defined(CONFIG_SMC911X) || defined(CONFIG_SMSC_LAN9311)

static struct resource smsc911x_resources[] = {
   [0] = {
      .start  = SMSC9215_BASE_ADDRESS ,
      .end    = (SMSC9215_BASE_ADDRESS + 0x100 - 1),
      .flags  = IORESOURCE_MEM,
   },
   [1] = {
      .start  = SMSC9215_INT , // for HSV1, see board-mx31homescreen.h
      .end    = SMSC9215_INT ,
      .flags  = IORESOURCE_IRQ,
   },
   [2] = {
      .start  = SMSC9215_INT_V2 , // from HSV2, see board-mx31homescreen.h 
      .end    = SMSC9215_INT_V2 ,
      .flags  = IORESOURCE_IRQ,
   },

};


#if defined(CONFIG_SMSC_LAN9311)
struct smsc_lan9311_platform_config {
	unsigned int irq_polarity;
	unsigned int irq_type;
};

static struct smsc_lan9311_platform_config smsc_lan9311_config = {
   .irq_polarity = 0,
   .irq_type = 0,
};
#endif

#if defined(CONFIG_SMSC911X)
static struct platform_device smsc911x_device = {
#if defined(CONFIG_SMSC911X)
   .name           = "smsc911x", /* official driver from smsc */
#else
   .name           = "CLD_DO_NOT_USE_smc911x", /* driver from stock linux kernel
                                                  this driver does not work
                                                  for now
                                                  with 16BIT interface */
#endif
   .id             = 0,
   .num_resources  = ARRAY_SIZE(smsc911x_resources),
   .resource       = smcs911x_resources,
};
#endif

#if defined(CONFIG_SMSC_LAN9311)
static struct platform_device smsc_lan9311_device = {
   .name           = "smsc_lan9311", /* SAGEM driver */
   .id             = 0,
   .dev = {
      .platform_data = &smsc_lan9311_config,
   },
   .num_resources  = ARRAY_SIZE(smsc911x_resources),
   .resource       = smsc911x_resources,
};
#endif

static void mxc_init_eth(void)
{
   /* init irq gpio */
#define ETHRESET_PAD_CFG (PAD_CTL_DRV_NORMAL| PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_100K_PU)
#define ETHIRQ_PAD_CFG (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_FAST | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_OpenDrain | PAD_CTL_100K_PU)

   /* Ethernet IRQ */
   if (M_BOARD_IS_HOMESCREEN_V1()) {
      mxc_iomux_set_pad(HOMESCREEN_V1_GPIO_ETHERNET_IRQ, ETHIRQ_PAD_CFG);
      mxc_request_iomux(HOMESCREEN_V1_GPIO_ETHERNET_IRQ,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);
      mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_ETHERNET_IRQ, GPIO_DIR_INPUT);
//      gpio_set_irq_type(MXC_GPIO_TO_IRQ(HOMESCREEN_V1_GPIO_ETHERNET_IRQ), IRQT_FALLING);
//      gpio_unmask_irq(MXC_GPIO_TO_IRQ(HOMESCREEN_V1_GPIO_ETHERNET_IRQ));
   } else {
      mxc_iomux_set_pad(HOMESCREEN_V2_GPIO_ETHERNET_IRQ, ETHIRQ_PAD_CFG);
      mxc_request_iomux(HOMESCREEN_V2_GPIO_ETHERNET_IRQ,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);
      mxc_set_gpio_direction(HOMESCREEN_V2_GPIO_ETHERNET_IRQ, GPIO_DIR_INPUT);
//      gpio_set_irq_type(MXC_GPIO_TO_IRQ(HOMESCREEN_V2_GPIO_ETHERNET_IRQ), IRQT_FALLING);
//      gpio_unmask_irq(MXC_GPIO_TO_IRQ(HOMESCREEN_V2_GPIO_ETHERNET_IRQ));
   }


   /* Ethernet RESET_N */
   mxc_request_iomux(HOMESCREEN_V1_GPIO_ETHERNET_RESET_N, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
   mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_ETHERNET_RESET_N, GPIO_DIR_OUTPUT);
   mxc_iomux_set_pad(HOMESCREEN_V1_GPIO_ETHERNET_RESET_N, ETHRESET_PAD_CFG);

   if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      // Stand-by Ethernet : 1 = Running, 0 = Stand By
      mxc_request_iomux(HOMESCREEN_V3_GPIO_ETHERNET_STBY_N, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
      mxc_set_gpio_direction(HOMESCREEN_V3_GPIO_ETHERNET_STBY_N, GPIO_DIR_OUTPUT);
      mxc_iomux_set_pad(HOMESCREEN_V3_GPIO_ETHERNET_STBY_N, ETHRESET_PAD_CFG);
      mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_ETHERNET_STBY_N, 1); /* \TODO: think about Power Management and bring this to 0 */
      mdelay(5); /* \FIXME: arbitray value */

      mxc_request_iomux(HOMESCREEN_V3_GPIO_ETHERNET_FIFO_SEL, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
      mxc_set_gpio_direction(HOMESCREEN_V3_GPIO_ETHERNET_FIFO_SEL, GPIO_DIR_OUTPUT);
      mxc_iomux_set_pad(HOMESCREEN_V3_GPIO_ETHERNET_FIFO_SEL, ETHRESET_PAD_CFG);
      mxc_set_gpio_dataout(HOMESCREEN_V3_GPIO_ETHERNET_FIFO_SEL, 0); /* 0: Not FIFO mode */
   }

   /* Ethernet RESET */
   mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_ETHERNET_RESET_N, 0);
   //udelay(100*1000);
   mdelay(100); /* LAN9311 spec says > 1ms + 28µs * eeprom_number_of_byte_to_read (up to 6s)*/ 
   mxc_set_gpio_dataout(HOMESCREEN_V1_GPIO_ETHERNET_RESET_N, 1);
   // printk("Reset deasserted for smsc.\n");

#define CSCR1U 0x10
#define CSCR1L 0x14
#define CSCR1A 0x18
   /* CLD: configure CS1 timing and width (16bits bus ; 32bits accesses) */
   __raw_writel(0x300FD500, IO_ADDRESS(WEIM_BASE_ADDR) + CSCR1U);
   __raw_writel(0x00000501, IO_ADDRESS(WEIM_BASE_ADDR) + CSCR1L);
   __raw_writel(0x00000000, IO_ADDRESS(WEIM_BASE_ADDR) + CSCR1A);

#if defined(CONFIG_SMSC911X)
   (void)platform_device_register(&smsc911x_device);
#elif defined(CONFIG_SMSC_LAN9311)
   (void)platform_device_register(&smsc_lan9311_device);
#endif
}
#else

static void mxc_init_eth(void)
{

}
#endif


#if defined(CONFIG_MXC_PMIC_MC13783) && defined(CONFIG_SND_MXC_PMIC)

static void __init mxc_init_pmic_audio(void)
{
   struct clk *pll_clk;
   struct clk *ssi_clk;
   struct clk *ckih_clk;
   struct clk *cko_clk;

   /* Enable 26 mhz clock on CKO1 for PMIC audio */
   ckih_clk = clk_get(NULL, "ckih");
   cko_clk = clk_get(NULL, "cko1_clk");
   if (IS_ERR(ckih_clk) || IS_ERR(cko_clk)) {
      printk(KERN_ERR "Unable to set CKO1 output to CKIH\n");
   } else {
      clk_set_parent(cko_clk, ckih_clk);
      clk_set_rate(cko_clk, clk_get_rate(ckih_clk));
      clk_enable(cko_clk);
   }
   clk_put(ckih_clk);
   clk_put(cko_clk);

   /* Assign USBPLL to be used by SSI1/2 */
   pll_clk = clk_get(NULL, "usb_pll");
   ssi_clk = clk_get(NULL, "ssi_clk.0");
   clk_set_parent(ssi_clk, pll_clk);
   clk_enable(ssi_clk);
   clk_put(ssi_clk);

   ssi_clk = clk_get(NULL, "ssi_clk.1");
   clk_set_parent(ssi_clk, pll_clk);
   clk_enable(ssi_clk);
   clk_put(ssi_clk);
   clk_put(pll_clk);

   gpio_activate_audio_ports();
}
#else
static void __inline mxc_init_pmic_audio(void)
{
}
#endif

#define GPIO_WIFI_POWER_DOWN MX31_PIN_GPIO3_0
#define GPIO_WIFI_POWER_DOWN_ACTIVE	(0)
#define GPIO_WIFI_POWER_DOWN_INACTIVE	(1)

#define GPIO_WIFI_RESET MX31_PIN_GPIO3_1
#define GPIO_WIFI_RESET_ACTIVE	(0)
#define GPIO_WIFI_RESET_INACTIVE	(1)

#define COMMAND_PAD_CFG (PAD_CTL_DRV_NORMAL | PAD_CTL_SRE_SLOW | PAD_CTL_HYS_CMOS | PAD_CTL_ODE_CMOS | PAD_CTL_100K_PU)

static unsigned char wifi_gpios_already_requested = 0; /* Global variable */

void mxc_init_sdio_wifi(void)
{
	if (!wifi_gpios_already_requested)
	{
		mxc_request_iomux(GPIO_WIFI_POWER_DOWN, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		mxc_iomux_set_pad(GPIO_WIFI_POWER_DOWN, COMMAND_PAD_CFG);
		mxc_set_gpio_direction(GPIO_WIFI_POWER_DOWN, GPIO_DIR_OUTPUT);
		mxc_request_iomux(GPIO_WIFI_RESET, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
		mxc_iomux_set_pad(GPIO_WIFI_RESET, COMMAND_PAD_CFG);
		mxc_set_gpio_direction(GPIO_WIFI_RESET, GPIO_DIR_OUTPUT);
		wifi_gpios_already_requested = 1;
	}

	// CLD: GPIO_WIFI_POWER_DOWN
	mxc_set_gpio_dataout(GPIO_WIFI_POWER_DOWN, GPIO_WIFI_POWER_DOWN_INACTIVE);

	// CLD: GPIO_WIFI_RESET
	mxc_set_gpio_dataout(GPIO_WIFI_RESET, GPIO_WIFI_RESET_ACTIVE);
	mdelay(1);
	mxc_set_gpio_dataout(GPIO_WIFI_RESET, GPIO_WIFI_RESET_INACTIVE);
	mdelay(100);
}

EXPORT_SYMBOL(mxc_init_sdio_wifi);

void mxc_deinit_sdio_wifi(void)
{
   // WIFI_POWER_DOWN_GPIO
   mxc_set_gpio_dataout(GPIO_WIFI_POWER_DOWN, GPIO_WIFI_POWER_DOWN_ACTIVE);
}

EXPORT_SYMBOL(mxc_deinit_sdio_wifi);

void mxc_init_sdcard(void)
{
}

void mx31hsv1_gpio_init(void);

#define PLL_PCTL_REG(pd, mfd, mfi, mfn)		\
	((((pd) - 1) << 26) + (((mfd) - 1) << 16) + ((mfi)  << 10) + mfn)

/* For 26MHz input clock */
#define PLL_532MHZ		PLL_PCTL_REG(1, 13, 10, 3)
#define PLL_399MHZ		PLL_PCTL_REG(1, 52, 7, 35)
#define PLL_133MHZ		PLL_PCTL_REG(2, 26, 5, 3)

/* For 27MHz input clock */
#define PLL_532_8MHZ		PLL_PCTL_REG(1, 15, 9, 13)
#define PLL_399_6MHZ		PLL_PCTL_REG(1, 18, 7, 7)
#define PLL_133_2MHZ		PLL_PCTL_REG(3, 5, 7, 2)

#define PDR0_REG(mcu, max, hsp, ipg, nfc)	\
	(MXC_CCM_PDR0_MCU_DIV_##mcu | MXC_CCM_PDR0_MAX_DIV_##max | \
	 MXC_CCM_PDR0_HSP_DIV_##hsp | MXC_CCM_PDR0_IPG_DIV_##ipg | \
	 MXC_CCM_PDR0_NFC_DIV_##nfc)

/* working point(wp): 0 - 133MHz; 1 - 266MHz; 2 - 399MHz; 3 - 532MHz */
/* 26MHz input clock table */
static struct cpu_wp cpu_wp_26[] = {
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 133000000,
	 .pdr0_reg = PDR0_REG(4, 4, 4, 2, 6),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 266000000,
	 .pdr0_reg = PDR0_REG(2, 4, 4, 2, 6),},
	{
	 .pll_reg = PLL_399MHZ,
	 .pll_rate = 399000000,
	 .cpu_rate = 399000000,
	 .pdr0_reg = PDR0_REG(1, 3, 3, 2, 6),},
	{
	 .pll_reg = PLL_532MHZ,
	 .pll_rate = 532000000,
	 .cpu_rate = 532000000,
	 .pdr0_reg = PDR0_REG(1, 4, 4, 2, 6),},
};

struct cpu_wp *get_cpu_wp(int *wp)
{
	*wp = 4;
	return cpu_wp_26;
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
   mxc_cpu_common_init();
   mxc_clocks_init();

   board_identifier();

   mxc_init_pmic_audio(); /* init clocks and pads */


   mxc_gpio_init();

   mx31hsv1_gpio_init(); /* init GPIOs and maintain SOFT_STOP to 1 */
   pm_power_off = mxc_power_off; /* power off callback called from kernel/process.c */

   mxc_init_keypad();

   mxc_init_nand_mtd();
   mxc_init_nor_mtd();

   mxc_init_eth();

   mxc_init_sdio_wifi();
   mxc_init_sdcard();

   /* PMIC (aka ATLAS) : IRQ for Homescreen at least V2 (no IRQ on HSV1) */
   if (!M_BOARD_IS_HOMESCREEN_V1())
      mxc_spi_board_info[0].irq = IOMUX_TO_IRQ(HOMESCREEN_V2_GPIO_PMIC_IRQ);

   if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) {
      mxc_spi_board_info[0].bus_num = 2; /* CSPI bus selection should be done here instead of 'menuconfig' */
   }

   spi_register_board_info(mxc_spi_board_info, ARRAY_SIZE(mxc_spi_board_info));
#if defined(CONFIG_SPI_DECT)
   spi_register_board_info(spi_dect_board_info, ARRAY_SIZE(spi_dect_board_info));
#endif

   mxc_init_fb();
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MX31ADS data structure.
 */
/* *INDENT-OFF* */
MACHINE_START(MX31ADS, "Sagem Communications MX31 HomeScreen")
/* Maintainer: Sagem Communication */
.phys_io = AIPS1_BASE_ADDR,
   .io_pg_offst = ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,

   .boot_params = PHYS_OFFSET + 0x100,
   .fixup = fixup_mxc_board,
   .map_io = mxc_map_io,
   .init_irq = mxc_init_irq,
   .init_machine = mxc_board_init,
   .timer = &mxc_timer,
   MACHINE_END
