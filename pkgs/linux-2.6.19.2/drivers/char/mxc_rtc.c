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

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/rtc.h>
#include <asm/mach/time.h>
#include <asm/uaccess.h>
#include "mxc_rtc.h"

/*!
 * @file mxc_rtc.c
 * @brief Real Time Clock interface
 *
 * This file contains Real Time Clock interface for Linux.
 *
 * @ingroup Timers
 */

#ifdef CONFIG_MXC_MC13783_RTC
#include <asm/arch/pmic_rtc.h>
#else
#define pmic_rtc_get_time(args)	MXC_EXTERNAL_RTC_NONE
#define pmic_rtc_set_time(args)	MXC_EXTERNAL_RTC_NONE
#define pmic_rtc_loaded()		0
#endif

#define RTC_VERSION		"1.0"
#define MXC_EXTERNAL_RTC_OK	0
#define MXC_EXTERNAL_RTC_ERR	-1
#define MXC_EXTERNAL_RTC_NONE	-2

/*!
 * This function reads the RTC value from some external source.
 *
 * @param  second       pointer to the returned value in second
 *
 * @return 0 if successful; non-zero otherwise
 */
int get_ext_rtc_time(u32 * second)
{
	int ret = 0;
	struct timeval tmp;

	if (!pmic_rtc_loaded()) {
		return MXC_EXTERNAL_RTC_NONE;
	}

	ret = pmic_rtc_get_time(&tmp);

	if (0 == ret)
		*second = tmp.tv_sec;
	else
		ret = MXC_EXTERNAL_RTC_ERR;

	return ret;
}

/*!
 * This function sets external RTC
 *
 * @param  second       value in second to be set to external RTC
 *
 * @return 0 if successful; non-zero otherwise
 */
int set_ext_rtc_time(u32 second)
{
	int ret = 0;
	struct timeval tmp;

	if (!pmic_rtc_loaded()) {
		return MXC_EXTERNAL_RTC_NONE;
	}

	tmp.tv_sec = second;

	ret = pmic_rtc_set_time(&tmp);

	if (0 != ret)
		ret = MXC_EXTERNAL_RTC_ERR;

	return ret;
}

static u32 rtc_freq = 2;	/* minimun value for PIE */
static unsigned long rtc_status;

static struct rtc_time g_rtc_alarm = {
	.tm_year = 0,
	.tm_mon = 0,
	.tm_mday = 0,
	.tm_hour = 0,
	.tm_mon = 0,
	.tm_sec = 0,
};

static DEFINE_SPINLOCK(rtc_lock);

/*!
 * This function is used to obtain the RTC time or the alarm value in
 * second.
 *
 * @param  time_alarm   use MXC_RTC_TIME for RTC time value; MXC_RTC_ALARM for alarm value
 *
 * @return The RTC time or alarm time in second.
 */
static u32 get_alarm_or_time(int time_alarm)
{
	u32 day, hr, min, sec, hr_min;

	if (time_alarm == MXC_RTC_TIME) {
		day = *_reg_RTC_DAYR;
		hr_min = *_reg_RTC_HOURMIN;
		sec = *_reg_RTC_SECOND;
	} else if (time_alarm == MXC_RTC_ALARM) {
		day = *_reg_RTC_DAYALARM;
		hr_min = (0x0000FFFF) & (*_reg_RTC_ALRM_HM);
		sec = *_reg_RTC_ALRM_SEC;
	} else {
		panic("wrong value for time_alarm=%d\n", time_alarm);
	}

	hr = hr_min >> 8;
	min = hr_min & 0x00FF;

	return ((((day * 24 + hr) * 60) + min) * 60 + sec);
}

/*!
 * This function sets the RTC alarm value or the time value.
 *
 * @param  time_alarm   the new alarm value to be updated in the RTC
 * @param  time         use MXC_RTC_TIME for RTC time value; MXC_RTC_ALARM for alarm value
 */
