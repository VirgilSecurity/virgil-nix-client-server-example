#   Copyright (C) 2015-2020 Virgil Security Inc.
#
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#       (1) Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#       (2) Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#       (3) Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
#   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
#   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
#   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
#   IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#   POSSIBILITY OF SUCH DAMAGE.
#
#   Lead Maintainer: Virgil Security Inc. <support@virgilsecurity.com>

if(XCODE_VERSION)
    cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
else()
    cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
endif()

# ---------------------------------------------------------------------------
# Define platform toolchain.
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/cmake/vsplatforms.cmake")

# ---------------------------------------------------------------------------
# Define project.
# ---------------------------------------------------------------------------
project(messenger VERSION "3.11.1" LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Configuration.
# ---------------------------------------------------------------------------
set(VS_CORE_VERSION "0.2.1.94")
set(VS_VERSION_DATABASE_SCHEME "3")

# ---------------------------------------------------------------------------
# Build options.
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "" FORCE)

# ---------------------------------------------------------------------------
# Configurable build options.
# ---------------------------------------------------------------------------
set(ENABLE_CLANGFORMAT ON CACHE BOOL "On/Off formatting source code with clangformat.")
set(ENABLE_XMPP_LOGS OFF CACHE BOOL "On/Off noisy XMPP logs.")
set(ENABLE_XMPP_EXTRA_LOGS OFF CACHE BOOL "On/Off QXMPP logs.")

# ---------------------------------------------------------------------------
# Include Cmake helpers
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(FetchContent)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/clangformat.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/android_version.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/getprebuild.cmake")

# ---------------------------------------------------------------------------
#   Customers selection
# ---------------------------------------------------------------------------
set(VS_CUSTOMER "Virgil" CACHE STRING "Customer for build")
set(VS_CUSTOMER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/customers/${VS_CUSTOMER}")
message(STATUS "Current customer : [${VS_CUSTOMER}]")
message(STATUS "Current customer directory: [${VS_CUSTOMER_DIR}]")
include("${VS_CUSTOMER_DIR}/customer.cmake")

# ---------------------------------------------------------------------------
# Define platform specific settings.
# ---------------------------------------------------------------------------
if(XCODE_VERSION)
    #
    #   Available from CMake 3.19
    #
    set(CMAKE_XCODE_LINK_BUILD_PHASE_MODE BUILT_ONLY)
endif()

if(VS_PLATFORM STREQUAL "macos")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "${VS_BUNDLE_PREFIX}.${VS_TARGET_NAME}")

