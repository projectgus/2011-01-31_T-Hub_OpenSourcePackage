/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 06/08/2007
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
 */

/*
 * Based on sa1100_ir.c - Copyright 2000-2001 Russell King
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * @file mxc_sir.c
 *
 * @brief Driver for the Freescale Semiconductor MXC IrDA.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <net/irda/irda.h>
#include <net/irda/wrapper.h>
#include <net/irda/irda_device.h>

#include <asm/irq.h>
#include <asm/dma.h>
#include <asm/hardware.h>
#include <asm/arch/mxc_uart.h>
#include <asm/arch/clock.h>
#include "mxc_sir.h"

//#define MXC_IRDA_DEBUG
#ifdef	MXC_IRDA_DEBUG
# define mxcirda_debug(fmt, arg...) \
	printk("%s: (%d) " fmt "\n", __FUNCTION__, __LINE__, ##arg)
#else
# define mxcirda_debug(fmt, arg...) \
	do { } while(0)
#endif

#define IS_SIR(mi)	( (mi)->speed <= 115200 )
#define IS_MIR(mi)	( (mi)->speed < 4000000 && (mi)->speed >= 576000 )
#define IS_FIR(mi)	( (mi)->speed >= 4000000 )

#define IRDA_FRAME_SIZE_LIMIT   2047
#define UART_BUFF_SIZE 		14384

#define UART4_UFCR_TXTL  	16
#define UART4_UFCR_RXTL 	16

/*
 * This structure is a way for the low level driver to define their own
 * mxc_irda structure. This structure includes SK buffers, DMA buffers.
 * and has other elements that are specifically required by this driver.
 */
struct mxc_irda {
	/*
	 * This keeps track of device is running or not
	 */
	unsigned char open;

	/*
	 * This holds current IrDA communication speed
	 */
	int speed;

	/*
	 * This holds IrDA communication speed for next packet
	 */
	int newspeed;

	/*
	 * SK buffer for transmitter
	 */
	struct sk_buff *txskb;

	/*
	 * SK buffer for receiver
	 */
	struct sk_buff *rxskb;

	/*
	 * DMA address for transmitter
	 */
	dma_addr_t dma_rx_buff_phy;

	/*
	 * DMA address for receiver
	 */
	dma_addr_t dma_tx_buff_phy;

	/*
	 * DMA Transmit buffer length
	 */
	unsigned int dma_tx_buff_len;

	/*
	 * IrDA network device statistics
	 */
	struct net_device_stats stats;

	/*
	 * The device structure used to get IrDA information
	 */
	struct device *dev;

	/*
	 * Resource structure for UART, which will maintain base addresses and IRQs.
	 */
	struct resource *uart_res;

	/*
	 * Base address of UART, used in readl and writel.
	 */
	void *uart_base;

	/*
	 * UART IRQ number.
	 */
	int uart_irq;

	/*
	 * UART clock needed for baud rate calculations
	 */
	unsigned int uartclk;

	/*
	 * IrLAP layer instance
	 */
	struct irlap_cb *irlap;

	/*
	 * Driver supported baudrate capabilities
	 */
	struct qos_info qos;

	/*
	 * Temporary transmit buffer used by the driver
	 */
	iobuff_t tx_buff;

	/*
	 * Temporary receive buffer used by the driver
	 */
	iobuff_t rx_buff;

	/*
	 * This holds the power management status of this module.
	 */
	int suspend;

};

static int max_rate = 115200;
static int uart_ir_mux = 1;
static struct clk *irda_clk = NULL;
static struct clk *per_clk = NULL;


extern void gpio_irda_active(void);
extern void gpio_irda_inactive(void);

/*
 * This function is called to set the IrDA communications speed.
 *
 * @param   si     IrDA specific structure.
 * @param   speed  new Speed to be configured for.
 *
 * @return  The function returns 0 on success and a non-zero value on
 *          failure.
 */
