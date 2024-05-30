/*
 *  Configure audio power amplifier.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: sagem-audio-pwr-amp.h
 *  Creation date: 12/12/2008
 *  Author: Olivier Le Roy, Farid Hammane, Sagemcom
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



#ifndef SND_AUDIO_AMP_H
#define SND_AUDIO_AMP_H




/** Turn audio amplifier off 
 * @param dev IN : prototype defined by sysfs. dev is not used.
 * @param state IN : prototype defined by sysfs. state is not used.
 * @return always 0
 */
int sagem_audio_pwr_amp_suspend(struct platform_device *dev, pm_message_t state);


/** Turn audio amplifier on
 * @param dev IN : prototype defined by sysfs. dev is not used.
 * @return always 0
 */
int sagem_audio_pwr_amp_resume(struct platform_device *pdev);



/** Geting amplifier state 
 * @param void 
 * @return 0 if successfull 
 */
int sagem_audio_pwr_amp_get_state(void);



/** Set headset_state to 0, and turn amplifier on if some devices need it 
 * @param struct platform_device *pdev 
 * @return 
 */
void sagem_audio_pwr_amp_headset_resume(void);


/** Turn audio amplifier off while headset is plugged, and set headset_state to 1. 
 * @param *dev
 * @param state 
 * @return 
 */
void sagem_audio_pwr_amp_headset_suspend(void);

/** Turn audio amplifier off with no condition
 */
void sagem_audio_pwr_amp_unconditional_suspend(void);

/** Disable unconditional suspend and resume
 * if necessary
 */
void sagem_audio_pwr_amp_disable_unconditional_suspend(void);


#endif
