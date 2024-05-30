/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *	otg_{get,set}_transceiver() are from arm/plat-omap/usb.c.
 *	which is Copyright (C) 2004 Texas Instruments, Inc.
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
 * @defgroup USB_MX27 ARC OTG USB Driver for i.MX27
 * @ingroup USB
 */

/*!
 * @file mach-mx27/usb.c
 *
 * @brief platform related part of usb driver.
 * @ingroup USB_MX27
 */

/*!
 * Include files
 */

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
#include <asm/arch/isp1504.h>
#include <asm/arch/isp1301.h>

#define PBC3_CLEAR	PBC_BCTRL3_CLEAR_REG
#define PBC3_SET	PBC_BCTRL3_SET_REG

#undef DEBUG
#undef VERBOSE

extern void gpio_usbh1_active(void);
extern void gpio_usbh1_inactive(void);
extern void gpio_usbh2_active(void);
extern void gpio_usbh2_inactive(void);
extern void gpio_usbotg_hs_active(void);
extern void gpio_usbotg_hs_inactive(void);
extern void gpio_usbotg_fs_active(void);
extern void gpio_usbotg_fs_inactive(void);

/* The dmamask must be set for EHCI to work */
static u64 ehci_dmamask = ~(u32) 0;

static struct clk *usb_clk;
static struct clk *usb_ahb_clk;
extern int clk_get_usecount(struct clk *clk);

/*
 * make sure USB_CLK is running at 60 MHz +/- 1000 Hz
 */
static int check_usbclk(void)
{
	unsigned long freq;

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
 * @return	return isp1504 register value
 */
u8 isp1504_read(int reg, volatile u32 * view)
{
	u32 data;

	/* make sure interface is running */
	if (!(__raw_readl(view) && ULPIVW_SS)) {
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
		} while (data & ULPIVW_WU);
	}

	/* read the register */
	__raw_writel((ULPIVW_RUN | (reg << ULPIVW_ADDR_SHIFT)), view);

	do {			/* wait for completion */
		data = __raw_readl(view);
	} while (data & ULPIVW_RUN);

	return (u8) (data >> ULPIVW_RDATA_SHIFT) & ULPIVW_RDATA_MASK;
}

EXPORT_SYMBOL(isp1504_read);

/*!
 * set bits into OTG ISP1504 register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  set value
 * @param	reg   which register
 * @param       view  the ULPI VIEWPORT register address
 */
void isp1504_set(u8 bits, int reg, volatile u32 * view)
{
	u32 data;

	/* make sure interface is running */
	if (!(__raw_readl(view) && ULPIVW_SS)) {
		printk(KERN_INFO "\nSS=0\n");
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
		} while (data & ULPIVW_WU);
	}

	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ISP1504_REG_SET) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	while (__raw_readl(view) & ULPIVW_RUN)	/* wait for completion */
		continue;
}

EXPORT_SYMBOL(isp1504_set);

/*!
 * clear bits in OTG ISP1504 register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  bits to clear
 * @param	reg   in this register
 * @param       view  the ULPI VIEWPORT register address
 */
void isp1504_clear(u8 bits, int reg, volatile u32 * view)
{
	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ISP1504_REG_CLEAR) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	while (__raw_readl(view) & ULPIVW_RUN)	/* wait for completion */
		continue;
}

EXPORT_SYMBOL(isp1504_clear);

/*!
 * set vbus power
 *
 * @param       on    power on or off
 * @param       view  the ULPI view register address
 */
