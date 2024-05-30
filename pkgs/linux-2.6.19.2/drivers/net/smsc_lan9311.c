/***************************************************************************
 *
 * Copyright (C) 2004-2007  SMSC
 * Copyright (C) 2005 ARM
 *
 * Modified by Sagemcom under GPL license on 28/04/2008 
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ***************************************************************************
 * Rewritten, heavily based on smsc911x simple driver by SMSC.
 * Partly uses io macros from smc91x.c by Nicolas Pitre
 *
 * Supported devices:
 *   LAN9115, LAN9116, LAN9117, LAN9118
 *   LAN9215, LAN9216, LAN9217, LAN9218
 *
 * History:
 *   05/05/2005 bahadir.balban@arm.com
 *     - Transition to linux coding style
 *     - Platform driver and module interface
 *
 *   17/07/2006 steve.glendinning@smsc.com
 *     - Added support for LAN921x family
 *     - Added workaround for multicast filters
 *
 *   31/07/2006 steve.glendinning@smsc.com
 *     - Removed tasklet, using NAPI * instead
 *     - Multiple device support
 *     - Large tidy-up following feedback from netdev list
 *
 *   03/08/2006 steve.glendinning@smsc.com
 *     - Added ethtool support
 *     - Convert to use generic MII interface
 *
 *   04/08/2006 bahadir.balban@arm.com
 *     - Added ethtool eeprom r/w support
 *
 *   17/06/2007 steve.glendinning@smsc.com
 *     - Incorporate changes from Bill Gatliff and Russell King
 *
 *   04/07/2007 steve.glendinning@smsc.com
 *     - move irq configuration to platform_device
 *     - fix link poller after interface is stopped and restarted
 *
 *   13/07/2007 bahadir.balban@arm.com
 *     - set irq polarity before requesting irq
 *
 *   28/04/2008 david.giguet@sagem.com
 *     - add compatiblity with LAN9311 chip
 */

#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/mii.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/version.h>
//#include <linux/bug.h>
#include <linux/bitops.h>
#include <linux/irq.h>
#include <asm/io.h>
#if defined(CONFIG_EVTPROBE2)
#include <linux/evtprobe_api.h>
#endif // CONFIG_EVTPROBE2

#ifdef CONFIG_MACH_MX31HSV1
#include <asm/arch/factory_conf.h>
//#define USE_MXC_DMA
#define USE_TASKLET
//unsigned char ag_temp_array[2048];
#endif

#ifdef USE_MXC_DMA
#undef USE_TASKLET
#define MAX_DMA_NB_BD 32
#include <linux/dma-mapping.h>
#include <asm/arch/dma.h>
#endif //#ifdef USE_MXC_DMA

#ifdef USE_TASKLET
#define MAX_DMA_NB_BD 64
#endif

#include "smsc_lan9311.h"

#define SMSC_CHIPNAME		"smsc_lan9311"
#define SMSC_DRV_VERSION	"2008-08-21"

#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_ETH)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

#if 0
#define	LOCK_API if (mutex_is_locked(&pdata->api_lock)) { \
			SMSC_DEBUG("API LOCKED"); \
			mutex_lock(&pdata->api_lock); \
			SMSC_DEBUG("API UNLOCKED"); \
			} \
			else \
			{ \
			SMSC_DEBUG("TAKE API LOCK"); \
			mutex_lock(&pdata->api_lock); \
			}

#define UNLOCK_API if (mutex_is_locked(&pdata->api_lock)) { \
			SMSC_DEBUG("UNLOCK API"); \
			mutex_unlock(&pdata->api_lock); \
			} \
			else \
			{ \
			SMSC_WARNING("Try to unlock API when it is not locked"); \
			}
#else
#define	LOCK_API mutex_lock(&pdata->api_lock);
#define UNLOCK_API mutex_unlock(&pdata->api_lock);
#endif

#ifdef CONFIG_DEBUG_SPINLOCK
#define LOCK_HW if (spin_is_locked(&pdata->hw_lock)) \
	{ \
	SMSC_DEBUG("HW LOCKED"); \
	spin_lock_irqsave(&pdata->hw_lock, flags); \
	SMSC_DEBUG("HW UNLOCKED"); \
	} \
	else \
	{ \
	SMSC_DEBUG("TAKE HW LOCK"); \
	spin_lock_irqsave(&pdata->hw_lock, flags); \
	}

#define UNLOCK_HW if (spin_is_locked(&pdata->hw_lock)) \
	{ \
	SMSC_DEBUG("UNLOCK HARDWARE"); \
	spin_unlock_irqrestore(&pdata->hw_lock, flags); \
	} \
	else \
	{ \
		SMSC_WARNING("Try to unlock HW when it is not locked"); \
	}

#else
#define LOCK_HW spin_lock_irqsave(&pdata->hw_lock, flags);
#define UNLOCK_HW spin_unlock_irqrestore(&pdata->hw_lock, flags);
#endif

MODULE_DESCRIPTION("SMSC Device Driver");
MODULE_AUTHOR("ARM, SMSC, SAGEM");
MODULE_LICENSE("GPL");

static unsigned char plugged_state_init = 0;

#if SMSC_CAN_USE_32BIT

static inline u32 smsc_lan9311_reg_read(struct smsc_lan9311_data *pdata, u32 reg)
{
	return readl(pdata->ioaddr + reg);
}

static inline void smsc_lan9311_reg_write(u32 val, struct smsc_lan9311_data *pdata,
				      u32 reg)
{
	writel(val, pdata->ioaddr + reg)
}

#else				/* SMSC_CAN_USE_32BIT */

static inline u32 smsc_lan9311_reg_read(struct smsc_lan9311_data *pdata, u32 reg)
{
	u32 reg_val;
	unsigned long flags;

	/* these two 16-bit reads must be performed consecutively, so must
	 * not be interrupted by our own ISR (which would start another
	 * read operation) */
	local_irq_save(flags);
	reg_val = ((readw(pdata->ioaddr + reg) & 0xFFFF) |
		   ((readw(pdata->ioaddr + reg + 2) & 0xFFFF) << 16));
	local_irq_restore(flags);

	return reg_val;
}

static inline void smsc_lan9311_reg_write(u32 val, struct smsc_lan9311_data *pdata,
				      u32 reg)
{
	unsigned long flags;

	/* these two 16-bit writes must be performed consecutively, so must
	 * not be interrupted by our own ISR (which would start another
	 * read operation) */
	local_irq_save(flags);
	writew(val & 0xFFFF, pdata->ioaddr + reg);
	writew((val >> 16) & 0xFFFF, pdata->ioaddr + reg + 2);
	local_irq_restore(flags);
}

#endif				/* SMSC_CAN_USE_32BIT */

static inline u32 smsc_lan9311_reg_read_lock(struct smsc_lan9311_data *pdata, u32 reg)
{
	u32 reg_val;
	unsigned long flags;

	LOCK_HW;
	reg_val=smsc_lan9311_reg_read(pdata,reg);
	UNLOCK_HW;
	return reg_val;
}

static inline void smsc_lan9311_reg_write_lock(u32 val, struct smsc_lan9311_data *pdata,u32 reg)
{
	unsigned long flags;

	LOCK_HW;
	smsc_lan9311_reg_write(val,pdata,reg);
	UNLOCK_HW;
}

/* Writes a packet to the TX_DATA_FIFO */
static inline void
smsc911x_tx_writefifo(struct smsc_lan9311_data *pdata, unsigned int *buf,
		      unsigned int wordcount)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
	{
		SMSC_WARNING("hw_lock not held");
	}
#endif				/* CONFIG_DEBUG_SPINLOCK*/
	while (wordcount--)
		smsc_lan9311_reg_write(*buf++, pdata, TX_DATA_FIFO);
}

/* Reads a packet out of the RX_DATA_FIFO */
static inline void
smsc911x_rx_readfifo(struct smsc_lan9311_data *pdata, unsigned int *buf,
		     unsigned int wordcount)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK*/
	while (wordcount--)
		*buf++ = smsc_lan9311_reg_read(pdata, RX_DATA_FIFO);
}

static inline void smsc911x_tx_writefifo_lock(struct smsc_lan9311_data *pdata, unsigned int *buf,unsigned int wordcount)
{
	unsigned long flags;

	LOCK_HW;
	while (wordcount--)
		*buf++ = smsc_lan9311_reg_read(pdata, RX_DATA_FIFO);
	UNLOCK_HW;
}

static inline void
smsc911x_rx_readfifo_lock(struct smsc_lan9311_data *pdata, unsigned int *buf,
		     unsigned int wordcount)
{
	unsigned long flags;

	LOCK_HW;
	while (wordcount--)
		*buf++ = smsc_lan9311_reg_read(pdata, RX_DATA_FIFO);
	UNLOCK_HW;
}

/* waits for MAC not busy, with timeout.  Only called by smsc_lan9311_mac_read
 * and smsc_lan9311_mac_write, so assumes hw_lock is held */
static int smsc_lan9311_mac_notbusy(struct smsc_lan9311_data *pdata)
{
	int i;
	u32 val;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK*/

	for (i = 0; i < 40; i++) {
		val = smsc_lan9311_reg_read(pdata, MAC_CSR_CMD);
		if (!(val & MAC_CSR_CMD_CSR_BUSY_))
			return 1;
	}
	SMSC_WARNING("Timed out waiting for MAC not BUSY. "
		     "MAC_CSR_CMD: 0x%08X", val);
	return 0;
}

/* Fetches a MAC register value. Assumes hw_lock is acquired */
static u32 smsc_lan9311_mac_read(struct smsc_lan9311_data *pdata, unsigned int offset)
{
	unsigned int temp;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK */

	temp = smsc_lan9311_reg_read(pdata, MAC_CSR_CMD);
	if (unlikely(temp & MAC_CSR_CMD_CSR_BUSY_)) {
		SMSC_WARNING("smsc_lan9311_mac_read failed, MAC busy at entry");
		return 0xFFFFFFFF;
	}

	/* Send the MAC cmd */
	smsc_lan9311_reg_write(((offset & 0xFF) | MAC_CSR_CMD_CSR_BUSY_
			    | MAC_CSR_CMD_R_NOT_W_), pdata, MAC_CSR_CMD);

	/* Workaround for hardware read-after-write restriction */
	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);

	/* Wait for the read to happen */
	if (likely(smsc_lan9311_mac_notbusy(pdata)))
		return smsc_lan9311_reg_read(pdata, MAC_CSR_DATA);

	SMSC_WARNING("smsc_lan9311_mac_read failed, MAC busy after read");
	return 0xFFFFFFFF;
}

/* Set a mac register, hw_lock must be acquired before calling */
static void smsc_lan9311_mac_write(struct smsc_lan9311_data *pdata,
			       unsigned int offset, u32 val)
{
	unsigned int temp;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK */

	temp = smsc_lan9311_reg_read(pdata, MAC_CSR_CMD);
	if (unlikely(temp & MAC_CSR_CMD_CSR_BUSY_)) {
		SMSC_WARNING("smsc_lan9311_mac_write failed, MAC busy at entry");
		return;
	}

	/* Send data to write */
	smsc_lan9311_reg_write(val, pdata, MAC_CSR_DATA);

	/* Write the actual data */
	smsc_lan9311_reg_write(((offset & 0xFF) | MAC_CSR_CMD_CSR_BUSY_), pdata,
			   MAC_CSR_CMD);

	/* Workaround for hardware read-after-write restriction */
	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);

	/* Wait for the write to complete */
	if (likely(smsc_lan9311_mac_notbusy(pdata)))
		return;

	SMSC_WARNING("smsc_lan9311_mac_write failed, MAC busy after write");
}

static u32 smsc_lan9311_mac_read_lock(struct smsc_lan9311_data *pdata, unsigned int offset)
{
	unsigned int temp;
	unsigned long flags;

	LOCK_HW;
	temp = smsc_lan9311_reg_read(pdata, MAC_CSR_CMD);
	if (unlikely(temp & MAC_CSR_CMD_CSR_BUSY_)) {
		SMSC_WARNING("smsc_lan9311_mac_read failed, MAC busy at entry");
		UNLOCK_HW;
		return 0xFFFFFFFF;
	}

	/* Send the MAC cmd */
	smsc_lan9311_reg_write(((offset & 0xFF) | MAC_CSR_CMD_CSR_BUSY_
			    | MAC_CSR_CMD_R_NOT_W_), pdata, MAC_CSR_CMD);

	/* Workaround for hardware read-after-write restriction */
	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);

	/* Wait for the read to happen */
	if (likely(smsc_lan9311_mac_notbusy(pdata)))
	{
		temp=smsc_lan9311_reg_read(pdata, MAC_CSR_DATA);
		UNLOCK_HW;
		return temp;
	}

	UNLOCK_HW;
	SMSC_WARNING("smsc_lan9311_mac_read failed, MAC busy after read");
	return 0xFFFFFFFF;
}

static void smsc_lan9311_mac_write_lock(struct smsc_lan9311_data *pdata,unsigned int offset, u32 val)
{
	unsigned long flags;
	unsigned int temp;

	LOCK_HW;
	temp = smsc_lan9311_reg_read(pdata, MAC_CSR_CMD);
	if (unlikely(temp & MAC_CSR_CMD_CSR_BUSY_)) {
		SMSC_WARNING("smsc_lan9311_mac_write failed, MAC busy at entry");
		UNLOCK_HW;
		return;
	}

	/* Send data to write */
	smsc_lan9311_reg_write(val, pdata, MAC_CSR_DATA);

	/* Write the actual data */
	smsc_lan9311_reg_write(((offset & 0xFF) | MAC_CSR_CMD_CSR_BUSY_), pdata,
			   MAC_CSR_CMD);

	/* Workaround for hardware read-after-write restriction */
	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);

	/* Wait for the write to complete */
	if (likely(smsc_lan9311_mac_notbusy(pdata)))
	{
		UNLOCK_HW;
		return;
	}

	UNLOCK_HW;
	SMSC_WARNING("smsc_lan9311_mac_write failed, MAC busy after write");

}

/* Gets a phy register, hw_lock must be acquired before calling */
static u16 smsc_lan9311_phy_read(struct smsc_lan9311_data *pdata, unsigned int phy_addr, unsigned int index)
{
	unsigned int addr;
	int i;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK */

	/* Confirm MII not busy */
	if (unlikely(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_)) {
		SMSC_WARNING("MII is busy in smsc_lan9311_phy_read???");
		return 0;
	}

	/* Set the address, index & direction (read from PHY) */
	addr = ((phy_addr & 0x1F) << 11)
	    | ((index & 0x1F) << 6);
	smsc_lan9311_mac_write(pdata, MII_ACC, addr);

	/* Wait for read to complete w/ timeout */
	for (i = 0; i < 100; i++) {
		/* See if MII is finished yet */
		if (!(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_)) {
			return smsc_lan9311_mac_read(pdata, MII_DATA);
		}
	}
	SMSC_WARNING("Timed out waiting for MII write to finish");
	return 0xFFFF;
}

/* Sets a phy register, hw_lock must be acquired before calling */
static void smsc_lan9311_phy_write(struct smsc_lan9311_data *pdata, unsigned int phy_addr,
			       unsigned int index, u16 val)
{
	unsigned int addr;
	int i;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK */

	/* Confirm MII not busy */
	if (unlikely(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_)) {
		SMSC_WARNING("MII is busy in smsc911x_write_phy???");
		return;
	}

	/* Put the data to write in the MAC */
	smsc_lan9311_mac_write(pdata, MII_DATA, val);

	/* Set the address, index & direction (write to PHY) */
	addr = ((phy_addr & 0x1F) << 11) |
	    ((index & 0x1F) << 6) | MII_ACC_MII_WRITE_;
	smsc_lan9311_mac_write(pdata, MII_ACC, addr);

	/* Wait for write to complete w/ timeout */
	for (i = 0; i < 100; i++) {
		/* See if MII is finished yet */
		if (!(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_))
			return;
	}
	SMSC_WARNING("Timed out waiting for MII write to finish");
}

