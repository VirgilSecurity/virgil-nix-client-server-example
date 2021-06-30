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

#include "CoreMessenger.h"

#include "CommKitBridge.h"
#include "CustomerEnv.h"
#include "IncomingMessage.h"
#include "MessageContentJsonUtils.h"
#include "OutgoingMessage.h"
#include "UserImpl.h"
#include "Platform.h"
#include "PlatformNotifications.h"

#include "NetworkAnalyzer.h"
#include "XmppDiscoveryManager.h"
#include "XmppContactManager.h"
#include "XmppLastActivityManager.h"
#include "XmppRoomParticipantsManager.h"
#include "XmppMucSubManager.h"
#include "XmppPushNotifications.h"

#include <virgil/crypto/foundation/vscf_ctr_drbg.h>

#include <virgil/sdk/core/vssc_json_object.h>
#include <virgil/sdk/comm-kit/vssq_messenger.h>
#include <virgil/sdk/comm-kit/vssq_error_message.h>
#include <virgil/sdk/comm-kit/vssq_messenger_file_cipher.h>
#include <virgil/sdk/comm-kit/vssq_messenger_cloud_fs.h>

#include <qxmpp/QXmppMessage.h>
#include <qxmpp/QXmppCarbonManager.h>
#include <qxmpp/QXmppUploadRequestManager.h>
#include <qxmpp/QXmppMucManager.h>
#include <qxmpp/QXmppPubSubItem.h>
#include <qxmpp/QXmppPubSubIq.h>
#include <qxmpp/QXmppMamManager.h>
#include <qxmpp/QXmppUtils.h>
#include <qxmpp/QXmppMamIq.h>

#include <QCryptographicHash>
#include <QMap>
#include <QXmlStreamWriter>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QPointer>

#include <memory>
#include <mutex>
#include <optional>

using namespace vm;
using namespace vm::platform;
using namespace notifications::xmpp;

using Self = vm::CoreMessenger;

Q_LOGGING_CATEGORY(lcCoreMessenger, "core-messenger");
Q_LOGGING_CATEGORY(lcCoreMessengerXMPP, "core-messenger-xmpp");

// --------------------------------------------------------------------------
// XMPP Helpers.
// --------------------------------------------------------------------------
template<typename T>
static QString toXmlString(const T &obj)
{
    QString xmlStr;
    QXmlStreamWriter xmlWriter(&xmlStr);
    xmlWriter.setAutoFormatting(true);
    obj.toXml(&xmlWriter);
    return xmlStr;
}

// --------------------------------------------------------------------------
// C Helpers.
// --------------------------------------------------------------------------
using vscf_ctr_drbg_ptr_t = vsc_unique_ptr<vscf_ctr_drbg_t>;

using vssc_json_object_ptr_t = vsc_unique_ptr<vssc_json_object_t>;
using vssq_messenger_creds_ptr_t = vsc_unique_ptr<const vssq_messenger_creds_t>;
using vssq_messenger_ptr_t = vsc_unique_ptr<vssq_messenger_t>;
using vssq_messenger_file_cipher_ptr_t = vsc_unique_ptr<vssq_messenger_file_cipher_t>;
using vssq_messenger_user_ptr_t = vsc_unique_ptr<const vssq_messenger_user_t>;
using vssq_messenger_user_list_ptr_t = vsc_unique_ptr<vssq_messenger_user_list_t>;
using vssq_messenger_group_ptr_t = vsc_unique_ptr<vssq_messenger_group_t>;
using vssq_messenger_config_ptr_t = vsc_unique_ptr<vssq_messenger_config_t>;

static vscf_ctr_drbg_ptr_t vscf_ctr_drbg_wrap_ptr(vscf_ctr_drbg_t *ptr)
{
    return vscf_ctr_drbg_ptr_t { ptr, vscf_ctr_drbg_delete };
}

static vssc_json_object_ptr_t vssc_json_object_wrap_ptr(vssc_json_object_t *ptr)
{
    return vssc_json_object_ptr_t { ptr, vssc_json_object_delete };
}

static vssq_messenger_creds_ptr_t vssq_messenger_creds_wrap_nullptr()
{
    return vssq_messenger_creds_ptr_t { nullptr, vssq_messenger_creds_delete };
}

static vssq_messenger_creds_ptr_t vssq_messenger_creds_wrap_ptr(vssq_messenger_creds_t *ptr)
{
    return vssq_messenger_creds_ptr_t { ptr, vssq_messenger_creds_delete };
}

static vssq_messenger_creds_ptr_t vssq_messenger_creds_wrap_ptr(const vssq_messenger_creds_t *ptr)
{
    return vssq_messenger_creds_ptr_t { vssq_messenger_creds_shallow_copy_const(ptr), vssq_messenger_creds_delete };
}

static vssq_messenger_ptr_t vssq_messenger_wrap_ptr(vssq_messenger_t *ptr)
{
    return vssq_messenger_ptr_t { ptr, vssq_messenger_delete };
}

static vssq_messenger_file_cipher_ptr_t vssq_messenger_file_cipher_wrap(vssq_messenger_file_cipher_t *ptr)
{
    return vssq_messenger_file_cipher_ptr_t { ptr, vssq_messenger_file_cipher_delete };
}

static vssq_messenger_user_ptr_t vssq_messenger_user_wrap_ptr(vssq_messenger_user_t *ptr)
{
    return vssq_messenger_user_ptr_t { ptr, vssq_messenger_user_delete };
}

static vssq_messenger_user_ptr_t vssq_messenger_user_wrap_ptr(const vssq_messenger_user_t *ptr)
{
    return vssq_messenger_user_ptr_t { vssq_messenger_user_shallow_copy_const(ptr), vssq_messenger_user_delete };
}

static vssq_messenger_user_ptr_t vssq_messenger_user_nullptr()
{
    return vssq_messenger_user_ptr_t { nullptr, vssq_messenger_user_delete };
}

static vssq_messenger_user_list_ptr_t vssq_messenger_user_list_wrap_ptr(vssq_messenger_user_list_t *ptr)
{
    return vssq_messenger_user_list_ptr_t { ptr, vssq_messenger_user_list_delete };
}

static vssq_messenger_group_ptr_t vssq_messenger_group_wrap_ptr(vssq_messenger_group_t *ptr)
{
    return vssq_messenger_group_ptr_t { ptr, vssq_messenger_group_delete };
}

static vssq_messenger_group_ptr_t vssq_messenger_group_null()
{
    return vssq_messenger_group_ptr_t { nullptr, vssq_messenger_group_delete };
}

static vssq_messenger_config_ptr_t vssq_messenger_config_wrap_ptr(vssq_messenger_config_t *ptr)
{
    return vssq_messenger_config_ptr_t { ptr, vssq_messenger_config_delete };
}

static CoreMessengerStatus mapStatus(vssq_status_t status)
{
    switch (status) {
    case vssq_status_SUCCESS:
        return CoreMessengerStatus::Success;

    case vssq_status_MODIFY_GROUP_FAILED_PERMISSION_VIOLATION:
        return CoreMessengerStatus::Error_ModifyGroup_PermissionViolation;

    case vssq_status_ACCESS_GROUP_FAILED_PERMISSION_VIOLATION:
        return CoreMessengerStatus::Error_AccessGroup_PermissionViolation;

    case vssq_status_CREATE_GROUP_FAILED_CRYPTO_FAILED:
        return CoreMessengerStatus::Error_CreateGroup_CryptoFailed;

    case vssq_status_IMPORT_GROUP_EPOCH_FAILED_PARSE_FAILED:
        return CoreMessengerStatus::Error_ImportGroupEpoch_ParseFailed;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_SESSION_ID_DOESNT_MATCH:
        return CoreMessengerStatus::Error_ProcessGroupMessage_SessionIDDoesntMatch;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_EPOCH_NOT_FOUND:
        return CoreMessengerStatus::Error_ProcessGroupMessage_EpochNotFound;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_WRONG_KEY_TYPE:
        return CoreMessengerStatus::Error_ProcessGroupMessage_WrongKeyType;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_INVALID_SIGNATURE:
        return CoreMessengerStatus::Error_ProcessGroupMessage_InvalidSignature;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_ED25519_FAILED:
        return CoreMessengerStatus::Error_ProcessGroupMessage_Ed25519Failed;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_DUPLICATE_EPOCH:
        return CoreMessengerStatus::Error_ProcessGroupMessage_DuplicateEpoch;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_PLAIN_TEXT_TOO_LONG:
        return CoreMessengerStatus::Error_ProcessGroupMessage_PlainTextTooLong;

    case vssq_status_PROCESS_GROUP_MESSAGE_FAILED_CRYPTO_FAILED:
        return CoreMessengerStatus::Error_ProcessGroupMessage_CryptoFailed;

    default:
        return CoreMessengerStatus::Error_UnexpectedCommKitError;
    }
}

class Self::GroupImpl
{
public:
    explicit GroupImpl(GroupHandler aLocalGroup)
        : localGroup(std::move(aLocalGroup)), commKitGroup(vssq_messenger_group_null())
    {
    }

    GroupImpl(GroupHandler aLocalGroup, vssq_messenger_group_ptr_t aCommKitGroup)
        : localGroup(std::move(aLocalGroup)), commKitGroup(std::move(aCommKitGroup))
    {
    }

    GroupHandler localGroup;
    vssq_messenger_group_ptr_t commKitGroup;
};

// --------------------------------------------------------------------------
// Configuration.
// --------------------------------------------------------------------------
class Self::Impl
{
public:
    vscf_impl_ptr_t random = vscf_impl_ptr_t(nullptr, vscf_impl_delete);

    vssq_messenger_ptr_t messenger = vssq_messenger_wrap_ptr(nullptr);
    vssq_messenger_creds_ptr_t creds = vssq_messenger_creds_wrap_nullptr();
    UserHandler currentUser = nullptr;
    std::mutex authMutex;

    std::map<GroupId, GroupImplHandler> groups;
    std::mutex groupsMutex;

    std::map<GroupId, QString> groupRenameRequests;

    using QueryId = QString;
    using QueryParamTo = GroupId;
    std::map<QueryId, QueryParamTo> historySyncQueryParams;

    QPointer<NetworkAnalyzer> networkAnalyzer;
    QPointer<Settings> settings;

    std::unique_ptr<QXmppClient> xmpp;
    std::unique_ptr<XmppDiscoveryManager> discoveryManager;
    std::unique_ptr<XmppContactManager> contactManager;

    QPointer<QXmppCarbonManager> xmppCarbonManager;
    QPointer<QXmppUploadRequestManager> xmppUploadManager;
    QPointer<QXmppMucManager> xmppGroupChatManager;
    QPointer<XmppLastActivityManager> lastActivityManager;
    QPointer<XmppRoomParticipantsManager> xmppRoomParticipantsManager;
    QPointer<XmppMucSubManager> xmppMucSubManager;
    QPointer<QXmppMamManager> xmppMamManager;

    std::map<QString, std::shared_ptr<User>> identityToUser;
    std::map<QString, std::shared_ptr<User>> usernameToUser;
    std::mutex findUserMutex;

    ConnectionState connectionState = ConnectionState::Disconnected;

    QDateTime startDisconnectAt;

    bool suspended = false;
    bool shouldDisconnectWhenSuspended = true;
};

// --------------------------------------------------------------------------
// QT type helpers.
// --------------------------------------------------------------------------
namespace {
struct RegisterMetaTypesOnce
{
    RegisterMetaTypesOnce()
    {
        qRegisterMetaType<QXmppClient::Error>();
        qRegisterMetaType<QXmppHttpUploadSlotIq>();
        qRegisterMetaType<QXmppHttpUploadRequestIq>();
        qRegisterMetaType<QXmppPresence>();
        qRegisterMetaType<QXmppMucItem::Affiliation>();

        qRegisterMetaType<vm::MessageHandler>("MessageHandler");
        qRegisterMetaType<vm::ModifiableMessageHandler>("ModifiableMessageHandler");
        qRegisterMetaType<vm::Messages>("Messages");
        qRegisterMetaType<vm::ModifiableMessages>("ModifiableMessages");
        qRegisterMetaType<vm::UserHandler>("UserHandler");
        qRegisterMetaType<vm::GroupHandler>("GroupHandler");
        qRegisterMetaType<vm::Groups>("Groups");
        qRegisterMetaType<vm::ChatHandler>("ChatHandler");
        qRegisterMetaType<vm::ModifiableChatHandler>("ModifiableChatHandler");
        qRegisterMetaType<vm::ModifiableChats>("ModifiableChats");
        qRegisterMetaType<vm::CloudFileHandler>("CloudFileHandler");
        qRegisterMetaType<vm::ModifiableCloudFileHandler>("ModifiableCloudFileHandler");
        qRegisterMetaType<vm::CloudFiles>("CloudFiles");
        qRegisterMetaType<vm::ModifiableCloudFiles>("ModifiableCloudFiles");
        qRegisterMetaType<vm::CloudFileMemberHandler>("CloudFileMemberHandler");
        qRegisterMetaType<vm::CloudFileMembers>("CloudFileMembers");
        qRegisterMetaType<vm::GroupUpdate>("GroupUpdate");
        qRegisterMetaType<vm::Users>("Users");
        qRegisterMetaType<vm::GroupMember>("GroupMember");
        qRegisterMetaType<vm::UserId>("UserId");
        qRegisterMetaType<vm::GroupId>("GroupId");
        qRegisterMetaType<vm::GroupMembers>("GroupMembers");
        qRegisterMetaType<vm::Contact>("Contact");
        qRegisterMetaType<vm::Contacts>("Contacts");
        qRegisterMetaType<vm::MutableContacts>("MutableContacts");
        qRegisterMetaType<vm::MessageUpdate>("MessageUpdate");
        qRegisterMetaType<vm::ContactUpdate>("ContactUpdate");

        qRegisterMetaType<vm::ChatId>("ChatId");
        qRegisterMetaType<vm::GroupId>("GroupId");
        qRegisterMetaType<vm::MessageId>("MessageId");
        qRegisterMetaType<vm::AttachmentId>("AttachmentId");
        qRegisterMetaType<vm::CloudFileId>("CloudFileId");
    }
};

[[maybe_unused]] const static RegisterMetaTypesOnce g_registerMetaTypesOnce;
} // namespace

// --------------------------------------------------------------------------
// Main implementation.
// --------------------------------------------------------------------------
Self::CoreMessenger(Settings *settings, QObject *parent) : QObject(parent), m_impl(std::make_unique<Self::Impl>())
{

    //
    //  Register self signals-slots
    //
    connect(this, &Self::activate, this, &Self::onActivate);
    connect(this, &Self::deactivate, this, &Self::onDeactivate);
    connect(this, &Self::suspend, this, &Self::onSuspend);
    connect(this, &Self::setShouldDisconnectWhenSuspended, this, &Self::onSetShouldDisconnectWhenSuspended);
    connect(this, &Self::connectionStateChanged, this, &Self::onLogConnectionStateChanged);
    connect(this, &Self::acceptGroupInvitation, this, &Self::onAcceptGroupInvitation);
    connect(this, &Self::rejectGroupInvitation, this, &Self::onRejectGroupInvitation);
    connect(this, &Self::sendMessageStatusDisplayed, this, &Self::onSendMessageStatusDisplayed);
    connect(this, &Self::renameGroupChat, this, &Self::onRenameGroupChat);
    connect(this, &Self::groupChatCreated, this, &Self::onGroupChatCreated);
    connect(this, &Self::groupChatCreateFailed, this, &Self::onGroupChatCreateFailed);

    connect(this, &Self::resetXmppConfiguration, this, &Self::onResetXmppConfiguration);
    connect(this, &Self::reconnectXmppServerIfNeeded, this, &Self::onReconnectXmppServerIfNeeded);
    connect(this, &Self::disconnectXmppServer, this, &Self::onDisconnectXmppServer);
    connect(this, &Self::cleanupXmppMucRooms, this, &Self::onCleanupXmppMucRooms);
    connect(this, &Self::cleanupCommKitMessenger, this, &Self::onCleanupCommKitMessenger);
    connect(this, &Self::registerPushNotifications, this, &Self::onRegisterPushNotifications);
    connect(this, &Self::deregisterPushNotifications, this, &Self::onDeregisterPushNotifications);
    connect(this, &Self::xmppCreateGroupChat, this, &Self::xmppOnCreateGroupChat);
    connect(this, &Self::xmppFetchRoomsFromServer, this, &Self::xmppOnFetchRoomsFromServer);
    connect(this, &Self::xmppJoinRoom, this, &Self::xmppOnJoinRoom);
    connect(this, &Self::xmppMessageDelivered, this, &Self::xmppOnMessageDelivered);

    connect(this, &Self::syncPrivateChatsHistory, this, &Self::onSyncPrivateChatsHistory, Qt::QueuedConnection);
    connect(this, &Self::syncGroupChatHistory, this, &Self::onSyncGroupChatHistory, Qt::QueuedConnection);

    //
    //  Configure Network Analyzer.
    //
    m_impl->networkAnalyzer = new NetworkAnalyzer(nullptr); // will be moved to the thread
    m_impl->settings = settings;
    m_impl->lastActivityManager = new XmppLastActivityManager(settings);

    connect(m_impl->networkAnalyzer, &NetworkAnalyzer::connectedChanged, this, &Self::onProcessNetworkState);

    //
    //  Configure Push Notifications
    //
    connect(&PlatformNotifications::instance(), &PlatformNotifications::pushTokenUpdated, this,
            &Self::onRegisterPushNotifications);
}

