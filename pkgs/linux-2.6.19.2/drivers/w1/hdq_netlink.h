/*
 * hdq_netlink.h
 *
 * Copyright (c) 2003 Evgeniy Polyakov <johnpol@2ka.mipt.ru>
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

#ifndef __HDQ_NETLINK_H
#define __HDQ_NETLINK_H

#include <asm/types.h>

#include "hdq.h"

enum hdq_netlink_message_types {
	HDQ_SLAVE_ADD = 0,
	HDQ_SLAVE_REMOVE,
	HDQ_MASTER_ADD,
	HDQ_MASTER_REMOVE,
};

struct hdq_netlink_msg
{
	__u8				type;
	__u8				reserved[3];
	union
	{
		__u64			hdq_id;
		struct
		{
			__u32		id;
			__u32		pid;
		} mst;
	} id;
};

#ifdef __KERNEL__

void hdq_netlink_send(struct hdq_master *, struct hdq_netlink_msg *);
int dev_init_netlink(struct hdq_master *dev);
void dev_fini_netlink(struct hdq_master *dev);

#endif /* __KERNEL__ */
#endif /* __HDQ_NETLINK_H */
