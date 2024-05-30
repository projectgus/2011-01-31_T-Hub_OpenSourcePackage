/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 04/07/2007 
 * Copyright (c) 2010 Sagemcom All rights reserved.
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
 * @file pmic_external.c
 * @brief This file contains all external functions of PMIC drivers.
 *
 * @ingroup PMIC_CORE
 */

/*
 * Includes
 */
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_status.h>

#include "pmic_config.h"

//SAGEM
#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_ATLAS)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

/*
 * External Functions
 */
extern int pmic_read(int reg_num, unsigned int *reg_val);
extern int pmic_write(int reg_num, const unsigned int reg_val);

/*!
 * This function is called by PMIC clients to read a register on PMIC.
 *
 * @param        reg        number of register
 * @param        reg_value  return value of register
 * @param        reg_mask   Bitmap mask indicating which bits to modify
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_read_reg(int reg, unsigned int *reg_value,
			  unsigned int reg_mask)
{
	int ret = 0;
	unsigned int temp = 0;

	ret = pmic_read(reg, &temp);
	if (ret != PMIC_SUCCESS) {
		return PMIC_ERROR;
	}
	*reg_value = (temp & reg_mask);

#ifdef DO_ULOG
	ulog(ULOG_ATLAS_READ_REG,reg);
	ulog(ULOG_ATLAS_READ_REG_VALUE,*reg_value);
	ulog(ULOG_ATLAS_READ_REG_MASK,reg_mask);
#endif
	return ret;
}

/*!
 * This function is called by PMIC clients to write a register on PMIC.
 *
 * @param        reg        number of register
 * @param        reg_value  New value of register
 * @param        reg_mask   Bitmap mask indicating which bits to modify
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_write_reg(int reg, unsigned int reg_value,
			   unsigned int reg_mask)
{
	int ret = 0;
	unsigned int temp = 0;

	ret = pmic_read(reg, &temp);
	if (ret != PMIC_SUCCESS) {
		return PMIC_ERROR;
	}
	temp = (temp & (~reg_mask)) | reg_value;
	ret = pmic_write(reg, temp);
	if (ret != PMIC_SUCCESS) {
		return PMIC_ERROR;
	}

#ifdef DO_ULOG
	ulog(ULOG_ATLAS_WRITE_REG,reg);
	ulog(ULOG_ATLAS_WRITE_REG_VALUE,reg_value);
	ulog(ULOG_ATLAS_WRITE_REG_MASK,reg_mask);
#endif

	return ret;
}

EXPORT_SYMBOL(pmic_read_reg);
EXPORT_SYMBOL(pmic_write_reg);