static int mxc_irda_set_speed(struct mxc_irda *si, int speed)
{
	unsigned long flags;
	u32 num, denom, baud;
	u16 cr;
	int ret;

	dev_dbg(si->dev, "Speed:%d\n", speed);
	switch (speed) {
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
		dev_dbg(si->dev, "Starting SIR\n");
		local_irq_save(flags);

		/* Disable Tx and Rx */
		cr = readl(si->uart_base + MXC_UARTUCR2);
		cr &= ~(MXC_UARTUCR2_RXEN | MXC_UARTUCR2_TXEN);
		writel(cr, si->uart_base + MXC_UARTUCR2);

		baud = speed;
		num = baud / 100 - 1;
		denom = si->uartclk / 1600 - 1;
		if ((denom < 65536) && (si->uartclk > 1600)) {
			writel(num, si->uart_base + MXC_UARTUBIR);
			writel(denom, si->uart_base + MXC_UARTUBMR);
		}

		si->speed = speed;

		writel(0xFFFF, si->uart_base + MXC_UARTUSR1);
		writel(0xFFFF, si->uart_base + MXC_UARTUSR2);

		/* Enable Receive Overrun and Data Ready interrupts. */
		cr = readl(si->uart_base + MXC_UARTUCR4);
		cr |= (MXC_UARTUCR4_OREN | MXC_UARTUCR4_DREN);// | MXC_UARTUCR4_ENIRI);
		writel(cr, si->uart_base + MXC_UARTUCR4);

		cr = readl(si->uart_base + MXC_UARTUCR2);
		cr |= (MXC_UARTUCR2_RXEN | MXC_UARTUCR2_TXEN);
		writel(cr, si->uart_base + MXC_UARTUCR2);

		local_irq_restore(flags);
		ret = 0;
		break;
	default:
		dev_err(si->dev, "Speed %d not supported by IrDA\n", speed);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*
 * This is the SIR transmit routine.
 *
 * @param   si     IrDA specific structure.
 *
 * @param   dev     pointer to the net_device structure
 *
 * @return  The function returns 0 on success and a non-zero value on
 *          failure.
 */
static int mxc_irda_sir_txirq(struct mxc_irda *si, struct net_device *dev)
{
	u16 sr1, sr2, cr;
	u16 status;

	sr1 = readl(si->uart_base + MXC_UARTUSR1);
	sr2 = readl(si->uart_base + MXC_UARTUSR2);
	cr = readl(si->uart_base + MXC_UARTUCR2);

	/*
	 * Echo cancellation for IrDA Transmit chars
	 * Disable the receiver and enable Transmit complete.
	 */
	cr &= ~MXC_UARTUCR2_RXEN;
	writel(cr, si->uart_base + MXC_UARTUCR2);
	cr = readl(si->uart_base + MXC_UARTUCR4);
	cr |= MXC_UARTUCR4_TCEN;
	writel(cr, si->uart_base + MXC_UARTUCR4);

	while ((sr1 & MXC_UARTUSR1_TRDY) && si->tx_buff.len) {

		writel(*si->tx_buff.data++, si->uart_base + MXC_UARTUTXD);
		si->tx_buff.len -= 1;
		sr1 = readl(si->uart_base + MXC_UARTUSR1);
	}

	if (si->tx_buff.len == 0) {
		si->stats.tx_packets++;
		si->stats.tx_bytes += si->tx_buff.data - si->tx_buff.head;

		/*Yoohoo...we are done...Lets stop Tx */
		cr = readl(si->uart_base + MXC_UARTUCR1);
		cr &= ~MXC_UARTUCR1_TRDYEN;
		writel(cr, si->uart_base + MXC_UARTUCR1);

		do {
			status = readl(si->uart_base + MXC_UARTUSR2);
		} while (!(status & MXC_UARTUSR2_TXDC));

		if (si->newspeed) {
			mxc_irda_set_speed(si, si->newspeed);
			si->newspeed = 0;
		}
		/* I'm hungry! */
		netif_wake_queue(dev);

		/* Is the transmit complete to reenable the receiver? */
		if (status & MXC_UARTUSR2_TXDC) {

			cr = readl(si->uart_base + MXC_UARTUCR2);
			cr |= MXC_UARTUCR2_RXEN;
			writel(cr, si->uart_base + MXC_UARTUCR2);
			/* Disable the Transmit complete interrupt bit */
			cr = readl(si->uart_base + MXC_UARTUCR4);
			cr &= ~MXC_UARTUCR4_TCEN;
			writel(cr, si->uart_base + MXC_UARTUCR4);
		}
	}

	return 0;
}

/*
 * This is the SIR receive routine.
 *
 * @param   si     IrDA specific structure.
 *
 * @param   dev     pointer to the net_device structure
 *
 * @return  The function returns 0 on success and a non-zero value on
 *          failure.
 */
static int mxc_irda_sir_rxirq(struct mxc_irda *si, struct net_device *dev)
{
	u16 data, status;
	volatile u16 sr2;

	sr2 = readl(si->uart_base + MXC_UARTUSR2);
	while ((sr2 & MXC_UARTUSR2_RDR) == 1) {
		data = readl(si->uart_base + MXC_UARTURXD);
		status = data & 0xf400;
		if (status & MXC_UARTURXD_ERR) {
			dev_err(si->dev, "Receive an incorrect data =0x%x.\n",
				data);
			si->stats.rx_errors++;
			if (status & MXC_UARTURXD_OVRRUN) {
				si->stats.rx_fifo_errors++;
				dev_err(si->dev, "Rx overrun.\n");
			}
			if (status & MXC_UARTURXD_FRMERR) {
				si->stats.rx_frame_errors++;
				dev_err(si->dev, "Rx frame error.\n");
			}
			if (status & MXC_UARTURXD_PRERR) {
				dev_err(si->dev, "Rx parity error.\n");
			}
			/* Other: it is the Break char.
			 * Do nothing for it. throw out the data.
			 */
			async_unwrap_char(dev, &si->stats, &si->rx_buff,
					  (data & 0xFF));
		} else {
			/* It is correct data. */
			data &= 0xFF;
			async_unwrap_char(dev, &si->stats, &si->rx_buff, data);

			dev->last_rx = jiffies;
		}
		sr2 = readl(si->uart_base + MXC_UARTUSR2);

		writel(0xFFFF, si->uart_base + MXC_UARTUSR1);
		writel(0xFFFF, si->uart_base + MXC_UARTUSR2);
	}	/* while */
	return 0;

}

/*
 * IrDA interrupt service routine
 */
static irqreturn_t mxc_irda_irq(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct mxc_irda *si = netdev_priv(dev);

	//if (readl(si->uart_base + MXC_UARTUCR2) & MXC_UARTUCR2_RXEN) {
	if (readl(si->uart_base + MXC_UARTUSR2) & MXC_UARTUSR2_RDR) {
		mxc_irda_sir_rxirq(si, dev);
	}
	if ((readl(si->uart_base + MXC_UARTUCR1) & MXC_UARTUCR1_TRDYEN) &&
	    (readl(si->uart_base + MXC_UARTUSR1) & MXC_UARTUSR1_TRDY)) {
		mxc_irda_sir_txirq(si, dev);
	}

	return IRQ_HANDLED;
}

/*
 * This function is called by Linux IrDA network subsystem to
 * transmit the Infrared data packet. The TX DMA channel is configured
 * to transfer SK buffer data to IrDA TX FIFO along with DMA transfer
 * completion routine.
 *
 * @param   skb   The packet that is queued to be sent
 * @param   dev   net_device structure.
 *
 * @return  The function returns 0 on success and a negative value on
 *          failure.
 */
static int mxc_irda_hard_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mxc_irda *si = netdev_priv(dev);
	int speed = irda_get_next_speed(skb);
	u16 cr;

	/*
	 * Does this packet contain a request to change the interface
	 * speed?  If so, remember it until we complete the transmission
	 * of this frame.
	 */
	if (speed != si->speed && speed != -1) {
		si->newspeed = speed;
	}

	/* If this is an empty frame, we can bypass a lot. */
	if (skb->len == 0) {
		if (si->newspeed) {
			si->newspeed = 0;
			mxc_irda_set_speed(si, speed);
		}
		dev_kfree_skb(skb);
		return 0;
	}

	if (IS_SIR(si)) {
		/* We must not be transmitting... */
		netif_stop_queue(dev);

		si->tx_buff.data = si->tx_buff.head;
		si->tx_buff.len = async_wrap_skb(skb, si->tx_buff.data,
						 si->tx_buff.truesize);

		/*
		 * Set the tramsmit interrupt enable. This will fire
		 * off an interrupt immediately. Note that we disable
		 * the receiver so we don't get spurious characters
		 * received.
		 */
		cr = readl(si->uart_base + MXC_UARTUCR1);
		cr |= MXC_UARTUCR1_TRDYEN;
		writel(cr, si->uart_base + MXC_UARTUCR1);

		dev_kfree_skb(skb);
	}

	dev->trans_start = jiffies;
	return 0;
}

/*
 * This function handles network interface ioctls passed to this driver..
 *
 * @param   dev   net device structure
 * @param   ifreq user request data
 * @param   cmd   command issued
 *
 * @return  The function returns 0 on success and a non-zero value on
 *           failure.
 */
static int mxc_irda_ioctl(struct net_device *dev, struct ifreq *ifreq, int cmd)
{
	struct if_irda_req *rq = (struct if_irda_req *)ifreq;
	struct mxc_irda *si = netdev_priv(dev);
	int ret = -EOPNOTSUPP;

	switch (cmd) {
		/* This function will be used by IrLAP to change the speed */
	case SIOCSBANDWIDTH:
		dev_dbg(si->dev, "%s:with cmd SIOCSBANDWIDTH\n", __FUNCTION__);
		if (capable(CAP_NET_ADMIN)) {
			/*
			 * We are unable to set the speed if the
			 * device is not running.
			 */
			if (si->open) {
				ret = mxc_irda_set_speed(si, rq->ifr_baudrate);
			} else {
				dev_err(si->dev, "mxc_ir_ioctl: SIOCSBANDWIDTH:\
                                         !netif_running\n");
				ret = 0;
			}
		}
		break;
	case SIOCSMEDIABUSY:
		dev_dbg(si->dev, "%s:with cmd SIOCSMEDIABUSY\n", __FUNCTION__);
		ret = -EPERM;
		if (capable(CAP_NET_ADMIN)) {
			irda_device_set_media_busy(dev, TRUE);
			ret = 0;
		}
		break;
	case SIOCGRECEIVING:
		rq->ifr_receiving =
		    IS_SIR(si) ? si->rx_buff.state != OUTSIDE_FRAME : 0;
		ret = 0;
		break;
	default:
		break;
	}
	return ret;
}

/*
 * Kernel interface routine to get current statistics of the device
 * which includes the number bytes/packets transmitted/received,
 * receive errors, CRC errors, framing errors etc.
 *
 * @param  dev  the net_device structure
 *
 * @return This function returns IrDA network statistics
 */
static struct net_device_stats *mxc_irda_stats(struct net_device *dev)
{
	struct mxc_irda *si = netdev_priv(dev);
	return &si->stats;
}

/*
 * This function enables IrDA port.
 *
 * @param   si  IrDA port specific structure.
 *
 * @return  The function returns 0 on success and a non-zero value on
 *          failure.
 */
static int mxc_irda_startup(struct mxc_irda *si)
{
	unsigned int num, denom, baud, ufcr = 0;
	unsigned int per_clk_val;
	unsigned int cr;
	int d = 1;
	int ret;

	/* Clear Status Registers 1 and 2 */
	writel(0xFFFF, si->uart_base + MXC_UARTUSR1);
	writel(0xFFFF, si->uart_base + MXC_UARTUSR2);

	/* Configure the IOMUX for the UART */
	gpio_irda_active();
#ifdef CONFIG_ARCH_MX31
	irda_clk = clk_get(NULL, "uart_baud.1");
#else
	irda_clk = clk_get(NULL, "uart_clk.2");
	if (IS_ERR(irda_clk)) {
		ret = PTR_ERR(irda_clk);
		irda_clk = NULL;
		return ret;
	}
#endif
        clk_enable(irda_clk);
	per_clk = clk_get(NULL, "per_clk.0");
#ifndef CONFIG_ARCH_MX31
	if (IS_ERR(per_clk)) {
		ret = PTR_ERR(per_clk);
		per_clk = NULL;
		return ret;
	}
#endif
	per_clk_val = clk_get_rate(per_clk);
	mxcirda_debug("per_clk_val = %d", per_clk_val);
	baud = per_clk_val / 16;
	if (baud > MAX_UART_BAUDRATE) {
		baud = MAX_UART_BAUDRATE;
		d = per_clk_val / ((baud * 16) + 1000);
		if (d > 6) {
			d = 6;
		}
	}
	si->uartclk = per_clk_val / d;
	writel(si->uartclk / 1000, si->uart_base + MXC_UARTONEMS);

	cr = readl(si->uart_base + MXC_UARTUCR4);
	cr |= MXC_IRDA_RX_INV | MXC_UARTUCR4_IRSC,
	writel(cr, si->uart_base + MXC_UARTUCR4);

	cr = readl(si->uart_base + MXC_UARTUCR3);
	if (uart_ir_mux) {
		cr |= MXC_UARTUCR3_RXDMUXSEL | MXC_IRDA_TX_INV;
	} else {
		cr |= MXC_IRDA_TX_INV;
	}
	writel(cr, si->uart_base + MXC_UARTUCR3);

	cr = readl(si->uart_base + MXC_UARTUCR2);
	cr |= MXC_UARTUCR2_IRTS | MXC_UARTUCR2_WS |
	      MXC_UARTUCR2_TXEN | MXC_UARTUCR2_RXEN;
	cr &= ~MXC_UARTUCR2_SRST;
	writel(cr, si->uart_base + MXC_UARTUCR2);
	/* Wait till we are out of software reset */
	do {
		cr = readl(si->uart_base + MXC_UARTUCR2);
	} while (!(cr & MXC_UARTUCR2_SRST));

	ufcr |= (UART4_UFCR_TXTL << MXC_UARTUFCR_TXTL_OFFSET) |
	    ((6 - d) << MXC_UARTUFCR_RFDIV_OFFSET) | UART4_UFCR_RXTL;
	writel(ufcr, si->uart_base + MXC_UARTUFCR);

	baud = 9600;
	num = baud / 100 - 1;
	denom = si->uartclk / 1600 - 1;
	if ((denom < 65536) && (si->uartclk > 1600)) {
		writel(num, si->uart_base + MXC_UARTUBIR);
		writel(denom, si->uart_base + MXC_UARTUBMR);
	}

	writel(0x0000, si->uart_base + MXC_UARTUTS);

	writel(MXC_UARTUCR1_UARTEN | MXC_UARTUCR1_IREN,
	       si->uart_base + MXC_UARTUCR1);

	mxcirda_debug("UCR1: 0x%04x", readl(si->uart_base + MXC_UARTUCR1));
	mxcirda_debug("UCR2: 0x%04x", readl(si->uart_base + MXC_UARTUCR2));
	mxcirda_debug("UCR3: 0x%04x", readl(si->uart_base + MXC_UARTUCR3));
	mxcirda_debug("UCR4: 0x%04x", readl(si->uart_base + MXC_UARTUCR4));
	mxcirda_debug("UFCR: 0x%04x", readl(si->uart_base + MXC_UARTUFCR));
	mxcirda_debug("UBIR: 0x%04x", readl(si->uart_base + MXC_UARTUBIR));
	mxcirda_debug("UBMR: 0x%04x", readl(si->uart_base + MXC_UARTUBMR));
	mxcirda_debug("UTS : 0x%04x", readl(si->uart_base + MXC_UARTUTS));
	mxcirda_debug("ONEMS: %d",    readl(si->uart_base + MXC_UARTONEMS));

	/* configure IrDA device for speed */
	ret = mxc_irda_set_speed(si, si->speed = 9600);

	return ret;
}

static void mxc_irda_shutdown(struct mxc_irda *si)
{
	u16 cr;
	
	/* Disable Tx and Rx and then disable the UART clock */
	cr = readl(si->uart_base + MXC_UARTUCR2);
	cr &= ~(MXC_UARTUCR2_TXEN | MXC_UARTUCR2_RXEN);
	writel(cr, si->uart_base + MXC_UARTUCR2);
	cr = readl(si->uart_base + MXC_UARTUCR1);
	cr &= ~MXC_UARTUCR1_UARTEN;
	writel(cr, si->uart_base + MXC_UARTUCR1);

	clk_disable(irda_clk);
	if (irda_clk)
		clk_put(irda_clk);
	if (per_clk)
		clk_put(per_clk);
	gpio_irda_inactive();
	return;
}

/*
 * When an ifconfig is issued which changes the device flag to include
 * IFF_UP this function is called. It is only called when the change
 * occurs, not when the interface remains up. The function grabs the interrupt
 * resources and registers IrDA interrupt service routines, requests for DMA
 * channels, configures the DMA channel. It then initializes  the IOMUX
 * registers to configure the pins for IrDA signals and finally initializes the
 * various IrDA registers and enables the port for reception.
 *
 * @param   dev   net device structure that is being opened
 *
 * @return  The function returns 0 for a successful open and non-zero value
 *          on failure.
 */
static int mxc_irda_start(struct net_device *dev)
{
	struct mxc_irda *si = netdev_priv(dev);
	int err;

	si->speed = 9600;

	err = request_irq(si->uart_irq, mxc_irda_irq, 0, dev->name, dev);
	if (err) {
		dev_err(si->dev, "%s:Failed to request the IRQ\n",
			__FUNCTION__);
		return err;
	}
	/*
	 * The interrupt must remain disabled for now.
	 */
	disable_irq(si->uart_irq);

	/* Setup the serial port port for the initial speed. */
	err = mxc_irda_startup(si);
	if (err) {
		goto err_startup;
	}

	/* Open a new IrLAP layer instance. */
	si->irlap = irlap_open(dev, &si->qos, "mxc_irda");
	err = -ENOMEM;
	if (!si->irlap) {
		goto err_irlap;
	}

	si->open = 1;
	si->suspend = 0;

	/* Now enable the interrupt and start the queue */
	enable_irq(si->uart_irq);
	netif_start_queue(dev);
	return 0;

err_irlap:
	si->open = 0;
	mxc_irda_shutdown(si);
err_startup:
		free_irq(si->uart_irq, dev);
	return err;
}

/*
 * This function is called when IFF_UP flag has been cleared by the user via
 * the ifconfig irda0 down command. This function stops any further
 * transmissions being queued, and then disables the interrupts.
 * Finally it resets the device.
 * @param   dev   the net_device structure
 *
 * @return  int   the function always returns 0 indicating a success.
 */
static int mxc_irda_stop(struct net_device *dev)
{
	struct mxc_irda *si = netdev_priv(dev);
	unsigned long flags;

	/* Stop IrLAP */
	if (si->irlap) {
		irlap_close(si->irlap);
		si->irlap = NULL;
	}

	netif_stop_queue(dev);

	/*Save flags and disable the IrDA interrupts.. */
	if (si->open) {
		local_irq_save(flags);
		disable_irq(si->uart_irq);
		local_irq_restore(flags);
		free_irq(si->uart_irq, dev);
		mxc_irda_shutdown(si);
		si->open = 0;
	}
	return 0;
}

#ifdef CONFIG_PM
/*
 * This function is called to put the IrDA in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on which IrDA
 *                to suspend
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxc_irda_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct mxc_irda *si;
	unsigned long flags;

	if (!dev) {
		return 0;
	}

	si = netdev_priv(dev);
	if (si->suspend == 1) {
		dev_err(si->dev,
			" suspend - Device is already suspended ... \n");
		return 0;
	}
	if (si->open) {

		netif_device_detach(dev);

		/*Save flags and disable the IrDA interrupts.. */
		local_irq_save(flags);
		disable_irq(si->uart_irq);
		local_irq_restore(flags);

		mxc_irda_shutdown(si);
		si->suspend = 1;
		si->open = 0;
	}

	return 0;
}

