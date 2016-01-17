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

        //Menu {
        //    title: "Technique"
        //    MenuItem {
        //        action: lampTechniqueAction
        //        exclusiveGroup: techniqueGroup
        //    }
        //    MenuItem {
        //        action: lspTechniqueAction
        //        exclusiveGroup: techniqueGroup
        //    }
        //    MenuItem {
        //        action: plmpTechniqueAction
        //        exclusiveGroup: techniqueGroup
        //    }
        //    MenuItem {
        //        action: pekalskaTechniqueAction
        //        exclusiveGroup: techniqueGroup
        //    }
        //}

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
        Layout.margins: 10
        anchors.margins: 10
        anchors.fill: parent
        columnSpacing: 10
        rowSpacing: 10

        // Main panel
        ColumnLayout {
            spacing: 10

            Rectangle {
                width: 512
                height: 512
                color: "#ffffff"

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

                        Rectangle { // Adds a border around the colormap
                            x: parent.x - 1
                            y: parent.y - 1
                            width: parent.width + 2
                            height: parent.height + 2
                            border.width: 1
                            border.color: "#000000"
                            color: "transparent"
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#cccccc"
                    color: "transparent"
                }
            }

            Rectangle { id: secret
                Layout.minimumHeight: 80
                Layout.fillHeight: true
                width: mainView.width
                color: "#ffffff"

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

                Rectangle {
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#cccccc"
                    color: "transparent"
                }
            }
        }

        // Options panel
        ColumnLayout {
            Layout.alignment: Qt.AlignTop | Qt.AlignLeft

            GroupBox {
                Layout.fillWidth: true
                title: "Projection metrics"

                Column {
                    ExclusiveGroup { id: wrtMetricsGroup }

                    RadioButton {
                        text: "Current"
                        exclusiveGroup: wrtMetricsGroup
                        checked: true
                    }
                    RadioButton {
                        text: "Diff. to previous"
                        exclusiveGroup: wrtMetricsGroup
                    }
                    RadioButton {
                        text: "Diff. to original"
                        exclusiveGroup: wrtMetricsGroup
                    }
                }
            }

            GroupBox {
                title: "Scatterplot"
                checkable: true
                __checkbox.onClicked: {
                    cpPlot.visible = this.checked;
                    plot.visible = this.checked;
                }

                ColumnLayout {
                    GroupBox {
                        flat: true
                        title: "Colors"

                        GridLayout {
                            columns: 2

                            Label { text: "Map to:" }
                            ComboBox {
                                id: plotMetricComboBox
                                model: metricsModel
                            }

                            Label { text: "Color map:" }
                            ComboBox {
                                id: scatterplotColormapCombo
                                model: [ "Categorical", "Continuous", "Divergent", "Rainbow" ]
                            }
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
                                maximumValue: 100
                                minimumValue: 8
                                value: cpPlot.glyphSize()
                                decimals: 1
                                stepSize: 1
                                onValueChanged: cpPlot.setGlyphSize(this.value)
                            }

                            Label { text: "Regular points:" }
                            SpinBox {
                                id: rpGlyphSizeSpinBox
                                maximumValue: 100
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
                checkable: true
                __checkbox.onClicked: {
                    splat.visible = this.checked
                }

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

                    Label { text: "Opacity:" }
                    Slider {
                        id: splatOpacitySlider
                        maximumValue: 1
                        minimumValue: 0
                        value: splat.opacity
                        onValueChanged: splat.opacity = this.value
                    }
                }
            }

            GroupBox {
                title: "Bar chart"
                checkable: true
                __checkbox.onClicked: {
                    bottomView.visible = this.checked
                }

                GridLayout {
                    columns: 2

                    Label { text: "Color map:" }
                    ComboBox {
                        id: barChartColormapCombo
                        model: [ "Categorical", "Continuous", "Divergent", "Rainbow" ]
                    }
                }
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

    //ExclusiveGroup {
    //    id: techniqueGroup

    //    Action {
    //        id: lampTechniqueAction
    //        text: "LAMP"
    //        shortcut: "Ctrl+1"
    //        checked: true
    //        checkable: true
    //        onTriggered: {
    //            Main.setTechnique(InteractionHandler.TECHNIQUE_LAMP)
    //        }
    //    }
    //    Action {
    //        id: lspTechniqueAction
    //        text: "LSP"
    //        shortcut: "Ctrl+2"
    //        checkable: true
    //        onTriggered: {
    //            Main.setTechnique(InteractionHandler.TECHNIQUE_LSP)
    //        }
    //    }
    //    Action {
    //        id: plmpTechniqueAction
    //        text: "PLMP"
    //        shortcut: "Ctrl+3"
    //        checkable: true
    //        onTriggered: {
    //            Main.setTechnique(InteractionHandler.TECHNIQUE_PLMP)
    //        }
    //    }
    //    Action {
    //        id: pekalskaTechniqueAction
    //        text: "Pekalska"
    //        shortcut: "Ctrl+4"
    //        checkable: true
    //        onTriggered: {
    //            Main.setTechnique(InteractionHandler.TECHNIQUE_PEKALSKA)
    //        }
    //    }
    //}

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

    ListModel {
        id: metricsModel

        ListElement { text: "Aggregate error" }
        ListElement { text: "CP influence" }
        ListElement { text: "Stress" }
    }
}
