/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <heimdall_access.h>
#include <heimdall_stream.h>

#include "heimdallaccess_image_provider.hpp"

#include "util/asynctask.hpp"
#include "qt.hpp"

#include <QImageReader>
#include <QFile>
#include <QQmlFile>
#include <QUrlQuery>

#define PATH_KEY "uri"

namespace {

class ImageReader : public AsyncTask<QImage>
{
public:
    /**
     * @brief ImageReader
     * @param device i/o source to read from
     *
     * @param requestedSize only taken as hint, the Image is resized with PreserveAspectCrop
     *
     * @param radius
     */
    ImageReader(std::unique_ptr<QIODevice> device, QSize requestedSize, HeimdallAccessImageProvider::ImagePostProcessCb postProcessCb)
        : device(std::move(device))
        , requestedSize {requestedSize}
        , postProcessCb {postProcessCb}
    {
    }

    QString errorString() const {
        return errorStr;
    }

    QImage execute() override
    {
        QImageReader reader;
        reader.setDevice(device.get());
        const QSize sourceSize = reader.size();

        if (requestedSize.isValid())
            reader.setScaledSize(sourceSize.scaled(requestedSize, Qt::KeepAspectRatioByExpanding));

        auto img = reader.read();

        if (img.isNull()) {
            errorStr = reader.errorString();
        }

        if (!img.isNull() && postProcessCb)
            img = postProcessCb(img, requestedSize);

        return img;
    }

private:
    std::unique_ptr<QIODevice> device;
    QSize requestedSize;
    QString errorStr;
    HeimdallAccessImageProvider::ImagePostProcessCb postProcessCb;
};


class HeimdallAccessImageResponse : public QQuickImageResponse
{
public:
    HeimdallAccessImageResponse(const QUrl& url, const QSize &requestedSize, HeimdallAccessImageProvider::ImagePostProcessCb postProcessCb = nullptr)
        : imagePostProcessCb(postProcessCb)
    {

        std::unique_ptr<QIODevice> device;
        if (url.scheme().compare(QStringLiteral("qrc"), Qt::CaseInsensitive) == 0)
        {
            QString qrcPath = QQmlFile::urlToLocalFileOrQrc(url);
            device = std::make_unique<QFile>(qrcPath);
        }
        else
        {
            QUrl fileUrl = url;
            if (fileUrl.scheme().isEmpty())
                fileUrl.setScheme("file");
            device = std::make_unique<HeimdallIODevice>(fileUrl.toString(QUrl::FullyEncoded));
        }
        reader.reset(new ImageReader(std::move(device), requestedSize, postProcessCb));

        connect(reader.get(), &ImageReader::result, this, &HeimdallAccessImageResponse::handleImageRead);

        reader->start(*QThreadPool::globalInstance());
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return result.isNull() ? nullptr : QQuickTextureFactory::textureFactoryForImage(result);
    }

    QString errorString() const override
    {
        if (result.isNull() && errorStr.isEmpty())
            return QStringLiteral("Unspecified error.");
        return errorStr;
    }

    void cancel() override
    {
        reader.reset();
        emit finished();
    }

private:
    void handleImageRead()
    {
        result = reader->takeResult();
        errorStr = reader->errorString();
        reader.reset();

        emit finished();
    }

    HeimdallAccessImageProvider::ImagePostProcessCb imagePostProcessCb;
    QImage result;
    TaskHandle<ImageReader> reader;
    QString errorStr;
};

} // anonymous namespace

//// HeimdallAccessImageProvider

HeimdallAccessImageProvider::HeimdallAccessImageProvider(HeimdallAccessImageProvider::ImagePostProcessCb cb)
    : QQuickAsyncImageProvider()
    , postProcessCb(cb)
{
}

QQuickImageResponse* HeimdallAccessImageProvider::requestImageResponse(const QString& id, const QSize& requestedSize)
{
    QUrl url {id};
    QUrlQuery query {url};
    if (!query.hasQueryItem(PATH_KEY))
        return nullptr;

    QString heimdallurl = query.queryItemValue(PATH_KEY, QUrl::FullyDecoded);
    return new HeimdallAccessImageResponse(QUrl::fromEncoded(heimdallurl.toUtf8()), requestedSize, postProcessCb);
}

QString HeimdallAccessImageProvider::wrapUri(QString path)
{
    QUrlQuery query;
    query.addQueryItem(PATH_KEY, path);
    return QStringLiteral("image://heimdallaccess/?") + query.toString(QUrl::FullyEncoded);
}

QQuickImageResponse* HeimdallAccessImageProvider::requestImageResponseUnWrapped(const QUrl url, const QSize &requestedSize, HeimdallAccessImageProvider::ImagePostProcessCb cb)
{
    return new HeimdallAccessImageResponse(url, requestedSize, cb);
}

//// HeimdallImageAccess

HeimdallAccessImage::HeimdallAccessImage(QObject* parent)
    : QObject(parent)
{}

QString HeimdallAccessImage::uri(const QString& path, const bool excludeLocalFileOrUnknownScheme)
{
    if (excludeLocalFileOrUnknownScheme)
    {
        const QUrl url(path);
        if (url.scheme().isEmpty() || url.scheme() == QLatin1String("qrc") || url.scheme() == QLatin1String("file"))
            return path;
    }
    return HeimdallAccessImageProvider::wrapUri(path);
}

//// HeimdallIODevice

HeimdallIODevice::HeimdallIODevice(const QString& filename, QObject* parent)
    : QIODevice(parent)
    , m_filename(filename)
{
}

HeimdallIODevice::~HeimdallIODevice()
{
    close();
}

bool HeimdallIODevice::open(OpenMode mode)
{
    //we only support reading
    if (mode & QIODevice::OpenModeFlag::WriteOnly)
        return false;

    m_stream = heimdall_access_NewMRL(nullptr, qtu(m_filename));
    if (m_stream == nullptr)
        return false;

    return QIODevice::open(mode);
}

bool HeimdallIODevice::isSequential() const
{
    assert(m_stream);
    //some access (like http) will perform really poorly with the way
    //Qt uses the QIODevice (lots of seeks)
    //return !heimdall_stream_CanSeek(m_stream);
    return true;
}

void HeimdallIODevice::close()
{
    if (!m_stream)
        return;
    heimdall_stream_Delete(m_stream);
    m_stream = nullptr;
}

qint64 HeimdallIODevice::pos() const
{
    assert(m_stream);
    return heimdall_stream_Tell(m_stream);
}

qint64 HeimdallIODevice::size() const
{
    assert(m_stream);
    uint64_t streamSize;
    bool ret = heimdall_stream_GetSize(m_stream, &streamSize);
    if (!ret)
        return -1;
    return static_cast<qint64>(streamSize);
}

bool HeimdallIODevice::seek(qint64 pos)
{
    assert(m_stream);
    QIODevice::seek(pos);
    if (pos < 0)
        return false;
    return heimdall_stream_Seek(m_stream, pos) == HEIMDALL_SUCCESS;
}

bool HeimdallIODevice::atEnd() const
{
    assert(m_stream);
    return heimdall_stream_Eof(m_stream);
}

bool HeimdallIODevice::reset()
{
    assert(m_stream);
    return  heimdall_stream_Seek(m_stream, 0) == HEIMDALL_SUCCESS;
}

qint64 HeimdallIODevice::readData(char* data, qint64 maxlen)
{
    assert(m_stream);
    return heimdall_stream_Read(m_stream, data, maxlen);
}

qint64 HeimdallIODevice::writeData(const char*, qint64)
{
    assert(m_stream);
    return -1;
}
