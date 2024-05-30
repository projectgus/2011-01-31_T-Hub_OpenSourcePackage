/*
 *  Spi driver for dect module.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: spi_dect.c
 *  Creation date: 17/07/2009
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


#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_AUDIO)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

#include "spi_dect.h"
#include <linux/mutex.h>

LIST_HEAD(spi_dect_dev_list);
DEFINE_MUTEX(spi_dect_dev_list_lock);

unsigned long minors[SPI_DECT_MINORS / BITS_PER_LONG];


/*struct spi_dect_activity *spi_dect_act;*/
struct workqueue_struct		*spi_dect_queue;
struct work_struct		spi_dect_work;
struct spi_dect_export_sysfs {
	int				nb_of_msg;		/*! Number of activity messages (32 bytes per messages) */
	int				cksum_mach_rep;
	int				cksum_mach_norep;	
	int				basc;			/*! Test If 1, sending 2 tranfers in 1 message. */
	int				debug_cmd;		/*! if not zero, print command message. */
	struct mutex            	mutex;
};
struct spi_dect_export_sysfs spi_dect_ex;

void spi_dect_clear_exported_vars(void){
	mutex_lock(&spi_dect_ex.mutex);
	spi_dect_ex.nb_of_msg = 0;
	spi_dect_ex.cksum_mach_rep = 0;
	spi_dect_ex.cksum_mach_norep = 0;
	mutex_unlock(&spi_dect_ex.mutex);
	return;
}

int activity = 0;

inline static void spi_dect_print_buffer(const unsigned char *p, int s, char *m){
	int i;
	printk("%s : (%d bytes) : \n", m, s);
	for(i = 0; i < s; i++){
		printk("%x -", p[i]);
	}
	printk("\n");
}

extern void spi_dect_set_basc_on(void){
	printk("Set basc on\n");
	spi_dect_ex.basc = 1;
}

void spi_dect_set_spi_loopback(struct spi_device *spi){
	struct spi_bus_info *cd = spi->controller_data;
	cd->spi_loopback = !cd->spi_loopback;
	printk(KERN_DEBUG "Loopback mode (%s)\n", (cd->spi_loopback)?"ON":"OFF");
}

int spi_dect_start_stop_activity(void){
	if(activity){
		printk("Stop activity\n");
		activity = 0;
	} else {
		printk("Start activity\n");
		activity = 1;
		if(!queue_delayed_work(spi_dect_queue, &spi_dect_work, usecs_to_jiffies(2500))){
			printk(KERN_ERR "%s %d : Error : cannot add work. FATAL : There is no activity on spi 1\n", __func__, __LINE__);
			activity = 0;
			return -EINVAL;
		}
	}
	return 0;
}

struct spi_dect_send_msg_list {
	struct list_head	list;
	unsigned char		m[SPI_DECT_MSG_LENGTH];	/*! Message to send */
	int			s;			/*! Size of message to send */
};

int spi_dect_add_message_to_send(struct spi_dect_context *spd, const unsigned char *msg, const int size){
	int done = 0;
	int rest = 0;
	int ret = 0;
	struct spi_dect_send_msg_list *msgl;

	if(size < 1 || !spd || !msg)
		return -EINVAL;

	if(spi_dect_ex.debug_cmd){
		spi_dect_print_buffer(msg, size, "Asking to send command");
	}

	mutex_lock(&spd->message_mutex);
	while(size - done){

		if(done > size){
			printk("%s() %d : FATAL (%d > %d)\n", __func__, __LINE__, done, size);
			BUG();
			panic("Invaid value");
		}

		msgl = kzalloc(sizeof(struct spi_dect_send_msg_list), GFP_KERNEL);
		if(!msgl){
			ret = -ENOMEM;
			break;
		}

		memset(msgl->m, SPI_DECT_PADDING, SPI_DECT_MSG_LENGTH);

		if((size - done) < SPI_DECT_MSG_LENGTH){
			rest = size - done;
			msgl->m[0] = rest;
			msgl->s = rest+1;
			memcpy(msgl->m+1, msg+done, rest);
			done += rest;
			list_add_tail(&msgl->list, &spd->message_list);
			if(spi_dect_ex.debug_cmd)
				printk(KERN_DEBUG "%s() %d : God %d bytes from buffer %p (done %d)\n", __func__, __LINE__, msgl->s, (void *)msg, done);
		} else if((size - done) >= SPI_DECT_MSG_LENGTH){
			msgl->m[0] = SPI_DECT_MSG_LENGTH-1;
			msgl->s = SPI_DECT_MSG_LENGTH;
			memcpy(msgl->m+1, msg+done, SPI_DECT_MSG_LENGTH-1);
			done += SPI_DECT_MSG_LENGTH-1;
			list_add_tail(&msgl->list, &spd->message_list);
			if(spi_dect_ex.debug_cmd)
				printk(KERN_DEBUG "%s() %d : God %d bytes from buffer %p (done %d)\n", __func__, __LINE__, msgl->s, (void *)msg, done);
		}
	}
	mutex_unlock(&spd->message_mutex);
	return ret;
}