static u16 smsc_lan9311_phy_read_lock(struct smsc_lan9311_data *pdata, unsigned int phy_addr, unsigned int index)
{
	unsigned int addr;
	int i;
	u32 reg_val;
	unsigned long flags;

	SMSC_DEBUG("Enter");
	LOCK_HW;

	/* Confirm MII not busy */
	if (unlikely(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_)) {
		UNLOCK_HW;
		SMSC_WARNING("MII is busy in smsc_lan9311_phy_read???");
		return 0;
	}

	/* Set the address, index & direction (read from PHY) */
	addr = ((phy_addr & 0x1F) << 11)
	    | ((index & 0x1F) << 6);
	smsc_lan9311_mac_write(pdata, MII_ACC, addr);

	/* Wait for read to complete w/ timeout */
	for (i = 0; i < 100; i++) {
		/* See if MII is finished yet */
		if (!(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_)) {

			reg_val=smsc_lan9311_mac_read(pdata, MII_DATA);
			UNLOCK_HW;
			SMSC_DEBUG("Exit");
			return reg_val;
		}
	}

	UNLOCK_HW;
	SMSC_WARNING("Timed out waiting for MII write to finish");
	return 0xFFFF;
}

static void smsc_lan9311_phy_write_lock(struct smsc_lan9311_data *pdata, unsigned int phy_addr,
			       unsigned int index, u16 val)
{
	unsigned int addr;
	int i;
	unsigned long flags;

	LOCK_HW;

	/* Confirm MII not busy */
	if (unlikely(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_)) {
		UNLOCK_HW;
		SMSC_WARNING("MII is busy in smsc911x_write_phy???");
		return;
	}

	/* Put the data to write in the MAC */
	smsc_lan9311_mac_write(pdata, MII_DATA, val);

	/* Set the address, index & direction (write to PHY) */
	addr = ((phy_addr & 0x1F) << 11) |
	    ((index & 0x1F) << 6) | MII_ACC_MII_WRITE_;
	smsc_lan9311_mac_write(pdata, MII_ACC, addr);

	/* Wait for write to complete w/ timeout */
	for (i = 0; i < 100; i++) {
		/* See if MII is finished yet */
		if (!(smsc_lan9311_mac_read(pdata, MII_ACC) & MII_ACC_MII_BUSY_))
		{
			UNLOCK_HW;
			return;
		}
	}
	UNLOCK_HW;
	SMSC_WARNING("Timed out waiting for MII write to finish");
}

static int smsc_lan9311_mdio_read(struct net_device *dev, int phy_id, int location)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	int reg;

	SMSC_DEBUG("Enter");
	LOCK_API;
	reg = smsc_lan9311_phy_read_lock(pdata, virt_phy_addr, location);
	UNLOCK_API;
	SMSC_DEBUG("Exit");

	return reg;
}

static void smsc_lan9311_mdio_write(struct net_device *dev, int phy_id,
				int location, int val)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	SMSC_DEBUG("Enter");
	LOCK_API;
	smsc_lan9311_phy_write_lock(pdata, virt_phy_addr, location, val);
	UNLOCK_API;
	SMSC_DEBUG("Exit");
}

/* waits for SWITCH not busy, with timeout.  Only called by smsc911x_sw_read
 * and smsc911x_sw_write, so assumes hw_lock is held */
static int smsc911x_sw_notbusy(struct smsc_lan9311_data *pdata)
{
	int i;
	u32 val;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK*/

	for (i = 0; i < 40; i++) {
		val = smsc_lan9311_reg_read(pdata, SW_CSR_CMD);
		if (!(val & SW_CSR_BUSY))
			return 1;
	}
	SMSC_WARNING("Timed out waiting for SW not BUSY. "
		     "SW_CSR_CMD: 0x%08X", val);
	return 0;
}

/* Reads switch fabric registers */
static u32 smsc911x_sw_read(struct smsc_lan9311_data *pdata, unsigned int index)
{
	unsigned int temp;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK*/

	temp = smsc_lan9311_reg_read(pdata, SW_CSR_CMD);
	if (unlikely(temp & SW_CSR_BUSY)) {
		SMSC_WARNING("smsc911x_sw_read failed, SWITCH busy at entry");
		return 0xFFFFFFFF;
	}

	/* Send the SWITCH cmd */
	smsc_lan9311_reg_write(((index & 0xFFFF) | SW_CSR_BUSY
			    | SW_CSR_READ), pdata, SW_CSR_CMD);

	/* Workaround for hardware read-after-write restriction */
	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);

	/* Wait for the read to happen */
	if (likely(smsc911x_sw_notbusy(pdata)))
	{
		temp=smsc_lan9311_reg_read(pdata, SW_CSR_DATA);
		return temp;
	}

	SMSC_WARNING("smsc_lan9311_mac_read failed, SW busy after read");
	return 0xFFFFFFFF;
}

/* Reads switch fabric registers */
static u32 smsc911x_sw_read_lock(struct smsc_lan9311_data *pdata, unsigned int index)
{
	unsigned int temp;
	unsigned long flags;

	LOCK_HW;

	temp = smsc_lan9311_reg_read(pdata, SW_CSR_CMD);
	if (unlikely(temp & SW_CSR_BUSY)) {
		SMSC_WARNING("smsc911x_sw_read failed, SWITCH busy at entry");
		UNLOCK_HW;
		return 0xFFFFFFFF;
	}

	/* Send the SWITCH cmd */
	smsc_lan9311_reg_write(((index & 0xFFFF) | SW_CSR_BUSY
			    | SW_CSR_READ), pdata, SW_CSR_CMD);

	/* Workaround for hardware read-after-write restriction */
	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);

	/* Wait for the read to happen */
	if (likely(smsc911x_sw_notbusy(pdata)))
	{
		temp=smsc_lan9311_reg_read(pdata, SW_CSR_DATA);
		UNLOCK_HW;
		return temp;
	}

	UNLOCK_HW;
	SMSC_WARNING("smsc_lan9311_mac_read failed, SW busy after read");
	return 0xFFFFFFFF;
}

/* Sets a SWITCH register, hw_lock must be acquired before calling */
static void smsc911x_sw_write_lock(struct smsc_lan9311_data *pdata,
			       unsigned int index, u32 val)
{
	unsigned int temp;
	unsigned long flags;

	LOCK_HW;

	/* Confirm SW_CSR not busy */
	if (unlikely(smsc911x_sw_read(pdata, SW_CSR_CMD) & SW_CSR_BUSY)) {
		UNLOCK_HW;
		SMSC_WARNING("SWITCH is busy in smsc911x_sw_write???");
		return;
	}

	/* Send data to write */
	smsc_lan9311_reg_write(val, pdata, SW_CSR_DATA);

	/* Write the actual data */
	smsc_lan9311_reg_write(((index & 0xFFFF) | SW_CSR_BUSY
				| SW_CSR_ENABLE_ALL), pdata, SW_CSR_CMD);

	/* Workaround for hardware read-after-write restriction */
	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);

	/* Wait for the write to complete */
	if (likely(smsc911x_sw_notbusy(pdata)))
	{
		UNLOCK_HW;
		return;
	}

	UNLOCK_HW;
	SMSC_WARNING("smsc911x_sw_write failed, SWITCH busy after write");
	SMSC_WARNING("Timed out waiting for SWITCH write to finish");
}

/* called by phy_initialise and loopback test */
static int smsc_lan9311_phy_reset(struct smsc_lan9311_data *pdata, unsigned int phy_addr)
{
	unsigned int temp;
	unsigned int i = 100000;

	SMSC_TRACE("Performing PHY BCR Reset");
	smsc_lan9311_phy_write_lock(pdata, phy_addr, MII_BMCR, BMCR_RESET);
	do {
		udelay(10);
		temp = smsc_lan9311_phy_read_lock(pdata, phy_addr, MII_BMCR);
	} while ((i--) && (temp & BMCR_RESET));


	if (temp & BMCR_RESET) {
		SMSC_WARNING("PHY reset failed to complete. PHY_ADDR = 0x%x.", phy_addr);
		return 0;
	}
	/* Extra delay required because the phy may not be completed with
	 * its reset when BMCR_RESET is cleared. Specs say 256 uS is
	 * enough delay but using 1ms here to be safe
	 */
	msleep(1);

	return 1;
}

/* Fetches a tx status out of the status fifo */
static unsigned int smsc911x_tx_get_txstatus(struct smsc_lan9311_data *pdata)
{
	unsigned int result;

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK*/

	result =
	    smsc_lan9311_reg_read(pdata, TX_FIFO_INF) & TX_FIFO_INF_TSUSED_;

	if (result != 0)
		result = smsc_lan9311_reg_read(pdata, TX_STATUS_FIFO);

	return result;
}

/* Fetches the next rx status */
static unsigned int smsc_lan9311_rx_get_rxstatus(struct smsc_lan9311_data *pdata)
{
	unsigned int result;

	result = smsc_lan9311_reg_read(pdata, RX_FIFO_INF);
	
	SMSC_DEBUG("result=0x%x",result);
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_RXFIFO_INF,result);
#endif
	result= result & RX_FIFO_INF_RXSUSED_;

	if (result != 0)
		result = smsc_lan9311_reg_read(pdata, RX_STATUS_FIFO);


	return result;
}

/* Fetches the next rx status */
/*static unsigned int smsc_lan9311_rx_get_rxstatus_lock(struct smsc_lan9311_data *pdata)
{
	unsigned int result;
	unsigned long flags;

	LOCK_HW;

	result =
		smsc_lan9311_reg_read(pdata, RX_FIFO_INF) & RX_FIFO_INF_RXSUSED_;

	if (result != 0)
		result = smsc_lan9311_reg_read(pdata, RX_STATUS_FIFO);

	UNLOCK_HW;

	return result;
}*/

#ifdef USE_PHY_WORK_AROUND
static int smsc911x_phy_check_loopbackpkt(struct smsc_lan9311_data *pdata)
{
	unsigned int tries;
	unsigned long flags;
	u32 wrsz;
	u32 rdsz;
	u32 bufp;

	for (tries = 0; tries < 10; tries++) {
		unsigned int txcmd_a;
		unsigned int txcmd_b;
		unsigned int status;
		unsigned int pktlength;
		unsigned int i;

		/* Zero-out rx packet memory */
		memset(pdata->loopback_rx_pkt, 0, MIN_PACKET_SIZE);

		/* Write tx packet to 118 */
		txcmd_a = (((unsigned int)pdata->loopback_tx_pkt)
			   & 0x03) << 16;
		txcmd_a |= TX_CMD_A_FIRST_SEG_ | TX_CMD_A_LAST_SEG_;
		txcmd_a |= MIN_PACKET_SIZE;

		txcmd_b = MIN_PACKET_SIZE << 16 | MIN_PACKET_SIZE;

		LOCK_HW;
		smsc_lan9311_reg_write(txcmd_a, pdata, TX_DATA_FIFO);
		smsc_lan9311_reg_write(txcmd_b, pdata, TX_DATA_FIFO);

		bufp = ((u32) pdata->loopback_tx_pkt) & 0xFFFFFFFC;
		wrsz = MIN_PACKET_SIZE + 3;
		wrsz += (((u32) pdata->loopback_tx_pkt) & 0x3);
		wrsz >>= 2;

		smsc911x_tx_writefifo(pdata, (unsigned int *)bufp, wrsz);
		UNLOCK_HW;

		/* Wait till transmit is done */
		i = 60;
		do {
			udelay(5);

			status = smsc911x_tx_get_txstatus_lock(pdata);
		} while ((i--) && (!status));

		if (!status) {
			SMSC_WARNING("Failed to transmit during loopback test");
			continue;
		}
		if (status & TX_STS_ES_) {
			SMSC_WARNING("Transmit encountered errors during "
				     "loopback test");
			continue;
		}

		/* Wait till receive is done */
		i = 60;
		do {
			udelay(5);
			status = smsc_lan9311_rx_get_rxstatus_lock(pdata);
		} while ((i--) && (!status));

		if (!status) {
			SMSC_WARNING("Failed to receive during loopback test");
			continue;
		}
		if (status & RX_STS_ES_) {
			SMSC_WARNING("Receive encountered errors during "
				     "loopback test");
			continue;
		}

		pktlength = ((status & 0x3FFF0000UL) >> 16);
		bufp = (u32)pdata->loopback_rx_pkt;
		rdsz = pktlength + 3;
		rdsz += ((u32)pdata->loopback_rx_pkt) & 0x3;
		rdsz >>= 2;

		smsc911x_rx_readfifo_lock(pdata, (unsigned int *)bufp, rdsz);

		if (pktlength != (MIN_PACKET_SIZE + 4)) {
			SMSC_WARNING("Unexpected packet size during "
				     "loop back test, size=%d, "
				     "will retry", pktlength);
		} else {
			unsigned int j;
			int mismatch = 0;
			for (j = 0; j < MIN_PACKET_SIZE; j++) {
				if (pdata->loopback_tx_pkt[j]
				    != pdata->loopback_rx_pkt[j]) {
					mismatch = 1;
					break;
				}
			}
			if (!mismatch) {
				SMSC_TRACE("Successfully verified "
					   "loopback packet");
				return 1;
			} else {
				SMSC_WARNING("Data miss match during "
					     "loop back test, will retry.");
			}
		}
	}

	return 0;
}

static int smsc911x_phy_loopbacktest(struct smsc_lan9311_data *pdata)
{
	int result = 0;
	unsigned int i;
	unsigned int val;
	unsigned long flags;

	SMSC_DEBUG("Enter");

	/* Initialise tx packet */
	for (i = 0; i < 6; i++) {
		/* Use broadcast destination address */
		pdata->loopback_tx_pkt[i] = (char)0xFF;
	}

	for (i = 6; i < 12; i++) {
		/* Use incrementing source address */
		pdata->loopback_tx_pkt[i] = (char)i;
	}

	/* Set length type field */
	pdata->loopback_tx_pkt[12] = 0x00;
	pdata->loopback_tx_pkt[13] = 0x00;
	for (i = 14; i < MIN_PACKET_SIZE; i++) {
		pdata->loopback_tx_pkt[i] = (char)i;
	}

	LOCK_HW;
	val = smsc_lan9311_reg_read(pdata, HW_CFG);
	val &= HW_CFG_TX_FIF_SZ_;
	val |= HW_CFG_SF_;
	smsc_lan9311_reg_write(val, pdata, HW_CFG);

	smsc_lan9311_reg_write(TX_CFG_TX_ON_, pdata, TX_CFG);
	smsc_lan9311_reg_write((((unsigned int)pdata->loopback_rx_pkt)
			    & 0x03) << 8, pdata, RX_CFG);
	UNLOCK_HW;

	for (i = 0; i < 10; i++) {
		/* Set PHY to 10/FD, no ANEG, and loopback mode */
		LOCK_HW;
		smsc_lan9311_phy_write(pdata, virt_phy_addr, MII_BMCR, 0x4100);

		/* Enable MAC tx/rx, FD */
		smsc_lan9311_mac_write(pdata, MAC_CR, MAC_CR_FDPX_
				   | MAC_CR_TXEN_ | MAC_CR_RXEN_);
		UNLOCK_HW;
		if (smsc911x_phy_check_loopbackpkt(pdata)) {
			result = 1;
			break;
		}
		pdata->resetcount++;

		/* Disable MAC rx */
		smsc_lan9311_mac_write_lock(pdata, MAC_CR, 0);

		smsc_lan9311_phy_reset(pdata, port1_phy_addr);
		smsc_lan9311_phy_reset(pdata, port2_phy_addr);
		smsc_lan9311_phy_reset(pdata, virt_phy_addr);
	}

	/* Disable MAC */
	LOCK_HW;
	smsc_lan9311_mac_write(pdata, MAC_CR, 0);

	/* Cancel PHY loopback mode */
	smsc_lan9311_phy_write(pdata, virt_phy_addr, MII_BMCR, 0);

	smsc_lan9311_reg_write(0, pdata, TX_CFG);
	smsc_lan9311_reg_write(0, pdata, RX_CFG);
	UNLOCK_HW;

 	SMSC_DEBUG("Exit");
	return result;
}
#endif				/* USE_PHY_WORK_AROUND */

