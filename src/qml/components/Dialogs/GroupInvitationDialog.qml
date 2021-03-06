import QtQuick 2.15
import QtQuick.Controls 2.15

import "../" // components
import "../../theme"

Rectangle {
    id: root
    color: Theme.mainBackgroundColor
    width: parent.width
    height: column.height + 2 * d.margin
    visible: false

    signal accepted()
    signal rejected()

    QtObject {
        id: d
        readonly property real margin: Theme.smallMargin
        property string groupTitle: ""
        property string helloText: ""
        property var message: null
    }

    Column {
        id: column
        y: d.margin
        anchors {
            left: parent.left
            right: parent.right
            margins: d.margin
        }
        spacing: Theme.spacing

        Text {
            id: titleLabel
            width: parent.width
            color: Theme.primaryTextColor
            font.pointSize: UiHelper.fixFontSz(17)
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            text: d.groupTitle
        }

        Text {
            id: textLabel
            width: parent.width
            color: Theme.primaryTextColor
            font.pointSize: UiHelper.fixFontSz(12)
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            text: d.helloText
            visible: text
        }

        Row {
            id: row
            y: Theme.margin
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Theme.spacing

            FormSecondaryButton {
                text: qsTr("Reject")
                onClicked: {
                    controllers.chats.rejectGroupInvitation(d.message)
                    root.visible = false
                }
            }

            FormPrimaryButton {
                text: qsTr("Accept")
                onClicked: {
                    controllers.chats.acceptGroupInvitation(d.message)
                    root.visible = false
                }
            }
        }
    }

    function open(ownerUsername, message) {
        d.groupTitle = qsTr("%1 invites you to the group").arg(ownerUsername)
        d.message = message
        root.visible = true
    }

    Component.onCompleted: {
        models.messages.groupInvitationReceived.connect(open)
    }

    Component.onDestruction: {
        models.messages.groupInvitationReceived.disconnect(open)
    }

    Connections {
        target: models.messages

        function onMessagesReset() { root.visible = false }
    }
}
