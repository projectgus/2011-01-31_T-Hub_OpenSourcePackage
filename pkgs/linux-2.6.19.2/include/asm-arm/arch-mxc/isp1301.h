
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
#ifndef __ASM_ARCH_MXC_ISP1301_H__
#define __ASM_ARCH_MXC_ISP1301_H__

#include <linux/i2c.h>
#include <asm/arch/mxc_i2c.h>

/* ISP1301 register addresses,all register of ISP1301 
 * is one-byte length register 
 */

/* ISP1301: I2C device address */
#define ISP1301_DEV_ADDR 		0x2D

/* ISP 1301 register set*/
#define ISP1301_MODE_REG1_SET		0x04
#define ISP1301_MODE_REG1_CLR		0x05

#define ISP1301_CTRL_REG1_SET		0x06
#define ISP1301_CTRL_REG1_CLR		0x07

#define ISP1301_INT_SRC_REG		0x08
#define ISP1301_INT_LAT_REG_SET		0x0a
#define ISP1301_INT_LAT_REG_CLR		0x0b
#define ISP1301_INT_FALSE_REG_SET	0x0c
#define ISP1301_INT_FALSE_REG_CLR	0x0d
#define ISP1301_INT_TRUE_REG_SET	0x0e
#define ISP1301_INT_TRUE_REG_CLR	0x0f

#define ISP1301_CTRL_REG2_SET		0x10
#define ISP1301_CTRL_REG2_CLR		0x11

#define ISP1301_MODE_REG2_SET		0x12
#define ISP1301_MODE_REG2_CLR		0x13

#define ISP1301_BCD_DEV_REG0		0x14
#define ISP1301_BCD_DEV_REG1		0x15

/* OTG Control register bit description */
#define DP_PULLUP			0x01
#define DM_PULLUP			0x02
#define DP_PULLDOWN			0x04
#define DM_PULLDOWN			0x08
#define ID_PULLDOWN			0x10
#define VBUS_DRV			0x20
#define VBUS_DISCHRG			0x40
#define VBUS_CHRG			0x80

/* Mode Control 1 register bit description */
#define SPEED_REG  			0x01
#define SUSPEND_REG			0x02
#define DAT_SE0				0x04
#define TRANSP_EN			0x08
#define BDIS_ACON_EN			0x10
#define OE_INT_EN			0x20
#define UART_EN				0x40

static int isp1301_attach(struct i2c_adapter *adapter);
static int isp1301_detach(struct i2c_client *client);

static struct i2c_driver isp1301_i2c_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "isp1301 Client",
		   },
	.attach_adapter = isp1301_attach,
	.detach_client = isp1301_detach,
};

static struct i2c_client isp1301_i2c_client = {
	.name = "isp1301 I2C dev",
	.addr = ISP1301_DEV_ADDR,
	.driver = &isp1301_i2c_driver,
};

static int isp1301_detect_client(struct i2c_adapter *adapter, int address,
				 int kind)
{
	isp1301_i2c_client.adapter = adapter;
	if (i2c_attach_client(&isp1301_i2c_client)) {
		isp1301_i2c_client.adapter = NULL;
		printk(KERN_ERR "isp1301_attach: i2c_attach_client failed\n");
		return -1;
	}

	printk(KERN_INFO "isp1301 Detected\n");
	return 0;
}

static unsigned short normal_i2c[] = { ISP1301_DEV_ADDR, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

/*!
 * isp1301 I2C attach function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int isp1301_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, &isp1301_detect_client);
}

/*!
 * isp1301 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int isp1301_detach(struct i2c_client *client)
{
	int err;

	if (!isp1301_i2c_client.adapter)
		return -1;

	err = i2c_detach_client(&isp1301_i2c_client);
	isp1301_i2c_client.adapter = NULL;

	return err;
}

static int isp1301_i2c_client_xfer(unsigned int addr, char *reg, int reg_len,
				   char *buf, int num, int tran_flag)
{
	struct i2c_msg msg[2];
	int ret;

	msg[0].addr = addr;
	msg[0].len = reg_len;
	msg[0].buf = reg;
	msg[0].flags = tran_flag;
	msg[0].flags &= ~I2C_M_RD;

	msg[1].addr = addr;
	msg[1].len = num;
	msg[1].buf = buf;
	msg[1].flags = tran_flag;

	if (tran_flag & 1) {
		msg[1].flags |= I2C_M_RD;
	} else {
		msg[1].flags &= ~I2C_M_RD;
	}

	ret = i2c_transfer(isp1301_i2c_client.adapter, msg, 2);
	if (ret >= 0)
		return 0;

	return ret;
}

static inline void isp1301_init(void)
{
	i2c_add_driver(&isp1301_i2c_driver);
}

static inline void isp1301_uninit(void)
{
	i2c_del_driver(&isp1301_i2c_driver);
}

/* Write ISP1301 register*/
static inline void isp1301_write_reg(char reg, char data)
{
	isp1301_i2c_client_xfer(ISP1301_DEV_ADDR, &reg, 1, &data, 1, 0);
}

/* read ISP1301 register*/
static inline char isp1301_read_reg(char reg)
{
	char data;
	isp1301_i2c_client_xfer(ISP1301_DEV_ADDR, &reg, 1, &data, 1, 1);
	return data;
}

/* set ISP1301 as USB host*/
static inline void isp1301_set_serial_host(void)
{
	isp1301_write_reg(ISP1301_CTRL_REG1_CLR, 0xFF);
	isp1301_write_reg(ISP1301_CTRL_REG1_SET,
			  (VBUS_DRV | DP_PULLDOWN | DM_PULLDOWN));
	isp1301_write_reg(ISP1301_MODE_REG1_SET, DAT_SE0);	/* SE0 state */
}

/* set ISP1301 as USB device*/
static inline void isp1301_set_serial_dev(void)
{
	isp1301_write_reg(ISP1301_MODE_REG1_CLR, 0xFF);
	/* FS mode, DP pull down, DM pull down */
	isp1301_write_reg(ISP1301_CTRL_REG1_SET,
			  (DP_PULLDOWN | DM_PULLDOWN | DP_PULLUP));
}

static inline void isp1301_set_vbus_power(int on)
{
	if (on) {
		/* disable D+ pull-up */
		isp1301_write_reg(ISP1301_CTRL_REG1_CLR, DP_PULLUP);
		/* enable D+ pull-down */
		isp1301_write_reg(ISP1301_CTRL_REG1_SET, DP_PULLDOWN);
		/* turn on Vbus */
		isp1301_write_reg(ISP1301_CTRL_REG1_SET, VBUS_DRV);
	} else {
		/* D+ pull up, D- pull down  */
		isp1301_write_reg(ISP1301_CTRL_REG1_SET,
				  (DP_PULLUP | DM_PULLDOWN));
		/* disable D- pull up, disable D+ pull down */
		isp1301_write_reg(ISP1301_CTRL_REG1_CLR,
				  (DM_PULLUP | DP_PULLDOWN));
	}
}

#endif
