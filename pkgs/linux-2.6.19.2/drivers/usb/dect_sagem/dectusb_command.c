/*
 *  DECT USB driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dectusb_command.c
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

#include <sound/dect_command_ioctls.h>
#include "dectusb_command.h"
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <asm/arch/board-mx31homescreen.h>
#include <asm/arch/gpio.h>
/* #include <arch/arm/mach-mx3/iomux.h> */

/*********************************************************************************/
/*				DATAS		 				 */
/*********************************************************************************/


#define SIZE_BUFFER_DATA		2048
struct st_data{
	int				current_size;
	char				buffer[SIZE_BUFFER_DATA];
	spinlock_t			sl;

};
static struct st_data		data;
static struct st_data		*Gdata = &data;


#define check_data_ctx() /* if(!mutex_is_locked(&ducm)) printk("%s() %d : mutex is not locked\n", __func__, __LINE__);\
			    if(!spin_is_locked(&sl_hard)) printk("%s() %d : spin lock is not locked\n", __func__, __LINE__);*/

/* Timeout for resetting the Device */
#define DECT_USB_RESET_WAIT	(1*HZ)

static int data_present(void){
	check_data_ctx();
	return Gdata->current_size;
}



static void data_free(void){
	check_data_ctx();
	Gdata->current_size = 0;
	memset(Gdata->buffer, 0x0, SIZE_BUFFER_DATA);
	return;
}
	


static int data_keep_buffer(unsigned char *buff, int size){
	int size_left = 0;
	int keep_size = 0;
	int ret = -EINVAL;
	
	check_data_ctx();
	
	assert(Gdata->current_size >= 0 && Gdata->current_size <= SIZE_BUFFER_DATA);
	assert(buff);
	assert(size >= 0 && size <= SIZE_BUFFER_DATA);

	if(!size || size == 1){
		WARNING("Will not keep %d bytes. Not enough data", size);
		ret = 0;
		goto fail;
	}

	size--; /* the first byte of buff is its length, it will not be kept */
	if(size != buff[0]){
		ERROR("buff[0] (%d) and size (%d) are different, keep nothing", buff[0], size);
		goto fail;
	}
	buff++; /* the first byte of buff is its length, it will not be kept */
	
	size_left = SIZE_BUFFER_DATA - Gdata->current_size;
	if(!size_left){
		WARNING("not enough space to keep data : %d bytes lost", size);
		ret = -ENOBUFS;
		goto fail;
	}

	keep_size = min(size_left, size);
	if(keep_size < size){
		WARNING("not enough space to keep all data : loosing %d bytes", size - keep_size);
	}

	if((keep_size + Gdata->current_size) > SIZE_BUFFER_DATA){
		ERROR("(current_size + keep_size) IS GREATER THAN SIZE_BUFFER_DATA");
		goto fail;
	}

	memcpy(Gdata->buffer + Gdata->current_size, buff, keep_size);
	Gdata->current_size += keep_size;
	INFO("New current size : (%d)", Gdata->current_size);
	return Gdata->current_size;
fail:
	return ret;
}

static int data_get_data(int size_to_get, unsigned char *dest_buff){
	int can_get_data = 0;
	int i;
	unsigned char *i_buff = NULL;
	unsigned char *i_newbuff = NULL;
	check_data_ctx();

	if(!dest_buff)
		return -EFAULT;

	if(!size_to_get)
		return 0;

	can_get_data = min(size_to_get, Gdata->current_size);
	assert(can_get_data >= 0);

	memcpy(dest_buff, Gdata->buffer, can_get_data);
	/* TODO 
	 * Replace by a circular buffer 
	 */
	i = can_get_data;
	i_buff = Gdata->buffer + can_get_data;
	i_newbuff = Gdata->buffer;
	while(i){
		*i_newbuff = *i_buff;
		i_buff++;
		i_newbuff++;
		i--;
	}

	Gdata->current_size -= can_get_data;
	INFO("Got (%d) bytes", can_get_data);
	return can_get_data;
}


/*********************************************************************************/
/*					DECT CONTEXT 				 */
/*********************************************************************************/


struct st_dectusb {
	struct usb_device       	*usbdev;
	