elseif(VS_PLATFORM STREQUAL "ios")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.0")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
    set(MACOSX_BUNDLE_BUNDLE_NAME "${VS_BUNDLE_PREFIX}.${VS_TARGET_NAME}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "${VS_BUNDLE_PREFIX}.${VS_TARGET_NAME}")

elseif(VS_PLATFORM STREQUAL "android")
    #
    #   Next variables must be set before: find_package(Qt5 ...)
    #
    set_android_version(ANDROID_VERSION_NAME ANDROID_VERSION_CODE "${PROJECT_VERSION}")
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/platforms/android/gradle")
    set(QT_ANDROID_APPLICATION_BINARY ${VS_TARGET_NAME})

    message(STATUS "Android version name: [${ANDROID_VERSION_NAME}]")
    message(STATUS "Android version code: [${ANDROID_VERSION_CODE}]")
    message(STATUS "Android package source directory: [${ANDROID_PACKAGE_SOURCE_DIR}]")
endif()

# ---------------------------------------------------------------------------
# Get prebuild core libraries
# ---------------------------------------------------------------------------
get_prebuild("${CMAKE_CURRENT_LIST_DIR}/ext")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
message("-- Build type: [${CMAKE_BUILD_TYPE}]")

set(CMAKE_BUILD_TYPE_LOWER "release")

set(PREBUILT_DIR "${CMAKE_CURRENT_LIST_DIR}/ext/prebuilt")
set(PREBUILT_QT_DIR "${PREBUILT_DIR}/qt")

if(VS_IOS_SIMULATOR AND VS_PLATFORM STREQUAL "ios")
    set(VS_IOS_SIM_PREFIX "-sim")
endif()

if(VS_PLATFORM STREQUAL "android")
    set(PREBUILT_BASE_DIR "${PREBUILT_DIR}/${VS_PLATFORM}.${ANDROID_ABI}/${CMAKE_BUILD_TYPE_LOWER}/installed/usr/local")
else ()
    set(PREBUILT_BASE_DIR "${PREBUILT_DIR}/${VS_PLATFORM}${VS_IOS_SIM_PREFIX}/${CMAKE_BUILD_TYPE_LOWER}/installed/usr/local")
endif()

set(PREBUILT_INCLUDE_DIR "${PREBUILT_BASE_DIR}/include")
set(PREBUILT_LIB_DIR "${PREBUILT_BASE_DIR}/lib")
set(QUICKFUTURE_DIR "${CMAKE_CURRENT_LIST_DIR}/ext/quickfuture/src")

message(STATUS "Setting prebuild QT directory:      [${PREBUILT_QT_DIR}]")
message(STATUS "Setting prebuild directory:         [${PREBUILT_BASE_DIR}]")
message(STATUS "Setting prebuild include directory: [${PREBUILT_INCLUDE_DIR}]")
message(STATUS "Setting prebuild library directory: [${PREBUILT_LIB_DIR}]")

# ---------------------------------------------------------------------------
#   Common libraries that is used by different targets.
# ---------------------------------------------------------------------------
find_package(Qt5 COMPONENTS Core Quick Sql Network Concurrent Qml Xml REQUIRED)

#
#   CommKit
#
add_library(comm-kit INTERFACE)

target_include_directories(comm-kit
        SYSTEM INTERFACE
        $<BUILD_INTERFACE:${PREBUILT_INCLUDE_DIR}>
        )

target_link_libraries(comm-kit
        INTERFACE
        vsc_comm_kit
        vsc_comm_kit_pb
        vsc_brainkey_sdk
        vsc_keyknox_sdk
        vsc_core_sdk
        vsc_foundation
        vsc_foundation_pb
        vsc_common
        protobuf-nanopb
        mbedcrypto
        ed25519
        json-c
        )

target_link_directories(comm-kit
        INTERFACE
        "$<BUILD_INTERFACE:${PREBUILT_LIB_DIR}>"
        )

#
#   Platform specific
#
add_library(platform-definitions INTERFACE)
target_compile_definitions(platform-definitions
        INTERFACE
        $<BUILD_INTERFACE:VERSION=${PROJECT_VERSION}>

        $<BUILD_INTERFACE:VS_MSGR_ENV_DEV=$<STREQUAL:"${VS_ENV}","dev">>
        $<BUILD_INTERFACE:VS_MSGR_ENV_STG=$<STREQUAL:"${VS_ENV}","stg">>
        $<BUILD_INTERFACE:VS_MSGR_ENV_PROD=$<STREQUAL:"${VS_ENV}","prod">>

        $<BUILD_INTERFACE:VS_ANDROID=$<STREQUAL:"${VS_PLATFORM}","android">>
        $<BUILD_INTERFACE:VS_IOS=$<STREQUAL:"${VS_PLATFORM}","ios">>
        $<BUILD_INTERFACE:VS_IOS_SIMULATOR=$<BOOL:${VS_IOS_SIMULATOR}>>
        $<BUILD_INTERFACE:VS_MACOS=$<STREQUAL:"${VS_PLATFORM}","macos">>
        $<BUILD_INTERFACE:VS_LINUX=$<STREQUAL:"${VS_PLATFORM}","linux">>
        $<BUILD_INTERFACE:VS_WINDOWS=$<STREQUAL:"${VS_PLATFORM}","windows">>

        $<BUILD_INTERFACE:$<$<CONFIG:Release>:QT_NO_DEBUG>>
        $<BUILD_INTERFACE:$<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>>
        $<BUILD_INTERFACE:$<$<CONFIG:Debug>:QT_QML_DEBUG>>
        $<BUILD_INTERFACE:$<$<CONFIG:RelWithDebInfo>:QT_QML_DEBUG>>
        )

#
#   Customers.
#
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/include/VSQCustomer.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/VSQCustomer.h"
        )

add_library(customer STATIC)

target_sources(customer
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/CustomerEnv.h"
        "${CMAKE_CURRENT_BINARY_DIR}/include/VSQCustomer.h"

        "${CMAKE_CURRENT_LIST_DIR}/src/CustomerEnv.cpp"
        )

target_include_directories(customer
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        )

target_link_libraries(customer
        PUBLIC
        Qt5::Core

        PRIVATE
        platform-definitions
        )

#
#   All platform dependent implementations.
#
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/platforms")

#
#   Logging
#
add_library(logging STATIC)

