/*
 *  Driver for leds.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: led-status.c
 *  Creation date: 29/08/2008
 *  Author: Karl Leplat, Sagemcom
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
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>

#include <asm/io.h>
#include <asm/byteorder.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>

extern void gpio_serviceled_active (void);
extern void gpio_serviceled_inactive(void);
extern void gpio_serviceled_set (int value);
extern int  gpio_serviceled_get (void);

/*Drivers for led status */

static int led_proc_read (char *page, char **start, off_t off, int count, int *eof,
		          void *data)
{
	char *out = page;
	int len;

        out += snprintf (out, count,
                         "%d\n", gpio_serviceled_get());
	len = out - page;
	len -= off;

	if (len < count) {
		*eof = 1;
		if (len <= 0)
			return 0;
	} else
		len = count;

	*start = page + off;
	return len;
}

static int led_proc_write (struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
   int i;

   if (!access_ok (VERIFY_READ, buffer, count))
      return -EFAULT;
   
   for (i = 0; i < count; i++) {
      char c;
      if (get_user(c, buffer))
         return -EFAULT;
      
      switch (c) {
	case '0':
           gpio_serviceled_set (0);
           break;

	case '1':
           gpio_serviceled_set (1);
           break;

	default:
		printk (KERN_DEBUG "%c is not valid\n", c);
		break;
      }
      buffer++;
   }

   return count;
}


int __devinit led_init (void)
{
   struct proc_dir_entry* entry;
   
   gpio_serviceled_active(); 
   entry = create_proc_entry ("led", 0660, NULL);
   if (entry)
   {
      entry->nlink      = 1;
      entry->write_proc = led_proc_write;
      entry->read_proc  = led_proc_read;
      entry->data       = NULL;
   }
   
   return 0;
}

void led_exit (void)
{
   remove_proc_entry ("led", NULL);
   gpio_serviceled_inactive();
}
module_init (led_init);
module_exit (led_exit);

MODULE_AUTHOR("Sagem");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Service");

