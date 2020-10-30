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

#include <Core.h>
#include <VSQContactManager.h>
#include <VSQCustomer.h>
#include <VSQDiscoveryManager.h>
#include <VSQDownload.h>
#include <VSQLastActivityManager.h>
#include <VSQMessenger.h>
#include <VSQSqlConversationModel.h>
#include <VSQUpload.h>
#include <Utils.h>
#include <Settings.h>
#include <android/VSQAndroid.h>
#include <helpers/FutureWorker.h>

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
#include <QtQml>
#include <QUuid>

#include <QuickFuture>
Q_DECLARE_METATYPE(VSQMessenger::EnStatus)
Q_DECLARE_METATYPE(VSQMessenger::EnResult)
Q_DECLARE_METATYPE(QFuture<VSQMessenger::EnResult>)

#include <cstring>
#include <cstdlib>

#ifndef USE_XMPP_LOGS
#define USE_XMPP_LOGS 0
#endif

const QString VSQMessenger::kProdEnvPrefix = "prod";
const QString VSQMessenger::kStgEnvPrefix = "stg";
const QString VSQMessenger::kDevEnvPrefix = "dev";
const int VSQMessenger::kConnectionWaitMs = 10000;
const int VSQMessenger::kKeepAliveTimeSec = 10;

using namespace vm;

Q_LOGGING_CATEGORY(lcMessenger, "messenger")

using namespace vm;

// Helper struct to categorize transfers
struct TransferId
{
    enum class Type
    {
        File,
        Thumbnail
    };

    TransferId() = default;
    TransferId(const QString &messageId, const Type type)
        : messageId(messageId)
        , type(type)
    {}
    ~TransferId() = default;

    static TransferId parse(const QString &rawTransferId)
    {
        const auto parts = rawTransferId.split(';');
        return TransferId(parts.front(), (parts.back() == "thumb") ? Type::Thumbnail : Type::File);
    }

    operator QString() const
    {
        return messageId + ((type == Type::Thumbnail) ? QLatin1String(";thumb") : QLatin1String(";file"));
    }

    QString messageId;
    Type type = Type::File;
};


