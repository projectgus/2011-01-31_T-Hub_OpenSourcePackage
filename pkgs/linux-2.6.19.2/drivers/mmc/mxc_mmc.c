/*
 *  linux/drivers/mmc/imxmmc.c - Motorola i.MX MMCI driver
 *
 *  Copyright (C) 2004 Sascha Hauer, Pengutronix <sascha@saschahauer.de>
 *
 *  derived from pxamci.c by Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_mmc.c
 *
 * @brief Driver for the Freescale Semiconductor MXC SDHC modules.
 *
 * This driver code is based on imxmmc.c, by Sascha Hauer,
 * Pengutronix <sascha@saschahauer.de>. This driver supports both Secure Digital
 * Host Controller modules (SDHC1 and SDHC2) of MXC. SDHC is also referred as
 * MMC/SD controller. This code is not tested for SD cards.
 *
 * @ingroup MMC_SD
 */

/*
 * Include Files
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/protocol.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/clk.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/sizes.h>
#include <asm/mach-types.h>
#include <asm/mach/irq.h>
#include <asm/arch/mmc.h>

#include "mxc_mmc.h"

#if defined(CONFIG_MXC_MC13783_POWER)
#include <asm/arch/pmic_power.h>
#endif

#define RSP_TYPE(x)	((x) & ~(MMC_RSP_BUSY|MMC_RSP_OPCODE))

static const int vdd_mapping[] = {
	0, 0,
	0,			/* MMC_VDD_160 */
	0, 0,
	1,			/* MMC_VDD_180 */
	0,
	2,			/* MMC_VDD_200 */
	0, 0, 0, 0, 0,
	3,			/* MMC_VDD_260 */
	4,			/* MMC_VDD_270 */
	5,			/* MMC_VDD_280 */
	6,			/* MMC_VDD_290 */
	7,			/* MMC_VDD_300 */
	7,			/* MMC_VDD_310 - HACK for LP1070, actually 3.0V */
	7,			/* MMC_VDD_320 - HACK for LP1070, actually 3.0V */
	0, 0, 0, 0
};

/*
 * This define is used to test the driver without using DMA
 */
#define MXC_MMC_DMA_ENABLE

/*!
 * Maxumum length of s/g list, only length of 1 is currently supported
 */
#define NR_SG   1

#ifdef CONFIG_MMC_DEBUG
static void dump_cmd(struct mmc_command *cmd)
{
	printk(KERN_INFO "%s: CMD: opcode: %d ", DRIVER_NAME, cmd->opcode);
	printk(KERN_INFO "arg: 0x%08x ", cmd->arg);
	printk(KERN_INFO "flags: 0x%08x\n", cmd->flags);
}

static void dump_status(const char *func, int sts)
{
	unsigned int bitset;
	printk(KERN_INFO "%s:status: ", func);
	while (sts) {
		/* Find the next bit set */
		bitset = sts & ~(sts - 1);
		switch (bitset) {
		case STATUS_CARD_INSERTION:
			printk(KERN_INFO "CARD_INSERTION|");
			break;
		case STATUS_CARD_REMOVAL:
			printk(KERN_INFO "CARD_REMOVAL |");
			break;
		case STATUS_YBUF_EMPTY:
			printk(KERN_INFO "YBUF_EMPTY |");
			break;
		case STATUS_XBUF_EMPTY:
			printk(KERN_INFO "XBUF_EMPTY |");
			break;
		case STATUS_YBUF_FULL:
			printk(KERN_INFO "YBUF_FULL |");
			break;
		case STATUS_XBUF_FULL:
			printk(KERN_INFO "XBUF_FULL |");
			break;
		case STATUS_BUF_UND_RUN:
			printk(KERN_INFO "BUF_UND_RUN |");
			break;
		case STATUS_BUF_OVFL:
			printk(KERN_INFO "BUF_OVFL |");
			break;
		case STATUS_READ_OP_DONE:
			printk(KERN_INFO "READ_OP_DONE |");
			break;
		case STATUS_WR_CRC_ERROR_CODE_MASK:
			printk(KERN_INFO "WR_CRC_ERROR_CODE |");
			break;
		case STATUS_READ_CRC_ERR:
			printk(KERN_INFO "READ_CRC_ERR |");
			break;
		case STATUS_WRITE_CRC_ERR:
			printk(KERN_INFO "WRITE_CRC_ERR |");
			break;
		case STATUS_SDIO_INT_ACTIVE:
			printk(KERN_INFO "SDIO_INT_ACTIVE |");
			break;
		case STATUS_END_CMD_RESP:
			printk(KERN_INFO "END_CMD_RESP |");
			break;
		case STATUS_WRITE_OP_DONE:
			printk(KERN_INFO "WRITE_OP_DONE |");
			break;
		case STATUS_CARD_BUS_CLK_RUN:
			printk(KERN_INFO "CARD_BUS_CLK_RUN |");
			break;
		case STATUS_BUF_READ_RDY:
			printk(KERN_INFO "BUF_READ_RDY |");
			break;
		case STATUS_BUF_WRITE_RDY:
			printk(KERN_INFO "BUF_WRITE_RDY |");
			break;
		case STATUS_RESP_CRC_ERR:
			printk(KERN_INFO "RESP_CRC_ERR |");
			break;
		case STATUS_TIME_OUT_RESP:
			printk(KERN_INFO "TIME_OUT_RESP |");
			break;
		case STATUS_TIME_OUT_READ:
			printk(KERN_INFO "TIME_OUT_READ |");
			break;
		default:
			printk(KERN_INFO "Invalid Status Register value0x%x\n",
			       bitset);
			break;
		}
		sts &= ~bitset;
	}
	printk(KERN_INFO "\n");
}
#endif

/*!
 * This structure is a way for the low level driver to define their own
 * \b mmc_host structure. This structure includes the core \b mmc_host
 * structure that is provided by Linux MMC/SD Bus protocol driver as an
 * element and has other elements that are specifically required by this
 * low-level driver.
 */
