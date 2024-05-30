/*
 *  Header file of register of IMX read/write module.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: atlas_imx_regs.h
 *  Creation date: 21/07/2007
 *  Author: Olivier Le Roy, Sagemcom
 *
 *  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General
 *  Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *  Write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA to
 *  receive a copy of the GNU General Public License.
 *  This Copyright notice should not be removed
 */

#include <asm/ioctl.h>

#define ATLAS_IMX_IOC_MAGIC 'a'

/*!
 * Set i.MX31 register.
 * Argument type: pointer to unsigned long.
 */
#define SET_IMX31_REGISTER    _IOW(ATLAS_IMX_IOC_MAGIC, 1, unsigned long)

/*!
 * Get i.MX31 register.
 * Argument type: pointer to unsigned long.
 */
#define GET_IMX31_REGISTER    _IOR(ATLAS_IMX_IOC_MAGIC, 2, unsigned long)

/*!
 * Set ATLAS register.
 * Argument type: pointer to unsigned long.
 */
#define SET_ATLAS_REGISTER    _IOW(ATLAS_IMX_IOC_MAGIC, 3, unsigned long)

/*!
 * Get ATLAS register.
 * Argument type: pointer to unsigned long.
 */
#define GET_ATLAS_REGISTER    _IOR(ATLAS_IMX_IOC_MAGIC, 4, unsigned long)
