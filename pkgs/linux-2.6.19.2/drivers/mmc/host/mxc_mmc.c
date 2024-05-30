/*
 *  linux/drivers/mmc/host/mxc_mmc.c - Freescale MXC/i.MX MMC driver
 *
 *  based on imxmmc.c
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
 * Copyright 2006-2008 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 25/11/2010.
 * Copyright (c) 2010 Sagemcom All rights reserved.
 * Author: David Giguet, Sagemcom
 *
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
#include <linux/scatterlist.h>
#include <linux/workqueue.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/list.h>

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

#if defined(CONFIG_ULOG_HOOKS) && defined(CONFIG_ULOG_MMC)
#include <ulog/ulog.h>
#define DO_ULOG
#endif

//#define DEBUG
#ifdef DEBUG
#undef pr_debug
#define pr_debug printk
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



/*
 * This define prevents waiting for ever SDHC status flags as
 * in some cases SDHC does not notifies timeout. 
 */
#define XFER_TIMEOUT


/*!
 * Maxumum length of s/g list, only length of 1 is currently supported
 */
#define NR_SG   1

#ifdef DEBUG
static void dump_cmd(struct mmc_command *cmd)
{
	printk(KERN_INFO "--> %s: CMD: opcode: %d ", DRIVER_NAME, cmd->opcode);
	printk(KERN_INFO "arg: 0x%08x ", cmd->arg);
	printk(KERN_INFO "retries : %d ", cmd->retries);
	printk(KERN_INFO "flags: 0x%08x\n", cmd->flags);
}
#endif

#ifdef CONFIG_MMC_DEBUG
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
 *   Insertion state machine, state definition.
 */

typedef enum
{
E_INS_STATE_IDLE	=0x100,	/**< Idle state, no device detected.*/
E_INS_STATE_INSERTED	=0x101, /**< A device has been detected, we can issue requests to it .*/
} e_SM_Insertion_State;

/*!
 *   Request state machine, state definition.
 */

typedef enum
{
E_REQUEST_STATE_IDLE			=0x200,			/**< Idle state.*/
E_REQUEST_STATE_REQUEST_ONGOING		=0x201,			/**< This state is set when a command has been issued.*/
E_REQUEST_STATE_CANCELING		=0x202,			/**< Canceling command ongoing.*/
} e_SM_Request_State;


/*!
 *   Power Management state machine, state definition.
 */

typedef enum
{
E_PM_STATE_ACTIVE		=0x300,			/**< Idle state.*/
E_PM_STATE_SUSPENDED            =0x301,			/**< This state is set when the driver is suspended.*/
} e_SM_PM_State;

/*!
* Driver workqueue.
*/
struct workqueue_struct *pg_workqueue;


/*!
* Define the maximum size of the work parameter list.
*/
 #define MAX_WORK_PARAM_LIST_SIZE 20

/*!
* Work parameter structure.
*/
struct mxcmci_work_param_list
{
	struct list_head list;		/**< Linked list.*/
	void *drv_context;		/**< Driver context.*/
	struct work_struct work;	/**< Work structure.*/
	union
	{
		int stat;		/**< Irq status.*/
		pm_message_t state;	/**< Suspend state parameter.*/
		struct mmc_ios ios;	/**< IOs parameter structure.*/
		int enable;		/**< SDIO irq enable.*/
		void *dev_context;	/**< device context.*/
	};
};

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
	 * Platform specific data
	 */
	struct mxc_mmc_platform_data *plat_data;

	/*!
	 * Insertion state.
	 */
	e_SM_Insertion_State insertion_state;

	/*!
	 * Request state.
	 */
	e_SM_Request_State request_state;

	/*!
	 * Power Management state.
	 */
	e_SM_PM_State PM_state;

	/*!
	 * API lock wait queue.
	 */
	wait_queue_head_t lock_api;

	/*!
	 * API lock flag.
	 */
	int lock_api_flag;

	/*!
	 * Work parameters list.
	 */
	struct mxcmci_work_param_list work_param_list;

	/*!
	 * Work parameters table.
	 */
	struct mxcmci_work_param_list *work_param_table[MAX_WORK_PARAM_LIST_SIZE];

	/*!
	 * API mutex, that forbid race conditions in API calls. This case probably never happen in normal usage.
	 */
	struct mutex api_mutex;
	/*!
	 * API mutex flag.
	 */
	unsigned int api_mutex_flag;
	/*!
	 * Watchdog timer used to detect a loss of connection.
	 */
	struct timer_list wd_timer;

	/*!
	 * Polling mask, indicate that the handling of irq is done inside the polling loop instead of irq handler.
	 */
	unsigned int poll_mask;

	/*!
	 * Irq status, changed inside the irq handler function.
	 */
	unsigned int status;
	/*!
	 * Insure context lock in betwen kernel threads/worqueue.
	 */
	struct mutex ctx_lock;
	/*!
	 * Context lock flag.
	 */
	unsigned int ctx_lock_flag;
	/*!
	 * Insure context lock betwen kernel threads/worqueue and irqs (including timer irq).
	 */
	spinlock_t irq_lock;
	/*!
	 * Timeout counter.
	 */
	unsigned int timeout_counter;
	/*!
	 * error counter.
	 */
	unsigned int error_counter;
};

/*!
* LOCK_IRQ is used to lock context between two irqs, it also protect timer against irqs and threads against timer and irqs.
* LOCK_CTX is used to lock context between API calling threads and driver workqueue.
* LOCK_MUTEX_API is used to insure sequential API calls in case if the client is running the driver from different threads.
* LOCK_API is used to block the calling thread when the previous request is not finished, this avoids to schedule to many works in the workqueue. This lock is necessary because all the calls are asynchronous, the API function is queuing the actual work inside the workqueue and is not really executing the user request but only scheduling it.
* LOCK_API and LOCK_MUTEX_API are probably never useful because kernel is usually waiting for the result of a command before issueing the next. 
*/

#ifdef CONFIG_DEBUG_SPINLOCK
#define LOCK_IRQ if (spin_is_locked(&host->irq_lock)) \
	{ \
	spin_lock_irqsave(&host->irq_lock, flags); \
	} \
	else \
	{ \
	spin_lock_irqsave(&host->irq_lock, flags); \
	}

#define UNLOCK_IRQ if (spin_is_locked(&host->irq_lock)) \
	{ \
	spin_unlock_irqrestore(&host->irq_lock, flags); \
	}

#else
#define LOCK_IRQ spin_lock_irqsave(&host->irq_lock, flags);
#define UNLOCK_IRQ spin_unlock_irqrestore(&host->irq_lock, flags);
#endif

#define	LOCK_CTX mutex_lock(&host->ctx_lock); \
			(host->ctx_lock_flag)++; \
			if (host->ctx_lock_flag!=1) panic("%s: line %d: MMC%d, ERROR - ctx_lock_flag != 1\n",__FUNCTION__,__LINE__, host->id);
#define UNLOCK_CTX (host->ctx_lock_flag)--; \
			mutex_unlock(&host->ctx_lock);	

#define LOCK_API if (!wait_event_timeout(host->lock_api,host->lock_api_flag==0,HZ*10)) \
	{ \
		panic ("%s: line %d: MMC%d, ERROR - wait_event has timed out\n",__FUNCTION__,__LINE__, host->id); \
	} \
	(host->lock_api_flag)++; \
	if (host->lock_api_flag!=1) panic("%s: line %d: MMC%d, ERROR - lock api flag != 1\n",__FUNCTION__,__LINE__, host->id);

#define UNLOCK_API (host->lock_api_flag)--; \
	wake_up(&host->lock_api);

#define LOCK_API_MUTEX mutex_lock(&host->api_mutex); \
			(host->api_mutex_flag)++; \
			if (host->api_mutex_flag!=1) panic("%s: line %d: MMC%d, ERROR - api_mutex_flag != 1\n",__FUNCTION__,__LINE__, host->id);
#define UNLOCK_API_MUTEX (host->api_mutex_flag)--; \
			mutex_unlock(&host->api_mutex);

static void mxcmci_resume_work(void *p);
static void mxcmci_finish_request_work(void *p);

static void mxcmci_request(struct mmc_host *mmc, struct mmc_request *req);
static void mxcmci_do_set_ios(struct mmc_host *mmc, struct mmc_ios *ios);
#ifdef CONFIG_PM
static void mxcmci_do_suspend(struct mxcmci_host *host,pm_message_t state);
static void mxcmci_do_resume(struct mxcmci_host *host);
#endif

/*!
 * Get insertion state value.
 *
 * @param       host Driver context.
 * @return	Return current state value. 
 */
static e_SM_Insertion_State Get_Insertion_State(struct mxcmci_host *host)
{
	return host->insertion_state;
}

/*!
 * Set insertion state value.
 *
 * @param       host Driver context.
 */
#define Set_Insertion_State(host,state) Set_Insertion_State_int(host,state,(char *)__FUNCTION__,__LINE__)
static void Set_Insertion_State_int(struct mxcmci_host *host,e_SM_Insertion_State state,char *func,int line)
{
	pr_debug("%s: line %d: MMC%d, switch Insertion state from 0x%x to 0x%x\n",func,line,host->id,host->insertion_state,state);
	host->insertion_state=state;
#ifdef DO_ULOG
	ulog( ULOG_KERNEL_MMC_MXCMCI_INSERTION_STATE,host->insertion_state);
#endif
}

/*!
 * Get request state value.
 *
 * @param       host Driver context.
 * @return	Return current state value. 
 */
static e_SM_Request_State Get_Request_State(struct mxcmci_host *host)
{
	return host->request_state;
}

/*!
 * Set request state value.
 *
 * @param   host    Driver context.
 */
#define Set_Request_State(host,state) Set_Request_State_int(host,state,(char *)__FUNCTION__,__LINE__)
static void Set_Request_State_int(struct mxcmci_host *host,e_SM_Request_State state,char *func,int line)
{
	pr_debug("%s: line %d: MMC%d, switch Request state from 0x%x to 0x%x\n",func,line,host->id,host->request_state,state);
	host->request_state=state;
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_REQUEST_STATE,host->request_state);
#endif
}

