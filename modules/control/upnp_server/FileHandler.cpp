/*****************************************************************************
 * FileHandler.cpp
 *****************************************************************************
 * Copyright © 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Alaric Senat <alaric@videolabs.io>
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
#include "config.h"
#endif

#include <ctime>
#include <sstream>

#include <heimdall_common.h>

#include <heimdall_addons.h>
#include <heimdall_cxx_helpers.hpp>
#include <heimdall_fourcc.h>
#include <heimdall_interface.h>
#include <heimdall_player.h>
#include <heimdall_rand.h>
#include <heimdall_stream.h>
#include <heimdall_stream_extractor.h>
#include <heimdall_url.h>

#include "FileHandler.hpp"
#include "ml.hpp"
#include "utils.hpp"

// static constexpr char DLNA_TRANSFER_MODE[] = "transfermode.dlna.org";
static constexpr char DLNA_CONTENT_FEATURE[] = "contentfeatures.dlna.org";
// static constexpr char DLNA_TIME_SEEK_RANGE[] = "timeseekrange.dlna.org";

/// Convenient C++ replacement of heimdall_ml_file_t to not have to deal with allocations
struct MLFile
{
    std::string mrl;
    int64_t size;
    time_t last_modification;
};

/// Usual filesystem FileHandler implementation, this is the most commonly used FileHandler, for
/// non-transcoded local medias, thumbnails and subs.
class MLFileHandler : public FileHandler
{
  public:
    MLFile file;
    utils::MimeType mime_type;

    std::unique_ptr<stream_t, decltype(&heimdall_stream_Delete)> stream = {nullptr, &heimdall_stream_Delete};

    MLFileHandler(MLFile &&file, utils::MimeType &&mime_type) :
        file(std::move(file)),
        mime_type(std::move(mime_type))
    {}

    bool get_info(UpnpFileInfo &info) final
    {
        UpnpFileInfo_set_ContentType(&info, mime_type.combine().c_str());
        UpnpFileInfo_set_FileLength(&info, file.size);
        UpnpFileInfo_set_LastModified(&info, file.last_modification);

        // const_cast is expected by the design of the upnp api as it only serves const list heads
        // FIXME: see if there's no way to patch that in libupnp, we shouldn't have to break const
        // to do something so usual.
        auto *head = const_cast<UpnpListHead *>(UpnpFileInfo_get_ExtraHeadersList(&info));
        utils::http::add_response_hdr(head, {DLNA_CONTENT_FEATURE, "DLNA.ORG_OP=01"});
        return true;
    }

    bool open(heimdall_object_t *parent) final
    {
        stream = heimdall::wrap_cptr(heimdall_stream_NewMRL(parent, file.mrl.c_str()), &heimdall_stream_Delete);
        return stream != nullptr;
    }

    size_t read(uint8_t buffer[], size_t buffer_len) noexcept final
    {
        return heimdall_stream_Read(stream.get(), buffer, buffer_len);
    }

    bool seek(SeekType type, off_t offset) noexcept final
    {
        uint64_t real_offset;
        switch (type)
        {
            case SeekType::Current:
                real_offset = heimdall_stream_Tell(stream.get()) + offset;
                break;
            case SeekType::End:
                if (heimdall_stream_GetSize(stream.get(), &real_offset) != HEIMDALL_SUCCESS)
                {
                    return false;
                }
                real_offset += offset;
                break;
            case SeekType::Set:
                real_offset = offset;
                break;
            default:
                return false;
        }

        return heimdall_stream_Seek(stream.get(), real_offset) == 0;
    }
};

//
// Url parsing and FileHandler Factory
//

template <typename MLHelper>
auto get_ml_object(const std::string &token,
                   std::string &extension,
                   const ml::MediaLibraryContext &ml)
{
    const auto extension_idx = token.find('.');
    extension = token.substr(extension_idx + 1);

    try
    {
        const int64_t ml_id = std::stoll(token.substr(0, extension_idx));
        return MLHelper::get(ml, ml_id);
    }
    catch (const std::exception &)
    {
        return typename MLHelper::Ptr{nullptr, nullptr};
    }
}

static std::unique_ptr<FileHandler> parse_media_url(std::stringstream &ss,
                                                    const ml::MediaLibraryContext &ml)
{
    std::string token;
    std::getline(ss, token, '/');

    if (token != "native")
    {
        // TODO Select a transcode profile
    }

    std::getline(ss, token);

    std::string extension;
    const auto media = get_ml_object<ml::Media>(token, extension, ml);
    if (media == nullptr)
    {
        return nullptr;
    }

    const auto main_files = utils::get_media_files(*media, HEIMDALL_ML_FILE_TYPE_MAIN);
    if (main_files.empty())
    {
        return nullptr;
    }
    const heimdall_ml_file_t &main_file = main_files.front();

    auto mime_type = utils::get_mimetype(media->i_type, extension);
    auto ret = std::make_unique<MLFileHandler>(
        MLFile{main_file.psz_mrl, main_file.i_size, main_file.i_last_modification_date},
        std::move(mime_type));
    return ret;
}

static std::unique_ptr<FileHandler> parse_thumbnail_url(std::stringstream &ss,
                                                        const ml::MediaLibraryContext &ml)
{
    std::string token;
    std::getline(ss, token, '/');
    heimdall_ml_thumbnail_size_t size;
    if (token == "small")
        size = HEIMDALL_ML_THUMBNAIL_SMALL;
    else if (token == "banner")
        size = HEIMDALL_ML_THUMBNAIL_BANNER;
    else
        return nullptr;

    std::getline(ss, token, '/');
    std::string extension;
    std::string mrl;
    if (token == "media")
    {
        std::getline(ss, token);
        const auto media = get_ml_object<ml::Media>(token, extension, ml);
        if (media && media->thumbnails[size].i_status == HEIMDALL_ML_THUMBNAIL_STATUS_AVAILABLE)
            mrl = media->thumbnails[size].psz_mrl;
    }
    else if (token == "album")
    {
        std::getline(ss, token);
        const auto album = get_ml_object<ml::Album>(token, extension, ml);
        if (album && album->thumbnails[size].i_status == HEIMDALL_ML_THUMBNAIL_STATUS_AVAILABLE)
            mrl = album->thumbnails[size].psz_mrl;
    }

    if (mrl.empty())
    {
        return nullptr;
    }

    return std::make_unique<MLFileHandler>(MLFile{mrl, -1, 0}, utils::MimeType{"image", extension});
}

static std::unique_ptr<FileHandler> parse_subtitle_url(std::stringstream &ss,
                                                       const ml::MediaLibraryContext &ml)
{
    std::string token;
    std::string extension;
    std::string mrl;

    std::getline(ss, token);
    const auto media = get_ml_object<ml::Media>(token, extension, ml);
    if (media == nullptr)
    {
        return nullptr;
    }

    const auto subtitles = utils::get_media_files(*media, HEIMDALL_ML_FILE_TYPE_SUBTITLE);
    if (subtitles.empty())
    {
        return nullptr;
    }

    const heimdall_ml_file_t &sub = subtitles.front();
    return std::make_unique<MLFileHandler>(
        MLFile{sub.psz_mrl, sub.i_size, sub.i_last_modification_date},
        utils::MimeType{"text", extension});
}

std::unique_ptr<FileHandler> parse_url(const char *url, const ml::MediaLibraryContext &ml)
{
    std::stringstream ss(url);

    std::string token;
    std::getline(ss, token, '/');
    if (!token.empty())
        return nullptr;

    std::getline(ss, token, '/');
    if (token == "media")
        return parse_media_url(ss, ml);
    else if (token == "thumbnail")
        return parse_thumbnail_url(ss, ml);
    else if (token == "subtitle")
        return parse_subtitle_url(ss, ml);
    return nullptr;
}
