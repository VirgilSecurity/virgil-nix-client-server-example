import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

import "../theme"

Item {
    id: form
    anchors.fill: parent

    default property alias children: formContainer.children
    property bool isLoading: false

    Item {
        id: formContainer
        anchors.fill: parent
        visible: !isLoading
    }

    // TODO: I don't like this eather, but let's
    // keep it simple for now ;)
    ColumnLayout {
        visible: isLoading

        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: parent.right
        }

        spacing: Theme.spacing

        FormImage {
            source: "../resources/icons/Logo.png"
        }

        FormLabel {
            id: busyDescryption
            horizontalAlignment: Text.AlignHCenter
        }
    }

    function showLoading(loadingText) {
        isLoading = true
        busyDescryption.text = loadingText
    }

    function hideLoading() {
        isLoading = false
        busyDescryption.text = ""
    }
}