static void __attribute((unused)) ulpi_set_vbus_power(int on,
						      volatile u32 * view)
{
	pr_debug("%s: on=%d  view=0x%p\n", __FUNCTION__, on, view);

	pr_debug("%s: ULPI Vendor ID 0x%x    Product ID 0x%x\n", __FUNCTION__,
		 (isp1504_read(ISP1504_VID_HIGH, view) << 8) |
		 isp1504_read(ISP1504_VID_LOW, view),
		 (isp1504_read(ISP1504_PID_HIGH, view) << 8) |
		 isp1504_read(ISP1504_PID_LOW, view));

	pr_debug("%s: OTG Control before = 0x%x\n", __FUNCTION__,
		 isp1504_read(ISP1504_OTGCTL, view));

	if (on) {
		isp1504_set(DRV_VBUS_EXT |	/* enable external Vbus */
			    DRV_VBUS |	/* enable internal Vbus */
			    USE_EXT_VBUS_IND |	/* use external indicator */
			    CHRG_VBUS,	/* charge Vbus */
			    ISP1504_OTGCTL, view);

	} else {
		isp1504_clear(DRV_VBUS_EXT |	/* disable external Vbus */
			      DRV_VBUS,	/* disable internal Vbus */
			      ISP1504_OTGCTL, view);

		isp1504_set(USE_EXT_VBUS_IND |	/* use external indicator */
			    DISCHRG_VBUS,	/* discharge Vbus */
			    ISP1504_OTGCTL, view);
	}

	pr_debug("%s: OTG Control after = 0x%x\n\n", __FUNCTION__,
		 isp1504_read(ISP1504_OTGCTL, view));
}
#endif

#if defined(CONFIG_USB_EHCI_ARC_OTGHS) || defined(CONFIG_USB_GADGET_ARC_OTGHS)
static int otg_used = 0;	/* OTG use-count */

static void otg_hs_set_xcvr(void)
{
	u32 tmp;

	/* set ULPI xcvr */
	tmp = UOG_PORTSC1 & ~PORTSC_PTS_MASK;
	tmp |= PORTSC_PTS_ULPI;
	UOG_PORTSC1 = tmp;

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
		 clk_get_usecount(usb_clk));
}

static int otg_hs_init(void)
{
	if (!otg_used) {
		pr_debug("%s: grab OTG-HS pins\n", __FUNCTION__);

		/* enable OTG/HS */
		clk_enable(usb_clk);
		pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
			 clk_get_usecount(usb_clk));
		__raw_writew(PBC_BCTRL3_OTG_HS_EN, PBC3_CLEAR);

		gpio_usbotg_hs_active();	/* grab our pins */
		mdelay(1);

		USBCTRL &= ~UCTRL_BPE;	/* disable bypass mode */
		USBCTRL |= UCTRL_OUIE |	/* ULPI intr enable */
		    UCTRL_OWIE |	/* OTG wakeup intr enable */
		    UCTRL_OPM;	/* power mask */

		otg_hs_set_xcvr();	/* set transciever type */
	}

	isp1504_set(ID_PULL_UP, ISP1504_OTGCTL, &UOG_ULPIVIEW);
#ifdef DEBUG
	if (!(isp1504_read(ISP1504_OTGCTL, &UOG_ULPIVIEW) & ID_PULL_UP)) {
		printk(KERN_ERR "ID_PULL_UP not set in OTGCTL!!\n");
	}
#endif

	otg_used++;
	return 0;
}

static void otg_hs_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

	otg_used--;
	if (!otg_used) {
		/* disable  OTG/HS */
		__raw_writew(PBC_BCTRL3_OTG_HS_EN, PBC3_SET);

		pr_debug("%s: free OTG-HS pins\n", __FUNCTION__);
		gpio_usbotg_hs_inactive();	/* release our pins */
	}
}

static void otg_hs_set_vbus_power(int on)
{
	pr_debug("%s: on=%d\n", __FUNCTION__, on);
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
	.power_budget    = 500,		/* 500 mA max power */
};
/* *INDENT-ON* */

