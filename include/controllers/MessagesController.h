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

#include "VSQCommon.h"

#include <QMutex>

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

    void loadMessages(const Chat::Id &chatId);
    Q_INVOKABLE void createSendMessage(const QString &body, const QVariant &attachmentUrl, const Enums::AttachmentType attachmentType);

    void setRecipientId(const Contact::Id &recipientId);

signals:
    void errorOccurred(const QString &errorText);

    void messageStatusChanged(const Message::Id &messageId, const Contact::Id &contactId, const Message::Status &status, QPrivateSignal); // TODO(fpohtmeh): remove this workaround

private:
    void setupTableConnections();
    void setUserId(const UserId &userId);

    void setMessageStatus(const Message::Id &messageId, const Contact::Id &contactId, const Message::Status &status);
    void setDeliveredStatus(const Jid &jid, const Message::Id &messageId);
    void receiveMessage(const QXmppMessage &msg);
    void sendMessage(const Message &message);
    void sendNotSentMessages();

    VSQMessenger *m_messenger;
    Models *m_models;
    UserDatabase *m_userDatabase;

    UserId m_userId;
    Chat::Id m_chatId;
    Contact::Id m_recipientId;

    QMutex m_messageGuard; // TODO(fpohtmeh): remove this workaround and mutex include
};
}

#endif // VM_MESSAGESCONTROLLER_H
