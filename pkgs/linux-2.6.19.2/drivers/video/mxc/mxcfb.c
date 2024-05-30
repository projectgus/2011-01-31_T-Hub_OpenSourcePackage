/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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

/*!
 * @defgroup Framebuffer Framebuffer Driver for SDC and ADC.
 */

/*!
 * @file mxcfb.c
 *
 * @brief MXC Frame buffer driver for SDC
 *
 * @ingroup Framebuffer
 */
#define DEBUG 1
#define VERBOSE 1
/*!
 * Include files
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/console.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/arch/ipu.h>
#include <asm/arch/mxcfb.h>

//SAGEM
#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_V4L2OUT)
#include <ulog/ulog.h>
#define DO_ULOG
#endif


/*
 * Driver name
 */
#define MXCFB_NAME      "mxc_sdc_fb"
/*!
 * Structure containing the MXC specific framebuffer information.
 */
struct mxcfb_info {
	int blank;
	ipu_channel_t ipu_ch;
	uint32_t ipu_ch_irq;
	uint32_t cur_ipu_buf;

	u32 pseudo_palette[16];

	struct semaphore flip_sem;
	spinlock_t fb_lock;
};

struct mxcfb_data {
	struct fb_info *fbi;
	struct fb_info *fbi_ovl;
	volatile int32_t vsync_flag;
	wait_queue_head_t vsync_wq;
	wait_queue_head_t suspend_wq;
	bool suspended;
	int backlight_level;
};

static struct mxcfb_data mxcfb_drv_data;

static char *fb_mode = NULL;
static unsigned long default_bpp = 16;

static uint32_t bpp_to_pixfmt(int bpp)
{
	uint32_t pixfmt = 0;
	switch (bpp) {
	case 24:
		pixfmt = IPU_PIX_FMT_BGR24;
		break;
	case 32:
		pixfmt = IPU_PIX_FMT_BGR32;
		break;
	case 16:
		pixfmt = IPU_PIX_FMT_RGB565;
		break;
	}
	return pixfmt;
}

#ifdef CONFIG_FB_MXC_TVOUT
#include <linux/video_encoder.h>
/*
 * FIXME: VGA mode is not defined by video_encoder.h
 * while FS453 supports VGA output.
 */
#ifndef VIDEO_ENCODER_VGA
#define VIDEO_ENCODER_VGA	32
#endif

#define MODE_PAL		"TV-PAL"
#define MODE_NTSC		"TV-NTSC"
#define MODE_VGA		"TV-VGA"

extern int fs453_ioctl(unsigned int cmd, void *arg);
#endif
static irqreturn_t mxcfb_irq_handler(int irq, void *dev_id);
static int mxcfb_blank(int blank, struct fb_info *info);
static int mxcfb_map_video_memory(struct fb_info *fbi, bool use_internal_ram);
static int mxcfb_unmap_video_memory(struct fb_info *fbi);