/*!
 * Get Power Mangement state value.
 *
 * @param       host Driver context.
 * @return	Return current state value. 
 */
static e_SM_PM_State Get_PM_State(struct mxcmci_host *host)
{
	return host->PM_state;
}

/*!
 * Set Power Management state value.
 *
 * @param       host Driver context.
 */
#define Set_PM_State(host,state) Set_PM_State_int(host,state,(char *)__FUNCTION__,__LINE__)
static void Set_PM_State_int(struct mxcmci_host *host,e_SM_PM_State state,char *func,int line)
{
	pr_debug("%s: line %d: MMC%d, switch Power Management state from 0x%x to 0x%x\n",func,line,host->id,host->PM_state,state);
	host->PM_state=state;
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_PM_STATE,host->PM_state);
#endif
}


/*!
 * Get element from work parameter list.
 *
 * @param	host Driver context.
 * @return	return element from lit.
 */
#define get_elt_from_list(host) get_elt_from_list_int(host,(char *)__FUNCTION__,__LINE__)
struct mxcmci_work_param_list *get_elt_from_list_int(struct mxcmci_host *host,char *func,int line)
{
	struct mxcmci_work_param_list *tmp=NULL;
	struct list_head *pos;
	int k=0;

	if (list_empty(&host->work_param_list.list))
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_GET_ELT,-1);
#endif
		printk("%s: line %d, list is empty\n",func,line);
		return NULL;
	}

	/* Count number of entry.*/
	list_for_each(pos, &host->work_param_list.list){
		k++;
	}
	
	if (k==1)
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_GET_ELT,-2);
#endif
		printk("%s: line %d, list is empty\n",func,line);
		return NULL;
	}

	/* Find the entry in the list */
	list_for_each(pos, &host->work_param_list.list){
		tmp = list_entry(pos,struct mxcmci_work_param_list, list);
		list_del(pos);
		break;
		
	}
/*#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_GET_ELT,k-1);
#endif*/
	//pr_debug("%s: line %d: MMC%d, number of elements %d to %d\n",func,line,host->id,k,k-1);
	return tmp;
}

/*!
 * Add element to work parameter list.
 *
 * @param	host Driver context.
 * @return	return element from list.
 */
#define add_elt_to_list(host,param) add_elt_to_list_int(host,param,(char *)__FUNCTION__,__LINE__)
void add_elt_to_list_int(struct mxcmci_host *host,struct mxcmci_work_param_list *param,char *func,int line)
{
	struct list_head *pos;
	int k=0;	

	list_add(&(param->list), &(host->work_param_list.list));

	/* Find the entry in the list */
	list_for_each(pos, &host->work_param_list.list){
		k++;
	}

/*#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_ADD_ELT,k);
#endif*/
	//pr_debug("%s: line %d: MMC%d, number of elements %d to %d\n",func,line,host->id,k-1,k);
}

#ifdef XFER_TIMEOUT
/*
 * Implement protected busy looping on SDHC status in IRQ context. 
 * If the condition expected is not true after u_timeout, the -ETIMEDOUT 
 * is returned. Otherwise, zero is returned.
 * 
 * This implements a work around in cases were the STATUS_BUF_READ_RDY, 
 * STATUS_READ_OP_DONE, STATUS_BUF_WRITE_RDY,  STATUS_WRITE_OP_DONE
 * is never set by the SDHC controller. This is to say that the 
 * hardware watchdog (READ_TO) set to READ_TO_VALUE never notifies the
 * timeout occurence
 */
int wait_for_status_watchdog(struct mxcmci_host *host, int flags, int u_timeout,int *stat)
{						
	int counter=0;
	int ret=0;
	unsigned int status;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_WAIT_FOR_STATUS_WD,flags);
#endif

	status=host->status;
	while( !( status & flags) ){							
		counter++;
		if(counter>=u_timeout){
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif			
			ret=-ETIMEDOUT;
			break;
		}
		udelay(1);
		status=host->status;
	}

	if (status & STATUS_READ_OP_DONE)
	{
		if (status & STATUS_TIME_OUT_READ) {
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			pr_debug("%s: line %d: MMC%d, ERROR - Read time out occurred\n",__FUNCTION__,__LINE__, host->id);
			ret = -ETIMEDOUT;

		} else if (status & STATUS_READ_CRC_ERR) {
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			pr_debug("%s: line %d: MMC%d, ERROR - Read CRC error occurred\n",__FUNCTION__,__LINE__, host->id);
			ret = -EILSEQ;

		}
	}

	if (status & STATUS_WRITE_OP_DONE) {
		if (status & STATUS_WRITE_CRC_ERR) {
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			pr_debug("%s: line %d: MMC%d, ERROR - Write CRC error occurred\n",__FUNCTION__,__LINE__, host->id);
			ret = -EILSEQ;
		}
	}

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_WAIT_FOR_STATUS_WD_DONE,counter);
#endif

	*stat=status;
	return ret;
}
#endif


#ifdef CONFIG_APPLY_BUFFER_WORK_AROUND
extern void gpio_serviceled_set(int value);
void buffer_work_around(void){
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_PULSE,0);
#endif
	pr_debug("%s: line %d: Pulse Work Around\n",__FUNCTION__,__LINE__);
	gpio_serviceled_set (1);
	udelay (10);
	gpio_serviceled_set (0);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_PULSE_DONE,0);
#endif
}
#else
void buffer_work_around(void){ return ;}
#endif
EXPORT_SYMBOL(buffer_work_around);



#if defined(CONFIG_MACH_MX31HSV1)
extern int gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);
extern void gpio_serviceled_set(int value);
int sdhc_get_card_write_protect_status(struct mmc_host *host);
#else
extern void gpio_sdhc_active(int module);
extern void gpio_sdhc_inactive(int module);
#endif

#ifdef MXC_MMC_DMA_ENABLE
static void mxcmci_dma_irq(void *devid, int error, unsigned int cnt);
#endif
static int mxcmci_data_done(struct mxcmci_host *host, unsigned int stat);

/* Wait count to start the clock */
//#if defined(CONFIG_MACH_MX31HSV1)
//#define CMD_WAIT_CNT 10
//#else
#define CMD_WAIT_CNT 100
//#endif

/*!
 * This function sets the SDHC register to stop the clock and waits for the
 * clock stop indication.
 */
static void mxcmci_stop_clock(struct mxcmci_host *host, bool wait)
{
	int wait_cnt = 0;
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_STOP_CLOCK,wait);
#endif
#if defined(CONFIG_MACH_MX31HSV1)
	while (wait_cnt != CMD_WAIT_CNT)
#else
	while (1)
#endif
	{
		__raw_writel(STR_STP_CLK_STOP_CLK, host->base + MMC_STR_STP_CLK);

		if (!wait)
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_STOP_CLOCK_DONE,__LINE__);
#endif
			break;
		}

		//mdelay(1);

#if defined(CONFIG_MACH_MX31HSV1)
		if ( ( __raw_readl(host->base + MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN) == 0)
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_WAIT_CNT,wait_cnt);
			ulog(ULOG_KERNEL_MMC_STOP_CLOCK_DONE,__LINE__);
#endif
			break;
		}
		
		
		/*if (status & STATUS_TIME_OUT_READ) {
			printk(KERN_WARNING "SDHC: Timeout read when stopping clock (%dms), giving up.\n",wait_cnt);
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
			ulog(ULOG_KERNEL_MMC_STOP_CLOCK_DONE,__LINE__);
#endif
			// DGI do not need to ACK, because it is done in the interrupt
			//__raw_writel(STATUS_TIME_OUT_READ, host->base + MMC_STATUS);
			break;
		}

		if (status & STATUS_TIME_OUT_RESP) {
			printk(KERN_WARNING "SDHC: Timeout response when stopping clock (%dms), giving up.\n",wait_cnt);
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
			ulog(ULOG_KERNEL_MMC_STOP_CLOCK_DONE,__LINE__);
#endif
			// DGI do not need to ACK, because it is done in the interrupt
			//__raw_writel(STATUS_TIME_OUT_RESP, host->base + MMC_STATUS);
			break;
		}*/

		wait_cnt++;
#else

		wait_cnt = CMD_WAIT_CNT;
		while (wait_cnt--) {
			if (!((__raw_readl(host->base + MMC_STATUS) &
			      STATUS_CARD_BUS_CLK_RUN)))
				break;
		}

		if (!((__raw_readl(host->base + MMC_STATUS) &
		      STATUS_CARD_BUS_CLK_RUN)))
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_STOP_CLOCK_DONE,__LINE__);
#endif
			break;
		}
#endif
	}
#if defined(CONFIG_MACH_MX31HSV1)
#ifdef DO_ULOG
	if (wait_cnt == CMD_WAIT_CNT)
	{
		ulog(ULOG_KERNEL_MMC_WAIT_CNT,wait_cnt);
		ulog(ULOG_KERNEL_MMC_STOP_CLOCK_DONE,__LINE__);
	}
#endif
#endif
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
	int wait_cnt=0;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_START_CLOCK,wait);
#endif
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif

#if defined(CONFIG_MACH_MX31HSV1)
	while (wait_cnt != CMD_WAIT_CNT)
#else
	while (1)
#endif
	{
		__raw_writel(STR_STP_CLK_START_CLK, host->base + MMC_STR_STP_CLK);
		if (!wait)
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_START_CLOCK_DONE,__LINE__);
#endif
			break;
		}

		if (__raw_readl(host->base + MMC_STATUS) & STATUS_CARD_BUS_CLK_RUN) {
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_WAIT_CNT,wait_cnt);
			ulog(ULOG_KERNEL_MMC_START_CLOCK_DONE,__LINE__);
