/*****************************************************************************
 * stream_io_callback.hpp : matroska demuxer
 *****************************************************************************
 * Copyright (C) 2003-2004 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Steve Lhomme <steve.lhomme@free.fr>
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

#ifndef HEIMDALL_MKV_STREAM_IO_CALLBACK_HPP_
#define HEIMDALL_MKV_STREAM_IO_CALLBACK_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_demux.h>

#include <ebml/IOCallback.h>
#include <ebml/EbmlStream.h>

using namespace libebml;

namespace mkv {

/*****************************************************************************
 * Stream management
 *****************************************************************************/
class heimdall_stream_io_callback: public IOCallback
{
  private:
    stream_t       *s;
    bool           mb_eof;
    bool           b_owner;

  public:
    heimdall_stream_io_callback( stream_t *, bool owner );

    virtual ~heimdall_stream_io_callback()
    {
        if( b_owner )
            heimdall_stream_Delete( s );
    }

    bool IsEOF() const { return mb_eof; }

    uint32_t read            ( void *p_buffer, size_t i_size) override;
    void     setFilePointer  ( int64_t i_offset, seek_mode mode = seek_beginning ) override;
    size_t   write           ( const void *p_buffer, size_t i_size) override;
    uint64_t getFilePointer  ( void ) override;
    void     close           ( void ) override { return; }
};

class matroska_iostream_c : public EbmlStream
{
public:
    matroska_iostream_c(heimdall_stream_io_callback & io)
        :EbmlStream(io)
    {}

    inline heimdall_stream_io_callback & I_O() {
        return static_cast<heimdall_stream_io_callback &>(EbmlStream::I_O());
    }

private:
    // hide generic method
    using EbmlStream::I_O;
};


} // namespace

#endif