/******************************************************************************/
VSQMessenger::VSQMessenger(QNetworkAccessManager *networkAccessManager, VSQSettings *settings, Validator *validator)
    : QObject()
    , m_xmpp()
    , m_settings(settings)
    , m_validator(validator)
    , m_transferManager(new VSQCryptoTransferManager(&m_xmpp, networkAccessManager, m_settings, this))
    , m_attachmentBuilder(settings, this)
{
    // Register QML typess
    qmlRegisterType<VSQMessenger>("MesResult", 1, 0, "Result");
    QuickFuture::registerType<VSQMessenger::EnResult>([](VSQMessenger::EnResult res) -> QVariant {
        return QVariant(static_cast<int>(res));
    });

    qRegisterMetaType<QXmppClient::Error>();

    // Connect to Database
    _connectToDatabase();
    m_sqlConversations = new VSQSqlConversationModel(validator, this);
    //m_sqlChatModel = new VSQSqlChatModel(this);

    // Add receipt messages extension
    m_xmppReceiptManager = new QXmppMessageReceiptManager();
    m_xmppCarbonManager = new QXmppCarbonManager();
    m_discoveryManager = new VSQDiscoveryManager(&m_xmpp, this);
    m_contactManager = new VSQContactManager(&m_xmpp, this);
    m_lastActivityManager = new VSQLastActivityManager(m_settings, this);

    m_xmpp.addExtension(m_xmppReceiptManager);
    m_xmpp.addExtension(m_xmppCarbonManager);
    m_xmpp.addExtension(m_lastActivityManager);

    // Signal connection
    connect(this, SIGNAL(fireError(QString)), this, SLOT(disconnect()));
    connect(this, &VSQMessenger::downloadThumbnail, this, &VSQMessenger::onDownloadThumbnail);

    // Uploading
    connect(m_transferManager, &VSQCryptoTransferManager::fireReadyToUpload, this,  &VSQMessenger::fireReady, Qt::QueuedConnection);
    connect(m_transferManager, &VSQCryptoTransferManager::statusChanged, this, &VSQMessenger::onAttachmentStatusChanged);
    connect(m_transferManager, &VSQCryptoTransferManager::progressChanged, this, &VSQMessenger::onAttachmentProgressChanged);
    connect(m_transferManager, &VSQCryptoTransferManager::fileDecrypted, this, &VSQMessenger::onAttachmentDecrypted);

    // Connect XMPP signals
    connect(&m_xmpp, SIGNAL(connected()), this, SLOT(onConnected()), Qt::QueuedConnection);
    connect(&m_xmpp, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(&m_xmpp, &QXmppClient::error, this, &VSQMessenger::onError);
    connect(&m_xmpp, SIGNAL(presenceReceived(const QXmppPresence &)), this, SLOT(onPresenceReceived(const QXmppPresence &)));
    connect(&m_xmpp, SIGNAL(iqReceived(const QXmppIq &)), this, SLOT(onIqReceived(const QXmppIq &)));
    connect(&m_xmpp, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(onSslErrors(const QList<QSslError> &)));
    connect(&m_xmpp, SIGNAL(stateChanged(QXmppClient::State)), this, SLOT(onStateChanged(QXmppClient::State)));

    // Network Analyzer
    connect(&m_networkAnalyzer, &VSQNetworkAnalyzer::fireStateChanged, this, &VSQMessenger::onProcessNetworkState, Qt::QueuedConnection);
    connect(&m_networkAnalyzer, &VSQNetworkAnalyzer::fireHeartBeat, this, &VSQMessenger::checkState, Qt::QueuedConnection);

    // Push notifications
#if VS_PUSHNOTIFICATIONS
    auto& pushNotifications  = PushNotifications::instance();
    connect(&pushNotifications, &PushNotifications::tokenUpdated, this, &VSQMessenger::onPushNotificationTokenUpdate);
#endif // VS_PUSHNOTIFICATIONS
}

VSQMessenger::~VSQMessenger()
{
}

void VSQMessenger::signIn(const QString &userId)
{
    FutureWorker::run(signInAsync(userId), [=](const FutureResult &result) {
        switch (result) {
        case MRES_OK:
            m_settings->setLastSignedInUserId(userId);
            emit signedIn(userId);
            break;
        case MRES_ERR_NO_CRED:
            emit signInErrorOccured(tr("Cannot load credentials"));
            break;
        case MRES_ERR_SIGNIN:
            emit signInErrorOccured(tr("Cannot sign-in user"));
            break;
        default:
            emit signInErrorOccured(tr("Unknown sign-in error"));
            break;
        }
    });
}

void VSQMessenger::signOut()
{
    FutureWorker::run(logoutAsync(), [=](const FutureResult &) {
        m_settings->setLastSignedInUserId(QString());
        emit signedOut();
    });
}

void VSQMessenger::signUp(const QString &userId)
{
    FutureWorker::run(signUpAsync(userId), [=](const FutureResult &result) {
        switch (result) {
        case MRES_OK:
            m_settings->setLastSignedInUserId(userId);
            emit signedUp(userId);
            break;
        case MRES_ERR_USER_ALREADY_EXISTS:
            emit signUpErrorOccured(tr("Username is already taken"));
            break;
        default:
            emit signUpErrorOccured(tr("Unknown sign-up error"));
            break;
        }
    });
}

void VSQMessenger::backupKey(const QString &password, const QString &confirmedPassword)
{
    if (password.isEmpty()) {
        emit backupKeyFailed(tr("Password cannot be empty"));
    }
    else if (password != confirmedPassword) {
        emit backupKeyFailed(tr("Passwords do not match"));
    }
    else {
        FutureWorker::run(backupKeyAsync(password), [=](const FutureResult &result) {
            if (result == MRES_OK) {
                emit keyBackuped(m_userId);
            }
            else {
                emit backupKeyFailed(tr("Backup private key error"));
            }
        });
    }
}

void VSQMessenger::downloadKey(const QString &userId, const QString &password)
{
    if (password.isEmpty()) {
        emit downloadKeyFailed(tr("Password cannot be empty"));
    }
    else {
        FutureWorker::run(signInWithBackupKeyAsync(userId, password), [=](const FutureResult &result) {
            if (result == MRES_OK) {
                emit keyDownloaded(userId);
            }
            else {
                emit downloadKeyFailed(tr("Private key download error"));
            }
        });
    }
}

/******************************************************************************/
void
VSQMessenger::_connectToDatabase() {
    QSqlDatabase database = QSqlDatabase::database();
    if (!database.isValid()) {
        database = QSqlDatabase::addDatabase("QSQLITE");
        if (!database.isValid())
            qFatal("Cannot add database: %s", qPrintable(database.lastError().text()));
    }

    const QDir writeDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!writeDir.mkpath("."))
        qFatal("Failed to create writable directory at %s", qPrintable(writeDir.absolutePath()));

    // Ensure that we have a writable location on all devices.
    const QString fileName = writeDir.absolutePath() + "/chat-database.sqlite3";
    // When using the SQLite driver, open() will create the SQLite database if it doesn't exist.
    database.setDatabaseName(fileName);
    if (!database.open()) {
        QFile::remove(fileName);
        qFatal("Cannot open database: %s", qPrintable(database.lastError().text()));
    }
}

