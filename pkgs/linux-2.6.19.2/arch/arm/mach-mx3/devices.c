/*
 * Author: MontaVista Software, Inc.
 *       <source@mvista.com>
 *
 * Modified by Sagemcom under GPL license on 17/07/2009 
 * Copyright (c) 2010 Sagemcom All rights reserved:
 *
 * Based on the OMAP devices.c
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/spi/spi.h>

#include <asm/hardware.h>
#include <asm/arch/mmc.h>
#include <asm/arch/clock.h>

#include <asm/arch/spba.h>
#include "iomux.h"
#include <asm/arch/sdma.h>
#include "sdma_script_code.h"
#include "sdma_script_code_pass2.h"

extern struct dptc_wp dptc_wp_allfreq_26ckih[DPTC_WP_SUPPORTED];
extern struct dptc_wp dptc_wp_allfreq_26ckih_TO_2_0[DPTC_WP_SUPPORTED];
extern struct dptc_wp dptc_wp_allfreq_27ckih_TO_2_0[DPTC_WP_SUPPORTED];

#if 0
int board_device_enable(u32 device_id);
int board_device_disable(u32 device_id);

int mxc_device_enable(u32 device_id)
{
	int ret = 0;

	switch (device_id) {
	default:
		ret = board_device_enable(device_id);
	}

	return ret;
}

int mxc_device_disable(u32 device_id)
{
	int ret = 0;

	switch (device_id) {
	default:
		ret = board_device_disable(device_id);
	}
	return ret;
}

EXPORT_SYMBOL(mxc_device_enable);
EXPORT_SYMBOL(mxc_device_disable);
#endif

void mxc_sdma_get_script_info(sdma_script_start_addrs * sdma_script_addr)
{
	if (system_rev == CHIP_REV_1_0) {
		sdma_script_addr->mxc_sdma_app_2_mcu_addr = app_2_mcu_ADDR;
		sdma_script_addr->mxc_sdma_ap_2_ap_addr = ap_2_ap_ADDR;
		sdma_script_addr->mxc_sdma_ap_2_bp_addr = -1;
		sdma_script_addr->mxc_sdma_bp_2_ap_addr = -1;
		sdma_script_addr->mxc_sdma_loopback_on_dsp_side_addr = -1;
		sdma_script_addr->mxc_sdma_mcu_2_app_addr = mcu_2_app_ADDR;
		sdma_script_addr->mxc_sdma_mcu_2_shp_addr = mcu_2_shp_ADDR;
		sdma_script_addr->mxc_sdma_mcu_interrupt_only_addr = -1;
		sdma_script_addr->mxc_sdma_shp_2_mcu_addr = shp_2_mcu_ADDR;
		sdma_script_addr->mxc_sdma_start_addr =
		    (unsigned short *)sdma_code;
		sdma_script_addr->mxc_sdma_uartsh_2_mcu_addr =
		    uartsh_2_mcu_ADDR;
		sdma_script_addr->mxc_sdma_uart_2_mcu_addr = uart_2_mcu_ADDR;
		sdma_script_addr->mxc_sdma_ram_code_size = RAM_CODE_SIZE;
		sdma_script_addr->mxc_sdma_ram_code_start_addr =
		    RAM_CODE_START_ADDR;
		sdma_script_addr->mxc_sdma_dptc_dvfs_addr = dptc_dvfs_ADDR;
		sdma_script_addr->mxc_sdma_firi_2_mcu_addr = firi_2_mcu_ADDR;
		sdma_script_addr->mxc_sdma_firi_2_per_addr = -1;
		sdma_script_addr->mxc_sdma_mshc_2_mcu_addr = mshc_2_mcu_ADDR;
		sdma_script_addr->mxc_sdma_per_2_app_addr = -1;
		sdma_script_addr->mxc_sdma_per_2_firi_addr = -1;
		sdma_script_addr->mxc_sdma_per_2_shp_addr = -1;
		sdma_script_addr->mxc_sdma_mcu_2_ata_addr = mcu_2_ata_ADDR;
		sdma_script_addr->mxc_sdma_mcu_2_firi_addr = mcu_2_firi_ADDR;
		sdma_script_addr->mxc_sdma_mcu_2_mshc_addr = mcu_2_mshc_ADDR;
		sdma_script_addr->mxc_sdma_ata_2_mcu_addr = ata_2_mcu_ADDR;
		sdma_script_addr->mxc_sdma_uartsh_2_per_addr = -1;
		sdma_script_addr->mxc_sdma_shp_2_per_addr = -1;
		sdma_script_addr->mxc_sdma_uart_2_per_addr = -1;
		sdma_script_addr->mxc_sdma_app_2_per_addr = -1;
	} else {
		sdma_script_addr->mxc_sdma_app_2_mcu_addr = app_2_mcu_ADDR_2;
		sdma_script_addr->mxc_sdma_ap_2_ap_addr = ap_2_ap_ADDR_2;
		sdma_script_addr->mxc_sdma_ap_2_bp_addr = ap_2_bp_ADDR_2;
		sdma_script_addr->mxc_sdma_bp_2_ap_addr = bp_2_ap_ADDR_2;
		sdma_script_addr->mxc_sdma_loopback_on_dsp_side_addr = -1;
		sdma_script_addr->mxc_sdma_mcu_2_app_addr = mcu_2_app_ADDR_2;
		sdma_script_addr->mxc_sdma_mcu_2_shp_addr = mcu_2_shp_ADDR_2;
		sdma_script_addr->mxc_sdma_mcu_interrupt_only_addr = -1;
		sdma_script_addr->mxc_sdma_shp_2_mcu_addr = shp_2_mcu_ADDR_2;
		sdma_script_addr->mxc_sdma_start_addr =
		    (unsigned short *)sdma_code_2;
		sdma_script_addr->mxc_sdma_uartsh_2_mcu_addr =
		    uartsh_2_mcu_ADDR_2;
		sdma_script_addr->mxc_sdma_uart_2_mcu_addr = uart_2_mcu_ADDR_2;
		sdma_script_addr->mxc_sdma_ram_code_size = RAM_CODE_SIZE_2;
		sdma_script_addr->mxc_sdma_ram_code_start_addr =
		    RAM_CODE_START_ADDR_2;
		sdma_script_addr->mxc_sdma_dptc_dvfs_addr = -1;
		sdma_script_addr->mxc_sdma_firi_2_mcu_addr = firi_2_mcu_ADDR_2;
		sdma_script_addr->mxc_sdma_firi_2_per_addr = firi_2_per_ADDR_2;
		sdma_script_addr->mxc_sdma_mshc_2_mcu_addr = mshc_2_mcu_ADDR_2;
		sdma_script_addr->mxc_sdma_per_2_app_addr = per_2_app_ADDR_2;
		sdma_script_addr->mxc_sdma_per_2_firi_addr = per_2_firi_ADDR_2;
		sdma_script_addr->mxc_sdma_per_2_shp_addr = per_2_shp_ADDR_2;
		sdma_script_addr->mxc_sdma_mcu_2_ata_addr = mcu_2_ata_ADDR_2;
		sdma_script_addr->mxc_sdma_mcu_2_firi_addr = mcu_2_firi_ADDR_2;
		sdma_script_addr->mxc_sdma_mcu_2_mshc_addr = mcu_2_mshc_ADDR_2;
		sdma_script_addr->mxc_sdma_ata_2_mcu_addr = ata_2_mcu_ADDR_2;
		sdma_script_addr->mxc_sdma_uartsh_2_per_addr =
		    uartsh_2_per_ADDR_2;
		sdma_script_addr->mxc_sdma_shp_2_per_addr = shp_2_per_ADDR_2;
		sdma_script_addr->mxc_sdma_uart_2_per_addr = uart_2_per_ADDR_2;
		sdma_script_addr->mxc_sdma_app_2_per_addr = app_2_per_ADDR_2;
	}
}

static void mxc_nop_release(struct device *dev)
{
	/* Nothing */
}

