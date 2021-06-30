message(STATUS "[Begin] customer.cmake")

# Set project target name and variables before define project in main Cmake file
set(VS_TARGET_NAME "VirgilMessenger")
message(STATUS "Set target name: [${VS_TARGET_NAME}]")

# -- Application variables
set(VS_ORGANIZATION_NAME "VirgilSecurity")
set(VS_ORGANIZATION_DISPLAYNAME "Virgil")

set(VS_APPLICATION_NAME "VirgilMessenger")
set(VS_APPLICATION_DISPLAY_NAME "Virgil Secure Communications Platform")

set(VS_ORGANIATION_DOMAIN "virgil.net")
set(VS_MESSANGER_URL_TEMPLATE "https://messenger%1.virgilsecurity.com")
set(VS_XMPP_URL_TEMPLATE "xmpp%1.virgilsecurity.com")

set(VS_XMPP_DOMAIN_TEMPLATE "xmpp%1.virgilsecurity.com")
set(CONTACT_DISCOVERY_URL_TEMPLATE "https://disco%1.virgilsecurity.com")
set(VS_PUSH_PROXY "push-notifications-proxy")
set(VS_PUSH_TOPIC "com.virgil.VirgilMessenger")

set(SECURITY_APPLICATION_GROUP_IDENTIFIER "group.com.virgil.VirgilMessenger")

set(VS_BUNDLE_PREFIX "com.virgil")

# -- Sources and includes
# Custom customer sources and includes
#set(VS_CUSTOMER_INCLUDE_DIR "${VS_CUSTOMER_DIR}/includes")
#set(VS_CUSTOMER_SOURCES "${VS_CUSTOMER_DIR}/src/Example.cpp")

# ----------
# Android
# ----------
# ***********************************************************************************
if(VS_PLATFORM STREQUAL "android")
    # Android package name for gradle
    set(VS_ANDROID_PACKAGE_NAME "com.virgilsecurity.android.virgil")

# ----------
# Windows
# ----------
# ***********************************************************************************
elseif(VS_PLATFORM STREQUAL "windows")
    # Windows sparkle feed URL
    set(WIN_SU_URL "https://virgilsecurity.bintray.com/messenger/windows/nightly")
    set(WIN_SU_FILE "${VS_TARGET_NAME}-installer-${PROJECT_VERSION}.exe")
    set(WIN_SU_FILE_URL "${WIN_SU_URL}/${WIN_SU_FILE}")
    set(WIN_SU_FEED_URL "${WIN_SU_URL}/appcast.xml")

# ----------
# MacOS
# ----------
# ***********************************************************************************
elseif(VS_PLATFORM STREQUAL "macos")
    # Sparkle data
    set(SU_FEED_URL "https://virgilsecurity.bintray.com/messenger/macos/nightly/appcast.xml")
    set(SU_PUBLIC_ED_KEY "44RVxRhV4h4Hlw+VOeXvCj78Z5NUhJ2Qi5N+kpu8KxI=")
    set(MACOSX_BUNDLE_ICON_FILE "MyIcon.icns")
    set(MACOSX_BUNDLE_BACKGROUND_FILE "Background.png")
endif()

message(STATUS "[End] customer.cmake")
