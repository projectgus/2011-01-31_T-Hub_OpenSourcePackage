/*
 *  API of battery driver
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: batt_ch.h
 *  Creation date: 13/11/2007
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

#define BATT_CH_IOC_MAGIC 'i'

/*!
 * Set charger shutdown.
 * Argument type: pointer to unsigned char.
 */
#define SET_CHG_SHDN    _IOW(BATT_CH_IOC_MAGIC, 1, unsigned char)

/*!
 * Get charger shutdown.
 * Argument type: pointer to unsigned char.
 */
#define GET_CHG_SHDN    _IOW(BATT_CH_IOC_MAGIC, 2, unsigned char)

/*!
 * Set chem.
 * Argument type: pointer to unsigned char.
 */
#define SET_CHEM    _IOW(BATT_CH_IOC_MAGIC, 3, unsigned char)

/*!
 * Get chem.
 * Argument type: pointer to unsigned char.
 */
#define GET_CHEM    _IOW(BATT_CH_IOC_MAGIC, 4, unsigned char)

/*!
 * Get charger monitoring.
 * Argument type: pointer to an array of 8 unsigned shorts.
 */
#define GET_CHG_MON    _IOR(BATT_CH_IOC_MAGIC, 5, unsigned short)

/*!
 * Get Vinbat.
 * Argument type: pointer to an array of 8 unsigned shorts.
 */
#define GET_VINBAT    _IOR(BATT_CH_IOC_MAGIC, 6, unsigned short)

/*!
 * Get Voutbat.
 * Argument type: pointer to an array of 8 unsigned shorts.
 */
#define GET_VOUTBAT    _IOR(BATT_CH_IOC_MAGIC, 7, unsigned short)

/*!
 * Get BP.
 * Argument type: pointer to an array of 8 unsigned shorts.
 */
#define GET_BP    _IOR(BATT_CH_IOC_MAGIC, 8, unsigned short)

/*!
 * Get BATT_THERM.
 * Argument type: pointer to an array of 8 unsigned shorts.
 */
#define GET_BATT_THERM    _IOR(BATT_CH_IOC_MAGIC, 9, unsigned short)

/*!
 * Get 3V3.
 * Argument type: pointer to an array of 8 unsigned shorts.
 */
#define GET_3V3    _IOR(BATT_CH_IOC_MAGIC, 10, unsigned short)

/*!
 * Get IGEN.
 * Argument type: pointer to an array of 8 unsigned shorts.
 */
#define GET_IGEN    _IOR(BATT_CH_IOC_MAGIC, 11, unsigned short)
