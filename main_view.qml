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
    onClosing: Qt.quit()
    Component.onCompleted: {
        setX(Screen.width / 2 - width / 2);
        setY(Screen.height / 2 - height / 2);

        this.minimumWidth = width;
        this.minimumHeight = height;
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem { action: savePlotAction }
            MenuItem { action: screenshotAction }
            MenuItem { action: quitAction }
        }

        Menu {
            title: "Select"
            MenuItem { action: selectRPsAction }
            MenuItem { action: selectCPsAction }
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
                        z: 0
                        anchors.fill: parent
                    }

                    Scatterplot {
                        id: rpPlot
                        objectName: "rpPlot"
                        x: parent.x
                        y: parent.y
                        z: 1
                        anchors.fill: parent
                    }

                    Scatterplot {
                        id: cpPlot
                        objectName: "cpPlot"
                        x: parent.x
                        y: parent.y
                        z: 2
                        anchors.fill: parent
                    }

                    Colormap {
                        id: colormap
                        objectName: "colormap"
                        x: parent.x + 5
                        y: parent.y + 5
                        z: 2
                        width: 128
                        height: 5

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

            Rectangle {
                Layout.minimumHeight: 60
                Layout.fillHeight: true
                width: mainView.width
                color: "#ffffff"

                Item {
                    id: bottomViewCP
                    anchors.fill: parent

                    BarChart {
                        id: cpBarChart
                        objectName: "cpBarChart"
                        anchors.fill: parent

                        Label {
                            anchors.fill: parent
                            anchors.margins: 5
                            horizontalAlignment: Text.AlignRight
                            text: "Control points"
                        }
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

            Rectangle {
                Layout.minimumHeight: 60
                Layout.fillHeight: true
                width: mainView.width
                color: "#ffffff"

                Item {
                    id: bottomViewRP
                    anchors.fill: parent

                    BarChart {
                        id: rpBarChart
                        objectName: "rpBarChart"
                        anchors.fill: parent

                        Label {
                            anchors.fill: parent
                            anchors.margins: 5
                            horizontalAlignment: Text.AlignRight
                            text: "Regular points"
                        }
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
                title: "Control points"
                checkable: true
                __checkbox.onClicked: {
                    cpPlot.visible = this.checked;

                    if (this.checked) {
                        cpPlot.z = 0;
                        rpPlot.z = 0;
                    } else {
                        cpPlot.z = 0;
                        rpPlot.z = 1;
                    }
                }

                GridLayout {
                    columns: 2

                    GroupBox {
                        Layout.columnSpan: 2
                        flat: true
                        title: "Colors"

                        GridLayout {
                            columns: 2

                            Label { text: "Map to:" }
                            ComboBox {
                                id: cpPlotMetricComboBox
                                model: metricsModel
                            }

                            Label { text: "Color map:" }
                            ComboBox {
                                id: cpPlotColormapCombo
                                model: colormapModel
                                onActivated: {
                                    Main.setCPPlotColorScale(model.get(index).value);
                                    Main.setCPBarChartColorScale(model.get(index).value);
                                }
                            }
                        }
                    }

                    Label { text: "Glyph size:" }
                    SpinBox {
                        id: cpGlyphSizeSpinBox
                        maximumValue: 100
                        minimumValue: 6
                        value: cpPlot.glyphSize()
                        decimals: 1
                        stepSize: 1
                        onValueChanged: cpPlot.setGlyphSize(this.value)
                    }

                    Label { text: "Opacity:" }
                    Slider {
                        id: cpPlotOpacitySlider
                        tickmarksEnabled: true
                        stepSize: 0.1
                        maximumValue: 1
                        minimumValue: 0
                        value: cpPlot.opacity
                        onValueChanged: cpPlot.opacity = this.value
                    }
                }
            }

            GroupBox {
                title: "Regular points"
                checked: true
                checkable: true
                __checkbox.onClicked: {
                    rpPlot.visible = this.checked;
                    splat.visible  = this.checked;
                }

                GridLayout {
                    columns: 2

                    GroupBox {
                        Layout.columnSpan: 2
                        flat: true
                        title: "Colors"

                        GridLayout {
                            columns: 2

                            Label { text: "Map to:" }
                            ComboBox {
                                id: rpPlotMetricComboBox
                                model: metricsModel
                            }

                            Label { text: "Color map:" }
                            ComboBox {
                                id: rpPlotColormapCombo
                                model: colormapModel
                                onActivated: {
                                    Main.setRPPlotColorScale(model.get(index).value);
                                    Main.setSplatColorScale(model.get(index).value);
                                    Main.setRPBarChartColorScale(model.get(index).value);
                                }
                            }
                        }
                    }

                    GroupBox {
                        Layout.columnSpan: 2
                        flat: true
                        title: "Splat"

                        GridLayout {
                            columns: 2

                            Label { text: "Blur (α):" }
                            SpinBox {
                                id: alphaSpinBox
                                maximumValue: 100
                                minimumValue: 1
                                value: splat.alpha()
                                decimals: 2
                                stepSize: 1
                                onValueChanged: splat.setAlpha(this.value)
                            }

                            Label { text: "Radius (β):" }
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
                                tickmarksEnabled: true
                                stepSize: 0.1
                                maximumValue: 1
                                minimumValue: 0
                                value: splat.opacity
                                onValueChanged: splat.opacity = this.value
                            }
                        }
                    }

                    Label { text: "Glyph size:" }
                    SpinBox {
                        id: rpGlyphSizeSpinBox
                        maximumValue: 100
                        minimumValue: 2
                        value: rpPlot.glyphSize()
                        decimals: 1
                        stepSize: 1
                        onValueChanged: rpPlot.setGlyphSize(this.value)
                    }

                    Label { text: "Opacity:" }
                    Slider {
                        id: rpPlotOpacitySlider
                        tickmarksEnabled: true
                        stepSize: 0.1
                        maximumValue: 1
                        minimumValue: 0
                        value: rpPlot.opacity
                        onValueChanged: rpPlot.opacity = this.value
                    }
                }
            }

            GroupBox {
                Layout.fillWidth: true
                title: "Projection metrics"

                Column {
                    ExclusiveGroup { id: wrtMetricsGroup }

                    RadioButton {
                        id: currentMetricRadioButton
                        text: "Current"
                        exclusiveGroup: wrtMetricsGroup
                        checked: true
                        onClicked: Main.setObserverType(Main.ObserverCurrent)
                    }
                    RadioButton {
                        id: diffPreviousMetricRadioButton
                        text: "Diff. to previous"
                        exclusiveGroup: wrtMetricsGroup
                        onClicked: {
                            if (!Main.setObserverType(Main.ObserverDiffPrevious)) {
                                this.checked = false;
                                currentMetricRadioButton.checked = true;
                            }
                        }
                    }
                    RadioButton {
                        id: diffOriginalMetricRadioButton
                        text: "Diff. to original"
                        exclusiveGroup: wrtMetricsGroup
                        onClicked: {
                            if (!Main.setObserverType(Main.ObserverDiffOriginal)) {
                                this.checked = false;
                                currentMetricRadioButton.checked = true;
                            }
                        }
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

            bottomViewCP.grabToImage(function(result) {
                result.saveToFile("screenshot-bottom-cp.png");
            });

            bottomViewRP.grabToImage(function(result) {
                result.saveToFile("screenshot-bottom-rp.png");
            });
        }
    }

    Action {
        id: savePlotAction
        text: "&Save data"
        shortcut: "Ctrl+S"
        onTriggered: {
            Main.saveData()
        }
    }

    Action {
        id: quitAction
        text: "&Quit"
        shortcut: "Ctrl+Q"
        onTriggered: Qt.quit()
    }

    Action {
        id: selectRPsAction
        text: "&Regular points"
        shortcut: "R"
        onTriggered: {
            Main.setSelectRPs();
            statusLabel.text = "Selecting regular points";
        }
    }

    Action {
        id: selectCPsAction
        text: "&Control points"
        shortcut: "C"
        onTriggered: {
            Main.setSelectCPs();
            statusLabel.text = "Selecting control points";
        }
    }

    ListModel {
        id: metricsModel

        ListElement { text: "Aggregate error" }
        ListElement { text: "CP influence" }
        ListElement { text: "Stress" }
    }

    ListModel {
        id: colormapModel

        Component.onCompleted: {
            // Data has to be fed this way; otherwise "value" is not correct
            this.append({
                "value": Main.ColorScaleRainbow,
                "text": "Rainbow"
            });
            this.append({
                "value": Main.ColorScaleContinuous,
                "text": "Continuous"
            });
            this.append({
                "value": Main.ColorScaleCategorical,
                "text": "Categorical"
            });
            this.append({
                "value": Main.ColorScaleDivergent,
                "text": "Divergent"
            });
        }
    }
}
