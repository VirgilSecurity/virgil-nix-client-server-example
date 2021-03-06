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

#ifndef VM_LASTACTIVITYMANAGER_H
#define VM_LASTACTIVITYMANAGER_H

#include <qxmpp/QXmppClientExtension.h>

#include "XmppLastActivityIq.h"

Q_DECLARE_LOGGING_CATEGORY(lcLastActivityManager)

namespace vm {

class Settings;

class XmppLastActivityManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    XmppLastActivityManager(Settings *settings);
    ~XmppLastActivityManager() override;

    void setCurrentJid(const QString &jid);
    void setEnabled(bool enabled);

    QStringList discoveryFeatures() const override;
    bool handleStanza(const QDomElement &element) override;

signals:
    void lastActivityDetected(std::chrono::seconds seconds);
    void lastActivityMissing(const QString &reason);
    void lastActivityTextChanged(const QString &text);
    void errorOccured(const QString &errorText);

private:
    void timerEvent(QTimerEvent *) override;

    QString requestInfo();
    bool canStart() const;

    void startUpdates(bool reset);
    void stopUpdates(bool reset);

    void onErrorOccured(const QString &errorText);

    Settings *m_settings;
    bool m_enabled = true;
    QString m_jid;
    int m_timerId = 0;
};

} // namespace vm

#endif // VM_LASTACTIVITYMANAGER_H
