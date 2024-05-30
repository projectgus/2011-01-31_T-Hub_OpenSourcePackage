/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_H__
#define __ASM_ARCH_MXC_H__

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#error "Do not include directly."
#endif

#ifndef __ASSEMBLY__
#include <linux/types.h>

/*!
 * @ingroup MSL_MX27 MSL_MX31
 */
/*!
 * gpio port structure
 */
struct mxc_gpio_port {
	u32 num;		/*!< gpio port number */
	u32 base;		/*!< gpio port base VA */
	u16 irq;		/*!< irq number to the core */
	u16 virtual_irq_start;	/*!< virtual irq start number */
};
/*!
 * This structure is used to define the One wire platform data.
 * It includes search rom accelerator.
 */

struct mxc_w1_config {
	int search_rom_accelerator;
};
/*!
 * This structure is used to define the SPI master controller's platform
 * data. It includes the SPI  bus number and the maximum number of
 * slaves/chips it supports.
 */
struct mxc_spi_master {
	/*!
	 * SPI Master's bus number.
	 */
	unsigned int bus_num;
	/*!
	 * SPI Master's maximum number of chip selects.
	 */
	unsigned int maxchipselect;
	/*!
	 * CSPI Hardware Version.
	 */
	unsigned int spi_version;
};
#endif

#define IOMUX_TO_GPIO(pin) 	((((unsigned int)pin >> MUX_IO_P) * GPIO_NUM_PIN) + ((pin >> MUX_IO_I) & ((1 << (MUX_IO_P - MUX_IO_I)) -1)))
#define IOMUX_TO_IRQ(pin)	(MXC_GPIO_BASE + IOMUX_TO_GPIO(pin))
#define GPIO_TO_PORT(n)		(n / GPIO_NUM_PIN)
#define GPIO_TO_INDEX(n)	(n % GPIO_NUM_PIN)

/*
 *****************************************
 * EPIT  Register definitions            *
 *****************************************
 */
#ifndef EPIT1_AP_BASE_ADDR
#define EPIT1_AP_BASE_ADDR 	EPIT1_BASE_ADDR
#endif

#define MXC_EPIT_EPITCR		(IO_ADDRESS(EPIT1_AP_BASE_ADDR + 0x00))
#define MXC_EPIT_EPITSR		(IO_ADDRESS(EPIT1_AP_BASE_ADDR + 0x04))
#define MXC_EPIT_EPITLR		(IO_ADDRESS(EPIT1_AP_BASE_ADDR + 0x08))
#define MXC_EPIT_EPITCMPR	(IO_ADDRESS(EPIT1_AP_BASE_ADDR + 0x0C))
#define MXC_EPIT_EPITCNR	(IO_ADDRESS(EPIT1_AP_BASE_ADDR + 0x10))

/*
 *****************************************
 * GPT  Register definitions             *
 *****************************************
 */
#if defined(CONFIG_ARCH_MX27)
#define GPT_BASE_ADDR(x)	(GPT ##x## _BASE_ADDR)
#define MXC_GPT_TCTL(x)		(IO_ADDRESS(GPT_BASE_ADDR(x) + 0x00))
#define MXC_GPT_TPRER(x)	(IO_ADDRESS(GPT_BASE_ADDR(x) + 0x04))
#define MXC_GPT_TCMP(x)		(IO_ADDRESS(GPT_BASE_ADDR(x) + 0x08))
#define MXC_GPT_TCR(x)		(IO_ADDRESS(GPT_BASE_ADDR(x) + 0x0C))
#define MXC_GPT_TCN(x)		(IO_ADDRESS(GPT_BASE_ADDR(x) + 0x10))
#define MXC_GPT_TSTAT(x)	(IO_ADDRESS(GPT_BASE_ADDR(x) + 0x14))
#define MXC_GPT_GPTCNT		MXC_GPT_TCN(MXC_TIMER_GPT1)
#define GPT_TSTAT_COMP		(1 << 0)
#define GPT_TSTAT_CAPT		(1 << 1)
#define GPT_TCTL_TEN		(1 << 0)
#define GPT_TCTL_SRC_PER1	(1 << 1)
#define GPT_TCTL_SRC_PER1_DIV4	(2 << 1)
#define GPT_TCTL_SRC_TIN	(3 << 1)
#define GPT_TCTL_SRC_32K	(4 << 1)
#define GPT_TCTL_COMPEN		(1 << 4)
#define GPT_TCTL_CAPTEN		(1 << 5)
#define GPT_TCTL_FRR		(1 << 8)
#define GPT_TCTL_OM		(1 << 9)
#define GPT_TCTL_CC		(1 << 10)
#define GPT_TCTL_SWR		(1 << 15)
#else
#define MXC_GPT_GPTCR		(IO_ADDRESS(GPT1_BASE_ADDR + 0x00))
#define MXC_GPT_GPTPR		(IO_ADDRESS(GPT1_BASE_ADDR + 0x04))
#define MXC_GPT_GPTSR		(IO_ADDRESS(GPT1_BASE_ADDR + 0x08))
#define MXC_GPT_GPTIR		(IO_ADDRESS(GPT1_BASE_ADDR + 0x0C))
#define MXC_GPT_GPTOCR1		(IO_ADDRESS(GPT1_BASE_ADDR + 0x10))
#define MXC_GPT_GPTOCR2		(IO_ADDRESS(GPT1_BASE_ADDR + 0x14))
#define MXC_GPT_GPTOCR3		(IO_ADDRESS(GPT1_BASE_ADDR + 0x18))
#define MXC_GPT_GPTICR1		(IO_ADDRESS(GPT1_BASE_ADDR + 0x1C))
#define MXC_GPT_GPTICR2		(IO_ADDRESS(GPT1_BASE_ADDR + 0x20))
#define MXC_GPT_GPTCNT		(IO_ADDRESS(GPT1_BASE_ADDR + 0x24))
#endif				/* #if defined(CONFIG_ARCH_MX27) */

