/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2006 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:clutter-version
 * @short_description: Versioning utility macros
 *
 * Clutter offers a set of macros for checking the version of the library
 * an application was linked to.
 */

#ifndef __CLUTTER_VERSION_H__
#define __CLUTTER_VERSION_H__

/**
 * CLUTTER_MAJOR_VERSION:
 *
 * The major version of the Clutter library (1, if %CLUTTER_VERSION is 1.2.3)
 */
#define CLUTTER_MAJOR_VERSION   (0)

/**
 * CLUTTER_MINOR_VERSION:
 *
 * The minor version of the Clutter library (2, if %CLUTTER_VERSION is 1.2.3)
 */
#define CLUTTER_MINOR_VERSION   (8)

/**
 * CLUTTER_MICRO_VERSION:
 *
 * The micro version of the Clutter library (3, if %CLUTTER_VERSION is 1.2.3)
 */
#define CLUTTER_MICRO_VERSION   (0)

/**
 * CLUTTER_VERSION:
 *
 * The full version of the Clutter library, like 1.2.3
 */
#define CLUTTER_VERSION         0.8.0

/**
 * CLUTTER_VERSION_S:
 *
 * The full version of the Clutter library, in string form (suited for
 * string concatenation)
 */
#define CLUTTER_VERSION_S       "0.8.0"

/**
 * CLUTTER_VERSION_HEX:
 *
 * Numerically encoded version of the Clutter library, like 0x010203
 */
#define CLUTTER_VERSION_HEX     ((CLUTTER_MAJOR_VERSION << 24) | \
                                 (CLUTTER_MINOR_VERSION << 16) | \
                                 (CLUTTER_MICRO_VERSION << 8))

/**
 * CLUTTER_CHECK_VERSION:
 * @major: major version, like 1 in 1.2.3
 * @minor: minor version, like 2 in 1.2.3
 * @micro: micro version, like 3 in 1.2.3
 *
 * Evaluates to %TRUE if the version of the Clutter library is greater
 * than @major, @minor and @micro
 */
#define CLUTTER_CHECK_VERSION(major,minor,micro) \
        (CLUTTER_MAJOR_VERSION > (major) || \
         (CLUTTER_MAJOR_VERSION == (major) && CLUTTER_MINOR_VERSION > (minor)) || \
         (CLUTTER_MAJOR_VERSION == (major) && CLUTTER_MINOR_VERSION == (minor) && CLUTTER_MICRO_VERSION >= (micro)))

/**
 * CLUTTER_FLAVOUR:
 *
 * GL Windowing system used
 *
 * Since: 0.4
 */
#define CLUTTER_FLAVOUR         "glx"

/**
 * CLUTTER_COGL
 *
 * Cogl (internal GL abstraction utility library) backend. Can be "gl" or
 * "gles" currently
 *
 * Since: 0.4
 */
#define CLUTTER_COGL            "gl"

/**
 * CLUTTER_STAGE_TYPE:
 *
 * The default GObject type for the Clutter stage. 
 *
 * Since 0.8
 */
#define CLUTTER_STAGE_TYPE CLUTTER_TYPE_STAGE_GLX

/**
 * CLUTTER_NO_FPU:
 *
 * Set to 1 if Clutter was built without FPU (i.e fixed math), 0 otherwise
 *
 * @Deprecated: 0.6: This macro is no longer defined (identical code is used
 *  regardless the presence of FPU).
 */
#define CLUTTER_NO_FPU          CLUTTER_NO_FPU_MACRO_WAS_REMOVED


#endif /* __CLUTTER_VERSION_H__ */