#endif
			break;
		}

		wait_cnt++;

	}
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, __raw_readl(host->base + MMC_STATUS));
#endif
#if defined(CONFIG_MACH_MX31HSV1)
#ifdef DO_ULOG
	if (wait_cnt == CMD_WAIT_CNT)
	{
		ulog(ULOG_KERNEL_MMC_WAIT_CNT,wait_cnt);
		ulog(ULOG_KERNEL_MMC_STOP_CLOCK_DONE,__LINE__);
	}
#endif
#endif
	//pr_debug("%s:CLK_RATE: 0x%08x\n", DRIVER_NAME,
	//	 __raw_readl(host->base + MMC_CLK_RATE));
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

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_SETUP_DATA,nob);
#endif
	if (data->flags & MMC_DATA_STREAM) {
		nob = 0xffff;
	}

	host->data = data;

	__raw_writel(nob, host->base + MMC_NOB);
	__raw_writel(data->blksz, host->base + MMC_BLK_LEN);

	host->dma_size = data->blocks * data->blksz;

#ifdef MXC_MMC_DMA_ENABLE
	if (host->dma_size <= (16 << host->mmc->ios.bus_width)) {
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_SETUP_DATA_DONE,__LINE__);
#endif
		return;
	}

	if (data->blksz & 0x3) {
		printk("%s: line %d: MMC%d, ERROR - block size not multiple of 4 bytes\n",__FUNCTION__,__LINE__,host->id);
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
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_SETUP_DATA_DONE,__LINE__);
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
	if (host->cmd != NULL)
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk ("%s: line %d: MMC%d, ERROR, host->cmd != NULL\n",__FUNCTION__,__LINE__, host->id);
	}
	host->cmd = cmd;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_START_CMD,cmd->opcode);
#endif

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

	// IMPORTANT, the clock SHOULD NOT be started on read operations as it is down by the SDHC itself, refer errata TLSbo93469
	if (!(cmdat & CMD_DAT_CONT_DATA_ENABLE) || (cmdat & CMD_DAT_CONT_WRITE)) {
		mxcmci_start_clock(host, true);
	} 

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_CMD_ARG,cmd->arg);
	ulog(ULOG_KERNEL_MMC_CMD_FLAGS,cmd->flags);
	ulog(ULOG_KERNEL_MMC_CMD_RETRIES,cmd->retries);
#endif

	// configure watchdog timer
#ifdef DEBUG
	mod_timer (&host->wd_timer, jiffies + msecs_to_jiffies(5000));
#else
	mod_timer (&host->wd_timer, jiffies + msecs_to_jiffies(1000));
#endif

	__raw_writel(cmd->opcode, host->base + MMC_CMD);
	__raw_writel(cmd->arg, host->base + MMC_ARG);

	__raw_writel(cmdat, host->base + MMC_CMD_DAT_CONT);

	pr_debug("%s: line %d: MMC%d, opcode=%d arg=0x%x flags=0x%x cmdat=0x%x retries=%d\n",__FUNCTION__,__LINE__, host->id, cmd->opcode, cmd->arg, cmd->flags,cmdat,cmd->retries);


#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_START_CMD_DONE,cmdat);
#endif
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
	if (req==NULL) panic("line %d, req==NULL\n",__LINE__);
	if (req->cmd==NULL) panic("line %d, req->cmd==NULL\n",__LINE__);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_FINISH_REQ,req->cmd->opcode);
#endif

	host->req = NULL;
	host->cmd = NULL;
	host->data = NULL;

	if (!(req->cmd->flags & MMC_KEEP_CLK_RUN)) {
		mxcmci_stop_clock(host, true);
	}


	/*
	 * Work around applied to Samsung card generating CRC error 
	 * at mount time. For some obscure reasons, we have to 
	 * generate a pulse SD2 bus after each CMD18 (multiblock)
	 * otherwise following CMD17 (single-block) or CMD18 
	 * would fail
	 */
	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC) {	
		if (req->cmd->opcode == MMC_READ_MULTIPLE_BLOCK &&	\
		    host->mmc->card->cid.manfid == 0x1B &&		\
		    host->mmc->card->cid.oemid == 0x534D)
		{
			gpio_serviceled_set (1);
			udelay (10);
			gpio_serviceled_set (0);
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_WORKAROUND_SAMSUNG,0);
#endif
		}
	}
	
	//DGI, according to errata TLSbo78667, we should issue some status read command to check that the card busy period is over after a stop command on a multiblock write, I have tried to only poll STATUS_BUF_WRITE_RDY. It should be enough.

	/*if (req->cmd->opcode==MMC_WRITE_MULTIPLE_BLOCK)
	{
		int val;
		unsigned int status;

		val=wait_for_status_watchdog(host,STATUS_BUF_WRITE_RDY | STATUS_WRITE_CRC_ERR,10000,&status);
		if (val) {
			printk("%s: line %d: MMC%d, ERROR - MMC WRITE transfert timeout\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		}
	}*/


	if (!req->cmd->error) 
	{
		//can reset timeout and error counter
		host->timeout_counter=0;
		host->error_counter=0;
	}
	else
	{
		//increment error counter
		(host->error_counter)++;
		if (host->error_counter==50)
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, the SD card controller is keeping returning erros, we issue a detect change method\n",__FUNCTION__,__LINE__, host->id);
			//issue a new detection request
			mmc_detect_change(host->mmc,0);
		}
	}
	//We now need to unlock the API in case request is issued again inside function mmc_request_done
	
	UNLOCK_API
	mmc_request_done(host->mmc, req);	

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_FINISH_REQ_DONE,0);
#endif
}

/*!
 * This function is called when a data transfert work has been issued by the data transfert method.
 *
 * @param   p callback parameter.
 */

static void mxcmci_data_transfert_work(void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	struct mmc_data *data = host->data;
	unsigned int status = param->stat;
	unsigned long *buf;
	u8 *buf8;
	int no_of_bytes;
	int no_of_words;
	int state;
	u32 temp_data;
	int i;
	int ret;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_DATA_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	/* Use PIO transfer of data */
	buf = page_address(data->sg->page) + data->sg->offset;
	buf8 = (u8 *) buf;

	/* calculate the number of bytes requested for transfer */
	no_of_bytes = data->blocks * data->blksz;
	no_of_words = (no_of_bytes + 3) / 4;

	if (data->flags & MMC_DATA_READ) {
		host->poll_mask = STATUS_READ_OP_DONE;
		
		for (i = 0; i < no_of_words; i++) {

			/* wait for buffers to be ready for read */
#ifdef XFER_TIMEOUT
			ret=wait_for_status_watchdog( host, STATUS_BUF_READ_RDY | STATUS_READ_CRC_ERR | STATUS_TIME_OUT_READ ,1000000,&status);
			if (ret) {
				printk("%s: line %d: MMC%d, ERROR - MMC READ transfert timeout, work around applied \n",__FUNCTION__,__LINE__, host->id);
				data->error = ret;
				goto complete_xfer;
			}
#else
			/*Original Busy loop*/
			while (!(__raw_readl(host->base + MMC_STATUS) &
				 (STATUS_BUF_READ_RDY | STATUS_READ_OP_DONE))) ;
#endif

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
#ifdef XFER_TIMEOUT
		ret = wait_for_status_watchdog(host,STATUS_READ_OP_DONE | STATUS_READ_CRC_ERR | STATUS_TIME_OUT_READ,1000000,&status);
		if (ret) {
			printk("%s: line %d: MMC%d, ERROR - MMC READ transfert timeout, work around applied \n",__FUNCTION__,__LINE__, host->id);
			data->error = ret;
			status = __raw_readl(host->base + MMC_STATUS) |
				STATUS_READ_OP_DONE | STATUS_TIME_OUT_READ ;
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_CMD_DONE,__LINE__);
#endif
			goto complete_xfer; 
		}  
#else
		while (!(__raw_readl(host->base + MMC_STATUS) &
			 STATUS_READ_OP_DONE)) ;
#endif

	} else {
		host->poll_mask = STATUS_WRITE_OP_DONE;

		for (i = 0; i < no_of_words; i++) {

			/* wait for buffers to be ready for write */

#ifdef XFER_TIMEOUT
		  	ret=wait_for_status_watchdog(host,STATUS_BUF_WRITE_RDY | STATUS_WRITE_CRC_ERR,1000000,&status);
			if (ret) {
				printk("%s: line %d: MMC%d, ERROR - MMC WRITE transfert timeout, work around applied\n",__FUNCTION__,__LINE__, host->id);
				data->error = ret;
				goto complete_xfer;
			}  
#else
			while (!(__raw_readl(host->base + MMC_STATUS) & STATUS_BUF_WRITE_RDY));
#endif

			/* write 32 bit data */
			__raw_writel(*buf++, host->base + MMC_BUFFER_ACCESS);
			if (__raw_readl(host->base + MMC_STATUS) &
			    STATUS_WRITE_OP_DONE) {
				break;
			}
		}

		/* wait for write operation completion bit */
#ifdef XFER_TIMEOUT
		ret=wait_for_status_watchdog(host,STATUS_WRITE_OP_DONE | STATUS_WRITE_CRC_ERR,1000000,&status);
		if (ret) {
			printk("%s: line %d: MMC%d, ERROR - MMC WRITE transfert timeout, work around applied\n",__FUNCTION__,__LINE__, host->id);
			data->error = ret;
			goto complete_xfer;
		}  
#else
		while (!(__raw_readl(host->base + MMC_STATUS) & STATUS_WRITE_OP_DONE)) ;
#endif
	}


complete_xfer:

	host->poll_mask=0;

	// ------------------------ run insertion state machine
	state = Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_INSERTED:
		case E_INS_STATE_IDLE:
		//nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
	}

	// ------------------------ run request state machine
	state = Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_CANCELING:
		{
			host->req->cmd->error = -ETIMEDOUT;
			mxcmci_finish_request(host,host->req);
			Set_Request_State(host,E_REQUEST_STATE_IDLE);
			//at this point we can delete watchdog timer
			del_timer(&host->wd_timer);
		}
		break;
		case E_REQUEST_STATE_REQUEST_ONGOING:
		{
			ret=mxcmci_data_done(host, status);
			/* complete the data transfer request, if there is an error the timeout timer will issue an error */
			if (ret<=0)
			{
				Set_Request_State(host,E_REQUEST_STATE_IDLE);
				//at this point we can delete watchdog timer
				del_timer(&host->wd_timer);
			}
		}
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	// ------------------------ run Power Management state machine
	state = Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		//nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
	}

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX

	pr_debug("%s: line %d: MMC%d, exit, status=0x%x\n",__FUNCTION__,__LINE__, host->id,status);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_DATA_WORK_DONE,status);
