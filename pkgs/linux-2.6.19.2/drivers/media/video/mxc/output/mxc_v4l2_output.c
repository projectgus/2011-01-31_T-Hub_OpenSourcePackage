/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 03/10/2007 
 * Copyright (c) 2010 Sagemcom All rights reserved:
 * - Deliver traces ulog for interrupts, task switches and mxc_v4l2_output
 * - Re-initialize static buffers to avoid artefacts on next video
 * - Apply a Freescale patch for reducing vertical line
 * - Allow to display video at a specific position in the screen to previous 316
 * - Deliver upscaling greater than 720 pixels
 * - Alloc video buffers at V4L2 initialization.
 *     It fixes a kernel panic when system cache is full and we want to allocate video buffers.
 *     It improves player stability and add a robustness on high resolution videos.
 *     By default 10 buffers of 1Mb are allocated
 * - Report interesting modification of ipu driver from kernel 2.6.22
 * - Allow v4l to display stream on fb0
 * - Allow userspace to move overlay
 * - Apply a patch from FreeScale to make v4l2 work with plugin v4l2sink
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
 * @file drivers/media/video/mxc/output/mxc_v4l2_output.c
 *
 * @brief MXC V4L2 Video Output Driver
 *
 * Video4Linux2 Output Device using MXC IPU Post-processing functionality.
 *
 * @ingroup MXC_V4L2_OUTPUT
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/semaphore.h>
#include <linux/dma-mapping.h>
#include "asm-arm/cacheflush.h"

#include <asm/arch/mxcfb.h>
#include "mxc_v4l2_output.h"

#include <asm/arch/ipu.h>

#include "../../../../mxc/ipu/ipu_prv.h"
#include "../../../../mxc/ipu/ipu_regs.h"
#include "../../../../mxc/ipu/ipu_param_mem.h"

inline static uint32_t channel_2_dma(ipu_channel_t ch, ipu_buffer_t type)
{
	return ((type == IPU_INPUT_BUFFER) ? ((uint32_t) ch & 0xFF) :
		((type == IPU_OUTPUT_BUFFER) ? (((uint32_t) ch >> 8) & 0xFF)
		 : (((uint32_t) ch >> 16) & 0xFF)));
};

inline static uint32_t DMAParamAddr(uint32_t dma_ch)
{
	return (0x10000 | (dma_ch << 4));
};

#ifdef CONFIG_VBUFPOOL
#include "../../../../vbufpool/vbufpool.h"
#endif

//#undef pr_debug
//#define pr_debug printk
//#define QBUF_DEBUG

//SAGEM
#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_V4L2OUT)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

vout_data *g_vout;
#define SDC_FG_FB_FORMAT        IPU_PIX_FMT_RGB565

static int cache_mode = 1;

static int frame_number_in=0;
static int frame_number_out=0;

extern void v6_flush_kern_cache_all_l2(void);

struct v4l2_output mxc_outputs[2] = {
	{
	 .index = MXC_V4L2_OUT_2_SDC,
	 .name = "DISP3 Video Out",
	 .type = V4L2_OUTPUT_TYPE_ANALOG,	/* not really correct,
						   but no other choice */
	 .audioset = 0,
	 .modulator = 0,
	 .std = V4L2_STD_UNKNOWN},
	{
	 .index = MXC_V4L2_OUT_2_ADC,
	 .name = "DISPx Video Out",
	 .type = V4L2_OUTPUT_TYPE_ANALOG,	/* not really correct,
						   but no other choice */
	 .audioset = 0,
	 .modulator = 0,
	 .std = V4L2_STD_UNKNOWN}
};

static int video_nr = 16;
static spinlock_t g_lock = SPIN_LOCK_UNLOCKED;

static int dq_intr_cnt=0;
static int dq_timeout_cnt=0;

#define QUEUE_SIZE (MAX_FRAME_NUM + 1)
static __inline int queue_size(v4l_queue * q)
{
    if (q->tail >= q->head)
	return (q->tail - q->head);
    else
	return ((q->tail + QUEUE_SIZE) - q->head);
}

static __inline int queue_buf(v4l_queue * q, int idx)
{
    if (((q->tail + 1) % QUEUE_SIZE) == q->head)
	return -1;	/* queue full */
    q->list[q->tail] = idx;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    return 0;
}

static __inline int dequeue_buf(v4l_queue * q)
{
    int ret;
    if (q->tail == q->head)
	return -1;	/* queue empty */
    ret = q->list[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    return ret;
}

static __inline int peek_next_buf(v4l_queue * q)
{
    if (q->tail == q->head)
	return -1;	/* queue empty */
    return q->list[q->head];
}

static __inline unsigned long get_jiffies(struct timeval *t)
{
    struct timeval cur;

    if (t->tv_usec >= 1000000) {
	t->tv_sec += t->tv_usec / 1000000;
	t->tv_usec = t->tv_usec % 1000000;
    }

    do_gettimeofday(&cur);
    if ((t->tv_sec < cur.tv_sec)
	|| ((t->tv_sec == cur.tv_sec) && (t->tv_usec < cur.tv_usec)))
	return jiffies;

    if (t->tv_usec < cur.tv_usec) {
	cur.tv_sec = t->tv_sec - cur.tv_sec - 1;
	cur.tv_usec = t->tv_usec + 1000000 - cur.tv_usec;
    } else {
	cur.tv_sec = t->tv_sec - cur.tv_sec;
	cur.tv_usec = t->tv_usec - cur.tv_usec;
    }

    return jiffies + timeval_to_jiffies(&cur);
}

/*!
 * Private function to free buffers
 *
 * @param bufs_paddr	Array of physical address of buffers to be freed
 *
 * @param bufs_vaddr	Array of virtual address of buffers to be freed
 *
 * @param num_buf	Number of buffers to be freed
 *
 * @param size		Size for each buffer to be free
 *
 * @return status  0 success.
 */
static int mxc_free_buffers(dma_addr_t bufs_paddr[], void *bufs_vaddr[],
			    int num_buf, int size)
{
	int i;

	for (i = 0; i < num_buf; i++) {
		if (bufs_vaddr[i] != 0) {
#ifdef CONFIG_VBUFPOOL
			pr_debug("v4l2out - vbuf_free %d size=%d addr=0x%x\n",i,size,(u32) bufs_paddr[i]);
			vbuf_free(bufs_vaddr[i], size, VBUF_TYPE_NON_CACHED, bufs_paddr[i]);
#else
			pr_debug("v4l2out - dma_free_coherent %d size=%d addr=0x%x\n",i,size,(u32) bufs_paddr[i]);
			
			dma_free_coherent(0, size, bufs_vaddr[i],
					  bufs_paddr[i]);
#endif
			pr_debug("freed @ paddr=0x%08X\n", (u32) bufs_paddr[i]);
			bufs_paddr[i] = 0;
			bufs_vaddr[i] = NULL;
		}
	}
	return 0;
}

/*!
 * Private function to allocate buffers
 *
 * @param bufs_paddr	Output array of physical address of buffers allocated
 *
 * @param bufs_vaddr	Output array of virtual address of buffers allocated
 *
 * @param num_buf	Input number of buffers to allocate
 *
 * @param size		Input size for each buffer to allocate
 *
 * @return status	-0 Successfully allocated a buffer, -ENOBUFS failed.
 */
static int mxc_allocate_buffers(dma_addr_t bufs_paddr[], void *bufs_vaddr[],
				int num_buf, int size)
{
    int i;

    for (i = 0; i < num_buf; i++) {
#ifdef CONFIG_VBUFPOOL
	bufs_vaddr[i] = vbuf_alloc(size, VBUF_TYPE_NON_CACHED, &bufs_paddr[i]);
	pr_debug("v4l2out - vbuf_alloc %d size=%d addr=0x%x\n",i,size,(u32) bufs_paddr[i]);
#else
	bufs_vaddr[i] = dma_alloc_coherent(0, size,
					   &bufs_paddr[i],
					   GFP_DMA | GFP_KERNEL);
	pr_debug("v4l2out - dma_alloc_coherent %d size=%d addr=0x%x\n",i,size,(u32) bufs_paddr[i]);
#endif
		
	if (bufs_vaddr[i] == 0) {
	    mxc_free_buffers(bufs_paddr, bufs_vaddr, i, size);
#ifdef CONFIG_VBUFPOOL
	    printk("v4l2out - vbuf_alloc failed.\n");
#else
	    printk("v4l2out - dma_alloc_coherent failed.\n");
#endif
	    return -ENOBUFS;
	}
	pr_debug("allocated @ paddr=0x%08X, size=%d.\n",(u32) bufs_paddr[i], size);
    }
	
    return 0;
}

/*
 * Returns bits per pixel for given pixel format
 *
 * @param pixelformat  V4L2_PIX_FMT_RGB565, V4L2_PIX_FMT_BGR24 or V4L2_PIX_FMT_BGR32
 *
 * @return bits per pixel of pixelformat
 */
static u32 fmt_to_bpp(u32 pixelformat)
{
    u32 bpp;

    switch (pixelformat) {
    case V4L2_PIX_FMT_RGB565:
	bpp = 16;
	break;
    case V4L2_PIX_FMT_BGR24:
    case V4L2_PIX_FMT_RGB24:
	bpp = 24;
	break;
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_RGB32:
	bpp = 32;
	break;
    default:
	bpp = 8;
	break;
    }
    return bpp;
}

static void mxc_v4l2out_timer_handler(unsigned long arg)
{
    int index;
    unsigned long timeout;
    u32 lock_flags = 0;
    int error;
    vout_data *vout = (vout_data *) arg;

    spin_lock_irqsave(&g_lock, lock_flags);

#ifdef QBUF_DEBUG
    pr_debug("v4l2out - timer handler: %lu\n", jiffies);
#endif

#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_TIMER_HANDLER, jiffies) ;
#endif

    dev_dbg(vout->video_dev->dev, "timer handler: %lu\n", jiffies);
    /*
     * If timer occurs before IPU h/w is ready, then set the state to
     * paused and the timer will be set again when next buffer is queued
     * or PP comletes
     */
    if (vout->ipu_buf[vout->next_rdy_ipu_buf] != -1) {
	dev_dbg(vout->video_dev->dev, "IPU buffer busy\n");
	vout->state = STATE_STREAM_PAUSED;
	pr_debug("%s - v4l2out - WARNING - stream has stopped\n",__FUNCTION__);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_IPU_BUFFER_BUSY, 0) ;
#endif
	goto exit0;
    }

    /* Dequeue buffer and pass to IPU */
    index = dequeue_buf(&vout->ready_q);
    if (index == -1) {	/* no buffers ready, should never occur */
	dev_err(vout->video_dev->dev,
		"mxc_v4l2out: timer - no queued buffers ready\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_TIMER_NO_QUEUED_BUFFERS_READY, 0) ;
#endif
	goto exit0;
    }

    vout->frame_count++;
    vout->ipu_buf[vout->next_rdy_ipu_buf] = index;

    /* UPDATE INPUT BUFFER ADDRESSES */
    /* double buffering is used to run the configuration only once and process 
       both half images. As the output is an RGB image, we don't need to update
       the U and V buffer offsets, and configuration can be done in one shot
    */

    /* left half image */
    if ((error=ipu_update_channel_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER,
					 0,
					 vout->v4l2_bufs[index].m.offset)) < 0) {
	dev_err(vout->video_dev->dev,
		"unable to update input buffer %d address\n",
		vout->next_rdy_ipu_buf);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_UNABLE_UPDATE_INPUT_BUFFER, vout->next_rdy_ipu_buf) ;
#endif
	goto exit0;
    }
    /* right half image - just offset by half a line */
    if ((error=ipu_update_channel_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER,
					 1,
					 vout->v4l2_bufs[index].m.offset + vout->v2f.fmt.pix.width / 2)) < 0) {
	dev_err(vout->video_dev->dev,
		"unable to update input buffer %d address\n",
		vout->next_rdy_ipu_buf);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_UNABLE_UPDATE_INPUT_BUFFER, vout->next_rdy_ipu_buf) ;
