/*****************************************************************************
 * AES3Buffer.hpp: AES3 audio buffer
 *****************************************************************************
 * Copyright © 2018 VideoLabs, VideoLAN and VideoLAN Authors
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
#ifndef AES3AUDIO_HPP
#define AES3AUDIO_HPP

#include <heimdall_common.h>
#include <heimdall_threads.h>
#include <heimdall_block.h>
#include <heimdall_block_helper.h>
#include <heimdall_es.h>

#define MAX_AES3_AUDIO_FRAMES     8
#define MAX_AES3_AUDIO_SUBFRAMES (MAX_AES3_AUDIO_FRAMES * 2)

namespace sdi_sout
{
    class AES3AudioSubFrameIndex
    {
        public:
            AES3AudioSubFrameIndex(uint8_t = MAX_AES3_AUDIO_SUBFRAMES);
            uint8_t index() const;
            bool isValid() const;
        private:
            uint8_t subframeindex;
    };

    class AES3AudioBuffer
    {
        public:
            AES3AudioBuffer(heimdall_object_t *, unsigned = 0);
            ~AES3AudioBuffer();
            void setSubFramesCount(uint8_t);
            heimdall_tick_t bufferStart() const;
            unsigned availableVirtualSamples(heimdall_tick_t) const;
            unsigned alignedInterleaveInSamples(heimdall_tick_t, unsigned) const;
            void push(block_t *);
            unsigned read(void *, unsigned, heimdall_tick_t,
                          const AES3AudioSubFrameIndex &,
                          const AES3AudioSubFrameIndex &, unsigned);
            void flushConsumed();
            void tagVirtualConsumed(heimdall_tick_t, unsigned);
            void forwardTo(heimdall_tick_t);
            void setCodec(heimdall_fourcc_t);
            heimdall_fourcc_t getCodec() const;

        private:
            heimdall_object_t *obj;
            void tagConsumed(unsigned);
            size_t   FramesToBytes(unsigned) const;
            heimdall_tick_t FramesToDuration(unsigned) const;
            int OffsetToBufferStart(heimdall_tick_t t) const;
            unsigned BytesToFrames(size_t) const;
            unsigned TicksDurationToFrames(heimdall_tick_t) const;
            block_bytestream_t bytestream;
            mutable heimdall_mutex_t bytestream_mutex;
            uint8_t buffersubframes;
            unsigned toconsume;
            heimdall_fourcc_t i_codec;
    };

    class AES3AudioSubFrameSource
    {
        public:
            AES3AudioSubFrameSource();
            AES3AudioSubFrameSource(AES3AudioBuffer *, AES3AudioSubFrameIndex);
            heimdall_tick_t bufferStartTime() const;
            unsigned copy(void *, unsigned count, heimdall_tick_t,
                          const AES3AudioSubFrameIndex &, unsigned width);
            void flushConsumed();
            void tagVirtualConsumed(heimdall_tick_t, unsigned);
            void forwardTo(heimdall_tick_t t);
            unsigned availableVirtualSamples(heimdall_tick_t) const;
            unsigned alignedInterleaveInSamples(heimdall_tick_t, unsigned) const;
            bool available() const;
            heimdall_fourcc_t getCodec() const;

        private:
            AES3AudioBuffer *aes3AudioBuffer;
            AES3AudioSubFrameIndex bufferSubFrameIdx; /* alias channel */
    };

    class AES3AudioFrameSource
    {
        public:
            AES3AudioFrameSource();
            heimdall_tick_t bufferStartTime() const;
            unsigned samplesUpToTime(heimdall_tick_t) const;
            unsigned availableVirtualSamples(heimdall_tick_t) const;
            unsigned alignedInterleaveInSamples(heimdall_tick_t, unsigned) const;
            void flushConsumed();
            void tagVirtualConsumed(heimdall_tick_t, unsigned);
            void forwardTo(heimdall_tick_t t);
            AES3AudioSubFrameSource subframe0;
            AES3AudioSubFrameSource subframe1;
    };

}

#endif // AES3AUDIO_HPP
