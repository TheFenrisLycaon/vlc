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
#ifndef HEIMDALLTICK_HPP
#define HEIMDALLTICK_HPP

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QObject>
#include <heimdall_common.h>
#include <heimdall_tick.h>
#include <QtQml/qqmlregistration.h>

class HeimdallTick
{
    Q_GADGET
public:
    enum FormatFlag {
        SubSecondFormattedAsMS = 1
    };
    Q_ENUM(FormatFlag)

    Q_INVOKABLE virtual bool valid() const = 0;

    Q_INVOKABLE bool isSubSecond() const;

    Q_INVOKABLE bool isSubHour() const;

    /**
     * @brief formatHMS
     * @param formatFlags flags to specialize formatting, default is SubSecondFormattedAsMS for legacy reasons
     * @return time as HH:MM:SS
     *
     * this method should be used to present running time or
     * time that will be compared to a running time
     *
     * milliseconds will be truncated towards 0
     */
    Q_INVOKABLE QString formatHMS(int formatFlags = SubSecondFormattedAsMS) const;

    /**
     * @brief formatLong
     * @param formatFlags flags to specialize formatting, default is SubSecondFormattedAsMS for legacy reasons
     * @return time in literal form
     * 1h 2min
     * 5 min
     * 10 sec
     * 43 ms
     */
    Q_INVOKABLE QString formatLong(int formatFlags = SubSecondFormattedAsMS) const;

    /**
     * @brief formatShort
     * @param formatFlags flags to specialize formatting, default is SubSecondFormattedAsMS for legacy reasons
     * @return time in literal form
     * 1h02
     * 02:42
     * 43 ms
     */
    Q_INVOKABLE QString formatShort(int formatFlags = SubSecondFormattedAsMS) const;

    inline heimdall_tick_t toHeimdallTick() const {
        return m_ticks;
    }

    Q_INVOKABLE int toMinutes() const;
    Q_INVOKABLE int toSeconds() const;
    Q_INVOKABLE int toHours()   const;
    int toMilliseconds() const;

protected:
    HeimdallTick(heimdall_tick_t ticks);
    HeimdallTick() = delete;

    virtual int64_t asMilliseconds() const = 0;
    virtual int64_t asSeconds() const = 0;

    heimdall_tick_t m_ticks;
};

class HeimdallDuration final : public HeimdallTick
{
    Q_GADGET
    QML_VALUE_TYPE(heimdallDuration)

public:
    HeimdallDuration();
    HeimdallDuration(heimdall_tick_t);

    Q_INVOKABLE static HeimdallDuration sum(const QList<HeimdallDuration>& durations)
    {
        return std::reduce(durations.constBegin(), durations.constEnd());
    }

    HeimdallDuration operator*(double) const;
    HeimdallDuration operator+(const HeimdallDuration &) const;
    HeimdallDuration& operator+=(const HeimdallDuration &);
    HeimdallDuration operator-(const HeimdallDuration &) const;
    HeimdallDuration& operator-=(const HeimdallDuration &);
    bool operator==(const HeimdallDuration &) const;
    bool operator>(const HeimdallDuration &) const;

    double toSecf() const;

    Q_INVOKABLE HeimdallDuration scale(float) const;

    static HeimdallDuration fromMS(int64_t ms);

    int64_t asMilliseconds() const override;
    int64_t asSeconds() const override;
    bool valid() const override;

private:
    bool m_valid;
};

class HeimdallTime final : public HeimdallTick
{
    Q_GADGET
    QML_VALUE_TYPE(heimdallTime)
public:
    HeimdallTime();
    HeimdallTime(heimdall_tick_t);
    HeimdallTime(HeimdallDuration);
    HeimdallDuration operator-(const HeimdallTime &rhs) const;
    bool operator<=(const HeimdallTime &) const;

    Q_INVOKABLE HeimdallTime scale(float) const;

    int64_t asMilliseconds() const override;
    int64_t asSeconds() const override;
    bool valid() const override;
};

//following boilerplate allow registering FormatFlag enum in the HeimdallTick namespace in QML
//HeimdallTick is not a QML_VALUE_TYPE so we don't need to define a distinct type to
//register it in the namespace

namespace HeimdallTickForeign
{
    Q_NAMESPACE
    QML_NAMED_ELEMENT(HeimdallTick)
    QML_FOREIGN_NAMESPACE(HeimdallTick)
}

#endif // HEIMDALLTICK_HPP
