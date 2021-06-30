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
#   Build platform dependent implementations as libraries.
#
#   May depends on targets:
#       * Qt5::*
#       * platform-definitions
#       * customer
#
#   Create targets:
#       * platform-api     - library that declares API for generic platform operations.
#       * platform-base    - base implementation of platform-api.
#       * platform         - platform specific implementation of platform-api.
#       * platform-fs-api  - library that declares API for specific File System operations.
#       * platform-fs-base - base implementation of platform-fs-api.
#       * platform-fs      - platform specific implementation of platform-fs-api.
#       * platform-gui-api - library that declares API for platform UI interactions.
#       * platform-gui      - platform specific implementation of platform-gui-api.
#
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

#
#   Create Platform API targets.
#
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/api")

#
#   Give a chance to crate platform dependent libraries.
#
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/${VS_PLATFORM}")

#
#   Use default implementation if missed platform dependent implementations.
#
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/default")

if(COMMAND add_clangformat AND ENABLE_CLANGFORMAT)
    add_clangformat(platform-base)
    add_clangformat(platform)
    add_clangformat(platform-fs-base)
    add_clangformat(platform-fs)
    add_clangformat(platform-notifications)
    add_clangformat(platform-updates)
    add_clangformat(platform-gui-api)
    add_clangformat(platform-gui)
endif()

