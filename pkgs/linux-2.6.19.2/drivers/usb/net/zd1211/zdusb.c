/* src/zdusb.c
*
* Implements the functions of the ZyDAS zd1211 MAC
*
* Copyright (C) 2004 ZyDAS Inc.  All Rights Reserved.
* --------------------------------------------------------------------
*
*
*
*   The contents of this file are subject to the Mozilla Public
*   License Version 1.1 (the "License"); you may not use this file
*   except in compliance with the License. You may obtain a copy of
*   the License at http://www.mozilla.org/MPL/
*
*   Software distributed under the License is distributed on an "AS
*   IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
*   implied. See the License for the specific language governing
*   rights and limitations under the License.
*
*   Alternatively, the contents of this file may be used under the
*   terms of the GNU Public License version 2 (the "GPL"), in which
*   case the provisions of the GPL are applicable instead of the
*   above.  If you wish to allow the use of your version of this file
*   only under the terms of the GPL and not to allow others to use
*   your version of this file under the MPL, indicate your decision
*   by deleting the provisions above and replace them with the notice
*   and other provisions required by the GPL.  If you do not delete
*   the provisions above, a recipient may use your version of this
*   file under either the MPL or the GPL.
*
* -------------------------------------------------------------------- */

#include <linux/version.h>

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif

#include <linux/module.h>

#include <linux/usb.h>

#include "zd1205.h"
#include "zdusb.h"
#include "zddebug.h"
#include "zdversion.h"
#include "zd1211.h"

#define ZD1211_DBG_LEVEL    1

MODULE_AUTHOR("Yarco Yang");
MODULE_DESCRIPTION("ZyDAS 802.11b/g USB Wireless LAN adapter");
MODULE_LICENSE("GPL");

#ifdef ZD1211
static const char driver_name[] = "zd1211";
#elif defined(ZD1211B)
static const char driver_name[] = "zd1211b";
#endif


/* table of devices that work with this driver */
static struct usb_device_id zd1211_ids [] = {
#ifdef ZD1211B
	{ USB_DEVICE(VENDOR_ZYDAS, 0x1215) },
    { USB_DEVICE(VENDOR_ZYDAS, 0xA215) },
    { USB_DEVICE(0x0053, 0x5301) },
    { USB_DEVICE(0x0053, 0x5302) },
    { USB_DEVICE(0x2019, 0x5303) }, //Add, 2006.04.17
    { USB_DEVICE(0x050D, 0x4050) },
    { USB_DEVICE(0x050D, 0x705C) },
    { USB_DEVICE(0x0586, 0x340F) },
    { USB_DEVICE(0x079B, 0x0062) },
    { USB_DEVICE(0x083A, 0x4505) },
    { USB_DEVICE(0x083A, 0xE501) },
    { USB_DEVICE(0x0BAF, 0x0121) },
    { USB_DEVICE(0x0CDE, 0x001A) },
    { USB_DEVICE(0x0DF6, 0x9075) },
    { USB_DEVICE(0x0F88, 0x3014) },
    { USB_DEVICE(0x1233, 0x0471) },
    { USB_DEVICE(0x1582, 0x6003) },
#elif defined(ZD1211)
	{ USB_DEVICE(VENDOR_ZYDAS, PRODUCT_1211) },
    { USB_DEVICE(VENDOR_ZYDAS, PRODUCT_A211) },
    { USB_DEVICE(VENDOR_ZYXEL, PRODUCT_G220) },
    { USB_DEVICE(VENDOR_3COM,  PRODUCT_A727) },
    { USB_DEVICE(0x2019, 0xc008) },
    { USB_DEVICE(0x2019, 0xc009) },
    { USB_DEVICE(0x079b, 0x004a) },
    { USB_DEVICE(0x07b8, 0x6001) },
    { USB_DEVICE(0x0b3b, 0x1630) },
    { USB_DEVICE(0x0b3b, 0x5630) },
    { USB_DEVICE(0x0b3b, 0x6630) },
    { USB_DEVICE(0x0cde, 0x0011) },
    { USB_DEVICE(0x0df6, 0x9071) },
    { USB_DEVICE(0x126f, 0xa006) },
    { USB_DEVICE(0x129b, 0x1666) },
    { USB_DEVICE(0x1435, 0x0711) },
    { USB_DEVICE(0x0DF6, 0x9071) },
    { USB_DEVICE(0x0105, 0x145F) },
    
#endif
	{ }					/* Terminating entry */
};