/* assumes hw_lock is held */
static void smsc911x_phy_update_flowcontrol(struct smsc_lan9311_data *pdata)
{
	unsigned int temp;
	SMSC_DEBUG("Enter");

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK*/

	if (pdata->mii.full_duplex) {
		unsigned int phy_adv;
		unsigned int phy_lpa;

		SMSC_TRACE("Full Duplex");
		phy_adv = smsc_lan9311_phy_read(pdata, port1_phy_addr, MII_ADVERTISE);
		SMSC_TRACE("PORT1 PHY MMI_ADVERTISE=0x%x",phy_adv);
		phy_lpa = smsc_lan9311_phy_read(pdata, port1_phy_addr, MII_LPA);
		SMSC_TRACE("PORT1 PHY MMI_LPA=0x%x",phy_lpa);
		phy_adv = smsc_lan9311_phy_read(pdata, port2_phy_addr, MII_ADVERTISE);
		SMSC_TRACE("PORT2 PHY MMI_ADVERTISE=0x%x",phy_adv);
		phy_lpa = smsc_lan9311_phy_read(pdata, port2_phy_addr, MII_LPA);
		SMSC_TRACE("PORT2 PHY MMI_LPA=0x%x",phy_lpa);
		phy_adv = smsc_lan9311_phy_read(pdata, virt_phy_addr, MII_ADVERTISE);
		SMSC_TRACE("VIRT PHY MMI_ADVERTISE=0x%x",phy_adv);
		phy_lpa = smsc_lan9311_phy_read(pdata, virt_phy_addr, MII_LPA);
		SMSC_TRACE("VIRT PHY MMI_LPA=0x%x",phy_lpa);


		if (phy_adv & phy_lpa & LPA_PAUSE_CAP) {
			SMSC_TRACE("Symmetric Pause");
			/* Both ends support symmetric pause, enable
			 * PAUSE receive and transmit */
			smsc_lan9311_mac_write(pdata, FLOW, 0xFFFF0002);
			temp = smsc_lan9311_reg_read(pdata, AFC_CFG);
			temp |= 0xF;
			smsc_lan9311_reg_write(temp, pdata, AFC_CFG);
		} else if (((phy_adv & ADVERTISE_PAUSE_ALL) ==
			    ADVERTISE_PAUSE_ALL) &&
			   ((phy_lpa & LPA_PAUSE_ALL) == LPA_PAUSE_ASYM)) {
			SMSC_TRACE("Asym Pause Rx");
			/* We support symmetric and asym pause, the
			 * other end only supports asym, Enable PAUSE
			 * receive, disable PAUSE transmit */
			smsc_lan9311_mac_write(pdata, FLOW, 0xFFFF0002);
			temp = smsc_lan9311_reg_read(pdata, AFC_CFG);
			temp &= ~0xF;
			smsc_lan9311_reg_write(temp, pdata, AFC_CFG);
		} else {
			SMSC_TRACE("Pause Disabled");
			/* Disable PAUSE receive and transmit */
			smsc_lan9311_mac_write(pdata, FLOW, 0);
			temp = smsc_lan9311_reg_read(pdata, AFC_CFG);
			temp &= ~0xF;
			smsc_lan9311_reg_write(temp, pdata, AFC_CFG);
		}
	} else {
		SMSC_TRACE("Half Duplex");
		smsc_lan9311_mac_write(pdata, FLOW, 0);
		temp = smsc_lan9311_reg_read(pdata, AFC_CFG);
		temp |= 0xF;
		smsc_lan9311_reg_write(temp, pdata, AFC_CFG);
	}

	temp = smsc_lan9311_mac_read(pdata, FLOW);
	SMSC_TRACE("FLOW=0x%x",temp);	
	temp = smsc_lan9311_reg_read(pdata, AFC_CFG);
	SMSC_TRACE("AFC_CFG=0x%x",temp);
	temp = smsc911x_sw_read(pdata, SW_MANUAL_FC_1);
	SMSC_TRACE("SW_MANUAL_FC_1=0x%x",temp);
	temp = smsc911x_sw_read(pdata, SW_MANUAL_FC_2);
	SMSC_TRACE("SW_MANUAL_FC_2=0x%x",temp);
	temp = smsc911x_sw_read(pdata, SW_MANUAL_FC_MII);
	SMSC_TRACE("SW_MANUAL_FC_MMI=0x%x",temp);

	SMSC_DEBUG("Exit");
}

/* Update link mode if any thing has changed */
static void smsc_lan9311_phy_update_linkmode(struct net_device *dev, int init)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned long flags;
	SMSC_DEBUG("Enter");

	//need to unlock, as mmi_check_media calls the mdio_read
	UNLOCK_API;
	if (mii_check_media(&pdata->mii, netif_msg_link(pdata), init)) {
		/* duplex state has changed */
		unsigned int mac_cr;

		LOCK_API;
		LOCK_HW;
		mac_cr = smsc_lan9311_mac_read(pdata, MAC_CR);
		if (pdata->mii.full_duplex) {
			SMSC_TRACE("configuring for full duplex mode");
			mac_cr |= MAC_CR_FDPX_;
		} else {
			SMSC_TRACE("configuring for half duplex mode");
			mac_cr &= ~MAC_CR_FDPX_;
		}
		smsc_lan9311_mac_write(pdata, MAC_CR, mac_cr);

		smsc911x_phy_update_flowcontrol(pdata);

		UNLOCK_HW;
	}
	else
	{
		LOCK_API;
	}
#ifndef CONFIG_SMSC_LAN9311
#ifdef USE_LED1_WORK_AROUND
	if (netif_carrier_ok(dev)) {
		if ((pdata->gpio_orig_setting & GPIO_CFG_LED1_EN_) &&
		    (!pdata->using_extphy)) {
			/* Restore orginal GPIO configuration */
			pdata->gpio_setting = pdata->gpio_orig_setting;
			smsc_lan9311_reg_write_lock(pdata->gpio_setting, pdata,
					   GPIO_CFG);
		}
	} else {
		/* Check global setting that LED1
		 * usage is 10/100 indicator */
		LOCK_HW;
		pdata->gpio_setting = smsc_lan9311_reg_read(pdata, GPIO_CFG);
		if ((pdata->gpio_setting & GPIO_CFG_LED1_EN_)
		    && (!pdata->using_extphy)) {
			/* Force 10/100 LED off, after saving
			 * orginal GPIO configuration */
			pdata->gpio_orig_setting = pdata->gpio_setting;

			pdata->gpio_setting &= ~GPIO_CFG_LED1_EN_;
			pdata->gpio_setting |= (GPIO_CFG_GPIOBUF0_
						| GPIO_CFG_GPIODIR0_
						| GPIO_CFG_GPIOD0_);
			smsc_lan9311_reg_write(pdata->gpio_setting, pdata,
					   GPIO_CFG);
		}
		UNLOCK_HW;
	}
#endif				/* USE_LED1_WORK_AROUND */
#endif //#ifndef CONFIG_SMSC_LAN9311
	SMSC_DEBUG("Exit");
}


static void link_work(void *arg)
{
	struct net_device *dev = (struct net_device *)arg;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned int temp;
	unsigned char plugged_state;

	SMSC_DEBUG("Enter");
	LOCK_API;
	temp = smsc_lan9311_phy_read_lock(pdata, port1_phy_addr, PHY_MODE_CONTROL_STATUS_x);
	plugged_state = ((temp & ENERGYON) == 0) ? 0 : 1;
	if ((plugged_state != pdata->port1_plugged_state) || !plugged_state_init)
	{
		pdata->port1_plugged_state = plugged_state;
#if defined(CONFIG_EVTPROBE2)
		send_netlink_msg(SYSM_G1, plugged_state, GFP_ATOMIC); /* PORT1 PHY plugged state event. */
#endif // CONFIG_EVTPROBE2
		if (plugged_state == 1) smsc_lan9311_phy_update_linkmode(dev, 1);
	}
	temp = smsc_lan9311_phy_read_lock(pdata, port2_phy_addr, PHY_MODE_CONTROL_STATUS_x);
	plugged_state = ((temp & ENERGYON) == 0) ? 0 : 1;
	if ((plugged_state != pdata->port2_plugged_state) || !plugged_state_init)
	{
		pdata->port2_plugged_state = plugged_state;
#if defined(CONFIG_EVTPROBE2)
		send_netlink_msg(SYSM_G2, plugged_state, GFP_ATOMIC); /* PORT2 PHY plugged state event. */
#endif // CONFIG_EVTPROBE2
		if (plugged_state == 1) smsc_lan9311_phy_update_linkmode(dev, 1);
	}

	plugged_state_init = 1;
	smsc_lan9311_phy_update_linkmode(dev, 0);
	if (!(pdata->stop_link_poll)) {
		pdata->link_poll_timer.expires = jiffies + 2 * HZ;
		add_timer(&pdata->link_poll_timer);
	} else {
		pdata->stop_link_poll = 0;
	}
	UNLOCK_API;
	SMSC_DEBUG("Exit");
}

/* Entry point for the link poller */
static void smsc_lan9311_phy_checklink(unsigned long ptr)
{
	struct net_device *dev = (struct net_device *)ptr;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	SMSC_DEBUG("Enter");
	//flush any pending work
	flush_workqueue(pdata->link_work_queue);
	//prepare work
	PREPARE_WORK(&pdata->link_work,link_work,(void *)dev);
	//submit work
	queue_work(pdata->link_work_queue,&pdata->link_work);

	SMSC_DEBUG("Exit");
}

/* Initialises the PHY layer.  Called at initialisation by open() so
 * interrupts are enabled */
static int smsc911x_phy_initialise(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned int temp;
	unsigned long flags;

	SMSC_DEBUG("Enter");
	/* Reset the phy */
	pdata->mii.phy_id = 1;
	if (!smsc_lan9311_phy_reset(pdata, port1_phy_addr) | !smsc_lan9311_phy_reset(pdata, port2_phy_addr) | !smsc_lan9311_phy_reset(pdata, virt_phy_addr)) {
		SMSC_WARNING("PHY reset failed to complete.");
		return 0;
	}

	/* Configure connector LEDs */
	smsc_lan9311_reg_write_lock(0x2FF, pdata, LED_CFG);

#ifdef USE_PHY_WORK_AROUND
	if (!smsc911x_phy_loopbacktest(pdata)) {
		SMSC_WARNING("Failed Loop Back Test");
		return 0;
	} else {
		SMSC_TRACE("Passed Loop Back Test");
	}
#endif			/* USE_PHY_WORK_AROUND */
	/* Advertise all speeds and pause capabilities */
	LOCK_HW;
	temp = smsc_lan9311_phy_read(pdata, virt_phy_addr, MII_ADVERTISE);
	temp |= (ADVERTISE_ALL | ADVERTISE_PAUSE_CAP /*| ADVERTISE_PAUSE_ASYM  | ADVERTISE_RFAULT */);
	smsc_lan9311_phy_write(pdata, virt_phy_addr, MII_ADVERTISE, temp);
	temp |= ADVERTISE_PAUSE_ASYM;
	smsc_lan9311_phy_write(pdata, port1_phy_addr, MII_ADVERTISE, temp);
	smsc_lan9311_phy_write(pdata, port2_phy_addr, MII_ADVERTISE, temp);
	pdata->mii.advertising = temp;

	/* Restart Auto-Negociation */
	smsc_lan9311_phy_write(pdata, port1_phy_addr, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);  /* Restart */
	smsc_lan9311_phy_write(pdata, port2_phy_addr, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);  /* Auto-Negociation for Port_1_PHY, */
	smsc_lan9311_phy_write(pdata, virt_phy_addr, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);   /* Port_2_PHY, and Virt_PHY. */

        temp=smsc_lan9311_phy_read(pdata, virt_phy_addr,VPHY_BASIC_CTRL);
	SMSC_TRACE("VPHY_BASIC_CTRL read = 0x%x", temp);
	temp |= (BMCR_ANENABLE | BMCR_ANRESTART);
	SMSC_TRACE("VPHY_BASIC_CTRL written = 0x%x", temp);
        smsc_lan9311_phy_write(pdata, virt_phy_addr, VPHY_BASIC_CTRL, temp);
	UNLOCK_HW;

	pdata->port1_plugged_state = 0;
	pdata->port2_plugged_state = 0;

	/* begin to establish link */
	smsc_lan9311_phy_update_linkmode(dev, 1);

	setup_timer(&pdata->link_poll_timer, smsc_lan9311_phy_checklink,
		(unsigned long)dev);
	pdata->link_poll_timer.expires = jiffies + 10 * HZ;
	add_timer(&pdata->link_poll_timer);
	pdata->stop_link_poll = 0;

	SMSC_TRACE("phy initialised succesfully");
	return 1;
}

/* Gets the number of tx statuses in the fifo */
static unsigned int smsc911x_tx_get_txstatcount(struct smsc_lan9311_data *pdata)
{
	unsigned int result = (smsc_lan9311_reg_read(pdata, TX_FIFO_INF)
			       & TX_FIFO_INF_TSUSED_) >> 16;
	return result;
}


/* Reads tx statuses and increments counters where necessary */
static void smsc911x_tx_update_txcounters(struct smsc_lan9311_data *pdata)
{
	unsigned int tx_stat;


	while ((tx_stat = smsc911x_tx_get_txstatus(pdata)) != 0) {
		if (unlikely(tx_stat & 0x80000000)) {
			/* In this driver the packet tag is used as the packet
			 * length. Since a packet length can never reach the
			 * size of 0x8000, this bit is reserved. It is worth
			 * noting that the "reserved bit" in the warning above
			 * does not reference a hardware defined reserved bit
			 * but rather a driver defined one.
			 */
			SMSC_WARNING("Packet tag reserved bit is high");
		} else {
			if (unlikely(tx_stat & 0x00008000)) {
				pdata->stats.tx_errors++;
			} else {
				pdata->stats.tx_packets++;
				pdata->stats.tx_bytes += (tx_stat >> 16);
			}
			if (unlikely(tx_stat & 0x00000100)) {
				pdata->stats.collisions += 16;
				pdata->stats.tx_aborted_errors += 1;
			} else {
				pdata->stats.collisions +=
				    ((tx_stat >> 3) & 0xF);
			}
			if (unlikely(tx_stat & 0x00000800)) {
				pdata->stats.tx_carrier_errors += 1;
			}
			if (unlikely(tx_stat & 0x00000200)) {
				pdata->stats.collisions++;
				pdata->stats.tx_aborted_errors++;
			}
		}
	}
}

/* Reads tx statuses and increments counters where necessary */
static void smsc911x_tx_update_txcounters_lock(struct smsc_lan9311_data *pdata)
{
	unsigned long flags;

	LOCK_HW;
	smsc911x_tx_update_txcounters(pdata);
	UNLOCK_HW;
}

