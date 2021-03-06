import QtQuick 2.15

import "../components"

SelectContactsPage {
    id: root
    appState: app.stateManager.newChatState

    signal created()

    header: Header {
        title: qsTr("New chat")
    }

    Component.onCompleted: {
        appState.reload()
        controllers.chats.chatCreated.connect(created)
    }
}
