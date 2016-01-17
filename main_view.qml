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
    contentItem.minimumWidth: 800
    contentItem.minimumHeight: 622
    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem { action: savePlotAction }
            MenuItem { action: screenshotAction }
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

    GridLayout {
        anchors.fill: parent
        anchors.margins: 10

        // Main panel
        ColumnLayout {
            Rectangle {
                width: 512
                height: 512
                border.width: 1
                border.color: "#cccccc"

                Item {
                    id: mainView
                    anchors.fill: parent
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

                    Colormap {
                        id: colormap
                        objectName: "colormap"
                        x: parent.x + 5
                        y: parent.y + 5
                        width: 128
                        height: 10
                    }
                }
            }

            Rectangle {
                Layout.minimumHeight: 80
                width: mainView.width
                border.width: 1
                border.color: "#cccccc"

                Item {
                    id: bottomView
                    anchors.fill: parent
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
        }

        // Options panel
        RowLayout {
            anchors.margins: parent.anchors.margins

            // Left column
            ColumnLayout {
                GroupBox {
                    title: "Scatterplot"

                    ColumnLayout {
                        GridLayout {
                            columns: 2

                            Label { text: "Colors:" }
                            ComboBox {
                                id: colormapCombo
                                model: [ "Categorical", "Continuous", "Divergent", "Rainbow" ]
                            }
                        }

                        GroupBox {
                            flat: true
                            title: "Glyph size"

                            GridLayout {
                                columns: 2
                                Label { text: "Control points:" }
                                SpinBox {
                                    id: cpGlyphSizeSpinBox
                                    maximumValue: 50
                                    minimumValue: 8
                                    value: cpPlot.glyphSize()
                                    decimals: 1
                                    stepSize: 1
                                    onValueChanged: cpPlot.setGlyphSize(this.value)
                                }

                                Label { text: "Regular points:" }
                                SpinBox {
                                    id: rpGlyphSizeSpinBox
                                    maximumValue: 50
                                    minimumValue: 8
                                    value: plot.glyphSize()
                                    decimals: 1
                                    stepSize: 1
                                    onValueChanged: plot.setGlyphSize(this.value)
                                }
                            }
                        }
                    }
                }

                GroupBox {
                    title: "Splat"

                    GridLayout {
                        columns: 2

                        Label { text: "Alpha:" }
                        SpinBox {
                            id: alphaSpinBox
                            maximumValue: 100
                            minimumValue: 1
                            value: splat.alpha()
                            decimals: 2
                            stepSize: 1
                            onValueChanged: splat.setAlpha(this.value)
                        }

                        Label { text: "Beta:" }
                        SpinBox {
                            id: betaSpinBox
                            maximumValue: 100
                            minimumValue: 1
                            value: splat.beta()
                            decimals: 2
                            stepSize: 1
                            onValueChanged: splat.setBeta(this.value)
                        }

                        Label { text: "Opacity (%):" }
                        SpinBox {
                            id: splatOpacitySpinBox
                            maximumValue: 100
                            minimumValue: 0
                            value: 100 * splat.opacity
                            decimals: 0
                            stepSize: 1
                            onValueChanged: splat.opacity = this.value / 100
                        }
                    }
                }
            }

            // Right column
            ColumnLayout {
            }
        }
    }

    Action {
        id: screenshotAction
        text: "Save screenshot"
        shortcut: "Ctrl+Shift+S"
        onTriggered: {
            mainView.grabToImage(function(result) {
                result.saveToFile("screenshot-main.png");
            });

            bottomView.grabToImage(function(result) {
                result.saveToFile("screenshot-bottom.png");
            });
        }
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

    Action {
        id: quitAction
        text: "&Quit"
        shortcut: "Ctrl+Q"
        onTriggered: Qt.quit()
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
}
