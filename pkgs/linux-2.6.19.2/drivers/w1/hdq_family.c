/*
 *	hdq_family.c
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

#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/delay.h>

#include "hdq_family.h"

DEFINE_SPINLOCK(hdq_flock);
static LIST_HEAD(hdq_families);
extern void hdq_reconnect_slaves(struct hdq_family *f);

int hdq_register_family(struct hdq_family *newf)
{
	struct list_head *ent, *n;
	struct hdq_family *f;
	int ret = 0;

	spin_lock(&hdq_flock);
	list_for_each_safe(ent, n, &hdq_families) {
		f = list_entry(ent, struct hdq_family, family_entry);

		if (f->fid == newf->fid) {
			ret = -EEXIST;
			break;
		}
	}

	if (!ret) {
		atomic_set(&newf->refcnt, 0);
		newf->need_exit = 0;
		list_add_tail(&newf->family_entry, &hdq_families);
	}
	spin_unlock(&hdq_flock);
	printk("in register_family before reconnect_slave\n");
	hdq_reconnect_slaves(newf);

	return ret;
}

void hdq_unregister_family(struct hdq_family *fent)
{
	struct list_head *ent, *n;
	struct hdq_family *f;

	spin_lock(&hdq_flock);
	list_for_each_safe(ent, n, &hdq_families) {
		f = list_entry(ent, struct hdq_family, family_entry);

		if (f->fid == fent->fid) {
			list_del(&fent->family_entry);
			break;
		}
	}

	fent->need_exit = 1;

	spin_unlock(&hdq_flock);

	while (atomic_read(&fent->refcnt)) {
		printk(KERN_INFO "Waiting for family %u to become free: refcnt=%d.\n",
				fent->fid, atomic_read(&fent->refcnt));

		if (msleep_interruptible(1000))
			flush_signals(current);
	}
}

/*
 * Should be called under hdq_flock held.
 */
struct hdq_family * hdq_family_registered(u8 fid)
{
	struct list_head *ent, *n;
	struct hdq_family *f = NULL;
	int ret = 0;

	list_for_each_safe(ent, n, &hdq_families) {
		f = list_entry(ent, struct hdq_family, family_entry);

		if (f->fid == fid) {
			ret = 1;
			break;
		}
	}

	return (ret) ? f : NULL;
}

void hdq_family_put(struct hdq_family *f)
{
	spin_lock(&hdq_flock);
	__hdq_family_put(f);
	spin_unlock(&hdq_flock);
}

void __hdq_family_put(struct hdq_family *f)
{
	if (atomic_dec_and_test(&f->refcnt))
		f->need_exit = 1;
}

void hdq_family_get(struct hdq_family *f)
{
	spin_lock(&hdq_flock);
	__hdq_family_get(f);
	spin_unlock(&hdq_flock);

}

void __hdq_family_get(struct hdq_family *f)
{
	smp_mb__before_atomic_inc();
	atomic_inc(&f->refcnt);
	smp_mb__after_atomic_inc();
}

EXPORT_SYMBOL(hdq_family_get);
EXPORT_SYMBOL(hdq_family_put);
EXPORT_SYMBOL(hdq_family_registered);
EXPORT_SYMBOL(hdq_unregister_family);
EXPORT_SYMBOL(hdq_register_family);
