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

#ifndef VM_UPLOADFILEOPERATION_H
#define VM_UPLOADFILEOPERATION_H

#include "LoadFileOperation.h"
#include "FileLoader.h"

#include <QPointer>

namespace vm {
class UploadFileOperation : public LoadFileOperation
{
    Q_OBJECT

public:
    UploadFileOperation(NetworkOperation *parent, FileLoader *fileLoader, const QString &filePath);

    void run() override;

signals:
    void uploadSlotReceived();
    void uploaded(const QUrl &getUrl);

protected:
    void startUploadToSlot(const QUrl &putUrl, const QUrl &getUrl);

private:
    void connectReply(QNetworkReply *reply) override;

    void startUpload();

    void onSlotRequestFinished(const QString &requestId, const QString &slotId);
    void onSlotRequestFailed(const QString &requestId);
    void onSlotUrlsReceived(const QString &slotId, const QUrl &putUrl, const QUrl &getUrl);
    void onSlotUrlErrorOcurrend(const QString &slotId, const QString &errorText);
    void onFinished();

    QString m_requestId;
    QString m_slotId;
    QUrl m_putUrl;
    QUrl m_getUrl;
    QPointer<FileLoader> m_fileLoader;
};
} // namespace vm

#endif // VM_UPLOADFILEOPERATION_H