	struct usb_endpoint_descriptor 	*in_endpoint;
	unsigned char          		*in_buffer;
	size_t				in_size;
	struct urb*             	in_interrupt_urb;
	wait_queue_head_t		in_wait;
	int				in_status_completed;

	struct usb_endpoint_descriptor 	*out_endpoint;
	unsigned char                   *out_buffer;
	size_t                          out_size;
	struct urb*             	out_interrupt_urb;



	int				state;
	int				poll_err;
};

static int set_struct(struct usb_interface *intf, struct st_dectusb **add_dectusb);

struct dectusb_ctx {
	struct st_dectusb *ctx;
};
static struct dectusb_ctx Dectusb = {
	.ctx = NULL,
};
#define bad_ctx(dect)	(!Dectusb.ctx || dect != Dectusb.ctx)
#define warning_bad_ctx(dect) WARNING("Bad context : Dectusb.ctx (%p) dectusb (%p)", Dectusb.ctx, dect)

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))

/*********************************************************************************/
/*				Reset		 				 */
/*********************************************************************************/

struct workqueue_struct 	*duc_queue = NULL;
struct work_struct      	duc_work;
static int usb_device_need_unlock = 0;

/*! \Brief Reset the device.
 *
 * Reset the device if a EPROTO error occurs
 *
 * I dont no yet if reset is supported while DECT audio is steaming. 
 * Using usb_reset_composite_device() will cause kernel calling
 * disconnect() without calling release(). There's may be a bug here.
 *
 */
static void duc_handle_reset(void *data){
	struct st_dectusb *dectusb = (struct st_dectusb *)data;
	struct usb_device *usbdev = NULL;
	int ret;
	INFO("Reset dect usb device");
	if(bad_ctx(dectusb)){
		warning_bad_ctx(dectusb);
		goto end;
	}
	usbdev = dectusb->usbdev;
	if(!usbdev){
		WARNING("usbdev is NULL");
		goto end;
	}
	/* :TODO:20.10.2009 18:16:20:: need locking ? */
	ret = usb_reset_composite_device(usbdev, NULL);
	if(ret && ret != -ENODEV)
		ERROR("usb reset composite device : error (%d)", ret);

end:
	INFO("Reset done");
	return;
}

static void prereset(struct usb_interface *intf)
{
	struct usb_device *usbdev = NULL;
	INFO("Enter");
	usbdev = interface_to_usbdev(intf);
	if(!usbdev){
		ERROR("usbdev is null");
		return;
	}

	usb_device_need_unlock = usb_lock_device_for_reset(usbdev, NULL);
	if(usb_device_need_unlock < 0)
		ERROR("Could not lock device for reset : error %d", usb_device_need_unlock);

	INFO("Done");
	return;
}


static void postreset(struct usb_interface *intf){
	struct usb_device *usbdev = NULL;
	INFO("Enter");
	usbdev = interface_to_usbdev(intf);
	if(!usbdev){
		ERROR("usbdev is null");
		return;
	}
	if(usb_device_need_unlock == 1)
		usb_unlock_device(usbdev);

	INFO("Done");
	return;
}

#endif
/*********************************************************************************/
/*				READ / WRITE 	 				 */
/*********************************************************************************/


/*! \brief Called when the user space exec a read system call.
 * @param *file
 * @param *bdest : The destination buffer.
 * @param nbR : Number of bytes to read.
 * @param *pos
 * @return If successful, read returns the number of bytes readed. Otherwise a negative value is returned.
 */
