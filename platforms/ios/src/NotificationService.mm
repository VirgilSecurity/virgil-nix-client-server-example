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

#import "NotificationService.h"

#include "CoreMessenger.h"
#include "Logging.h"
#include "Settings.h"

#include <memory>

using namespace vm;

Q_LOGGING_CATEGORY(lcNotificationExtension, "notification-extension");

@interface NotificationService ()

@property (nonatomic, strong) void (^contentHandler)(UNNotificationContent* contentToDeliver);
@property (nonatomic, strong) UNMutableNotificationContent* bestAttemptContent;

@end

@implementation NotificationService

- (void)didReceiveNotificationRequest:(UNNotificationRequest*)request
                   withContentHandler:(void (^)(UNNotificationContent* _Nonnull))contentHandler
{
    Logging logging;
    Settings settings;

    self.contentHandler = contentHandler;
    self.bestAttemptContent = [request.content mutableCopy];

    //
    //  Extract encrypted message.
    //
    auto encryptedContent = request.content.body;
    auto senderJid = (NSString*)request.content.userInfo[@"from"];
    auto recipientJid = (NSString*)request.content.userInfo[@"to"];
    auto ciphertext = (NSString*)request.content.userInfo[@"ciphertext"];

    if (0 == senderJid.length) {
        qCWarning(lcNotificationExtension) << "Required field 'from' is missing. Show notification as is.";
        self.contentHandler(self.bestAttemptContent);
        return;
    } else {
        qCDebug(lcNotificationExtension) << "Sender JID" << senderJid;
    }

    const bool isGroupMessage = (0 == ciphertext.length);
    if (isGroupMessage) {
        //
        //  Show group message as is.
        //
        self.contentHandler(self.bestAttemptContent);
        return;
    }

    if (0 == recipientJid.length) {
        qCWarning(lcNotificationExtension) << "Required field 'to' is missing. Show notification as is.";
        self.contentHandler(self.bestAttemptContent);
        return;
    } else {
        qCDebug(lcNotificationExtension) << "Recipient JID" << recipientJid;
    }

    if (0 == ciphertext.length) {
        qCWarning(lcNotificationExtension) << "Required field 'ciphertext' is missing. Show notification as is.";
        self.contentHandler(self.bestAttemptContent);
        return;
    } else {
        qCDebug(lcNotificationExtension) << "Ciphertext" << ciphertext;
    }

    //
    //  Try to decrypt encrypted message.
    //
    auto decryptResult = CoreMessenger::decryptStandaloneMessage(settings, QString::fromNSString(recipientJid),
        QString::fromNSString(senderJid), QString::fromNSString(ciphertext));

    if (auto result = std::get_if<CoreMessengerStatus>(&decryptResult)) {
        qCWarning(lcNotificationExtension) << "Failed to decrypt message. Show notification as is.";
        self.contentHandler(self.bestAttemptContent);
        return;
    }

    auto message = std::move(*std::get_if<MessageHandler>(&decryptResult));
    self.bestAttemptContent.title = message->senderUsername().toNSString();

    if (auto text = std::get_if<MessageContentText>(&message->content())) {
        self.bestAttemptContent.body = text->text().toNSString();

    } else if (auto file = std::get_if<MessageContentFile>(&message->content())) {
        self.bestAttemptContent.body = [NSString stringWithFormat:@"File: %@", file->fileName().toNSString()];

    } else if (auto picture = std::get_if<MessageContentPicture>(&message->content())) {
        self.bestAttemptContent.body = @"Picture";

    } else if (auto groupInvitation = std::get_if<MessageContentGroupInvitation>(&message->content())) {
        self.bestAttemptContent.body = @"Group Invitation";

    } else {
        qCWarning(lcNotificationExtension) << "Unexpected message content.";
    }

    self.contentHandler(self.bestAttemptContent);
}

- (void)serviceExtensionTimeWillExpire
{
    // Called just before the extension will be terminated by the system.
    // Use this as an opportunity to deliver your "best attempt" at modified content, otherwise the original push
    // payload will be used.
    self.contentHandler(self.bestAttemptContent);
}

@end
