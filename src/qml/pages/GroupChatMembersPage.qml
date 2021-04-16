import QtQuick 2.15

MultiSelectContactsPage {
    appState: app.stateManager.groupChatMembersState

    signal selected(var contacts)

    Component.onCompleted: appState.contactsSelected.connect(selected)
}
