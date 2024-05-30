/*
 * Copyright 2005-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 03/11/2008 
 * Copyright (c) 2010 Sagemcom All rights reserved:
 * Added freq points and core voltage points set via sysfs in mxc_pm.c
 * Improved frequency scaling, in mxc_pm.
 * Backported kernel 2.6.24 Dynamic Process and Temperature Compensation (DPTC) and Dynamic Voltage and Frequency Scaling (DVFS) drivers, merged kernel 2.6.24 clocks initialisation and schemes (including rate propagation to children).
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
 * @defgroup DPM_MX31 Power Management
 * @ingroup MSL_MX31
 */
/*!
 * @file mach-mx3/mxc_pm.c
 *
 * @brief This file provides all the kernel level and user level API
 * definitions for the CRM_MCU and DPLL in mx3.
 *
 * @ingroup DPM_MX31
 */

/*
 * Include Files
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <asm/hardware.h>
#include <asm/arch/system.h>
#include <asm/arch/mxc_pm.h>
#include <asm/irq.h>
#include <asm/arch/pmic_power.h>

#include "crm_regs.h"
#include "../drivers/mxc/ipu/ipu_regs.h"

/* Local defines */
#define FREQ_COMP_TOLERANCE      200	/* tolerance percentage times 100 */
#define MCU_PLL_MAX_FREQ   600000000	/* Maximum frequency MCU PLL clock */
#define MCU_PLL_MIN_FREQ   160000000	/* Minimum frequency MCU PLL clock */
#define NFC_MAX_FREQ        20000000	/* Maximum frequency NFC clock */
#define PRE_DIV_MIN_FREQ    10000000	/* Minimum Frequency after Predivider */

extern unsigned long mxc_ccm_get_reg(unsigned int reg_offset);
extern void mxc_ccm_modify_reg(unsigned int reg_offset, unsigned int mask,
			       unsigned int data);

static struct clk *mcu_pll_clk;
static struct clk *cpu_clk;
static struct clk *ahb_clk;
static struct clk *ipg_clk;

/*!
 * Compare two frequences using allowable tolerance
 *
 * The MX3 PLL can generate many frequencies. This function
 * compares the generated frequency to the requested frequency
 * and determines it they are within and acceptable tolerance.
 *
 * @param   freq1  desired frequency
 * @param   freq2  generated frequency
 *
 * @return       Returns 0 is frequencies are within talerance
 *               and non-zero is they are not.
 */
static int freq_equal(unsigned long freq1, unsigned long freq2)
{
	if (freq1 > freq2) {
		return (freq1 - freq2) <= (freq1 / FREQ_COMP_TOLERANCE);
	}
	return (freq2 - freq1) <= (freq1 / FREQ_COMP_TOLERANCE);
}

/*!
 * Calculate new MCU clock dividers for the PDR0 regiser.
 *
 * @param   mcu_main_clk PLL output frequency (Hz)
 * @param   arm_freq     desired ARM frequency (Hz)
 * @param   max_freq     desired MAX frequency (Hz)
 * @param   ip_freq      desired IP frequency (Hz)
 * @param   mask         were to return PDR0 mask
 * @param   value        were to return PDR0 value
 *
 * @return             Returns 0 on success or
 *                     Returns non zero if error
 *                       PLL_LESS_ARM_ERR if pll frequency is less than
 *                       desired core frequency
 *                       FREQ_OUT_OF_RANGE if desided frequencies ar not
 *                       possible with the current mcu pll frequency.
 */