static ssize_t read(struct file *file, char *bdest, size_t nbR, loff_t *pos) {
	struct st_dectusb *dectusb = NULL;
	int ret = 0, bytes_to_read = 0;
	char tmp_buff[SIZE_BUFFER_DATA];

	INFO("Enter");

	if(nbR < 0 || !bdest)
		return -EINVAL;

	if(!nbR)
		return 0;

	memset(tmp_buff, 0x0, SIZE_BUFFER_DATA);
	lock_dect();
	dectusb = (struct st_dectusb *)file->private_data;
	if(bad_ctx(dectusb)){
		warning_bad_ctx(dectusb);
		ret = -ENXIO;
		goto end;
	}

	if(dectusb->state != DECT_OPEN){
		ret = -EPERM;
		goto end;
	}

	if(dectusb->in_status_completed){
		ret = dectusb->in_status_completed;
		dectusb->in_status_completed = 0;
		goto end;
	}

	if(dectusb->poll_err){
		ret = dectusb->poll_err;
		dectusb->poll_err = 0;
		goto end;
	}

	lock_irq();
	if(!data_present()){
		unlock_irq();
		ret = 0;
		goto end;
	}

	/* TODO
	 * Impl. a circular buffer
	 */
	bytes_to_read = data_get_data((int)nbR, tmp_buff);
	if(bytes_to_read <= 0){
		ret = bytes_to_read;
		unlock_irq();
		goto end;
	}
	unlock_irq();

	TRACE_DB_BUFF(1, "Dongle --> Tablet", tmp_buff, bytes_to_read);

	ret = copy_to_user(bdest, tmp_buff, bytes_to_read);
	if(ret){
		ERROR("copy_to_user : error %d", ret);
		ret = -EFAULT;
		goto end;
	}

	ret = bytes_to_read;
end:
	unlock_dect();
	INFO("Exit. Returning (%d)", ret);
	return ret;
}


/*! \brief Called when the user space exec a write system call. 
 * @param *file 
 * @param *buf_src : The source buffer.
 * @param nb_to_write : Number of bytes to write.
 * @param *pos_de_la_tete
 * @return If successful, write returns the number of bytes written. Otherwise a negative value is returned.
 */
static ssize_t write(struct file *file, const char *buf_src, size_t nb_to_write, loff_t *pos_de_la_tete) {
	struct st_dectusb *dectusb = NULL;
	size_t writesize = 0;
	int bytes_written = 0;
	int ret = -EINVAL;
	char *ptr;

	INFO("Enter");

	if(nb_to_write < 0)
		return -EINVAL;

	if(nb_to_write == 0)
		return 0;

	lock_dect();
	dectusb = (struct st_dectusb *)file->private_data;
	if(bad_ctx(dectusb)){
		warning_bad_ctx(dectusb);
		ret = -EINVAL;
		goto end;
	}

	if(dectusb->state != DECT_OPEN){
		ret = -EIO;
		goto end;
	}

	writesize = min(nb_to_write, dectusb->out_size);
	assert(writesize > 0);
	memset(dectusb->out_buffer, 0x0, writesize);

	ret = copy_from_user(dectusb->out_buffer+1, buf_src, writesize);
	if(ret){
		ERROR("copy_from_user");
		goto end;
	}

	dectusb->out_buffer[0] = 0x3f+(writesize);
	ptr = dectusb->out_buffer+1;
	TRACE_DB_BUFF(1, "Tablet --> Dongle", ptr, writesize);

	ret = usb_interrupt_msg(dectusb->usbdev,
			usb_sndintpipe(dectusb->usbdev, dectusb->out_endpoint->bEndpointAddress),
			dectusb->out_buffer,
			writesize+1, /* +1 for the report ID */
			&bytes_written,
			2000);

	if(ret) {
		ERROR("Cannot send usb message : error (%d)", ret);
		goto end;
	}

end:
	INFO("Exit, error (%d) wrote (%d) bytes, (%d) expected because of the report id, (%d) asked", ret, bytes_written, writesize+1, nb_to_write);
	unlock_dect();
	return ret?ret:--bytes_written; /* Report id has been add in kernel space. */
}


/*********************************************************************************/
/*				IOCTL	 	 				 */
/*********************************************************************************/


/*! \brief Called when the user space exec a ioctl system call.
 * 
 * @param *inode 
 * @param *file
 * @param cmd
 * @param arg
 * @return 
 */