#endif
    	goto exit0;
    }

    /* UPDATE OUTPUT BUFFER ADDRESSES */
    /* Double buffering is also used here, BUT we're getting a YUV buffer here : 
       we can configure the 2 Y start buffer addresses in one shot, but we'll have
       to come back to update the U and V buffer offsets
    */

    /* left half image */
    if ((error=ipu_update_channel_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER,
					 0,
					 vout->display_bufs[(vout->frame_count-1)%2] )) < 0) {

	dev_err(vout->video_dev->dev,
        	"unable to update output buffer 0 address\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_UNABLE_UPDATE_OUTPUT_BUFFER, 0) ;
#endif
    	goto exit0;
    }

    /* right half image - just offset by half a line */
    if ((error=ipu_update_channel_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER,
					 1,
					 vout->display_bufs[(vout->frame_count-1)%2] 
					 + vout->crop_current.width / 2 
					 * bytes_per_pixel(SDC_FG_FB_FORMAT))) < 0) {

	dev_err(vout->video_dev->dev,
        	"unable to update output buffer 1 address\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_UNABLE_UPDATE_OUTPUT_BUFFER, 1) ;
#endif
    	goto exit0;
    }

    /* START TRANSFERS FROM PP TO DISPLAY BUF */
    /* Mark the output buffers as ready : the IPU will initiate the transfers
       as soon as it gets its input data. 
       Both output buffers can be enabled at once, since no update in the address
       will occur while the processing of the 2 half images
    */
    if (ipu_select_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER, 0)<0)
    {
	pr_debug("output buffer %d select error\n", vout->next_rdy_ipu_buf);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_OUTPUT_BUFFER_SELECT_ERROR, vout->next_rdy_ipu_buf) ;
#endif
    } 

    /* START TRANSFERS FROM MEM TO PP */
    /* ONLY start the channel corresponding to the first half of the image
       the other one will be started after the 1st half
       of the image has been processed (PP_EOF_handler) 
       as we need to update U and V offsets before.  
    */

    if (ipu_select_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER,
			  0) < 0) {
	dev_err(vout->video_dev->dev,
		"unable to set IPU buffer ready\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_ENABLE_SET_IPU_BUFFER_READY, 0) ;
#endif
    }

    /* Setup timer for next buffer */
    index = peek_next_buf(&vout->ready_q);
    if (index != -1) {
	/* if timestamp is 0, then default to 30fps */
	if ((vout->v4l2_bufs[index].timestamp.tv_sec == 0)
	    && (vout->v4l2_bufs[index].timestamp.tv_usec == 0))
	    timeout =
		vout->start_jiffies + vout->frame_count * HZ / 30;
	else
	    timeout =
		get_jiffies(&vout->v4l2_bufs[index].timestamp);

	if (jiffies >= timeout) {
	    dev_dbg(vout->video_dev->dev,
		    "warning: timer timeout already expired.\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_TIMER_ALREADY_EXPIRED, 0) ;
#endif
	}
	if (mod_timer(&vout->output_timer, timeout))
    {
	    dev_dbg(vout->video_dev->dev,
		    "warning: timer was already set\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_TIMER_ALREADY_SET, 0) ;
#endif
    }
	dev_dbg(vout->video_dev->dev,
		"timer handler next schedule: %lu\n", timeout);
    } else {
	vout->state = STATE_STREAM_PAUSED;
    }

  exit0:
    spin_unlock_irqrestore(&g_lock, lock_flags);
}


/* PP input irq handler */
/* This function is executed each time the PP has finished to fetch a half image
*/

static irqreturn_t mxc_v4l2out_pp_in_irq_handler(int irq, void *dev_id)
{
    u32 lock_flags = 0;
    vout_data *vout = dev_id;
    spin_lock_irqsave(&g_lock, lock_flags);

/*  use frame_number counter to figure out what needs to be done.
    frame_number is initialized to 1 in streamon function. 
    frame_number is incremented at the end of this interrupt (PP_in_EOF)
    4 cases to differentiate :
    frame_number=1 : end of processing of the 1st half of the image that is on buffer 1
    frame_number=2 : end of processing of the 2nd half of the image that is on buffer 1
    frame_number=3 : end of processing of the 1st half of the image that is on buffer 2
    frame_number=4 : end of processing of the 2nd half of the image that is on buffer 2 
*/

    if(frame_number_in%2 == 0 ){ 
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_IN_IRQ_HANDLER_FRAME_2_4, 0) ;
#endif
/*      frame_number = 2 or 4 
        Just finished processing second half of an image.
        restore U and V offsets to original values.
*/
 
        uint32_t local_params[4];


/*      recreate original value of the registers from width and height, or we will write zeroes
 */
        local_params[3] =
	    (uint32_t) ((vout->v2f.fmt.pix.width/2 - 1) << 12) 
            | ((uint32_t) (vout->v2f.fmt.pix.height - 1) << 24);

/*      populate structure with updated offset values */
        local_params[0] = 0;
	local_params[1] = (1UL << (46 - 32)) | ( vout->offset.u_offset << (53 - 32));
	local_params[2] = vout->offset.u_offset >> (64 - 53);
	local_params[2] |=  vout->offset.v_offset<< (79 - 64);
	local_params[3] |=  vout->offset.v_offset>> (96 - 79);

/*      write data to IPU DMA */
        _ipu_write_param_mem(DMAParamAddr(channel_2_dma(
					      vout->post_proc_ch, IPU_INPUT_BUFFER)), local_params, 4);


    } else {
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_IN_IRQ_HANDLER_FRAME_1_3, 0) ;
#endif
/*      
        frame_number = 1 or 3
        Just finished processing the first half of an image 
        Configure the IPU to process the second half of this same image, 
        which is update the U and V offset values of the input DMA,
        and set buffer ready bit 
*/
        uint32_t local_params[4];
        uint32_t updated_u_offs, updated_v_offs;


/*      first update U and V offsets 
 */

        updated_u_offs = vout->offset.u_offset - vout->v2f.fmt.pix.width /4;
        updated_v_offs = vout->offset.v_offset - vout->v2f.fmt.pix.width /4;

/*      recreate original value from width and height, or we will replace them 
        by zeroes 
*/
        local_params[3] =
	    (uint32_t) ((vout->v2f.fmt.pix.width /2 - 1) << 12) 
            | ((uint32_t) (vout->v2f.fmt.pix.height- 1) << 24);

/*      populate structure with updated offset values */
        local_params[0] = 0;
	local_params[1] = (1UL << (46 - 32)) | (updated_u_offs << (53 - 32));
	local_params[2] = updated_u_offs >> (64 - 53);
	local_params[2] |= updated_v_offs << (79 - 64);
	local_params[3] |= updated_v_offs >> (96 - 79);


/*       write data to IPU DMA */
	_ipu_write_param_mem(DMAParamAddr(channel_2_dma(
					      vout->post_proc_ch, IPU_INPUT_BUFFER)), local_params, 4);
        
	if (ipu_select_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER,
			      1) < 0) {
	    dev_err(vout->video_dev->dev,
		    "unable to set IPU buffer ready\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_IN_IRQ_HANDLER_UNABLE_SET_IPU, 0) ;
#endif
        }
    
    }

    frame_number_in ++;
	
    spin_unlock_irqrestore(&g_lock, lock_flags);

	
    return IRQ_HANDLED;
}