int spi_dect_parse_message(struct spi_transfer *t){
	unsigned char *p;
	unsigned int i, cksum = 0;
	if(!t)
		return -EFAULT;

	p = t->rx_buf;
	if(spi_dect_ex.debug_cmd > 1){
		spi_dect_print_buffer(p, t->len * 2, "Got message from DECT");
	}

	
	for(i = 0; i < SPI_DECT_MSG_LENGTH; i ++){
		cksum ^= (p[i] + 1) << i;
	}
	
	if(cksum == 0xca15ca5c){
		spi_dect_ex.cksum_mach_norep++;
	} else if(cksum == 0x28572894){
		spi_dect_ex.cksum_mach_rep++;
	}


	return 0;
}

static void spi_dect_handle_workqueue(void *arg){
	int ret;
	unsigned char tbuff[SPI_DECT_MSG_LENGTH];
	unsigned char rbuff[SPI_DECT_MSG_LENGTH];
	struct spi_dect_send_msg_list *lm = NULL;
	struct spi_dect_context *spi_dect_ctx = (struct spi_dect_context *)arg;
	struct spi_transfer t;
	struct spi_message m;

#if defined(DO_ULOG)
	ulog(ULOG_SPI_DECT_ACTIVITY, 0);
#endif

	if(!activity){
		printk(KERN_DEBUG "%s() %d : Stop activity\n", __func__, __LINE__);
		return;
	}

	spi_message_init(&m);

	memset(&t, 0x0, sizeof(struct spi_transfer));
	memset(rbuff, SPI_DECT_PADDING, SPI_DECT_MSG_LENGTH);

	if(!(list_empty(&spi_dect_ctx->message_list))){
		mutex_lock(&spi_dect_ctx->message_mutex);
		list_for_each_entry(lm, &spi_dect_ctx->message_list, list){
			memcpy(tbuff, lm->m, SPI_DECT_MSG_LENGTH);
			list_del(&lm->list);
			if(spi_dect_ex.debug_cmd)
				printk(KERN_DEBUG "%s() %d : Sending command 0x%x\n", __func__, __LINE__, lm->m[1]);
			kfree(lm);
			lm = NULL;
			break;
		}
		mutex_unlock(&spi_dect_ctx->message_mutex);
	}else{
		memset(tbuff, SPI_DECT_PADDING, SPI_DECT_MSG_LENGTH);
		tbuff[0] = 0;
	}

	t.tx_buf = (const void *)tbuff;
	t.rx_buf = rbuff;
	t.len = SPI_DECT_MSG_LENGTH/2;
	spi_message_add_tail(&t, &m);

	mutex_lock(&spi_dect_ex.mutex); /* protect data exported in /sys */

	if(spi_dect_ex.debug_cmd > 2){
		spi_dect_print_buffer(tbuff, SPI_DECT_MSG_LENGTH, "Sending message");
	}

	ret = spi_sync(spi_dect_ctx->spi, &m);
	if(ret || m.status != 0){
		dev_err(&spi_dect_ctx->spi->dev, "%s() %d: Error : ret %d status %d. There is no more activity. Reboot screen\n", __func__, __LINE__, 
				ret, m.status);

		mutex_unlock(&spi_dect_ex.mutex);
		return;
	}
	spi_dect_ex.nb_of_msg++;

	spi_dect_parse_message(&t);

	mutex_unlock(&spi_dect_ex.mutex); /* protect data exported in /sys */
	if(!queue_delayed_work(spi_dect_queue, &spi_dect_work, usecs_to_jiffies(2500)))
		printk("%s %d : cannot add work. There is no activity on spi 1\n", __func__, __LINE__);

#if defined(DO_ULOG)
	ulog(ULOG_SPI_DECT_ACTIVITY, 1);
#endif

	return;
}

static irqreturn_t spi_dect_irq(int irq, void *dev_id){
	printk("Got irq from spi 1 (irq %d)\n", irq);
	return 0;
}

void spi_dect_set_mode(struct spi_device *spi){
	spi->mode = ++(spi->mode)%4;
	printk("Setting mode to %d\n", spi->mode);
	return;
}

