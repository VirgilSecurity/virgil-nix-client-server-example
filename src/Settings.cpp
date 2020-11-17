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

#include "Settings.h"

#include <QStandardPaths>

#include "VSQCustomer.h"
#include "Utils.h"

static const QString kUsersGroup = "Users";
static const QString kUsersList = "UsersList";
static const QString kCredenitalsGroup = "Credentials";

static const QString kDeviceId = "DeviceId";

static const QString kLastSessionGroup = "LastSession";
static const QString kWindowGeometryId = "WindowGeometry";
static const QString kSessionId = "SessionId";
static const QString kSignedInUserId = "SignedInUserId";

using namespace vm;

Q_LOGGING_CATEGORY(lcSettings, "settings")

Settings::Settings(QObject *parent)
    : QSettings(Customer::OrganizationName, Customer::ApplicationName, parent)
    , m_sessionId(Utils::createUuid())
{
    if (deviceId().isEmpty()) {
        createDeviceId();
        removeGroup(kUsersGroup);
        removeGroup(kCredenitalsGroup);
    }

    m_databaseDir.setPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/database"));
    m_attachmentCacheDir.setPath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/attachments"));
    m_thumbnaisDir.setPath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/thumbnails"));
    m_downloadsDir.setPath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
}

Settings::~Settings()
{
#ifdef VS_DEVMODE
    qCDebug(lcDev) << "~Settings";
#endif
}

void Settings::print()
{
    qCDebug(lcSettings) << "Settings";
    qCDebug(lcSettings) << "Device id:" << deviceId();
    qCDebug(lcSettings) << "Database dir:" << databaseDir().absolutePath();
    qCDebug(lcSettings) << "Attachment cache dir:" << attachmentCacheDir().absolutePath();
    qCDebug(lcSettings) << "Attachment max file size:" << attachmentMaxFileSize();
    qCDebug(lcSettings) << "Thumbnails dir:" << thumbnailsDir().absolutePath();
    qCDebug(lcSettings) << "Thumbnail max size:" << attachmentMaxFileSize();
    qCDebug(lcSettings) << "Downloads dir:" << downloadsDir().absolutePath();
    if (devMode()) {
        qCDebug(lcSettings) << "Dev mode:" << true;
    }
}

void Settings::setLastSignedInUserId(const QString &userId)
{
    if (lastSignedInUserId() == userId) {
        return;
    }
    setGroupValue(kLastSessionGroup, kSignedInUserId, userId);
    sync();
    emit lastSignedInUserIdChanged(userId);
}

QString Settings::lastSignedInUserId() const
{
    return groupValue(kLastSessionGroup, kSignedInUserId).toString();
}

void Settings::setUsersList(const QStringList &users)
{
    if (usersList() == users) {
        return;
    }
    setGroupValue(kUsersGroup, kUsersList, users);
    sync();
    emit usersListChanged(users);
}

QStringList Settings::usersList() const
{
    return groupValue(kUsersGroup, kUsersList).toStringList();
}

void Settings::addUserToList(const QString &user)
{
    auto users = usersList();
    // FIFO
    if (users.contains(user)) {
        users.removeOne(user);
    }
    setUsersList(QStringList(user) + users);
}

QString Settings::userCredential(const QString &user) const
{
    return groupValue(kCredenitalsGroup, user).toString();
}

void Settings::setUserCredential(const QString &user, const QString &credential)
{
    setGroupValue(kCredenitalsGroup, user, credential);
    sync();
}

QString Settings::deviceId() const
{
    return value(kDeviceId).toString();
}

bool Settings::runFlag() const
{
    auto lastSessionId = groupValue(kLastSessionGroup, kSessionId).toString();
    return !lastSessionId.isEmpty() && lastSessionId != m_sessionId;
}

void Settings::setRunFlag(bool run)
{
    if (run) {
        qCDebug(lcSettings) << "Save session id" << m_sessionId;
        setGroupValue(kLastSessionGroup, kSessionId, m_sessionId);
    }
    else {
        qCDebug(lcSettings) << "Reset session id";
        removeGroupKey(kLastSessionGroup, kSessionId);
    }
}

QDir Settings::databaseDir() const
{
    if (!m_databaseDir.exists()) {
        Utils::forceCreateDir(m_databaseDir.absolutePath());
    }
    return m_databaseDir;
}

DataSize Settings::attachmentMaxFileSize() const
{
    return 25 * 1024 * 1024;
}

QDir Settings::attachmentCacheDir() const
{
    if(!m_attachmentCacheDir.exists()) {
        Utils::forceCreateDir(m_attachmentCacheDir.absolutePath());
    }
    return m_attachmentCacheDir;
}

QDir Settings::thumbnailsDir() const
{
    if(!m_thumbnaisDir.exists()) {
        Utils::forceCreateDir(m_thumbnaisDir.absolutePath());
    }
    return m_thumbnaisDir;
}

QDir Settings::downloadsDir() const
{
    if(!m_downloadsDir.exists()) {
        Utils::forceCreateDir(m_downloadsDir.absolutePath());
    }
    return m_downloadsDir;
}

QString Settings::makeThumbnailPath(const Attachment::Id &attachmentId, bool isPreview) const
{
    return thumbnailsDir().filePath((isPreview ? QLatin1String("p-") : QLatin1String("t-")) + attachmentId + QLatin1String(".png"));
}

QSize Settings::thumbnailMaxSize() const
{
    return QSize(100, 80);
}

QSize Settings::previewMaxSize() const
{
    return 3 * thumbnailMaxSize();
}

bool Settings::devMode() const
{
#ifdef VS_DEVMODE
    return true;
#else
    return false;
#endif // VS_DEVMODE
}

bool Settings::fileCloudEnabled() const
{
#ifdef VS_NO_FILECLOUD
    return false;
#else
    return true;
#endif // VS_NO_FILECLOUD
}

QRect Settings::windowGeometry() const
{
    return groupValue(kLastSessionGroup, kWindowGeometryId).toRect();
}

void Settings::setWindowGeometry(const QRect &geometry)
{
    setGroupValue(kLastSessionGroup, kWindowGeometryId, geometry);
    sync();
}

Seconds Settings::nowInterval() const
{
    return 5;
}

QString Settings::makeGroupKey(const QString &group, const QString &key) const
{
    return group + '/' + key;
}

void Settings::setGroupValue(const QString &group, const QString &key, const QVariant &value)
{
    setValue(makeGroupKey(group, key), value);
    sync();
}

QVariant Settings::groupValue(const QString &group, const QString &key, const QVariant &defaultValue) const
{
    return value(makeGroupKey(group, key), defaultValue);
}

void Settings::removeGroupKey(const QString &group, const QString &key)
{
    remove(makeGroupKey(group, key));
}

void Settings::removeGroup(const QString &group)
{
    beginGroup(group);
    remove(QString());
    endGroup();
}

void Settings::createDeviceId()
{
    setValue(kDeviceId, Utils::createUuid());
    sync();
}