static int
cal_pdr0_value(unsigned long mcu_main_clk,
	       long arm_freq,
	       long max_freq,
	       long ip_freq, unsigned long *mask, unsigned long *value)
{
	unsigned long arm_div;	/* ARM core clock divider */
	unsigned long max_div;	/* MAX clock divider */
	unsigned long ipg_div;	/* IPG clock divider */
	unsigned long nfc_div;	/* NFC (Nand Flash Controller) clock divider */
	unsigned long hsp_div;	/* HSP clock divider */

	if (arm_freq > mcu_main_clk) {
		return -PLL_LESS_ARM_ERR;
	}

	arm_div = mcu_main_clk / arm_freq;
	if ((arm_div == 0) || !freq_equal(arm_freq, mcu_main_clk / arm_div)) {
		return FREQ_OUT_OF_RANGE;
	}
	max_div = mcu_main_clk / max_freq;
	if ((max_div == 0) || !freq_equal(max_freq, mcu_main_clk / max_div)) {
		return FREQ_OUT_OF_RANGE;
	}
	hsp_div = max_div;

	ipg_div = max_freq / ip_freq;
	if ((ipg_div == 0) || !freq_equal(ip_freq, max_freq / ipg_div)) {
		return FREQ_OUT_OF_RANGE;
	}

	nfc_div = ((max_freq - 1000000) / NFC_MAX_FREQ) + 1;

	/* All of the divider values have been calculated.
	 * Now change the hardware register. */

	*mask = MXC_CCM_PDR0_HSP_PODF_MASK |
	    MXC_CCM_PDR0_NFC_PODF_MASK |
	    MXC_CCM_PDR0_IPG_PODF_MASK |
	    MXC_CCM_PDR0_MAX_PODF_MASK | MXC_CCM_PDR0_MCU_PODF_MASK;

	*value = ((hsp_div - 1) << MXC_CCM_PDR0_HSP_PODF_OFFSET) |
	    ((nfc_div - 1) << MXC_CCM_PDR0_NFC_PODF_OFFSET) |
	    ((ipg_div - 1) << MXC_CCM_PDR0_IPG_PODF_OFFSET) |
	    ((max_div - 1) << MXC_CCM_PDR0_MAX_PODF_OFFSET) |
	    ((arm_div - 1) << MXC_CCM_PDR0_MCU_PODF_OFFSET);

	return 0;
}

/*!
 * Integer clock scaling
 *
 * Change main arm clock frequencies without changing the PLL.
 * The integer dividers are changed to produce the desired
 * frequencies. The number of valid frequency are limited and
 * are determined by the current MCU PLL frequency
 *
 * @param   arm_freq    desired ARM frequency (Hz)
 * @param   max_freq    desired MAX frequency (Hz)
 * @param   ip_freq     desired IP frequency (Hz)
 *
 * @return             Returns 0 on success or
 *                     Returns non zero if error
 *                       PLL_LESS_ARM_ERR if pll frequency is less than
 *                       desired core frequency
 *                       FREQ_OUT_OF_RANGE if desided frequencies ar not
 *                       possible with the current mcu pll frequency.
 */
int mxc_pm_intscale(long arm_freq, long max_freq, long ip_freq)
{
	unsigned long mcu_main_clk;	/* mcu clock domain main clock */
	unsigned long mask;
	unsigned long value;
	int ret_value;

	printk(KERN_INFO "arm_freq=%ld, max_freq=%ld, ip_freq=%ld\n",
	       arm_freq, max_freq, ip_freq);
	//print_frequencies();  /* debug */

	mcu_main_clk = clk_get_rate(mcu_pll_clk);
	ret_value = cal_pdr0_value(mcu_main_clk, arm_freq, max_freq, ip_freq,
				   &mask, &value);
/*	if ((arm_freq != clk_round_rate(cpu_clk, arm_freq)) ||
	    (max_freq != clk_round_rate(ahb_clk, max_freq)) ||
	    (ip_freq != clk_round_rate(ipg_clk, ip_freq))) {
		return -EINVAL;
	} */

	if ((max_freq != clk_get_rate(ahb_clk)) ||
	    (ip_freq != clk_get_rate(ipg_clk))) {
		return -EINVAL;
	}

	if (arm_freq != clk_get_rate(cpu_clk)) {
		ret_value = clk_set_rate(cpu_clk, arm_freq);
	}
	return ret_value;
}

/*!
 * PLL clock scaling
 *
 * Change MCU PLL frequency and adjust derived clocks. Integer
 * dividers are used generate the derived clocks so changed to produce
 * the desired the valid frequencies are limited by the desired ARM
 * frequency.
 *
 * The clock source for the MCU is set to the MCU PLL.
 *
 * @param   arm_freq    desired ARM frequency (Hz)
 * @param   max_freq    desired MAX frequency (Hz)
 * @param   ip_freq     desired IP frequency (Hz)
 *
 * @return             Returns 0 on success or
 *                     Returns non zero if error
 *                       PLL_LESS_ARM_ERR if pll frequency is less than
 *                       desired core frequency
 *                       FREQ_OUT_OF_RANGE if desided frequencies ar not
 *                       possible with the current mcu pll frequency.
 */