/* Increments the Rx error counters */
static void
smsc911x_rx_counterrors(struct smsc_lan9311_data *pdata, unsigned int rxstat)
{
	int crc_err = 0;

	if (unlikely(rxstat & 0x00008000)) {
		pdata->stats.rx_errors++;
		SMSC_TRACE("nb rx error=%d",(int)pdata->stats.rx_errors);
		if (unlikely(rxstat & 0x00000002)) {
			pdata->stats.rx_crc_errors++;
			SMSC_TRACE("nb rx crc error=%d",(int)pdata->stats.rx_crc_errors);
			crc_err = 1;
		}
	}
	if (likely(!crc_err)) {
		if (unlikely((rxstat & 0x00001020) == 0x00001020)) {
			/* Frame type indicates length,
			 * and length error is set */
			pdata->stats.rx_length_errors++;
			SMSC_TRACE("rx length error=%d",(int)pdata->stats.rx_length_errors);
		}
		if (rxstat & RX_STS_MCAST_)
			pdata->stats.multicast++;
	}
}

/* Quickly dumps bad packets */
static void
smsc911x_rx_fastforward(struct smsc_lan9311_data *pdata, unsigned int pktbytes)
{
	unsigned int pktwords = (pktbytes + NET_IP_ALIGN + 3) >> 2;

	if (likely(pktwords >= 4)) {
		unsigned int timeout = 500;
		unsigned int val;
		smsc_lan9311_reg_write(RX_DP_CTRL_RX_FFWD_, pdata, RX_DP_CTRL);
		do {
			udelay(1);
			val = smsc_lan9311_reg_read(pdata, RX_DP_CTRL);
		} while (timeout-- && (val & RX_DP_CTRL_RX_FFWD_));

		if (unlikely(timeout == 0))
			SMSC_WARNING("Timed out waiting for RX FFWD "
				     "to finish, RX_DP_CTRL: 0x%08X", val);
	} else {
		unsigned int temp;
		while (pktwords--)
			temp = smsc_lan9311_reg_read(pdata, RX_DATA_FIFO);
	}
}

/* Read more fragment tag */
int smsc911x_read_is_last_packet(unsigned char *ip_buffer)
{
	unsigned short *ip_flag;

	ip_flag=(short *)&ip_buffer[14];
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_LAST_FRAG,*ip_flag);
#endif
	SMSC_DEBUG("ip_flag=0x%x",*ip_flag);
	if (*ip_flag==0x0008)
	{
#ifdef DO_ULOG
		ulog(ULOG_LAN9311_LAST_FRAG,ip_buffer[22]) ;
#endif
		SMSC_DEBUG("last_frag_flag=0x%x",ip_buffer[22]);
		if (ip_buffer[22]&0x20)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	return 1;
}

#if (defined USE_MXC_DMA) || (defined USE_TASKLET)
/* NAPI poll function */
static int smsc911x_poll(struct net_device *dev, int *budget)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	int npackets = 0;
	int quota;
	//int last_frag;
	unsigned long flags;
	struct sk_buff *skb;

	SMSC_DEBUG("Enter");
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_POLL,0) ;
#endif

	LOCK_HW;

	quota = min(dev->quota, *budget);
	SMSC_DEBUG("budget=%d, dev->quota=%d, quota=%d",*budget,dev->quota,quota); 
	while (npackets < quota) {
		
		SMSC_DEBUG("npackets=%d, req_no=%d, completed_no=%d, read_no=%d, nb_bd=%d",npackets,pdata->dma_rx_last_req_index,pdata->dma_rx_last_completed_index,pdata->dma_rx_last_read_index,pdata->dma_rx_nb_bd);
		/* break out of while loop if there are no more packets waiting */
		if (pdata->dma_rx_last_read_index==pdata->dma_rx_last_completed_index)
		{
			break;
		}
		skb=pdata->skb_dma_req_rx[pdata->dma_rx_last_read_index];
		//last_frag=smsc911x_read_is_last_packet(skb->head);
		(pdata->dma_rx_last_read_index)++;
		if (pdata->dma_rx_last_read_index==MAX_DMA_NB_BD)
		{
			//the value is %MAX_DMA_NB_BD
			pdata->dma_rx_last_read_index=0;
		}
		npackets++;
		UNLOCK_HW;
		dev->last_rx = jiffies;
		skb->dev = dev;
		skb->protocol = eth_type_trans(skb, dev);
		skb->ip_summed = CHECKSUM_NONE;
		netif_receive_skb(skb);
		/*if (last_frag)
		{
			*budget -= npackets;
			dev->quota -= npackets;
			netif_rx_complete(dev);
			SMSC_DEBUG("Exit, ret=0, budget=%d",*budget);
#ifdef DO_ULOG
			ulog(ULOG_LAN9311_POLL_DONE,0) ;
#endif
			return 0;
		}*/
		LOCK_HW;
	}

	*budget -= npackets;
	dev->quota -= npackets;

	if (npackets < quota) {
		// We processed all packets available.  Tell NAPI it can
		// stop polling then re-enable rx interrupts 
		netif_rx_complete(dev);
		
		UNLOCK_HW;
		SMSC_DEBUG("%s - Exit, ret=0, budget=%d\n",__FUNCTION__,*budget);
		return 0;
	}
	UNLOCK_HW;
	// There are still packets waiting
	SMSC_DEBUG("Exit, ret=1, budget=%d",*budget);
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_POLL_DONE,1) ;
#endif
	return 1;
}
#else
/* NAPI poll function */
static int smsc911x_poll(struct net_device *dev, int *budget)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	int npackets = 0;
	int quota;
	unsigned long flags;
	unsigned int temp;
	//int last_frag=1;
	unsigned int rxstat=0;

#ifdef DO_ULOG
	ulog(ULOG_LAN9311_POLL,0) ;
#endif
	SMSC_DEBUG("Enter");
	LOCK_HW;
	quota = min(dev->quota, *budget);
	SMSC_TRACE("budget=%d, dev->quota=%d, quota=%d",*budget,dev->quota,quota); 
	while (npackets < quota) {
		unsigned int pktlength;
		unsigned int pktwords;
		rxstat = smsc_lan9311_rx_get_rxstatus(pdata);
		//nb_dropped = smsc_lan9311_reg_read(pdata, RX_DROP);
		pktlength = ((rxstat & 0x3FFF0000) >> 16);
		pktwords = (pktlength + NET_IP_ALIGN + 3) >> 2;  //NET_IP_ALGIGN for RX_CFG configuration RXDOFF=2, +3 for RX_CFG configuration RX_EA=0 (algined on 4 bytes)
	
		SMSC_DEBUG("npackets=%d, rxstat=0x%x, pktlength=%d, pktwords=%d",npackets,rxstat,pktlength,pktwords);
		/* break out of while loop if there are no more packets waiting */
#ifdef DO_ULOG
		ulog(ULOG_LAN9311_PACKET,npackets);
		ulog(ULOG_LAN9311_RXSTAT,rxstat);
		ulog(ULOG_LAN9311_PKTLENGTH,pktlength);
#endif
		if (!rxstat)
		{
			break;
		}

		smsc911x_rx_counterrors(pdata, rxstat);

		if (likely((rxstat & RX_STS_ES_) == 0)) {
			struct sk_buff *skb;
			//unsigned char *ptr;
			UNLOCK_HW;
			skb = dev_alloc_skb((pktwords<<2));
			if (likely(skb)) {
				skb->data = skb->head;
				skb->tail = skb->head;
				/* Align IP on 16B boundary */
				skb_reserve(skb, NET_IP_ALIGN);
				skb_put(skb, (pktlength-2));	//-2 for RX_CFG configuration RXDOFF=2
				LOCK_HW;
				smsc911x_rx_readfifo(pdata,
						     (unsigned int *)skb->head,
						     pktwords);
				//ptr=(unsigned char *)skb->head;
				//SMSC_DEBUG("ptr=0x%x: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",(unsigned int)ptr,ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);
				/* Update counters */
				pdata->stats.rx_packets++;
				pdata->stats.rx_bytes += (pktlength-2); //-2 for RX_CFG configuration RXDOFF=2
				dev->last_rx = jiffies;
				UNLOCK_HW;
				skb->dev = dev;
				skb->protocol = eth_type_trans(skb, dev);
				skb->ip_summed = CHECKSUM_NONE;
				//last_frag = smsc911x_read_is_last_packet(skb->head);
				netif_receive_skb(skb);
				npackets++;
				LOCK_HW;
				continue;
			} else {
				LOCK_HW;
				SMSC_WARNING("Unable to allocate sk_buff "
					     "for rx packet, in PIO path");
				pdata->stats.rx_dropped++;
			}
		}
		/* At this point, the packet is to be read out
		 * of the fifo and discarded */
		SMSC_TRACE("discard pktlength=%d",pktlength);
		smsc911x_rx_fastforward(pdata, pktlength);
	}

	*budget -= npackets;
	dev->quota -= npackets;

	//acknowledge RSFL
	smsc_lan9311_reg_write(INT_STS_RSFL_, pdata, INT_STS);
	//Enable again the interrupt
	temp = smsc_lan9311_reg_read(pdata, INT_EN);
	temp |= INT_EN_RSFL_EN_;
	smsc_lan9311_reg_write(temp, pdata, INT_EN);
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_IRQ_ENABLE,0);
#endif

	if (npackets < quota) {
		// We processed all packets available.  Tell NAPI it can
		// stop polling 
		UNLOCK_HW;
		netif_rx_complete(dev);
		SMSC_DEBUG("Exit, ret=0, budget=%d",*budget);
#ifdef DO_ULOG
		ulog(ULOG_LAN9311_POLL_DONE,0) ;
#endif
		return 0;
	}
	UNLOCK_HW;
	// There are still packets waiting 
	SMSC_DEBUG("Exit, ret=1, budget=%d",*budget);
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_POLL_DONE,1) ;
#endif
	return 1;
}
#endif

/* Returns hash bit number for given MAC address
 * Example:
 * 01 00 5E 00 00 01 -> returns bit number 31 */
static unsigned int smsc911x_hash(char addr[ETH_ALEN])
{
	unsigned int crc;
	unsigned int result;

	crc = ether_crc(ETH_ALEN, addr);
	result = (crc >> 26) & 0x3f;

	return result;
}

static void smsc911x_rx_multicast_update(struct smsc_lan9311_data *pdata)
{
	/* Performs the multicast & mac_cr update.  This is called when
	 * safe on the current hardware, and with the hw_lock held */
	unsigned int mac_cr = smsc_lan9311_mac_read(pdata, MAC_CR);

#ifdef CONFIG_DEBUG_SPINLOCK
	if (!spin_is_locked(&pdata->hw_lock))
		SMSC_WARNING("hw_lock not held");
#endif				/* CONFIG_DEBUG_SPINLOCK*/

	mac_cr |= pdata->set_bits_mask;
	mac_cr &= ~(pdata->clear_bits_mask);
	smsc_lan9311_mac_write(pdata, MAC_CR, mac_cr);
	smsc_lan9311_mac_write(pdata, HASHH, pdata->hashhi);
	smsc_lan9311_mac_write(pdata, HASHL, pdata->hashlo);

	SMSC_TRACE("maccr 0x%08X, HASHH 0x%08X, HASHL 0x%08X", mac_cr,
		   pdata->hashhi, pdata->hashlo);
}

static void smsc911x_rx_multicast_update_workaround(struct smsc_lan9311_data *pdata)
{
	unsigned int mac_cr;

	/* This function is only called for older LAN911x devices
	 * (revA or revB), where MAC_CR, HASHH and HASHL should not
	 * be modified during Rx - newer devices immediately update the
	 * registers.
	 *
	 * This is called from interrupt context */


	/* Check Rx has stopped */
	if (smsc_lan9311_mac_read(pdata, MAC_CR) & MAC_CR_RXEN_)
		SMSC_WARNING("Rx not stopped");

	/* Perform the update - safe to do now Rx has stopped */
	smsc911x_rx_multicast_update(pdata);

	/* Re-enable Rx */
	mac_cr = smsc_lan9311_mac_read(pdata, MAC_CR);
	mac_cr |= MAC_CR_RXEN_;
	smsc_lan9311_mac_write(pdata, MAC_CR, mac_cr);

	pdata->multicast_update_pending = 0;
}

/* Sets the device MAC address to dev_addr, called with hw_lock held */
static void
smsc911x_set_mac_address_lock(struct smsc_lan9311_data *pdata, u8 dev_addr[6])
{
	unsigned long flags;
	u32 mac_high16 = (dev_addr[5] << 8) | dev_addr[4];
	u32 mac_low32 = (dev_addr[3] << 24) | (dev_addr[2] << 16) |
	    (dev_addr[1] << 8) | dev_addr[0];

	LOCK_HW;
	smsc_lan9311_mac_write(pdata, ADDRH, mac_high16);
	smsc_lan9311_mac_write(pdata, ADDRL, mac_low32);
	UNLOCK_HW;
}

static int smsc911x_soft_reset(struct smsc_lan9311_data *pdata)
{
       unsigned int timeout;
       unsigned int temp;

       /* Reset the LAN911x */
       smsc_lan9311_reg_write(HW_CFG_SRST_, pdata, HW_CFG);
       timeout = 10;
       do {
               udelay(10);
               temp = smsc_lan9311_reg_read(pdata, HW_CFG);
       } while ((--timeout) && (temp & HW_CFG_SRST_));

       if (unlikely(temp & HW_CFG_SRST_)) {
               SMSC_WARNING("Failed to complete reset");
               return -ENODEV;
       }
       return 0;
}