/*
 * Set fixed framebuffer parameters based on variable settings.
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_set_fix(struct fb_info *info)
{
	struct fb_fix_screeninfo *fix = &info->fix;
	struct fb_var_screeninfo *var = &info->var;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;

	if (mxc_fbi->ipu_ch == MEM_SDC_FG)
		strncpy(fix->id, "DISP3 FG", 8);
	else
		strncpy(fix->id, "DISP3 BG", 8);

	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;

	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->xpanstep = 1;
	fix->ypanstep = 1;

	return 0;
}

/*
 * Set framebuffer parameters and change the operating mode.
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_set_par(struct fb_info *fbi)
{
	int retval, rgb_pix_format;
	bool use_iram = false;
	u32 mem_len;
	ipu_di_signal_cfg_t sig_cfg;
	ipu_panel_t mode = IPU_PANEL_TFT;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	ipu_disable_irq(mxc_fbi->ipu_ch_irq);
	ipu_disable_channel(mxc_fbi->ipu_ch, true);
	ipu_uninit_channel(mxc_fbi->ipu_ch);
	ipu_clear_irq(mxc_fbi->ipu_ch_irq);
	mxcfb_set_fix(fbi);

	mem_len = fbi->var.yres_virtual * fbi->fix.line_length;
	if (mem_len > fbi->fix.smem_len) {
		if (fbi->fix.smem_start)
			mxcfb_unmap_video_memory(fbi);

		if (mxcfb_map_video_memory(fbi, use_iram) < 0)
			return -ENOMEM;
	}

	ipu_init_channel(mxc_fbi->ipu_ch, NULL);

	if (mxc_fbi->ipu_ch == MEM_SDC_BG) {
		memset(&sig_cfg, 0, sizeof(sig_cfg));
		if (fbi->var.sync & FB_SYNC_HOR_HIGH_ACT)
			sig_cfg.Hsync_pol = true;
		if (fbi->var.sync & FB_SYNC_VERT_HIGH_ACT)
			sig_cfg.Vsync_pol = true;
		if (fbi->var.sync & FB_SYNC_CLK_INVERT)
			sig_cfg.clk_pol = true;
		if (fbi->var.sync & FB_SYNC_DATA_INVERT)
			sig_cfg.data_pol = true;
		if (fbi->var.sync & FB_SYNC_OE_ACT_HIGH)
			sig_cfg.enable_pol = true;
		if (fbi->var.sync & FB_SYNC_CLK_IDLE_EN)
			sig_cfg.clkidle_en = true;
		if (fbi->var.sync & FB_SYNC_SHARP_MODE)
			mode = IPU_PANEL_SHARP_TFT;

		dev_dbg(fbi->device, "pixclock = %ul Hz\n",
			(u32) (PICOS2KHZ(fbi->var.pixclock) * 1000UL));

        if (fbi->var.sync & FB_SYNC_RGB_565)
            rgb_pix_format = IPU_PIX_FMT_RGB565;
        else
            if (fbi->var.sync & FB_SYNC_SWAP_RGB)
                rgb_pix_format = IPU_PIX_FMT_BGR666;
            else
                rgb_pix_format = IPU_PIX_FMT_RGB666;  

		if (ipu_sdc_init_panel(mode,
				       (PICOS2KHZ(fbi->var.pixclock)) * 1000UL,
				       fbi->var.xres, fbi->var.yres,
				       rgb_pix_format,
				       fbi->var.left_margin,
				       fbi->var.hsync_len,
				       fbi->var.right_margin +
				       fbi->var.hsync_len,
				       fbi->var.upper_margin,
				       fbi->var.vsync_len,
				       fbi->var.lower_margin +
				       fbi->var.vsync_len, sig_cfg) != 0) {
			dev_err(fbi->device,
				"mxcfb: Error initializing panel.\n");
			return -EINVAL;
		}
	}

	ipu_sdc_set_window_pos(mxc_fbi->ipu_ch, 0, 0);

	mxc_fbi->cur_ipu_buf = 0;
	sema_init(&mxc_fbi->flip_sem, 1);

	retval = ipu_init_channel_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER,
				bpp_to_pixfmt(fbi->var.bits_per_pixel),
				fbi->var.xres, fbi->var.yres,
				fbi->var.xres_virtual,
				IPU_ROTATE_NONE,
					 fbi->fix.smem_start,
					 fbi->fix.smem_start +
					 (fbi->fix.line_length * fbi->var.yres),
					 0, 0);
	if (retval) {
		dev_err(fbi->device,
			"ipu_init_channel_buffer error %d\n", retval);
		return retval;
	}

	ipu_select_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER, 0);
	if (mxc_fbi->blank == FB_BLANK_UNBLANK) {
		ipu_enable_channel(mxc_fbi->ipu_ch);
	}

	return 0;
}

/*
 * Check framebuffer variable parameters and adjust to valid values.
 *
 * @param       var      framebuffer variable parameters
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	u32 vtotal;
	u32 htotal;

	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

	if ((var->bits_per_pixel != 32) && (var->bits_per_pixel != 24) &&
	    (var->bits_per_pixel != 16)) {
		var->bits_per_pixel = default_bpp;
	}

	switch (var->bits_per_pixel) {
	case 16:
		var->red.length = 5;
		var->red.offset = 11;
		var->red.msb_right = 0;

		var->green.length = 6;
		var->green.offset = 5;
		var->green.msb_right = 0;

		var->blue.length = 5;
		var->blue.offset = 0;
		var->blue.msb_right = 0;

		var->transp.length = 0;
		var->transp.offset = 0;
		var->transp.msb_right = 0;
		break;
	case 24:
		var->red.length = 8;
		var->red.offset = 16;
		var->red.msb_right = 0;

		var->green.length = 8;
		var->green.offset = 8;
		var->green.msb_right = 0;

		var->blue.length = 8;
		var->blue.offset = 0;
		var->blue.msb_right = 0;

		var->transp.length = 0;
		var->transp.offset = 0;
		var->transp.msb_right = 0;
		break;
	case 32:
		var->red.length = 8;
		var->red.offset = 16;
		var->red.msb_right = 0;

		var->green.length = 8;
		var->green.offset = 8;
		var->green.msb_right = 0;

		var->blue.length = 8;
		var->blue.offset = 0;
		var->blue.msb_right = 0;

		var->transp.length = 8;
		var->transp.offset = 24;
		var->transp.msb_right = 0;
		break;
	}

	if (var->pixclock < 1000) {
		htotal = var->xres + var->right_margin + var->hsync_len +
		    var->left_margin;
		vtotal = var->yres + var->lower_margin + var->vsync_len +
		    var->upper_margin;
		var->pixclock = (vtotal * htotal * 6UL) / 100UL;
		var->pixclock = KHZ2PICOS(var->pixclock);
		dev_dbg(info->device,
			"pixclock set for 60Hz refresh = %u ps\n",
			var->pixclock);
	}

	var->height = -1;
	var->width = -1;
	var->grayscale = 0;

	/* Copy nonstd field to/from sync for fbset usage */
	var->sync |= var->nonstd;
	var->nonstd |= var->sync;

	return 0;
}

