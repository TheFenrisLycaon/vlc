/*****************************************************************************
 * packetizer.h: packetizer unit testing
 *****************************************************************************
 * Copyright (C) 2019 VideoLabs, VideoLAN and VLC Authors
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
#include <heimdall_common.h>
#include <heimdall_plugin.h>
#include <heimdall_modules.h>
#include <heimdall_demux.h>
#include <heimdall_codec.h>
#include <heimdall_meta.h>

enum
{
    OK = HEIMDALL_SUCCESS,
    FAIL = HEIMDALL_EGENERIC,
};

struct params_s
{
    libheimdall_instance_t *heimdall;
    heimdall_object_t *obj;
    heimdall_fourcc_t codec;
    unsigned i_rate_num;
    unsigned i_rate_den;
    unsigned i_read_size;
    unsigned i_frame_count;
    bool b_extra;
};

#define BAILOUT(run) { fprintf(stderr, "failed %s line %d\n", run, __LINE__); \
                        return 1; }
#define RUN(run, test, a, b, res) \
    if(test(#test " " run, a, b, &params) != res) BAILOUT(#test " " run)
#define EXPECT(foo) if(!(foo)) BAILOUT(run)


struct packetizer_owner
{
    decoder_t   packetizer;
    es_format_t fmt_in;
};

static void delete_packetizer(decoder_t *p_pack)
{
    struct packetizer_owner *owner = container_of(p_pack, struct packetizer_owner, packetizer);
    if(p_pack->p_module)
        module_unneed(p_pack, p_pack->p_module);
    es_format_Clean(&owner->fmt_in);
    es_format_Clean(&p_pack->fmt_out);
    if(p_pack->p_description)
        heimdall_meta_Delete(p_pack->p_description);
    heimdall_object_delete(p_pack);
}

static decoder_t *create_packetizer(libheimdall_instance_t *heimdall,
                                    unsigned num, unsigned den,
                                    heimdall_fourcc_t codec)
{
    struct packetizer_owner *owner;
    owner = heimdall_object_create(heimdall->p_libheimdall_int, sizeof(*owner));
    if(!owner)
        return NULL;
    decoder_t *p_pack = &owner->packetizer;
    p_pack->pf_decode = NULL;
    p_pack->pf_packetize = NULL;

    es_format_Init(&owner->fmt_in, VIDEO_ES, codec);
    es_format_Init(&p_pack->fmt_out, VIDEO_ES, 0);
    owner->fmt_in.video.i_frame_rate = num;
    owner->fmt_in.video.i_frame_rate_base = den;
    owner->fmt_in.b_packetized = false;
    p_pack->fmt_in = &owner->fmt_in;

    decoder_LoadModule( p_pack, true, false );
    if(!p_pack->p_module)
        delete_packetizer(p_pack);
    return p_pack;
}

static int test_packetize(const char *run,
                          const uint8_t *p_data, size_t i_data,
                          const struct params_s *params)
{
    decoder_t *p = create_packetizer(params->heimdall,
                                     params->i_rate_num,
                                     params->i_rate_den,
                                     params->codec);
    EXPECT(p != NULL);

    stream_t *s = heimdall_stream_MemoryNew(params->obj,
                                       (uint8_t *)p_data, i_data, true);
    EXPECT(s != NULL);
    block_t *outchain = NULL;
    block_t **outappend = &outchain;
    block_t *p_block;
    unsigned i_count = 0;
    do
    {
        p_block = heimdall_stream_Block(s, params->i_read_size);
        block_t *in = p_block;
        if(in && outchain == NULL)
            in->i_dts = HEIMDALL_TICK_0;
        block_t *out;
        do
        {
            out = p->pf_packetize(p, in ? &in : NULL);
            if(out)
            {
                fprintf(stderr, "block #%u dts %"PRId64
                                " flags %x sz %zu""\n",
                        i_count, out->i_dts,
                        out->i_flags, out->i_buffer );
                block_ChainLastAppend(&outappend, out);
                ++i_count;
            }
        } while(out);
    } while(p_block);

    EXPECT(i_count == params->i_frame_count);

    if(params->i_rate_num && params->i_rate_den)
    {
        EXPECT(p->fmt_out.video.i_frame_rate == params->i_rate_num);
        EXPECT(p->fmt_out.video.i_frame_rate_base == params->i_rate_den);
        EXPECT(p->fmt_out.video.i_visible_width);
        EXPECT(p->fmt_out.video.i_visible_height);
    }

    if(params->i_frame_count)
    {
        EXPECT(outchain != NULL);
    }
    block_ChainRelease(outchain);

    EXPECT(!!params->b_extra == !!p->fmt_out.i_extra);

    delete_packetizer(p);

    heimdall_stream_Delete(s);

    return OK;
}
