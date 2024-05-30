/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/* ehci_arc_hc_driver.flags value */
#define FSL_PLATFORM_HC_FLAGS HCD_USB2

static inline int fsl_platform_verify(struct platform_device *pdev)
{
	return 0;
}

static inline void fsl_platform_usb_setup(struct usb_hcd *hcd)
{
}

static inline void fsl_platform_set_host_mode(struct usb_hcd *hcd)
{
	unsigned int temp;

	/* set host mode */
	temp = readl(hcd->regs + 0x1a8);
	writel(temp | USBMODE_CM_HOST, hcd->regs + 0x1a8);
}
