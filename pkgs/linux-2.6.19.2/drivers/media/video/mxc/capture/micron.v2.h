/*
 *  mt9v112 camera driver functions.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: micron.v2.h
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
	{
		"Reset",
		{
			{ K_MICRON_CMD_BITFIELD, NULL, {0, 0x0D, 0x0001, 1} },  //RESET_REG - reset sensor
			{ K_MICRON_CMD_BITFIELD, NULL, {0, 0x0D, 0x0020, 1} },  //RESET_REG - reset soc
			{ K_MICRON_CMD_BITFIELD, NULL, {0, 0x0D, 0x0021, 0} },  //RESET_REG - resume both
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Default Registers",
		{
			{ K_MICRON_CMD_LOAD, "Timing Settings 27mhz", {0, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {0,53,0x2029, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,6,0x600E, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1, 0xA5, 0x0000, 0} },  	//HORIZ_PAN_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xA7, 0x0280, 0} },  	//HORIZ_SIZE_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xA8, 0x0000, 0} },  	//VERT_PAN_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xAA, 0x01E0, 0} },  	//VERT_SIZE_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {2, 0x37, 0x0080, 0} },  	//SHUTTER_WIDTH_LIM_AE
			{ K_MICRON_CMD_REG, NULL, {1, 0x05, 0x0006, 0} },  	//APERTURE_GAIN
			// AWB Defaults
			{ K_MICRON_CMD_REG, NULL, {2, 31, 0x0090, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {2, 34, 0x9080, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {2, 35, 0x8878, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {2, 40, 0xEF02, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {2, 41, 0x867A, 0} }, 
			//  Enable Auto Sharpening
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 5, 0x0008, 1} }, 
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0C44, 0} }, 
			{ K_MICRON_CMD_LOAD, "gamma 0.45 BC 8 JPEG", {0, 0, 0, 0} },
			{ K_MICRON_CMD_LOAD, "Fast CCM", {0, 0, 0, 0} },
			{ K_MICRON_CMD_LOAD, "Vivid Mode", {0, 0, 0, 0} },
			{ K_MICRON_CMD_LOAD, "Pixel Noise Reduction", {0, 0, 0, 0} },
			{ K_MICRON_CMD_LOAD, "Row Noise Reduction", {0, 0, 0, 0} },
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"gamma 0.45 BC 8 JPEG",
		{
			{ K_MICRON_CMD_REG, NULL, {1, 0x34, 0x0000, 0} },  	//LUMA_OFFSET
			{ K_MICRON_CMD_REG, NULL, {1, 0x35, 0xFF00, 0} },  	//(CLIPPING_LIM_OUT_LUMA
			{ K_MICRON_CMD_REG, NULL, {1, 0x53, 0x1A08, 0} },  	//GAMMA_A_Y1_Y2
			{ K_MICRON_CMD_REG, NULL, {1, 0x54, 0x603D, 0} },  	//GAMMA_A_Y3_Y4
			{ K_MICRON_CMD_REG, NULL, {1, 0x55, 0xAB8C, 0} },  	//GAMMA_A_Y5_Y6
			{ K_MICRON_CMD_REG, NULL, {1, 0x56, 0xDAC4, 0} },  	//GAMMA_A_Y7_Y8
			{ K_MICRON_CMD_REG, NULL, {1, 0x57, 0xFFED, 0} },  	//GAMMA_A_Y9_Y10
			{ K_MICRON_CMD_REG, NULL, {1, 0xDC, 0x1A08, 0} },  	//GAMMA_B_Y1_Y2
			{ K_MICRON_CMD_REG, NULL, {1, 0xDD, 0x603D, 0} },  	//GAMMA_B_Y3_Y4
			{ K_MICRON_CMD_REG, NULL, {1, 0xDE, 0xAB8C, 0} },  	//GAMMA_B_Y5_Y6
			{ K_MICRON_CMD_REG, NULL, {1, 0xDF, 0xDAC4, 0} },  	//GAMMA_B_Y7_Y8
			{ K_MICRON_CMD_REG, NULL, {1, 0xE0, 0xFFED, 0} },  	//GAMMA_B_Y9_Y10
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Vivid Mode",
		{
			{ K_MICRON_CMD_REG, NULL, {1, 0x25, 0x002D, 0} },  	//AWB_SPEED_SATURATION
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Real Mode",
		{
			{ K_MICRON_CMD_REG, NULL, {1, 0x25, 0x0005, 0} },  	//AWB_SPEED_SATURATION
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Pixel Noise Reduction",
		{
			{ K_MICRON_CMD_REG, NULL, {0,0x34, 0xC019, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {0,0x40, 0x1800, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {0,0x76, 0x7358, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {0,0x04, 642, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {0,0x03, 482, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0xA0, 642, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0xA3, 482, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0xA6, 642, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0xA9, 482, 0} }, 
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Row Noise Reduction",
		{
			{ K_MICRON_CMD_REG, NULL, {0,0x5F, 0x3630, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {0,0x30, 0x043E, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0x3B, 0x043E, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0000, 0} }, 
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_REG, NULL, {2,0x3D, 0x17DD, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {2,0x2E,0x0C44, 0} }, 
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Fast CCM",
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x4000, 0} },  //MATRIX_ADJ_LIMITS
			{ K_MICRON_CMD_REG, NULL, {2, 0xF5, 0x0020, 0} },  //MWB POSITION
			{ K_MICRON_CMD_REG, NULL, {2, 0x5E, 0x4962, 0} },  //RATIO_BASE_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x5F, 0x7A58, 0} },  //RATIO_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x60, 0x0002, 0} },  //SIGNS_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x02, 0x00EA, 0} },  //BASE_MATRIX_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x03, 0x291A, 0} },  //BASE_MATRIX_SCALE_K1_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x04, 0x04A4, 0} },  //BASE_MATRIX_SCALE_K6_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x09, 0x0097, 0} },  //BASE_MATRIX_COEF_K1
			{ K_MICRON_CMD_REG, NULL, {2, 0x0A, 0x0072, 0} },  //BASE_MATRIX_COEF_K2
			{ K_MICRON_CMD_REG, NULL, {2, 0x0B, 0x001E, 0} },  //BASE_MATRIX_COEF_K3
			{ K_MICRON_CMD_REG, NULL, {2, 0x0C, 0x001D, 0} },  //BASE_MATRIX_COEF_K4
			{ K_MICRON_CMD_REG, NULL, {2, 0x0D, 0x007E, 0} },  //BASE_MATRIX_COEF_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x0E, 0x0072, 0} },  //BASE_MATRIX_COEF_K6
			{ K_MICRON_CMD_REG, NULL, {2, 0x0F, 0x0011, 0} },  //BASE_MATRIX_COEF_K7
			{ K_MICRON_CMD_REG, NULL, {2, 0x10, 0x0034, 0} },  //BASE_MATRIX_COEF_K8
			{ K_MICRON_CMD_REG, NULL, {2, 0x11, 0x0082, 0} },  //BASE_MATRIX_COEF_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x15, 0x0111, 0} },  //DELTA_COEFS_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x16, 0x003A, 0} },  //DELTA_MATRIX_COEF_D1
			{ K_MICRON_CMD_REG, NULL, {2, 0x17, 0x003B, 0} },  //DELTA_MATRIX_COEF_D2
			{ K_MICRON_CMD_REG, NULL, {2, 0x18, 0x0022, 0} },  //DELTA_MATRIX_COEF_D3
			{ K_MICRON_CMD_REG, NULL, {2, 0x19, 0x0051, 0} },  //DELTA_MATRIX_COEF_D4
			{ K_MICRON_CMD_REG, NULL, {2, 0x1A, 0x002B, 0} },  //DELTA_MATRIX_COEF_D5
			{ K_MICRON_CMD_REG, NULL, {2, 0x1B, 0x0032, 0} },  //DELTA_MATRIX_COEF_D6
			{ K_MICRON_CMD_REG, NULL, {2, 0x1C, 0x0071, 0} },  //DELTA_MATRIX_COEF_D7
			{ K_MICRON_CMD_REG, NULL, {2, 0x1D, 0x00BB, 0} },  //DELTA_MATRIX_COEF_D8
			{ K_MICRON_CMD_REG, NULL, {2, 0x1E, 0x00CB, 0} },  //DELTA_MATRIX_COEF_D9
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 1} }, 
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 0} }, 
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Fast CCM - Horizon Cancelling",
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x4000, 0} },  //MATRIX_ADJ_LIMITS
			{ K_MICRON_CMD_REG, NULL, {2, 0xF5, 0x0020, 0} },  //MWB POSITION
			{ K_MICRON_CMD_REG, NULL, {2, 0x5E, 0x4962, 0} },  //RATIO_BASE_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x5F, 0x8E68, 0} },  //RATIO_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x60, 0x0002, 0} },  //SIGNS_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x02, 0x00EA, 0} },  //BASE_MATRIX_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x03, 0x291A, 0} },  //BASE_MATRIX_SCALE_K1_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x04, 0x04A4, 0} },  //BASE_MATRIX_SCALE_K6_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x09, 0x0097, 0} },  //BASE_MATRIX_COEF_K1
			{ K_MICRON_CMD_REG, NULL, {2, 0x0A, 0x0072, 0} },  //BASE_MATRIX_COEF_K2
			{ K_MICRON_CMD_REG, NULL, {2, 0x0B, 0x001E, 0} },  //BASE_MATRIX_COEF_K3
			{ K_MICRON_CMD_REG, NULL, {2, 0x0C, 0x001D, 0} },  //BASE_MATRIX_COEF_K4
			{ K_MICRON_CMD_REG, NULL, {2, 0x0D, 0x007E, 0} },  //BASE_MATRIX_COEF_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x0E, 0x0072, 0} },  //BASE_MATRIX_COEF_K6
			{ K_MICRON_CMD_REG, NULL, {2, 0x0F, 0x0011, 0} },  //BASE_MATRIX_COEF_K7
			{ K_MICRON_CMD_REG, NULL, {2, 0x10, 0x0034, 0} },  //BASE_MATRIX_COEF_K8
			{ K_MICRON_CMD_REG, NULL, {2, 0x11, 0x0082, 0} },  //BASE_MATRIX_COEF_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x15, 0x0111, 0} },  //DELTA_COEFS_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x16, 0x003A, 0} },  //DELTA_MATRIX_COEF_D1
			{ K_MICRON_CMD_REG, NULL, {2, 0x17, 0x003B, 0} },  //DELTA_MATRIX_COEF_D2
			{ K_MICRON_CMD_REG, NULL, {2, 0x18, 0x0022, 0} },  //DELTA_MATRIX_COEF_D3
			{ K_MICRON_CMD_REG, NULL, {2, 0x19, 0x0051, 0} },  //DELTA_MATRIX_COEF_D4
			{ K_MICRON_CMD_REG, NULL, {2, 0x1A, 0x002B, 0} },  //DELTA_MATRIX_COEF_D5
			{ K_MICRON_CMD_REG, NULL, {2, 0x1B, 0x0032, 0} },  //DELTA_MATRIX_COEF_D6
			{ K_MICRON_CMD_REG, NULL, {2, 0x1C, 0x0071, 0} },  //DELTA_MATRIX_COEF_D7
			{ K_MICRON_CMD_REG, NULL, {2, 0x1D, 0x00BB, 0} },  //DELTA_MATRIX_COEF_D8
			{ K_MICRON_CMD_REG, NULL, {2, 0x1E, 0x00CB, 0} },  //DELTA_MATRIX_COEF_D9
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 1} }, 
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 0} }, 
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Std CCM",
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x7F00, 0} },  //MATRIX_ADJ_LIMITS
			{ K_MICRON_CMD_REG, NULL, {2, 0xF5, 0x0040, 0} },  //MWB POSITION
			{ K_MICRON_CMD_REG, NULL, {2, 0x5E, 0x674C, 0} },  //RATIO_BASE_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x5F, 0x3D2C, 0} },  //RATIO_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x60, 0x0002, 0} },  //SIGNS_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x02, 0x00EA, 0} },  //BASE_MATRIX_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x03, 0x3922, 0} },  //BASE_MATRIX_SCALE_K1_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x04, 0x04E4, 0} },  //BASE_MATRIX_SCALE_K6_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x09, 0x0080, 0} },  //BASE_MATRIX_COEF_K1
			{ K_MICRON_CMD_REG, NULL, {2, 0x0A, 0x00B9, 0} },  //BASE_MATRIX_COEF_K2
			{ K_MICRON_CMD_REG, NULL, {2, 0x0B, 0x001D, 0} },  //BASE_MATRIX_COEF_K3
			{ K_MICRON_CMD_REG, NULL, {2, 0x0C, 0x0020, 0} },  //BASE_MATRIX_COEF_K4
			{ K_MICRON_CMD_REG, NULL, {2, 0x0D, 0x00E9, 0} },  //BASE_MATRIX_COEF_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x0E, 0x0068, 0} },  //BASE_MATRIX_COEF_K6
			{ K_MICRON_CMD_REG, NULL, {2, 0x0F, 0x000A, 0} },  //BASE_MATRIX_COEF_K7
			{ K_MICRON_CMD_REG, NULL, {2, 0x10, 0x00BA, 0} },  //BASE_MATRIX_COEF_K8
			{ K_MICRON_CMD_REG, NULL, {2, 0x11, 0x00A9, 0} },  //BASE_MATRIX_COEF_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x15, 0x0081, 0} },  //DELTA_COEFS_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x16, 0x001A, 0} },  //DELTA_MATRIX_COEF_D1
			{ K_MICRON_CMD_REG, NULL, {2, 0x17, 0x0031, 0} },  //DELTA_MATRIX_COEF_D2
			{ K_MICRON_CMD_REG, NULL, {2, 0x18, 0x0000, 0} },  //DELTA_MATRIX_COEF_D3
			{ K_MICRON_CMD_REG, NULL, {2, 0x19, 0x001D, 0} },  //DELTA_MATRIX_COEF_D4
			{ K_MICRON_CMD_REG, NULL, {2, 0x1A, 0x0006, 0} },  //DELTA_MATRIX_COEF_D5
			{ K_MICRON_CMD_REG, NULL, {2, 0x1B, 0x0007, 0} },  //DELTA_MATRIX_COEF_D6
			{ K_MICRON_CMD_REG, NULL, {2, 0x1C, 0x003E, 0} },  //DELTA_MATRIX_COEF_D7
			{ K_MICRON_CMD_REG, NULL, {2, 0x1D, 0x006C, 0} },  //DELTA_MATRIX_COEF_D8
			{ K_MICRON_CMD_REG, NULL, {2, 0x1E, 0x003E, 0} },  //DELTA_MATRIX_COEF_D9
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 1} }, 
			{ K_MICRON_CMD_DELAY, NULL, {500, 0, 0, 0} },
			{ K_MICRON_CMD_BITFIELD, NULL, {1, 6, 0x8000, 0} }, 
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Timing Settings 27mhz",
		{
			{ K_MICRON_CMD_REG, NULL, {0, 0x05, 0x014C, 0} },      // Context B (full-res) Horizontal Blank
			{ K_MICRON_CMD_REG, NULL, {0, 0x06, 0x000D, 0} },      // Context B (full-res) Vertical Blank
			{ K_MICRON_CMD_REG, NULL, {0, 0x07, 0x014C, 0} },      // Context A (preview) Horizontal Blank
			{ K_MICRON_CMD_REG, NULL, {0, 0x08, 0x000D, 0} },      // Context A (preview) Vertical Blank
			{ K_MICRON_CMD_REG, NULL, {0, 0x20, 0x0700, 0} },      // Read Mode Context B
			{ K_MICRON_CMD_REG, NULL, {0, 0x21, 0x0400, 0} },      // Read Mode Context A
			{ K_MICRON_CMD_REG, NULL, {2, 0x39, 0x03D4, 0} },      // AE Line size Context A
			{ K_MICRON_CMD_REG, NULL, {2, 0x3A, 0x03D4, 0} },      // AE Line size Context B
			{ K_MICRON_CMD_REG, NULL, {2, 0x3B, 0x03D4, 0} },      // AE shutter delay limit Context A
			{ K_MICRON_CMD_REG, NULL, {2, 0x3C, 0x03D4, 0} },      // AE shutter delay limit Context B
			{ K_MICRON_CMD_REG, NULL, {2, 0x57, 0x01CB, 0} },      // Context A Flicker full frame time (60Hz)
			{ K_MICRON_CMD_REG, NULL, {2, 0x58, 0x0227, 0} },      // Context A Flicker full frame time (50Hz)
			{ K_MICRON_CMD_REG, NULL, {2, 0x59, 0x01CB, 0} },      // Context B Flicker full frame time (60Hz)
			{ K_MICRON_CMD_REG, NULL, {2, 0x5A, 0x0227, 0} },      // Context B Flicker full frame time (50Hz)
			{ K_MICRON_CMD_REG, NULL, {2, 0x5C, 0x100B, 0} },      // 60Hz Flicker Search Range
			{ K_MICRON_CMD_REG, NULL, {2, 0x5D, 0x140F, 0} },      // 50Hz Flicker Search Range
			{ K_MICRON_CMD_REG, NULL, {2, 0x64, 0x5E1C, 0} },      // Flicker parameter
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Set Night Mode",
		{
			{ K_MICRON_CMD_REG, NULL, {2,0x37,0x0300, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0x05, 0x000E, 0} },  	// APERTURE_GAIN
			{ K_MICRON_CMD_REG, NULL, {1,0x25, 0x0005, 0} },  	// AWB_SPEED_SATURATION
			{ K_MICRON_CMD_REG, NULL, {1,0x53, 0x0803, 0} },  	// GAMMA_A_Y1_Y2
			{ K_MICRON_CMD_REG, NULL, {1,0x54, 0x3C19, 0} },  	// GAMMA_A_Y3_Y4
			{ K_MICRON_CMD_REG, NULL, {1,0x55, 0x936D, 0} },  	// GAMMA_A_Y5_Y6
			{ K_MICRON_CMD_REG, NULL, {1,0x56, 0xCFB3, 0} },  	// GAMMA_A_Y7_Y8
			{ K_MICRON_CMD_REG, NULL, {1,0x57, 0xFFE8, 0} },  	// GAMMA_A_Y9_Y10
			{ K_MICRON_CMD_REG, NULL, {1,0x58, 0x0000, 0} },  	// GAMMA_A_Y0
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Set Day Mode",
		{
			{ K_MICRON_CMD_REG, NULL, {2,0x37,0x0080, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0x05,0x0006, 0} },  	// APERTURE_GAIN
			{ K_MICRON_CMD_REG, NULL, {1,0x06,0x601C, 0} },  	// MODE_CONTROL
			{ K_MICRON_CMD_REG, NULL, {1,0x25,0x002D, 0} },  	// AWB_SPEED_SATURATION
			{ K_MICRON_CMD_REG, NULL, {1,0x53,0x0502, 0} },  	// GAMMA_A_Y1_Y2
			{ K_MICRON_CMD_REG, NULL, {1,0x54,0x280F, 0} },  	// GAMMA_A_Y3_Y4
			{ K_MICRON_CMD_REG, NULL, {1,0x55,0x8155, 0} },  	// GAMMA_A_Y5_Y6
			{ K_MICRON_CMD_REG, NULL, {1,0x56,0xC9A8, 0} },  	// GAMMA_A_Y7_Y8
			{ K_MICRON_CMD_REG, NULL, {1,0x57,0xFFE6, 0} },  	// GAMMA_A_Y9_Y10
			{ K_MICRON_CMD_REG, NULL, {1,0x58,0x0000, 0} },  	// GAMMA_A_Y0
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Set middle Mode",
		{
			{ K_MICRON_CMD_REG, NULL, {2,0x37,0x0100, 0} }, 
			{ K_MICRON_CMD_REG, NULL, {1,0x05, 0x0005, 0} },  	// APERTURE_GAIN
			{ K_MICRON_CMD_REG, NULL, {1,0x25, 0x002D, 0} },  	// AWB_SPEED_SATURATION
			{ K_MICRON_CMD_REG, NULL, {1, 0x53, 0x1E0A, 0} },  	//(112) GAMMA_A_Y1_Y2
			{ K_MICRON_CMD_REG, NULL, {1, 0x54, 0x6740, 0} },  	//(112) GAMMA_A_Y3_Y4
			{ K_MICRON_CMD_REG, NULL, {1, 0x55, 0xBD9C, 0} },  	//(112) GAMMA_A_Y5_Y6
			{ K_MICRON_CMD_REG, NULL, {1, 0x56, 0xE5D3, 0} },  	//(112) GAMMA_A_Y7_Y8
			{ K_MICRON_CMD_REG, NULL, {1, 0x57, 0xFFF3, 0} },  	//(112) GAMMA_A_Y9_Y10
			{ K_MICRON_CMD_REG, NULL, {1, 0x58, 0x0000, 0} },  	//(56) GAMMA_A_Y0
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Viewfinder ON",
		{
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xD2, 0x007F, 0} },     //DEFAULT_CONFIG set to Context A
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 4, 0} },                  //PROGRAM_SELECT DEFAULT PROGRAM
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },     //PROGRAM_ADVANCE, ADVANCE_1
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Viewfinder OFF",
		{
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xD2, 0x007F, 0x7F} },  //DEFAULT_CONFIG set to Context B
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 4, 0} },                  //PROGRAM_SELECT DEFAULT PROGRAM 
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },     //PROGRAM_ADVANCE, ADVANCE_1
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Video Capture ON",
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 3, 0} },                  //PROGRAM_SELECT VIDEO CLIP
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },     //PROGRAM_ADVANCE, ADVANCE_1
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Video Capture OFF",
		{
			{ K_MICRON_CMD_REG, NULL, {2, 0xCC, 4, 0} },                  //PROGRAM_SELECT DEFAULT PROGRAM
			{ K_MICRON_CMD_BITFIELD, NULL, {2, 0xCB, 0x0001, 1} },     //PROGRAM_ADVANCE, ADVANCE_1
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Lens Correction: Calibration - Setup",
		{
			{ K_MICRON_CMD_REG, NULL, {1,   37, 0, 0} },        // Turn off saturation attenuation
			{ K_MICRON_CMD_REG, NULL, {1,    5, 0, 0} },        // Disable aperture correction
			{ K_MICRON_CMD_REG, NULL, {1,    6, 0x941C, 0} },   // Lens, MWB, Bypass Col. Proc.: ON. Defect, flicker, AWB, AE: OFF.
			{ K_MICRON_CMD_REG, NULL, {1,   83, 0x0804, 0} },   // Linear Gamma Knee Points Y1 and Y2
			{ K_MICRON_CMD_REG, NULL, {1,   84, 0x2010, 0} },   // Linear Gamma Knee Points Y3 and Y4
			{ K_MICRON_CMD_REG, NULL, {1,   85, 0x6040, 0} },   // Linear Gamma Knee Points Y5 and Y6
			{ K_MICRON_CMD_REG, NULL, {1,   86, 0xA080, 0} },   // Linear Gamma Knee Points Y7 and Y8 
			{ K_MICRON_CMD_REG, NULL, {1,   87, 0xE0C0, 0} },   // Linear Gamma Knee Points Y9 and Y10
			{ K_MICRON_CMD_REG, NULL, {1,   88, 0x0000, 0} },   // Linear Gamma Knee Point  Y0
			{ K_MICRON_CMD_REG, NULL, {1,  220, 0x0804, 0} },   // Linear Gamma Knee Points Y1 and Y2
			{ K_MICRON_CMD_REG, NULL, {1,  221, 0x2010, 0} },   // Linear Gamma Knee Points Y3 and Y4
			{ K_MICRON_CMD_REG, NULL, {1,  222, 0x6040, 0} },   // Linear Gamma Knee Points Y5 and Y6
			{ K_MICRON_CMD_REG, NULL, {1,  223, 0xA080, 0} },   // Linear Gamma Knee Points Y7 and Y8 
			{ K_MICRON_CMD_REG, NULL, {1,  224, 0xE0C0, 0} },   // Linear Gamma Knee Points Y9 and Y10
			{ K_MICRON_CMD_REG, NULL, {1,  225, 0x0000, 0} },   // Linear Gamma Knee Point  Y0
			{ K_MICRON_CMD_REG, NULL, {0,   47, 0x0020, 0} },   // Set Global Gain to 1.0
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Lens Correction: Calibration - Flat Curves",
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
			{ K_MICRON_CMD_REG, NULL, {1,  138, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  139, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  140, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  141, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  142, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  143, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  144, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  145, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  146, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  147, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  148, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  149, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  182, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  183, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  184, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  185, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  186, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  187, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  188, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  189, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  190, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  191, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  192, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  193, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  194, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  195, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  196, 0, 0} },       
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Lens Correction: Calibration - Flat Horizontal Curves",
		{
			{ K_MICRON_CMD_REG, NULL, {1,  138, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  139, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  140, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  141, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  142, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  143, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  144, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  145, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  146, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  147, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  148, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  149, 0, 0} },   
			{ K_MICRON_CMD_REG, NULL, {1,  188, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  189, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  190, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  191, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  192, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  193, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  194, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  195, 0, 0} },        
			{ K_MICRON_CMD_REG, NULL, {1,  196, 0, 0} },     
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Lens Correction - Calibration - Flat Vertical Curves",
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
	{
		"Data Types",
		{
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Zoom",
		{
			{ K_MICRON_CMD_REG, NULL, {0x01, 0xA6, 0x0280, 0} },  	//HORIZ_ZOOM_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {0x01, 0xA9, 0x01E0, 0} },  	//VERT_ZOOM_RESIZE_A
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"Pan A",
		{
			{ K_MICRON_CMD_BITFIELD, NULL, {0x01, 0xA5, 0x07FF, 0x0000} },  	//
			{ K_MICRON_CMD_BITFIELD, NULL, {0x01, 0xA5, 0x4000, 0x0001} },  	//
			{ K_MICRON_CMD_BITFIELD, NULL, {0x01, 0xA8, 0x07FF, 0x0000} },  	//
			{ K_MICRON_CMD_BITFIELD, NULL, {0x01, 0xA8, 0x4000, 0x0001} },  	//
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		"INIT CIF 15FPS",
		{
			{ K_MICRON_CMD_REG, NULL, {0, 0x03, 0x01E2, 0} },  	//(1) ROW_WINDOW_SIZE_REG
			{ K_MICRON_CMD_REG, NULL, {0, 0x04, 0x0282, 0} },  	//(1) COL_WINDOW_SIZE_REG
			{ K_MICRON_CMD_REG, NULL, {0, 0x05, 0x014C, 0} },  	//(1) HORZ_BLANK_B
			{ K_MICRON_CMD_REG, NULL, {0, 0x06, 0x000D, 0} },  	//(1) VERT_BLANK_B
			{ K_MICRON_CMD_REG, NULL, {0, 0x07, 0x014C, 0} },  	//(1) HORZ_BLANK_A
			{ K_MICRON_CMD_REG, NULL, {0, 0x08, 0x000D, 0} },  	//(1) VERT_BLANK_A
			{ K_MICRON_CMD_REG, NULL, {0, 0x0D, 0x0008, 0} },  	//(3) RESET_REG
			{ K_MICRON_CMD_REG, NULL, {0, 0x20, 0x0700, 0} },  	//(1) READ_MODE_B
			{ K_MICRON_CMD_REG, NULL, {0, 0x21, 0x0400, 0} },  	//(1) READ_MODE_A
			{ K_MICRON_CMD_REG, NULL, {0, 0x30, 0x043E, 0} },  	//(1) ROW_NOISE_CONTROL_REG
			{ K_MICRON_CMD_REG, NULL, {0, 0x34, 0xC019, 0} },  	//(1) RESERVED_SENSOR_34
			{ K_MICRON_CMD_REG, NULL, {0, 0x35, 0x2029, 0} },  	//(1) ANTI_ECLIPSE
			{ K_MICRON_CMD_REG, NULL, {0, 0x40, 0x1800, 0} },  	//(1) RESERVED_SENSOR_40
			{ K_MICRON_CMD_REG, NULL, {0, 0x5F, 0x3630, 0} },  	//(1) CAL_THRESHOLD
			{ K_MICRON_CMD_REG, NULL, {0, 0x76, 0x7358, 0} },  	//(1) RESERVED_SENSOR_76
			{ K_MICRON_CMD_REG, NULL, {1, 0x05, 0x0005, 0} },  	//(6) APERTURE_GAIN
			{ K_MICRON_CMD_REG, NULL, {1, 0x06, 0x608E, 0} },  	//(4) MODE_CONTROL
			{ K_MICRON_CMD_REG, NULL, {1, 0x25, 0x002D, 0} },  	//(7) AWB_SPEED_SATURATION
			{ K_MICRON_CMD_REG, NULL, {1, 0x34, 0x0000, 0} },  	//(1) LUMA_OFFSET
			{ K_MICRON_CMD_REG, NULL, {1, 0x35, 0xFF00, 0} },  	//(1) CLIPPING_LIM_OUT_LUMA
			{ K_MICRON_CMD_REG, NULL, {1, 0x3B, 0x043E, 0} },  	//(1) IFP_BLACK_LEVEL_SUBTRACTION
			{ K_MICRON_CMD_REG, NULL, {1, 0x53, 0x1E0A, 0} },  	//(112) GAMMA_A_Y1_Y2
			{ K_MICRON_CMD_REG, NULL, {1, 0x54, 0x6740, 0} },  	//(112) GAMMA_A_Y3_Y4
			{ K_MICRON_CMD_REG, NULL, {1, 0x55, 0xBD9C, 0} },  	//(112) GAMMA_A_Y5_Y6
			{ K_MICRON_CMD_REG, NULL, {1, 0x56, 0xE5D3, 0} },  	//(112) GAMMA_A_Y7_Y8
			{ K_MICRON_CMD_REG, NULL, {1, 0x57, 0xFFF3, 0} },  	//(112) GAMMA_A_Y9_Y10
			{ K_MICRON_CMD_REG, NULL, {1, 0x58, 0x0000, 0} },  	//(56) GAMMA_A_Y0
			{ K_MICRON_CMD_REG, NULL, {1, 0xA0, 0x0282, 0} },  	//(1) HORIZ_ZOOM_RESIZE_B
			{ K_MICRON_CMD_REG, NULL, {1, 0xA3, 0x01E2, 0} },  	//(1) VERT_ZOOM_RESIZE_B
			{ K_MICRON_CMD_REG, NULL, {1, 0xA5, 0x4000, 0} },  	//(11) HORIZ_PAN_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xA6, 0x0280, 0} },  	//(2) HORIZ_ZOOM_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xA7, 0x0160, 0} },  	//(2) HORIZ_SIZE_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xA8, 0x4000, 0} },  	//(11) VERT_PAN_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xA9, 0x01E0, 0} },  	//(2) VERT_ZOOM_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xAA, 0x0120, 0} },  	//(2) VERT_SIZE_RESIZE_A
			{ K_MICRON_CMD_REG, NULL, {1, 0xDC, 0x1A08, 0} },  	//(1) GAMMA_B_Y1_Y2
			{ K_MICRON_CMD_REG, NULL, {1, 0xDD, 0x603D, 0} },  	//(1) GAMMA_B_Y3_Y4
			{ K_MICRON_CMD_REG, NULL, {1, 0xDE, 0xAB8C, 0} },  	//(1) GAMMA_B_Y5_Y6
			{ K_MICRON_CMD_REG, NULL, {1, 0xDF, 0xDAC4, 0} },  	//(1) GAMMA_B_Y7_Y8
			{ K_MICRON_CMD_REG, NULL, {1, 0xE0, 0xFFED, 0} },  	//(1) GAMMA_B_Y9_Y10
			{ K_MICRON_CMD_REG, NULL, {2, 0x02, 0x00CA, 0} },  	//(4) BASE_MATRIX_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x03, 0x2922, 0} },  	//(4) BASE_MATRIX_SCALE_K1_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x04, 0x04A4, 0} },  	//(4) BASE_MATRIX_SCALE_K6_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x09, 0x0073, 0} },  	//(4) BASE_MATRIX_COEF_K1
			{ K_MICRON_CMD_REG, NULL, {2, 0x0A, 0x0084, 0} },  	//(4) BASE_MATRIX_COEF_K2
			{ K_MICRON_CMD_REG, NULL, {2, 0x0B, 0x000E, 0} },  	//(4) BASE_MATRIX_COEF_K3
			{ K_MICRON_CMD_REG, NULL, {2, 0x0C, 0x0000, 0} },  	//(4) BASE_MATRIX_COEF_K4
			{ K_MICRON_CMD_REG, NULL, {2, 0x0D, 0x006C, 0} },  	//(4) BASE_MATRIX_COEF_K5
			{ K_MICRON_CMD_REG, NULL, {2, 0x0E, 0x004B, 0} },  	//(4) BASE_MATRIX_COEF_K6
			{ K_MICRON_CMD_REG, NULL, {2, 0x0F, 0x0021, 0} },  	//(4) BASE_MATRIX_COEF_K7
			{ K_MICRON_CMD_REG, NULL, {2, 0x10, 0x001D, 0} },  	//(4) BASE_MATRIX_COEF_K8
			{ K_MICRON_CMD_REG, NULL, {2, 0x11, 0x0060, 0} },  	//(4) BASE_MATRIX_COEF_K9
			{ K_MICRON_CMD_REG, NULL, {2, 0x15, 0x0111, 0} },  	//(4) DELTA_COEFS_SIGNS
			{ K_MICRON_CMD_REG, NULL, {2, 0x16, 0x003A, 0} },  	//(4) DELTA_MATRIX_COEF_D1
			{ K_MICRON_CMD_REG, NULL, {2, 0x17, 0x0076, 0} },  	//(4) DELTA_MATRIX_COEF_D2
			{ K_MICRON_CMD_REG, NULL, {2, 0x18, 0x0022, 0} },  	//(4) DELTA_MATRIX_COEF_D3
			{ K_MICRON_CMD_REG, NULL, {2, 0x19, 0x0051, 0} },  	//(4) DELTA_MATRIX_COEF_D4
			{ K_MICRON_CMD_REG, NULL, {2, 0x1A, 0x002B, 0} },  	//(4) DELTA_MATRIX_COEF_D5
			{ K_MICRON_CMD_REG, NULL, {2, 0x1B, 0x0032, 0} },  	//(4) DELTA_MATRIX_COEF_D6
			{ K_MICRON_CMD_REG, NULL, {2, 0x1C, 0x0071, 0} },  	//(4) DELTA_MATRIX_COEF_D7
			{ K_MICRON_CMD_REG, NULL, {2, 0x1D, 0x00BB, 0} },  	//(4) DELTA_MATRIX_COEF_D8
			{ K_MICRON_CMD_REG, NULL, {2, 0x1E, 0x00CB, 0} },  	//(4) DELTA_MATRIX_COEF_D9
			{ K_MICRON_CMD_REG, NULL, {2, 0x1F, 0x0090, 0} },  	//(1) AWB_CR_CB_LIMITS
			{ K_MICRON_CMD_REG, NULL, {2, 0x22, 0x9080, 0} },  	//(1) AWB_RED_LIMIT
			{ K_MICRON_CMD_REG, NULL, {2, 0x23, 0x8878, 0} },  	//(1) AWB_BLUE_LIMIT
			{ K_MICRON_CMD_REG, NULL, {2, 0x24, 0x4000, 0} },  	//(1) MATRIX_ADJ_LIMITS
			{ K_MICRON_CMD_REG, NULL, {2, 0x28, 0xEF02, 0} },  	//(1) AWB_ADVANCED_CONTROL_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x29, 0x867A, 0} },  	//(1) AWB_WIDE_GATES
			{ K_MICRON_CMD_REG, NULL, {2, 0x2E, 0x0C46, 0} },  	//(58) AE_PRECISION_TARGET
			{ K_MICRON_CMD_REG, NULL, {2, 0x37, 0x0100, 0} },  	//(2) SHUTTER_WIDTH_LIM_AE
			{ K_MICRON_CMD_REG, NULL, {2, 0x39, 0x03D4, 0} },  	//(1) AE_LINE_SIZE_REG_60HZ
			{ K_MICRON_CMD_REG, NULL, {2, 0x3A, 0x03D4, 0} },  	//(1) AE_LINE_SIZE_REG_50HZ
			{ K_MICRON_CMD_REG, NULL, {2, 0x3B, 0x03D4, 0} },  	//(1) AE_LIMIT_SHUTTER_DELAY_60HZ
			{ K_MICRON_CMD_REG, NULL, {2, 0x3C, 0x03D4, 0} },  	//(1) AE_LIMIT_SHUTTER_DELAY_50HZ
			{ K_MICRON_CMD_REG, NULL, {2, 0x3D, 0x17DD, 0} },  	//(1) ADC_LIMITS_AE_ADJ
			{ K_MICRON_CMD_REG, NULL, {2, 0x57, 0x01CB, 0} },  	//(1) AE_WIDTH_60HZ_PREVIEW
			{ K_MICRON_CMD_REG, NULL, {2, 0x58, 0x0227, 0} },  	//(1) AE_WIDTH_50HZ_PREVIEW
			{ K_MICRON_CMD_REG, NULL, {2, 0x59, 0x01CB, 0} },  	//(1) AE_WIDTH_60HZ_FULLRES
			{ K_MICRON_CMD_REG, NULL, {2, 0x5A, 0x0227, 0} },  	//(1) AE_WIDTH_50HZ_FULLRES
			{ K_MICRON_CMD_REG, NULL, {2, 0x5C, 0x100B, 0} },  	//(1) SEARCH_FLICKER_60
			{ K_MICRON_CMD_REG, NULL, {2, 0x5D, 0x140F, 0} },  	//(1) SEARCH_FLICKER_50
			{ K_MICRON_CMD_REG, NULL, {2, 0x5E, 0x4962, 0} },  	//(1) RATIO_BASE_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x5F, 0x7A58, 0} },  	//(1) RATIO_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x60, 0x0002, 0} },  	//(1) SIGNS_DELTA_REG
			{ K_MICRON_CMD_REG, NULL, {2, 0x64, 0x5E1C, 0} },  	//(1) RESERVED_CAMCTRL_64
			{ K_MICRON_CMD_REG, NULL, {2, 0xF5, 0x0020, 0} },  	//(1) FLASH_MATRIX_COEF_F1
			{ K_MICRON_CMD_END, NULL, {0, 0, 0, 0} }
		}
	},
	{
		NULL, 
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
    unsigned short	nTemp = 0;

    f_MICRON_SetPage(nPage);
    i2c_read(nReg, &nTemp);
    i2c_write(nReg, (unsigned short)((~nMask & nTemp) | (nValue ? nMask : 0)));
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
