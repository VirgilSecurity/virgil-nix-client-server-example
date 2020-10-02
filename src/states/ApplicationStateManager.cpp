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

#include "states/ApplicationStateManager.h"

#include "VSQMessenger.h"

using namespace VSQ;

ApplicationStateManager::ApplicationStateManager(VSQMessenger *messenger, VSQSettings *settings, QObject *parent)
    : QStateMachine(parent)
    , m_messenger(messenger)
    , m_settings(settings)
    , m_startState(new StartState(this))
    , m_splashScreenState(new SplashScreenState(messenger, settings, this))
    , m_accountSelectionState(new AccountSelectionState(messenger, this))
    , m_chatListState(new ChatListState(this))
    , m_chatState(new ChatState(this))
    , m_newChatState(new NewChatState(messenger, this))
    , m_accountSettingsState(new AccountSettingsState(this))
    , m_backupKeyState(new BackupKeyState(settings, this))
{
    registerStatesMetaTypes();
    setupConnections();
    addTransitions();
    setInitialState(m_startState);
    start();
}

ApplicationStateManager::~ApplicationStateManager()
{}

void ApplicationStateManager::registerStatesMetaTypes()
{
    // Qt requires registering to avoid namespace issues
    qRegisterMetaType<StartState *>("StartState*");
    qRegisterMetaType<SplashScreenState *>("SplashScreenState*");
    qRegisterMetaType<AccountSelectionState *>("AccountSelectionState*");
    qRegisterMetaType<ChatListState *>("ChatListState*");
    qRegisterMetaType<ChatState *>("ChatState*");
    qRegisterMetaType<NewChatState *>("NewChatState*");
    qRegisterMetaType<AccountSettingsState *>("AccountSettingsState*");
    qRegisterMetaType<BackupKeyState *>("BackupKeyState*");
}

void ApplicationStateManager::setupConnections()
{
    // NOTE: Queued connection is needed for correct transition
    connect(this, &ApplicationStateManager::setUiState, this, std::bind(&ApplicationStateManager::splashScreenRequested, this, QPrivateSignal()), Qt::QueuedConnection);
    connect(this, &ApplicationStateManager::signOut, m_messenger, &VSQMessenger::signOut);
    connect(this, &ApplicationStateManager::openChat, this, &ApplicationStateManager::onOpenChat);
    connect(this, &ApplicationStateManager::addContact, m_newChatState, &NewChatState::addContact);
    connect(m_newChatState, &NewChatState::addContactFinished, this, &ApplicationStateManager::openChat);
}

void ApplicationStateManager::addTransitions()
{
    m_startState->addTransition(this, &ApplicationStateManager::splashScreenRequested, m_splashScreenState);
    m_splashScreenState->addTransition(m_splashScreenState, &SplashScreenState::signInUserNotSelected, m_accountSelectionState);
    m_splashScreenState->addTransition(m_splashScreenState, &SplashScreenState::signInErrorOccurred, m_accountSelectionState);
    m_splashScreenState->addTransition(m_splashScreenState, &SplashScreenState::signInFinished, m_chatListState);
    m_accountSelectionState->addTransition(m_accountSelectionState, &AccountSelectionState::signInFinished, m_chatListState);
    m_chatListState->addTransition(m_messenger, &VSQMessenger::signedOut, m_accountSelectionState);
    m_chatListState->addTransition(this, &ApplicationStateManager::openAccountSettings, m_accountSettingsState);
    m_chatListState->addTransition(this, &ApplicationStateManager::openAddContact, m_newChatState);
    m_chatListState->addTransition(this, &ApplicationStateManager::chatRequested, m_chatState);
    m_accountSettingsState->addTransition(m_messenger, &VSQMessenger::signedOut, m_accountSelectionState);
    m_accountSettingsState->addTransition(this, &ApplicationStateManager::goBack, m_chatListState);
    m_newChatState->addTransition(this, &ApplicationStateManager::goBack, m_chatListState);
    m_newChatState->addTransition(this, &ApplicationStateManager::chatRequested, m_chatState);
    m_chatState->addTransition(this, &ApplicationStateManager::goBack, m_chatListState);
}

void ApplicationStateManager::onOpenChat(const QString &contactId)
{
    m_chatState->setContactId(contactId);
    emit chatRequested(QPrivateSignal());
}