struct mxcmci_host {
	/*!
	 * The mmc structure holds all the information about the device
	 * structure, current SDHC io bus settings, the current OCR setting,
	 * devices attached to this host, and so on.
	 */
	struct mmc_host *mmc;

	/*!
	 * This variable is used for locking the host data structure from
	 * multiple access.
	 */
	spinlock_t lock;

	/*!
	 * Resource structure, which will maintain base addresses and IRQs.
	 */
	struct resource *res;

	/*!
	 * Base address of SDHC, used in readl and writel.
	 */
	void *base;

	/*!
	 * SDHC IRQ number.
	 */
	int irq;

	/*!
	 * Card Detect IRQ number.
	 */
	int detect_irq;

	/*!
	 * Clock id to hold ipg_perclk.
	 */
	struct clk *clk;
	/*!
	 * MMC mode.
	 */
	int mode;

	/*!
	 * DMA channel number.
	 */
	int dma;

	/*!
	 * Pointer to hold MMC/SD request.
	 */
	struct mmc_request *req;

	/*!
	 * Pointer to hold MMC/SD command.
	 */
	struct mmc_command *cmd;

	/*!
	 * Pointer to hold MMC/SD data.
	 */
	struct mmc_data *data;

	/*!
	 * Holds the number of bytes to transfer using DMA.
	 */
	unsigned int dma_size;

	/*!
	 * Value to store in Command and Data Control Register
	 * - currently unused
	 */
	unsigned int cmdat;

	/*!
	 * Power mode - currently unused
	 */
	unsigned int power_mode;

	/*!
	 * DMA address for scatter-gather transfers
	 */
	dma_addr_t sg_dma;

	/*!
	 * Length of the scatter-gather list
	 */
	unsigned int dma_len;

	/*!
	 * Holds the direction of data transfer.
	 */
	unsigned int dma_dir;

	/*!
	 * Id for MMC block.
	 */
	unsigned int id;

	/*!
	 * Note whether this driver has been suspended.
	 */
	unsigned int mxc_mmc_suspend_flag;

	/*!
	 * Note whether the sdio irq wake function is enabled.
	 */
	unsigned int sdio_set_wake_enable;

	/*!
	 * Platform specific data
	 */
	struct mxc_mmc_platform_data *plat_data;
};

extern void gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);

#ifdef MXC_MMC_DMA_ENABLE
static void mxcmci_dma_irq(void *devid, int error, unsigned int cnt);
#endif
static int mxcmci_data_done(struct mxcmci_host *host, unsigned int stat);

/* Wait count to start the clock */
#define CMD_WAIT_CNT 100

/*!
 * This function sets the SDHC register to stop the clock and waits for the
 * clock stop indication.
 */
static void mxcmci_stop_clock(struct mxcmci_host *host, bool wait)
{
	int wait_cnt = 0;
	while (1) {
		__raw_writel(STR_STP_CLK_IPG_CLK_GATE_DIS |
			     STR_STP_CLK_IPG_PERCLK_GATE_DIS |
			     STR_STP_CLK_STOP_CLK,
			     host->base + MMC_STR_STP_CLK);

		if (!wait)
			break;

		wait_cnt = CMD_WAIT_CNT;
		while (wait_cnt--) {
			if (!(__raw_readl(host->base + MMC_STATUS) &
			      STATUS_CARD_BUS_CLK_RUN))
				break;
		}

		if (!(__raw_readl(host->base + MMC_STATUS) &
		      STATUS_CARD_BUS_CLK_RUN))
			break;
	}
}

/*!
 * This function sets the SDHC register to start the clock and waits for the
 * clock start indication. When the clock starts SDHC module starts processing
 * the command in CMD Register with arguments in ARG Register.
 *
 * @param host Pointer to MMC/SD host structure
 * @param wait Boolean value to indicate whether to wait for the clock to start or come out instantly
 */
static void mxcmci_start_clock(struct mxcmci_host *host, bool wait)
{
	int wait_cnt;

#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif

	while (1) {
		__raw_writel(STR_STP_CLK_IPG_CLK_GATE_DIS |
			     STR_STP_CLK_IPG_PERCLK_GATE_DIS |
			     STR_STP_CLK_START_CLK,
			     host->base + MMC_STR_STP_CLK);
		if (!wait)
			break;

		wait_cnt = CMD_WAIT_CNT;
		while (wait_cnt--) {
			if (__raw_readl(host->base + MMC_STATUS) &
			    STATUS_CARD_BUS_CLK_RUN) {
				break;
			}
		}

		if (__raw_readl(host->base + MMC_STATUS) &
		    STATUS_CARD_BUS_CLK_RUN) {
			break;
		}
	}
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif
	pr_debug("%s:CLK_RATE: 0x%08x\n", DRIVER_NAME,
		 __raw_readl(host->base + MMC_CLK_RATE));
}

/*!
 * This function resets the SDHC host.
 *
 * @param host  Pointer to MMC/SD  host structure
 */
static void mxcmci_softreset(struct mxcmci_host *host)
{
	/* reset sequence */
	__raw_writel(0x8, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x9, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x1, host->base + MMC_STR_STP_CLK);
	__raw_writel(0x3f, host->base + MMC_CLK_RATE);

	__raw_writel(0xff, host->base + MMC_RES_TO);
	__raw_writel(512, host->base + MMC_BLK_LEN);
	__raw_writel(1, host->base + MMC_NOB);
}

/*!
 * This function is called to setup SDHC register for data transfer.
 * The function allocates DMA buffers, configures the DMA channel.
 * Start the DMA channel to transfer data. When DMA is not enabled this
 * function set ups only Number of Block and Block Length registers.
 *
 * @param host  Pointer to MMC/SD host structure
 * @param data  Pointer to MMC/SD data structure
 */
