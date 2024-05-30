/*
 *  This file expose API to control DECT audio bridge.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dect_command_ioctls.h
 *  Creation date: 17/04/2009
 *  Author: Farid Hammane, Sagemcom
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

#ifndef AUDIO_BRIDGE_4_HANDSET
#define AUDIO_BRIDGE_4_PMIC_TO_HANDSET


#define AUDIO_BRIDGE_MODULE_NAME	"Audio_Bridge" 


#define IOCTL_AUDIO_BRIDGE_OPEN_DECT_CAPTURE 		0x4
#define IOCTL_AUDIO_BRIDGE_CLOSE_DECT_CAPTURE		0x5
#define IOCTL_AUDIO_BRIDGE_START_DECT_CAPTURE		0x6
#define IOCTL_AUDIO_BRIDGE_STOP_DECT_CAPTURE		0x7

#define IOCTL_AUDIO_BRIDGE_OPEN_PMIC_CAPTURE 		0x14
#define IOCTL_AUDIO_BRIDGE_CLOSE_PMIC_CAPTURE		0x15
#define IOCTL_AUDIO_BRIDGE_START_PMIC_CAPTURE		0x16
#define IOCTL_AUDIO_BRIDGE_STOP_PMIC_CAPTURE		0x17

#define IOCTL_AUDIO_BRIDGE_START 			0xa1
#define IOCTL_AUDIO_BRIDGE_STOP				0xa2

#define IOCTL_AUDIO_BRIDGE_GET_RELAY_BUFFER_SIZE	0xa3

#endif

