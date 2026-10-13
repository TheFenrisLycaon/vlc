/*****************************************************************************
 * ancillary.c: test for ancillary
 *****************************************************************************
 * Copyright (C) 2021 the VideoLAN team
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_common.h>
#include <heimdall_frame.h>
#include <heimdall_picture.h>
#include <heimdall_ancillary.h>

#include <assert.h>

static void
ancillary_free(void *data)
{
    assert(strncmp(data, "test", 4) == 0);
    free(data);
}

int main( void )
{
    heimdall_frame_t *frame = heimdall_frame_Alloc(1);
    assert(frame);

    int ret;
    struct heimdall_ancillary *ancillary;

    /* Create and try to attach 3 ancillaries to the frame, only 2 will be
     * attached. */
    for (size_t i = 0; i < 3; ++i)
    {
        char *data;
        ret = asprintf(&data, "test%zu", i);
        assert(ret > 0);

        heimdall_ancillary_id id;
        switch (i)
        {
            case 0:
                /* Check that only one ancillary of the same id is added (the
                 * last one take precedence). */
            case 1: id = HEIMDALL_ANCILLARY_ID('t','s','t','1'); break;
            case 2: id = HEIMDALL_ANCILLARY_ID('t','s','t','2'); break;
            default: heimdall_assert_unreachable();
        }

        ancillary = heimdall_ancillary_CreateWithFreeCb(data, id, ancillary_free);
        assert(ancillary);

        ret = heimdall_frame_AttachAncillary(frame, ancillary);
        assert(ret == HEIMDALL_SUCCESS);
        heimdall_ancillary_Release(ancillary);
    }

    /* Check that ancillaries are copied via a heimdall_frame_CopyProperties() (done
     * by heimdall_frame_Duplicate()). */
    heimdall_frame_t *copy_frame = heimdall_frame_Duplicate(frame);
    assert(copy_frame);
    heimdall_frame_Release(frame);
    frame = copy_frame;

    picture_t *picture = picture_New(HEIMDALL_CODEC_NV12, 1, 1, 1, 1);
    assert(picture);

    /* Manually attach both ancillaries to a newly allocated picture. */
    ancillary = heimdall_frame_GetAncillary(frame, HEIMDALL_ANCILLARY_ID('t','s','t','1'));
    assert(ancillary);
    picture_AttachAncillary(picture, ancillary);

    ancillary = heimdall_frame_GetAncillary(frame, HEIMDALL_ANCILLARY_ID('t','s','t','2'));
    assert(ancillary);
    picture_AttachAncillary(picture, ancillary);

    const char test3[] = "test3";
    void *data = picture_AttachNewAncillary(picture, HEIMDALL_ANCILLARY_ID('t','s','t','3'), sizeof(test3));
    assert(data);
    strcpy(data, test3);

    heimdall_frame_Release(frame);

    /* Check that ancillaries are copied via a picture_Clone(). */
    picture_t *clone = picture_Clone(picture);
    assert(clone);
    picture_Release(picture);

    /* Check that ancillaries are copied via a picture_Copy(). */
    picture_t *copy = picture_New(HEIMDALL_CODEC_I420, 1, 1, 1, 1);
    assert(copy);
    picture_Copy(copy, clone);
    picture_Release(clone);

    /* Check that ancillaries are still valid. */
    ancillary = picture_GetAncillary(copy, HEIMDALL_ANCILLARY_ID('t','s','t','1'));
    assert(ancillary);
    assert(strcmp("test1", heimdall_ancillary_GetData(ancillary)) == 0);

    ancillary = picture_GetAncillary(copy, HEIMDALL_ANCILLARY_ID('t','s','t','2'));
    assert(ancillary);
    assert(strcmp("test2", heimdall_ancillary_GetData(ancillary)) == 0);

    ancillary = picture_GetAncillary(copy, HEIMDALL_ANCILLARY_ID('t','s','t','3'));
    assert(ancillary);
    assert(strcmp("test3", heimdall_ancillary_GetData(ancillary)) == 0);

    picture_Release(copy);

    return 0;
}
