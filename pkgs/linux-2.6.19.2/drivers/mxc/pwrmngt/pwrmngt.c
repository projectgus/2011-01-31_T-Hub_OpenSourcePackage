/*!
 ***************************************************************************
 * \file pwrmngt_cmds.c
 * \brief summary
 * \ingroup DPM
 *
 * \par Copyright
 \verbatim Copyright 2007 Sagem Communication. All Rights Reserved.
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 \endverbatim
 */

/**************/
/* Prototypes */
/**************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/security.h>
#include <linux/device.h>

#if defined(CONFIG_DEVFS_FS) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#include <linux/devfs_fs_kernel.h>
#else
#include <linux/proc_fs.h>
#endif

#include <asm/arch/pwrmngt_api.h>

#include "pwrmngt.h"

/*!
 * Holds the automatically selected PWRMNGT driver pwrmngt_major number.
 */
static int pwrmngt_major;
// static struct class_simple *pwrmngt_class;

static struct t_pwrmngt_state  powermngt_current_state;

/*!
 * Number of users waiting in suspendq
 */
static int swait = 0;

/*!
 * To indicate whether any of the adc devices are suspending
 */
static int suspend_flag = 0;

/*!
 * The suspendq is used by blocking application calls
 */
static wait_queue_head_t suspendq;

static struct class *pwrmngt_class;

/*!
 * This fonction command transtion between states
 * 
 *
 * @param    target_state   Value of target state 
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int 
pwrmngt_change_state(struct t_pwrmngt_state target_state)
{
int error;
	if ((target_state.mode == powermngt_current_state.mode)&&(target_state.num == powermngt_current_state.num))
	{
		return 0;
	}
	if (powermngt_current_state.mode == (e_pwrmngt_mode) stand_by ) {
		pwrmngt_md_standby_out(powermngt_current_state.num);
	}
	switch (target_state.mode)
	{
		case (e_pwrmngt_mode) normal:
			error = pwrmngt_md_normal_in(target_state.num);
			break;
		case (e_pwrmngt_mode) doze:
			error = pwrmngt_md_doze_in(target_state.num);
			break;
		case (e_pwrmngt_mode) stand_by:
			error = pwrmngt_md_standby_in(target_state.num);
			break;
		case (e_pwrmngt_mode) switch_off:
			error = pwrmngt_md_switchoff_in(target_state.num);
			break;
	}
	if (!error) {
		powermngt_current_state.mode = target_state.mode;
		powermngt_current_state.num = target_state.num;
	}
return error;	
}

/*!
 * This function is called when the driver is opened. This function
 * checks if the user that open the device has root privileges.
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int 
pwrmngt_open(struct inode *inode, struct file *filp)
{
        /*
         * check if the program that opened the driver has root
         * privileges, if not return error.
         */
        if (!capable(CAP_SYS_ADMIN)) {
                return -EACCES;
        }

        return 0;
}

/*!
 * This function is called when the driver is closed. 
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int 
pwrmngt_release(struct inode *inode, struct file *filp)
{
	/* nothing to do */
        return 0;
}

/*!
 * This function provides the IO Controls for simple power management.
 *
 * @param    inode    Pointer to device inode
 * @param    filp     Pointer to device file structure
 * @param    cmd      Ioctl command
 * @param    arg      the parameter (not use)
 *
 *
 * @return    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int
pwrmngt_ioctl(struct inode *inode, struct file *filp,
                  unsigned int cmd, unsigned long arg)
{
struct t_pwrmngt_state target_state;
	switch (cmd) {
		case PWRMNGT_NORMAL :
			target_state.mode = (e_pwrmngt_mode) normal;
			target_state.num = 0;
			break;
		case PWRMNGT_DOZE :
			target_state.mode = (e_pwrmngt_mode) doze;
			target_state.num = 0;
			break;
		case PWRMNGT_DOZE_1 :
			target_state.mode = (e_pwrmngt_mode) doze;
			target_state.num = 1;
			break;
		case PWRMNGT_DOZE_2 :
			target_state.mode = (e_pwrmngt_mode) doze;
			target_state.num = 2;
			break;
		case PWRMNGT_DOZE_3 :
			target_state.mode = (e_pwrmngt_mode) doze;
			target_state.num = 3;
			break;
		case PWRMNGT_STANDBY :
			target_state.mode = (e_pwrmngt_mode) stand_by;
			target_state.num = 0;
			break;
		case PWRMNGT_STANDBY_1 :
			target_state.mode = (e_pwrmngt_mode) stand_by;
			target_state.num = 1;
			break;
		case PWRMNGT_STANDBY_2 :
			target_state.mode = (e_pwrmngt_mode) stand_by;
			target_state.num = 2;
			break;
		case PWRMNGT_SWITCHOFF :
			break;
	}
	pwrmngt_change_state(target_state);
	
return 0;
};

static struct file_operations fops = {
        .open 		= pwrmngt_open,
        .release 	= pwrmngt_release,
        .ioctl 		= pwrmngt_ioctl,
};

/*!
 * This function is called for module initialization.
 * registers the PWRMNGT driver, 
 *
 * @return   0 to indicate success else returns a negative number.
 *
 */
static int __init pwrmngt_init(void)
{
int err;
struct class_device *temp_class;

	printk(KERN_INFO "PWRMNGT: Initialisation\n");

	/* initialisation of current state */
	powermngt_current_state.mode = (e_pwrmngt_mode) init;	
	powermngt_current_state.num = 0;	

        pwrmngt_major = register_chrdev(0, PWRMNGT_DEVICE_NAME, &fops);

	/*
         * Return error if a negative pwrmngt_major number is returned.
         */
        if (pwrmngt_major < 0) {
                printk(KERN_ERR
                       "PWRMNGT: Registering driver failed with %d\n", pwrmngt_major);
                return pwrmngt_major;
        }
	init_waitqueue_head(&suspendq);
	
	pwrmngt_class = class_create(THIS_MODULE, PWRMNGT_DEVICE_NAME);
        if (IS_ERR(pwrmngt_class)) {
                printk(KERN_ERR "PWRMNGT: Error creating class\n");
                err = PTR_ERR(pwrmngt_class);
		return err;
        }

        temp_class = class_device_create(pwrmngt_class, NULL,
                                         MKDEV(pwrmngt_major, 0),
                                         NULL, PWRMNGT_DEVICE_NAME);
        if (IS_ERR(temp_class)) {
                printk(KERN_ERR "PWRMNGT: Error creating class device\n");
		class_destroy(pwrmngt_class);
        	unregister_chrdev(pwrmngt_major, PWRMNGT_DEVICE_NAME);
                err = PTR_ERR(temp_class);
        }


return err;
}

static void __exit pwrmngt_cleanup(void)
{
	class_destroy(pwrmngt_class);
        unregister_chrdev(pwrmngt_major, PWRMNGT_DEVICE_NAME);
}

module_init(pwrmngt_init);
module_exit(pwrmngt_cleanup);

MODULE_AUTHOR("Sagem");
MODULE_DESCRIPTION("Simple Power Management driver");
MODULE_LICENSE("GPL");

