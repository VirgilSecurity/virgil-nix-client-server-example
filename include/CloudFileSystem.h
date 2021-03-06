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

#ifndef VM_CLOUD_FILE_SYSTEM_H
#define VM_CLOUD_FILE_SYSTEM_H

#include <QObject>
#include <QLoggingCategory>
#include <QPointer>

#include "CloudFile.h"
#include "CloudFileMember.h"
#include "CloudFileRequestId.h"
#include "CoreMessengerCloudFs.h"
#include "Settings.h"

Q_DECLARE_LOGGING_CATEGORY(lcCloudFileSystem)

namespace vm {
class CoreMessenger;
class Messenger;

class CloudFileSystem : public QObject
{
    Q_OBJECT

public:
    CloudFileSystem(CoreMessenger *coreMessenger, Messenger *messenger);

    void signIn();
    void signOut();

    bool checkPermissions();
    bool createDownloadsDir();

    CloudFileRequestId fetchList(const CloudFileHandler &parentFolder);
    CloudFileRequestId createFile(const QString &filePath, const CloudFileHandler &parentFolder);
    CloudFileRequestId createFolder(const QString &name, const CloudFileHandler &parentFolder,
                                    const CloudFileMembers &members);
    CloudFileRequestId getDownloadInfo(const CloudFileHandler &file);
    bool decryptFile(const QString &sourcePath, const QByteArray &encryptionKey, const CloudFileHandler &file,
                     const CloudFileHandler &parentFolder);
    CloudFileRequestId deleteFiles(const CloudFiles &files);
    CloudFileRequestId setMembers(const CloudFileMembers &members, const CloudFileHandler &file);
    CloudFileRequestId fetchMembers(const CloudFileHandler &file);

signals:
    void downloadsDirChanged(const QDir &downloadsDir);

    void listFetched(CloudFileRequestId requestId, const ModifiableCloudFileHandler &parentFolder,
                     const ModifiableCloudFiles &files);
    void fetchListErrorOccured(CloudFileRequestId requestId, const QString &errorText);

    void fileCreated(CloudFileRequestId requestId, const ModifiableCloudFileHandler &cloudFile,
                     const QString &encryptedFilePath, const QUrl &uploadUrl);
    void createFileErrorOccurred(CloudFileRequestId requestId, const QString &errorText);

    void folderCreated(CloudFileRequestId requestId, const ModifiableCloudFileHandler &folder);
    void createFolderErrorOccured(CloudFileRequestId requestId, const QString &errorText);

    void downloadInfoGot(CloudFileRequestId requestId, const CloudFileHandler &file, const QUrl &url,
                         const QByteArray &encryptionKey);
    void getDownloadInfoErrorOccurred(CloudFileRequestId requestId, const QString &errorText);

    void fileDeleted(CloudFileRequestId requestId, const CloudFileHandler &file);
    void deleteFileErrorOccurred(CloudFileRequestId requestId, const QString &errorText);

    void membersSet(CloudFileRequestId requestId, const CloudFileHandler &file, const CloudFileMembers &members);
    void setMembersErrorOccurred(CloudFileRequestId requestId, const QString &errorText);

    void membersFetched(CloudFileRequestId requestId, const CloudFileHandler &file, CloudFileMembers members);
    void fetchMembersErrorOccurred(CloudFileRequestId requestId, const QString &errorText);

private:
    ModifiableCloudFileHandler createParentFolderFromInfo(const CloudFsFolder &fsFolder,
                                                          const CloudFileHandler &oldFolder) const;
    ModifiableCloudFileHandler createFolderFromInfo(const CloudFsFolderInfo &info, const CloudFileId &parentId,
                                                    const QString &localPath) const;
    ModifiableCloudFileHandler createFileFromInfo(const CloudFsFileInfo &info, const CloudFileId &parentId,
                                                  const QString &localPath) const;
    CloudFsFolder createFsFolder(const CloudFileHandler &folder) const;
    CloudFsFolderInfo createFsFolderInfo(const CloudFileHandler &folder) const;

    QPointer<CoreMessenger> m_coreMessenger;
    std::optional<CoreMessengerCloudFs> m_coreFs;
    std::atomic<CloudFileRequestId> m_requestId = 0;
    QPointer<Messenger> m_messenger;
    QDir m_downloadsDir;
};
} // namespace vm

#endif // VM_CLOUD_FILE_SYSTEM_H
