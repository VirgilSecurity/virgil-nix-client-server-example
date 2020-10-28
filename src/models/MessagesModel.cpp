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

#include "models/MessagesModel.h"

#include "Utils.h"

using namespace vm;

MessagesModel::MessagesModel(QObject *parent)
    : QAbstractListModel(parent)
{}

MessagesModel::~MessagesModel()
{}

void MessagesModel::setMessages(const Messages &messages)
{
    beginResetModel();
    m_messages = messages;
    endResetModel();
}

Message MessagesModel::createMessage(const Chat::Id &chatId, const Contact::Id &authorId, const QString &body, const Optional<Attachment> &attachment)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    Message message;
    message.id = Utils::createUuid();
    message.timestamp = QDateTime::currentDateTime();
    message.chatId = chatId;
    message.authorId = authorId;
    message.body = body;
    message.attachment = attachment;
    m_messages.push_back(message);
    endInsertRows();
    return message;
}

int MessagesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_messages.size();
}

QVariant MessagesModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();
    const auto &message = m_messages[row];
    switch (role) {
    case IdRole:
        return message.id;
    case DayRole:
        return message.timestamp.date();
    case DisplayTimeRole:
        return message.timestamp.toString("• hh:mm");
    case AuthorIdRole:
        return message.authorId;
    case StatusRole:
    {
        const auto nextMessage = (row + 1 == rowCount()) ? nullptr : &m_messages[row + 1];
        if (nextMessage && nextMessage->authorId == message.authorId && nextMessage->status == message.status) {
            return QString();
        }
        return QVariant::fromValue(message.status);
    }
    case BodyRole:
        return message.body.split('\n').join("<br/>");
    case AttachmentIdRole:
    {
        if (message.attachment) {
            return message.attachment->id;
        }
        return QString();
    }
    case AttachmentTypeRole:
    {
        if (message.attachment) {
            return QVariant::fromValue(message.attachment->type);
        }
        return QVariant();
    }
    case AttachmentStatusRole:
    {
        if (message.attachment) {
            return QVariant::fromValue(message.attachment->status);
        }
        return QVariant();
    }
    case AttachmentImageRole:
    {
        // TODO(fpohtmeh): implement
        return QVariant();
    }
    case AttachmentImageSizeRole:
    {
        // TODO(fpohtmeh): implement
        return QVariant();
    }
    case AttachmentDisplaySizeRole:
    {
        // TODO(fpohtmeh): implement
        return QVariant();
    }
    case AttachmentBytesTotalRole:
    {
        // TODO(fpohtmeh): implement
        return QVariant();
    }
    case AttachmentBytesLoadedRole:
    {
        // TODO(fpohtmeh): implement
        return QVariant();
    }
    case AttachmentFileExistsRole:
    {
        // TODO(fpohtmeh): implement
        return QVariant();
    }
    case FailedRole:
    {
        if (message.status == Message::Status::Failed) {
            return true;
        }
        if (message.attachment) {
            const auto &status = message.attachment->status;
            return status == Attachment::Status::Failed || status == Attachment::Status::Invalid;
        }
        return false;
    }
    case FirstInRowRole:
    {
        const auto prevMessage = (row == 0) ? nullptr : &m_messages[row - 1];
        return !prevMessage || prevMessage->authorId != message.authorId || prevMessage->timestamp.addSecs(5 * 60) <= message.timestamp;
    }
    case InRowRole:
    {
        const auto nextMessage = (row + 1 == rowCount()) ? nullptr : &m_messages[row + 1];
        return nextMessage && message.authorId == nextMessage->authorId;
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> MessagesModel::roleNames() const
{
    return {
        { IdRole, "id" },
        { DayRole, "day" },
        { DisplayTimeRole, "displayTime" },
        { AuthorIdRole, "authorId" },
        { StatusRole, "status" },
        { BodyRole, "body" },
        { AttachmentIdRole, "attachmentId" },
        { AttachmentTypeRole, "attachmentType" },
        { AttachmentStatusRole, "attachmentStatus" },
        { AttachmentImageRole, "attachmentImage" },
        { AttachmentImageSizeRole, "attachmentImageSize" },
        { AttachmentDisplaySizeRole, "attachmentDisplaySize" },
        { AttachmentBytesTotalRole, "attachmentBytesTotal" },
        { AttachmentBytesLoadedRole, "attachmentBytesLoaded" },
        { AttachmentFileExistsRole, "attachmentFileExists" },
        { FailedRole, "failed" },
        { FirstInRowRole, "firstInRow" },
        { InRowRole, "inRow" },
    };
}
