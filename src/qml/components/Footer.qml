import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12

import "../theme"

Item {
    property alias text: footerText.text
    readonly property real tightLetterSpacing: 0.23

    height: 40
    width: parent.width
    visible: text !== "" ? true : false

    Text {
        id: footerText
        anchors.centerIn: parent
        font.letterSpacing: tightLetterSpacing
        verticalAlignment: Text.AlignBottom
        color: Theme.labelColor
        horizontalAlignment: Text.AlignHCenter
        font.pointSize: UiHelper.fixFontSz(10)
    }
}
