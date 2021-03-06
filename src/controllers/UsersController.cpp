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

#include "controllers/UsersController.h"

#include "Messenger.h"
#include "database/UserDatabase.h"
#include "database/ContactsTable.h"
#include "models/Models.h"
#include "models/ChatsModel.h"
#include "models/MessagesModel.h"

#if VS_ANDROID
#    include "VSQAndroid.h"
#endif

#include <utility>

using namespace vm;
using Self = UsersController;

Self::UsersController(Messenger *messenger, Models *models, UserDatabase *userDatabase, QObject *parent)
    : QObject(parent), m_messenger(messenger), m_userDatabase(userDatabase)
{
    connect(messenger, &Messenger::signedIn, userDatabase, &UserDatabase::openUser);
    connect(messenger, &Messenger::signedUp, userDatabase, &UserDatabase::openUser);
    connect(messenger, &Messenger::keyDownloaded, userDatabase, &UserDatabase::openUser);

    connect(messenger, &Messenger::signedOut, this, &Self::onMessengerSignedOut);
    connect(messenger, &Messenger::signInErrorOccured, this, &Self::userNotLoaded);
    connect(messenger, &Messenger::userWasFound, this, &Self::writeContactToDatabase);

    connect(userDatabase, &UserDatabase::opened, this, &Self::onUserDatabaseOpened);
    connect(userDatabase, &UserDatabase::errorOccurred, this, &Self::onUserDatabaseErrorOccurred);

    connect(models->chats(), &ChatsModel::chatAdded, this, &Self::onChatAdded);

    connect(this, &Self::userLoaded, this, &Self::hideSplashScreen);
    connect(this, &Self::userNotLoaded, this, &Self::hideSplashScreen);

    // Notifications
    auto notifyAboutError = [this](const QString &text) { emit notificationCreated(text, true); };
    connect(messenger, &Messenger::signInErrorOccured, notifyAboutError);
    connect(messenger, &Messenger::signUpErrorOccured, notifyAboutError);
    connect(messenger, &Messenger::downloadKeyFailed, notifyAboutError);
    connect(userDatabase, &UserDatabase::errorOccurred, notifyAboutError);
}

void Self::initialSignIn()
{
    const auto settings = m_messenger->settings();
    const auto username = settings->lastSignedInUser();
    if (username.isEmpty() || settings->userCredential(username).isEmpty()) {
        emit userNotLoaded();
    } else {
        m_messenger->signIn(username);
    }
}

QString Self::currentUserId() const
{
    const auto user = m_messenger->currentUser();
    return user ? user->id() : QString();
}

QString Self::currentUsername() const
{
    const auto user = m_messenger->currentUser();
    return user ? user->username() : QString();
}

void Self::updateCurrentUser()
{
    const auto user = m_messenger->currentUser();
    emit currentUserIdChanged(user->id());
    emit currentUsernameChanged(user->username());

    writeContactToDatabase(user);
}

void Self::writeContactToDatabase(const UserHandler &user)
{
    Contact contact(user->id());
    contact.setUsername(user->username());
    m_userDatabase->contactsTable()->addContact(contact);
}

void Self::hideSplashScreen()
{
#if VS_ANDROID
    if (m_splashScreenVisible) {
        VSQAndroid::hideSplashScreen();
        m_splashScreenVisible = false;
    }
#endif
}

void Self::onMessengerSignedOut()
{
    m_userDatabase->closeUser();
    emit userNotLoaded();
}

void Self::onUserDatabaseOpened()
{
    updateCurrentUser();
    emit userLoaded();
}

void Self::onUserDatabaseErrorOccurred()
{
    if (m_messenger->currentUser()) {
        m_messenger->signOut();
    } else {
        emit userNotLoaded();
    }
}

void Self::onChatAdded(const ChatHandler &chat)
{
    if (chat->type() == Chat::Type::Personal) {
        m_messenger->subscribeToUser(UserId(chat->id()));
    }
}
