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

#include "PlatformIos.h"

#include "VSQCustomer.h"

#include <Foundation/Foundation.h>
#include <QUrl>

using namespace vm;
using namespace platform;

using Self = PlatformIos;

Platform& Platform::instance() { return Self::instance(); }

Self& Self::instance()
{
    static Self impl;
    return impl;
}

static NSString* getEnvSuffix()
{
#if VS_MSGR_ENV_DEV
    return @".dev";
#elif VS_MSGR_ENV_STG
    return @".stg";
#else
    return @"";
#endif
}

QDir Self::appDataLocation() const
{
    NSString* appGroup =
        [NSString stringWithFormat:@"%@%@", Customer::kSecurityApplicationGroupIdentifier.toNSString(), getEnvSuffix()];

    NSURL* appDataLocationNative =
        [[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:appGroup];

    QUrl appDataLocation = QUrl::fromNSURL(appDataLocationNative);

    return QDir(appDataLocation.toLocalFile());
}
