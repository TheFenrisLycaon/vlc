/*****************************************************************************
 * Thumbnailer.cpp: medialibrary thumbnailer implementation using libheimdallcore
 *****************************************************************************
 * Copyright © 2018 VLC authors, VideoLAN and VideoLabs
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "medialibrary.h"

#include <heimdall_fs.h>
#include <heimdall_block.h>
#include <heimdall_url.h>
#include <heimdall_cxx_helpers.hpp>
#include <heimdall_preparser.h>

#include <climits>
#include <stdexcept>

Thumbnailer::Thumbnailer( heimdall_medialibrary_module_t* ml )
    : m_currentContext(nullptr)
    , m_thumbnailer(HEIMDALL_OBJECT(ml), {
        .types = HEIMDALL_PREPARSER_TYPE_THUMBNAIL_TO_FILES,
        .max_parser_threads = 0,
        .max_thumbnailer_threads = 1,
        .timeout = HEIMDALL_TICK_FROM_SEC( 3 ),
#if !defined(HAVE_HEIMDALL_EXTERNAL_PREPARSER)
        .external_process = false,
#else
        .external_process = true,
#endif
    })
{
}

void Thumbnailer::onThumbnailToFilesComplete(heimdall_preparser_req *req, int ,
                                             const bool *result_array,
                                             size_t result_count, void *data)
{
    ThumbnailerCtx* ctx = static_cast<ThumbnailerCtx*>( data );

    heimdall::threads::mutex_locker lock( ctx->thumbnailer->m_mutex );
    ctx->done = true;
    if (result_count != 1) {
        ctx->error = true;
    } else {
        ctx->error = !result_array[0];
    }
    ctx->thumbnailer->m_currentContext = nullptr;
    ctx->thumbnailer->m_cond.broadcast();
    heimdall_preparser_req_Release( req );
}

bool Thumbnailer::generate( const medialibrary::IMedia&, const std::string& mrl,
                            uint32_t desiredWidth, uint32_t desiredHeight,
                            float position, const std::string& dest )
{
#if INT_MAX < UINT32_MAX
    assert(desiredWidth < (uint32_t)INT_MAX);
    assert(desiredHeight < (uint32_t)INT_MAX);
#endif

    ThumbnailerCtx ctx{};

    auto item = heimdall::wrap_cptr( input_item_New( mrl.c_str(), nullptr ),
                                &input_item_Release );
    if ( unlikely( item == nullptr ) )
        return false;

    ctx.done = false;
    ctx.thumbnailer = this;
    ctx.error = true;
    {
        heimdall::threads::mutex_locker lock( m_mutex );
        m_currentContext = &ctx;
        struct heimdall_thumbnailer_arg thumb_arg = {
            .seek = {
                .type = heimdall_thumbnailer_arg::seek::HEIMDALL_THUMBNAILER_SEEK_POS,
                .pos = position,
                .speed = heimdall_thumbnailer_arg::seek::HEIMDALL_THUMBNAILER_SEEK_FAST,
            },
            .hw_dec = false,
        };

        struct heimdall_thumbnailer_output thumb_out = {
            .format = HEIMDALL_THUMBNAILER_FORMAT_JPEG,
            .width = (int)desiredWidth,
            .height = (int)desiredHeight,
            .crop = true,
            .file_path = dest.c_str(),
            .creat_mode = 0600,
        };

        static const struct heimdall_thumbnailer_to_files_cbs cbs = {
            .on_ended = onThumbnailToFilesComplete,
        };

        heimdall_preparser_t *thumbnailer = m_thumbnailer.instance();
        if (thumbnailer == nullptr) {
            return false;
        }

        heimdall_preparser_req *preparserReq =
            heimdall_preparser_req_NewThumbnailToFiles(thumbnailer, item.get(),
                                                  &thumb_arg, &thumb_out, 1,
                                                  &cbs, &ctx);
        if (preparserReq == nullptr)
        {
            m_currentContext = nullptr;
            return false;
        }

        if (heimdall_preparser_Submit(thumbnailer, preparserReq) != HEIMDALL_SUCCESS)
        {
            heimdall_preparser_req_Release(preparserReq);
            m_currentContext = nullptr;
            return false;
        }

        while ( ctx.done == false )
            m_cond.wait( m_mutex );
        m_currentContext = nullptr;
    }

    return !ctx.error;
}

void Thumbnailer::stop()
{
    heimdall_preparser_t *thumbnailer = m_thumbnailer.get();
    if (thumbnailer == nullptr) {
        return;
    }

    /* heimdall_preparser_Cancel can call the callback from this thread so the mutex
     * must be unlock */
    heimdall_preparser_Cancel(thumbnailer, nullptr);

    heimdall::threads::mutex_locker lock(m_mutex);
    if ( m_currentContext != nullptr )
    {
        while (m_currentContext != nullptr && m_currentContext->done == false)
            m_cond.wait( m_mutex );
    }
}
