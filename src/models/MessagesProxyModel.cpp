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

#include "MessagesProxyModel.h"

#include "MessagesModel.h"
#include "Messenger.h"

using namespace vm;
using Self = MessagesProxyModel;

Self::MessagesProxyModel(Messenger *messenger, MessagesModel *model)
    : ListProxyModel(model), m_messenger(messenger), m_model(model)
{
    setSortRole(MessagesModel::SortRole);
    sort(0, Qt::DescendingOrder);
}

QModelIndex Self::getNeighbourIndex(int row, int offset) const
{
    const auto rowIndex = mapFromSource(m_model->index(row));
    const auto neighbourIndex = index(rowIndex.row() - offset, 0);
    return mapToSource(neighbourIndex);
}

QModelIndex Self::getFirstIndex() const
{
    return mapToSource(index(m_model->rowCount() - 1, 0));
}

QModelIndex Self::getLastIndex() const
{
    return mapToSource(index(0, 0));
}

bool MessagesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_model->findIncomingInvitationMessage()) {
        return false;
    }
    return ListProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
