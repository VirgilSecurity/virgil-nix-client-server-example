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

#ifndef VS_MESSAGEOPERATION_H
#define VS_MESSAGEOPERATION_H

#include "NetworkOperation.h"

namespace vm
{
class MessageOperationFactory;

class MessageOperation : public NetworkOperation
{
    Q_OBJECT

public:
    MessageOperation(const GlobalMessage &message, MessageOperationFactory *factory, FileLoader *fileLoader, QObject *parent);

    const GlobalMessage *message() const;
    const Attachment *attachment() const;
    MessageOperationFactory *factory();

    void setAttachmentStatus(const Attachment::Status status);
    void setAttachmentUrl(const QUrl &url);
    void setAttachmentLocalPath(const QString &localPath);
    void setAttachmentFignerprint(const QString &fingerprint);
    void setAttachmentExtras(const QVariant &extras);
    void setAttachmentPreviewPath(const QString &previewPath);
    void setAttachmentThumbnailPath(const QString &thumbnailPath);
    void setAttachmentThumbnailUrl(const QUrl &thumbnailUrl);

    void setAttachmentProcessedSize(const DataSize &bytes);
    void setAttachmentEncryptedSize(const DataSize &size);
    void setAttachmentEncryptedThumbnailSize(const DataSize &bytes);

signals:
    void statusChanged(const Message::Id &messageId, const Contact::Id &contactId, const Message::Status &status);
    void attachmentStatusChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const Attachment::Status &status);
    void attachmentUrlChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const QUrl &url);
    void attachmentLocalPathChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const QString &localPath);
    void attachmentFingerprintChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const QString &fingerpint);

    void attachmentExtrasChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const Attachment::Type &type, const QVariant &extras);
    void attachmentProcessedSizeChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const DataSize &bytes);
    void attachmentEncryptedSizeChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const DataSize &bytes);

protected:
    void connectChild(Operation *child) override;

private:
    Attachment *writableAttachment();
    void setStatus(const Message::Status &status);

    MessageOperationFactory *m_factory;
    GlobalMessage m_message;
};
}

#endif // VS_MESSAGEOPERATION_H
