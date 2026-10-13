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
#ifndef HEIMDALLACCESSIMAGEPROVIDER_HPP
#define HEIMDALLACCESSIMAGEPROVIDER_HPP

#include <QObject>
#include <QQuickAsyncImageProvider>
#include <QString>
#include <QIODevice>

typedef struct stream_t stream_t;

/**
 * HeimdallIODevice is a QIODevice based on heimdall_access
 */
class HeimdallIODevice : public QIODevice
{
public:
    HeimdallIODevice(const QString &filename, QObject *parent = nullptr);

    virtual ~HeimdallIODevice();

public:
    bool open(QIODevice::OpenMode mode) override;

    bool isSequential() const override;

    void close() override;

    qint64 pos() const override;

    qint64 size() const override;

    bool seek(qint64 pos) override;

    bool atEnd() const override;

    bool reset() override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;

    qint64 writeData(const char*, qint64) override;

private:
    QString m_filename;
    stream_t* m_stream = nullptr;
};

class HeimdallAccessImageProvider: public QQuickAsyncImageProvider
{
public:
    typedef std::function<QImage (QImage&, const QSize &)> ImagePostProcessCb;

    HeimdallAccessImageProvider(ImagePostProcessCb cb = nullptr);

    QQuickImageResponse* requestImageResponse(const QString &id, const QSize &requestedSize) override;

    static QString wrapUri(QString path);

    static QQuickImageResponse* requestImageResponseUnWrapped(const QUrl, const QSize &requestedSize, ImagePostProcessCb cb = nullptr);

private:
    ImagePostProcessCb postProcessCb;
};

class HeimdallAccessImage : public QObject {
    Q_OBJECT
public:
    HeimdallAccessImage(QObject* parent = nullptr);

    /**
     * @brief adapt @path to open it using
     * @param path to the artwork
     *
     * sample usage:
     *
     * @code{qml}
     *   Image {
     *     src: HeimdallImageAccess.uri("file:///path/to/assert.svg")
     *   }
     * @code
     *
     */
    Q_INVOKABLE QString uri(const QString& path, bool excludeLocalFileOrUnknownScheme = true);

};

#endif // HEIMDALLACCESSIMAGEPROVIDER_HPP
