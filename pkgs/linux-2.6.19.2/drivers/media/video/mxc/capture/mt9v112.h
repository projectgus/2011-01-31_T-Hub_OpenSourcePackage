/*
 *  mt9v112 camera driver functions.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: mt9v112.h
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

/*!
 * @defgroup Camera Sensor Drivers
 */

/*!
 * @file mt9v112.h
 *
 * @brief MT9V112 Camera Header file
 *
 * It include all the defines for bitmaps operations, also two main structure
 * one for IFP interface structure, other for sensor core registers.
 *
 * @ingroup Camera
 */

#ifndef MT9V112_H_
#define MT9V112_H_

/*!
 * mt9v111 CHIP VERSION 
 */
#define MT9V112_CHIP_VERSION	0x1229

#define MT9V112_REGISTER_SELECT             0xF0

/*!
 * mt9v112 IFP REGISTER BANK MAP Colorpipe Registers
 */
#define MT9V112ICP_APERTURE_GAIN            0x5
#define MT9V112ICP_MODE_CONTROL             0x6
#define MT9V112ICP_OUTPUT_FORMAT_CTRL       0x8
#define MT9V112ICP_COLOR_SAT_CONTROL        0x25
#define MT9V112ICP_LUMA_OFFSET              0x34
#define MT9V112ICP_CLIP_LIMIT_OUTPUT_LUMI   0x35
#define MT9V112ICP_OUTPUT_FORMAT_CTRL2_CTXTA      0x3a
#define MT9V112ICP_BLACK_SUBSTRACTION       0x3b
#define MT9V112ICP_BLACK_ADDITION           0x3c
#define MT9V112ICP_INTERP_DEFECT_CORR_EDGE_THRES  0x47
#define MT9V112ICP_TESTPATTERNGEN           0x48
#define MT9V112ICP_GAMMA_Y1Y2_CTXA          0x53
#define MT9V112ICP_GAMMA_Y3Y4_CTXA          0x54
#define MT9V112ICP_GAMMA_Y5Y6_CTXA          0x55
#define MT9V112ICP_GAMMA_Y7Y8_CTXA          0x56
#define MT9V112ICP_GAMMA_Y9Y10_CTXA         0x57
#define MT9V112ICP_GAMMA_Y0_CTXA                 0x58
#define MT9V112ICP_LENS_CORR_CTRL           0x80
#define MT9V112ICP_LENS_V_RED_KNEE_0        0x81
#define MT9V112ICP_LENS_V_RED_KNEE_12       0x82
#define MT9V112ICP_LENS_V_RED_KNEE_34       0x83
#define MT9V112ICP_LENS_V_GREEN_KNEE_0      0x84
#define MT9V112ICP_LENS_V_GREEN_KNEE_12     0x85
#define MT9V112ICP_LENS_V_GREEN_KNEE_34     0x86
#define MT9V112ICP_LENS_V_BLUE_KNEE_0       0x87
#define MT9V112ICP_LENS_V_BLUE_KNEE_12      0x88
#define MT9V112ICP_LENS_V_BLUE_KNEE_34      0x89
#define MT9V112ICP_LENS_H_RED_KNEE_0        0x8A
#define MT9V112ICP_LENS_H_RED_KNEE_12       0x8B
#define MT9V112ICP_LENS_H_RED_KNEE_34       0x8C
#define MT9V112ICP_LENS_H_RED_KNEE_5        0x8D
#define MT9V112ICP_LENS_H_GREEN_KNEE_0      0x8E
#define MT9V112ICP_LENS_H_GREEN_KNEE_12     0x8F
#define MT9V112ICP_LENS_H_GREEN_KNEE_34     0x90
#define MT9V112ICP_LENS_H_GREEN_KNEE_5      0x91
#define MT9V112ICP_LENS_H_BLUE_KNEE_0       0x92
#define MT9V112ICP_LENS_H_BLUE_KNEE_12      0x93
#define MT9V112ICP_LENS_H_BLUE_KNEE_34      0x94
#define MT9V112ICP_LENS_H_BLUE_KNEE_5       0x95
#define MT9V112ICP_LINE_COUNTER             0x99
#define MT9V112ICP_FRAME_COUNTER            0x9A
#define MT9V112ICP_OUTPUT_FMT_CTRL2_CTXB    0x9B
#define MT9V112ICP_DEFECT_CORR_CTRL         0x9D
#define MT9V112ICP_H_SIZE_CTXB              0xA1
#define MT9V112ICP_V_SIZE_CTXB              0xA4
#define MT9V112ICP_H_PAN                    0xA5
#define MT9V112ICP_H_ZOOM                   0xA6
#define MT9V112ICP_H_SIZE_CTXTA             0xA7
#define MT9V112ICP_V_PAN                    0xA8
#define MT9V112ICP_V_ZOOM                   0xA9
#define MT9V112ICP_V_SIZE_CTXTA             0xAA
#define MT9V112ICP_CUR_HZOOM                0xAB
#define MT9V112ICP_CUR_VZOOM                0xAC
#define MT9V112ICP_ZOOM_STEP_SIZE           0xAE
#define MT9V112ICP_ZOOM_CONTROL             0xAF
#define MT9V112ICP_LENS_V_RED_KNEE_65       0xB6
#define MT9V112ICP_LENS_V_RED_KNEE_87       0xB7
#define MT9V112ICP_LENS_V_GREEN_KNEE_65     0xB8
#define MT9V112ICP_LENS_V_GREEN_KNEE_87     0xB9
#define MT9V112ICP_LENS_V_BLUE_KNEE_65      0xBA
#define MT9V112ICP_LENS_V_BLUE_KNEE_87      0xBB
#define MT9V112ICP_LENS_H_RED_KNEE_76       0xBC
#define MT9V112ICP_LENS_H_RED_KNEE_98       0xBD
#define MT9V112ICP_LENS_H_RED_KNEE_10       0xBE
#define MT9V112ICP_LENS_H_GREEN_KNEE_76     0xBF
#define MT9V112ICP_LENS_H_GREEN_KNEE_98     0xC0
#define MT9V112ICP_LENS_H_GREEN_KNEE_10     0xC1
#define MT9V112ICP_LENS_H_BLUE_KNEE_76      0xC2
#define MT9V112ICP_LENS_H_BLUE_KNEE_98      0xC3
#define MT9V112ICP_LENS_H_BLUE_KNEE_10      0xC4
#define MT9V112ICP_GLOBAL_CONTEXT_CONTROL   0xC8
#define MT9V112ICP_GAMMAB_KNEE_Y2Y1_CTXB    0xDC
#define MT9V112ICP_GAMMAB_KNEE_Y4Y3_CTXB    0xDD
#define MT9V112ICP_GAMMAB_KNEE_Y5Y6_CTXB    0xDE
#define MT9V112ICP_GAMMAB_KNEE_Y7Y8_CTXB    0xDF
#define MT9V112ICP_GAMMAB_KNEE_Y9Y10_CTXB   0xE0
#define MT9V112ICP_GAMMAB_KNEE_Y0_CTXB      0xE1
#define MT9V112ICP_EFFECTS_MODE             0xE2
#define MT9V112ICP_EFFECTS_SEPIA            0xE3
#define MT9V112ICP_REGISTER_SELECT          MT9V112_REGISTER_SELECT