/*
 * This function is called to bring the IrDA back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on which IrDA
 *                to resume
 *
 * @return  The function always returns 0.
 */
static int mxc_irda_resume(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct mxc_irda *si;
	unsigned long flags;

	if (!dev) {
		return 0;
	}

	si = netdev_priv(dev);
	if (si->suspend == 1 && !si->open) {
		/*Initialise the UART first */
 		mxc_irda_startup(si);

		/* Enable the UART and IrDA interrupts.. */
		local_irq_save(flags);
		enable_irq(si->uart_irq);
		local_irq_restore(flags);

		/* Let the kernel know that we are alive and kicking.. */
		netif_device_attach(dev);

		si->suspend = 0;
		si->open = 1;
	}
	return 0;
}
#else
#define mxc_irda_suspend NULL
#define mxc_irda_resume  NULL
#endif

static int mxc_irda_init_iobuf(iobuff_t *io, int size)
{
	io->head = kmalloc(size, GFP_KERNEL | GFP_DMA);
	if (io->head != NULL) {
		io->truesize = size;
		io->in_frame = FALSE;
		io->state = OUTSIDE_FRAME;
		io->data = io->head;
	}
	return io->head ? 0 : -ENOMEM;

}

/*
 * This function is called during the driver binding process.
 * This function requests for memory, initializes net_device structure and
 * registers with kernel.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions
 *
 * @return  The function returns 0 on success and a non-zero value on failure
 */