static int smsc_lan9311_open(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned int timeout;
	unsigned int temp;
	unsigned int intcfg;
#ifdef CONFIG_FACTORY_ZONE
	extern int factory_parse_parameter(factory_parameter_t *parameter);
	static int result;
	static factory_parameter_t parameter;
	static char w[3];
#endif
	SMSC_DEBUG("Enter");
	LOCK_API;

	/* Reset the LAN911x */
	smsc_lan9311_reg_write_lock(HW_CFG_SRST_, pdata, HW_CFG);
	timeout = 10;
	do {
		udelay(10);
		temp = smsc_lan9311_reg_read_lock(pdata, HW_CFG);
	} while ((--timeout) && (temp & HW_CFG_SRST_));

	if (unlikely(temp & HW_CFG_SRST_)) {
		SMSC_WARNING("Failed to complete reset");
		UNLOCK_API;
		SMSC_DEBUG("Exit");
		return -ENODEV;
	}

	/* Set TX FIFO size and RX FIFO size to 8KB */
	smsc_lan9311_reg_write_lock(0x00080000, pdata, HW_CFG);

	/* Automatic Flow Control High Level = 0x6e
	 * Automatic Flow Control Low Level = 0x37
	 * Back pressure duration = 100 us at 100 MBit/s 102.2 at 10 MBit/s */
	smsc_lan9311_reg_write_lock(0x00402050, pdata, AFC_CFG);

	/* Make sure EEPROM has finished loading before setting GPIO_CFG */
	timeout = 50;
	while ((timeout--) &&
	       (smsc_lan9311_reg_read_lock(pdata, E2P_CMD) & E2P_CMD_EPC_BUSY_)) {
		udelay(10);
	}

	if (unlikely(timeout == 0)) {
		SMSC_WARNING("Timed out waiting for EEPROM "
			     "busy bit to clear");
	}
#ifndef CONFIG_SMSC_LAN9311
#if USE_DEBUG >= 1
	smsc_lan9311_reg_write_lock(0x00670700, pdata, GPIO_CFG);
#else
	smsc_lan9311_reg_write_lock(0x70070000, pdata, GPIO_CFG);
#endif
#endif //#ifndef CONFIG_SMSC_LAN9311

	/* Initialise irqs, but leave all sources disabled */
	smsc_lan9311_reg_write_lock(0, pdata, INT_EN);
	smsc_lan9311_reg_write_lock(0xFFFFFFFF, pdata, INT_STS);

	/* Set interrupt deassertion to 100uS */
	intcfg = ((10 << 24) | INT_CFG_IRQ_EN_);

	if (pdata->irq_polarity) {
		SMSC_TRACE("irq polarity: active high");
		intcfg |= INT_CFG_IRQ_POL_;
	} else {
		SMSC_TRACE("irq polarity: active low");
	}

	if (pdata->irq_type) {
		SMSC_TRACE("irq type: push-pull");
		intcfg |= INT_CFG_IRQ_TYPE_;
	} else {
		SMSC_TRACE("irq type: open drain");
	}

	SMSC_TRACE("0x%x written into INT_CFG",intcfg);
	smsc_lan9311_reg_write_lock(intcfg, pdata, INT_CFG);

	SMSC_TRACE("Testing irq handler using IRQ %d", dev->irq);
	pdata->software_irq_signal = 0;
	smp_wmb();

	temp = smsc_lan9311_reg_read_lock(pdata, INT_EN);
	temp |= INT_EN_SW_INT_EN_;

	smsc_lan9311_reg_write_lock(temp, pdata, INT_EN);

	timeout = 1000;
	while (timeout--) {
		smp_rmb();
		if (pdata->software_irq_signal)
			break;
		msleep(1);
	}

	if (!pdata->software_irq_signal) {
		SMSC_WARNING("%s: ISR failed signaling test (IRQ %d)",
		       dev->name, dev->irq);
		UNLOCK_API;
		SMSC_DEBUG("Exit");
		return -ENODEV;
	}
	SMSC_TRACE("IRQ handler passed test using IRQ %d", dev->irq);

#ifdef CONFIG_SMSC_LAN9311
	SMSC_TRACE(KERN_INFO "%s: SMSC9311 identified at %#08lx, IRQ: %d",
	       dev->name, (unsigned long)pdata->ioaddr, dev->irq);
#else
	SMSC_TRACE(KERN_INFO "%s: SMSC911x/921x identified at %#08lx, IRQ: %d",
	       dev->name, (unsigned long)pdata->ioaddr, dev->irq);
#endif

	/* Check if mac address has been specified when bringing interface up */
	if (is_valid_ether_addr(dev->dev_addr)) {
		smsc911x_set_mac_address_lock(pdata, dev->dev_addr);
		SMSC_TRACE("MAC Address is specified by configuration");
	} else {
		/* Try reading mac address from device. if EEPROM is present
		 * it will already have been set */
		u32 mac_high16 = smsc_lan9311_mac_read_lock(pdata, ADDRH);
		u32 mac_low32 = smsc_lan9311_mac_read_lock(pdata, ADDRL);
		dev->dev_addr[0] = (u8)(mac_low32);
		dev->dev_addr[1] = (u8)(mac_low32 >> 8);
		dev->dev_addr[2] = (u8)(mac_low32 >> 16);
		dev->dev_addr[3] = (u8)(mac_low32 >> 24);
		dev->dev_addr[4] = (u8)(mac_high16);
		dev->dev_addr[5] = (u8)(mac_high16 >> 8);

		if (is_valid_ether_addr(dev->dev_addr)) {
			/* eeprom values are valid  so use them */
			SMSC_TRACE("Mac Address is read from LAN911x EEPROM");
		} else {
#ifdef CONFIG_FACTORY_ZONE
			strcpy(&parameter.name[0], "MAC address");
			strcpy(&parameter.application[0], "smsc911x");
			strcpy(&parameter.version[0], "");
			result = factory_parse_parameter(&parameter);
			if (result == 0)
			{
				if (strlen(&parameter.value[0]) == 17)
				{
					w[0] = parameter.value[0];
					w[1] = parameter.value[1];
					w[2] = 0;
					dev->dev_addr[0] = (int)simple_strtoul(&w[0], NULL, 16);
					w[0] = parameter.value[3];
					w[1] = parameter.value[4];
					dev->dev_addr[1] = (int)simple_strtoul(&w[0], NULL, 16);
					w[0] = parameter.value[6];
					w[1] = parameter.value[7];
					dev->dev_addr[2] = (int)simple_strtoul(&w[0], NULL, 16);
					w[0] = parameter.value[9];
					w[1] = parameter.value[10];
					dev->dev_addr[3] = (int)simple_strtoul(&w[0], NULL, 16);
					w[0] = parameter.value[12];
					w[1] = parameter.value[13];
					dev->dev_addr[4] = (int)simple_strtoul(&w[0], NULL, 16);
					w[0] = parameter.value[15];
					w[1] = parameter.value[16];
					dev->dev_addr[5] = (int)simple_strtoul(&w[0], NULL, 16);
				}
				else
				{
					// Let's attribute always the same dummy MAC address
					dev->dev_addr[0]=0x00;
					dev->dev_addr[1]=0x12;
					dev->dev_addr[2]=0x34;
					dev->dev_addr[3]=0x56;
					dev->dev_addr[4]=0x78;
					dev->dev_addr[5]=0x9a;
				}
			}
			else
			{
				// Let's attribute always the same dummy MAC address
				dev->dev_addr[0]=0x00;
				dev->dev_addr[1]=0x12;
				dev->dev_addr[2]=0x34;
				dev->dev_addr[3]=0x56;
				dev->dev_addr[4]=0x78;
				dev->dev_addr[5]=0x9a;
			}
			smsc911x_set_mac_address_lock(pdata,dev->dev_addr);
#else
			/* eeprom values are invalid, generate random MAC */
			random_ether_addr(dev->dev_addr);
			smsc911x_set_mac_address_lock(pdata, dev->dev_addr);
			SMSC_TRACE("MAC Address is set to random_ether_addr");
#endif
		}
	}



	SMSC_TRACE("%s: MAC Address: %02x:%02x:%02x:%02x:%02x:%02x",
	       dev->name, dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
	       dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);

	netif_carrier_off(dev);

	if (!smsc911x_phy_initialise(dev)) {
		SMSC_WARNING("Failed to initialize PHY");
		UNLOCK_API;
		return -ENODEV;
	}

	/* Store and Forward a frame of transmit data */
	temp = smsc_lan9311_reg_read_lock(pdata, HW_CFG);
	temp &= HW_CFG_TX_FIF_SZ_;
	temp |= HW_CFG_SF_;
	smsc_lan9311_reg_write_lock(temp, pdata, HW_CFG);

	/* TX Data Available Level = 0x78, RX Status Level = 0. */
	temp = smsc_lan9311_reg_read_lock(pdata, FIFO_INT);
	temp &= ~FIFO_INT_TX_AVAIL_LEVEL_;
	temp |= (0x78 << 24);
	temp &= ~FIFO_INT_RX_STS_LEVEL_;
	smsc_lan9311_reg_write_lock(temp, pdata, FIFO_INT);

#ifdef USE_MXC_DMA
	/* DMA requires 32-bits alignement */
	smsc_lan9311_reg_write_lock(0, pdata, RX_CFG);
#else
	/* set RX Data offset to 2 bytes for alignment */
	smsc_lan9311_reg_write_lock((2 << 8), pdata, RX_CFG);
#endif

	temp = smsc_lan9311_reg_read_lock(pdata, INT_EN);
	temp |= (PHY_INT1_EN | PHY_INT2_EN | INT_EN_RXE_EN_ | INT_EN_TXE_EN_ | 
		INT_EN_RSFL_EN_ | INT_EN_TDFA_EN_ | INT_EN_RXSTOP_INT_EN_ | INT_EN_TXSTOP_INT_EN_ | INT_EN_RXDF_EN_);
	smsc_lan9311_reg_write_lock(temp, pdata, INT_EN);

	temp = smsc_lan9311_mac_read_lock(pdata, MAC_CR);
	temp |= (MAC_CR_TXEN_ | MAC_CR_RXEN_);
	smsc_lan9311_mac_write_lock(pdata, MAC_CR, temp);

	smsc_lan9311_reg_write_lock(TX_CFG_TX_ON_, pdata, TX_CFG);

	netif_start_queue(dev);

#ifdef CONFIG_SMSC_LAN9311
	// test SWITCH fabric interface
	{
		u32 temp;
		//check version
		temp=smsc911x_sw_read_lock(pdata,SW_DEV_ID);
		SMSC_TRACE("Switch Fabric Device Type=0x%x",(0x00ff0000&temp)>>16);
		SMSC_TRACE("Switch Fabric Chip Version Code=0x%x",(0x0000ff00&temp)>>8);
		SMSC_TRACE("Switch Fabric Chip Version Code=0x%x",(0xff&temp));
		// unmask all switch interrupts
		temp=SW_IMR_MASK_BM;
 		smsc911x_sw_write_lock(pdata,SW_IMR,temp);
	}
#endif
	UNLOCK_API;
	SMSC_DEBUG("Exit");
	return 0;
}

void dump_registers(struct smsc_lan9311_data *pdata)
{	int i;
	unsigned int temp, temp1, temp2;
	unsigned long flags;

	printk("Address Offset	Register Value\n");
	LOCK_HW;
	for (i = ID_REV; i <= 0x1F8; i += (sizeof(u32)))
	{
		temp = smsc_lan9311_reg_read(pdata, i);
		printk("0x%03x		0x%08x\n", i, temp);
	}
	printk("Index		PORT1 PHY		PORT2 PHY		VIRT PHY\n");
	for (i = 0; i <= 31; i++)
	{
		temp = smsc_lan9311_phy_read(pdata, port1_phy_addr, i);
		temp1 = smsc_lan9311_phy_read(pdata, port2_phy_addr, i);
		temp2 = smsc_lan9311_phy_read(pdata, virt_phy_addr, i);
		printk("%u			0x%08x		0x%08x		0x%08x\n", i, temp, temp1, temp2);
	}
	UNLOCK_HW;
}

/* Entry point for stopping the interface */
static int smsc911x_stop(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	SMSC_DEBUG("Enter");
	LOCK_API;
// dump_registers(pdata);

	pdata->stop_link_poll = 1;
	del_timer_sync(&pdata->link_poll_timer);

	smsc_lan9311_reg_write_lock((smsc_lan9311_reg_read(pdata, INT_CFG) &
			    (~INT_CFG_IRQ_EN_)), pdata, INT_CFG);
	netif_stop_queue(dev);

	/* At this point all Rx and Tx activity is stopped */
	pdata->stats.rx_dropped += smsc_lan9311_reg_read_lock(pdata, RX_DROP);
	smsc911x_tx_update_txcounters_lock(pdata);

	SMSC_TRACE("Interface stopped");
	UNLOCK_API;
	SMSC_DEBUG("Exit");
	return 0;
}

/* Entry point for transmitting a packet */
static int smsc_lan9311_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned int freespace;
	unsigned int tx_cmd_a;
	unsigned int tx_cmd_b;
	unsigned int temp;
	u32 wrsz;
	u32 bufp;
	unsigned long flags;

	SMSC_DEBUG("Enter");
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_XMIT,skb->len);
#endif

	LOCK_HW;
	freespace = smsc_lan9311_reg_read(pdata, TX_FIFO_INF) & TX_FIFO_INF_TDFREE_;

	if (unlikely(freespace < TX_FIFO_LOW_THRESHOLD))
		SMSC_WARNING("smsc LAN9311 - Tx data fifo low, space available: %d",
			     freespace);

	/* Word alignment adjustment */
	tx_cmd_a = ((((unsigned int)(skb->data)) & 0x03) << 16);
	tx_cmd_a |= TX_CMD_A_FIRST_SEG_ | TX_CMD_A_LAST_SEG_;
	tx_cmd_a |= (unsigned int)skb->len;

	tx_cmd_b = ((unsigned int)skb->len) << 16;
	tx_cmd_b |= (unsigned int)skb->len;

	smsc_lan9311_reg_write(tx_cmd_a, pdata, TX_DATA_FIFO);
	smsc_lan9311_reg_write(tx_cmd_b, pdata, TX_DATA_FIFO);

	bufp = ((u32)skb->data) & 0xFFFFFFFC;
	wrsz = (u32)skb->len + 3;
	wrsz += ((u32)skb->data) & 0x3;
	wrsz >>= 2;

	smsc911x_tx_writefifo(pdata, (unsigned int *)bufp, wrsz);
	freespace -= (skb->len + 32);
	dev_kfree_skb(skb);
	dev->trans_start = jiffies;

	if (unlikely(smsc911x_tx_get_txstatcount(pdata) >= 30))
		smsc911x_tx_update_txcounters(pdata);

	if (freespace < TX_FIFO_LOW_THRESHOLD) {
		netif_stop_queue(dev);
		temp = smsc_lan9311_reg_read(pdata, FIFO_INT);
		temp &= 0x00FFFFFF;
		temp |= 0x32000000;
		smsc_lan9311_reg_write(temp, pdata, FIFO_INT);
	}
	UNLOCK_HW;
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_XMIT_DONE,freespace);
#endif
	SMSC_DEBUG("Exit");
	return NETDEV_TX_OK;
}

/* Entry point for getting status counters */
static struct net_device_stats *smsc911x_get_stats(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned long flags;

	SMSC_DEBUG("Enter");
	LOCK_API;
	LOCK_HW;
	smsc911x_tx_update_txcounters(pdata);
	//pdata->stats.rx_dropped += smsc_lan9311_reg_read(pdata, RX_DROP);
	UNLOCK_HW;
	UNLOCK_API;
	SMSC_DEBUG("Exit");

	return &pdata->stats;
}

/* Entry point for setting addressing modes */
static void smsc911x_set_multicast_list(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned long flags;

	SMSC_DEBUG("Enter");
	LOCK_API;
	if (dev->flags & IFF_PROMISC) {
		/* Enabling promiscuous mode */
		pdata->set_bits_mask = MAC_CR_PRMS_;
		pdata->clear_bits_mask = (MAC_CR_MCPAS_ | MAC_CR_HPFILT_);
		pdata->hashhi = 0;
		pdata->hashlo = 0;
	} else if (dev->flags & IFF_ALLMULTI) {
		/* Enabling all multicast mode */
		pdata->set_bits_mask = MAC_CR_MCPAS_;
		pdata->clear_bits_mask = (MAC_CR_PRMS_ | MAC_CR_HPFILT_);
		pdata->hashhi = 0;
		pdata->hashlo = 0;
	} else if (dev->mc_count > 0) {
		/* Enabling specific multicast addresses */
		unsigned int hash_high = 0;
		unsigned int hash_low = 0;
		unsigned int count = 0;
		struct dev_mc_list *mc_list = dev->mc_list;

		pdata->set_bits_mask = MAC_CR_HPFILT_;
		pdata->clear_bits_mask = (MAC_CR_PRMS_ | MAC_CR_MCPAS_);

		while (mc_list) {
			count++;
			if ((mc_list->dmi_addrlen) == ETH_ALEN) {
				unsigned int bitnum =
				    smsc911x_hash(mc_list->dmi_addr);
				unsigned int mask = 0x01 << (bitnum & 0x1F);
				if (bitnum & 0x20)
					hash_high |= mask;
				else
					hash_low |= mask;
			} else {
				SMSC_WARNING("dmi_addrlen != 6");
			}
			mc_list = mc_list->next;
		}
		if (count != (unsigned int)dev->mc_count)
			SMSC_WARNING("mc_count != dev->mc_count");

		pdata->hashhi = hash_high;
		pdata->hashlo = hash_low;
	} else {
		/* Enabling local MAC address only */
		pdata->set_bits_mask = 0;
		pdata->clear_bits_mask =
		    (MAC_CR_PRMS_ | MAC_CR_MCPAS_ | MAC_CR_HPFILT_);
		pdata->hashhi = 0;
		pdata->hashlo = 0;
	}

	LOCK_HW;

	if (pdata->generation <= 1) {
		/* Older hardware revision - cannot change these flags while
		 * receiving data */
		if (!pdata->multicast_update_pending) {
			unsigned int temp;
			SMSC_TRACE("scheduling mcast update");
			pdata->multicast_update_pending = 1;

			/* Request the hardware to stop, then perform the
			 * update when we get an RX_STOP interrupt */
			smsc_lan9311_reg_write(INT_STS_RXSTOP_INT_, pdata, INT_STS);
			temp = smsc_lan9311_reg_read(pdata, INT_EN);
			temp |= INT_EN_RXSTOP_INT_EN_;
			smsc_lan9311_reg_write(temp, pdata, INT_EN);

			temp = smsc_lan9311_mac_read(pdata, MAC_CR);
			temp &= ~(MAC_CR_RXEN_);
			smsc_lan9311_mac_write(pdata, MAC_CR, temp);
		} else {
			/* There is another update pending, this should now
			 * use the newer values */
		}
	} else {
		/* Newer hardware revision - can write immediately */
		smsc911x_rx_multicast_update(pdata);
	}

	UNLOCK_HW;
	UNLOCK_API;
	SMSC_DEBUG("Exit");
}