int mxc_pm_pllscale(long arm_freq, long max_freq, long ip_freq)
{
	signed long pll_freq = 0;	/* target pll frequency */
	unsigned long old_pll;
	unsigned long mask;
	unsigned long value;
	int ret_value;

	printk(KERN_INFO "arm_freq=%ld, max_freq=%ld, ip_freq=%ld\n",
	       arm_freq, max_freq, ip_freq);
	//print_frequencies();

	do {
		pll_freq += arm_freq;
		if ((pll_freq > MCU_PLL_MAX_FREQ) || (pll_freq / 8 > arm_freq)) {
			return FREQ_OUT_OF_RANGE;
		}
		if (pll_freq < MCU_PLL_MIN_FREQ) {
			ret_value = 111;
		} else {
			ret_value =
			    cal_pdr0_value(pll_freq, arm_freq, max_freq,
					   ip_freq, &mask, &value);
		}
	} while (ret_value != 0);

	old_pll = clk_get_rate(mcu_pll_clk);
	if (pll_freq > old_pll) {
		/* if pll freq is increasing then change dividers first */
		mxc_ccm_modify_reg(MXC_CCM_PDR0, mask, value);
		ret_value = clk_set_rate(mcu_pll_clk, pll_freq);
	} else {
		/* if pll freq is decreasing then change pll first */
		ret_value = clk_set_rate(mcu_pll_clk, pll_freq);
		mxc_ccm_modify_reg(MXC_CCM_PDR0, mask, value);
	}
	//print_frequencies();
	return ret_value;
}

/*!
 * Implementing steps required to transition to low-power modes
 *
 * @param   mode    The desired low-power mode. Possible values are,
 *                  WAIT_MODE, DOZE_MODE, STOP_MODE or DSM_MODE
 *
 */
void mxc_pm_lowpower(int mode)
{
	unsigned int lpm, ipu_conf;
	unsigned long reg;

	local_irq_disable();
	ipu_conf = __raw_readl(IPU_CONF);

	switch (mode) {
	case DOZE_MODE:
		lpm = 1;
		break;

	case STOP_MODE:
		/* State Retention mode */
		lpm = 2;
		__raw_writel(INT_GPT, AVIC_INTDISNUM);
		__raw_writel(INT_GPIO1, AVIC_INTDISNUM);

		/* work-around for SR mode after camera related test */
		__raw_writel(0x51, IPU_CONF);

		/* Enable Well Bias and set VSTBY
		 * VSTBY pin will be asserted during SR mode. This asks the
		 * PM IC to set the core voltage to the standby voltage
		 * Must clear the MXC_CCM_CCMR_SBYCS bit as well
		 */
		mxc_ccm_modify_reg(MXC_CCM_CCMR,
				   MXC_CCM_CCMR_WBEN | MXC_CCM_CCMR_VSTBY |
				   MXC_CCM_CCMR_SBYCS,
				   MXC_CCM_CCMR_WBEN | MXC_CCM_CCMR_VSTBY);

		mxc_ccm_modify_reg(MXC_CCM_CCMR,
				   MXC_CCM_CCMR_LPM_MASK,
				   0x2 << MXC_CCM_CCMR_LPM_OFFSET);
		break;

	case DSM_MODE:
		/* Deep Sleep Mode */
		lpm = 3;
		/* Disable timer interrupt */
		__raw_writel(INT_GPT, AVIC_INTDISNUM);
		/* Enabled Well Bias
		 * SBYCS = 0, MCU clock source is disabled*/
		mxc_ccm_modify_reg(MXC_CCM_CCMR,
				   MXC_CCM_CCMR_WBEN | MXC_CCM_CCMR_SBYCS,
				   MXC_CCM_CCMR_WBEN);
		break;

	default:
	case WAIT_MODE:
		/* Wait is the default mode used when idle. */
		lpm = 0;
		break;
	}
	reg = __raw_readl(MXC_CCM_CCMR);
	reg = (reg & (~MXC_CCM_CCMR_LPM_MASK)) | lpm << MXC_CCM_CCMR_LPM_OFFSET;
	__raw_writel(reg, MXC_CCM_CCMR);

	/* Executing CP15 (Wait-for-Interrupt) Instruction */
	/* wait for interrupt */

	/*
	 * For WFI errata TLSbo65953
	 *
	 * Without the work around this could simply be:
	 *   WFI; nop; nop; nop; nop; nop;
	 */
	__asm__ __volatile__(
		"mrc p15, 0, %0, c1, c0, 0\n"
		"bic %0, %0, #0x00001000\n"
		"bic %0, %0, #0x00000004\n"
		"mcr p15, 0, %0, c1, c0, 0\n"
		"mov %0, #0\n"
		"mcr p15, 0, %0, c7, c5, 0\n"
		"mov %0, #0\n"
		"mcr p15, 0, %0, c7, c14, 0\n"
		"mov %0, #0\n"
		"mcr p15, 0, %0, c7, c0, 4\n"
		"nop\n" "nop\n" "nop\n" "nop\n"
		"nop\n" "nop\n" "nop\n"
		"mrc p15, 0, %0, c1, c0, 0\n"
		"orr %0, %0, #0x00001000\n"
		"orr %0, %0, #0x00000004\n"
		"mcr p15, 0, %0, c1, c0, 0\n"
		:: "r" (reg));

	/* work-around for SR mode after camera related test */
	__raw_writel(ipu_conf, IPU_CONF);

	__raw_writel(INT_GPT, AVIC_INTENNUM);
	__raw_writel(INT_GPIO1, AVIC_INTENNUM);

	local_irq_enable();
}

