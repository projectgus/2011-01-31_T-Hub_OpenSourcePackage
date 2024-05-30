/*
 *	hdq_io.c
 *
 */

#include <asm/io.h>

#include <linux/delay.h>
#include <linux/moduleparam.h>

#include "hdq.h"
#include "hdq_log.h"
#include "hdq_io.h"

#define READ_FLAG 0
#define WRITE0_FLAG 1

int hdq_delay_parm = 1;
module_param_named(delay_coef, hdq_delay_parm, int, 0);

void hdq_delay(unsigned long tm)
{
	udelay(tm * hdq_delay_parm);
}

static void hdq_write_bit(struct hdq_master *dev, int bit);
static u8 hdq_read_bit(struct hdq_master *dev);

/**
 * Generates a write-0 or write-1 cycle and samples the level.
 */
u8 hdq_touch_bit(struct hdq_master *dev, int bit)
{
	if (dev->bus_master->touch_bit)
		return dev->bus_master->touch_bit(dev->bus_master->data, bit);
	else if (bit)
		return hdq_read_bit(dev);
	else {
		hdq_write_bit(dev, 0);
		return(0);
	}
}

/**
 * Generates a write-0 or write-1 cycle.
 * Only call if dev->bus_master->touch_bit is NULL
 */
static void hdq_write_bit(struct hdq_master *dev, int bit)
{
	if (bit) {
		dev->bus_master->write_bit(dev->bus_master->data, 0);
		hdq_delay(6);
		dev->bus_master->write_bit(dev->bus_master->data, 1);
		hdq_delay(64);
	} else {
		dev->bus_master->write_bit(dev->bus_master->data, 0);
		hdq_delay(60);
		dev->bus_master->write_bit(dev->bus_master->data, 1);
		hdq_delay(10);
	}
}

/**
 * Writes 8 bits.
 *
 * @param dev     the master device
 * @param byte    the byte to write
 */
void hdq_write_8(struct hdq_master *dev, u8 byte)
{
	int i;

	if (dev->bus_master->write_byte)
		dev->bus_master->write_byte(dev->bus_master->data, byte);
	else
		for (i = 0; i < 8; ++i)
			hdq_touch_bit(dev, (byte >> i) & 0x1);
}


/**
 * Generates a write-1 cycle and samples the level.
 * Only call if dev->bus_master->touch_bit is NULL
 */
static u8 hdq_read_bit(struct hdq_master *dev)
{
	int result;

	dev->bus_master->write_bit(dev->bus_master->data, 0);
	hdq_delay(6);
	dev->bus_master->write_bit(dev->bus_master->data, 1);
	hdq_delay(9);

	result = dev->bus_master->read_bit(dev->bus_master->data);
	hdq_delay(55);

	return result & 0x1;
}

/**
 * Does a triplet - used for searching ROM addresses.
 * Return bits:
 *  bit 0 = id_bit
 *  bit 1 = comp_bit
 *  bit 2 = dir_taken
 * If both bits 0 & 1 are set, the search should be restarted.
 *
 * @param dev     the master device
 * @param bdir    the bit to write if both id_bit and comp_bit are 0
 * @return        bit fields - see above
 */


/**
 * Reads 8 bits.
 *
 * @param dev     the master device
 * @return        the byte read
 */
u8 hdq_read_8(struct hdq_master * dev)
{
	int i;
	u8 res = 0, res_8 = 0;

	if (dev->bus_master->read_byte)
		res = dev->bus_master->read_byte(dev->bus_master->data);
	else
		for (i = 0; i < 8; ++i)
		  {
		    hdq_delay(55);
		    res = hdq_touch_bit(dev,1) & 0x1;
		    hdq_delay(55);
		    res_8 |= (res & 0x1) << i;
		  }

	return res_8;
}

/**
 * Writes a series of bytes.
 *
 * @param dev     the master device
 * @param buf     pointer to the data to write
 * @param len     the number of bytes to write
 * @return        the byte read
 */
void hdq_write_block(struct hdq_master *dev, const u8 *buf, int len)
{
	int i;

	if (dev->bus_master->write_block)
		dev->bus_master->write_block(dev->bus_master->data, buf, len);
	else
		for (i = 0; i < len; ++i)
			hdq_write_8(dev, buf[i]);
}

/**
 * Reads a series of bytes.
 *
 * @param dev     the master device
 * @param buf     pointer to the buffer to fill
 * @param len     the number of bytes to read
 * @return        the number of bytes read
 */
u8 hdq_read_block(struct hdq_master *dev, u8 *buf, int len)
{
	int i;
	u8 ret;

	if (dev->bus_master->read_block)
		ret = dev->bus_master->read_block(dev->bus_master->data, buf, len);
	else {
		for (i = 0; i < len; ++i)
			buf[i] = hdq_read_8(dev);
		ret = len;
	}

	return ret;
}



void hdq_search_devices(struct hdq_master *dev, hdq_slave_found_callback cb)
{
	dev->attempts++;
	if (dev->bus_master->search)
		dev->bus_master->search(dev->bus_master->data, cb);
	else
		hdq_search(dev, cb);
}

// Command used to send the read command: First are transmitted the adress bits of the register we want to read (MSB first and finally the last bit is 0 which corresponds to a read command 

void hdq_bq27000_send_read(struct hdq_master *dev, int bq27000_register)
{
  int i=0, bit_read=0;
  for (i = 0; i < 7; i++)
    {
      bit_read = bq27000_register>>i  & 0x01;
      (bit_read) ? hdq_touch_bit(dev,1) : hdq_touch_bit(dev,0);
    }
  hdq_touch_bit(dev,READ_FLAG);
}

// Command used in hdq protocol to do a reset of the slave, this command is used everytime a communication needs to be established with battery

void hdq_bq27000_send_break(struct hdq_master *dev)
{
  dev->bus_master->reset_bus(dev->bus_master->data);
}

EXPORT_SYMBOL(hdq_touch_bit);
EXPORT_SYMBOL(hdq_write_8);
EXPORT_SYMBOL(hdq_read_8);
EXPORT_SYMBOL(hdq_delay);
EXPORT_SYMBOL(hdq_read_block);
EXPORT_SYMBOL(hdq_write_block);
EXPORT_SYMBOL(hdq_search_devices);
EXPORT_SYMBOL(hdq_bq27000_send_read);
EXPORT_SYMBOL(hdq_bq27000_send_break);