#ifdef USE_TASKLET
static void smsc_tasklet_schedule_rx(struct net_device *dev);


static void smsc_tasklet_clbk_tx(void *data,unsigned int count)
{

}

static void smsc_tasklet_clbk_rx(void *data,unsigned int count)
{
	struct net_device *dev = (struct net_device *)data;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	struct sk_buff *skb;
	unsigned long flags;
	unsigned int temp;
	//int last_frag=0;
	//unsigned char *ptr;

	SMSC_DEBUG("Enter, count=0x%x, req_no=%d, completed_no=%d, read_no=%d, nb_bd=%d",count, pdata->dma_rx_last_req_index,pdata->dma_rx_last_completed_index,pdata->dma_rx_last_read_index,pdata->dma_rx_nb_bd);
	LOCK_HW;

	/* Update counters */
	pdata->stats.rx_packets++;
	//pdata->stats.rx_bytes += (count-2); //-2 for RX_CFG configuration RXDOFF=2
	pdata->stats.rx_bytes += count;
	skb=pdata->skb_dma_req_rx[pdata->dma_rx_last_completed_index];
	//ptr=(unsigned char *)skb->head;
	//SMSC_DEBUG("ptr=0x%x: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",(unsigned int)ptr,ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);
	(pdata->dma_rx_last_completed_index)++;
	if (pdata->dma_rx_last_completed_index==MAX_DMA_NB_BD)
	{
		//the value is %MAX_DMA_NB_BD
		pdata->dma_rx_last_completed_index=0;
	}
	(pdata->dma_rx_nb_bd)++;

	//last_frag += smsc911x_read_is_last_packet(skb->head);

	if (pdata->dma_rx_last_req_index==pdata->dma_rx_last_completed_index)
	{
		if (pdata->dma_active_rx)
		{
			//disable DMA and schedule NAPI
			SMSC_DEBUG("Schedule NAPI");
			pdata->dma_active_rx=0;
			//if (last_frag)
			{
				// Schedule NAPI poll
				netif_rx_schedule(dev);
			}
			//acknowledge reach of RX interrupt level
			smsc_lan9311_reg_write(INT_STS_RSFL_, pdata, INT_STS);
			//enable interrupt again
			temp = smsc_lan9311_reg_read(pdata, INT_EN);
			temp |= INT_EN_RSFL_EN_;
			smsc_lan9311_reg_write(temp, pdata, INT_EN);
#ifdef DO_ULOG
			ulog(ULOG_LAN9311_IRQ_ENABLE,0);
#endif
		}
		else
		{
			SMSC_WARNING("DMA disabled twice, this is not normal");
		}
	}
	UNLOCK_HW;

	SMSC_DEBUG("Exit");	
}

static void tasklet_tx(unsigned long data)
{
}

static void tasklet_rx(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	struct sk_buff *skb;
	int pktwords;
	int pktbytes;
	unsigned long flags;

	SMSC_DEBUG("Enter");

	while (pdata->tasklet_rx_index!=pdata->dma_rx_last_req_index)
	{
		LOCK_HW;
		SMSC_DEBUG("tasklet_index=%d req_no=%d",pdata->tasklet_rx_index,pdata->dma_rx_last_req_index);

		skb=pdata->skb_dma_req_rx[pdata->tasklet_rx_index];
		pktwords=pdata->skb_tasklet_req_size_words[pdata->tasklet_rx_index];
		pktbytes=(pktwords<<2);
		(pdata->tasklet_rx_index)++;
		if (pdata->tasklet_rx_index==MAX_DMA_NB_BD)
		{
			//the value is %MAX_DMA_NB_BD
			pdata->tasklet_rx_index=0;
		}
        	smsc911x_rx_readfifo(pdata,(unsigned int *)skb->head,pktwords);
		UNLOCK_HW;
		pdata->tasklet_clbk_rx(dev,pktbytes);
	}
	SMSC_DEBUG("Exit");
}

static void smsc_tasklet_schedule_rx(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	int npackets = 0;
	//int nb_dropped;
	int nb_max_requests=pdata->dma_rx_nb_bd;
	unsigned int rxstat=0;

	SMSC_DEBUG("Enter, nb_max_requests=%d",nb_max_requests);
	
	if (nb_max_requests==0)
	{
		SMSC_WARNING("RX Data congestion, nb_max_requests=0");
		return;
	}

	while (npackets<nb_max_requests) {
		unsigned int pktlength;
		unsigned int pktwords;
		unsigned int pktbytes;

		rxstat = smsc_lan9311_rx_get_rxstatus(pdata);
		pktlength = ((rxstat & 0x3FFF0000) >> 16);
		pktwords = (pktlength + NET_IP_ALIGN + 3) >> 2; //+2 for RX_CFG configuration RXDOFF=2 , +3 for 32_bit alignement
		//pktwords = (pktlength + 3) >> 2; //+3 for 32-bit alignement
		pktbytes = (pktwords<<2);

		SMSC_TRACE("npackets=%d, rxstat=0x%x, pktlength=%d, pktbytes=%d req_no=%d, completed_no=%d, read_no=%d, nb_bd=%d",
		npackets,rxstat,pktlength,pktbytes,pdata->dma_rx_last_req_index,pdata->dma_rx_last_completed_index,pdata->dma_rx_last_read_index,pdata->dma_rx_nb_bd);

#ifdef DO_ULOG
		ulog(ULOG_LAN9311_PACKET,npackets);
		ulog(ULOG_LAN9311_RXSTAT,rxstat);
		ulog(ULOG_LAN9311_PKTLENGTH,pktlength);
#endif

		/* break out of while loop if there are no more packets waiting */
		if (!rxstat)
		{
			break;
		}

		smsc911x_rx_counterrors(pdata, rxstat);
		
		if (likely((rxstat & RX_STS_ES_) == 0)) {
			struct sk_buff *skb;
			skb = dev_alloc_skb((pktwords<<2));
			if (likely(skb)) {
				skb->data = skb->head;
				skb->tail = skb->head;

				/* Align IP on 16B boundary */
				skb_reserve(skb, NET_IP_ALIGN);
				skb_put(skb, pktlength - 2); //-2 for RX_CFG configuration RXDOFF=2
				//skb_put(skb, pktlength);
				//skb->dev = dev;
				//skb->protocol = eth_type_trans(skb, dev);
				//skb->ip_summed = CHECKSUM_NONE;

				//update the skb table of dma requests
				pdata->skb_dma_req_rx[pdata->dma_rx_last_req_index]=skb;
				pdata->skb_tasklet_req_size_words[pdata->dma_rx_last_req_index]=pktwords;

				//schedule tasklet
				pdata->dma_active_rx=1;
				tasklet_schedule(&pdata->tasklet_rx);

				npackets++;
				(pdata->dma_rx_nb_bd)--;
				(pdata->dma_rx_last_req_index)++;
				if (pdata->dma_rx_last_req_index==MAX_DMA_NB_BD)
				{
					//the value is %MAX_DMA_NB_BD
					pdata->dma_rx_last_req_index=0;
				}
				if (pdata->dma_rx_last_req_index==pdata->dma_rx_last_read_index)
				{
					SMSC_WARNING("Dropping oldest packet");
					(pdata->dma_rx_last_read_index)++;
					(pdata->stats.rx_dropped)++;
				}
				continue;
			} else {
				rxstat=0;
				SMSC_WARNING("Unable to allocate sk_buff "
					     "for rx packet, in PIO path");
				pdata->stats.rx_dropped++;
			}
		}
		/* At this point, the packet is to be read out
		 * of the fifo and discarded */
		SMSC_TRACE("discard pktlength=%d",pktlength);
		smsc911x_rx_fastforward(pdata, pktlength);
	}

	if (rxstat!=0)
	{
		SMSC_WARNING("RX Data congestion");
	}

	SMSC_DEBUG("Exit");
}
#endif

#ifdef USE_MXC_DMA
static void smsc_dma_schedule_rx(struct net_device *dev);

static void smsc_dma_clbk_tx(void *data, int error,unsigned int count)
{

}

static void smsc_dma_clbk_rx(void *data, int error,unsigned int count)
{
	struct net_device *dev = (struct net_device *)data;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	struct sk_buff *skb;
	unsigned long flags;
	unsigned char *ptr;

	SMSC_DEBUG("Enter, error=0x%x, count=0x%x, req_no=%d, completed_no=%d, read_no=%d, nb_bd=%d", error,count, pdata->dma_rx_last_req_index,pdata->dma_rx_last_completed_index,pdata->dma_rx_last_read_index,pdata->dma_rx_nb_bd);
	LOCK_HW;
	
	/* Update counters */
	pdata->stats.rx_packets++;
	//pdata->stats.rx_bytes += (count-2); //-2 for RX_CFG configuration RXDOFF=2
	pdata->stats.rx_bytes += count;
	skb=pdata->skb_dma_req_rx[pdata->dma_rx_last_completed_index];
	ptr=(unsigned char *)skb->head;
	SMSC_DEBUG("ptr=0x%x: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",(unsigned int)ptr,ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);
	(pdata->dma_rx_last_completed_index)++;
	if (pdata->dma_rx_last_completed_index==MAX_DMA_NB_BD)
	{
		//the value is %MAX_DMA_NB_BD
		pdata->dma_rx_last_completed_index=0;
	}
	(pdata->dma_rx_nb_bd)++;

	if (pdata->dma_rx_last_req_index==pdata->dma_rx_last_completed_index)
	{
		if (pdata->dma_active_rx)
		{
			unsigned int temp;
			SMSC_DEBUG("Schedule NAPI");
			pdata->dma_active_rx=0;
			mxc_dma_disable(pdata->dma_rx_channel);
			// Schedule NAPI poll
			netif_rx_schedule(dev);
			//acknowledge reach of RX interrupt level
			smsc_lan9311_reg_write(INT_STS_RSFL_, pdata, INT_STS);
			//enable interrupt again
			temp = smsc_lan9311_reg_read(pdata, INT_EN);
			temp |= INT_EN_RSFL_EN_;
			smsc_lan9311_reg_write(temp, pdata, INT_EN);
#ifdef DO_ULOG
			ulog(ULOG_LAN9311_IRQ_ENABLE,0);
#endif
		}
		else
		{
			SMSC_WARNING("DMA disabled twice, this is not normal");
		}
	}

	UNLOCK_HW;
	SMSC_DEBUG("Exit");	
}

static void smsc_dma_schedule_rx(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	int npackets = 0;
	int ret;
	int nb_max_requests=pdata->dma_rx_nb_bd;
	unsigned int rxstat=0;

	SMSC_DEBUG("Enter, nb_max_requests=%d",nb_max_requests);
	
	if (nb_max_requests==0)
	{
		SMSC_WARNING("RX Data congestion, nb_max_requests=0");
		return;
	}

	while (npackets<nb_max_requests) {
		unsigned int pktlength;
		unsigned int pktwords;
		unsigned int pktbytes;
		rxstat = smsc_lan9311_rx_get_rxstatus(pdata);
		pktlength = ((rxstat & 0x3FFF0000) >> 16);
		//pktwords = (pktlength + NET_IP_ALIGN + 3) >> 2;
		pktwords = (pktlength + 3) >> 2;
		pktbytes = (pktwords << 2);
	
		SMSC_TRACE("npackets=%d, rxstat=0x%x, pktlength=%d, pktbytes=%d req_no=%d, completed_no=%d, read_no=%d, nb_bd=%d",
		npackets,rxstat,pktlength,pktbytes,pdata->dma_rx_last_req_index,pdata->dma_rx_last_completed_index,pdata->dma_rx_last_read_index,pdata->dma_rx_nb_bd);

#ifdef DO_ULOG
		ulog(ULOG_LAN9311_PACKET,npackets);
		ulog(ULOG_LAN9311_RXSTAT,rxstat);
		ulog(ULOG_LAN9311_PKTLENGTH,pktlength);
#endif

		/* break out of while loop if there are no more packets waiting */
		if (!rxstat)
		{
			break;
		}

		smsc911x_rx_counterrors(pdata, rxstat);
		
		if (likely((rxstat & RX_STS_ES_) == 0)) {
			struct sk_buff *skb;
			//unsigned char *ptr;
			skb = dev_alloc_skb((pktwords<<2));
			if (likely(skb)) {
				mxc_dma_requestbuf_t dma_request;

				skb->data = skb->head;
				skb->tail = skb->head;
				/*ptr=(unsigned char *)skb->head;
				ptr[0]=0xff;
				ptr[1]=0xff;
				ptr[2]=0xff;
				ptr[3]=0xff;
				ptr[4]=0xff;
				ptr[5]=0xff;
				ptr[6]=0xff;
				ptr[7]=0xff;
				SMSC_DEBUG("ptr=0x%x: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",(unsigned int)ptr,ptr[0],ptr[1],ptr[2],ptr[3],ptr[4],ptr[5],ptr[6],ptr[7]);*/
				/* Align IP on 16B boundary */
				//skb_reserve(skb, NET_IP_ALIGN);
				//skb_put(skb, pktlength - 2); //-2 for RX_CFG configuration RXDOFF=2
				skb_put(skb, pktlength);
				//skb->dev = dev;
				//skb->protocol = eth_type_trans(skb, dev);
				//skb->ip_summed = CHECKSUM_NONE;

				//update the skb table of dma requests
				pdata->skb_dma_req_rx[pdata->dma_rx_last_req_index]=skb;

				//do the DMA request
				memset(&dma_request, 0, sizeof(mxc_dma_requestbuf_t));
				dma_request.dst_addr = (dma_addr_t) (dma_map_single(NULL,
					skb->head,
					pktbytes,
					DMA_FROM_DEVICE));
				dma_request.src_addr =
					(dma_addr_t) (dma_map_single(NULL,
					pdata->ioaddr+RX_DATA_FIFO,
					2048,
					DMA_TO_DEVICE));
				//dma_request.src_addr = (dma_addr_t)&ag_temp_array;
				//dma_request.src_addr = (dma_addr_t)(dma_map_single(NULL,
				//	&ag_temp_array,
				//	2048,
				//	DMA_TO_DEVICE));
				dma_request.num_of_bytes = pktbytes;
				ret = mxc_dma_config(pdata->dma_rx_channel, &dma_request, 1,MXC_DMA_MODE_READ);
				if (ret!=0)
				{
					SMSC_WARNING("dma configuration failed - err=%d\n",ret);
				}
				else
				{
					ret = mxc_dma_enable(pdata->dma_rx_channel);
					if (ret!=0)
					{
						pdata->dma_active_rx=1;
						SMSC_WARNING("dma enabled failed - err=%d\n",ret);
					}
				}

				npackets++;
				(pdata->dma_rx_nb_bd)--;
				(pdata->dma_rx_last_req_index)++;
				if (pdata->dma_rx_last_req_index==MAX_DMA_NB_BD)
				{
					//the value is %MAX_DMA_NB_BD
					pdata->dma_rx_last_req_index=0;
				}
				if (pdata->dma_rx_last_req_index==pdata->dma_rx_last_read_index)
				{
					SMSC_WARNING("Dropping oldest packet");
					(pdata->dma_rx_last_read_index)++;
					(pdata->stats.rx_dropped)++;
				}
				continue;
			} else {
				rxstat=0;
				SMSC_WARNING("Unable to allocate sk_buff "
					     "for rx packet, in PIO path");
				pdata->stats.rx_dropped++;
			}
		}
		/* At this point, the packet is to be read out
		 * of the fifo and discarded */
		SMSC_TRACE("discard pktlength=%d",pktlength);
		smsc911x_rx_fastforward(pdata, pktlength);
	}

	if (rxstat!=0)
	{
		SMSC_WARNING("RX Data congestion");
	}

	/*nb_dropped = smsc_lan9311_reg_read(pdata, RX_DROP);
	pdata->stats.rx_dropped += nb_dropped;
	if (nb_dropped)
	{
		SMSC_TRACE("nb packets dropped in switch=%d",nb_dropped);
	}*/

	//acknowledge reach of RX interrupt level
	//smsc_lan9311_reg_write(INT_STS_RSFL_, pdata, INT_STS);

	SMSC_DEBUG("Exit");
}
#endif //#ifdef USE_MXC_DMA