static inline u_int _chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}
static int
mxcfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int trans, struct fb_info *fbi)
{
	unsigned int val;
	int ret = 1;

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no matter what visual we are using.
	 */
	if (fbi->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green +
				      7471 * blue) >> 16;
	switch (fbi->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/*
		 * 16-bit True Colour.  We encode the RGB value
		 * according to the RGB bitfield information.
		 */
		if (regno < 16) {
			u32 *pal = fbi->pseudo_palette;

			val = _chan_to_field(red, &fbi->var.red);
			val |= _chan_to_field(green, &fbi->var.green);
			val |= _chan_to_field(blue, &fbi->var.blue);

			pal[regno] = val;
			ret = 0;
		}
		break;

	case FB_VISUAL_STATIC_PSEUDOCOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		break;
	}

	return ret;
}

/*
 * Function to handle custom ioctls for MXC framebuffer.
 *
 * @param       inode   inode struct
 *
 * @param       file    file struct
 *
 * @param       cmd     Ioctl command to handle
 *
 * @param       arg     User pointer to command arguments
 *
 * @param       fbi     framebuffer information pointer
 */
static int mxcfb_ioctl(struct fb_info *fbi, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_MXCFB_IOCTL, 0) ;
#endif
	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	switch (cmd) {
	case MXCFB_WAIT_FOR_VSYNC:
		{
#ifndef CONFIG_ARCH_MX3
			mxcfb_drv_data.vsync_flag = 0;
			ipu_enable_irq(IPU_IRQ_SDC_DISP3_VSYNC);
			if (!wait_event_interruptible_timeout
			    (mxcfb_drv_data.vsync_wq,
			     mxcfb_drv_data.vsync_flag != 0, 1 * HZ)) {
				dev_err(fbi->device,
					"MXCFB_WAIT_FOR_VSYNC: timeout\n");
				retval = -ETIME;
				break;
			} else if (signal_pending(current)) {
				dev_err(fbi->device,
					"MXCFB_WAIT_FOR_VSYNC: interrupt received\n");
				retval = -ERESTARTSYS;
				break;
			}
#endif
			break;
		}
	case MXCFB_SET_BRIGHTNESS:
		{
			uint8_t level;
			if (copy_from_user(&level, (void *)arg, sizeof(level))) {
				retval = -EFAULT;
				break;
			}

			mxcfb_drv_data.backlight_level = level;
			retval = ipu_sdc_set_brightness(level);
			dev_dbg(fbi->device, "Set brightness to %d\n", level);
			break;
		}
	case MXCFB_GET_BRIGHTNESS:
		{
			if (copy_to_user((void *)arg, &mxcfb_drv_data.backlight_level, sizeof(mxcfb_drv_data.backlight_level))) {
				retval = -EFAULT;
				break;
			}
			dev_dbg(fbi->device, "get brightness is %d\n", mxcfb_drv_data.backlight_level);
			break;
		}
	case MXCFB_SET_BLANK:
		{
			uint8_t blank;
			if (copy_from_user(&blank, (void *)arg, sizeof(blank))) {
				retval = -EFAULT;
				break;
			}
			if (mxcfb_blank(blank,fbi) != 0)
			{
				retval = -ENODEV;
				break;
			}
			dev_dbg(fbi->device, "Set blank to %d\n", blank);
			break;
		}
	case MXCFB_GET_BLANK:
		{
			struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;
			if (copy_to_user((void *)arg, &(mxc_fbi->blank), sizeof(mxc_fbi->blank))) {
				retval = -EFAULT;
				break;
			}
			dev_dbg(fbi->device, "get blank is %d\n", mxc_fbi->blank);
			break;
		}
#ifdef CONFIG_FB_MXC_TVOUT
	case ENCODER_GET_CAPABILITIES:
		{
			struct video_encoder_capability cap;

			if ((retval = fs453_ioctl(cmd, &cap)))
				break;

			if (copy_to_user((void *)arg, &cap, sizeof(cap)))
				retval = -EFAULT;
			break;
		}
	case ENCODER_SET_NORM:
		{
			unsigned long mode;
			char *smode;
			struct fb_var_screeninfo var;

			if (copy_from_user(&mode, (void *)arg, sizeof(mode))) {
				retval = -EFAULT;
				break;
			}
			if ((retval = fs453_ioctl(cmd, &mode)))
				break;

			if (mode == VIDEO_ENCODER_PAL)
				smode = MODE_PAL;
			else if (mode == VIDEO_ENCODER_NTSC)
				smode = MODE_NTSC;
			else
				smode = MODE_VGA;

			var = fbi->var;
			var.nonstd = 0;
			retval = fb_find_mode(&var, fbi, smode, mxcfb_modedb,
					      mxcfb_modedb_sz, NULL,
					      default_bpp);
			if ((retval != 1) && (retval != 2)) {	/* specified mode not found */
				retval = -ENODEV;
				break;
			}

			fbi->var = var;
			fb_mode = smode;
			retval = mxcfb_set_par(fbi);
			break;
		}
	case ENCODER_SET_INPUT:
	case ENCODER_SET_OUTPUT:
	case ENCODER_ENABLE_OUTPUT:
		{
			unsigned long varg;

			if (copy_from_user(&varg, (void *)arg, sizeof(varg))) {
				retval = -EFAULT;
				break;
			}
			retval = fs453_ioctl(cmd, &varg);
			break;
		}
#endif
	default:
		retval = -EINVAL;
	}
	return retval;
}