#endif
	
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
	struct mxcmci_work_param_list *tmp=NULL;	
	u32 a, b, c;
	int i;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_CMD,stat);
#endif

	if (!cmd) {
		/* There is no command for completion */
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		return 0;
	}

	/* check for Time out errors */
	if (stat & STATUS_TIME_OUT_RESP) {
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		//acknowledge STATUS_TIME_OUT_RESP
		__raw_writel(STATUS_TIME_OUT_RESP, host->base + MMC_STATUS);

		pr_debug("%s: line %d: MMC%d, ERROR - CMD TIMEOUT\n",__FUNCTION__,__LINE__, host->id);
		cmd->error = -ETIMEDOUT;
	} else if (stat & STATUS_RESP_CRC_ERR && cmd->flags & MMC_RSP_CRC) {
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		//acknowledge STATUS_RESP_CRC_ERR
		__raw_writel(STATUS_RESP_CRC_ERR, host->base + MMC_STATUS);

		pr_debug("%s: line %d: MMC%d, ERROR - cmd crc error\n",__FUNCTION__,__LINE__, host->id);
		cmd->error = -EILSEQ;
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

	pr_debug("%s: line %d: MMC%d, resp= 0x%08x 0x%08x 0x%08x 0x%08x\n",__FUNCTION__,__LINE__, host->id, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_CMD_RESP,cmd->resp[0]);
	ulog(ULOG_KERNEL_MMC_CMD_RESP,cmd->resp[1]);
	ulog(ULOG_KERNEL_MMC_CMD_RESP,cmd->resp[2]);
	ulog(ULOG_KERNEL_MMC_CMD_RESP,cmd->resp[3]);
#endif

	if (!host->data) { //if (!host->data || cmd->error) {
#ifdef DO_ULOG
		if (cmd->error)
		{
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,cmd->error);
		}
#endif
		tmp=get_elt_from_list(host);
		if (tmp==NULL)
		{
			printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
			ulog(ULOG_KERNEL_MMC_CMD_DONE,__LINE__);
#endif
			return 0;
		}

		tmp->drv_context=(void*)host;
		tmp->stat=stat;

		/* complete the command, finish request is differed to the workqueue */
		PREPARE_WORK(&tmp->work, mxcmci_finish_request_work, tmp);
		if (!queue_work(pg_workqueue,&tmp->work))
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
		}

#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_CMD_DONE,__LINE__);
#endif
		return 1;
	}

	if (cmd->error)
	{
		//do not process the data
		return 0;
	}

	/* The command has a data transfer */
#ifdef MXC_MMC_DMA_ENABLE
	/* Use DMA if transfer size is greater than fifo size */
	if (host->dma_size > (16 << host->mmc->ios.bus_width)) {
		mxc_dma_enable(host->dma);
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_CMD_DONE,__LINE__);
#endif
		return 1;
	}
#endif

	tmp=get_elt_from_list(host);
	if (tmp==NULL) 
	{
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
		ulog(ULOG_KERNEL_MMC_CMD_DONE,__LINE__);
#endif
		return 0;
	}
	tmp->drv_context=(void*)host;
	tmp->stat=stat;

	/* complete the command, finish request is differed to the workqueue */
	PREPARE_WORK(&tmp->work, mxcmci_data_transfert_work, tmp);
	if (!queue_work(pg_workqueue, &tmp->work))
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
	}

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_CMD_DONE,__LINE__);
#endif

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
 * @return This function returns -1 if no data transfer otherwise return 0 if transfert is finished and 1 if transfert continue. 
 */
static int mxcmci_data_done(struct mxcmci_host *host, unsigned int stat)
{
	struct mmc_data *data = host->data;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DATA,stat);
#endif

	if (!data) {
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
		ulog(ULOG_KERNEL_MMC_DATA_DONE,__LINE__);
#endif
		return -1;
	}
#ifdef MXC_MMC_DMA_ENABLE
	if (host->dma_size > (16 << host->mmc->ios.bus_width)) {
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, host->dma_len,
			     host->dma_dir);
	}
#endif
	if (__raw_readl(host->base + MMC_STATUS) & STATUS_ERR_MASK) {
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - request failed. status: 0x%08x\n",__FUNCTION__,__LINE__, host->id,__raw_readl(host->base + MMC_STATUS));
	}

	host->cmd = NULL;
	host->data = NULL;
	data->bytes_xfered = host->dma_size;

	//if ((host->req->stop && !data->error) || 
	//	(host->req->stop && (data->error==-EILSEQ))) {
	if (host->req->stop && !data->error) {
		mxcmci_start_cmd(host, host->req->stop, 0);
		
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_DATA_DONE,__LINE__);
#endif
		return 1;
	} else {
		mxcmci_finish_request(host, host->req);
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_DATA_DONE,__LINE__);
#endif
		return 0;
	}



}

/*!
 * Data done work callback.
 *
 * @param	p	driver private data
 */
static void mxcmci_data_done_work(void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	int ret=-1;
	int state;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DATA_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	// ------------------------ run insertion state machine
	state = Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_INSERTED:
		case E_INS_STATE_IDLE:
		//nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
	}

	// ------------------------ run request state machine
	state = Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_CANCELING:
		{
			host->req->cmd->error = -ETIMEDOUT;
			mxcmci_finish_request(host,host->req);
			Set_Request_State(host,E_REQUEST_STATE_IDLE);
			//at this point we can delete watchdog timer
			del_timer(&host->wd_timer);
		}
		break;
		case E_REQUEST_STATE_REQUEST_ONGOING:
		{
			ret=mxcmci_data_done(host,__raw_readl(host->base + MMC_STATUS));
			if (ret<=0)
			{
				Set_Request_State(host,E_REQUEST_STATE_IDLE);
				//at this point we can delete watchdog timer
				del_timer(&host->wd_timer);
			}
		}
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	// ------------------------ run Power Management state machine
	state = Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		//nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
	}

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DATA_WORK_DONE,0);
#endif	
}

/*!
 * Finish request work callback.
 *
 * @param	p	driver private data
 */
static void mxcmci_finish_request_work (void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	int state;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_FINISH_REQ_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	// ------------------------ run insertion state machine
	state = Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_INSERTED:
		case E_INS_STATE_IDLE:
		//nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
	}

	// ------------------------ run request state machine
	state = Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_CANCELING:
		case E_REQUEST_STATE_REQUEST_ONGOING:
		{
			mxcmci_finish_request(host,host->req);
			Set_Request_State(host,E_REQUEST_STATE_IDLE);
			//at this point we can delete watchdog timer
			del_timer(&host->wd_timer);
		}
		break;
		default:
		{
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		}
		break;
	}

	// ------------------------ run power management state machine
	state = Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		{
			//nothing to do here
		}
		break;
		default:
		{
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		}
		break;
	}

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_FINISH_REQ_WORK_DONE,0);
#endif
}

/*!
 * Card insertion / removal work callback.
 *
 * @param	p	driver private data
 */
static void mxcmci_gpio_debounce_work(void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	int card_gpio_status;
	int state;
	unsigned long flags;

	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DEBOUNCE,0);
#endif

	card_gpio_status = host->plat_data->status(host->mmc->parent);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_GPIO_STATUS,card_gpio_status);
#endif

	// ------------------------ run insertion state machine
	state = Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_IDLE:
		{
			if (card_gpio_status==host->plat_data->card_inserted_state)
			{
				Set_Insertion_State(host,E_INS_STATE_INSERTED);

				mmc_detect_change(host->mmc,0);
#ifdef DO_ULOG
				ulog(ULOG_KERNEL_MMC_DETECT_CHANGE,__LINE__);
#endif
				printk("%s: line %d: MMC%d, device inserted\n",__FUNCTION__,__LINE__, host->id);

				do {
					card_gpio_status = host->plat_data->status(host->mmc->parent);
#ifdef DO_ULOG
					ulog(ULOG_KERNEL_MMC_GPIO_STATUS,card_gpio_status);
#endif
					pr_debug("%s: line %d: MMC%d, card_gpio_status=%d\n",__FUNCTION__,__LINE__, host->id,card_gpio_status);
					if (card_gpio_status) {
						set_irq_type(host->detect_irq, IRQT_FALLING);
					} else {
						set_irq_type(host->detect_irq, IRQT_RISING);
					}
				} while (card_gpio_status !=
					host->plat_data->status(host->mmc->parent));

			}
		}
		break;
		case E_INS_STATE_INSERTED:
		{
			// ------------------------ re-initialize the request state machine
			if (Get_Request_State(host)==E_REQUEST_STATE_REQUEST_ONGOING)
			{
				Set_Request_State(host,E_REQUEST_STATE_CANCELING);
			}

			if (card_gpio_status!=host->plat_data->card_inserted_state)
			{
				// stop the clock if is still on
				mxcmci_stop_clock(host,1);

				Set_Insertion_State(host,E_INS_STATE_IDLE);
				//set_irq_type(host->detect_irq, IRQT_RISING);
				mmc_detect_change(host->mmc,0);
#ifdef DO_ULOG
				ulog(ULOG_KERNEL_MMC_DETECT_CHANGE,__LINE__);
#endif
				printk("%s: line %d: MMC%d, device has been removed\n",__FUNCTION__,__LINE__, host->id);
			}

			do {
				card_gpio_status = host->plat_data->status(host->mmc->parent);
#ifdef DO_ULOG
				ulog(ULOG_KERNEL_MMC_GPIO_STATUS,card_gpio_status);
#endif
				pr_debug("%s: line %d: MMC%d, card_gpio_status=%d\n",__FUNCTION__,__LINE__, host->id,card_gpio_status);
				if (card_gpio_status) {
					set_irq_type(host->detect_irq, IRQT_FALLING);
				} else {
					set_irq_type(host->detect_irq, IRQT_RISING);
				}
			} while (card_gpio_status !=
				host->plat_data->status(host->mmc->parent));
		}
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		}
		break;
	}

	//run the request state machine
	state = Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_IDLE:
		case E_REQUEST_STATE_REQUEST_ONGOING:
		case E_REQUEST_STATE_CANCELING:
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);	
		}
		break;
	}
	
	//run the Power Management state machine
	state = Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_SUSPENDED:
		{
			if (card_gpio_status==host->plat_data->card_inserted_state)
			{
				//let's resume the driver when inserting a device
#ifdef CONFIG_PM
				mxcmci_do_resume(host);
#endif
				Set_PM_State(host,E_PM_STATE_ACTIVE);
			}
		}
		break;
		case E_PM_STATE_ACTIVE:
		{
			//nothing to do here
		}
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		}
		break;
	}

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX

	enable_irq(host->detect_irq);
	//printk("TRACE %s IRQ enabled\n",__func__);

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DEBOUNCE_DONE,0);
#endif

	return;
}


