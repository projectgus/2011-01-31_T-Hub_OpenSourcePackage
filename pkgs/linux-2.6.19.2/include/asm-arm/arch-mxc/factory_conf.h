/*
 *  Header file of factory conf driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: factory_conf.h
 *  Creation date: 02/10/2007
 *  Author: Olivier Le Roy, Sagemcom
 *
 *  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General
 *  Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *  Write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA to
 *  receive a copy of the GNU General Public License.
 *  This Copyright notice should not be removed
 */

#include <asm/ioctl.h>

#define BUFSIZE 0x8000

#define FACTORYCONF_IOC_MAGIC 'z'

/*!
 * Reload factory zone.
 * Argument type: none.
 */
#define RELOAD_FACTORY_TABLE	_IO(FACTORYCONF_IOC_MAGIC, 1)

/*!
 * Write factory table.
 * Argument type: pointer to 32k factory zone.
 */
#define WRITE_FACTORY_TABLE	_IO(FACTORYCONF_IOC_MAGIC, 2)

typedef struct {
	char name[32];		/* Null terminated parameter name */
	char application[32];	/* Null terminated application name */
	char version[16];	/* Null terminated version name */
	char value[32];		/* Null terminated parameter value */
} factory_parameter_t;
