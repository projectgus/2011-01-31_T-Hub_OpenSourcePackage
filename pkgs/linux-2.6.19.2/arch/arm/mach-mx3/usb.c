/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *	otg_{get,set}_transceiver() are from arm/plat-omap/usb.c.
 *	which is Copyright (C) 2004 Texas Instruments, Inc.
 *
 * Modified by Sagemcom under GPL license on 15/09/2007
 * Copyright (c) 2010 Sagemcom All rights reserved.
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
 * @defgroup USB_MX31 ARC OTG USB Driver for i.MX31
 * @ingroup USB
 */

/*!
 * @file mach-mx3/usb.c
 *
 * @brief platform related part of usb driver.
 * @ingroup USB_MX31
 */

/*!
 *Include files
 */
#define DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/usb/otg.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/mach-types.h>

#include <asm/arch/board.h>
#include <asm/arch/arc_otg.h>
#include <asm/arch/gpio.h>
#include <asm/arch/ulpi.h>
#include <asm/arch/isp1301.h>
#include <asm/arch/board-mx31homescreen.h>
#define USE_PMIC
#ifdef USE_PMIC
#include <asm/arch/pmic_convity.h>
#include <asm/arch/pmic.h>
PMIC_CONVITY_HANDLE pg_pmic_handle=0;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#endif

#ifdef CONFIG_MACH_MX31ADS
#define PBC3_CLEAR	(PBC_BASE_ADDRESS + PBC_BCTRL3_CLEAR)
#define PBC3_SET	(PBC_BASE_ADDRESS + PBC_BCTRL3_SET)
#endif

#undef DEBUG
#undef VERBOSE

//#undef pr_debug
//#define pr_debug printk

#ifdef CONFIG_MACH_MX31HSV1
extern int gpio_usb_host_ulpi_active(void);
extern int gpio_usb_host_ulpi_inactive(void);
extern int gpio_usb_otg_ulpi_active(void);
extern int gpio_usb_otg_ulpi_inactive(void);
#endif
extern int gpio_usbh1_active(void);
extern void gpio_usbh1_inactive(void);
extern int gpio_usbh2_active(void);
extern void gpio_usbh2_inactive(void);
extern int gpio_usbotg_hs_active(void);
extern void gpio_usbotg_hs_inactive(void);
extern int gpio_usbotg_fs_active(void);
extern void gpio_usbotg_fs_inactive(void);

extern int gpio_usbh2_ulpi_reset(int active);
extern int gpio_usbotg_ulpi_reset(int active);

/* The dmamask must be set for EHCI to work */
static u64 ehci_dmamask = ~(u32) 0;

static struct clk *usb_clk;
static struct clk *usb_ahb_clk;
static struct clk *usb_pll;
extern int clk_get_usecount(struct clk *clk);
static int usb_is_ulpi_ok(volatile u32 * view);

struct ulpi_ids_t {
   u32 id;
   const char *name;
} ulpi_ids [] = {
   /*            VID     PID      name        */
   { MK_ULPI_ID(0x04cc, 0x1504), "'NXP ISP 1504'"},
   { MK_ULPI_ID(0x0424, 0x0006), "'SMSC USB3311'"},
};

/*
 * make sure USB_CLK is running at 60 MHz +/- 1000 Hz
 */
static int check_usbclk(void)
{
	unsigned long freq;

	usb_pll = clk_get(NULL, "usb_pll");
	clk_put(usb_pll);

	usb_ahb_clk = clk_get(NULL, "usb_ahb_clk");
	clk_enable(usb_ahb_clk);
	clk_put(usb_ahb_clk);

	usb_clk = clk_get(NULL, "usb_clk");
	freq = clk_get_rate(usb_clk);
	clk_put(usb_clk);
	if ((freq < 59999000) || (freq > 60001000)) {
		printk(KERN_ERR "USB_CLK=%lu, should be 60MHz\n", freq);
		return -1;
	}

	return 0;
}

#if defined(CONFIG_ISP1504_MXC) || defined(CONFIG_ISP1504_MXC_MODULE)
 
/*!
 * read ULPI register 'reg' thru VIEWPORT register 'view'
 *
 * @param       reg   register to read
 * @param       view  the ULPI VIEWPORT register address
 * @return		return ulpi register value
 */
u8 ulpi_read(int reg, volatile u32 *view)
{
	u32 data = 0;
	int cnt = 100000;
	int cnt2 = 10;

start:
	/* make sure interface is running */
	if (!(__raw_readl(view) & ULPIVW_SS)) {
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
			cnt--;
		} while (cnt && (data & ULPIVW_WU));
	}
	if (!cnt)
	{
		printk(KERN_ERR "%s: timeout on read for register 0x%x (ULPIVW_WU)\n",__FUNCTION__, reg);
	}
	cnt = 100000;
	/* read the register */
	__raw_writel((ULPIVW_RUN | (reg << ULPIVW_ADDR_SHIFT)) & ~ULPIVW_WRITE, view);
	do {			/* wait for completion */
		data = __raw_readl(view);
		cnt--;
	} while (cnt && (data & ULPIVW_RUN) );
	if (!cnt)
	{
		cnt = 100000;
		data = 0;

		if ((unsigned int)view == (unsigned int)&UH2_ULPIVIEW) // host2 ulpi
		{
			printk(KERN_ERR "%s: timeout on read for register 0x%x (ULPIVW_RUN)\nCount= %d. Reseting host2 phy and retrying.\n",
					 __FUNCTION__,  reg, 11 - cnt2);
			/* Reseting phy */
         gpio_usbh2_ulpi_reset(1);
         gpio_usbh2_ulpi_reset(0);
		} 
		else if ((unsigned int)view == (unsigned int)&UOG_ULPIVIEW) // OTG ulpi
		{
         printk(KERN_ERR "%s: timeout on read for register 0x%x (ULPIVW_RUN)\nCount= %d. Reseting OTG phy and retrying.\n",
               __FUNCTION__,  reg, 11 - cnt2);

         /* Reseting phy : release the reset */
         gpio_usbotg_ulpi_reset(1);
         gpio_usbotg_ulpi_reset(0);
      }
	if (--cnt2) goto start;
	}
	