static void set_alarm_or_time(int time_alarm, u32 time)
{
	u32 day, hr, min, sec, temp;

	day = time / 86400;
	time -= day * 86400;
	/* time is within a day now */
	hr = time / 3600;
	time -= hr * 3600;
	/* time is within an hour now */
	min = time / 60;
	sec = time - min * 60;

	temp = (hr << 8) + min;

	if (time_alarm == MXC_RTC_TIME) {
		*_reg_RTC_DAYR = day;
		*_reg_RTC_SECOND = sec;
		*_reg_RTC_HOURMIN = temp;
	} else if (time_alarm == MXC_RTC_ALARM) {
		*_reg_RTC_DAYALARM = day;
		*_reg_RTC_ALRM_SEC = sec;
		*_reg_RTC_ALRM_HM = temp;
	} else {
		panic("wrong value for time_alarm=%d\n", time_alarm);
	}
}

/*!
 * This function updates the RTC alarm registers and then clears all the
 * interrupt status bits.
 *
 * @param  alrm         the new alarm value to be updated in the RTC
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int rtc_update_alarm(struct rtc_time *alrm)
{
	struct rtc_time alarm_tm, now_tm;
	unsigned long now, time;
	int ret;

	now = get_alarm_or_time(MXC_RTC_TIME);
	rtc_time_to_tm(now, &now_tm);
	rtc_next_alarm_time(&alarm_tm, &now_tm, alrm);
	ret = rtc_tm_to_time(&alarm_tm, &time);

	/* clear all the interrupt status bits */
	*_reg_RTC_RTCISR = *_reg_RTC_RTCISR;

	set_alarm_or_time(MXC_RTC_ALARM, time);

	return ret;
}

/*!
 * This function is the RTC interrupt service routine.
 *
 * @param  irq          RTC IRQ number
 * @param  dev_id       device ID which is not used
 *
 * @return IRQ_HANDLED as defined in the include/linux/interrupt.h file.
 */
static irqreturn_t mxc_rtc_interrupt(int irq, void *dev_id)
{
	u32 status;
	u32 events = 0;

	spin_lock(&rtc_lock);

	status = (*_reg_RTC_RTCISR) & (*_reg_RTC_RTCIENR);
	/* clear interrupt sources */
	*_reg_RTC_RTCISR = status;

	/* clear alarm interrupt if it has occurred */
	if (status & RTC_ALM_BIT) {
		status &= ~RTC_ALM_BIT;
	}

	/* update irq data & counter */
	if (status & RTC_ALM_BIT) {
		events |= (RTC_AF | RTC_IRQF);
	}
	if (status & RTC_1HZ_BIT) {
		events |= (RTC_UF | RTC_IRQF);
	}
	if (status & PIT_ALL_ON) {
		events |= (RTC_PF | RTC_IRQF);
	}

	if ((status & RTC_ALM_BIT) && rtc_periodic_alarm(&g_rtc_alarm)) {
		rtc_update_alarm(&g_rtc_alarm);
	}

	spin_unlock(&rtc_lock);

	rtc_update(1, events);

	return IRQ_HANDLED;
}

/*!
 * This function is used to open the RTC driver by registering the RTC
 * interrupt service routine.
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_open(void)
{
	if (test_and_set_bit(1, &rtc_status))
		return -EBUSY;
	return 0;
}

/*!
 * clear all interrupts and release the IRQ
 */
static void mxc_rtc_release(void)
{
	spin_lock_irq(&rtc_lock);
	*_reg_RTC_RTCIENR = 0;	/* Disable all rtc interrupts */
	*_reg_RTC_RTCISR = 0xFFFFFFFF;	/* Clear all interrupt status */
	spin_unlock_irq(&rtc_lock);
	rtc_status = 0;
}

/*!
 * This function is used to support some ioctl calls directly.
 * Other ioctl calls are supported indirectly through the
 * arm/common/rtctime.c file.
 *
 * @param  cmd          ioctl command as defined in include/linux/rtc.h
 * @param  arg          value for the ioctl command
 *
 * @return  0 if successful or negative value otherwise.
 */
