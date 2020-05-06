import QtQuick 2.12
import QtQuick.Controls 2.12

import "../js/login.js" as LoginLogic

StackView {
    focus: true
    anchors.fill: parent

    initialItem: Qt.createComponent("../pages/AuthenticationPage.qml")

    Keys.onReleased: {
        if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
            if (depth > 1) {
                event.accepted = true
                back()
            }
        }
    }

    property var chatLayout

    function back() {
      pop()
    }

    function showAuthentication() {
        replace(Qt.createComponent("../pages/AuthenticationPage.qml"))
    }

    function showLogin() {
        push(Qt.createComponent("./pages/LoginPage.qml"))
    }

    function showRegister() {
        push(Qt.createComponent("../pages/RegisterPage.qml"))
    }

    function signIn(user) {
        if (LoginLogic.validateUser(user)) {
            if (!chatLayout) {
                chatLayout = Qt.createComponent("./ChatView.qml").createObject(parent)
            }
            Messenger.signIn(user)
            showPopupInform("Sign In ...")
            replace(chatLayout)
        } else {
            root.showPopupError(qsTr("Incorrect user name"))
        }
    }

    // Sign up
    function signUp(user) {
        if (LoginLogic.validateUser(user)) {
            if (!chatLayout) {
                chatLayout = Qt.createComponent("./ChatView.qml").createObject(parent)
            }
            Messenger.signUp(user)
            showPopupInform("Sign Up ...")
            replace(chatLayout)
        } else {
            showPopupError(qsTr("Incorrect user name"))
        }
    }

    function logout() {
        Messenger.logout()

        showAuthentication()
    }

    function showUserSettings() {
        chatLayout.showUserSettings()
    }
}