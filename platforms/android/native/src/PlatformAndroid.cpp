//  Copyright (C) 2015-2021 Virgil Security, Inc.
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

#include "PlatformAndroid.h"

#include <QtAndroid>

#include "PlatformNotifications.h"

#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>

using namespace vm;
using namespace platform;

using Self = PlatformAndroid;

static int pfd[2];
static pthread_t loggingThread;

Platform &Platform::instance()
{
    return Self::instance();
}

Self &Self::instance()
{
    static Self impl;
    return impl;
}

static bool jniCheckOrRequestPermission(const QString &permission)
{
    auto result = QtAndroid::checkPermission(permission);
    if (result == QtAndroid::PermissionResult::Denied) {
        auto resultHash = QtAndroid::requestPermissionsSync(QStringList({ permission }));
        if (resultHash[permission] == QtAndroid::PermissionResult::Denied) {
            return false;
        }
    }
    return true;
}

static bool jniRequestPermissions()
{
    const QStringList permissions({ "android.permission.WRITE_EXTERNAL_STORAGE",
                                    "android.permission.READ_EXTERNAL_STORAGE", "android.permission.READ_CONTACTS" });

    for (const QString &permission : permissions) {
        if (!jniCheckOrRequestPermission(permission)) {
            return false;
        }
    }

    return true;
}

static void *loggingFunction(void *)
{
    ssize_t readSize;
    char buf[4096];

    while ((readSize = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if (buf[readSize - 1] == '\n') {
            --readSize;
        }

        buf[readSize] = 0; // add null-terminator

        __android_log_write(ANDROID_LOG_DEBUG, "", buf); // Set any log level you want
    }

    return nullptr;
}

static bool runLoggingThread()
{
    // run this function to redirect your output to android log
    setvbuf(stdout, nullptr, _IOLBF, 0); // make stdout line-buffered
    setvbuf(stderr, nullptr, _IONBF, 0); // make stderr unbuffered

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if (pthread_create(&loggingThread, nullptr, loggingFunction, nullptr) == -1) {
        return false;
    }

    pthread_detach(loggingThread);

    return true;
}

bool Self::init() const
{
    const bool isLoggingThreadStarted = runLoggingThread();
    Q_UNUSED(isLoggingThreadStarted); // It is not critical, so can be ignored.

    auto destCertFile = caBundlePath();
    QFile::remove(destCertFile);
    const bool isCertPrepared = QFile::copy(":qml/resources/cert.pem", destCertFile);
    if (!isCertPrepared) {
        return false;
    }

    return true;
}

bool Self::uiInit() const
{
    if (!jniRequestPermissions()) {
        return false;
    }

    PlatformNotifications::instance().init();

    return true;
}

QString Self::caBundlePath() const
{
    return appDataLocation().filePath("cert.pem");
}