#define MT9V112ICP_SEL_IFP                  0x1
#define MT9V112ICP_SEL_SCA                  0x4
#define MT9V112ICP_FC_RGB_OR_YUV            0x1000

/*!
 * mt9v112 IFP REGISTER BANK MAP Camera control Registers
 */

#define MT9V112ICR_BASE_MATRIX_SIGNS        0x02
#define MT9V112ICR_COLOR_CORR_MTX_SCL_K1K5  0x03
#define MT9V112ICR_COLOR_CORR_MTX_SCL_K6K9  0x04
#define MT9V112ICR_COLOR_CORR_K1            0x09
#define MT9V112ICR_COLOR_CORR_K2            0x0A
#define MT9V112ICR_COLOR_CORR_K3            0x0B
#define MT9V112ICR_COLOR_CORR_K4            0x0C
#define MT9V112ICR_COLOR_CORR_K5            0x0D
#define MT9V112ICR_COLOR_CORR_K6            0x0E
#define MT9V112ICR_COLOR_CORR_K7            0x0F
#define MT9V112ICR_COLOR_CORR_K8            0x10
#define MT9V112ICR_COLOR_CORR_K9            0x11
#define MT9V112ICR_COLOR_CORR_POS           0x12
#define MT9V112ICR_CURRENT_AWB_RED          0x13
#define MT9V112ICR_CURRENT_AWB_BLUE         0x14
#define MT9V112ICR_DELTA_MTX_SIGNS          0x15
#define MT9V112ICR_DELTA_COLOR_CORR_D1      0x16
#define MT9V112ICR_DELTA_COLOR_CORR_D2      0x17
#define MT9V112ICR_DELTA_COLOR_CORR_D3      0x18
#define MT9V112ICR_DELTA_COLOR_CORR_D4      0x19
#define MT9V112ICR_DELTA_COLOR_CORR_D5      0x1A
#define MT9V112ICR_DELTA_COLOR_CORR_D6      0x1B
#define MT9V112ICR_DELTA_COLOR_CORR_D7      0x1C
#define MT9V112ICR_DELTA_COLOR_CORR_D8      0x1D
#define MT9V112ICR_DELTA_COLOR_CORR_D9      0x1E
#define MT9V112ICR_CHROMA_LIMITS            0x1F
#define MT9V112ICR_AWB_LUMA_LIMITS          0x20
#define MT9V112ICR_MWB_RED_BLUE_GAINS       0x21
#define MT9V112ICR_RED_GAIN_AWB_LIMIT       0x22
#define MT9V112ICR_BLUE_GAIN_AWB_LIMIT      0x23
#define MT9V112ICR_CCM_AWB_LIMIT            0x24
#define MT9V112ICR_H_BOUND_AE               0x26
#define MT9V112ICR_V_BOUND_AE               0x27
#define MT9V112ICR_AWB_ADVANCED_CTRL        0x28
#define MT9V112ICR_WB_GAIN_WIDE_STABL_GATE  0x29
#define MT9V112ICR_WB_ZONE_VALIDITY_LIMITS  0x2A
#define MT9V112ICR_H_BOUND_AE_CEN_WIN       0x2B
#define MT9V112ICR_V_BOUND_AE_CEN_WIN       0x2C
#define MT9V112ICR_BOUND_AWB_WIN            0x2D
#define MT9V112ICR_AE_PRECISION_TARGET      0x2E
#define MT9V112ICR_AE_SPEED_CTXTA           0x2F
#define MT9V112ICR_AWB_RED_MEASUREMENT      0x30
#define MT9V112ICR_AWB_LUM_MEASUREMENT      0x31
#define MT9V112ICR_AWB_BLUE_MEASUREMENT     0x32
#define MT9V112ICR_AE_SHARP_SAT_CTRL        0x33
#define MT9V112ICR_AE_GAIN_LIMITS           0x36
#define MT9V112ICR_AE_GAIN_ZONE_LIMITS      0x37
#define MT9V112ICR_AE_GAIN_ZONE_RANGE_CTRL  0x38
#define MT9V112ICR_AE_LINE_SIZE_CTXA        0x39
#define MT9V112ICR_AE_LINE_SIZE_CTXB        0x3A
#define MT9V112ICR_AE_SHUTT_DELAY_LIM_CTXA  0x3B
#define MT9V112ICR_AE_SHUTT_DELAY_LIM_CTXB  0x3C
#define MT9V112ICR_AE_ADC_GAIN_CTRL         0x3D
#define MT9V112ICR_CCM_GAIN_THRESHOLD       0x3E
#define MT9V112ICR_AE_CURR_GAIN_ZONE        0x3F
#define MT9V112ICR_AE_LUM_THRESHOLD         0x46
#define MT9V112ICR_AE_CURR_LUM_MONITOR      0x4C
#define MT9V112ICR_AE_TIME_AVG_LUM_MONITOR  0x4D
#define MT9V112ICR_AE_SHUTT_60HZ_WIDTH_CTXA 0x57
#define MT9V112ICR_AE_SHUTT_50HZ_WIDTH_CTXA 0x58
#define MT9V112ICR_AE_SHUTT_60HZ_WIDTH_CTXB 0x59
#define MT9V112ICR_AE_SHUTT_50HZ_WIDTH_CTXB 0x5A
#define MT9V112ICR_FLICKER_CONTROL          0x5B
#define MT9V112ICR_FLICKER_60HZ             0x5C
#define MT9V112ICR_FLICKER_50HZ             0x5D
#define MT9V112ICR_BASE_CORE_GAIN_RATIOS    0x5E
#define MT9V112ICR_DELTA_CORE_GAIN_RATIOS   0x5F
#define MT9V112ICR_DELTA_CORE_GAIN_RATIOS_SIGNS 0x60
#define MT9V112ICR_GAIN_RATIOS_MONITOR      0x61
#define MT9V112ICR_AE_GAIN                  0x62
#define MT9V112ICR_AE_LUMA_OFFSET           0x65
#define MT9V112ICR_MAX_GAIN_AE              0x67
#define MT9V112ICR_AE_GAIN_ZONE1_DELTA      0x82
#define MT9V112ICR_AE_GAIN_ZONE2_DELTA      0x83
#define MT9V112ICR_AE_GAIN_ZONE3_DELTA      0x84
#define MT9V112ICR_AE_GAIN_ZONE4_DELTA      0x85
#define MT9V112ICR_AE_GAIN_ZONE5_DELTA      0x86
#define MT9V112ICR_AE_GAIN_ZONE6_DELTA      0x87
#define MT9V112ICR_AE_GAIN_ZONE7_DELTA      0x88
#define MT9V112ICR_AE_GAIN_ZONE8_DELTA      0x89
#define MT9V112ICR_AE_GAIN_ZONE9_DELTA      0x8A
#define MT9V112ICR_AE_GAIN_ZONE10_DELTA     0x8B
#define MT9V112ICR_AE_GAIN_ZONE11_DELTA     0x8C
#define MT9V112ICR_AE_GAIN_ZONE12_DELTA     0x8D
#define MT9V112ICR_AE_GAIN_ZONE13_DELTA     0x8E
#define MT9V112ICR_AE_GAIN_ZONE14_DELTA     0x8F
#define MT9V112ICR_AE_GAIN_ZONE15_DELTA     0x90
#define MT9V112ICR_AE_GAIN_ZONE16_17_DELTA  0x91
#define MT9V112ICR_AE_GAIN_ZONE18_19_DELTA  0x92
#define MT9V112ICR_AE_GAIN_ZONE20_21_DELTA  0x93
#define MT9V112ICR_AE_GAIN_ZONE22_23_DELTA  0x94
#define MT9V112ICR_AE_GAIN_ZONE24_DELTA     0x95
#define MT9V112ICR_AE_SPEED_CTXTB           0x9C
#define MT9V112ICR_GLOBAL_CONTEXT_CONTROL   0xC8
#define MT9V112ICR_CAM_CTRL_CTX_CTRL        0xC9
#define MT9V112ICR_CTX_CTRL_PRG_STATUS      0xCA
#define MT9V112ICR_CTX_CTRL_PRG_ADVANCE     0xCB
#define MT9V112ICR_CTX_CTRL_PRG_SELECT      0xCC
#define MT9V112ICR_CTX_CTRL_SNAPSHOT_PRG_CF 0xCD
#define MT9V112ICR_CTX_CTRL_LED_PRG_CF      0xCE
#define MT9V112ICR_CTX_CTRL_LED_DELTA_LUM   0xCF
#define MT9V112ICR_CTX_CTRL_XENON_PRG_CF    0xD0
#define MT9V112ICR_CTX_CTRL_VIDEO_PRG_CF    0xD1
#define MT9V112ICR_CTX_CTRL_DEFAULT_CF      0xD2
#define MT9V112ICR_CTX_CTRL_USER_GLOBAL_CTX 0xD3
#define MT9V112ICR_CTX_CTRL_XENON_AE        0xD4
#define MT9V112ICR_CTX_CTRL_CAM_CTRL        0xD5
#define MT9V112ICR_AWB_FLASH_ADV_CTRL       0xEF
#define MT9V112ICR_REGISTER_SELECT          MT9V112_REGISTER_SELECT
#define MT9V112ICR_AWB_RED_BLUE_GAIN_OFFSET 0xF2
#define MT9V112ICR_MWB_POS                  0xF5
#define MT9V112ICR_FLASH_WB_POS             0xF6
#define MT9V112ICR_FLASH_SENSOR_CORE_GAINS  0xFF

