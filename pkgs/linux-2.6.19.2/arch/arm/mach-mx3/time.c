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

/* System Timer Interrupt reconfigured to run in free-run mode.
 * Author: Vitaly Wool
 * Copyright 2004 MontaVista Software Inc.
 */

/*!
 * @defgroup Timers_MX31 RTC, OS tick, Watchdog Timers
 * @ingroup MSL_MX31
 */
/*!
 * @file mach-mx3/time.c
 * @brief This file contains OS tick and wdog timer implementations.
 *
 * This file contains OS tick and wdog timer implementations.
 *
 * @ingroup Timers_MX31
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/mach/time.h>
#include <asm/io.h>
#include "time_priv.h"
#include <linux/irq.h>

extern unsigned long clk_early_get_timer_rate(void);

/*!
 * This function converts system timer ticks to microseconds
 *
 * @param  x	system timer ticks
 *
 * @return elapsed microseconds
 */
unsigned long __noinstrument clock_to_usecs(unsigned long x)
{
	return (unsigned long)(x * (tick_nsec / 1000)) / LATCH;
}

/*
 * WatchDog
 */
typedef struct {
	volatile __u16 WDOG_WCR;	/* 16bit watchdog control reg */
	volatile __u16 WDOG_WSR;	/* 16bit watchdog service reg */
	volatile __u16 WDOG_WRSR;	/* 16bit watchdog reset status reg */
} wdog_reg_t;

/*!
 * The base addresses for the WDOG module
 */
static volatile wdog_reg_t *wdog_base[1] = {
	(volatile wdog_reg_t *)IO_ADDRESS(WDOG_BASE_ADDR),
};

/*!
 * The corresponding WDOG won't be serviced unless the corresponding global
 * variable is set to a non-zero value.
 */
volatile unsigned short g_wdog1_enabled;

/* WDOG WCR register's WT value */
static int wdog_tmout[1] = { WDOG1_TIMEOUT };

/*!
 * This function provides the required service for the watchdog to avoid
 * the timeout.
 */
static inline void kick_wd(void)
{
	if (g_wdog1_enabled) {
		/* issue the service sequence instructions */
		wdog_base[0]->WDOG_WSR = 0x5555;
		wdog_base[0]->WDOG_WSR = 0xAAAA;
	}
}

/*!
 * This is the watchdog initialization routine to setup the timeout
 * value and enable it.
 */
void mxc_wd_init(int port)
{
	unsigned volatile short timeout =
	    ((wdog_tmout[port] / 1000) * 2) << WDOG_WT;

	if (port == 0) {
		/* enable WD, suspend WD in DEBUG mode */
		wdog_base[port]->WDOG_WCR = timeout | WCR_WOE_BIT |
		    WCR_SRS_BIT | WCR_WDA_BIT | WCR_WDE_BIT | WCR_WDBG_BIT;
	}
}

#ifdef WDOG_SERVICE_PERIOD
static int g_wdog_count = 0;
#endif

/*!
 * This is the timer interrupt service routine to do required tasks.
 * It also services the WDOG timer at the frequency of twice per WDOG
 * timeout value. For example, if the WDOG's timeout value is 4 (2
 * seconds since the WDOG runs at 0.5Hz), it will be serviced once
 * every 2/2=1 second.
 *
 * @param  irq          GPT interrupt source number (not used)
 * @param  dev_id       this parameter is not used
 * @return always returns \b IRQ_HANDLED as defined in
 *         include/linux/interrupt.h.
 */
