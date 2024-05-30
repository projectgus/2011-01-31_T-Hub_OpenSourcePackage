/*
 * Copyright (C) 2006-2009, Sagem Communications. All Rights Reserved.
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
 * @file progessbar.c 
 * @brief Progress bar drawing over logo.
 *
 *  Created 2006 by Vincent Galceran <vincent.galceran@sagem.com>
 *  Updated 2006-2009 Cedric Le Dillau <cedric.ledillau@sagem.com> to 
 *       make it configurable.
 * 
 * @ingroup PROGRESSBAR
 */

/*
 * Includes
 */
#include <asm/arch/hardware.h>
#include <linux/progressbar.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/memory.h>
#include <linux/fb.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h> /* copy_from_user, copy_to_user */

#define DEBUG 0

#if defined(DEBUG) && (DEBUG)
#define static
#endif

/* Conversion functions */
#define RGB32(r,g,b)             (uint32_t)((((r)&0xFF)<<16) | (((g)&0xFF)<<8) | ((b)&0xFF))
#define RGB888_TO_RGB565(rgb32)  ( (((rgb32) >> 8) & 0xf800 ) | (((rgb32)>>5) & 0x07E0) | (((rgb32)>>3) & 0x001F) )  

/* Color samples */
#define ORANGE          RGB32(255, 102,   0) 
#define SCOM_RED        RGB32(199,  13,  44)
#define GREY_192        RGB32(192, 192, 192)
#define GREEN				RGB32( 30, 255,  30)

/* Video parameters */
#define PIXEL_SIZE   (2)
#define SCREEN_BUFFER_SIZE (480 * 800 * PIXEL_SIZE)
#define SCREEN_WIDTH 800

/* Pixle color conversion */
#if PIXEL_SIZE == (2)
#define RGB888_TO_PIXEL(a)       RGB888_TO_RGB565(a)
#elif  PIXEL_SIZE == (3)
#define RGB888_TO_PIXEL(rgb32)   ( (rgb32) & 0x00ffffff )
#else
#error Unsupported pixel size: PIXEL_SIZE  
#endif // PIXEL_SIZE

//#define MY_BLOCK_SIZE 4096
#define MY_BLOCK_SIZE 16384

/*
 * Global variables
 */
static int in_use = 0;
uint64_t first_jiffies;
//static int to_free = 0;

//#define CONFIG_PROGRESSBAR_ASPECT_ROUNDED
#ifdef CONFIG_PROGRESSBAR_ASPECT_ROUNDED
int round_depth[] = { 
	1,
	3,
	5,
	6,
	7,
	8,
	9,
	9,
	9,
	10,
	10,
	10,
	10,
	10,
	10,
	10,
	9,
	9,
	8,
	8,
	7,
	6,
	5,
	3,
	1,
};

int round_height = sizeof(round_depth) / sizeof(round_depth[0]) ;
uint16_t rgb_rightcolor = RGB888_TO_PIXEL(0x009d9d9d);
uint16_t rgb_uppercolor = RGB888_TO_PIXEL(0x00575757);
#define PATTERN_WIDTH   (25)


/**
 * Draw rounded box base on already displayed pattern
 * 
 * @param topleft_address  address of the topleft pixel of the rectangular part
 * @param already_drawn    number of vertical lines draws starting from rectangular part
 * @param to               right x of the rectangular part
 */
void draw_rounded(uint8_t *topleft_address, int at)
{
	int j;
	uint16_t *pix_address_w = (uint16_t*)topleft_address;

	pix_address_w += at ;
	*pix_address_w = rgb_uppercolor ;

	for(j=1;j<round_height;j++) {
		/* go to next line */
		pix_address_w += ( SCREEN_WIDTH + round_depth[j] - round_depth[j-1] - 1 ) ;
		*pix_address_w = *(pix_address_w - PATTERN_WIDTH) ;
		pix_address_w++ ;
		*pix_address_w = rgb_rightcolor ;
	}
}

