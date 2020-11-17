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

#include "operations/DownloadAttachmentOperation.h"

#include "Settings.h"
#include "Utils.h"
#include "operations/DownloadDecryptFileOperation.h"
#include "operations/MessageOperation.h"
#include "operations/MessageOperationFactory.h"

using namespace vm;

DownloadAttachmentOperation::DownloadAttachmentOperation(MessageOperation *parent, const Settings *settings, const Parameter &parameter)
    : LoadAttachmentOperation("DownloadAttachment", parent)
    , m_parent(parent)
    , m_settings(settings)
    , m_parameter(parameter)
{
}

bool DownloadAttachmentOperation::populateChildren()
{
    const auto message = m_parent->message();
    const auto attachment = m_parent->attachment();
    auto factory = m_parent->factory();
    const QString preffix = name() + QChar('/');

    if (m_parameter.type == Parameter::Type::Preload) {
        if (attachment->type == Attachment::Type::Picture) {
            const auto extras = attachment->extras.value<PictureExtras>();
            // Create preview from original file
            if (!Utils::fileExists(extras.previewPath) && Utils::fileExists(attachment->localPath)) {
                const auto filePath = m_settings->makeThumbnailPath(attachment->id, true);
                factory->populateCreateAttachmentPreview(preffix + QString("CreateAttachmentPreview"), m_parent, this, attachment->localPath, filePath);
            }
            // Otherwise download/decrypt thumbnail
            else if (!Utils::fileExists(extras.thumbnailPath)) {
                const auto filePath = m_settings->makeThumbnailPath(attachment->id, false);
                auto op = factory->populateDownloadDecrypt(preffix + QString("DownloadDecryptThumbnail"), this, extras.thumbnailUrl, extras.encryptedThumbnailSize, filePath, message->senderId);
                connect(op, &Operation::started, this, std::bind(&LoadAttachmentOperation::startLoadOperation, this, extras.encryptedThumbnailSize));
                connect(op, &DownloadDecryptFileOperation::progressChanged, this, &LoadAttachmentOperation::setLoadOperationProgress);
                connect(op, &DownloadDecryptFileOperation::decrypted, m_parent, &MessageOperation::setAttachmentThumbnailPath);
            }
        }
    }
    else {
        // Download/decrypt file
        auto downloadPath = m_parameter.filePath;
        if (!Utils::fileExists(downloadPath)) {
            qCDebug(lcOperation) << "Download attachment to:" << downloadPath;
            auto op = factory->populateDownloadDecrypt(preffix + QString("DownloadDecrypt"), this, attachment->url, attachment->encryptedSize, downloadPath, message->senderId);
            connect(op, &Operation::started, this, std::bind(&LoadAttachmentOperation::startLoadOperation, this, attachment->encryptedSize));
            connect(op, &DownloadDecryptFileOperation::progressChanged, this, &LoadAttachmentOperation::setLoadOperationProgress);
            connect(op, &DownloadDecryptFileOperation::decrypted, m_parent, &MessageOperation::setAttachmentLocalPath);
        }
        // Create picture preview
        if (attachment->type == Attachment::Type::Picture) {
            const auto extras = attachment->extras.value<PictureExtras>();
            if (!Utils::fileExists(extras.previewPath)) {
                const auto filePath = m_settings->makeThumbnailPath(attachment->id, true);
                factory->populateCreateAttachmentPreview(preffix + QString("CreateAttachmentPreview"), m_parent, this, downloadPath, filePath);
            }
        }
    }
    return hasChildren();
}