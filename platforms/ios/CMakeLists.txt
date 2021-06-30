#   Copyright (C) 2015-2021 Virgil Security Inc.
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

#
#   Provide default (Qt) implementation for platform dependent operations.
#
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# ---------------------------------------------------------------------------
#   System introspection.
# ---------------------------------------------------------------------------
if(NOT XCODE_VERSION)
        message(FATAL_ERROR "Xcode only")
endif()

find_package(Qt5 COMPONENTS QmlImportScanner Gui REQUIRED)

# ---------------------------------------------------------------------------
#   Libraries
# ---------------------------------------------------------------------------
#
#   Library: platform.
#
add_library(platform STATIC)

target_sources(platform
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/PlatformIos.h"
        "${CMAKE_CURRENT_LIST_DIR}/src/PlatformIos.mm"
        )

target_link_libraries(platform
        PUBLIC
        platform-base
        customer
        )

target_include_directories(platform
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        )

#
#   Library: platform-fs.
#
add_library(platform-fs STATIC)

target_sources(platform-fs
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/PlatformFsIos.h"
        "${CMAKE_CURRENT_LIST_DIR}/src/PlatformFsIos.cpp"
        )

target_include_directories(platform-fs
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        )

target_link_libraries(platform-fs
        PUBLIC
        platform-fs-base
        )

#
#   Library: platform-gui.
#
add_library(platform-gui STATIC)

target_sources(platform-gui
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/PlatformDocumentInteractionControllerDataSourceIos.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/PlatformDocumentInteractionControllerIos.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/PlatformDocumentInteractionControllerPreviewItemIos.h"

        "${CMAKE_CURRENT_LIST_DIR}/src/PlatformDocumentInteractionControllerDataSourceIos.mm"
        "${CMAKE_CURRENT_LIST_DIR}/src/PlatformDocumentInteractionControllerIos.mm"
        "${CMAKE_CURRENT_LIST_DIR}/src/PlatformDocumentInteractionControllerPreviewItemIos.mm"
        )

target_include_directories(platform-gui
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        )

find_library(QUICK_LOOK_FRAMEWORK QuickLook REQUIRED)

target_link_libraries(platform-gui
        PUBLIC
        platform
        platform-gui-api

        ${QUICK_LOOK_FRAMEWORK}
        Qt5::Gui
        )

#
#   Library: platform-notifications.
#
add_library(platform-notifications STATIC)

target_sources(platform-notifications
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/include/PlatformNotificationsIos.h"
        "${CMAKE_CURRENT_LIST_DIR}/src/PlatformNotificationsIos.cpp"
        )

target_include_directories(platform-notifications
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
        )

target_link_libraries(platform-notifications
        PUBLIC
        platform-notifications-api
        )

# ---------------------------------------------------------------------------
#   Common build settings.
# ---------------------------------------------------------------------------
if(VS_IOS_SIMULATOR)
    set(CMAKE_XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphonesimulator")
    set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphonesimulator")
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
else ()
    set(CMAKE_XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos")
    set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()


# ---------------------------------------------------------------------------
#   Notification Extension.
# ---------------------------------------------------------------------------
set(VS_TARGET_NOTIFICATION_EXTENSION_NAME ${VS_TARGET_NAME}_NotificationExtension)

#
#   Configuration.
#
add_executable(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} MACOSX_BUNDLE)

set_target_properties(${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
        PROPERTIES
        BUNDLE YES
        BUNDLE_EXTENSION appex
        XCODE_ATTRIBUTE_WRAPPER_EXTENSION appex
        XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY YES
        XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
        XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
        XCODE_EXPLICIT_FILE_TYPE "wrapper.app-extension"
        )

set_target_properties(${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
        PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/NotificationExtensionInfo.plist.in"
        )

#
#   Sources.
#
target_sources(${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/NotificationService.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/NotificationService.mm"
        )

target_include_directories(${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        )

#
#   Dependencies.
#
find_library(USER_NOTIFICATIONS_LIB UserNotifications)
find_library(USER_NOTIFICATIONS_UI_LIB UserNotificationsUI)

target_link_libraries(${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
        PRIVATE
        ${USER_NOTIFICATIONS_LIB}
        ${USER_NOTIFICATIONS_UI_LIB}
        core-messenger
        )

#
#   Code signing.
#
if(VS_CODE_SIGN_IDENTITY)
    set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} CODE_SIGN_IDENTITY "${VS_CODE_SIGN_IDENTITY}")
endif()

if(VS_DEVELOPMENT_TEAM)
    set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} DEVELOPMENT_TEAM "${VS_DEVELOPMENT_TEAM}")
endif()

if(VS_PROVISIONING_PROFILE_SPECIFIER)
    set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
            PROVISIONING_PROFILE_SPECIFIER "${VS_PROVISIONING_PROFILE_SPECIFIER}"
            )
endif()

set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} TARGETED_DEVICE_FAMILY "1,2")
set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} ONLY_ACTIVE_ARCH NO)
set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} IPHONEOS_DEPLOYMENT_TARGET "${IOS_DEPLOYMENT_TARGET}")
set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
        CODE_SIGN_ENTITLEMENTS
        "${VS_CUSTOMER_DIR}/platforms/ios/Entitlements/VirgilMessenger_NotificationExtension.entitlements"
        )