static int spi_dect_probe(struct spi_device *spi){
	int ret;
	struct spi_dect_context *spi_dect_ctx = NULL;
	int minor;
	struct spi_master *master = NULL;
	printk(KERN_DEBUG "Probe\n");

	spi->bits_per_word = SPI_DECT_BITS_PER_WORD;

	minor = find_first_zero_bit(minors, SPI_DECT_MINORS);
	if(minor >= SPI_DECT_MINORS){
		dev_err(&spi->dev, "Could not find first zero bit for minor\n");
		return -ENODEV;
	}

	master = spi_alloc_master(&(spi->dev), sizeof(struct spi_dect_context));
	if(!master){
		dev_err(&(spi->dev), "Could not alloc space for spi master\n");
		return -ENOMEM;
	}
	spi_dect_ctx = spi_master_get_devdata(master);
	spi_dect_ctx->master = master;
	spi_dect_ctx->spi = spi;

	spi_dect_ctx->devt = MKDEV(SPI_DECT_MAJOR, minor);
	set_bit(minor, minors);

	mutex_lock(&spi_dect_dev_list_lock);
	INIT_LIST_HEAD(&spi_dect_ctx->device_entry);
	list_add(&spi_dect_ctx->device_entry, &spi_dect_dev_list);
	mutex_unlock(&spi_dect_dev_list_lock);

	mutex_init(&spi_dect_ctx->message_mutex);
	INIT_LIST_HEAD(&spi_dect_ctx->message_list);
	
	dev_set_drvdata(&(spi->dev), spi_dect_ctx);

	ret = request_irq(INT_UART2, spi_dect_irq, 0, "spi_dect", spi_dect_ctx);
	if(ret){
		dev_err(&spi_dect_ctx->spi->dev, "request_irq failed for spi_dect (bus num %d)\n", master->bus_num);
		spi_master_put(master);
		return ret;
	}


	spi_dect_queue = create_workqueue("spi_dect_workqueue");
	if(!spi_dect_queue){
		printk(KERN_ERR "Cannot allocate memory for spi_dect_workqueue\n");
		spi_master_put(master);
		return -ENOMEM;
	}
	INIT_WORK(&spi_dect_work, spi_dect_handle_workqueue, spi_dect_ctx);

	activity = 1;
	if(!queue_delayed_work(spi_dect_queue, &spi_dect_work, usecs_to_jiffies(2500))){
		printk("%s %d : Error : cannot add work. FATAL : There is no activity on spi 1\n", __func__, __LINE__);
		spi_master_put(master);
		destroy_workqueue(spi_dect_queue);
	}

	dev_dbg(&spi_dect_ctx->spi->dev, "Probe done : @ spi (%p) ctx (%p)\n", spi, spi_dect_ctx);
	return 0;
}


static int spi_dect_remove(struct spi_device *spi){
	struct spi_dect_context *spi_dect_ctx = dev_get_drvdata(&spi->dev);
	dev_dbg(&spi->dev, "Removing\n");
	dev_set_drvdata(&(spi->dev), NULL);
	if(!spi_dect_ctx){
		dev_err(&spi->dev,"spi_dev : Null pointer : spi ctx (%p)\n", spi_dect_ctx);
		return -EFAULT;
	}
	if(spi_dect_queue){
		cancel_delayed_work(&spi_dect_work);
		flush_workqueue(spi_dect_queue);
		destroy_workqueue(spi_dect_queue);
	}

	dev_dbg(&spi->dev,"Remove : @ spi (%p) ctx (%p)\n", spi_dect_ctx->spi, spi_dect_ctx);
	free_irq(INT_UART2, spi_dect_ctx);
	mutex_lock(&spi_dect_dev_list_lock);
	list_del(&spi_dect_ctx->device_entry);
	mutex_unlock(&spi_dect_dev_list_lock);
	clear_bit(MINOR(spi_dect_ctx->devt), minors);
	if(!spi_dect_ctx->in_use){
		spi_master_put(spi_dect_ctx->master);
	}else{
		printk("ERROR : spi_dect_remove : spi dect device still in use. Cannot free memory !\n");
		BUG();
		return -EBUSY;
	}


	return 0;
}

static struct spi_driver spi_dect_driver = {
    .driver = {
        .name       = SPI_DECT_DEVICE_NAME,
        .owner      = THIS_MODULE,
    },
    .probe          = spi_dect_probe,
    .remove         = spi_dect_remove,
};


static ssize_t spi_dect_sys_read_db(struct sys_device *dev, char *buf){
	int size = 0;
	mutex_lock(&spi_dect_ex.mutex);
	size += sprintf((buf + size), "Debug level : %d\n", spi_dect_ex.debug_cmd);
	mutex_unlock(&spi_dect_ex.mutex);
	return size;
}

