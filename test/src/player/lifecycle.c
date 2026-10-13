// SPDX-License-Identifier: LGPL-2.1-or-later
/*****************************************************************************
 * lifecycle.c: player lifecycle test
 *****************************************************************************
 * Copyright (C) 2018-2025 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *****************************************************************************/

#include "common.h"

static void
test_delete_while_playback(heimdall_object_t *obj, bool start)
{
    test_log("delete_while_playback (start: %d)\n", start);
    heimdall_player_t *player = heimdall_player_New(obj, HEIMDALL_PLAYER_LOCK_NORMAL);

    struct media_params params = DEFAULT_MEDIA_PARAMS(HEIMDALL_TICK_FROM_SEC(10));
    input_item_t *media = create_mock_media("media1", &params);
    assert(media);

    heimdall_player_Lock(player);
    int ret = heimdall_player_SetCurrentMedia(player, media);
    assert(ret == HEIMDALL_SUCCESS);
    input_item_Release(media);

    if (start)
    {
        ret = heimdall_player_Start(player);
        assert(ret == HEIMDALL_SUCCESS);
    }

    heimdall_player_Unlock(player);

    heimdall_player_Delete(player);
}

int
main(void)
{
    test_init();

    libheimdall_instance_t *dummy = libheimdall_new(0, NULL);
    assert(dummy != NULL);

    test_delete_while_playback(HEIMDALL_OBJECT(dummy->p_libheimdall_int), true);
    test_delete_while_playback(HEIMDALL_OBJECT(dummy->p_libheimdall_int), false);

    libheimdall_release(dummy);
    return 0;
}
