/*****************************************************************************
 * inhibit.c: test for the video output screensaver inhibition
 *****************************************************************************
 * Copyright (C) 2026 VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Author: Alexandre Janniaux <ajanni@videolabs.io>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
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
 * The vout window is created once per vout thread, and the vout is recycled
 * from one input to the next one. Check that switching to another media while
 * the playback is paused, which is what a playlist does when the user selects
 * another item, restores the inhibition of the screensaver.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_vout_inhibit
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>
#include <heimdall_fourcc.h>
#include <heimdall_frame.h>
#include <heimdall_inhibit.h>
#include <heimdall_interface.h>
#include <heimdall_player.h>
#include <heimdall_vout_display.h>
#include <heimdall_window.h>

#include <limits.h>

#include "../lib/libheimdall_internal.h"

const char heimdall_module_name[] = MODULE_STRING;

static const char dec_dev_arg[] = "--dec-dev=" MODULE_STRING;

static const char source_800_600[] =
    "mock://video_track_count=1;length=100000000000;video_width=800;video_height=600";

static struct
{
    heimdall_mutex_t lock;
    heimdall_cond_t wait;

    /* Number of inhibiter instances created, used to check whether the
     * inhibiter was recycled or recreated. */
    size_t inhibit_count;
    /* Flags of the last inhibition request. */
    unsigned inhibit_flags;
    /* Number of display instances created, one per media
     * (decoder_UpdateVideoOutput) here. */
    size_t display_count;
    /* Number of pictures sent to the display. */
    size_t picture_count;
} ctx = {
    .lock = HEIMDALL_STATIC_MUTEX,
    .wait = HEIMDALL_STATIC_COND,
    .inhibit_count = 0,
    .inhibit_flags = HEIMDALL_INHIBIT_NONE,
    .display_count = 0,
    .picture_count = 0,
};

/* Wait for a state change from the vout, or abort the test if it never
 * happens. This simplify the writing of the test by helping writing it
 * in a synchronous way. */
#define WAIT_UNTIL(condition) do { \
    heimdall_tick_t deadline_ = heimdall_tick_now() + HEIMDALL_TICK_FROM_SEC(30); \
    heimdall_mutex_lock(&ctx.lock); \
    while (!(condition)) \
        assert(heimdall_cond_timedwait(&ctx.wait, &ctx.lock, deadline_) == 0 \
               && "timed out waiting for: " #condition); \
    heimdall_mutex_unlock(&ctx.lock); \
} while (0)

/* Inhibit module implementation */

static void Inhibit(heimdall_inhibit_t *ih, unsigned flags)
{
    msg_Info(ih, "inhibition flags: 0x%x", flags);

    heimdall_mutex_lock(&ctx.lock);
    ctx.inhibit_flags = flags;
    heimdall_cond_broadcast(&ctx.wait);
    heimdall_mutex_unlock(&ctx.lock);
}

static int OpenInhibit(heimdall_object_t *obj)
{
    heimdall_inhibit_t *ih = (heimdall_inhibit_t *)obj;
    ih->inhibit = Inhibit;

    heimdall_mutex_lock(&ctx.lock);
    ctx.inhibit_count++;
    heimdall_mutex_unlock(&ctx.lock);

    return HEIMDALL_SUCCESS;
}

/* Decoder mock implementation */

static int DecoderDecode(decoder_t *dec, heimdall_frame_t *frame)
{
    int i_ret = HEIMDALL_SUCCESS;

    if (frame == NULL)
        return HEIMDALL_SUCCESS;

    i_ret = decoder_UpdateVideoOutput(dec, NULL);
    if (i_ret != HEIMDALL_SUCCESS)
        goto error;

    picture_t *pic = picture_NewFromFormat(&dec->fmt_out.video);
    if (pic == NULL)
    {
        i_ret = HEIMDALL_ENOMEM;
        goto error;
    }

    pic->date = frame->i_pts;
    pic->b_progressive = true;
    decoder_QueueVideo(dec, pic);

error:
    heimdall_frame_Release(frame);
    return i_ret;
}

static int OpenDecoder(heimdall_object_t *obj)
{
    decoder_t *dec = (decoder_t *)obj;

    es_format_Clean(&dec->fmt_out);
    es_format_Copy(&dec->fmt_out, dec->fmt_in);
    dec->fmt_out.i_codec
        = dec->fmt_out.video.i_chroma
        = HEIMDALL_CODEC_I420;
    dec->fmt_out.video.i_visible_width
        = dec->fmt_out.video.i_width
        = 800;
    dec->fmt_out.video.i_visible_height
        = dec->fmt_out.video.i_height
        = 600;

    dec->pf_decode = DecoderDecode;
    return HEIMDALL_SUCCESS;
}

/* Window mock implementation */

static int OpenWindow(heimdall_window_t *wnd)
{
    static const struct heimdall_window_operations ops =
    {

    };
    wnd->ops = &ops;
    return HEIMDALL_SUCCESS;
}

/* Display mock implementation */

