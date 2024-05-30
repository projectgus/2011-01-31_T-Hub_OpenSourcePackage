/*
 *	hdq.c
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

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/suspend.h>

#include <asm/atomic.h>

#include "hdq.h"
#include "hdq_io.h"
#include "hdq_log.h"
#include "hdq_int.h"
#include "hdq_family.h"
#include "hdq_netlink.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laurent Isenegger");
MODULE_DESCRIPTION("Driver for HDQ network protocol.");

static int hdq_timeout = 10000;
static int hdq_control_timeout = 1;
int hdq_max_slave_count = 1;
int hdq_max_slave_ttl = 10;

module_param_named(timeout, hdq_timeout, int, 0);
module_param_named(control_timeout, hdq_control_timeout, int, 0);
module_param_named(max_slave_count, hdq_max_slave_count, int, 0);
module_param_named(slave_ttl, hdq_max_slave_ttl, int, 0);

DEFINE_SPINLOCK(hdq_mlock);
LIST_HEAD(hdq_masters);

static pid_t control_thread;
static int control_needs_exit;
static DECLARE_COMPLETION(hdq_control_complete);

static int hdq_master_match(struct device *dev, struct device_driver *drv)
{
	return 1;
}

static int hdq_master_probe(struct device *dev)
{
	return -ENODEV;
}

static void hdq_master_release(struct device *dev)
{
	struct hdq_master *md = dev_to_hdq_master(dev);

	dev_dbg(dev, "%s: Releasing %s.\n", __func__, md->name);

	dev_fini_netlink(md);
	memset(md, 0, sizeof(struct hdq_master) + sizeof(struct hdq_bus_master));
	kfree(md);
}

static void hdq_slave_release(struct device *dev)
{
	struct hdq_slave *sl = dev_to_hdq_slave(dev);

	dev_dbg(dev, "%s: Releasing %s.\n", __func__, sl->name);

	while (atomic_read(&sl->refcnt)) {
		dev_dbg(dev, "Waiting for %s to become free: refcnt=%d.\n",
				sl->name, atomic_read(&sl->refcnt));
		if (msleep_interruptible(1000))
			flush_signals(current);
	}

	hdq_family_put(sl->family);
	sl->master->slave_count--;

	complete(&sl->released);
}

static ssize_t hdq_slave_read_name(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hdq_slave *sl = dev_to_hdq_slave(dev);

	return sprintf(buf, "%s\n", sl->name);
}

static ssize_t hdq_slave_read_id(struct kobject *kobj, char *buf, loff_t off, size_t count)
{
	struct hdq_slave *sl = kobj_to_hdq_slave(kobj);

	atomic_inc(&sl->refcnt);
	if (off > 8) {
		count = 0;
	} else {
		if (off + count > 8)
			count = 8 - off;

	}
	atomic_dec(&sl->refcnt);

	return count;
}

static struct device_attribute hdq_slave_attr_name =
	__ATTR(name, S_IRUGO, hdq_slave_read_name, NULL);

static struct bin_attribute hdq_slave_attr_bin_id = {
      .attr = {
              .name = "id",
              .mode = S_IRUGO,
              .owner = THIS_MODULE,
      },
      .size = 8,
      .read = hdq_slave_read_id,
};

static int hdq_hotplug(struct device *dev, char **envp, int num_envp, char *buffer, int buffer_size);

static struct bus_type hdq_bus_type = {
	.name = "hdq",
	.match = hdq_master_match,
	.hotplug = hdq_hotplug,
};

struct device_driver hdq_master_driver = {
	.name = "hdq_master_driver",
	.bus = &hdq_bus_type,
	.probe = hdq_master_probe,
};

struct device hdq_master_device = {
	.parent = NULL,
	.bus = &hdq_bus_type,
	.bus_id = "hdq bus master",
	.driver = &hdq_master_driver,
	.release = &hdq_master_release
};

struct device_driver hdq_slave_driver = {
	.name = "hdq_slave_driver",
	.bus = &hdq_bus_type,
};

struct device hdq_slave_device = {
	.parent = NULL,
	.bus = &hdq_bus_type,
	.bus_id = "hdq bus slave",
	.driver = &hdq_slave_driver,
	.release = &hdq_slave_release
};

static ssize_t hdq_master_attribute_show_name(struct device *dev, char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%s\n", md->name);

	up(&md->mutex);

	return count;
}

static ssize_t hdq_master_attribute_store_search(struct device * dev,
						struct device_attribute *attr,
						const char * buf, size_t count)
{
	struct hdq_master *md = dev_to_hdq_master(dev);

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	md->search_count = simple_strtol(buf, NULL, 0);

	up(&md->mutex);

	return count;
}


static ssize_t hdq_master_attribute_show_search(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%d\n", md->search_count);

	up(&md->mutex);

	return count;
}

static ssize_t hdq_master_attribute_show_pointer(struct device *dev, char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible(&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "0x%p\n", md->bus_master);

	up(&md->mutex);
	return count;
}

static ssize_t hdq_master_attribute_show_timeout(struct device *dev, char *buf)
{
  struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible(&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%d\n", hdq_timeout);
	
	up(&md->mutex);
	return count;
}

static ssize_t hdq_master_attribute_show_max_slave_count(struct device *dev, char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible(&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%d\n", md->max_slave_count);

	up(&md->mutex);
	return count;
}

static ssize_t hdq_master_attribute_show_attempts(struct device *dev, char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible(&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%lu\n", md->attempts);

	up(&md->mutex);
	return count;
}

static ssize_t hdq_master_attribute_show_slave_count(struct device *dev, char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible(&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%d\n", md->slave_count);

	up(&md->mutex);
	return count;
}

static ssize_t hdq_master_attribute_show_slaves(struct device *dev, char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	int c = PAGE_SIZE;

	if (down_interruptible(&md->mutex))
		return -EBUSY;

	if (md->slave_count == 0)
	  {
	    c -= snprintf(buf + PAGE_SIZE - c, c, "not found.\n");
	  }
	else {
		struct list_head *ent, *n;
		struct hdq_slave *sl;

		list_for_each_safe(ent, n, &md->slist) {
			sl = list_entry(ent, struct hdq_slave, hdq_slave_entry);

			c -= snprintf(buf + PAGE_SIZE - c, c, "%s\n", sl->name);
		}
	}

	up(&md->mutex);

	return PAGE_SIZE - c;
}


static ssize_t hdq_master_attribute_store_write_0(struct device * dev,
						struct device_attribute *attr,
						const char * buf, size_t count)
{
	struct hdq_master *md = dev_to_hdq_master(dev);

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	md->write_0 = simple_strtol(buf, NULL, 0);

	up(&md->mutex);

	return count;
}


static ssize_t hdq_master_attribute_show_write_0(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%d\n", md->write_0);

	up(&md->mutex);

	return count;
}


static ssize_t hdq_master_attribute_store_write_1(struct device * dev,
						struct device_attribute *attr,
						const char * buf, size_t count)
{
	struct hdq_master *md = dev_to_hdq_master(dev);

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	md->write_1 = simple_strtol(buf, NULL, 0);

	up(&md->mutex);

	return count;
}


static ssize_t hdq_master_attribute_show_write_1(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%d\n", md->write_1);

	up(&md->mutex);

	return count;
}


static ssize_t hdq_master_attribute_store_cycle_period(struct device * dev,
						struct device_attribute *attr,
						const char * buf, size_t count)
{
	struct hdq_master *md = dev_to_hdq_master(dev);

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	md->cycle_period = simple_strtol(buf, NULL, 0);

	up(&md->mutex);

	return count;
}


static ssize_t hdq_master_attribute_show_cycle_period(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct hdq_master *md = dev_to_hdq_master(dev);
	ssize_t count;

	if (down_interruptible (&md->mutex))
		return -EBUSY;

	count = sprintf(buf, "%d\n", md->cycle_period);

	up(&md->mutex);

	return count;
}


#define HDQ_MASTER_ATTR_RO(_name, _mode)				\
	struct device_attribute hdq_master_attribute_##_name =	\
		__ATTR(hdq_master_##_name, _mode,		\
		       hdq_master_attribute_show_##_name, NULL)

#define HDQ_MASTER_ATTR_RW(_name, _mode)				\
	struct device_attribute hdq_master_attribute_##_name =	\
		__ATTR(hdq_master_##_name, _mode,		\
		       hdq_master_attribute_show_##_name,	\
		       hdq_master_attribute_store_##_name)

static HDQ_MASTER_ATTR_RO(name, S_IRUGO);
static HDQ_MASTER_ATTR_RO(slaves, S_IRUGO);
static HDQ_MASTER_ATTR_RO(slave_count, S_IRUGO);
static HDQ_MASTER_ATTR_RO(max_slave_count, S_IRUGO);
static HDQ_MASTER_ATTR_RO(attempts, S_IRUGO);
static HDQ_MASTER_ATTR_RO(timeout, S_IRUGO);
static HDQ_MASTER_ATTR_RO(pointer, S_IRUGO);
static HDQ_MASTER_ATTR_RW(search, S_IRUGO | S_IWUGO);
static HDQ_MASTER_ATTR_RW(write_0, S_IRUGO | S_IWUGO);
static HDQ_MASTER_ATTR_RW(write_1, S_IRUGO | S_IWUGO);
static HDQ_MASTER_ATTR_RW(cycle_period, S_IRUGO | S_IWUGO);

static struct attribute *hdq_master_default_attrs[] = {
	&hdq_master_attribute_name.attr,
	&hdq_master_attribute_slaves.attr,
	&hdq_master_attribute_slave_count.attr,
	&hdq_master_attribute_max_slave_count.attr,
	&hdq_master_attribute_attempts.attr,
	&hdq_master_attribute_timeout.attr,
	&hdq_master_attribute_pointer.attr,
	&hdq_master_attribute_search.attr,
	&hdq_master_attribute_write_0.attr,
	&hdq_master_attribute_write_1.attr,
	&hdq_master_attribute_cycle_period.attr,

	NULL
};

static struct attribute_group hdq_master_defattr_group = {
	.attrs = hdq_master_default_attrs,
};

int hdq_create_master_attributes(struct hdq_master *master)
{
	return sysfs_create_group(&master->dev.kobj, &hdq_master_defattr_group);
}

void hdq_destroy_master_attributes(struct hdq_master *master)
{
	sysfs_remove_group(&master->dev.kobj, &hdq_master_defattr_group);
}


static int hdq_hotplug(struct device *dev, char **envp, int num_envp, char *buffer, int buffer_size)
{
	return 0;
}

static int __hdq_attach_slave_device(struct hdq_slave *sl)
{
	int err;
	sl->dev.parent = &sl->master->dev;
	sl->dev.driver = &hdq_slave_driver;
	sl->dev.bus = &hdq_bus_type;
	sl->dev.release = &hdq_slave_release;

	snprintf(&sl->dev.bus_id[0], sizeof(sl->dev.bus_id),
		 "BQ27000_Battery");
	snprintf(&sl->name[0], sizeof(sl->name),
		 "BQ27000_Battery");

	dev_dbg(&sl->dev, "%s: registering %s as %p.\n", __func__, &sl->dev.bus_id[0]);

	err = device_register(&sl->dev);
	if (err < 0) {
		dev_err(&sl->dev,
			"Device registration [%s] failed. err=%d\n",
			sl->dev.bus_id, err);
		return err;
	}
	/* Create "name" entry */
	err = device_create_file(&sl->dev, &hdq_slave_attr_name);
	if (err < 0) {
		dev_err(&sl->dev,
			"sysfs file creation for [%s] failed. err=%d\n",
			sl->dev.bus_id, err);
		goto out_unreg;
	}
	/* Create "id" entry */
	err = sysfs_create_bin_file(&sl->dev.kobj, &hdq_slave_attr_bin_id);
	if (err < 0) {
		dev_err(&sl->dev,
			"sysfs file creation for [%s] failed. err=%d\n",
			sl->dev.bus_id, err);
		goto out_rem1;
	}

	/* if the family driver needs to initialize something... */
	if (sl->family->fops && sl->family->fops->add_slave)
	  {
	    if ((err = sl->family->fops->add_slave(sl)) < 0) 
	      {
		dev_err(&sl->dev,
			"sysfs file creation for [%s] failed. err=%d\n",
		  sl->dev.bus_id, err);
		goto out_rem2;
	      }
	  }

	mdelay(1000);
	list_add_tail(&sl->hdq_slave_entry, &sl->master->slist);

	return 0;

