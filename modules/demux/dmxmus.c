/*****************************************************************************
 * dmxmus.c : DMX audio music (.mus) demux module for heimdall
 *****************************************************************************
 * Copyright © 2019 Rémi Denis-Courmont
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Perusing documentation by Vladimir Arnost and Adam Nielsen.
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

#include <assert.h>
#include <string.h>

#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_demux.h>

typedef struct
{
    es_out_id_t *es;
    date_t pts; /*< Play timestamp */
    heimdall_tick_t tick; /*< Last tick timestamp */
    heimdall_tick_t length; /*< Total length */
    uint16_t start_offset; /*< Start byte offset of music events */
    unsigned end_offset:17; /*< End byte offset of music events */
    unsigned primaries:4; /*< Number of primary channels (0-9) */
    unsigned secondaries:3; /*< Number of secondary channels (10-14) */
    unsigned char volume[16]; /*< Volume of last note on each channel */
} demux_sys_t;

enum {
    MUS_EV_RELEASE,
    MUS_EV_PLAY,
    MUS_EV_PITCH,
    MUS_EV_CONTROL,
    MUS_EV_CONTROL_VALUE,
    MUS_EV_MEASURE_END,
    MUS_EV_TRACK_END,
    MUS_EV_DUMMY,
};

#define MUS_EV(byte) (((byte) >> 4) & 0x7)

enum {
    MUS_CTRL_PROGRAM_CHANGE,
    MUS_CTRL_BANK_SELECT,
    MUS_CTRL_MODULATION,
    MUS_CTRL_VOLUME,
    MUS_CTRL_PAN,
    MUS_CTRL_EXPRESSION,
    MUS_CTRL_REVERB,
    MUS_CTRL_CHORUS,
    MUS_CTRL_PEDAL_HOLD,
    MUS_CTRL_PEDAL_SOFT,
    MUS_CTRL_SOUND_OFF,
    MUS_CTRL_NOTES_OFF,
    MUS_CTRL_MONO,
    MUS_CTRL_POLY,
    MUS_CTRL_RESET,
    MUS_CTRL_EVENT,
};

#define MUS_FREQ 140

static int GetByte(demux_t *demux)
{
    unsigned char c;

    return (heimdall_stream_Read(demux->s, &c, 1) < 1) ? -1 : c;
}

/**
 * Reads one event from the bit stream.
 */
static int ReadEvent(demux_t *demux, unsigned char *buf,
                     unsigned *restrict delay)
{
    int byte = GetByte(demux);
    if (byte < 0)
        return -1;

    uint_fast8_t type = MUS_EV(byte);

    buf[0] = byte;

    if (likely(type != MUS_EV_MEASURE_END && type != MUS_EV_TRACK_END)) {
        int c = GetByte(demux);
        if (c < 0)
            return -1;

        buf[1] = c;

        switch (type) {
            case MUS_EV_PLAY:
                if (c & 0x80) {
            case MUS_EV_CONTROL_VALUE:
                    c = GetByte(demux);
                    if (c < 0)
                        return -1;

                    buf[2] = c;
                }
                break;
        }
    }

    /* Compute delay until next event */
    *delay = 0;

    while (byte & 0x80) {
        byte = GetByte(demux);

        if (byte < 0)
            return -1;

        *delay <<= 7;
        *delay |= byte & 0x7f;
    }

    return 0;
}

static block_t *Event2(uint8_t type, uint8_t channel, uint8_t data)
{
    block_t *ev = block_Alloc(2);
    if (likely(ev != NULL)) {
        ev->p_buffer[0] = type | channel;
        ev->p_buffer[1] = data;
    }
    return ev;
}

static block_t *Event3(uint8_t type, uint8_t channel,
                       uint8_t data, uint8_t data2)
{
    block_t *ev = block_Alloc(3);
    if (likely(ev != NULL)) {
        ev->p_buffer[0] = type | channel;
        ev->p_buffer[1] = data;
        ev->p_buffer[2] = data2;
    }
    return ev;
}

static block_t *HandleControl(demux_t *demux, uint8_t channel, uint8_t num)
{
    block_t *ev = NULL;

    switch (num & 0x7f) {
        case MUS_CTRL_SOUND_OFF:
            return Event3(0xB0, channel, 120, 0);

        case MUS_CTRL_NOTES_OFF:
            return Event3(0xB0, channel, 123, 0);

        case MUS_CTRL_MONO:
        case MUS_CTRL_POLY:
            break; /* only meaningful for OPL3, not soft synth */

        case MUS_CTRL_RESET:
            return Event3(0xB0, channel, 121, 0);

        case MUS_CTRL_EVENT:
            break;

        default:
            msg_Warn(demux, "unknown control %u", num & 0x7f);
    }

    return ev;
}

