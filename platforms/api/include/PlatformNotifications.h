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

#ifndef VM_PLATFORM_PLATFORM_NOTIFICATONS
#define VM_PLATFORM_PLATFORM_NOTIFICATONS

#include <QObject>

namespace vm {
namespace platform {

//
//  Provides API for application notifications.
//
class PlatformNotifications : public QObject
{
    Q_OBJECT

signals:
    void pushTokenUpdated(const QString &pushToken);

public:
    //
    //  Perform platform dependent initialization.
    //
    virtual void init();

    //
    //  Return true if Push Notifications are supported by a platform.
    //
    virtual bool isPushSupported() const = 0;

    //
    //  Return true if Push Notifications are supported and push token is available.
    //
    bool isPushAvailable() const;

    //
    //  Return valid push token, or an empty string
    //  if push notifications are not supported or token is not available yet.
    //
    QString pushToken() const;

    //
    //  Update push token.
    //  Emit signal: 'pushTokenUpdated()'.
    //
    void updatePushToken(QString pushToken);

    //
    //  This method should be implemented within derived class.
    //
    static PlatformNotifications &instance();

private:
    QString m_pushToken;
};

} // namespace platform
} // namespace vm

#endif // VM_PLATFORM_PLATFORM_NOTIFICATONS