#if defined(CONFIG_W1_MASTER_MXC) || defined(CONFIG_W1_MASTER_MXC_MODULE)
static struct mxc_w1_config mxc_w1_data = {
	.search_rom_accelerator = 0,
};

static struct platform_device mxc_w1_devices = {
	.name = "mxc_w1",
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_w1_data,
		},
	.id = 0
};

static void mxc_init_owire(void)
{
	(void)platform_device_register(&mxc_w1_devices);
}
#else
static inline void mxc_init_owire(void)
{
}
#endif

#if defined(CONFIG_MXC_RTC)

static struct platform_device mxc_rtc_device = {
	.name = "mxc_rtc",
	.id = 0,
};
static void mxc_init_rtc(void)
{
	(void)platform_device_register(&mxc_rtc_device);
}
#else
static inline void mxc_init_rtc(void)
{
}
#endif

#if defined(CONFIG_MXC_WATCHDOG) || defined(CONFIG_MXC_WATCHDOG_MODULE)

static struct resource wdt_resources[] = {
	{
	 .start = WDOG_BASE_ADDR,
	 .end = WDOG_BASE_ADDR + 0x30,
	 .flags = IORESOURCE_MEM,
	 },
};

static struct platform_device mxc_wdt_device = {
	.name = "mxc_wdt",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
	.num_resources = ARRAY_SIZE(wdt_resources),
	.resource = wdt_resources,
};