static block_t *HandleControlValue(demux_t *demux, uint8_t channel,
                                   uint8_t num, uint8_t val)
{
    val &= 0x7f;

    switch (num & 0x7f) {
        case MUS_CTRL_PROGRAM_CHANGE:
            return Event2(0xC0, channel, val);

        case MUS_CTRL_BANK_SELECT:
            return NULL;

        case MUS_CTRL_MODULATION:
            return Event3(0xB0, channel, 1, val);

        case MUS_CTRL_VOLUME:
            return Event3(0xB0, channel, 7, val);

        case MUS_CTRL_PAN:
            return Event3(0xB0, channel, 10, val);

        case MUS_CTRL_EXPRESSION:
            return Event3(0xB0, channel, 11, val);

        case MUS_CTRL_REVERB:
            return Event3(0xB0, channel, 91, val);

        case MUS_CTRL_CHORUS:
            return Event3(0xB0, channel, 93, val);

        case MUS_CTRL_PEDAL_HOLD:
            return Event3(0xB0, channel, 64, val);

        case MUS_CTRL_PEDAL_SOFT:
            return Event3(0xB0, channel, 67, val);

        default:
            return HandleControl(demux, channel, num);
    }
}

static int Demux(demux_t *demux)
{
    stream_t *stream = demux->s;
    demux_sys_t *sys = demux->p_sys;

    if (heimdall_stream_Tell(stream) >= sys->end_offset)
        return HEIMDALL_DEMUXER_EOF;
    /* We might overflow the end offset by 2 bytes. Not really a problem. */

    /* Inject the MIDI Tick every 10 ms */
    if (sys->tick < date_Get(&sys->pts)) {
        block_t *tick = block_Alloc(1);
        if (unlikely(tick == NULL))
            return HEIMDALL_ENOMEM;

        tick->p_buffer[0] = 0xF9;
        tick->i_dts = tick->i_pts = sys->tick;

        es_out_Send(demux->out, sys->es, tick);
        es_out_SetPCR(demux->out, sys->tick);

        sys->tick += HEIMDALL_TICK_FROM_MS(10);
        return HEIMDALL_DEMUXER_SUCCESS;
    }

    /* Read one MIDI event */
    block_t *ev = NULL;
    unsigned char buf[3];
    unsigned delay;

    if (ReadEvent(demux, buf, &delay))
        return HEIMDALL_DEMUXER_EGENERIC;

    uint_fast8_t channel = buf[0] & 0xf;

    if (channel >= ((channel < 10) ? sys->primaries : (10 + sys->secondaries)))
        channel = 9;

    switch (MUS_EV(buf[0])) {
        case MUS_EV_RELEASE:
            ev = Event2(0x80, channel, buf[1] & 0x7f);
            break;

        case MUS_EV_PLAY:
            if (buf[1] & 0x80)
                sys->volume[channel] = buf[2] & 0x7f;

            ev = Event3(0x90, channel, buf[1] & 0x7f, sys->volume[channel]);
            break;

        case MUS_EV_PITCH:
            ev = Event3(0xE0, channel, (buf[1] << 6) & 0x7f,
                        (buf[1] >> 1) & 0x7f);
            break;

        case MUS_EV_CONTROL:
            ev = HandleControl(demux, channel, buf[1]);
            break;

        case MUS_EV_CONTROL_VALUE:
            ev = HandleControlValue(demux, channel, buf[1], buf[2]);
            break;

        case MUS_EV_MEASURE_END:
            break;

        case MUS_EV_TRACK_END:
            return HEIMDALL_DEMUXER_EOF;

        case MUS_EV_DUMMY:
            break;

        default:
            heimdall_assert_unreachable();
    }

    if (ev != NULL) {
        ev->i_pts = ev->i_dts = date_Get(&sys->pts);
        es_out_Send(demux->out, sys->es, ev);
    }

    date_Increment(&sys->pts, delay);
    return HEIMDALL_DEMUXER_SUCCESS;
}

static int SeekSet0(demux_t *demux)
{
    demux_sys_t *sys = demux->p_sys;

    if (heimdall_stream_Seek(demux->s, sys->start_offset))
        return -1;

    date_Set(&sys->pts, HEIMDALL_TICK_0);
    return 0;
}

/**
 * Gets the total length in ticks.
 *
 * @note This function clobbers the read offset of the byte stream.
 */
static heimdall_tick_t GetLength(demux_t *demux)
{
    demux_sys_t *sys = demux->p_sys;
    unsigned parts = 0;

    if (SeekSet0(demux))
        return HEIMDALL_TICK_INVALID;

    for (;;) {
        unsigned char buf[3];
        unsigned delay;

        if (ReadEvent(demux, buf, &delay)
         || MUS_EV(buf[0]) == MUS_EV_TRACK_END
         || heimdall_stream_Tell(demux->s) >= sys->end_offset)
            break;

        parts += delay;
    }

    return heimdall_tick_from_samples(parts, MUS_FREQ);
}