static void mxcmci_setup_data(struct mxcmci_host *host, struct mmc_data *data)
{
	unsigned int nob = data->blocks;

	if (data->flags & MMC_DATA_STREAM) {
		nob = 0xffff;
	}

	host->data = data;

	__raw_writel(nob, host->base + MMC_NOB);
	__raw_writel(data->blksz, host->base + MMC_BLK_LEN);

	host->dma_size = data->blocks * data->blksz;
	pr_debug("%s:Request bytes to transfer:%d\n", DRIVER_NAME,
		 host->dma_size);

#ifdef MXC_MMC_DMA_ENABLE
	if (host->dma_size <= (16 << host->mmc->ios.bus_width)) {
		return;
	}

	if (data->blksz & 0x3) {
		printk(KERN_ERR
		       "mxc_mci: block size not multiple of 4 bytes\n");
	}

	if (data->flags & MMC_DATA_READ) {
		host->dma_dir = DMA_FROM_DEVICE;
	} else {
		host->dma_dir = DMA_TO_DEVICE;
	}
	host->dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
				   host->dma_dir);

	if (data->flags & MMC_DATA_READ) {
		mxc_dma_sg_config(host->dma, data->sg, data->sg_len,
				  host->dma_size, MXC_DMA_MODE_READ);
	} else {
		mxc_dma_sg_config(host->dma, data->sg, data->sg_len,
				  host->dma_size, MXC_DMA_MODE_WRITE);
	}
#endif
}

/*!
 * This function is called by \b mxcmci_request() function to setup the SDHC
 * register to issue command. This function disables the card insertion and
 * removal detection interrupt.
 *
 * @param host  Pointer to MMC/SD host structure
 * @param cmd   Pointer to MMC/SD command structure
 * @param cmdat Value to store in Command and Data Control Register
 */
static void mxcmci_start_cmd(struct mxcmci_host *host, struct mmc_command *cmd,
			     unsigned int cmdat)
{
	WARN_ON(host->cmd != NULL);
	host->cmd = cmd;

	switch (RSP_TYPE(mmc_resp_type(cmd))) {
	case RSP_TYPE(MMC_RSP_R1):	/* r1, r1b, r6 */
		cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R1;
		break;
	case RSP_TYPE(MMC_RSP_R3):
		cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R3;
		break;
	case RSP_TYPE(MMC_RSP_R2):
		cmdat |= CMD_DAT_CONT_RESPONSE_FORMAT_R2;
		break;
	default:
		/* No Response required */
		break;
	}

	if (cmd->opcode == MMC_GO_IDLE_STATE) {
		cmdat |= CMD_DAT_CONT_INIT;	/* This command needs init */
	}

	if (host->mmc->ios.bus_width == MMC_BUS_WIDTH_4) {
		cmdat |= CMD_DAT_CONT_BUS_WIDTH_4;
	}

	__raw_writel(cmd->opcode, host->base + MMC_CMD);
	__raw_writel(cmd->arg, host->base + MMC_ARG);

	__raw_writel(cmdat, host->base + MMC_CMD_DAT_CONT);

	mxcmci_start_clock(host, true);
}

/*!
 * This function is called to complete the command request.
 * This function enables insertion or removal interrupt.
 *
 * @param host Pointer to MMC/SD host structure
 * @param req  Pointer to MMC/SD command request structure
 */
static void mxcmci_finish_request(struct mxcmci_host *host,
				  struct mmc_request *req)
{

	host->req = NULL;
	host->cmd = NULL;
	host->data = NULL;

	if (!(req->cmd->flags & MMC_KEEP_CLK_RUN)) {
		mxcmci_stop_clock(host, true);
	}
	mmc_request_done(host->mmc, req);
}

/*!
 * This function is called when the requested command is completed.
 * This function reads the response from the card and data if the command is for
 * data transfer. This function checks for CRC error in response FIFO or
 * data FIFO.
 *
 * @param host  Pointer to MMC/SD host structure
 * @param stat  Content of SDHC Status Register
 *
 * @return This function returns 0 if there is no pending command, otherwise 1
 * always.
 */