return (u8) (data >> ULPIVW_RDATA_SHIFT) & ULPIVW_RDATA_MASK;
}

EXPORT_SYMBOL(ulpi_read);

/*!
 * set bits into OTG ULPI register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  set value
 * @param		reg   which register
 * @param       view  the ULPI VIEWPORT register address
 */
void ulpi_set(u8 bits, int reg, volatile u32 * view)
{
	u32 data=0;
	int cnt=100000;
	/* make sure interface is running */
	if (!(__raw_readl(view) & ULPIVW_SS)) {
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
			cnt--;
		} while (cnt && (data & ULPIVW_WU));
	}

	if (!cnt)
	{
		printk(KERN_ERR "%s: timeout on read for register 0x%x (ULPIVW_WU)\n", __FUNCTION__,reg);
	}
	cnt=100000;
	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ULPI_REG_SET) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);
	while ( cnt && (__raw_readl(view) & ULPIVW_RUN)) 	/* wait for completion */
	{
		cnt--;
	}
	if (!cnt)
	{
		printk(KERN_ERR "%s: timeout on read for register 0x%x (ULPIVW_WU)\n", __FUNCTION__,reg);
	}
}

EXPORT_SYMBOL(ulpi_set);

/*!
 * clear bits in OTG ULPI register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  bits to clear
 * @param	reg   in this register
 * @param       view  the ULPI VIEWPORT register address
 */
void ulpi_clear(u8 bits, int reg, volatile u32 * view)
{
	int cnt=100000;
	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ULPI_REG_CLEAR) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     	view);
	while ( cnt && (__raw_readl(view) & ULPIVW_RUN)) /* wait for completion */
	{
		cnt--;
	}
	if (!cnt)
	{
		printk(KERN_ERR "%s: timeout on read for register 0x%x (ULPIVW_RUN)\n", __FUNCTION__,reg);
	}
}

EXPORT_SYMBOL(ulpi_clear);

static int usb_is_ulpi_ok(volatile u32 * view)
{
#if defined(CONFIG_MACH_MX31HSV1)
	/* Use Viewport to check if the tranceiver is working correctly.
	 * requiered to handle board clocking failure on some versions. We need to do that
	 * before using the i.MX31 USB unit, because the chip locks up if the tranceiver
	 * does not respond. (Using the viewport, we can handle timeouts)
	 */
	int i;
	unsigned short vid, pid;
	unsigned long id;

	vid =  ulpi_read(ULPI_VID_HIGH, view) << 8;
	vid |= ulpi_read(ULPI_VID_LOW,  view) ;
	pid = ulpi_read(ULPI_PID_HIGH, view) << 8;
	pid |= ulpi_read(ULPI_PID_LOW,  view);

	printk(KERN_INFO "USB Port: ULPI Vendor ID 0x%04x  Product ID 0x%04x\n", vid, pid);

	id =  MK_ULPI_ID(vid,pid);

	for ( i=0; i<ARRAY_SIZE(ulpi_ids); i++)
	{
		if ( id == ulpi_ids[i].id ) {
			printk(KERN_INFO "Found %s USB tranceiver.\n", ulpi_ids[i].name );
			return 0;
		}
	}

	printk(KERN_ERR "%s: Cannot detect ULPI USB tranceiver 'VID_%04x:PID_%04x', aborting detection.\n", 
		__FUNCTION__,
		vid,
		pid
		);

	clk_disable(usb_clk);
	return -ENODEV;

#else  /* !CONFIG_MACH_MX31HSV1 */ 

	return 0;
#endif /* !CONFIG_MACH_MX31HSV1 */
}

/*!
 * set vbus power
 *
 * @param       on    power on or off
 * @param       view  the ULPI view register address
 */
static void ulpi_set_vbus_power(int on, volatile u32 * view)
{
#if defined(CONFIG_MACH_MX31LSV0)
	if (on) {
		ulpi_clear(DRV_VBUS_EXT ,	/* enable external Vbus (inverted logic) */
					  ULPI_OTGCTL, view);
						
		ulpi_set(USE_EXT_VBUS_IND , /* use external indicator */
			         ULPI_OTGCTL, view);

	} else {
		ulpi_clear(USE_EXT_VBUS_IND ,	/* use external indicator */
			          ULPI_OTGCTL, view);

		ulpi_set(DRV_VBUS_EXT ,	/* disable external Vbus (inverted logic) */
			        ULPI_OTGCTL, view);
	}
#elif defined(CONFIG_MACH_MX31HSV1)
	if (on) {
		ulpi_clear(DRV_VBUS_EXT ,	/* enable external Vbus (inverted logic) */
					  ULPI_OTGCTL, view);
						
		ulpi_set(USE_EXT_VBUS_IND , /* use external indicator */
			         ULPI_OTGCTL, view);

	} else {
		ulpi_clear(USE_EXT_VBUS_IND ,	/* use external indicator */
			          ULPI_OTGCTL, view);

		ulpi_set(DRV_VBUS_EXT ,	/* disable external Vbus (inverted logic) */
			        ULPI_OTGCTL, view);
	}
#elif defined(CONFIG_MACH_MX31ADS)
if (on) {
		ulpi_set(DRV_VBUS_EXT |	/* enable external Vbus */
			    DRV_VBUS |		/* enable internal Vbus */
			    USE_EXT_VBUS_IND |	/* use external indicator */
			    CHRG_VBUS,		/* charge Vbus */
			    ULPI_OTGCTL, view);
	} else {
		ulpi_clear(DRV_VBUS_EXT |	/* disable external Vbus */
			      DRV_VBUS,		/* disable internal Vbus */
			      ULPI_OTGCTL, view);
		ulpi_set(USE_EXT_VBUS_IND |	/* use external indicator */
			    DISCHRG_VBUS,	/* discharge Vbus */
			    ULPI_OTGCTL, view);
	}
#else
#error USB: Unknown MACH for USB PHY.
#endif

}
#endif