/* PP output EOF interrupt */
/* This is to synchronize display with PP output
    use the marker set in the PP_in_EOF irq to determine
    which buffer should be displayedi. This permits to make
    sure that we've finished writing to the output buffer
    before we start to display it. 
*/
static irqreturn_t mxc_v4l2out_pp_out_irq_handler(int irq, void *dev_id)
{
    vout_data *vout = dev_id;
    u32 lock_flags = 0;
    int index, buffer;
    unsigned long timeout;
    int last_buf;
    int line, stride, width, height, offset;
    uint32_t llpix, rrpix, lpix1, lpix2, rpix1, rpix2;
    uint32_t lpix1_r, lpix1_g, lpix1_b, lpix2_r, lpix2_g, lpix2_b;
    uint32_t rpix1_r, rpix1_g, rpix1_b, rpix2_r, rpix2_g, rpix2_b;
    uint32_t new_lpix2_r, new_lpix2_g, new_lpix2_b;
    uint32_t new_rpix1_r, new_rpix1_g, new_rpix1_b;
    uint32_t new_rpix1, new_lpix2;

    spin_lock_irqsave(&g_lock, lock_flags);

/*  frame_number_out = 1 : 1st half of buf 0
    frame_number_out = 2 : 2nd half of buf 0 */

    if ((frame_number_out % 2) == 0)
    {
	/* Process previous buffer */
	last_buf = vout->ipu_buf[vout->next_done_ipu_buf];

	if (last_buf != -1) {
	    vout->v4l2_bufs[last_buf].flags = V4L2_BUF_FLAG_DONE;
	    queue_buf(&vout->done_q, last_buf);
	    vout->ipu_buf[vout->next_done_ipu_buf] = -1;
	    wake_up_interruptible(&vout->v4l_bufq);
	} 
/*
  Give the buffer to the display, as the frame is complete */
/*
  frame_number        1   2   3   4   5   6   7   8   ....            
  frame_number%2      1   0   1   0   1   0   1   0   ....
  frame_number/2      0   1   1   2   2   3   3   4   ....
  frame_number/2%2    0   1   1   0   0   1   1   0   ....

*/   
/*  if (frame_number %2 is 0) (frame_number /2 %2) is 1, buffer 0 is ready to be displayed
    otherwise display buffer 1
*/    
 
	if ((frame_number_out / 2) % 2 == 1) 
	    buffer = 0;
	else
	    buffer = 1;
  
	stride = vout->crop_bounds[vout->cur_disp_output].width;
	width = vout->crop_current.width;
	height = vout->crop_current.height;

	/* Filter here before displaying the image */
	for (line = 0; line < height; line++)
	{
	    offset = (2 * stride * line) + width;

	    llpix = *(uint32_t *)(phys_to_virt(vout->display_bufs[buffer] + offset - 4));
	    rrpix = *(uint32_t *)(phys_to_virt(vout->display_bufs[buffer] + offset));
	    
	    lpix1 = (llpix & 0xFFFF0000) >> 16;
	    lpix2 =  llpix & 0x0000FFFF;
	    rpix1 = (rrpix & 0xFFFF0000) >> 16;
	    rpix2 =  rrpix & 0x0000FFFF;
	    
	    lpix1_r = (lpix1 & 0xF800) >> 11;
	    lpix1_g = (lpix1 & 0x07E0) >>  5;
	    lpix1_b =  lpix1 & 0x001F;
	    
	    lpix2_r = (lpix2 & 0xF800) >> 11;
	    lpix2_g = (lpix2 & 0x07E0) >>  5;
	    lpix2_b =  lpix2 & 0x001F;
	    
	    rpix1_r = (rpix1 & 0xF800) >> 11;
	    rpix1_g = (rpix1 & 0x07E0) >>  5;
	    rpix1_b =  rpix1 & 0x001F;
	    
	    rpix2_r = (rpix2 & 0xF800) >> 11;
	    rpix2_g = (rpix2 & 0x07E0) >>  5;
	    rpix2_b =  rpix2 & 0x001F;
	    
	    new_lpix2_r = (lpix1_r + lpix2_r + 2 * rpix1_r) / 4;
	    new_lpix2_g = (lpix1_g + lpix2_g + 2 * rpix1_g) / 4;
	    new_lpix2_b = (lpix1_b + lpix2_b + 2 * rpix1_b) / 4;
	    
	    new_rpix1_r = (2 * lpix2_r + rpix1_r + rpix2_r) / 4;
	    new_rpix1_g = (2 * lpix2_g + rpix1_g + rpix2_g) / 4;
	    new_rpix1_b = (2 * lpix2_b + rpix1_b + rpix2_b) / 4;
	    
	    new_lpix2 = (new_lpix2_r << 11) | (new_lpix2_g << 5) | (new_lpix2_b);
	    new_rpix1 = (new_rpix1_r << 11) | (new_rpix1_g << 5) | (new_rpix1_b);
	    
	    *(uint32_t *)(phys_to_virt(vout->display_bufs[buffer] + offset - 4)) = (lpix2) | (new_lpix2 << 16);
	    *(uint32_t *)(phys_to_virt(vout->display_bufs[buffer] + offset)) = (new_rpix1) | (rpix1 << 16);
	}
          
	if (ipu_select_buffer(vout->display_ch, IPU_INPUT_BUFFER, buffer) < 0)
    {
	    pr_debug("error selecting display buf %i", buffer);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_OUT_IRQ_HANDLER_SELECTING_DISPLAY_BUF, buffer) ;
#endif
    }

	if (vout->state == STATE_STREAM_STOPPING) {
            printk("Stream stopping !!! \n");

	    if ((vout->ipu_buf[0] == -1) && (vout->ipu_buf[1] == -1)) {
		vout->state = STATE_STREAM_OFF;
	    }
	} else if ((vout->state == STATE_STREAM_PAUSED)
		   && ((index = peek_next_buf(&vout->ready_q)) != -1)) {
	    /* Setup timer for next buffer, when stream has been paused */
	    pr_debug("next index %d\n", index);

	    /* if timestamp is 0, then default to 30fps */
	    if ((vout->v4l2_bufs[index].timestamp.tv_sec == 0)
		&& (vout->v4l2_bufs[index].timestamp.tv_usec == 0))
        {
		timeout =
		    vout->start_jiffies + vout->frame_count * HZ / 30;
        }
	    else
        {
		timeout =
		    get_jiffies(&vout->v4l2_bufs[index].timestamp);
        }

	    if (jiffies >= timeout) {
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_OUT_IRQ_HANDLER_TIMER_EXPIRED, 0) ;
#endif
		pr_debug("warning: timer timeout already expired.\n");
	    }

	    vout->state = STATE_STREAM_ON;

	    if (mod_timer(&vout->output_timer, timeout))
{
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_OUT_IRQ_HANDLER_TIMER_ALREADY_SET, 0) ;
#endif
		pr_debug("warning: timer was already set\n");
}

	    pr_debug("timer handler next schedule: %lu\n", timeout);
	}

    }  else {
	if (ipu_select_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER, 1)<0)
	{
	    pr_debug("output buffer %d select error\n", vout->next_rdy_ipu_buf);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_OUT_IRQ_HANDLER_OUT_BUF_SELECT_ERR, vout->next_rdy_ipu_buf) ;
#endif
	} 
    }
 

    frame_number_out++;
    spin_unlock_irqrestore(&g_lock, lock_flags);

    return IRQ_HANDLED;
}

/*!
 * Start the output stream
 *
 * @param vout      structure vout_data *
 *
 * @return status  0 Success
 */
