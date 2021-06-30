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

#
#   Library: platform-deps.
#
if (NOT TARGET platform-deps)
    add_library(platform-deps INTERFACE)
endif()

#
#   Library: platform.
#
if (NOT TARGET platform)
    add_library(platform STATIC)

    target_sources(platform
            PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include/PlatformDefault.h"
            "${CMAKE_CURRENT_LIST_DIR}/src/PlatformDefault.cpp"
            )

    target_include_directories(platform
            PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            )

    target_link_libraries(platform
            PUBLIC
            platform-base
            )
endif()

#
#   Library: platform-fs.
#
if (NOT TARGET platform-fs)
    add_library(platform-fs STATIC)

    target_sources(platform-fs
            PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include/PlatformFsDefault.h"
            "${CMAKE_CURRENT_LIST_DIR}/src/PlatformFsDefault.cpp"
            )

    target_include_directories(platform-fs
            PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            )

    target_link_libraries(platform-fs
            PUBLIC
            platform-fs-base
            )
endif()

#
#   Library: platform-notifications.
#
if (NOT TARGET platform-notifications)
    add_library(platform-notifications STATIC)

    target_sources(platform-notifications
            PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include/PlatformNotificationsDefault.h"
            "${CMAKE_CURRENT_LIST_DIR}/src/PlatformNotificationsDefault.cpp"
            )

    target_include_directories(platform-notifications
            PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            )

    target_link_libraries(platform-notifications
            PUBLIC
            platform-notifications-api
            )
endif()

#
#   Library: platform-updates.
#
if (NOT TARGET platform-updates)
    add_library(platform-updates STATIC)

    target_sources(platform-updates
            PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include/PlatformUpdatesDefault.h"
            "${CMAKE_CURRENT_LIST_DIR}/src/PlatformUpdatesDefault.cpp"
            )

    target_include_directories(platform-updates
            PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            )

    target_link_libraries(platform-updates
            PUBLIC
            platform-updates-api
            )
endif()

#
#   Library: platform-gui.
#
if (NOT TARGET platform-gui)
    add_library(platform-gui STATIC)

    target_sources(platform-gui
            PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/include/PlatformDocumentInteractionControllerDefault.h"
            "${CMAKE_CURRENT_LIST_DIR}/src/PlatformDocumentInteractionControllerDefault.cpp"
            )

    target_include_directories(platform-gui
            PRIVATE
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
            )

    target_link_libraries(platform-gui
            PUBLIC
            Qt5::Gui
            platform-gui-api
            )
endif()
