/*****************************************************************************
 * DBMSDIOutput.hpp: Decklink SDI Output
 *****************************************************************************
 * Copyright © 2014-2016 VideoLAN and VideoLAN Authors
 *                  2018 VideoLabs
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
#ifndef DBMSDIOUTPUT_HPP
#define DBMSDIOUTPUT_HPP

#include "SDIOutput.hpp"

#include <heimdall_es.h>
#include <heimdall_threads.h>
#include "../../access/heimdall_decklink.h"

namespace sdi_sout
{
    class DBMSDIOutput : public SDIOutput,
                         public IDeckLinkVideoOutputCallback
    {
        public:
            DBMSDIOutput(sout_stream_t *);
            ~DBMSDIOutput();
            virtual AbstractStream *Add(const es_format_t *); /* reimpl */
            virtual int Open(); /* impl */
            virtual int Process(); /* impl */

            virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID, LPVOID *);
            virtual ULONG STDMETHODCALLTYPE AddRef ();
            virtual ULONG STDMETHODCALLTYPE Release ();
            virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted (IDeckLinkVideoFrame *, BMDOutputFrameCompletionResult);
            virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped (void);

        protected:
            int ProcessVideo(picture_t *, block_t *);
            int ProcessAudio(block_t *);
            virtual int ConfigureVideo(const video_format_t *); /* impl */
            virtual int ConfigureAudio(const audio_format_t *); /* impl */

        private:
            IDeckLink *p_card;
            IDeckLinkOutput *p_output;

            BMDTimeScale timescale;
            BMDTimeValue frameduration;
            heimdall_tick_t lasttimestamp;
            /* XXX: workaround card clock drift */
            struct
            {
                heimdall_tick_t offset; /* > 0 if clock card is slower */
                heimdall_tick_t system_reference;
                BMDTimeValue hardware_reference;
            } clock;
            bool b_running;
            heimdall_tick_t streamStartTime;
            int StartPlayback();
            struct
            {
                heimdall_mutex_t lock; /* Driver calls callback... until buffer is empty :/ */
                heimdall_cond_t cond;
                heimdall_thread_t thread;
                bool canceled;
            } feeder;
            static void *feederThreadCallback(void *);
            void feederThread();
            int doSchedule();
            int doProcessVideo(picture_t *, block_t *);
            int FeedOneFrame();
            int FeedAudio(heimdall_tick_t, heimdall_tick_t, bool);
            void checkClockDrift();
            bool isDrained();
    };
}

#endif // DBMSDIOUTPUT_HPP
