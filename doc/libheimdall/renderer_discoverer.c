/*****************************************************************************
 * renderer_discoverer.c:  libheimdall renderer discoverer API sample usage
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

static void
print_usage(const char *name, libheimdall_instance_t *libheimdall)
{
    /* Print usage and list all available renderer discoverer plugins */
    fprintf(stderr, "Usage: %s <discoverer_name>\n\n"
            "List of discoverer:\n", name);


    libheimdall_rd_description_t **desclist;

    size_t count = libheimdall_renderer_discoverer_list_get(libheimdall, &desclist);
    for (size_t j = 0; j < count; j++)
    {
        libheimdall_rd_description_t *desc = desclist[j];
        fprintf(stderr, "\t\"%s\" (%s)\n", desc->psz_name, desc->psz_longname);
    }

    libheimdall_renderer_discoverer_list_release(desclist, count);
}

static void
on_item_added(void *opaque, libheimdall_renderer_item_t *item)
{
    (void) opaque;
    printf("item_added: \"%s\"\n  type: \"%s\"\n\n",
           libheimdall_renderer_item_name(item),
           libheimdall_renderer_item_type(item));
}

static void
on_item_removed(void *opaque, libheimdall_renderer_item_t *item)
{
    (void) opaque;
    printf("item_removed: \"%s\"\n\n",
           libheimdall_renderer_item_name(item));
}

int
main(int argc, const char **argv)
{
    static const char* const heimdallargs[] = {
        "--verbose=1",
    };
    libheimdall_instance_t *libheimdall = libheimdall_new(sizeof heimdallargs / sizeof *heimdallargs,
                                           heimdallargs);
    if (libheimdall == NULL)
        return EXIT_FAILURE;

    if (argc < 2)
    {
        print_usage(argv[0], libheimdall);
        libheimdall_release(libheimdall);
        return EXIT_FAILURE;
    }

    /* Setup discoverer callbacks */
    static const struct libheimdall_renderer_discoverer_cbs cbs = {
        .version = 0,
        .on_item_added = on_item_added,
        .on_item_removed = on_item_removed,
    };

    libheimdall_renderer_discoverer_t *discoverer =
        libheimdall_renderer_discoverer_new(libheimdall, argv[1],
                                       &cbs, NULL);

    int ret = discoverer == NULL ? -1 : libheimdall_renderer_discoverer_start(discoverer);
    if (ret != 0)
    {
        fprintf(stderr, "Error: unknown discoverer: \"%s\"\n", argv[1]);
        print_usage(argv[0], libheimdall);

        if (discoverer != NULL)
            libheimdall_renderer_discoverer_destroy(discoverer);
        libheimdall_release(libheimdall);
        return EXIT_FAILURE;
    }

    printf("Discoverer \"%s\" running, press ENTER to stop\n", argv[1]);
    getchar();

    libheimdall_renderer_discoverer_stop(discoverer);

    libheimdall_renderer_discoverer_destroy(discoverer);
    libheimdall_release(libheimdall);
    return EXIT_SUCCESS;
}