/******************************************************************************/
QString
VSQMessenger::_xmppPass() {
    char pass[VS_MESSENGER_VIRGIL_TOKEN_SZ_MAX];

    // Get XMPP password
    if (VS_CODE_OK != vs_messenger_virgil_get_xmpp_pass(pass, VS_MESSENGER_VIRGIL_TOKEN_SZ_MAX)) {
        emit fireError(tr("Cannot get XMPP password"));
        return "";
    }

    return QString::fromLatin1(pass);
}

/******************************************************************************/
Q_DECLARE_METATYPE(QXmppConfiguration)
bool
VSQMessenger::_connect(QString userWithEnv, QString deviceId, QString userId, bool forced) {
    if (forced) {
        m_connectGuard.lock();
    } else if (!m_connectGuard.tryLock()) {
        return false;
    }

    static int cnt = 0;

    const int cur_val = cnt++;

    qCDebug(lcNetwork) << ">>>>>>>>>>> _connect: START " << cur_val;

    // Update users list
    m_settings->addUserToList(userWithEnv);

    // Connect to XMPP
    emit fireConnecting();

    QString jid = userId + "@" + xmppURL() + "/" + deviceId;
    m_conf.setJid(jid);
    m_conf.setHost(xmppURL());
    m_conf.setPassword(_xmppPass());
    m_conf.setAutoReconnectionEnabled(false);
    m_conf.setAutoAcceptSubscriptions(true);
#if VS_ANDROID
    m_conf.setKeepAliveInterval(kKeepAliveTimeSec);
    m_conf.setKeepAliveTimeout(kKeepAliveTimeSec - 1);
#endif
    qDebug() << "SSL: " << QSslSocket::supportsSsl();

    auto logger = QXmppLogger::getLogger();
    logger->setLoggingType(QXmppLogger::SignalLogging);
    logger->setMessageTypes(QXmppLogger::AnyMessage);

#if USE_XMPP_LOGS
    connect(logger, &QXmppLogger::message, [=](QXmppLogger::MessageType, const QString &text){
        qDebug() << text;
    });

    m_xmpp.setLogger(logger);
#endif
    if (m_xmpp.isConnected()) {
        // Wait for disconnection
        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        connect(&m_xmpp, &QXmppClient::connected, &loop, &QEventLoop::quit);
        connect(&m_xmpp, &QXmppClient::disconnected, &loop, &QEventLoop::quit);
        connect(&m_xmpp, &QXmppClient::error, &loop, &QEventLoop::quit);
        connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        QMetaObject::invokeMethod(&m_xmpp, "disconnectFromServer", Qt::QueuedConnection);

        timer.start(5000);
        loop.exec();
    }

    // Wait for connection
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect(&m_xmpp, &QXmppClient::connected, &loop, &QEventLoop::quit);
    connect(&m_xmpp, &QXmppClient::disconnected, &loop, &QEventLoop::quit);
    connect(&m_xmpp, &QXmppClient::error, &loop, &QEventLoop::quit);
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    qRegisterMetaType<QXmppConfiguration>("QXmppConfiguration");
    QMetaObject::invokeMethod(&m_xmpp, "connectToServer", Qt::QueuedConnection, Q_ARG(QXmppConfiguration, m_conf));

    timer.start(kConnectionWaitMs);
    loop.exec();

    const bool connected = m_xmpp.isConnected();
    qCDebug(lcNetwork) << "<<<<<<<<<<< _connect: FINISH connected = " << connected << "  " << cur_val;

    qDebug() << "Carbons " << (connected ? "enable" : "disable");
    m_xmppCarbonManager->setCarbonsEnabled(connected);
    m_connectGuard.unlock();
    return connected;
}

/******************************************************************************/
void
VSQMessenger::onProcessNetworkState(bool online) {
    if (online) {
        _reconnect();
    } else {
        emit fireError("No internet connection");
    }
}

/******************************************************************************/
QString
VSQMessenger::_caBundleFile() {
#if VS_ANDROID
    return VSQAndroid::caBundlePath();
#else
    return qgetenv("VS_CURL_CA_BUNDLE");
#endif
}

/******************************************************************************/
QString
VSQMessenger::_prepareLogin(const QString &user) {
    QString userId = user;
    m_envType = _defaultEnv;

    // NOTE(vova.y): deprecated logic, user can't contain @
    // Check required environment
    QStringList pieces = user.split("@");
    if (2 == pieces.size()) {
        userId = pieces.at(1);
        if (kProdEnvPrefix == pieces.first()) {
            m_envType = PROD;
        } else if (kStgEnvPrefix == pieces.first()) {
            m_envType = STG;
        }
        else if (kDevEnvPrefix == pieces.first()) {
            m_envType = DEV;
        }
    }

    vs_messenger_virgil_logout();
    char *cCABundle = strdup(_caBundleFile().toStdString().c_str());
    if (VS_CODE_OK != vs_messenger_virgil_init(_virgilURL().toStdString().c_str(), cCABundle)) {
        qCritical() << "Cannot initialize low level messenger";
    }
    free(cCABundle);

    m_crashReporter->setVirgilUrl(_virgilURL());
    m_crashReporter->setkApp(Customer::ApplicationName);
    m_crashReporter->setkOrganization(Customer::OrganizationName);

    // Set current user
    m_user = userId;
    m_sqlConversations->setUser(userId);
    //m_sqlChatModel->init(userId);

    // Inform about user activation
    emit fireCurrentUserChanged();

    return userId;
}

