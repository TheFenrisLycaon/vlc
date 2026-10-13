// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * seeks.c: seeks player test
 *****************************************************************************
 * Copyright (C) 2018-2025 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *****************************************************************************/

#include "common.h"

static void
test_seeks(struct ctx *ctx)
{
    test_log("seeks\n");
    heimdall_player_t *player = ctx->player;

    struct media_params params = DEFAULT_MEDIA_PARAMS(HEIMDALL_TICK_FROM_SEC(10));
    player_set_current_mock_media(ctx, "media1", &params, false);

    /* only the last one will be taken into account before start */
    heimdall_player_SetTimeFast(player, 0);
    heimdall_player_SetTimeFast(player, HEIMDALL_TICK_FROM_SEC(100));
    heimdall_player_SetTimeFast(player, 10);

    heimdall_tick_t seek_time = HEIMDALL_TICK_FROM_SEC(5);
    heimdall_player_SetTimeFast(player, seek_time);
    player_start(ctx);

    {
        vec_on_position_changed *vec = &ctx->report.on_position_changed;
        while (vec->size == 0)
            heimdall_player_CondWait(player, &ctx->wait);

        assert(VEC_LAST(vec).time >= seek_time);
        assert_position(ctx, &VEC_LAST(vec));

        heimdall_tick_t last_time = VEC_LAST(vec).time;

        heimdall_tick_t jump_time = -HEIMDALL_TICK_FROM_SEC(2);
        heimdall_player_JumpTime(player, jump_time);

        while (VEC_LAST(vec).time >= last_time)
            heimdall_player_CondWait(player, &ctx->wait);

        assert(VEC_LAST(vec).time >= last_time + jump_time);
        assert_position(ctx, &VEC_LAST(vec));
    }

    heimdall_player_SetPosition(player, 1.0f);

    test_prestop(ctx);

    wait_state(ctx, HEIMDALL_PLAYER_STATE_STOPPED);
    assert_normal_state(ctx);

    test_end(ctx);
}

int
main(void)
{
    struct ctx ctx;
    ctx_init(&ctx, 0);
    test_seeks(&ctx);
    ctx_destroy(&ctx);
    return 0;
}
