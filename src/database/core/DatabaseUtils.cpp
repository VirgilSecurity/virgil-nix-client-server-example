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

#include "database/core/DatabaseUtils.h"

#include <QSqlError>
#include <QSqlQuery>

#include "Utils.h"
#include "database/core/Database.h"

using namespace vm;

namespace
{
    QString queryPath(const QString &fileName)
    {
        return QString(":/resources/database/%1.sql").arg(fileName);
    }

    Optional<QStringList> readQueryTexts(const QString &queryId)
    {
        const auto text = Utils::readTextFile(queryPath(queryId));
        QStringList queries;
        const auto texts = text->split(";");
        for (auto text : texts) {
            auto query = text.trimmed();
            if (!query.isEmpty()) {
                queries << query;
            }
        }
        return queries;
    }
}

bool DatabaseUtils::isValidName(const QString &id)
{
    // TODO(fpohtmeh): add regexp check
    return !id.isEmpty();
}

bool DatabaseUtils::readExecQueries(Database *database, const QString &queryId)
{
    const auto texts = readQueryTexts(queryId);
    if (!texts) {
        return false;
    }
    for (auto text : *texts) {
        auto query = database->createQuery();
        if (!query.exec(text)) {
            qCCritical(lcDatabase) << "Failed to run query:" << text;
            return false;
        }
    }
    return true;
}

Optional<QSqlQuery> DatabaseUtils::readExecQuery(Database *database, const QString &queryId, const BindValues &values)
{
    const auto text = Utils::readTextFile(queryPath(queryId));
    if (!text) {
        return NullOptional;
    }
    auto query = database->createQuery();
    if (!query.prepare(*text)) {
        qCCritical(lcDatabase) << "Failed to prepare query:" << query.lastError().databaseText();
        return NullOptional;
    }
    for (auto &v : values) {
        query.bindValue(v.first, v.second);
    }
    if (!query.exec()) {
        qCCritical(lcDatabase) << "Failed to exec query:" << query.lastError().databaseText();
        return NullOptional;
    }
    return query;
}

Optional<Attachment> DatabaseUtils::readAttachment(const QSqlQuery &query)
{
    const auto attachmentId = query.value("attachmentId").value<Attachment::Id>();
    if (attachmentId.isEmpty()) {
        return NullOptional;
    }
    Attachment attachment;
    attachment.id = attachmentId;
    attachment.type = static_cast<Attachment::Type>(query.value("attachmentType").toInt());
    attachment.status = static_cast<Attachment::Status>(query.value("attachmentStatus").toInt());
    attachment.fileName = query.value("attachmentFilename").toString();
    attachment.size = query.value("attachmentSize").value<DataSize>();
    attachment.localPath = query.value("attachmentLocalPath").toString();
    attachment.fingerprint = query.value("attachmentFingerprint").toString();
    attachment.url = query.value("attachmentUrl").toUrl();
    attachment.encryptedSize = query.value("attachmentEncryptedSize").value<DataSize>();
    attachment.extras = Utils::extrasFromJson(query.value("attachmentExtras").toString(), attachment.type, false);
    return attachment;
}

Optional<Message> DatabaseUtils::readMessage(const QSqlQuery &q, const QString &idColumn)
{
    const auto idc = idColumn.isEmpty() ? QLatin1String("messageId") : idColumn;
    const auto messageId = q.value(idc).value<Message::Id>();
    if (messageId.isEmpty()) {
        return NullOptional;
    }
    Message message;
    message.id = messageId;
    message.timestamp = q.value("messageTimestamp").toDateTime();
    message.chatId = q.value("messageChatId").value<Chat::Id>();
    message.authorId = q.value("messageAuthorId").value<Contact::Id>();
    message.status = static_cast<Message::Status>(q.value("messageStatus").toInt());
    message.body = q.value("messageBody").toString();
    message.attachment = DatabaseUtils::readAttachment(q);
    return message;
}