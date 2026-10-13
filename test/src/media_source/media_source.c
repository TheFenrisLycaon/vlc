/*****************************************************************************
 * test/src/media_source.c
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "../../libheimdall/test.h"
#include "../lib/libheimdall_internal.h"

#include <assert.h>
#include <heimdall_common.h>
#include <heimdall_media_source.h>
#include <heimdall_vector.h>
#include <heimdall/heimdall.h>

static const char *libheimdall_argv[] = {
    "-v",
    "--ignore-config",
    "-Idummy",
    "--no-media-library",
};

static void
test_list(void)
{
    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(libheimdall_argv), libheimdall_argv);
    assert(heimdall);

    heimdall_media_source_provider_t *provider =
            heimdall_media_source_provider_Get(heimdall->p_libheimdall_int);
    assert(provider);

    heimdall_media_source_meta_list_t *list =
            heimdall_media_source_provider_List(provider, 0);
    assert(list);

    size_t count = heimdall_media_source_meta_list_Count(list);
    assert(count);
    for (size_t i = 0; i < count; ++i)
    {
        struct heimdall_media_source_meta *meta =
                heimdall_media_source_meta_list_Get(list, i);
        assert(meta);
        assert(meta->name);
        assert(meta->longname);
        assert(meta->category); /* there is no category 0 */
    }

    heimdall_media_source_meta_list_Delete(list);
    libheimdall_release(heimdall);
}

static void
test_list_filtered_by_category(void)
{
    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(libheimdall_argv), libheimdall_argv);
    assert(heimdall);

    heimdall_media_source_provider_t *provider =
            heimdall_media_source_provider_Get(heimdall->p_libheimdall_int);
    assert(provider);

    heimdall_media_source_meta_list_t *list =
            heimdall_media_source_provider_List(provider, SD_CAT_LAN);
    assert(list);

    size_t count = heimdall_media_source_meta_list_Count(list);
    assert(count);
    for (size_t i = 0; i < count; ++i)
    {
        struct heimdall_media_source_meta *meta =
                heimdall_media_source_meta_list_Get(list, i);
        assert(meta);
        assert(meta->name);
        assert(meta->longname);
        assert(meta->category == SD_CAT_LAN);
    }

    heimdall_media_source_meta_list_Delete(list);
    libheimdall_release(heimdall);
}

int main(void)
{
    test_init();

    test_list();
    test_list_filtered_by_category();
    return 0;
}
