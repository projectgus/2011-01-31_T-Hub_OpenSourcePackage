/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file fsl_platform.h
 *
 * Header file to isolate code which might be platform-dependent
 *
 * @ingroup MXCSAHARA2
 */

#ifndef FSL_PLATFORM_H
#define FSL_PLATFORM_H

#ifdef __KERNEL__
#include "portable_os.h"
#endif

#if defined(FSL_PLATFORM_OTHER)

/* Makefile or other method of setting FSL_HAVE_* flags */

#elif defined(CONFIG_ARCH_MX27)

#define FSL_HAVE_SAHARA2
#define FSL_HAVE_RTIC2
#define FSL_HAVE_SCC

#elif defined(CONFIG_ARCH_MX3)	/* i.MX31 */

#define FSL_HAVE_SCC
#define FSL_HAVE_RTIC2		/* ? */
#define FSL_HAVE_RNGA

#else

#error UNKNOWN_PLATFORM

#endif				/* platform checks */

#endif				/* FSL_PLATFORM_H */