out_rem2:
	sysfs_remove_bin_file(&sl->dev.kobj, &hdq_slave_attr_bin_id);
out_rem1:
	device_remove_file(&sl->dev, &hdq_slave_attr_name);
out_unreg:
	device_unregister(&sl->dev);
	return err;
}

static int hdq_attach_slave_device(struct hdq_master *dev)
{
	struct hdq_slave *sl;
	struct hdq_family *f;
	int err;
	struct hdq_netlink_msg msg;


	sl = kmalloc(sizeof(struct hdq_slave), GFP_KERNEL);
	if (!sl) {
		dev_err(&dev->dev,
			 "%s: failed to allocate new slave device.\n",
			 __func__);
		return -ENOMEM;
	}

	memset(sl, 0, sizeof(*sl));

	sl->owner = THIS_MODULE;
	sl->master = dev;
	set_bit(HDQ_SLAVE_ACTIVE, (long *)&sl->flags);


	atomic_set(&sl->refcnt, 0);
	init_completion(&sl->released);

	spin_lock(&hdq_flock);

	f = hdq_family_registered(HDQ_BATTERY_BQ27000);

	__hdq_family_get(f);
	spin_unlock(&hdq_flock);

	sl->family = f;

	err = __hdq_attach_slave_device(sl);
	if (err < 0) {
		dev_err(&dev->dev, "%s: Attaching %s failed.\n", __func__,
			 sl->name);
		hdq_family_put(sl->family);
		kfree(sl);
		return err;
	}

	sl->ttl = dev->slave_ttl;
	dev->slave_count++;

	msg.type = HDQ_SLAVE_ADD;
	hdq_netlink_send(dev, &msg);

	return 0;
}

