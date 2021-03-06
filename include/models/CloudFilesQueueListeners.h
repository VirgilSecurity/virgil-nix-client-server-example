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

#ifndef VM_CLOUD_FILES_QUEUE_LISTENER_H
#define VM_CLOUD_FILES_QUEUE_LISTENER_H

#include "CloudFileOperationSource.h"
#include "CloudFilesUpdate.h"
#include "OperationQueueListener.h"

#include <QLoggingCategory>
#include <QMutex>

Q_DECLARE_LOGGING_CATEGORY(lcCloudFilesQueueListener);

namespace vm {
class CloudFilesQueueListener : public OperationQueueListener
{
    Q_OBJECT

public:
    using SourceType = CloudFileOperationSource::Type;

    using OperationQueueListener::OperationQueueListener;

    virtual bool preRunCloudFile(CloudFileOperationSource *source);
    virtual void postRunCloudFile(CloudFileOperationSource *source);
    virtual void updateCloudFiles(const CloudFilesUpdate &update);

private:
    bool preRun(OperationSourcePtr source) override;
    void postRun(OperationSourcePtr source) override;
};

using CloudFilesQueueListenerPtr = QPointer<CloudFilesQueueListener>;

//
// Listener that skips non-unique operations and warns about it
//
class UniqueCloudFileFilter : public CloudFilesQueueListener
{
    Q_OBJECT

public:
    using CloudFilesQueueListener::CloudFilesQueueListener;

private:
    bool preRunCloudFile(CloudFileOperationSource *source) override;
    void postRunCloudFile(CloudFileOperationSource *source) override;
    void clear() override;

    QString createUniqueId(CloudFileOperationSource *source) const;
    void notifyNotUnique(CloudFileOperationSource *source);

    QStringList m_ids;
    QMutex m_mutex;
};

//
// Listener that counts list active updates
//
class CloudListUpdatingCounter : public CloudFilesQueueListener
{
    Q_OBJECT

public:
    explicit CloudListUpdatingCounter(QObject *parent);

    bool preRunCloudFile(CloudFileOperationSource *source) override;
    void postRunCloudFile(CloudFileOperationSource *source) override;
    void clear() override;

signals:
    void countChanged(int count);

    void increment(int inc, QPrivateSignal);

private:
    int getIncValue(CloudFileOperationSource *source) const;
    void onIncrement(int inc);

    int m_count = 0;
};

class CloudFilesQueue;

//
// Listener that watch for folder update operations
//
class CloudFolderUpdateWatcher : public CloudFilesQueueListener
{
    Q_OBJECT

public:
    using CloudFilesQueueListener::CloudFilesQueueListener;

    void subscribe(const CloudFileHandler &cloudFolder);

signals:
    void finished(const CloudFileHandler &cloudFolder, bool updated);

private:
    struct Item
    {
        CloudFileHandler folder;
        bool subscribed = false;
        bool updated = false;
    };

    bool preRunCloudFile(CloudFileOperationSource *source) override;
    void postRunCloudFile(CloudFileOperationSource *source) override;
    void updateCloudFiles(const CloudFilesUpdate &update) override;
    void clear() override;

    std::vector<Item> m_items;
    QMutex m_mutex;
};
} // namespace vm

#endif // VM_CLOUD_FILES_QUEUE_LISTENER_H