static int mxc_irda_probe(struct platform_device *pdev)
{
	struct net_device *dev;
	struct mxc_irda *si;
	struct resource *uart_res;
	unsigned int baudrate_mask;
	int uart_irq;
	int err;

	uart_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	uart_irq = platform_get_irq(pdev, 0);
	if (!uart_res || uart_irq == NO_IRQ) {
		dev_err(&pdev->dev, "Unable to find resources\n");
		return -ENXIO;
	}

	if (!request_mem_region(uart_res->start,
				uart_res->end - uart_res->start +1,
				pdev->name)) {
		dev_err(&pdev->dev, "Unable to request resource\n");
		return -ENOMEM;
	}
	mxcirda_debug("UART base: %x, IRQ: %d", uart_res->start, uart_irq);

	dev = alloc_irdadev(sizeof(struct mxc_irda));
	if (!dev) {
		dev_err(&pdev->dev, "Failed to request memory region\n");
		err = -ENOMEM;
		goto err_mem_1;
	}

	si = netdev_priv(dev);
	si->dev = &pdev->dev;
	si->uart_res = uart_res;
	si->uart_irq = uart_irq;

	si->uart_base = (void *)IO_ADDRESS(uart_res->start);
	if (!(si->uart_base)) {
		dev_err(&pdev->dev, "Failed to remap memory region\n");
		err = -ENOMEM;
		goto err_mem_2;
	}

	/*
	 * Initialise the SIR buffers
	 */
	err = mxc_irda_init_iobuf(&si->rx_buff, UART_BUFF_SIZE);
	if (err) {
		dev_err(&pdev->dev, "Failed to request memory for rx buffer\n");
		goto err_mem_2;
	}

	err = mxc_irda_init_iobuf(&si->tx_buff, UART_BUFF_SIZE);
	if (err) {
		dev_err(&pdev->dev, "Failed to request memory for tx buffer\n");
		goto err_mem_3;
	}

	dev->hard_start_xmit	= mxc_irda_hard_xmit;
	dev->open		= mxc_irda_start;
	dev->stop		= mxc_irda_stop;
	dev->do_ioctl		= mxc_irda_ioctl;
	dev->get_stats		= mxc_irda_stats;
	dev->irq		= uart_irq;

	irda_init_max_qos_capabilies(&si->qos);

	/*
	 * We support
	 * SIR(9600, 19200,38400, 57600 and 115200 bps)
	 * Min Turn Time set to 1ms or greater.
	 */
	baudrate_mask = IR_9600;
	switch (max_rate) {
		case 4000000:	baudrate_mask |= IR_4000000 << 8;
		case 1152000:	baudrate_mask |= IR_1152000;
		case 576000:	baudrate_mask |= IR_576000;
		case 115200:	baudrate_mask |= IR_115200;
		case 57600:	baudrate_mask |= IR_57600;
		case 38400:	baudrate_mask |= IR_38400;
		case 19200:	baudrate_mask |= IR_19200;
	}
	si->qos.baud_rate.bits &= baudrate_mask;
	si->qos.min_turn_time.bits = 0x7;
	irda_qos_bits_to_value(&si->qos);

	err = register_netdev(dev);
	if (err == 0) {
		printk("%s: is found on a MXC IrDA\n", dev->name);
		platform_set_drvdata(pdev, dev);
	} else {
		kfree(si->tx_buff.head);
err_mem_3:
		kfree(si->rx_buff.head);
err_mem_2:
		free_netdev(dev);
err_mem_1:
		release_mem_region(uart_res->start,
				uart_res->end - uart_res->start + 1);
	}
	return err;
}

