/*****************************************************************************
 * sdi.cpp: test for the SDI output decoded streams
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Define a builtin module for mocked parts */
#define MODULE_NAME test_sdi_mock
#undef HEIMDALL_DYNAMIC_PLUGIN

#include "../../libheimdall/test.h"
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_codec.h>

#include "../../../modules/stream_out/sdi/SDIStream.hpp"
#include "../lib/libheimdall_internal.h"

#include <climits>
#include <cstdint>
#include <memory>

const char heimdall_module_name[] = MODULE_STRING;

static unsigned decoder_opened = 0;
static unsigned decoder_closed = 0;

struct dec_fmt {
    heimdall_fourcc_t codec;
};

static void DecoderDeviceClose(struct heimdall_decoder_device *device)
    { HEIMDALL_UNUSED(device); }

static const struct heimdall_decoder_device_operations decoder_device_ops =
{
    DecoderDeviceClose,
};

static int OpenDecoderDevice(struct heimdall_decoder_device *device,
                             heimdall_window_t *window)
{
    HEIMDALL_UNUSED(window);
    device->ops = &decoder_device_ops;
    /* The private parts are never used in this test. */
    device->type = HEIMDALL_DECODER_DEVICE_VAAPI;
    return HEIMDALL_SUCCESS;
}

static int DecoderDecode(decoder_t *dec, heimdall_frame_t *frame)
{
    HEIMDALL_UNUSED(dec);
    if (frame != NULL)
        heimdall_frame_Release(frame);
    return HEIMDALLDEC_SUCCESS;
}

static void CloseDecoder(heimdall_object_t *obj)
{
    decoder_t *dec = reinterpret_cast<decoder_t *>(obj);
    dec_fmt *sys = static_cast<dec_fmt*>(dec->p_sys);

    /* An owner cleaning fmt_in before unloading the module would leave an
     * UNKNOWN_ES format behind. */
    assert(dec->fmt_in->i_cat == VIDEO_ES || dec->fmt_in->i_cat == AUDIO_ES);
    assert(dec->fmt_in->i_codec == sys->codec);

    decoder_closed++;
    delete sys;
}

static int OpenDecoder(heimdall_object_t *obj)
{
    decoder_t *dec = reinterpret_cast<decoder_t *>(obj);

    if (dec->fmt_in->i_cat == VIDEO_ES)
    {
        /* Make the owner instantiate its decoder device, so that the
         * teardown has to release it from the owner storage. */
        heimdall_decoder_device *dev = decoder_GetDecoderDevice(dec);
        assert(dev != NULL);
        heimdall_decoder_device_Release(dev);
    }

    es_format_Clean(&dec->fmt_out);
    es_format_Copy(&dec->fmt_out, dec->fmt_in);

    /* Remember the format the module was opened with, so that the close
     * callback can check it is still the one referenced by fmt_in. */
    dec_fmt *sys = new dec_fmt;
    sys->codec = dec->fmt_in->i_codec;
    dec->p_sys = reinterpret_cast<void *>(sys);
    dec->pf_decode = DecoderDecode;

    decoder_opened++;
    return HEIMDALL_SUCCESS;
}

static void test_decoded_stream(heimdall_object_t *obj, heimdall_fourcc_t i_codec,
                                es_format_category_e i_cat)
{
    std::unique_ptr<sdi_sout::AbstractStreamOutputBuffer> buffer;
    std::unique_ptr<sdi_sout::AbstractDecodedStream> stream;
    es_format_t output;

    const sdi_sout::StreamID id(1);

    if (i_cat == VIDEO_ES)
    {
        buffer = std::make_unique<sdi_sout::PictureStreamOutputBuffer>();
        stream = std::make_unique<sdi_sout::VideoDecodedStream>(obj, id, buffer.get());

        es_format_Init(&output, VIDEO_ES, HEIMDALL_CODEC_UYVY);
        output.video.i_chroma = output.i_codec;
        output.video.i_width = output.video.i_visible_width = 720;
        output.video.i_height = output.video.i_visible_height = 576;
    }
    else
    {
        buffer = std::make_unique<sdi_sout::BlockStreamOutputBuffer>();
        stream = std::make_unique<sdi_sout::AudioDecodedStream>(obj, id, buffer.get());

        es_format_Init(&output, AUDIO_ES, HEIMDALL_CODEC_S16N);
        output.audio.i_format = output.i_codec;
        output.audio.i_rate = 48000;
        output.audio.i_physical_channels = AOUT_CHANS_STEREO;
        aout_FormatPrepare(&output.audio);
    }

    stream->setOutputFormat(&output);
    es_format_Clean(&output);

    es_format_t input;
    es_format_Init(&input, i_cat, i_codec);
    if (i_cat == VIDEO_ES)
    {
        input.video.i_width = input.video.i_visible_width = 720;
        input.video.i_height = input.video.i_visible_height = 576;
    }
    else
    {
        input.audio.i_rate = 48000;
        input.audio.i_physical_channels = AOUT_CHANS_STEREO;
    }

    const unsigned opened = decoder_opened;
    const unsigned closed = decoder_closed;

    bool success = stream->init(&input);
    assert(success);
    assert(decoder_opened == opened + 1);

    /* Check decoder unloading. */
    stream.reset();
    assert(decoder_closed == closed + 1);

    es_format_Clean(&input);
    buffer.reset();
}

heimdall_module_begin()
    set_callbacks(OpenDecoder, CloseDecoder)
    set_capability("video decoder", INT_MAX)

    add_submodule()
        set_callbacks(OpenDecoder, CloseDecoder)
        set_capability("audio decoder", INT_MAX)

    add_submodule()
        set_callback_dec_device(OpenDecoderDevice, 0)
heimdall_module_end()

extern "C" HEIMDALL_EXPORT const heimdall_plugin_cb heimdall_static_modules[];
extern "C" const heimdall_plugin_cb heimdall_static_modules[] = {
    HEIMDALL_SYMBOL(heimdall_entry),
    NULL
};

int main(int argc, char **argv)
{
    HEIMDALL_UNUSED(argc); HEIMDALL_UNUSED(argv);
    test_init();

    const char * const args[] = {
        "-vv", "--codec=" MODULE_STRING, "--dec-dev=" MODULE_STRING,
        "--no-auto-preparse",
    };

    libheimdall_instance_t *heimdall = libheimdall_new(ARRAY_SIZE(args), args);
    assert(heimdall != NULL);

    heimdall_object_t *obj = HEIMDALL_OBJECT(heimdall->p_libheimdall_int);
    test_decoded_stream(obj, HEIMDALL_CODEC_MP4V, VIDEO_ES);
    test_decoded_stream(obj, HEIMDALL_CODEC_MPGA, AUDIO_ES);

    libheimdall_release(heimdall);
    assert(decoder_opened == 2);
    assert(decoder_closed == 2);
    return 0;
}
