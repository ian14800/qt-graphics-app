// Copyright 2026 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

import QtQuick
import QtQuick.Controls
import Esri.QtGraphicsTester

Item {
    property var renderers: [
        { name: "Simple renderer 1", id: "sRenderer1" },
        { name: "Simple renderer 2", id: "sRenderer2" },
        { name: "Web styles", id: "webStyleRenderer" },
        { name: "Unique value renderer", id: "uvRenderer" },
        { name: "Class breaks renderer", id: "cbRenderer" },
        { name: "Visual variables renderer", id: "vvRenderer" },
        { name: "Custom renderer", id: "cusRenderer" }
    ]

    property var updateTypes: [
        { name: "Change renderer", id: "updateRenderers" },
        { name: "Random symbol", id: "randomSymbols" },
        { name: "Uniform symbol", id: "uniformSymbols" },
        { name: "Move uniform direction", id: "uniformMove" },
        { name: "Move random direction", id: "randomMove" },
        { name: "Animated movement", id: "animatedMove" }
    ]

    LocalSceneView {
        id: view
        anchors.fill: parent
    }

    QtGraphicsTester {
        id: tester
        sceneView: view
    }

    Column {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 3
        spacing: 6

        Row {
            spacing: 6
            TextField {
                inputMethodHints: Qt.ImhPreferNumbers

                id: graphicsCountInput
                width: 200
                placeholderText: "Graphics count"

                // onActivated: {
                //     const graphicsCount = graphicsCountInput.value;
                // }
            }

            Button {
                text: "Add Graphics"
                onClicked: {
                    const count = parseInt(graphicsCountInput.text);
                    if (!isNaN(count) && count > 0)
                        tester.addGraphics(count);
                }
            }
        }

        Row {
            spacing: 6


            ComboBox {
                id: overlayRendererInput
                width: 200
                model: renderers.map(s => s.name)
                currentIndex: 0

                onActivated: {
                    const selected = renderers[currentIndex];
                    tester.setOverlayRenderer(selected.id);
                }

            }

            Button {
                text: "Remove Graphics"
                onClicked: {
                    const count = parseInt(graphicsCountInput.text);
                    if (!isNaN(count) && count > 0)
                        tester.removeGraphics(count);
                }
            }
        }

        Row {
            spacing: 6


            ComboBox {
                id: updateTypeInput
                width: 200
                model: updateTypes.map(s => s.name)

            }

            Button {
                text: "Update Graphics"
                onClicked: {
                    const selected = updateTypes[updateTypeInput.currentIndex];
                    const count = parseInt(graphicsCountInput.text);
                    if (!isNaN(count) && count > 0)
                        tester.updateAllGraphics(selected.id, count);
                }
            }
        }

        Row {
            spacing: 6


            Button {
                text: "Add and move graphics"
                onClicked: {
                    tester.addGraphicsAndMove();
                }
            }

            Button {
                text: "Spin graphic (pitch)"
                onClicked: {
                    tester.spinGraphics("pitch");
                }
            }
        }

        Row {
            spacing: 6


            Button {
                text: "Move graphic point 1"
                onClicked: {
                    tester.moveGraphic(0, 0);
                }
            }

            Button {
                text: "Move graphic point 2"
                onClicked: {
                    tester.moveGraphic(120, 0);
                }
            }
        }

        Row {
            spacing: 6


            Button {
                text: "Move Camera point 1"
                onClicked: {
                    tester.moveCamera(0, 0);
                }
            }

            Button {
                text: "Move Camera point 2"
                onClicked: {
                    tester.moveCamera(120, 0);
                }
            }
        }

    }

    Rectangle {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        width: 360
        height: 180
        radius: 8
        color: "#CC101010"
        border.color: "#66FFFFFF"
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            Text {
                text: "Diagnostics"
                color: "white"
                font.bold: true
                font.pixelSize: 15
            }

            ScrollView {
                width: parent.width
                height: parent.height - 30

                TextArea {
                    text: tester.diagnosticsText
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    color: "white"
                    background: null
                    selectByMouse: true
                }
            }
        }
    }
}
