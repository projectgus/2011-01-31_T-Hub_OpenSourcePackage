/*
 *  linux/drivers/char/watchdog/mxc_wdt.h
 *
 *  BRIEF MODULE DESCRIPTION
 *      MXC Watchdog timer register definitions
 *
 * Author: MontaVista Software, Inc.
 *       <AKuster@mvista.com> or <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc.
 * Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef __MXC_WDT_H__
#define __MXC_WDT_H__

#define MXC_WDT_WCR             IO_ADDRESS(WDOG_BASE_ADDR + 0x00)
#define MXC_WDT_WSR             IO_ADDRESS(WDOG_BASE_ADDR + 0x02)
#define MXC_WDT_WRSR            IO_ADDRESS(WDOG_BASE_ADDR + 0x04)
#define WCR_WOE_BIT             (1 << 6)
#define WCR_WDA_BIT             (1 << 5)
#define WCR_SRS_BIT             (1 << 4)
#define WCR_WRE_BIT             (1 << 3)
#define WCR_WDE_BIT             (1 << 2)
#define WCR_WDBG_BIT            (1 << 1)
#define WCR_WDZST_BIT           (1 << 0)
#define WDT_MAGIC_1             0x5555
#define WDT_MAGIC_2             0xAAAA

#define TIMER_MARGIN_MAX    	127
#define TIMER_MARGIN_DEFAULT	60	/* 60 secs */
#define TIMER_MARGIN_MIN	1

#define WDOG_SEC_TO_COUNT(s)  ((s * 2) << 8)
#define WDOG_COUNT_TO_SEC(c)  ((c >> 8) / 2)

#endif				/* __MXC_WDT_H__ */