#
#  Generate .dSYM file for all configurations and strip them from the release.
#
set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} DEPLOYMENT_POSTPROCESSING[variant=MinSizeRel] "YES")
set_xcode_property(${VS_TARGET_NOTIFICATION_EXTENSION_NAME} DEPLOYMENT_POSTPROCESSING[variant=Release] "YES")

#
#   Format.
#
if(COMMAND add_clangformat AND ENABLE_CLANGFORMAT)
    add_clangformat(${VS_TARGET_NOTIFICATION_EXTENSION_NAME})
endif()

# ---------------------------------------------------------------------------
#   Main Application.
# ---------------------------------------------------------------------------
add_executable(${VS_TARGET_NAME} MACOSX_BUNDLE)

target_sources(${VS_TARGET_NAME}
        PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/src/AppDelegate.mm"
        )

#   TODO: Remove next includes when Notifications goes to a separate library.
target_include_directories(${VS_TARGET_NAME}
        PRIVATE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        )

set_xcode_property(${VS_TARGET_NAME} TARGETED_DEVICE_FAMILY "1,2")
set_xcode_property(${VS_TARGET_NAME} ONLY_ACTIVE_ARCH NO)

#
#   Dependencies.
#
find_library(FOUNDATION_LIB Foundation)

target_link_libraries(${VS_TARGET_NAME}
        PUBLIC
        platform-gui
        Qt5::QIosOptionalPlugin_NSPhotoLibrary
        ${FOUNDATION_LIB}
        ${USER_NOTIFICATIONS_LIB}
        )

set_target_properties(${VS_TARGET_NAME}
        PROPERTIES
        LINK_FLAGS "-Wl,-e,_qt_main_wrapper"
        )

#
#   Signing.
#
if(VS_CODE_SIGN_IDENTITY)
    set_xcode_property(${VS_TARGET_NAME} CODE_SIGN_IDENTITY "${VS_CODE_SIGN_IDENTITY}")
endif()

if(VS_DEVELOPMENT_TEAM)
    set_xcode_property(${VS_TARGET_NAME} DEVELOPMENT_TEAM "${VS_DEVELOPMENT_TEAM}")
endif()

if(VS_PROVISIONING_PROFILE_SPECIFIER)
    set_xcode_property(${VS_TARGET_NAME} PROVISIONING_PROFILE_SPECIFIER "${VS_PROVISIONING_PROFILE_SPECIFIER}")
endif()

set_xcode_property(${VS_TARGET_NAME} IPHONEOS_DEPLOYMENT_TARGET "${IOS_DEPLOYMENT_TARGET}")
set_xcode_property(${VS_TARGET_NAME} CODE_SIGN_ENTITLEMENTS "${VS_CUSTOMER_DIR}/platforms/ios/Entitlements/VirgilMessenger.entitlements")
set_xcode_property(${VS_TARGET_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")

set_target_properties(${VS_TARGET_NAME}
        PROPERTIES
        XCODE_ATTRIBUTE_INSTALL_PATH "/Applications"
        XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
        )

#
#   Resources.
#
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/LaunchScreen.storyboard.in"
        "${CMAKE_BINARY_DIR}/${VS_TARGET_NAME}.xcodeproj/LaunchScreen.storyboard"
        @ONLY
        )

configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/Default-568h@2x.png"
        "${CMAKE_BINARY_DIR}/${VS_TARGET_NAME}.xcodeproj/Default-568h@2x.png"
        COPYONLY
        )

