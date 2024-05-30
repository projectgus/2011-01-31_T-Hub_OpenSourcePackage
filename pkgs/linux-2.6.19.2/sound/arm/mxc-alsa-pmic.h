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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Copyright (C) 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Modified by Sagemcom under GPL license on 04/07/2007 
 * Copyright (c) 2010 Sagemcom All rights reserved.
 *
 */

 /*!
  * @file mxc-alsa-pmic.h
  * @brief
  * @ingroup SOUND_DRV
  */

#ifndef __MXC_ALSA_PMIC_H__
#define __MXC_ALSA_PMIC_H__

#ifdef __KERNEL__

 /**/
#define PMIC_MASTER					0x1
#define PMIC_SLAVE					0x2
     /**/
#define DAM_PORT_4					port_4
#define DAM_PORT_5					port_5
     /**/
#define PMIC_STEREODAC					0x1
#define PMIC_CODEC					0x2
     /**/
#define PMIC_AUDIO_ADDER_STEREO				0x1
#define PMIC_AUDIO_ADDER_STEREO_OPPOSITE		0x2
#define PMIC_AUDIO_ADDER_MONO				0x4
#define PMIC_AUDIO_ADDER_MONO_OPPOSITE			0x8
#define TX_WATERMARK					0x4
#define RX_WATERMARK					0x6
     /**/
#define SSI_DEFAULT_FIFO				0x0
#define DEFAULT_MASTER_CLOCK				0x1
     /**/
#define SDMA_TXFIFO_WATERMARK				0x4
#define SDMA_RXFIFO_WATERMARK				0x6
     /**/
#define TIMESLOTS_2					0x3
#define TIMESLOTS_4					0x2
#define SAMPLE_RATE_MAX					0x9
     /**/
#define CLK_SELECT_SLAVE_BCL				0x7
#define CLK_SELECT_SLAVE_CLI				0x5
#define CLK_SELECT_MASTER_CLI				0x4
/* Volume to balance ratio */
#define VOLUME_BALANCE_RATIO				((6 + 39) / (21 + 21))
#define PMIC_BALANCE_MIN				0x0
#define PMIC_BALANCE_MAX				0x7
#define BALANCE_MIN					0x0
#define NO_BALANCE					0x32
#define BALANCE_MAX					0x64

#define PMIC_INPUT_VOLUME_MIN             		0x0
#define PMIC_INPUT_VOLUME_MAX_CAP			0x1f
#define PMIC_INPUT_VOLUME_MAX             		0x1f
#define PMIC_INPUT_VOLUME_DEFAULT                       0xf

#define PMIC_OUTPUT_VOLUME_MIN				0x0
#define PMIC_OUTPUT_VOLUME_MAX_CAP			0xd
#define PMIC_OUTPUT_VOLUME_MAX            		0xa
#define PMIC_OUTPUT_VOLUME_DEFAULT                      0x6

#define PMIC_HEADSET_VOLUME_MAX_CAP                     0xd
#define PMIC_HEADSET_VOLUME_MAX                         0xa

#define INPUT_VOLUME_MIN				0x0
#define INPUT_VOLUME_MAX				0x64
#define OUTPUT_VOLUME_MIN				0x0
#define OUTPUT_VOLUME_MAX				0x64
#define SAMPLE_FREQ_MAX					96000
#define SAMPLE_FREQ_MIN					8000
/*!
 * Define channels available on MC13783. This is mainly used
 * in mixer interface to control different input/output
 * devices
 */
#define MXC_STEREO_OUTPUT				( SOUND_MASK_VOLUME | SOUND_MASK_PCM )
#define MXC_STEREO_INPUT				( SOUND_MASK_PHONEIN )
#define MXC_MONO_OUTPUT					( SOUND_MASK_PHONEOUT | SOUND_MASK_SPEAKER )
#define MXC_MONO_INPUT					( SOUND_MASK_LINE | SOUND_MASK_MIC )
#define SNDCTL_CLK_SET_MASTER                   	_SIOR('Z', 30, int)
#define SNDCTL_PMIC_READ_OUT_BALANCE      		_SIOR('Z', 8, int)
//#define SNDCTL_MC13783_WRITE_OUT_BALANCE                      _SIOWR('Z', 9, int)
#define SNDCTL_PMIC_WRITE_OUT_ADDER			_SIOWR('Z', 10, int)
#define SNDCTL_PMIC_READ_OUT_ADDER			_SIOR('Z', 11, int)
#define SNDCTL_PMIC_WRITE_CODEC_FILTER		_SIOWR('Z', 12, int)
#define SNDCTL_PMIC_READ_CODEC_FILTER		_SIOR('Z', 13, int)
#define SNDCTL_DAM_SET_OUT_PORT				_SIOWR('Z', 14, int)
#endif				/* __KERNEL__ */
#endif				/* __MXC_ALSA_PMIC_H__ */
