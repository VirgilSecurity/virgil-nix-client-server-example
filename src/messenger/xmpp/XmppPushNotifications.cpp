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

#include "XmppPushNotifications.h"

#include "VSQCustomer.h"
#include "PlatformNotifications.h"

#include <qxmpp/QXmppMessage.h>
#include <qxmpp/QXmppPushEnableIq.h>
#include <qxmpp/QXmppDataForm.h>

using namespace vm::platform;
using namespace notifications;
using namespace notifications::xmpp;
using namespace Customer;

using Self = XmppPushNotifications;

static const QString kPushNotificationsDeviceID = "device_id";
static const QString kPushNotificationsFormType = "FORM_TYPE";
static const QString kPushNotificationsFormTypeVal = "http://jabber.org/protocol/pubsub#publish-options";

static const QString kPushNotificationsService = "service";

#if VS_ANDROID
static const QString kPushNotificationsServiceVal = "fcm";
#elif VS_IOS
static const QString kPushNotificationsServiceVal = "apns";
#else
static const QString kPushNotificationsServiceVal = "<undefined>";
#endif

#ifdef QT_DEBUG
static const QString kPushNotificationsPushModeVal = "dev";
#else
static const QString kPushNotificationsPushModeVal = "prod";
#endif // QT_DEBUG

Self &Self::instance()
{
    static Self instance;
    return instance;
}

QXmppPushEnableIq Self::buildEnableIq() const
{
    // Create request
    QXmppPushEnableIq xmppPush;
    xmppPush.setType(QXmppIq::Set);
    xmppPush.setMode(QXmppPushEnableIq::Enable);
    xmppPush.setJid(kPushNotificationsProxy);

    // Create a Data Form fields
    QList<QXmppDataForm::Field> fields;

    { // Subscribe Form Type
        QXmppDataForm::Field field;
        field.setKey(kPushNotificationsFormType);
        field.setValue(kPushNotificationsFormTypeVal);
        fields << field;
    }
    { // Subscribe service
        QXmppDataForm::Field field;
        field.setKey(kPushNotificationsService);
        field.setValue(kPushNotificationsServiceVal);
        fields << field;
    }
    { // Subscribe device
        QXmppDataForm::Field field;
        field.setKey(kPushNotificationsDeviceID);
        field.setValue(PlatformNotifications::instance().pushToken());
        fields << field;
    }
#if VS_IOS
    {
        QXmppDataForm::Field field;
        field.setKey("topic");
        field.setValue(kPushNotificationsTopic); // FIXME: Restore ENV functionality ".stg", ".dev"
        fields << field;
    }
    {
        QXmppDataForm::Field field;
        field.setKey("sound");
        field.setValue("default");
        fields << field;
    }
    {
        QXmppDataForm::Field field;
        field.setKey("push_mode");
        field.setValue(kPushNotificationsPushModeVal);
        fields << field;
    }
#endif // VS_IOS

    QXmppDataForm dataForm;
    dataForm.setType(QXmppDataForm::Submit);
    dataForm.setFields(fields);

    // Setup the Data Form fields
    xmppPush.setDataForm(dataForm);

    return xmppPush;
}

QXmppPushEnableIq Self::buildDisableIq() const
{
    QXmppPushEnableIq xmppPush;
    xmppPush.setType(QXmppIq::Set);
    xmppPush.setMode(QXmppPushEnableIq::Disable);
    xmppPush.setJid(kPushNotificationsProxy);

    return xmppPush;
}
