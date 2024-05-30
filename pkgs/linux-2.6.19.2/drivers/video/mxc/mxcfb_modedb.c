/*
 * Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 17/07/2007 
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

#include <linux/kernel.h>
#include <asm/arch/mxcfb.h>

#define FB_FLMPOL_ACT_HIGH      0x00800000
#define FB_LPPOL_ACT_HIGH       0x00400000
#define FB_CLKPOL_ACT_POS       0x00200000

const struct fb_videomode mxcfb_modedb[] = {
	[0] = {
	       /* 240x320 @ 60 Hz */
	       "Sharp-QVGA", 60, 240, 320, 185925, 9, 16, 7, 9, 1, 1,
	       FB_SYNC_HOR_HIGH_ACT | FB_SYNC_SHARP_MODE |
	       FB_SYNC_CLK_INVERT | FB_SYNC_DATA_INVERT | FB_SYNC_CLK_IDLE_EN,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[1] = {
	       /* 640x480 @ 60 Hz */
	       "NEC-VGA", 60, 640, 480, 38255, 144, 0, 34, 40, 1, 1,
	       FB_SYNC_VERT_HIGH_ACT | FB_SYNC_OE_ACT_HIGH,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[2] = {
	       /* NTSC TV output */
	       "TV-NTSC", 60, 640, 480, 37538,
	       38, 858 - 640 - 38 - 3,
	       36, 518 - 480 - 36 - 1,
	       3, 1,
	       0,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[3] = {
	       /* PAL TV output */
	       "TV-PAL", 50, 640, 480, 37538,
	       38, 960 - 640 - 38 - 32,
	       32, 555 - 480 - 32 - 3,
	       32, 3,
	       0,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[4] = {
	       /* TV output VGA mode, 640x480 @ 65 Hz */
	       "TV-VGA", 60, 640, 480, 40574, 35, 45, 9, 1, 46, 5,
	       0, FB_VMODE_NONINTERLACED, 0,
	       },
	[5] = {
	       /* Samsung WVGA, 800x480 @ 60 Hz */
	       "Samsung-WVGA", 60, 800, 480, 33834, 15, 74, 32, 32, 63, 3,
//	       FB_SYNC_OE_ACT_HIGH|FB_SYNC_DATA_INVERT|FB_SYNC_CLK_IDLE_EN|FB_FLMPOL_ACT_HIGH|FB_LPPOL_ACT_HIGH|FB_CLKPOL_ACT_POS,
	       FB_SYNC_OE_ACT_HIGH|FB_SYNC_CLK_IDLE_EN|FB_FLMPOL_ACT_HIGH|FB_LPPOL_ACT_HIGH|FB_CLKPOL_ACT_POS,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[6] = {
			/* Samsung Thin-WVGA, 800x480 @ 60 Hz */
			.name="Samsung-WVGA-slim",
			.refresh=50, /*60*/
			.xres=800,
			.yres=480, 
			.pixclock= 45113, /*37594*/
			.left_margin=15,
			.right_margin=11,
			.upper_margin=8,
			.lower_margin=8,
			.hsync_len=63,
			.vsync_len=3,
			.sync=FB_SYNC_OE_ACT_HIGH|FB_FLMPOL_ACT_HIGH|FB_LPPOL_ACT_HIGH|FB_CLKPOL_ACT_POS,
			.vmode=FB_VMODE_NONINTERLACED,
			.flag=0
		},
	[7] = {
			/* Samsung Thin-WVGA, 800x480 @ 60 Hz */
			.name="DataImage-WVGA-slim",
			.refresh=50, /*60*/
			.xres=800,
			.yres=480, 
			.pixclock=45113, /*37594*/
			.left_margin=36,
			.right_margin=30,
			.upper_margin=8,
			.lower_margin=8,
			.hsync_len=63,
			.vsync_len=3,
			.sync=FB_SYNC_OE_ACT_HIGH|FB_FLMPOL_ACT_HIGH|FB_LPPOL_ACT_HIGH|FB_CLKPOL_ACT_POS,
			.vmode=FB_VMODE_NONINTERLACED,
			.flag=0
		},

};

int mxcfb_modedb_sz = ARRAY_SIZE(mxcfb_modedb);
EXPORT_SYMBOL(mxcfb_modedb);
EXPORT_SYMBOL(mxcfb_modedb_sz);