static int mxcmci_cmd_done(struct mxcmci_host *host, unsigned int stat)
{
	struct mmc_command *cmd = host->cmd;
	struct mmc_data *data = host->data;
	int i;
	u32 a, b, c;
	u32 temp_data;
	unsigned int status;
	unsigned long *buf;
	u8 *buf8;
	int no_of_bytes;
	int no_of_words;

	if (!cmd) {
		/* There is no command for completion */
		return 0;
	}

	/* As this function finishes the command, initialize cmd to NULL */
	host->cmd = NULL;

	/* check for Time out errors */
	if (stat & STATUS_TIME_OUT_RESP) {
		__raw_writel(STATUS_TIME_OUT_RESP, host->base + MMC_STATUS);
		pr_debug("%s: CMD TIMEOUT\n", DRIVER_NAME);
		cmd->error = MMC_ERR_TIMEOUT;
	} else if (stat & STATUS_RESP_CRC_ERR && cmd->flags & MMC_RSP_CRC) {
		__raw_writel(STATUS_RESP_CRC_ERR, host->base + MMC_STATUS);
		printk(KERN_ERR "%s: cmd crc error\n", DRIVER_NAME);
		cmd->error = MMC_ERR_BADCRC;
	}

	/* Read response from the card */
	switch (RSP_TYPE(mmc_resp_type(cmd))) {
	case RSP_TYPE(MMC_RSP_R1):	/* r1, r1b, r6 */
		a = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		b = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		c = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		cmd->resp[0] = a << 24 | b << 8 | c >> 8;
		break;
	case RSP_TYPE(MMC_RSP_R3):	/* r3, r4 */
		a = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		b = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		c = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
		cmd->resp[0] = a << 24 | b << 8 | c >> 8;
		break;
	case RSP_TYPE(MMC_RSP_R2):
		for (i = 0; i < 4; i++) {
			a = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
			b = __raw_readl(host->base + MMC_RES_FIFO) & 0xffff;
			cmd->resp[i] = a << 16 | b;
		}
		break;
	default:
		break;
	}

	pr_debug("%s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", DRIVER_NAME,
		 cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);

	if (!host->data || cmd->error != MMC_ERR_NONE) {
		/* complete the command */
		mxcmci_finish_request(host, host->req);
		return 1;
	}

	/* The command has a data transfer */
#ifdef MXC_MMC_DMA_ENABLE
	/* Use DMA if transfer size is greater than fifo size */
	if (host->dma_size > (16 << host->mmc->ios.bus_width)) {
		mxc_dma_enable(host->dma);
		return 1;
	}
#endif
	/* Use PIO tranfer of data */
	buf =
	    (unsigned long *)(page_address(data->sg->page) + data->sg->offset);
	buf8 = (u8 *) buf;

	/* calculate the number of bytes requested for transfer */
	no_of_bytes = data->blocks * data->blksz;
	no_of_words = (no_of_bytes + 3) / 4;
	pr_debug("no_of_words=%d\n", no_of_words);

	if (data->flags & MMC_DATA_READ) {
		for (i = 0; i < no_of_words; i++) {
			/* wait for buffers to be ready for read */
			while (!(__raw_readl(host->base + MMC_STATUS) &
				 (STATUS_BUF_READ_RDY | STATUS_READ_OP_DONE))) ;

			/* read 32 bit data */
			temp_data = __raw_readl(host->base + MMC_BUFFER_ACCESS);
			if (no_of_bytes >= 4) {
				*buf++ = temp_data;
				no_of_bytes -= 4;
			} else {
				do {
					*buf8++ = temp_data;
					temp_data = temp_data >> 8;
				} while (--no_of_bytes);
			}
		}

		/* wait for read operation completion bit */
		while (!(__raw_readl(host->base + MMC_STATUS) &
			 STATUS_READ_OP_DONE)) ;

		/* check for time out and CRC errors */
		status = __raw_readl(host->base + MMC_STATUS);
		if (status & STATUS_TIME_OUT_READ) {
			pr_debug("%s: Read time out occurred\n", DRIVER_NAME);
			data->error = MMC_ERR_TIMEOUT;
			__raw_writel(STATUS_TIME_OUT_READ,
				     host->base + MMC_STATUS);
		} else if (status & STATUS_READ_CRC_ERR) {
			pr_debug("%s: Read CRC error occurred\n", DRIVER_NAME);
			data->error = MMC_ERR_BADCRC;
			__raw_writel(STATUS_READ_CRC_ERR,
				     host->base + MMC_STATUS);
		}
		__raw_writel(STATUS_READ_OP_DONE, host->base + MMC_STATUS);

		pr_debug("%s: Read %u words\n", DRIVER_NAME, i);
	} else {
		for (i = 0; i < no_of_words; i++) {

			/* wait for buffers to be ready for write */
			while (!(__raw_readl(host->base + MMC_STATUS) &
				 STATUS_BUF_WRITE_RDY)) ;

			/* write 32 bit data */
			__raw_writel(*buf++, host->base + MMC_BUFFER_ACCESS);
			if (__raw_readl(host->base + MMC_STATUS) &
			    STATUS_WRITE_OP_DONE) {
				break;
			}
		}

		/* wait for write operation completion bit */
		while (!(__raw_readl(host->base + MMC_STATUS) &
			 STATUS_WRITE_OP_DONE)) ;

		/* check for CRC errors */
		status = __raw_readl(host->base + MMC_STATUS);
		if (status & STATUS_WRITE_CRC_ERR) {
			pr_debug("%s: Write CRC error occurred\n", DRIVER_NAME);
			data->error = MMC_ERR_BADCRC;
			__raw_writel(STATUS_WRITE_CRC_ERR,
				     host->base + MMC_STATUS);
		}
		__raw_writel(STATUS_WRITE_OP_DONE, host->base + MMC_STATUS);
		pr_debug("%s: Written %u words\n", DRIVER_NAME, i);
	}

	/* complete the data transfer request */
	mxcmci_data_done(host, status);

	return 1;
}

/*!
 * This function is called when the data transfer is completed either by DMA
 * or by core. This function is called to clean up the DMA buffer and to send
 * STOP transmission command for commands to transfer data. This function
 * completes request issued by the MMC/SD core driver.
 *
 * @param host   pointer to MMC/SD host structure.
 * @param stat   content of SDHC Status Register
 *
 * @return This function returns 0 if no data transfer otherwise return 1
 * always.
 */
static int mxcmci_data_done(struct mxcmci_host *host, unsigned int stat)
{
	struct mmc_data *data = host->data;

	if (!data) {
		return 0;
	}
#ifdef MXC_MMC_DMA_ENABLE
	if (host->dma_size > (16 << host->mmc->ios.bus_width)) {
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, host->dma_len,
			     host->dma_dir);
	}
#endif
	if (__raw_readl(host->base + MMC_STATUS) & STATUS_ERR_MASK) {
		pr_debug("%s: request failed. status: 0x%08x\n",
			 DRIVER_NAME, __raw_readl(host->base + MMC_STATUS));
	}

	host->data = NULL;
	data->bytes_xfered = host->dma_size;

	if (host->req->stop && data->error == MMC_ERR_NONE) {
		mxcmci_start_cmd(host, host->req->stop, 0);
	} else {
		mxcmci_finish_request(host, host->req);
	}

	return 1;
}

/*!
 * GPIO interrupt service routine registered to handle the SDHC interrupts.
 * This interrupt routine handles card insertion and card removal interrupts.
 *
 * @param   irq    the interrupt number
 * @param   devid  driver private data
 *
 * @return  The function returns \b IRQ_RETVAL(1)
 */