static int mxc_rtc_ioctl(unsigned int cmd, unsigned long arg)
{
	int i;

	switch (cmd) {
	case RTC_PIE_OFF:
		*_reg_RTC_RTCIENR &= ~PIT_ALL_ON;
		return 0;
	case RTC_IRQP_SET:
		if (arg < 2 || arg > MAX_PIE_FREQ || (arg % 2) != 0)
			return -EINVAL;	/* Also make sure a power of 2Hz */
		if ((arg > 64) && (!capable(CAP_SYS_RESOURCE)))
			return -EACCES;
		rtc_freq = arg;
		return 0;
	case RTC_IRQP_READ:
		return put_user(rtc_freq, (u32 *) arg);
	case RTC_PIE_ON:
		for (i = 0; i < MAX_PIE_NUM; i++) {
			if (PIE_BIT_DEF[i][0] == rtc_freq) {
				break;
			}
		}
		if (i == MAX_PIE_NUM) {
			return -EACCES;
		}
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR |= PIE_BIT_DEF[i][1];
		spin_unlock_irq(&rtc_lock);
		return 0;
	case RTC_AIE_OFF:
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR &= ~RTC_ALM_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;

	case RTC_AIE_ON:
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR |= RTC_ALM_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;

	case RTC_UIE_OFF:	/* UIE is for the 1Hz interrupt */
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR &= ~RTC_1HZ_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;

	case RTC_UIE_ON:
		spin_lock_irq(&rtc_lock);
		*_reg_RTC_RTCIENR |= RTC_1HZ_BIT;
		spin_unlock_irq(&rtc_lock);
		return 0;
	}

	return -EINVAL;
}

/*!
 * This function reads the current RTC time into tm in Gregorian date.
 *
 * @param  tm           contains the RTC time value upon return
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_read_time(struct rtc_time *tm)
{
	u32 val;

	/* Avoid roll-over from reading the different registers */
	do {
		val = get_alarm_or_time(MXC_RTC_TIME);
	} while (val != get_alarm_or_time(MXC_RTC_TIME));

	rtc_time_to_tm(val, tm);
	return 0;
}

/*!
 * This function sets the internal RTC time based on tm in Gregorian date.
 *
 * @param  tm           the time value to be set in the RTC
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_set_time(struct rtc_time *tm)
{
	unsigned long time;
	int ret;

	ret = rtc_tm_to_time(tm, &time);
	if (ret != 0) {
		printk("Failed to set local RTC\n");
		return ret;
	}

	/* Avoid roll-over from reading the different registers */
	do {
		set_alarm_or_time(MXC_RTC_TIME, time);
	} while (time != get_alarm_or_time(MXC_RTC_TIME));

	ret = set_ext_rtc_time(time);

	if (ret != MXC_EXTERNAL_RTC_OK) {
		if (ret == MXC_EXTERNAL_RTC_NONE) {
			pr_info("No external RTC\n");
			ret = 0;
		} else
			pr_info("Failed to set external RTC\n");
	}

	return ret;
}

/*!
 * This function is the actual implementation of set_rtc() which is used
 * by programs like NTPD to sync the internal RTC with some external clock
 * source.
 *
 * @return 0 if successful; non-zero if there is pending alarm.
 */
static int mxc_set_rtc(void)
{
	unsigned long current_time = xtime.tv_sec;

	if ((*_reg_RTC_RTCIENR & RTC_ALM_BIT) != 0) {
		/* make sure not to forward the clock over an alarm */
		unsigned long alarm = get_alarm_or_time(MXC_RTC_ALARM);
		if (current_time >= alarm &&
		    alarm >= get_alarm_or_time(MXC_RTC_TIME))
			return -ERESTARTSYS;
	}

	set_alarm_or_time(MXC_RTC_TIME, current_time);
	return 0;
}