static void hdq_slave_detach(struct hdq_slave *sl)
{
	struct hdq_netlink_msg msg;

	dev_dbg(&sl->dev, "%s: detaching %s [%p].\n", __func__, sl->name, sl);

	list_del(&sl->hdq_slave_entry);

	if (sl->family->fops && sl->family->fops->remove_slave)
		sl->family->fops->remove_slave(sl);

/* 	memcpy(&msg.id.id, &sl->reg_num, sizeof(msg.id.id)); */
	msg.type = HDQ_SLAVE_REMOVE;
	hdq_netlink_send(sl->master, &msg);

	sysfs_remove_bin_file(&sl->dev.kobj, &hdq_slave_attr_bin_id);
	device_remove_file(&sl->dev, &hdq_slave_attr_name);
	device_unregister(&sl->dev);

	wait_for_completion(&sl->released);
	kfree(sl);
}

static struct hdq_master *hdq_search_master(unsigned long data)
{
	struct hdq_master *dev;
	int found = 0;

	spin_lock_bh(&hdq_mlock);
	list_for_each_entry(dev, &hdq_masters, hdq_master_entry) {
		if (dev->bus_master->data == data) {
			found = 1;
			atomic_inc(&dev->refcnt);
			break;
		}
	}
	spin_unlock_bh(&hdq_mlock);