static void mxc_init_wdt(void)
{
	(void)platform_device_register(&mxc_wdt_device);
}
#else
static inline void mxc_init_wdt(void)
{
}
#endif

#if defined(CONFIG_MXC_IPU) || defined(CONFIG_MXC_IPU_MODULE)
static struct mxc_ipu_config mxc_ipu_data = {
	.rev = 1,
};

static struct resource ipu_resources[] = {
	{
	 .start = IPU_CTRL_BASE_ADDR,
	 .end = IPU_CTRL_BASE_ADDR + SZ_4K,
	 .flags = IORESOURCE_MEM,
	 },
	{
	 .start = INT_IPU_SYN,
	 .flags = IORESOURCE_IRQ,
	 },
	{
	 .start = INT_IPU_ERR,
	 .flags = IORESOURCE_IRQ,
	 },
};

static struct platform_device mxc_ipu_device = {
	.name = "mxc_ipu",
	.id = -1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_ipu_data,
		},
	.num_resources = ARRAY_SIZE(ipu_resources),
	.resource = ipu_resources,
};

static void mxc_init_ipu(void)
{
	platform_device_register(&mxc_ipu_device);
}
#else
static inline void mxc_init_ipu(void)
{
}
#endif

#if defined(CONFIG_MXC_FIR) || defined(CONFIG_MXC_FIR_MODULE)
/*!
 * Resource definition for the FIR
 */
static struct resource mxcir_resources[] = {
	[0] = {
	       .start = UART2_BASE_ADDR,
	       .end = UART2_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_UART2,
	       .end = INT_UART2,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = FIRI_BASE_ADDR,
	       .end = FIRI_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[3] = {
	       .start = INT_FIRI,
	       .end = INT_FIRI,
	       .flags = IORESOURCE_IRQ,
	       },
	[4] = {
	       .start = INT_UART2,
	       .end = INT_UART2,
	       .flags = IORESOURCE_IRQ,
	       }
};

static struct mxc_ir_platform_data ir_data;

/*! Device Definition for MXC FIR */
static struct platform_device mxcir_device = {
	.name = "mxcir",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &ir_data,
		},
	.num_resources = ARRAY_SIZE(mxcir_resources),
	.resource = mxcir_resources,
};

static inline void mxc_init_ir(void)
{
	ir_data.uart_ir_mux = 1;
	ir_data.uart_clk = clk_get(NULL, "uart_clk.1");;
	(void)platform_device_register(&mxcir_device);
}
#else
static inline void mxc_init_ir(void)
{
}
#endif

/* MMC device data */

#if defined(CONFIG_MMC_MXC) || defined(CONFIG_MMC_MXC_MODULE)