/*
 * Function to handle custom ioctls for MXC framebuffer.
 *
 * @param       inode   inode struct
 *
 * @param       file    file struct
 *
 * @param       cmd     Ioctl command to handle
 *
 * @param       arg     User pointer to command arguments
 *
 * @param       fbi     framebuffer information pointer
 */
static int mxcfb_ioctl_ovl(struct fb_info *fbi, unsigned int cmd,
			   unsigned long arg)
{
	int retval = 0;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	switch (cmd) {
	case MXCFB_SET_GBL_ALPHA:
		{
			struct mxcfb_gbl_alpha ga;
			if (copy_from_user(&ga, (void *)arg, sizeof(ga))) {
				retval = -EFAULT;
				break;
			}
			retval =
			    ipu_sdc_set_global_alpha((bool) ga.enable,
						     ga.alpha);
			dev_dbg(fbi->device, "Set global alpha to %d\n",
				ga.alpha);
			break;
		}
	case MXCFB_SET_CLR_KEY:
		{
			struct mxcfb_color_key key;
			if (copy_from_user(&key, (void *)arg, sizeof(key))) {
				retval = -EFAULT;
				break;
			}
			retval = ipu_sdc_set_color_key(MEM_SDC_FG, key.enable,
						       key.color_key);
			dev_dbg(fbi->device, "Set color key to 0x%08X\n",
				key.color_key);
			break;
		}
	case MXCFB_SET_OVERLAY_POS:
		{
			struct mxcfb_pos pos;
			if (copy_from_user(&pos, (void *)arg, sizeof(pos))) {
				retval = -EFAULT;
				break;
			}
			retval = ipu_sdc_set_window_pos(mxc_fbi->ipu_ch,
							pos.x, pos.y);
			break;
		}
	default:
		retval = -EINVAL;
	}
	return retval;
}

/*
 * mxcfb_blank():
 *      Blank the display.
 */
static int mxcfb_blank(int blank, struct fb_info *info)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;

#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_MXCFB_BLANK, 0) ;
#endif

	dev_dbg(info->device, "blank = %d\n", blank);

	if (mxc_fbi->blank == blank)
		return 0;
   
	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	mxc_fbi->blank = blank;

	switch (blank) {
	case FB_BLANK_POWERDOWN:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
		ipu_disable_channel(MEM_SDC_BG, true);
		ipu_sdc_set_brightness(0);
		
#ifdef CONFIG_FB_MXC_TVOUT
		if (fb_mode) {
			int enable = 0;

			if ((strcmp(fb_mode, MODE_VGA) == 0)
			    || (strcmp(fb_mode, MODE_NTSC) == 0)
			    || (strcmp(fb_mode, MODE_PAL) == 0))
				fs453_ioctl(ENCODER_ENABLE_OUTPUT, &enable);
		}
#endif
		break;
	case FB_BLANK_UNBLANK:
		ipu_enable_channel(MEM_SDC_BG);
		ipu_sdc_set_brightness(mxcfb_drv_data.backlight_level);
      
#ifdef CONFIG_FB_MXC_TVOUT
		if (fb_mode) {
			unsigned long mode = 0;

			if (strcmp(fb_mode, MODE_VGA) == 0)
				mode = VIDEO_ENCODER_VGA;
			else if (strcmp(fb_mode, MODE_NTSC) == 0)
				mode = VIDEO_ENCODER_NTSC;
			else if (strcmp(fb_mode, MODE_PAL) == 0)
				mode = VIDEO_ENCODER_PAL;
			if (mode)
				fs453_ioctl(ENCODER_SET_NORM, &mode);
		}
#endif
		break;
	}
	return 0;
}

#ifdef CONFIG_FB_MXC_OVERLAY
/*
 * mxcfb_blank_ovl():
 *      Blank the display.
 */
static int mxcfb_blank_ovl(int blank, struct fb_info *info)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;

#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_MXCFB_BLANK_OVL, 0) ;
#endif

	dev_dbg(info->device, "ovl blank = %d\n", blank);

	if (mxc_fbi->blank == blank)
		return 0;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	mxc_fbi->blank = blank;

	switch (blank) {
	case FB_BLANK_POWERDOWN:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
		ipu_disable_channel(MEM_SDC_FG, true);
		break;
	case FB_BLANK_UNBLANK:
		ipu_select_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER, 0);
		ipu_enable_channel(MEM_SDC_FG);
		break;
	}
	return 0;
}
#endif