static struct platform_device *otg_hs_host_device;
#endif				/* CONFIG_USB_EHCI_ARC_OTGHS */
#endif				/* CONFIG_USB_EHCI_ARC_OTGHS || CONFIG_USB_GADGET_ARC_OTGHS */

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
	pr_debug("%s: grab OTG-FS pins\n", __FUNCTION__);
	clk_enable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));

	isp1301_init();

	isp1301_set_serial_host();
	gpio_usbotg_fs_active();	/* grab our pins */
	mdelay(1);

	/* enable OTG VBUS */
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_CLEAR);

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

	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_SET);	/* disable OTG VBUS */

	isp1301_uninit();

	gpio_usbotg_fs_inactive();	/* release our pins */
	clk_disable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));
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
	.power_budget    = 500,		/* 500 mA max power */
};
/* *INDENT-ON* */

static struct platform_device *otg_fs_host_device;
#endif				/* CONFIG_USB_EHCI_ARC_OTGFS */

/* Host 1 */
#ifdef CONFIG_USB_EHCI_ARC_H1
static void usbh1_set_xcvr(void)
{
	UH1_PORTSC1 &= ~PORTSC_PTS_MASK;
	UH1_PORTSC1 |= PORTSC_PTS_SERIAL;
}

static int usbh1_init(void)
{
	pr_debug("%s: grab H1 pins\n", __FUNCTION__);

	clk_enable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));

	gpio_usbh1_active();
	mdelay(1);

	__raw_writew(PBC_BCTRL3_FSH_MOD, PBC3_CLEAR);	/* single ended */
	__raw_writew(PBC_BCTRL3_FSH_VBUS_EN, PBC3_CLEAR);	/* enable FSH VBUS */

	USBCTRL &= ~(UCTRL_H1SIC_MASK | UCTRL_BPE);	/* disable bypass mode */
	USBCTRL |= UCTRL_H1SIC_SU6 |	/* single-ended / unidir. */
	    UCTRL_H1WIE | UCTRL_H1DT |	/* disable H1 TLL */
	    UCTRL_H1PM;		/* power mask */

	usbh1_set_xcvr();
	return 0;
}

static void usbh1_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

	__raw_writew(PBC_BCTRL3_FSH_VBUS_EN, PBC3_SET);	/* disable FSH VBUS */

	gpio_usbh1_inactive();	/* release our pins */
	clk_disable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));
}

/* *INDENT-OFF* */
static struct arc_usb_config usbh1_config = {
	.name            = "Host 1",
	.platform_init   = usbh1_init,
	.platform_uninit = usbh1_uninit,
	.xcvr_type       = PORTSC_PTS_SERIAL,
	.usbmode         = (u32) &UH1_USBMODE,
	.power_budget    = 500,		/* 500 mA max power */
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
static void usbh2_set_xcvr(void)
{
	UH2_PORTSC1 &= ~PORTSC_PTS_MASK;	/* set ULPI xcvr */
	UH2_PORTSC1 |= PORTSC_PTS_ULPI;

	/* need to reset the controller here so that the ID pin
	 * is correctly detected.
	 */
	UH2_USBCMD |= UCMD_RESET;

	/* allow controller to reset, and leave time for
	 * the ULPI transceiver to reset too.
	 */
	mdelay(10);

	/* Turn off the usbpll for ulpi tranceivers */
	clk_disable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));
}

static int usbh2_init(void)
{
	pr_debug("%s: grab H2 pins\n", __FUNCTION__);

	__raw_writew(PBC_BCTRL3_HSH_EN, PBC3_CLEAR);	/* enable OTG_VBUS_EN */
	clk_enable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));

	gpio_usbh2_active();	/* grab our pins */
	mdelay(1);

	USBCTRL &= ~(UCTRL_BPE);	/* disable bypass mode */
	USBCTRL |= UCTRL_H2WIE |	/* wakeup intr enable */
	    UCTRL_H2UIE |	/* ULPI intr enable */
	    UCTRL_H2DT |	/* disable H2 TLL */
	    UCTRL_H2PM;		/* power mask */

	pr_debug("%s: success\n", __FUNCTION__);
	usbh2_set_xcvr();	/* set transceiver type */
	return 0;
}

