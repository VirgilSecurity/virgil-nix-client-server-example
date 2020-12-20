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

#include "VSQContactManager.h"
#include "VSQCustomer.h"
#include "VSQDiscoveryManager.h"
#include "VSQLastActivityManager.h"
#include "Messenger.h"
#include "Utils.h"
#include "Settings.h"
#include "android/VSQAndroid.h"
#include "helpers/FutureWorker.h"

#if VS_PUSHNOTIFICATIONS
#include "PushNotifications.h"
#include "XmppPushNotifications.h"
using namespace notifications;
using namespace notifications::xmpp;
#endif // VS_PUSHNOTIFICATIONS

#include <qxmpp/QXmppMessage.h>
#include <qxmpp/QXmppUtils.h>
#include <qxmpp/QXmppPushEnableIq.h>
#include <qxmpp/QXmppMessageReceiptManager.h>
#include <qxmpp/QXmppCarbonManager.h>

#include <QtConcurrent>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSslSocket>
#include <QUuid>

#ifndef USE_XMPP_LOGS
#define USE_XMPP_LOGS 0
#endif

Q_LOGGING_CATEGORY(lcMessenger, "messenger")

using namespace vm;
using Self = Messenger;


Self::Messenger(Settings *settings, Validator *validator)
    : MessageSender()
    , m_settings(settings)
    , m_validator(validator)
    , m_coreMessenger(new CoreMessenger(settings, this))
    , m_crashReporter(new CrashReporter(settings, m_coreMessenger, this))
    , m_fileLoader(new FileLoader(m_coreMessenger, this))
{
    //
    //  Proxy messenger signals.
    //
    connect(m_coreMessenger, &CoreMessenger::lastActivityTextChanged, this, &Self::lastActivityTextChanged);
    connect(m_coreMessenger, &CoreMessenger::messageUpadted, this, &Self::updateMessage);

    //
    //  Handle connection states.
    //
    connect(m_coreMessenger, &CoreMessenger::connectionStateChanged, this, &Self::onConnectionStateChanged);

    //
    //  Handle received messages.
    //
    connect(m_coreMessenger, &CoreMessenger::messageReceived, this, &Self::onMessageReceived);

    //
    // Push notifications
    //
#if VS_PUSHNOTIFICATIONS
    auto& pushNotifications  = PushNotifications::instance();
    connect(&pushNotifications, &PushNotifications::tokenUpdated, this, &Self::onPushNotificationTokenUpdate);
#endif // VS_PUSHNOTIFICATIONS
}


void
Self::signIn(const QString &username)
{
    FutureWorker::run(m_coreMessenger->signIn(username), [this, username = username](const FutureResult &result) {
        switch (result) {
        case FutureResult::Success:
            m_settings->setLastSignedInUser(username);
            emit signedIn(username);
            break;

        case FutureResult::Error_NoCred:
            emit signInErrorOccured(tr("Cannot load credentials"));
            break;

        case FutureResult::Error_ImportCredentials:
            emit signInErrorOccured(tr("Cannot import loaded credentials"));
            break;

        case FutureResult::Error_Signin:
            emit signInErrorOccured(tr("Cannot sign-in user"));
            break;

        default:
            emit signInErrorOccured(tr("Unknown sign-in error"));
            break;
        }
    });
}

void
Self::signOut()
{
    FutureWorker::run(m_coreMessenger->signOut(), [this](const FutureResult &) {
        m_settings->setLastSignedInUser(QString());
        emit signedOut();
    });
}


void
Self::signUp(const QString &username)
{
    FutureWorker::run(m_coreMessenger->signUp(username), [this, username = username](const FutureResult &result) {
        switch (result) {

        case FutureResult::Success:
            m_settings->addUserToList(username);
            m_settings->setLastSignedInUser(username);
            emit signedUp(username);
            break;

        case FutureResult::Error_UserAlreadyExists:
            emit signUpErrorOccured(tr("Username is already taken"));
            break;

        default:
            emit signUpErrorOccured(tr("Unknown sign-up error"));
            break;
        }
    });
}


void
Self::backupKey(const QString &password, const QString &confirmedPassword)
{
    if (password.isEmpty()) {
        emit backupKeyFailed(tr("Password cannot be empty"));

    } else if (password != confirmedPassword) {
        emit backupKeyFailed(tr("Passwords do not match"));

    } else {
        FutureWorker::run(m_coreMessenger->backupKey(password), [this](const FutureResult &result) {
            if (result == FutureResult::Success) {
                auto username = m_coreMessenger->currentUser()->username();
                emit keyBackuped(username);
            }
            else {
                emit backupKeyFailed(tr("Backup private key error"));
            }
        });
    }
}


void
Self::downloadKey(const QString &username, const QString &password)
{
    if (password.isEmpty()) {
        emit downloadKeyFailed(tr("Password cannot be empty"));

    } else {
        FutureWorker::run(m_coreMessenger->signInWithBackupKey(username, password), [this, username = username](const FutureResult &result) {
            if (result == FutureResult::Success) {
                m_settings->addUserToList(username);
                m_settings->setLastSignedInUser(username);
                emit keyDownloaded(username);
            }
            else {
                emit downloadKeyFailed(tr("Private key download error"));
            }
        });
    }
}


UserHandler
Self::findUserByUsername(const QString &username) const {
    return m_coreMessenger->findUserByUsername(username);
}


UserHandler
Self::findUserById(const UserId &id) const {
    return m_coreMessenger->findUserById(id);
}


UserHandler
Self::currentUser() const {
    return m_coreMessenger->currentUser();
}

bool
Self::sendMessage(MessageHandler message) {

    auto future = m_coreMessenger->sendMessage(message);

    return future.result() == FutureResult::Success;
}


bool
Self::subscribeToUser(const UserId &userId)
{
    auto user = m_coreMessenger->findUserById(userId);
    if (user) {
        return m_coreMessenger->subscribeToUser(*user);
    }

    qCCritical(lcMessenger) << "Cannot subscribe to the user. User not found, but should be at this point";

    return false;
}



QPointer<CrashReporter> Self::crashReporter() noexcept {
    return m_crashReporter;
}


QPointer<FileLoader> Self::fileLoader() noexcept {
    return m_fileLoader;
}


void
Self::setApplicationActive(bool active)
{
    if (active) {
        m_coreMessenger->activate();
    } else {
        m_coreMessenger->deactivate();
    }
}


void
Self::setCurrentRecipient(const QString &recipient)
{
    m_coreMessenger->setCurrentRecipient(recipient);
}


void
Self::onPushNotificationTokenUpdate() {
    m_coreMessenger->registerForNotifications();
}


void
Self::onConnectionStateChanged(CoreMessenger::ConnectionState state) {
    const bool isOnline = CoreMessenger::ConnectionState::Connected == state;

    emit onlineStatusChanged(isOnline);
}


void
Self::onMessageReceived(ModifiableMessageHandler message) {
    emit messageReceived(std::move(message));
}