/*
 * Pan or Wrap the Display
 *
 * This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
 *
 * @param               var     Variable screen buffer information
 * @param               info    Framebuffer information pointer
 */
static int
mxcfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;
	unsigned long lock_flags = 0;
	int retval;
	u_int y_bottom;
	unsigned long base;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	if (var->xoffset > 0) {
		dev_dbg(info->device, "x panning not supported\n");
		return -EINVAL;
	}

	if ((info->var.xoffset == var->xoffset) &&
	    (info->var.yoffset == var->yoffset)) {
		return 0;	// No change, do nothing
	}

	y_bottom = var->yoffset;

	if (!(var->vmode & FB_VMODE_YWRAP)) {
		y_bottom += var->yres;
	}

	if (y_bottom > info->var.yres_virtual) {
		return -EINVAL;
	}

	base = (var->yoffset * var->xres_virtual + var->xoffset);
	base *= (var->bits_per_pixel) / 8;
	base += info->fix.smem_start;

	down(&mxc_fbi->flip_sem);

	spin_lock_irqsave(&mxc_fbi->fb_lock, lock_flags);

	dev_dbg(info->device, "Updating SDC BG buf %d address=0x%08lX\n",
		mxc_fbi->cur_ipu_buf, base);

	mxc_fbi->cur_ipu_buf = !mxc_fbi->cur_ipu_buf;
	if (ipu_update_channel_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER,
				      mxc_fbi->cur_ipu_buf, base) == 0) {
		ipu_select_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER,
				  mxc_fbi->cur_ipu_buf);
		ipu_clear_irq(mxc_fbi->ipu_ch_irq);
		ipu_enable_irq(mxc_fbi->ipu_ch_irq);
	} else {
		dev_err(info->device,
			"Error updating SDC buf %d to address=0x%08lX\n",
			mxc_fbi->cur_ipu_buf, base);
	}

	spin_unlock_irqrestore(&mxc_fbi->fb_lock, lock_flags);

	dev_dbg(info->device, "Update complete\n");

	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;

	if (var->vmode & FB_VMODE_YWRAP) {
		info->var.vmode |= FB_VMODE_YWRAP;
	} else {
		info->var.vmode &= ~FB_VMODE_YWRAP;
	}

	return 0;
}

/*! 
 * Hook called when the framebuffer is opened.
 * We just select the first buffer within fb0
 */
static int
mxcfb_open(struct fb_info *info, int open)
{
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_MXCFB_OPEN, 0) ;
#endif
	struct mxcfb_info *mxc_fbi = NULL;
	
	if ((info == NULL) || (info->par == NULL))
		return -ENODEV;
	mxc_fbi = (struct mxcfb_info *)info->par;
	if (mxc_fbi->ipu_ch == MEM_SDC_BG)
	{
		ipu_select_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER, 0);
		if (mxc_fbi->blank == FB_BLANK_UNBLANK) {
			ipu_enable_channel(mxc_fbi->ipu_ch);
		}
	}
	return 0;
}

/*!
 * This structure contains the pointers to the control functions that are
 * invoked by the core framebuffer driver to perform operations like
 * blitting, rectangle filling, copy regions and cursor definition.
 */
static struct fb_ops mxcfb_ops = {
	.owner = THIS_MODULE,
	.fb_set_par = mxcfb_set_par,
	.fb_check_var = mxcfb_check_var,
	.fb_setcolreg = mxcfb_setcolreg,
	.fb_pan_display = mxcfb_pan_display,
	.fb_ioctl = mxcfb_ioctl,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_blank = mxcfb_blank,
//      .fb_cursor = soft_cursor,
	.fb_open = mxcfb_open,
};

#ifdef CONFIG_FB_MXC_OVERLAY
static struct fb_ops mxcfb_ovl_ops = {
	.owner = THIS_MODULE,
	.fb_set_par = mxcfb_set_par,
	.fb_check_var = mxcfb_check_var,
	.fb_setcolreg = mxcfb_setcolreg,
	.fb_pan_display = mxcfb_pan_display,
	.fb_ioctl = mxcfb_ioctl_ovl,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_blank = mxcfb_blank_ovl,
//      .fb_cursor = soft_cursor,
};
#endif

static irqreturn_t mxcfb_vsync_irq_handler(int irq, void *dev_id)
{
	struct mxcfb_data *fb_data = dev_id;

	ipu_disable_irq(irq);

	fb_data->vsync_flag = 1;
	wake_up_interruptible(&fb_data->vsync_wq);
	return IRQ_HANDLED;
}

static irqreturn_t mxcfb_irq_handler(int irq, void *dev_id)
{
	struct fb_info *fbi = dev_id;
	struct mxcfb_info *mxc_fbi = fbi->par;

	up(&mxc_fbi->flip_sem);
	ipu_disable_irq(irq);
	return IRQ_HANDLED;
}

#ifdef CONFIG_PM
/*
 * Power management hooks.      Note that we won't be called from IRQ context,
 * unlike the blank functions above, so we may sleep.
 */

