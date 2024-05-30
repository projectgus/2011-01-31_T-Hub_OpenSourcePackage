/*
 *  This driver provide capability to read and write into the factory zone either on EEPROM of Flash Memory.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: factory_conf.c
 *  Creation date: 02/07/2008
 *  Author: Olivier Le Roy, Sagemcom
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

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/i2c.h>
#include <linux/mtd/mtd.h>
#include <linux/delay.h>
#include <asm/uaccess.h> /* copy_from_user, copy_to_user */
#include <asm/arch/factory_conf.h>
#include <asm/arch/board-mx31homescreen.h>
#include <asm/arch/gpio.h>
#include "iomux.h"

#define OUTPUT_MAX  PAGE_SIZE

char *factory_table = NULL;
static int factory_zone_major;
static struct class *factory_zone_class;
static struct proc_dir_entry *proc_entry = NULL;
static unsigned char media_is_i2c_or_flash; /*	0: i2c,
						1: flash. */
extern void homescreen_v1_eeprom_write_unprotect(void);
extern void homescreen_v1_eeprom_write_protect(void);
extern struct mtd_info *get_mtd_device(struct mtd_info *mtd, int num);
static struct mtd_info *mtdblock2; /* factory partition is located on mtd block 2 on HSV3 */

typedef struct {
	char *from;
	char *to;
} substring_t;

/**
 * match: - Determines if a string matches a simple pattern
 * @s: the string to examine for presense of the pattern
 * @p: the string containing the pattern
 * @from: the position in the string where matching the pattern starts
 * @arg: &substring_t variable. Used to return match location.
 *
 * Description: Determines if the pattern @p is present in string @s. If the pattern is found,
 * the location of the argument will be returned in the @arg variable.
 */
static int match(char *s, char *p, int from, substring_t *arg) {
	int pos, i;
	char *tmp;

	if (!p || (strlen(p) > strlen(s))) return 1;
	tmp = kmalloc(strlen(p) + 1, GFP_KERNEL);
	if (!tmp) return -ENOMEM;
	for (pos = from; pos <= strlen(s) - strlen(p); pos++) {
		if (s[pos] == p[0]) {
			for (i = 0; i < strlen(p); i++) *(tmp + i) = *(s + pos + i);
			tmp[strlen(p)] = 0;
			if (strcmp(p, tmp) == 0) {
				arg->from = s + pos;
				arg->to = s + pos+ strlen(p) - 1;
				kfree(tmp);
				return 0;
			}
		}
	}
	kfree(tmp);
	return 1;
}

/**
 * substring_strcpy: - copies the characters from a substring_t to a string
 * @to: string to copy characters to.
 * @s: &substring_t to copy
 *
 * Description: Copies the set of characters represented by the given
 * &substring_t @s to the c-style string @to. Caller guarantees that @to is
 * large enough to hold the characters of @s.
 */
void substring_strcpy(char *to, substring_t *s)
{
	memcpy(to, s->from, s->to - s->from + 1);
	to[s->to - s->from + 1] = 0;
}

