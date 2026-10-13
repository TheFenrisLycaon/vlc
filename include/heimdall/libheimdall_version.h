/*****************************************************************************
 * libheimdall_version.h
 *****************************************************************************
 * Copyright (C) 2010 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/**
 * \file
 * This file defines version macros for LibHeimdall.
 * Those macros are primilarly intended for conditional (pre)compilation.
 * To get the run-time LibHeimdall version, use libheimdall_get_version() instead
 * (the run-time version may be more recent than build-time one, thanks to
 * backward binary compatibility).
 *
 * \version This header file is available in LibHeimdall 1.1.4 and higher.
 */

#ifndef LIBHEIMDALL_VERSION_H
# define LIBHEIMDALL_VERSION_H 1

/** LibHeimdall major version number */
# define LIBHEIMDALL_VERSION_MAJOR    (4)

/** LibHeimdall minor version number */
# define LIBHEIMDALL_VERSION_MINOR    (0)

/** LibHeimdall revision */
# define LIBHEIMDALL_VERSION_REVISION (0)

# define LIBHEIMDALL_VERSION_EXTRA    (0)

/** Makes a single integer from a LibHeimdall version numbers */
# define LIBHEIMDALL_VERSION(maj,min,rev,extra) \
         ((maj << 24) | (min << 16) | (rev << 8) | (extra))

/** LibHeimdall full version as a single integer (for comparison) */
# define LIBHEIMDALL_VERSION_INT \
         LIBHEIMDALL_VERSION(LIBHEIMDALL_VERSION_MAJOR, LIBHEIMDALL_VERSION_MINOR, \
                        LIBHEIMDALL_VERSION_REVISION, LIBHEIMDALL_VERSION_EXTRA)


/** LibHeimdall ABI major version number, updated when incompatible changes are added */
# define LIBHEIMDALL_ABI_VERSION_MAJOR  (12)

/** LibHeimdall ABI minor version number, updated when compatible changes are added */
# define LIBHEIMDALL_ABI_VERSION_MINOR  (0)

/** LibHeimdall ABI micro version number, updated with new releases */
# define LIBHEIMDALL_ABI_VERSION_MICRO  (0)

/** LibHeimdall full ABI version combining the major Heimdall version and the .so version:
 * - A 0xFF000000 mask gives the Heimdall major version,
 * - A 0x00FF0000 mask gives the LibHeimdall major ABI version,
 * - A 0x0000FF00 mask gives the LibHeimdall minor ABI version,
 * - A 0x000000FF mask gives the LibHeimdall ABI revision.
 *
 * LibHeimdall is considered compatible with your code if the Heimdall major and LibHeimdall
 * major values are equal and the minor ABI version is equal or higher than the
 * value you compiled with.
 */
# define LIBHEIMDALL_ABI_VERSION_INT \
         LIBHEIMDALL_VERSION(LIBHEIMDALL_VERSION_MAJOR, LIBHEIMDALL_ABI_VERSION_MAJOR, \
                        LIBHEIMDALL_ABI_VERSION_MINOR, LIBHEIMDALL_ABI_VERSION_MICRO )

#endif