target_sources(logging
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/logging/LogConfig.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/logging/LogContext.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/logging/Logging.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/logging/LogWorker.h"

        "${CMAKE_CURRENT_LIST_DIR}/src/logging/LogConfig.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/logging/Logging.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/logging/LogWorker.cpp"
        )

target_include_directories(logging
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/logging>"
        )

target_link_libraries(logging
        PUBLIC
        Qt::Core
        platform

        PRIVATE
        platform-definitions
        )


#
#   Helpers
#
add_library(helpers STATIC)

target_sources(helpers
        PRIVATE
        #
        #   Includes
        #
        "${CMAKE_CURRENT_LIST_DIR}/include/helpers/FileUtils.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/helpers/FormatUtils.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/helpers/UidUtils.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/helpers/FutureWorker.h"

        #
        #   Sources
        #
        "${CMAKE_CURRENT_LIST_DIR}/src/helpers/FileUtils.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/helpers/FormatUtils.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/helpers/UidUtils.cpp"
        )

target_include_directories(helpers
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/helpers>"
        )

target_link_libraries(helpers
        PUBLIC
        Qt::Core

        logging
        platform-fs

        PRIVATE
        platform-definitions
        )

#
#   Application settings.
#
add_library(settings STATIC)

target_sources(settings
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/settings/Settings.h"
        "${CMAKE_CURRENT_LIST_DIR}/src/settings/Settings.cpp"
        )

target_include_directories(settings
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/settings>"
        )


target_link_libraries(settings
        PUBLIC
        Qt5::Core

        customer
        helpers
        platform
        platform-fs

        PRIVATE
        platform-definitions
        )

#
#   Core Messenger
#
add_library(core-messenger STATIC)

target_sources(core-messenger
        PRIVATE
        #
        #   Includes
        #
        "${CMAKE_CURRENT_LIST_DIR}/include/CloudFileMember.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/AttachmentId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/Chat.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/ChatId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/ChatType.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFileId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFsFileId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFsFileInfo.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFsFolder.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFsFolderId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFsFolderInfo.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFsNewFile.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CloudFsSharedGroupId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CommKitBridge.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/Contact.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/ContactUpdate.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CoreMessenger.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/CoreMessengerCloudFs.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/Group.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/GroupAffiliation.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/GroupId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/GroupInvitationStatus.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/GroupMember.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/GroupUpdate.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/IncomingMessage.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/IncomingMessageStage.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/Message.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContent.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentAttachment.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentDownloadStage.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentEncrypted.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentFile.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentGroupInvitation.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentJsonUtils.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentPicture.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentText.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentType.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentUploadStage.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageGroupChatInfo.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageRequest.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageSender.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageStatus.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageUpdate.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageUpdateable.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/OutgoingMessage.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/OutgoingMessageStage.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/User.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/UserId.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/UserImpl.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppContactManager.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppDiscoveryManager.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppLastActivityIq.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppLastActivityManager.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppMucSubIq.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppMucSubManager.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppPushNotifications.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp/XmppRoomParticipantsManager.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/network/NetworkAnalyzer.h"

        #
        #   Sources
        #
        "${CMAKE_CURRENT_LIST_DIR}/src/CloudFileMember.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/AttachmentId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/Chat.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/ChatId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/ChatType.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFileId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFsFileId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFsFileInfo.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFsFolder.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFsFolderId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFsFolderInfo.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFsNewFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CloudFsSharedGroupId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CommKitBridge.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/Contact.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/ContactUpdate.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CoreMessenger.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/CoreMessengerCloudFs.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/Group.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/GroupAffiliation.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/GroupId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/GroupInvitationStatus.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/GroupMember.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/GroupUpdate.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/IncomingMessage.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/IncomingMessageStage.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/Message.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentAttachment.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentDownloadStage.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentEncrypted.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentFile.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentGroupInvitation.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentJsonUtils.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentPicture.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentText.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentType.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentUploadStage.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageGroupChatInfo.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageRequest.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageStatus.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageUpdate.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/OutgoingMessage.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/OutgoingMessageStage.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/User.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/UserId.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppContactManager.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppDiscoveryManager.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppLastActivityIq.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppLastActivityManager.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppMucSubIq.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppMucSubManager.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppPushNotifications.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/xmpp/XmppRoomParticipantsManager.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/network/NetworkAnalyzer.cpp"
        )