/*!
 * This function reads the current alarm value into the passed in \b alrm
 * argument. It updates the \b alrm's pending field value based on the whether
 * an alarm interrupt occurs or not.
 *
 * @param  alrm         contains the RTC alarm value upon return
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_read_alarm(struct rtc_wkalrm *alrm)
{
	rtc_time_to_tm(get_alarm_or_time(MXC_RTC_ALARM), &alrm->time);
	alrm->pending = (((*_reg_RTC_RTCISR) & RTC_ALM_BIT) != 0) ? 1 : 0;

	return 0;
}

/*!
 * This function sets the RTC alarm based on passed in alrm.
 *
 * @param  alrm         the alarm value to be set in the RTC
 *
 * @return  0 if successful; non-zero otherwise.
 */
static int mxc_rtc_set_alarm(struct rtc_wkalrm *alrm)
{
	struct rtc_time atm, ctm;
	int ret;

	spin_lock_irq(&rtc_lock);
	if (rtc_periodic_alarm(&alrm->time)) {
		if (alrm->time.tm_sec > 59 ||
		    alrm->time.tm_hour > 23 || alrm->time.tm_min > 59) {
			ret = -EINVAL;
			goto out;
		}
		mxc_rtc_read_time(&ctm);
		rtc_next_alarm_time(&atm, &ctm, &alrm->time);
		ret = rtc_update_alarm(&atm);
	} else {
		if ((ret = rtc_valid_tm(&alrm->time)))
			goto out;
		ret = rtc_update_alarm(&alrm->time);
	}

	if (ret == 0) {
		memcpy(&g_rtc_alarm, &alrm->time, sizeof(struct rtc_time));

		if (alrm->enabled) {
			*_reg_RTC_RTCIENR |= RTC_ALM_BIT;
		} else {
			*_reg_RTC_RTCIENR &= ~RTC_ALM_BIT;
		}
	}
      out:
	spin_unlock_irq(&rtc_lock);

	return ret;
}

/*!
 * This function is used to provide the content for the /proc/driver/rtc
 * file.
 *
 * @param  buf          the buffer to hold the information that the driver wants to write
 *
 * @return  The number of bytes written into the rtc file.
 */
static int mxc_rtc_proc(char *buf)
{
	char *p = buf;

	p += sprintf(p, "alarm_IRQ\t: %s\n",
		     (((*_reg_RTC_RTCIENR) & RTC_ALM_BIT) != 0) ? "yes" : "no");
	p += sprintf(p, "update_IRQ\t: %s\n",
		     (((*_reg_RTC_RTCIENR) & RTC_1HZ_BIT) != 0) ? "yes" : "no");
	p += sprintf(p, "periodic_IRQ\t: %s\n",
		     (((*_reg_RTC_RTCIENR) & PIT_ALL_ON) != 0) ? "yes" : "no");
	p += sprintf(p, "periodic_freq\t: %d\n", rtc_freq);

	return p - buf;
}

/*!
 * The RTC driver structure
 */
static struct rtc_ops mxc_rtc_ops = {
	.owner = THIS_MODULE,
	.open = mxc_rtc_open,
	.release = mxc_rtc_release,
	.ioctl = mxc_rtc_ioctl,
	.read_time = mxc_rtc_read_time,
	.set_time = mxc_rtc_set_time,
	.read_alarm = mxc_rtc_read_alarm,
	.set_alarm = mxc_rtc_set_alarm,
	.proc = mxc_rtc_proc,
};

/*! MXC RTC Power management control */

static struct timespec mxc_rtc_delta;

