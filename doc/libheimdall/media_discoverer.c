/*****************************************************************************
 * media_discoverer.c:  libheimdall media discoverer API sample usage
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#include <stdio.h>
#include <stdlib.h>

#include <heimdall/heimdall.h>

static const char *
category_to_string(libheimdall_media_discoverer_category_t cat)
{
    switch (cat)
    {
        case libheimdall_media_discoverer_devices:   return "devices";
        case libheimdall_media_discoverer_lan:       return "lan";
        case libheimdall_media_discoverer_podcasts:  return "podcasts";
        case libheimdall_media_discoverer_localdirs: return "localdirs";
        default: abort(); /* assert/unreachable */
    }
}

static void
print_usage(const char *name, libheimdall_instance_t *libheimdall)
{
    /* Print usage and list all available media discoverer plugins */
    fprintf(stderr, "Usage: %s <discoverer_name>\n\n"
            "List of discoverers by category:\n", name);

    static const libheimdall_media_discoverer_category_t cats[] = {
        libheimdall_media_discoverer_devices,
        libheimdall_media_discoverer_lan,
        libheimdall_media_discoverer_podcasts,
        libheimdall_media_discoverer_localdirs
    };

    for (size_t i = 0; i < sizeof(cats) / sizeof(cats[0]); ++i)
    {
        libheimdall_media_discoverer_category_t cat = cats[i];
        libheimdall_media_discoverer_description_t **desclist;

        fprintf(stderr, "[%s]:\n", category_to_string(cat));
        size_t count = libheimdall_media_discoverer_list_get(libheimdall, cat, &desclist);
        for (size_t j = 0; j < count; j++)
        {
            libheimdall_media_discoverer_description_t *desc = desclist[j];
            fprintf(stderr, "\t\"%s\" (%s)\n", desc->psz_name, desc->psz_longname);
        }

        libheimdall_media_discoverer_list_release(desclist, count);
    }
}

static void
on_media_added(void *opaque, libheimdall_media_t *parent, libheimdall_media_t *media)
{
    (void) opaque;
    char *parentname = parent == NULL ? NULL
                     : libheimdall_media_get_meta(parent, libheimdall_meta_Title);
    char *mrl = libheimdall_media_get_mrl(media);
    char *name = libheimdall_media_get_meta(media, libheimdall_meta_Title);
    printf("media_added: \"%s\"\n  parent: \"%s\"\n  url: \"%s\"\n\n",
           name != NULL ? name : "<nil>",
           parentname != NULL ? parentname : "<nil>",
           mrl != NULL ? mrl : "<nil>");
    free(mrl);
    free(name);
    free(parentname);
}

static void
on_media_removed(void *opaque, libheimdall_media_t *media)
{
    (void) opaque;
    char *mrl = libheimdall_media_get_mrl(media);
    char *name = libheimdall_media_get_meta(media, libheimdall_meta_Title);
    printf("media_removed: \"%s\"\n  url: \"%s\"\n\n",
           name != NULL ? name : "<nil>",
           mrl != NULL ? mrl : "<nil>");
    free(mrl);
    free(name);
}

int
main(int argc, const char **argv)
{
    static const char* const args[] = {
        "--verbose=1",
    };
    libheimdall_instance_t *libheimdall = libheimdall_new(sizeof args / sizeof *args, args);
    if (libheimdall == NULL)
        return EXIT_FAILURE;

    if (argc < 2)
    {
        print_usage(argv[0], libheimdall);
        libheimdall_release(libheimdall);
        return EXIT_FAILURE;
    }

    /* Setup discoverer callbacks */
    static const struct libheimdall_media_discoverer_cbs cbs = {
        .version = 0,
        .on_media_added = on_media_added,
        .on_media_removed = on_media_removed,
    };

    libheimdall_media_discoverer_t *discoverer =
        libheimdall_media_discoverer_new(libheimdall, argv[1],
                                    &cbs, NULL);

    int ret = discoverer == NULL ? -1 : libheimdall_media_discoverer_start(discoverer);
    if (ret != 0)
    {
        fprintf(stderr, "Error: unknown discoverer: \"%s\"\n", argv[1]);
        print_usage(argv[0], libheimdall);

        if (discoverer != NULL)
            libheimdall_media_discoverer_destroy(discoverer);
        libheimdall_release(libheimdall);
        return EXIT_FAILURE;
    }

    printf("Discoverer \"%s\" running, press ENTER to stop\n", argv[1]);
    getchar();

    libheimdall_media_discoverer_stop(discoverer);

    libheimdall_media_discoverer_destroy(discoverer);
    libheimdall_release(libheimdall);
    return EXIT_SUCCESS;
}
