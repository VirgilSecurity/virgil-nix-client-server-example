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

#ifndef VM_CHATSCONTROLLER_H
#define VM_CHATSCONTROLLER_H

#include "ChatId.h"
#include "MessageId.h"
#include "UserId.h"
#include "Chat.h"
#include "User.h"

#include <QObject>
#include <QPointer>

#include <memory>

namespace vm
{
class Models;
class UserDatabase;
class Messenger;

class ChatsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentChatName READ currentChatName NOTIFY currentChatNameChanged)

public:
    ChatsController(Messenger *messenger, Models *models, UserDatabase *userDatabase, QObject *parent);

    void loadChats();
    void clearChats();

    void createChatWithUsername(const QString &username);
    void createChatWithUserId(const UserId &userId);

    void openChat(const ChatHandler& chat);
    Q_INVOKABLE void openChat(const QString& chatId); // can be used within QML only
    Q_INVOKABLE void closeChat();
    Q_INVOKABLE QString currentChatName() const;
    ChatHandler currentChat() const;

    Q_INVOKABLE void addParticipant(const QString &username);
    Q_INVOKABLE void removeParticipant(const QString &username);
    Q_INVOKABLE void leaveGroup();

signals:
    void errorOccurred(const QString &errorText); // TODO(fpohtmeh): remove this signal everywhere?

    void chatsLoaded();
    void chatOpened(const ChatHandler &chat);
    void chatCreated(const ChatHandler &chat);
    void chatClosed();

    void currentChatNameChanged(const QString &name);

    void createChatWithUser(const UserHandler& user, QPrivateSignal);

private:
    void setupTableConnections();
    void setCurrentChat(ChatHandler chat);

    void onChatsLoaded(ModifiableChats chats);
    void onCreateChatWithUser(const UserHandler &user);

    QPointer<Messenger> m_messenger;
    QPointer<Models> m_models;
    QPointer<UserDatabase> m_userDatabase;
    ChatHandler m_currentChat;
};
}

#endif // VM_CHATSSCONTROLLER_H
