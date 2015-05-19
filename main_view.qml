import QtQuick 2.0
import QtQuick.Controls 1.3
import PM 1.0

ApplicationWindow {
    visible: true
    width: 960
    height: 480

    menuBar: MenuBar {
        Menu {
            title: "Application"
            MenuItem { action: quitAction }
        }

        Menu {
            title: "File"
            MenuItem { action: openAction }
            MenuItem { action: savePlotAction }
            MenuItem { action: saveDataAction }
        }
    }

    Item {
        anchors.fill: parent

        Rectangle {
            anchors.fill: subsamplePlot
            border.width: 1
            border.color: "#cccccc"
            z: 0
        }

        Rectangle {
            anchors.fill: plot
            border.width: 1
            border.color: "#cccccc"
            z: 0
        }

        Scatterplot {
            id: subsamplePlot
            objectName: "subsamplePlot"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 0.5 * parent.width
            z: 1
        }

        Scatterplot {
            id: plot
            objectName: "plot"
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 0.5 * parent.width
            z: 1
        }
    }

    Action {
        id: quitAction
        text: "&Quit"
        shortcut: "Ctrl+Q"
        onTriggered: Qt.quit()
    }

    Action {
        id: openAction
        text: "&Open..."
        shortcut: "Ctrl+O"
        onTriggered: console.log("Open file")
    }

    Action {
        id: savePlotAction
        text: "Save &plot"
        shortcut: "Ctrl+S"
        onTriggered: console.log("Save plot")
    }

    Action {
        id: saveDataAction
        text: "Save &data"
        shortcut: "Ctrl+D"
        onTriggered: console.log("Save data")
    }
}