set(STORYBOARD_DEFAULT
        "${CMAKE_BINARY_DIR}/${VS_TARGET_NAME}.xcodeproj/LaunchScreen.storyboard"
        "${CMAKE_BINARY_DIR}/${VS_TARGET_NAME}.xcodeproj/Default-568h@2x.png"
        )

set(VS_ASSET_DIR "${VS_CUSTOMER_DIR}/platforms/ios/Assets.xcassets")
target_sources(${VS_TARGET_NAME}
        PRIVATE
        "${VS_ASSET_DIR}"
        "${STORYBOARD_DEFAULT}"
        )

set_source_files_properties("${VS_ASSET_DIR}"
        PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
        )

set_target_properties(${VS_TARGET_NAME}
        PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist.in"
        RESOURCE "${STORYBOARD_DEFAULT}"
        )

#
#   Scan and link to implicit dependencies.
#
qt5_import_qml_plugins(${VS_TARGET_NAME}
        PATH_TO_SCAN "${PROJECT_SOURCE_DIR}/src/qml"
        )

#
#   Copy extensions.
#   TODO: Use 'XCODE_EMBED_APP_EXTENSIONS' when cmake 3.21 will be released.
#
add_custom_command(TARGET ${VS_TARGET_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
    ARGS -E copy_directory
        "\"\${BUILT_PRODUCTS_DIR}/${VS_TARGET_NOTIFICATION_EXTENSION_NAME}.appex\""
        "\"\${BUILT_PRODUCTS_DIR}/${VS_TARGET_NAME}.app/PlugIns/${VS_TARGET_NOTIFICATION_EXTENSION_NAME}.appex\""
    MAIN_DEPENDENCY ${VS_TARGET_NOTIFICATION_EXTENSION_NAME}
    )

add_dependencies(${VS_TARGET_NAME} ${VS_TARGET_NOTIFICATION_EXTENSION_NAME})

#
#  Generate .dSYM file for all configurations and strip them from the release.
#
set_xcode_property(${VS_TARGET_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS "YES")
set_xcode_property(${VS_TARGET_NAME} DEPLOYMENT_POSTPROCESSING[variant=MinSizeRel] "YES")
set_xcode_property(${VS_TARGET_NAME} DEPLOYMENT_POSTPROCESSING[variant=Release] "YES")

# ---------------------------------------------------------------------------
#   Deploy
# ---------------------------------------------------------------------------
add_custom_target(xcarchive
        COMMENT "Building xcarchive..."

        COMMAND xcodebuild
            -project "${PROJECT_BINARY_DIR}/messenger.xcodeproj"
            -scheme ${VS_TARGET_NAME}
            -sdk iphoneos
            -destination generic/platform=iOS
            -configuration Release
            archive
            -archivePath "${CMAKE_CURRENT_BINARY_DIR}/${VS_TARGET_NAME}.xcarchive"

        COMMAND "${CMAKE_COMMAND}" -E
            tar czf "${VS_TARGET_NAME}-${PROJECT_VERSION}.tgz" -- "${CMAKE_CURRENT_BINARY_DIR}/${VS_TARGET_NAME}.xcarchive"
        )

add_custom_target(upload
        COMMENT "Upload xcarchive..."
        COMMAND xcodebuild
            -exportArchive
            -archivePath "${CMAKE_CURRENT_BINARY_DIR}/${VS_TARGET_NAME}.xcarchive"
            -exportOptionsPlist "${CMAKE_CURRENT_LIST_DIR}/exportOptions.plist"
            -allowProvisioningUpdates
        )

add_custom_target(ipa
        COMMENT "Build IPA..."
        COMMAND xcodebuild
        -exportArchive
        -archivePath "${CMAKE_CURRENT_BINARY_DIR}/${VS_TARGET_NAME}.xcarchive"
        -exportOptionsPlist "${CMAKE_CURRENT_LIST_DIR}/exportOptions-ipa.plist"
        -allowProvisioningUpdates
        -exportPath "${CMAKE_BINARY_DIR}"
        )
