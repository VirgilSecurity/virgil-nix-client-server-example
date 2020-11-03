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

#ifndef VS_MESSAGESQUEUE_H
#define VS_MESSAGESQUEUE_H

#include "VSQCommon.h"
#include "operations/Operation.h"

class VSQMessenger;
class VSQSettings;

namespace vm
{
class MessageOperation;
class MessageOperationFactory;
class Operation;
class FileLoader;
class UserDatabase;

class MessagesQueue : public Operation
{
    Q_OBJECT

public:
    MessagesQueue(const VSQSettings *settings, VSQMessenger *messenger, UserDatabase *userDatabase, FileLoader *fileLoader, QObject *parent);
    ~MessagesQueue() override;

signals:
    void pushMessageOperation(const GlobalMessage &message);
    void pushDownloadOperation(const GlobalMessage &message, const QString &filePath);
    void sendNotSentMessages();

    void messageStatusChanged(const Message::Id &messageId, const Contact::Id &contactId, const Message::Status status);
    void attachmentStatusChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const Attachment::Status &status);
    void attachmentProgressChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const DataSize &bytesLoaded, const DataSize &bytesTotal);
    void attachmentUrlChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const QUrl &url);
    void attachmentExtrasChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const Attachment::Type &type, const QVariant &extras);
    void attachmentLocalPathChanged(const Attachment::Id &attachmentId, const Contact::Id &contactId, const QString &localPath);

    void notificationCreated(const QString &notification);

private:
    void setUserId(const UserId &userId);
    void setMessages(const GlobalMessages &messages);

    void connectMessageOperation(MessageOperation *op);
    void appendMessageOperation(const GlobalMessage &message);

    void onPushMessage(const GlobalMessage &message);
    void onPushDownloadOperation(const GlobalMessage &message, const QString &filePath);
    void onSendNotSentMessages();
    void onMessageOperationStatusChanged(const MessageOperation *operation);
    void onMessageOperationAttachmentStatusChanged(const MessageOperation *operation);
    void onMessageOperationAttachmentProgressChanged(const MessageOperation *operation);
    void onMessageOperationAttachmentUrlChanged(const MessageOperation *operation);
    void onMessageOperationAttachmentExtrasChanged(const MessageOperation *operation);
    void onMessageOperationAttachmentLocalPathChanged(const MessageOperation *operation);

    VSQMessenger *m_messenger;
    UserDatabase *m_userDatabase;
    MessageOperationFactory *m_factory;
    UserId m_userId;
};
}

#endif // VS_MESSAGESQUEUE_H