int factory_parse_parameter(factory_parameter_t *parameter) {
	int result;
	char *p, *tmp;
	substring_t arg, arg2;

	parameter->application[31] = 0;
	parameter->version[15] = 0;
	parameter->name[31] = 0;
	if (!factory_table) {
		printk(KERN_WARNING "factory_zone - Invalid factory zone!\n");
		return -1;
	}
	tmp = kmalloc(BUFSIZE, GFP_KERNEL);
	if (!tmp) return -ENOMEM;
	p = kmalloc(9 + 31 + 11 + 15 + 3, GFP_KERNEL);
	if (!p) {
		kfree(tmp);
		return -ENOMEM;
	}
	strcpy(p, "<APP id=\"");
	strcat(p, &(parameter->application[0]));
	strcat(p, "\" version=\"");
	strcat(p, &(parameter->version[0]));
	strcat(p, "\">");
	result = match(factory_table, p, 0, &arg);
	if (result != 0) {
		printk(KERN_WARNING "factory_zone - Invalid app / version!\n");
		kfree(p);
		kfree(tmp);
		return -1;
	}
	kfree(p);
	arg2.from = arg.to + 1;
	p = kmalloc(7, GFP_KERNEL);
	if (!p) {
		kfree(tmp);
		return -ENOMEM;
	}
	result = match(factory_table, "</APP>", arg2.from - factory_table, &arg);
	if (result != 0) {
		printk(KERN_WARNING "factory_zone - Invalid xml format!\n");
		kfree(p);
		kfree(tmp);
		return -1;
	}
	kfree(p);
	arg2.to = arg.from - 1;
	substring_strcpy(tmp, &arg2);
	p = kmalloc(15 + 31 + 3, GFP_KERNEL);
	if (!p) {
		kfree(tmp);	
		return -ENOMEM;
	}
	strcpy(p, "<PARAMETER id=\"");
	strcat(p, &(parameter->name[0]));
	strcat(p, "\">");
	result = match(tmp, p, 0, &arg);
	if (result != 0) {
		printk(KERN_WARNING "factory_zone - Parameter not found !\n");
		kfree(p);
		kfree(tmp);
		return -1;
	}
	kfree(p);
	arg2.from = arg.to + 1;
	p = kmalloc(13, GFP_KERNEL);
	if (!p) {
		kfree(tmp);
		return -ENOMEM;
	}
	result = match(tmp, "</PARAMETER>", arg2.from - tmp, &arg);
	if (result != 0) {
		printk(KERN_WARNING "factory_zone - Invalid xml format!\n");
		kfree(p);
		kfree(tmp);
		return -1;
	}
	kfree(p);
	arg2.to = arg.from - 1;
	substring_strcpy(&(parameter->value[0]), &arg2);
	kfree(tmp);
	return 0;
}

EXPORT_SYMBOL_GPL(factory_table);
EXPORT_SYMBOL_GPL(factory_parse_parameter);

/**
 * /proc/factory_zone
 *
 * Reading the proc file in user space will return the current factory table.
 * 
 */
static int
proc_read(char *page, char **start, off_t off, int count,
		 int *eof, void *data)
{
	int len;
	char output[OUTPUT_MAX];
	len = 0;
	

	len += snprintf(output+len, OUTPUT_MAX-len, factory_table);	

	if (len > count) {
		len = count;
	} else {
		*eof = 1;
	}

	memcpy(page, output, len);
	return len;
}

static void erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *)done->priv;
	wake_up(wait_q);
}

