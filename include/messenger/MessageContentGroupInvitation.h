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

#ifndef VM_MESSAGE_CONTENT_GROUP_INVITATION_H
#define VM_MESSAGE_CONTENT_GROUP_INVITATION_H

#include "UserId.h"

#include <QString>
#include <QJsonObject>

namespace vm {

//
//  Class with a group invitation message content.
//
class MessageContentGroupInvitation
{

public:
    MessageContentGroupInvitation() = default;

    explicit MessageContentGroupInvitation(UserId superOwnerId, QString title, QString helloText = {});

    //
    //  Return identifier of owner that has unique ownership for a group.
    //
    UserId superOwnerId() const;

    //
    //  Return group title.
    //
    QString title() const;

    //
    //  Return greetings.
    //
    QString helloText() const;

    //
    //  Serialize object to JSON.
    //
    void writeJson(QJsonObject &json) const;

    //
    //  Restore object from JSON.
    //
    bool readJson(const QJsonObject &json);

private:
    UserId m_superOwnerId;
    QString m_title;
    QString m_helloText;
};
} // namespace vm

#endif // VM_MESSAGE_CONTENT_GROUP_INVITATION_H
