/*
 *  This driver provides API for power management.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: pwrmngt_cmds.h
 *  Creation date: 04/07/2007
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

/*!
 ***************************************************************************
 * \file pwrmngt_cmds.h
 * \brief summary
 * \ingroup DPM
 *
 * \dot
 * digraph PowerManagement {
 * 	NORMAL -> IDLE ;
 * 	IDLE -> DOZE_1  [label="5'"] ;
 * 	DOZE_1 -> STANDBY_1 [label="5'"] ;
 * 	DOZE_1 -> STANDBY_2 [label="5'"] ;
 * 	IDLE -> DOZE_2 [label="1'"] ;
 * 	DOZE_2 -> STANDBY_2 [label="1'"] ;
 *
 *    IDLE [ label="IDLE\n(no pm ioctl)" ];
 * 	DOZE_1 [ shape=polygon,sides=4,label="DOZE_1\nPower supplied" ];
 * 	DOZE_2 [ shape=polygon,sides=4,label="DOZE_2\nBattery powered" ];
 *
 * 	STANDBY_1 [ shape=polygon,sides=4,label="STANDBY_1\n(clock displayed)" ];
 * 	STANDBY_2 [ shape=polygon,sides=4,label="STANDBY_2\n(switch off display)" ];
 *
 * 	NORMAL [ shape=polygon,sides=4,label="NORMAL\n(at least 1 app is running)" ];
 * }
 * \enddot
 *
 */

#ifndef __PWRMNGT_CMDS_H__
#define __PWRMNGT_CMDS_H__
#define PWRMNGT_DEVICE_NAME "pwrmngt"

#define PWRMNGT_IOC_MAGIC 'p'                      /*!< Mangling key */


#define PWRMNGT_NORMAL		_IO(PWRMNGT_IOC_MAGIC,0)  /*!< Default mode without powermanagement */

#define PWRMNGT_DOZE		_IO(PWRMNGT_IOC_MAGIC,16) /*!< reserved default doze mode */
#define PWRMNGT_DOZE_1		_IO(PWRMNGT_IOC_MAGIC,17) /*!< first doze mode */
#define PWRMNGT_DOZE_2		_IO(PWRMNGT_IOC_MAGIC,18) /*!< second doze mode */
#define PWRMNGT_DOZE_3		_IO(PWRMNGT_IOC_MAGIC,19) /*!< reserved : communication doze mode */

#define PWRMNGT_STANDBY       _IO(PWRMNGT_IOC_MAGIC,32)  /*!< Default standby mode unecessary drivers suspend */
#define PWRMNGT_STANDBY_1		_IO(PWRMNGT_IOC_MAGIC,33) /*!< Default standby mode unecessary drivers suspend */
#define PWRMNGT_STANDBY_2		_IO(PWRMNGT_IOC_MAGIC,34) /*!< Default standby mode unecessary drivers suspend */

#define PWRMNGT_SWITCHOFF	_IO(PWRMNGT_IOC_MAGIC,64) /*!< reserved : hard machine poweroff */

#endif /* __PWRMNGT_CMDS_H__ */

