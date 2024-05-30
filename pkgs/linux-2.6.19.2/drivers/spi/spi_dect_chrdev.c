/*
 *  Spi char device for dect module.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: spi_dect_chrdev.c
 *  Creation date: 17/07/2009
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

#include "spi_dect.h"
#include <asm/uaccess.h>

struct spi_transfer spit;
struct spi_message spim ;
 
extern void spi_dect_clear_exported_vars(void);
extern int spi_dect_start_stop_activity(void);
extern void spi_dect_set_basc_on(void);
extern int spi_dect_add_message_to_send(struct spi_dect_context *spd, const unsigned char *msg, const int size);
extern void spi_dect_set_mode(struct spi_device *spi);
extern void spi_dect_set_spi_loopback(struct spi_device *spi);

static ssize_t spi_dect_open(struct inode *inode, struct file *file) {
	struct spi_dect_context *spi_dect_ctx = NULL;
	int ret = -ENXIO;
	mutex_lock(&spi_dect_dev_list_lock);
	list_for_each_entry(spi_dect_ctx, &spi_dect_dev_list, device_entry){
		if(spi_dect_ctx->devt == inode->i_rdev){
			ret = 0;
			break;
		}
	}
	if(ret){
		mutex_unlock(&spi_dect_dev_list_lock);
		return ret;
	}
	spi_dect_ctx->in_use++;
	
	file->private_data = spi_dect_ctx;
	nonseekable_open(inode, file);
	mutex_unlock(&spi_dect_dev_list_lock);
	return 0;
}

static ssize_t spi_dect_release(struct inode *inode, struct file *file){
	struct spi_dect_context *spi_dect_ctx = file->private_data;
	if(!spi_dect_ctx)
		return -EFAULT;
	
	mutex_lock(&spi_dect_dev_list_lock);
	file->private_data = NULL;
	spi_dect_ctx->in_use--;
	mutex_unlock(&spi_dect_dev_list_lock);
	return 0;
}

static ssize_t spi_dect_read(struct file *file, char *bdest, size_t r, loff_t *pos){
	printk("%s() %d : Read : Nothing is done here.\n", __func__, __LINE__);
	return r;
}

static ssize_t spi_dect_write(struct file *file, const char *src, size_t size, loff_t *pos){
	struct spi_dect_context *spi_dect_ctx = file->private_data;
	u8 *buffer;
	ssize_t ret;

	if(!src)
		return -EFAULT;

	if(size < 0)
		return -EINVAL;

	if(!size)
		return 0;

	if(!spi_dect_ctx)
		return -EFAULT;

	buffer = kmalloc(size, GFP_KERNEL);
	if(!buffer){
		dev_err(&spi_dect_ctx->spi->dev, "Error : could not alloc space to get buffer from user space\n");
		return -ENOBUFS;
	}

	if(copy_from_user(buffer, src, size)){
		dev_err(&spi_dect_ctx->spi->dev, "Error : Could not copy data from user space\n");
		kfree(buffer);
		return -EFAULT;
	}

	ret = spi_dect_add_message_to_send(spi_dect_ctx, buffer, size);
	if(ret){
		dev_err(&spi_dect_ctx->spi->dev, "Error : Could not queue data\n");
		kfree(buffer);
		return ret;
	}

	dev_dbg(&spi_dect_ctx->spi->dev, "Writing (%d) bytes (ret %d)\n", size, ret);

	kfree(buffer);
	return ret?ret:size;
}

static int spi_dect_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
	struct spi_dect_context *spi_dect_ctx = file->private_data;

	if(!spi_dect_ctx)
		return -EFAULT;

	switch(cmd){

		case 1: spi_dect_start_stop_activity();break;
		case 2: spi_dect_clear_exported_vars();break;
		case 3:	spi_dect_set_basc_on();break;
		case 4: spi_dect_set_mode(spi_dect_ctx->spi);break;
		case 5: spi_dect_set_spi_loopback(spi_dect_ctx->spi);break;
		default:
			dev_err(&spi_dect_ctx->spi->dev, "ioctl :\tdefault case (%d)\n\t\tioctl 1 : start/stop activity\n\t\tioctl 2 : clear exported vars\n\t\tioctl 3 : send message to dect\n\t\tioctl 4 : Set mode (0-3)\n\t\tioctl 5 : Set loopback mode\n", cmd);
			return -EINVAL;
	}
	return 0;
}


struct file_operations spi_dect_fops = {
	.owner = THIS_MODULE,
	.open = spi_dect_open,
	.release = spi_dect_release, 
	.read = spi_dect_read,
	.write = spi_dect_write,
	.ioctl = spi_dect_ioctl,
};



