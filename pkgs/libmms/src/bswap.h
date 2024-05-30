#ifndef BSWAP_H_INCLUDED
#define BSWAP_H_INCLUDED

/*
 * Copyright (C) 2004 Maciej Katafiasz <mathrick@users.sourceforge.net>
 *
 * Modified by Sagemcom under LGPL license on 03/09/2009 
 * Copyright (c) 2010 Sagemcom All rights reserved.
 * As indicated in file README.license, original GPL code was taken from Xine project
 * (http://xine.sf.net). Relicensed to LGPL with explicit approval from
 * all copyright holders.
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


/* Go cheap now, will rip out glib later. *Sigh* */
#include <glib.h>

/* NOTE:
 * Now, to clear up confusion: LE_XX means "from LE to native, XX bits wide"
 * I know it's not very clear naming (tell me about it, I
 * misinterpreted in first version and caused bad nasty bug, *sigh*),
 * but that's inherited code, will clean up as things go
 * Oh, and one more thing -- they take *pointers*, not actual ints
 */

#define LE_16(val) (*(val+0) << 0 | \
					*(val+1) << 8)
#define LE_32(val) (GINT32_FROM_LE (*((u_int32_t*)(val))))
#define LE_64(val) ((u_int64_t)(*(val+0)) << 0 | \
					(u_int64_t)(*(val+1)) << 8 | \
					(u_int64_t)(*(val+2)) << 16 | \
					(u_int64_t)(*(val+3)) << 24 | \
					(u_int64_t)(*(val+3)) << 32 | \
					(u_int64_t)(*(val+3)) << 40 | \
					(u_int64_t)(*(val+3)) << 48 | \
					(u_int64_t)(*(val+3)) << 56)

#define BE_16(val) (GINT16_FROM_BE (*((u_int16_t*)(val))))
#define BE_32(val) (GINT32_FROM_BE (*((u_int32_t*)(val))))
#define BE_64(val) (GINT64_FROM_BE (*((u_int64_t*)(val))))

#endif /* BSWAP_H_INCLUDED */
