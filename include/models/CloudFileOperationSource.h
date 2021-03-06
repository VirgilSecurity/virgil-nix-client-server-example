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

#ifndef VM_CLOUD_FILE_OPERATION_SOURCE_H
#define VM_CLOUD_FILE_OPERATION_SOURCE_H

#include "CloudFile.h"
#include "CloudFileMember.h"
#include "OperationSource.h"

namespace vm {
class CloudFileOperationSource : public OperationSource
{
public:
    enum class Type { ListFolder, CreateFolder, Upload, Download, Delete, ListMembers, SetMembers };

    explicit CloudFileOperationSource(Type type);

    Type type() const;

    CloudFileHandler folder() const;
    void setFolder(const CloudFileHandler &folder);
    CloudFiles files() const;
    CloudFileHandler file() const;
    void setFiles(const CloudFiles &files);
    QString filePath() const;
    void setFilePath(const QString &path);
    QString name() const;
    void setName(const QString &name);
    PostFunction postFunction() const;
    void setPostFunction(const PostFunction &func);
    CloudFileMembers members() const;
    void setMembers(const CloudFileMembers &members);

    bool isValid() const override;
    QString toString() const override;

private:
    Type m_type;
    CloudFileHandler m_folder;
    CloudFiles m_files;
    QString m_filePath;
    QString m_name;
    PostFunction m_postFunction;
    CloudFileMembers m_members;
};
} // namespace vm

#endif // VM_CLOUD_FILE_OPERATION_SOURCE_H
