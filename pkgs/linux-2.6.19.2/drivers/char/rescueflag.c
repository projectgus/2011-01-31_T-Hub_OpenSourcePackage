/*
 * linux/drivers/char/rescueflag.c
 *
 * Copyright 2006 Sagem Communications, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/config.h>
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
#include <linux/platform_device.h>

#ifdef CONFIG_PROGRESS
#include <linux/progress.h>
#endif

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	     /* copy_*_user */

#include <asm/arch/hardware.h> /*for irq number*/

/*
 * TODO: move the 3 following lines in a header file.
 */
#define RESCUEFLAG_IOC_MAGIC  'R'
#define RESCUEFLAG_IOCTL_SET_FLAG       _IOW(RESCUEFLAG_IOC_MAGIC,  0, int)
#define RESCUEFLAG_IOCTL_GET_FLAG       _IOR(RESCUEFLAG_IOC_MAGIC,  1, int)
#define RESCUEFLAG_IOCTL_PROGRESS       _IOR(RESCUEFLAG_IOC_MAGIC,  2, int)

#define RESCUEFLAG_OFFSET 0x3FF0

#define RESCUEFLAG_MAJOR 92
/* int rescueflag_minor = 0 ; */

#define RESCUEFLAG_NAME "rescueflag"

/* #define DEBUG */

struct file_operations rescueflag_fops;

static int rescueflag_open(struct inode *inode, struct file *filp)
{
        int retval = 0;
#ifdef DEBUG
        printk("Open rescueflag\n");
#endif
	return retval;
}

static int rescueflag_ioctl(struct inode *inode, struct file *filp,
                     unsigned int cmd, unsigned long arg)
{
        int retval = 0;
        int err = 0;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != RESCUEFLAG_IOC_MAGIC){
                return -ENOTTY;
        }

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if ((_IOC_DIR(cmd) & _IOC_READ) != 0){
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
        } else if ((_IOC_DIR(cmd) & _IOC_WRITE) != 0) {
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
        }

	if (err){
                return -EFAULT;
        }

	switch(cmd) {
        case RESCUEFLAG_IOCTL_SET_FLAG:
                {
                        unsigned int flag[2];
#ifdef DEBUG
                        int i;
#endif
                        retval = copy_from_user(&flag , (void __user *)arg, 8);
                        if(retval != 0){
                                break;
                        }

#ifdef DEBUG
                        printk("set_flag = %08x%08x ", flag[0], flag[1]);
                        for(i=0;i<8;i++){
                                char c = flag[i>>2] >> ((i&3)<<3);
                                printk("%c", c >= 32 && c <= 126 ? c : '?');
                        }
                        printk("\n");
#endif

                        outl(flag[0], IRAM_BASE_ADDR_VIRT + RESCUEFLAG_OFFSET);
                        outl(flag[1], IRAM_BASE_ADDR_VIRT + RESCUEFLAG_OFFSET + 4);
                        outl(flag[0], IRAM_BASE_ADDR_VIRT + RESCUEFLAG_OFFSET + 8);
                        outl(flag[1], IRAM_BASE_ADDR_VIRT + RESCUEFLAG_OFFSET + 12);
                        break;
                }
        case RESCUEFLAG_IOCTL_GET_FLAG:
                {
                        unsigned int flag[2];
#ifdef DEBUG
                        int i;
#endif
                        flag[0] = inl(IRAM_BASE_ADDR_VIRT + RESCUEFLAG_OFFSET);
                        flag[1] = inl(IRAM_BASE_ADDR_VIRT + RESCUEFLAG_OFFSET + 4);

#ifdef DEBUG
                        printk("get_flag = %08x%08x ", flag[0], flag[1]);
                        for(i=0;i<8;i++){
                                char c = flag[i>>2] >> ((i&3)<<3);
                                printk("%c", c >= 32 && c <= 126 ? c : '?');
                        }
                        printk("\n");
#endif

                        retval = copy_to_user((void __user *)arg, &flag , 8);
                        break;
                }
#ifdef CONFIG_PROGRESS_BAR
	case RESCUEFLAG_IOCTL_PROGRESS:
                {
#ifdef DEBUG
                        printk("RESCUEFLAG_IOCTL_PROGRESS: %ld\n", arg);
#endif
                        progress_bar(arg);
                        retval = 0;
                        break;
                }
#endif
        default:
#ifdef DEBUG
                printk("Invalid ioctl command: %d\n", cmd);
#endif
                retval = -ENOIOCTLCMD;    
	}
	return retval;
}

static int rescueflag_release(struct inode *inode, struct file *filp)
{
        int retval = 0;
#ifdef DEBUG
        printk("Close rescueflag\n");        
#endif
	return retval;
}

/*
static ssize_t rescueflag_write(struct file *filp,
                         const char __user *buf,
                         size_t count,
                         loff_t *f_pos)
{
#ifdef DEBUG
        printk("Write rescueflag\n");
#endif
        return -1;
}
*/

int __init rescueflag_init (void)
{
        int result;

#ifdef DEBUG
        printk("rescueflag_init\n");
#endif

	/*
	 * TODO: register only one (Major, Minor) couple.
	 */
        result = register_chrdev(RESCUEFLAG_MAJOR, RESCUEFLAG_NAME, &rescueflag_fops);

	if(result != 0){
	  printk("rescueflag: failed to register driver (%d)\n", result);
	}

        return result;
}


void __exit rescueflag_exit(void)
{
#ifdef DEBUG
        printk("rescueflag_exit\n");
#endif
        unregister_chrdev(RESCUEFLAG_MAJOR, RESCUEFLAG_NAME);
}

struct file_operations rescueflag_fops = {
	.owner =    THIS_MODULE,
        /* .write =    rescueflag_write, */
 	.ioctl =    rescueflag_ioctl, 
	.open =     rescueflag_open,
	.release =  rescueflag_release,
};

module_init(rescueflag_init);
module_exit(rescueflag_exit);
