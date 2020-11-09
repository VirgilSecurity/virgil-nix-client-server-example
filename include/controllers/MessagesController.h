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

#ifndef VM_MESSAGESCONTROLLER_H
#define VM_MESSAGESCONTROLLER_H

#include <QObject>

#include <qxmpp/QXmppMessage.h>

#include "VSQCommon.h"

class QXmppMessage;
class VSQMessenger;

namespace vm
{
class Models;
class UserDatabase;

class MessagesController : public QObject
{
    Q_OBJECT

public:
    MessagesController(VSQMessenger *messenger, Models *models, UserDatabase *userDatabase, QObject *parent);

    void setUserId(const UserId &userId);
    void loadMessages(const Chat &chat);
    Q_INVOKABLE void createSendMessage(const QString &body, const QVariant &attachmentUrl, const Enums::AttachmentType attachmentType);

signals:
    void errorOccurred(const QString &errorText);
    void notificationCreated(const QString &notification);

    void messageCreated(const GlobalMessage &message);
    void messageStatusChanged(const Message::Id &messageId, const Contact::Id &contactId, const Message::Status &status);

    void displayImageNotFound(const QString &messageId);

private:
    void setupTableConnections();

    void onChatUpdated(const Chat &chat);

    void setMessageStatus(const Message::Id &messageId, const Contact::Id &contactId, const Message::Status &status);
    void setDeliveredStatus(const Jid &jid, const Message::Id &messageId);
    void setAttachmentStatus(const Attachment::Id &attachmentId, const Contact::Id &contactId, const Attachment::Status &status);
    void setAttachmentUrl(const Attachment::Id &attachmentId, const Contact::Id &contactId, const QUrl &url);
    void setAttachmentLocalPath(const Attachment::Id &attachmentId, const Contact::Id &contactId, const QString &localPath);
    void setAttachmentExtras(const Attachment::Id &attachmentId, const Contact::Id &contactId, const Attachment::Type &type, const QVariant &extras);
    void setAttachmentProcessedSize(const Attachment::Id &attachmentId, const Contact::Id &contactId, const DataSize &processedSize);
    void setAttachmentEncryptedSize(const Attachment::Id &attachmentId, const Contact::Id &contactId, const DataSize &encryptedSize);

    void receiveMessage(const QXmppMessage &msg);

    VSQMessenger *m_messenger;
    Models *m_models;
    UserDatabase *m_userDatabase;

    UserId m_userId;
    Chat m_chat;

    // NOTE(fpohtmeh): this workaround is needed when messages are received before chat list loading
    // it will be remove right after offline mode
    std::vector<QXmppMessage> m_receivedMessagesWithoutUser;
    struct DeliveredStatusInfo
    {
        const Jid jid;
        const Message::Id messageId;
    };
    std::vector<DeliveredStatusInfo> m_deliveredStatusesWithoutUser;
};
}

#endif // VM_MESSAGESCONTROLLER_H
