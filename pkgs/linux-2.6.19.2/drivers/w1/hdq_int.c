/*
 *	hdq_int.c
 *
 * Copyright (c) 2004 Evgeniy Polyakov <johnpol@2ka.mipt.ru>
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/delay.h>

#include "hdq.h"
#include "hdq_log.h"
#include "hdq_netlink.h"

static u32 hdq_ids = 1;

extern struct device_driver hdq_master_driver;
extern struct bus_type hdq_bus_type;
extern struct device hdq_master_device;
extern int hdq_max_slave_count;
extern int hdq_max_slave_ttl;
extern struct list_head hdq_masters;
extern spinlock_t hdq_mlock;

extern int hdq_process(void *);

static struct hdq_master * hdq_alloc_dev(u32 id, int slave_count, int slave_ttl,
				       struct device_driver *driver,
				       struct device *device)
{
	struct hdq_master *dev;
	int err;

	/*
	 * We are in process context(kernel thread), so can sleep.
	 */
	dev = kmalloc(sizeof(struct hdq_master) + sizeof(struct hdq_bus_master), GFP_KERNEL);
	if (!dev) {
		printk(KERN_ERR
			"Failed to allocate %zd bytes for new hdq device.\n",
			sizeof(struct hdq_master));
		return NULL;
	}

	memset(dev, 0, sizeof(struct hdq_master) + sizeof(struct hdq_bus_master));

	dev->bus_master = (struct hdq_bus_master *)(dev + 1);

	dev->owner		= THIS_MODULE;
	dev->max_slave_count	= slave_count;
	dev->slave_count	= 0;
	dev->attempts		= 0;
	dev->kpid		= -1;
	dev->initialized	= 0;
	dev->id			= id;
	dev->slave_ttl		= slave_ttl;
        dev->search_count	= -1; /* continual scan */
	dev->cycle_period       = 117;
	dev->write_0            = 100;
	dev->write_1            = 6;
	atomic_set(&dev->refcnt, 2);

	INIT_LIST_HEAD(&dev->slist);
	init_MUTEX(&dev->mutex);

	init_completion(&dev->dev_exited);

	memcpy(&dev->dev, device, sizeof(struct device));
	snprintf(dev->dev.bus_id, sizeof(dev->dev.bus_id),
		  "hdq_bus_master%u", dev->id);
	snprintf(dev->name, sizeof(dev->name), "hdq_bus_master%u", dev->id);

	dev->driver = driver;

	dev->groups = 1;
	dev->seq = 1;
	dev_init_netlink(dev);

	err = device_register(&dev->dev);
	if (err) {
		printk(KERN_ERR "Failed to register master device. err=%d\n", err);

		dev_fini_netlink(dev);

		memset(dev, 0, sizeof(struct hdq_master));
		kfree(dev);
		dev = NULL;
	}

	return dev;
}

void hdq_free_dev(struct hdq_master *dev)
{
	device_unregister(&dev->dev);
}

int hdq_add_master_device(struct hdq_bus_master *master)
{
	struct hdq_master *dev;
	int retval = 0;
	struct hdq_netlink_msg msg;

        /* validate minimum functionality */
        if (!(master->touch_bit && master->reset_bus) &&
            !(master->write_bit && master->read_bit)) {
		printk(KERN_ERR "hdq_add_master_device: invalid function set\n");
		return(-EINVAL);
        }

	dev = hdq_alloc_dev(hdq_ids++, hdq_max_slave_count, hdq_max_slave_ttl, &hdq_master_driver, &hdq_master_device);
	if (!dev)
		return -ENOMEM;

	dev->kpid = kernel_thread(&hdq_process, dev, 0);
	if (dev->kpid < 0) {
		dev_err(&dev->dev,
			 "Failed to create new kernel thread. err=%d\n",
			 dev->kpid);
		retval = dev->kpid;
		goto err_out_free_dev;
	}

	retval =  hdq_create_master_attributes(dev);
	if (retval)
		goto err_out_kill_thread;

	memcpy(dev->bus_master, master, sizeof(struct hdq_bus_master));

	dev->initialized = 1;


	spin_lock(&hdq_mlock);
	list_add(&dev->hdq_master_entry, &hdq_masters);
	spin_unlock(&hdq_mlock);

	msg.id.mst.id = dev->id;
	msg.id.mst.pid = dev->kpid;
	msg.type = HDQ_MASTER_ADD;
	hdq_netlink_send(dev, &msg);

	return 0;

err_out_kill_thread:
	set_bit(HDQ_MASTER_NEED_EXIT, &dev->flags);
	if (kill_proc(dev->kpid, SIGTERM, 1))
		dev_err(&dev->dev,
			 "Failed to send signal to hdq kernel thread %d.\n",
			 dev->kpid);
	wait_for_completion(&dev->dev_exited);

err_out_free_dev:
	hdq_free_dev(dev);

	return retval;
}

void __hdq_remove_master_device(struct hdq_master *dev)
{
	int err;
	struct hdq_netlink_msg msg;

	set_bit(HDQ_MASTER_NEED_EXIT, &dev->flags);
	err = kill_proc(dev->kpid, SIGTERM, 1);
	if (err)
		dev_err(&dev->dev,
			 "%s: Failed to send signal to hdq kernel thread %d.\n",
			 __func__, dev->kpid);

	while (atomic_read(&dev->refcnt)) {
		dev_dbg(&dev->dev, "Waiting for %s to become free: refcnt=%d.\n",
				dev->name, atomic_read(&dev->refcnt));

		if (msleep_interruptible(1000))
			flush_signals(current);
	}

	msg.id.mst.id = dev->id;
	msg.id.mst.pid = dev->kpid;
	msg.type = HDQ_MASTER_REMOVE;
	hdq_netlink_send(dev, &msg);

	hdq_free_dev(dev);
}

void hdq_remove_master_device(struct hdq_bus_master *bm)
{
	struct hdq_master *dev = NULL;

	list_for_each_entry(dev, &hdq_masters, hdq_master_entry) {
		if (!dev->initialized)
			continue;

		if (dev->bus_master->data == bm->data)
			break;
	}

	if (!dev) {
		printk(KERN_ERR "Device doesn't exist.\n");
		return;
	}

	__hdq_remove_master_device(dev);
}

EXPORT_SYMBOL(hdq_add_master_device);
EXPORT_SYMBOL(hdq_remove_master_device);