#endif

typedef struct {
	uint8_t* buffer;

	uint32_t upperleft;
	uint32_t buflen;

	/* top left */
	uint32_t x1;
	uint32_t y1;

	/* bottom right */
	uint32_t x2;
	uint32_t y2;

	/* sizes */
	uint32_t width;
	uint32_t height;

	uint32_t fg_color;
	uint32_t bg_color;

	uint32_t max_progress;     /*! Full bar width in pixel */
	uint32_t current_progress; /*! Cursor in pixel from x1 */
} progressbar_t;

static progressbar_t my_pb;

static void draw_pixel(uint8_t* buffer,
		uint32_t x,
		uint32_t y,
		uint32_t rgb)
{
	uint32_t first = ((y * SCREEN_WIDTH) + x ) * PIXEL_SIZE;

	rgb = RGB888_TO_PIXEL(rgb) ;

	buffer[first] = rgb & 0xff;
	buffer[first+1] = (rgb >> 8) & 0xff;
#if (PIXEL_SIZE>2)
	buffer[first+2] = (rgb >> 16) & 0xff;
#endif
}

/*!
 * Draw a vertical line with solid RGB color.
 *
 * Draw top to bottom lines.
 *
 * @param framebuffer Screen buffer
 * @param x   Horizontal x position
 * @param y1  top y
 * @param y2  bottom y
 * @param rgb RGB888 LSB aligned color
 */
static void draw_vline(uint8_t* framebuffer, uint32_t x, uint32_t y1, uint32_t y2, uint32_t rgb32)
{
	int y;
	for(y=y1; y<y2; y++){
		draw_pixel(framebuffer, x, y, rgb32);
	}
}

/*!
 * Draw a filled-box with solid RGB color.
 *
 * Draw top to bottom horizontal lines.
 *
 * @param buffer Screen buffer
 * @param x1  left x
 * @param y1  top y
 * @param x2  right x
 * @param y2  bottom y
 * @param rgb RGB888 LSB aligned color
 */
static void draw_box(uint8_t* buffer, uint32_t x1, uint32_t y1,uint32_t x2, uint32_t y2, uint32_t rgb) 
{
	int y;
	int x;
	uint8_t *topleft_address = buffer + (x1 + y1 * SCREEN_WIDTH) * PIXEL_SIZE ;
	uint8_t *address = topleft_address;
	uint32_t widthsize = SCREEN_WIDTH * PIXEL_SIZE ;
	uint32_t linesize = (x2 - x1) * PIXEL_SIZE ;

	for ( address=topleft_address, x=x1 ; x<x2 ; x++ )
	{
		*address++ = rgb & 0xff;
		*address++ = (rgb >> 8) & 0xff;
#if (PIXEL_SIZE>2)
		*address++ = (rgb >> 16) & 0xff;
#endif
	}

	address = topleft_address + widthsize ; /*! \warning MUST already be the correct value */

	for ( y=y1+1 ; y<y2 ; y++, address += (SCREEN_WIDTH * PIXEL_SIZE) )
	{
		memcpy(address, topleft_address, linesize);
	}
}

static void update_progress(progressbar_t* pb, uint32_t cur_state, uint32_t max_state)
{
	uint32_t new_progress = (pb->max_progress * cur_state / max_state);
	int i;

	if ( num_registered_fb == 0 )
		return ;

	if(new_progress > pb->max_progress){
		new_progress = pb->max_progress;
	}

	for(i=pb->current_progress; i<new_progress; i++){
#ifdef CONFIG_PROGRESSBAR_ASPECT_ROUNDED
		draw_rounded(pb->buffer + (pb->x1 + pb->y1 * SCREEN_WIDTH) * PIXEL_SIZE, i) ;
#else
		draw_vline(pb->buffer, pb->x1+i, pb->y1, pb->y2, pb->fg_color);
#endif
	}
	pb->current_progress = new_progress;
}

