import QtQuick 2.0
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.2
import QtQuick.Extras 1.4
import PM 1.0

ApplicationWindow {
    title: "Projection Manipulation"
    visible: true
    width: 1200
    height: 600

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem { action: openAction }
            MenuItem { action: savePlotAction }
            MenuItem { action: quitAction }
        }

        Menu {
            title: "View"
            MenuItem {
                action: noneColorAction
                exclusiveGroup: coloringGroup
            }
            MenuItem {
                action: npColorAction
                exclusiveGroup: coloringGroup
            }
            MenuItem {
                action: stressColorAction
                exclusiveGroup: coloringGroup
            }
            MenuItem {
                action: silhouetteColorAction
                exclusiveGroup: coloringGroup
            }
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

    FileDialog {
        id: fileOpenDialog
        title: "Choose a data set to load..."
        selectMultiple: false
        selectExisting: true

        onAccepted: {
            console.log("Loading data set: " + this.fileUrl)
        }
    }

    FileDialog {
        id: fileSaveDialog
        title: "Save subsample mapping..."
        selectMultiple: false
        selectExisting: false

        onAccepted: {
            subsamplePlot.saveToFile(this.fileUrl)
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
        onTriggered: fileOpenDialog.open()
    }

    Action {
        id: savePlotAction
        text: "&Save subsample"
        shortcut: "Ctrl+S"
        onTriggered: {
            console.log("Saving subsample mapping...")
            fileSaveDialog.open()
        }
    }

    ExclusiveGroup {
        id: coloringGroup

        Action {
            id: noneColorAction
            text: "None"
            shortcut: "Shift+O"
            checked: true
            checkable: true
            onTriggered: console.log("None")
        }

        Action {
            id: npColorAction
            text: "Neighborhood Preservation"
            shortcut: "Shift+N"
            checkable: true
            onTriggered: console.log("NP")
        }

        Action {
            id: stressColorAction
            text: "Stress"
            shortcut: "Shift+S"
            checkable: true
            onTriggered: console.log("Stress")
        }

        Action {
            id: silhouetteColorAction
            text: "Silhouette"
            shortcut: "Shift+T"
            checkable: true
            onTriggered: console.log("Silhouette")
        }
    }
}
