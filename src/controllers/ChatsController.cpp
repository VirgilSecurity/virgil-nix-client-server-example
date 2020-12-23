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

#include "controllers/ChatsController.h"

#include <QtConcurrent>

#include "Messenger.h"
#include "database/ChatsTable.h"
#include "database/MessagesTable.h"
#include "database/UserDatabase.h"
#include "models/ChatsModel.h"
#include "models/MessagesModel.h"
#include "models/Models.h"
#include "Controller.h"

using namespace vm;
using Self = ChatsController;

Self::ChatsController(Messenger *messenger, Models *models, UserDatabase *userDatabase, QObject *parent)
    : QObject(parent)
    , m_messenger(messenger)
    , m_models(models)
    , m_userDatabase(userDatabase)
{
    connect(userDatabase, &UserDatabase::opened, this, &Self::setupTableConnections);
    connect(this, &Self::createChatWithUser, this, &Self::onCreateChatWithUser);
}

ChatHandler Self::currentChat() const
{
    return m_currentChat;
}

void ChatsController::addParticipant(const QString &username) {

}

void ChatsController::removeParticipant(const QString &username) {

}

void ChatsController::leaveGroup() {

}

QString Self::currentChatName() const
{
    return m_currentChat ? m_currentChat->title() : QString();
}

void Self::loadChats()
{
    qCDebug(lcController) << "Started to load chats...";
    m_userDatabase->chatsTable()->fetch();
}

void Self::clearChats()
{
    qCDebug(lcController) << "Clear chats...";
    m_models->chats()->clearChats();
}

void Self::createChatWithUsername(const QString &username)
{
    QtConcurrent::run([this, username = username]() {
        if (!m_messenger) {
            qCWarning(lcController) << "Messenger was not initialized";
            emit errorOccurred(tr("Chat can not be created"));
        }
        auto user = m_messenger->findUserByUsername(username);
        if (user) {
            emit createChatWithUser(user, QPrivateSignal());
        }
        else {
            emit errorOccurred(tr("Contact not found"));
        }
    });
}

void Self::createChatWithUserId(const UserId &userId)
{
    QtConcurrent::run([this, userId = userId]() {
        if (!m_messenger) {
            qCWarning(lcController) << "Messenger was not initialized";
            emit errorOccurred(tr("Contact not found"));
        }
        auto user = m_messenger->findUserById(userId);
        if (user) {
            emit createChatWithUser(user, QPrivateSignal());
        }
        else {
            emit errorOccurred(tr("Contact not found"));
        }
    });
}

void Self::openChat(const ChatHandler& chat)
{
    qCDebug(lcController) << "Opening chat with id: " << chat->id();
    if (chat->unreadMessageCount() > 0) {
        m_models->chats()->resetUnreadCount(chat->id());
        m_userDatabase->resetUnreadCount(chat);
    }
    m_currentChat = chat;
    emit chatOpened(m_currentChat);
}

void Self::openChat(const ChatId &chatId)
{
    openChat(m_models->chats()->findChat(chatId));
}

void Self::closeChat()
{
    m_currentChat = nullptr;
    emit chatClosed();
}

void Self::setupTableConnections()
{
    auto table = m_userDatabase->chatsTable();
    connect(table, &ChatsTable::errorOccurred, this, &Self::errorOccurred);
    connect(table, &ChatsTable::fetched, this, &Self::onChatsLoaded);
}

void Self::onCreateChatWithUser(const UserHandler &user)
{
    auto newChat = std::make_shared<Chat>();
    newChat->setId(ChatId(user->id()));
    newChat->setCreatedAt(QDateTime::currentDateTime());
    newChat->setType(Chat::Type::Personal);
    newChat->setTitle(user->username().isEmpty() ? user->id() : user->username());
    m_userDatabase->chatsTable()->addChat(newChat);
    openChat(newChat);
}

void Self::onChatsLoaded(ModifiableChats chats)
{
    qCDebug(lcController) << "Chats loaded from the database";
    m_models->chats()->setChats(chats);
    emit chatsLoaded();
}
