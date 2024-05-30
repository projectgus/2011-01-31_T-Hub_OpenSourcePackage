/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 *
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
 */

#ifndef __SPI_INTERFACE_H__
#define __SPI_INTERFACE_H__

/*!
 * @file AD7877_spi_inter.h
 * @brief This file contains prototypes of SPI interface.
 *
 */

/*
 * Includes
 */

/*!
 * This function configures the SPI access.
 *
 * @return       This function returns 0 if successful.
 */
int spi_7877_init(void);

/*!
 * This function is used to write on a AD7877 register.
 *
 * @param        num_reg    AD7877 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_write_7877reg(int num_reg, unsigned int reg_value);

/*!
 * This function is used to read on a AD7877 register.
 *
 * @param        reg_value  AD7877 register to read
 * @param        buffer     Register data read
 *
 * @return       This function returns 0 if successful.
 */
int spi_read_7877reg(int reg_value, unsigned int* buffer);

/*!
 * This function is used to send a frame on SPI bus.
 *
 * @param        num_reg    AD7877 register number
 * @param        reg_value  Register value
 *
 * @return       This function returns 0 if successful.
 */
int spi_send_frame_7877(int num_reg, unsigned int* reg_value);

#endif /* __SPI_INTERFACE_H__ */
