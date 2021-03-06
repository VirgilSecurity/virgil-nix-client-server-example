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

#include "GroupMembersTable.h"

#include "core/Database.h"
#include "core/DatabaseUtils.h"
#include "Utils.h"

using namespace vm;
using Self = GroupMembersTable;

Self::GroupMembersTable(Database *database) : DatabaseTable(QLatin1String("groupMembers"), database)
{
    connect(this, &Self::add, this, &Self::onAdd);
    connect(this, &Self::fetch, this, &Self::onFetch);
    connect(this, &Self::updateGroup, this, &Self::onUpdateGroup);
    connect(this, &Self::deleteGroupMembers, this, &Self::onDeleteGroupMembers);
}

bool Self::create()
{
    if (DatabaseUtils::readExecQueries(database(), QLatin1String("createGroupMembers"))) {
        qCDebug(lcDatabase) << "Table 'groupMembers' was created.";
        return true;

    } else {
        qCCritical(lcDatabase) << "Failed to create table 'groupMembers'.";
        return false;
    }
}

void Self::onAdd(const GroupMembers &groupMembers)
{
    qCDebug(lcDatabase) << "Start adding group members";

    ScopedConnection connection(*database());

    for (const auto &member : groupMembers) {
        qCDebug(lcDatabase) << "Start adding group member" << member->memberId();

        DatabaseUtils::BindValues bindValues;
        bindValues.push_back({ ":groupId", QString(member->groupId()) });
        bindValues.push_back({ ":memberId", QString(member->memberId()) });
        bindValues.push_back({ ":memberAffiliation", GroupAffiliationToString(member->memberAffiliation()) });

        auto query = DatabaseUtils::readExecQuery(database(), QLatin1String("insertGroupMember"), bindValues);
        if (!query) {
            qCCritical(lcDatabase) << "GroupMembersTable::onAdd error";
        }
    }
}

void Self::onFetch(const GroupId &groupId)
{
    qCDebug(lcDatabase) << "Start fetching group members by for group:" << groupId;

    ScopedConnection connection(*database());

    const DatabaseUtils::BindValues bindValues { { ":groupId", QString(groupId) } };

    auto query = DatabaseUtils::readExecQuery(database(), QLatin1String("selectGroupMembers"), bindValues);
    if (!query) {
        qCCritical(lcDatabase) << "GroupMembersTable::onFetchFailed error";
        emit errorOccurred(tr("Failed to fetch group members"));
    } else {
        GroupMembers groupMembers;
        while (query->next()) {
            if (auto maybyGroupMember = readGroupMember(*query); maybyGroupMember) {
                groupMembers.push_back(std::move(maybyGroupMember));
            }
        }
        emit fetched(groupId, groupMembers);
    }
}

void Self::onUpdateGroup(const GroupUpdate &groupUpdate)
{
    std::list<DatabaseUtils::BindValues> bindValuesCollection;
    QLatin1String queryId;

    if (auto update = std::get_if<GroupMemberAffiliationUpdate>(&groupUpdate)) {
        queryId = QLatin1String("updateGroupMemberAffiliation");

        DatabaseUtils::BindValues bindValues;
        bindValues.push_back({ ":groupId", QString(update->groupId) });
        bindValues.push_back({ ":memberId", QString(update->memberId) });
        bindValues.push_back({ ":memberAffiliation", GroupAffiliationToString(update->memberAffiliation) });

        bindValuesCollection.push_back(std::move(bindValues));
    }

    if (queryId.isEmpty()) {
        return;
    }

    ScopedConnection connection(*database());
    ScopedTransaction transaction(*database());
    for (const auto &bindValues : bindValuesCollection) {

        const auto query = DatabaseUtils::readExecQuery(database(), queryId, bindValues);
        if (query) {
            qCDebug(lcDatabase) << "GroupMembers was updated: " << bindValues.front().second;
        } else {
            transaction.rollback();
            qCCritical(lcDatabase) << "GroupMembersTable::onUpdateGroup error";
            emit errorOccurred(tr("Failed to update group members table"));
        }
    }
}

void Self::onDeleteGroupMembers(const GroupId &groupId)
{
    const DatabaseUtils::BindValues values { { ":id", QString(groupId) } };
    const auto query = DatabaseUtils::readExecQuery(database(), QLatin1String("deleteGroupMembersByGroupId"), values);
    if (query) {
        qCDebug(lcDatabase) << "Group members were removed, group id:" << groupId;
    } else {
        qCCritical(lcDatabase) << "GroupMembersTable::onDeleteGroupMembers deletion error";
        emit errorOccurred(tr("Failed to delete group members"));
    }
}

GroupMemberHandler Self::readGroupMember(const QSqlQuery &query)
{

    auto groupId = query.value("groupId").toString();
    auto memberId = query.value("memberId").toString();
    auto memberAffiliation = query.value("memberAffiliation").toString();

    if (groupId.isEmpty() || memberId.isEmpty() || memberAffiliation.isEmpty()) {

        qCCritical(lcDatabase) << "GroupMembersTable: failed to parse query result";

        return GroupMemberHandler();
    }

    auto contact = std::make_unique<Contact>();
    contact->setUserId(UserId(std::move(memberId)));
    contact->setUsername(query.value("username").toString());
    contact->setEmail(query.value("email").toString());
    contact->setPhone(query.value("phone").toString());

    return std::make_shared<GroupMember>(GroupId(std::move(groupId)), std::move(contact),
                                         GroupAffiliationFromString(memberAffiliation));
}