static int factory_zone_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long argument) {
	int result, bytecnt;
	u16 address;
	u8 transfer[2];
	substring_t arg, arg2;
	char *buf, *buf2;
	static struct i2c_adapter *adapter;
	static struct i2c_msg msgs[2];
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	static size_t retlen;

	switch (cmd) {

		case RELOAD_FACTORY_TABLE:
reload:
			if (factory_table) kfree(factory_table);
			buf = kmalloc(BUFSIZE, GFP_KERNEL);
			if (!buf) return -ENOMEM;
			if (media_is_i2c_or_flash == 0) /*	0: i2c */
			{
				adapter = i2c_get_adapter(0); /* I2C2 is the only I2C device selected (over 3 possible). First is #0 */
				if (!adapter)
				{
					kfree(buf);
					return -ENODEV;
				}
				transfer[0] = transfer[1] = 0;
				msgs[0].addr = 0x50; /* 24C256 EEprom is wired at address 0x50 */
				msgs[0].flags = 0; /* Selective read */
				msgs[0].len = 2;
				msgs[0].buf = &transfer[0]; /* Start at address 0 */
				msgs[1].addr = 0x50;
				msgs[1].flags = I2C_M_RD;
				msgs[1].len = BUFSIZE;
				msgs[1].buf = buf;
				result = i2c_transfer(adapter, &msgs[0], 2);
			        if (result < 0)
				{
					printk(KERN_DEBUG "factory_conf: %d: i2c read failed\n", result);
					kfree(buf);
					return -EIO;
			        }
			} else if (media_is_i2c_or_flash == 1) /*	1: flash */
			{
				result = mtdblock2->read(mtdblock2, (loff_t)0, BUFSIZE, &retlen, buf);
			        if (retlen != BUFSIZE)
				{
					printk(KERN_DEBUG "factory_conf: %d: flash read failed\n", retlen);
					kfree(buf);
					return -EIO;
			        }
			}
			if (buf[0] != '<' || buf[1] != 'F' || buf[2] != 'A' || buf[3] != 'C' || buf[4] != 'T' \
					|| buf[5] != 'O' || buf[6] != 'R' || buf[7] != 'Y' || buf[8] != '>')
			{
				printk(KERN_NOTICE "Factory zone not present!\n");
				kfree(buf);
				return -EPERM;
			}
			arg2.from = buf;
			result = match(buf, "</FACTORY>\n", 0, &arg);
			if (result != 0) {
				printk(KERN_NOTICE "Factory zone not present!\n");
				kfree(buf);
				return -EPERM;
			}
			arg2.to = arg.to;
			factory_table = kmalloc(arg2.to - arg2.from + 1, GFP_KERNEL);
			if (!factory_table) {
				kfree(buf);
				return -ENOMEM;
			}
			memset(factory_table, 0, arg2.to - arg2.from + 1);
			substring_strcpy(factory_table, &arg2);
			kfree(buf);
			if (!proc_entry)
			{
				proc_entry = create_proc_entry("factory_zone", 0644, NULL);
				if (proc_entry) {
					proc_entry->read_proc  = proc_read;
					proc_entry->data       = NULL;
				}
			}
			printk(KERN_NOTICE "factory table reloaded\n");
			break;

		case WRITE_FACTORY_TABLE:
			buf = kmalloc(BUFSIZE, GFP_KERNEL);
			if (!buf) return -ENOMEM;
			result = copy_from_user(buf, (u32 *)argument, BUFSIZE);
			if (result)
			{
				printk(KERN_ERR "factory_zone_ioctl: %d. WRITE_FACTORY_TABLE: copy_from_user failed.\n", result);
				kfree(buf);
				return -EFAULT;
			}
			if (media_is_i2c_or_flash == 0) /*	0: i2c */
			{
				homescreen_v1_eeprom_write_unprotect(); /* Enable I2C EEprom write */
				adapter = i2c_get_adapter(0); /* First device is #0. I2C2 is the only I2C device selected (over 3 possible). */
				if (!adapter)
				{
					kfree(buf);
					homescreen_v1_eeprom_write_protect(); /* Disable all I2C EEprom writes */
					return -ENODEV;
				}
				for(address = 0; address < BUFSIZE; address += 64)
				{
					bytecnt = (address <= BUFSIZE - 65) ? 64 : BUFSIZE - address -1; 
					transfer[0] = (u8)((address & 0xff00) >> 8);
					transfer[1] = (u8)(address & 0xff);
					msgs[0].addr = 0x50; /* 24C256 EEprom is wired at address 0x50. */
					msgs[0].flags = 0; /* Page write */
					msgs[0].len = 2;
					msgs[0].buf = &transfer[0];
					msgs[1].addr = 0x50;
					msgs[1].flags = 0;
					msgs[1].len = bytecnt;
					msgs[1].buf = (u8 *)(buf + address);
					result = i2c_transfer(adapter, &msgs[0], 2);
					if (result < 0)
					{
						printk(KERN_DEBUG "factory_conf: i2c write failed, page= %d\n", address / 64);
						kfree(buf);
						homescreen_v1_eeprom_write_protect(); /* Disable all I2C EEprom writes */
						return -EIO;
					}
					mdelay(5);
				}
				homescreen_v1_eeprom_write_protect(); /* Disable all I2C EEprom writes */
			} else if (media_is_i2c_or_flash == 1) /*	1: flash */
			{

				/*
				 * First, let's erase the flash block.
				 */

				init_waitqueue_head(&wait_q);
				erase.mtd = mtdblock2;
				erase.callback = erase_callback;
				erase.addr = 0;
				erase.len = 0x20000; /* size is 1 sector = 128k */
				erase.priv = (u_long)&wait_q;

				set_current_state(TASK_INTERRUPTIBLE);
				add_wait_queue(&wait_q, &wait);

				result = mtdblock2->erase(mtdblock2, &erase);
				if (result) {
					kfree(buf);
					set_current_state(TASK_RUNNING);
					remove_wait_queue(&wait_q, &wait);
					printk (KERN_WARNING "mtdblock: erase of region [0x%lx, 0x%x] "
							     "on \"%s\" failed\n",
						(long unsigned int)0, 0x20000, mtdblock2->name);
					return result;
				}

				schedule();  /* Wait for erase to finish. */
				remove_wait_queue(&wait_q, &wait);

				/*
				 * Next, write the data to flash.
				 */

				buf2 = kmalloc(0x20000, GFP_KERNEL);
				if (!buf2)
				{
					kfree(buf);
					return -ENOMEM;
				}
				memset(buf2, 0xFF, 0x20000);
				memcpy(buf2, buf, BUFSIZE);
				result = mtdblock2->write(mtdblock2, (loff_t)0, 0x20000, &retlen, buf2);
			        if (retlen != 0x20000)
				{
					printk(KERN_DEBUG "factory_conf: %d: flash write failed\n", retlen);
					kfree(buf);
					kfree(buf2);
					return -EIO;
			        }
				kfree(buf2);
			}
			kfree(buf);
			printk(KERN_NOTICE "factory table written\n");
			goto reload;

		default:
			pr_debug("factory_zone_ioctl: unsupported ioctl command 0x%x\n", cmd);
			return -EINVAL;
	}
	return 0;
}