extern unsigned int sdhc_get_card_det_status(struct device *dev);
extern int sdhc_init_card_det(int id);
int sdhc_init_card_write_protect(int id);
int sdhc_get_card_write_protect_status(struct device *dev);

static struct mxc_mmc_platform_data mmc_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30,
	.min_clk = 150000,
	.max_clk = 25000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
};

/*!
 * Resource definition for the SDHC1
 */
/* static struct resource mxcsdhc1_resources[] = {
	[0] = {
	       .start = MMC_SDHC1_BASE_ADDR,
	       .end = MMC_SDHC1_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_MMC_SDHC1,
	       .end = INT_MMC_SDHC1,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = 0,
	       .end = 0,
	       .flags = IORESOURCE_IRQ,
	       },
	[3] = {
	       .start = MXC_SDIO1_CARD_IRQ,
	       .end = MXC_SDIO1_CARD_IRQ,
	       .flags = IORESOURCE_IRQ,
	       },
}; */ /* SAGEM LE ROY O: sdhc1 is used by Marvell sd8686 proprietary driver */

/*!
 * Resource definition for the SDHC2
 */
static struct resource mxcsdhc2_resources[] = {
	[0] = {
	       .start = MMC_SDHC2_BASE_ADDR,
	       .end = MMC_SDHC2_BASE_ADDR + SZ_16K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_MMC_SDHC2,
	       .end = INT_MMC_SDHC2,
	       .flags = IORESOURCE_IRQ,
	       },
	[2] = {
	       .start = 0,
	       .end = 0,
	       .flags = IORESOURCE_IRQ,
	       },
	[3] = {
	       .start = MXC_SDIO2_CARD_IRQ,
	       .end = MXC_SDIO2_CARD_IRQ,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Device Definition for MXC SDHC1 */
/* static struct platform_device mxcsdhc1_device = {
	.name = "mxcmci",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mmc_data,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc1_resources),
	.resource = mxcsdhc1_resources,
}; */ /* SAGEM LE ROY O: sdhc1 is used by Marvell sd8686 proprietary driver */

/*! Device Definition for MXC SDHC2 */
static struct platform_device mxcsdhc2_device = {
	.name = "mxcmci",
	.id = 1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mmc_data,
		},
	.num_resources = ARRAY_SIZE(mxcsdhc2_resources),
	.resource = mxcsdhc2_resources,
};

static inline void mxc_init_mmc(void)
{
	int cd_irq;

	sdhc_init_card_write_protect(0);
	cd_irq = sdhc_init_card_det(0);
//SAGEM Changed by leroyo. SDHC1 device is used by Marvell sdio driver.
/*	if (cd_irq) {
		mxcsdhc1_device.resource[2].start = cd_irq;
		mxcsdhc1_device.resource[2].end = cd_irq;
	}
*/
	sdhc_init_card_write_protect(1);
	cd_irq = sdhc_init_card_det(1);
	if (cd_irq) {
		mxcsdhc2_device.resource[2].start = cd_irq;
		mxcsdhc2_device.resource[2].end = cd_irq;
	}

//	spba_take_ownership(SPBA_SDHC1, SPBA_MASTER_A | SPBA_MASTER_C); // SAGEM Changed by leroyo. SDHC1 device 
//	(void)platform_device_register(&mxcsdhc1_device);               // is used by Marvell sdio driver.
	spba_take_ownership(SPBA_SDHC2, SPBA_MASTER_A | SPBA_MASTER_C);
	(void)platform_device_register(&mxcsdhc2_device);
}
#else
static inline void mxc_init_mmc(void)
{
}
#endif

/* SPI controller and device data */
#if defined(CONFIG_SPI_MXC) || defined(CONFIG_SPI_MXC_MODULE)

#if defined(CONFIG_MACH_MX31HSV1)
#if defined(CONFIG_SPI_MXC_SELECT1)
/*!
 * Resource definition for the CSPI1
 */
static struct resource mxcspi1_resources[] = {
	[0] = {
	       .start = CSPI1_BASE_ADDR,
	       .end = CSPI1_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_CSPI1,
	       .end = INT_CSPI1,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC CSPI1 */
static struct mxc_spi_master mxcspi1_data = {
	.maxchipselect = 4,
	.spi_version = 4,
};

/*! Device Definition for MXC CSPI1 */
static struct platform_device mxcspi1_device = {
	.name = "mxc_spi",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxcspi1_data,
		},
	.num_resources = ARRAY_SIZE(mxcspi1_resources),
	.resource = mxcspi1_resources,
};

#endif				/* CONFIG_SPI_MXC_SELECT1 */

#if defined(CONFIG_SPI_MXC_SELECT2)
/*!
 * Resource definition for the CSPI2
 */
static struct resource mxcspi2_resources[] = {
	[0] = {
	       .start = CSPI2_BASE_ADDR,
	       .end = CSPI2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_CSPI2,
	       .end = INT_CSPI2,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC CSPI2 */
static struct mxc_spi_master mxcspi2_data = {
	.maxchipselect = 4,
	.spi_version = 4,
};

/*! Device Definition for MXC CSPI2 */
static struct platform_device mxcspi2_device = {
	.name = "mxc_spi",
	.id = 1,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxcspi2_data,
		},
	.num_resources = ARRAY_SIZE(mxcspi2_resources),
	.resource = mxcspi2_resources,
};
#endif				/* CONFIG_SPI_MXC_SELECT2 */

#ifdef CONFIG_SPI_MXC_SELECT3
/*!
 * Resource definition for the CSPI3
 */
static struct resource mxcspi3_resources[] = {
	[0] = {
	       .start = CSPI3_BASE_ADDR,
	       .end = CSPI3_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_CSPI3,
	       .end = INT_CSPI3,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC CSPI3 */
static struct mxc_spi_master mxcspi3_data = {
	.maxchipselect = 4,
	.spi_version = 4,
};

/*! Device Definition for MXC CSPI3 */
static struct platform_device mxcspi3_device = {
	.name = "mxc_spi",
	.id = 2,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxcspi3_data,
		},
	.num_resources = ARRAY_SIZE(mxcspi3_resources),
	.resource = mxcspi3_resources,
};
#endif				/* CONFIG_SPI_MXC_SELECT3 */
#endif				/* CONFIG_MACH_MX31HSV1 */

static inline void mxc_init_spi(void)
{
	/* SPBA configuration for CSPI2 - MCU is set */
	spba_take_ownership(SPBA_CSPI2, SPBA_MASTER_A);
#if defined(CONFIG_MACH_MX31HSV1) && ! defined(CONFIG_SPI_DECT) && defined(CONFIG_SPI_MXC_SELECT1) && defined(CONFIG_SPI_MXC_SELECT2)
	if (M_BOARD_IS_HOMESCREEN_V1() || M_BOARD_IS_HOMESCREEN_V2()) {
		if (platform_device_register(&mxcspi1_device) < 0)
			printk("Error: Registering the SPI Controller_1\n");
	}
	else {
		if (platform_device_register(&mxcspi2_device) < 0)
			printk("Error: Registering the SPI Controller_2\n");
	}
#else
#ifdef CONFIG_SPI_MXC_SELECT1
	if (platform_device_register(&mxcspi1_device) < 0)
		printk("Error: Registering the SPI Controller_1\n");
#endif				/* CONFIG_SPI_MXC_SELECT1 */
#ifdef CONFIG_SPI_MXC_SELECT2
	if (platform_device_register(&mxcspi2_device) < 0)
		printk("Error: Registering the SPI Controller_2\n");
#endif				/* CONFIG_SPI_MXC_SELECT2 */
#ifdef CONFIG_SPI_MXC_SELECT3
	if (platform_device_register(&mxcspi3_device) < 0)
		printk("Error: Registering the SPI Controller_3\n");
#endif				/* CONFIG_SPI_MXC_SELECT3 */
#endif /* MX31HS */
}
#else
static inline void mxc_init_spi(void)
{
}
#endif

/* I2C controller and device data */
#if defined(CONFIG_I2C_MXC) || defined(CONFIG_I2C_MXC_MODULE)

#ifdef CONFIG_I2C_MXC_SELECT1
/*!
 * Resource definition for the I2C1
 */
static struct resource mxci2c1_resources[] = {
	[0] = {
	       .start = I2C_BASE_ADDR,
	       .end = I2C_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_I2C,
	       .end = INT_I2C,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c1_data = {
	.i2c_clk = 100000,
};
#endif

#if defined(CONFIG_I2C_MXC_SELECT2) || defined(CONFIG_MACH_MX31HSV1)
/*!
 * Resource definition for the I2C2
 */
static struct resource mxci2c2_resources[] = {
	[0] = {
	       .start = I2C2_BASE_ADDR,
	       .end = I2C2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_I2C2,
	       .end = INT_I2C2,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c2_data = {
	.i2c_clk = 400000,
};
#endif

#ifdef CONFIG_I2C_MXC_SELECT3
/*!
 * Resource definition for the I2C3
 */
static struct resource mxci2c3_resources[] = {
	[0] = {
	       .start = I2C3_BASE_ADDR,
	       .end = I2C3_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = INT_I2C3,
	       .end = INT_I2C3,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c3_data = {
	.i2c_clk = 100000,
};
#endif

/*! Device Definition for MXC I2C1 */
#if defined(CONFIG_MACH_MX31HSV1)
static struct platform_device mxci2c_device = {
        .name = "mxc_i2c",
        .id = 1,
        .dev = {
                 .release = mxc_nop_release,
                 .platform_data = &mxci2c2_data,
                 },
        .num_resources = ARRAY_SIZE(mxci2c2_resources),
	.resource = mxci2c2_resources,
};

#else
static struct platform_device mxci2c_devices[] = {
#ifdef CONFIG_I2C_MXC_SELECT1
	{
	 .name = "mxc_i2c",
	 .id = 0,
	 .dev = {
		 .release = mxc_nop_release,
		 .platform_data = &mxci2c1_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c1_resources),
	 .resource = mxci2c1_resources,},
#endif
#if defined(CONFIG_I2C_MXC_SELECT2)
	{
	 .name = "mxc_i2c",
	 .id = 1,
	 .dev = {
		 .release = mxc_nop_release,
		 .platform_data = &mxci2c2_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c2_resources),
	 .resource = mxci2c2_resources,},
#endif
#ifdef CONFIG_I2C_MXC_SELECT3
	{
	 .name = "mxc_i2c",
	 .id = 2,
	 .dev = {
		 .release = mxc_nop_release,
		 .platform_data = &mxci2c3_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c3_resources),
	 .resource = mxci2c3_resources,},
#endif
};
#endif

static inline void mxc_init_i2c(void)
{
#if defined(CONFIG_MACH_MX31HSV1)
/* AAS: Only activate i2c2 on hsv1 as it messes with touchscreen on hsv3*/
	if (M_BOARD_IS_HOMESCREEN_V1() || M_BOARD_IS_HOMESCREEN_V2()) {
		if (platform_device_register(&mxci2c_device) < 0)
                        dev_err(&mxci2c_device.dev,
                                "Unable to register I2C device\n");
	}
#else
	int i;

	for (i = 0; i < ARRAY_SIZE(mxci2c_devices); i++) {
		if (platform_device_register(&mxci2c_devices[i]) < 0)
			dev_err(&mxci2c_devices[i].dev,
				"Unable to register I2C device\n");
	}
#endif /* MX31HS */
}
#else
static inline void mxc_init_i2c(void)
{
}
#endif

struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
	 .num = 0,
	 .base = IO_ADDRESS(GPIO1_BASE_ADDR),
	 .irq = INT_GPIO1,
	 .virtual_irq_start = MXC_GPIO_BASE,
	 },
	{
	 .num = 1,
	 .base = IO_ADDRESS(GPIO2_BASE_ADDR),
	 .irq = INT_GPIO2,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN,
	 },
	{
	 .num = 2,
	 .base = IO_ADDRESS(GPIO3_BASE_ADDR),
	 .irq = INT_GPIO3,
	 .virtual_irq_start = MXC_GPIO_BASE + GPIO_NUM_PIN * 2,
	 },
};

#if defined(CONFIG_PCMCIA_MX31ADS) || defined(CONFIG_PCMCIA_MX31ADS_MODULE)

static struct platform_device mx31ads_device = {
	.name = "Mx31ads pcmcia socket",
	.id = 0,
	.dev.release = mxc_nop_release,
};
static inline void mxc_init_pcmcia(void)
{
	platform_device_register(&mx31ads_device);
}
#else
static inline void mxc_init_pcmcia(void)
{
}
#endif

#if defined(CONFIG_MXC_HMP4E) || defined(CONFIG_MXC_HMP4E_MODULE)
static struct platform_device hmp4e_device = {
	.name = "mxc_hmp4e",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		}
};

static inline void mxc_init_hmp4e(void)
{
	/* override fuse for Hantro HW clock */
	if (readl(IO_ADDRESS(IIM_BASE_ADDR + 0x808)) == 0x4) {
		if (!(readl(IO_ADDRESS(IIM_BASE_ADDR + 0x800)) & (1 << 5))) {
			writel(readl(IO_ADDRESS(IIM_BASE_ADDR + 0x808)) &
			       0xfffffffb, IO_ADDRESS(IIM_BASE_ADDR + 0x808));
		}
	}

	platform_device_register(&hmp4e_device);
}
#else
static inline void mxc_init_hmp4e(void)
{
}
#endif

static struct platform_device mxc_dma_device = {
	.name = "mxc_dma",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		},
};

static inline void mxc_init_dma(void)
{
	(void)platform_device_register(&mxc_dma_device);
}

/*! Device Definition for DPTC */
static struct platform_device mxc_dptc_device = {
	.name = "mxc_dptc",
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &dptc_wp_allfreq_26ckih,
		},
};

static inline void mxc_init_dptc(void)
{
	unsigned long ckih_rate = 26000000;

	/* TO2 silicon has unique DPTC tables */
	if (cpu_is_mx31_rev(CHIP_REV_2_0) == 1) {
		if (ckih_rate == 27000000) {
			mxc_dptc_device.dev.platform_data =
				&dptc_wp_allfreq_27ckih_TO_2_0;
		}
		else {
			mxc_dptc_device.dev.platform_data =
				&dptc_wp_allfreq_26ckih_TO_2_0;
		}
	}
	else {
		if (ckih_rate == 27000000) {
			mxc_dptc_device.dev.platform_data = NULL;
			goto err;
		}
		else {
			mxc_dptc_device.dev.platform_data =
				&dptc_wp_allfreq_26ckih;
		}
	}
	(void)platform_device_register(&mxc_dptc_device);
	return;

err:	printk(KERN_ERR "DPTC disabled for this chip rev and clock rate!\n");
}

/*! Device Definition for SAGEM Audio Pwr Amp */
static struct platform_device sagem_audio_pwr_amp_device = {
	.name = "sagem_audio_pwr_amp",
};

static inline void sagem_init_audio_pwr_amp(void)
{
	(void)platform_device_register(&sagem_audio_pwr_amp_device);
	return;
}

static int __init mxc_init_devices(void)
{
	mxc_init_wdt();
	mxc_init_ipu();
	mxc_init_mmc();
	mxc_init_ir();
	mxc_init_spi();
	mxc_init_i2c();
	mxc_init_rtc();
	mxc_init_owire();
	mxc_init_pcmcia();
	mxc_init_hmp4e();
	mxc_init_dma();
	mxc_init_dptc();
	sagem_init_audio_pwr_amp();

	/* SPBA configuration for SSI2 - SDMA and MCU are set */
	spba_take_ownership(SPBA_SSI2, SPBA_MASTER_C | SPBA_MASTER_A);
	return 0;
}

arch_initcall(mxc_init_devices);