#if defined(CONFIG_USB_EHCI_ARC_OTGHS) || defined(CONFIG_USB_GADGET_ARC_OTGHS)
static int otg_used = 0;	/* OTG use-count */

static int otg_hs_set_xcvr(void)
{
	u32 tmp;
	
   /* set ULPI xcvr */
	tmp = UOG_PORTSC1 & ~PORTSC_PTS_MASK;
	tmp |= PORTSC_PTS_ULPI;
	UOG_PORTSC1 = tmp;

	mdelay(10);

	if ( usb_is_ulpi_ok(&UOG_ULPIVIEW) ) {
		printk(KERN_ERR "%s: USB OTG ULPI identification failed.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* need to reset the controller here so that the ID pin
	 * is correctly detected.
	 */
	UOG_USBCMD |= UCMD_RESET;

	/* allow controller to reset, and leave time for
	 * the ULPI transceiver to reset too.
	 */
	mdelay(10);
	
	/* Turn off the usbpll for ulpi tranceivers */
	clk_disable(usb_clk);
	pr_debug("%s: usb_pll usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_pll));
	return 0;
}

static int otg_hs_init(void)
{
	if (!otg_used) {
#ifdef CONFIG_MACH_MX31HSV1
		gpio_usb_otg_ulpi_active();
#endif

		pr_debug("%s: OTG-HS pins grabed\n", __FUNCTION__);

		clk_enable(usb_clk);
		pr_debug("%s: usb_pll usecount %d\n\n", __FUNCTION__,
			 clk_get_usecount(usb_pll));

		if (gpio_usbotg_hs_active())	/* grab our pins */
			return -EINVAL;

#ifdef CONFIG_MACH_MX31ADS
		/* enable OTG/HS */
		__raw_writew(PBC_BCTRL3_OTG_HS_EN, PBC3_CLEAR);

		/* disable OTG/FS */
		__raw_writew(PBC_BCTRL3_OTG_FS_EN, PBC3_SET);
#endif

		USBCTRL &= ~UCTRL_BPE;	/* disable bypass mode */
		USBCTRL |= UCTRL_OUIE |	/* ULPI intr enable */
		    UCTRL_OWIE |	/* OTG wakeup intr enable */
		    UCTRL_OPM;	/* power mask */
	}
	/* set transceiver type */
	otg_used++;
	return otg_hs_set_xcvr();
}

static void otg_hs_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

	otg_used--;
	if (!otg_used) {
#ifdef CONFIG_MACH_MX31ADS
		/* disable  OTG/HS */
		__raw_writew(PBC_BCTRL3_OTG_HS_EN, PBC3_SET);
#endif

		pr_debug("%s: freeing OTG-HS pins\n", __FUNCTION__);
		gpio_usbotg_hs_inactive();	/* release our pins */
#ifdef CONFIG_MACH_MX31HSV1
		gpio_usb_otg_ulpi_inactive();
#endif
	}
}

static void otg_hs_set_vbus_power(int on)
{
	printk(KERN_NOTICE "%s: Switching power on OTG bus to %s\n", __FUNCTION__, on ? "on" : "off");
	ulpi_set_vbus_power(on, &UOG_ULPIVIEW);
}

/*!
 * OTG HS host config
 */
#ifdef CONFIG_USB_EHCI_ARC_OTGHS
/* *INDENT-OFF* */
static struct arc_usb_config otg_hs_host_config = {
	.name            = "OTG HS Host",
	.platform_init   = otg_hs_init,
	.platform_uninit = otg_hs_uninit,
	.set_vbus_power  = otg_hs_set_vbus_power,
	.xcvr_type       = PORTSC_PTS_ULPI,
	.usbmode         = (u32) &UOG_USBMODE,
	.does_otg        = 1,
	.power_budget = 500,          /* DMO : 500 mA max power */
};
/* *INDENT-ON* */

static struct platform_device *otg_hs_host_device;
#endif				/* CONFIG_USB_EHCI_ARC_OTGHS */
				/* CONFIG_USB_EHCI_ARC_OTGHS || CONFIG_USB_GADGET_ARC_OTGHS */

/* *INDENT-OFF* */
static struct resource otg_resources[] = {
	{
		.start = (u32) (USB_OTGREGS_BASE),
		.end   = (u32) (USB_OTGREGS_BASE + 0x1ff),
		.flags = IORESOURCE_MEM,
	},
	{
		.start = INT_USB3,
		.flags = IORESOURCE_IRQ,
	},
};
#endif
/* *INDENT-ON* */

#if defined(CONFIG_USB_EHCI_ARC_OTGFS) || defined(CONFIG_USB_GADGET_ARC_OTGFS)
static void otg_fs_set_xcvr(void)
{
	u32 tmp;

	tmp = UOG_PORTSC1 & ~PORTSC_PTS_MASK;
	tmp |= PORTSC_PTS_SERIAL;
	UOG_PORTSC1 = tmp;
}
#endif				/* CONFIG_USB_EHCI_ARC_OTGFS || CONFIG_USB_GADGET_ARC_OTGFS */

#ifdef CONFIG_USB_EHCI_ARC_OTGFS
static int otg_fs_host_init(void)
{
	pr_debug("%s: OTG-FS pins grabed\n", __FUNCTION__);

	clk_enable(usb_clk);

	isp1301_init();

	isp1301_set_serial_host();
	if (gpio_usbotg_fs_active())	/* grab our pins */
		return -EINVAL;
	mdelay(1);

	/* enable OTG VBUS */
	clk_disable(usb_clk);
#ifdef CONFIG_MACH_MX31ADS
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_CLEAR);
#else
#warning CLD: TODO: USB VBUS Enable
#endif
	clk_enable(usb_clk);

	otg_fs_set_xcvr();	/* set transceiver type */

	USBCTRL &= ~(UCTRL_OSIC_MASK | UCTRL_BPE);	/* disable bypass mode */
	USBCTRL |= UCTRL_OSIC_SU6 |	/* single-ended, unidir, 6 wire */
	    UCTRL_OWIE |	/* OTG wakeup intr enable */
	    UCTRL_OPM;		/* power mask */

	/* need reset */
	UOG_USBCMD |= UCMD_RESET;
	mdelay(10);

	return 0;
}

static void otg_fs_host_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

#ifdef CONFIG_MACH_MX31ADS
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_SET);	/* disable OTG VBUS */
#else
#warning CLD: TODO: USB VBUS Enable
#endif

	isp1301_uninit();

	gpio_usbotg_fs_inactive();	/* release our pins */
	clk_disable(usb_clk);
}