	return (found)?dev:NULL;
}

void hdq_reconnect_slaves(struct hdq_family *f)
{
	struct hdq_master *dev;
	spin_lock_bh(&hdq_mlock);
	list_for_each_entry(dev, &hdq_masters, hdq_master_entry) {
		dev_dbg(&dev->dev, "Reconnecting slaves in %s into new family %02x.\n",
				dev->name, f->fid);
		set_bit(HDQ_MASTER_NEED_RECONNECT, &dev->flags);
	}
	spin_unlock_bh(&hdq_mlock);
}

static void hdq_slave_found(unsigned long data)
{

	int indicator;
	struct hdq_slave *sl;
	struct list_head *ent;
	struct hdq_master *dev;
/* 	u64 rn_le = cpu_to_le64(rn); */

	dev = hdq_search_master(data);
	if (!dev) {
		printk(KERN_ERR "Failed to find hdq master device for data %08lx, it is impossible.\n",
				data);
		return;
	}

/* 	tmp = (struct hdq_reg_num *) &rn; */
	indicator = 0;
	list_for_each(ent, &dev->slist)
	  {	    
	    sl = list_entry(ent, struct hdq_slave, hdq_slave_entry);	    
	    if (sl->family->fid == HDQ_BATTERY_BQ27000)
	      {
		set_bit(HDQ_SLAVE_ACTIVE, (long *)&sl->flags);
		indicator=1;   
	      }
	  }
	if (indicator == 0)
	  {
	    hdq_attach_slave_device(dev);
	}
	
	atomic_dec(&dev->refcnt);
}