static int Control(demux_t *demux, int query, va_list args)
{
    demux_sys_t *sys = demux->p_sys;

    switch (query) {
        case DEMUX_CAN_SEEK:
            *va_arg(args, bool *) = false; /* TODO */
            break;

        case DEMUX_GET_POSITION: {
            double pos = 0.;

            static_assert(HEIMDALL_TICK_INVALID <= 0, "Oops");
            if (sys->length > 0)
                pos = (date_Get(&sys->pts) - HEIMDALL_TICK_0) * 1. / sys->length;

            *va_arg(args, double *) = pos;
            break;
        }

        case DEMUX_SET_POSITION:
            return HEIMDALL_EGENERIC;

        case DEMUX_GET_LENGTH:
            if (sys->length == HEIMDALL_TICK_INVALID)
                return HEIMDALL_EGENERIC;
            *va_arg(args, heimdall_tick_t *) = sys->length;
            break;

        case DEMUX_GET_TIME:
            *va_arg(args, heimdall_tick_t *) = date_Get(&sys->pts) - HEIMDALL_TICK_0;
            break;

        case DEMUX_SET_TIME:
            return HEIMDALL_EGENERIC;

        case DEMUX_CAN_PAUSE:
        case DEMUX_SET_PAUSE_STATE:
        case DEMUX_CAN_CONTROL_PACE:
        case DEMUX_GET_PTS_DELAY:
            return demux_vaControlHelper(demux->s, 0, -1, 0, 1, query, args);

        default:
            return HEIMDALL_EGENERIC;
    }
    return HEIMDALL_SUCCESS;
}

static int Open(heimdall_object_t *obj)
{
    demux_t *demux = (demux_t *)obj;
    stream_t *stream = demux->s;
    const uint8_t *hdr;

    if (heimdall_stream_Peek(stream, &hdr, 16) < 16)
        return HEIMDALL_EGENERIC;
    if (memcmp(hdr, "MUS\x1A", 4))
        return HEIMDALL_EGENERIC;

    uint_fast16_t length = GetWLE(hdr + 4);
    uint_fast16_t offset = GetWLE(hdr + 6);
    uint_fast16_t primaries = GetWLE(hdr + 8);
    uint_fast16_t secondaries = GetWLE(hdr + 10);

    if (primaries > 8 || secondaries > 5)
        return HEIMDALL_EGENERIC;

    /* Ignore the patch list and jump to the event offset. */
    size_t instc = GetWLE(hdr + 12);
    size_t hdrlen = 16 + 2 * instc;

    if (offset < hdrlen)
        return HEIMDALL_EGENERIC;

    msg_Dbg(demux, "MIDI channels: %u primary, %u secondary",
            GetWLE(hdr + 8), GetWLE(hdr + 10));

    demux_sys_t *sys = heimdall_obj_malloc(obj, sizeof (*sys));
    if (unlikely(sys == NULL))
        return HEIMDALL_ENOMEM;

    demux->p_sys = sys;
    sys->start_offset = offset;
    sys->end_offset = (uint_fast32_t)offset + (uint_fast32_t)length;
    sys->primaries = primaries;
    sys->secondaries = secondaries;
    memset(sys->volume, 0, sizeof (sys->volume));

    bool can_seek;
    heimdall_stream_Control(demux->s, STREAM_CAN_SEEK, &can_seek);
    if (can_seek) {
        sys->length = GetLength(demux);

        if (SeekSet0(demux))
            return HEIMDALL_EGENERIC;
    } else {
        sys->length = HEIMDALL_TICK_INVALID;
        offset -= hdrlen;

        if ((size_t)heimdall_stream_Read(stream, NULL, offset) != offset)
            return HEIMDALL_EGENERIC;
    }

    es_format_t fmt;

    es_format_Init(&fmt, AUDIO_ES, HEIMDALL_CODEC_MIDI);
    fmt.audio.i_channels = 2;
    fmt.audio.i_rate = 44100;
    sys->es = es_out_Add(demux->out, &fmt);
    if(unlikely(!sys->es))
        return HEIMDALL_EGENERIC;

    date_Init(&sys->pts, MUS_FREQ, 1);
    date_Set(&sys->pts, HEIMDALL_TICK_0);
    sys->tick = HEIMDALL_TICK_0;

    demux->pf_demux = Demux;
    demux->pf_control = Control;
    return HEIMDALL_SUCCESS;
}

heimdall_module_begin()
    set_description(N_("DMX music demuxer"))
    set_subcategory(SUBCAT_INPUT_DEMUX)
    set_capability("demux", 20)
    set_callbacks(Open, NULL)
heimdall_module_end()