/*!
 * OTG FS host config
 */
/* *INDENT-OFF* */
static struct arc_usb_config otg_fs_host_config = {
	.name            = "OTG FS Host",
	.platform_init   = otg_fs_host_init,
	.platform_uninit = otg_fs_host_uninit,
	.set_vbus_power  = isp1301_set_vbus_power,
	.xcvr_type       = PORTSC_PTS_SERIAL,
	.usbmode         = (u32) &UOG_USBMODE,
	.does_otg        = 1,
	.power_budget    = 150,		/* 150 mA max power */ // DDD check!!
};
/* *INDENT-ON* */

static struct platform_device *otg_fs_host_device;
#endif				/* CONFIG_USB_EHCI_ARC_OTGFS */

/* Host 1 */
#if defined(CONFIG_USB_EHCI_ARC_H1)
static void usbh1_set_xcvr(void)
{
	UH1_PORTSC1 &= ~PORTSC_PTS_MASK;
	UH1_PORTSC1 |= PORTSC_PTS_SERIAL;
}

#ifdef USE_PMIC
void pmic_convity_clbk(const PMIC_CONVITY_EVENTS event)
{
	switch(event)
	{
		case USB_DETECT_4V4_RISE:pr_debug("%s - USB Host 1 - Detect 4.4 V rise\n",__FUNCTION__);break;
		case USB_DETECT_4V4_FALL:pr_debug("%s - USB Host 1 - Detect 4.4 V fall\n",__FUNCTION__);break;
		case USB_DETECT_2V0_RISE:pr_debug("%s - USB Host 1 - Detect 2.0 V rise\n",__FUNCTION__);break;
		case USB_DETECT_2V0_FALL:pr_debug("%s - USB Host 1 - Detect 2.0 V fall\n",__FUNCTION__);break;
		case USB_DETECT_0V8_RISE:pr_debug("%s - USB Host 1 - Detect 0.8 V rise\n",__FUNCTION__);break;
		case USB_DETECT_0V8_FALL:pr_debug("%s - USB Host 1 - Detect 0.8 V fall\n",__FUNCTION__);break;
		case USB_DETECT_MINI_A:pr_debug("%s - USB Host 1 - Detect mini A\n",__FUNCTION__);break;
		case USB_DETECT_MINI_B:pr_debug("%s - USB Host 1 - Detect mini B\n",__FUNCTION__);break;
		case USB_DETECT_NON_USB_ACCESSORY:pr_debug("%s - USB Host 1 - Detect non USB accessory\n",__FUNCTION__);break;
		case USB_DETECT_FACTORY_MODE:printk("%s - USB Host 1 - Detect a factory mode connection\n",__FUNCTION__);break;
		case USB_DP_HI:pr_debug("%s - USB Host 1 - Event DP HI\n",__FUNCTION__);break;
		case USB_DM_HI:pr_debug("%s - USB Host 1 - Event DM HI\n",__FUNCTION__);break;
		default:
		pr_debug("%s - USB Host 1 - Unknown event %d\n",__FUNCTION__,event);
		break;
   }
}
#endif //#ifdef USE_PMIC