#define MT9V112ICR_SEL_IFP                0x2
#define MT9V112ICR_SEL_SCA                  0x4
#define MT9V112ICR_FC_RGB_OR_YUV            0x1000


/*!
 * Mt9v112 SENSOR CORE REGISTER BANK MAP
 */
#define MT9V112S_CHIP_VERSION             0x0
#define MT9V112S_ROW_START                0x1
#define MT9V112S_COLUMN_START             0x2
#define MT9V112S_ROW_WIDTH                0x3
#define MT9V112S_COL_WIDTH                0x4
#define MT9V112S_HOR_BLANKING_CTXTB       0x5
#define MT9V112S_VER_BLANKING_CTXTB       0x6
#define MT9V112S_HOR_BLANKING_CTXTA       0x7
#define MT9V112S_VER_BLANKING_CTXTA       0x8
#define MT9V112S_SHUTTER_WIDTH            0x9
#define MT9V112S_ROW_SPEED                0xA
#define MT9V112S_EXTRA_DELAY              0xB
#define MT9V112S_SHUTTER_DELAY            0xC
#define MT9V112S_RESET                    0xD
#define MT9V112S_READ_MODE_CTXTB          0x20
#define MT9V112S_READ_MODE_CTXTA          0x21
#define MT9V112S_SHOW DARK_COL_AND_ROW    0x22
#define MT9V112S_FLASH_CONTROL            0x23
#define MT9V112S_EXTRA_RESET              0x24
#define MT9V112S_GREEN1_GAIN              0x2B
#define MT9V112S_BLUE_GAIN                0x2C
#define MT9V112S_RED_GAIN                 0x2D
#define MT9V112S_GREEN2_GAIN              0x2E
#define MT9V112S_GLOBAL_GAIN              0x2F
#define MT9V112S_ROW_NOISE                0x30
#define MT9V112S_VREF_DACS                0x41
#define MT9V112S_BLACK_ROWS               0x59
#define MT9V112S_DARK_GREEN1_FRAME_AVG    0x5B
#define MT9V112S_DARK_BLUE_FRAME_AVG      0x5C
#define MT9V112S_DARK_RED_FRAME_AVG       0x5D
#define MT9V112S_DARK_GREEN2_FRAME_AVG    0x5E
#define MT9V112S_BLACK_LEVEL_CAL_TH       0x5F
#define MT9V112S_BLACK_LEVEL_CAL_CTRL     0x60
#define MT9V112S_GREEN1_OFFSET_CAL_VAL    0x61
#define MT9V112S_BLUE_OFFSET_CAL_VAL      0x62
#define MT9V112S_RED_OFFSET_CAL_VAL       0x63
#define MT9V112S_GREEN2_OFFSET_CAL_VAL    0x64
#define MT9V112S_CONTEXT_CONTROL          0xC8
#define MT9V112S_REGISTER_SELECT          MT9V112_REGISTER_SELECT
#define MT9V112S_BYTEWISE_ADDRESS         0xF1
#define MT9V112S_CHIP_VERSION2            0xFF
#define MT9V112S_SEL              0x0