target_include_directories(core-messenger
        SYSTEM PUBLIC
        "$<BUILD_INTERFACE:${PREBUILT_INCLUDE_DIR}>"
        "$<BUILD_INTERFACE:${PREBUILT_INCLUDE_DIR}/qxmpp>"
        )

target_include_directories(core-messenger
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/messenger>"

        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/notifications>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/messenger/xmpp>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/messenger/network>"
        )

target_link_libraries(core-messenger
    PUBLIC
        settings

        qxmpp
        comm-kit
        helpers
        settings
        logging
        platform-definitions

        Qt5::Core
        Qt5::Concurrent
        Qt5::Network
        Qt5::Xml
        )

target_compile_definitions(core-messenger
        PUBLIC
        $<BUILD_INTERFACE:ENABLE_XMPP_LOGS=$<BOOL:${ENABLE_XMPP_LOGS}>>
        $<BUILD_INTERFACE:ENABLE_XMPP_EXTRA_LOGS=$<BOOL:${ENABLE_XMPP_EXTRA_LOGS}>>
        )

#
#   Core Messenger GUI
#
add_library(core-messenger-gui STATIC)

target_sources(core-messenger-gui
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/Utils.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/messenger/MessageContentPictureFactory.h"

        "${CMAKE_CURRENT_LIST_DIR}/src/Utils.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/messenger/MessageContentPictureFactory.cpp"
        )

target_include_directories(core-messenger
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/messenger>"
        )

target_link_libraries(core-messenger-gui
        PUBLIC
        Qt5::Gui
        core-messenger
        )

#
#   Format
#
if(COMMAND add_clangformat AND ENABLE_CLANGFORMAT)
    add_clangformat(logging)
    add_clangformat(helpers)
    add_clangformat(settings)
    add_clangformat(core-messenger)
    add_clangformat(core-messenger-gui)
endif()

# ---------------------------------------------------------------------------
#   Target sources and libraries
# ---------------------------------------------------------------------------
if(NOT TARGET ${VS_TARGET_NAME})
    message(FATAL_ERROR "Unknown target platform")
endif()