static int usbh1_init(void)
{
	pr_debug("%s: H1 pins grabed\n", __FUNCTION__);

	clk_enable(usb_clk);
	pr_debug("%s: usb_pll usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_pll));

	if (gpio_usbh1_active())
		return -EINVAL;

#ifdef CONFIG_MACH_MX31ADS
	__raw_writew(PBC_BCTRL3_FSH_EN, PBC3_CLEAR);	/* enable FSH */
	__raw_writew(PBC_BCTRL3_FSH_SEL, PBC3_SET);	/* Group B */
	__raw_writew(PBC_BCTRL3_FSH_MOD, PBC3_CLEAR);	/* single ended */
	__raw_writew(PBC_BCTRL3_FSH_VBUS_EN, PBC3_CLEAR);	/* enable FSH VBUS */
#endif

	if (get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC )
	{
#ifdef USE_PMIC
		PMIC_STATUS status;

		USBCTRL &= ~(UCTRL_H1SIC_MASK | UCTRL_BPE);	/* disable bypass mode */
		USBCTRL |= UCTRL_H1SIC_DU6 |	/* differential, unidirectional 6 wires. */
	    		UCTRL_H1WIE | UCTRL_H1DT |	/* disable H1 TLL */
	    		UCTRL_H1PM;		/* power mask */

		status=pmic_convity_open(&pg_pmic_handle,USB);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - Could not open connectivy service of ATLAS, status=0x%x\n",__FUNCTION__,status);
			pg_pmic_handle = 0;
			goto error;
		}

		printk("%s - pmic_convity_open OK\n",__FUNCTION__);
		status=pmic_convity_set_callback(pg_pmic_handle,
				      pmic_convity_clbk,
				      USB_DETECT_4V4_RISE | USB_DETECT_4V4_FALL |
					USB_DETECT_2V0_RISE | USB_DETECT_2V0_FALL |
					USB_DETECT_0V8_RISE | USB_DETECT_0V8_FALL |
					USB_DETECT_MINI_A | USB_DETECT_MINI_B |
					USB_DETECT_NON_USB_ACCESSORY | USB_DETECT_FACTORY_MODE 
               /* | USB_DP_HI | USB_DM_HI */ /* new USB device detectors */
               );
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_set_callback failed, status=0x%x\n",__FUNCTION__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}

		status = pmic_convity_usb_set_speed(pg_pmic_handle,USB_FULL_SPEED);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_usb_set_speed failed, status=0x%x\n",__FUNCTION__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}

		status = pmic_convity_set_output(pg_pmic_handle,1,0);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_set_output failed line %d, status=0x%x\n",__FUNCTION__,__LINE__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}

		status = pmic_convity_set_output(pg_pmic_handle,FALSE,TRUE);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_set_output failed line %d, status=0x%x\n",__FUNCTION__,__LINE__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}

		status = pmic_convity_usb_set_power_source(pg_pmic_handle,
					      USB_POWER_INTERNAL,
					      USB_POWER_3V3);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_usb_set_power_source failed, status=0x%x\n",__FUNCTION__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}
		
		status = pmic_convity_usb_set_xcvr(pg_pmic_handle,USB_DIFFERENTIAL_UNIDIR);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_usb_set_xcvr failed, status=0x%x\n",__FUNCTION__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}

		status = pmic_convity_usb_otg_set_config(pg_pmic_handle,
					    USB_OTG_SE0CONN | USB_DP150K_PU | USBXCVREN);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_usb_otg_set_config failed, status=0x%x\n",__FUNCTION__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}
      /* 
	status = pmic_event_unmask(EVENT_UDPM);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_event_unmask, status=0x%x : unable to allow USB interrupt for UDPM.\n",__FUNCTION__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}
      status = pmic_event_unmask(EVENT_UDMM);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_event_unmask, status=0x%x : unable to allow USB interrupt for UDMM.\n",__FUNCTION__,status);
			//status=pmic_convity_close(pg_pmic_handle);
			//pg_pmic_handle = 0;
		}*/



		error:;
#endif //#ifdef USE_PMIC
	}
	else
	{
		USBCTRL &= ~(UCTRL_H1SIC_MASK | UCTRL_BPE);	/* disable bypass mode */
		USBCTRL |= UCTRL_H1SIC_SU6 |	/* single-ended / unidir. */
	    		UCTRL_H1WIE | UCTRL_H1DT |	/* disable H1 TLL */
	    		UCTRL_H1PM;		/* power mask */
	}



	usbh1_set_xcvr();

	return 0;
}

static void usbh1_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

#ifdef USE_PMIC
	if (( get_board_type() >= BOARD_TYPE_V3_HOMESCREEN_GENERIC ) && (pg_pmic_handle != 0)) 
	{
		PMIC_STATUS status;

		status=pmic_convity_close(pg_pmic_handle);
		if (status != PMIC_SUCCESS)
		{
			printk("%s - pmic_convity_close failed, status=0x%x\n",__FUNCTION__,status);
		}

		printk("%s - pmic_convity closed OK\n", __FUNCTION__);
	}
#endif

#ifdef CONFIG_MACH_MX31ADS
	__raw_writew(PBC_BCTRL3_FSH_EN, PBC3_SET);	/* disable FSH */
	__raw_writew(PBC_BCTRL3_FSH_VBUS_EN, PBC3_SET);	/* disable FSH VBUS */
#endif
	gpio_usbh1_inactive();	/* release our pins */
	clk_disable(usb_clk);
}

/* *INDENT-OFF* */
static struct arc_usb_config usbh1_config = {
	.name            = "Host 1",
	.platform_init   = usbh1_init,
	.platform_uninit = usbh1_uninit,
	.xcvr_type       = PORTSC_PTS_SERIAL,
	.usbmode         = (u32) &UH1_USBMODE,
	.power_budget = 500,		/* VGA: 500 mA max power */
};