// OUTPUT_CTRL
#define MT9V112S_OUTCTRL_SYNC             0x8000
#define MT9V112S_OUTCTRL_CHIP_ENABLE      0x0008
#define MT9V112S_OUTCTRL_TEST_MODE        0x40

// READ_MODE
#define MT9V112S_RM_NOBADFRAME            0x1
#define MT9V112S_RM_NODESTRUCT            0x2
#define MT9V112S_RM_COLUMNSKIP            0x4
#define MT9V112S_RM_ROWSKIP               0x8
#define MT9V112S_RM_BOOSTEDRESET          0x1000
#define MT9V112S_RM_COLUMN_LATE           0x10
#define MT9V112S_RM_ROW_LATE              0x80
#define MT9V112S_RM_RIGTH_TO_LEFT         0x4000
#define MT9V112S_RM_BOTTOM_TO_TOP         0x8000

/*! I2C Slave Address */

// !! Slave address of MT9V112 is 90 but here, we only consider the 7 fist bits
#define MT9V112_I2C_ADDRESS               0x48

/*!
 * The image resolution enum for the mt9v112 sensor
 */
/*typedef enum {
	MT9V112_OutputResolution_VGA = 0,	//VGA size
	MT9V112_OutputResolution_QVGA,	// QVGA size 
	MT9V112_OutputResolution_CIF,	// CIF size 
	MT9V112_OutputResolution_QCIF,	// QCIF size 
	MT9V112_OutputResolution_QQVGA,	// QQVGA size 
	MT9V112_OutputResolution_SXGA	// SXGA size 
} MT9V112_OutputResolution;*/


