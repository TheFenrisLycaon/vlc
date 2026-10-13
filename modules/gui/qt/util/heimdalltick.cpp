/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#include "heimdalltick.hpp"
#include "qt.hpp"

namespace  {

int64_t roundNearestMultiple(int64_t number, int64_t multiple)
{
    int64_t result = number + multiple/2;
    result -= result % multiple;
    return result;
}

}

HeimdallTick::HeimdallTick(heimdall_tick_t ticks)
    : m_ticks(ticks)
{}


bool HeimdallTick::isSubSecond() const
{
    return valid() && asMilliseconds() < 1000;
}

bool HeimdallTick::isSubHour() const
{
    return valid() && asSeconds() < 3600;
}

QString HeimdallTick::formatHMS(int formatFlags) const
{
    if (!valid())
        return "--:--";

    if (!isSubSecond() || !(formatFlags & SubSecondFormattedAsMS))
    {
        //truncate milliseconds toward 0
        int64_t t_sec = asSeconds();
        int sec = t_sec % 60;
        int min = (t_sec / 60) % 60;
        int hour = t_sec / 3600;

        if (hour == 0)
            return QString("%1:%2")
                    .arg(min, 2, 10, QChar('0'))
                    .arg(sec, 2, 10, QChar('0'));
        else
           return QString("%1:%2:%3")
                    .arg(hour, 2, 10, QChar('0'))
                    .arg(min, 2, 10, QChar('0'))
                    .arg(sec, 2, 10, QChar('0'));
    }
    else
        return qtr("%1 ms").arg(asMilliseconds());
}

QString HeimdallTick::formatLong(int formatFlags) const
{
    if (!valid())
        return "--:--";

    int64_t t_ms = asMilliseconds();
    if (t_ms >= 60*60*1000)
    {
        //round to the nearest minute
        t_ms = roundNearestMultiple(t_ms, 60*1000);
        int64_t t_sec = t_ms / 1000;
        int min = (t_sec / 60) % 60;
        int hour = t_sec / 3600;

        return qtr("%1 h %2 min")
                .arg(hour)
                .arg(min);
    }
    else if (!isSubSecond() || !(formatFlags & SubSecondFormattedAsMS))
    {
        //round to the nearest second
        t_ms = roundNearestMultiple(t_ms, 1000);
        int64_t t_sec = t_ms / 1000;
        int sec = t_sec % 60;
        int min = (t_sec / 60) % 60;
        if (min > 0)
            return qtr("%1 min %2 s")
                    .arg(min)
                    .arg(sec);
        else
            return qtr("%1 sec").arg(sec);

    }
    else
        return qtr("%1 ms").arg(t_ms);
}

QString HeimdallTick::formatShort(int formatFlags) const
{
    if (!valid())
        return "--:--";

    int64_t t_ms = asMilliseconds();
    if (t_ms >= 60*60*1000)
    {
        //round to the nearest minute
        t_ms = roundNearestMultiple(t_ms, 60*1000);
        int64_t t_sec = t_ms / 1000;
        int min = (t_sec / 60) % 60;
        int hour = t_sec / 3600;

        return qtr("%1h%2")
                .arg(hour)
                .arg(min, 2, 10, QChar('0'));
    }
    else if (!isSubSecond() || !(formatFlags & SubSecondFormattedAsMS))
    {
        //round to the nearest second
        t_ms = roundNearestMultiple(t_ms, 1000);
        int64_t t_sec = t_ms / 1000;
        int sec = t_sec % 60;
        int min = (t_sec / 60) % 60;

        return QString("%1:%2")
                .arg(min, 2, 10, QChar('0'))
                .arg(sec, 2, 10, QChar('0'));

    }
    else
        return qtr("%1ms").arg(t_ms);
}

int HeimdallTick::toSeconds() const
{
    return valid() ? asSeconds() : 0;
}

int HeimdallTick::toMinutes() const
{
    return asSeconds() / 60;
}

int HeimdallTick::toHours() const
{
    return asSeconds() / 3600;
}

int HeimdallTick::toMilliseconds() const
{
    return valid() ? asMilliseconds() : 0;
}