static irqreturn_t smsc_lan9311_irqhandler(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned int intsts;
	unsigned int inten;
	unsigned int temp;
	unsigned int nb_dropped;
	int serviced = IRQ_NONE;

	SMSC_DEBUG("Enter");
	intsts = smsc_lan9311_reg_read(pdata, INT_STS);
	inten = smsc_lan9311_reg_read(pdata, INT_EN);

	SMSC_TRACE("intsts = 0x%x inten = 0x%x", intsts, inten);
//	dump_registers(pdata);
#ifdef DO_ULOG
	ulog(ULOG_LAN9311_IRQ,intsts) ;
#endif

	if (unlikely(intsts & inten & PHY_INT1_STS)) {
		smsc_lan9311_reg_write(PHY_INT1_STS, pdata, INT_STS);
		temp = smsc_lan9311_phy_read(pdata, port1_phy_addr, PHY_INTERRUPT_SOURCE_x);
		SMSC_TRACE("PORT1 PHY interrupt, sts 0x%04X", (u16)temp);
		serviced = IRQ_HANDLED;
	}

	if (unlikely(intsts & inten & PHY_INT2_STS)) {
		smsc_lan9311_reg_write(PHY_INT2_STS, pdata, INT_STS);
		temp = smsc_lan9311_phy_read(pdata, port2_phy_addr, PHY_INTERRUPT_SOURCE_x);
		SMSC_TRACE("PORT2 PHY interrupt, sts 0x%04X", (u16)temp);
		serviced = IRQ_HANDLED;
	}

	if (unlikely(intsts & inten & INT_STS_RXE_)) {
		/* receive error */
		SMSC_TRACE("smsc LAN9311 - RX Error interrupt !");
		smsc_lan9311_reg_write(INT_STS_RXE_, pdata, INT_STS);
		serviced = IRQ_HANDLED;
	}

	if (unlikely(intsts & inten & INT_STS_TXE_)) {
		/* transmit error */
		SMSC_TRACE("smsc LAN9311 - TX Error interrupt !");
		smsc_lan9311_reg_write(INT_STS_TXE_, pdata, INT_STS);
		serviced = IRQ_HANDLED;
	}

	if (likely(intsts & inten & INT_STS_RSFL_)) {
		/* Disable Rx interrupts */
		temp = smsc_lan9311_reg_read(pdata, INT_EN);
		temp &= (~INT_EN_RSFL_EN_);
		smsc_lan9311_reg_write(temp, pdata, INT_EN);
#if (defined USE_MXC_DMA)
		/* Schedule DMA.*/
		smsc_dma_schedule_rx(dev);
#elif (defined USE_TASKLET)
		/* Schedule tasklet.*/
		smsc_tasklet_schedule_rx(dev);
#else
		/* Schedule NAPI poll */
		netif_rx_schedule(dev);
#endif

		serviced = IRQ_HANDLED;
	}

	if (unlikely(intsts & inten & INT_STS_RXDF_)) 
	{
		nb_dropped = smsc_lan9311_reg_read(pdata, RX_DROP);

		//acknowledge Drop Frame interrupt
		smsc_lan9311_reg_write(INT_STS_RXDF_, pdata, INT_STS);
		SMSC_TRACE("Switch has dropped packets, nb_drop=%d",nb_dropped);
#ifdef DO_ULOG
		ulog(ULOG_LAN9311_RX_DROP,nb_dropped);
#endif
		serviced = IRQ_HANDLED;
	}

	if (intsts & inten & INT_STS_TDFA_) {
		temp = smsc_lan9311_reg_read(pdata, FIFO_INT);
		temp |= FIFO_INT_TX_AVAIL_LEVEL_;
		smsc_lan9311_reg_write(temp, pdata, FIFO_INT);
		smsc_lan9311_reg_write(INT_STS_TDFA_, pdata, INT_STS);
		netif_wake_queue(dev);
		serviced = IRQ_HANDLED;
	}

	if (unlikely(intsts & inten & INT_STS_RXSTOP_INT_)) {
		/* Called when there is a multicast update scheduled and
		 * it is now safe to complete the update */
		SMSC_DEBUG("RX Stop interrupt");
		temp = smsc_lan9311_reg_read(pdata, INT_EN);
		temp &= (~INT_EN_RXSTOP_INT_EN_);
		smsc_lan9311_reg_write(temp, pdata, INT_EN);
		smsc_lan9311_reg_write(INT_STS_RXSTOP_INT_, pdata, INT_STS);
		smsc911x_rx_multicast_update_workaround(pdata);
		serviced = IRQ_HANDLED;
	}

	if (unlikely(intsts & inten & INT_STS_TXSTOP_INT_)) {
		/* Called when the emitter stopped */
		SMSC_DEBUG("smsc LAN9311 - TX Stop interrupt");
		smsc_lan9311_reg_write(INT_STS_TXSTOP_INT_, pdata, INT_STS);
		serviced = IRQ_HANDLED;
	}

	if (unlikely(intsts & inten & INT_STS_SW_INT_)) {
		temp = smsc_lan9311_reg_read(pdata, INT_EN);
		temp &= (~INT_EN_SW_INT_EN_);
		smsc_lan9311_reg_write(temp, pdata, INT_EN);
		smsc_lan9311_reg_write(INT_STS_SW_INT_, pdata, INT_STS);
		pdata->software_irq_signal = 1;
		smp_wmb();
		serviced = IRQ_HANDLED;
	}
	SMSC_DEBUG("Exit");

	return serviced;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
void smsc911x_poll_controller(struct net_device *dev)
{
	disable_irq(dev->irq);
	smsc_lan9311_irqhandler(0, dev);
	enable_irq(dev->irq);
}
#endif				/* CONFIG_NET_POLL_CONTROLLER */

/* Standard ioctls for mii-tool */
static int smsc911x_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct smsc_lan9311_data *pdata=netdev_priv(dev);
	struct mii_ioctl_data *data;

	LOCK_API;
	data = if_mii(ifr);
	SMSC_DEBUG("Enter");
	SMSC_TRACE("ioctl cmd 0x%x", cmd);
	switch (cmd) {
	case SIOCGMIIPHY:
		data->phy_id = pdata->mii.phy_id;
		UNLOCK_API;
		SMSC_DEBUG("Exit");
		return 0;
	case SIOCGMIIREG:
		data->val_out = smsc_lan9311_phy_read_lock(pdata, virt_phy_addr, data->reg_num);
		UNLOCK_API;
		SMSC_DEBUG("Exit");
		return 0;
	case SIOCSMIIREG:
		smsc_lan9311_phy_write_lock(pdata, virt_phy_addr, data->reg_num, data->val_in);
		UNLOCK_API;
		SMSC_DEBUG("Exit");
		return 0;
	}
	SMSC_TRACE("unsupported ioctl cmd");
	UNLOCK_API;
	SMSC_DEBUG("Exit");
	return -1;
}

static int
smsc911x_ethtool_getsettings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	int ret;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	cmd->maxtxpkt = 1;
	cmd->maxrxpkt = 1;
	ret=mii_ethtool_gset(&pdata->mii, cmd);

	return ret;
}

static int
smsc911x_ethtool_setsettings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	int ret;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	ret=mii_ethtool_sset(&pdata->mii, cmd);

	return ret; 
}

static void smsc911x_ethtool_getdrvinfo(struct net_device *dev,
					struct ethtool_drvinfo *info)
{
	strncpy(info->driver, SMSC_CHIPNAME, sizeof(info->driver));
	strncpy(info->version, SMSC_DRV_VERSION, sizeof(info->version));
	strncpy(info->bus_info, dev->class_dev.dev->bus_id,sizeof(info->bus_info));
}

static int smsc911x_ethtool_nwayreset(struct net_device *dev)
{
	int ret;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	ret = mii_nway_restart(&pdata->mii);

	return ret;
}

static u32 smsc911x_ethtool_getmsglevel(struct net_device *dev)
{
	int ret;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	LOCK_API;
	ret= pdata->msg_enable;
	UNLOCK_API;

	return ret;
}

static void smsc911x_ethtool_setmsglevel(struct net_device *dev, u32 level)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	LOCK_API;
	pdata->msg_enable = level;
	UNLOCK_API;
}

static int smsc911x_ethtool_getregslen(struct net_device *dev)
{
	return (((E2P_CMD - ID_REV) / 4 + 1) + (WUCSR - MAC_CR) + 1 + 32) *
	    sizeof(u32);
}

static void
smsc911x_ethtool_getregs(struct net_device *dev, struct ethtool_regs *regs,
			 void *buf)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	unsigned long flags;
	unsigned int i;
	unsigned int j = 0;
	u32 *data = buf;

	LOCK_API;
	regs->version = pdata->idrev;
	LOCK_HW;
	for (i = ID_REV; i <= E2P_CMD; i += (sizeof(u32)))
		data[j++] = smsc_lan9311_reg_read(pdata, i);

	for (i = MAC_CR; i <= WUCSR; i++)
		data[j++] = smsc_lan9311_mac_read(pdata, i);
	for (i = 0; i <= 31; i++)
		data[j++] = smsc_lan9311_phy_read(pdata, virt_phy_addr, i);
	UNLOCK_HW;
	UNLOCK_API;
}

static void smsc911x_eeprom_enable_access(struct smsc_lan9311_data *pdata)
{
#ifndef CONFIG_SMSC_LAN9311
    unsigned int temp;
    unsigned long flags;

    LOCK_HW;
    temp = smsc_lan9311_reg_read(pdata, GPIO_CFG);
	temp &= ~GPIO_CFG_EEPR_EN_;
	smsc_lan9311_reg_write(temp, pdata, GPIO_CFG);
	msleep(1);
    UNLOCK_HW;
#endif
}

static int smsc911x_eeprom_send_cmd(struct smsc_lan9311_data *pdata, u32 op)
{
	int timeout = 100;
	u32 e2cmd;

	SMSC_TRACE("op 0x%08x", op);
	if (smsc_lan9311_reg_read_lock(pdata, E2P_CMD) & E2P_CMD_EPC_BUSY_) {
		SMSC_WARNING("Busy at start");
		return -EBUSY;
	}

	e2cmd = op | E2P_CMD_EPC_BUSY_;
	smsc_lan9311_reg_write_lock(e2cmd, pdata, E2P_CMD);

	do {
		msleep(1);
		e2cmd = smsc_lan9311_reg_read_lock(pdata, E2P_CMD);
	} while ((e2cmd & E2P_CMD_EPC_BUSY_) && (timeout--));

	if (!timeout) {
		SMSC_TRACE("TIMED OUT");
		return -EAGAIN;
	}

	if (e2cmd & E2P_CMD_EPC_TIMEOUT_) {
		SMSC_TRACE("Error occured during eeprom operation");
		return -EINVAL;
	}

	return 0;
}

static int smsc911x_eeprom_read_location(struct smsc_lan9311_data *pdata,
					 u8 address, u8 *data)
{
	u32 op = E2P_CMD_EPC_CMD_READ_ | address;
	int ret;

	SMSC_TRACE("address 0x%x", address);
	ret = smsc911x_eeprom_send_cmd(pdata, op);

	if (!ret)
		data[address] = smsc_lan9311_reg_read_lock(pdata, E2P_DATA);

	return ret;
}

static int smsc911x_eeprom_write_location(struct smsc_lan9311_data *pdata,
					  u8 address, u8 data)
{
	u32 op = E2P_CMD_EPC_CMD_ERASE_ | address;
	int ret;

	SMSC_TRACE("address 0x%x, data 0x%x", address, data);
	ret = smsc911x_eeprom_send_cmd(pdata, op);

	if (!ret) {
		op = E2P_CMD_EPC_CMD_WRITE_ | address;
		smsc_lan9311_reg_write_lock((u32)data, pdata, E2P_DATA);
		ret = smsc911x_eeprom_send_cmd(pdata, op);
	}

	return ret;
}

static int smsc911x_ethtool_get_eeprom_len(struct net_device *dev)
{
	return SMSC911X_EEPROM_SIZE;
}

static int smsc911x_ethtool_get_eeprom(struct net_device *dev,
				       struct ethtool_eeprom *eeprom, u8 *data)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);
	u8 eeprom_data[SMSC911X_EEPROM_SIZE];
	int len;
	int i;

	LOCK_API;
	smsc911x_eeprom_enable_access(pdata);

	len = min(eeprom->len, SMSC911X_EEPROM_SIZE);
	for (i = 0; i < len; i++) {
		int ret = smsc911x_eeprom_read_location(pdata, i, eeprom_data);
		if (ret < 0) {
			eeprom->len = 0;
			return ret;
		}
	}

	memcpy(data, &eeprom_data[eeprom->offset], len);
	eeprom->len = len;
	UNLOCK_API;
	return 0;
}

static int smsc911x_ethtool_set_eeprom(struct net_device *dev,
				       struct ethtool_eeprom *eeprom, u8 *data)
{
	int ret;
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	LOCK_API;
	smsc911x_eeprom_enable_access(pdata);
	smsc911x_eeprom_send_cmd(pdata, E2P_CMD_EPC_CMD_EWEN_);
	ret = smsc911x_eeprom_write_location(pdata, eeprom->offset, *data);
	smsc911x_eeprom_send_cmd(pdata, E2P_CMD_EPC_CMD_EWDS_);

	/* Single byte write, according to man page */
	eeprom->len = 1;
	UNLOCK_API;

	return ret;
}

static struct ethtool_ops smsc911x_ethtool_ops = {
	.get_settings = smsc911x_ethtool_getsettings,
	.set_settings = smsc911x_ethtool_setsettings,
	.get_link = ethtool_op_get_link,
	.get_drvinfo = smsc911x_ethtool_getdrvinfo,
	.nway_reset = smsc911x_ethtool_nwayreset,
	.get_msglevel = smsc911x_ethtool_getmsglevel,
	.set_msglevel = smsc911x_ethtool_setmsglevel,
	.get_regs_len = smsc911x_ethtool_getregslen,
	.get_regs = smsc911x_ethtool_getregs,
	.get_eeprom_len = smsc911x_ethtool_get_eeprom_len,
	.get_eeprom = smsc911x_ethtool_get_eeprom,
	.set_eeprom = smsc911x_ethtool_set_eeprom,
};