// define format constants
#define MT9V112_WINWIDTH_MAX      0x288
#define MT9V112_WINWIDTH          0x282
#define MT9V112_WINWIDTH_DEFAULT  0x280
#define MT9V112_WINHEIGHT_MAX     0x1E8
#define MT9V112_WINHEIGHT         0x1E2
#define MT9V112_WINHEIGHT_DEFAULT 0x1E0

// define blanking constants
#define MT9V112_HORZBLANK         0x84
#define MT9V112_HORZBLANK_DEFAULT 0xCB
#define MT9V112_HORZBLANK_MIN     0x84
#define MT9V112_VERTBLANK_DEFAULT 0xB
#define MT9V112_VERTBLANK_MIN     0x1

// define default frame rate
#define MT9V112_DEFAULT_FRAMERATE 30

// define operating mode control
#define OMC_ENABLE_MANUAL_WB          0x8000
#define OMC_ENABLE_AE                 0x4000
#define OMC_ENABLE_DEFECT_CORR        0x2000
#define OMC_ENABLE_LENS_SHADING_CORR  0x0400
#define OMC_ENABLE_FLICKER_DETECTION  0x0080
#define OMC_BYPASS_COLOR_CORR         0x0010
#define OMC_ENABLE_AWB                0x0002
// backlight comepensation parameters
typedef enum {
OMC_AE_LARGE_WINDOW           = 0x0000,
OMC_AE_SMALL_WINDOW           = 0x0004, 
OMC_AE_AVG_WINDOW             = 0x000C
} e_OMC_BackLightComp;

