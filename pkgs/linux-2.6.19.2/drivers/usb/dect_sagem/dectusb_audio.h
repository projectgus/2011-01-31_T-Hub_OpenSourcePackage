/*
 *  DECT USB driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dectusb_audio.h
 *  Creation date: 10/07/2008
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


#ifndef DECT_USB_AUDIO_H
#define DECT_USB_AUDIO_H

typedef void (*elapsed_t)(void *p_UserCtx, int error, unsigned int count);

extern struct dectusb_audio_ops mxc_dectusb_operations;
extern int mxc_dectusb_capture_register_elased(void *handle, elapsed_t capture_elapsed);
extern int mxc_dectusb_playback_register_elased(void *handle, elapsed_t playback_elapsed);
extern void *mxc_dectusb_get_handle(void);


struct dectusb_audio_ops {
	int (*playback_open) (void *private_data);
	void (*playback_close) (void *private_data);
	int (*playback_start) (void *private_data, void *src, int period_size, void *user_ctx, elapsed_t callback);
	int (*playback_stop) (void *private_data);
	
	int (*capture_open) (void *private_data);
	void (*capture_close) (void *private_data);	
	int (*capture_start) (void *private_data, void *src, int period_size, void *user_ctx, elapsed_t callback);
	int (*capture_stop) (void *private_data);

	void *private_data;
};

#endif
