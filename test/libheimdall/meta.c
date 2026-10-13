/*
 * meta.c - libheimdall smoke test
 *
 */

/**********************************************************************
 *  Copyright (C) 2007 Rémi Denis-Courmont.                           *
 *  Copyright (C) 2008 Pierre d'Herbemont.                            *
 *  This program is free software; you can redistribute and/or modify *
 *  it under the terms of the GNU General Public License as published *
 *  by the Free Software Foundation; version 2 of the license, or (at *
 *  your option) any later version.                                   *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *  See the GNU General Public License for more details.              *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, you can get it from:             *
 *  http://www.gnu.org/copyleft/gpl.html                              *
 *  Copyright (C) 2026 TheFenrisLycaon
 **********************************************************************/

#include <string.h>

#include "test.h"
#include "media_utils.h"

#include <heimdall_modules.h>

static int test_meta (const char ** argv, int argc)
{
    libheimdall_instance_t *heimdall;
    libheimdall_media_t *media;
    char * artist;

    test_log ("Testing meta\n");

    heimdall = libheimdall_new (argc, argv);
    assert (heimdall != NULL);

    if (!module_exists("taglib"))
        return 77;

    media = libheimdall_media_new_path(SRCDIR "/samples/meta.mp3");
    assert( media );

    libheimdall_media_parse_sync(heimdall, media, libheimdall_media_parse, -1);

    artist = libheimdall_media_get_meta (media, libheimdall_meta_Artist);

    const char *expected_artist = "mike";

    assert (artist);
    test_log ("+ got '%s' as Artist, expecting %s\n", artist, expected_artist);

    int string_compare = strcmp (artist, expected_artist);
    assert (!string_compare);

    free (artist);
    libheimdall_media_release (media);
    libheimdall_release (heimdall);

    return 0;
}


int main (void)
{
    test_init();

    return test_meta (test_defaults_args, test_defaults_nargs);
}