#define MAX_PROGRESS 900
void progressbar_proc_init(void);
void progressbar_at(int step)
{
	uint64_t progress = jiffies - first_jiffies;
#ifdef CONFIG_PROGRESSBAR_DEBUG
	printk("progress step %d at %llu\n", step, progress);
#endif

	if ( step > 2 )
		progressbar_proc_init();

	if(in_use){
		update_progress(&my_pb,
				step,
				200);
	}else{
#ifdef CONFIG_PROGRESSBAR_DEBUG
		printk("progress: oups\n");
#endif
	}
}

#if 0

static ssize_t progressbar_value_show(struct sys_device *dev, char *buf)
{
	int size = 0;
	return size;
}

static ssize_t progressbar_value_store(struct sys_device *dev, const char *buf,
		size_t size)
{
	int value = 0;
	int i=0;

	while( (buf[i]>='0') && (buf[i]<='9')) {
		value = value * 10 + ( buf[i] - '\0' );
	}

	if ( value > 0 )
		progressbar_at(value);

	return i;
}


static SYSDEV_ATTR(value, 0600, progressbar_value_show, progressbar_value_store);

static struct sysdev_class progressbar_sysclass = {
	set_kset_name("progressbar"),
};

static struct sys_device progressbar_device = {
	.id = 0,
	.cls = &progressbar_sysclass,
};

static int progressbar_sysdev_ctrl_init(void)
{
	int err;

	err = sysdev_class_register(&progressbar_sysclass);
	if (!err)
		err = sysdev_register(&progressbar_device);
	if (!err) {
		err = sysdev_create_file(&progressbar_device, &attr_value);
	}

	return err;
}

static void progressbar_sysdev_ctrl_exit(void)
{
	sysdev_remove_file(&progressbar_device, &attr_value);
	sysdev_unregister(&progressbar_device);
	sysdev_class_unregister(&progressbar_sysclass);
}

#endif

#ifdef CONFIG_PROC_FS
#define OUTPUT_MAX  PAGE_SIZE
#define COMMAND_MAX 64

static struct proc_dir_entry *proc_entry = NULL; 

static char * skip_white_space(char *cmd)
{
	while (*cmd <= ' ' && *cmd != 0) {
		cmd++;
	}
	return cmd;
}

static int proc_read(char *page, char **start, off_t off, int count,
		int *eof, void *data)
{
	char *buf=page;
	int len=0;

	if (off==0)
	{
		len += snprintf(buf+len, OUTPUT_MAX-len, "progressbar:\t%d/%d\n", 
				my_pb.current_progress, 
				my_pb.max_progress 
				);
	}
	*eof = 1;

	return len;
}

	static int 
proc_write(struct file* file, const char* buffer, 
		unsigned long count, void* data)
{
	char cmd[COMMAND_MAX];
	char  *cur;
	int value = 0;

	if (count >= COMMAND_MAX) {
		return -E2BIG;
	}

	if(copy_from_user(cmd, buffer, count)) {
		return -EFAULT;
	}

	cur=cmd;
	cmd[count] = 0;
	cur = skip_white_space(cur);

	if (*cur == 0) 
		goto error;

	value = simple_strtoul(cur, &cur, 0);

	if (value>0)
		progressbar_at(value);
error:
	return cur-cmd;
}

void progressbar_proc_init(void)
{
	if ( proc_entry ) 
		return;

	proc_entry = create_proc_entry("progressbar", 0644, NULL);
	if (proc_entry) {
		proc_entry->write_proc = proc_write;
		proc_entry->read_proc  = proc_read;
		proc_entry->data       = NULL;
	}
}

#endif