static struct resource usbh1_resources[] = {
	{
		.start = (u32) (USB_H1REGS_BASE),
		.end   = (u32) (USB_H1REGS_BASE + 0x1ff),
		.flags = IORESOURCE_MEM,
	},
	{
		.start = INT_USB1,
		.flags = IORESOURCE_IRQ,
	},
};
/* *INDENT-ON* */

static struct platform_device *usbh1_device;
#endif				/* CONFIG_USB_EHCI_ARC_H1 */

#ifdef CONFIG_USB_EHCI_ARC_H2
static int usbh2_set_xcvr(void)
{
	u32 tmp = 0;
	
	tmp = UH2_PORTSC1 & ~PORTSC_PTS_MASK;
	tmp |= PORTSC_PTS_ULPI;
	UH2_PORTSC1 = tmp;
	
	mdelay(10);

	if ( usb_is_ulpi_ok(&UH2_ULPIVIEW) )
	{
		printk(KERN_ERR "%s: USB Host2 ULPI identification failed.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* allow controller to reset, and leave time for
	 * the ULPI transceiver to reset too.
	 */
	mdelay(10);

	/* Turn off the usbpll for ulpi tranceivers */
	clk_disable(usb_clk);
	pr_debug("%s: usb_pll usecount %d\n", __FUNCTION__,
		 clk_get_usecount(usb_pll));
	return 0;
}

static int usbh2_init(void)
{
	pr_debug("%s: H2 pins grabed\n", __FUNCTION__);

	clk_enable(usb_clk);
	pr_debug("%s: usb_pll usecount %d\n", __FUNCTION__,
		 clk_get_usecount(usb_pll));

#ifdef CONFIG_MACH_MX31HSV1
	gpio_usb_host_ulpi_active();
#endif

#ifdef CONFIG_MACH_MX31ADS
	/* abort the init if NAND card is present */
	if ((__raw_readw(PBC_BASE_ADDRESS + PBC_BSTAT1) &
	     PBC_BSTAT1_NF_DET) == 0) {
		printk(KERN_ERR "USBH2 port not configured: "
		       "pin conflict with NAND flash.\n");
		return -EINVAL;
	}
	/*
	 * If the ATA interface is enabled, turning on
	 * PBC_BCTRL3_HSH_SEL will make it unusable.
	 * To avoid that, abort now if ATA is enabled.
	 */
	if ((__raw_readw(PBC_BASE_ADDRESS + PBC_BCTRL2_SET) &
	     PBC_BCTRL2_ATA_EN) == 0) {
		printk(KERN_ERR "USBH2 port not configured: "
		       "pin conflict with ATA.\n");
		return -EINVAL;
	}
	__raw_writew(PBC_BCTRL3_HSH_SEL, PBC3_SET);	/* enable HSH select */
	__raw_writew(PBC_BCTRL3_HSH_EN, PBC3_CLEAR);	/* enable HSH */
	
#endif

	if (gpio_usbh2_active())	/* grab our pins */
		return -EINVAL;

	USBCTRL &= ~(UCTRL_H2SIC_MASK | UCTRL_BPE);	/* disable bypass mode */
	USBCTRL |= UCTRL_H2WIE |	/* wakeup intr enable */
	    UCTRL_H2UIE |	/* ULPI intr enable */
	    UCTRL_H2DT |	/* disable H2 TLL */
	    UCTRL_H2PM;		/* power mask */

	pr_debug("%s: success\n", __FUNCTION__);

	return usbh2_set_xcvr();	/* set transceiver type */
}

static void usbh2_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

#ifdef CONFIG_MACH_MX31ADS
	__raw_writew(PBC_BCTRL3_HSH_SEL, PBC3_CLEAR);	/* disable HSH select */
	__raw_writew(PBC_BCTRL3_HSH_EN, PBC3_SET);	/* disable HSH */
#endif

	gpio_usbh2_inactive();	/* release our pins */
#ifdef CONFIG_MACH_MX31HSV1
	gpio_usb_host_ulpi_inactive();
#endif
}

static void usbh2_set_vbus_power(int on)
{
	printk(KERN_NOTICE "%s: Switching power on H2 bus to %s\n", __FUNCTION__, on ? "on" : "off");
	ulpi_set_vbus_power(on, &UH2_ULPIVIEW);
}

/* *INDENT-OFF* */
static struct arc_usb_config usbh2_config = {
	.name            = "Host 2",
	.platform_init   = usbh2_init,
	.platform_uninit = usbh2_uninit,
	.set_vbus_power  = usbh2_set_vbus_power,
	.xcvr_type       = PORTSC_PTS_ULPI,
	.usbmode         = (u32) &UH2_USBMODE,
	.power_budget = 500,           /*  DMO : 500 mA max power  */
};

static struct resource usbh2_resources[] = {
	{
		.start = (u32) (USB_H2REGS_BASE),
		.end   = (u32) (USB_H2REGS_BASE + 0x1ff),
		.flags = IORESOURCE_MEM,
	},
	{
		.start = INT_USB2,
		.flags = IORESOURCE_IRQ,
	},
};
/* *INDENT-ON* */

static struct platform_device *usbh2_device;
#endif				/* CONFIG_USB_EHCI_ARC_H2 */

#ifdef CONFIG_USB_GADGET_ARC
#ifdef CONFIG_USB_GADGET_ARC_OTGHS
/* *INDENT-OFF* */
static struct arc_usb_config udc_hs_config = {
	.name            = "OTG HS Gadget",
	.platform_init   = otg_hs_init,
	.platform_uninit = otg_hs_uninit,
	.set_vbus_power  = otg_hs_set_vbus_power,
	.xcvr_type       = PORTSC_PTS_ULPI,
	.usbmode         = (u32) &UOG_USBMODE,
};
/* *INDENT-ON* */
#endif

#ifdef CONFIG_USB_GADGET_ARC_OTGFS
int otg_fs_dev_init(void)
{
	pr_debug("%s: OTG-FS pins grabed\n", __FUNCTION__);

	isp1301_init();

	isp1301_set_serial_dev();
	gpio_usbotg_fs_active();	/* grab our pins */
	mdelay(1);

	/* disable OTG VBUS */
#ifdef CONFIG_MACH_MX31ADS
	// DDD need this?? mxc_clks_disable(USB_CLK);
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_SET);
	// DDD need this?? mxc_clks_enable(USB_CLK);
#endif

	otg_fs_set_xcvr();	/* set transceiver type */

	USBCTRL &= ~(UCTRL_OSIC_MASK | UCTRL_BPE);	/* disable bypass */
	USBCTRL |= UCTRL_OSIC_DU6 |	/* differential, unidir, 6 wire */
	    UCTRL_OWIE |	/* OTG wakeup intr enable */
	    UCTRL_OPM;		/* power mask */

	USB_OTG_MIRROR = 0xd;
	return 0;
}

