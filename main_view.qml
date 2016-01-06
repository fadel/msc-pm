import QtQuick 2.5
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.2
import QtQuick.Extras 1.4
import QtQuick.Layouts 1.2
import PM 1.0

ApplicationWindow {
    id: mainWindow
    title: "Projection Manipulation"
    visible: true
    minimumWidth: 900
    minimumHeight: 600
    maximumWidth: minimumWidth
    maximumHeight: minimumHeight

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

    ColumnLayout {
        spacing: 10
        anchors.fill: parent
        anchors.margins: this.spacing

        RowLayout {
            spacing: 10
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                border.width: 1
                border.color: "#cccccc"

                Scatterplot {
                    id: subsamplePlot
                    objectName: "subsamplePlot"
                    anchors.fill: parent
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                border.width: 1
                border.color: "#cccccc"

                Scatterplot {
                    id: plot
                    objectName: "plot"
                    width: 512
                    height: 512
                    anchors.margins: 10
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.minimumHeight: 150
            border.width: 1
            border.color: "#cccccc"

            HistoryGraph {
                id: history
                objectName: "history"
                anchors.fill: parent
            }
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
            console.log("Saving subsample mapping...")
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
}