/******************************************************************************/
QString VSQMessenger::currentUser() const {
    return m_user;
}

/******************************************************************************/
QString VSQMessenger::currentRecipient() const {
    return m_recipient;
}

QXmppClient *VSQMessenger::xmpp()
{
    return &m_xmpp;
}

/******************************************************************************/
QFuture<VSQMessenger::EnResult>
VSQMessenger::backupKeyAsync(QString password) {
    // m_userId = _prepareLogin(user);
    return QtConcurrent::run([=]() -> EnResult {

        // Upload current user key to the cloud
        if (VS_CODE_OK != vs_messenger_virgil_set_sign_in_password(password.toStdString().c_str())) {
            emit fireError(tr("Cannot set sign in password"));
            return VSQMessenger::EnResult::MRES_ERR_ENCRYPTION;
        }

        return MRES_OK;
    });
}

/******************************************************************************/
QFuture<VSQMessenger::EnResult>
VSQMessenger::signInWithBackupKeyAsync(QString username, QString password) {
    m_userId = _prepareLogin(username);
    return QtConcurrent::run([=]() -> EnResult {

        vs_messenger_virgil_user_creds_t creds;
        memset(&creds, 0, sizeof (creds));

        // Download private key from the cloud
        if (VS_CODE_OK != vs_messenger_virgil_sign_in_with_password(username.toStdString().c_str(), password.toStdString().c_str(), &creds)) {
            emit fireError(tr("Cannot set sign in password"));
            return MRES_ERR_SIGNUP;
        }

        // Save credentials
        _saveCredentials(username, creds);

        return _connect(m_user, m_settings->deviceId(), m_userId, true) ? MRES_OK : MRES_ERR_SIGNIN;;
    });
}

/******************************************************************************/
QFuture<VSQMessenger::EnResult>
VSQMessenger::signInAsync(QString user) {
    m_userId = _prepareLogin(user);
    return QtConcurrent::run([=]() -> EnResult {
        qDebug() << "Trying to sign in: " << m_userId;

        vs_messenger_virgil_user_creds_t creds;
        memset(&creds, 0, sizeof (creds));

        // Load User Credentials
        if (!_loadCredentials(m_userId, creds)) {
            qDebug() << "Sign-in error, code" << MRES_ERR_NO_CRED;
            return MRES_ERR_NO_CRED;
        }

        // Sign In user, using Virgil Service
        if (VS_CODE_OK != vs_messenger_virgil_sign_in(&creds)) {
            qDebug() << "Sign-in error, code:" << MRES_ERR_SIGNIN;
            return MRES_ERR_SIGNIN;
        }

        // Check previous run is crashed
        m_crashReporter->checkAppCrash();

        // Connect over XMPP
        return _connect(m_user, m_settings->deviceId(), m_userId, true) ? MRES_OK : MRES_ERR_SIGNIN;
    });
}



/******************************************************************************/
QFuture<VSQMessenger::EnResult>
VSQMessenger::signUpAsync(QString user) {
    m_userId = _prepareLogin(user);
    return QtConcurrent::run([=]() -> EnResult {
        qInfo() << "Trying to sign up: " << m_userId;

        vs_messenger_virgil_user_creds_t creds;
        memset(&creds, 0, sizeof (creds));

        VirgilIoTKit::vs_status_e status = vs_messenger_virgil_sign_up(m_userId.toStdString().c_str(), &creds);

        if (status != VS_CODE_OK) {
            emit fireError(tr("Cannot sign up user"));
            return MRES_ERR_USER_ALREADY_EXISTS;
        }

        qInfo() << "User has been successfully signed up: " << m_userId;

        // Save credentials
        _saveCredentials(m_userId, creds);

        // Connect over XMPP
        return _connect(m_user, m_settings->deviceId(), m_userId, true) ? MRES_OK : MRES_ERR_SIGNUP;
    });
}

/******************************************************************************/
bool VSQMessenger::subscribeToContact(const Contact::Id &contactId)
{
    return m_contactManager->addContact(contactId + "@" + xmppURL(), contactId, QString());
}