/*!
 * GPIO interrupt service routine registered to handle the SDHC interrupts.
 * This interrupt routine handles card insertion and card removal interrupts.
 *
 * @param   irq    the interrupt number
 * @param   devid  driver private data
 * @param   regs   holds a snapshot of the processor's context before the
 *                 processor entered the interrupt code
 *
 * @return  The function returns \b IRQ_RETVAL(1)
 */
static irqreturn_t mxcmci_gpio_irq(int irq, void *devid)
{
	struct mxcmci_host *host = devid;
	struct mxcmci_work_param_list *tmp=NULL;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_GPIO_IRQ,0);
#endif

	LOCK_IRQ

	/*Prevent mmc detection bouncing from generating IRQs*/
	disable_irq(irq);

	tmp=get_elt_from_list(host);
	if (tmp==NULL)
	{
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
		ulog(ULOG_KERNEL_MMC_MXCMCI_GPIO_IRQ_DONE,__LINE__);
#endif
		UNLOCK_IRQ
		return IRQ_HANDLED;
	}

	tmp->drv_context=(void*)host;

	PREPARE_WORK(&tmp->work, mxcmci_gpio_debounce_work, tmp);
	if (!queue_delayed_work(pg_workqueue, &tmp->work, msecs_to_jiffies(250))) /* 250 ms debounce and card insertion delay */
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
	}

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_GPIO_IRQ_DONE,__LINE__);
#endif

	UNLOCK_IRQ
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
 * @param   regs   holds a snapshot of the processor's context before the
 *                 processor entered the interrupt code
 *
 * @return  The function returns \b IRQ_RETVAL(1) if interrupt was handled,
 *          returns \b IRQ_RETVAL(0) if the interrupt was not handled.
 */
static irqreturn_t mxcmci_irq(int irq, void *devid)
{
	struct mxcmci_host *host = devid;
	struct mxcmci_work_param_list *tmp=NULL;
	unsigned int status = 0;
	u32 intctrl;
	unsigned long flags;

	LOCK_IRQ

	status = __raw_readl(host->base + MMC_STATUS);
	host->status=status;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_IRQ,status);
#endif
#ifdef CONFIG_MMC_DEBUG
	dump_status(__FUNCTION__, status);
#endif
	intctrl = __raw_readl(host->base + MMC_INT_CNTR);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_ISR_CTRL,intctrl);
#endif

	if (status & STATUS_END_CMD_RESP) {
		//acknowledge END_CMD_RESP
		__raw_writel(STATUS_END_CMD_RESP, host->base + MMC_STATUS);

		mxcmci_cmd_done(host, status);
	}

	if (status & STATUS_CARD_INSERTION) {
		//acknowledge CARD_INSERTION
		__raw_writel(STATUS_CARD_INSERTION, host->base + MMC_STATUS);
	}

	if (status & STATUS_CARD_REMOVAL) {
		//acknowledge CARD_REMOVAL
		__raw_writel(STATUS_CARD_REMOVAL, host->base + MMC_STATUS);
	}

	if ((status & STATUS_SDIO_INT_ACTIVE)
	    && (intctrl & INT_CNTR_SDIO_IRQ_EN)) {
		//acknowledge STATUS_SDIO_INT_ACTIVE
		__raw_writel(STATUS_SDIO_INT_ACTIVE, host->base + MMC_STATUS);


		/*Here we do not handle the sdio interrupt to client driver
		   if the host is in suspend state */
		if (Get_PM_State(host) == E_PM_STATE_ACTIVE) {
			mmc_signal_sdio_irq(host->mmc);
		}
	}

	if (status & (STATUS_WRITE_OP_DONE | STATUS_READ_OP_DONE)) {
		if (!host->data) /* An error occured due to card removal */
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			pr_debug("%s: line %d: MMC%d, ERROR - host->data = NULL\n",__FUNCTION__,__LINE__, host->id);
			//pr_debug("%s:READ/WRITE OPERATION DONE\n", DRIVER_NAME);
			/* check for time out and CRC errors */
			if (status & STATUS_READ_OP_DONE) {
				if (status & STATUS_TIME_OUT_READ) {
					pr_debug("%s: line %d: MMC%d, ERROR - Read time out occurred\n",__FUNCTION__,__LINE__, host->id);
					//acknowledge STATUS_TIME_OUT_READ
					__raw_writel(STATUS_TIME_OUT_READ,host->base + MMC_STATUS);

				} else if (status & STATUS_READ_CRC_ERR) {
					pr_debug("%s: line %d: MMC%d, ERROR - Read CRC error occurred\n",__FUNCTION__,__LINE__, host->id);
					//acknowledge STATUS_READ_CRC_ERR
					__raw_writel(STATUS_READ_CRC_ERR,host->base + MMC_STATUS);

				}
				//acknowledge STATUS_READ_OP_DONE
				__raw_writel(STATUS_READ_OP_DONE,host->base + MMC_STATUS);

			}
	
			/* check for CRC errors */
			if (status & STATUS_WRITE_OP_DONE) {
				if (status & STATUS_WRITE_CRC_ERR) {
#ifdef DO_ULOG
					ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
					pr_debug("%s: Write CRC error occurred\n",
						DRIVER_NAME);
					//acknowledge STATUS_WRITE_CRC_ERR
					__raw_writel(STATUS_WRITE_CRC_ERR,host->base + MMC_STATUS);

					//acknowledge STATUS_WR_CRC_ERROR_CODE_MASK
					__raw_writel(STATUS_WR_CRC_ERROR_CODE_MASK,host->base + MMC_STATUS);
				}
				//acknowledge STATUS_WRITE_OP_DONE
				__raw_writel(STATUS_WRITE_OP_DONE,host->base + MMC_STATUS);

			}
			mxcmci_cmd_done(host, STATUS_TIME_OUT_RESP);
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXCMCI_IRQ_DONE,__LINE__);
#endif
			goto exit;
		}
		else
		{
			struct mmc_data *data = host->data;
	
			//pr_debug("%s:READ/WRITE OPERATION DONE\n", DRIVER_NAME);
			/* check for time out and CRC errors */
			if (status & STATUS_READ_OP_DONE) {
				if (status & STATUS_TIME_OUT_READ) {
#ifdef DO_ULOG
					ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
					pr_debug("%s: line %d: MMC%d, ERROR - Read time out occurred\n",__FUNCTION__,__LINE__, host->id);
					data->error = -ETIMEDOUT;
					//acknowledge STATUS_TIME_OUT_READ
					__raw_writel(STATUS_TIME_OUT_READ,host->base + MMC_STATUS);

				} else if (status & STATUS_READ_CRC_ERR) {
#ifdef DO_ULOG
					ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
					pr_debug("%s: line %d: MMC%d, ERROR - Read CRC error occurred\n",__FUNCTION__,__LINE__, host->id);
					data->error = -EILSEQ;
					//acknowledge STATUS_READ_CRC_ERR
					__raw_writel(STATUS_READ_CRC_ERR,host->base + MMC_STATUS);

				}
				//acknowledge STATUS_READ_OP_DONE
				__raw_writel(STATUS_READ_OP_DONE,host->base + MMC_STATUS);

			}
	
			/* check for CRC errors */
			if (status & STATUS_WRITE_OP_DONE) {
				if (status & STATUS_WRITE_CRC_ERR) {
#ifdef DO_ULOG
					ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
					pr_debug("%s: line %d: MMC%d, ERROR - Write CRC error occurred\n",__FUNCTION__,__LINE__, host->id);
					data->error = -EILSEQ;
					//acknowledge STATUS_WRITE_CRC_ERR
					__raw_writel(STATUS_WRITE_CRC_ERR,host->base + MMC_STATUS);

					//acknowledge STATUS_WR_CRC_ERROR_CODE_MASK
					__raw_writel(STATUS_WR_CRC_ERROR_CODE_MASK,host->base + MMC_STATUS);
				}

				//acknowledge STATUS_WRITE_OP_DONE
				__raw_writel(STATUS_WRITE_OP_DONE,host->base + MMC_STATUS);

			}

			if (!(host->poll_mask & (STATUS_WRITE_OP_DONE | STATUS_READ_OP_DONE)))
			{
				tmp=get_elt_from_list(host);
				if (tmp==NULL) 
				{
					printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
#ifdef DO_ULOG
					ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
					goto exit;
				}

				tmp->drv_context=(void*)host;
				tmp->stat=status;

				PREPARE_WORK(&tmp->work, mxcmci_data_done_work,tmp);
				if (!queue_work(pg_workqueue, &tmp->work))
				{
#ifdef DO_ULOG
					ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
					printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
				}
			}
		}
	}
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_IRQ_DONE,__LINE__);
#endif
exit:
	UNLOCK_IRQ
	return IRQ_HANDLED;
}