/*
 * Suspends the framebuffer and blanks the screen. Power management support
 */
static int mxcfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mxcfb_data *drv_data = platform_get_drvdata(pdev);
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)drv_data->fbi->par;
#ifdef CONFIG_FB_MXC_OVERLAY
	struct mxcfb_info *mxc_fbi_ovl =
	    (struct mxcfb_info *)drv_data->fbi_ovl->par;
#endif

   printk(KERN_INFO "mxcfb: suspend\n");
   
	drv_data->suspended = true;

#ifdef CONFIG_FB_MXC_OVERLAY
	if (mxc_fbi_ovl->blank == FB_BLANK_UNBLANK) {
		ipu_disable_channel(MEM_SDC_FG, true);
	}
#endif

	if (mxc_fbi->blank == FB_BLANK_UNBLANK) {
		ipu_disable_channel(MEM_SDC_BG, true);
		ipu_sdc_set_brightness(0);
#ifdef CONFIG_FB_MXC_TVOUT
		if (fb_mode) {
			int enable = 0;

			if ((strcmp(fb_mode, MODE_VGA) == 0)
			    || (strcmp(fb_mode, MODE_NTSC) == 0)
			    || (strcmp(fb_mode, MODE_PAL) == 0))
				fs453_ioctl(ENCODER_ENABLE_OUTPUT, &enable);
		}
#endif
	}
	return 0;
}

/*
 * Resumes the framebuffer and unblanks the screen. Power management support
 */
static int mxcfb_resume(struct platform_device *pdev)
{
	struct mxcfb_data *drv_data = platform_get_drvdata(pdev);
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)drv_data->fbi->par;
#ifdef CONFIG_FB_MXC_OVERLAY
	struct mxcfb_info *mxc_fbi_ovl =
	    (struct mxcfb_info *)drv_data->fbi_ovl->par;
#endif
   printk(KERN_INFO "mxcfb: resume\n");

	drv_data->suspended = false;

	if (mxc_fbi->blank == FB_BLANK_UNBLANK) {
		ipu_enable_channel(MEM_SDC_BG);
		ipu_sdc_set_brightness(drv_data->backlight_level);
#ifdef CONFIG_FB_MXC_TVOUT
		if (fb_mode) {
			u32 mode = 0;

			if (strcmp(fb_mode, MODE_VGA) == 0)
				mode = VIDEO_ENCODER_VGA;
			else if (strcmp(fb_mode, MODE_NTSC) == 0)
				mode = VIDEO_ENCODER_NTSC;
			else if (strcmp(fb_mode, MODE_PAL) == 0)
				mode = VIDEO_ENCODER_PAL;
			if (mode)
				fs453_ioctl(ENCODER_SET_NORM, &mode);
		}
#endif
	}
#ifdef CONFIG_FB_MXC_OVERLAY
	if (mxc_fbi_ovl->blank == FB_BLANK_UNBLANK) {
		ipu_enable_channel(MEM_SDC_FG);
	}
#endif

	wake_up_interruptible(&drv_data->suspend_wq);
	return 0;
}
#else
#define mxcfb_suspend   NULL
#define mxcfb_resume    NULL
#endif

/*
 * Main framebuffer functions
 */

/*!
 * Allocates the DRAM memory for the frame buffer.      This buffer is remapped
 * into a non-cached, non-buffered, memory region to allow palette and pixel
 * writes to occur without flushing the cache.  Once this area is remapped,
 * all virtual memory access to the video memory should occur at the new region.
 *
 * @param       fbi     framebuffer information pointer
 * @param       use_internal_ram flag on whether to use internal RAM for memory
 *
 * @return      Error code indicating success or failure
 */
static int mxcfb_map_video_memory(struct fb_info *fbi, bool use_internal_ram)
{
	int retval = 0;

	{
		fbi->fix.smem_len = fbi->var.yres_virtual *
		    fbi->fix.line_length;
		fbi->screen_base =
		    dma_alloc_writecombine(fbi->device,
					   fbi->fix.smem_len,
					   (dma_addr_t *) & fbi->fix.smem_start,
					   GFP_DMA);

		if (fbi->screen_base == 0) {
			dev_err(fbi->device,
				"Unable to allocate framebuffer memory\n");
			retval = -EBUSY;
			goto err0;
		}
	}

	dev_dbg(fbi->device, "allocated fb @ paddr=0x%08X, size=%d.\n",
		(uint32_t) fbi->fix.smem_start, fbi->fix.smem_len);

	fbi->screen_size = fbi->fix.smem_len;

	/* Clear the screen */
	memset((char *)fbi->screen_base, 0, fbi->fix.smem_len);

	return 0;

      err0:
	fbi->fix.smem_len = 0;
	fbi->fix.smem_start = 0;
	fbi->screen_base = NULL;
	return retval;
}

/*!
 * De-allocates the DRAM memory for the frame buffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @return      Error code indicating success or failure
 */
