import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "../components"
import "../theme"

FlowView {
    id: flowListView
    clip: true
    model: selectedModel.proxy

    readonly property var selectedModel: models.discoveredContacts.selectedContacts
    readonly property var editedModel: models.discoveredContacts
    readonly property real recommendedHeight: d.recommendedHeight

    signal contactSelected(string contactUsername)

    QtObject {
        id: d

        readonly property real flowItemHeight: 28
        readonly property real lineWidth: 2
        // TODO(fpohtmeh): remove usage of external variable root
        readonly property real recommendedHeight: Math.min(flowListView.contentHeight, 3 * (d.flowItemHeight + root.spacing) - root.spacing)
    }

    delegate: Rectangle {
        height: d.flowItemHeight
        width: row.width + 0.5 * radius
        color: isSelected ? Theme.buttonPrimaryColor : Theme.contactPressedColor
        radius: 0.5 * height

        MouseArea {
            anchors.fill: parent
            onClicked: selectedModel.selection.toggle(index)
        }

        Row {
            id: row
            spacing: flowListView.spacing

            // remove contact button
            Item {
                height: d.flowItemHeight
                width: height
                visible: model.isSelected

                Repeater {
                    model: 2

                    Rectangle {
                        anchors.centerIn: parent
                        width: 0.5 * parent.width
                        height: d.lineWidth
                        radius: height
                        color: Theme.brandColor
                        rotation: index ? -45 : 45
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: flowListView.contactSelected(model.username)
                }
            }

            // avatar
            Avatar {
                nickname: model.displayName
                avatarUrl: model.avatarUrl
                diameter: d.flowItemHeight
                visible: !model.isSelected
            }

            // contact name
            Text {
                color: Theme.primaryTextColor
                font.pointSize: UiHelper.fixFontSz(15)
                text: model.displayName
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
