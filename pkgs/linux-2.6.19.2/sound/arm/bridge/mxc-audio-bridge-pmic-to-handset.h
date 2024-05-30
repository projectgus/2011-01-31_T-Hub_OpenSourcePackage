/*
 *  Header file of DECT audio bridge driver.
 *
 *  Copyright (C) 2006 - 2009 Sagemcom All rights reserved
 *
 *  File name: mxc-audio-bridge-pmic-to-handset.c
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

#ifndef AUDIO_BRIDGE_PMIC_TO_HANDSET
#define AUDIO_BRIDGE_PMIC_TO_HANDSET


#define AUDIO_BRIDGE_MODULE_NAME	"Audio_Bridge" 

#ifdef __KERNEL__

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysctl.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <dectusb_audio.h>
#include <linux/dma-mapping.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
#include <sound/driver.h>
#endif
#include <sound/pcm.h>
#include <sound/core.h>
#include <sound/sound_bridge.h>



#define AUDIO_BRIDGE_NODE_NUMBER	190
#define BRIDGE_FILE_OPS_DECTUSB		0x1
#define BRIDGE_FILE_OPS_PMIC		0x2

/*Define errors*/
#define PLAYBACK_ELASPSE_COUNT_ERR     1
#define CAPTURE_ELAPSE_COUNT_ERR       2
#define COMPENSATION_OVERRUN           3
#define COMPENSATION_UNDERRUN          4



/*Define Contexts, used by methods to discriminate use cases*/
#define AT_NO_CONTEXT                   0
#define AT_CAPTURE_ELAPSED              1   /*in capture_elapsed method,  atomic context, before binfo update*/
#define AT_PLAYBACK_ELAPSED             2   /*in playback_elapsed method, atomic context, before binfo update*/

/*
 * This structure is the heart of buffer management. it is 
 *     used by the audio bridge itself and its client     
 *
 */
struct bridge_buffer_info {

	short         	        *buffer;     /* Pointer to buffer of sample of size_of(short) length */
	unsigned int            buffer_size; /* buffer size in samples set by audio bridge*/
	dma_addr_t 		dma_addr;    /* Physical address of the buffer */

	spinlock_t              lock;  /*TODO to be initialized with spin_lock_init(&lock);*/
	
	unsigned int            min_period_size;       /*min(playback_period_size,capture_period_size)*/
	unsigned int            max_period_size;       /*max(playback_period_size,capture_period_size)*/

	/*Playback*/
	unsigned int            playback_period_size; 	/*number of samples consumed per period*/
	unsigned int            playback_chained; 	/*! Set to 1 if driver is chaining transfer */
	unsigned int		playback_offset;      	/*offset of playback data in buffer*/

	/*Capture*/
	unsigned int            capture_period_size;  /*number of samples capture per period*/
	unsigned int            capture_chained; 	/*! Set to 1 if driver is chaining transfer */
	unsigned int		capture_offset;       /*offset of capture data in buffer*/

	int			target_delay;
	int			delay_tolerance_hight;
	int			delay_tolerance_low;
	

	unsigned int		overrun;              
	unsigned int		underrun;



#ifdef CONFIG_RELAY_TO_DEBUGFS
#define SUBBUF_SIZE 16000
#define N_SUBBUFS 10
	struct rchan 		*playback_rchan;
	struct rchan            *capture_rchan;
#endif

};


/* 
 * This structure is filled by the drivers before registration 
 *     to the audio bridge. It contains standard playback and 
 *     capture methods as well as variables used by the drivers
 *     for identifying there streams.
 *     
 */
#define SIZE_MAX_BRIDGE_FOPS_NAME	32
struct audio_bridge_operations {

	void *private_data; 


	int (*playback_open) (void *private_data);   /*prepare stream before start*/
	void (*playback_close) (void *private_data); 
	int (*playback_start) (void *private_data, void *src, int period_size, void *bridge_ctx, elapsed_t callback);
	int (*playback_stop) (void *private_data);
	int playback_device_number;
	int playback_stream_id;
	
	int (*capture_open) (void *private_data);    /*prepare capture before start*/
	void (*capture_close) (void *private_data);  
	int (*capture_start) (void *private_data, void *src, int period_size, void *bridge_ctx, elapsed_t callback);
	int (*capture_stop) (void *private_data);
	int capture_stream_id;
	int capture_device_number;
	
	struct bridge_buffer_info *buffer_infos;
	
	
	int period_size; //previously "min_period_size"


	char name[SIZE_MAX_BRIDGE_FOPS_NAME];
	char dummy[128];
};



/*register/unregister methods for the audio bridge*/
struct bridge_stream *register_audio_bridge(struct audio_bridge_operations *pops);
void unregister_audio_bridge(struct bridge_stream *pstream);

/* Period size in samples */
int start_chaning_playback_dma(struct bridge_stream *pstream, void **src, int *period_size);
int start_chaining_capture_dma(struct bridge_stream *pstream, void **src, int *period_size);

#endif /* __KERNEL__ */


#endif