Self::~CoreMessenger() noexcept = default;

Self::Result Self::resetCommKitConfiguration()
{

    qCDebug(lcCoreMessenger) << "Reset Comm Kit configuration";

    auto messengerServiceUrl = CustomerEnv::messengerServiceUrl();
    auto xmppServiceUrl = CustomerEnv::xmppServiceUrl();
    auto contactDiscoveryServiceUrl = CustomerEnv::contactDiscoveryServiceUrl();

    qCDebug(lcCoreMessenger) << "Messenger Service URL: " << messengerServiceUrl;
    qCDebug(lcCoreMessenger) << "XMPP Service URL     : " << xmppServiceUrl;
    qCDebug(lcCoreMessenger) << "Contact Discovery URL: " << contactDiscoveryServiceUrl;

    auto messengerServiceUrlStd = messengerServiceUrl.toStdString();
    auto xmppServiceUrlStd = xmppServiceUrl.toStdString();
    auto contactDiscoveryServiceUrlStd = contactDiscoveryServiceUrl.toStdString();

    vssq_messenger_config_t *config = vssq_messenger_config_new_with(vsc_str_from(messengerServiceUrlStd),
                                                                     vsc_str_from(contactDiscoveryServiceUrlStd),
                                                                     vsc_str_from(xmppServiceUrlStd));

    auto caBundlePath = Platform::instance().caBundlePath().toStdString();
    if (!caBundlePath.empty()) {
        vssq_messenger_config_set_ca_bundle(config, vsc_str_from(caBundlePath));
    }

    m_impl->messenger = vssq_messenger_wrap_ptr(vssq_messenger_new_with_config(config));
    vssq_messenger_config_destroy(&config);

    if (!m_impl->random) {
        auto randomImpl = vscf_ctr_drbg_wrap_ptr(vscf_ctr_drbg_new());
        auto randomStatus = vscf_ctr_drbg_setup_defaults(randomImpl.get());
        if (randomStatus != vscf_status_SUCCESS) {
            qCWarning(lcCoreMessenger) << "Got error status: failed to init crypto module - random";
            return Self::Result::Error_CryptoInit;
        }

        m_impl->random = vscf_impl_wrap_ptr(vscf_ctr_drbg_impl(randomImpl.release()));
    }

    vssq_messenger_use_random(m_impl->messenger.get(), m_impl->random.get());
    auto status = vssq_messenger_setup_defaults(m_impl->messenger.get());

    if (status != vssq_status_SUCCESS) {
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_status(status));
        return Self::Result::Error_CryptoInit;
    }

    //
    //  Cleanup caches.
    //
    std::scoped_lock _(m_impl->groupsMutex);
    m_impl->groups.clear();

    return Self::Result::Success;
}

void Self::onResetXmppConfiguration()
{
    if (m_impl->xmpp) {
        //
        //  TODO: Make more reliable approach, see slot onCleanupXmppMucRooms() for details.
        //
        if (nullptr == m_impl->xmppGroupChatManager) {
            m_impl->xmppGroupChatManager = new QXmppMucManager();
            m_impl->xmpp->addExtension(m_impl->xmppGroupChatManager);

            connect(m_impl->xmppGroupChatManager, &QXmppMucManager::invitationReceived, this,
                    &Self::xmppOnMucInvitationReceived);

            connect(m_impl->xmppGroupChatManager, &QXmppMucManager::roomAdded, this, &Self::xmppOnMucRoomAdded);
        }
        return;
    }

    qCDebug(lcCoreMessenger) << "Reset XMPP configuration";

    m_impl->xmpp = std::make_unique<QXmppClient>();

    // Add receipt messages extension
    m_impl->discoveryManager = std::make_unique<XmppDiscoveryManager>(m_impl->xmpp.get(), this);
    m_impl->contactManager = std::make_unique<XmppContactManager>(m_impl->xmpp.get(), this);

    //  Create & connect extensions.
    m_impl->xmppCarbonManager = new QXmppCarbonManager();
    m_impl->xmppUploadManager = new QXmppUploadRequestManager();
    m_impl->xmppGroupChatManager = new QXmppMucManager();
    m_impl->xmppRoomParticipantsManager = new XmppRoomParticipantsManager();
    m_impl->xmppMucSubManager = new XmppMucSubManager();
    m_impl->xmppMamManager = new QXmppMamManager();

    // Parent is implicitly changed to the QXmppClient within addExtension()
    m_impl->xmpp->addExtension(m_impl->xmppCarbonManager);
    m_impl->xmpp->addExtension(m_impl->xmppUploadManager);
    m_impl->xmpp->addExtension(m_impl->xmppGroupChatManager);
    m_impl->xmpp->addExtension(m_impl->lastActivityManager);
    m_impl->xmpp->addExtension(m_impl->xmppRoomParticipantsManager);
    m_impl->xmpp->addExtension(m_impl->xmppMucSubManager);
    m_impl->xmpp->addExtension(m_impl->xmppMamManager);

    // Connect XMPP signals
    connect(m_impl->lastActivityManager, &XmppLastActivityManager::lastActivityTextChanged, this,
            &Self::lastActivityTextChanged);

    connect(m_impl->xmppUploadManager, &QXmppUploadRequestManager::serviceFoundChanged, this,
            &Self::xmppOnUploadServiceFound);

    connect(m_impl->xmppUploadManager, &QXmppUploadRequestManager::slotReceived, this, &Self::xmppOnUploadSlotReceived);

    connect(m_impl->xmppUploadManager, &QXmppUploadRequestManager::requestFailed, this,
            &Self::xmppOnUploadRequestFailed);

    connect(m_impl->xmppGroupChatManager, &QXmppMucManager::invitationReceived, this,
            &Self::xmppOnMucInvitationReceived);

    connect(m_impl->xmppGroupChatManager, &QXmppMucManager::roomAdded, this, &Self::xmppOnMucRoomAdded);

    connect(m_impl->xmppRoomParticipantsManager, &XmppRoomParticipantsManager::participantReceived, this,
            &Self::xmppOnRoomParticipantReceived);

    connect(m_impl->xmppMucSubManager, &XmppMucSubManager::subscribeReceived, this, &Self::xmppOnMucSubscribeReceived);

    connect(m_impl->xmppMucSubManager, &XmppMucSubManager::subscribedRoomsCountReceived, this,
            &Self::xmppOnMucSubscribedRoomsCountReceived);

    connect(m_impl->xmppMucSubManager, &XmppMucSubManager::subscribedRoomReceived, this,
            &Self::xmppOnMucSubscribedRoomReceived);

    connect(m_impl->xmppMucSubManager, &XmppMucSubManager::roomSubscriberReceived, this,
            &Self::xmppOnMucRoomSubscriberReceived);

    connect(m_impl->xmppMucSubManager, &XmppMucSubManager::subscribedRoomsProcessed, this,
            &Self::xmppOnMucSubscribedRoomsProcessed);

    connect(m_impl->xmppMucSubManager, &XmppMucSubManager::roomSubscribersProcessed, this,
            &Self::xmppOnMucRoomSubscribersProcessed);

    connect(m_impl->xmppMucSubManager, &XmppMucSubManager::messageReceived, this, &Self::xmppOnMessageReceived);

    connect(m_impl->xmppMamManager, &QXmppMamManager::archivedMessageReceived, this,
            &Self::xmppOnArchivedMessageReceived);

    connect(m_impl->xmppMamManager, &QXmppMamManager::resultsRecieved, this, &Self::xmppOnArchivedResultsRecieved);

    connect(m_impl->xmpp.get(), &QXmppClient::connected, this, &Self::xmppOnConnected);
    connect(m_impl->xmpp.get(), &QXmppClient::disconnected, this, &Self::xmppOnDisconnected);
    connect(m_impl->xmpp.get(), &QXmppClient::stateChanged, this, &Self::xmppOnStateChanged);
    connect(m_impl->xmpp.get(), &QXmppClient::error, this, &Self::xmppOnError);
    connect(m_impl->xmpp.get(), &QXmppClient::sslErrors, this, &Self::xmppOnSslErrors);

    //
    //
    //
    connect(m_impl->xmpp.get(), &QXmppClient::presenceReceived, this, &Self::xmppOnPresenceReceived);
    connect(m_impl->xmpp.get(), &QXmppClient::iqReceived, this, &Self::xmppOnIqReceived);
    connect(m_impl->xmpp.get(), &QXmppClient::messageReceived, this, &Self::xmppOnMessageReceived);

    //
    //  Handle carbons (message copies).
    //  See, XEP-0280.
    //
    connect(m_impl->xmppCarbonManager, &QXmppCarbonManager::messageReceived, this, &Self::xmppOnMessageReceived);
    connect(m_impl->xmppCarbonManager, &QXmppCarbonManager::messageSent, this, &Self::xmppOnCarbonMessageReceived);

    //
    // Add extra logging
    //
#if ENABLE_XMPP_EXTRA_LOGS
    auto logger = QXmppLogger::getLogger();
    logger->setLoggingType(QXmppLogger::SignalLogging);
    logger->setMessageTypes(QXmppLogger::AnyMessage);

    connect(logger, &QXmppLogger::message, [](QXmppLogger::MessageType, const QString &text) {
        qCDebug(lcCoreMessengerXMPP).noquote() << "" << text;
    });

    m_impl->xmpp->setLogger(logger);
#endif
}

// --------------------------------------------------------------------------
// Statuses.
// --------------------------------------------------------------------------
bool Self::isOnline() const noexcept
{
    return isAuthenticated() && isNetworkOnline() && isXmppConnected();
}

bool Self::isSignedIn() const noexcept
{
    return m_impl->messenger && m_impl->creds;
}

bool CoreMessenger::isAuthenticated() const noexcept
{
    return m_impl->messenger && vssq_messenger_is_authenticated(m_impl->messenger.get());
}

CoreMessenger::ConnectionState Self::connectionState() const
{
    return m_impl->connectionState;
}

bool Self::isNetworkOnline() const noexcept
{
    return m_impl->networkAnalyzer->isConnected();
}

bool Self::isXmppConnected() const noexcept
{
    if (m_impl->xmpp) {
        return m_impl->xmpp->state() == QXmppClient::ConnectedState;
    }

    return false;
}

bool Self::isXmppConnecting() const noexcept
{
    if (m_impl->xmpp) {
        return m_impl->xmpp->state() == QXmppClient::ConnectingState;
    }

    return false;
}

bool Self::isXmppDisconnected() const noexcept
{
    if (m_impl->xmpp) {
        return m_impl->xmpp->state() == QXmppClient::DisconnectedState;
    }

    return true;
}

// --------------------------------------------------------------------------
// State controls.
// --------------------------------------------------------------------------
void Self::onActivate()
{
    m_impl->suspended = false;
    m_impl->shouldDisconnectWhenSuspended = true;

    if (m_impl->xmpp) {
        m_impl->xmpp->setActive(true);
    }

    if (isOnline()) {
        QXmppPresence presenceOnline(QXmppPresence::Available);
        presenceOnline.setAvailableStatusType(QXmppPresence::Online);
        m_impl->xmpp->setClientPresence(presenceOnline);

    } else {
        //
        //  When application was suspended lets give some time, to finish disconnect.
        //
        m_impl->startDisconnectAt = QDateTime::currentDateTime();
        reconnectXmppServerIfNeeded();
    }
}

void Self::onDeactivate()
{
    if (m_impl->xmpp) {
        m_impl->xmpp->setActive(false);
    }

    if (isOnline()) {
        QXmppPresence presenceAway(QXmppPresence::Available);
        presenceAway.setAvailableStatusType(QXmppPresence::Away);
        m_impl->xmpp->setClientPresence(presenceAway);
    }
}

void Self::onSuspend()
{
    if (m_impl->xmpp && m_impl->shouldDisconnectWhenSuspended) {
        //
        //  Setting QXmppPresence::Unavailable also call the disconnectFromServer() function underneath.
        //
        qCDebug(lcCoreMessenger) << "Start disconnect due to suspended";
        QXmppPresence presenceAway(QXmppPresence::Unavailable);
        presenceAway.setAvailableStatusType(QXmppPresence::XA);
        m_impl->xmpp->setClientPresence(presenceAway);
        m_impl->startDisconnectAt = QDateTime::currentDateTime();
    }

    m_impl->suspended = true;
}

void Self::onSetShouldDisconnectWhenSuspended(bool disconnectWhenSuspended)
{
    qCDebug(lcCoreMessenger) << "Should disconnect when suspended:" << disconnectWhenSuspended;
    m_impl->shouldDisconnectWhenSuspended = disconnectWhenSuspended;
}

// --------------------------------------------------------------------------
// Persistent storage helpers.
// --------------------------------------------------------------------------
Self::Result Self::saveCurrentUserInfo()
{
    vssq_error_t error;
    vssq_error_reset(&error);

    const auto user = vssq_messenger_user(m_impl->messenger.get());
    Q_ASSERT(user != nullptr);

    //
    //  Save credentials.
    //
    qCInfo(lcCoreMessenger) << "Save user credentials";
    auto creds = vssq_messenger_creds(m_impl->messenger.get());
    auto credsJson = vssc_json_object_wrap_ptr(vssq_messenger_creds_to_json(creds, &error));

    if (vssq_error_has_error(&error)) {
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return Self::Result::Error_ExportCredentials;
    }

    const auto credentials = vsc_str_to_qstring(vssc_json_object_as_str(credsJson.get()));
    const auto username = vsc_str_to_qstring(vssq_messenger_user_username(user));
    const auto userId = vsc_str_to_qstring(vssq_messenger_user_identity(user));
    m_impl->settings->setUserCredential(username, credentials);

    //
    //  Save info.
    //
    qCInfo(lcCoreMessenger) << "Save user info";
    const auto userJson = vssc_json_object_wrap_ptr(vssq_messenger_user_to_json(user, &error));

    if (vssq_error_has_error(&error)) {
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return Self::Result::Error_ExportUser;
    }

    const auto userInfo = vsc_str_to_qstring(vssc_json_object_as_str(userJson.get()));
    m_impl->settings->setUserInfo(username, userId, userInfo);

    return Self::Result::Success;
}

// --------------------------------------------------------------------------
// User authorization.
// --------------------------------------------------------------------------
QFuture<Self::Result> Self::signIn(const QString &username)
{
    return QtConcurrent::run([this, username = username]() -> Result {
        std::scoped_lock<std::mutex> _(m_impl->authMutex);

        qCInfo(lcCoreMessenger) << "Trying to sign in user";

        auto result = resetCommKitConfiguration();
        if (result != Self::Result::Success) {
            qCCritical(lcCoreMessenger) << "Can not initialize C CommKit module";
            return result;
        }

        qCInfo(lcCoreMessenger) << "Load user credentials";
        auto credentialsString = m_impl->settings->userCredential(username).toStdString();
        if (credentialsString.empty()) {
            qCWarning(lcCoreMessenger) << "User credentials are not found locally";
            return Self::Result::Error_NoCred;
        }

        qCInfo(lcCoreMessenger) << "Parse user credentials";
        vssq_error_t error;
        vssq_error_reset(&error);
        m_impl->creds = vssq_messenger_creds_wrap_ptr(
                vssq_messenger_creds_from_json_str(vsc_str_from(credentialsString), &error));

        if (vssq_error_has_error(&error)) {
            qCWarning(lcCoreMessenger) << "Got error status:"
                                       << vsc_str_to_qstring(vssq_error_message_from_error(&error));
            return Self::Result::Error_ImportCredentials;
        }

        const auto userInfo = m_impl->settings->userInfo(username);
        if (!userInfo.isEmpty()) {
            //
            //  Fast sign-in.
            //
            qCInfo(lcCoreMessenger) << "Trying a fast sign-in";
            const auto userInfoStd = userInfo.toStdString();
            auto userC = vssq_messenger_user_from_json_str(vsc_str_from(userInfoStd), m_impl->random.get(), &error);
            if (!vssq_error_has_error(&error)) {
                qCInfo(lcCoreMessenger) << "Fast sign-in - success";
                m_impl->currentUser = std::make_shared<User>(std::make_unique<UserImpl>(userC));

                resetXmppConfiguration();

                if (isNetworkOnline()) {
                    authenticate();
                }

                return Self::Result::Success;
            } else {
                qCWarning(lcCoreMessenger)
                        << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
            }

            // Go to a full sign-in.
        }

        //
        //  Full sign-in.
        //
        if (!isNetworkOnline()) {
            return Self::Result::Error_Offline;
        }

        qCInfo(lcCoreMessenger) << "Authenticate user during sign-in";
        error.status = vssq_messenger_authenticate(m_impl->messenger.get(), m_impl->creds.get());

        if (vssq_error_has_error(&error)) {
            qCWarning(lcCoreMessenger) << "Got error status:"
                                       << vsc_str_to_qstring(vssq_error_message_from_error(&error));
            return Self::Result::Error_Signin;
        }

        return finishSignIn();
    });
}

