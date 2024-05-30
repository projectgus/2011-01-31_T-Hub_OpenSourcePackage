/*
 *  mt9v112 camera driver functions.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: micron_config_ini.h
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

#if !defined(_K_MICRON_CONFIG_H_)
#define _K_MICRON_CONFIG_H_

#if !defined(NULL)
#define	NULL	0x0
#endif /* NULL */

#define	K_MICRON_CMD_REG	1
#define	K_MICRON_CMD_BITFIELD	2
#define	K_MICRON_CMD_DELAY      3
#define	K_MICRON_CMD_LOAD	4
#define	K_MICRON_CMD_END	0

typedef struct _Command
{
    int		m_nType;
    char	*m_szParam;
    int		m_anParam[4];
}
TSf_MICRON_Command;

typedef struct _Section
{
    const char		*m_szName;
    TSf_MICRON_Command	m_atsCommand[512];
}
TSf_MICRON_Section;


static TSf_MICRON_Section	f_atsSectionList[] = 
{

	{"Viewfinder ON", 
		{
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xD2, 0x007F, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 4, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Fast CCM", 
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x4000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0xF5, 0x0020, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5E, 0x4962, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5F, 0x7A58, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x60, 0x0002, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x02, 0x00EA, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x03, 0x291A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x04, 0x04A4, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x09, 0x0097, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0A, 0x0072, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0B, 0x001E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0C, 0x001D, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0D, 0x007E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0E, 0x0072, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0F, 0x0011, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x10, 0x0034, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x11, 0x0082, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x15, 0x0111, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x16, 0x003A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x17, 0x003B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x18, 0x0022, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x19, 0x0051, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1A, 0x002B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1B, 0x0032, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1C, 0x0071, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1D, 0x00BB, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1E, 0x00CB, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 1} },
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"SEMCO 30fps", 
		{
			{ K_MICRON_CMD_REG, NULL, {2,0x3D, 0x17DD, 0} },
			{ K_MICRON_CMD_REG, NULL, {0,0x5F, 0x3630, 0} },
			{ K_MICRON_CMD_REG, NULL, {0,0x30, 0x043E, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,0x3B, 0x043E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0000, 0} },
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0C44, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x0D, 0x0029, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x0D, 0x0008, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x20, 0x0300, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x22, 0x012B, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x24, 0x4000, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x35, 0x2029, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x41, 0x00D7, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x58, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x59, 0x000C, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0xC8, 0x000B, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x34, 0xC019, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x40, 0x1800, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x76, 0x7358, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x21, 0x040C, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x37, 0x0300, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x5B, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x35, 0x2029, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x5F, 0x231D, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x60, 0x0080, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0xF0, 0x0002, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3D, 0x17DD, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x47, 0x0018, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x9D, 0x24AE, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x53, 0x0C06, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x54, 0x4826, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x55, 0xA87C, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x56, 0xE0C8, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x57, 0xFEF0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x58, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDC, 0x0C06, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDD, 0x4826, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDE, 0xA87C, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDF, 0xE0C8, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE0, 0xFEF0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE1, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x05, 0x000B, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x2F, 0x0020, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1F, 0x0080, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x20, 0xE030, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x21, 0x8080, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x22, 0xA878, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x23, 0x8870, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x6018, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x28, 0xEF04, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x29, 0x827E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5E, 0x644B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5F, 0x5F40, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x60, 0x0002, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0xEF, 0x0008, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0xF2, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x02, 0x00AE, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x03, 0x3923, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x04, 0x0524, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x09, 0x00C2, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0A, 0x0052, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0B, 0x002E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0C, 0x0051, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0D, 0x00E9, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0E, 0x0035, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0F, 0x003A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x10, 0x00B3, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x11, 0x0099, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x15, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x16, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x17, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x18, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x19, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1A, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1B, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1C, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1D, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1E, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x5A, 0xC00A, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xF0, 0x0001, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x25, 0x002D, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x9D, 0x24A2, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x30, 0x0440, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xF0, 0x0001, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x3B, 0x0440, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x3C, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x62, 0x2010, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x71, 0x7B0A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x38, 0x0440, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x83, 0x0200, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x84, 0x0160, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x85, 0x0100, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x86, 0x00C0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x87, 0x00A0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x88, 0x00A0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x89, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x8A, 0x0019, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x8B, 0x001A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x8C, 0x001A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x6018, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x2A, 0x00D0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x29, 0x827E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x28, 0xEF04, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x2D, 0xE2A0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0xF0, 0x0002, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x26, 0x7808, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x27, 0x7808, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x2B, 0x5828, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x2C, 0x7832, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x2E, 0x0644, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x2F, 0xDF20, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x9C, 0xDF20, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x8D, 0x001B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x8E, 0x001B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x8F, 0x001B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x90, 0x001C, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x91, 0x039C, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x92, 0x03BC, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x93, 0x03BD, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x94, 0x03BD, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3E, 0x10FF, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xF0, 0x0001, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x34, 0x0401, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x35, 0xFF00, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xF0, 0x0001, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x48, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE2, 0x7000, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE3, 0xB023, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x06, 0xF48E, 0} },
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x06, 0x748E, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x80, 0x0003, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x81, 0xBC0B, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x82, 0xEADA, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x83, 0x0CFB, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x84, 0xCF07, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x85, 0xF1E6, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x86, 0x0800, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x87, 0xD406, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x88, 0xF4E9, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x89, 0x0A00, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x8A, 0xA01A, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x8B, 0xD1AF, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x8C, 0xF2E2, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x8D, 0x0002, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x8E, 0xBA13, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x8F, 0xDAC9, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x90, 0xF7EA, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x91, 0x0001, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x92, 0xBB11, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x93, 0xDCD0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x94, 0xF7ED, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x95, 0x0001, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xB6, 0x2F19, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xB7, 0x6152, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xB8, 0x2415, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xB9, 0x6542, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xBA, 0x2615, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xBB, 0x4939, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xBC, 0x2613, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xBD, 0x543E, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xBE, 0x005C, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xBF, 0x1C0D, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xC0, 0x3B2D, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xC1, 0x0050, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xC2, 0x1B0C, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xC3, 0x3B2A, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xC4, 0x0038, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 0x06, 0x0400, 1} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE1, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDC, 0x0702, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDC, 0x0702, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDD, 0x3815, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDD, 0x3815, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDE, 0xA070, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDE, 0xA070, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDF, 0xDAC1, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDF, 0xDAC1, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE0, 0xFEEE, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE0, 0xFEEE, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x58, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x53, 0x0702, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x53, 0x0702, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x54, 0x3815, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x54, 0x3815, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x55, 0xA070, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x55, 0xA070, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x56, 0xDAC1, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x56, 0xDAC1, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x57, 0xFEEE, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x57, 0xFEEE, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x05, 0x0096, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x06, 0x000D, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x07, 0x0096, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x08, 0x000D, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x20, 0x0700, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x21, 0x0400, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x37, 0x0080, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x39, 0x031E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3A, 0x031E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3B, 0x055B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3C, 0x055B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x57, 0x01F5, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x58, 0x0259, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x59, 0x01F5, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5A, 0x0259, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5C, 0x221C, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5D, 0x2923, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x64, 0x1E1C, 0} }
		}
	},
	{"Set Night Mode", 
		{
			{ K_MICRON_CMD_REG, NULL, {2,0x37,0x0300, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0000, 0} },
			{ K_MICRON_CMD_DELAY, NULL, {1000, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0C44, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Vivid Mode", 
		{
			{ K_MICRON_CMD_REG, NULL, {1, 0x25, 0x002D, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"gamma 0.45 BC 8 JPEG", 
		{
			{ K_MICRON_CMD_REG, NULL, {1, 0x34, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x35, 0xFF00, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x53, 0x1A08, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x54, 0x603D, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x55, 0xAB8C, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x56, 0xDAC4, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0x57, 0xFFED, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDC, 0x1A08, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDD, 0x603D, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDE, 0xAB8C, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xDF, 0xDAC4, 0} },
			{ K_MICRON_CMD_REG, NULL, {1, 0xE0, 0xFFED, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Timing Settings 27mhz", 
		{
			{ K_MICRON_CMD_REG, NULL, {0, 0x05, 0x014C, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x06, 0x000D, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x07, 0x014C, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x08, 0x000D, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x20, 0x0700, 0} },
			{ K_MICRON_CMD_REG, NULL, {0, 0x21, 0x0400, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x39, 0x03D4, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3A, 0x03D4, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3B, 0x03D4, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x3C, 0x03D4, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x57, 0x01CB, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x58, 0x0227, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x59, 0x01CB, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5A, 0x0227, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5C, 0x100B, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5D, 0x140F, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x64, 0x5E1C, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Real Mode", 
		{
			{ K_MICRON_CMD_REG, NULL, {1, 0x25, 0x0005, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Video Capture OFF", 
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 4, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Std CCM", 
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x7F00, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0xF5, 0x0040, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5E, 0x674C, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x5F, 0x3D2C, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x60, 0x0002, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x02, 0x00EA, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x03, 0x3922, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x04, 0x04E4, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x09, 0x0080, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0A, 0x00B9, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0B, 0x001D, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0C, 0x0020, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0D, 0x00E9, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0E, 0x0068, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x0F, 0x000A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x10, 0x00BA, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x11, 0x00A9, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x15, 0x0081, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x16, 0x001A, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x17, 0x0031, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x18, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x19, 0x001D, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1A, 0x0006, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1B, 0x0007, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1C, 0x003E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1D, 0x006C, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 0x1E, 0x003E, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 1} },
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Default Registers", 
		{
			{ K_MICRON_CMD_LOAD, "Timing Settings 27mhz", {0, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {0,53,0x2029, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,6,0x600E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 31, 0x0090, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 34, 0x9080, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 35, 0x8878, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 40, 0xEF02, 0} },
			{ K_MICRON_CMD_REG, NULL, {2, 41, 0x847C, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 5, 0x0008, 1} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0C44, 0} },
			{ K_MICRON_CMD_LOAD, "gamma 0.45 BC 8 JPEG", {0, 0, 0, 0} },
			{ K_MICRON_CMD_LOAD, "Fast CCM", {0, 0, 0, 0} },
			{ K_MICRON_CMD_LOAD, "Real Mode", {0, 0, 0, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Video Capture ON", 
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 3, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Viewfinder OFF", 
		{
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xD2, 0x007F, 0x7F} },
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 4, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Lens Correction: Calibration - Flat Curves", 
		{
			{ K_MICRON_CMD_REG, NULL, {   1,  129, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  130, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  131, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  132, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  133, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  134, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  135, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  136, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  137, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  138, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  139, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  140, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  141, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  142, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  143, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  144, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  145, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  146, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  147, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  148, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  149, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  182, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  183, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  184, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  185, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  186, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  187, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  188, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  189, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  190, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  191, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  192, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  193, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  194, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  195, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  196, 0, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Lens Correction: Calibration - Setup", 
		{
			{ K_MICRON_CMD_REG, NULL, {   1,   37, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,    5, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,    6, 0x941C, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,   83, 0x0804, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,   84, 0x2010, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,   85, 0x6040, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,   86, 0xA080, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,   87, 0xE0C0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,   88, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  220, 0x0804, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  221, 0x2010, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  222, 0x6040, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  223, 0xA080, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  224, 0xE0C0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  225, 0x0000, 0} },
			{ K_MICRON_CMD_REG, NULL, {   0,   47, 0x0040, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Lens Correction - Calibration - Flat Vertical Curves", 
		{
			{ K_MICRON_CMD_REG, NULL, {1,  129, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  130, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  131, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  132, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  133, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  134, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  135, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  136, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  137, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  182, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  183, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  184, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  185, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  186, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,  187, 0, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Lens Correction: Calibration - Flat Horizontal Curves", 
		{
			{ K_MICRON_CMD_REG, NULL, {   1,  138, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  139, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  140, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  141, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  142, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  143, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  144, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  145, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  146, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  147, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  148, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  149, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  188, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  189, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  190, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  191, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  192, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  193, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  194, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  195, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {   1,  196, 0, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Noise Reduction", 
		{
			{ K_MICRON_CMD_REG, NULL, {2,0x3D, 0x17DD, 0} },
			{ K_MICRON_CMD_REG, NULL, {0,0x5F, 0x3630, 0} },
			{ K_MICRON_CMD_REG, NULL, {0,0x30, 0x043E, 0} },
			{ K_MICRON_CMD_REG, NULL, {1,0x3B, 0x043E, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0000, 0} },
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0C44, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Reset", 
		{
			{ K_MICRON_CMD_BITFIELD, NULL, {0, 0x0D, 0x0001, 1} },
			{ K_MICRON_CMD_BITFIELD, NULL, {0, 0x0D, 0x0020, 1} },
			{ K_MICRON_CMD_BITFIELD, NULL, {0, 0x0D, 0x0021, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{"Set Day Mode", 
		{
			{ K_MICRON_CMD_REG, NULL, {2,0x37,0x0080, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0000, 0} },
			{ K_MICRON_CMD_DELAY, NULL, {1000, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0C44, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{NULL, 
		{ 
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		} 
	}
};

static void f_MICRON_SetPage(int nPage)
{
    if (nPage == 0)
	ic_select();
    else if (nPage == 1)
	ifp_cpr_select();
    else if (nPage == 2)
	ifp_ccr_select();
}

static void f_MICRON_REG_Set(int nPage, int nReg, int nValue)
{
    f_MICRON_SetPage(nPage);
    i2c_write(nReg, nValue);
}

static void f_MICRON_BITFIELD_Set(int nPage, int nReg, int nMask, int nValue)
{
    unsigned short nTemp = 0;

    f_MICRON_SetPage(nPage);
    i2c_read(nReg, &nTemp);
    i2c_write(nReg, (~nMask & nTemp) | ((nValue) ? nMask : 0));
}

void	g_MICRON_Config(const char *szSection)
{
    int	i;
    int	j;

    printk("g_MICRON_Config: Load section [%s]\n", szSection);
    for (i = 0; f_atsSectionList[i].m_szName != NULL; i++)
    {
	if (!strcmp(f_atsSectionList[i].m_szName, szSection))
	{
	    for (j = 0; f_atsSectionList[i].m_atsCommand[j].m_nType != K_MICRON_CMD_END; j++)
	    {
		switch (f_atsSectionList[i].m_atsCommand[j].m_nType)
		{
		    case K_MICRON_CMD_REG:
			f_MICRON_REG_Set(f_atsSectionList[i].m_atsCommand[j].m_anParam[0], 
					 f_atsSectionList[i].m_atsCommand[j].m_anParam[1], 
					 f_atsSectionList[i].m_atsCommand[j].m_anParam[2]);
			break ;
		    case K_MICRON_CMD_BITFIELD:
			f_MICRON_BITFIELD_Set(f_atsSectionList[i].m_atsCommand[j].m_anParam[0], 
					      f_atsSectionList[i].m_atsCommand[j].m_anParam[1], 
					      f_atsSectionList[i].m_atsCommand[j].m_anParam[2],
					      f_atsSectionList[i].m_atsCommand[j].m_anParam[3]);
			break ;
		    case K_MICRON_CMD_DELAY:
			udelay(f_atsSectionList[i].m_atsCommand[j].m_anParam[0] * 1000);
			break ;
		    case K_MICRON_CMD_LOAD:
			g_MICRON_Config(f_atsSectionList[i].m_atsCommand[j].m_szParam);
			break ;
		    case K_MICRON_CMD_END:
			break ;
		}
	    }
	}
    }
}

#endif /* _K_MICRON_CONFIG_H_ */