struct file_operations factory_zone_fops = {
	.owner = THIS_MODULE,
	.ioctl = factory_zone_ioctl
};


/*
 * Initialization and Exit
 */

static int __init factory_init(void) {
	int result, ret = 0;
	u8 transfer[2];
	substring_t arg, arg2;
	char *buf;
	static struct i2c_adapter *adapter;
	static struct i2c_msg msgs[2];
	static size_t retlen;
	struct class_device *temp_class;


	mtdblock2 = get_mtd_device(NULL, 2); /* factory partition is located on mtd block 2 on HSV3 */
	pr_debug("factory_conf driver loading...\n");
	factory_zone_major = register_chrdev(0, "factory_zone", &factory_zone_fops);
	if (factory_zone_major < 0) {
		printk(KERN_ERR "Unable to get a major for factory_zone\n");
		return factory_zone_major;
	}

	factory_zone_class = class_create(THIS_MODULE, "factory_zone");
	if (IS_ERR(factory_zone_class)) {
		pr_debug(KERN_ERR "Error creating factory_zone class.\n");
		ret = PTR_ERR(factory_zone_class);
		goto err_out1;
	}

	temp_class = class_device_create(factory_zone_class, NULL, MKDEV(factory_zone_major, 0), NULL, "factory_zone");
	if (IS_ERR(temp_class)) {
		pr_debug(KERN_ERR "Error creating factory_zone class device.\n");
		ret = PTR_ERR(temp_class);
		goto err_out2;
	}

	buf = kmalloc(BUFSIZE, GFP_KERNEL);
	if (!buf) return -ENOMEM;


	result = mxc_request_iomux(HOMESCREEN_V1_GPIO_EEPROM_WP, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO);
	if (!result) /* Write Protect GPIO obtained */
	{	
		mxc_set_gpio_direction(HOMESCREEN_V1_GPIO_EEPROM_WP, 0); /* set GPIO WP port direction = 0 for output */
		homescreen_v1_eeprom_write_protect(); /* Disable all I2C EEprom writes */
		adapter = i2c_get_adapter(0); /* First device is #0. I2C2 is the only I2C device selected (over 3 possible). */
	}
	if (!result && adapter)
	{
		transfer[0] = transfer[1] = 0;
		msgs[0].addr = 0x50; /* 24C256 EEprom is wired at address 0x50 */
		msgs[0].flags = 0; /* Selective read */
		msgs[0].len = 2;
		msgs[0].buf = &transfer[0]; /* Start at address 0 */
		msgs[1].addr = 0x50;
		msgs[1].flags = I2C_M_RD;
		msgs[1].len = BUFSIZE;
		msgs[1].buf = buf;
		result = i2c_transfer(adapter, &msgs[0], 2);
		if (result >= 0) /* factory zone: i2c read succeeded. */
		{
			media_is_i2c_or_flash = 0;/*	0: i2c */
			goto media_found;
		}
	}
	result = mtdblock2->read(mtdblock2, (loff_t)0, BUFSIZE, &retlen, buf);
	if (retlen == BUFSIZE) /* factory zone: flash read succeeded. */
	{
		media_is_i2c_or_flash = 1;/*	1: flash */
		goto media_found;
	}
	printk(KERN_ERR "Unable to get a factory zone either in EEPROM or in flash memory\n");
	kfree(buf);
	return -ENODEV;

media_found:
	if (media_is_i2c_or_flash == 0) printk(KERN_NOTICE "factory_conf loaded successfully from e2prom\n");
	else if (media_is_i2c_or_flash == 1) printk(KERN_NOTICE "factory_conf loaded successfully from flash\n");
	if (buf[0] != '<' || buf[1] != 'F' || buf[2] != 'A' || buf[3] != 'C' || buf[4] != 'T' \
			|| buf[5] != 'O' || buf[6] != 'R' || buf[7] != 'Y' || buf[8] != '>')
	{
		printk(KERN_NOTICE "Factory zone not present!\n");
		kfree(buf);
		return -EPERM;
	}
	arg2.from = buf;
	result = match(buf, "</FACTORY>\n", 0, &arg);
	if (result != 0) {
		printk(KERN_NOTICE "Factory zone not present!\n");
		kfree(buf);
		return -EPERM;
	}
	arg2.to = arg.to;
	factory_table = kmalloc(arg2.to - arg2.from + 1, GFP_KERNEL);
	if (!factory_table) {
		kfree(buf);
		return -ENOMEM;
	}
	memset(factory_table, 0, arg2.to - arg2.from + 1);
	substring_strcpy(factory_table, &arg2);
	kfree(buf);
	proc_entry = create_proc_entry("factory_zone", 0644, NULL);
	if (proc_entry) {
		proc_entry->read_proc  = proc_read;
		proc_entry->data       = NULL;
	}
	return ret;
err_out2:
	class_destroy(factory_zone_class);
err_out1:
	unregister_chrdev(factory_zone_major, "factory_zone");
	return ret;
}


static void __exit factory_exit(void) {
	if (proc_entry) {
		remove_proc_entry("factory_zone", NULL);
		proc_entry = NULL;
	}
	class_device_destroy(factory_zone_class, MKDEV(factory_zone_major, 0));
	class_destroy(factory_zone_class);
	unregister_chrdev(factory_zone_major, "factory_zone");
	pr_debug(KERN_INFO "factory_conf driver successfully unloaded\n");
}

subsys_initcall_sync(factory_init);
module_exit(factory_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Olivier Le Roy");
MODULE_DESCRIPTION("Homescreen and Brakali factory zone routines");