/*!
 * Request work callback.
 *
 * @param	p	driver private data
 */
static void mxcmci_request_work (void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	struct mmc_request *req = host->req;
	/* Holds the value of Command and Data Control Register */
	int state;
	unsigned long cmdat;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_REQUEST_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	cmdat = 0;
	if (req->data) {
		mxcmci_setup_data(host, req->data);

		cmdat |= CMD_DAT_CONT_DATA_ENABLE;

		if (req->data->flags & MMC_DATA_WRITE) {
			cmdat |= CMD_DAT_CONT_WRITE;
		}
		if (req->data->flags & MMC_DATA_STREAM) {
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - MXC MMC does not support stream mode\n",__FUNCTION__,__LINE__, host->id);
		}
	}

	// ------------------------ run insertion state machine
	state = Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_INSERTED:
		case E_INS_STATE_IDLE:
		//nothing to do here
		break;
		default:
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run the Power Management state machine
	state = Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		{
			//nothing to do here
		}
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		}
		break;
	}


	//run the request state machine
	state = Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_IDLE:
		{
			mxcmci_start_cmd(host, req->cmd, cmdat);
			Set_Request_State(host,E_REQUEST_STATE_REQUEST_ONGOING);
		}
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);	
		}
		break;
	}
	
	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX

	pr_debug("%s: line %d: MMC%d, exit cmdat=0x%x\n",__FUNCTION__,__LINE__, host->id,(unsigned int)cmdat);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_REQUEST_WORK_DONE,cmdat);
#endif
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
	struct mxcmci_work_param_list *tmp=NULL;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_REQUEST,0);
#endif
	LOCK_API_MUTEX
	LOCK_API

	if (host->req!=NULL) panic("%s: line %d: MMC%d, ERROR - host->req should not be NULL\n",__FUNCTION__,__LINE__,host->id);

	host->req = req;
#ifdef DEBUG
	dump_cmd(req->cmd);
#endif

	tmp=get_elt_from_list(host);
	if (tmp==NULL)
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
		UNLOCK_API;
	}
	else
	{
		tmp->drv_context=(void*)host;

		PREPARE_WORK(&tmp->work, mxcmci_request_work,tmp);
		if (!queue_work(pg_workqueue, &tmp->work))
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
		}
	}

	UNLOCK_API_MUTEX
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_REQUEST_DONE,0);
#endif
}

/*!
 * This function is used to change the clock
 * speed of MMC or SD card
 *
 * @param mmc Pointer to MMC/SD host structure
 * @param ios Pointer to MMC/SD I/O type structure
 */
static void mxcmci_do_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mxcmci_host *host = mmc_priv(mmc);
	int prescaler;
	int clk_rate = clk_get_rate(host->clk);
#ifdef MXC_MMC_DMA_ENABLE
	mxc_dma_device_t dev_id = 0;
#endif

#if defined(CONFIG_MXC_MC13783_POWER)
	t_regulator_voltage voltage;
#endif

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_IO,0);
#endif
	//printk("%s: line %d: MMC%d, clock=0x%x vdd=0x%x bus_mode=0x%x chip_select=0x%x power_mode=0x%x,bus_width=0x%x,timing=0x%x\n",__FUNCTION__,__LINE__, host->id,ios->clock,ios->vdd,ios->bus_mode,ios->chip_select,ios->power_mode,ios->bus_width,ios->timing);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_IOS_PRM,ios->clock);
	ulog(ULOG_KERNEL_MMC_IOS_PRM,ios->vdd);
	ulog(ULOG_KERNEL_MMC_IOS_PRM,ios->bus_mode);
	ulog(ULOG_KERNEL_MMC_IOS_PRM,ios->chip_select);
	ulog(ULOG_KERNEL_MMC_IOS_PRM,ios->power_mode);
	ulog(ULOG_KERNEL_MMC_IOS_PRM,ios->bus_width);
	ulog(ULOG_KERNEL_MMC_IOS_PRM,ios->timing);
#endif
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
			panic("%s: line %d: MMC%d, ERROR - Cannot allocate MMC DMA channel\n",__FUNCTION__,__LINE__, host->id);
		}
		host->mode = mmc->ios.bus_width;
		mxc_dma_callback_set(host->dma, mxcmci_dma_irq, (void *)host);
	}
#endif

#if defined(CONFIG_MXC_MC13783_POWER)
	switch (ios->power_mode) {
	case MMC_POWER_UP:
#ifdef CONFIG_MACH_MX31HSV1
      if ( get_board_type() < BOARD_TYPE_V3_HOMESCREEN_GENERIC )
      {
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
		//pr_debug("mmc power on\n");
		msleep(300);
      }
#endif  /* CONFIG_MACH_MX31HSV1 */
 
		break;
	case MMC_POWER_OFF:
#ifdef CONFIG_MACH_MX31HSV1
      if ( get_board_type() < BOARD_TYPE_V3_HOMESCREEN_GENERIC )
      {
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
		//pr_debug("mmc power off\n");
      }
#endif /* CONFIG_MACH_MX31HSV1 */
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

				//pr_debug("x=%d, clock=%d %d\n", x, ios->clock,
				//	 clk_dev);
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

		pr_debug("%s: line %d: MMC%d, prescaler = 0x%x, divider = 0x%x\n",__FUNCTION__,__LINE__, host->id,prescaler,clk_dev);
		mxcmci_stop_clock(host, true);
		__raw_writel((prescaler << 4) | clk_dev,
			     host->base + MMC_CLK_RATE);
		mxcmci_start_clock(host, false);
	} else {
		mxcmci_stop_clock(host, true);
	}

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_IO_DONE,0);
#endif
}


/*!
 * Set ios work callback.
 *
 * @param	p	driver private data
 */
static void mxcmci_set_ios_work (void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	struct mmc_ios *ios = &param->ios;
	struct mmc_host *mmc = host->mmc;
	int state;
	int do_set_ios=0;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_IO_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	//run insertion state machine
	state = Get_Insertion_State (host);
	switch (state)
	{
		case E_INS_STATE_IDLE:
		case E_INS_STATE_INSERTED:
		// nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run request state machine
	state = Get_Request_State (host);
	switch (state)
	{
		case E_REQUEST_STATE_IDLE:
		{
			do_set_ios=1;
		}
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run Power Management state machine
	state = Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		//nothing to do
		break;
		case E_PM_STATE_SUSPENDED:
		{
			//let's resume the driver
			mxcmci_do_resume(host);
		}
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	if (do_set_ios)
	{
		mxcmci_do_set_ios(mmc,ios);
	}

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX
	UNLOCK_API

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_IO_WORK_DONE,0);
#endif
}

/*!
 * This function is called by the user space to configure the ios.
 *
 * @param mmc Pointer to MMC/SD host structure
 * @param ios Pointer to MMC/SD I/O type structures
 */
static void mxcmci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct mxcmci_host *host = mmc_priv(mmc);
	struct mxcmci_work_param_list *tmp=NULL;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_IO,0);
#endif

	//printk("%s: line %d: MMC%d, clock=0x%x vdd=0x%x bus_mode=0x%x chip_select=0x%x power_mode=0x%x,bus_width=0x%x,timing=0x%x\n",__FUNCTION__,__LINE__, host->id,ios->clock,ios->vdd,ios->bus_mode,ios->chip_select,ios->power_mode,ios->bus_width,ios->timing);

	LOCK_API_MUTEX
	LOCK_API
	LOCK_CTX
	LOCK_IRQ

	tmp=get_elt_from_list(host);
	if (tmp==NULL) 
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
		UNLOCK_API;
	}
	else
	{
		tmp->drv_context=(void*)host;
		memcpy(&tmp->ios,ios,sizeof(struct mmc_ios));

		PREPARE_WORK(&tmp->work, mxcmci_set_ios_work,tmp);
		if (!queue_work(pg_workqueue, &tmp->work))
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
		}
	}
	UNLOCK_IRQ
	UNLOCK_CTX
	UNLOCK_API_MUTEX
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_IO_DONE,0);
#endif
}

/*!
 * Method provided by the driver to enable/disable the sdio irq.
 *
 * @param	host	driver context.
 * @param       enable  1 to enable the irq, 0 otherwise.
 */
static void mxcmci_do_enable_sdio_irq(struct mxcmci_host *host, int enable)
{
	u32 intctrl;
	unsigned long flags;

	LOCK_IRQ
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_ENABLE_SDIO,enable);
#endif

	intctrl = __raw_readl(host->base + MMC_INT_CNTR);
	intctrl &= ~INT_CNTR_SDIO_IRQ_EN;

	if (enable)
		intctrl |= INT_CNTR_SDIO_IRQ_EN;

	__raw_writel(intctrl, host->base + MMC_INT_CNTR);

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_ENABLE_SDIO_DONE,0);
#endif
	UNLOCK_IRQ
}

/*!
 * Set enable sdio work callback.
 *
 * @param	p	driver private data
 */
static void mxcmci_enable_sdio_work (void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	int state;
	int enable=param->enable;
	int apply_change;
	unsigned long flags;
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_ENABLE_SDIO_WORK,enable);
#endif
	pr_debug("%s: line %d: MMC%d, enter enable=%d\n",__FUNCTION__,__LINE__, host->id,enable);

	// run insertion state machine
	state = Get_Insertion_State (host);
	switch (state)
	{
		case E_INS_STATE_IDLE:
		case E_INS_STATE_INSERTED:
		// nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run request state machine
	state = Get_Request_State (host);
	switch (state)
	{
		case E_REQUEST_STATE_IDLE:
		{
			apply_change=1;
		}
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run power management state machine
	state = Get_PM_State (host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		case E_PM_STATE_SUSPENDED:
		//nothing to do
		break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	if (apply_change) mxcmci_do_enable_sdio_irq(host,enable);

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_CTX
	UNLOCK_IRQ
	UNLOCK_API

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_ENABLE_SDIO_WORK_DONE,0);
#endif
}

/*!
 * Method provided by the driver to enable/disable the sdio irq.
 *
 * @param	mmc	driver context.
 * @param	enable  set to 1 to enable the, and to 0 to disable it.
 */
static void mxcmci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct mxcmci_host *host = mmc_priv(mmc);
	struct mxcmci_work_param_list *tmp=NULL;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_ENABLE_SDIO_IRQ,enable);