/*
 *****************************************
 * AVIC Registers                        *
 *****************************************
 */
#define AVIC_BASE		IO_ADDRESS(AVIC_BASE_ADDR)
#define AVIC_INTCNTL		(AVIC_BASE + 0x00)	/* int control reg */
#define AVIC_NIMASK		(AVIC_BASE + 0x04)	/* int mask reg */
#define AVIC_INTENNUM		(AVIC_BASE + 0x08)	/* int enable number reg */
#define AVIC_INTDISNUM		(AVIC_BASE + 0x0C)	/* int disable number reg */
#define AVIC_INTENABLEH		(AVIC_BASE + 0x10)	/* int enable reg high */
#define AVIC_INTENABLEL		(AVIC_BASE + 0x14)	/* int enable reg low */
#define AVIC_INTTYPEH		(AVIC_BASE + 0x18)	/* int type reg high */
#define AVIC_INTTYPEL		(AVIC_BASE + 0x1C)	/* int type reg low */
#define AVIC_NIPRIORITY7	(AVIC_BASE + 0x20)	/* norm int priority lvl7 */
#define AVIC_NIPRIORITY6	(AVIC_BASE + 0x24)	/* norm int priority lvl6 */
#define AVIC_NIPRIORITY5	(AVIC_BASE + 0x28)	/* norm int priority lvl5 */
#define AVIC_NIPRIORITY4	(AVIC_BASE + 0x2C)	/* norm int priority lvl4 */
#define AVIC_NIPRIORITY3	(AVIC_BASE + 0x30)	/* norm int priority lvl3 */
#define AVIC_NIPRIORITY2	(AVIC_BASE + 0x34)	/* norm int priority lvl2 */
#define AVIC_NIPRIORITY1	(AVIC_BASE + 0x38)	/* norm int priority lvl1 */
#define AVIC_NIPRIORITY0	(AVIC_BASE + 0x3C)	/* norm int priority lvl0 */
#define AVIC_NIVECSR		(AVIC_BASE + 0x40)	/* norm int vector/status */
#define AVIC_FIVECSR		(AVIC_BASE + 0x44)	/* fast int vector/status */
#define AVIC_INTSRCH		(AVIC_BASE + 0x48)	/* int source reg high */
#define AVIC_INTSRCL		(AVIC_BASE + 0x4C)	/* int source reg low */
#define AVIC_INTFRCH		(AVIC_BASE + 0x50)	/* int force reg high */
#define AVIC_INTFRCL		(AVIC_BASE + 0x54)	/* int force reg low */
#define AVIC_NIPNDH		(AVIC_BASE + 0x58)	/* norm int pending high */
#define AVIC_NIPNDL		(AVIC_BASE + 0x5C)	/* norm int pending low */
#define AVIC_FIPNDH		(AVIC_BASE + 0x60)	/* fast int pending high */
#define AVIC_FIPNDL		(AVIC_BASE + 0x64)	/* fast int pending low */

/*
 *****************************************
 * EDIO Registers                        *
 *****************************************
 */
#ifdef EDIO_BASE_ADDR
#define EDIO_EPPAR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x00)
#define EDIO_EPDDR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x02)
#define EDIO_EPDR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x04)
#define EDIO_EPFR		(IO_ADDRESS(EDIO_BASE_ADDR) + 0x06)
#endif

#define SYSTEM_PREV_REG		IO_ADDRESS(IIM_BASE_ADDR + 0x20)
#define SYSTEM_SREV_REG		IO_ADDRESS(IIM_BASE_ADDR + 0x24)
#define IIM_PROD_REV_SH		3
#define IIM_PROD_REV_LEN	5

/* DMA driver defines */
#define MXC_IDE_DMA_WATERMARK	32	/* DMA watermark level in bytes */
#define MXC_IDE_DMA_BD_NR	(512/3/4)	/* Number of BDs per channel */

#define DPTC_WP_SUPPORTED	17

#ifndef IS_MEM_DEVICE_NONSHARED
/* all peripherals on MXC so far are below 0x80000000 but leave L2CC alone */
#define IS_MEM_DEVICE_NONSHARED(x)  ((x) < 0x80000000 && (x) != L2CC_BASE_ADDR)
#endif

#ifndef __ASSEMBLY__
#include <linux/types.h>
struct dptc_wp {
	u32 dcvr0;
	u32 dcvr1;
	u32 dcvr2;
	u32 dcvr3;
	u32 regulator;
	u32 voltage;
};
struct cpu_wp {
	u32 pll_reg;
	u32 pll_rate;
	u32 cpu_rate;
	u32 pdr0_reg;
};

struct cpu_wp *get_cpu_wp(int *wp);

/*!
 * This function is called to put the DPTC in a low power state.
 *
 */
void dptc_disable(void);

/*!
 * This function is called to resume the DPTC from a low power state.
 *
 */
void dptc_enable(void);

#endif

#endif				/*  __ASM_ARCH_MXC_H__ */
