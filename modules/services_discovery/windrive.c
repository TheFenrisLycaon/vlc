/**
 * @file win_disc.c
 * @brief List of disc drives for Heimdall media player for Windows
 */
/*****************************************************************************
 * Copyright © 2010 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <heimdall_common.h>
#include <heimdall_services_discovery.h>
#include <heimdall_plugin.h>

#include <windows.h>

static int Open (heimdall_object_t *);

HEIMDALL_SD_PROBE_HELPER("disc", N_("Discs"), SD_CAT_DEVICES)

/*
 * Module descriptor
 */
heimdall_module_begin ()
    set_shortname (N_("Discs"))
    set_description (N_("Discs"))
    set_subcategory (SUBCAT_PLAYLIST_SD)
    set_capability ("services_discovery", 0)
    set_callback(Open)
    add_shortcut ("disc")

    HEIMDALL_SD_PROBE_SUBMODULE

heimdall_module_end ()

/**
 * Probes and initializes.
 */
static int Open (heimdall_object_t *obj)
{
    services_discovery_t *sd = (services_discovery_t *)obj;

    sd->description = _("Discs");

    LONG drives = GetLogicalDrives ();
    char mrl[12] = "file:///A:/", name[3] = "A:";
    CHAR path[4] = "A:\\";

    for (char d = 0; d < 26; d++)
    {
        input_item_t *item;
        CHAR letter = 'A' + d;

        /* Does this drive actually exist? */
        if (!(drives & (1 << d)))
            continue;
        /* Is it a disc drive? */
        path[0] = letter;
        if (GetDriveTypeA (path) != DRIVE_CDROM)
            continue;

        mrl[8] = name[0] = letter;
        item = input_item_NewDisc (mrl, name, INPUT_DURATION_INDEFINITE);
        msg_Dbg (sd, "adding %s (%s)", mrl, name);
        if (item == NULL)
            break;

        services_discovery_AddItem(sd, item);
    }
    return HEIMDALL_SUCCESS;
}