/**
 * Performs a ROM Search & registers any devices found.
 * The 1-wire search is a simple binary tree search.
 * For each bit of the address, we read two bits and write one bit.
 * The bit written will put to sleep all devies that don't match that bit.
 * When the two reads differ, the direction choice is obvious.
 * When both bits are 0, we must choose a path to take.
 * When we can scan all 64 bits without having to choose a path, we are done.
 *
 * See "Application note 187 1-wire search algorithm" at www.maxim-ic.com
 *
 * @dev        The master device to search
 * @cb         Function to call when a device is found
 */
void hdq_search(struct hdq_master *dev, hdq_slave_found_callback cb)
{
	u64 last_rn, rn;
	int slave_count = 0;
	int last_zero, last_device;
	int search_bit, desc_bit;

	search_bit = 0;
	rn = last_rn = 0;
	last_device = 0;
	last_zero = -1;

	desc_bit = 64;

	while ( !last_device && (slave_count++ < dev->max_slave_count) ) {
		last_rn = rn;
		rn = 0;

		cb(dev->bus_master->data);
		
	}
}

static int hdq_control(void *data)
{
	struct hdq_slave *sl, *sln;
	struct hdq_master *dev, *n;
	int err, have_to_wait = 0;

	daemonize("hdq_control");
	allow_signal(SIGTERM);

	while (!control_needs_exit || have_to_wait) {
		have_to_wait = 0;
		
		if (current->flags & PF_FREEZE)
			refrigerator(PF_FREEZE);
		msleep_interruptible(hdq_control_timeout * 1000);


		if (signal_pending(current))
			flush_signals(current);

		list_for_each_entry_safe(dev, n, &hdq_masters, hdq_master_entry) {
			if (!control_needs_exit && !dev->flags)
				continue;

			/*
			 * Little race: we can create thread but not set the flag.
			 * Get a chance for external process to set flag up.
			 */
			if (!dev->initialized) {
				have_to_wait = 1;
				continue;
			}

			if (control_needs_exit) {
				set_bit(HDQ_MASTER_NEED_EXIT, &dev->flags);

				err = kill_proc(dev->kpid, SIGTERM, 1);
				if (err)
					dev_err(&dev->dev,
						 "Failed to send signal to hdq kernel thread %d.\n",
						 dev->kpid);
			}

			if (test_bit(HDQ_MASTER_NEED_EXIT, &dev->flags)) {
				wait_for_completion(&dev->dev_exited);
				spin_lock_bh(&hdq_mlock);
				list_del(&dev->hdq_master_entry);
				spin_unlock_bh(&hdq_mlock);

				down(&dev->mutex);
				list_for_each_entry_safe(sl, sln, &dev->slist, hdq_slave_entry) {
					hdq_slave_detach(sl);
				}
				hdq_destroy_master_attributes(dev);
				up(&dev->mutex);
				atomic_dec(&dev->refcnt);
				continue;
			}

			if (test_bit(HDQ_MASTER_NEED_RECONNECT, &dev->flags)) {
			  
			  dev_dbg(&dev->dev, "Reconnecting slaves in device %s.\n", dev->name);
			  down(&dev->mutex);
			  list_for_each_entry_safe(sl, sln, &dev->slist, hdq_slave_entry) {
			    hdq_slave_detach(sl);
			    hdq_attach_slave_device(dev);
			  }
			  dev_dbg(&dev->dev, "Reconnecting slaves in device %s has been finished.\n", dev->name);
			  clear_bit(HDQ_MASTER_NEED_RECONNECT, &dev->flags);
			  up(&dev->mutex);
			}
		}
	}

	complete_and_exit(&hdq_control_complete, 0);
}

