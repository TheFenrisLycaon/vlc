/*****************************************************************************
 * thumbnail.c: test thumbnailing API
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

#include "../../libheimdall/test.h"
#include "../lib/libheimdall_internal.h"

#include <heimdall_common.h>
#include <heimdall_preparser.h>
#include <heimdall_input_item.h>
#include <heimdall_picture.h>

#include <errno.h>

#define MOCK_DURATION HEIMDALL_TICK_FROM_SEC( 5 * 60 )

const struct
{
    uint32_t i_nb_video_tracks;
    uint32_t i_nb_audio_tracks;
    heimdall_tick_t i_add_video_track_at;
    heimdall_tick_t i_time;
    float f_pos;
    bool b_use_pos;
    bool b_fast_seek;
    bool b_can_control_pace;
    heimdall_tick_t i_timeout;
    bool b_expected_success;
} test_params[] = {
    /* Simple test with a thumbnail at 60s, with a video track */
    { 1, 0, HEIMDALL_TICK_INVALID, HEIMDALL_TICK_FROM_SEC( 60 ), .0f, false, true, true,
      HEIMDALL_TICK_INVALID, true },
    /* Test without fast-seek */
    { 1, 0, HEIMDALL_TICK_INVALID, HEIMDALL_TICK_FROM_SEC( 60 ), .0f, false, false, true,
      HEIMDALL_TICK_INVALID, true },
    /* Seek by position test */
    { 1, 0, HEIMDALL_TICK_INVALID, 0, .3f, true, true, true, HEIMDALL_TICK_INVALID, true },
    /* Seek at a negative position */
    { 1, 0, HEIMDALL_TICK_INVALID, -12345, .0f, false, true, true, HEIMDALL_TICK_INVALID, true },
    /* Take a thumbnail of a file without video, which should timeout. */
    { 0, 1, HEIMDALL_TICK_INVALID, HEIMDALL_TICK_FROM_SEC( 60 ), .0f, false, true, false, HEIMDALL_TICK_FROM_MS( 100 ), false },
    /* Take a thumbnail of a file with a video track starting later */
    { 1, 1, HEIMDALL_TICK_FROM_SEC( 60 ), HEIMDALL_TICK_FROM_SEC( 30 ), .0f, false, true, true,
      HEIMDALL_TICK_INVALID, true },
};

struct test_ctx
{
    heimdall_cond_t cond;
    heimdall_mutex_t lock;
    size_t test_idx;
    bool b_done;
};

static void thumbnailer_callback( heimdall_preparser_req *req, int status,
                                  picture_t* thumbnail, void *data )
{
    struct test_ctx* p_ctx = data;
    heimdall_mutex_lock( &p_ctx->lock );

    if ( thumbnail != NULL )
    {
        assert( test_params[p_ctx->test_idx].b_expected_success &&
                "Expected failure but got a thumbnail" );
        assert( thumbnail->format.i_chroma == HEIMDALL_CODEC_ARGB );
        assert( status == HEIMDALL_SUCCESS );

        /* TODO: Enable this once the new clock is merged */
#if 0
        heimdall_tick_t expected_date;
        /* Don't rely on the expected date if it was purposely invalid */
        if ( test_params[p_ctx->test_idx].b_use_pos == true )
            expected_date = MOCK_DURATION * test_params[p_ctx->test_idx].f_pos;
        else if ( test_params[p_ctx->test_idx].i_add_video_track_at != HEIMDALL_TICK_INVALID )
            expected_date = test_params[p_ctx->test_idx].i_add_video_track_at;
        else
        {
            if ( test_params[p_ctx->test_idx].i_time < 0 )
                expected_date = HEIMDALL_TICK_0;
            else
                expected_date = test_params[p_ctx->test_idx].i_time;
        }
        assert( thumbnail->date == expected_date && "Unexpected picture date");
#endif
    }
    else
    {
        assert( !test_params[p_ctx->test_idx].b_expected_success &&
                "Expected a thumbnail but got a failure" );
        if (test_params[p_ctx->test_idx].i_timeout == HEIMDALL_TICK_INVALID)
            assert( status == HEIMDALL_EGENERIC );
        else
            assert( status == HEIMDALL_ETIMEOUT );
    }

    p_ctx->b_done = true;
    heimdall_cond_signal( &p_ctx->cond );
    heimdall_mutex_unlock( &p_ctx->lock );
    heimdall_preparser_req_Release( req );
}