target_sources(${VS_TARGET_NAME}
        PRIVATE
        #
        #  Includes
        #
        ${CMAKE_CURRENT_LIST_DIR}/include/JsonUtils.h
        ${CMAKE_CURRENT_LIST_DIR}/include/VSQApplication.h
        ${CMAKE_CURRENT_LIST_DIR}/include/VSQClipboardProxy.h
        ${CMAKE_CURRENT_LIST_DIR}/include/CrashReporter.h
        ${CMAKE_CURRENT_LIST_DIR}/include/FileLoader.h
        ${CMAKE_CURRENT_LIST_DIR}/include/Messenger.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ui/VSQUiHelper.h
        ${CMAKE_CURRENT_LIST_DIR}/include/KeyboardEventFilter.h
        ${CMAKE_CURRENT_LIST_DIR}/include/Validator.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ContactAvatarLoader.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ConfirmationCodeType.h
        ${CMAKE_CURRENT_LIST_DIR}/include/CloudFile.h
        ${CMAKE_CURRENT_LIST_DIR}/include/CloudFileRequestId.h
        ${CMAKE_CURRENT_LIST_DIR}/include/CloudFileSystem.h
        ${CMAKE_CURRENT_LIST_DIR}/include/CloudFileUpdateSource.h
        ${CMAKE_CURRENT_LIST_DIR}/include/CloudFilesUpdate.h
        ${CMAKE_CURRENT_LIST_DIR}/include/TimeProfiler.h
        ${CMAKE_CURRENT_LIST_DIR}/include/TimeProfilerSection.h
        # Controllers
        ${CMAKE_CURRENT_LIST_DIR}/include/controllers/Controller.h
        ${CMAKE_CURRENT_LIST_DIR}/include/controllers/AttachmentsController.h
        ${CMAKE_CURRENT_LIST_DIR}/include/controllers/ChatsController.h
        ${CMAKE_CURRENT_LIST_DIR}/include/controllers/CloudFilesController.h
        ${CMAKE_CURRENT_LIST_DIR}/include/controllers/Controllers.h
        ${CMAKE_CURRENT_LIST_DIR}/include/controllers/MessagesController.h
        ${CMAKE_CURRENT_LIST_DIR}/include/controllers/UsersController.h
        # Applications states
        ${CMAKE_CURRENT_LIST_DIR}/include/states/ApplicationStateManager.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/BackupKeyState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/CloudFileListState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/CloudFolderMembersState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/EditChatInfoState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/EditProfileState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/ChatState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/DownloadKeyState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/GroupChatMembersState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/OperationState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/NewChatState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/RestoreAccountUsernameState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/SelectContactsState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/SignUpState.h
        ${CMAKE_CURRENT_LIST_DIR}/include/states/VerifyProfileState.h
        # Database
        ${CMAKE_CURRENT_LIST_DIR}/include/database/core/Database.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/core/DatabaseTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/core/DatabaseUtils.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/core/Migration.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/core/Patch.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/core/ScopedConnection.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/core/ScopedTransaction.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/AttachmentsTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/ContactsTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/ChatsTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/CloudFilesTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/GroupMembersTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/GroupsTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/MessagesTable.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/UserDatabase.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/UserDatabaseMigration.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/patches/version1/PatchContacts.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/patches/version2/PatchCloudFiles.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/patches/version3/PatchChats.h
        ${CMAKE_CURRENT_LIST_DIR}/include/database/patches/version3/PatchGroups.h
        # Models
        ${CMAKE_CURRENT_LIST_DIR}/include/models/AccountSelectionModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/ChatObject.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/ChatsModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFileObject.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFilePropertiesModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFileMembersModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFileOperationSource.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFilesModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFilesTransfersModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFilesQueue.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/CloudFilesQueueListeners.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/ContactsModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/ContactsProxyModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/DiscoveredContactsModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/DiscoveredContactsProxyModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/GroupMembersModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/ListModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/ListProxyModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/ListSelectionModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/MessageOperationSource.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/MessagesModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/MessagesProxyModel.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/MessagesQueue.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/MessagesQueueListeners.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/Models.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/Model.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/OperationQueue.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/OperationQueueListener.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/OperationSource.h
        ${CMAKE_CURRENT_LIST_DIR}/include/models/TransfersModel.h
        # Operations
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/CalculateAttachmentFingerprintOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/CalculateFileFingerprintOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/CloudFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/ConvertImageFormatOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/CreateAttachmentPreviewOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/CreateAttachmentThumbnailOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/CreateCloudFolderOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/CreateThumbnailOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/DecryptFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/DeleteCloudFilesOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/DownloadAttachmentOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/DownloadCloudFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/DownloadFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/DownloadDecryptFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/EncryptFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/EncryptUploadFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/ListCloudFolderOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/ListMembersCloudFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/LoadAttachmentOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/LoadFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/MessageOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/MessageOperationFactory.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/NetworkOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/Operation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/SendMessageOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/SetMembersCloudFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/UploadAttachmentOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/UploadFileOperation.h
        ${CMAKE_CURRENT_LIST_DIR}/include/operations/UploadCloudFileOperation.h

        # Android
        # TODO: Hide this under platform API.
        $<$<STREQUAL:"${VS_PLATFORM}","android">:${CMAKE_CURRENT_LIST_DIR}/platforms/android/native/include/VSQAndroid.h>

        #
        #  Sources
        #
        ${CMAKE_CURRENT_LIST_DIR}/src/VSQClipboardProxy.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/CrashReporter.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/FileLoader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Messenger.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/VSQApplication.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ui/VSQUiHelper.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/KeyboardEventFilter.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/Validator.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/ContactAvatarLoader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/CustomerEnv.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/CloudFile.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/CloudFileSystem.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/TimeProfiler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/TimeProfilerSection.cpp
        # Controllers
        ${CMAKE_CURRENT_LIST_DIR}/src/controllers/Controller.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/controllers/AttachmentsController.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/controllers/ChatsController.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/controllers/CloudFilesController.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/controllers/Controllers.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/controllers/MessagesController.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/controllers/UsersController.cpp
        # Applications states
        ${CMAKE_CURRENT_LIST_DIR}/src/states/ApplicationStateManager.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/BackupKeyState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/EditChatInfoState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/EditProfileState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/GroupChatMembersState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/ChatState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/CloudFileListState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/CloudFolderMembersState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/DownloadKeyState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/NewChatState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/RestoreAccountUsernameState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/SelectContactsState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/SignUpState.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/states/VerifyProfileState.cpp
        # Database
        ${CMAKE_CURRENT_LIST_DIR}/src/database/core/Database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/core/DatabaseTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/core/DatabaseUtils.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/core/Migration.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/core/Patch.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/core/ScopedConnection.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/core/ScopedTransaction.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/AttachmentsTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/ContactsTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/ChatsTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/CloudFilesTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/GroupMembersTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/GroupsTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/MessagesTable.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/UserDatabase.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/UserDatabaseMigration.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/patches/version1/PatchContacts.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/patches/version2/PatchCloudFiles.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/patches/version3/PatchChats.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/database/patches/version3/PatchGroups.cpp
        # Models
        ${CMAKE_CURRENT_LIST_DIR}/src/models/AccountSelectionModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/ChatObject.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/ChatsModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFileMembersModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFileObject.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFilePropertiesModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFileOperationSource.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFilesModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFilesTransfersModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFilesQueue.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/CloudFilesQueueListeners.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/ContactsModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/ContactsProxyModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/DiscoveredContactsModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/DiscoveredContactsProxyModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/GroupMembersModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/TransfersModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/ListModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/ListProxyModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/ListSelectionModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/MessageOperationSource.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/MessagesModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/MessagesProxyModel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/MessagesQueue.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/MessagesQueueListeners.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/Models.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/Model.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/models/OperationQueue.cpp
        # Operations
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/CalculateAttachmentFingerprintOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/CalculateFileFingerprintOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/CloudFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/ConvertImageFormatOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/CreateAttachmentPreviewOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/CreateAttachmentThumbnailOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/CreateCloudFolderOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/CreateThumbnailOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/DecryptFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/DeleteCloudFilesOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/DownloadAttachmentOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/DownloadCloudFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/DownloadFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/DownloadDecryptFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/EncryptFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/EncryptUploadFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/ListCloudFolderOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/ListMembersCloudFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/LoadAttachmentOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/LoadFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/MessageOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/MessageOperationFactory.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/NetworkOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/Operation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/SendMessageOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/SetMembersCloudFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/UploadAttachmentOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/UploadFileOperation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/operations/UploadCloudFileOperation.cpp
        # QT QRC
        ${CMAKE_CURRENT_LIST_DIR}/src/resources.qrc
        ${VS_CUSTOMER_DIR}/src/customer.qrc
        # Customer sources
        ${VS_CUSTOMER_SOURCES}
        # Android
        # TODO: Hide this under platform API.
        $<$<STREQUAL:"${VS_PLATFORM}","android">:${CMAKE_CURRENT_LIST_DIR}/platforms/android/native/src/VSQAndroid.cpp>
        )