int hdq_process(void *data)
{
	struct hdq_master *dev = (struct hdq_master *) data;
	struct hdq_slave *sl, *sln;

	daemonize("%s", dev->name);
	allow_signal(SIGTERM);

	while (!test_bit(HDQ_MASTER_NEED_EXIT, &dev->flags)) {
		if (current->flags & PF_FREEZE)
			refrigerator(PF_FREEZE);
		msleep_interruptible(hdq_timeout);

		if (signal_pending(current))
			flush_signals(current);

		if (test_bit(HDQ_MASTER_NEED_EXIT, &dev->flags))
			break;

		if (!dev->initialized)
			continue;

		if (dev->search_count == 0)
			continue;

		if (down_interruptible(&dev->mutex))
			continue;

		list_for_each_entry(sl, &dev->slist, hdq_slave_entry)
			clear_bit(HDQ_SLAVE_ACTIVE, (long *)&sl->flags);

		hdq_search_devices(dev, hdq_slave_found);

		list_for_each_entry_safe(sl, sln, &dev->slist, hdq_slave_entry) {
			if (!test_bit(HDQ_SLAVE_ACTIVE, (unsigned long *)&sl->flags) && !--sl->ttl) {
				hdq_slave_detach(sl);

				dev->slave_count--;
			} else if (test_bit(HDQ_SLAVE_ACTIVE, (unsigned long *)&sl->flags))
				sl->ttl = dev->slave_ttl;
		}

		if (dev->search_count > 0)
			dev->search_count--;

		up(&dev->mutex);
	}

	atomic_dec(&dev->refcnt);
	complete_and_exit(&dev->dev_exited, 0);

	return 0;
}

static int hdq_init(void)
{
	int retval;
	printk(KERN_INFO "Driver for HDQ network protocol.\n");

	retval = bus_register(&hdq_bus_type);
	if (retval) {
	  printk(KERN_INFO "Failed to register bus. err=%d.\n",retval);
		goto err_out_exit_init;
	}

	retval = driver_register(&hdq_master_driver);
	if (retval) {
		printk(KERN_INFO
		       "Failed to register master driver. err=%d.\n",retval);
		goto err_out_bus_unregister;
	}

	retval = driver_register(&hdq_slave_driver);
	if (retval) {
		printk(KERN_INFO
		       "Failed to register slave driver. err=%d.\n",retval);
		goto err_out_master_unregister;
	}

	control_thread = kernel_thread(&hdq_control, NULL, 0);
	if (control_thread < 0) {
	  printk(KERN_INFO "Failed to create control thread. err=%d\n",control_thread);
		retval = control_thread;
		goto err_out_slave_unregister;
	}

	return 0;

err_out_slave_unregister:
	driver_unregister(&hdq_slave_driver);

err_out_master_unregister:
	driver_unregister(&hdq_master_driver);

err_out_bus_unregister:
	bus_unregister(&hdq_bus_type);

err_out_exit_init:
	return retval;
}

static void hdq_fini(void)
{
	struct hdq_master *dev;

	list_for_each_entry(dev, &hdq_masters, hdq_master_entry)
		__hdq_remove_master_device(dev);

	control_needs_exit = 1;
	wait_for_completion(&hdq_control_complete);

	driver_unregister(&hdq_slave_driver);
	driver_unregister(&hdq_master_driver);
	bus_unregister(&hdq_bus_type);
}

module_init(hdq_init);
module_exit(hdq_fini);