#endif
	LOCK_API_MUTEX
	LOCK_API
	LOCK_CTX
	LOCK_IRQ

	tmp=get_elt_from_list(host);
	if (tmp==NULL) 
	{
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		UNLOCK_API;
	}
	else
	{
		tmp->drv_context=(void*)host;
		tmp->enable=enable;

		PREPARE_WORK(&tmp->work, mxcmci_enable_sdio_work,tmp);
		if (!queue_work(pg_workqueue, &tmp->work))
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
		}
	}
	UNLOCK_IRQ
	UNLOCK_CTX
	UNLOCK_API_MUTEX
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_ENABLE_SDIO_IRQ_DONE,0);
#endif
}

/*!
 * MMC/SD host operations structure.
 * These functions are registered with MMC/SD Bus protocol driver.
 */
static struct mmc_host_ops mxcmci_ops = {
	.request = mxcmci_request,
	.set_ios = mxcmci_set_ios,
#ifdef CONFIG_MACH_MX31HSV1
	.get_ro  = sdhc_get_card_write_protect_status,
#endif
	.enable_sdio_irq = mxcmci_enable_sdio_irq,
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
	struct mxcmci_work_param_list *tmp=NULL;
	ulong nob, blk_size, blk_len;
	unsigned long flags;

	LOCK_IRQ;
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_DMA_IRQ,cnt);
#endif

	mxc_dma_disable(host->dma);

	if (error) {
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,error);
#endif
		pr_debug("%s: line %d: MMC%d, ERROR - error in DMA transfert\n",__FUNCTION__,__LINE__,host->id);

		tmp=get_elt_from_list(host);
		if (tmp==NULL)
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
		}
		else
		{
			tmp->drv_context=(void*)host;

			PREPARE_WORK(&tmp->work, mxcmci_data_done_work,tmp);
			if (!queue_work(pg_workqueue, &tmp->work))
			{
#ifdef DO_ULOG
				ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
				printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
			}
		}
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXCMCI_DMA_IRQ_DONE,__LINE__);
#endif
		UNLOCK_IRQ
		return;
	}
	//pr_debug("%s: Transfered bytes:%d\n", DRIVER_NAME, cnt);
	nob = __raw_readl(host->base + MMC_REM_NOB);
	blk_size = __raw_readl(host->base + MMC_REM_BLK_SIZE);
	blk_len = __raw_readl(host->base + MMC_BLK_LEN);
	//pr_debug("%s: REM_NOB:%lu REM_BLK_SIZE:%lu\n", DRIVER_NAME, nob,blk_size);

	/*
	 * Now wait for an OP_DONE interrupt before checking
	 * error status and finishing the data phase
	 */
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_DMA_IRQ_DONE,__LINE__);
#endif
	UNLOCK_IRQ
	return;
}
#endif

/*!
 * Watchdog timer work.
 *
 * @param	p	driver private data
 */
static void mxcmci_wd_timer_work(void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	int state;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_WD_TIMER_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	// ------------------------ run insertion state machine
	state = Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_IDLE:
		case E_INS_STATE_INSERTED:
		//nothing to do
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		}
		break;
	}

	//run the request state machine
	state = Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_IDLE:
		{
			//we should be in requesting state, we finally might have received the response, let's do nothing
			pr_debug("%s: line %d: MMC%d, Timeout is ignored\n",__FUNCTION__,__LINE__, host->id);
		}
		break;
		case E_REQUEST_STATE_REQUEST_ONGOING:
		case E_REQUEST_STATE_CANCELING:
		{
			(host->timeout_counter)++;
			
			//the request has timed out, try to reset the device, submit a timeout to the API and unlock it
			printk("%s: line %d: MMC%d, Timeout SD controller, send a pulse\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_CANCEL_CMD,__LINE__);
#endif
			if (host->req==NULL) panic("%s: line %d: MMC%d, host->req==NULL\n",__FUNCTION__,__LINE__, host->id);
			if (host->req->cmd==NULL) panic("%s: line %d: MMC%d, host->req->cmd==NULL\n",__FUNCTION__,__LINE__, host->id);
			pr_debug("%s: line %d: MMC%d, Canceling cmd %d\n",__FUNCTION__,__LINE__, host->id,host->req->cmd->opcode);
			host->req->cmd->error = -ETIMEDOUT;
			host->cmd->retries=0;
			mxcmci_finish_request(host,host->req);

			if (host->timeout_counter==3)
			{
#ifdef DO_ULOG
				ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
				printk("%s: line %d: MMC%d, the SD card controller does not respond anymore, we issue a detect change method\n",__FUNCTION__,__LINE__, host->id);
				mmc_detect_change(host->mmc,0);
			}

			Set_Request_State(host,E_REQUEST_STATE_IDLE);
		}
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);	
		}
		break;
	}
	
	//run the Power Management state machine
	state = Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		{
			//nothing to do here
		}
		break;
		default:
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		}
		break;
	}

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_WD_TIMER_WORK_DONE,0);
#endif

	return;
}

/*!
 * Watchdog timer callback.
 *
 * @param   p User context.
 */

static void watchdog_timer (unsigned long p)
{
	struct mxcmci_host *host = (struct mxcmci_host *)p;
	struct mxcmci_work_param_list *tmp=NULL;
	unsigned long flags;

	LOCK_IRQ
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_WD_TIMER,0);
#endif

	tmp=get_elt_from_list(host);
	if (tmp==NULL) 
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
	}
	else
	{
		tmp->drv_context=(void*)host;

		/* complete the command, finish request is differed to the workqueue */
		PREPARE_WORK(&tmp->work, mxcmci_wd_timer_work, tmp);
		if (!queue_work(pg_workqueue, &tmp->work))
		{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
		}
	}

	UNLOCK_IRQ
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_WD_TIMER_DONE,0);
#endif
}

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
	int i;

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
	mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ;

	host = mmc_priv(mmc);
	host->mmc = mmc;

	host->dma = -1;
	host->dma_dir = DMA_NONE;
	host->id = pdev->id;
	//host->apply_TLSbo78667_workaround=0;
	//host->current_rca=0;
	host->lock_api_flag=0;
	host->timeout_counter=0;
	host->error_counter=0;

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

	host->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!host->res) {
		ret = -ENOMEM;
		goto out;
	}

	if (!request_mem_region(host->res->start,
				host->res->end -
				host->res->start + 1, pdev->name)) {
		printk("%s: line %d: ERROR - request_mem_region failed\n",__FUNCTION__,__LINE__);
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
		card_gpio_status = host->plat_data->status(host->mmc->parent);
		if (card_gpio_status) {
			set_irq_type(host->detect_irq, IRQT_FALLING);
		} else {
			set_irq_type(host->detect_irq, IRQT_RISING);
		}
	} while (card_gpio_status !=
		 host->plat_data->status(host->mmc->parent));

	if (card_gpio_status==host->plat_data->card_inserted_state)
	{
		Set_Insertion_State(host,E_INS_STATE_INSERTED);
		Set_Request_State(host,E_REQUEST_STATE_IDLE);
		Set_PM_State(host,E_PM_STATE_ACTIVE);
	}
	else
	{
		Set_Insertion_State(host,E_INS_STATE_IDLE);
		Set_Request_State(host,E_REQUEST_STATE_IDLE);
		Set_PM_State(host,E_PM_STATE_ACTIVE);
	}

	ret =
	    request_irq(host->detect_irq, mxcmci_gpio_irq, 0, pdev->name, host);
	if (ret) {
		goto out1;
	}

	mxcmci_softreset(host);

	if (__raw_readl(host->base + MMC_REV_NO) != SDHC_REV_NO) {
		printk("%s: line %d: ERROR - wrong rev.no. 0x%08x. aborting.\n",__FUNCTION__,__LINE__,MMC_REV_NO);
		goto out3;
	}
	__raw_writel(READ_TO_VALUE, host->base + MMC_READ_TO);

	__raw_writel(INT_CNTR_END_CMD_RES | INT_CNTR_WRITE_OP_DONE |
			INT_CNTR_READ_OP_DONE, host->base + MMC_INT_CNTR);

	//Initialize poll mask
	host->poll_mask=0;

	//initialize list of work parameters
	INIT_LIST_HEAD(&host->work_param_list.list);
	for (i=0;i<MAX_WORK_PARAM_LIST_SIZE;i++)
	{
		host->work_param_table[i]= (struct mxcmci_work_param_list *)kzalloc(sizeof(struct mxcmci_work_param_list),GFP_KERNEL);
		INIT_WORK(&host->work_param_table[i]->work,NULL,NULL);
		list_add(&(host->work_param_table[i]->list), &(host->work_param_list.list));
	}

	//initialize wait queue api lock
	init_waitqueue_head(&host->lock_api);

	//initialize API mutex
	mutex_init(&host->api_mutex);
	host->api_mutex_flag=0;

	//initialize context mutex
	mutex_init(&host->ctx_lock);
	host->ctx_lock_flag=0;

	//initialize harware access spinlock
	spin_lock_init(&host->irq_lock);

	//Initialize timer
	setup_timer(&host->wd_timer,watchdog_timer,(unsigned long)host);

	ret = request_irq(host->irq, mxcmci_irq, 0, pdev->name, host);
	if (ret) {
		goto out3;
	}

	if ( gpio_sdhc_active(pdev->id) != 0 ) {
		goto out4;
	}
	if ((ret = mmc_add_host(mmc)) < 0) {
		goto out5;
	}

	printk(KERN_INFO "%s-%d found\n", pdev->name, pdev->id);

#if 0 //DGI, if you need to debug the init
#ifdef DO_ULOG
#include "../../mxc/ulog/ulog.h"
	{
		struct ulog_setup_cfg setup_cfg;
		//initialize ulog driver
		setup_cfg.size = 1000000;
		setup_cfg.mask = 0xbfffffff;
        	ulog_setup(setup_cfg);
		//start capture
		ulog_start();
	}