target_link_libraries(${VS_TARGET_NAME}
        PRIVATE
        comm-kit
        core-messenger
        core-messenger-gui
        customer
        platform
        platform-definitions
        platform-fs
        platform-notifications
        platform-updates
        platform-gui
        settings

        Qt5::Core
        Qt5::Quick
        Qt5::Sql
        Qt5::Concurrent
        Qt5::Qml

        platform-deps # must be the last
        )

#
#   Common include directories
#
target_include_directories(${VS_TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/database>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/models>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/logging>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/controllers>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/states>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/operations>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/notifications>

        # TODO: Remove it when VSQAndroid will be hidden under platform API.
        $<$<STREQUAL:"${VS_PLATFORM}","android">:${CMAKE_CURRENT_LIST_DIR}/platforms/android/native/include>

        $<BUILD_INTERFACE:${VS_CUSTOMER_INCLUDE_DIR}>
        )

target_compile_definitions(${VS_TARGET_NAME}
        PRIVATE
        $<BUILD_INTERFACE:VS_DESKTOP=$<IN_LIST:${VS_PLATFORM},${VS_DESKTOP_PLATFORM_LIST}>>
        $<BUILD_INTERFACE:VS_MOBILE=$<IN_LIST:${VS_PLATFORM},${VS_MOBILE_PLATFORM_LIST}>>

        $<BUILD_INTERFACE:INFO_CLIENT=1>
        $<BUILD_INTERFACE:CFG_CLIENT=1>
        $<BUILD_INTERFACE:VERSION_DATABASE_SCHEME=${VS_VERSION_DATABASE_SCHEME}>
        $<BUILD_INTERFACE:VS_CUSTOMER=${VS_CUSTOMER}>
        )

target_compile_options(${VS_TARGET_NAME}
        INTERFACE
        -Wno-multichar
        )

if(COMMAND add_clangformat AND ENABLE_CLANGFORMAT)
    add_clangformat(${VS_TARGET_NAME})
endif()
