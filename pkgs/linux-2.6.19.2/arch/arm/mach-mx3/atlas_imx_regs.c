/*
 *  This driver provide capability to read and write ATLAS and i.MX31 registers.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: atlas_imx_regs.c
 *  Creation date: 26/11/2007
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
#include <linux/errno.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* copy_from_user, copy_to_user */
#include <asm/arch/atlas_imx_regs.h>
#include "asm/arch/pmic_external.h"

static int atlas_imx_major;

static struct class *atlas_imx_class;

static int atlas_imx_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	unsigned int param[2];

	switch (cmd) {
		case SET_IMX31_REGISTER:
			if (copy_from_user(&param, (u32 *) arg, sizeof(param))) return -EFAULT;
			__raw_writel(param[1], IO_ADDRESS(param[0]));
			break;

		case GET_IMX31_REGISTER:
			if (copy_from_user(&param[0], (u32 *)arg, sizeof(param[0]))) return -EFAULT;
			param[1] = __raw_readl(IO_ADDRESS(param[0]));
			if (copy_to_user((u32 *)arg, &param, sizeof(param))) return -EFAULT;
			break;

		case SET_ATLAS_REGISTER:
			if (copy_from_user(&param, (u32 *) arg, sizeof(param))) return -EFAULT;
			pmic_write_reg(param[0], param[1], PMIC_ALL_BITS);
			break;

		case GET_ATLAS_REGISTER:
			if (copy_from_user(&param[0], (u32 *)arg, sizeof(param[0]))) return -EFAULT;
			pmic_read_reg(param[0], &param[1], PMIC_ALL_BITS);
			if (copy_to_user((u32 *) arg, &param, sizeof(param))) return -EFAULT;
			break;

		default:
			pr_debug("atlas_imx_ioctl: unsupported ioctl command 0x%x\n", cmd);
			return -EINVAL;
	}
	return 0;
}
struct file_operations atlas_imx_fops = {
	.owner = THIS_MODULE,
	.ioctl = atlas_imx_ioctl,
};

/*
 * Initialization and Exit
 */

static int __init atlas_imx_init(void) {
	int ret = 0;

	struct class_device *temp_class;

	pr_debug("atlas_imx_regs driver loading...\n");
	atlas_imx_major = register_chrdev(0, "atlas_imx_regs", &atlas_imx_fops);
	if (atlas_imx_major < 0) {
		printk(KERN_ERR "Unable to get a major for atlas_imx_regs\n");
		return atlas_imx_major;
	}
	atlas_imx_class = class_create(THIS_MODULE, "atlas_imx_regs");
	if (IS_ERR(atlas_imx_class)) {
		pr_debug(KERN_ERR "Error creating atlas_imx class.\n");
		ret = PTR_ERR(atlas_imx_class);
		goto err_out1;
	}
	temp_class = class_device_create(atlas_imx_class, NULL, MKDEV(atlas_imx_major, 0), NULL, "atlas_imx_regs");
	if (IS_ERR(temp_class)) {
		pr_debug(KERN_ERR "Error creating atlas_imx class device.\n");
		ret = PTR_ERR(temp_class);
		goto err_out2;
	}
	printk(KERN_NOTICE "atlas_imx_regs loaded successfully\n");
	return ret;

      err_out2:
	class_destroy(atlas_imx_class);
      err_out1:
	unregister_chrdev(atlas_imx_major, "atlas_imx_regs");
	return ret;
}

static void __exit atlas_imx_exit(void) {
	class_device_destroy(atlas_imx_class, MKDEV(atlas_imx_major, 0));
	class_destroy(atlas_imx_class);
	unregister_chrdev(atlas_imx_major, "atlas_imx_regs");
	pr_debug(KERN_INFO "atlas_imx_regs driver successfully unloaded\n");
}


module_init(atlas_imx_init);
module_exit(atlas_imx_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Olivier Le Roy <olivier.leroy@sagem.com>");
MODULE_DESCRIPTION("ATLAS i.MX31 registers access driver");

