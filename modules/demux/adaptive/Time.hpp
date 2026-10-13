/*****************************************************************************
 * Time.hpp: Adaptative streaming time definitions
 *****************************************************************************
 * Copyright © 2015 - VideoLAN and VLC Authors
 * Copyright (C) 2026 TheFenrisLycaon
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
#ifndef TIME_HPP
#define TIME_HPP

#include <heimdall_common.h>
#include <heimdall_tick.h>
#include <list>

namespace adaptive
{

/* Scaled time */
using stime_t = int64_t;

class Timescale
{
    public:
        Timescale(uint64_t v = 0) : scale(v) {}

        heimdall_tick_t ToTime(stime_t t) const
        {
            if( !scale ) return 0;
            stime_t v = t / scale;
            stime_t r = t % scale;
            return v * CLOCK_FREQ + r * CLOCK_FREQ / scale;
        }

        stime_t ToScaled(heimdall_tick_t t) const
        {
            int64_t v = t / CLOCK_FREQ;
            int64_t r = t % CLOCK_FREQ;
            return v * scale + r * scale / CLOCK_FREQ;
        }

        bool isValid() const { return !!scale; }
        operator uint64_t() const { return scale; }

    private:
        uint64_t scale;
};

class SegmentTimes
{
    public:
        SegmentTimes()
        {
            demux = HEIMDALL_TICK_INVALID;
            media = HEIMDALL_TICK_INVALID;
            display = HEIMDALL_TICK_INVALID;
        }
        SegmentTimes(heimdall_tick_t a, heimdall_tick_t b, heimdall_tick_t c = HEIMDALL_TICK_INVALID)
        {
            demux = a;
            media = b;
            display = c;
        }
        void offsetBy(heimdall_tick_t v)
        {
            if(v == 0)
                return;
            if(demux != HEIMDALL_TICK_INVALID)
                demux += v;
            if(media != HEIMDALL_TICK_INVALID)
                media += v;
            if(display != HEIMDALL_TICK_INVALID)
                display += v;
        }
        heimdall_tick_t demux;
        heimdall_tick_t media;
        heimdall_tick_t display;
};

class Times
{
    public:
        Times()
        {
            continuous = HEIMDALL_TICK_INVALID;
        }
        Times(const SegmentTimes &s, heimdall_tick_t a)
        {
            segment = s;
            continuous = a;
        }
        void offsetBy(heimdall_tick_t v)
        {
            if(continuous != HEIMDALL_TICK_INVALID)
                continuous += v;
            segment.offsetBy(v);
        }
        heimdall_tick_t continuous;
        SegmentTimes segment;
};

using SynchronizationReference = std::pair<uint64_t, Times>;

class SynchronizationReferences
{
    public:
        SynchronizationReferences() = default;
        void addReference(uint64_t seq, const Times &t)
        {
            for(auto &r : refs)
                if(r.first == seq)
                {
                    /* update reference when the timestamps are really old to prevent false roll */
                    constexpr heimdall_tick_t quarterroll = (INT64_C(0x1FFFFFFFF) * 100 / 9) >> 2;
                    if(t.continuous - r.second.continuous > quarterroll)
                        r.second = t;
                    return;
                }
            while(refs.size() > 10)
                refs.pop_back();
            refs.push_front(SynchronizationReference(seq, t));
        }
        bool getReference(uint64_t seq, heimdall_tick_t,
                          SynchronizationReference &ref) const
        {
            for(auto t : refs)
            {
                if(t.first != seq)
                    continue;
                ref = t;
                return true;
            }
            return false;
        }

    private:
        std::list<SynchronizationReference> refs;
};

}

#endif // TIME_HPP