// define flicker mode
#define FM_ENABLE_AUTO_FLICKER       0x0000
#define FM_ENABLE_FLICKER_50HZ       0x0001
#define FM_ENABLE_FLICKER_60HZ       0x0003

// define color saturation control
typedef enum
{
CSC_FULL_COLOR_SAT            = 0x0000,
CSC_75_100_COLOR_SAT          = 0x0008,
CSC_50_100_COLOR_SAT          = 0x0010,
CSC_37_100_COLOR_SAT          = 0x0018,
CSC_25_100_COLOR_SAT          = 0x0020,
CSC_150_100_COLOR_SAT         = 0x0028,
CSC_BLACK_AND_WHITE           = 0x0030
} e_CSC_settings;
typedef enum
{
CSC_NO_ATTENUATION            = 0x0000,
CSC_ATTENUATION_216           = 0x0001,
CSC_ATTENUATION_208           = 0x0002,
CSC_ATTENUATION_192           = 0x0003,
CSC_ATTENUATION_160           = 0x0004,
CSC_ATTENUATION_96            = 0x0005
} e_CSC_attenutation; 

// define aperture correction
#define AC_AUTO_SHARPEN       0x0008
typedef enum
{
AC_NO_SHARPENING              = 0x0000,
AC_25_100_SHARPEN             = 0x0001,
AC_50_100_SHARPEN             = 0x0002,
AC_75_100_SHARPEN             = 0x0003,
AC_100_100_SHARPEN            = 0x0004,
AC_125_100_SHARPEN            = 0x0005,
AC_150_100_SHARPEN            = 0x0006,
AC_200_100_SHARPEN            = 0x0007
} e_AC_sharpen;

// define effect mode
typedef enum
{
EF_NO_EFFECT                  = 0x0000,
EF_MONOCHROME                 = 0x0001,
EF_SEPIA                      = 0x0002,
EF_NEGATIVE                   = 0x0003,
EF_SOLARIZE1                  = 0x0004,
EF_SOLARIZE2                  = 0x0005
} e_EF_mode;

// define Read Mode
#define RM_XOR_LINE_VALID     0x8000
#define RM_CONT_LINE_VALID    0x4000
#define RM_SHOW_BORDER        0x0700
#define RM_ROW_SKIP           0x0004
#define RM_COL_SKIP           0x0008
#define RM_MIRROR             0x0002

/*!
 * Mt9v112 Core Register structure.
 */