/* Initializing private device structures */
static int smsc911x_init(struct net_device *dev)
{
	struct smsc_lan9311_data *pdata = netdev_priv(dev);

	SMSC_TRACE("Driver Parameters:");
	SMSC_TRACE("LAN base: 0x%08lX", (unsigned long)pdata->ioaddr);
	SMSC_TRACE("IRQ: %d", dev->irq);
	SMSC_TRACE("PHY will be autodetected.");

	/*ag_temp_array[0]=0x12;
	ag_temp_array[1]=0x34;
	ag_temp_array[2]=0x56;
	ag_temp_array[3]=0x78;
	ag_temp_array[4]=0x9A;
	ag_temp_array[5]=0xBC;
	ag_temp_array[6]=0xDE;
	ag_temp_array[7]=0xF0;*/

	if (pdata->ioaddr == 0) {
		SMSC_WARNING("pdata->ioaddr: 0x00000000");
		return -ENODEV;
	}

	/* Default generation to zero (all workarounds apply) */
	pdata->generation = 0;

	pdata->idrev = smsc_lan9311_reg_read(pdata, ID_REV);
	if (((pdata->idrev >> 16) & 0xFFFF) == (pdata->idrev & 0xFFFF)) {
		SMSC_WARNING("idrev top 16 bits equal to bottom 16 bits, "
			     "idrev: 0x%08X", pdata->idrev);
		SMSC_TRACE("This may mean the chip is set for 32 bit while "
			   "the bus is reading as 16 bit");
		return -ENODEV;
	}
	switch (pdata->idrev & 0xFFFF0000) {
	case 0x01180000:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_TRACE("LAN9118 Beacon identified, idrev: 0x%08X",
				   pdata->idrev);
			pdata->generation = 0;
			break;
		case 1UL:
			SMSC_TRACE
			    ("LAN9118 Concord A0 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 1;
			break;
		case 2UL:
			SMSC_TRACE
			    ("LAN9118 Concord A1 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		default:
			SMSC_TRACE
			    ("LAN9118 Concord A1 identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		}
		break;

	case 0x01170000:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_TRACE("LAN9117 Beacon identified, idrev: 0x%08X",
				   pdata->idrev);
			pdata->generation = 0;
			break;
		case 1UL:
			SMSC_TRACE
			    ("LAN9117 Concord A0 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 1;
			break;
		case 2UL:
			SMSC_TRACE
			    ("LAN9117 Concord A1 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		default:
			SMSC_TRACE
			    ("LAN9117 Concord A1 identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		}
		break;

	case 0x01160000:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_WARNING("LAN911x not identified, line %d, idrev: 0x%08X",
				     __LINE__,pdata->idrev);
			return -ENODEV;
		case 1UL:
			SMSC_TRACE
			    ("LAN9116 Concord A0 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 1;
			break;
		case 2UL:
			SMSC_TRACE
			    ("LAN9116 Concord A1 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		default:
			SMSC_TRACE
			    ("LAN9116 Concord A1 identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		}
		break;

	case 0x01150000:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_WARNING("LAN911x not identified, line %d, idrev: 0x%08X",
				     __LINE__,pdata->idrev);
			return -ENODEV;
		case 1UL:
			SMSC_TRACE
			    ("LAN9115 Concord A0 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 1;
			break;
		case 2UL:
			SMSC_TRACE
			    ("LAN9115 Concord A1 identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		default:
			SMSC_TRACE
			    ("LAN9115 Concord A1 identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 2;
			break;
		}
		break;

	case 0x118A0000UL:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_TRACE
			    ("LAN9218 Boylston identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		default:
			SMSC_TRACE
			    ("LAN9218 Boylston identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		}
		break;

	case 0x117A0000UL:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_TRACE
			    ("LAN9217 Boylston identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		default:
			SMSC_TRACE
			    ("LAN9217 Boylston identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		}
		break;

	case 0x116A0000UL:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_TRACE
			    ("LAN9216 Boylston identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		default:
			SMSC_TRACE
			    ("LAN9216 Boylston identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		}
		break;

	case 0x115A0000UL:
		switch (pdata->idrev & 0x0000FFFFUL) {
		case 0UL:
			SMSC_TRACE
			    ("LAN9215 Boylston identified, idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		default:
			SMSC_TRACE
			    ("LAN9215 Boylston identified (NEW), idrev: 0x%08X",
			     pdata->idrev);
			pdata->generation = 3;
			break;
		}
		break;
	case 0x93110000UL:
		SMSC_WARNING("LAN9311 identified, idrev: 0x%08X",
				   pdata->idrev);
		pdata->generation = 2; 
		break;
	default:
		SMSC_WARNING("LAN911x not identified, line %d,idrev: 0x%08X",
			     __LINE__,pdata->idrev);
		return -ENODEV;
	}

	if (pdata->generation == 0)
		SMSC_WARNING("This driver is not intended "
			     "for this chip revision");

    /* Reset the LAN911x */
    if (smsc911x_soft_reset(pdata))
            return -ENODEV;

    /* Disable all interrupt sources until we bring the device up */
    smsc_lan9311_reg_write(0, pdata, INT_EN);

	ether_setup(dev);
	dev->open = smsc_lan9311_open;
	dev->stop = smsc911x_stop;
	dev->hard_start_xmit = smsc_lan9311_hard_start_xmit;
	dev->get_stats = smsc911x_get_stats;
	dev->set_multicast_list = smsc911x_set_multicast_list;
	dev->flags |= IFF_MULTICAST;
	dev->do_ioctl = smsc911x_do_ioctl;
	dev->poll = smsc911x_poll;
	dev->weight = 64;
	dev->ethtool_ops = &smsc911x_ethtool_ops;

#ifdef CONFIG_NET_POLL_CONTROLLER
	dev->poll_controller = smsc911x_poll_controller;
#endif				/* CONFIG_NET_POLL_CONTROLLER */

	pdata->mii.phy_id_mask = 0x1f;
	pdata->mii.reg_num_mask = 0x1f;
	pdata->mii.force_media = 0;
	pdata->mii.full_duplex = 0;
	pdata->mii.dev = dev;
	pdata->mii.mdio_read = smsc_lan9311_mdio_read;
	pdata->mii.mdio_write = smsc_lan9311_mdio_write;

	pdata->msg_enable = NETIF_MSG_LINK;

	return 0;
}

static int smsc911x_drv_remove(struct platform_device *pdev)
{
	struct net_device *dev;
	struct smsc_lan9311_data *pdata;
	struct resource *res;

	dev = platform_get_drvdata(pdev);
	BUG_ON(!dev);
	pdata = netdev_priv(dev);
	BUG_ON(!pdata);
	BUG_ON(!pdata->ioaddr);

	SMSC_TRACE("Stopping driver.");

	//flush all pending works
	flush_workqueue(pdata->link_work_queue);
	//terminate work queue
	destroy_workqueue(pdata->link_work_queue);

#ifdef USE_MXC_DMA
	//set active DMA variable to 0
	//pdata->dma_active_tx=0;
	pdata->dma_active_rx=0;
	//pause, discard all runing DMA requests
	//mxc_dma_pause(pdata->dma_tx_channel);
	mxc_dma_pause(pdata->dma_rx_channel);	
	//disable DMA channels
	//mxc_dma_disable(pdata->dma_tx_channel);
	mxc_dma_disable(pdata->dma_rx_channel);
	//destroy all the DMA channels
	//mxc_dma_free(pdata->dma_tx_channel);
	mxc_dma_free(pdata->dma_rx_channel);
#endif //#ifdef USE_MXC_DMA
#ifdef USE_TASKLET
	tasklet_kill(&pdata->tasklet_rx);
	//tasklet_kill(&pdata->tasklet_tx);
#endif //#ifdef USE_TASKLET
	platform_set_drvdata(pdev, NULL);
	unregister_netdev(dev);
	free_irq(dev->irq, dev);
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
                                       "smsc_lan9311");
	if (!res)
		platform_get_resource(pdev, IORESOURCE_MEM, 0);

	release_mem_region(res->start, res->end - res->start);

	iounmap(pdata->ioaddr);

	free_netdev(dev);

	return 0;
}

static int smsc911x_drv_probe(struct platform_device *pdev)
{
	struct net_device *dev;
	struct smsc_lan9311_data *pdata;
	struct resource *res;
	unsigned int intcfg = 0;
	int res_size;
	int retval;
	unsigned int temp;

	SMSC_TRACE("%s: Driver version %s.", SMSC_CHIPNAME,
		SMSC_DRV_VERSION);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "smsc_lan9311");
	if (!res)
	{
		SMSC_TRACE("Get resources by default");
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	}
	if (!res) {
		SMSC_WARNING("%s: Could not allocate resource.",
		       SMSC_CHIPNAME);
		retval = -ENODEV;
		goto out_0;
	}
	res_size = res->end - res->start;

	if (!request_mem_region(res->start, res_size, SMSC_CHIPNAME)) {
		retval = -EBUSY;
		goto out_0;
	}

	dev = alloc_etherdev(sizeof(struct smsc_lan9311_data));
	if (!dev) {
		SMSC_WARNING("%s: Could not allocate device.",
		       SMSC_CHIPNAME);
		retval = -ENOMEM;
		goto out_release_io_1;
	}

	SET_MODULE_OWNER(dev);
	SET_NETDEV_DEV(dev, &pdev->dev);

	pdata = netdev_priv(dev);
#ifdef CONFIG_MACH_MX31HSV1
   if (get_board_type()>BOARD_TYPE_V1_HOMESCREEN_GENERIC)
   {
      SMSC_TRACE("Homescreen board > V1 detected");
      dev->irq = platform_get_irq(pdev, 1);
   }
   else
   {
      SMSC_TRACE("Homescreen board V1 detected");
      dev->irq = platform_get_irq(pdev, 0);
   }
#endif

   SMSC_TRACE("irq=%d",dev->irq);

	pdata->ioaddr = ioremap_nocache(res->start, res_size);

	/* copy config parameters across if present, otherwise pdata
	 * defaults to zeros */
	if (pdev->dev.platform_data) {
		struct smsc_lan9311_platform_config *config = pdev->dev.platform_data;
		SMSC_TRACE("pdev->dev.platform_data != NULL");
		pdata->irq_polarity = config->irq_polarity;
		pdata->irq_type  = config->irq_type;
	}

	SMSC_TRACE("irq_polarity=%d",pdata->irq_polarity);
	SMSC_TRACE("irq_type=%d",pdata->irq_type);

	if (pdata->ioaddr == NULL) {
		SMSC_WARNING("Error smsc911x base address invalid");
		retval = -ENOMEM;
		goto out_free_netdev_2;
	}

	if ((retval = smsc911x_init(dev)) < 0)
		goto out_unmap_io_3;

	//initialize API mutex
	mutex_init(&pdata->api_lock);
	//initialize harware access spinlock
	spin_lock_init(&pdata->hw_lock);
	//create the link work queue
	pdata->link_work_queue=create_singlethread_workqueue("LAN9311 link");
	//init work
	INIT_WORK(&pdata->link_work,link_work,(void *)dev);

#ifdef USE_MXC_DMA
	//initialize the dma channels
	/*pdata->dma_tx_channel = mxc_dma_request(MXC_DMA_ETH_TX, "LAN9311 TX DMA");
	if (pdata->dma_tx_channel<0)
	{
		SMSC_WARNING("Could not allocate DMA TX");
	}

	if (mxc_dma_callback_set(pdata->dma_tx_channel, (mxc_dma_callback_t)smsc_dma_clbk_tx,(void *)dev)!= 0) {
		mxc_dma_free(pdata->dma_tx_channel);
		SMSC_WARNING("Error when setting DMA TX callback\n");
	}*/

	pdata->dma_rx_channel = mxc_dma_request(MXC_DMA_ETH_RX, "LAN9311 RX DMA");
	if (pdata->dma_rx_channel<0)
	{
		SMSC_WARNING("Could not allocate DMA RX");
	}

	if (mxc_dma_callback_set(pdata->dma_rx_channel, (mxc_dma_callback_t)smsc_dma_clbk_rx,(void *)dev)!= 0) {
		mxc_dma_free(pdata->dma_rx_channel);
		SMSC_WARNING("Error when setting DMA RX callback\n");
	}
	//pdata->dma_tx_nb_bd=MAX_DMA_NB_BD;
	pdata->dma_rx_nb_bd=MAX_DMA_NB_BD;
	//pdata->dma_tx_last_req_index=0;
	pdata->dma_rx_last_req_index=0;
	//pdata->dma_tx_last_completed_index=0;
	pdata->dma_rx_last_completed_index=0;
#endif //#ifdef USE_MXC_DMA
#ifdef USE_TASKLET
	tasklet_init(&pdata->tasklet_rx, tasklet_rx,(unsigned long)dev);
	pdata->tasklet_clbk_rx=smsc_tasklet_clbk_rx;
	//tasklet_init(&pdata->tasklet_tx, tasklet_tx,(unsigned long)dev);
	//pdata->tasklet_clbk_tx=smsc_tasklet_clbk_tx;
	//pdata->dma_tx_nb_bd=MAX_DMA_NB_BD;
	pdata->dma_rx_nb_bd=MAX_DMA_NB_BD;
	//pdata->dma_tx_last_req_index=0;
	pdata->dma_rx_last_req_index=0;
	//pdata->dma_tx_last_completed_index=0;
	pdata->dma_rx_last_completed_index=0;
	//pdata->tasklet_tx_index=0;
	pdata->tasklet_rx_index=0;
#endif //#ifdef USE_TASKLET
	/* configure irq polarity and type before connecting isr */
	if (pdata->irq_polarity)
		intcfg |= INT_CFG_IRQ_POL_;

	if (pdata->irq_type)
		intcfg |= INT_CFG_IRQ_TYPE_;

	temp = smsc_lan9311_reg_read(pdata, BYTE_TEST);
	SMSC_TRACE("Endianness test, BYTE_TEST = 0x%x", temp);

	SMSC_TRACE("0x%x written into INT_CFG", intcfg);
	smsc_lan9311_reg_write(intcfg, pdata, INT_CFG);

	set_irq_type(dev->irq, IRQF_TRIGGER_LOW);	
	retval = request_irq(dev->irq, smsc_lan9311_irqhandler,0,
			     SMSC_CHIPNAME, dev);
	if (retval) {
		SMSC_WARNING("Unable to claim requested irq: %d", dev->irq);
		goto out_unmap_io_3;
	}

	platform_set_drvdata(pdev, dev);
	retval = register_netdev(dev);
	if (retval) {
		SMSC_WARNING("Error %i registering device", retval);
		goto out_unset_drvdata_4;
	} else {
		SMSC_TRACE("Network interface: \"%s\"", dev->name);
	}

	return 0;

out_unset_drvdata_4:
	platform_set_drvdata(pdev, NULL);
	free_irq(dev->irq, dev);
out_unmap_io_3:
	iounmap(pdata->ioaddr);
out_free_netdev_2:
	free_netdev(dev);
out_release_io_1:
	release_mem_region(res->start, res->end - res->start);
out_0:
	return retval;
}

static struct platform_driver smsc_lan9311_driver = {
	.driver = {
		.name = SMSC_CHIPNAME,
	},
	.suspend = 0,           /* TODO: Add suspend routine */
	.resume = 0,            /* TODO: Add resume routine */
	.probe = smsc911x_drv_probe,
	.remove = smsc911x_drv_remove,


};


/* Entry point for loading the module */
static int __init smsc911x_init_module(void)
{
	int ret;
	ret = platform_driver_register(&smsc_lan9311_driver);
	return ret;
}

/* entry point for unloading the module */
static void __exit smsc911x_cleanup_module(void)
{
	platform_driver_unregister(&smsc_lan9311_driver);
}

module_init(smsc911x_init_module);
module_exit(smsc911x_cleanup_module);