QFuture<Self::Result> Self::signUp(const QString &username)
{
    return QtConcurrent::run([this, username = username.toStdString()]() -> Result {
        std::scoped_lock<std::mutex> _(m_impl->authMutex);

        qCInfo(lcCoreMessenger) << "Trying to sign up";

        auto result = resetCommKitConfiguration();
        if (result != Self::Result::Success) {
            qCCritical(lcCoreMessenger) << "Can initialize C CommKit module";
            return result;
        }

        vssq_error_t error;
        vssq_error_reset(&error);
        error.status = vssq_messenger_register(m_impl->messenger.get(), vsc_str_from(username));

        if (vssq_error_has_error(&error)) {
            qCWarning(lcCoreMessenger) << "Got error status:"
                                       << vsc_str_to_qstring(vssq_error_message_from_error(&error));
            return Self::Result::Error_Signup;
        }

        m_impl->creds = vssq_messenger_creds_wrap_ptr(vssq_messenger_creds(m_impl->messenger.get()));

        qCInfo(lcCoreMessenger) << "User has been successfully signed up";

        return finishSignIn();
    });
}

QFuture<Self::Result> Self::backupKey(const QString &password)
{
    return QtConcurrent::run([this, password = password.toStdString()]() -> Result {
        qCInfo(lcCoreMessenger) << "Upload current user key to the cloud";

        const vssq_status_t status = vssq_messenger_backup_creds(m_impl->messenger.get(), vsc_str_from(password));
        if (vssq_status_SUCCESS != status) {
            qCWarning(lcCoreMessenger) << "Got error status:"
                                       << vsc_str_to_qstring(vssq_error_message_from_status(status));
            return Self::Result::Error_MakeKeyBackup;
        }

        return Self::Result::Success;
    });
}

QFuture<Self::Result> Self::signInWithBackupKey(const QString &username, const QString &password)
{
    return QtConcurrent::run([this, username = username.toStdString(), password = password.toStdString()]() -> Result {
        std::scoped_lock<std::mutex> _(m_impl->authMutex);

        qCInfo(lcCoreMessenger) << "Load user key from the cloud";

        auto result = resetCommKitConfiguration();
        if (result != Self::Result::Success) {
            qCCritical(lcCoreMessenger) << "Can initialize C CommKit module";
            return result;
        }

        const vssq_status_t status = vssq_messenger_authenticate_with_backup_creds(
                m_impl->messenger.get(), vsc_str_from(username), vsc_str_from(password));

        if (vssq_status_SUCCESS != status) {
            qCWarning(lcCoreMessenger) << "Got error status:"
                                       << vsc_str_to_qstring(vssq_error_message_from_status(status));
            return Self::Result::Error_RestoreKeyBackup;
        }

        m_impl->creds = vssq_messenger_creds_wrap_ptr(vssq_messenger_creds(m_impl->messenger.get()));

        qCInfo(lcCoreMessenger) << "User has been successfully signed in with a backup key";

        return finishSignIn();
    });
}

QFuture<Self::Result> Self::signOut()
{
    return QtConcurrent::run([this]() -> Result {
        qCInfo(lcCoreMessenger) << "Signing out";

        emit deregisterPushNotifications();
        emit disconnectXmppServer();
        emit cleanupXmppMucRooms();
        emit cleanupCommKitMessenger();

        return Self::Result::Success;
    });
}

QString Self::currentUserJid() const
{
    auto user = vssq_messenger_user(m_impl->messenger.get());
    vsc_str_t userIdentity = vssq_messenger_user_identity(user);
    return vsc_str_to_qstring(userIdentity) + "@" + CustomerEnv::xmppServiceDomain() + "/"
            + m_impl->settings->deviceId();
}

UserId Self::userIdFromJid(const QString &jid)
{
    return UserId(jid.split("@").first());
}

QString Self::userIdToJid(const UserId &userId) const
{
    if (userId == currentUser()->id()) {
        return userId + "@" + CustomerEnv::xmppServiceDomain() + "/" + m_impl->settings->deviceId();
    } else {
        return userId + "@" + CustomerEnv::xmppServiceDomain();
    }
}

QString Self::groupChatsDomain()
{
    return "conference." + CustomerEnv::xmppServiceDomain();
}

QString Self::groupIdToJid(const GroupId &groupId)
{
    return groupId + "@" + groupChatsDomain();
}

GroupId Self::groupIdFromJid(const QString &jid)
{
    return GroupId(jid.split("@").first());
}

UserId Self::groupUserIdFromJid(const QString &jid)
{
    return UserId(jid.split("/").last());
}

void Self::onRegisterPushNotifications()
{
    if (!PlatformNotifications::instance().isPushAvailable()) {
        return;
    }

    if (!isOnline()) {
        qCWarning(lcCoreMessenger) << "Can not subscribe for push notifications, no connection. Will try it later.";
        return;
    }

    qCInfo(lcCoreMessenger) << "Register for push notifications on XMPP server.";

    auto xmppPush = XmppPushNotifications::instance().buildEnableIq();

    xmppPush.setNode(currentUserJid());

    qCDebug(lcCoreMessengerXMPP).noquote() << "Subscribe XMPP request:" << toXmlString(xmppPush);

    const bool sentStatus = m_impl->xmpp->sendPacket(xmppPush);

    qCInfo(lcCoreMessenger) << "Register for push notifications on XMPP server status:" << sentStatus;
}

void Self::onDeregisterPushNotifications()
{
    if (!PlatformNotifications::instance().isPushAvailable()) {
        return;
    }

    if (!isOnline()) {
        qCWarning(lcCoreMessenger) << "Can not unsubscribe from the push notifications, no connection.";
    }

    auto xmppPush = XmppPushNotifications::instance().buildDisableIq();

    xmppPush.setNode(currentUserJid());

    qCDebug(lcCoreMessengerXMPP).noquote() << "Unsubscribe XMPP request:" << toXmlString(xmppPush);

    const bool sentStatus = m_impl->xmpp->sendPacket(xmppPush);

    qCDebug(lcCoreMessenger) << "Unsubscribe from push notifications status:" << (sentStatus ? "success" : "failed");
}

bool Self::subscribeToUser(const User &user)
{

    auto userJid = userIdToJid(user.id());

    return m_impl->contactManager->addContact(userJid, user.id(), QString());
}

QUrl Self::getCrashReportEndpointUrl() const
{
    auto urlStr = CustomerEnv::messengerServiceUrl() + "/send-logs";

    return QUrl(urlStr);
}

QString Self::getAuthHeaderVaue() const
{

    qCInfo(lcCoreMessenger) << "Generate auth header for the Messenger Backend Service";

    vssq_error_t error;
    vssq_error_reset(&error);

    const vssq_messenger_auth_t *auth = vssq_messenger_auth(m_impl->messenger.get());
    vssc_http_header_t *auth_header = vssq_messenger_auth_generate_messenger_auth_header(auth, &error);

    if (vssq_error_has_error(&error)) {
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return QString();
    }

    auto authHeaderValue = vsc_str_to_qstring(vssc_http_header_value(auth_header));

    vssc_http_header_destroy(&auth_header);

    qCDebug(lcCoreMessenger) << "Auth header for the Messenger Backend Service:" << authHeaderValue;

    return authHeaderValue;
}

void Self::changeConnectionState(ConnectionState state)
{
    if (m_impl->connectionState != state) {
        m_impl->connectionState = state;
        emit connectionStateChanged(state);
    }
}

CoreMessenger::Result Self::finishSignIn()
{
    auto userC = vssq_messenger_user(m_impl->messenger.get());
    m_impl->currentUser = std::make_shared<User>(std::make_unique<UserImpl>(userC));

    const auto saveResult = saveCurrentUserInfo();

    if (saveResult == Self::Result::Success) {
        emit reconnectXmppServerIfNeeded();
    }

    return saveResult;
}

void Self::authenticate()
{
    Q_ASSERT(isSignedIn());

    QtConcurrent::run([this] {
        std::scoped_lock<std::mutex> _(m_impl->authMutex);
        if (isAuthenticated()) {
            qCWarning(lcCoreMessenger) << "Try to authenticate when already authenticated";
            return;
        }

        vssq_error_t error;
        vssq_error_reset(&error);

        if (!m_impl->messenger) {
            qCWarning(lcCoreMessenger) << "Try to authenticate when signed out";
            return;
        }

        qCInfo(lcCoreMessenger) << "Authenticate user";
        error.status = vssq_messenger_authenticate(m_impl->messenger.get(), m_impl->creds.get());

        if (vssq_error_has_error(&error)) {
            qCWarning(lcCoreMessenger) << "Got error status:"
                                       << vsc_str_to_qstring(vssq_error_message_from_error(&error));
            return;
        }

        const auto result = finishSignIn();

        if (result == Self::Result::Success) {
            emit reconnectXmppServerIfNeeded();
        }
    });
}

void Self::connectXmppServer()
{
    //
    //  Prevent XMPP start connection, when disconnect was not completed.
    //
    if (m_impl->startDisconnectAt.isValid()) {
        const auto distance = m_impl->startDisconnectAt.msecsTo(QDateTime::currentDateTime());
        if (distance < 2000) {
            QTimer::singleShot(2000 - distance, this, &Self::reconnectXmppServerIfNeeded);
            qCDebug(lcCoreMessenger) << "Prevent XMPP start connection, when disconnect was not completed.";
            qCDebug(lcCoreMessenger) << "Emit reconnect in" << 2000 - distance << "ms from: connectXmppServer()";
            return;
        }
    }

    vssq_error_t error;
    vssq_error_reset(&error);

    qCDebug(lcCoreMessenger) << "Obtain XMPP credentials...";

    const vssq_messenger_auth_t *auth = vssq_messenger_auth(m_impl->messenger.get());
    const vssq_ejabberd_jwt_t *jwt = vssq_messenger_auth_ejabberd_jwt(auth, &error);

    if (vssq_error_has_error(&error)) {
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        changeConnectionState(Self::ConnectionState::Disconnected);
        return;
    }

    const auto jwtExpiresAt = vssq_ejabberd_jwt_expires_at(jwt);
    qCDebug(lcCoreMessenger) << "XMPP credentials token will expire at: "
                             << QDateTime::fromTime_t(jwtExpiresAt).toLocalTime();

    QString xmppPass = vsc_str_to_qstring(vssq_ejabberd_jwt_as_string(jwt));

    qCDebug(lcCoreMessenger) << "Connect user with JID:" << currentUserJid();

    QXmppConfiguration config {};
    config.setJid(currentUserJid());
    config.setHost(CustomerEnv::xmppServiceUrl());
    config.setPassword(xmppPass);
    config.setAutoReconnectionEnabled(false);
    config.setAutoAcceptSubscriptions(true);

    resetXmppConfiguration();

    qCDebug(lcCoreMessenger) << "Connecting to XMPP server...";
    m_impl->xmpp->connectToServer(config);
    m_impl->startDisconnectAt = QDateTime();
}

void Self::onReconnectXmppServerIfNeeded()
{
    if (!isSignedIn() || !isNetworkOnline() || m_impl->suspended) {
        return;
    }

    if (!isAuthenticated()) {
        authenticate();

    } else if (isXmppDisconnected()) {
        connectXmppServer();
    }
}

void Self::onDisconnectXmppServer()
{
    if (m_impl->xmpp != nullptr && (m_impl->xmpp->state() != QXmppClient::DisconnectedState)) {
        qCDebug(lcCoreMessenger) << "Start XMPP disconnect...";
        m_impl->xmpp->disconnectFromServer();
        m_impl->startDisconnectAt = QDateTime::currentDateTime();
    }
}

void Self::onCleanupXmppMucRooms()
{
    if (!m_impl->xmppGroupChatManager) {
        return;
    }

    for (auto xmppRoom : m_impl->xmppGroupChatManager->rooms()) {
        xmppRoom->leave("Signed out");
        delete xmppRoom;
    }

    //
    //  Old rooms manager should be completely replaced, because after sign-out and then sign-in
    //  the old rooms are iterated and contains dangling pointers, that leads to a crash.
    //  TODO: Make more reliable approach.
    //
    m_impl->xmpp->removeExtension(m_impl->xmppGroupChatManager);
    m_impl->xmppGroupChatManager = nullptr;
}

void Self::onCleanupCommKitMessenger()
{
    std::scoped_lock<std::mutex> _(m_impl->authMutex);
    m_impl->messenger = nullptr;
    m_impl->creds = nullptr;
    m_impl->currentUser = nullptr;
}

// --------------------------------------------------------------------------
// Find users.
// --------------------------------------------------------------------------
std::shared_ptr<User> Self::findUserByUsername(const QString &username) const
{
    std::scoped_lock _(m_impl->findUserMutex);

    qCDebug(lcCoreMessenger) << "Trying to find user with username:" << username;

    //
    //  Check cache first.
    //
    auto userIt = m_impl->usernameToUser.find(username);
    if (userIt != m_impl->usernameToUser.end()) {
        const auto publicKeyId =
                vsc_data_to_qbytearray(vssq_messenger_user_public_key_id(userIt->second->impl()->user.get()));
        qCDebug(lcCoreMessenger) << "User found in the cache with public key id:" << publicKeyId.toHex();
        emit userWasFound(userIt->second);
        return userIt->second;
    }

    //
    //  Search on-line.
    //
    if (!isOnline()) {
        qCWarning(lcCoreMessenger) << "Attempt to find user when offline.";
        return nullptr;
    }

    vssq_error_t error;
    vssq_error_reset(&error);

    auto usernameStdStr = username.toStdString();

    auto user = vssq_messenger_find_user_with_username(m_impl->messenger.get(), vsc_str_from(usernameStdStr), &error);

    if (vssq_error_has_error(&error)) {
        qCDebug(lcCoreMessenger) << "User not found";
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return nullptr;
    }

    const auto publicKeyId = vsc_data_to_qbytearray(vssq_messenger_user_public_key_id(user));
    qCDebug(lcCoreMessenger) << "User found in the cloud with public key id:" << publicKeyId.toHex();

    //
    //  Cache and return.
    //
    auto commKitUserImpl = std::make_unique<UserImpl>(user);
    auto commKitUser = std::make_shared<User>(std::move(commKitUserImpl));

    m_impl->usernameToUser[username] = commKitUser;
    m_impl->identityToUser[commKitUser->id()] = commKitUser;

    emit userWasFound(commKitUser);
    emit updateContact(UsernameContactUpdate { commKitUser->id(), commKitUser->username() });

    return commKitUser;
}

std::shared_ptr<User> Self::findUserById(const UserId &userId) const
{
    std::scoped_lock _(m_impl->findUserMutex);

    qCDebug(lcCoreMessenger) << "Trying to find user with id:" << userId;

    //
    //  Check cache first.
    //
    auto userIt = m_impl->identityToUser.find(userId);
    if (userIt != m_impl->identityToUser.end()) {
        const auto publicKeyId =
                vsc_data_to_qbytearray(vssq_messenger_user_public_key_id(userIt->second->impl()->user.get()));
        qCDebug(lcCoreMessenger) << "User found in the cache with public key id:" << publicKeyId.toHex();
        emit userWasFound(userIt->second);
        return userIt->second;
    }

    //
    //  Search on-line.
    //
    if (!isOnline()) {
        return nullptr;
    }

    vssq_error_t error;
    vssq_error_reset(&error);

    auto userIdStdStr = QString(userId).toStdString();

    auto user = vssq_messenger_find_user_with_identity(m_impl->messenger.get(), vsc_str_from(userIdStdStr), &error);

    if (vssq_error_has_error(&error)) {
        qCDebug(lcCoreMessenger) << "User not found";
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return nullptr;
    }

    const auto publicKeyId = vsc_data_to_qbytearray(vssq_messenger_user_public_key_id(user));
    qCDebug(lcCoreMessenger) << "User found in the cloud with public key id:" << publicKeyId.toHex();

    //
    //  Cache and return.
    //
    auto commKitUserImpl = std::make_unique<UserImpl>(user);
    auto commKitUser = std::make_shared<User>(std::move(commKitUserImpl));

    m_impl->identityToUser[userId] = commKitUser;

    emit userWasFound(commKitUser);

    return commKitUser;
}