static void otg_fs_dev_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

	/* disable OTG VBUS */
#ifdef CONFIG_MACH_MX31ADS
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_SET);
#endif

	isp1301_uninit();

	gpio_usbotg_fs_inactive();	/* release our pins */
}

/* *INDENT-OFF* */
static struct arc_usb_config udc_fs_config = {
	.name            = "OTG FS Gadget",
	.platform_init   = otg_fs_dev_init,
	.platform_uninit = otg_fs_dev_uninit,
	.set_vbus_power  = isp1301_set_vbus_power,
	.xcvr_type       = PORTSC_PTS_SERIAL,
	.usbmode         = (u32) &UOG_USBMODE,
};
/* *INDENT-ON* */
#endif				/* CONFIG_USB_GADGET_ARC_OTGFS */

/*!
 * OTG HS/FS gadget device
 */

static void usb_release(struct device *dev)
{
	/* normally not freed */
}

static u64 udc_dmamask = ~(u32) 0;
/* *INDENT-OFF* */
static struct platform_device udc_device = {
	.name = "arc_udc",
	.id   = -1,
	.dev  = {
		.release           = usb_release,
		.dma_mask          = &udc_dmamask,
		.coherent_dma_mask = 0xffffffff,
#if   defined CONFIG_USB_GADGET_ARC_OTGHS
		.platform_data     = &udc_hs_config,
#elif defined CONFIG_USB_GADGET_ARC_OTGFS
		.platform_data     = &udc_fs_config,
#else
#error "No OTG port configured."
#endif
		},
	.num_resources = ARRAY_SIZE(otg_resources),
	.resource      = otg_resources,
};
/* *INDENT-ON* */
#endif				/* CONFIG_USB_GADGET_ARC */

#if defined(CONFIG_USB_OTG)
static struct otg_transceiver *xceiv;

/**
 * otg_get_transceiver - find the (single) OTG transceiver driver
 *
 * Returns the transceiver driver, after getting a refcount to it; or
 * null if there is no such transceiver.  The caller is responsible for
 * releasing that count.
 */
struct otg_transceiver *otg_get_transceiver(void)
{
	pr_debug("%s xceiv=0x%p\n\n", __FUNCTION__, xceiv);
	if (xceiv)
		get_device(xceiv->dev);
	return xceiv;
}

EXPORT_SYMBOL(otg_get_transceiver);

int otg_set_transceiver(struct otg_transceiver *x)
{
	pr_debug("%s xceiv=0x%p  x=0x%p\n\n", __FUNCTION__, xceiv, x);
	if (xceiv && x)
		return -EBUSY;
	xceiv = x;
	return 0;
}

EXPORT_SYMBOL(otg_set_transceiver);

static void ulpi_release(struct device *dev)
{
	/* normally not freed */
}

/* *INDENT-OFF* */
static struct arc_xcvr_config ulpi_config = {
	.name            = "ulpi",
	.platform_init   = otg_hs_init,
	.platform_uninit = otg_hs_uninit,
	.regs            = (void *)&UOG_ID,
};

static struct resource ulpi_resources[] = {
	{
		.start = INT_USB3,
		.flags = IORESOURCE_IRQ,
	},
};

/*!
 * ULPI device
 */
static u64 ulpi_dmamask = ~(u32) 0;
static struct platform_device ulpi_device = {
	.name = "ulpi_arc",
	.id   = -1,
	.dev  = {
		.release           = ulpi_release,
		.dma_mask          = &ulpi_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data     = &ulpi_config,
		},
	.num_resources = ARRAY_SIZE(ulpi_resources),
	.resource      = ulpi_resources,
};
/* *INDENT-ON* */
#endif				/* CONFIG_USB_OTG */

/*!
 * Register an instance of a USB host platform device.
 *
 * @param	res:	resource pointer
 * @param       n_res:	number of resources
 * @param       config: config pointer
 *
 * @return      newly-registered platform_device
 *
 * The USB controller supports 3 host interfaces, and the
 * kernel can be configured to support some number of them.
 * Each supported host interface is registered as an instance
 * of the "fsl-ehci" device.  Call this function multiple times
 * to register each host interface.
 */