static irqreturn_t mxcmci_gpio_irq(int irq, void *devid)
{
	struct mxcmci_host *host = devid;
	int card_gpio_status = host->plat_data->status(host->mmc->dev);

	pr_debug("%s: MMC%d status=%d %s\n", DRIVER_NAME, host->id,
		 card_gpio_status, card_gpio_status ? "removed" : "inserted");

	if (card_gpio_status == host->plat_data->card_inserted_state) {
		mmc_detect_change(host->mmc, msecs_to_jiffies(100));
	} else {
		mxcmci_cmd_done(host, STATUS_TIME_OUT_RESP);
		mmc_detect_change(host->mmc, msecs_to_jiffies(50));
	}

	do {
		card_gpio_status = host->plat_data->status(host->mmc->dev);
		if (card_gpio_status) {
			set_irq_type(host->detect_irq, IRQT_FALLING);
		} else {
			set_irq_type(host->detect_irq, IRQT_RISING);
		}
	} while (card_gpio_status != host->plat_data->status(host->mmc->dev));

	return IRQ_HANDLED;
}

/*!
 * Interrupt service routine registered to handle the SDHC interrupts.
 * This interrupt routine handles end of command, card insertion and
 * card removal interrupts. If the interrupt is card insertion or removal then
 * inform the MMC/SD core driver to detect the change in physical connections.
 * If the command is END_CMD_RESP read the Response FIFO. If DMA is not enabled
 * and data transfer is associated with the command then read or write the data
 * from or to the BUFFER_ACCESS FIFO.
 *
 * @param   irq    the interrupt number
 * @param   devid  driver private data
 *
 * @return  The function returns \b IRQ_RETVAL(1) if interrupt was handled,
 *          returns \b IRQ_RETVAL(0) if the interrupt was not handled.
 */
static irqreturn_t mxcmci_irq(int irq, void *devid)
{
	struct mxcmci_host *host = devid;
	unsigned int status = 0;
	u32 intctrl;

	if (host->mxc_mmc_suspend_flag == 1) {
		clk_enable(host->clk);
	}

	status = __raw_readl(host->base + MMC_STATUS);
	intctrl = __raw_readl(host->base + MMC_INT_CNTR);
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, status);
#endif
	if (status & STATUS_END_CMD_RESP) {
		__raw_writel(STATUS_END_CMD_RESP, host->base + MMC_STATUS);
		mxcmci_cmd_done(host, status);
	}
	if ((status & STATUS_SDIO_INT_ACTIVE)
	    && (intctrl & INT_CNTR_SDIO_IRQ_EN)) {
		struct irqdesc *d;

		__raw_writel(STATUS_SDIO_INT_ACTIVE, host->base + MMC_STATUS);

		/*Here we do not handle the sdio interrupt to client driver
		   if the host is in suspend state */
		if (host->mxc_mmc_suspend_flag == 0) {
			d = irq_desc + host->mmc->sdio_irq;
			if (unlikely(!(d->handle_irq))) {
				printk(KERN_ERR "\nSDIO int unhandled\n");
				BUG();	/* oops */
			}
			d->handle_irq(host->mmc->sdio_irq, d);
		}
	}

	return IRQ_HANDLED;
}

/*!
 * This function is called by MMC/SD Bus Protocol driver to issue a MMC
 * and SD commands to the SDHC.
 *
 * @param  mmc  Pointer to MMC/SD host structure
 * @param  req  Pointer to MMC/SD command request structure
 */
static void mxcmci_request(struct mmc_host *mmc, struct mmc_request *req)
{
	struct mxcmci_host *host = mmc_priv(mmc);
	/* Holds the value of Command and Data Control Register */
	unsigned long cmdat;

	WARN_ON(host->req != NULL);

	host->req = req;
#ifdef CONFIG_MMC_DEBUG
	dump_cmd(req->cmd);
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif

	cmdat = 0;
	if (req->data) {
		mxcmci_setup_data(host, req->data);

		cmdat |= CMD_DAT_CONT_DATA_ENABLE;

		if (req->data->flags & MMC_DATA_WRITE) {
			cmdat |= CMD_DAT_CONT_WRITE;
		}
		if (req->data->flags & MMC_DATA_STREAM) {
			printk(KERN_ERR
			       "MXC MMC does not support stream mode\n");
		}
	}
	mxcmci_start_cmd(host, req->cmd, cmdat);
}

/*!
 * This function is called by MMC/SD Bus Protocol driver to change the clock
 * speed of MMC or SD card
 *
 * @param mmc Pointer to MMC/SD host structure
 * @param ios Pointer to MMC/SD I/O type structure
 */
static void mxcmci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mxcmci_host *host = mmc_priv(mmc);
	/*This variable holds the value of clock prescaler */
	int prescaler;
	int clk_rate = clk_get_rate(host->clk);
#ifdef MXC_MMC_DMA_ENABLE
	mxc_dma_device_t dev_id = 0;
#endif

#if defined(CONFIG_MXC_MC13783_POWER)
	t_regulator_voltage voltage;
#endif
	pr_debug("%s: clock %u, bus %lu, power %u, vdd %u\n", DRIVER_NAME,
		 ios->clock, 1UL << ios->bus_width, ios->power_mode, ios->vdd);

	host->dma_dir = DMA_NONE;

#ifdef MXC_MMC_DMA_ENABLE
	if (mmc->ios.bus_width != host->mode) {
		mxc_dma_free(host->dma);
		if (mmc->ios.bus_width == MMC_BUS_WIDTH_4) {
			if (host->id == 0) {
				dev_id = MXC_DMA_MMC1_WIDTH_4;
			} else {
				dev_id = MXC_DMA_MMC2_WIDTH_4;
			}
		} else {
			if (host->id == 0) {
				dev_id = MXC_DMA_MMC1_WIDTH_1;
			} else {
				dev_id = MXC_DMA_MMC2_WIDTH_1;
			}
		}
		host->dma = mxc_dma_request(dev_id, "MXC MMC");
		if (host->dma < 0) {
			pr_debug("Cannot allocate MMC DMA channel\n");
		}
		host->mode = mmc->ios.bus_width;
		mxc_dma_callback_set(host->dma, mxcmci_dma_irq, (void *)host);
	}