static void test_thumbnails( libheimdall_instance_t* p_heimdall, bool external )
{
    struct test_ctx ctx;
    heimdall_cond_init( &ctx.cond );
    heimdall_mutex_init( &ctx.lock );

    for ( size_t i = 0; i < ARRAY_SIZE(test_params); ++i)
    {
        char* psz_mrl;

        ctx.test_idx = i;
        ctx.b_done = false;

        const struct heimdall_preparser_cfg cfg = {
            .types = HEIMDALL_PREPARSER_TYPE_THUMBNAIL,
            .timeout = test_params[i].i_timeout,
            .external_process = external,
        };

        heimdall_preparser_t* p_thumbnailer = heimdall_preparser_New(
                    HEIMDALL_OBJECT( p_heimdall->p_libheimdall_int ), &cfg );
        assert( p_thumbnailer != NULL );


        if ( asprintf( &psz_mrl, "mock://video_track_count=%u;audio_track_count=%u"
                       ";length=%" PRId64 ";can_control_pace=%s;video_chroma=ARGB;video_add_track_at=%" PRId64,
                       test_params[i].i_nb_video_tracks,
                       test_params[i].i_nb_audio_tracks, MOCK_DURATION,
                       test_params[i].b_can_control_pace ? "true" : "false",
                       test_params[i].i_add_video_track_at ) < 0 )
            assert( !"Failed to allocate mock mrl" );
        input_item_t* p_item = input_item_New( psz_mrl, "mock item" );
        assert( p_item != NULL );

        heimdall_mutex_lock( &ctx.lock );

        heimdall_preparser_req *req;
        struct heimdall_thumbnailer_arg thumb_arg;
        if ( test_params[i].b_use_pos )
        {
            thumb_arg.seek.type = HEIMDALL_THUMBNAILER_SEEK_POS;
            thumb_arg.seek.pos = test_params[i].f_pos;
            thumb_arg.seek.speed = test_params[i].b_fast_seek ?
                HEIMDALL_THUMBNAILER_SEEK_FAST : HEIMDALL_THUMBNAILER_SEEK_PRECISE;
        }
        else
        {
            thumb_arg.seek.type = HEIMDALL_THUMBNAILER_SEEK_TIME;
            thumb_arg.seek.time = test_params[i].i_time;
            thumb_arg.seek.speed = test_params[i].b_fast_seek ?
                HEIMDALL_THUMBNAILER_SEEK_FAST : HEIMDALL_THUMBNAILER_SEEK_PRECISE;
        }
        thumb_arg.hw_dec = false;
        static const struct heimdall_thumbnailer_cbs cbs = {
            .on_ended = thumbnailer_callback,
        };
        req = heimdall_preparser_req_NewThumbnail( p_thumbnailer, p_item,
                                              &thumb_arg, &cbs, &ctx );
        assert( req != NULL );
        assert( heimdall_preparser_Submit( p_thumbnailer, req ) == HEIMDALL_SUCCESS );

        while ( ctx.b_done == false )
            heimdall_cond_wait( &ctx.cond, &ctx.lock );

        heimdall_mutex_unlock( &ctx.lock );

        input_item_Release( p_item );
        free( psz_mrl );

        heimdall_preparser_Delete( p_thumbnailer );
    }
}

static void thumbnailer_callback_cancel( heimdall_preparser_req *req, int status,
                                         picture_t* p_thumbnail, void *data )
{
    (void)req;
    assert( p_thumbnail == NULL );
    assert( status == -EINTR );

    heimdall_sem_t *sem = data;
    heimdall_sem_post(sem);
}

static void test_cancel_thumbnail( libheimdall_instance_t* p_heimdall, bool external )
{
    const struct heimdall_preparser_cfg cfg = {
        .types = HEIMDALL_PREPARSER_TYPE_THUMBNAIL,
        .timeout = HEIMDALL_TICK_INVALID,
        .external_process = external,
    };
    heimdall_preparser_t* p_thumbnailer = heimdall_preparser_New(
                HEIMDALL_OBJECT( p_heimdall->p_libheimdall_int ), &cfg );
    assert( p_thumbnailer != NULL );

    const char* psz_mrl = "mock://video_track_count=0;audio_track_count=1;"
                          /* force timeout: parsing will take the same time as length */
                          "can_control_pace=false;"
                          "length=20000000";
    input_item_t* p_item = input_item_New( psz_mrl, "mock item" );
    assert( p_item != NULL );

    static const struct heimdall_thumbnailer_cbs cbs = {
        .on_ended = thumbnailer_callback_cancel,
    };

    heimdall_sem_t sem;
    heimdall_sem_init(&sem, 0);
    heimdall_preparser_req *req =
        heimdall_preparser_req_NewThumbnail( p_thumbnailer, p_item, NULL, &cbs, &sem );
    assert( req != NULL );
    assert( heimdall_preparser_Submit( p_thumbnailer, req ) == HEIMDALL_SUCCESS );

    heimdall_preparser_Cancel( p_thumbnailer, req );

    heimdall_sem_wait(&sem);

    heimdall_preparser_req_Release( req );

    input_item_Release( p_item );

    heimdall_preparser_Delete( p_thumbnailer );
}

int main( void )
{
    test_init();

    static const char * argv[] = {
        "-v",
        "--ignore-config",
    };
    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(argv), argv);
    assert(heimdall);

    fprintf(stderr, "Run with internal preparser...\n");
    test_thumbnails( heimdall, false );
    test_cancel_thumbnail( heimdall, false );

#if !defined(HAVE_HEIMDALL_PROCESS_SPAWN)
    fprintf(stderr, "Run with external preparser...\n");
    test_thumbnails( heimdall, true );
    test_cancel_thumbnail( heimdall, true );
#endif


    libheimdall_release( heimdall );
}
