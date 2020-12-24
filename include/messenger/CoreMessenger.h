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

#ifndef VM_CORE_MESSENGER_H
#define VM_CORE_MESSENGER_H


#include "Message.h"
#include "MessageUpdate.h"
#include "User.h"
#include "Chat.h"
#include "Settings.h"

#include <qxmpp/QXmppClient.h>
#include <qxmpp/QXmppHttpUploadIq.h>

#include <QLoggingCategory>
#include <QObject>
#include <QFuture>
#include <QUrl>
#include <QPointer>

#include <memory>

Q_DECLARE_LOGGING_CATEGORY(lcCommKitMessenger)

namespace vm
{
class CoreMessenger : public QObject
{
    Q_OBJECT
public:
    enum class Result
    {
        Success,
        Error_CryptoInit,
        Error_Offline,
        Error_NoCred,
        Error_Signin,
        Error_Signup,
        Error_MakeKeyBackup,
        Error_RestoreKeyBackup,
        Error_UserNotFound,
        Error_UserAlreadyExists,
        Error_ExportCredentials,
        Error_ImportCredentials,
        Error_Attachment,
        Error_InvalidMessageFormat,
        Error_InvalidMessageVersion,
        Error_InvalidMessageCiphertext,
        Error_SendMessageFailed,
    };

    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Error
    };

signals:
    //
    //  Should be called when application became activated.
    //
    void activate();

    //
    //  Should be called when application goes to the background.
    //
    void deactivate();

    //
    //  Info signals.
    //
    void connectionStateChanged(ConnectionState state);

    void lastActivityTextChanged(const QString& text);

    void messageReceived(ModifiableMessageHandler message);
    void updateMessage(const MessageUpdate& messageUpdate);

    //
    //  Private signals, to resolve thread. issues.
    //
    void reconnectXmppServerIfNeeded();
    void disconnectXmppServer();
    void cleanupCommKitMessenger();
    void deregisterFromPushNotifications();
public:
    //
    //  Create.
    //
    explicit CoreMessenger(Settings *settings, QObject *parent = nullptr);
    ~CoreMessenger() noexcept;

    //
    //  Info / Controls.
    //

    //
    //  Return true if messenger has Internet connection with all services.
    //
    bool isOnline() const noexcept;
    bool isSignedIn() const noexcept;

    //
    //  Sign-in / Sign-up / Backup.
    //
    QFuture<Result> signIn(const QString& username);
    QFuture<Result> signUp(const QString& username);
    QFuture<Result> signInWithBackupKey(const QString& username, const QString& password);
    QFuture<Result> backupKey(const QString& password);
    QFuture<Result> signOut();

    //
    // Users.
    //
    std::shared_ptr<User> findUserByUsername(const QString &username) const;
    std::shared_ptr<User> findUserById(const UserId &userId) const;
    std::shared_ptr<User> currentUser() const;

    //
    //  Messages.
    //
    QFuture<Result> sendMessage(MessageHandler message);
    QFuture<Result> processReceivedXmppMessage(const QXmppMessage& xmppMessage);

    //
    //  Push Notifications.
    //
    bool registerForNotifications();
    bool deregisterFromNotifications();

    //
    //  Contacts (XMPP).
    //
    bool subscribeToUser(const User &user);
    void setCurrentRecipient(const QString& recipientId);

    //
    //  Internal helpers.
    //
    QUrl getCrashReportEndpointUrl() const;
    QString getAuthHeaderVaue() const;

    //--
    //  File upload.
    //
public:
    bool isUploadServiceFound() const;
    QString requestUploadSlot(const QString &filePath);

signals:
    void uploadServiceFound(bool found);
    void uploadSlotReceived(const QString &slotId, const QUrl &putUrl, const QUrl &getUrl);
    void uploadSlotErrorOccurred(const QString &slotId, const QString &errorText);
    //
    //--

private:
    //
    //  Configuration.
    //
    Result resetCommKitConfiguration();
    void resetXmppConfiguration();

    //
    //  Connection
    //
    void connectXmppServer();
    void changeConnectionState(ConnectionState state);

    //
    //  Helpers.
    //
    UserId userIdFromJid(const QString& jid) const;
    QString userIdToJid(const UserId& userId) const;
    QString currentUserJid() const;

    bool isNetworkOnline() const noexcept;
    bool isXmppConnected() const noexcept;
    bool isXmppConnecting() const noexcept;
    bool isXmppDisconnected() const noexcept;


private slots:
    void onActivate();
    void onDeactivate();
    void xmppOnConnected();
    void xmppOnDisconnected();
    void xmppOnError(QXmppClient::Error);
    void xmppOnPresenceReceived(const QXmppPresence &presence);
    void xmppOnIqReceived(const QXmppIq &iq);
    void xmppOnSslErrors(const QList<QSslError> &errors);
    void xmppOnMessageReceived(const QXmppMessage &xmppMessage);
    void xmppOnMessageDelivered(const QString &jid, const QString &messageId);
    void xmppOnUploadServiceFound();
    void xmppOnUploadSlotReceived(const QXmppHttpUploadSlotIq &slot);
    void xmppOnUploadRequestFailed(const QXmppHttpUploadRequestIq &request);

    void onReconnectXmppServerIfNeeded();
    void onDisconnectXmppServer();
    void onCleanupCommKitMessenger();
    void onDeregisterFromPushNotifications();

    void onProcessNetworkState(bool online);
    void onLogConnectionStateChanged(CoreMessenger::ConnectionState state);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
    QPointer<Settings> m_settings;
};
}

Q_DECLARE_METATYPE(vm::MessageHandler);
Q_DECLARE_METATYPE(vm::ModifiableMessageHandler);
Q_DECLARE_METATYPE(vm::Messages);
Q_DECLARE_METATYPE(vm::ModifiableMessages);
Q_DECLARE_METATYPE(vm::UserHandler);
Q_DECLARE_METATYPE(vm::ChatHandler);
Q_DECLARE_METATYPE(vm::ModifiableChatHandler);
Q_DECLARE_METATYPE(vm::ModifiableChats);
Q_DECLARE_METATYPE(vm::ChatId);
Q_DECLARE_METATYPE(vm::MessageId);
Q_DECLARE_METATYPE(vm::AttachmentId);

Q_DECLARE_METATYPE(QXmppClient::Error);
Q_DECLARE_METATYPE(QXmppPresence);
Q_DECLARE_METATYPE(QXmppIq);
Q_DECLARE_METATYPE(QXmppHttpUploadSlotIq);
Q_DECLARE_METATYPE(QXmppHttpUploadRequestIq);

#endif // VM_CORE_MESSENGER_H
