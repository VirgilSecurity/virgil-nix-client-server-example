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
//

#ifndef VM_XMPP_ROOM_PARTICIPANTS_MANAGER_H
#define VM_XMPP_ROOM_PARTICIPANTS_MANAGER_H

#include <qxmpp/QXmppClientExtension.h>
#include <qxmpp/QXmppMucIq.h>
#include <qxmpp/QXmppClient.h>

#include <QPointer>

namespace vm {

//
//  TODO: Add handle <presence/> with membership changes.
//
class XmppRoomParticipantsManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    //
    //  Should be called on the same thread where QXmppClient was created.
    //
    void requestAll(const QString &roomJid);

    //
    //  Handle IQs with room participants.
    //
    bool handleStanza(const QDomElement &element) override;

Q_SIGNALS:
    //
    //  This signal is emitted when a room participant info received.
    //
    void participantReceived(const QString &roomJid, const QString &jid, QXmppMucItem::Affiliation affiliation);

    //
    //  This signal is emitted when a room participant affiliation was changed.
    //
    void participantAffiliationChanged(const QString &roomJid, const QString &jid,
                                       QXmppMucItem::Affiliation affiliation);

protected:
    void setClient(QXmppClient *client) override;

private:
    QPointer<QXmppClient> m_client;
};

} // namespace vm

#endif // VM_XMPP_ROOM_PARTICIPANTS_MANAGER_H