static int ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg){
	struct st_dectusb *dectusb = NULL;
	long timeout;
	INFO("Enter");
	lock_dect();

	switch(cmd){

		case IOCTL_SET_DEBUG_LEVEL:
			verbose_level = (int)arg;
			INFO("Debug level is set to %d", verbose_level);
			break;

		case IOCTL_RESET_USB_DEVICE:
/*INFO("Will reset usb device");

			dectusb = (struct st_dectusb *)file->private_data;
			if(bad_ctx(dectusb)){
				warning_bad_ctx(dectusb);
				unlock_dect();
				return -EFAULT;
			}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
			if(!queue_work(duc_queue, &duc_work)){
				WARNING("Cannot add reset work. reset in progress ?");
			}
#endif

			file->private_data = NULL;

*/
			timeout	= DECT_USB_RESET_WAIT;
			/* power off device */
			mxc_set_gpio_dataout(HOMESCREEN_GPIO_DECT_USB_RESET,0);
			/* Add Timeout */
			set_current_state(TASK_INTERRUPTIBLE);
			WARNING ("_____________Device is powered OFF : schedule timeout");
			schedule_timeout (timeout);
			WARNING ("_____________Power OFF : time is out");
			/* power on Device */
			WARNING ("_____________powering ON device");
			mxc_set_gpio_dataout(HOMESCREEN_GPIO_DECT_USB_RESET,1);
			break;

		case IOCTL_POWER_ON_USB_DEVICE:
			mxc_set_gpio_dataout(HOMESCREEN_GPIO_DECT_USB_RESET,1);
			break;

		case IOCTL_POWER_OFF_USB_DEVICE:
			mxc_set_gpio_dataout(HOMESCREEN_GPIO_DECT_USB_RESET,0);
			break;

		default:
			ERROR("Command unknown");
			unlock_dect();
			return -EINVAL;

	}

	INFO("Done");
	unlock_dect();
	return 0;
}

/*********************************************************************************/
/*				INTERRUPT IN	 				 */
/*********************************************************************************/
#define debug_int_msgs	/*INFO("urb : transfer_buffer_length = %d, actual_length  = %d, in_buffer[0] = %d", \
			  urb->transfer_buffer_length, urb->actual_length, dectusb->in_buffer[0]);\
			  TRACE_DB_BUFF(1, "Data : Dongle --> Tablet", dectusb->in_buffer, urb->actual_length);*/


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
static void dectusb_interrupt_in_callback(struct urb *urb, struct pt_regs *regs)
#else
static void dectusb_interrupt_in_callback(struct urb *urb)
#endif
{
	struct st_dectusb *dectusb = NULL;
	int ret;
	dectusb = (struct st_dectusb *)urb->context;
	if(bad_ctx(dectusb)){
		warning_bad_ctx(dectusb);
		goto wakeup;
	}

	dectusb->in_status_completed = urb->status;
	switch(urb->status){
		case 0:
			break;
		case -EPROTO:
			ERROR("Proto error.");
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
			if(!queue_work(duc_queue, &duc_work)){
				WARNING("Cannot add reset work. reset in progress ?");
			}
#endif
			goto wakeup;
		default:
			WARNING("urb error (%d). Driver is not synchronized anymore", urb->status);
			goto wakeup;
	}

	if(dectusb->state != DECT_OPEN){
		TRACE_DB_BUFF(0, "Device is not open, ignoring data : ", dectusb->in_buffer, urb->actual_length);
		dectusb->in_status_completed = -EPERM;
	}else{
		debug_int_msgs;
		data_keep_buffer(dectusb->in_buffer, urb->actual_length);
	}

	memset(dectusb->in_buffer, 0x0, dectusb->in_size);
	usb_fill_int_urb(dectusb->in_interrupt_urb,
			dectusb->usbdev,
			usb_rcvintpipe(dectusb->usbdev, dectusb->in_endpoint->bEndpointAddress),
			dectusb->in_buffer,
			dectusb->in_size,
			dectusb_interrupt_in_callback,
			dectusb,
			dectusb->in_endpoint->bInterval);


	ret = usb_submit_urb(dectusb->in_interrupt_urb, GFP_ATOMIC);
	if (ret){
		/* :TODO:20.10.2009 11:11:29:: test if we can recover from error */
		ERROR("usb submit urb error (%d).", ret);
		if(ret != -EINPROGRESS)
			ERROR("Driver is not synchronized anymore");
		dectusb->in_status_completed = ret;
	}

wakeup:
	wake_up_interruptible(&dectusb->in_wait);
	return;
}


/*********************************************************************************/
/*					POLL	 				 */
/*********************************************************************************/


