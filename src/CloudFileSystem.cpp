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

#include "CloudFileSystem.h"

#include <QMimeDatabase>

#include "CoreMessenger.h"
#include "FutureWorker.h"
#include "Messenger.h"
#include "Settings.h"
#include "Utils.h"

Q_LOGGING_CATEGORY(lcCloudFileSystem, "cloud-fs")

using namespace vm;

CloudFileSystem::CloudFileSystem(CoreMessenger *coreMessenger, Messenger *messenger)
    : QObject(messenger)
    , m_coreMessenger(coreMessenger)
    , m_messenger(messenger)
{
}

void CloudFileSystem::signIn()
{
    qCDebug(lcCloudFileSystem) << "Sign-in cloud-fs";
    m_coreFs = m_coreMessenger->cloudFs();

    const auto userName = m_messenger->currentUser()->username();
    m_downloadsDir = m_messenger->settings()->cloudFilesDownloadsDir(userName);
    emit downloadsDirChanged(m_downloadsDir);
}

void CloudFileSystem::signOut()
{
    qCDebug(lcCloudFileSystem) << "Sign-out cloud-fs";
    m_coreFs = {};
}

void CloudFileSystem::fetchList(const CloudFileHandler &parentFolder)
{
    const auto parentFolderId = parentFolder->id().coreFolderId();
    FutureWorker::run(m_coreFs->listFolder(parentFolderId), [this, parentFolder](auto result) {
        if (std::holds_alternative<CoreMessengerStatus>(result)) {
            emit fetchListErrorOccured(tr("Cloud folder listing error"));
            return;
        }
        const auto fsFolder = std::get_if<CloudFsFolder>(&result);
        const auto listedFolder = createParentFolderFromInfo(*fsFolder, parentFolder);
        const QDir localDir(listedFolder->localPath());
        // Build list
        ModifiableCloudFiles list;
        for (auto &info : fsFolder->folders) {
            list.push_back(createFolderFromInfo(info, listedFolder->id(), localDir.filePath(info.name)));
        }
        for (auto &info : fsFolder->files) {
            list.push_back(createFileFromInfo(info, listedFolder->id(), localDir.filePath(info.name)));
        }
        emit listFetched(listedFolder, list);
    });
}

void CloudFileSystem::createFile(const QString &filePath, const CloudFileHandler &parentFolder)
{
    const auto parentFolderId = parentFolder->id().coreFolderId();
    const auto isRoot = !parentFolderId.isValid();
    const auto tempDir = m_messenger->settings()->cloudFilesCacheDir();
    const auto encFilePath = tempDir.filePath(QLatin1String("upload-") + Utils::createUuid());
    auto future = isRoot
        ? m_coreFs->createFile(filePath, encFilePath)
        : m_coreFs->createFile(filePath, encFilePath, parentFolderId, parentFolder->publicKey());
    FutureWorker::run(future, [this, filePath, encFilePath, parentFolder](auto result) {
        if (std::holds_alternative<CoreMessengerStatus>(result)) {
            emit createFileErrorOccurred(tr("Cloud file creation error: %1").arg(filePath));
            return;
        }

        const auto fsNewFile = std::get_if<CloudFsNewFile>(&result);
        auto createdFile = createFileFromInfo(fsNewFile->info, parentFolder->id(),
                                              QDir(parentFolder->localPath()).filePath(fsNewFile->info.name));
        emit fileCreated(createdFile, encFilePath, fsNewFile->uploadLink);
    });
}

void CloudFileSystem::createFolder(const QString &name, const CloudFileHandler &parentFolder)
{
    const auto parentFolderId = parentFolder->id().coreFolderId();
    const auto isRoot = !parentFolderId.isValid();
    auto future = isRoot ? m_coreFs->createFolder(name) : m_coreFs->createFolder(name, parentFolderId, parentFolder->publicKey());
    FutureWorker::run(future, [this, parentFolder, name](auto result) {
        if (std::holds_alternative<CoreMessengerStatus>(result)) {
            emit createFolderErrorOccured(tr("Cloud folder creation error: %1").arg(name));
            return;
        }

        const auto fsFolder = std::get_if<CloudFsFolder>(&result);
        auto createdFolder = createFolderFromInfo(fsFolder->info, parentFolder->id(),
                                                  QDir(parentFolder->localPath()).filePath(fsFolder->info.name));
        emit folderCreated(createdFolder);
    });
}

void CloudFileSystem::deleteFiles(const CloudFiles &files)
{
    for (auto &file : files) {
        auto future = file->isFolder() ? m_coreFs->deleteFolder(file->id().coreFolderId()) : m_coreFs->deleteFile(file->id().coreFileId());
        FutureWorker::run(future, [this, file](auto result) {
            if (result == CoreMessengerStatus::Success) {
                emit fileDeleted(file);
            }
            else {
                emit deleteFileErrorOccured(tr("Cloud file deletion error: %1. Code: %2").arg(file->name()).arg(static_cast<int>(result)));
            }
        });
    }
}

QDir CloudFileSystem::downloadsDir() const
{
    return m_downloadsDir;
}

ModifiableCloudFileHandler CloudFileSystem::createParentFolderFromInfo(const CloudFsFolder &fsFolder, const CloudFileHandler &oldFolder) const
{
    auto folder = createFolderFromInfo(fsFolder.info, oldFolder->parentId(), oldFolder->localPath());
    folder->setId(oldFolder->id()); // avoid root with non-empty id
    folder->update(*oldFolder, CloudFileUpdateSource::ListedParent);
    folder->setEncryptedKey(fsFolder.folderEncryptedKey);
    folder->setPublicKey(fsFolder.folderPublicKey);
    return folder;
}

ModifiableCloudFileHandler CloudFileSystem::createFolderFromInfo(const CloudFsFolderInfo &info, const CloudFileId &parentId, const QString &localPath) const
{
    auto folder = std::make_shared<CloudFile>();
    folder->setId(info.id);
    folder->setParentId(parentId);
    folder->setName(info.name);
    folder->setIsFolder(true);
    folder->setCreatedAt(info.createdAt);
    folder->setUpdatedAt(info.updatedAt);
    folder->setUpdatedBy(info.updatedBy);
    folder->setLocalPath(localPath);
    return folder;
}

ModifiableCloudFileHandler CloudFileSystem::createFileFromInfo(const CloudFsFileInfo &info, const CloudFileId &parentId, const QString &localPath) const
{
    auto file = std::make_shared<CloudFile>();
    file->setId(info.id);
    file->setParentId(parentId);
    file->setName(info.name);
    file->setType(info.type);
    file->setSize(info.size);
    file->setCreatedAt(info.createdAt);
    file->setUpdatedAt(info.updatedAt);
    file->setUpdatedBy(info.updatedBy);
    file->setLocalPath(localPath);
    return file;
}