MODULE_DEVICE_TABLE(usb, zd1211_ids);
int zd1211_FirstLoad = 1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
module_param(zd1211_FirstLoad, int, S_IRUGO);
#else
MODULE_PARM(zd1211_FirstLoad, "i");
#endif


extern struct net_device *g_dev;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
static void *zd1211_probe(struct usb_device *dev, unsigned int ifnum,
			   const struct usb_device_id *id)
#else
static int zd1211_probe(struct usb_interface *interface,
	const struct usb_device_id *id)
#endif			   
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	struct usb_interface *interface = &dev->actconfig->interface[ifnum];
#else
	struct usb_device *dev = interface_to_usbdev(interface);
#endif
	
	struct net_device *net = NULL;
	struct zd1205_private *macp = NULL;
	int vendor_id, product_id;
	int dev_index = id - zd1211_ids;
	int result = 0;
    
	//char serial_number[30];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
 //Drivers for USB interfaces should normally record such references in
 //their probe() methods, when they bind to an interface, and release
 //them by calling usb_put_dev(), in their disconnect() methods.
 //A pointer to the device with the incremented reference counter is returned.
	usb_get_dev(dev);
#endif    

	vendor_id = zd_le16_to_cpu(dev->descriptor.idVendor);
	product_id = zd_le16_to_cpu(dev->descriptor.idProduct);

#if 1
	printk(KERN_NOTICE "vendor_id = %04x\n", vendor_id);
	printk(KERN_NOTICE "product_id = %04x\n", product_id);

	if (dev->speed == USB_SPEED_HIGH)
		printk(KERN_NOTICE "USB 2.0 Host\n");
	else
		printk(KERN_NOTICE "USB 1.1 Host\n");  
#endif
	
	//memset(serial_number, 0, 30);
	//usb_string(dev, dev->descriptor.iSerialNumber, serial_number, 29);
	//printk("Device serial number is %s\n", serial_number);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))	
	if (usb_set_configuration(dev, dev->config[0].bConfigurationValue)) {
        	printk(KERN_ERR "usb_set_configuration() failed\n");
		result = -EIO;
		goto fail;
	}
#endif
    
#if 1
	//kernel 2.6
	if (!(macp = kmalloc(sizeof(struct zd1205_private), GFP_KERNEL))) {
		printk(KERN_ERR "out of memory allocating device structure\n");
		result = -ENOMEM;
		goto fail;
	}
	
	memset(macp, 0, sizeof(struct zd1205_private));
#endif		


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	usb_inc_dev_use(dev);
#endif    

	net = alloc_etherdev(0);  //kernel 2.6
	//net = alloc_etherdev(sizeof (struct zd1205_private));  //kernel 2.4

	if (!net) {
		printk(KERN_ERR "zd1211: Not able to alloc etherdev struct\n");
		result = -ENOMEM;
		goto fail1;
	}
	
	g_dev = net;  //save this for CBs use
	//macp = net->priv; //kernel 2.4
	net->priv = macp;   //kernel 2.6
	macp->device = net;
	macp->usb = dev;
	SET_MODULE_OWNER(net);
	macp->dev_index = dev_index;
	macp->release = zd_le16_to_cpu(dev->descriptor.bcdDevice);
	printk(KERN_NOTICE "Release Ver = %04x\n", macp->release);
	macp->flags = 0;
	macp->dbg_flag = ZD1211_DBG_LEVEL;

	/* set up the endpoint information */
	/* check out the endpoints */
	macp->interface = interface;
	
	init_waitqueue_head(&macp->regSet_wait);
	init_waitqueue_head(&macp->iorwRsp_wait);
	init_waitqueue_head(&macp->term_wait);
	init_waitqueue_head(&macp->msdelay);

	if (!zd1211_alloc_all_urbs(macp)){
        printk("Calling zd1211_alloc_all_urbs fails\n");
		result = -ENOMEM;
		goto fail2;
	}	

	//zd1211_DownLoadUSBCode(macp, "WS11Uext.bin", NULL, cFIRMWARE_EXT_CODE);
    //If the driver was removed and reinstall without unplug the device.
    //You can tell the driver not to download the firmware again by issing
    //insmod zd1211b zd1211_FirstDown=0. Redownload causes device crash
    if(zd1211_FirstLoad) 
    {
        if (zd1211_Download_IncludeFile(macp) != 0){
            printk(KERN_ERR "zd1211_Download_IncludeFile failed\n");
            result = -EIO;
            goto fail3;
        }
    }

	//to enable firmware
