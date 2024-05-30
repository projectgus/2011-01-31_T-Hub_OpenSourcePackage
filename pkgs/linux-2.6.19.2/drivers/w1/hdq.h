/*
 *	hdq.h
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

#ifndef __HDQ_H
#define __HDQ_H

#ifdef __KERNEL__

#include <linux/completion.h>
#include <linux/device.h>

#include <net/sock.h>

#include <asm/semaphore.h>

#include "hdq_family.h"

#define HDQ_MAXNAMELEN		32
#define HDQ_SLAVE_DATA_SIZE	128

#define HDQ_SEARCH		0xF0
#define HDQ_CONDITIONAL_SEARCH	0xEC
#define HDQ_CONVERT_TEMP		0x44
#define HDQ_SKIP_ROM		0xCC
#define HDQ_READ_SCRATCHPAD	0xBE
#define HDQ_READ_ROM		0x33
#define HDQ_READ_PSUPPLY		0xB4
#define HDQ_MATCH_ROM		0x55

#define HDQ_SLAVE_ACTIVE		(1<<0)

struct hdq_slave
{
	struct module		*owner;
	unsigned char		name[HDQ_MAXNAMELEN];
	struct list_head	hdq_slave_entry;
	atomic_t		refcnt;
	u8			rom[9];
	u32			flags;
	int			ttl;

	struct hdq_master	*master;
	struct hdq_family	*family;
	void			*family_data;
	struct device		dev;
	struct completion	released;
};

typedef void (* hdq_slave_found_callback)(unsigned long);


/**
 * Note: read_bit and write_bit are very low level functions and should only
 * be used with hardware that doesn't really support 1-wire operations,
 * like a parallel/serial port.
 * Either define read_bit and write_bit OR define, at minimum, touch_bit and
 * reset_bus.
 */
struct hdq_bus_master
{
	/** the first parameter in all the functions below */
	unsigned long	data;

	/**
	 * Sample the line level
	 * @return the level read (0 or 1)
	 */
	u8		(*read_bit)(unsigned long);

	/** Sets the line level */
	void		(*write_bit)(unsigned long, u8);

	/**
	 * touch_bit is the lowest-level function for devices that really
	 * support the 1-wire protocol.
	 * touch_bit(0) = write-0 cycle
	 * touch_bit(1) = write-1 / read cycle
	 * @return the bit read (0 or 1)
	 */
	u8		(*touch_bit)(unsigned long, u8);

	/**
	 * Reads a bytes. Same as 8 touch_bit(1) calls.
	 * @return the byte read
	 */
	u8		(*read_byte)(unsigned long);

	/**
	 * Writes a byte. Same as 8 touch_bit(x) calls.
	 */
	void		(*write_byte)(unsigned long, u8);

	/**
	 * Same as a series of read_byte() calls
	 * @return the number of bytes read
	 */
	u8		(*read_block)(unsigned long, u8 *, int);

	/** Same as a series of write_byte() calls */
	void		(*write_block)(unsigned long, const u8 *, int);

	/**
	 * Combines two reads and a smart write for ROM searches
	 * @return bit0=Id bit1=comp_id bit2=dir_taken
	 */
	u8		(*triplet)(unsigned long, u8);

	/**
	 * long write-0 with a read for the presence pulse detection
	 * @return -1=Error, 0=Device present, 1=No device present
	 */
	u8		(*reset_bus)(unsigned long);

	/** Really nice hardware can handles the ROM searches */
	void		(*search)(unsigned long, hdq_slave_found_callback);
};

#define HDQ_MASTER_NEED_EXIT		0
#define HDQ_MASTER_NEED_RECONNECT	1

struct hdq_master
{
	struct list_head	hdq_master_entry;
	struct module		*owner;
	unsigned char		name[HDQ_MAXNAMELEN];
	struct list_head	slist;
	int			max_slave_count, slave_count;
	unsigned long		attempts;
	int			slave_ttl;
	int			initialized;
	u32			id;
	int			search_count;
        unsigned long           hdq_timeout;
	atomic_t		refcnt;
        int                     write_0;
        int                     write_1;
        int                     cycle_period;
	void			*priv;
	int			priv_size;

	long			flags;

	pid_t			kpid;
	struct semaphore	mutex;

	struct device_driver	*driver;
	struct device		dev;
	struct completion	dev_exited;

	struct hdq_bus_master	*bus_master;

	u32			seq, groups;
	struct sock		*nls;
};

int hdq_create_master_attributes(struct hdq_master *);
void hdq_search(struct hdq_master *dev, hdq_slave_found_callback cb);

static inline struct hdq_slave* dev_to_hdq_slave(struct device *dev)
{
	return container_of(dev, struct hdq_slave, dev);
}

static inline struct hdq_slave* kobj_to_hdq_slave(struct kobject *kobj)
{
	return dev_to_hdq_slave(container_of(kobj, struct device, kobj));
}

static inline struct hdq_master* dev_to_hdq_master(struct device *dev)
{
	return container_of(dev, struct hdq_master, dev);
}

static inline struct hdq_master* kobj_to_hdq_master(struct kobject *kobj)
{
	return dev_to_hdq_master(container_of(kobj, struct device, kobj));
}

#endif /* __KERNEL__ */

#endif /* __HDQ_H */