#endif
#endif

	return 0;

      out5:
	gpio_sdhc_inactive(pdev->id);
	  out4:
	free_irq(host->irq, host);
      out3:
	free_irq(host->detect_irq, host);
	//pr_debug("%s: Error in initializing....", pdev->name);
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
 * This function is called when a remove work has been issue.
 *
 * @param   p callback parameter.
 */

static void mxcmci_remove_work(void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mmc_host *mmc=(struct mmc_host *)param->drv_context;
	struct mxcmci_host *host = mmc_priv(mmc);
	struct platform_device *pdev=(struct platform_device *)param->dev_context;

	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	if (mmc) {
		struct mxcmci_work_param_list *tmp;
		struct list_head *pos;
		int i;
		unsigned long flags;

		LOCK_CTX
		LOCK_IRQ

		//queue the parameter in the list
		add_elt_to_list(host,param);

		mmc_remove_host(mmc);
		free_irq(host->irq, host);
		free_irq(host->detect_irq, host);

		flush_workqueue(pg_workqueue);

		//free list of work parameters
		list_for_each(pos, &host->work_param_list.list){
			tmp= list_entry(pos, struct mxcmci_work_param_list, list);
			list_del(pos);
		}

		//free parameters
		for (i=0;i<MAX_WORK_PARAM_LIST_SIZE;i++)
		{
			kfree(host->work_param_table[i]);
		}
		UNLOCK_IRQ
		UNLOCK_CTX

#ifdef MXC_MMC_DMA_ENABLE
		mxc_dma_free(host->dma);
#endif
		release_mem_region(host->res->start,
				   host->res->end - host->res->start + 1);

		mmc_free_host(mmc);
		gpio_sdhc_inactive(pdev->id);
	}

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
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
	struct mxcmci_work_param_list *tmp=NULL;
	struct mxcmci_host *host=mmc_priv(mmc);
	struct list_head *pos;
	unsigned long flags;

	platform_set_drvdata(pdev, NULL);

	LOCK_IRQ
	if (list_empty(&host->work_param_list.list))
	{
		panic("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);	
	}

	/* Find the entry in the list */
	list_for_each(pos, &host->work_param_list.list){
		tmp = list_entry(pos,struct mxcmci_work_param_list, list);
		list_del(pos);
		break;
	}

	if (tmp==NULL) 
	{
		panic("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
	}

	tmp->drv_context=(void*)host;
	tmp->dev_context=(void*)pdev;

	/* complete the command, finish request is differed to the workqueue */
	PREPARE_WORK(&tmp->work, mxcmci_remove_work, tmp);
	if (!queue_work(pg_workqueue, &tmp->work))
	{
		printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
	}
	UNLOCK_IRQ

	return 0;
}

#ifdef CONFIG_PM

/*!
 * This function is called to put the SDHC in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param  host driver context.
 * @param  state suspend state.
 */
static void mxcmci_do_suspend(struct mxcmci_host *host,pm_message_t state)
{
	struct mmc_host *mmc=host->mmc;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_SUSPEND,0);
#endif

	
	pr_debug("%s: line %d: MMC%d, suspend driver\n",__FUNCTION__,__LINE__, host->id);

	if (mmc_suspend_host(mmc,state))
	{
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			printk("%s: line %d: MMC%d, ERROR - mmc_suspend_host has returned an error\n",__FUNCTION__,__LINE__, host->id);
	}
	clk_disable(host->clk);

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_SUSPEND_DONE,0);
#endif
}

/*!
 * This function is called when a suspension request work has been issued by the suspend method.
 *
 * @param   p private context.
 */

static void mxcmci_suspend_work(void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	int state;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_SUSPEND_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	//run insertion state machine
	state=Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_IDLE:
		case E_INS_STATE_INSERTED:
		//nothing to do
		break;
		default:
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run request state machine
	state=Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_IDLE:
		//nothing to do
		break;
		default:
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run Power Management state machine
	state=Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		{
#ifdef CONFIG_PM
			mxcmci_do_suspend(host,param->state);
#endif
			Set_PM_State(host,E_PM_STATE_SUSPENDED);
		}
		break;
		case E_PM_STATE_SUSPENDED:
		{
			//let's call again suspend, it should not hurt.
#ifdef CONFIG_PM
			mxcmci_do_suspend(host,param->state);
#endif
		}
		break;
		default:
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}	

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX
	UNLOCK_API

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_SUSPEND_WORK_DONE,0);
#endif
}

/*!
 * This function is called when suspend is requested by the user space.
 *
 * @param   pdev  the device structure used to give information on which SDHC
 *                to resume
 *
 * @param   state the power state the device is entering
 *
 * @return  The function always returns 0.
 */
static int mxcmci_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct mxcmci_host *host=mmc_priv(mmc);
	struct mxcmci_work_param_list *tmp=NULL;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_SUSPEND,0);
#endif

	if (mmc==NULL)
	{
		printk("%s: line %d: MMC%d, ERROR - Do not support suspend with undefined driver context\n",__FUNCTION__,__LINE__,host->id);
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
		ulog(ULOG_KERNEL_MMC_MXCMCI_SUSPEND_DONE,__LINE__);
#endif
		return 0;
	}

	LOCK_API_MUTEX
	LOCK_API
	LOCK_CTX
	LOCK_IRQ

	tmp=get_elt_from_list(host);
	if (tmp==NULL)
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
		UNLOCK_API;
	}

	tmp->drv_context=(void*)host;
	tmp->state=state;

	PREPARE_WORK(&tmp->work, mxcmci_suspend_work,tmp);
	if (!queue_work(pg_workqueue, &tmp->work))
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
	}
	UNLOCK_IRQ
	UNLOCK_CTX
	UNLOCK_API_MUTEX

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_SUSPEND_DONE,__LINE__);
#endif
	return 0;
}

/*!
 * This function is called to bring the SDHC back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   host driver context.
 */
static void mxcmci_do_resume(struct mxcmci_host *host)
{
	struct mmc_host *mmc=host->mmc;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_RESUME,0);
#endif

	clk_enable(host->clk);

	if (mmc_resume_host(mmc)!=0)
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - mmc_resume_host has returned an error\n",__FUNCTION__,__LINE__, host->id);
	}

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_DO_RESUME_DONE,0);
#endif
}

/*!
 * This function is called when a resume request work has been issued by the resume method.
 *
 * @param   p private context.
 */

static void mxcmci_resume_work(void *p)
{
	struct mxcmci_work_param_list *param=(struct mxcmci_work_param_list *)p;
	struct mxcmci_host *host =  (struct mxcmci_host *)param->drv_context;
	int state;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_RESUME_WORK,0);
#endif
	pr_debug("%s: line %d: MMC%d, enter\n",__FUNCTION__,__LINE__, host->id);

	//run insertion state machine
	state=Get_Insertion_State(host);
	switch (state)
	{
		case E_INS_STATE_IDLE:
		case E_INS_STATE_INSERTED:
		// does not matter
		break;	
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run request state machine, we should be in idle
	state=Get_Request_State(host);
	switch (state)
	{
		case E_REQUEST_STATE_IDLE:break;
		default:
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//run Power Management state machine
	state=Get_PM_State(host);
	switch (state)
	{
		case E_PM_STATE_ACTIVE:
		{
			//nothing to do, already active
		}
		break;
		case E_PM_STATE_SUSPENDED:
		{
#ifdef CONFIG_PM
			mxcmci_do_resume(host);
#endif
			Set_PM_State(host,E_PM_STATE_ACTIVE);
		}
		break;
		default:
#ifdef DO_ULOG
			ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
			panic("%s: line %d: MMC%d, ERROR - event not supported in state <0x%x>\n",__FUNCTION__,__LINE__, host->id,state);
		break;
	}

	//queue the parameter in the list
	LOCK_CTX
	LOCK_IRQ
	add_elt_to_list(host,param);
	UNLOCK_IRQ
	UNLOCK_CTX
	UNLOCK_API

	pr_debug("%s: line %d: MMC%d, exit\n",__FUNCTION__,__LINE__, host->id);
#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_RESUME_WORK_DONE,0);
#endif
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
	struct mxcmci_host *host=mmc_priv(mmc);
	struct mxcmci_work_param_list *tmp=NULL;
	int ret = 0;
	unsigned long flags;

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_RESUME,__LINE__);
#endif

	if (mmc==NULL)
	{
		printk("%s: line %d: MMC%d, ERROR - Do not support resume with undefined driver context\n",__FUNCTION__,__LINE__,host->id);
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
		ulog(ULOG_KERNEL_MMC_MXCMCI_RESUME_DONE,__LINE__);
#endif
		return 0;
	}

	LOCK_API_MUTEX
	LOCK_API
	LOCK_CTX
	LOCK_IRQ

	tmp=get_elt_from_list(host);
	if (tmp==NULL)
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - list empty\n",__FUNCTION__,__LINE__,host->id);
		UNLOCK_API;
	}

	tmp->drv_context=(void*)host;

	PREPARE_WORK(&tmp->work, mxcmci_resume_work,tmp);
	if (!queue_work(pg_workqueue, &tmp->work))
	{
#ifdef DO_ULOG
		ulog(ULOG_KERNEL_MMC_MXC_MMC_ERROR,__LINE__);
#endif
		printk("%s: line %d: MMC%d, ERROR - queue work failed\n",__FUNCTION__,__LINE__,host->id);
	}
	UNLOCK_IRQ
	UNLOCK_CTX
	UNLOCK_API_MUTEX

#ifdef DO_ULOG
	ulog(ULOG_KERNEL_MMC_MXCMCI_RESUME_DONE,__LINE__);
#endif
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
	pg_workqueue = create_singlethread_workqueue("mmc-mxc-workqueue");
	if (!pg_workqueue) {
		panic("%s: line %d: ERROR - cannot create workqueue\n",__FUNCTION__,__LINE__);
	}

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
