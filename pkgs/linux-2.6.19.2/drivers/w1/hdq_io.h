/*
 *	hdq_io.h
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

#ifndef __HDQ_IO_H
#define __HDQ_IO_H

#include "hdq.h"

void hdq_delay(unsigned long);
u8 hdq_touch_bit(struct hdq_master *, int);
u8 hdq_triplet(struct hdq_master *dev, int bdir);
void hdq_write_8(struct hdq_master *, u8);
u8 hdq_read_8(struct hdq_master *);
int hdq_reset_bus(struct hdq_master *);
u8 hdq_calc_crc8(u8 *, int);
void hdq_write_block(struct hdq_master *, const u8 *, int);
u8 hdq_read_block(struct hdq_master *, u8 *, int);
void hdq_search_devices(struct hdq_master *dev, hdq_slave_found_callback cb);
int hdq_reset_select_slave(struct hdq_slave *sl);
void hdq_bq27000_send_read(struct hdq_master *, int );
void hdq_bq27000_send_break(struct hdq_master *);

#endif /* __HDQ_IO_H */