static void Display(vout_display_t *vd, picture_t *picture)
{
    HEIMDALL_UNUSED(vd); HEIMDALL_UNUSED(picture);

    heimdall_mutex_lock(&ctx.lock);
    ctx.picture_count++;
    heimdall_cond_broadcast(&ctx.wait);
    heimdall_mutex_unlock(&ctx.lock);
}

static int OpenDisplay(vout_display_t *vd, video_format_t *fmtp,
                       struct heimdall_video_context *vctx)
{
    HEIMDALL_UNUSED(fmtp); HEIMDALL_UNUSED(vctx);

    static const struct heimdall_display_operations ops =
    {
        .display = Display,
    };
    vd->ops = &ops;

    heimdall_mutex_lock(&ctx.lock);
    ctx.display_count++;
    heimdall_cond_broadcast(&ctx.wait);
    heimdall_mutex_unlock(&ctx.lock);

    return HEIMDALL_SUCCESS;
}

/* Interface implementation, which actually starts the test */
static int OpenIntf(heimdall_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;

    var_Create(intf, "codec", HEIMDALL_VAR_STRING);
    var_SetString(intf, "codec", MODULE_STRING);

    var_Create(intf, "vout", HEIMDALL_VAR_STRING);
    var_SetString(intf, "vout", MODULE_STRING);

    var_Create(intf, "window", HEIMDALL_VAR_STRING);
    var_SetString(intf, "window", MODULE_STRING);

    input_item_t *media1 = input_item_New(source_800_600, "media1");
    assert(media1 != NULL);
    input_item_t *media2 = input_item_New(source_800_600, "media2");
    assert(media2 != NULL);

    heimdall_player_t *player = heimdall_player_New(&intf->obj, HEIMDALL_PLAYER_LOCK_NORMAL);
    assert(player != NULL);

    msg_Info(intf, "Starting the first media");
    heimdall_player_Lock(player);
    heimdall_player_SetCurrentMedia(player, media1);
    heimdall_player_Start(player);
    heimdall_player_Unlock(player);

    /* Enabling the window of the vout inhibits the screensaver. Wait for a
     * picture so that the decoder is known to be attached to the vout, which
     * is required for the pause below to reach it. */
    WAIT_UNTIL(ctx.picture_count > 0 && ctx.inhibit_flags == HEIMDALL_INHIBIT_VIDEO);

    heimdall_mutex_lock(&ctx.lock);
    assert(ctx.display_count == 1);
    heimdall_mutex_unlock(&ctx.lock);

    msg_Info(intf, "Pausing the first media");
    heimdall_player_Lock(player);
    heimdall_player_Pause(player);
    heimdall_player_Unlock(player);

    /* Pausing the playback releases the inhibition. */
    WAIT_UNTIL(ctx.inhibit_flags == HEIMDALL_INHIBIT_NONE);

    /* Switch to another media without resuming first, like a playlist does
     * when the user selects another item while paused. The vout is recycled
     * with its window still enabled. */
    msg_Info(intf, "Switching to the second media while paused");
    heimdall_player_Lock(player);
    heimdall_player_SetCurrentMedia(player, media2);
    heimdall_player_Unlock(player);

    /* The window is enabled again before the display of the second media is
     * created, so the inhibition state is settled once it shows up. */
    WAIT_UNTIL(ctx.display_count == 2);

    heimdall_mutex_lock(&ctx.lock);
    /* The window, and thus the inhibiter, must have been reused. */
    assert(ctx.inhibit_count == 1);
    /* The second media is playing, the screensaver must be inhibited again. */
    assert(ctx.inhibit_flags == HEIMDALL_INHIBIT_VIDEO);
    heimdall_mutex_unlock(&ctx.lock);

    heimdall_player_Delete(player);
    input_item_Release(media2);
    input_item_Release(media1);

    var_Destroy(intf, "window");
    var_Destroy(intf, "vout");
    var_Destroy(intf, "codec");

    return HEIMDALL_SUCCESS;
}

/**
 * Inject the mocked modules as a static plugin:
 *  - decoder generating pictures for the video output
 *  - window and display to run a video output without any display server
 *  - inhibit to monitor the screensaver inhibition requests
 **/
heimdall_module_begin()
    set_callback(OpenDecoder)
    set_capability("video decoder", INT_MAX)

    add_submodule()
        set_callback(OpenWindow)
        set_capability("vout window", INT_MAX)

    add_submodule()
        set_callback_display(OpenDisplay, 0)

    add_submodule()
        set_callback(OpenInhibit)
        set_capability("inhibit", INT_MAX)

    /* Interface module to start the test */
    add_submodule()
        set_callback(OpenIntf)
        set_capability("interface", 0)

heimdall_module_end()

HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    test_init();

    const char * const args[] = {
        "-vvv", "--aout=dummy", "--text-renderer=dummy",
        "--no-auto-preparse", "--no-hw-dec",
        "--no-spu", "--no-osd",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);
    assert(heimdall != NULL);

    libheimdall_InternalAddIntf(heimdall->p_libheimdall_int, MODULE_STRING);
    libheimdall_InternalPlay(heimdall->p_libheimdall_int);

    libheimdall_release(heimdall);
    return 0;
}
