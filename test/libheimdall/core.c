/*
 * core.c - libheimdall smoke test
 *
 */

/**********************************************************************
 *  Copyright (C) 2007 Rémi Denis-Courmont.                           *
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

#include "test.h"

#include <string.h>

static void test_core (const char ** argv, int argc)
{
    libheimdall_instance_t *heimdall;

    test_log ("Testing core\n");

    heimdall = libheimdall_new (argc, argv);
    assert (heimdall != NULL);

    libheimdall_retain (heimdall);
    libheimdall_release (heimdall);
    libheimdall_release (heimdall);
}

static void test_moduledescriptionlist (libheimdall_module_description_t *list)
{
    libheimdall_module_description_t *module = list;
    while ( module ) {
        assert (strlen (module->psz_name) );
        assert (strlen (module->psz_shortname) );
        assert (module->psz_longname == NULL || strlen (module->psz_longname));
        assert (module->psz_help == NULL || strlen (module->psz_help));
        assert (module->psz_help_html == NULL || strlen (module->psz_help_html));
        module = module->p_next;
    }    

    libheimdall_module_description_list_release (list);
}

static void test_audiovideofilterlists (const char ** argv, int argc)
{
    libheimdall_instance_t *heimdall;

    test_log ("Testing libheimdall_(audio|video)_filter_list_get()\n");

    heimdall = libheimdall_new (argc, argv);
    assert (heimdall != NULL);

    test_moduledescriptionlist (libheimdall_audio_filter_list_get (heimdall));
    test_moduledescriptionlist (libheimdall_video_filter_list_get (heimdall));

    libheimdall_release (heimdall);
}

static void test_audio_output (void)
{
    libheimdall_instance_t *heimdall = libheimdall_new (0, NULL);
    assert (heimdall != NULL);

    libheimdall_audio_output_t *mods = libheimdall_audio_output_list_get (heimdall);
    assert (mods != NULL);

    puts ("Audio outputs:");
    for (const libheimdall_audio_output_t *o = mods; o != NULL; o = o->p_next)
        printf(" %s: %s\n", o->psz_name, o->psz_description);
    libheimdall_audio_output_list_release (mods);
    libheimdall_release (heimdall);
}

int main (void)
{
    test_init();

    test_core (test_defaults_args, test_defaults_nargs);
    test_audiovideofilterlists (test_defaults_args, test_defaults_nargs);
    test_audio_output ();

    return 0;
}
