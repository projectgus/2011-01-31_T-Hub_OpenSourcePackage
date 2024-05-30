/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Copyright 2006 Sagem BDt : adaptation for the AD7877
 *
 */

/*!
 * @file AD7877_spi_inter.c
 * @brief This file contains interface with the SPI driver.
 *
 */

/*
 * Includes
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/types.h>

#include "ad7877_spi_inter.h"
#include "ad7877_ts.h"
#include <linux/spi/spi.h>

static spi_config the_config;
static void* spi_id;


/*!
 * This function configures the SPI access.
 *
 * @return       This function returns 0 if successful.
 */
int spi_7877_init(void)
{
        /* Initialize the semaphore */

        the_config.module_number = SPI2;
        the_config.priority = HIGH;
        the_config.master_mode = true;                // True : Master mode
        the_config.bit_rate = 2000000;                // Default 12000000
        the_config.bit_count = 16;                    // Bit wide
        the_config.active_high_polarity = true;      // False : Clock polarity control : 0 = idle
        the_config.active_high_ss_polarity = false;   // False : SS polarity active low
        the_config.phase = false;                      // True  : Clk/Data: data shift out on clk rising edge
        the_config.ss_low_between_bursts = true;      // False : Only 1 burst will be transmitted, true
                                                      // True  : Multiple bursts, negate SS between bursts
        the_config.ss_asserted = SS_0;
        the_config.tx_delay = 0;
        spi_id = spi_get_device_id ( (spi_config *)&the_config );
        return 0;
}


/*!
 * This function is used to write on a AD7877 register.
 *
 * @param        num_reg    AD7877 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_write_7877reg(int num_reg, unsigned int reg_value)
{
        spi_send_frame_7877(num_reg, &reg_value);
        return 0;
}

/*!
 * This function is used to read on a AD7877 register.
 *
 * @param        reg_value  AD7877 register to read
 * @param        buffer     Register data read
 *
 * @return       This function returns 0 if successful.
 */
int spi_read_7877reg(int reg_value, unsigned int* buffer)
{
        *buffer = reg_value << 2;
        spi_send_frame_7877(AD7877_REG_CTRL1, buffer);
        spi_send_frame_7877(AD7877_REG_CTRL1, buffer);
        return 0;
}

/*!
 * This function is used to send a frame on SPI bus.
 *
 * @param        num_reg    AD7877 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_send_frame_7877(int num_reg, unsigned int* reg_value)
{
        unsigned char send_val[4];
        unsigned int frame = 0;
        unsigned int frame_ret = 0;
        unsigned int result = 0;

        frame |= *reg_value & 0x0fff;
        frame |= ((unsigned int) num_reg << 12) & 0xf000;

	send_val[0] = (frame >> 8) /* & 0xff implicit operation */;
	send_val[1] = frame /* & 0xff implicit operation */;

        /* use this to launch SPI operation.*/
        result = spi_send_frame(&send_val, 2, spi_id);

	frame_ret = (send_val[0] << 8) | send_val[1];

        *reg_value = frame_ret & 0x0fff;

        return 0;
}

