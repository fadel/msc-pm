import QtQuick 2.0
import PM 1.0

Item {
    width: 480
    height: 480

    Scatterplot {
        id: plot
        objectName: "plot"
        anchors.fill: parent
    }

    Item {
        id: messageBox
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        Rectangle {
            color: Qt.rgba(1, 1, 1, 0.7)
            radius: 5
            border.width: 1
            border.color: "white"
            anchors.fill: messageLabel
            anchors.margins: -10
        }

        Text {
            id: messageLabel
            color: "black"
            wrapMode: Text.WordWrap
            text: "The background here is a squircle rendered with raw OpenGL using the 'beforeRender()' signal in QQuickWindow. This text label and its border is rendered using QML"
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 20
        }
    }
}
