import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "../theme"
import "../components"

ToolBar {

    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    property alias showBackButton: backButton.visible
    property alias showSeporator: seporator.visible

    default property alias menu: contextMenu.contentData

    background: Rectangle {
        implicitHeight: 60
        color: "transparent"

        Rectangle {
            id: seporator
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            height: 1
            color: Theme.chatBackgroundColor
            anchors.bottom: parent.bottom
        }
    }

    RowLayout {
        anchors.fill: parent

        Column {
            Layout.fillWidth: true
            Layout.leftMargin: 20

            Label {
                id: titleLabel
                elide: Label.ElideRight

                font.family: Theme.mainFont
                font.pointSize: UiHelper.fixFontSz(15)
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                font.bold: true
                color: Theme.primaryTextColor
            }

            Label {
                id: descriptionLabel
                elide: Label.ElideRight

                font.family: Theme.mainFont
                horizontalAlignment: Qt.AlignLeft
                verticalAlignment: Qt.AlignVCenter
                font.pointSize: UiHelper.fixFontSz(11)
                color: Theme.secondaryTextColor
            }
        }

        ImageButton {

            id: backButton
            imageSource: "../resources/icons/Search.png"

            // TODO: Componets sholdn't know about mainView
            // The logic must be as abastract as possible.
            onClicked: mainView.back()
        }

        ImageButton {
            Layout.leftMargin: 5
            Layout.rightMargin: 12

            id: menuButton
            imageSource: "../resources/icons/More.png"
            // visible: menu.length
            opacity: menu.length ? 1 : 0
            enabled: menu.length
            onClicked: {
                contextMenu.open()
            }

            ContextMenu {
                id: contextMenu
            }
        }
    }
}