void VSQMessenger::onDownloadThumbnail(const MessageV0 message, const QString sender)
{
    qCDebug(lcTransferManager) << "Downloading of thumbnail for message:" << message.messageId;
    auto attachment = *message.attachment;
    if (attachment.thumbnailPath.isEmpty()) {
        attachment.thumbnailPath = m_attachmentBuilder.generateThumbnailFileName();
    }
    auto future = QtConcurrent::run([=]() {
        const TransferId id(message.messageId, TransferId::Type::Thumbnail);
        auto download = m_transferManager->startCryptoDownload(id, attachment.remoteThumbnailUrl, attachment.thumbnailPath, sender);
        bool success = false;
        QEventLoop loop;
        connect(download, &VSQDownload::ended, [&](bool failed) {
            success = !failed;
            loop.quit();
        });
        loop.exec();
        if(!success) {
            m_sqlConversations->setAttachmentStatus(message.messageId, AttachmentV0::Status::Created);
        }
    });
}

void VSQMessenger::onAttachmentStatusChanged(const QString &uploadId, const Enums::AttachmentStatus status)
{
    const auto id = TransferId::parse(uploadId);
    if (id.type == TransferId::Type::File) {
        m_sqlConversations->setAttachmentStatus(id.messageId, status);
    }
}

void VSQMessenger::onAttachmentProgressChanged(const QString &uploadId, const DataSize bytesReceived, const DataSize bytesTotal)
{
    const auto id = TransferId::parse(uploadId);
    if (id.type == TransferId::Type::File) {
        m_sqlConversations->setAttachmentProgress(id.messageId, bytesReceived, bytesTotal);
    }
}

void VSQMessenger::onAttachmentDecrypted(const QString &uploadId, const QString &filePath)
{
    const auto id = TransferId::parse(uploadId);
    if (id.type == TransferId::Type::File) {
        m_sqlConversations->setAttachmentFilePath(id.messageId, filePath);
    }
    else if (id.type == TransferId::Type::Thumbnail) {
        m_sqlConversations->setAttachmentThumbnailPath(id.messageId, filePath);
    }
}

/******************************************************************************/
QString
VSQMessenger::_virgilURL() {
    QString res = qgetenv("VS_MSGR_VIRGIL");
    if (res.isEmpty()) {
        switch (m_envType) {
        case PROD:
            res = Customer::MessengerUrlTemplate.arg(QString());
            break;

        case STG:
            res = Customer::MessengerUrlTemplate.arg(QLatin1String("-stg"));
            break;

        case DEV:
            res = Customer::MessengerUrlTemplate.arg(QLatin1String("-dev"));
            break;
        }
    }

    qDebug("%s URL: [%s]", qPrintable(Customer::OrganizationName), qPrintable(res));
    return res;
}

/******************************************************************************/
QString
VSQMessenger::xmppURL() const {
    QString res = qgetenv("VS_MSGR_XMPP_URL");

    if (res.isEmpty()) {
        switch (m_envType) {
        case PROD:
            res = Customer::XmppUrlTemplate.arg(QString());;
            break;

        case STG:
            res = Customer::XmppUrlTemplate.arg(QLatin1String("-stg"));
            break;

        case DEV:
            res = Customer::XmppUrlTemplate.arg(QLatin1String("-dev"));
            break;
        }
    }

    qDebug("XMPP URL: %s", res.toStdString().c_str());
    return res;
}

/******************************************************************************/
uint16_t
VSQMessenger::_xmppPort() {
    uint16_t res = 5222;
    QString portStr = qgetenv("VS_MSGR_XMPP_PORT");

    if (!portStr.isEmpty()) {
        bool ok;
        int port = portStr.toInt(&ok);
        if (ok) {
            res = static_cast<uint16_t> (port);
        }
    }

    qDebug("XMPP PORT: %d", static_cast<int> (res));

    return res;
}

/******************************************************************************/
// TODO: Use SecBox
bool
VSQMessenger::_saveCredentials(const QString &user, const vs_messenger_virgil_user_creds_t &creds) {
    // Save credentials
    const QByteArray baCred(reinterpret_cast<const char*>(&creds), sizeof(creds));

    QJsonObject jsonObject;
    jsonObject.insert("creds", QJsonValue::fromVariant(baCred.toBase64()));

    const QJsonDocument doc(jsonObject);
    const QString json = doc.toJson(QJsonDocument::Compact);

    qDebug() << "Saving user credentails: " << json;

    m_settings->setUserCredential(user, json);

    return true;
}

/******************************************************************************/
bool
VSQMessenger::_loadCredentials(const QString &user, vs_messenger_virgil_user_creds_t &creds) {
    const auto settingsJson = m_settings->userCredential(user);
    const QJsonDocument json(QJsonDocument::fromJson(settingsJson.toUtf8()));
    const auto baCred = QByteArray::fromBase64(json["creds"].toString().toUtf8());

    if (baCred.size() != sizeof(creds)) {
        qWarning("Cannot load credentials for : %s", user.toStdString().c_str());
        return false;
    }

    memcpy(&creds, baCred.data(), static_cast<size_t> (baCred.size()));
    return true;
}

