/*
 *  Infrared driver.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: infrared.h
 *  Creation date: 10/10/2007
 *  Author: Stephane Lapie
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

#ifndef INFRARED_H_
# define INFRARED_H_

/*
** IOCTL config idx
*/
# define IR_PARAM	12

# define IR_HDER	0
# define IR_HDER_UTIME	1
# define IR_HDER_LTIME	2
# define IR_HDER_SDATA	3

# define IR_BIT0	4
# define IR_BIT0_UTIME	5
# define IR_BIT0_LTIME	6
# define IR_BIT0_SDATA	7

# define IR_BIT1	8
# define IR_BIT1_UTIME	9
# define IR_BIT1_LTIME	10
# define IR_BIT1_SDATA	11

#endif /* !INFRARED_H_ */