static unsigned int poll(struct file *file, poll_table *wait){
	unsigned int mask = 0;
	int d;
	struct st_dectusb *dectusb = NULL;

	INFO("Enter");
	lock_dect();

	dectusb = (struct st_dectusb *)file->private_data;
	if(bad_ctx(dectusb)){
		mask |= POLLERR;
		goto end;
	}

	if(!dectusb->usbdev){
		ERROR("USB disconnected");
		dectusb->poll_err = -ENODEV;
		mask |= POLLERR;
		goto end;
	}

	if(dectusb->state != DECT_OPEN){
		ERROR("dectusb_command is not OPEN");
		mask |= POLLERR;
		goto end;
	}

	poll_wait(file, &(dectusb->in_wait), wait);

	if(dectusb->in_status_completed == -EPROTO){
		mask |= POLLERR;
		goto end;
	}

	lock_irq();
	d = data_present();
	unlock_irq();
	if(d){
		INFO("Got data : %d bytes", d);
		mask |= POLLIN | POLLRDNORM;

	}else {
		INFO("No data available");
	}

end:
	INFO("Exit");
	unlock_dect();
	return mask;
}



/*********************************************************************************/
/*				TEST INTERFACE	 				 */
/*********************************************************************************/


/*! \brief Test if this interface is expected
 *
 * @param *intf : The struct usb_interface given to the probe function 
 * @return If this interface is expected, test_intf() returns 0 
 */
static inline int test_intf(struct usb_interface *intf) {
	struct usb_host_interface *litf = intf->cur_altsetting;
	struct usb_device *udev = interface_to_usbdev(intf);
	char string[DECT_INTERFACE_STRING_NAME_SIZE_MAX];
	int ret = -EINVAL;
	
	if (litf->desc.bNumEndpoints != 2)
		return -ENODEV;

	memset(string, 0x0, DECT_INTERFACE_STRING_NAME_SIZE_MAX);
	ret = usb_string(udev, litf->desc.iInterface, string, DECT_INTERFACE_STRING_NAME_SIZE_MAX);
	if(ret < 0){
		ERROR("Could not get string of usb interface (%d)", litf->desc.iInterface);
		return ret;
	}

	if(strlen(string) != strlen(DECT_INTERFACE_STRING_NAME_COMMAND) ||
			memcmp(string, DECT_INTERFACE_STRING_NAME_COMMAND, strlen(DECT_INTERFACE_STRING_NAME_COMMAND))) 
		return -ENODEV;

	return 0;
}


/*********************************************************************************/
/*				OPEN / RELEASE					 */
/*********************************************************************************/

/*! \brief Called when the user space exec a open system call
 * 
 * @param *inode
 * @param *file 
 * @return Returns 0 if successful.
 */
static ssize_t open(struct inode *inode, struct file *file) {
	struct st_dectusb *dectusb = NULL;
	int ret = -ENODEV;

	INFO("Enter");
	lock_dect();
	log_ctx(LOG_CTX_OPEN);

	dectusb = usb_get_intfdata(usb_find_interface(&driver, iminor(inode)));
	if(bad_ctx(dectusb))
		return -EFAULT;

	switch(dectusb->state){
		case DECT_OPEN:
			ERROR("dectusb already open");
			ret = -EBUSY;
			goto fail;
		case DECT_CLOSE:
			break;
		default:
			ERROR("dectusb : unknown state");
			ret = -EINVAL;
			goto fail;
	}
	
	lock_irq();
	dectusb->state = DECT_OPEN;
	driver_state = 1;

	dectusb->in_status_completed = 0;
	dectusb->poll_err = 0;
	data_free();

	file->private_data = dectusb;
	unlock_irq();

	log_ctx_done(LOG_CTX_OPEN);
	INFO("Exit");
	unlock_dect();
	return 0;
fail:
	file->private_data = NULL;
	log_ctx_done(LOG_CTX_OPE);
	WARNING("Fail with error : %d", ret);
	unlock_dect();
	return ret;
}

/*! \brief Called when the user space exec a close system call 
 * 
 * @param *inode 
 * @param *file 
 * @return Returns 0 if successful.
 */
