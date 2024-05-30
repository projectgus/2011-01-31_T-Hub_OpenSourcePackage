/*
 *  DECT USB driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: dectusb_commun.h
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

#ifndef DECT_USB_COMMUN
#define DECT_USB_COMMUN



#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <linux/version.h>

#define DECT_OPEN			25
#define DECT_CLOSE			26

#define DONGLE_D45_USB_VENDOR_ID	0x079b
#define DONGLE_D45_USB_DEVICE_ID	0x007d

#define ENDPOINT_IN_COMMAND    0
#define ENDPOINT_OUT_COMMAND   1

#define SIZE_PATH	64

#define SIZE_OF_DONGLE_DECT_MESSAGE	82

#define SIZE_OF_FIRST_MESSAGE_HID	64
#define SIZE_OF_FIRST_MESSAGE		63

#define SIZE_OF_SECOND_MESSAGE_HID	18
#define SIZE_OF_SECOND_MESSAGE		17



#define DECT_INTERFACE_STRING_NAME_SIZE_MAX		20
#define DECT_INTERFACE_STRING_NAME_COMMAND		"HID Control"
#define DECT_INTERFACE_STRING_NAME_AUDIO		"HID Audio"


#define LOCK_DECT(a)			
/*down(&a);*/
#define UNLOCK_DECT(a)			
/*up(&a);*/

#define DECT_BUG() do { \
	        printk("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __FUNCTION__); \
	        panic("BUG!"); \
		BUG(); \
} while (0)

#define DEBUG_WITH_PANIC 		BUG()

#define assert(a) \
	do{ \
		if(unlikely(!(a))) { \
			printk(KERN_ERR "ASSERT FAILED (%s) at: %s:%d:%s()\n", #a, __FILE__, __LINE__, __func__); \
			BUG();\
			panic("BUG!");\
		}\
	}while(0)


/* On veut que le sous sys usb appel les deux pilotes, audio et commande, et que chacun enregistre son device */
#define USB_VENDOR_DEVICE(vend) \
	.match_flags = USB_DEVICE_ID_MATCH_DEVICE, .idVendor = (vend)

/* Symbole exporte par dectusb_audio.c */
int dectusb_audio_probe(struct usb_interface *intf, const struct usb_device_id *id);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
int usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}

int usb_endpoint_dir_out(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

int usb_endpoint_xfer_bulk(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		USB_ENDPOINT_XFER_BULK);
}

int usb_endpoint_xfer_int(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		USB_ENDPOINT_XFER_INT);
}

int usb_endpoint_xfer_isoc(const struct usb_endpoint_descriptor *epd)
{
	return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		USB_ENDPOINT_XFER_ISOC);
}

int usb_endpoint_is_bulk_in(const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_bulk(epd) && usb_endpoint_dir_in(epd));
}

int usb_endpoint_is_bulk_out(const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_bulk(epd) && usb_endpoint_dir_out(epd));
}

int usb_endpoint_is_int_in(const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_int(epd) && usb_endpoint_dir_in(epd));
}

int usb_endpoint_is_int_out(const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_int(epd) && usb_endpoint_dir_out(epd));
}

int usb_endpoint_is_isoc_in(const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_isoc(epd) && usb_endpoint_dir_in(epd));
}

int usb_endpoint_is_isoc_out(const struct usb_endpoint_descriptor *epd)
{
	return (usb_endpoint_xfer_isoc(epd) && usb_endpoint_dir_out(epd));
}
#endif

#define seek_endpoints_type(endpoint) {	\
	if (usb_endpoint_dir_in(endpoint)) {\
		printk("\tendpoint type :\tdir_in\n");\
	}\
	if (usb_endpoint_dir_out(endpoint)) {\
		printk("\tendpoint type :\tdir_out\n");\
	}\
	if (usb_endpoint_is_bulk_in(endpoint)) {\
		printk("\tendpoint type :\tbulk_in\n");\
	}\
	if (usb_endpoint_is_bulk_out(endpoint)) {\
		 printk("\tendpoint type :\tbulk_out\n");\
	}\
	if (usb_endpoint_is_int_in(endpoint)) {\
		printk("\tendpoint type :\tint_in\n");\
	}\
	if (usb_endpoint_is_int_out(endpoint)) {\
		printk("\tendpoint type :\tint_out\n");\
	}\
	if (usb_endpoint_is_isoc_in(endpoint)) {\
		printk("\tendpoint type :\tisoc_in\n");\
	}\
	if (usb_endpoint_is_isoc_out(endpoint)) {\
		printk("\tendpoint type :\tisoc_out\n");\
	}\
}



#endif

