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

#ifndef VM_FILECLOUDCONTROLLER_H
#define VM_FILECLOUDCONTROLLER_H

#include <QDir>
#include <QObject>

class Settings;

namespace vm
{
class FileCloudModel;
class Models;

class FileCloudController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString displayPath MEMBER m_displayPath NOTIFY displayPathChanged)

public:
    FileCloudController(const Settings *settings, Models *models, QObject *parent);

    Q_INVOKABLE void openFile(const QVariant &proxyRow);
    Q_INVOKABLE void setDirectory(const QVariant &proxyRow);
    Q_INVOKABLE void cdUp();
    Q_INVOKABLE void addFile(const QVariant &attachmentUrl);
    Q_INVOKABLE void deleteFiles();
    Q_INVOKABLE void createDirectory(const QString &name);

signals:
    void displayPathChanged(const QString &path);

private:
    FileCloudModel *model();
    void setDirectory(const QDir &dir);

    const Settings *m_settings;
    Models *m_models;

    QDir m_rootDir;
    QDir m_currentDir;
    QString m_displayPath;
};
}


#endif // VM_FILECLOUDCONTROLLER_H
