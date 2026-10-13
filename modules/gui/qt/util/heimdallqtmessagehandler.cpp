/*****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "heimdallqtmessagehandler.hpp"

#include <QtGlobal>
#include <QString>
#include <QLoggingCategory>

#include <heimdall_common.h>

static heimdall_object_t* g_intf = nullptr;
static bool g_logQtMessages = false;
static QtMessageHandler g_defaultMessageHandler = nullptr;

static void heimdallQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    //application logs from QML should be logged using heimdall log system
    //"qml" and "js" are the category used by for console.xxxx
    //"default" is for qDebug(), most of our code logs using msg_Dbg but some parts will use qDebug,
    //usually when they don't have access to the heimdall_object. Qt itself uses categorised logger so it
    //should be safe to assume that the default logger is only used by us.
    if (g_logQtMessages
        || qstrcmp(context.category, "default") == 0
        || qstrcmp(context.category, "qml") == 0
        || qstrcmp(context.category, "js") == 0
        || qstrcmp(context.category, "qt.rhi.general") == 0)
    {
        const char *file = context.file ? context.file : "";
        const char *function = context.function ? context.function : "";
        int heimdallLogLevel = 0;
        switch (type)
        {
        case QtDebugMsg:
            heimdallLogLevel = HEIMDALL_MSG_DBG;
            break;
        case QtWarningMsg:
            heimdallLogLevel = HEIMDALL_MSG_WARN;
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            heimdallLogLevel = HEIMDALL_MSG_ERR;
            break;
        case QtInfoMsg:
            heimdallLogLevel = HEIMDALL_MSG_INFO;
            break;
        default:
            heimdallLogLevel = HEIMDALL_MSG_DBG;
        }

        heimdall_object_Log(g_intf, heimdallLogLevel, heimdall_module_name, file, context.line, function, "(%s) %s", context.category,  msg.toUtf8().constData());
    }
    else
    {
        g_defaultMessageHandler(type, context, msg);
    }
};

void setupHeimdallQtMessageHandler(heimdall_object_t* p_intf)
{
    assert(g_intf == nullptr);
    assert(p_intf != nullptr);

    g_intf = p_intf;
    g_logQtMessages = var_InheritBool(p_intf, "qt-verbose");

    g_defaultMessageHandler = qInstallMessageHandler(heimdallQtMessageHandler);
    qSetMessagePattern("[qt] (%{category}) %{message}");

    QString filterRules;

    if (g_logQtMessages)
    {
        filterRules = QStringLiteral("*=true\n"
                                    "qt.*.debug=false\n" /* Qt's own debug messages are way too much verbose */
                                    "qt.rhi.general=true\n" /* Qt RHI general messages are not verbose */
                                    "qt.widgets.painting=false\n" /* Not necessary */);
    }
    else
    {
        filterRules = QStringLiteral("qt.rhi.general=true\n");
    }

    QLoggingCategory::setFilterRules(filterRules);
}

void cleanupHeimdallQtMessageHandler()
{
    assert(g_intf != nullptr);

    qInstallMessageHandler(nullptr);
    g_defaultMessageHandler = nullptr;
    g_intf = nullptr;
}
