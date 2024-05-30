/*!
 ***************************************************************************
 * \file pwrmngt.h
 * \brief summary
 * \ingroup DPM
 *
 * \par Copyright
 \verbatim Copyright 2007 Sagem Communication. All Rights Reserved.
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 \endverbatim
 */

#ifndef __PWRMNGT_H__
#define __PWRMNGT_H__

typedef enum {
	init,
	normal,
	doze,
	stand_by,
	switch_off
} e_pwrmngt_mode;

typedef struct t_pwrmngt_state {
	e_pwrmngt_mode mode;
	unsigned short num;
} t_pwrmngt_state;

int pwrmngt_md_normal_in (unsigned short );
int pwrmngt_md_doze_in (unsigned short );
int pwrmngt_md_standby_in (unsigned short );
int pwrmngt_md_standby_out (unsigned short );
int pwrmngt_md_switchoff_in (unsigned short );

#endif /* __PWRMNGT_CMDS_H__ */