static int __init mxc_rtc_probe(struct platform_device *pdev)
{
	struct clk *clkp;
	struct timespec tv;
	struct rtc_time temp_time;
	u32 sec;
	int ret;

	clkp = clk_get(&pdev->dev, "rtc_clk");
	clk_enable(clkp);

	/* Configure and enable the RTC */
	if ((ret =
	     request_irq(INT_RTC, mxc_rtc_interrupt, 0, "rtc", NULL)) != 0) {
		printk(KERN_ERR "rtc: IRQ%d already in use. \n", INT_RTC);
		return ret;
	}
	ret = register_rtc(&mxc_rtc_ops);
	if (ret < 0) {
		printk(KERN_ERR "Unable to register_rtc \n");
		free_irq(INT_RTC, NULL);
		return ret;
	}
	ret = get_ext_rtc_time(&sec);

	if (ret == MXC_EXTERNAL_RTC_OK) {
		rtc_time_to_tm(sec, &temp_time);
		mxc_rtc_set_time(&temp_time);
	} else if (ret == MXC_EXTERNAL_RTC_NONE) {
		pr_info("No external RTC clock\n");
	} else {
		pr_info("Reading external RTC failed\n");
	}
	tv.tv_nsec = 0;
	tv.tv_sec = get_alarm_or_time(MXC_RTC_TIME);
	do_settimeofday(&tv);

	set_rtc = mxc_set_rtc;
	*_reg_RTC_RTCCTL = RTC_INPUT_CLK | RTC_ENABLE_BIT;
	if (((*_reg_RTC_RTCCTL) & RTC_ENABLE_BIT) == 0) {
		printk(KERN_ALERT "rtc : hardware module can't be enabled!\n");
		return -1;
	}
	printk("Real TIme clock Driver v%s \n", RTC_VERSION);
	return ret;
}
static int __exit mxc_rtc_remove(struct device *dev)
{
	unregister_rtc(&mxc_rtc_ops);
	free_irq(INT_RTC, NULL);
	mxc_rtc_release();
	return 0;
}

/*!
 * This function is called to save the system time delta relative to
 * the MXC RTC when enterring a low power state. This time delta is
 * then used on resume to adjust the system time to account for time
 * loss while suspended.
 *
 * @param   pdev  not used
 * @param   state Power state to enter.
 *
 * @return  The function always returns 0.
 */
static int mxc_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct timespec tv;

	/* calculate time delta for suspend */
	tv.tv_nsec = 0;
	tv.tv_sec = get_alarm_or_time(MXC_RTC_TIME);
	save_time_delta(&mxc_rtc_delta, &tv);

	return 0;
}

/*!
 * This function is called to correct the system time based on the
 * current MXC RTC time relative to the time delta saved during
 * suspend.
 *
 * @param   pdev  not used
 *
 * @return  The function always returns 0.
 */
static int mxc_rtc_resume(struct platform_device *pdev)
{
	struct timespec tv;

	tv.tv_nsec = 0;
	tv.tv_sec = get_alarm_or_time(MXC_RTC_TIME);
	restore_time_delta(&mxc_rtc_delta, &tv);

	return 0;
}

/*!
 * Contains pointers to the power management callback functions.
 */
static struct platform_driver mxc_rtc_driver = {
	.driver = {
		   .name = "mxc_rtc",
		   .bus = &platform_bus_type,
		   },
	.probe = mxc_rtc_probe,
	.remove = __exit_p(mxc_rtc_remove),
	.suspend = mxc_rtc_suspend,
	.resume = mxc_rtc_resume,
};

/*!
 * This function creates the /proc/driver/rtc file and registers the device RTC
 * in the /dev/misc directory. It also reads the RTC value from external source
 * and setup the internal RTC properly.
 *
 * @return  -1 if RTC is failed to initialize; 0 is successful.
 */
static int __init mxc_rtc_init(void)
{
	int ret;
	ret = platform_driver_register(&mxc_rtc_driver);
	return ret;
}

/*!
 * This function removes the /proc/driver/rtc file and un-registers the
 * device RTC from the /dev/misc directory.
 */
static void __exit mxc_rtc_exit(void)
{
	platform_driver_unregister(&mxc_rtc_driver);

}

module_init(mxc_rtc_init);
module_exit(mxc_rtc_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Realtime Clock Driver (RTC)");
MODULE_LICENSE("GPL");
