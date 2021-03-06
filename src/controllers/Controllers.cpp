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

#include "controllers/Controllers.h"

#include "Settings.h"
#include "Messenger.h"
#include "PlatformDocumentInteractionController.h"
#include "controllers/AttachmentsController.h"
#include "controllers/ChatsController.h"
#include "controllers/CloudFilesController.h"
#include "controllers/MessagesController.h"
#include "controllers/UsersController.h"
#include "database/UserDatabase.h"
#include "models/DiscoveredContactsModel.h"
#include "models/Models.h"

using namespace vm;
using namespace vm::platform;

Controllers::Controllers(Messenger *messenger, Settings *settings, Models *models, UserDatabase *userDatabase,
                         QObject *parent)
    : QObject(parent),
      m_attachments(new AttachmentsController(settings, models, this)),
      m_users(new UsersController(messenger, models, userDatabase, this)),
      m_chats(new ChatsController(messenger, models, userDatabase, this)),
      m_messages(new MessagesController(messenger, settings, models, userDatabase, this)),
      m_cloudFiles(new CloudFilesController(messenger, models, userDatabase, this)),
      m_documentInteraction(createDocumentInteraction())
{
    connect(m_attachments, &AttachmentsController::notificationCreated, this, &Controllers::notificationCreated);
    connect(m_messages, &MessagesController::notificationCreated, this, &Controllers::notificationCreated);
    connect(m_chats, &ChatsController::notificationCreated, this, &Controllers::notificationCreated);
    connect(m_cloudFiles, &CloudFilesController::notificationCreated, this, &Controllers::notificationCreated);
    connect(m_users, &UsersController::notificationCreated, this, &Controllers::notificationCreated);
    connect(m_documentInteraction, &PlatformDocumentInteractionController::notificationCreated, this,
            &Controllers::notificationCreated);

    connect(m_attachments, &AttachmentsController::openUrlRequested, m_documentInteraction,
            &PlatformDocumentInteractionController::openUrl);
    connect(m_cloudFiles, &CloudFilesController::openUrlRequested, m_documentInteraction,
            &PlatformDocumentInteractionController::openUrl);

    connect(userDatabase, &UserDatabase::opened, m_chats, &ChatsController::loadChats);
    connect(userDatabase, &UserDatabase::closed, m_chats, &ChatsController::clearChats);
    connect(userDatabase, &UserDatabase::closed, m_cloudFiles, &CloudFilesController::clearFiles);
    connect(m_chats, &ChatsController::chatOpened, m_messages, &MessagesController::loadChat);
    connect(m_chats, &ChatsController::chatCreated, m_messages, &MessagesController::loadNewChat);
    connect(m_chats, &ChatsController::chatClosed, m_messages, &MessagesController::closeChat);

    qRegisterMetaType<AttachmentsController *>("AttachmentsController*");
    qRegisterMetaType<ChatsController *>("ChatsController*");
    qRegisterMetaType<MessagesController *>("MessagesController*");
    qRegisterMetaType<UsersController *>("UsersController*");
    qRegisterMetaType<CloudFilesController *>("CloudFilesController*");
    qRegisterMetaType<platform::PlatformDocumentInteractionController *>("DocumentInteractionController*");
}

const AttachmentsController *Controllers::attachments() const
{
    return m_attachments;
}

AttachmentsController *Controllers::attachments()
{
    return m_attachments;
}

const UsersController *Controllers::users() const
{
    return m_users;
}

UsersController *Controllers::users()
{
    return m_users;
}

const ChatsController *Controllers::chats() const
{
    return m_chats;
}

ChatsController *Controllers::chats()
{
    return m_chats;
}

const MessagesController *Controllers::messages() const
{
    return m_messages;
}

MessagesController *Controllers::messages()
{
    return m_messages;
}

const CloudFilesController *Controllers::cloudFiles() const
{
    return m_cloudFiles;
}

CloudFilesController *Controllers::cloudFiles()
{
    return m_cloudFiles;
}

const PlatformDocumentInteractionController *Controllers::documentInteraction() const
{
    return m_documentInteraction;
}

PlatformDocumentInteractionController *Controllers::documentInteraction()
{
    return m_documentInteraction;
}

PlatformDocumentInteractionController *Controllers::createDocumentInteraction()
{
    return PlatformDocumentInteractionController::create(this);
}
