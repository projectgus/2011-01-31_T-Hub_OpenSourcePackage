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

#ifndef __ASM_ARCH_MXC_BOARD_H__
#define __ASM_ARCH_MXC_BOARD_H__

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#error "Do not include directly."
#endif

#ifndef __ASSEMBLY__

struct mxc_ipu_config {
	int rev;
};

struct mxc_ir_platform_data {
	int uart_ir_mux;
	struct clk *uart_clk;
};

struct mxc_i2c_platform_data {
	u32 i2c_clk;
};

#endif

/*
 * The modes of the UART ports
 */
#define MODE_DTE                0
#define MODE_DCE                1
/*
 * Is the UART configured to be a IR port
 */
#define IRDA                    0
#define NO_IRDA                 1

#endif				/* __ASM_ARCH_MXC_BOARD_H__ */