static ssize_t mxc_pm_532_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		unsigned long arm_freq = clk_get_rate(cpu_clk);

		switch(arm_freq)
		{
			case 532000000:
				break;

			default:
				mxc_pm_intscale(532000000, 133000000, 66500000);
		}
	}

	return size;
}

static ssize_t mxc_pm_399_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		unsigned long arm_freq = clk_get_rate(cpu_clk);

		switch(arm_freq)
		{
			case 133000000:
			{
				mxc_pm_intscale(266000000, 133000000, 66500000);
				mxc_pm_intscale(399000000, 133000000, 66500000);
				break;
			}

			case 266000000:
			{
				mxc_pm_intscale(399000000, 133000000, 66500000);
				break;
			}

			case 399000000:
				break;

			case 532000000:
			{
				mxc_pm_intscale(399000000, 133000000, 66500000);
				break;
			}
		}
	}

	return size;
}

static ssize_t mxc_pm_266_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		unsigned long arm_freq = clk_get_rate(cpu_clk);

		switch(arm_freq)
		{
			case 133000000:
			{
				mxc_pm_intscale(266000000, 133000000, 66500000);
				break;
			}

			case 266000000:
				break;

			case 399000000:
			{
				mxc_pm_intscale(266000000, 133000000, 66500000);
				break;
			}

			case 532000000:
			{
				mxc_pm_intscale(266000000, 133000000, 66500000);
				break;
			}
		}
	}

	return size;
}

static ssize_t mxc_pm_133_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		unsigned long arm_freq = clk_get_rate(cpu_clk);

		switch(arm_freq)
		{
			case 133000000:
				break;

			case 266000000:
			{
				mxc_pm_intscale(133000000, 133000000, 66500000);
				break;
			}

			case 399000000:
			{
				mxc_pm_intscale(266000000, 133000000, 66500000);
				mxc_pm_intscale(133000000, 133000000, 66500000);
				break;
			}

			case 532000000:
			{
				mxc_pm_intscale(266000000, 133000000, 66500000);
				mxc_pm_intscale(133000000, 133000000, 66500000);
				break;
			}
		}
	}

	return size;
}

static ssize_t mxc_pm_1_625V_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		t_regulator_voltage volt, voltage;

		pmic_power_regulator_get_voltage(SW_SW1A, &voltage);
		if (voltage.sw1a == SW1A_1_625V)
			return size;
		if (voltage.sw1a < SW1A_1_625V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a <= SW1A_1_625V; volt.sw1a++)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
		else if (voltage.sw1a > SW1A_1_625V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a >= SW1A_1_625V; volt.sw1a--)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
	}

	return size;
}

static ssize_t mxc_pm_1_5V_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		t_regulator_voltage volt, voltage;

		pmic_power_regulator_get_voltage(SW_SW1A, &voltage);
		if (voltage.sw1a == SW1A_1_5V)
			return size;
		if (voltage.sw1a < SW1A_1_5V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a <= SW1A_1_5V; volt.sw1a++)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
		else if (voltage.sw1a > SW1A_1_5V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a >= SW1A_1_5V; volt.sw1a--)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
	}

	return size;
}

static ssize_t mxc_pm_1_375V_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		t_regulator_voltage volt, voltage;

		pmic_power_regulator_get_voltage(SW_SW1A, &voltage);
		if (voltage.sw1a == SW1A_1_375V)
			return size;
		if (voltage.sw1a < SW1A_1_375V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a <= SW1A_1_375V; volt.sw1a++)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
		else if (voltage.sw1a > SW1A_1_375V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a >= SW1A_1_375V; volt.sw1a--)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
	}

	return size;
}