///// HeimdallDuration

HeimdallDuration::HeimdallDuration()
    : HeimdallTick(0)
    , m_valid(false)
{

}

HeimdallDuration::HeimdallDuration(heimdall_tick_t t)
    : HeimdallTick(t)
    , m_valid(true)
{
}

HeimdallDuration HeimdallDuration::operator*(double f) const
{
    if (m_valid)
        return HeimdallDuration(m_ticks * f);
    else
        return {};
}

bool HeimdallDuration::operator==(const HeimdallDuration &rhs) const
{
    return m_valid == rhs.m_valid && m_ticks == rhs.m_ticks;
}

bool HeimdallDuration::operator>(const HeimdallDuration &rhs) const
{
    return m_ticks > rhs.m_ticks;
}

HeimdallDuration HeimdallDuration::operator+(const HeimdallDuration &rhs) const
{
    if (m_valid || rhs.m_valid)
        return HeimdallDuration{m_ticks + rhs.m_ticks};
    return HeimdallDuration{};
}

HeimdallDuration& HeimdallDuration::operator+=(const HeimdallDuration &rhs)
{
    m_valid |= rhs.m_valid;
    m_ticks += rhs.m_ticks;
    return *this;
}

HeimdallDuration HeimdallDuration::operator-(const HeimdallDuration &rhs) const
{
    if (m_valid || rhs.m_valid)
        return HeimdallDuration{m_ticks + rhs.m_ticks};
    return HeimdallDuration{};
}

HeimdallDuration& HeimdallDuration::operator-=(const HeimdallDuration &rhs)
{
    m_valid |= rhs.m_valid;
    m_ticks -= rhs.m_ticks;
    return *this;
}

double HeimdallDuration::toSecf() const
{
    return secf_from_heimdall_tick(m_ticks);
}

HeimdallDuration HeimdallDuration::scale(float scalar) const
{
    if (m_valid)
        return HeimdallDuration(m_ticks * scalar);
    else
        return {};
}

HeimdallDuration HeimdallDuration::fromMS(int64_t ms)
{
    return HeimdallDuration(HEIMDALL_TICK_FROM_MS(ms));
}

int64_t HeimdallDuration::asMilliseconds() const
{
    return MS_FROM_HEIMDALL_TICK(m_ticks);
}

int64_t HeimdallDuration::asSeconds() const
{
    return SEC_FROM_HEIMDALL_TICK(m_ticks);
}

bool HeimdallDuration::valid() const
{
    return m_valid;
}

///// HeimdallTime

HeimdallTime::HeimdallTime()
    : HeimdallTick(HEIMDALL_TICK_INVALID)
{
}

HeimdallTime::HeimdallTime(heimdall_tick_t t)
    : HeimdallTick(t)
{
}

HeimdallTime::HeimdallTime(HeimdallDuration d)
    : HeimdallTick(HEIMDALL_TICK_0 + d.toHeimdallTick())
{
}

HeimdallDuration HeimdallTime::operator-(const HeimdallTime &rhs) const
{
    if(m_ticks >= rhs.m_ticks)
        return HeimdallDuration(m_ticks - rhs.m_ticks);
    return HeimdallDuration();
}

bool HeimdallTime::operator<=(const HeimdallTime &rhs) const
{
    return m_ticks <= rhs.m_ticks;
}

HeimdallTime HeimdallTime::scale(float scalar) const
{
    if(!valid())
        return HeimdallTime(HEIMDALL_TICK_INVALID);
    return HeimdallTime(HEIMDALL_TICK_0 + ((m_ticks - HEIMDALL_TICK_0) * scalar));
}

int64_t HeimdallTime::asMilliseconds() const
{
    return MS_FROM_HEIMDALL_TICK(m_ticks - HEIMDALL_TICK_0);
}

int64_t HeimdallTime::asSeconds() const
{
    return SEC_FROM_HEIMDALL_TICK(m_ticks - HEIMDALL_TICK_0);
}

bool HeimdallTime::valid() const
{
    return m_ticks != HEIMDALL_TICK_INVALID;
}