typedef struct {
  u32 addrSpaceSel;        
  u32 chipversion;
  u32 rowStart;		/*!< Starting Row */
  u32 columnStart;	/*!< Starting Column */
  u32 windowHeight;	/*!< Window Height */
  u32 windowWidth;	/*!< Window Width */
  u32 horizontalBlankingCtxtB;	/*!< Horizontal Blank time, in pixels */
  u32 verticalBlankingCtxtB;	/*!< Vertical Blank time, in pixels */
  u32 horizontalBlankingCtxtA;	/*!< Horizontal Blank time, in pixels */
  u32 verticalBlankingCtxtA;	/*!< Vertical Blank time, in pixels */
  //u32 shutterWidth;
  u32 rowSpeed;	/*!< pixel date rate */ 
  u32 extraDelay;		/*!< Abandon the readout of current frame */
  u32 shutterDelay;
  u32 reset;		/*!< reset the sensor to the default mode */
  u32 readModeCtxtB;		/*!< Readmode: aspects of the readout of the sensor */
  u32 readModeCtxtA;		/*!< Readmode: aspects of the readout of the sensor */
  u32 green1Gain;		/*!< Gain Settings */
  u32 blueGain;
  u32 redGain;
  u32 green2Gain;
  u32 globalCtxtCtrl;
  u32 bytewiseAddress;		/*!< Image core Registers written by image flow processor */
} mt9v112_coreReg;

/*!
 * Mt9v112 IFP Register structure.Camera control Registers
 */
typedef struct {
  u32 addrSpaceSel;
	u32 HBoundAE;		/*!< Horizontal boundaries of AWB measurement window */
	u32 VBoundAE;		/*!< Vertical boundaries of AWB measurement window */
	u32 HBoundAECenWin;	/*!< Horizontal boundaries of AE measurement window for backlight compensation */
	u32 VBoundAECenWin;	/*!< Vertical boundaries of AE measurement window for backlight compensation */
	u32 boundAwbWin;	/*!< Boundaries of AWB measurement window */
	u32 AEPrecisionTarget;	/*!< Auto exposure target and precision control */
	u32 AESpeedCtxtA;		/*!< AE speed and sensitivity control register */
	u32 flickerControl;
	u32 aeGain;
	u32 gainLimitAE;
	u32 AESpeedCtxtB;
	u32 globalContextControl;	/*!< Shade Parameters */
	u32 VSize;		/*!< Vertical output size in decimation */
	u32 ShutterWidth;       /*!< Shutter width */
} mt9v112_IFPCRReg;

/*!
 * Mt9v112 IFP Register structure.Colorpipe Registers
 */
typedef struct {
  u32 addrSpaceSel;
  u32 apertureGain;       /*!< sharpening */
  u32 modeControl;        /*!< bit 7 CCIR656 sync codes are embedded in the image */
  u32 outputFormatControl;
  u32 outputFormatControl2CtxtA;      /*!< bit12 1 for RGB565, 0 for YcrCb */
  u32 colorSatControl;
  u32 lumaOffset;         /*!< Luminance offset control (brightness control) */
  u32 clipLimitOutputLumi;        /*!< Clipping limits for output luminance */
  u32 lineCounter;        /*!< Line counter */
  u32 frameCounter;       /*!< Frame counter */
  u32 outputFormatControl2CtxtB;
  u32 HPan;               /*!< Horizontal pan in decimation */
  u32 HZoom;              /*!< Horizontal zoom in decimation */
  u32 HSizeCtxtA;
  u32 HSizeCtxtB;
  u32 VSizeCtxtB;              /*!< Horizontal output size iIn decimation */
  u32 VPan;               /*!< Vertical pan in decimation */
  u32 VZoom;              /*!< Vertical zoom in decimation */
  u32 VSizeCtxtA;              /*!< Vertical output size in decimation */
  u32 ZoomstepSize;
  u32 ZoomControl;
  u32 GlobalContextControl;
  u32 EffectsMode;
  u32 EffectsSepia;
  u32 ResgisterSelect;
  u32 TestPattern;        /*! < Test pattern register */
} mt9v112_IFPCPReg;

/*!
 * mt9v112 Config structure
 */
typedef struct {
	mt9v112_coreReg *coreReg;	/*!< Sensor Core Register Bank */
	mt9v112_IFPCPReg *ifpCPReg;	/*!< IFP Register Bank Colorpipe */
        mt9v112_IFPCRReg *ifpCRReg;     /*!< IFP Register Bank Control Register */
	u32 frame_rate;                 /*!< Current frame rate */
        u32 max_pan_width;              /*!< Max pan width in pixels */
        u32 max_pan_height;             /*!< Max pan height in pixels */
	u32 output_width;               /*!< Current output width in pixels */
        u32 output_height;              /*!< Current output height in pixels */
} mt9v112_conf;

/*typedef struct {
	u8 index;
	u16 width;
	u16 height;
} mt9v112_image_format;*/



#endif				// MT9V112_H_