std::shared_ptr<User> Self::currentUser() const
{
    return m_impl->currentUser;
}

// --------------------------------------------------------------------------
//  Message handling.
// --------------------------------------------------------------------------
QFuture<Self::Result> Self::sendMessage(MessageHandler message)
{

    return QtConcurrent::run([this, message = std::move(message)]() -> Result {
        qCInfo(lcCoreMessenger) << "Trying to send message with id:" << QString(message->id());

        if (!isOnline()) {
            qCInfo(lcCoreMessenger) << "Trying to send message when offline";
            return Self::Result::Error_Offline;
        }

        switch (message->chatType()) {
        case ChatType::Personal:
            return sendPersonalMessage(message);

        case ChatType::Group:
            return sendGroupMessage(message);

        default:
            throw std::logic_error("Invalid chat type");
        }
    });
}

std::variant<CoreMessengerStatus, QByteArray> Self::encryptPersonalMessage(const UserId &recipientId,
                                                                           const QByteArray &messageData)
{
    //
    //  Find recipient.
    //
    auto recipient = findUserById(recipientId);
    if (!recipient) {
        //
        //  Got network troubles to find recipient, so cache message and try later.
        //
        qCWarning(lcCoreMessenger) << "Can not send message - recipient is not found";
        return Self::Result::Error_UserNotFound;
    }

    //
    //  Encrypt message.
    //
    auto ciphertextDataMinLen = vssq_messenger_encrypted_message_len(m_impl->messenger.get(), messageData.size(),
                                                                     recipient->impl()->user.get());

    qCDebug(lcCoreMessenger) << "Message Len   :" << messageData.size();
    qCDebug(lcCoreMessenger) << "ciphertext Len:" << ciphertextDataMinLen;

    auto [ciphertextData, ciphertext] = makeMappedBuffer(ciphertextDataMinLen);

    const vssq_status_t encryptionStatus = vssq_messenger_encrypt_data(
            m_impl->messenger.get(), vsc_data_from(messageData), recipient->impl()->user.get(), ciphertext.get());

    if (encryptionStatus != vssq_status_SUCCESS) {
        qCWarning(lcCoreMessenger) << "Can not encrypt ciphertext:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(encryptionStatus));
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    adjustMappedBuffer(ciphertext, ciphertextData);

    return ciphertextData;
}

std::variant<CoreMessengerStatus, QByteArray> Self::decryptPersonalMessage(const UserId &senderId,
                                                                           const QByteArray &messageCiphertext)
{
    //
    //  Find sender.
    //
    auto sender = findUserById(senderId);
    if (!sender) {
        //
        //  Got network troubles to find sender, so cache message and try later.
        //
        qCWarning(lcCoreMessenger) << "Can not decrypt ciphertext - sender is not found";
        return Self::Result::Success;
    }

    //
    //  Decrypt message.
    //
    auto plaintextDataMinLen = vssq_messenger_decrypted_message_len(m_impl->messenger.get(), messageCiphertext.size());

    auto [messageData, plaintext] = makeMappedBuffer(plaintextDataMinLen);

    const vssq_status_t decryptionStatus = vssq_messenger_decrypt_data(
            m_impl->messenger.get(), vsc_data_from(messageCiphertext), sender->impl()->user.get(), plaintext.get());

    if (decryptionStatus != vssq_status_SUCCESS) {
        qCWarning(lcCoreMessenger) << "Can not decrypt ciphertext:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(decryptionStatus));
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    adjustMappedBuffer(plaintext, messageData);

    qCDebug(lcCoreMessenger) << "Received XMPP message was decrypted";

    return messageData;
}

Self::Result Self::sendPersonalMessage(const MessageHandler &message)
{
    //
    //  Encrypt message.
    //
    auto messageData = packMessage(message);
    auto ciphertextResult = encryptPersonalMessage(message->recipientId(), messageData);
    if (auto status = std::get_if<Self::Result>(&ciphertextResult)) {
        return *status;
    }

    auto ciphertextData = std::move(*std::get_if<QByteArray>(&ciphertextResult));

    //
    //  Pack JSON body.
    //
    auto messageBody = packXmppMessageBody(ciphertextData, PushType::Alert);

    //
    //  Send message.
    //
    qCDebug(lcCoreMessenger) << "Will send XMPP message with body:" << messageBody;

    auto senderJid = userIdToJid(message->senderId());
    auto recipientJid = userIdToJid(UserId(message->chatId()));

    qCDebug(lcCoreMessenger) << "Will send XMPP message from JID:" << senderJid;
    qCDebug(lcCoreMessenger) << "Will send XMPP message to JID:" << recipientJid;

    QXmppMessage xmppMessage(senderJid, recipientJid, messageBody);
    xmppMessage.setId(message->id());
    xmppMessage.setStamp(message->createdAt());
    xmppMessage.setType(QXmppMessage::Type::Chat);
    xmppMessage.setMarkable(true);

    //
    //  Send.
    //
    bool isSent = m_impl->xmpp->sendPacket(xmppMessage);
    if (isSent) {
        qCDebug(lcCoreMessenger) << "XMPP message was sent:" << message->id();
        return Self::Result::Success;
    } else {
        qCWarning(lcCoreMessenger) << "Can not send message - XMPP send failed:" << message->id();
        return Self::Result::Error_SendMessageFailed;
    }
}

Self::Result Self::sendGroupMessage(const MessageHandler &message)
{

    qCDebug(lcCoreMessenger) << "Will send group message:" << message->id() << ", from user:" << message->senderId()
                             << ", to group:" << message->groupChatInfo()->groupId();
    ;

    //
    //  Encrypt message for a group.
    //
    auto groupId = message->groupChatInfo()->groupId();

    auto messageData = packMessage(message);

    auto encryptedMessageDataResult = encryptGroupMessage(groupId, messageData);

    if (auto status = std::get_if<Self::Result>(&encryptedMessageDataResult)) {
        qCDebug(lcCoreMessenger) << "Failed to send message with id:" << message->id();
        return *status;
    }

    auto encryptedMessageData = std::move(*std::get_if<QByteArray>(&encryptedMessageDataResult));

    //
    //  Pack JSON body.
    //
    auto messageBody = packXmppMessageBody(encryptedMessageData, PushType::Alert);

    qCDebug(lcCoreMessenger) << "Will send XMPP message with body:" << messageBody;

    auto senderJid = currentUserJid();
    auto groupJid = groupIdToJid(groupId);

    qCDebug(lcCoreMessenger) << "Will send XMPP message from jid:" << senderJid;

    QXmppMessage xmppMessage(senderJid, groupJid, messageBody);
    xmppMessage.setId(message->id());
    xmppMessage.setStamp(message->createdAt());
    xmppMessage.setType(QXmppMessage::Type::GroupChat);
    xmppMessage.setMarkable(true);
    xmppMessage.addHint(QXmppMessage::Store);

    //
    //  Send.
    //
    bool isSent = m_impl->xmpp->sendPacket(xmppMessage);
    if (isSent) {
        qCDebug(lcCoreMessenger) << "XMPP message was sent";
        return Self::Result::Success;

    } else {
        qCWarning(lcCoreMessenger) << "Can not send message - XMPP send failed";
        return Self::Result::Error_SendMessageFailed;
    }
}

std::variant<Self::Result, MessageHandler> Self::decryptStandaloneMessage(const Settings &settings,
                                                                          const QString &recipientJid,
                                                                          const QString &senderJid,
                                                                          const QString &ciphertext)
{
    //
    //  Find recipient's credentials.
    //
    auto recipientId = userIdFromJid(recipientJid);
    auto recipientUsername = settings.usernameForId(recipientId);
    if (recipientUsername.isEmpty()) {
        qCWarning(lcCoreMessenger) << "User name was not found locally by it's identifier";
        return Self::Result::Error_NoCred;
    }

    auto recipientCredentialsString = settings.userCredential(recipientUsername).toStdString();
    if (recipientCredentialsString.empty()) {
        qCWarning(lcCoreMessenger) << "User credentials are not found locally";
        return Self::Result::Error_NoCred;
    }

    //
    //  Parse recipient's credentials.
    //
    qCInfo(lcCoreMessenger) << "Parse user credentials";
    vssq_error_t error;
    vssq_error_reset(&error);
    auto recipientCredentials = vssq_messenger_creds_wrap_ptr(
            vssq_messenger_creds_from_json_str(vsc_str_from(recipientCredentialsString), &error));

    if (vssq_error_has_error(&error)) {
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return Self::Result::Error_ImportCredentials;
    }

    //
    //  Create CommKit messenger.
    //
    auto messengerServiceUrlStd = CustomerEnv::messengerServiceUrl().toStdString();
    auto xmppServiceUrlStd = CustomerEnv::xmppServiceUrl().toStdString();
    auto contactDiscoveryServiceUrlStd = CustomerEnv::contactDiscoveryServiceUrl().toStdString();

    const auto config = vssq_messenger_config_wrap_ptr(vssq_messenger_config_new_with(
            vsc_str_from(messengerServiceUrlStd), vsc_str_from(contactDiscoveryServiceUrlStd),
            vsc_str_from(xmppServiceUrlStd)));

    auto caBundlePath = Platform::instance().caBundlePath().toStdString();
    if (!caBundlePath.empty()) {
        vssq_messenger_config_set_ca_bundle(config.get(), vsc_str_from(caBundlePath));
    }

    auto messenger = vssq_messenger_wrap_ptr(vssq_messenger_new_with_config(config.get()));

    const auto initStatus = vssq_messenger_setup_defaults(messenger.get());
    if (initStatus != vssq_status_SUCCESS) {
        qCWarning(lcCoreMessenger) << "Got error status:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(initStatus));
        return Self::Result::Error_CryptoInit;
    }

    //
    //  Authenticate the recipient.
    //
    qCInfo(lcCoreMessenger) << "Authenticate user";
    const auto authStatus = vssq_messenger_authenticate(messenger.get(), recipientCredentials.get());

    if (authStatus != vssq_status_SUCCESS) {
        qCWarning(lcCoreMessenger) << "Got error status:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(authStatus));
        return Self::Result::Error_Signin;
    }

    //
    //  Find sender.
    //
    auto senderIdStdStr = QString(userIdFromJid(senderJid)).toStdString();
    if (senderIdStdStr.empty()) {
        qCWarning(lcCoreMessenger) << "Given sender is not defined";
        return Self::Result::Error_UserNotFound;
    }

    auto sender = vssq_messenger_user_wrap_ptr(
            vssq_messenger_find_user_with_identity(messenger.get(), vsc_str_from(senderIdStdStr), &error));
    if (vssq_error_has_error(&error)) {
        qCDebug(lcCoreMessenger) << "User not found";
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return nullptr;
    }

    //
    //  Decrypt message.
    //
    if (ciphertext.isEmpty()) {
        qCWarning(lcCoreMessenger) << "Given ciphertext is empty";
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    auto ciphertextDecoded = QByteArray::fromBase64Encoding(ciphertext.toLatin1());
    if (!ciphertextDecoded) {
        qCWarning(lcCoreMessenger) << "Given ciphertext is not base64 encoded";
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    auto ciphertextData = std::move(*ciphertextDecoded);

    auto plaintextDataMinLen = vssq_messenger_decrypted_message_len(messenger.get(), ciphertextData.size());

    auto [messageData, plaintext] = makeMappedBuffer(plaintextDataMinLen);

    const vssq_status_t decryptionStatus =
            vssq_messenger_decrypt_data(messenger.get(), vsc_data_from(ciphertextData), sender.get(), plaintext.get());

    if (decryptionStatus != vssq_status_SUCCESS) {
        qCWarning(lcCoreMessenger) << "Can not decrypt ciphertext:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(decryptionStatus));
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    adjustMappedBuffer(plaintext, messageData);

    //
    //  Unpack message.
    //
    auto message = std::make_unique<IncomingMessage>();

    if (auto result = unpackMessage(messageData, *message); result != Self::Result::Success) {
        return result;
    }

    return message;
}

QFuture<Self::Result> Self::processReceivedXmppMessage(const QXmppMessage &xmppMessage)
{
    return QtConcurrent::run([this, xmppMessage]() -> Result {
        qCInfo(lcCoreMessenger) << "Received XMPP message";
        qCDebug(lcCoreMessenger) << "Received XMPP message with id:" << xmppMessage.id()
                                 << "from:" << xmppMessage.from();

        //
        //  Handle receipts (may come from archived messages).
        //
        switch (xmppMessage.marker()) {
        case QXmppMessage::Marker::Received: {
            emit xmppMessageDelivered(xmppMessage.from(), xmppMessage.markedId());
            return Self::Result::Success;
        }
        case QXmppMessage::Marker::Acknowledged:
        case QXmppMessage::Marker::Displayed: {
            emit updateMessage(
                    OutgoingMessageStageUpdate { MessageId(xmppMessage.markedId()), OutgoingMessageStage::Read });
            return Self::Result::Success;
        }
        default:
            break;
        }

        switch (xmppMessage.type()) {
        case QXmppMessage::Type::Normal:
            return Self::Result::Success;

        case QXmppMessage::Type::Chat:
            return processChatReceivedXmppMessage(xmppMessage);

        case QXmppMessage::Type::GroupChat:
            return processGroupChatReceivedXmppMessage(xmppMessage);

        case QXmppMessage::Type::Error:
            return processErrorXmppMessage(xmppMessage);

        default:
            break;
        }

        qCWarning(lcCoreMessenger) << "Got unexpected message of type:" << xmppMessage.type();
        qCDebug(lcCoreMessengerXMPP).noquote() << "Got unexpected message:" << toXmlString(xmppMessage);

        return Self::Result::Success;
    });
}

Self::Result Self::processChatReceivedXmppMessage(const QXmppMessage &xmppMessage)
{
    auto message = std::make_unique<IncomingMessage>();

    message->setId(MessageId(xmppMessage.id()));
    message->setRecipientId(userIdFromJid(xmppMessage.to()));
    message->setSenderId(userIdFromJid(xmppMessage.from()));
    message->setCreatedAt(xmppMessage.stamp().toLocalTime());

    if (message->recipientId() != currentUser()->id()) {
        qCWarning(lcCoreMessenger) << "Got message sent to an another account:" << message->recipientId();

        return Self::Result::Error_InvalidMessageRecipient;
    }

    //
    //  Decode message body from Base64 and JSON.
    //
    auto messageCiphertextResult = unpackXmppMessageBody(xmppMessage.body());
    if (auto status = std::get_if<CoreMessengerStatus>(&messageCiphertextResult)) {
        return *status;
    }

    auto messageCiphertext = std::move(*std::get_if<QByteArray>(&messageCiphertextResult));

    //
    //  Decrypt message.
    //
    auto messageDataResult = decryptPersonalMessage(message->senderId(), messageCiphertext);
    if (auto status = std::get_if<CoreMessengerStatus>(&messageDataResult)) {
        if (*status == Self::Result::Success) {
            qCWarning(lcCoreMessenger) << "Can not decrypt message for now, try it later.";
            message->setContent(MessageContentEncrypted(std::move(messageCiphertext)));
            emit messageReceived(std::move(message));
        }
        return *status;
    }

    auto messageData = std::move(*std::get_if<QByteArray>(&messageDataResult));

    //
    //  Unpack message.
    //
    if (auto result = unpackMessage(messageData, *message); result != Self::Result::Success) {
        return result;
    }

    //
    //  Tell the world we got a message.
    //
    message->setStage(IncomingMessageStage::Decrypted);

    emit messageReceived(std::move(message));

    return Self::Result::Success;
}

Self::Result Self::processGroupChatReceivedXmppMessage(const QXmppMessage &xmppMessage)
{
    const auto groupId = groupIdFromJid(xmppMessage.from());
    const auto senderId = groupUserIdFromJid(xmppMessage.from());
    const auto recipientId = UserId(groupId);

    if (senderId == currentUser()->id()) {
        return processGroupChatReceivedXmppCarbonMessage(xmppMessage);
    }

    auto message = std::make_unique<IncomingMessage>();
    message->setId(MessageId(xmppMessage.id()));
    message->setRecipientId(recipientId);
    message->setSenderId(senderId);
    message->setCreatedAt(xmppMessage.stamp());
    message->setGroupChatInfo(std::make_unique<MessageGroupChatInfo>(groupId));

    qCDebug(lcCoreMessenger) << "Received group message:" << message->id() << ", from user:" << message->senderId()
                             << ", to group:" << groupId;

    //
    //  Decode message body from Base64 and JSON.
    //
    auto ciphertextResult = unpackXmppMessageBody(xmppMessage.body());
    if (auto status = std::get_if<CoreMessengerStatus>(&ciphertextResult)) {
        return *status;
    }

    auto ciphertext = std::move(*std::get_if<QByteArray>(&ciphertextResult));

    //
    //  Decrypt message.
    //
    auto decryptedMessageResult = decryptGroupMessage(groupId, message->senderId(), ciphertext);

    if (auto status = std::get_if<Self::Result>(&decryptedMessageResult)) {
        qCDebug(lcCoreMessenger) << "Can not decrypt a message for group:" << groupId
                                 << ", from:" << message->senderId();

        if (*status == Self::Result::Error_GroupNotFound) {
            message->setContent(MessageContentEncrypted(std::move(ciphertext)));
            emit messageReceived(std::move(message));
            return Self::Result::Success;
        }

        return *status;
    }

    auto plaintextData = std::move(*std::get_if<QByteArray>(&decryptedMessageResult));

    message->setStage(IncomingMessageStage::Decrypted);

    qCInfo(lcCoreMessenger) << "Received XMPP message was decrypted";

    //
    //  Unpack message.
    //
    if (auto result = unpackMessage(plaintextData, *message); result != Self::Result::Success) {
        return result;
    }

    qCInfo(lcCoreMessenger) << "Received XMPP message was parsed";

    //
    //  Tell the world we got a message.
    //
    emit messageReceived(std::move(message));

    return Self::Result::Success;
}

Self::Result Self::processErrorXmppMessage(const QXmppMessage &xmppMessage)
{

    qCDebug(lcCoreMessengerXMPP).noquote() << "Got error message:" << toXmlString(xmppMessage);

    //
    //  TODO: Emit error.
    //
    return Self::Result::Success;
}

QFuture<Self::Result> Self::processReceivedXmppCarbonMessage(const QXmppMessage &xmppMessage)
{

    return QtConcurrent::run([this, xmppMessage]() -> Result {
        qCInfo(lcCoreMessenger) << "Received Carbon XMPP message:" << xmppMessage.id();
        qCDebug(lcCoreMessenger) << "Received Carbon XMPP message:" << xmppMessage.id()
                                 << "from:" << xmppMessage.from();

        switch (xmppMessage.marker()) {
        case QXmppMessage::Marker::Displayed:
        case QXmppMessage::Marker::Acknowledged: {
            emit updateMessage(
                    IncomingMessageStageUpdate { MessageId(xmppMessage.markedId()), IncomingMessageStage::Read });
            return Self::Result::Success;
        }
        default:
            break;
        }

        switch (xmppMessage.type()) {
        case QXmppMessage::Type::Chat:
            return processChatReceivedXmppCarbonMessage(xmppMessage);

        case QXmppMessage::Type::GroupChat:
            return processGroupChatReceivedXmppCarbonMessage(xmppMessage);

        case QXmppMessage::Type::Error:
            return processErrorXmppMessage(xmppMessage);

        default:
            break;
        }

        qCWarning(lcCoreMessenger) << "Got unexpected message of type:" << xmppMessage.type();
        qCDebug(lcCoreMessengerXMPP).noquote() << "Got unexpected message:" << toXmlString(xmppMessage);

        return Self::Result::Success;
    });
}

Self::Result Self::processChatReceivedXmppCarbonMessage(const QXmppMessage &xmppMessage)
{
    auto senderId = userIdFromJid(xmppMessage.from());
    auto recipientId = userIdFromJid(xmppMessage.to());

    if (currentUser()->id() != senderId) {
        qCWarning(lcCoreMessenger) << "Got message carbons copy from an another account:" << senderId;
        return Self::Result::Error_InvalidCarbonMessage;
    }

    auto message = std::make_unique<OutgoingMessage>();

    message->setId(MessageId(xmppMessage.id()));
    message->setRecipientId(recipientId);
    message->setSenderId(senderId);
    message->setCreatedAt(xmppMessage.stamp());
    message->markAsOutgoingCopyFromOtherDevice();
    message->setStage(OutgoingMessageStage::Sent);

    //
    //  Decode message body from Base64 and JSON.
    //
    auto ciphertextResult = unpackXmppMessageBody(xmppMessage.body());
    if (auto status = std::get_if<CoreMessengerStatus>(&ciphertextResult)) {
        return *status;
    }

    auto ciphertext = std::move(*std::get_if<QByteArray>(&ciphertextResult));

    //
    //  Sender is a current user.
    //
    auto sender = currentUser();

    //
    //  Decrypt message.
    //
    auto plaintextDataMinLen = vssq_messenger_decrypted_message_len(m_impl->messenger.get(), ciphertext.size());

    auto [plaintextData, plaintext] = makeMappedBuffer(plaintextDataMinLen);

    const vssq_status_t decryptionStatus = vssq_messenger_decrypt_data(
            m_impl->messenger.get(), vsc_data_from(ciphertext), sender->impl()->user.get(), plaintext.get());

    if (decryptionStatus != vssq_status_SUCCESS) {
        qCWarning(lcCoreMessenger) << "Can not decrypt ciphertext from carbon message:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(decryptionStatus));
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    adjustMappedBuffer(plaintext, plaintextData);

    qCInfo(lcCoreMessenger) << "Received XMPP carbon message was decrypted";

    //
    //  Unpack message.
    //
    if (auto result = unpackMessage(plaintextData, *message); result != Self::Result::Success) {
        return result;
    }

    qCInfo(lcCoreMessenger) << "Received XMPP message was parsed";

    //
    //  Tell the world we got a message.
    //
    emit messageReceived(std::move(message));

    return Self::Result::Success;
};

Self::Result Self::processGroupChatReceivedXmppCarbonMessage(const QXmppMessage &xmppMessage)
{

    auto groupId = groupIdFromJid(xmppMessage.from());

    auto message = std::make_unique<OutgoingMessage>();

    message->setId(MessageId(xmppMessage.id()));
    message->setRecipientId(UserId(QString(groupIdFromJid(xmppMessage.from()))));
    message->setSenderId(groupUserIdFromJid(xmppMessage.from()));
    message->setCreatedAt(xmppMessage.stamp());
    message->setGroupChatInfo(std::make_unique<MessageGroupChatInfo>(groupId));
    message->setStage(OutgoingMessageStage::Sent);
    message->markAsOutgoingCopyFromOtherDevice();

    qCDebug(lcCoreMessenger) << "Received carbon group message:" << message->id()
                             << ", from user:" << message->senderId() << ", to group:" << groupId;

    //
    //  Decode message body from Base64 and JSON.
    //
    auto ciphertextResult = unpackXmppMessageBody(xmppMessage.body());
    if (auto status = std::get_if<CoreMessengerStatus>(&ciphertextResult)) {
        return *status;
    }

    auto ciphertext = std::move(*std::get_if<QByteArray>(&ciphertextResult));

    //
    //  Decrypt message.
    //
    auto decryptedMessageResult = decryptGroupMessage(groupId, message->senderId(), ciphertext);

    if (auto status = std::get_if<Self::Result>(&decryptedMessageResult)) {
        qCDebug(lcCoreMessenger) << "Can not decrypt a message for group:" << groupId
                                 << ", from:" << message->senderId();

        if (*status == Self::Result::Error_GroupNotFound) {
            message->setContent(MessageContentEncrypted(std::move(ciphertext)));
            emit messageReceived(std::move(message));
            return Self::Result::Success;
        }

        return *status;
    }

    auto plaintextData = std::move(*std::get_if<QByteArray>(&decryptedMessageResult));

    qCInfo(lcCoreMessenger) << "Received XMPP message was decrypted";

    //
    //  Unpack message.
    //
    if (auto result = unpackMessage(plaintextData, *message); result != Self::Result::Success) {
        return result;
    }

    qCInfo(lcCoreMessenger) << "Received XMPP message was parsed";

    //
    //  Tell the world we got a message.
    //
    message->setStage(OutgoingMessageStage::Delivered);
    emit messageReceived(std::move(message));

    return Self::Result::Success;
}

std::tuple<Self::Result, QByteArray, QByteArray> Self::encryptFile(const QString &sourceFilePath,
                                                                   const QString &destFilePath)
{
    //
    //  Create helpers for error handling.
    //
    auto cryptoError = [](vssq_status_t status) -> std::tuple<Result, QByteArray, QByteArray> {
        qCWarning(lcCoreMessenger) << "Can not encrypt file:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(status));

        return std::make_tuple(Self::Result::Error_FileEncryptionCryptoFailed, QByteArray(), QByteArray());
    };

    auto fileError = [](Self::Result error) -> std::tuple<Result, QByteArray, QByteArray> {
        qCWarning(lcCoreMessenger) << "Can not encrypt file - read/write failed.";

        return std::make_tuple(error, QByteArray(), QByteArray());
    };

    //
    //  Open File Streams.
    //
    QFile sourceFile(sourceFilePath);
    if (!sourceFile.exists()) {
        qCWarning(lcCoreMessenger) << "Can not encrypt file - source file not exists.";
        return fileError(Self::Result::Error_FileDecryptionReadFailed);
    }

    if (!sourceFile.open(QIODevice::ReadOnly)) {
        return fileError(Self::Result::Error_FileEncryptionReadFailed);
    }

    QFile destFile(destFilePath);
    if (!destFile.open(QIODevice::WriteOnly)) {
        return fileError(Self::Result::Error_FileEncryptionReadFailed);
    }

    const auto fileSize = sourceFile.size();

    //
    //  Encrypt - Step 1 - Initialize cipher.
    //
    auto fileCipher = vssq_messenger_file_cipher_wrap(vssq_messenger_file_cipher_new());
    vssq_status_t encryptionStatus = vssq_messenger_file_cipher_setup_defaults(fileCipher.get());

    if (encryptionStatus != vssq_status_SUCCESS) {
        return cryptoError(encryptionStatus);
    }

    //
    //  Encrypt - Step 2 - Get and store decryption key.
    //
    const auto decryptionKeyBufLen = vssq_messenger_file_cipher_init_encryption_out_key_len(fileCipher.get());
    auto [decryptionKey, decryptionKeyBuf] = makeMappedBuffer(decryptionKeyBufLen);

    encryptionStatus = vssq_messenger_file_cipher_init_encryption(fileCipher.get(), decryptionKeyBuf.get());

    if (encryptionStatus != vssq_status_SUCCESS) {
        return cryptoError(encryptionStatus);
    }

    adjustMappedBuffer(decryptionKeyBuf, decryptionKey);

    //
    //  Encrypt - Step 3 - Write head.
    //
    auto workingBuffer = vsc_buffer_wrap_ptr(vsc_buffer_new());

    const auto headLen = vssq_messenger_file_cipher_start_encryption_out_len(fileCipher.get());
    vsc_buffer_reset_with_capacity(workingBuffer.get(), headLen);

    encryptionStatus = vssq_messenger_file_cipher_start_encryption(fileCipher.get(), workingBuffer.get());

    if (encryptionStatus != vssq_status_SUCCESS) {
        return cryptoError(encryptionStatus);
    }

    if (destFile.write((const char *)vsc_buffer_bytes(workingBuffer.get()), vsc_buffer_len(workingBuffer.get()))
        == -1) {
        return fileError(Self::Result::Error_FileEncryptionWriteFailed);
    }

    //
    //  Encrypt - Step 4 - Encrypt file.
    //
    qint64 processedFileSize = 0;
    while (!sourceFile.atEnd()) {
        //
        //  Read chunk.
        //
        char readBytes[2048];
        const auto readBytesSize = sourceFile.read(readBytes, sizeof(readBytes));

        if (readBytesSize == -1) {
            return fileError(Self::Result::Error_FileEncryptionReadFailed);
        } else {
            processedFileSize += readBytesSize;
        }

        vsc_data_t readBytesData = vsc_data((byte *)readBytes, (size_t)readBytesSize);

        //
        //  Encrypt chunk.
        //
        const auto workingBufferLen =
                vssq_messenger_file_cipher_process_encryption_out_len(fileCipher.get(), readBytesData.len);

        vsc_buffer_reset_with_capacity(workingBuffer.get(), workingBufferLen);

        encryptionStatus =
                vssq_messenger_file_cipher_process_encryption(fileCipher.get(), readBytesData, workingBuffer.get());

        if (encryptionStatus != vssq_status_SUCCESS) {
            return cryptoError(encryptionStatus);
        }

        //
        //  Write chunk.
        //
        if (destFile.write((const char *)vsc_buffer_bytes(workingBuffer.get()),
                           (qint64)vsc_buffer_len(workingBuffer.get()))
            == -1) {
            return fileError(Self::Result::Error_FileEncryptionWriteFailed);
        }
    }

    if (processedFileSize != fileSize) {
        return fileError(Self::Result::Error_FileEncryptionReadFailed);
    }

    //
    //  Encrypt - Step 5 - Write tail and produce signature.
    //
    const auto tailLen = vssq_messenger_file_cipher_finish_encryption_out_len(fileCipher.get());
    vsc_buffer_reset_with_capacity(workingBuffer.get(), tailLen);

    const auto ownerPrivateKey = vssq_messenger_creds_private_key(vssq_messenger_creds(m_impl->messenger.get()));

    const auto signatureBufferLen =
            vssq_messenger_file_cipher_finish_encryption_signature_len(fileCipher.get(), ownerPrivateKey);
    auto [signature, signatureBuffer] = makeMappedBuffer(signatureBufferLen);

    encryptionStatus = vssq_messenger_file_cipher_finish_encryption(fileCipher.get(), ownerPrivateKey,
                                                                    workingBuffer.get(), signatureBuffer.get());

    if (encryptionStatus != vssq_status_SUCCESS) {
        return cryptoError(encryptionStatus);
    }

    if (destFile.write((const char *)vsc_buffer_bytes(workingBuffer.get()), vsc_buffer_len(workingBuffer.get()))
        == -1) {
        return fileError(Self::Result::Error_FileEncryptionWriteFailed);
    }

    //
    //  Return result.
    //
    return std::make_tuple(Self::Result::Success, std::move(decryptionKey), std::move(signature));
}

Self::Result Self::decryptFile(const QString &sourceFilePath, const QString &destFilePath,
                               const QByteArray &decryptionKey, const QByteArray &signature, const UserId senderId)
{
    //
    //  Create helpers for error handling.
    //
    auto cryptoError = [](vssq_status_t status) -> Self::Result {
        qCWarning(lcCoreMessenger) << "Can not decrypt file:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(status));

        return Self::Result::Error_FileDecryptionCryptoFailed;
    };

    auto fileError = [](Self::Result error) -> Self::Result {
        qCWarning(lcCoreMessenger) << "Can not decrypt file - read/write failed.";

        return error;
    };

    //
    //  Find file sender.
    //
    auto sender = findUserById(senderId);
    if (!sender) {
        qCWarning(lcCoreMessenger) << "Can not decrypt file - file sender info is not found.";
        return Self::Result::Error_UserNotFound;
    }

    //
    //  Open File Streams.
    //
    QFile sourceFile(sourceFilePath);
    if (!sourceFile.exists()) {
        qCWarning(lcCoreMessenger) << "Can not decrypt file - source file not exists.";
        return fileError(Self::Result::Error_FileDecryptionReadFailed);
    }

    if (!sourceFile.open(QIODevice::ReadOnly)) {
        return fileError(Self::Result::Error_FileDecryptionReadFailed);
    }

    QFile destFile(destFilePath);
    if (!destFile.open(QIODevice::WriteOnly)) {
        return fileError(Self::Result::Error_FileDecryptionReadFailed);
    }

    const auto fileSize = sourceFile.size();

    //
    //  Decrypt - Step 1 - Initialize crypto.
    //
    auto fileCipher = vssq_messenger_file_cipher_wrap(vssq_messenger_file_cipher_new());
    vssq_status_t decryptionStatus = vssq_messenger_file_cipher_setup_defaults(fileCipher.get());

    if (decryptionStatus != vssq_status_SUCCESS) {
        return cryptoError(decryptionStatus);
    }

    //
    //  Decrypt - Step 2 - Setup decryption key.
    //
    decryptionStatus = vssq_messenger_file_cipher_start_decryption(fileCipher.get(), vsc_data_from(decryptionKey),
                                                                   vsc_data_from(signature));

    if (decryptionStatus != vssq_status_SUCCESS) {
        return cryptoError(decryptionStatus);
    }

    //
    //  Decrypt - Step 3 - Decrypt file.
    //
    auto workingBuffer = vsc_buffer_wrap_ptr(vsc_buffer_new());

    qint64 processedFileSize = 0;
    while (!sourceFile.atEnd()) {
        //
        //  Read chunk.
        //
        char readBytes[2048];
        const auto readBytesSize = sourceFile.read(readBytes, sizeof(readBytes));

        if (readBytesSize == -1) {
            return fileError(Self::Result::Error_FileDecryptionReadFailed);
        } else {
            processedFileSize += readBytesSize;
        }

        vsc_data_t readBytesData = vsc_data((byte *)readBytes, (size_t)readBytesSize);

        //
        //  Decrypt chunk.
        //
        const auto workingBufferLen =
                vssq_messenger_file_cipher_process_decryption_out_len(fileCipher.get(), readBytesData.len);

        vsc_buffer_reset_with_capacity(workingBuffer.get(), workingBufferLen);

        decryptionStatus =
                vssq_messenger_file_cipher_process_decryption(fileCipher.get(), readBytesData, workingBuffer.get());

        if (decryptionStatus != vssq_status_SUCCESS) {
            return cryptoError(decryptionStatus);
        }

        //
        //  Write chunk.
        //
        if (destFile.write((const char *)vsc_buffer_bytes(workingBuffer.get()),
                           (qint64)vsc_buffer_len(workingBuffer.get()))
            == -1) {
            return fileError(Self::Result::Error_FileDecryptionWriteFailed);
        }
    }

    if (processedFileSize != fileSize) {
        return fileError(Self::Result::Error_FileDecryptionReadFailed);
    }

    //
    //  Decrypt - Step 4 - Write tail and verify signature.
    //
    const auto tailLen = vssq_messenger_file_cipher_finish_decryption_out_len(fileCipher.get());
    vsc_buffer_reset_with_capacity(workingBuffer.get(), tailLen);

    const auto senderPublicKey = vssq_messenger_user_public_key(sender->impl()->user.get());
    decryptionStatus =
            vssq_messenger_file_cipher_finish_decryption(fileCipher.get(), senderPublicKey, workingBuffer.get());

    if (decryptionStatus != vssq_status_SUCCESS) {
        return cryptoError(decryptionStatus);
    }

    if (destFile.write((const char *)vsc_buffer_bytes(workingBuffer.get()), vsc_buffer_len(workingBuffer.get()))
        == -1) {
        return fileError(Self::Result::Error_FileDecryptionWriteFailed);
    }

    return Self::Result::Success;
}

// --------------------------------------------------------------------------
//  XMPP event handlers.
// --------------------------------------------------------------------------
void Self::xmppOnConnected()
{
    m_impl->lastActivityManager->setEnabled(true);

    //
    //  TODO: Extract to a method
    //
    for (auto xmppRoom : m_impl->xmppGroupChatManager->rooms()) {
        if (xmppRoom->isJoined()) {
            qCDebug(lcCoreMessenger) << "XMPP room is already joined:" << xmppRoom->jid();
        } else {
            qCDebug(lcCoreMessenger) << "Re-join XMPP room:" << xmppRoom->jid();
            if (xmppRoom->join()) {
                qCDebug(lcCoreMessenger) << "Sent request to join XMPP room:" << xmppRoom->jid();
            } else {
                qCDebug(lcCoreMessenger) << "Failed to sent request to join XMPP room:" << xmppRoom->jid();
            }
        }
    }

    changeConnectionState(Self::ConnectionState::Connected);

    //
    //  TODO: get available status from the cache as described within XEP-0318
    //
    QXmppPresence presenceOnline(QXmppPresence::Available);
    presenceOnline.setAvailableStatusType(QXmppPresence::Online);
    m_impl->xmpp->setClientPresence(presenceOnline);

    registerPushNotifications();

    syncLocalAndRemoteGroups();

    syncPrivateChatsHistory();
}

void Self::xmppOnDisconnected()
{
    m_impl->lastActivityManager->setEnabled(false);

    changeConnectionState(Self::ConnectionState::Disconnected);
}

void Self::xmppOnStateChanged(QXmppClient::State state)
{
    if (QXmppClient::ConnectingState == state) {
        changeConnectionState(Self::ConnectionState::Connecting);
    }
}

void Self::xmppOnError(QXmppClient::Error error)
{
    qCWarning(lcCoreMessenger) << "XMPP error:" << error;

    switch (m_impl->connectionState) {
    case Self::ConnectionState::Connecting:
    case Self::ConnectionState::Connected:
        disconnectXmppServer();
        break;

    case Self::ConnectionState::Disconnected:
    case Self::ConnectionState::Error:
        break;
    }
    emit connectionStateChanged(Self::ConnectionState::Error);

    // Wait 10 second and try to reconnect.
    qCDebug(lcCoreMessenger) << "Emit reconnect in 10s from: xmppOnError()";
    QTimer::singleShot(10000, this, &Self::reconnectXmppServerIfNeeded);
}

void Self::xmppOnPresenceReceived(const QXmppPresence &presence)
{
    qCDebug(lcCoreMessengerXMPP).noquote() << "Got XMPP presence:" << toXmlString(presence);
}

void Self::xmppOnIqReceived(const QXmppIq &iq)
{
    qCDebug(lcCoreMessengerXMPP).noquote() << "Got XMPP IQ:" << toXmlString(iq);
}

void Self::xmppOnSslErrors(const QList<QSslError> &errors)
{
    qCWarning(lcCoreMessenger) << "XMPP SSL errors:" << errors;
    emit connectionStateChanged(Self::ConnectionState::Error);

    // Wait 10 second and try to reconnect.
    qCDebug(lcCoreMessenger) << "Emit reconnect in 10s from: xmppOnSslErrors()";
    QTimer::singleShot(10000, this, &Self::reconnectXmppServerIfNeeded);
}

void Self::xmppOnMessageReceived(const QXmppMessage &xmppMessage)
{
    //
    //  Got non archived message so send 'received' mark.
    //  TODO: Decide if need to filter group chat messages.
    //
    const bool isIncomingValidMarkableMessage = (xmppMessage.type() == QXmppMessage::Chat)
            && !xmppMessage.from().isEmpty() && !xmppMessage.id().isEmpty() && xmppMessage.isMarkable();

    if (isIncomingValidMarkableMessage) {
        QXmppMessage mark;
        mark.setTo(xmppMessage.from());
        mark.setFrom(xmppMessage.to());
        mark.setMarkerId(xmppMessage.id());
        mark.addHint(QXmppMessage::Store);
        mark.setMarker(QXmppMessage::Marker::Received);
        m_impl->xmpp->sendPacket(mark);

        qCDebug(lcCoreMessenger) << "Send 'received' marker for message:" << xmppMessage.id();
    }

    //
    //
    //  TODO: handle result.
    //
    processReceivedXmppMessage(xmppMessage);
}

void Self::xmppOnCarbonMessageReceived(const QXmppMessage &xmppMessage)
{
    //
    //  TODO: handle result.
    //
    processReceivedXmppCarbonMessage(xmppMessage);
}

void Self::xmppOnMessageDelivered(const QString &jid, const QString &messageId)
{
    qCDebug(lcCoreMessenger) << "Message delivered to:" << jid;
    OutgoingMessageStageUpdate update;
    update.messageId = MessageId(messageId);
    update.stage = OutgoingMessageStage::Delivered;
    emit updateMessage(update);
}

// --------------------------------------------------------------------------
//  Network event handlers.
// --------------------------------------------------------------------------
void Self::onProcessNetworkState(bool isOnline)
{
    if (isOnline) {
        qCDebug(lcCoreMessenger) << "Network go online.";
        qCDebug(lcCoreMessenger) << "Emit reconnect when network changed";

        emit disconnectXmppServer();
        emit reconnectXmppServerIfNeeded();
    } else {
        qCDebug(lcCoreMessenger) << "Network go offline.";
        emit disconnectXmppServer();
    }
}

// --------------------------------------------------------------------------
//  LastActivityManager: controls and events.
// --------------------------------------------------------------------------
void Self::setCurrentRecipient(const UserId &recipientId)
{
    m_impl->lastActivityManager->setCurrentJid(recipientId.isValid() ? userIdToJid(recipientId) : QString());
}

// --------------------------------------------------------------------------
//  File upload: controls and events.
// --------------------------------------------------------------------------
bool Self::isUploadServiceFound() const
{
    return m_impl->xmppUploadManager->serviceFound();
}

QString Self::requestUploadSlot(const QString &filePath)
{
    return m_impl->xmppUploadManager->requestUploadSlot(QFileInfo(filePath));
}

void Self::xmppOnUploadServiceFound()
{

    const auto found = m_impl->xmppUploadManager->serviceFound();
    qCDebug(lcCoreMessenger) << "Upload service found:" << found;
    emit uploadServiceFound(found);
}

void Self::xmppOnUploadSlotReceived(const QXmppHttpUploadSlotIq &slot)
{
    emit uploadSlotReceived(slot.id(), slot.putUrl(), slot.getUrl());
}

void Self::xmppOnUploadRequestFailed(const QXmppHttpUploadRequestIq &request)
{

    const auto error = request.error();

    qCDebug(lcCoreMessenger)
            << QString("code(%1), condition(%2), text(%3)").arg(error.code()).arg(error.condition()).arg(error.text());

    if (error.condition() == QXmppStanza::Error::Condition::NotAcceptable && error.code() == 406) {
        emit uploadSlotErrorOccurred(request.id(), tr("File is larger than limit"));
    } else {
        emit uploadSlotErrorOccurred(request.id(), tr("Fail to upload file"));
    }
}

// --------------------------------------------------------------------------
//  Handle self events for debug purposes.
// --------------------------------------------------------------------------
void Self::onLogConnectionStateChanged(CoreMessenger::ConnectionState state)
{
    switch (state) {
    case Self::ConnectionState::Disconnected:
        qCDebug(lcCoreMessenger) << "New connection status: disconnected";
        break;

    case Self::ConnectionState::Connecting:
        qCDebug(lcCoreMessenger) << "New connection status: connecting";
        break;

    case Self::ConnectionState::Connected:
        qCDebug(lcCoreMessenger) << "New connection status: connected";
        break;

    case Self::ConnectionState::Error:
        qCDebug(lcCoreMessenger) << "New connection status: error";
        break;
    }
}

// --------------------------------------------------------------------------
//  Message processing helpers.
// --------------------------------------------------------------------------
QByteArray Self::packMessage(const MessageHandler &message)
{
    //
    //  Pack message to JSON.
    //
    QJsonObject messageJson;
    messageJson.insert("version", "v3");
    messageJson.insert("timestamp", static_cast<qint64>(message->createdAt().toTime_t()));
    messageJson.insert("from", message->senderUsername());
    messageJson.insert("to", message->recipientUsername());
    messageJson.insert("content", MessageContentJsonUtils::to(message->content()));

    return MessageContentJsonUtils::toBytes(messageJson);
}

CoreMessengerStatus Self::unpackMessage(const QByteArray &messageData, Message &message)
{
    //
    //  Parse message.
    //
    const auto messageJsonDocument = QJsonDocument::fromJson(messageData);
    if (messageJsonDocument.isNull()) {
        qCWarning(lcCoreMessenger) << "Got invalid message format - not a JSON";
        return Self::Result::Error_InvalidMessageFormat;
    }

    const auto messageJson = messageJsonDocument.object();

    //
    //  Check version and timestamp.
    //
    const auto version = messageJson["version"].toString();
    if (version != "v3") {
        qCWarning(lcCoreMessenger) << "Got invalid message - unsupported version, expected v3, got" << version;
        return Self::Result::Error_InvalidMessageVersion;
    }

    const auto timestamp = messageJson["timestamp"].toVariant().value<uint>();
    if (auto xmppStamp = message.createdAt().toTime_t(); timestamp != xmppStamp) {
        qCWarning(lcCoreMessenger) << "Got invalid message - timestamp mismatch. Expected" << timestamp << ", xmpp"
                                   << xmppStamp;
        message.setCreatedAt(QDateTime::fromTime_t(timestamp));
    }

    //
    //  Get sender and recipient usernames.
    //
    auto senderUsername = messageJson["from"].toString();
    if (senderUsername.isEmpty()) {
        qCWarning(lcCoreMessenger) << "Got invalid message - missing 'from' username";
        return Self::Result::Error_InvalidMessageFormat;
    }

    message.setSenderUsername(std::move(senderUsername));

    auto recipientUsername = messageJson["to"].toString();
    if (!recipientUsername.isEmpty()) {
        message.setRecipientUsername(std::move(recipientUsername));
    }

    //
    //  Parse content.
    //
    const auto contentJson = messageJson["content"].toObject();
    if (contentJson.isEmpty()) {
        qCWarning(lcCoreMessenger) << "Got invalid message - missing content";
        return Self::Result::Error_InvalidMessageFormat;
    }

    QString errorString;
    auto content = MessageContentJsonUtils::from(contentJson, errorString);

    if (std::get_if<std::monostate>(&content)) {
        qCWarning(lcCoreMessenger) << "Got invalid message - invalid content:" << errorString;
        return Self::Result::Error_InvalidMessageFormat;
    }

    message.setContent(std::move(content));

    qCDebug(lcCoreMessenger) << "Received XMPP message was unpacked:" << message.id();

    return Self::Result::Success;
}

QByteArray Self::packXmppMessageBody(const QByteArray &messageCiphertext, PushType pushType)
{
    //
    //  Pack to JSON and return Base64 string.
    //
    QJsonObject messageBodyJson;

    switch (pushType) {
    case PushType::None:
        messageBodyJson["pushType"] = "none";
        break;

    case PushType::Alert:
        messageBodyJson["pushType"] = "alert";
        break;

    case PushType::Voip:
        messageBodyJson["pushType"] = "voip";
        break;
    }

    messageBodyJson["ciphertext"] = QString::fromLatin1(messageCiphertext.toBase64());

    return MessageContentJsonUtils::toBytes(messageBodyJson).toBase64();
}

std::variant<CoreMessengerStatus, QByteArray> Self::unpackXmppMessageBody(const QString &xmppMessageBody)
{
    //
    //  Decode message body from Base64 and JSON.
    //
    auto messageBody = xmppMessageBody.toLatin1();
    if (messageBody.isEmpty()) {
        qCWarning(lcCoreMessenger) << "Got invalid XMPP message - body is empty";
        return Self::Result::Error_InvalidMessageFormat;
    }

    auto messageBodyJsonString = QByteArray::fromBase64Encoding(messageBody);
    if (!messageBodyJsonString) {
        qCWarning(lcCoreMessenger) << "Got invalid XMPP message - body is not Base64";
        return Self::Result::Error_InvalidMessageFormat;
    }

    const auto messageBodyJsonDocument = QJsonDocument::fromJson(*messageBodyJsonString);
    if (messageBodyJsonDocument.isNull()) {
        qCWarning(lcCoreMessenger) << "Got invalid XMPP message - body is not JSON within Base64";
        return Self::Result::Error_InvalidMessageFormat;
    }

    const auto messageBodyJson = messageBodyJsonDocument.object();

    //
    //  Get ciphertext.
    //
    const auto ciphertextBase64 = messageBodyJson["ciphertext"].toString();
    if (ciphertextBase64.isEmpty()) {
        qCWarning(lcCoreMessenger) << "Got invalid XMPP message - empty ciphertext";
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    auto ciphertextDecoded = QByteArray::fromBase64Encoding(ciphertextBase64.toLatin1());
    if (!ciphertextDecoded) {
        qCWarning(lcCoreMessenger) << "Got invalid XMPP message - ciphertext is not base64 encoded";
        return Self::Result::Error_InvalidMessageCiphertext;
    }

    return *ciphertextDecoded;
}

// --------------------------------------------------------------------------
//  Cloud FS.
// --------------------------------------------------------------------------
CoreMessengerCloudFs Self::cloudFs() const
{
    auto randomCopy = vscf_impl_wrap_ptr(vscf_impl_shallow_copy(m_impl->random.get()));
    return CoreMessengerCloudFs(this, std::move(randomCopy));
}

const vssq_messenger_cloud_fs_t *Self::cloudFsC() const
{
    const auto result = vssq_messenger_cloud_fs(m_impl->messenger.get());
    Q_ASSERT(result != nullptr);
    return result;
}

// --------------------------------------------------------------------------
//  Group chats.
// --------------------------------------------------------------------------
void Self::createGroupChat(const QString &groupName, const Contacts &contacts)
{
    QtConcurrent::run([this, groupName, contacts]() {
        //
        // Find participants.
        //
        const auto groupId = GroupId::generate();

        qCInfo(lcCoreMessenger) << "Trying to create group chat:" << groupId;
        qCDebug(lcCoreMessenger) << "Create group chat - start to find participants";

        GroupMembers groupMembers;
        auto userListC = vssq_messenger_user_list_wrap_ptr(vssq_messenger_user_list_new());
        for (const auto &contact : contacts) {
            std::shared_ptr<User> user { nullptr };
            //
            //  Find by username.
            //
            if (!contact->username().isEmpty()) {
                user = findUserByUsername(contact->username());
            }

            //
            //  TODO: Add contact by email and phone as well.
            //

            if (user) {
                //
                //  Copy contact to be able define founded user identity.
                //
                auto memberContact = std::make_unique<Contact>(*contact);
                memberContact->setUserId(user->id());

                //
                //  For now User Identity is used as user nickname.
                //  This hack is needed to fetch user when decrypt a group message to be able to verify signature.
                //  Also initial affiliation is "None" that means participant was not added to the XMPP group yet.
                //
                auto groupMember =
                        std::make_unique<GroupMember>(groupId, std::move(memberContact), GroupAffiliation::None);
                groupMembers.push_back(std::move(groupMember));

                vssq_messenger_user_list_add(userListC.get(), (vssq_messenger_user_t *)user->impl()->user.get());
            }
        }

        qCDebug(lcCoreMessenger) << "Create group chat - found" << groupMembers.size() << " participants";
        for (const auto &member : groupMembers) {
            qCDebug(lcCoreMessenger) << "Create group chat - found participant:" << member->memberId();
        }

        if (!vssq_messenger_user_list_has_item(userListC.get())) {
            emit groupChatCreateFailed(groupId, CoreMessengerStatus::Error_GroupNoParticipants);
            return;
        }

        //
        //  Add self as owner to the group.
        //
        auto owner = currentUser();

        auto ownerContact = std::make_unique<Contact>();
        ownerContact->setUserId(owner->id());
        ownerContact->setUsername(owner->username());

        auto groupOwner = std::make_unique<GroupMember>(groupId, std::move(ownerContact), GroupAffiliation::Owner);
        groupMembers.push_back(std::move(groupOwner));

        //
        //  Create Comm Kit group chat.
        //
        qCInfo(lcCoreMessenger) << "Trying to create group chat within CommKit";

        vssq_error_t error;
        vssq_error_reset(&error);

        auto groupIdStd = QString(groupId).toStdString();
        auto groupC = vssq_messenger_group_wrap_ptr(vssq_messenger_create_group(
                m_impl->messenger.get(), vsc_str_from(groupIdStd), userListC.get(), &error));

        if (vssq_error_has_error(&error)) {
            qCWarning(lcCoreMessenger) << "Got error status:"
                                       << vsc_str_to_qstring(vssq_error_message_from_error(&error));
            emit groupChatCreateFailed(groupId, mapStatus(vssq_error_status(&error)));
            return;
        }

        auto groupSessionCacheJson = vssc_json_object_wrap_ptr(vssq_messenger_group_to_json(groupC.get()));
        auto groupSessionCache = vsc_str_to_qstring(vssc_json_object_as_str(groupSessionCacheJson.get()));

        const auto group = std::make_shared<Group>(groupId, currentUser()->id(), groupName,
                                                   GroupInvitationStatus::Accepted, std::move(groupSessionCache));

        {
            std::scoped_lock _(m_impl->groupsMutex);
            m_impl->groups[groupId] = std::make_shared<GroupImpl>(group, std::move(groupC));
        }

        //
        //  Create XMPP multi-user chat room, aka group chat.
        //
        xmppCreateGroupChat(group, groupMembers);
    });
}

void Self::loadGroupChats(const Groups &groups)
{
    Q_ASSERT(isSignedIn());

    QtConcurrent::run([this, groups]() {
        for (const auto &group : groups) {
            vssq_error_t error;
            vssq_error_reset(&error);

            //
            //  Load CommKit group from cached session.
            //
            auto groupImpl = std::make_shared<GroupImpl>(group);
            if (auto groupSessionCache = group->cache().toStdString(); !groupSessionCache.empty()) {
                groupImpl->commKitGroup = vssq_messenger_group_wrap_ptr(vssq_messenger_load_group_from_json_str(
                        m_impl->messenger.get(), vsc_str_from(groupSessionCache), &error));
            }

            {
                std::scoped_lock _(m_impl->groupsMutex);
                m_impl->groups[group->id()] = std::move(groupImpl);
            }

            xmppJoinRoom(group->id());
        }

        syncLocalAndRemoteGroups();
    });
}

void Self::onAcceptGroupInvitation(const GroupId &groupId, const UserId &groupOwnerId)
{
    GroupInvitationUpdate update;
    update.groupId = groupId;
    update.invitationStatus = GroupInvitationStatus::Accepted;
    emit updateGroup(update);

    tryLoadGroupInBackground(groupId, groupOwnerId, false /* do not emit a new group creating */);
}

void Self::onRejectGroupInvitation(const GroupId &groupId, const UserId &groupOwnerId)
{
    GroupInvitationUpdate update;
    update.groupId = groupId;
    update.invitationStatus = GroupInvitationStatus::Rejected;
    emit updateGroup(update);

    if (isOnline()) {
        QXmppElement xElement;
        xElement.setTagName("x");
        xElement.setAttribute("xmlns", "http://jabber.org/protocol/muc#user");

        QXmppElement declineElement;
        declineElement.setTagName("decline");
        declineElement.setAttribute("to", userIdToJid(groupOwnerId));

        xElement.appendChild(declineElement);

        QXmppMessage rejectInvitationMessage;
        rejectInvitationMessage.setTo(groupIdToJid(groupId));
        rejectInvitationMessage.setFrom(currentUserJid());

        rejectInvitationMessage.setExtensions(QXmppElementList() << xElement);

        m_impl->xmpp->sendPacket(rejectInvitationMessage);
    }
}

void Self::onRenameGroupChat(const GroupId &groupId, const QString &groupName)
{
    Q_ASSERT(!groupName.isEmpty());

    if (!isOnline()) {
        qCWarning(lcCoreMessenger) << "Failed to rename group" << groupId << "- no connection";
        return;
    }

    if (!isGroupCached(groupId)) {
        qCWarning(lcCoreMessenger) << "Failed to rename group" << groupId << "- group is not loaded";
        return;
    }

    m_impl->groupRenameRequests[groupId] = groupName;
    if (!xmppRequestRoomConfiguration(groupIdToJid(groupId))) {
        qCWarning(lcCoreMessenger) << "Failed to rename group" << groupId << "- failed to send XMPP request";
        m_impl->groupRenameRequests.erase(groupId);
    }
}

void Self::onGroupChatCreated(const GroupHandler &group, const GroupMembers &groupMembers)
{
    //
    //  Reconnect room signals as group was created.
    //
    for (auto room : m_impl->xmppGroupChatManager->rooms()) {
        const auto roomGroupId = groupIdFromJid(room->jid());
        if (group->id() == roomGroupId) {
            room->disconnect();
            connectXmppRoomSignals(room);
            break;
        }
    }
}

void Self::onGroupChatCreateFailed(const GroupId &chatId, CoreMessengerStatus errorStatus)
{
    //
    //  TODO: Remove CommKit group if it was created.
    //
}

void Self::xmppOnCreateGroupChat(const GroupHandler &group, const GroupMembers &members)
{
    qCInfo(lcCoreMessenger) << "Trying to create group chat within XMPP server";

    //
    //  Create XMPP multi-user chat room, aka group chat.
    //
    auto roomJid = groupIdToJid(group->id());

    QXmppMucRoom *room = m_impl->xmppGroupChatManager->addRoom(roomJid);
    room->setNickName(group->superOwnerId());
    room->setSubject(group->name());

    connect(room, &QXmppMucRoom::joined, [this, group, room]() {
        qCDebug(lcCoreMessenger) << "Joined to the new XMPP room:" << room->jid();

        //
        //  Request Room Configuration to Create Reserved Room.
        //  This is required to set a room name.
        //
        if (!xmppRequestRoomConfiguration(room->jid())) {
            emit groupChatCreateFailed(group->id(), CoreMessengerStatus::Error_CreateGroup_XmppConfigFailed);
        }
    });

    connect(room, &QXmppMucRoom::configurationReceived,
            [this, group, room, members](const QXmppDataForm &configuration) {
                qCDebug(lcCoreMessenger) << "Configure the new XMPP room:" << room->jid();
                qCDebug(lcCoreMessengerXMPP).noquote()
                        << "Got an initial room configuration:" << toXmlString(configuration);

                if (!xmppSendRoomConfiguration(room->jid(), group->name())) {
                    emit groupChatCreateFailed(group->id(), CoreMessengerStatus::Error_CreateGroup_XmppConfigFailed);
                    return;
                }

                //
                //  Now we assume that the group chat was created.
                //  Note, errors are not handled.
                //
                emit groupChatCreated(group, members);

                //
                //  Subscribe to Muc/Sub events.
                //
                m_impl->xmppMucSubManager->unsubscribe(room->jid());
                m_impl->xmppMucSubManager->subscribe(
                        {
                                XmppMucSubEvent::Messages,
                        },
                        room->jid(), currentUser()->id());

                //
                //  Add group members to the room.
                //
                for (const auto &member : members) {
                    const auto memberId = member->memberId();

                    if (memberId == currentUser()->id()) {
                        //  Do not add myself.
                        continue;
                    }

                    //
                    //  Add to the room.
                    //
                    QXmppMucItem addMemberItem;
                    addMemberItem.setAffiliation(QXmppMucItem::MemberAffiliation);
                    addMemberItem.setJid(userIdToJid(memberId));

                    QXmppMucAdminIq requestAddMember;
                    requestAddMember.setFrom(currentUserJid());
                    requestAddMember.setTo(room->jid());
                    requestAddMember.setItems({ addMemberItem });
                    requestAddMember.setType(QXmppIq::Type::Set);

                    if (m_impl->xmpp->sendPacket(requestAddMember)) {
                        qCDebug(lcCoreMessenger)
                                << "Requested to add user:" << memberId << "the XMPP room:" << group->id();
                        emit updateGroup(
                                GroupMemberAffiliationUpdate { group->id(), memberId, GroupAffiliation::Member });

                    } else {
                        qCDebug(lcCoreMessenger) << "User invitation was postponed:" << memberId;
                        continue;
                    }

                    //
                    //  Send invitation.
                    //
                    auto invitationMessage = std::make_shared<OutgoingMessage>();
                    invitationMessage->setId(MessageId::generate());
                    invitationMessage->setSenderId(currentUser()->id());
                    invitationMessage->setSenderUsername(currentUser()->username());
                    invitationMessage->setRecipientId(memberId);
                    // TODO: Review next line to pass display name when search by email and contact will be added.
                    invitationMessage->setRecipientUsername(member->contact()->username());
                    invitationMessage->setContent(
                            MessageContentGroupInvitation { currentUser()->id(), group->name(), "Hello!" });
                    invitationMessage->setGroupChatInfo(std::make_unique<MessageGroupChatInfo>(group->id()));
                    invitationMessage->setCreatedNow();

                    auto invitationMessageData = packMessage(invitationMessage);

                    auto encryptedInvitationMessageResult = encryptPersonalMessage(memberId, invitationMessageData);

                    if (auto status = std::get_if<Self::Result>(&encryptedInvitationMessageResult)) {
                        qCDebug(lcCoreMessenger) << "User invitation was postponed:" << memberId;
                        continue;
                    }

                    auto encryptedInvitationMessage =
                            std::get_if<QByteArray>(&encryptedInvitationMessageResult)->toBase64();

                    if (room->sendInvitation(userIdToJid(memberId), encryptedInvitationMessage)) {
                        qCDebug(lcCoreMessenger) << "User was invited:" << memberId;
                        invitationMessage->setStage(OutgoingMessageStage::Sent);

                    } else {
                        qCDebug(lcCoreMessenger) << "User invitation was postponed:" << memberId;
                    }

                    emit messageReceived(invitationMessage);
                }
            });

    connect(room, &QXmppMucRoom::error, [this, room, group](const QXmppStanza::Error &error) {
        qCWarning(lcCoreMessenger) << "Got room:" << room->jid() << ", error:" << toXmlString(error);
        emit groupChatCreateFailed(group->id(), CoreMessengerStatus::Error_CreateGroup_XmppConfigFailed);
    });

    if (!room->join()) {
        emit groupChatCreateFailed(group->id(), CoreMessengerStatus::Error_CreateGroup_XmppFailed);
    }
}

void Self::xmppOnFetchRoomsFromServer()
{
    qCDebug(lcCoreMessenger) << "Start fetching group chats from an XMPP server...";

    m_impl->xmppMucSubManager->retrieveSubscribedRooms(groupChatsDomain());
}

void Self::xmppOnJoinRoom(const GroupId &groupId)
{
    Q_ASSERT(m_impl->xmppGroupChatManager);

    const auto group = findGroupInCache(groupId);
    Q_ASSERT(group);

    //
    //  Filter out not accepted groups.
    //
    if (group->localGroup->invitationStatus() != GroupInvitationStatus::Accepted) {
        return;
    }

    //
    //  Join to XMPP group.
    //
    auto roomJid = groupIdToJid(groupId);
    qCDebug(lcCoreMessenger) << "Joining XMPP room:" << roomJid;

    auto room = m_impl->xmppGroupChatManager->addRoom(roomJid);
    room->setNickName(currentUser()->id());

    connectXmppRoomSignals(room);

    if (isOnline()) {
        if (room->join()) {
            qCDebug(lcCoreMessenger) << "Send request to join XMPP room:" << roomJid;
        } else {
            qCDebug(lcCoreMessenger) << "Failed to join to XMPP room:" << roomJid;
        }
    } else {
        qCDebug(lcCoreMessenger) << "Network offline so will join to XMPP room later:" << roomJid;
    }
}

void Self::xmppOnMucInvitationReceived(const QString &roomJid, const QString &inviter, const QString &reason)
{

    qCDebug(lcCoreMessenger) << "Got XMPP room invitation:";
    qCDebug(lcCoreMessenger) << "    -> room:" << roomJid;
    qCDebug(lcCoreMessenger) << "    -> inviter:" << inviter;

    QtConcurrent::run([this, roomJid, inviter, reason]() {
        auto groupId = groupIdFromJid(roomJid);
        auto recipientId = currentUser()->id();
        auto senderId = userIdFromJid(inviter);

        //
        //  Create a local invitation message.
        //
        auto message = std::make_shared<IncomingMessage>();
        message->setId(MessageId::generate());
        message->setSenderId(senderId);
        message->setRecipientId(recipientId);
        message->setGroupChatInfo(std::make_unique<MessageGroupChatInfo>(groupId));

        //
        //  Decode from Base64.
        //
        auto maybeEncryptedInvitationMessage = QByteArray::fromBase64Encoding(reason.toLatin1());
        if (!maybeEncryptedInvitationMessage) {
            qCDebug(lcCoreMessenger) << "Received invitation that is not Base64 encoded.";
            return;
        }

        auto messageCiphertext = std::move(*maybeEncryptedInvitationMessage);

        //
        //  Decrypt message.
        //
        auto messageDataResult = decryptPersonalMessage(message->senderId(), messageCiphertext);
        if (auto status = std::get_if<CoreMessengerStatus>(&messageDataResult)) {
            if (*status == Self::Result::Success) {
                qCWarning(lcCoreMessenger) << "Can not decrypt message for now, try it later.";
                message->setContent(MessageContentEncrypted(std::move(messageCiphertext)));
                emit messageReceived(message);
            }
            return;
        }

        auto messageData = std::move(*std::get_if<QByteArray>(&messageDataResult));

        //
        //  Unpack message.
        //
        if (auto result = unpackMessage(messageData, *message); result != Self::Result::Success) {
            return;
        }

        //
        //  Tell the world we got a message.
        //
        message->setStage(IncomingMessageStage::Decrypted);

        emit messageReceived(std::move(message));
    });
}

void Self::xmppOnMucRoomAdded(QXmppMucRoom *room)
{
    qCDebug(lcCoreMessenger) << "!!!!Room was added:" << room->jid();
}

void Self::xmppOnRoomParticipantReceived(const QString &roomJid, const QString &jid,
                                         QXmppMucItem::Affiliation affiliation)
{

    auto mapAffiliation = [](QXmppMucItem::Affiliation affiliation) {
        switch (affiliation) {
        case QXmppMucItem::UnspecifiedAffiliation:
            throw std::logic_error("Got UnspecifiedAffiliation when handle participantAffiliationChanged() signal.");

        case QXmppMucItem::OutcastAffiliation:
            return GroupAffiliation::Outcast;

        case QXmppMucItem::NoAffiliation:
            return GroupAffiliation::None;

        case QXmppMucItem::MemberAffiliation:
            return GroupAffiliation::Member;

        case QXmppMucItem::AdminAffiliation:
            return GroupAffiliation::Admin;

        case QXmppMucItem::OwnerAffiliation:
            return GroupAffiliation::Owner;

        default:
            throw std::logic_error("Invalid affiliation");
        }
    };

    const auto groupId = groupIdFromJid(roomJid);
    const auto userId = userIdFromJid(jid);
    const auto userAffiliation = mapAffiliation(affiliation);

    if (isGroupCached(groupId)) {
        emit updateGroup(GroupMemberAffiliationUpdate { groupId, userId, userAffiliation });

    } else if (userAffiliation == GroupAffiliation::Owner) {
        tryLoadGroupInBackground(groupId, userId, true /* emit creating a new group */);
    }
}

void Self::xmppOnMucSubscribeReceived(const QString &roomJid, const QString &subscriberJid, const QString &nickname)
{
    qCDebug(lcCoreMessenger) << "New subscriber:" << subscriberJid << "nickname" << nickname
                             << "to the room:" << roomJid;
}

void Self::xmppOnMucSubscribedRoomsCountReceived(const QString &id, qsizetype totalRoomsCount)
{
    qCDebug(lcCoreMessenger) << "Got total rooms count:" << totalRoomsCount;
}

void Self::xmppOnMucSubscribedRoomReceived(const QString &id, const QString &roomJid, const QString &subscriberJid,
                                           const std::list<XmppMucSubEvent> &events)
{
    qCDebug(lcCoreMessenger) << "Got subscribed room:" << roomJid;

    const auto groupId = groupIdFromJid(roomJid);
    if (isGroupCached(groupId)) {
        return;
    }

    //
    //  Group is not loaded yet, so try to find it's owner and then load CommKit group.
    //
    m_impl->xmppRoomParticipantsManager->requestAll(roomJid);
}

void Self::xmppOnMucRoomSubscriberReceived(const QString &id, const QString &roomJid, const QString &subscriberJid,
                                           const std::list<XmppMucSubEvent> &events) {};

void Self::xmppOnMucSubscribedRoomsProcessed(const QString &id) { }

void Self::xmppOnMucRoomSubscribersProcessed(const QString &id, const QString &roomJid) { }

void Self::connectXmppRoomSignals(QXmppMucRoom *room)
{
    qCDebug(lcCoreMessenger) << "Connecting room signals:" << room->jid();

    connect(room, &QXmppMucRoom::configurationReceived, [this, room](const QXmppDataForm &configuration) {
        qCDebug(lcCoreMessengerXMPP).noquote() << "Room configuration received:" << toXmlString(configuration);
        //
        //  For now only group name can be changed.
        //
        const auto groupId = groupIdFromJid(room->jid());

        //
        //  Find the group new name and remove it from the cache.
        //
        const auto groupName = [this, &groupId]() {
            const auto it = m_impl->groupRenameRequests.find(groupId);
            auto result = it != m_impl->groupRenameRequests.cend() ? it->second : QString();
            m_impl->groupRenameRequests.erase(groupId);
            return result;
        }();

        if (groupName.isEmpty()) {
            qCCritical(lcCoreMessenger) << "Failed to rename group" << groupId << "- group name was not cached";
            return;
        }

        if (xmppSendRoomConfiguration(room->jid(), groupName)) {
            emit updateGroup(GroupNameUpdate { groupId, groupName });

        } else {
            qCWarning(lcCoreMessenger) << "Failed to rename group" << groupId << "- failed to send configuration";
        }
    });

    connect(room, &QXmppMucRoom::error, [room](const QXmppStanza::Error &error) {
        qCWarning(lcCoreMessenger) << "Got room:" << room->jid() << ", error:" << toXmlString(error);
    });

    connect(room, &QXmppMucRoom::joined, [this, room]() {
        qCDebug(lcCoreMessenger) << "Joined to the XMPP room:" << room->jid();

        // TODO: Maybe replace it with MUC/Sub Subscribers mechanism?
        m_impl->xmppRoomParticipantsManager->requestAll(room->jid());

        m_impl->xmppMucSubManager->unsubscribe(room->jid());
        m_impl->xmppMucSubManager->subscribe(
                {
                        XmppMucSubEvent::Messages,
                },
                room->jid(), currentUser()->id());

        auto groupId = groupIdFromJid(room->jid());
        syncGroupChatHistory(groupId);
    });

    connect(room, &QXmppMucRoom::nameChanged, [this, room](const QString &name) {
        qCDebug(lcCoreMessenger) << "Room name changed:" << room->jid() << name;
        const auto groupId = groupIdFromJid(room->jid());

        emit updateGroup(GroupNameUpdate { groupId, name });
    });
}

CoreMessengerStatus Self::loadGroup(const GroupId &groupId, const UserId &superOwnerId, bool emitNewGroup)
{
    qCDebug(lcCoreMessenger) << "Loading group with id:" << groupId << "and owner:" << superOwnerId;

    //
    //  Find group owner.
    //
    auto groupOwner = findUserById(superOwnerId);
    if (!groupOwner) {
        qCDebug(lcCoreMessenger) << "Group owner:" << superOwnerId << "is not found for group:" << groupId;
        return CoreMessengerStatus::Error_UserNotFound;
    }

    //
    //  Load a group.
    //
    vssq_error_t error;
    vssq_error_reset(&error);

    auto groupIdStdStr = QString(groupId).toStdString();

    auto groupC = vssq_messenger_group_wrap_ptr(vssq_messenger_load_group(
            m_impl->messenger.get(), vsc_str_from(groupIdStdStr), groupOwner->impl()->user.get(), &error));

    if (vssq_error_has_error(&error)) {
        qCDebug(lcCoreMessenger) << "Group not loaded:" << groupId;
        qCWarning(lcCoreMessenger) << "Got error status:" << vsc_str_to_qstring(vssq_error_message_from_error(&error));
        return CoreMessengerStatus::Error_GroupNotFound;
    }

    qCDebug(lcCoreMessenger) << "Group" << groupId << "loaded from the cloud";

    //
    //  Store it locally.
    //
    auto groupSessionCacheJson = vssc_json_object_wrap_ptr(vssq_messenger_group_to_json(groupC.get()));
    auto groupSessionCache = vsc_str_to_qstring(vssc_json_object_as_str(groupSessionCacheJson.get()));

    const auto group = std::make_shared<Group>(groupId, superOwnerId, "Group", GroupInvitationStatus::Accepted,
                                               std::move(groupSessionCache));

    {
        std::scoped_lock _(m_impl->groupsMutex);
        m_impl->groups[groupId] = std::make_shared<GroupImpl>(group, std::move(groupC));
    }

    if (emitNewGroup) {
        emit newGroupChatLoaded(group);
    }

    xmppJoinRoom(groupId);

    return Self::Result::Success;
}

void Self::tryLoadGroupInBackground(const GroupId &groupId, const UserId &superOwnerId, bool emitNewGroup)
{
    QtConcurrent::run([this, groupId, superOwnerId, emitNewGroup]() {
        const auto status = loadGroup(groupId, superOwnerId, emitNewGroup);
        Q_UNUSED(status);
    });
}

bool Self::isGroupCached(const GroupId &groupId) const
{
    return findGroupInCache(groupId).get() != nullptr;
}

void Self::syncLocalAndRemoteGroups()
{
    if (!isOnline()) {
        return;
    }

    QtConcurrent::run([this]() {
        //
        //  Load CommKit group from the service if online and if the cache loading failed.
        //
        for (auto &groupImplIt : m_impl->groups) {
            auto &groupImpl = groupImplIt.second;

            if (!groupImpl->commKitGroup) {
                auto groupOwner = findUserById(groupImpl->localGroup->superOwnerId());
                if (groupOwner) {
                    vssq_error_t error;
                    vssq_error_reset(&error);

                    const auto groupId = QString(groupImpl->localGroup->id()).toStdString();

                    groupImpl->commKitGroup = vssq_messenger_group_wrap_ptr(vssq_messenger_load_group(
                            m_impl->messenger.get(), vsc_str_from(groupId), groupOwner->impl()->user.get(), &error));

                    if (groupImpl->commKitGroup) {
                        updateGroupCache(groupImpl);
                    }
                }
            }
        }

        //
        //  Then fetch XMPP rooms from the remote server.
        //
        xmppFetchRoomsFromServer();
    });
}

void Self::updateGroupCache(const GroupImplHandler &group) const
{
    auto groupSessionCacheJson = vssc_json_object_wrap_ptr(vssq_messenger_group_to_json(group->commKitGroup.get()));
    auto groupSessionCache = vsc_str_to_qstring(vssc_json_object_as_str(groupSessionCacheJson.get()));

    emit updateGroup(GroupCacheUpdate { group->localGroup->id(), groupSessionCache });
}

const Self::GroupImplHandler Self::findGroupInCache(const GroupId &groupId) const
{
    std::scoped_lock _(m_impl->groupsMutex);

    const auto groupIt = m_impl->groups.find(groupId);

    if (groupIt != m_impl->groups.cend()) {
        return groupIt->second;
    }

    return nullptr;
}

std::variant<CoreMessengerStatus, QByteArray> Self::encryptGroupMessage(const GroupId &groupId,
                                                                        const QByteArray &messageData)
{
    //
    //  Find group.
    //
    auto group = findGroupInCache(groupId);
    if (!group || !group->commKitGroup) {
        return Self::Result::Error_GroupNotLoaded;
    }

    //
    //  Encrypt message.
    //
    const auto outLen = vssq_messenger_group_encrypted_message_len(group->commKitGroup.get(), messageData.size());
    auto [out, outBuf] = makeMappedBuffer(outLen);

    const auto status = vssq_messenger_group_encrypt_binary_message(group->commKitGroup.get(),
                                                                    vsc_data_from(messageData), outBuf.get());

    if (status == vssq_status_SUCCESS) {
        adjustMappedBuffer(outBuf, out);
        return out;
    }

    return mapStatus(status);
}

std::variant<CoreMessengerStatus, QByteArray> Self::decryptGroupMessage(const GroupId &groupId, const UserId &senderId,
                                                                        const QByteArray &encryptedMessageData)
{

    //
    //  Find group.
    //
    auto group = findGroupInCache(groupId);
    if (!group || !group->commKitGroup) {
        return Self::Result::Error_GroupNotLoaded;
    }

    //  Find sender.
    //
    auto sender = findUserById(senderId);
    if (!sender) {
        return Self::Result::Error_UserNotFound;
    }

    //
    //  Decrypt message.
    //
    const auto outLen =
            vssq_messenger_group_decrypted_message_len(group->commKitGroup.get(), encryptedMessageData.size());
    auto [out, outBuf] = makeMappedBuffer(outLen);

    const auto status = vssq_messenger_group_decrypt_binary_message(
            group->commKitGroup.get(), vsc_data_from(encryptedMessageData), sender->impl()->user.get(), outBuf.get());

    if (status == vssq_status_SUCCESS) {
        adjustMappedBuffer(outBuf, out);
        return out;
    } else {
        qCWarning(lcCoreMessenger) << "Failed to decrypt group message:"
                                   << vsc_str_to_qstring(vssq_error_message_from_status(status));
    }

    return mapStatus(status);
}

bool Self::xmppRequestRoomConfiguration(const QString &roomJid)
{
    Q_ASSERT(m_impl->xmpp);

    QXmppMucOwnerIq requestConfigurationIq;
    requestConfigurationIq.setFrom(currentUserJid());
    requestConfigurationIq.setTo(roomJid);
    requestConfigurationIq.setType(QXmppIq::Type::Get);

    return m_impl->xmpp->sendPacket(requestConfigurationIq);
}

bool Self::xmppSendRoomConfiguration(const QString &roomJid, const QString &roomName)
{
    Q_ASSERT(m_impl->xmpp);

    QXmppMucOwnerIq acceptIq;
    acceptIq.setFrom(currentUserJid());
    acceptIq.setTo(roomJid);
    acceptIq.setType(QXmppIq::Type::Set);

    QXmppDataForm submitForm;
    submitForm.setType(QXmppDataForm::Type::Submit);

    using Field = QXmppDataForm::Field::Type;

    QList<QXmppDataForm::Field> configFields {
        { Field::HiddenField, "FORM_TYPE", "http://jabber.org/protocol/muc#roomconfig" },
        { Field::TextSingleField, "muc#roomconfig_roomname", roomName },
    };

    submitForm.setFields(configFields);

    acceptIq.setForm(submitForm);

    return m_impl->xmpp->sendPacket(acceptIq);
}

// --------------------------------------------------------------------------
//  Message History (MAM).
// --------------------------------------------------------------------------
void Self::xmppOnArchivedMessageReceived(const QString &queryId, const QXmppMessage &message)
{
    if (message.type() == QXmppMessage::GroupChat) {
        qCDebug(lcCoreMessenger) << "Got group archived message:" << message.id() << "from:" << message.from();
    } else {
        qCDebug(lcCoreMessenger) << "Got archived message:" << message.id() << "from:" << message.from();
    }

    qCDebug(lcCoreMessengerXMPP).noquote() << "Got archived message:" << toXmlString(message);

    const auto senderId = [this, &message]() {
        if (message.type() == QXmppMessage::GroupChat) {
            return groupUserIdFromJid(message.from());
        } else {
            return userIdFromJid(message.from());
        }
    }();

    auto future = [this, &message, &senderId]() {
        if (senderId == currentUser()->id()) {
            return processReceivedXmppCarbonMessage(message);
        } else {
            return processReceivedXmppMessage(message);
        }
    }();

    //
    //  Archived messages should be processed sequentially to grantee correct order of messages and it's marks.
    //
    future.waitForFinished();
}

void Self::xmppOnArchivedResultsRecieved(const QString &queryId, const QXmppResultSetReply &resultSetReply,
                                         bool complete)
{

    qCDebug(lcCoreMessengerXMPP).noquote() << "Got archived messages result:" << toXmlString(resultSetReply);
    qCDebug(lcCoreMessengerXMPP).noquote() << "Got archived messages result complete?:" << complete;

    const auto queryParamIt = m_impl->historySyncQueryParams.find(queryId);
    Q_ASSERT(queryParamIt != m_impl->historySyncQueryParams.cend());
    const auto groupId = queryParamIt->second;

    if (!complete) {
        QXmppResultSetQuery resultSetQuery;
        resultSetQuery.setAfter(resultSetReply.last());
        const auto lastSyncDate = m_impl->settings->chatHistoryLastSyncDate(QString(groupId));
        const auto toJid = groupId.isValid() ? groupIdToJid(groupId) : QString();
        const auto syncQueryId =
                m_impl->xmppMamManager->retrieveArchivedMessages(toJid, {}, {}, lastSyncDate, {}, resultSetQuery);
        m_impl->historySyncQueryParams[syncQueryId] = groupId;

    } else {
        m_impl->historySyncQueryParams.erase(queryParamIt);
        m_impl->settings->setChatHistoryLastSyncDate(QString(groupId));
    }
}

void Self::onSendMessageStatusDisplayed(const MessageHandler &message)
{
    Q_ASSERT(message->isIncoming());

    if (message->stageString() == IncomingMessageStageToString(IncomingMessageStage::Read)) {
        // Status already sent.
        return;
    }

    if (message->chatType() != ChatType::Personal) {
        //
        //  For now "displayed" should be send for personal messages only.
        //  Motivation:
        //      - Marker "Displayed" triggers push-notification, and should be filtered within messenger application.
        //      - This marker is not used for Group Chats as expected:
        //          - "Displayed" status is not shown per group member.
        //
        return;
    }

    QXmppMessage mark;
    mark.setType(QXmppMessage::Chat);
    mark.setTo(userIdToJid(message->senderId()));
    mark.setFrom(currentUserJid());
    mark.setId(QXmppUtils::generateStanzaUuid());
    mark.setMarkerId(message->id());
    mark.addHint(QXmppMessage::Store);
    mark.setMarker(QXmppMessage::Marker::Displayed);

    if (m_impl->xmpp->sendPacket(mark)) {
        qCDebug(lcCoreMessenger) << "Sent 'displayed' marker for message:" << message->id();
    } else {
        qCDebug(lcCoreMessenger) << "Marker 'displayed' was not sent for message:" << message->id();
    }
}

void Self::onSyncPrivateChatsHistory()
{
    qCInfo(lcCoreMessenger) << "Start loading private chats history";
    const auto lastSyncDate = m_impl->settings->chatHistoryLastSyncDate();
    const auto chatSyncQueryId = m_impl->xmppMamManager->retrieveArchivedMessages({}, {}, {}, lastSyncDate);
    m_impl->historySyncQueryParams[chatSyncQueryId] = GroupId();
}

void Self::onSyncGroupChatHistory(const GroupId &groupId)
{
    qCInfo(lcCoreMessenger) << "Start loading group chat history for group:" << groupId;
    const auto lastSyncDate = m_impl->settings->chatHistoryLastSyncDate(QString(groupId));
    const auto groupSyncQueryId =
            m_impl->xmppMamManager->retrieveArchivedMessages(groupIdToJid(groupId), {}, {}, lastSyncDate);
    m_impl->historySyncQueryParams[groupSyncQueryId] = groupId;
}