static void progessbar_get_geometry(progressbar_t* pb)
{
#if defined(CONFIG_PROGRESSBAR_GEOMETRY)
	int w, h, x, y;
	if ( sscanf(CONFIG_PROGRESSBAR_GEOMETRY, 
				"%ux%u+%u+%u", &w, &h, &x, &y) == 4) {
		pb->x1 = x;
		pb->y1 = y;
		pb->width = w;
		pb->height = h;
		pb->x2 = x + w;
		pb->y2 = y + h;
	}
	else
#endif
	{
#if defined(CONFIG_PROGRESSBAR_GEOMETRY)
		printk(KERN_ERR "Unable to parse progressbar geometry.\n");
#endif

#ifdef CONFIG_PROGRESSBAR_ASPECT_ROUNDED
		pb->x1 = 221;
		pb->y1 = 404;
		pb->x2 = 575;
		pb->y2 = 427;
#else

#if 1
		/* Sagem Communications RED prodress bar*/
		pb->x1 = 221;
		pb->y1 = 404;
		pb->x2 = 575;
		pb->y2 = 427;
#else
		/* Sagem Communications hsv1 (orange) progress bar*/
		pb->x1 = 286;
		pb->y1 = 329;
		pb->x2 = 636;
		pb->y2 = 333;
#endif
#endif
		pb->width = pb->x2 - pb->x1;
		pb->height = pb->y2 - pb->y1;
	}
}

void progressbar_init(void)
{
	uint32_t upperleft;
	uint32_t lowerright;

	printk(KERN_NOTICE "Progressbar init.\n");

	struct fb_info *info = registered_fb[0];
	unsigned long screen_buffer = info->fix.smem_start;

	progessbar_get_geometry(&my_pb);

	printk(KERN_INFO "Progressbar geometry: %ux%u+%u+%u (to %u,%u)\n", 
			my_pb.width, my_pb.height, my_pb.x1, my_pb.y1, my_pb.x2, my_pb.y2);

	my_pb.max_progress = my_pb.x2 - my_pb.x1;

	/* kick the cursor as the machine is runnning already */
#ifdef CONFIG_PROGRESSBAR_ASPECT_ROUNDED
	my_pb.current_progress = (247-my_pb.x1) ;
#else
	//my_pb.current_progress = (my_pb.x2 - my_pb.x1) * 50 / 100;
#endif

#if defined(CONFIG_PROGRESSBAR_COLOR)
	/* use to configured color if present */
	my_pb.fg_color = RGB888_TO_PIXEL(CONFIG_PROGRESSBAR_COLOR);
#else
	/* fallback to Sagem Communications' RED color */
	my_pb.fg_color = SCOM_RED;
#endif

	upperleft =
		screen_buffer + (my_pb.x1 + my_pb.y1 * SCREEN_WIDTH) * PIXEL_SIZE;
	lowerright =
		screen_buffer + (my_pb.x2 + my_pb.y2 * SCREEN_WIDTH) * PIXEL_SIZE;

	upperleft = (upperleft / MY_BLOCK_SIZE) * MY_BLOCK_SIZE;
	lowerright =
		(lowerright + MY_BLOCK_SIZE - 1)
		/ MY_BLOCK_SIZE
		* MY_BLOCK_SIZE;

	my_pb.upperleft = upperleft;
	my_pb.buflen = lowerright - my_pb.upperleft;

	my_pb.upperleft = screen_buffer;
	my_pb.buflen = SCREEN_BUFFER_SIZE;

	request_mem_region(my_pb.upperleft, my_pb.buflen, "progressbar");

	my_pb.buffer = ioremap(my_pb.upperleft, my_pb.buflen);

	draw_box(my_pb.buffer, my_pb.x1, my_pb.y1, my_pb.x2, my_pb.y2, GREEN);
	
	in_use = 1;

	first_jiffies = jiffies;
}


void progressbar_free(void)
{
	if(in_use){
		release_mem_region(my_pb.upperleft, my_pb.buflen);
		in_use = 0;
	}
}

/*
	EXPORT_SYMBOL(progressbar_init);
	EXPORT_SYMBOL(progressbar_at);
	EXPORT_SYMBOL(progressbar_free);
 */
