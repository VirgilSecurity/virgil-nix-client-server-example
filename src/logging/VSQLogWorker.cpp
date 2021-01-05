//  Copyright (C) 2015-2020 Virgil Security, Inc.
//
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      (1) Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//      (2) Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in
//      the documentation and/or other materials provided with the
//      distribution.
//
//      (3) Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
//  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
//  Lead Maintainer: Virgil Security Inc. <support@virgilsecurity.com>

#include "logging/VSQLogWorker.h"

#include "CustomerEnv.h"
#include "Settings.h"

#include <QTextStream>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>

#include <stdio.h>

constexpr const qint64 LOG_MAX_FILESIZE = 20 * 1024 * 1024;

using namespace vm;

VSQLogWorker::VSQLogWorker(QObject *parent)
    : QObject(parent), m_logFile(), m_logFileIndex(0)
{
}

void VSQLogWorker::processMessage(QtMsgType type, const VSQMessageLogContext &context, const QString &message)
{
    consoleMessageHandler(type, context, message);
    fileMessageHandler(type, context, message);
#ifdef QT_DEBUG
#endif
}

void VSQLogWorker::fileMessageHandler(QtMsgType type, const VSQMessageLogContext &context, const QString &message)
{

    auto formattedMessage = QString("%1 [%2] [%3:%4] %5").arg(formatLogType(type))
                                                            .arg(context.category)
                                                            .arg(context.fileName)
                                                            .arg(context.line)
                                                            .arg(message);
    logToFile(formattedMessage);
}

void VSQLogWorker::consoleMessageHandler(QtMsgType type, const VSQMessageLogContext &context, const QString &message)
{
    auto formattedMessage = QString("%1 [%2] %3").arg(formatLogType(type))
                                                    .arg(context.category)
                                                    .arg(message);
    logToConsole(formattedMessage);
}


bool VSQLogWorker::prepareLogFile(qint64 messageLen) {
    if (messageLen > LOG_MAX_FILESIZE) {
        return false;
    }

    if (!m_logFile.isOpen()) {
        // Open log file
        const auto fileName = QCoreApplication::applicationName() + "_" + QString::number(m_logFileIndex) + ".log";
        m_logFile.setFileName(Settings::logsDir().filePath(fileName));

        if (!m_logFile.open(QIODevice::WriteOnly)) {
            return false;
        }

        m_logFileIndex = (0 == m_logFileIndex) ? 1 : 0;

    } else if (m_logFile.size() > LOG_MAX_FILESIZE - messageLen) {
        // Rotate log file.
        m_logFile.close();

        return prepareLogFile(messageLen);
    }

    return true;
}


void VSQLogWorker::logToFile(const QString& formattedMessage)
{
    if (!prepareLogFile(formattedMessage.size())) {
        return;
    }

    QTextStream{&m_logFile} << formattedMessage << "\n";
}


void VSQLogWorker::logToConsole(const QString& formattedMessage)
{
    fflush(stdout);
    QFile consoleFile;
    if (consoleFile.open(stderr, QIODevice::WriteOnly)) {
        QTextStream{&consoleFile} << formattedMessage << "\n";
    }
}

QString VSQLogWorker::formatLogType(QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return "D:";

    case QtInfoMsg:
        return "I:";

    case QtWarningMsg:
        return "W:";

    case QtCriticalMsg:
        return "C:";

    case QtFatalMsg:
        return "F:";

    default:
        throw std::logic_error("Invalid Qt message type");
        return "";
    }
}