/*
 * Dissociates the driver from the IrDA device. Removes the appropriate IrDA
 * port structure from the kernel.
 *
 * @param   pdev  the device structure used to give information on which IrDA
 *                to remove
 *
 * @return  The function always returns 0.
 */
static int mxc_irda_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct mxc_irda *si = netdev_priv(dev);

	if (si->uart_res->start)
		release_mem_region(si->uart_res->start,
				si->uart_res->end - si->uart_res->start + 1);
	if (si->tx_buff.head)
		kfree(si->tx_buff.head);
	if (si->rx_buff.head)
		kfree(si->rx_buff.head);

	platform_set_drvdata(pdev, NULL);
	unregister_netdev(dev);
	free_netdev(dev);

	return 0;
}

/*
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxcirda_driver = {
	.driver		= {
		.name	= "mxc_irda",
	},
	.probe		= mxc_irda_probe,
	.remove		= mxc_irda_remove,
	.suspend	= mxc_irda_suspend,
	.resume		= mxc_irda_resume,
};

/*
 * This function is used to initialize the IrDA driver module. The function
 * registers the power management callback functions with the kernel and also
 * registers the IrDA callback functions.
 *
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int __init mxc_irda_init(void)
{
	return platform_driver_register(&mxcirda_driver);
}

/*
 * This function is used to cleanup all resources before the driver exits.
 */
static void __exit mxc_irda_exit(void)
{
	platform_driver_unregister(&mxcirda_driver);
}

module_init(mxc_irda_init);
module_exit(mxc_irda_exit);
module_param(max_rate, int, 0);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC IrDA(SIR) driver");
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(max_rate, "Maximum baud rate (115200, 57600, 38400, 19200, 9600)");
