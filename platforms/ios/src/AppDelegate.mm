#import "UIKit/UIKit.h"

#import <Intents/Intents.h>
#import <UserNotifications/UserNotifications.h>

#include "PlatformNotifications.h"

#include <QDebug>

using namespace vm::platform;

@interface NSData (HexString)
- (NSString*)hexEncodedString;
@end

@implementation NSData (HexString)
- (NSString*)hexEncodedString
{
    const unsigned char* dataBuffer = (const unsigned char*)[self bytes];

    if (!dataBuffer)
        return [NSString string];

    NSUInteger dataLength = self.length;
    NSMutableString* hexString = [NSMutableString stringWithCapacity:(2 * dataLength)];

    for (NSUInteger i = 0; i < dataLength; ++i)
        [hexString appendString:[NSString stringWithFormat:@"%02lx", (unsigned long)dataBuffer[i]]];

    return [NSString stringWithString:hexString];
}
@end

@interface QIOSApplicationDelegate
@end

@interface QIOSApplicationDelegate (VirgilMessengerApplicationDelegate)
@end

@implementation QIOSApplicationDelegate (VirgilMessengerApplicationDelegate)

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    (void)application;
    (void)launchOptions;

    [self registerRemoteNotificationsForAppication:application];

    return YES;
}

- (void)registerRemoteNotificationsForAppication:(UIApplication*)application
{

    auto center = UNUserNotificationCenter.currentNotificationCenter;

    [center getNotificationSettingsWithCompletionHandler:^(UNNotificationSettings* settings) {
        if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
            [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert | UNAuthorizationOptionBadge
                    | UNAuthorizationOptionSound
                                  completionHandler:^(BOOL granted, NSError* __nullable error) {
                                      qDebug() << QString("User allowed notifications: %1").arg(granted);

                                      if (granted) {
                                          dispatch_async(dispatch_get_main_queue(),
                                              ^{ [application registerForRemoteNotifications]; });
                                      }

                                      if (error != nil) {
                                          qWarning() << "Granting notifications permissions failed: "
                                                     << error.localizedDescription;
                                      }
                                  }];

        } else if (settings.authorizationStatus == UNAuthorizationStatusAuthorized) {
            dispatch_async(dispatch_get_main_queue(), ^{ [application registerForRemoteNotifications]; });
        }
    }];
}

- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken
{
    QString tokenStr = [deviceToken hexEncodedString].UTF8String;

    qDebug() << "Received device push token: " << tokenStr;

    PlatformNotifications::instance().updatePushToken(tokenStr);
}

- (void)application:(UIApplication*)application
    didFailToRegisterForRemoteNotificationsWithError:(NSError* _Nullable)error
{
    qWarning() << "Failed to get device token";

    PlatformNotifications::instance().updatePushToken({});
}

@end