static ssize_t spi_dect_sys_write_db(struct sys_device *dev, const char *buf, size_t size){
	int data = 0;
	data = simple_strtol(buf, NULL, 10);
	if(data < 0){
		return -EINVAL;
	}
	printk(KERN_DEBUG "Setting debug level to %d\n", data);
	mutex_lock(&spi_dect_ex.mutex);
	spi_dect_ex.debug_cmd = data;
	mutex_unlock(&spi_dect_ex.mutex);
	return size;
}


static ssize_t spi_dect_sys_read(struct sys_device *dev, char *buf){
	int size = 0;
	mutex_lock(&spi_dect_ex.mutex);
	size += sprintf((buf + size), "Actvity : (%s) Nb of messages (%d)\n", activity?"ON":"OFF", spi_dect_ex.nb_of_msg);

	size += sprintf((buf + size), "Nb of checksum ok : repeate (%d) no repeate (%d)\n",
		       spi_dect_ex.cksum_mach_rep, spi_dect_ex.cksum_mach_norep);	

	size += sprintf((buf + size), "Debug messages %d\n", spi_dect_ex.debug_cmd);
	mutex_unlock(&spi_dect_ex.mutex);
	return size;
}

static SYSDEV_ATTR(spi_dect, 0444, spi_dect_sys_read, NULL);
static SYSDEV_ATTR(spi_dect_db, 0666, spi_dect_sys_read_db, spi_dect_sys_write_db);

static struct sysdev_class spi_dect_sysclass = {
	 set_kset_name("spi_dect"),
};

static struct sys_device spi_dect_sysdevice = {
	.id = 0,
	.cls = &spi_dect_sysclass,
};

static void spi_dect_sysdev_exit(void){
	sysdev_remove_file(&spi_dect_sysdevice, &attr_spi_dect_db);
	sysdev_remove_file(&spi_dect_sysdevice, &attr_spi_dect);
	sysdev_unregister(&spi_dect_sysdevice);
	sysdev_class_unregister(&spi_dect_sysclass);

}

static int spi_dect_sysdev_init(void){
	int err;
	err = sysdev_class_register(&spi_dect_sysclass);
	if(err){
		printk(SPI_DECT_DEVICE_NAME " Could not register class : error %d", err);
		return err;
	}

	err = sysdev_register(&spi_dect_sysdevice);
	if(err){
		printk(SPI_DECT_DEVICE_NAME" Could not register device : error %d", err);
		spi_dect_sysdev_exit();
		return err;
	}

	err = sysdev_create_file(&spi_dect_sysdevice, &attr_spi_dect);
	if(err){
		printk(SPI_DECT_DEVICE_NAME" Could not create a sysdev file to export pins state : error %d", err);
		spi_dect_sysdev_exit();
		return err;
	}

	err = sysdev_create_file(&spi_dect_sysdevice, &attr_spi_dect_db);
	if(err){
		printk(SPI_DECT_DEVICE_NAME" Could not create a sysdev file to export pins state : error %d", err);
		spi_dect_sysdev_exit();
		return err;
	}

	return 0;
}



static int __init spi_dect_init(void){
	int ret;
	printk("Init "SPI_DECT_DEVICE_NAME"\n");
	memset(&spi_dect_ex, 0x0, sizeof(struct spi_dect_export_sysfs));
	mutex_init(&spi_dect_ex.mutex);
	ret = spi_dect_sysdev_init();
	if(ret)
		return ret;
	ret = register_chrdev(SPI_DECT_MAJOR, SPI_DECT_DEVICE_NAME, &spi_dect_fops);
	if(ret < 0){
		printk("Could not register char device for spi dect\n");
		return ret;
	}
	ret = spi_register_driver(&spi_dect_driver);
	if(ret < 0){
		printk("Could not register "SPI_DECT_DEVICE_NAME" driver\n");
		unregister_chrdev(SPI_DECT_MAJOR, SPI_DECT_DEVICE_NAME);
		return ret;
	}
	mutex_init(&spi_dect_dev_list_lock);
	return 0;
}

static void __exit spi_dect_exit(void){
	printk("Uninit "SPI_DECT_DEVICE_NAME"\n");
	spi_dect_sysdev_exit();
	spi_unregister_driver(&spi_dect_driver);
	unregister_chrdev(SPI_DECT_MAJOR, SPI_DECT_DEVICE_NAME);
}

subsys_initcall(spi_dect_init);
module_exit(spi_dect_exit);

MODULE_DESCRIPTION("DET SPI Master Controller driver");
MODULE_AUTHOR("Hammane Farid, Sagemcom");
MODULE_LICENSE("GPL");