/******************************************************************************/
QFuture<VSQMessenger::EnResult>
VSQMessenger::logoutAsync() {
    return QtConcurrent::run([=]() -> EnResult {
        qDebug() << "Logout";
        QMetaObject::invokeMethod(this, "deregisterFromNotifications", Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(&m_xmpp, "disconnectFromServer", Qt::BlockingQueuedConnection);
        vs_messenger_virgil_logout();
        m_user = "";
        m_userId = "";
        return MRES_OK;
    });
}

/******************************************************************************/
QFuture<VSQMessenger::EnResult> VSQMessenger::disconnect() {
    bool connected = m_xmpp.isConnected();
    return QtConcurrent::run([=]() -> EnResult {
        qDebug() << "Disconnect";
        if (connected) {
            QMetaObject::invokeMethod(&m_xmpp, "disconnectFromServer", Qt::BlockingQueuedConnection);
        }
        return MRES_OK;
    });
}

VSQLastActivityManager *VSQMessenger::lastActivityManager()
{
    return m_lastActivityManager;
}

void VSQMessenger::setCrashReporter(VSQCrashReporter *crashReporter)
{
    m_crashReporter = crashReporter;
}

/******************************************************************************/
void
VSQMessenger::onPushNotificationTokenUpdate() {
    registerForNotifications();
}

void
VSQMessenger::registerForNotifications() {
#if VS_PUSHNOTIFICATIONS

    if (!m_xmpp.isConnected()) {
        qInfo() << "Can not subscribe for push notifications, no connection. Will try it later.";
        return;
    }

    auto xmppPush = XmppPushNotifications::instance().buildEnableIq();

    xmppPush.setNode(currentUser() + "@" + xmppURL() + "/" + m_settings->deviceId());

#ifdef QT_DEBUG
    QString xml;
    QXmlStreamWriter xmlWriter(&xml);
    xmlWriter.setAutoFormatting(true);
    xmppPush.toXml(&xmlWriter);
    qDebug().noquote() << "Subscribe XMPP request:" << xml;
#endif

    const bool sentStatus = m_xmpp.sendPacket(xmppPush);

    qInfo() << "Subscribe for push notifications status: " << (sentStatus ? "sucess" : "failed");
#endif // VS_PUSHNOTIFICATIONS
}

void
VSQMessenger::deregisterFromNotifications() {
#if VS_PUSHNOTIFICATIONS

    if (!m_xmpp.isConnected()) {
        qInfo() << "Can not unsubscribe from push notifications, no connection.";
        return;
    }

    auto xmppPush = XmppPushNotifications::instance().buildDisableIq();

    xmppPush.setNode(currentUser() + "@" + xmppURL() + "/" + m_settings->deviceId());

#ifdef QT_DEBUG
    QString xml;
    QXmlStreamWriter xmlWriter(&xml);
    xmlWriter.setAutoFormatting(true);
    xmppPush.toXml(&xmlWriter);
    qDebug().noquote() << "Unsubscribe XMPP request:" << xml;
#endif

    const bool sentStatus = m_xmpp.sendPacket(xmppPush);

    qInfo() << "Unsubscribe from push notifications status: " << (sentStatus ? "sucess" : "failed");
#endif // VS_PUSHNOTIFICATIONS
}

/******************************************************************************/
void
VSQMessenger::onConnected() {
    registerForNotifications();
}

// TODO(fpohtmeh): use
/*
Optional<Attachment> VSQMessenger::uploadAttachment(const Message &message)
{
    if (!m_transferManager->isReady()) {
        qCDebug(lcMessenger) << "Transfer manager is not ready";
        emit messageStatusChanged(message.id, m_recipient, Message::Status::Failed);
        return NullOptional;
    }

    auto uploadedAttachment = *message.attachment;

    bool thumbnailUploadNeeded = attachment.type == AttachmentV0::Type::Picture && attachment.remoteThumbnailUrl.isEmpty();
    if (thumbnailUploadNeeded) {
        qCDebug(lcMessenger) << "Thumbnail uploading...";
        const TransferId uploadId(messageId, TransferId::Type::Thumbnail);
        if (m_transferManager->hasTransfer(uploadId)) {
            qCCritical(lcMessenger) << "Thumbnail upload for" << messageId << "already exists";
            return NullOptional;
        }
        else if (auto upload = m_transferManager->startCryptoUpload(uploadId, attachment.thumbnailPath, recipient)) {
            m_sqlConversations->setAttachmentStatus(messageId, AttachmentV0::Status::Loading);
            QEventLoop loop;
            QMutex guard;
            auto con = connect(upload, &VSQUpload::ended, [&](bool failed) {
                QMutexLocker l(&guard);
                if (failed) {
                    emit messageStatusChanged(messageId, recipient, Message::Status::Failed);
                }
                else {
                    m_sqlConversations->setAttachmentThumbnailRemoteUrl(messageId, *upload->remoteUrl());
                    uploadedAttachment.remoteThumbnailUrl = *upload->remoteUrl();
                    thumbnailUploadNeeded = false;
                }
                loop.quit();
            });
            connect(upload, &VSQUpload::connectionChanged, &loop, &QEventLoop::quit);
            qCDebug(lcMessenger) << "Upload waiting: start";
            loop.exec();
            QObject::disconnect(con);
            QMutexLocker l(&guard);
            qCDebug(lcMessenger) << "Upload waiting: end";
            if (!thumbnailUploadNeeded) {
                qCDebug(lcMessenger) << "Thumbnail was uploaded";
            }
        }
        else  {
            qCDebug(lcMessenger) << "Unable to start upload";
            emit messageStatusChanged(messageId, recipient, Message::Status::Failed);
            return NullOptional;
        }
    }

    bool attachmentUploadNeeded = attachment.remoteUrl.isEmpty();
    if (attachmentUploadNeeded) {
        qCDebug(lcMessenger) << "Attachment uploading...";
        const TransferId id(messageId, TransferId::Type::File);
        if (m_transferManager->hasTransfer(messageId)) {
            qCCritical(lcMessenger) << "Upload for" << messageId << "already exists";
            return NullOptional;
        }
        else if (auto upload = m_transferManager->startCryptoUpload(id, attachment.filePath, recipient)) {
            m_sqlConversations->setAttachmentStatus(messageId, AttachmentV0::Status::Loading);
            uploadedAttachment.bytesTotal = upload->fileSize();
            m_sqlConversations->setAttachmentBytesTotal(messageId, upload->fileSize());
            QEventLoop loop;
            QMutex guard;
            auto con = connect(upload, &VSQUpload::ended, [&](bool failed) {
                QMutexLocker locker(&guard);
                if (failed) {
                    emit messageStatusChanged(messageId, recipient, Message::Status::Failed);
                }
                else {
                    m_sqlConversations->setAttachmentRemoteUrl(messageId, *upload->remoteUrl());
                    uploadedAttachment.remoteUrl = *upload->remoteUrl();
                    attachmentUploadNeeded = false;
                }
                loop.quit();
            });
            connect(upload, &VSQUpload::connectionChanged, &loop, &QEventLoop::quit);
            qCDebug(lcMessenger) << ": start";
            loop.exec();
            QObject::disconnect(con);
            QMutexLocker locker(&guard);
            qCDebug(lcMessenger) << "Upload waiting: end";
            if (!attachmentUploadNeeded) {
                qCDebug(lcMessenger) << "Attachment was uploaded";
            }
        }
        else {
            qCDebug(lcMessenger) << "Unable to start upload";
            emit messageStatusChanged(messageId, recipient, Message::Status::Failed);
            return NullOptional;
        }
    }

    if (thumbnailUploadNeeded || attachmentUploadNeeded) {
        qCDebug(lcMessenger) << "Thumbnail or/and attachment were not uploaded";
        return NullOptional;
    }
    qCDebug(lcMessenger) << "Everything was uploaded";
    m_sqlConversations->setAttachmentStatus(messageId, AttachmentV0::Status::Loaded);
    uploadedAttachment.status = AttachmentV0::Status::Loaded;
    return uploadedAttachment;
}
*/

void
VSQMessenger::checkState() {
    if (vs_messenger_virgil_is_signed_in() && (m_xmpp.state() == QXmppClient::DisconnectedState)) {
        qCDebug(lcNetwork) << "We should be connected, but it's not so. Let's try to reconnect.";
#if VS_ANDROID
        emit fireError(tr("Disconnected ..."));
#endif
        _reconnect();
    } else {
#if 0
        qCDebug(lcNetwork) << "Connection is ok";
#endif
    }
}

/******************************************************************************/
void
VSQMessenger::_reconnect() {
    if (!m_user.isEmpty() && !m_userId.isEmpty() && vs_messenger_virgil_is_signed_in()) {
        QtConcurrent::run([=]() {
            _connect(m_user, m_settings->deviceId(), m_userId);
        });
    }
}

/******************************************************************************/
void
VSQMessenger::onDisconnected() {
    qDebug() << "onDisconnected  state:" << m_xmpp.state();
    qDebug() << "Carbons disable";
    m_xmppCarbonManager->setCarbonsEnabled(false);
}

/******************************************************************************/
void
VSQMessenger::onError(QXmppClient::Error err) {
    qDebug("onError");
    qDebug() << "onError : " << err << "   state:" << m_xmpp.state();
    emit fireError(tr("Connection error ..."));
}

void VSQMessenger::setApplicationActive(bool active)
{
    m_lastActivityManager->setEnabled(active);
}

void VSQMessenger::downloadAndProcess(MessageV0 message, const Function &func)
{
    if (!message.attachment) {
        qCDebug(lcTransferManager) << "Message has no attachment:" << message.messageId;
        return;
    }
    auto future = QtConcurrent::run([=]() {
        auto msg = message;
        auto &attachment = *msg.attachment;
        auto &filePath = attachment.filePath;
        if (QFile::exists(filePath)) {
            func(msg);
            return;
        }
        // Update attachment filePath
        const auto downloads = m_settings->downloadsDir();
        if (filePath.isEmpty() || QFileInfo(filePath).dir() != downloads) {
            filePath = Utils::findUniqueFileName(downloads.filePath(attachment.fileName));
        }
        const TransferId id(msg.messageId, TransferId::Type::File);
        auto download = m_transferManager->startCryptoDownload(id, attachment.remoteUrl, filePath, msg.sender);
        bool success = false;
        QEventLoop loop;
        QMutex guard;
        auto con = connect(download, &VSQDownload::ended, [&](bool failed) {
            QMutexLocker l(&guard);
            success = !failed;
            loop.quit();
        });
        connect(m_transferManager, &VSQCryptoTransferManager::fileDecrypted, download, [=](const QString &id, const QString &filePath) {
            qCDebug(lcTransferManager) << "Comparing of downloaded file with requested..." << filePath;
            if (TransferId::parse(id).messageId == msg.messageId) {
                auto msgWithPath = msg;
                msgWithPath.attachment->filePath = filePath;
                func(msgWithPath);
            }
        });
        loop.exec();
        QObject::disconnect(con);
        QMutexLocker l(&guard);

        if(!success) {
            m_sqlConversations->setAttachmentStatus(message.messageId, AttachmentV0::Status::Created);
        }
    });
}

void
VSQMessenger::setStatus(VSQMessenger::EnStatus status) {
    QXmppPresence::Type presenceType = VSQMessenger::MSTATUS_ONLINE == status ?
                QXmppPresence::Available : QXmppPresence::Unavailable;
    m_xmpp.setClientPresence(QXmppPresence(presenceType));
}

/******************************************************************************/
void VSQMessenger::setCurrentRecipient(const QString &recipient)
{
    m_recipient = recipient;
    if (recipient.isEmpty()) {
        m_lastActivityManager->setCurrentJid(QString());
    }
    else {
        m_lastActivityManager->setCurrentJid(recipient + "@" + xmppURL());
    }
}

void VSQMessenger::saveAttachmentAs(const QString &messageId, const QVariant &fileUrl)
{
    auto message = m_sqlConversations->getMessage(messageId);
    if (!message) {
        return;
    }
    const auto dest = Utils::urlToLocalFile(fileUrl.toUrl());
    qCDebug(lcTransferManager) << "Saving of attachment as" << dest;
    downloadAndProcess(*message, [=](const MessageV0 &msg) {
        qCDebug(lcTransferManager) << "Message attachment saved as:" << dest;
        const auto src = msg.attachment->filePath;
        QFile::copy(src, dest);
    });
}

void VSQMessenger::downloadAttachment(const QString &messageId)
{
    auto message = m_sqlConversations->getMessage(messageId);
    if (!message) {
        return;
    }
    qCDebug(lcTransferManager) << "Downloading of attachment:" << messageId;
    downloadAndProcess(*message, [this](const MessageV0 &msg) {
        qCDebug(lcTransferManager) << QString("Message '%1' attachment was downloaded").arg(msg.messageId);
        emit informationRequested("Saved to downloads");
    });
}

void VSQMessenger::openAttachment(const QString &messageId)
{
    auto message = m_sqlConversations->getMessage(messageId);
    if (!message) {
        return;
    }
    qCDebug(lcTransferManager) << "Opening of attachment:" << messageId;
    downloadAndProcess(*message, [this](const MessageV0 &msg) {
        const auto url = Utils::localFileToUrl(msg.attachment->filePath);
        qCDebug(lcTransferManager) << "Opening of message attachment:" << url;
        emit openPreviewRequested(url);
    });
}

/******************************************************************************/
void
VSQMessenger::onPresenceReceived(const QXmppPresence &presence) {
    Q_UNUSED(presence)
}

/******************************************************************************/
void
VSQMessenger::onIqReceived(const QXmppIq &iq) {
    Q_UNUSED(iq)
}

/******************************************************************************/
void
VSQMessenger::onSslErrors(const QList<QSslError> &errors) {
    Q_UNUSED(errors)
    emit fireError(tr("Secure connection error ..."));
}

/******************************************************************************/
void
VSQMessenger::onStateChanged(QXmppClient::State state) {
    if (QXmppClient::ConnectingState == state) {
        emit fireConnecting();
    }
}

/******************************************************************************/