#endif

#if defined(CONFIG_MXC_MC13783_POWER)
	switch (ios->power_mode) {
	case MMC_POWER_UP:
		if (host->id == 0) {
			voltage.vmmc1 = vdd_mapping[ios->vdd];
			pmic_power_regulator_set_voltage(REGU_VMMC1, voltage);
			pmic_power_regulator_set_lp_mode(REGU_VMMC1,
							 LOW_POWER_DISABLED);
			pmic_power_regulator_on(REGU_VMMC1);
		}
		if (host->id == 1) {
			voltage.vmmc2 = vdd_mapping[ios->vdd];
			pmic_power_regulator_set_voltage(REGU_VMMC2, voltage);
			pmic_power_regulator_set_lp_mode(REGU_VMMC2,
							 LOW_POWER_DISABLED);
			pmic_power_regulator_on(REGU_VMMC2);
		}
		pr_debug("mmc power on\n");
		msleep(300);
		break;
	case MMC_POWER_OFF:
		if (host->id == 0) {
			pmic_power_regulator_set_lp_mode(REGU_VMMC1,
							 LOW_POWER_EN);
			pmic_power_regulator_off(REGU_VMMC1);
		}

		if (host->id == 1) {
			pmic_power_regulator_set_lp_mode(REGU_VMMC2,
							 LOW_POWER_EN);
			pmic_power_regulator_off(REGU_VMMC2);
		}
		pr_debug("mmc power off\n");
		break;
	default:
		break;
	}
#endif

	/*
	 *  Vary divider first, then prescaler.
	 **/
	if (ios->clock) {
		unsigned int clk_dev = 0;

		/*
		 * when prescaler = 16, CLK_20M = CLK_DIV / 2
		 */
		if (ios->clock == mmc->f_min)
			prescaler = 16;
		else
			prescaler = 0;

		/* clk_dev =1, CLK_DIV = ipg_perclk/2 */

		while (prescaler <= 0x800) {
			for (clk_dev = 1; clk_dev <= 0xF; clk_dev++) {
				int x;
				if (prescaler != 0) {
					x = (clk_rate / (clk_dev + 1)) /
					    (prescaler * 2);
				} else {
					x = clk_rate / (clk_dev + 1);
				}

				pr_debug("x=%d, clock=%d %d\n", x, ios->clock,
					 clk_dev);
				if (x <= ios->clock) {
					break;
				}
			}
			if (clk_dev < 0x10) {
				break;
			}
			if (prescaler == 0)
				prescaler = 1;
			else
				prescaler <<= 1;
		}

		pr_debug("prescaler = 0x%x, divider = 0x%x\n", prescaler,
			 clk_dev);
		mxcmci_stop_clock(host, true);
		__raw_writel((prescaler << 4) | clk_dev,
			     host->base + MMC_CLK_RATE);
		mxcmci_start_clock(host, false);
	} else {
		mxcmci_stop_clock(host, true);
	}
}

/*!
 * MMC/SD host operations structure.
 * These functions are registered with MMC/SD Bus protocol driver.
 */
static struct mmc_host_ops mxcmci_ops = {
	.request = mxcmci_request,
	.set_ios = mxcmci_set_ios
};

#ifdef MXC_MMC_DMA_ENABLE
/*!
 * This function is called by DMA Interrupt Service Routine to indicate
 * requested DMA transfer is completed.
 *
 * @param   devid  pointer to device specific structure
 * @param   error any DMA error
 * @param   cnt   amount of data that was transferred
 */
static void mxcmci_dma_irq(void *devid, int error, unsigned int cnt)
{
	struct mxcmci_host *host = devid;
	struct mmc_data *data = host->data;
	u32 status;
	ulong nob, blk_size, i, blk_len;

	mxc_dma_disable(host->dma);

	if (error) {
		pr_debug("Error in DMA transfer\n");
		status = __raw_readl(host->base + MMC_STATUS);
#ifdef CONFIG_MMC_DEBUG
		dump_status(__FUNCTION__, status);
#endif
		mxcmci_data_done(host, status);
		return;
	}
	pr_debug("%s: Transfered bytes:%d\n", DRIVER_NAME, cnt);
	nob = __raw_readl(host->base + MMC_REM_NOB);
	blk_size = __raw_readl(host->base + MMC_REM_BLK_SIZE);
	blk_len = __raw_readl(host->base + MMC_BLK_LEN);
	pr_debug("%s: REM_NOB:%lu REM_BLK_SIZE:%lu\n", DRIVER_NAME, nob,
		 blk_size);
	i = 0;
	do {
		status = __raw_readl(host->base + MMC_STATUS);
		udelay(1);
	} while (!(status & (STATUS_READ_OP_DONE | STATUS_WRITE_OP_DONE)));
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, status);
#endif
	if (status & (STATUS_READ_OP_DONE | STATUS_WRITE_OP_DONE)) {
		pr_debug("%s:READ/WRITE OPERATION DONE\n", DRIVER_NAME);
		/* check for time out and CRC errors */
		status = __raw_readl(host->base + MMC_STATUS);
		if (status & STATUS_READ_OP_DONE) {
			if (status & STATUS_TIME_OUT_READ) {
				pr_debug("%s: Read time out occurred\n",
					 DRIVER_NAME);
				data->error = MMC_ERR_TIMEOUT;
				__raw_writel(STATUS_TIME_OUT_READ,
					     host->base + MMC_STATUS);
			} else if (status & STATUS_READ_CRC_ERR) {
				pr_debug("%s: Read CRC error occurred\n",
					 DRIVER_NAME);
				data->error = MMC_ERR_BADCRC;
				__raw_writel(STATUS_READ_CRC_ERR,
					     host->base + MMC_STATUS);
			}
			__raw_writel(STATUS_READ_OP_DONE,
				     host->base + MMC_STATUS);
		}

		/* check for CRC errors */
		if (status & STATUS_WRITE_OP_DONE) {
			if (status & STATUS_WRITE_CRC_ERR) {
				pr_debug("%s: Write CRC error occurred\n",
					 DRIVER_NAME);
				data->error = MMC_ERR_BADCRC;
				__raw_writel(STATUS_WRITE_CRC_ERR,
					     host->base + MMC_STATUS);
			}
			__raw_writel(STATUS_WRITE_OP_DONE,
				     host->base + MMC_STATUS);
		}
	} else {
		data->error = MMC_ERR_FAILED;
		pr_debug("%s:%d: MXC MMC DMA transfer failed.\n", __FUNCTION__,
			 __LINE__);
	}

	mxcmci_data_done(host, status);
}
#endif