static int instance_id = 0;
static struct platform_device *host_pdev_register(struct resource *res,
						  int n_res,
						  struct arc_usb_config *config)
{
	int rc;
	struct platform_device *pdev;

	pdev = platform_device_register_simple("fsl-ehci",
					       instance_id, res, n_res);
	if (IS_ERR(pdev)) {
		pr_debug("can't register %s Host, %ld\n",
			 config->name, PTR_ERR(pdev));
		return NULL;
	}

	pdev->dev.coherent_dma_mask = 0xffffffff;
	pdev->dev.dma_mask = &ehci_dmamask;
	rc = platform_device_add_data(pdev, config,
				      sizeof(struct arc_usb_config));
	if (rc) {
		platform_device_unregister(pdev);
		return NULL;
	}

	printk(KERN_INFO "usb: %s registered\n", config->name);
	pr_debug("%s: pdev=0x%p  dev=0x%p  resources=0x%p  pdata=0x%p\n",
		 __FUNCTION__, pdev, &pdev->dev, pdev->resource,
		 pdev->dev.platform_data);
	instance_id++;

	return pdev;
}

extern void gpio_usb_set_bypass(unsigned int bypass);
extern int gpio_usb_get_bypass(void);


static ssize_t usbbypass_value_show(struct sys_device *dev, char *buf)
{
   int size = 0;
   int bypassvalue = gpio_usb_get_bypass();

   if ( ! bypassvalue ) {
      size += sprintf(buf, "none\n");
   }
   else {
      if ( bypassvalue & 1 )
         size += sprintf(buf, "HOST2 is bypassed\n");

      if ( bypassvalue & 2 )
         size += sprintf(&buf[size], "OTG is bypassed\n");
   }
   return size;
}

static ssize_t usbbypass_value_store(struct sys_device *dev, const char *buf,
      size_t size)
{
   if (buf[0]>='0' && buf[0]<='3') {
      gpio_usb_set_bypass(buf[0] - '0' );
   }

   return size;
}


static SYSDEV_ATTR(value, 0600, usbbypass_value_show, usbbypass_value_store);

static struct sysdev_class usbbypass_sysclass = {
	set_kset_name("usbbypass"),
};

static struct sys_device usbbypass_device = {
	.id = 0,
	.cls = &usbbypass_sysclass,
};

static int usbbypass_sysdev_ctrl_init(void)
{
	int err;

	err = sysdev_class_register(&usbbypass_sysclass);
	if (!err)
		err = sysdev_register(&usbbypass_device);
	if (!err) {
		err = sysdev_create_file(&usbbypass_device, &attr_value);
	}

	return err;
}

static void usbbypass_sysdev_ctrl_exit(void)
{
	sysdev_remove_file(&usbbypass_device, &attr_value);
	sysdev_unregister(&usbbypass_device);
	sysdev_class_unregister(&usbbypass_sysclass);
}




static int __init mx3_usb_init(void)
{
	int rc __attribute((unused));

	pr_debug("%s: \n", __FUNCTION__);

#if defined(CONFIG_USB_OTG)
	rc = platform_device_register(&ulpi_device);
	if (rc) {
		pr_debug("can't register ulpi dvc, %d\n", rc);
	} else {
		printk(KERN_INFO "usb: ulpi registered\n");
		pr_debug("%s: ulpi: platform_device_register succeeded.\n",
			 __FUNCTION__);
		pr_debug("ulpi_device=0x%p  resources=0x%p.",
			 &ulpi_device, ulpi_device.resource);
	}
#endif

#ifdef CONFIG_USB_EHCI_ARC_H1
#ifdef CONFIG_MACH_MX31HSV1
	if (!M_BOARD_IS_HOMESCREEN_V1() && !M_BOARD_IS_HOMESCREEN_V2()) {
#endif /* MACH_MX31HSV1 */
		usbh1_device = host_pdev_register(usbh1_resources,
					  ARRAY_SIZE(usbh1_resources),
					  &usbh1_config);
#ifdef CONFIG_MACH_MX31HSV1
	}
#endif /* MACH_MX31HSV1 */
printk(KERN_INFO "usb: USBH1 registered\n");
#endif /* USB_EHCI_ARC_H1 */

#ifdef CONFIG_USB_EHCI_ARC_H2
	usbh2_device = host_pdev_register(usbh2_resources,
					  ARRAY_SIZE(usbh2_resources),
					  &usbh2_config);
printk(KERN_INFO "usb: USBH2 registered\n");
#endif

#if defined(CONFIG_USB_EHCI_ARC_OTGHS)
	otg_hs_host_device = host_pdev_register(otg_resources,
						ARRAY_SIZE(otg_resources),
						&otg_hs_host_config);
printk(KERN_INFO "usb: USBOTGHS registered\n");
#elif defined(CONFIG_USB_EHCI_ARC_OTGFS)
	otg_fs_host_device = host_pdev_register(otg_resources,
						ARRAY_SIZE(otg_resources),
						&otg_fs_host_config);
printk(KERN_INFO "usb: USBFS registered\n");
#endif

#ifdef CONFIG_USB_GADGET_ARC
	rc = platform_device_register(&udc_device);
	if (rc)
		pr_debug("can't register OTG Gadget, %d\n", rc);
	else
		printk(KERN_INFO "usb: %s registered\n",
		       ((struct arc_usb_config *)udc_device.dev.platform_data)->
		       name);
#endif

	usbbypass_sysdev_ctrl_init();

	if (check_usbclk() != 0)
		return -EINVAL;

	return 0;
}

subsys_initcall(mx3_usb_init);
