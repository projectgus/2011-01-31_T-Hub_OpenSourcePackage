/*
 * Copyright (C) 2006-2009, Sagem Communications. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file progessbar.c 
 * @brief Progress bar drawing over logo.
 *
 *  Created 2006 by Vincent Galceran <vincent.galceran@sagem.com>
 *  Updated 2006-2009 Cedric Le Dillau <cedric.ledillau@sagem.com> to 
 *       make it configurable.
 * 
 * @defgroup PROGRESSBAR
 * @ingroup  PROGRESSBAR
 */

#ifndef __LINUX_PROGRESSBAR_H
#define __LINUX_PROGRESSBAR_H

extern void progressbar_init(void);
extern void progressbar_at(int percent);
extern void progressbar_free(void);

#endif /* __LINUX_PROGRESSBAR_H */
