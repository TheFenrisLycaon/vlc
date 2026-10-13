/*****************************************************************************
 * moving_avg.h: Moving average helper
 *****************************************************************************
 * Copyright © 2018 VideoLabs, VideoLAN, VLC authors
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
#ifndef HEIMDALL_MOVING_AVG_H
#define HEIMDALL_MOVING_AVG_H

#ifndef MVA_PACKETS
 #define MVA_PACKETS 6
#endif

struct mva_packet_s
{
    heimdall_tick_t duration;
    heimdall_tick_t dts;
    heimdall_tick_t diff;
};

struct moving_average_s
{
    struct mva_packet_s packets[MVA_PACKETS];
    unsigned i_packet;
};

static void mva_init(struct moving_average_s *m)
{
    m->i_packet = 0;
}

static heimdall_tick_t mva_get(const struct moving_average_s *m);

static void mva_add(struct moving_average_s *m,
                     heimdall_tick_t dts, heimdall_tick_t duration)
{
    m->packets[m->i_packet % MVA_PACKETS].dts = dts;
    m->packets[m->i_packet % MVA_PACKETS].duration = duration;
    m->packets[m->i_packet % MVA_PACKETS].diff = duration; /* will erase, last one */
    m->i_packet++;
}

static heimdall_tick_t mva_get(const struct moving_average_s *m)
{
    unsigned start;
    const struct mva_packet_s *min = NULL, *max = NULL;

    if(likely(m->i_packet >= MVA_PACKETS))
    {
        start = m->i_packet - MVA_PACKETS;
        for(unsigned i = start; i < m->i_packet; i++)
        {
            if(!min || m->packets[i % MVA_PACKETS].diff < min->diff)
                min = &m->packets[i % MVA_PACKETS];

            if(!max || m->packets[i % MVA_PACKETS].diff > max->diff)
                max = &m->packets[i % MVA_PACKETS];
        }
    }
    else start = 0;

    unsigned count = 0;
    heimdall_tick_t avgdiff = 0;
    for(unsigned i = start; i < m->i_packet; i++)
    {
        if(&m->packets[i % MVA_PACKETS] == min ||
            &m->packets[i % MVA_PACKETS] == max)
            continue;
        const heimdall_tick_t diff = m->packets[i % MVA_PACKETS].diff;
        if(diff != 0 || (i+1) < m->i_packet) /* ignore last packet when we have no duration */
        {
            avgdiff += diff;
            count++;
        }
    }

    return count ? avgdiff / count : 0;
}

static struct mva_packet_s * mva_getLastPacket(struct moving_average_s *m)
{
    return m->i_packet > 0 ? &m->packets[(m->i_packet - 1) % MVA_PACKETS] : NULL;
}

static heimdall_tick_t mva_getLastDTS(struct moving_average_s *m)
{
    struct mva_packet_s *p = mva_getLastPacket(m);
    return p ? p->dts : 0;
}

#endif
