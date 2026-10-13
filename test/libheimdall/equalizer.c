/*
 * equalizer.c - libheimdall smoke test
 */

#include <string.h>
#include <math.h>

#include "test.h"

static void test_equalizer (const char ** argv, int argc)
{
    libheimdall_instance_t *heimdall;
    libheimdall_media_player_t *mp;
    libheimdall_equalizer_t *equalizer;
    unsigned u_presets, u_bands;

    test_log ("Testing equalizer\n");

    heimdall = libheimdall_new (argc, argv);
    assert (NULL != heimdall);

    mp = libheimdall_media_player_new (heimdall, NULL, NULL);
    assert (NULL != mp);

    // presets

    test_log ("Testing equalizer presets\n");

    u_presets = libheimdall_audio_equalizer_get_preset_count();
    assert (u_presets > 0);

    for (unsigned i = 0; i < u_presets; i++)
    {
        assert (NULL != libheimdall_audio_equalizer_get_preset_name(i));

        equalizer = libheimdall_audio_equalizer_new_from_preset(i);
        assert (equalizer);

        libheimdall_audio_equalizer_release(equalizer);
    }

    // preset out of range

    assert (NULL == libheimdall_audio_equalizer_get_preset_name(u_presets));
    assert (NULL == libheimdall_audio_equalizer_new_from_preset(u_presets));

    // bands

    test_log ("Testing equalizer bands\n");

    u_bands = libheimdall_audio_equalizer_get_band_count();
    assert (u_bands > 0);

    for (unsigned i = 0; i < u_bands; i++)
        assert (-1.0f != libheimdall_audio_equalizer_get_band_frequency(i));

    // band out of range

    assert (-1.0f == libheimdall_audio_equalizer_get_band_frequency(u_bands));

    // initialisation

    test_log ("Testing equalizer initialisation\n");

    equalizer = libheimdall_audio_equalizer_new();
    assert (equalizer);

    assert (libheimdall_audio_equalizer_get_preamp(equalizer) == 0.0f);

    for (unsigned i = 0; i < u_bands; i++)
        assert (libheimdall_audio_equalizer_get_amp_at_index(equalizer, i) == 0.0f);

    // preamp (range -20.0 to 20.0)

    test_log ("Testing equalizer preamp\n");

    assert (0 == libheimdall_audio_equalizer_set_preamp (equalizer, 19.9f));
    assert (libheimdall_audio_equalizer_get_preamp(equalizer) == 19.9f);

    assert (0 == libheimdall_audio_equalizer_set_preamp (equalizer, 20.0f));
    assert (libheimdall_audio_equalizer_get_preamp(equalizer) == 20.0f);

    assert (0 == libheimdall_audio_equalizer_set_preamp (equalizer, 20.1f));
    assert (libheimdall_audio_equalizer_get_preamp(equalizer) == 20.0f);

    assert (0 == libheimdall_audio_equalizer_set_preamp (equalizer, -19.9f));
    assert (libheimdall_audio_equalizer_get_preamp(equalizer) == -19.9f);

    assert (0 == libheimdall_audio_equalizer_set_preamp (equalizer, -20.0f));
    assert (libheimdall_audio_equalizer_get_preamp(equalizer) == -20.0f);

    assert (0 == libheimdall_audio_equalizer_set_preamp (equalizer, -20.1f));
    assert (libheimdall_audio_equalizer_get_preamp(equalizer) == -20.0f);

    // amp at index (range -20.0 to 20.0)

    test_log ("Testing equalizer amp at index\n");

    for (unsigned i = 0; i < u_bands; i++)
    {
        assert (0 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, 19.9f, i));
        assert (libheimdall_audio_equalizer_get_amp_at_index (equalizer, i) == 19.9f);

        assert (0 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, 20.0f, i));
        assert (libheimdall_audio_equalizer_get_amp_at_index (equalizer, i) == 20.0f);

        assert (0 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, 20.1f, i));
        assert (libheimdall_audio_equalizer_get_amp_at_index (equalizer, i) == 20.0f);

        assert (0 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, -19.9f, i));
        assert (libheimdall_audio_equalizer_get_amp_at_index (equalizer, i) == -19.9f);

        assert (0 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, -20.0f, i));
        assert (libheimdall_audio_equalizer_get_amp_at_index (equalizer, i) == -20.0f);

        assert (0 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, -20.1f, i));
        assert (libheimdall_audio_equalizer_get_amp_at_index (equalizer, i) == -20.0f);
    }

    // amp at index out of range

    assert (isnan(libheimdall_audio_equalizer_get_amp_at_index (equalizer, u_bands)));
    assert (-1 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, 19.9f, u_bands));

    // no equalizer

    test_log ("Testing release NULL\n");

    libheimdall_audio_equalizer_release (NULL);

    // media player

    test_log ("Testing set equalizer\n");

    assert (0 == libheimdall_media_player_set_equalizer(mp, NULL));
    assert (0 == libheimdall_media_player_set_equalizer(mp, equalizer));

    // maximum string length (test string concatenation)

    test_log ("Testing equalizer-bands string limit\n");

    for (unsigned i = 0; i < u_bands; i++)
        assert (0 == libheimdall_audio_equalizer_set_amp_at_index (equalizer, -19.1234567f, i));

    assert (0 == libheimdall_media_player_set_equalizer(mp, equalizer));

    // cleanup

    test_log ("Testing equalizer cleanup\n");

    libheimdall_audio_equalizer_release (equalizer);

    test_log ("Finished testing equalizer\n");

    libheimdall_media_player_release (mp);

    libheimdall_release (heimdall);
}

int main (void)
{
    test_init();

    test_equalizer (test_defaults_args, test_defaults_nargs);

    return 0;
}