//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))    
//	if (usb_set_configuration(dev, dev->config[0].bConfigurationValue)) {
//#else
//	if (usb_set_interface(dev, interface->altsetting[0].desc.bInterfaceNumber, 	0)){
        // Use the lowest USBD API to issue set_configuration command.
    if ((usb_control_msg(dev, usb_sndctrlpipe(dev,0),USB_REQ_SET_CONFIGURATION,0, 1, 0, NULL, 0, HZ))<0)
    {
//#endif        
		printk(KERN_ERR "usb_set_configuration() failed\n");
		result = -EIO;
		goto fail3;
	}
	
	set_bit(ZD1211_RUNNING, &macp->flags);
	macp->bUSBDeveiceAttached = 1;

 	if (!zd1211_InitSetup(net, macp))
    {
        printk("Calling zd1211_InitSetup fails\n");
		result = -EIO;
		goto fail3;
	}
	else
    {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
		usb_set_intfdata(interface, macp);
		SET_NETDEV_DEV(net, &interface->dev);
		//defer_kevent(macp, KEVENT_REGISTER_NET);
#endif

#if 1 //don't register net
		if (register_netdev(net) != 0) 
        {
            printk("register_netdev fails\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))            
			usb_set_intfdata(interface, NULL);
#endif            
			goto fail3;
		}
#endif          
	}
    
	goto done;    
	
 fail3:
	zd1211_free_all_urbs(macp);	

 fail2:
	free_netdev(net);  //kernel 2.6
	//kfree(net);
    
 fail1:
	kfree(macp);
    
 fail:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	usb_put_dev(dev);
#endif
	macp = NULL;
        goto exit;
done: 
    netif_carrier_off(macp->device); 
exit:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
	return macp;
#else
	return result;
#endif	
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
static void zd1211_disconnect(struct usb_device *dev, void *ptr)
#else
static void zd1211_disconnect(struct usb_interface *interface)
#endif
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	struct zd1205_private *macp = (struct zd1205_private *) usb_get_intfdata(interface);
#else
	struct zd1205_private *macp = (struct zd1205_private *)ptr;
#endif	

	if (!macp) {
		printk(KERN_ERR "unregistering non-existant device\n");
		return;
	}

	set_bit(ZD1211_UNPLUG, &macp->flags);
	macp->bUSBDeveiceAttached = 0;

	if (macp->driver_isolated){
		if (macp->device->flags & IFF_UP)
			zd1205_close(macp->device);
	}
    
	unregister_netdev(macp->device);
    
	//assuming we used keventd, it must quiesce too
	flush_scheduled_work();
    mdelay(1000);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))    
	usb_dec_dev_use(dev);
#else
	usb_put_dev(interface_to_usbdev(interface));
#endif

	mdelay(1);
	zd1211_unlink_all_urbs(macp);
	mdelay(1);
	zd1211_free_all_urbs(macp);
	mdelay(1);
	zd1205_clear_structs(macp->device);
	kfree(macp);
	macp = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))
	usb_set_intfdata(interface, NULL);
#endif
	//ZEXIT(0);
}

static struct usb_driver zd1211_driver = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0))    
	//.owner =		THIS_MODULE,
#endif    
	.name =		    driver_name,
	.probe =		zd1211_probe,
	.disconnect =	zd1211_disconnect,
	.id_table =	    zd1211_ids,
};


int __init zd1211_init(void)
{
        printk("\n");
        printk(" _____     ____    _    ____\n");
        printk("|__  /   _|  _ \\  / \\  / ___|\n");
        printk("  / / | | | | | |/ _ \\ \\___ \\\n");
        printk(" / /| |_| | |_| / ___ \\ ___) |\n");
        printk("/____\\__, |____/_/   \\_\\____/\n");
        printk("     |___/\n");

	printk(KERN_NOTICE "%s - version %s\n",  DRIVER_NAME, VERSIONID);
	return usb_register(&zd1211_driver);
}

void __exit zd1211_exit(void)
{
	usb_deregister(&zd1211_driver);
}

module_init(zd1211_init);
module_exit(zd1211_exit);

