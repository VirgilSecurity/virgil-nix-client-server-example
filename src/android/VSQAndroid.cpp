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

#if (VS_ANDROID)

#include <QtCore>
#include <QtAndroid>

#include "android/VSQAndroid.h"
#include "Utils.h"

#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>

using namespace vm;

// TODO(fpohtmeh): Remove it
static int pfd[2];
static pthread_t loggingThread;

/******************************************************************************/
QString VSQAndroid::caBundlePath() {
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appDataPath + QDir::separator() + "cert.pem";
}

/******************************************************************************/
static bool _checkPermissions() {
    const QStringList permissions({
        "android.permission.WRITE_EXTERNAL_STORAGE",
        "android.permission.READ_EXTERNAL_STORAGE",
        "android.permission.READ_CONTACTS"
    });
    for(const QString &permission : permissions) {
        auto result = QtAndroid::checkPermission(permission);
        if(result == QtAndroid::PermissionResult::Denied){
            auto resultHash = QtAndroid::requestPermissionsSync(QStringList({permission}));
            if(resultHash[permission] == QtAndroid::PermissionResult::Denied)
                return false;
        }
    }

    return true;
}

/******************************************************************************/
bool VSQAndroid::prepare() {
    _checkPermissions();
    runLoggingThread();
    auto certFile = caBundlePath();
    QFile::remove(certFile);
    QFile::copy(":qml/resources/cert.pem", certFile);

    return true;
}

/******************************************************************************/
void VSQAndroid::hideSplashScreen()
{
    QtAndroid::hideSplashScreen();
}

/******************************************************************************/
static void *loggingFunction(void*) {
    ssize_t readSize;
    char buf[128];

    while((readSize = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if(buf[readSize - 1] == '\n') {
            --readSize;
        }

        buf[readSize] = 0;  // add null-terminator

        __android_log_write(ANDROID_LOG_DEBUG, "", buf); // Set any log level you want
    }

    return nullptr;
}

/******************************************************************************/
int VSQAndroid::runLoggingThread() { // run this function to redirect your output to android log
    setvbuf(stdout, nullptr, _IOLBF, 0); // make stdout line-buffered
    setvbuf(stderr, nullptr, _IONBF, 0); // make stderr unbuffered

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if(pthread_create(&loggingThread, nullptr, loggingFunction, nullptr) == -1) {
        return -1;
    }

    pthread_detach(loggingThread);

    return 0;
}

/******************************************************************************/

QString VSQAndroid::getDisplayName(const QUrl &url)
{
    const QString urlString = url.toString();
    const auto javaUrl = QAndroidJniObject::fromString(urlString);
    const auto javaDisplayName = QAndroidJniObject::callStaticObjectMethod(
        "org/virgil/utils/Utils",
        "getDisplayName",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        QtAndroid::androidContext().object(),
        javaUrl.object<jstring>()
    );
    return javaDisplayName.toString();
}

DataSize VSQAndroid::getFileSize(const QUrl &url)
{
    const QString urlString = url.toString();
    const auto javaUrl = QAndroidJniObject::fromString(urlString);
    const auto javaFileSize = QAndroidJniObject::callStaticMethod<jint>(
        "org/virgil/utils/Utils",
        "getFileSize",
        "(Landroid/content/Context;Ljava/lang/String;)I",
        QtAndroid::androidContext().object(),
        javaUrl.object<jstring>()
    );
    return static_cast<DataSize>(javaFileSize);
}

Contacts VSQAndroid::getContacts()
{
    const auto javaStr = QAndroidJniObject::callStaticObjectMethod(
        "org/virgil/utils/ContactUtils",
        "getContacts",
        "(Landroid/content/Context;)Ljava/lang/String;",
        QtAndroid::androidActivity().object<jobject>()
    );
    const auto lines = javaStr.toString().split('\n');

    Contacts contacts;
    for (int i = 0, s = lines.size() - 4; i <= s; i += 4) {
        Contact contact;
        contact.name = lines[i];
        contact.phoneNumber = lines[i + 1];
        contact.email = lines[i + 2];
        const auto &platformIdStr = lines[i + 3];
        contact.id = QLatin1String("AndroidContact(%1)").arg(platformIdStr);
        contact.platformId = platformIdStr.toLongLong();
        contact.avatarUrlRetryCount = 1;
        contacts.push_back(contact);
    }
    return contacts;
}

QUrl VSQAndroid::getContactAvatarUrl(const Contact &contact)
{
    const QString idString = QString::number(contact.platformId.toLongLong());
    const auto javaIdString = QAndroidJniObject::fromString(idString);
    const auto javaFilePath = QAndroidJniObject::callStaticObjectMethod(
        "org/virgil/utils/ContactUtils",
        "getContactThumbnailUrl",
        "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;",
        QtAndroid::androidContext().object(),
        javaIdString.object<jstring>()
    );
    return Utils::localFileToUrl(javaFilePath.toString());
}

#endif // VS_ANDROID