static void mxc_sdio_mask_irq(unsigned int irq)
{
	u32 reg;
	struct mmc_host *host = get_irq_chipdata(irq);
	struct mxcmci_host *mxc_host = mmc_priv(host);

	if (host->sdio_irq != irq)
		return;

	reg = __raw_readl(mxc_host->base + MMC_INT_CNTR);
	reg &= ~INT_CNTR_SDIO_IRQ_EN;
	__raw_writel(reg, mxc_host->base + MMC_INT_CNTR);
}

static void mxc_sdio_ack_irq(unsigned int irq)
{
	struct mmc_host *host = get_irq_chipdata(irq);
	struct mxcmci_host *mxc_host = mmc_priv(host);

	if (host->sdio_irq != irq)
		return;

	mxc_sdio_mask_irq(irq);

	__raw_writel(STATUS_SDIO_INT_ACTIVE, mxc_host->base + MMC_STATUS);
}

static void mxc_sdio_unmask_irq(unsigned int irq)
{
	u32 reg;
	struct mmc_host *host = get_irq_chipdata(irq);
	struct mxcmci_host *mxc_host = mmc_priv(host);

	if (host->sdio_irq != irq)
		return;

	__raw_writel(STATUS_SDIO_INT_ACTIVE, mxc_host->base + MMC_STATUS);

	reg = __raw_readl(mxc_host->base + MMC_INT_CNTR);
	reg |= INT_CNTR_SDIO_IRQ_EN;
	__raw_writel(reg, mxc_host->base + MMC_INT_CNTR);
}

static int mxc_sdio_set_wake(unsigned int irq, unsigned int enable)
{
	struct mmc_host *host = get_irq_chipdata(irq);
	struct mxcmci_host *mxc_host = mmc_priv(host);

	if (host->sdio_irq != irq) {
		return -EINVAL;
	}

	if (enable != 0) {
		pr_debug("enable sdio wake up function\n");
		mxc_host->sdio_set_wake_enable = 1;
	} else {
		pr_debug("disable sdio wake up function\n");
		mxc_host->sdio_set_wake_enable = 0;
	}

	return 0;
}

static struct irq_chip mxc_sdio_irq_chip = {
	.ack = mxc_sdio_ack_irq,
	.mask = mxc_sdio_mask_irq,
	.unmask = mxc_sdio_unmask_irq,
	.set_wake = mxc_sdio_set_wake,
};

/*!
 * This function is called during the driver binding process. Based on the SDHC
 * module that is being probed this function adds the appropriate SDHC module
 * structure in the core driver.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions.
 *
 * @return  The function returns 0 on successful registration and initialization
 *          of SDHC module. Otherwise returns specific error code.
 */
static int mxcmci_probe(struct platform_device *pdev)
{
	struct mxc_mmc_platform_data *mmc_plat = pdev->dev.platform_data;
	struct mmc_host *mmc;
	struct mxcmci_host *host = NULL;
	int card_gpio_status;
	int ret = -ENODEV;

	if (!mmc_plat) {
		return -EINVAL;
	}

	mmc = mmc_alloc_host(sizeof(struct mxcmci_host), &pdev->dev);
	if (!mmc) {
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, mmc);

	mmc->ops = &mxcmci_ops;
	mmc->ocr_avail = mmc_plat->ocr_mask;

	/* Hack to work with LP1070 */
	mmc->ocr_avail |= MMC_VDD_31_32;

	mmc->max_phys_segs = NR_SG;
	mmc->caps = MMC_CAP_4_BIT_DATA;

	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->dma = -1;
	host->dma_dir = DMA_NONE;
	host->id = pdev->id;
	host->mxc_mmc_suspend_flag = 0;
	host->sdio_set_wake_enable = 0;
	host->mode = -1;
	host->plat_data = mmc_plat;
	if (!host->plat_data) {
		ret = -EINVAL;
		goto out;
	}

	host->clk = clk_get(&pdev->dev, "sdhc_clk");
	clk_enable(host->clk);

	mmc->f_min = mmc_plat->min_clk;
	mmc->f_max = mmc_plat->max_clk;
	pr_debug("SDHC:%d clock:%lu\n", pdev->id, clk_get_rate(host->clk));

	spin_lock_init(&host->lock);
	host->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!host->res) {
		ret = -ENOMEM;
		goto out;
	}

	if (!request_mem_region(host->res->start,
				host->res->end -
				host->res->start + 1, pdev->name)) {
		printk(KERN_ERR "request_mem_region failed\n");
		ret = -ENOMEM;
		goto out;
	}
	host->base = (void *)IO_ADDRESS(host->res->start);
	if (!host->base) {
		ret = -ENOMEM;
		goto out1;
	}

	host->irq = platform_get_irq(pdev, 0);
	if (!host->irq) {
		ret = -ENOMEM;
		goto out1;
	}

	host->detect_irq = platform_get_irq(pdev, 1);
	if (!host->detect_irq) {
		goto out1;
	}

	do {
		card_gpio_status = host->plat_data->status(host->mmc->dev);
		if (card_gpio_status) {
			set_irq_type(host->detect_irq, IRQT_FALLING);
		} else {
			set_irq_type(host->detect_irq, IRQT_RISING);
		}
	} while (card_gpio_status != host->plat_data->status(host->mmc->dev));

	ret =
	    request_irq(host->detect_irq, mxcmci_gpio_irq, 0, pdev->name, host);
	if (ret) {
		goto out1;
	}

	mxcmci_softreset(host);

	if (__raw_readl(host->base + MMC_REV_NO) != SDHC_REV_NO) {
		printk(KERN_ERR "%s: wrong rev.no. 0x%08x. aborting.\n",
		       pdev->name, MMC_REV_NO);
		goto out3;
	}
	__raw_writel(READ_TO_VALUE, host->base + MMC_READ_TO);

	__raw_writel(INT_CNTR_END_CMD_RES, host->base + MMC_INT_CNTR);

	ret = request_irq(host->irq, mxcmci_irq, 0, pdev->name, host);
	if (ret) {
		goto out3;
	}

	host->mmc->sdio_irq = platform_get_irq(pdev, 2);
	if (host->mmc->sdio_irq) {
		mmc->caps |= MMC_CAP_SDIO;
		set_irq_chip(host->mmc->sdio_irq, &mxc_sdio_irq_chip);
		set_irq_chipdata(host->mmc->sdio_irq, host->mmc);
		set_irq_handler(host->mmc->sdio_irq, handle_level_irq);
		set_irq_flags(host->mmc->sdio_irq, IRQF_VALID);
	}

	gpio_sdhc_active(pdev->id);

	if ((ret = mmc_add_host(mmc)) < 0) {
		goto out4;
	}

	printk(KERN_INFO "%s-%d found\n", pdev->name, pdev->id);

	return 0;

      out4:
	gpio_sdhc_inactive(pdev->id);
	free_irq(host->irq, host);
      out3:
	free_irq(host->detect_irq, host);
	pr_debug("%s: Error in initializing....", pdev->name);
      out1:
	release_mem_region(pdev->resource[0].start,
			   pdev->resource[0].end - pdev->resource[0].start + 1);
      out:
	clk_disable(host->clk);
	mmc_free_host(mmc);
	platform_set_drvdata(pdev, NULL);
	return ret;
}