static ssize_t mxc_pm_1_25V_store(struct sys_device *dev, const char *buf,
				 size_t size)
{
	if (strstr(buf, "1") != NULL)
	{
		t_regulator_voltage volt, voltage;

		pmic_power_regulator_get_voltage(SW_SW1A, &voltage);
		if (voltage.sw1a == SW1A_1_25V)
			return size;
		if (voltage.sw1a < SW1A_1_25V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a <= SW1A_1_25V; volt.sw1a++)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
		else if (voltage.sw1a > SW1A_1_25V)
			for(volt.sw1a = voltage.sw1a; volt.sw1a >= SW1A_1_25V; volt.sw1a--)
			{
				pmic_power_regulator_set_voltage(SW_SW1A, volt);
				mdelay(5); /* 25mV each 5ms */
			}
	}

	return size;
}

static SYSDEV_ATTR(_532, 0200, NULL, mxc_pm_532_store);
static SYSDEV_ATTR(_399, 0200, NULL, mxc_pm_399_store);
static SYSDEV_ATTR(_266, 0200, NULL, mxc_pm_266_store);
static SYSDEV_ATTR(_133, 0200, NULL, mxc_pm_133_store);
static SYSDEV_ATTR(_1_625V, 0200, NULL, mxc_pm_1_625V_store);
static SYSDEV_ATTR(_1_5V, 0200, NULL, mxc_pm_1_5V_store);
static SYSDEV_ATTR(_1_375V, 0200, NULL, mxc_pm_1_375V_store);
static SYSDEV_ATTR(_1_25V, 0200, NULL, mxc_pm_1_25V_store);

static struct sysdev_class mxc_pm_sysclass = {
	set_kset_name("mxc_pm"),
};

static struct sys_device mxc_pm_device = {
	.id = 0,
	.cls = &mxc_pm_sysclass,
};

static int mxc_pm_sysdev_ctrl_init(void)
{
	int err;

	err = sysdev_class_register(&mxc_pm_sysclass);
	if (!err)
		err = sysdev_register(&mxc_pm_device);
	if (!err) {
		err = sysdev_create_file(&mxc_pm_device, &attr__532);
		err = sysdev_create_file(&mxc_pm_device, &attr__399);
		err = sysdev_create_file(&mxc_pm_device, &attr__266);
		err = sysdev_create_file(&mxc_pm_device, &attr__133);
		err = sysdev_create_file(&mxc_pm_device, &attr__1_625V);
		err = sysdev_create_file(&mxc_pm_device, &attr__1_5V);
		err = sysdev_create_file(&mxc_pm_device, &attr__1_375V);
		err = sysdev_create_file(&mxc_pm_device, &attr__1_25V);
	}

	return err;
}

static void mxc_pm_sysdev_ctrl_exit(void)
{
	sysdev_remove_file(&mxc_pm_device, &attr__532);
	sysdev_remove_file(&mxc_pm_device, &attr__399);
	sysdev_remove_file(&mxc_pm_device, &attr__266);
	sysdev_remove_file(&mxc_pm_device, &attr__133);
	sysdev_remove_file(&mxc_pm_device, &attr__1_625V);
	sysdev_remove_file(&mxc_pm_device, &attr__1_5V);
	sysdev_remove_file(&mxc_pm_device, &attr__1_375V);
	sysdev_remove_file(&mxc_pm_device, &attr__1_25V);
	sysdev_unregister(&mxc_pm_device);
	sysdev_class_unregister(&mxc_pm_sysclass);
}

/*!
 * This function is used to load the module.
 *
 * @return   Returns an Integer on success
 */
static int __init mxc_pm_init_module(void)
{
	int err;

	printk(KERN_INFO "Low-Level PM Driver module loaded\n");

	mcu_pll_clk = clk_get(NULL, "mcu_pll");
	cpu_clk = clk_get(NULL, "cpu_clk");
	ahb_clk = clk_get(NULL, "ahb_clk");
	ipg_clk = clk_get(NULL, "ipg_clk");

	err = mxc_pm_sysdev_ctrl_init();
	if (err) {
		printk(KERN_ERR
		       "MXC_PM: Unable to register sysdev entry for mxc_pm");
		return err;
	}
	return 0;
}

/*!
 * This function is used to unload the module
 */
static void __exit mxc_pm_cleanup_module(void)
{
	mxc_pm_sysdev_ctrl_exit();
	clk_put(mcu_pll_clk);
	clk_put(cpu_clk);
	clk_put(ahb_clk);
	clk_put(ipg_clk);
	printk(KERN_INFO "Low-Level PM Driver module Unloaded\n");
}

module_init(mxc_pm_init_module);
module_exit(mxc_pm_cleanup_module);

EXPORT_SYMBOL(mxc_pm_intscale);
EXPORT_SYMBOL(mxc_pm_pllscale);
EXPORT_SYMBOL(mxc_pm_lowpower);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MX3 Low-level Power Management Driver");
MODULE_LICENSE("GPL");
