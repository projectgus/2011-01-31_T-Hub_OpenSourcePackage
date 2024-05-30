/***************************************************************************
 *            evtprobe.h
 *
 *  Mon Apr 30 16:24:11 2007
 *  Copyright  2007  Tristan LELONG
 *  Email tristan.lelong@os4i.com
 ****************************************************************************/

/*
* Includes
*/
#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>	/* modules */
#include <linux/init.h>		/* module_{init,exit}() */
#include <linux/netlink.h>	/* netlink socket */
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <asm-arm/arch-mxc/mxc.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm-arm/arch-mxc/gpio.h>
#include "../../arch/arm/mach-mx27/gpio_mux.h"

#ifndef NETLINK_EVTPROBE
#define NETLINK_EVTPROBE	29
#endif

#define MSGSIZE 1024
	
#ifndef PE5
#define PE5	MX27_PIN_PWMO //_MX27_BUILD_PIN(4, 5)
#endif

#ifndef PC14
#define PC14 MX27_PIN_TOUT //	_MX27_BUILD_PIN(2, 14)
#endif

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)<<16+(b)<<8+(c))
#endif

/// netlink socket
struct sock * nl_sock;

/// netlink buffer
struct sk_buff *nl_buf;
	
/// send msg
int send_netlink_msg(int event, long value, int priority);
	
/// thread pointer
static struct task_struct *th;

/// thread running
static int nl_thread(void * data);

/// irq handler function
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
static irqreturn_t k_irq_handler(int irq, void *dev_id);
#else
static irqreturn_t k_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
#endif

/// module load function
static int __init k_init(void);

/// module unload function
static void __exit k_exit(void);

