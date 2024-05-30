/*
 *  DECT USB driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dectusb_command.h
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

#ifndef DECT_USB_COMMAND
#define DECT_USB_COMMAND

#include "dectusb_commun.h" 

#define DECTUSB_COMMAND_NODE_NAME	"dectusb_cmd"

static struct usb_driver driver;
static int driver_state;

static int verbose_level = 0;
module_param(verbose_level, int, 0);
MODULE_PARM_DESC(verbose_level, "Verbose level");

#define DRIVER_VERSION		"v2.0.2"
#define DRIVER_AUTHOR 		"Farid Hammane <fhammane@gmail.com>"
#define DRIVER_DESC 		"DONGLE DECT USB. COMMAND INTERFACE"
#define DRIVER_LICENSE 		"GPL"
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#define DECT_USB_MINOR_BASE	176
#define DECT_CONTROL_NAME	"[DECTUSB_CONTROL]"

#define INFO(fmt, args...)		if(verbose_level >= 2) printk(DECT_CONTROL_NAME " INFO  %d %s()  "fmt"\n", __LINE__, __func__, ## args);
#define WARNING(fmt, args...)		printk(DECT_CONTROL_NAME " WARNING  %d %s()  "fmt"\n", __LINE__, __func__, ## args);
#define ERROR(fmt, args...)		printk(DECT_CONTROL_NAME " ERROR  %d %s()  "fmt"\n", __LINE__, __func__, ## args);
#define TRACES_DB_BUFF(bask, level, msg, buff, size)    \
	if(bask >= level) {\
		int i; \
		printk(DECT_CONTROL_NAME" %s() %d %s : ", __func__, __LINE__, msg); \
		for(i=0; i<size; i++) \
		printk("%x ", (unsigned char)buff[i]);\
		printk("\n");\
	}
#define TRACE_DB_BUFF(level, m, b, s)           TRACES_DB_BUFF(verbose_level, level, m, b, s)

#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_AUDIO)
#define DO_ULOG
#include <ulog/ulog.h>
#else
#define ulog(x,y)
#endif

#if defined (DO_ULOG) && defined(CONFIG_ULOG_DECTUSB_CONTROL_CTX)
#warning "Compiling ulog traces for function context"
#define log_ctx(x) \
	if(in_irq()){\
		ulog(ULOG_DECTUSB_CONTROL_HARDIRQ_CTX,(__LINE__<<16 | x));\
	}else if(in_softirq()){\
		ulog(ULOG_DECTUSB_CONTROL_SOFTIRQ_CTX,__LINE__<<16 | x);\
	}else{\
		ulog(ULOG_DECTUSB_CONTROL_NOT_ATOMIC,__LINE__<<16 | x);\
	}

#define log_ctx_done(x) \
	if(in_irq()){\
		ulog(ULOG_DECTUSB_CONTROL_HARDIRQ_CTX_DONE,(__LINE__<<16 | x));\
	}else if(in_softirq()){\
		ulog(ULOG_DECTUSB_CONTROL_SOFTIRQ_CTX_DONE,__LINE__<<16 | x);\
	}else{\
		ulog(ULOG_DECTUSB_CONTROL_NOT_ATOMIC_DONE,__LINE__<<16 | x);\
	}
#else
#define log_ctx(x)
#define log_ctx_done(x)
#endif

enum {
	LOG_CTX_OPEN = 0x50,
	LOG_CTX_RELEASE,
} ENUMS_LOG_CTX;

#if defined(CONFIG_ULOG_DECTUSB_CONTROL_PLINE)
#define pline(fmt, args...)	printk(DECT_CONTROL_NAME " %s() %d : "fmt"\n", __func__, __LINE__, ## args);
#warning "Compiling printk traces for spin/mutex locks"
#else
#define pline(fmt, args...)
#endif

static spinlock_t sl_hard;
#if defined (DO_ULOG) && defined(CONFIG_ULOG_DECTUSB_CONTROL_SPINLOCK)
#warning "Compiling ulog traces for spin locks"
#define ulog_spin(x,y)	ulog(x,y)
#else
#define ulog_spin(x,y)
#endif

#define lock_irq()										\
		pline("spin lock : locking");							\
		ulog_spin(ULOG_DECTUSB_CONTROL_SPINLOCK_LOCK,__LINE__<<16);			\
		spin_lock_irq(&sl_hard); 							

#define unlock_irq()										\
		pline("spin lock : freeing");							\
		ulog_spin(ULOG_DECTUSB_CONTROL_SPINLOCK_DONE,__LINE__<<16);			\
		spin_unlock_irq(&sl_hard);

struct mutex            	ducm;
#define dect_mutex_init()	mutex_init(&ducm);
#if defined (DO_ULOG) && defined(CONFIG_ULOG_DECTUSB_CONTROL_MUTEX)
#warning "Compiling ulog traces for spin mutexs"
#define ulog_mutex(x,y) ulog(x,y)
#else
#define ulog_mutex(x,y)
#endif

#define lock_dect()									\
	if(mutex_is_locked(&ducm)){							\
		pline("mutex : wait for locking");					\
		ulog_mutex(ULOG_DECTUSB_CONTROL_MUTEX_WAIT, __LINE__<<16);		\
		mutex_lock(&ducm); 							\
		pline("mutex : locked");						\
		ulog_mutex(ULOG_DECTUSB_CONTROL_MUTEX_WAIT_DONE, __LINE__<<16);		\
	}										\
	else										\
	{										\
		pline("mutex : locking");						\
		ulog_mutex(ULOG_DECTUSB_CONTROL_MUTEX_LOCK, __LINE__<<16);		\
		mutex_lock(&ducm); 							\
		pline("mutex : locked");						\
		ulog_mutex(ULOG_DECTUSB_CONTROL_MUTEX_LOCK_DONE, __LINE__<<16);		\
	}
		
#define unlock_dect()									\
	if(mutex_is_locked(&ducm)){							\
		pline("mutex : freeing");						\
		ulog_mutex(ULOG_DECTUSB_CONTROL_MUTEX_FREE, __LINE__<<16);		\
		mutex_unlock(&ducm);							\
	}										\
	else										\
	{										\
		pline("mutex : Was not locked locking");				\
		ulog_mutex(ULOG_DECTUSB_CONTROL_MUTEX_WAS_NOT_LOCKED,__LINE__<<16);	\
		ERROR("mutex : was not locked");					\
		mutex_unlock(&ducm);							\
	}
/*
enum {
	IOCTL_SET_DEBUG_LEVEL,
	IOCTL_DECT_AUDIO_OPEN,
	IOCTL_DECT_AUDIO_CLOSE,
	IOCTL_DECT_AUDIO_RECORD_REGISTER_CALLBACK,
	IOCTL_DECT_AUDIO_PLAY_REGISTER_CALLBACK,
	IOCTL_DECT_AUDIO_RECORD_START,
	IOCTL_DECT_AUDIO_PLAY_START,
	IOCTL_DECT_AUDIO_DEBUG_LEVEL,
	IOCTL_SET_READ_TIMEOUT = 10,
	IOCTL_SET_WRITE_TIMEOUT,
	IOCTL_RESET_USB_DEVICE,
	IOCTL_POWER_ON_USB_DEVICE,
	IOCTL_POWER_OFF_USB_DEVICE,
} ENUMS_IOCTL;
*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
static void dectusb_interrupt_in_callback(struct urb *urb, struct pt_regs *regs);
#else
static void dectusb_interrupt_in_callback(struct urb *urb);
#endif

void print_endpoint(char * whoami, struct usb_endpoint_descriptor *pe) {

	if (!pe) {
		printk("%s: %s : Erreur pe == NULL\n", __func__, whoami);
	}


	printk("%s: ___________________\n", __func__);

	printk("\tbLength =\t\t%d\n", pe->bLength);
	printk("\tbDescriptorType =\t0x%x\n", pe->bDescriptorType);
	printk("\tbEndpointAddress =\t0x%x\n", pe->bEndpointAddress);
	printk("\tbmAttributes =\t\t0x%x\n", pe->bmAttributes);
	printk("\twMaxPacketSize =\t%d\n", pe->wMaxPacketSize);
	printk("\tbInterval =\t\t%d\n", pe->bInterval);
	printk("\tbRefresh =\t\t%d\n", pe->bRefresh);
	printk("\tbSynchAddress =\t\t0x%x\n", pe->bSynchAddress);

	printk("end %s ________________\n\n", __func__);
	return;

}

void print_usb_interface(char *whoami, struct usb_interface *pu) {

	if (!pu) {
		printk("%s: %s : Erreur pu == NULL\n", __func__, whoami);
	}

	printk("%s: ___________________\n", __func__);
	printk("\tminor =\t\t%d\n", pu->minor);
	printk("\tdev :\n");
	printk("\t\tbus_id=\t\t%s\n", pu->dev.bus_id);

	printk("end %s ________________\n\n", __func__);

	return;

}

void print_usb_device_id(char *whoami, struct usb_device_id *pi) {

	if (!pi) {
		printk("%s: %s : Erreur pi == NULL\n", __func__, whoami);
	}

	printk("%s: ___________________\n", __func__);
	printk("\tidVendor =\t\t0x%x\n", pi->idVendor);
	printk("\tidProduct =\t\t0x%x\n", pi->idProduct);


	printk("\tbDeviceClass =\t\t0x%x\n", pi->bDeviceClass);
	printk("\tbDeviceSubClass =\t0x%x\n", pi->bDeviceSubClass);
	printk("\tbDeviceProtocol =\t0x%x\n", pi->bDeviceProtocol);

	printk("\tbInterfaceClass =\t0x%x\n", pi->bInterfaceClass);
	printk("\tbInterfaceSubClass =\t0x%x\n", pi->bInterfaceSubClass);
	printk("\tbInterfaceProtocol =\t0x%x\n", pi->bInterfaceProtocol);

	printk("end %s ________________\n\n", __func__);

	return;

}

#endif