static void usbh2_uninit(void)
{
	pr_debug("%s: \n", __FUNCTION__);

	__raw_writew(PBC_BCTRL3_HSH_EN, PBC3_SET);	/* disable HSH */

	gpio_usbh2_inactive();	/* release our pins */
}

static void usbh2_set_vbus_power(int on)
{
	pr_debug("%s: on=%d\n", __FUNCTION__, on);
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
	.power_budget    = 500,		/* 500 mA max power */
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
	pr_debug("%s: grab OTG-FS pins\n", __FUNCTION__);

	clk_enable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));

	isp1301_init();

	isp1301_set_serial_dev();
	gpio_usbotg_fs_active();	/* grab our pins */
	mdelay(1);

	/* disable OTG VBUS */
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_SET);

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
	__raw_writew(PBC_BCTRL3_OTG_VBUS_EN, PBC3_SET);

	isp1301_uninit();
	gpio_usbotg_fs_inactive();	/* release our pins */
	clk_disable(usb_clk);
	pr_debug("%s: usb_clk usecount %d\n\n", __FUNCTION__,
		 clk_get_usecount(usb_clk));
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

static void isp1504_release(struct device *dev)
{
	/* normally not freed */
}

/* *INDENT-OFF* */
static struct arc_xcvr_config isp1504_config = {
	.name            = "isp1504",
	.platform_init   = otg_hs_init,
	.platform_uninit = otg_hs_uninit,
	.regs            = (void *)&UOG_ID,
};

static struct resource isp1504_resources[] = {
	{
		.start = INT_USB3,
		.flags = IORESOURCE_IRQ,
	},
};

/*!
 * ISP1504 device
 */
static u64 isp1504_dmamask = ~(u32) 0;
static struct platform_device isp1504_device = {
	.name = "isp1504_arc",
	.id   = -1,
	.dev  = {
		.release           = isp1504_release,
		.dma_mask          = &isp1504_dmamask,
		.coherent_dma_mask = 0xffffffff,
		.platform_data     = &isp1504_config,
		},
	.num_resources = ARRAY_SIZE(isp1504_resources),
	.resource      = isp1504_resources,
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

static int __init mx27_usb_init(void)
{
	int rc __attribute((unused));

	pr_debug("%s: \n", __FUNCTION__);

#if defined(CONFIG_USB_OTG)
	rc = platform_device_register(&isp1504_device);
	if (rc) {
		pr_debug("can't register isp1504 dvc, %d\n", rc);
	} else {
		printk(KERN_INFO "usb: isp1504 registered\n");
		pr_debug("%s: isp1504: platform_device_register succeeded.\n",
			 __FUNCTION__);
		pr_debug("%s: isp1504_device=0x%p  resources=0x%p.\n",
			 __FUNCTION__, &isp1504_device,
			 isp1504_device.resource);
	}
#endif

#ifdef CONFIG_USB_EHCI_ARC_H1
	usbh1_device = host_pdev_register(usbh1_resources,
					  ARRAY_SIZE(usbh1_resources),
					  &usbh1_config);
#endif

#ifdef CONFIG_USB_EHCI_ARC_H2
	usbh2_device = host_pdev_register(usbh2_resources,
					  ARRAY_SIZE(usbh2_resources),
					  &usbh2_config);
#endif

#if defined(CONFIG_USB_EHCI_ARC_OTGHS)
	otg_hs_host_device = host_pdev_register(otg_resources,
						ARRAY_SIZE(otg_resources),
						&otg_hs_host_config);
#elif defined(CONFIG_USB_EHCI_ARC_OTGFS)
	otg_fs_host_device = host_pdev_register(otg_resources,
						ARRAY_SIZE(otg_resources),
						&otg_fs_host_config);
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

	if (check_usbclk() != 0)
		return -EINVAL;

	return 0;
}

subsys_initcall(mx27_usb_init);
