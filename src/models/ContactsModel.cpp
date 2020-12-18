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

#include "models/ContactsModel.h"

#include <QSortFilterProxyModel>

#include "ContactAvatarLoader.h"

using namespace vm;

ContactsModel::ContactsModel(bool sorted, QObject *parent)
    : ListModel(parent)
    , m_avatarLoader(new ContactAvatarLoader(this))
{
    qRegisterMetaType<ContactsModel *>("ContactsModel*");

    if (sorted) {
        proxy()->setSortRole(NameRole);
        proxy()->sort(0, Qt::AscendingOrder);
    }
    proxy()->setFilterRole(FilterRole);

    connect(this, &ContactsModel::avatarUrlNotFound, this, &ContactsModel::loadAvatarUrl);
    connect(m_avatarLoader, &ContactAvatarLoader::loaded, this, &ContactsModel::setAvatarUrl);
}

void ContactsModel::setContacts(const Contacts &contacts)
{
    beginResetModel();
    m_contacts = contacts;
    endResetModel();
    m_avatarLoader->load(m_contacts, 10);
}

const Contacts &ContactsModel::getContacts() const
{
    return m_contacts;
}

Contact ContactsModel::createContact(const Contact::Id &contactId) const
{
    Contact contact;
    contact.id = contactId;
    contact.name = contactId;
    contact.platformId = contactId;
    return contact;
}

const Contact &ContactsModel::getContact(const int row) const
{
    return m_contacts[row];
}

bool ContactsModel::hasContact(const Contact::Id &contactId) const
{
    return findRowByContactId(contactId) != NullOptional;
}

void ContactsModel::addContact(const Contact &contact)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_contacts.push_back(contact);
    endInsertRows();
    m_avatarLoader->load(m_contacts.back());
}

void ContactsModel::removeContact(const Contact::Id &contactId)
{
    if (auto row = findRowByContactId(contactId)) {
        beginRemoveRows(QModelIndex(), *row, *row);
        m_contacts.erase(m_contacts.begin() + *row);
        endRemoveRows();
    }
}

Optional<int> ContactsModel::findRowByContactId(const Contact::Id &contactId) const
{
    int row = -1;
    for (auto &info : m_contacts) {
        ++row;
        if (info.id == contactId) {
            return row;
        }
    }
    return NullOptional;
}

void ContactsModel::loadAvatarUrl(const Contact::Id &contactId)
{
    if (const auto row = findRowByContactId(contactId)) {
        m_avatarLoader->load(m_contacts[*row]);
    }
}

void ContactsModel::setAvatarUrl(const Contact &contact, const QUrl &url)
{
    if (const auto row = findRowByContactId(contact.id)) {
        m_contacts[*row].avatarUrl = url;
        const auto idx = index(*row);
        emit dataChanged(idx, idx, { AvatarUrlRole });
    }
}

int ContactsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_contacts.size();
}

QVariant ContactsModel::data(const QModelIndex &index, int role) const
{
    const auto &info = m_contacts[index.row()];
    switch (role) {
    case IdRole:
        return info.id;
    case NameRole:
        return info.name;
    case DetailsRole:
    {
        if (info.email.isEmpty()) {
            return info.phoneNumber.isEmpty() ? tr("No phone/email") : info.phoneNumber;
        }
        else {
            return info.phoneNumber.isEmpty() ? info.email : (info.phoneNumber + QLatin1String(" / ") + info.email);
        }
    }
    case AvatarUrlRole:
    {
        const auto &url = info.avatarUrl;
        if (url.isEmpty()) {
            emit avatarUrlNotFound(info.id, QPrivateSignal());
        }
        return url;
    }
    case FilterRole:
        return info.name + QLatin1Char('\n') + info.email + QLatin1Char('\n') + info.phoneNumber;
    default:
        return ListModel::data(index, role);
    }
}

QHash<int, QByteArray> ContactsModel::roleNames() const
{
    return unitedRoleNames({
        { IdRole, "contactId" },
        { NameRole, "name" },
        { DetailsRole, "details" },
        { AvatarUrlRole, "avatarUrl" }
        // Filter role is hidden
    });
}