static irqreturn_t mxc_timer_interrupt(int irq, void *dev_id)
{
	unsigned int next_match;

	write_seqlock(&xtime_lock);

	if (__raw_readl(MXC_GPT_GPTSR) & GPTSR_OF1)
		do {
#ifdef WDOG_SERVICE_PERIOD
			if (g_wdog1_enabled &&
			    (++g_wdog_count >=
			     ((WDOG_SERVICE_PERIOD / 1000) * HZ))) {
				kick_wd();
				g_wdog_count = 0;	/* reset */
			}
#else
			kick_wd();
#endif				/* WDOG_SERVICE_PERIOD */
			timer_tick();
			next_match = __raw_readl(MXC_GPT_GPTOCR1) + LATCH;
			__raw_writel(GPTSR_OF1, MXC_GPT_GPTSR);
			__raw_writel(next_match, MXC_GPT_GPTOCR1);
		} while ((signed long)(next_match -
				       __raw_readl(MXC_GPT_GPTCNT)) <= 0);

	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

/*!
 * This function is used to obtain the number of microseconds since the last
 * timer interrupt. Note that interrupts is disabled by do_gettimeofday().
 *
 * @return the number of microseconds since the last timer interrupt.
 */
static unsigned long __noinstrument mxc_gettimeoffset(void)
{
	long ticks_to_match, elapsed, usec;

	/* Get ticks before next timer match */
	ticks_to_match =
	    __raw_readl(MXC_GPT_GPTOCR1) - __raw_readl(MXC_GPT_GPTCNT);

	/* We need elapsed ticks since last match */
	elapsed = LATCH - ticks_to_match;

	/* Now convert them to usec */
	usec = (unsigned long)(elapsed * (tick_nsec / 1000)) / LATCH;

	return usec;
}

/*!
 * The OS tick timer interrupt structure.
 */
static struct irqaction timer_irq = {
	.name = "MXC Timer Tick",
	.flags = IRQF_DISABLED | IRQF_TIMER,
	.handler = mxc_timer_interrupt
};

/*!
 * This function is used to initialize the GPT to produce an interrupt
 * every 10 msec. It is called by the start_kernel() during system startup.
 */
void __init mxc_init_time(void)
{
	u32 reg, v;
	reg = __raw_readl(MXC_GPT_GPTCR);
	reg &= ~GPTCR_ENABLE;
	__raw_writel(reg, MXC_GPT_GPTCR);
	reg |= GPTCR_SWR;
	__raw_writel(reg, MXC_GPT_GPTCR);

	while ((__raw_readl(MXC_GPT_GPTCR) & GPTCR_SWR) != 0)
		mb();

	reg = __raw_readl(MXC_GPT_GPTCR);

	reg = 0 * GPTCR_OM3_CLEAR | GPTCR_FRR | GPTCR_CLKSRC_HIGHFREQ;

	__raw_writel(reg, MXC_GPT_GPTCR);

	/* Normal clk api are not yet initialized, so use early verion */
	v = clk_early_get_timer_rate();

	__raw_writel((v / CLOCK_TICK_RATE) - 1, MXC_GPT_GPTPR);

	if ((v % CLOCK_TICK_RATE) != 0) {
		pr_info("\nWARNING: Can't generate CLOCK_TICK_RATE at %d Hz\n",
			CLOCK_TICK_RATE);
	}
	pr_info("Actual CLOCK_TICK_RATE is %d Hz\n",
		v / ((__raw_readl(MXC_GPT_GPTPR) & 0xFFF) + 1));

	reg = __raw_readl(MXC_GPT_GPTCNT);
	reg += LATCH;
	__raw_writel(reg, MXC_GPT_GPTOCR1);

	setup_irq(INT_GPT, &timer_irq);

	reg = __raw_readl(MXC_GPT_GPTCR);
	reg = GPTCR_FRR | GPTCR_CLKSRC_HIGHFREQ |
	    GPTCR_STOPEN |
	    GPTCR_DOZEN | GPTCR_WAITEN | GPTCR_ENMOD | GPTCR_ENABLE;
	__raw_writel(reg, MXC_GPT_GPTCR);

	__raw_writel(GPTIR_OF1IE, MXC_GPT_GPTIR);

#ifdef WDOG1_ENABLE
	mxc_wd_init(0);
	g_wdog1_enabled = 1;
#else
	g_wdog1_enabled = (wdog_base[0]->WDOG_WCR) & WCR_WDE_BIT;
#endif

	kick_wd();
}

EXPORT_SYMBOL(g_wdog1_enabled);
EXPORT_SYMBOL(mxc_wd_init);

struct sys_timer mxc_timer = {
	.init = mxc_init_time,
	.offset = mxc_gettimeoffset,
};