static ssize_t release(struct inode *inode, struct file *file){
	struct st_dectusb *dectusb = NULL;
	INFO("Enter");
	lock_dect();
	log_ctx(LOG_CTX_RELEASE);

	dectusb = file->private_data;
	if(bad_ctx(dectusb)){
		warning_bad_ctx(dectusb);
		file->private_data = NULL;
		goto fail;
	}

	lock_irq();
	file->private_data = NULL;
	dectusb->in_status_completed = -ENODEV;
	dectusb->poll_err = 0;
	dectusb->state = DECT_CLOSE;
	driver_state = 0;
	data_free();
	log_ctx_done(LOG_CTX_RELEASE);
	unlock_irq();
fail:
	INFO("Exit");
	unlock_dect();
	return 0;
}


/*********************************************************************************/
/*				PROBE / DISCONNECT				 */
/*********************************************************************************/


static struct file_operations fops = {
	.owner		=	THIS_MODULE,
	.open		=	open,
	.release	=	release,
	.read		=	read,
	.write		=	write,
	.ioctl		=	ioctl,
	.poll		=	poll,
};

static struct usb_class_driver class = {
	.name		=       DECTUSB_COMMAND_NODE_NAME"%d",
	.fops		=       &fops,
	.minor_base	=   	DECT_USB_MINOR_BASE,
};   


void dectusb_free(struct st_dectusb *pdect){
	if(pdect){
		if(pdect->in_buffer)		kfree(pdect->in_buffer);
		if(pdect->in_interrupt_urb)	usb_free_urb(pdect->in_interrupt_urb);
		if(pdect->out_buffer)		kfree(pdect->out_buffer);
		if(pdect->out_interrupt_urb)	usb_free_urb(pdect->out_interrupt_urb);
		kfree(pdect);
	}
	return;
}

static int probe(struct usb_interface *intf, const struct usb_device_id *id){
	struct st_dectusb *dectusb = NULL;
	int ret = -ENODEV;
	
	INFO("probe DECT control interface\n");
	if(Dectusb.ctx){
		ERROR("Cannot handle more than 1 device");
		return -EBUSY;
	}

	if((ret = test_intf(intf)) || (ret = set_struct(intf, &dectusb)))
		goto fail1;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
	duc_queue = create_workqueue("reset_DECT");
	if(!duc_queue){
		ERROR("Cannot create reset work queue");
		ret = -ENODEV;
		goto fail1;
	}
	INIT_WORK(&duc_work, duc_handle_reset, (void *)dectusb);
#endif

	ret = usb_register_dev(intf, &class);
	if(ret){
		ERROR("usb_register_dev : error num %d", ret);
		goto fail2;
	}

	usb_fill_int_urb(dectusb->in_interrupt_urb,
			dectusb->usbdev,
			usb_rcvintpipe(dectusb->usbdev, dectusb->in_endpoint->bEndpointAddress),
			dectusb->in_buffer,
			dectusb->in_size,
			dectusb_interrupt_in_callback,
			dectusb,
			dectusb->in_endpoint->bInterval);

	ret = usb_submit_urb(dectusb->in_interrupt_urb, GFP_ATOMIC);
	if (ret){
		ERROR("Cannot submit urb (%d)", ret);
		goto fail3;
	}

	Dectusb.ctx = dectusb;
	usb_set_intfdata(intf, dectusb);
	INFO("DECT control interface probed\n");
	return 0;
fail3:
	lock_kernel();
	usb_deregister_dev(intf, &class);
	unlock_kernel();
fail2:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
	if(duc_queue){
		flush_workqueue(duc_queue);
		destroy_workqueue(duc_queue);
		duc_queue = NULL;
	}
#endif
fail1:
	dectusb_free(dectusb);
	WARNING("Fail : error (%d)", ret);
	return ret?ret:-ENODEV;
}

/*! \brief Called by the usb core subsystem when 
 * the device is disconnect.
 * 
 * @param *intf : The struct usb_interface
 */
