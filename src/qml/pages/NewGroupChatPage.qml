import QtQuick 2.15

import "../components"

SelectContactsPage {
    appState: app.stateManager.newGroupChatState

    header: NewGroupChatHeader {
        id: newGroupChatHeader
        title: qsTr("Add members")
        description: selectedContacts.count > 0 ? qsTr("%1 selected".arg(selectedContacts.count)) : ""
        rightButtonEnabled: selectedContacts.count > 0
        onActionButtonClicked: appState.requestChatName()
    }
}