/*!
 * Dissociates the driver from the SDHC device. Removes the appropriate SDHC
 * module structure from the core driver.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to remove
 *
 * @return  The function always returns 0.
 */
static int mxcmci_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	platform_set_drvdata(pdev, NULL);

	if (mmc) {
		struct mxcmci_host *host = mmc_priv(mmc);

		mmc_remove_host(mmc);
		free_irq(host->irq, host);
		free_irq(host->detect_irq, host);
#ifdef MXC_MMC_DMA_ENABLE
		mxc_dma_free(host->dma);
#endif
		release_mem_region(host->res->start,
				   host->res->end - host->res->start + 1);
		mmc_free_host(mmc);
		gpio_sdhc_inactive(pdev->id);
	}
	return 0;
}

#ifdef CONFIG_PM

/*!
 * This function is called to put the SDHC in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to suspend
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxcmci_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct mxcmci_host *host = mmc_priv(mmc);
	int ret = 0;
	u32 reg;

	if (mmc) {
		host->mxc_mmc_suspend_flag = 1;
		ret = mmc_suspend_host(mmc, state);
	}
	if (host->sdio_set_wake_enable == 1) {
		__raw_writel(STATUS_SDIO_INT_ACTIVE, host->base + MMC_STATUS);
		reg = __raw_readl(host->base + MMC_INT_CNTR);
		reg |= INT_CNTR_SDIO_INT_WKP_EN;
		__raw_writel(reg, host->base + MMC_INT_CNTR);
	} else {
		reg = __raw_readl(host->base + MMC_INT_CNTR);
		reg &= ~INT_CNTR_SDIO_INT_WKP_EN;
		__raw_writel(reg, host->base + MMC_INT_CNTR);
	}
	clk_disable(host->clk);

	return ret;
}

/*!
 * This function is called to bring the SDHC back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to resume
 *
 * @return  The function always returns 0.
 */
static int mxcmci_resume(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct mxcmci_host *host = mmc_priv(mmc);
	int ret = 0;
	u32 reg;

	/*
	 * Note that a card insertion interrupt will cause this
	 * driver to resume automatically.  In that case we won't
	 * actually have to do any work here.  Return success.
	 */
	if (!host->mxc_mmc_suspend_flag) {
		return 0;
	}
	clk_enable(host->clk);

	if (mmc) {
		ret = mmc_resume_host(mmc);
		host->mxc_mmc_suspend_flag = 0;
	}
	if (host->sdio_set_wake_enable == 1) {
		reg = __raw_readl(host->base + MMC_INT_CNTR);
		reg &= ~INT_CNTR_SDIO_INT_WKP_EN;
		__raw_writel(reg, host->base + MMC_INT_CNTR);
	}
	return ret;
}
#else
#define mxcmci_suspend  NULL
#define mxcmci_resume   NULL
#endif				/* CONFIG_PM */

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct platform_driver mxcmci_driver = {
	.driver = {
		   .name = "mxcmci",
		   },
	.probe = mxcmci_probe,
	.remove = mxcmci_remove,
	.suspend = mxcmci_suspend,
	.resume = mxcmci_resume,
};

/*!
 * This function is used to initialize the MMC/SD driver module. The function
 * registers the power management callback functions with the kernel and also
 * registers the MMC/SD callback functions with the core MMC/SD driver.
 *
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int __init mxcmci_init(void)
{
	printk(KERN_INFO "MXC MMC/SD driver\n");
	return platform_driver_register(&mxcmci_driver);
}

/*!
 * This function is used to cleanup all resources before the driver exits.
 */
static void __exit mxcmci_exit(void)
{
	platform_driver_unregister(&mxcmci_driver);
}

module_init(mxcmci_init);
module_exit(mxcmci_exit);

MODULE_DESCRIPTION("MXC Multimedia Card Interface Driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
