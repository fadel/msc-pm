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
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            MenuItem { action: savePlotAction }
            MenuItem { action: screenshotAction }
            MenuItem { action: quitAction }
        }

        Menu {
            title: "Edit"
            MenuItem { action: undoManipulationAction }
            MenuItem { action: resetManipulationAction }
        }

        Menu {
            title: "Select"
            MenuItem { action: selectRPsAction }
            MenuItem { action: selectCPsAction }
        }

        Menu {
            title: "View"
            MenuItem { action: toggleOptionsAction }
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

                    LinePlot {
                        id: bundlePlot
                        objectName: "bundlePlot"
                        x: parent.x
                        y: parent.y
                        z: 1
                        anchors.fill: parent
                    }

                    Scatterplot {
                        id: rpPlot
                        objectName: "rpPlot"
                        x: parent.x
                        y: parent.y
                        z: 2
                        anchors.fill: parent
                        glyphSize: 3.0
                    }

                    Scatterplot {
                        id: cpPlot
                        objectName: "cpPlot"
                        x: parent.x
                        y: parent.y
                        z: 3
                        anchors.fill: parent
                    }

                    TransitionControl {
                        id: plotTC
                        objectName: "plotTC"
                        x: parent.x
                        y: parent.y
                        z: 4
                        anchors.fill: parent
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    border.width: 1
                    border.color: "#cccccc"
                    color: "transparent"
                }
            }

            RowLayout {
                Layout.minimumHeight: 60
                Layout.fillHeight: true
                width: mainView.width

                Colormap {
                    Layout.fillHeight: true
                    id: cpColormap
                    objectName: "cpColormap"
                    width: 5
                    orientation: Colormap.Vertical

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

                Rectangle {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
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
                    }

                    Rectangle {
                        anchors.fill: parent
                        border.width: 1
                        border.color: "#cccccc"
                        color: "transparent"
                    }
                }
            }

            RowLayout {
                Layout.minimumHeight: 60
                Layout.fillHeight: true
                width: mainView.width

                Colormap {
                    Layout.fillHeight: true
                    id: rpColormap
                    objectName: "rpColormap"
                    width: 5
                    orientation: Colormap.Vertical

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

                Rectangle {
                    Layout.minimumHeight: 60
                    Layout.fillHeight: true
                    Layout.fillWidth: true
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
                    }

                    Rectangle {
                        anchors.fill: parent
                        border.width: 1
                        border.color: "#cccccc"
                        color: "transparent"
                    }
                }
            }
        }

        // Options panel
        RowLayout {
            id: optionsPanel

            ColumnLayout {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft

                GroupBox {
                    Layout.fillWidth: true
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

                    ColumnLayout {
                        GroupBox {
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
                                    onActivated:
                                        Main.setCPColorScale(model.get(index).value);
                                }
                            }
                        }

                        GroupBox {
                            flat: true
                            title: "Glyphs"

                            GridLayout {
                                columns: 2

                                Label { text: "Size:" }
                                SpinBox {
                                    id: cpGlyphSizeSpinBox
                                    maximumValue: 100
                                    minimumValue: 6
                                    decimals: 1
                                    stepSize: 1
                                    value: cpPlot.glyphSize
                                    onValueChanged: cpPlot.glyphSize = this.value
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
                    }
                }

                GroupBox {
                    Layout.fillWidth: true
                    title: "Regular points"
                    checked: true
                    checkable: true
                    __checkbox.onClicked: {
                        rpPlot.visible = this.checked;
                        splat.visible  = this.checked;
                    }

                    ColumnLayout {
                        GroupBox {
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
                                    onActivated:
                                        Main.setRPColorScale(model.get(index).value);
                                }
                            }
                        }

                        GroupBox {
                            flat: true
                            title: "Splat"

                            GridLayout {
                                columns: 2

                                Label { text: "Blur (α):" }
                                SpinBox {
                                    id: alphaSpinBox
                                    maximumValue: 100
                                    minimumValue: 1
                                    value: splat.alpha
                                    decimals: 2
                                    stepSize: 1
                                    onValueChanged: splat.alpha = this.value
                                }

                                Label { text: "Radius (β):" }
                                SpinBox {
                                    id: betaSpinBox
                                    maximumValue: 100
                                    minimumValue: 1
                                    value: splat.beta
                                    decimals: 2
                                    stepSize: 1
                                    onValueChanged: splat.beta = this.value
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

                        GroupBox {
                            flat: true
                            title: "Glyphs"

                            GridLayout {
                                columns: 2

                                Label { text: "Size:" }
                                SpinBox {
                                    id: rpGlyphSizeSpinBox
                                    maximumValue: 100
                                    minimumValue: 2
                                    decimals: 1
                                    stepSize: 1
                                    value: rpPlot.glyphSize
                                    onValueChanged: rpPlot.glyphSize = this.value
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
                    }
                }

                GroupBox {
                    Layout.fillWidth: true
                    id: metricsGroupBox
                    title: "Projection metrics"
                    property RadioButton current: currentMetricRadioButton

                    Column {
                        ExclusiveGroup { id: wrtMetricsGroup }

                        RadioButton {
                            id: currentMetricRadioButton
                            text: "Current"
                            exclusiveGroup: wrtMetricsGroup
                            checked: true
                            onClicked: {
                                if (!Main.setObserverType(Main.ObserverCurrent)) {
                                    metricsGroupBox.current.checked = true;
                                } else {
                                    metricsGroupBox.current = this;
                                }
                            }
                        }
                        RadioButton {
                            id: diffPreviousMetricRadioButton
                            text: "Diff. to previous"
                            exclusiveGroup: wrtMetricsGroup
                            onClicked: {
                                if (!Main.setObserverType(Main.ObserverDiffPrevious)) {
                                    metricsGroupBox.current.checked = true;
                                } else {
                                    metricsGroupBox.current = this;
                                }
                            }
                        }
                        RadioButton {
                            id: diffFirstMetricRadioButton
                            text: "Diff. to first"
                            exclusiveGroup: wrtMetricsGroup
                            onClicked: {
                                if (!Main.setObserverType(Main.ObserverDiffFirst)) {
                                    metricsGroupBox.current.checked = true;
                                } else {
                                    metricsGroupBox.current = this;
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft

                GroupBox {
                    Layout.fillWidth: true
                    id: bundlingGroupBox
                    title: "Bundling"
                    checked: true
                    checkable: true
                    __checkbox.onClicked: {
                        bundlePlot.visible = this.checked;
                    }

                    ColumnLayout {
                        GroupBox {
                            flat: true
                            title: "Main bundling"

                            GridLayout {
                                columns: 2

                                Label { text: "Iterations:" }
                                SpinBox {
                                    maximumValue: 100
                                    minimumValue: 0
                                    decimals: 0
                                    stepSize: 1
                                    value: bundlePlot.iterations
                                    onValueChanged: bundlePlot.iterations = this.value
                                }

                                Label { text: "Kernel size:" }
                                SpinBox {
                                    maximumValue: 100
                                    minimumValue: 3
                                    decimals: 1
                                    stepSize: 1
                                    value: bundlePlot.kernelSize
                                    onValueChanged: bundlePlot.kernelSize = this.value
                                }

                                Label { text: "Smoothing factor:" }
                                Slider {
                                    tickmarksEnabled: true
                                    stepSize: 0.1
                                    maximumValue: 1
                                    minimumValue: 0
                                    value: bundlePlot.smoothingFactor
                                    onValueChanged: bundlePlot.smoothingFactor = this.value
                                }

                                Label { text: "Smoothing iterations:" }
                                SpinBox {
                                    maximumValue: 100
                                    minimumValue: 0
                                    decimals: 0
                                    stepSize: 1
                                    value: bundlePlot.smoothingIterations
                                    onValueChanged: bundlePlot.smoothingIterations = this.value
                                }
                            }
                        }

                        GroupBox {
                            flat: true
                            title: "Ends bundling"

                            GridLayout {
                                columns: 2

                                CheckBox {
                                    Layout.columnSpan: 2
                                    text: "Block endpoints"
                                    checked: bundlePlot.blockEndpoints
                                    onClicked: bundlePlot.blockEndpoints = this.checked
                                }

                                Label { text: "Iterations:" }
                                SpinBox {
                                    maximumValue: 100
                                    minimumValue: 0
                                    decimals: 0
                                    stepSize: 1
                                    value: bundlePlot.endsIterations
                                    onValueChanged: bundlePlot.endsIterations = this.value
                                }

                                Label { text: "Kernel size:" }
                                SpinBox {
                                    maximumValue: 100
                                    minimumValue: 3
                                    decimals: 1
                                    stepSize: 1
                                    value: bundlePlot.endsKernelSize
                                    onValueChanged: bundlePlot.endsKernelSize = this.value
                                }

                                Label { text: "Smoothing factor:" }
                                Slider {
                                    tickmarksEnabled: true
                                    stepSize: 0.1
                                    maximumValue: 1
                                    minimumValue: 0
                                    value: bundlePlot.endsSmoothingFactor
                                    onValueChanged: bundlePlot.endsSmoothingFactor = this.value
                                }
                            }
                        }

                        GroupBox {
                            flat: true
                            title: "General"

                            GridLayout {
                                columns: 2

                                Label { text: "Line width:" }
                                SpinBox {
                                    maximumValue: 100
                                    minimumValue: 1
                                    decimals: 1
                                    stepSize: 1
                                    value: bundlePlot.lineWidth
                                    onValueChanged: bundlePlot.lineWidth = this.value
                                }

                                Label { text: "Opacity:" }
                                Slider {
                                    tickmarksEnabled: true
                                    stepSize: 0.1
                                    maximumValue: 1
                                    minimumValue: 0
                                    value: bundlePlot.opacity
                                    onValueChanged: bundlePlot.opacity = this.value
                                }

                                Label { text: "Edge sampling:" }
                                SpinBox {
                                    maximumValue: 100
                                    minimumValue: 3
                                    decimals: 1
                                    stepSize: 1
                                    value: bundlePlot.edgeSampling
                                    onValueChanged: bundlePlot.edgeSampling = this.value
                                }

                                Label { text: "Advection speed:" }
                                Slider {
                                    tickmarksEnabled: true
                                    stepSize: 0.1
                                    maximumValue: 1
                                    minimumValue: 0
                                    value: bundlePlot.advectionSpeed
                                    onValueChanged: bundlePlot.advectionSpeed = this.value
                                }

                                Label { text: "Density estimation:" }
				RowLayout {
				    ExclusiveGroup { id: densityEstimationGroup }
				    RadioButton {
					text: "Exact"
					exclusiveGroup: densityEstimationGroup
				    }
				    RadioButton {
					text: "Fast"
					exclusiveGroup: densityEstimationGroup
					checked: true
				    }
				}

                                Label { text: "Bundle shape:" }
				RowLayout {
				    ExclusiveGroup { id: bundleShapeGroup }
				    RadioButton {
					text: "FDEB"
					exclusiveGroup: bundleShapeGroup
					checked: true
				    }
				    RadioButton {
					text: "HEB"
					exclusiveGroup: bundleShapeGroup
				    }
				}

                                Label { text: "Relaxation:" }
                                Slider {
                                    tickmarksEnabled: true
                                    stepSize: 0.1
                                    maximumValue: 1
                                    minimumValue: 0
                                    value: bundlePlot.relaxation
                                    onValueChanged: bundlePlot.relaxation = this.value
                                }

                                CheckBox {
                                    Layout.columnSpan: 2
                                    text: "Use GPU"
                                    checked: bundlePlot.bundleGPU
                                    onClicked: bundlePlot.bundleGPU = this.checked
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: prefixDialog
        title: "Save screenshot"
        standardButtons: StandardButton.Save | StandardButton.Cancel

        TextField {
            id: prefixTextField
            placeholderText: "Enter prefix"
        }

        Timer {
            id: screenshotTimer
            interval: 500
            running: false
            repeat: false
            onTriggered: {
                var prefix = prefixTextField.text;
                if (prefix.length == 0) {
                    prefix = "screenshot";
                }

                mainView.grabToImage(function(result) {
                    result.saveToFile(prefix + "-main.png");
                });

                bottomViewCP.grabToImage(function(result) {
                    result.saveToFile(prefix + "-bottom-cp.png");
                });

                bottomViewRP.grabToImage(function(result) {
                    result.saveToFile(prefix + "-bottom-rp.png");
                });

                prefixTextField.text = "";
            }
        }

        onAccepted: screenshotTimer.start()
    }

    Action {
        id: screenshotAction
        text: "Save screenshot"
        shortcut: "Ctrl+Shift+S"
        onTriggered: {
            prefixDialog.open();
            prefixTextField.forceActiveFocus();
        }
    }

    Action {
        id: savePlotAction
        text: "&Save data"
        shortcut: "Ctrl+S"
        onTriggered: Main.saveData()
    }

    Action {
        id: quitAction
        text: "&Quit"
        shortcut: "Ctrl+Q"
        onTriggered: Qt.quit()
    }

    Action {
        id: undoManipulationAction
        text: "&Undo manipulation"
        shortcut: "Ctrl+Z"
        onTriggered: Main.undoManipulation()
    }

    Action {
        id: resetManipulationAction
        text: "&Reset manipulation"
        shortcut: "Ctrl+R"
        onTriggered: Main.resetManipulation()
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
        id: toggleOptionsAction
        text: "&Options"
        shortcut: "Ctrl+O"
        checkable: true
        checked: true
        onToggled: {
            optionsPanel.visible = this.checked;
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