static int mxcfb_unmap_video_memory(struct fb_info *fbi)
{
	{
		dma_free_writecombine(fbi->device, fbi->fix.smem_len,
				      fbi->screen_base, fbi->fix.smem_start);
	}
	fbi->screen_base = 0;
	fbi->fix.smem_start = 0;
	fbi->fix.smem_len = 0;
	return 0;
}

/*!
 * Initializes the framebuffer information pointer. After allocating
 * sufficient memory for the framebuffer structure, the fields are
 * filled with custom information passed in from the configurable
 * structures.  This includes information such as bits per pixel,
 * color maps, screen width/height and RGBA offsets.
 *
 * @return      Framebuffer structure initialized with our information
 */
static struct fb_info *mxcfb_init_fbinfo(struct device *dev, struct fb_ops *ops)
{
	struct fb_info *fbi;
	struct mxcfb_info *mxcfbi;

	/*
	 * Allocate sufficient memory for the fb structure
	 */
	fbi = framebuffer_alloc(sizeof(struct mxcfb_info), dev);
	if (!fbi)
		return NULL;

	mxcfbi = (struct mxcfb_info *)fbi->par;

	fbi->var.activate = FB_ACTIVATE_NOW;

	fbi->fbops = ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = mxcfbi->pseudo_palette;

	spin_lock_init(&mxcfbi->fb_lock);

	/*
	 * Allocate colormap
	 */
	fb_alloc_cmap(&fbi->cmap, 16, 0);

	return fbi;
}

/*!
 * Probe routine for the framebuffer driver. It is called during the
 * driver binding process.      The following functions are performed in
 * this routine: Framebuffer initialization, Memory allocation and
 * mapping, Framebuffer registration, IPU initialization.
 *
 * @return      Appropriate error code to the kernel common code
 */