static int mxc_v4l2out_streamon(vout_data * vout)
{
    struct device *dev = vout->video_dev->dev;
    ipu_channel_params_t params;
    struct mxcfb_pos fb_pos;
    struct fb_var_screeninfo fbvar;
    struct fb_info *fbi =
	registered_fb[vout->output_fb_num[vout->cur_disp_output]];
    int pp_in_buf[2];
    u16 out_width;
    u16 out_stride;
    u16 out_height;
    ipu_channel_t display_input_ch = MEM_PP_MEM;
    bool use_direct_adc = false;

    if (!vout)
	return -EINVAL;

    if (vout->state != STATE_STREAM_OFF)
	return -EBUSY;
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_STREAMON, 0) ;
#endif

    if (queue_size(&vout->ready_q) < 2) {
	dev_err(dev, "2 buffers not been queued yet!\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
	return -EINVAL;
    }

    out_width = vout->crop_current.width;
    out_height = vout->crop_current.height;
    if (out_width > 800)
	out_width = 800;

    vout->next_done_ipu_buf = 0;
    vout->next_rdy_ipu_buf = 0;
    /*  modify initial conditions */
    vout->ipu_buf[0] = pp_in_buf[0] = dequeue_buf(&vout->ready_q);
    vout->ipu_buf[1] = pp_in_buf[1] = vout->ipu_buf[0];
    vout->frame_count = 1;
    frame_number_in=1;    
    frame_number_out=1;    

    ipu_enable_irq(IPU_IRQ_PP_IN_EOF);
    /* enable also PP_OUT IRQ */
    ipu_enable_irq(IPU_IRQ_PP_OUT_EOF);

    /* Init Display Channel */
#ifdef CONFIG_FB_MXC_ASYNC_PANEL
    if (vout->cur_disp_output < DISP3) {
	mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_OFF, 0);
	if (vout->rotate < IPU_ROTATE_90_RIGHT) {
	    dev_dbg(dev, "Using PP direct to ADC channel\n");
	    use_direct_adc = true;
	    vout->display_ch = MEM_PP_ADC;
	    vout->post_proc_ch = MEM_PP_ADC;

	    memset(&params, 0, sizeof(params));
	    params.mem_pp_adc.in_width = vout->v2f.fmt.pix.width;
	    params.mem_pp_adc.in_height = vout->v2f.fmt.pix.height;
	    params.mem_pp_adc.in_pixel_fmt =
		vout->v2f.fmt.pix.pixelformat;
	    params.mem_pp_adc.out_width = out_width;
	    params.mem_pp_adc.out_height = out_height;
	    params.mem_pp_adc.out_pixel_fmt = SDC_FG_FB_FORMAT;
#ifdef CONFIG_FB_MXC_EPSON_PANEL
	    params.mem_pp_adc.out_left =
		2 + vout->crop_current.left;
#else
	    params.mem_pp_adc.out_left =
		12 + vout->crop_current.left;
#endif
	    params.mem_pp_adc.out_top = vout->crop_current.top;
	    if (ipu_init_channel(vout->post_proc_ch, &params) != 0) {
		dev_err(dev, "Error initializing PP chan\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }

	    if (ipu_init_channel_buffer(vout->post_proc_ch,
					IPU_INPUT_BUFFER,
					params.mem_pp_adc.
					in_pixel_fmt,
					params.mem_pp_adc.in_width,
					params.mem_pp_adc.in_height,
					vout->v2f.fmt.pix.
					bytesperline /
					bytes_per_pixel(params.
							mem_pp_adc.
							in_pixel_fmt),
					vout->rotate,
					vout->
					v4l2_bufs[pp_in_buf[0]].m.
					offset,
					vout->
					v4l2_bufs[pp_in_buf[1]].m.
					offset,
					vout->offset.u_offset,
					vout->offset.v_offset) !=
		0) {
		dev_err(dev, "Error initializing PP in buf\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }

	    if (ipu_init_channel_buffer(vout->post_proc_ch,
					IPU_OUTPUT_BUFFER,
					params.mem_pp_adc.
					out_pixel_fmt, out_width,
					out_height, out_width,
					vout->rotate, 0, 0, 0,
					0) != 0) {
		dev_err(dev,
			"Error initializing PP output buffer\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }

	} else {
	    dev_dbg(dev, "Using ADC SYS2 channel\n");
	    vout->display_ch = ADC_SYS2;
	    vout->post_proc_ch = MEM_PP_MEM;

	    if (vout->display_bufs[0]) {
		mxc_free_buffers(vout->display_bufs,
				 vout->display_bufs_vaddr,
				 2, vout->display_buf_size);
	    }

	    vout->display_buf_size = vout->crop_current.width *
		vout->crop_current.height *
		fmt_to_bpp(SDC_FG_FB_FORMAT) / 8;
	    mxc_allocate_buffers(vout->display_bufs,
				 vout->display_bufs_vaddr,
				 2, vout->display_buf_size);

	    memset(&params, 0, sizeof(params));
	    params.adc_sys2.disp = vout->cur_disp_output;
	    params.adc_sys2.ch_mode = WriteTemplateNonSeq;
#ifdef CONFIG_FB_MXC_EPSON_PANEL
	    params.adc_sys2.out_left = 2 + vout->crop_current.left;
#else
	    params.adc_sys2.out_left = 12 + vout->crop_current.left;
#endif
	    params.adc_sys2.out_top = vout->crop_current.top;
	    if (ipu_init_channel(ADC_SYS2, &params) < 0)
		return -EINVAL;

	    if (ipu_init_channel_buffer(vout->display_ch,
					IPU_INPUT_BUFFER,
					SDC_FG_FB_FORMAT,
					out_width, out_height,
					out_width, IPU_ROTATE_NONE,
					vout->display_bufs[0],
					vout->display_bufs[1], 0,
					0) != 0) {
		dev_err(dev,
			"Error initializing SDC FG buffer\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }
	}
    } else
#endif
    {			/* Use SDC */
	dev_dbg(dev, "Using SDC channel\n");

	if (vout->cur_disp_output == 3)
	{
	    vout->display_ch = MEM_SDC_FG;

	    fbvar = fbi->var;
	    fbvar.xres = fbvar.xres_virtual = out_width;
	    fbvar.yres = out_height;
	    fbvar.yres_virtual = out_height * 2;
	    fbvar.bits_per_pixel = 16;
	    fb_set_var(fbi, &fbvar);

	    out_stride = out_width;

	    fb_pos.x = vout->crop_current.left;
	    fb_pos.y = vout->crop_current.top;
	    if (fbi->fbops->fb_ioctl)
		fbi->fbops->fb_ioctl(fbi, MXCFB_SET_OVERLAY_POS,
				     (unsigned long)&fb_pos);
		    
	    vout->display_bufs[0] = fbi->fix.smem_start;
	    vout->display_bufs[1] = fbi->fix.smem_start +
		(fbi->fix.line_length * fbi->var.yres);
	    vout->display_buf_size = vout->crop_current.width *
		vout->crop_current.height *
		fmt_to_bpp(SDC_FG_FB_FORMAT) / 8;
	}
	else
	{
	    unsigned long offset;

	    vout->display_ch = MEM_SDC_BG;

	    out_stride = vout->crop_bounds[vout->cur_disp_output].width;
	    offset = (vout->crop_current.top * out_stride + vout->crop_current.left) * 
		(fmt_to_bpp(SDC_FG_FB_FORMAT) / 8);

	    vout->display_bufs[0] = fbi->fix.smem_start + offset;
	    vout->display_bufs[1] = fbi->fix.smem_start + offset +
		(fbi->fix.line_length * fbi->var.yres);
	    vout->display_buf_size = vout->crop_current.width *
		vout->crop_current.height *
		fmt_to_bpp(SDC_FG_FB_FORMAT) / 8;		    
	}

	/* Clear screen */
	memset((char *)fbi->screen_base, 0x0, fbi->fix.smem_len);

	vout->post_proc_ch = MEM_PP_MEM;
    }

    /* Init PP */
    if (use_direct_adc == false) {
	if (vout->rotate >= IPU_ROTATE_90_RIGHT) {
	    out_width = vout->crop_current.height;
	    out_height = vout->crop_current.width;
	}
	memset(&params, 0, sizeof(params));
	params.mem_pp_mem.in_width = vout->v2f.fmt.pix.width;
	params.mem_pp_mem.in_height = vout->v2f.fmt.pix.height;
	params.mem_pp_mem.in_pixel_fmt = vout->v2f.fmt.pix.pixelformat;
	params.mem_pp_mem.out_width = out_width;
	params.mem_pp_mem.out_height = out_height;
	params.mem_pp_mem.out_pixel_fmt = SDC_FG_FB_FORMAT;
	if (ipu_init_channel(vout->post_proc_ch, &params) != 0) {
	    dev_err(dev, "Error initializing PP channel\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
	    return -EINVAL;
	}

	if (ipu_init_channel_buffer(vout->post_proc_ch,
				    IPU_INPUT_BUFFER,
				    params.mem_pp_mem.in_pixel_fmt,
				    /* Update the width of the image : we're upscalig 2 half images */
				    params.mem_pp_mem.in_width / 2,  
				    params.mem_pp_mem.in_height,
				    ((vout->v2f.fmt.pix.bytesperline) /
				     bytes_per_pixel(params.mem_pp_mem.in_pixel_fmt)), 
				    IPU_ROTATE_NONE,
				    (vout->v4l2_bufs[pp_in_buf[0]].m.offset),
				    /* OFFSET second buffer by w/2 */
				    (vout->v4l2_bufs[pp_in_buf[0]].m.offset) +
				    params.mem_pp_mem.in_width / 2, 
				    vout->offset.u_offset,
				    vout->offset.v_offset
				    /*0, 0  *//* set U and V offsets so that they're automatically calculated*/ 
		) != 0) {
	    dev_err(dev, "Error initializing PP input buffer\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
	    return -EINVAL;
	}

	if (vout->rotate >= IPU_ROTATE_90_RIGHT) {
	    if (vout->rot_pp_bufs[0]) {
		mxc_free_buffers(vout->rot_pp_bufs,
				 vout->rot_pp_bufs_vaddr, 2,
				 vout->display_buf_size);
	    }
	    if (mxc_allocate_buffers
		(vout->rot_pp_bufs, vout->rot_pp_bufs_vaddr, 2,
		 vout->display_buf_size) < 0) {
		return -ENOBUFS;
	    }

	    if (ipu_init_channel_buffer(vout->post_proc_ch,
					IPU_OUTPUT_BUFFER,
					params.mem_pp_mem.
					out_pixel_fmt, out_width,
					out_height, out_width,
					IPU_ROTATE_NONE,
					vout->rot_pp_bufs[0],
					vout->rot_pp_bufs[1], 0,
					0) != 0) {
		dev_err(dev,
			"Error initializing PP output buffer\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }

	    if (ipu_init_channel(MEM_ROT_PP_MEM, NULL) != 0) {
		dev_err(dev,
			"Error initializing PP ROT channel\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }

	    if (ipu_init_channel_buffer(MEM_ROT_PP_MEM,
					IPU_INPUT_BUFFER,
					params.mem_pp_mem.
					out_pixel_fmt, out_width,
					out_height, out_width,
					vout->rotate,
					vout->rot_pp_bufs[0],
					vout->rot_pp_bufs[1], 0,
					0) != 0) {
		dev_err(dev,
			"Error initializing PP ROT input buffer\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }

	    /* swap width and height */
	    out_width = vout->crop_current.width;
	    out_height = vout->crop_current.height;

	    if (ipu_init_channel_buffer(MEM_ROT_PP_MEM,
					IPU_OUTPUT_BUFFER,
					params.mem_pp_mem.
					out_pixel_fmt, out_width,
					out_height, out_width,
					IPU_ROTATE_NONE,
					vout->display_bufs[0],
					vout->display_bufs[1], 0,
					0) != 0) {
		dev_err(dev,
			"Error initializing PP output buffer\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }

	    if (ipu_link_channels(vout->post_proc_ch,
				  MEM_ROT_PP_MEM) < 0) {
		return -EINVAL;
	    }
	    ipu_select_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 0);
	    ipu_select_buffer(MEM_ROT_PP_MEM, IPU_OUTPUT_BUFFER, 1);

	    ipu_enable_channel(MEM_ROT_PP_MEM);

	    display_input_ch = MEM_ROT_PP_MEM;
	} else {
	    if (ipu_init_channel_buffer(vout->post_proc_ch,
					IPU_OUTPUT_BUFFER,
					params.mem_pp_mem.
					out_pixel_fmt,
					/* we're processing half images for upscaling */ 
					out_width / 2,
					out_height, 
					out_stride,  
					vout->rotate,
					vout->display_bufs[0],
					vout->display_bufs[0] 
					+ out_width / 2 * bytes_per_pixel(params.mem_pp_mem.in_pixel_fmt), 0,
					0) != 0) {
		dev_err(dev,
			"Error initializing PP output buffer\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
		return -EINVAL;
	    }
	}
	/* unlink display channels for half frame processing */
	if (ipu_unlink_channels(display_input_ch, vout->display_ch) < 0) {

	    dev_err(dev, "Error linking ipu channels\n");
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_ERROR_STREAMON, 0) ;
#endif
	    return -EINVAL;
	}
    }

    vout->state = STATE_STREAM_PAUSED;

    ipu_select_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER, 0);
/*	ipu_select_buffer(vout->post_proc_ch, IPU_INPUT_BUFFER, 1);*/

    if (use_direct_adc == false) {
	ipu_select_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER, 0);
/*		ipu_select_buffer(vout->post_proc_ch, IPU_OUTPUT_BUFFER, 1); */

	ipu_enable_channel(vout->post_proc_ch);
	if ((vout->display_ch == MEM_SDC_FG) ||
	    (vout->display_ch == MEM_SDC_BG)) {
	    acquire_console_sem();
	    fb_blank(fbi, FB_BLANK_UNBLANK);
	    release_console_sem();
	} else {
	    ipu_enable_channel(vout->display_ch);
	}
    } else {
	ipu_enable_channel(vout->post_proc_ch);
    }

    vout->start_jiffies = jiffies;
    dev_dbg(dev,
	    "streamon: start time = %lu jiffies\n", vout->start_jiffies);

    return 0;
}

/*!
 * Shut down the voutera
 *
 * @param vout      structure vout_data *
 *
 * @return status  0 Success
 */
static int mxc_v4l2out_streamoff(vout_data * vout)
{
	struct fb_info *fbi =
	    registered_fb[vout->output_fb_num[vout->cur_disp_output]];
	int i, retval = 0;
	unsigned long lockflag = 0;

	if (!vout)
		return -EINVAL;

	if (vout->state == STATE_STREAM_OFF) {
		return 0;
	}
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_STREAMOFF, 0) ;
#endif
	spin_lock_irqsave(&g_lock, lockflag);

	del_timer(&vout->output_timer);

	if (vout->state == STATE_STREAM_ON) {
		vout->state = STATE_STREAM_STOPPING;
	}

	ipu_disable_irq(IPU_IRQ_PP_IN_EOF);
	ipu_disable_irq(IPU_IRQ_PP_OUT_EOF);

	spin_unlock_irqrestore(&g_lock, lockflag);

	if (vout->post_proc_ch == MEM_PP_MEM) {	/* SDC or ADC with Rotation */
		if (vout->rotate >= IPU_ROTATE_90_RIGHT) {
			ipu_unlink_channels(MEM_PP_MEM, MEM_ROT_PP_MEM);
			ipu_unlink_channels(MEM_ROT_PP_MEM, vout->display_ch);
			ipu_disable_channel(MEM_ROT_PP_MEM, true);
		} else {
			ipu_unlink_channels(MEM_PP_MEM, vout->display_ch);
		}
		ipu_disable_channel(MEM_PP_MEM, true);
		if ((vout->display_ch != MEM_SDC_FG) &&
		    (vout->display_ch != MEM_SDC_BG)) {
		    ipu_disable_channel(vout->display_ch, true);
		    ipu_uninit_channel(vout->display_ch);
		}
		else
		{
		    ipu_select_buffer(vout->display_ch, IPU_INPUT_BUFFER, 0);
		    ipu_enable_channel(vout->display_ch);
		}

		ipu_uninit_channel(MEM_PP_MEM);
		if (vout->rotate >= IPU_ROTATE_90_RIGHT)
		ipu_uninit_channel(MEM_ROT_PP_MEM);
	} else {		/* ADC Direct */
		ipu_disable_channel(MEM_PP_ADC, true);
		ipu_uninit_channel(MEM_PP_ADC);
	}
	vout->ready_q.head = vout->ready_q.tail = 0;
	vout->done_q.head = vout->done_q.tail = 0;
	for (i = 0; i < vout->buffer_cnt; i++) {
		vout->v4l2_bufs[i].flags = 0;
		vout->v4l2_bufs[i].timestamp.tv_sec = 0;
		vout->v4l2_bufs[i].timestamp.tv_usec = 0;
	}

	vout->state = STATE_STREAM_OFF;

	/* Clear screen */
	memset((char *)fbi->screen_base, 0x0, fbi->fix.smem_len);

#ifdef CONFIG_FB_MXC_ASYNC_PANEL
	if (vout->cur_disp_output < DISP3) {
		mxcfb_set_refresh_mode(registered_fb
				       [vout->
					output_fb_num[vout->cur_disp_output]],
				       MXCFB_REFRESH_PARTIAL, 0);
	}
#endif

	return retval;
}

/*
 * Valid whether the palette is supported
 *
 * @param palette  V4L2_PIX_FMT_RGB565, V4L2_PIX_FMT_BGR24 or V4L2_PIX_FMT_BGR32
 *
 * @return 1 if supported, 0 if failed
 */
static inline int valid_mode(u32 palette)
{
	return ((palette == V4L2_PIX_FMT_RGB565) ||
		(palette == V4L2_PIX_FMT_BGR24) ||
		(palette == V4L2_PIX_FMT_RGB24) ||
		(palette == V4L2_PIX_FMT_BGR32) ||
		(palette == V4L2_PIX_FMT_RGB32) ||
		(palette == V4L2_PIX_FMT_YUV422P) ||
		(palette == V4L2_PIX_FMT_YUV420));
}

/*
 * V4L2 - Handles VIDIOC_G_FMT Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param v4l2_format structure v4l2_format *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_v4l2out_g_fmt(vout_data * vout, struct v4l2_format *f)
{
	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		return -EINVAL;
	}
	*f = vout->v2f;
	return 0;
}

/*
 * V4L2 - Handles VIDIOC_S_FMT Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param v4l2_format structure v4l2_format *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_v4l2out_s_fmt(vout_data * vout, struct v4l2_format *f)
{
	int retval = 0;
	u32 size = 0;
	u32 bytesperline;

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		retval = -EINVAL;
		goto err0;
	}
	if (!valid_mode(f->fmt.pix.pixelformat)) {
		dev_err(vout->video_dev->dev, "pixel format not supported\n");
		retval = -EINVAL;
		goto err0;
	}

	bytesperline = (f->fmt.pix.width * fmt_to_bpp(f->fmt.pix.pixelformat)) /
	    8;
	if (f->fmt.pix.bytesperline < bytesperline) {
		f->fmt.pix.bytesperline = bytesperline;
	} else {
		bytesperline = f->fmt.pix.bytesperline;
	}

	switch (f->fmt.pix.pixelformat) {
	case V4L2_PIX_FMT_YUV422P:
		/* byteperline for YUV planar formats is for
		   Y plane only */
		size = bytesperline * f->fmt.pix.height * 2;
		break;
	case V4L2_PIX_FMT_YUV420:
		size = (bytesperline * f->fmt.pix.height * 3) / 2;
		break;
	default:
		size = bytesperline * f->fmt.pix.height;
		break;
	}

	/* Return the actual size of the image to the app */
	if (f->fmt.pix.sizeimage < size) {
		f->fmt.pix.sizeimage = size;
	} else {
		size = f->fmt.pix.sizeimage;
	}

	vout->v2f.fmt.pix = f->fmt.pix;
	if (vout->v2f.fmt.pix.priv != 0) {
		if (copy_from_user(&vout->offset,
				   (void *)vout->v2f.fmt.pix.priv,
				   sizeof(vout->offset))) {
			retval = -EFAULT;
			goto err0;
		}
	}

	retval = 0;
      err0:
	return retval;
}

/*
 * V4L2 - Handles VIDIOC_G_CTRL Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param c           structure v4l2_control *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_get_v42lout_control(vout_data * vout, struct v4l2_control *c)
{
	switch (c->id) {
	case V4L2_CID_HFLIP:
		return (vout->rotate & IPU_ROTATE_HORIZ_FLIP) ? 1 : 0;
	case V4L2_CID_VFLIP:
		return (vout->rotate & IPU_ROTATE_VERT_FLIP) ? 1 : 0;
	case (V4L2_CID_PRIVATE_BASE + 1):
		return vout->rotate;
	default:
		return -EINVAL;
	}
}

/*
 * V4L2 - Handles VIDIOC_S_CTRL Ioctl
 *
 * @param vout         structure vout_data *
 *
 * @param c           structure v4l2_control *
 *
 * @return  status    0 success, EINVAL failed
 */
static int mxc_set_v42lout_control(vout_data * vout, struct v4l2_control *c)
{
	switch (c->id) {
	case V4L2_CID_HFLIP:
		vout->rotate |= c->value ? IPU_ROTATE_HORIZ_FLIP :
		    IPU_ROTATE_NONE;
		break;
	case V4L2_CID_VFLIP:
		vout->rotate |= c->value ? IPU_ROTATE_VERT_FLIP :
		    IPU_ROTATE_NONE;
		break;
	case V4L2_CID_MXC_ROT:
		vout->rotate = c->value;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*!
 * V4L2 interface - open function
 *
 * @param inode        structure inode *
 *
 * @param file         structure file *
 *
 * @return  status    0 success, ENODEV invalid device instance,
 *                    ENODEV timeout, ERESTARTSYS interrupted by user
 */
static int mxc_v4l2out_open(struct inode *inode, struct file *file)
{
	struct video_device *dev = video_devdata(file);
	vout_data *vout = video_get_drvdata(dev);
	int err;

	dq_intr_cnt = 0;
	dq_timeout_cnt = 0;
	if (!vout) {
		return -ENODEV;
	}

	down(&vout->busy_lock);

	err = -EINTR;
	if (signal_pending(current))
		goto oops;

	if (vout->open_count++ == 0) {
		ipu_request_irq(IPU_IRQ_PP_IN_EOF,
				mxc_v4l2out_pp_in_irq_handler,
				0, dev->name, vout);

        /* MN PP output IRQ registration */
		ipu_request_irq(IPU_IRQ_PP_OUT_EOF,
				mxc_v4l2out_pp_out_irq_handler,
				/*mxc_v4l2out_pp_in_irq_handler,*/
				0, dev->name, vout);

		init_waitqueue_head(&vout->v4l_bufq);

		init_timer(&vout->output_timer);
		vout->output_timer.function = mxc_v4l2out_timer_handler;
		vout->output_timer.data = (unsigned long)vout;

		vout->state = STATE_STREAM_OFF;
	}

	file->private_data = dev;

	up(&vout->busy_lock);

	return 0;

      oops:
	up(&vout->busy_lock);
	return err;
}

/*!
 * V4L2 interface - close function
 *
 * @param inode    struct inode *
 *
 * @param file     struct file *
 *
 * @return         0 success
 */
static int mxc_v4l2out_close(struct inode *inode, struct file *file)
{
	struct video_device *dev = video_devdata(file);
	vout_data *vout = video_get_drvdata(dev);
	int i;

	if (--vout->open_count == 0) {
		if (vout->state != STATE_STREAM_OFF)
			mxc_v4l2out_streamoff(vout);

		ipu_free_irq(IPU_IRQ_PP_IN_EOF, vout);
		/*MN free previously requested irq */
        ipu_free_irq(IPU_IRQ_PP_OUT_EOF, vout);

		file->private_data = NULL;

#if !defined(CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS)
		mxc_free_buffers(vout->queue_buf_paddr, vout->queue_buf_vaddr,
				 vout->buffer_cnt, vout->queue_buf_size);
#else
		/* Re-initialize static buffers to avoid artefacts on next video */
		for (i = 0; i < vout->static_buffer_cnt; i++)
		    memset(vout->static_queue_buf_vaddr[i], 0, vout->static_queue_buf_size);

		memset(vout->queue_buf_paddr, 0, sizeof(vout->queue_buf_paddr));
		memset(vout->queue_buf_vaddr, 0, sizeof(vout->queue_buf_vaddr));
#endif /* CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS */
		vout->buffer_cnt = 0;
		mxc_free_buffers(vout->rot_pp_bufs, vout->rot_pp_bufs_vaddr, 2,
				 vout->display_buf_size);

		/* capture off */
		wake_up_interruptible(&vout->v4l_bufq);
	}

	return 0;
}

/*!
 * V4L2 interface - ioctl function
 *
 * @param inode      struct inode *
 *
 * @param file       struct file *
 *
 * @param ioctlnr    unsigned int
 *
 * @param arg        void *
 *
 * @return           0 success, ENODEV for invalid device instance,
 *                   -1 for other errors.
 */
static int
mxc_v4l2out_do_ioctl(struct inode *inode, struct file *file,
		     unsigned int ioctlnr, void *arg)
{
	struct video_device *vdev = file->private_data;
	vout_data *vout = video_get_drvdata(vdev);
	int retval = 0;
	int i = 0;

	if (!vout)
		return -EBADF;

	/* make this _really_ smp-safe */
	if (down_interruptible(&vout->busy_lock))
		return -EBUSY;

	switch (ioctlnr) {
	case VIDIOC_QUERYCAP:
		{
			struct v4l2_capability *cap = arg;
			strcpy(cap->driver, "mxc_v4l2_output");
			cap->version = 0;
			cap->capabilities =
			    V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
			cap->card[0] = '\0';
			cap->bus_info[0] = '\0';
			retval = 0;
			break;
		}
	case VIDIOC_G_FMT:
		{
			struct v4l2_format *gf = arg;
			retval = mxc_v4l2out_g_fmt(vout, gf);
			break;
		}
	case VIDIOC_S_FMT:
		{
			struct v4l2_format *sf = arg;
			if (vout->state != STATE_STREAM_OFF) {
				retval = -EBUSY;
				break;
			}
			retval = mxc_v4l2out_s_fmt(vout, sf);
			break;
		}
	case VIDIOC_REQBUFS:
		{
			struct v4l2_requestbuffers *req = arg;
			if ((req->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) ||
			    (req->memory != V4L2_MEMORY_MMAP)) {
				dev_dbg(vdev->dev,
					"VIDIOC_REQBUFS: incorrect buffer type\n");
				retval = -EINVAL;
				break;
			}

			if (req->count == 0)
				mxc_v4l2out_streamoff(vout);

			if (vout->state == STATE_STREAM_OFF) {
#if !defined(CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS)
				if (vout->queue_buf_paddr[0] != 0) {
					mxc_free_buffers(vout->queue_buf_paddr,
							 vout->queue_buf_vaddr,
							 vout->buffer_cnt,
							 vout->queue_buf_size);
					dev_dbg(vdev->dev,
						"VIDIOC_REQBUFS: freed buffers\n");
				}
#endif /* CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS */
				vout->buffer_cnt = 0;
			} else {
				dev_dbg(vdev->dev,
					"VIDIOC_REQBUFS: Buffer is in use\n");
				retval = -EBUSY;
				break;
			}

			if (req->count == 0)
				break;

			if (req->count < MIN_FRAME_NUM) {
				req->count = MIN_FRAME_NUM;
			} else if (req->count > MAX_FRAME_NUM) {
				req->count = MAX_FRAME_NUM;
			}
			
#if defined(CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS)
			if (req->count > vout->static_buffer_cnt || 
			    vout->v2f.fmt.pix.sizeimage > vout->static_queue_buf_size)
			{
			    printk("V4L2: Request out of capabilities:\n\tbuffers: %i => %i\n\tmemory: %i => %i\n",
				   req->count, vout->static_buffer_cnt, vout->v2f.fmt.pix.sizeimage, vout->static_queue_buf_size);
			    break ;
			}
			
			vout->buffer_cnt = req->count;
			vout->queue_buf_size = PAGE_ALIGN(vout->v2f.fmt.pix.sizeimage);
			
			/* Init buffer queues */
			vout->done_q.head = 0;
			vout->done_q.tail = 0;
			vout->ready_q.head = 0;
			vout->ready_q.tail = 0;
			
			for (i = 0; i < vout->buffer_cnt; i++) 
			{
			    memset(&(vout->v4l2_bufs[i]), 0, sizeof(vout->v4l2_bufs[i]));
			    vout->v4l2_bufs[i].flags = 0;
			    vout->v4l2_bufs[i].memory = V4L2_MEMORY_MMAP;
			    vout->v4l2_bufs[i].index = i;
			    vout->v4l2_bufs[i].type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			    vout->v4l2_bufs[i].length =	PAGE_ALIGN(vout->v2f.fmt.pix.sizeimage);
			    vout->v4l2_bufs[i].m.offset = (unsigned long)vout->static_queue_buf_paddr[i];
			    vout->v4l2_bufs[i].timestamp.tv_sec = 0;
			    vout->v4l2_bufs[i].timestamp.tv_usec = 0;
			}
#else /* !CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS */
			vout->buffer_cnt = req->count;
			vout->queue_buf_size =
			    PAGE_ALIGN(vout->v2f.fmt.pix.sizeimage);

			retval = mxc_allocate_buffers(vout->queue_buf_paddr,
						      vout->queue_buf_vaddr,
						      vout->buffer_cnt,
						      vout->queue_buf_size);
			if (retval < 0)
				break;

			/* Init buffer queues */
			vout->done_q.head = 0;
			vout->done_q.tail = 0;
			vout->ready_q.head = 0;
			vout->ready_q.tail = 0;

			for (i = 0; i < vout->buffer_cnt; i++) {
				memset(&(vout->v4l2_bufs[i]), 0,
				       sizeof(vout->v4l2_bufs[i]));
				vout->v4l2_bufs[i].flags = 0;
				vout->v4l2_bufs[i].memory = V4L2_MEMORY_MMAP;
				vout->v4l2_bufs[i].index = i;
				vout->v4l2_bufs[i].type =
				    V4L2_BUF_TYPE_VIDEO_OUTPUT;
				vout->v4l2_bufs[i].length =
				    PAGE_ALIGN(vout->v2f.fmt.pix.sizeimage);
				vout->v4l2_bufs[i].m.offset =
				    (unsigned long)vout->queue_buf_paddr[i];
				vout->v4l2_bufs[i].timestamp.tv_sec = 0;
				vout->v4l2_bufs[i].timestamp.tv_usec = 0;
			}
#endif /* CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS */
			break;
		}
	case VIDIOC_QUERYBUF:
		{
			struct v4l2_buffer *buf = arg;
			u32 type = buf->type;
			int index = buf->index;

			if ((type != V4L2_BUF_TYPE_VIDEO_OUTPUT) ||
			    (index >= vout->buffer_cnt)) {
				dev_dbg(vdev->dev,
					"VIDIOC_QUERYBUFS: incorrect buffer type\n");
				retval = -EINVAL;
				break;
			}
			down(&vout->param_lock);
			memcpy(buf, &(vout->v4l2_bufs[index]), sizeof(*buf));
			up(&vout->param_lock);
			break;
		}
	case VIDIOC_QBUF:
		{
			struct v4l2_buffer *buf = arg;
			int index = buf->index;
			unsigned long lock_flags;

			if ((buf->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) ||
			    (index >= vout->buffer_cnt)) {
				retval = -EINVAL;
				break;
			}

			dev_dbg(vdev->dev, "VIDIOC_QBUF: %d\n", buf->index);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_QBUF, buf->index) ;
#endif

			if (cache_mode == 1){
			    v6_flush_kern_cache_all_l2(); // overkill, invalidate the entire cache, but it's to check if we can flush something ;-)
			    // called in mxc_pf.c 
			}

			spin_lock_irqsave(&g_lock, lock_flags);

			memcpy(&(vout->v4l2_bufs[index]), buf, sizeof(*buf));
			vout->v4l2_bufs[index].flags |= V4L2_BUF_FLAG_QUEUED;

			queue_buf(&vout->ready_q, index);
			if (vout->state == STATE_STREAM_PAUSED) {
				unsigned long timeout;

				index = peek_next_buf(&vout->ready_q);

				/* if timestamp is 0, then default to 30fps */
				if ((vout->v4l2_bufs[index].timestamp.tv_sec ==
				     0)
				    && (vout->v4l2_bufs[index].timestamp.
					tv_usec == 0))
					timeout =
					    vout->start_jiffies +
					    vout->frame_count * HZ / 30;
				else
					timeout =
					    get_jiffies(&vout->v4l2_bufs[index].
							timestamp);

				if (jiffies >= timeout) {
					dev_dbg(vout->video_dev->dev,
						"warning: timer timeout already expired.\n");
				}
				vout->output_timer.expires = timeout;
				dev_dbg(vdev->dev,
					"QBUF: frame #%u timeout @ %lu jiffies, current = %lu\n",
					vout->frame_count, timeout, jiffies);
				add_timer(&vout->output_timer);
				vout->state = STATE_STREAM_ON;
			}

			spin_unlock_irqrestore(&g_lock, lock_flags);
			break;
		}
	case VIDIOC_DQBUF:
		{
			struct v4l2_buffer *buf = arg;
			int idx;

			if ((queue_size(&vout->done_q) == 0) &&
			    (file->f_flags & O_NONBLOCK)) {
				retval = -EAGAIN;
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_DQBUF_ERROR, 0) ;
#endif
				break;
			}

			if (!wait_event_interruptible_timeout(vout->v4l_bufq,
							      queue_size(&vout->
									 done_q)
							      != 0, 10 * HZ)) {
				if(dq_timeout_cnt == 0){
					dev_dbg(vdev->dev, "VIDIOC_DQBUF: timeout\n");
				}
				dq_timeout_cnt++;
				retval = -ETIME;
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_DQBUF_ERROR, 0) ;
#endif
				break;
			} else if (signal_pending(current)) {
				if (dq_intr_cnt == 0) {
					dev_dbg(vdev->dev,
						"VIDIOC_DQBUF: interrupt received\n");
				}
				dq_intr_cnt++;
				retval = -ERESTARTSYS;
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_DQBUF_ERROR, 0) ;
#endif
				break;
			}
			idx = dequeue_buf(&vout->done_q);
			if (idx == -1) {	/* No frame free */
				dev_dbg(vdev->dev,
					"VIDIOC_DQBUF: no free buffers, returning\n");
				retval = -EAGAIN;
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_DQBUF_ERROR, 0) ;
#endif
				break;
			}
			if ((vout->v4l2_bufs[idx].flags & V4L2_BUF_FLAG_DONE) ==
			    0)
				dev_dbg(vdev->dev,
					"VIDIOC_DQBUF: buffer in done q, but not "
					"flagged as done\n");

			vout->v4l2_bufs[idx].flags = 0;
			memcpy(buf, &(vout->v4l2_bufs[idx]), sizeof(*buf));
			dev_dbg(vdev->dev, "VIDIOC_DQBUF: %d\n", buf->index);
#if defined(DO_ULOG)
    ulog(ULOG_V4L2OUT_DQBUF, buf->index) ;
#endif
			break;
		}
	case VIDIOC_STREAMON:
		{
			retval = mxc_v4l2out_streamon(vout);
			break;
		}
	case VIDIOC_STREAMOFF:
		{
			retval = mxc_v4l2out_streamoff(vout);
			break;
		}
	case VIDIOC_G_CTRL:
		{
			retval = mxc_get_v42lout_control(vout, arg);
			break;
		}
	case VIDIOC_S_CTRL:
		{
			retval = mxc_set_v42lout_control(vout, arg);
			break;
		}
	case VIDIOC_CROPCAP:
		{
			struct v4l2_cropcap *cap = arg;

			if (cap->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				retval = -EINVAL;
				break;
			}
			cap->bounds = vout->crop_bounds[vout->cur_disp_output];
			cap->defrect = vout->crop_bounds[vout->cur_disp_output];
			retval = 0;
			break;
		}
	case VIDIOC_G_CROP:
		{
			struct v4l2_crop *crop = arg;

			if (crop->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				retval = -EINVAL;
				break;
			}
			crop->c = vout->crop_current;
			break;
		}
	case VIDIOC_S_CROP:
		{
			struct v4l2_crop *crop = arg;
			struct v4l2_rect *b =
			    &(vout->crop_bounds[vout->cur_disp_output]);

			if (crop->type != V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				retval = -EINVAL;
				break;
			}
			if (crop->c.height < 0) {
				retval = -EINVAL;
				break;
			}
			if (crop->c.width < 0) {
				retval = -EINVAL;
				break;
			}

			if (crop->c.top < b->top)
				crop->c.top = b->top;
			if (crop->c.top >= b->top + b->height)
				crop->c.top = b->top + b->height - 1;
			if (crop->c.height > b->top - crop->c.top + b->height)
				crop->c.height =
				    b->top - crop->c.top + b->height;

			if (crop->c.left < b->left)
				crop->c.left = b->left;
			if (crop->c.left >= b->left + b->width)
				crop->c.left = b->left + b->width - 1;
			if (crop->c.width > b->left - crop->c.left + b->width)
				crop->c.width =
				    b->left - crop->c.left + b->width;

			/* stride line limitation */
			crop->c.height -= crop->c.height % 8;
			crop->c.width -= crop->c.width % 8;

			vout->crop_current = crop->c;
			break;
		}
	case VIDIOC_ENUMOUTPUT:
		{
			struct v4l2_output *output = arg;

			if ((output->index >= 5) ||
			    (vout->output_enabled[output->index] == false)) {
				retval = -EINVAL;
				break;
			}

			if (output->index < 3) {
				*output = mxc_outputs[MXC_V4L2_OUT_2_ADC];
				output->name[4] = '0' + output->index;
			} else {
				*output = mxc_outputs[MXC_V4L2_OUT_2_SDC];
			}
			break;
		}
	case VIDIOC_G_OUTPUT:
		{
			int *p_output_num = arg;

			*p_output_num = vout->cur_disp_output;
			break;
		}
	case VIDIOC_S_OUTPUT:
		{
			int *p_output_num = arg;

			if ((*p_output_num >= 5) ||
			    (vout->output_enabled[*p_output_num] == false)) {
				retval = -EINVAL;
				break;
			}

			if (vout->state != STATE_STREAM_OFF) {
				retval = -EBUSY;
				break;
			}

			vout->cur_disp_output = *p_output_num;
			vout->crop_current =
			    vout->crop_bounds[vout->cur_disp_output];
			break;
		}
	case VIDIOC_ENUM_FMT:
	case VIDIOC_TRY_FMT:
	case VIDIOC_QUERYCTRL:
	case VIDIOC_G_PARM:
	case VIDIOC_ENUMSTD:
	case VIDIOC_G_STD:
	case VIDIOC_S_STD:
	case VIDIOC_G_TUNER:
	case VIDIOC_S_TUNER:
	case VIDIOC_G_FREQUENCY:
	case VIDIOC_S_FREQUENCY:
	default:
		retval = -EINVAL;
		break;
	}

	up(&vout->busy_lock);
	return retval;
}

/*
 * V4L2 interface - ioctl function
 *
 * @return  None
 */
static int
mxc_v4l2out_ioctl(struct inode *inode, struct file *file,
		  unsigned int cmd, unsigned long arg)
{
	return video_usercopy(inode, file, cmd, arg, mxc_v4l2out_do_ioctl);
}

/*!
 * V4L2 interface - mmap function
 *
 * @param file          structure file *
 *
 * @param vma           structure vm_area_struct *
 *
 * @return status       0 Success, EINTR busy lock error,
 *                      ENOBUFS remap_page error
 */
static int mxc_v4l2out_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *vdev = video_devdata(file);
	unsigned long size = vma->vm_end - vma->vm_start;
	int res = 0;
	int i;
	vout_data *vout = video_get_drvdata(vdev);

	dev_dbg(vdev->dev, "pgoff=0x%lx, start=0x%lx, end=0x%lx\n",
		vma->vm_pgoff, vma->vm_start, vma->vm_end);

	/* make this _really_ smp-safe */
	if (down_interruptible(&vout->busy_lock))
		return -EINTR;

	for (i = 0; i < vout->buffer_cnt; i++) {
		if ((vout->v4l2_bufs[i].m.offset ==
		     (vma->vm_pgoff << PAGE_SHIFT)) &&
		    (vout->v4l2_bufs[i].length >= size)) {
			vout->v4l2_bufs[i].flags |= V4L2_BUF_FLAG_MAPPED;
			break;
		}
	}
	if (i == vout->buffer_cnt) {
		res = -ENOBUFS;
		goto mxc_mmap_exit;
	}

	/* make buffers write-thru cacheable */
	//vma->vm_page_prot = __pgprot(pgprot_val(vma->vm_page_prot) &
	//			     ~L_PTE_BUFFERABLE);

	/* make buffers cacheable and / or buffereable */
       
	if (cache_mode == 1) {
	    vma->vm_page_prot = __pgprot(pgprot_val(vma->vm_page_prot) | (L_PTE_CACHEABLE | L_PTE_BUFFERABLE));
	} 
	else {
	    if(cache_mode == 0) {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	    }
	    else {
		vma->vm_page_prot = __pgprot((pgprot_val(vma->vm_page_prot) | L_PTE_BUFFERABLE) & ~(L_PTE_CACHEABLE));
	    }
	}

	if (remap_pfn_range(vma, vma->vm_start,
			    vma->vm_pgoff, size, vma->vm_page_prot)) {
		dev_dbg(vdev->dev, "mmap remap_pfn_range failed\n");
		res = -ENOBUFS;
		goto mxc_mmap_exit;
	}

	vma->vm_flags &= ~VM_IO;	/* using shared anonymous pages */

      mxc_mmap_exit:
	up(&vout->busy_lock);
	return res;
}

/*!
 * V4L2 interface - poll function
 *
 * @param file       structure file *
 *
 * @param wait       structure poll_table *
 *
 * @return  status   POLLIN | POLLRDNORM
 */
static unsigned int mxc_v4l2out_poll(struct file *file, poll_table * wait)
{
	struct video_device *dev = video_devdata(file);
	vout_data *vout = video_get_drvdata(dev);

	wait_queue_head_t *queue = NULL;
	int res = POLLIN | POLLRDNORM;

	if (down_interruptible(&vout->busy_lock))
		return -EINTR;

	queue = &vout->v4l_bufq;
	poll_wait(file, queue, wait);

	up(&vout->busy_lock);
	return res;
}

static struct
file_operations mxc_v4l2out_fops = {
	.owner = THIS_MODULE,
	.open = mxc_v4l2out_open,
	.release = mxc_v4l2out_close,
	.ioctl = mxc_v4l2out_ioctl,
	.mmap = mxc_v4l2out_mmap,
	.poll = mxc_v4l2out_poll,
};

static struct video_device mxc_v4l2out_template = {
	.owner = THIS_MODULE,
	.name = "MXC Video Output",
	.type = 0,
	.type2 = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING,
	.hardware = 0,
	.fops = &mxc_v4l2out_fops,
	.release = video_device_release,
};

/*!
 * Probe routine for the framebuffer driver. It is called during the
 * driver binding process.      The following functions are performed in
 * this routine: Framebuffer initialization, Memory allocation and
 * mapping, Framebuffer registration, IPU initialization.
 *
 * @return      Appropriate error code to the kernel common code
 */
static int mxc_v4l2out_probe(struct platform_device *pdev)
{
	int i;
	vout_data *vout;

	/*
	 * Allocate sufficient memory for the fb structure
	 */
	g_vout = vout = kmalloc(sizeof(vout_data), GFP_KERNEL);

	if (!vout)
		return 0;

	memset(vout, 0, sizeof(vout_data));

	vout->video_dev = video_device_alloc();
	if (vout->video_dev == NULL)
		return -1;
	vout->video_dev->dev = &pdev->dev;
	vout->video_dev->minor = -1;

	*(vout->video_dev) = mxc_v4l2out_template;

	/* register v4l device */
	if (video_register_device(vout->video_dev,
				  VFL_TYPE_GRABBER, video_nr) == -1) {
		dev_dbg(&pdev->dev, "video_register_device failed\n");
		return 0;
	}
	dev_info(&pdev->dev, "Registered device video%d\n",
		 vout->video_dev->minor & 0x1f);
	vout->video_dev->dev = &pdev->dev;

	video_set_drvdata(vout->video_dev, vout);

	init_MUTEX(&vout->param_lock);
	init_MUTEX(&vout->busy_lock);

	/* setup outputs and cropping */
	vout->cur_disp_output = -1;
	for (i = 0; i < num_registered_fb; i++) {
		char *idstr = registered_fb[i]->fix.id;
		if (strncmp(idstr, "DISP", 4) == 0) {
			int disp_num = idstr[4] - '0';
			if ((disp_num == 3) &&
			    (strncmp(idstr, "DISP3 BG", 8) == 0)) {
				disp_num = 4;
			}
			vout->crop_bounds[disp_num].left = 0;
			vout->crop_bounds[disp_num].top = 0;
			vout->crop_bounds[disp_num].width =
			    registered_fb[i]->var.xres;
			vout->crop_bounds[disp_num].height =
			    registered_fb[i]->var.yres;
			vout->output_enabled[disp_num] = true;
			vout->output_fb_num[disp_num] = i;
			if (vout->cur_disp_output == -1) {
				vout->cur_disp_output = disp_num;
			}
		}

	}
	vout->crop_current = vout->crop_bounds[vout->cur_disp_output];

#if defined(CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS)
	{
	    int retval;

	    vout->static_buffer_cnt = K_V4L2_STATIC_BUFFERS_NUM;
	    vout->static_queue_buf_size = PAGE_ALIGN(K_V4L2_STATIC_BUFFERS_SIZE);
	    
	    retval = mxc_allocate_buffers(vout->static_queue_buf_paddr,
					  vout->static_queue_buf_vaddr,
					  vout->static_buffer_cnt,
					  vout->static_queue_buf_size);
	    if (retval)
		vout->static_buffer_cnt = 0;
	}
#endif /* CONFIG_VIDEO_MXC_OUTPUT_STATIC_BUFFERS */

	platform_set_drvdata(pdev, vout);

	return 0;
}

static int mxc_v4l2out_remove(struct platform_device *pdev)
{
	vout_data *vout = platform_get_drvdata(pdev);

	if (vout->video_dev) {
		if (-1 != vout->video_dev->minor)
			video_unregister_device(vout->video_dev);
		else
			video_device_release(vout->video_dev);
		vout->video_dev = NULL;
	}

	platform_set_drvdata(pdev, NULL);

	kfree(vout);

	return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxc_v4l2out_driver = {
	.driver = {
		   .name = "MXC Video Output",
		   },
	.probe = mxc_v4l2out_probe,
	.remove = mxc_v4l2out_remove,
};

static struct platform_device mxc_v4l2out_device = {
	.name = "MXC Video Output",
	.id = 0,
};

/*!
 * mxc v4l2 init function
 *
 */
static int mxc_v4l2out_init(void)
{
	u8 err = 0;

	err = platform_driver_register(&mxc_v4l2out_driver);
	if (err == 0) {
		platform_device_register(&mxc_v4l2out_device);
	}
	return err;
}

/*!
 * mxc v4l2 cleanup function
 *
 */
static void mxc_v4l2out_clean(void)
{
	video_unregister_device(g_vout->video_dev);

	platform_driver_unregister(&mxc_v4l2out_driver);
	platform_device_unregister(&mxc_v4l2out_device);
	kfree(g_vout);
	g_vout = NULL;
}

module_init(mxc_v4l2out_init);
module_exit(mxc_v4l2out_clean);

module_param(video_nr, int, 0444);
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("V4L2-driver for MXC video output");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("video");