static void disconnect(struct usb_interface *intf) {
	struct st_dectusb *dectusb = NULL; 
	INFO("Enter");
	lock_dect();
	lock_kernel();
	lock_irq();
	Dectusb.ctx = NULL;
	data_free();
	unlock_irq();
	dectusb = usb_get_intfdata(intf);
	usb_set_intfdata(intf, NULL);
	usb_deregister_dev(intf, &class);
	unlock_kernel();
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
	if(duc_queue){
		flush_workqueue(duc_queue);
		destroy_workqueue(duc_queue);
		duc_queue = NULL;
	}
#endif
	dectusb_free(dectusb);
	unlock_dect();
	printk("%s() : %d : Interface disconnected\n", __func__, __LINE__);
	return;
}

/********************************************************************************/
/*				/sys  info 				 	*/
/********************************************************************************/

static ssize_t read_verbose_level(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "verbose level (%d)\n", verbose_level);
	return size;
}


static ssize_t write_verbose_level(struct sys_device *dev, const char *buf, size_t size){
	int vl = simple_strtol(buf, NULL, 10);
	if(vl < 0)
		return -EINVAL;
	INFO("Setting verbose level to  (%d)", vl);
	verbose_level = vl;
	return size;
}

static ssize_t read_driver_info(struct sys_device *dev, char *buf){
	int size = 0;
	size += sprintf((buf + size), "dectusb control : version (%s)\n", DRIVER_VERSION);
	size += sprintf((buf + size), "driver state (%d)\n", driver_state);
	return size;
}


static SYSDEV_ATTR(driver_info, 0444, read_driver_info, NULL);
static SYSDEV_ATTR(verbose_level, 0644, read_verbose_level, write_verbose_level);


static struct sysdev_class sysclass = {
	set_kset_name("dectusb_control"),
};

static struct sys_device sysdevice = {
	.id = 0,
	.cls = &sysclass,
};

static void sysdev_exit(void){
	sysdev_remove_file(&sysdevice, &attr_driver_info);
	sysdev_remove_file(&sysdevice, &attr_verbose_level);
	sysdev_unregister(&sysdevice);
	sysdev_class_unregister(&sysclass);
}

static int sysdev_init(void){

	int err;
	err = sysdev_class_register(&sysclass);
	if(err){
		ERROR("Could not register class : error %d", err);
		return err;
	}

	err = sysdev_register(&sysdevice);
	if(err){
		ERROR("Could not register device : error %d", err);
		sysdev_exit();
		return err;
	}

	err = sysdev_create_file(&sysdevice, &attr_driver_info);
	if(err){
		ERROR("Could not create a sysdev file to export dectusb control informations : error %d", err);
		sysdev_exit();
		return err;
	}
	
	err = sysdev_create_file(&sysdevice, &attr_verbose_level);
	if(err){
		ERROR("Could not create a sysdev file to export dectusb control debug level : error %d", err);
		sysdev_exit();
		return err;
	}
	
	return 0;
}


/*********************************************************************************/
/*				__INIT / EXIT__ 				 */
/*********************************************************************************/

/* Tableau des gadgets qui fonctionnent avec ce pilote */
static struct usb_device_id dectusb_command_table [] = {
	{ USB_VENDOR_DEVICE(DONGLE_D45_USB_VENDOR_ID) },
	{ USB_INTERFACE_INFO(3, 0, 0) },
	{ USB_DEVICE_INFO(0, 0, 0) },
	{} /* Fin de la liste */
};
MODULE_DEVICE_TABLE(usb, dectusb_command_table);


static struct usb_driver driver = {
	.name		=	"dectusb_command",
	.id_table	=  	dectusb_command_table,
	.probe		=	probe,
	.disconnect 	=	disconnect,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
	.pre_reset	=	prereset,
	.post_reset	=	postreset,
#endif
};

static int __init dectusb_command_init(void){
	int err;
	extern void gpio_dect_usb_active(void);
	INFO("insmod of dectusb command");
	dect_mutex_init();
	spin_lock_init(&sl_hard);
	driver_state = 0;
	gpio_dect_usb_active();
	err = sysdev_init();
	if(err){
		ERROR("Could not export data using /sys. Not fatal");
	}
	return usb_register(&driver);
}

/** 
 * @return Returns 0 if successful. 
 */
