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

#include "states/ChatListState.h"

#include "ChatsController.h"
#include "ChatsModel.h"
#include "Controllers.h"
#include "Messenger.h"
#include "UsersController.h"

#include <QTimer>

using namespace vm;
using Self = ChatListState;

Self::ChatListState(Messenger *messenger, Controllers *controllers, ChatsModel *model, QState *parent)
    : QState(parent), m_messenger(messenger), m_controllers(controllers), m_model(model)
{
    connect(m_messenger, &Messenger::onlineStatusChanged, this, &Self::trySignIn);
    connect(controllers->users(), &UsersController::signInErrorOccured, this, &Self::retrySignIn);
}

void Self::onEntry(QEvent *)
{
    m_model->clearFilter();
    m_controllers->chats()->closeChat();
    trySignIn();
}

void Self::trySignIn()
{
    const auto username = m_messenger->settings()->lastSignedInUser();
    if (!username.isEmpty() && m_messenger->isReadyToSignIn()) {
        m_controllers->users()->signIn(username);
    }
}

void Self::retrySignIn()
{
    QTimer::singleShot(m_messenger->settings()->retrySignInInterval(), this, &Self::trySignIn);
}