static int mxcfb_probe(struct platform_device *pdev)
{
	char *mode = pdev->dev.platform_data;
	struct fb_info *fbi;
	struct mxcfb_info *mxcfbi;
#ifdef CONFIG_FB_MXC_OVERLAY
	struct fb_info *fbi_ovl;
#endif
	int ret = 0;

	/*
	 * Initialize FB structures
	 */
	fbi = mxcfb_init_fbinfo(&pdev->dev, &mxcfb_ops);
	if (!fbi) {
		ret = -ENOMEM;
		goto err0;
	}
	mxcfbi = (struct mxcfb_info *)fbi->par;

	mxcfbi->ipu_ch_irq = IPU_IRQ_SDC_BG_EOF;
	mxcfbi->cur_ipu_buf = 0;
	mxcfbi->ipu_ch = MEM_SDC_BG;
	mxcfb_drv_data.backlight_level = 153;
	ipu_sdc_set_brightness(mxcfb_drv_data.backlight_level);

	ipu_sdc_set_global_alpha(true, 0xff);
	ipu_sdc_set_color_key(MEM_SDC_BG, false, 0);

	if (ipu_request_irq(IPU_IRQ_SDC_BG_EOF, mxcfb_irq_handler, 0,
			    MXCFB_NAME, fbi) != 0) {
		dev_err(&pdev->dev, "Error registering BG irq handler.\n");
		ret = -EBUSY;
		goto err1;
	}
	ipu_disable_irq(IPU_IRQ_SDC_BG_EOF);

	if (fb_mode == NULL) {
		fb_mode = mode;
	}

	if (!fb_find_mode(&fbi->var, fbi, fb_mode, mxcfb_modedb,
			  mxcfb_modedb_sz, NULL, default_bpp)) {
		ret = -EBUSY;
		goto err2;
	}
	/* Default Y virtual size is 2x panel size */
	fbi->var.yres_virtual = fbi->var.yres * 2 + 1;

	mxcfb_drv_data.fbi = fbi;
	mxcfb_drv_data.suspended = false;
	init_waitqueue_head(&mxcfb_drv_data.suspend_wq);

	mxcfb_blank(FB_BLANK_UNBLANK, fbi);
	ret = mxcfb_set_par(fbi);
	if (ret < 0) {
		goto err2;
	}

	/*
	 * Register framebuffer
	 */
	ret = register_framebuffer(fbi);
	if (ret < 0) {
		goto err2;
	}
#ifdef CONFIG_FB_MXC_OVERLAY
	/*
	 * Initialize Overlay FB structures
	 */
	fbi_ovl = mxcfb_init_fbinfo(&pdev->dev, &mxcfb_ovl_ops);
	if (!fbi_ovl) {
		ret = -ENOMEM;
		goto err3;
	}
	mxcfb_drv_data.fbi_ovl = fbi_ovl;
	mxcfbi = (struct mxcfb_info *)fbi_ovl->par;

	mxcfbi->ipu_ch_irq = IPU_IRQ_SDC_FG_EOF;
	mxcfbi->cur_ipu_buf = 0;
	mxcfbi->ipu_ch = MEM_SDC_FG;

	if (ipu_request_irq(IPU_IRQ_SDC_FG_EOF, mxcfb_irq_handler, 0,
			    MXCFB_NAME, fbi_ovl) != 0) {
		dev_err(fbi->device, "Error registering FG irq handler.\n");
		ret = -EBUSY;
		goto err4;
	}
	ipu_disable_irq(mxcfbi->ipu_ch_irq);

	/* Default Y virtual size is 2x panel size */
	fbi_ovl->var = fbi->var;
	fbi_ovl->var.yres_virtual = fbi->var.yres * 2 + 1;

	/* Overlay is blanked by default */
	mxcfbi->blank = FB_BLANK_NORMAL;

	ret = mxcfb_set_par(fbi_ovl);
	if (ret < 0) {
		goto err5;
	}

	/*
	 * Register overlay framebuffer
	 */
	ret = register_framebuffer(fbi_ovl);
	if (ret < 0) {
		goto err5;
	}
#else
	mxcfb_drv_data.fbi_ovl = NULL;
#endif
	platform_set_drvdata(pdev, &mxcfb_drv_data);

	init_waitqueue_head(&mxcfb_drv_data.vsync_wq);
	if (!cpu_is_mx31()) {
		if ((ret = ipu_request_irq(IPU_IRQ_SDC_DISP3_VSYNC,
					   mxcfb_vsync_irq_handler,
					   0, MXCFB_NAME,
					   &mxcfb_drv_data)) < 0) {
			goto err6;
		}
		ipu_disable_irq(IPU_IRQ_SDC_DISP3_VSYNC);
	}

#if defined(CONFIG_MACH_MX31LSV0) || defined(CONFIG_MACH_MX31HSV1)
	fb_prepare_logo(fbi,0);
	fb_show_logo(fbi,0);
	//fb_prepare_logo(fbi_ovl,0);
	//fb_show_logo(fbi_ovl,0);
#endif // defined(CONFIG_MACH_MX31LSV0) || defined(CONFIG_MACH_MX31HSV1)

	printk(KERN_INFO "mxcfb: fb registered, using mode %s\n", fb_mode);
	return 0;

      err6:
#ifdef CONFIG_FB_MXC_OVERLAY
	unregister_framebuffer(fbi_ovl);
      err5:
	ipu_free_irq(IPU_IRQ_SDC_FG_EOF, fbi_ovl);
      err4:
	fb_dealloc_cmap(&fbi_ovl->cmap);
	framebuffer_release(fbi_ovl);
      err3:
	unregister_framebuffer(fbi);
#endif
      err2:
	ipu_free_irq(IPU_IRQ_SDC_BG_EOF, fbi);
      err1:
	fb_dealloc_cmap(&fbi->cmap);
	framebuffer_release(fbi);
      err0:
	printk(KERN_ERR "mxcfb: failed to register fb\n");
	return ret;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxcfb_driver = {
	.driver = {
		   .name = MXCFB_NAME,
		   },
	.probe = mxcfb_probe,
	.suspend = mxcfb_suspend,
	.resume = mxcfb_resume,
};

/*
 * Parse user specified options (`video=trident:')
 * example:
 * 	video=trident:800x600,bpp=16,noaccel
 */
int mxcfb_setup(char *options)
{
	char *opt;
	if (!options || !*options)
		return 0;
	while ((opt = strsep(&options, ",")) != NULL) {
		if (!*opt)
			continue;
		if (!strncmp(opt, "bpp=", 4))
			default_bpp = simple_strtoul(opt + 4, NULL, 0);
		else
			fb_mode = opt;
	}
	return 0;
}

/*!
 * Main entry function for the framebuffer. The function registers the power
 * management callback functions with the kernel and also registers the MXCFB
 * callback functions with the core Linux framebuffer driver \b fbmem.c
 *
 * @return      Error code indicating success or failure
 */
int __init mxcfb_init(void)
{
	int ret = 0;
#ifndef MODULE
	char *option = NULL;
#endif

#ifndef MODULE
	if (fb_get_options("mxcfb", &option))
		return -ENODEV;
	mxcfb_setup(option);
#endif

	ret = platform_driver_register(&mxcfb_driver);
	return ret;
}

void mxcfb_exit(void)
{
	struct fb_info *fbi = mxcfb_drv_data.fbi;

	if (fbi) {
		mxcfb_unmap_video_memory(fbi);

		if (&fbi->cmap)
			fb_dealloc_cmap(&fbi->cmap);

		unregister_framebuffer(fbi);
		framebuffer_release(fbi);
	}

	fbi = mxcfb_drv_data.fbi_ovl;
	if (fbi) {
		mxcfb_unmap_video_memory(fbi);

		if (&fbi->cmap)
			fb_dealloc_cmap(&fbi->cmap);

		unregister_framebuffer(fbi);
		framebuffer_release(fbi);
	}
#ifndef CONFIG_ARCH_MX3
	ipu_free_irq(IPU_IRQ_SDC_DISP3_VSYNC, &mxcfb_drv_data);
#endif

	platform_driver_unregister(&mxcfb_driver);
}

module_init(mxcfb_init);
module_exit(mxcfb_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC framebuffer driver");
MODULE_SUPPORTED_DEVICE("fb");
