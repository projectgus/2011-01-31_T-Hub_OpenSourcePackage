/*
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: lb2tablet_buttons_driver.c
 *  Creation date: 06/08/2007
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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	     /* error codes */
#include <linux/types.h>	     /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	     /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <asm/system.h>		
#include <asm/uaccess.h>	     /* copy_*_user */
#include <asm/signal.h>

#include <asm/bitops.h>
#include <asm/atomic.h>

#ifndef CONFIG_ARCH_MX3
#error CONFIG_ARCH_MX3 not defined !!!
#else
#include <asm/arch/mx31.h> 
#include <asm/arch/gpio.h> 
#include <asm/arch/irqs.h> 
#include <asm/arch/board-mx31ads.h>
#include <asm/arch/mx31_pins.h>
#include "../../../arch/arm/mach-mx3/iomux.h"
#endif

#define USE_DEBUG

#ifdef USE_DEBUG
//select debug modes
#define USE_WARNING
#define USE_TRACE
#define USE_ASSERT

#endif //USE_DEBUG

#define DRIVER_VERSION (0x00000111UL)


MODULE_DESCRIPTION("Lb2 Tablet Buttons Driver");
/* MODULE_LICENSE("Sagem"); */



#ifdef USE_TRACE
static unsigned long debug_mode; 
#ifndef USE_WARNING
#define USE_WARNING
#endif
#	define BT_TRACE(msg,args...)			\
	if(debug_mode&0x01UL) {					\
		printk("BT: " msg "\n", ## args);	\
	}
#else
#	define BT_TRACE(msg,args...)
#endif

#ifdef USE_WARNING
static unsigned long debug_mode;
#ifndef USE_ASSERT
#define USE_ASSERT
#endif
#	define BT_WARNING(msg, args...)				\
	if(debug_mode&0x02UL) {							\
		printk("BT_WARNING: " msg "\n",## args);	\
	}
#else
#	define BT_WARNING(msg, args...)
#endif


#ifdef USE_ASSERT
#	define BT_ASSERT(condition)													\
	if(!(condition)) {																\
		printk("BT_ASSERTION_FAILURE: File=" __FILE__ ", Line=%d\n",__LINE__);	\
		while(1);																	\
	}
#else
#	define BT_ASSERT(condition)
#endif


#ifdef USE_DEBUG
static unsigned long debug_mode=0x3UL;
#else
static unsigned long debug_mode=0x0UL;
#endif
module_param(debug_mode,ulong,S_IRUGO);

#include "lb2tablet_buttons_driver.h"

struct buttons_dev {

    struct cdev cdev;	  /* Char device structure		*/
    wait_queue_head_t	poll_wait;
    iomux_pin_name_t button_sup_pin;
    iomux_pin_name_t button_mid_pin;
    iomux_pin_name_t factory_reset_pin;
    spinlock_t lock;
    unsigned long button;
    bool button_pressed;
};


int buttons_major = BUTTONS_MAJOR ;
int buttons_minor = 0 ;

struct buttons_dev *buttons_devices;


int buttons_ioctl(struct inode *inode, struct file *filp,
		  unsigned int cmd, unsigned long arg)
{
    
    int retval = 0, err =0 ;
    struct buttons_dev *dev =(struct buttons_dev *)  filp->private_data;

    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != BUTTONS_IOC_MAGIC) return -ENOTTY;
    //	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
	err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
	err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;
    
    switch(cmd) {
	
    case BUTTONS_IOCTL_GET_BUTTON_PRESSED:

        put_user(dev->button,(unsigned long *)arg);
	retval=1;
	break;
		
    default:
	BT_WARNING("Invalid ioctl command");
	return -ENOIOCTLCMD;
    }
    return retval;
}

irqreturn_t button_sup_interrupt(int irq,void *dev_id, struct pt_regs *regs)
{
    struct buttons_dev *dev = (struct buttons_dev*) dev_id;

    disable_irq_nosync(irq);

    mdelay(500);

    enable_irq(irq);

    spin_lock_irq(&dev->lock);
    dev->button = BUT_SUP_VALUE;
    dev->button_pressed=true;

    wake_up_interruptible(&dev->poll_wait);
/*     BT_TRACE("S"); */
    spin_unlock_irq(&(dev->lock));

    return IRQ_HANDLED;
}


irqreturn_t button_mid_interrupt(int irq,void *dev_id, struct pt_regs *regs)
{
    struct buttons_dev *dev = (struct buttons_dev *)dev_id;

    disable_irq_nosync(irq);

    mdelay(500);

    enable_irq(irq);

    spin_lock_irq(&dev->lock);
    dev->button= BUT_MID_VALUE ;
    dev->button_pressed=true;


    wake_up_interruptible(&dev->poll_wait);
/*     BT_TRACE("M"); */
    spin_unlock_irq(&dev->lock);

    return IRQ_HANDLED;
}

unsigned int buttons_poll(struct file *filp, struct poll_table_struct *poll_data)
{

    unsigned int		mask = 0;
    struct buttons_dev *dev = filp->private_data ;
/*      BT_TRACE("poll_wait_start"); */
    

    /* adding a wait queue to the poll table*/
    poll_wait(filp, &dev->poll_wait, poll_data);

    spin_lock(&dev->lock);
    if (dev->button_pressed) {
	mask |= (POLLIN | POLLRDNORM);
	dev->button_pressed = false;
    /*       mask |= (POLLHUP);    */
    }
    spin_unlock(&dev->lock);

/*     BT_TRACE("poll_wait_end"); */
    return mask;
}


int buttons_open(struct inode *inode, struct file *filp)
{
    int result;

    struct buttons_dev *dev ; /* device information */

    dev = container_of(inode->i_cdev, struct buttons_dev, cdev);
    filp->private_data = dev; /* for other methods */

    gpio_config(GPIO_TO_PORT(IOMUX_TO_GPIO(buttons_devices->button_sup_pin )),
		GPIO_TO_INDEX(IOMUX_TO_GPIO(buttons_devices->button_sup_pin )),
		false,
		GPIO_INT_FALL_EDGE);
    
    if( gpio_request_irq(GPIO_TO_PORT(IOMUX_TO_GPIO(buttons_devices->button_sup_pin )),
			 GPIO_TO_INDEX(IOMUX_TO_GPIO(buttons_devices->button_sup_pin )),
			 GPIO_HIGH_PRIO,
			 button_sup_interrupt,
			 0,
/* 			 SA_INTERRUPT, */
			 "gpio sup pin fall edge",
			 dev)!=0 )
	{
	    result=-ENODEV;
	    BT_WARNING("Can't get PIN MID IRQ line");
	    
	}


    gpio_config(GPIO_TO_PORT(IOMUX_TO_GPIO(buttons_devices->button_mid_pin )),
		GPIO_TO_INDEX(IOMUX_TO_GPIO(buttons_devices->button_mid_pin )),
		false,
		GPIO_INT_FALL_EDGE);
    
    if( gpio_request_irq(GPIO_TO_PORT(IOMUX_TO_GPIO(buttons_devices->button_mid_pin )),
			 GPIO_TO_INDEX(IOMUX_TO_GPIO(buttons_devices->button_mid_pin )),
			 GPIO_HIGH_PRIO,
			 button_mid_interrupt,
			 0,
/* 			 SA_INTERRUPT, */
			 "gpio mid pin fall edge",
			 dev)!=0 )
	{
	    result=-ENODEV;
	    BT_WARNING("Can't get PIN MID IRQ line");
	}


    return 0;
}


int buttons_release(struct inode *inode, struct file *filp)
{
    struct buttons_dev *dev = filp->private_data;

    free_irq(IOMUX_TO_IRQ(buttons_devices->button_sup_pin),dev);
    free_irq(IOMUX_TO_IRQ(buttons_devices->button_mid_pin),dev);

    return 0;
}

struct file_operations buttons_fops = {
    .owner =    THIS_MODULE,
    .ioctl =    buttons_ioctl,
    .open =     buttons_open,
    .release =  buttons_release,
    .poll =     buttons_poll,
};


static void buttons_cleanup_module(void)
{

    dev_t devno = MKDEV(buttons_major, buttons_minor);

    mxc_free_iomux(buttons_devices->button_sup_pin,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);
    mxc_free_iomux(buttons_devices->button_mid_pin,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);

/* 	Get rid of our char dev entries */
    if (buttons_devices) {
	cdev_del(&buttons_devices->cdev);
	kfree(buttons_devices);
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno,BUTTONS_NR_DEVS);
}


static int __init buttons_init_module(void)
{
    int result,err;
    dev_t dev = 0;

    if(buttons_major){
	dev = MKDEV(buttons_major,buttons_minor);
	result = register_chrdev_region(dev, BUTTONS_NR_DEVS , "lb2tablet_buttons");
    }else{
	result = alloc_chrdev_region(&dev, buttons_minor ,BUTTONS_NR_DEVS ,"lb2tablet_buttons");
	BT_WARNING("BUTTONS_MAJOR number must be assigned in a static way DYNAMIC ALLOCATION DONE");
    }
    if (result<0){
	BT_WARNING("can't get major number %i \n",buttons_major);
	return result;
    }

    buttons_devices = kmalloc(BUTTONS_NR_DEVS * sizeof(struct buttons_dev), GFP_KERNEL );
    if(!buttons_devices){
	result = -ENOMEM;
	goto fail;  /* Make this more graceful */
    }

    memset(buttons_devices, 0 , BUTTONS_NR_DEVS *  sizeof(struct buttons_dev));

    spin_lock_init(&buttons_devices->lock);
    init_waitqueue_head(&buttons_devices->poll_wait);

    buttons_devices->button_sup_pin = IOMUX_BUTTON_SUP_NAME;
    if(!(mxc_request_iomux(buttons_devices->button_sup_pin,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO )==0)){
	BT_WARNING("  REQUEST_IOMUX FOR BUTTON SUP NOT AVAILABLE");
	result=-ENODEV;
	goto fail;  /* Make this more graceful */
    }

    buttons_devices->button_mid_pin = IOMUX_BUTTON_MID_NAME;
    if(!(mxc_request_iomux(buttons_devices->button_mid_pin,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO )==0)){
	BT_WARNING("  REQUEST_IOMUX FOR BUTTON MID NOT AVAILABLE");
	result=-ENODEV;
	goto fail;  /* Make this more graceful */
    }

    /* Device registration */
    cdev_init(&buttons_devices->cdev, &buttons_fops );
    buttons_devices->cdev.owner = THIS_MODULE;
    buttons_devices->cdev.ops = &buttons_fops;
    err = cdev_add(&buttons_devices->cdev, dev, 1);
    if (err)
	BT_WARNING("Error %d adding cs4341a", err);
		
    return 0; /* succeed*/

  fail:
	
    buttons_cleanup_module();
    return result;
}

module_init(buttons_init_module);
module_exit(buttons_cleanup_module);


