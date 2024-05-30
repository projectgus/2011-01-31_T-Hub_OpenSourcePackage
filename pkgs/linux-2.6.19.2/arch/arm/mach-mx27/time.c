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
 * @defgroup Timers_MX27 RTC, OS tick, Watchdog Timers
 * @ingroup MSL_MX27
 */
/*!
 * @file mach-mx27/time.c
 * @brief This file contains OS tick implementations.
 *
 * This file contains OS tick implementations.
 *
 * @ingroup Timers_MX27
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clocksource.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach/time.h>

#include <asm/arch/hardware.h>

#ifndef __noinstrument
#define __noinstrument
#endif

extern unsigned long clk_early_get_timer_rate(void);

/* OS tick defines */
#define MXC_GPT_INT_TICK		INT_GPT
#define MXC_GPT_TCMP_TICK		MXC_GPT_TCMP(MXC_TIMER_GPT1)
#define MXC_GPT_TSTAT_TICK		MXC_GPT_TSTAT(MXC_TIMER_GPT1)
#define MXC_GPT_TCTL_TICK		MXC_GPT_TCTL(MXC_TIMER_GPT1)
#define MXC_GPT_TPRER_TICK		MXC_GPT_TPRER(MXC_TIMER_GPT1)
#define MXC_GPT_TCN_TICK		MXC_GPT_TCN(MXC_TIMER_GPT1)
/* High resolution timer defines */
#define MXC_GPT_INT_HRT			INT_GPT2
#define MXC_GPT_TCMP_HRT		MXC_GPT_TCMP(MXC_TIMER_GPT2)
#define MXC_GPT_TSTAT_HRT		MXC_GPT_TSTAT(MXC_TIMER_GPT2)
#define MXC_GPT_TCTL_HRT		MXC_GPT_TCTL(MXC_TIMER_GPT2)
#define MXC_GPT_TPRER_HRT		MXC_GPT_TPRER(MXC_TIMER_GPT2)
#define MXC_GPT_TCN_HRT			MXC_GPT_TCN(MXC_TIMER_GPT2)

/*!
 * This is the timer interrupt service routine to do required tasks.
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

	do {
		timer_tick();
		next_match = __raw_readl(MXC_GPT_TCMP_TICK) + LATCH;
		__raw_writel(GPT_TSTAT_COMP, MXC_GPT_TSTAT_TICK);
		__raw_writel(next_match, MXC_GPT_TCMP_TICK);
	} while ((signed long)(next_match - __raw_readl(MXC_GPT_TCN_TICK)) <=
		 0);

	write_sequnlock(&xtime_lock);

	return IRQ_HANDLED;
}

#ifndef CONFIG_GENERIC_TIME
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
	    __raw_readl(MXC_GPT_TCMP_TICK) - __raw_readl(MXC_GPT_TCN_TICK);

	/* We need elapsed ticks since last match */
	elapsed = LATCH - ticks_to_match;

	/* Now convert them to usec */
	usec = (unsigned long)(elapsed * (tick_nsec / 1000)) / LATCH;

	return usec;
}
#endif /* CONFIG_GENERIC_TIME */

cycle_t mx27_hrt_get_cycles(void)
{
	unsigned long t = __raw_readl(MXC_GPT_TCN_HRT);
	return t;
}

static struct clocksource clocksource_mx27_hrt = {
	.name 		= "mx27_hrt",
	.rating		= 200,
	.read		= mx27_hrt_get_cycles,
	.mask		= CLOCKSOURCE_MASK(32),
	/*
	 * Choose a shift value for the denominator that
	 * will give the lowest error when converting
	 * the 13.3 MHz clock to nanoseconds.  A value of
	 * 22 results in an error of about .000317 Hz,
	 * which is far less than the crystal stability.
	 */
	.shift 		= 22,
	.is_continuous 	= 1,
};

static int __init mx27_hrt_clocksource_init(void)
{
	int ret;
	u32 reg, v;

	__raw_writel(0, MXC_GPT_TCTL_HRT);
	__raw_writel(GPT_TCTL_SWR, MXC_GPT_TCTL_HRT);

	while ((__raw_readl(MXC_GPT_TCTL_HRT) & GPT_TCTL_SWR) != 0) {
		mb();
	}
	__raw_writel(GPT_TCTL_FRR | GPT_TCTL_COMPEN | GPT_TCTL_SRC_PER1,
		     MXC_GPT_TCTL_HRT);

	v = clk_early_get_timer_rate();
	__raw_writel((v / CLOCK_TICK_RATE) - 1, MXC_GPT_TPRER_HRT);

	if ((v % CLOCK_TICK_RATE) != 0) {
		pr_info("\nWARNING1: Can't generate HRT CLOCK_TICK_RATE at %d Hz\n",
			CLOCK_TICK_RATE);
	}
	pr_info("Actual HRT CLOCK_TICK_RATE is %d Hz\n",
		v / ((__raw_readl(MXC_GPT_TPRER_HRT) & 0x7FF) + 1));

	clocksource_mx27_hrt.mult =
		clocksource_hz2mult(CLOCK_TICK_RATE,
				    clocksource_mx27_hrt.shift);
	ret = clocksource_register(&clocksource_mx27_hrt);

	reg = __raw_readl(MXC_GPT_TCTL_HRT) | GPT_TCTL_TEN;
	__raw_writel(reg, MXC_GPT_TCTL_HRT);

	return ret;
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

	__raw_writel(0, MXC_GPT_TCTL_TICK);
	__raw_writel(GPT_TCTL_SWR, MXC_GPT_TCTL_TICK);

	while ((__raw_readl(MXC_GPT_TCTL_TICK) & GPT_TCTL_SWR) != 0)
		mb();

	reg = GPT_TCTL_FRR | GPT_TCTL_COMPEN | GPT_TCTL_SRC_PER1;

	__raw_writel(reg, MXC_GPT_TCTL_TICK);

	v = clk_early_get_timer_rate();
	__raw_writel((v / CLOCK_TICK_RATE) - 1, MXC_GPT_TPRER_TICK);

	if ((v % CLOCK_TICK_RATE) != 0) {
		pr_info("\nWARNING: Can't generate CLOCK_TICK_RATE at %d Hz\n",
			CLOCK_TICK_RATE);
	}
	pr_info("Actual CLOCK_TICK_RATE is %d Hz\n",
		v / ((__raw_readl(MXC_GPT_TPRER_TICK) & 0x7FF) + 1));

	reg = __raw_readl(MXC_GPT_TCN_TICK);
	reg += LATCH;
	__raw_writel(reg, MXC_GPT_TCMP_TICK);

	setup_irq(MXC_GPT_INT_TICK, &timer_irq);

	reg = __raw_readl(MXC_GPT_TCTL_TICK) | GPT_TCTL_TEN;
	__raw_writel(reg, MXC_GPT_TCTL_TICK);

	mx27_hrt_clocksource_init();

}

struct sys_timer mxc_timer = {
	.init = mxc_init_time,
#ifndef CONFIG_GENERIC_TIME
	.offset = mxc_gettimeoffset,
#endif
};
