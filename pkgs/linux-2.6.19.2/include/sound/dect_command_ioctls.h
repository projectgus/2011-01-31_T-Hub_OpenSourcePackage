/*
 *  This file expose API to control DECT driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dect_command_ioctls.h
 *  Creation date: 02/03/2010
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

/* IOCTL list of the DECT command driver */

#ifndef DECT_COMMAND_IOCTL
#define DECT_COMMAND_IOCTL




enum {
	IOCTL_SET_DEBUG_LEVEL,                       /* configure log level for debug */
	IOCTL_DECT_AUDIO_OPEN,
	IOCTL_DECT_AUDIO_CLOSE,
	IOCTL_DECT_AUDIO_RECORD_REGISTER_CALLBACK,
	IOCTL_DECT_AUDIO_PLAY_REGISTER_CALLBACK,
	IOCTL_DECT_AUDIO_RECORD_START,
	IOCTL_DECT_AUDIO_PLAY_START,
	IOCTL_DECT_AUDIO_DEBUG_LEVEL,
	IOCTL_SET_READ_TIMEOUT = 10,
	IOCTL_SET_WRITE_TIMEOUT,
	IOCTL_RESET_USB_DEVICE,                      /* Reset USB Device */
	IOCTL_POWER_ON_USB_DEVICE,                   /* Power On USB Device */
	IOCTL_POWER_OFF_USB_DEVICE,                  /* Power Off USB Device */
} ENUMS_IOCTL;

#endif