static void __exit dectusb_command_exit (void) {
	INFO("rmmod of dectusb command");
	sysdev_exit();
	usb_deregister(&driver);
}

module_init(dectusb_command_init);
module_exit(dectusb_command_exit);
MODULE_LICENSE("GPL");



/*! \brief Allocate memory for struct st_dectusb and set it.
 *
 * @param **ptr : A pointer to struct dectusb_command_t
 * @return 0 if successful.
 */
static int set_struct(struct usb_interface *intf, struct st_dectusb **add_dectusb) {
	int ret = -ENODEV;
	struct usb_endpoint_descriptor *lendpoint = NULL;
	struct st_dectusb *dectusb = NULL;

	INFO("Driver version %s. (alt settings %d)", DRIVER_VERSION, intf->num_altsetting);

	*add_dectusb = kzalloc(sizeof(struct st_dectusb), GFP_KERNEL);
	dectusb = *add_dectusb;
	if(!dectusb){
		ERROR("kzalloc on dectusb");
		return -ENOMEM;
	}

	dectusb->usbdev = usb_get_dev(interface_to_usbdev(intf));
	if(!dectusb->usbdev){
		ERROR("usbdev is NULL");
		goto fail;
	}

	lendpoint = &intf->cur_altsetting->endpoint[ENDPOINT_IN_COMMAND].desc;
	if(!lendpoint){
		ERROR("endpoint %d NULL", ENDPOINT_IN_COMMAND);
		goto fail;
	}

	/* print_endpoint("func_probe", lendpoint); */
	if(verbose_level > 2)
		seek_endpoints_type(lendpoint);

	if(!usb_endpoint_is_int_in(lendpoint)){
		ERROR("lendpoint %d is not interrupt IN", ENDPOINT_IN_COMMAND);
		goto fail;
	}

	dectusb->in_interrupt_urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!dectusb->in_interrupt_urb){
		ERROR("Could not allocate mem for in_interrupt_urb");
		ret = -ENOMEM;
		goto fail;
	}

	dectusb->in_endpoint =		lendpoint;
	dectusb->in_size = 		le16_to_cpu(lendpoint->wMaxPacketSize);
	dectusb->in_buffer =		kmalloc(dectusb->in_size, GFP_KERNEL);
	if(!dectusb->in_buffer){
		ERROR("Could not allocate in_buffer");
		ret = -ENOMEM;
		goto fail;
	}

	lendpoint = &intf->cur_altsetting->endpoint[ENDPOINT_OUT_COMMAND].desc;
	if(!lendpoint){
		ERROR("Endpoint %d is NULL", ENDPOINT_OUT_COMMAND);
		goto fail;
	}

	/* print_endpoint("func_probe", lendpoint); */
	if(verbose_level > 2)
		seek_endpoints_type(lendpoint);


	if(!usb_endpoint_is_int_out(lendpoint)){
		ERROR("lendpoint %d not interrupt OUT", ENDPOINT_OUT_COMMAND);
		goto fail;
	}

	dectusb->out_endpoint = 	lendpoint;
	dectusb->out_size =		le16_to_cpu(lendpoint->wMaxPacketSize);
	dectusb->out_buffer =		kmalloc(dectusb->out_size, GFP_KERNEL);
	if(!dectusb->out_buffer){
		ERROR("Could not allocate memory for out_buffer");
		ret = -ENOMEM;
		goto fail;
	}

	/* Defined but not yet used */
	dectusb->out_interrupt_urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!dectusb->out_interrupt_urb){
		ERROR("Could not allocate out_interrupt_urb");
		ret = -ENOMEM;
		goto fail;
	}

	dectusb->state = DECT_CLOSE;
	driver_state = 0;

	init_waitqueue_head(&(dectusb->in_wait));

	return 0;

fail:
	ERROR("Fail");
	if(dectusb->in_interrupt_urb) 	usb_free_urb(dectusb->in_interrupt_urb);
	if(dectusb->in_buffer)		kfree(dectusb->in_buffer);
	if(dectusb->out_buffer)		kfree(dectusb->out_buffer);
	if(dectusb->out_interrupt_urb)	usb_free_urb(dectusb->out_interrupt_urb);
	kfree(dectusb);
	return ret;
}

