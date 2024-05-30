/*
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: lb2tablet_buttons_driver.h
 *  Creation date: 06/08/2007
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

#ifndef _LB2TABLET_BUTTONS_DRIVER_H_
#define _LB2TABLET_BUTTONS_H_
 
#ifndef BUTTONS_MAJOR
#define BUTTONS_MAJOR 200
#endif

#ifndef BUTTONS_NR_DEVS
#define BUTTONS_NR_DEVS 1
#endif

#define IOMUX_BUTTON_SUP_NAME MX31_PIN_KEY_COL7
#define IOMUX_BUTTON_MID_NAME MX31_PIN_KEY_ROW7
#define IOMUX_FACTORY_RESET_NAME MX31_PIN_KEY_COL3

#define BUT_SUP_VALUE 0
#define BUT_MID_VALUE 1

/* Use 'k' as magic number */
#define BUTTONS_IOC_MAGIC  'k'
/* Please use a different 8-bit number in your code */


#define BUTTONS_IOCTL_GET_BUTTON_PRESSED        _IOR(BUTTONS_IOC_MAGIC,  1,int)


#endif
