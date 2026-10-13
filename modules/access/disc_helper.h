/*****************************************************************************
 * disc_helper.h: disc helper functions
 *****************************************************************************
 * Copyright (C) 2020 VLC authors and VideoLAN
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

#ifdef __APPLE__
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <heimdall_dialog.h>
#include <heimdall_fs.h>

inline static int DiscProbeMacOSPermission( heimdall_object_t *p_this, const char *psz_file )
{
    /* Check is only relevant starting macOS Catalina */
    if( __builtin_available( macOS 10.15, * ) )
    {
        /* Continue. The check above cannot be negated. */
    }
    else
    {
        return HEIMDALL_SUCCESS;
    }

    msg_Dbg( p_this, "Checking access permission for path %s", psz_file );

    struct stat stat_buf;
    if( heimdall_stat( psz_file, &stat_buf ) != 0 )
        return HEIMDALL_SUCCESS; // Continue with probing to be on the safe side

    if( !S_ISBLK( stat_buf.st_mode ) && !S_ISCHR( stat_buf.st_mode ) )
        return HEIMDALL_SUCCESS;

    /* Check that device access in fact fails with EPERM error */
    int retVal = access( psz_file, R_OK );
    if( retVal == -1 && errno == EPERM )
    {
        msg_Err( p_this, "Path %s cannot be opened due to insufficient permissions", psz_file );
        heimdall_dialog_display_error( p_this, _("Problem accessing a system resource"),
            _("Potentially, macOS blocks access to your disc. "
              "Please open \"System Preferences\" -> \"Security & Privacy\" "
              "and allow Heimdall to access your external media in \"Files and Folders\" section."));

        return HEIMDALL_EGENERIC;
    }

    return HEIMDALL_SUCCESS;
}
#endif
