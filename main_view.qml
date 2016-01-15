import QtQuick 2.5
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.2
import QtQuick.Extras 1.4
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import PM 1.0

ApplicationWindow {
    id: mainWindow
    title: "Projection"
    visible: true
    contentItem.minimumWidth: 532
    contentItem.minimumHeight: 622
    contentItem.maximumWidth: contentItem.minimumWidth
    contentItem.maximumHeight: contentItem.minimumHeight
    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem { action: savePlotAction }
            MenuItem { action: quitAction }
        }

        Menu {
            title: "Technique"
            MenuItem {
                action: lampTechniqueAction
                exclusiveGroup: techniqueGroup
            }
            MenuItem {
                action: lspTechniqueAction
                exclusiveGroup: techniqueGroup
            }
            MenuItem {
                action: plmpTechniqueAction
                exclusiveGroup: techniqueGroup
            }
            MenuItem {
                action: pekalskaTechniqueAction
                exclusiveGroup: techniqueGroup
            }
        }

        Menu {
            title: "Mode"
            MenuItem {
                action: cpModeAction
                exclusiveGroup: modeGroup
            }
            MenuItem {
                action: rpModeAction
                exclusiveGroup: modeGroup
            }
        }

        Menu {
            title: "View"
            MenuItem {
                action: labelColorAction
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

    statusBar: StatusBar {
        RowLayout {
            anchors.fill: parent
            Label {
                id: statusLabel
                text: "Selecting control points"
            }
        }
    }

    ColumnLayout {
        spacing: 10
        anchors.fill: parent
        anchors.margins: this.spacing

        Rectangle {
            //Layout.fillWidth: true
            //Layout.fillHeight: true
            width: 512
            height: 512
            border.width: 1
            border.color: "#cccccc"

            VoronoiSplat {
                id: splat
                objectName: "splat"
                x: parent.x
                y: parent.y
                anchors.fill: parent
            }

            Scatterplot {
                id: plot
                objectName: "plot"
                x: parent.x
                y: parent.y
                anchors.fill: parent
            }

            Scatterplot {
                id: cpPlot
                objectName: "cpPlot"
                x: parent.x
                y: parent.y
                anchors.fill: parent
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.minimumHeight: 80
            border.width: 1
            border.color: "#cccccc"

            BarChart {
                id: barChart
                objectName: "barChart"
                anchors.fill: parent
            }

            //HistoryGraph {
            //    id: history
            //    objectName: "history"
            //    anchors.fill: parent
            //}
        }
    }

    Action {
        id: quitAction
        text: "&Quit"
        shortcut: "Ctrl+Q"
        onTriggered: Qt.quit()
    }

    Action {
        id: savePlotAction
        text: "&Save data"
        shortcut: "Ctrl+S"
        onTriggered: {
            console.log("Saving control points' map...")
            Main.saveData()
        }
    }

    ExclusiveGroup {
        id: techniqueGroup

        Action {
            id: lampTechniqueAction
            text: "LAMP"
            shortcut: "Ctrl+1"
            checked: true
            checkable: true
            onTriggered: {
                Main.setTechnique(InteractionHandler.TECHNIQUE_LAMP)
            }
        }
        Action {
            id: lspTechniqueAction
            text: "LSP"
            shortcut: "Ctrl+2"
            checkable: true
            onTriggered: {
                Main.setTechnique(InteractionHandler.TECHNIQUE_LSP)
            }
        }
        Action {
            id: plmpTechniqueAction
            text: "PLMP"
            shortcut: "Ctrl+3"
            checkable: true
            onTriggered: {
                Main.setTechnique(InteractionHandler.TECHNIQUE_PLMP)
            }
        }
        Action {
            id: pekalskaTechniqueAction
            text: "Pekalska"
            shortcut: "Ctrl+4"
            checkable: true
            onTriggered: {
                Main.setTechnique(InteractionHandler.TECHNIQUE_PEKALSKA)
            }
        }
    }

    ExclusiveGroup {
        id: modeGroup

        Action {
            id: cpModeAction
            text: "Control points"
            checked: true
            checkable: true
            onTriggered: statusLabel.text = "Selecting control points"
        }
        Action {
            id: rpModeAction
            text: "Regular points"
            checkable: true
            onTriggered: statusLabel.text = "Selecting regular points"
        }
    }

    ExclusiveGroup {
        id: coloringGroup

        Action {
            id: labelColorAction
            text: "Labels"
            shortcut: "Shift+L"
            checked: true
            checkable: true
            onTriggered: console.log("stub: Labels")
        }

        Action {
            id: npColorAction
            text: "Neighborhood Preservation"
            shortcut: "Shift+N"
            checkable: true
            onTriggered: console.log("stub: NP")
        }

        Action {
            id: stressColorAction
            text: "Stress"
            shortcut: "Shift+S"
            checkable: true
            onTriggered: console.log("stub: Stress")
        }

        Action {
            id: silhouetteColorAction
            text: "Silhouette"
            shortcut: "Shift+T"
            checkable: true
            onTriggered: console.log("stub: Silhouette")
        }
    }

    // TODO
    //Window {
    //    title: "Options"
    //    minimumWidth: 500
    //    minimumHeight: 300
    //    visible: false
    //}
}
