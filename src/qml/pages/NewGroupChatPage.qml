import QtQuick 2.15

MultiSelectContactsPage {
    appState: app.stateManager.newGroupChatState
    onFinished: appState.requestChatName()
}
