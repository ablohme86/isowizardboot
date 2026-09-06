import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1060
    height: 880
    minimumWidth: 820
    minimumHeight: 740
    visible: true
    title: "IsoWizardBoot — USB image writer"
    color: "#101216"
    font.family: "Noto Sans"
    font.pixelSize: 13

    readonly property color ink: "#f3f0e9"
    readonly property color muted: "#92969f"
    readonly property color accent: "#ffac69"
    readonly property color line: "#30333a"
    property string selectedPath: ""
    property var selectedDevice: null
    property string confirmationPath: ""
    property string confirmationIdentity: ""
    property bool confirmVerify: true
    readonly property bool tooSmall: selectedDevice !== null && backend.imageSize > selectedDevice.size
    readonly property bool ready: backend.imageSize > 0 && selectedDevice !== null && !tooSmall && !selectedDevice.mounted && !backend.busy

    function updateSelection() {
        let selected = null
        for (let i = 0; i < backend.devices.length; ++i) {
            if (backend.devices[i].path === selectedPath) {
                selected = backend.devices[i]
                usbSelect.currentIndex = i
                break
            }
        }
        if (selected === null) {
            selectedPath = ""
            usbSelect.currentIndex = -1
        }
        selectedDevice = selected
    }

    Connections {
        target: backend
        function onDevicesChanged() { window.updateSelection() }
    }

    onClosing: function(close) {
        if (backend.busy) {
            close.accepted = false
            stopDialog.open()
        }
    }

    component Symbol: Canvas {
        property string kind: "disc"
        property color tint: window.accent
        implicitWidth: 24
        implicitHeight: 24
        onTintChanged: requestPaint()
        onKindChanged: requestPaint()
        onPaint: {
            let c = getContext("2d")
            c.reset()
            c.strokeStyle = tint
            c.fillStyle = tint
            c.lineWidth = 1.6
            c.lineCap = "round"
            c.lineJoin = "round"
            c.beginPath()
            if (kind === "disc") {
                c.arc(12, 12, 9, 0, Math.PI * 2)
                c.moveTo(15, 12)
                c.arc(12, 12, 3, 0, Math.PI * 2)
                c.moveTo(7, 7); c.lineTo(9, 5)
            } else if (kind === "usb") {
                c.rect(7, 9, 10, 12)
                c.rect(9, 2, 6, 7)
                c.moveTo(11, 5); c.lineTo(13, 5)
                c.moveTo(10, 17); c.lineTo(14, 17)
            } else if (kind === "refresh") {
                c.arc(12, 12, 8, -0.6, 4.8)
                c.moveTo(19, 3); c.lineTo(20, 8); c.lineTo(15, 8)
            } else if (kind === "arrow") {
                c.moveTo(4, 12); c.lineTo(20, 12)
                c.moveTo(14, 6); c.lineTo(20, 12); c.lineTo(14, 18)
            } else if (kind === "check") {
                c.moveTo(5, 12); c.lineTo(10, 17); c.lineTo(20, 6)
            } else {
                c.moveTo(12, 2); c.lineTo(21, 6); c.lineTo(20, 15)
                c.quadraticCurveTo(18, 20, 12, 23)
                c.quadraticCurveTo(6, 20, 4, 15)
                c.lineTo(3, 6); c.closePath()
                c.moveTo(8, 12); c.lineTo(11, 15); c.lineTo(16, 9)
            }
            c.stroke()
        }
    }

    component ActionButton: Button {
        id: control
        property bool primary: false
        implicitHeight: 42
        leftPadding: 18
        rightPadding: 18
        hoverEnabled: true
        font.weight: Font.DemiBold
        contentItem: Text {
            text: control.text
            font: control.font
            color: control.enabled ? (control.primary ? "#1b1713" : window.ink) : "#696c74"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 8
            color: !control.enabled ? "#26282d" : control.primary ? (control.down ? "#e49454" : control.hovered ? "#ffbc86" : window.accent) : control.hovered ? "#33363e" : "#272a31"
            border.color: control.activeFocus ? window.accent : control.primary && control.enabled ? window.accent : "#3a3d45"
            border.width: control.activeFocus ? 2 : 1
            Behavior on color { ColorAnimation { duration: 100 } }
        }
    }

    component Toggle: CheckBox {
        id: toggle
        spacing: 10
        indicator: Rectangle {
            implicitWidth: 20
            implicitHeight: 20
            x: toggle.leftPadding
            y: (toggle.height - height) / 2
            radius: 5
            color: toggle.checked ? window.accent : "#191b20"
            border.color: toggle.activeFocus ? window.ink : toggle.checked ? window.accent : "#53565f"
            Symbol { anchors.centerIn: parent; width: 20; height: 20; scale: 0.65; kind: "check"; tint: "#17191c"; visible: toggle.checked }
        }
        contentItem: Text {
            text: toggle.text
            font: toggle.font
            color: toggle.enabled ? window.ink : window.muted
            leftPadding: toggle.indicator.width + toggle.spacing
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }

    FileDialog {
        id: imageDialog
        title: "Velg et diskbilde"
        nameFilters: ["Diskbilder (*.iso *.img *.ISO *.IMG)"]
        fileMode: FileDialog.OpenFile
        onAccepted: backend.selectImage(selectedFile)
    }

    Rectangle {
        id: sidebar
        width: 206
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        color: "#17191e"
        Rectangle { width: 1; anchors { right: parent.right; top: parent.top; bottom: parent.bottom } color: "#292c32" }
        ColumnLayout {
            anchors { fill: parent; margins: 24 }
            spacing: 0
            RowLayout {
                spacing: 10
                Rectangle {
                    width: 33; height: 39; radius: 9; color: window.accent
                    Symbol { anchors.centerIn: parent; kind: "usb"; tint: "#18191d" }
                }
                Column {
                    Text { text: "IsoWizard"; font { pixelSize: 15; weight: Font.Bold; letterSpacing: -0.4 } color: window.ink }
                    Text { text: "BOOT"; font { pixelSize: 12; weight: Font.Medium; letterSpacing: 4 } color: window.accent }
                }
            }
            Text { Layout.topMargin: 48; text: "ARBEIDSOMRÅDE"; color: "#696e79"; font { pixelSize: 9; weight: Font.Bold; letterSpacing: 1.3 } }
            Rectangle {
                Layout.topMargin: 14; Layout.fillWidth: true; height: 42; radius: 7; color: "#2c2724"
                Row { anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 } spacing: 8
                    Symbol { kind: "usb"; scale: 0.8 }
                    Text { text: "Skriv USB"; color: window.accent; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
                }
            }
            Text { Layout.topMargin: 32; text: "TRE ENKLE STEG"; color: "#696e79"; font { pixelSize: 9; weight: Font.Bold; letterSpacing: 1.3 } }
            Repeater {
                model: ["Velg bildefil", "Velg USB-enhet", "Skriv og start"]
                delegate: RowLayout {
                    required property int index
                    required property string modelData
                    Layout.topMargin: 19
                    spacing: 10
                    readonly property bool passed: index === 0 ? backend.imageSize > 0 : index === 1 ? window.selectedDevice !== null : backend.stage === "complete"
                    Rectangle {
                        width: 22; height: 22; radius: 11; color: parent.passed ? "#284138" : "#24272d"
                        Text { anchors.centerIn: parent; text: parent.parent.passed ? "✓" : (index + 1); color: parent.parent.passed ? "#9cd5b6" : "#8a8e98"; font.pixelSize: 11 }
                    }
                    Text { text: modelData; color: parent.passed ? "#d7d9d8" : "#818691"; font.pixelSize: 11 }
                }
            }
            Item { Layout.fillHeight: true }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#2c2f35" }
            RowLayout {
                Layout.topMargin: 20; spacing: 7
                Rectangle { width: 6; height: 6; radius: 3; color: "#9cd5b6" }
                Text { text: "Lokalt. Enkelt. Klart."; color: "#a2a6ae"; font.pixelSize: 10 }
            }
            Text { Layout.topMargin: 8; text: "IsoWizardBoot 1.0  /  Linux"; color: "#626874"; font.pixelSize: 10 }
        }
    }

    ScrollView {
        id: scroll
        anchors { left: sidebar.right; right: parent.right; top: parent.top; bottom: parent.bottom }
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 0
            Item { Layout.preferredHeight: 29 }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 34; Layout.rightMargin: 34
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "ET NYTT SYSTEM STARTER HER"; color: window.accent; font { pixelSize: 10; weight: Font.DemiBold; letterSpacing: 1.7 } }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        implicitWidth: 86; implicitHeight: 25; radius: 12; color: "#20252a"; border.color: "#343b40"
                        Text { anchors.centerIn: parent; text: "ISO → USB"; color: "#bec8cc"; font { pixelSize: 10; weight: Font.Medium } }
                    }
                }
                Text { Layout.fillWidth: true; text: "Gjør USB-enheten oppstartsklar."; wrapMode: Text.WordWrap; color: window.ink; font { pixelSize: window.width >= 990 ? 28 : 23; weight: Font.DemiBold; letterSpacing: -0.7 } }
                Text { Layout.fillWidth: true; text: "Velg et diskbilde. Koble til en USB-enhet. Vi tar oss av skrivingen."; wrapMode: Text.WordWrap; color: window.muted; font.pixelSize: 12 }

                Rectangle {
                    Layout.topMargin: 19; Layout.fillWidth: true
                    implicitHeight: imageContent.implicitHeight + 36
                    color: drop.containsDrag ? "#2b2926" : "#1b1e24"; radius: 12
                    border.color: drop.containsDrag ? window.accent : window.line
                    DropArea {
                        id: drop
                        anchors.fill: parent
                        enabled: !backend.busy
                        onDropped: function(event) { if (event.hasUrls && event.urls.length === 1) backend.selectImage(event.urls[0]) }
                    }
                    ColumnLayout {
                        id: imageContent
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 18 }
                        spacing: 14
                        RowLayout {
                            spacing: 9
                            Text { text: "01"; color: window.accent; font { pixelSize: 11; weight: Font.Bold } }
                            Text { text: "Bildefil"; color: window.ink; font { pixelSize: 14; weight: Font.DemiBold } }
                            Item { Layout.fillWidth: true }
                            Text { text: ".ISO / .IMG"; color: "#777d88"; font { pixelSize: 9; letterSpacing: 1 } }
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 14
                            Rectangle {
                                width: 48; height: 48; radius: 10; color: "#302923"
                                Symbol { anchors.centerIn: parent; kind: "disc" }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                Text { Layout.fillWidth: true; text: backend.imageName || "Velg eller slipp en bildefil her"; elide: Text.ElideMiddle; color: window.ink; font { pixelSize: 13; weight: Font.Medium } }
                                Text { Layout.fillWidth: true; text: backend.imageName ? backend.imageSizeText + "  ·  " + backend.imagePath : "Ukomprimerte ISO- og IMG-filer"; elide: Text.ElideMiddle; color: window.muted; font.pixelSize: 11 }
                            }
                            ActionButton { text: "Velg fil …"; enabled: !backend.busy; onClicked: imageDialog.open() }
                        }
                    }
                }

                Rectangle {
                    Layout.topMargin: 5; Layout.fillWidth: true
                    implicitHeight: deviceContent.implicitHeight + 36
                    color: "#1b1e24"; radius: 12; border.color: window.line
                    ColumnLayout {
                        id: deviceContent
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 18 }
                        spacing: 12
                        RowLayout {
                            spacing: 9
                            Text { text: "02"; color: window.accent; font { pixelSize: 11; weight: Font.Bold } }
                            Text { text: "USB-enhet"; color: window.ink; font { pixelSize: 14; weight: Font.DemiBold } }
                            Item { Layout.fillWidth: true }
                            Text { text: backend.devices.length + " funnet"; color: window.muted; font.pixelSize: 11 }
                            Button {
                                implicitWidth: 28; implicitHeight: 28; enabled: !backend.busy
                                padding: 2
                                Accessible.name: "Oppdater USB-enheter"
                                ToolTip.visible: hovered; ToolTip.text: "Oppdater enhetslisten"
                                background: Rectangle { radius: 5; color: parent.hovered ? "#343840" : "transparent"; border.color: parent.activeFocus ? window.accent : "transparent" }
                                contentItem: Symbol { kind: "refresh"; tint: window.muted; scale: 0.7 }
                                onClicked: backend.refreshDevices()
                            }
                        }
                        ComboBox {
                            id: usbSelect
                            Layout.fillWidth: true; implicitHeight: 46
                            enabled: !backend.busy && backend.devices.length > 0
                            model: backend.devices
                            textRole: "label"
                            currentIndex: -1
                            Accessible.name: "Velg USB-enhet som skal overskrives"
                            onActivated: function(index) { window.selectedPath = backend.devices[index].path; window.updateSelection() }
                            contentItem: Text {
                                leftPadding: 14; rightPadding: 34
                                text: usbSelect.currentIndex >= 0 ? usbSelect.displayText : backend.devices.length ? "Velg USB-enhet …" : "Koble til en USB-enhet …"
                                color: usbSelect.currentIndex >= 0 ? window.ink : window.muted
                                font.pixelSize: 12; elide: Text.ElideMiddle; verticalAlignment: Text.AlignVCenter
                            }
                            indicator: Text { x: parent.width - 27; y: 12; text: "⌄"; color: window.muted; font.pixelSize: 17 }
                            background: Rectangle { radius: 8; color: "#13161b"; border.color: usbSelect.activeFocus ? window.accent : "#3b3f48" }
                            popup: Popup {
                                y: usbSelect.height + 5; width: usbSelect.width; padding: 5
                                implicitHeight: Math.min(contentItem.implicitHeight + 10, 220)
                                background: Rectangle { color: "#262a32"; border.color: "#484d58"; radius: 8 }
                                contentItem: ListView { clip: true; implicitHeight: contentHeight; model: usbSelect.popup.visible ? usbSelect.delegateModel : null; currentIndex: usbSelect.highlightedIndex; ScrollIndicator.vertical: ScrollIndicator {} }
                            }
                            delegate: ItemDelegate {
                                required property var modelData
                                required property int index
                                width: usbSelect.width - 10
                                contentItem: Text { text: modelData.label; color: window.ink; font.pixelSize: 12; elide: Text.ElideMiddle; verticalAlignment: Text.AlignVCenter }
                                background: Rectangle { radius: 5; color: parent.highlighted ? "#464038" : "transparent" }
                                highlighted: usbSelect.highlightedIndex === index
                            }
                        }
                        Text {
                            Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 11
                            color: backend.scanError || window.tooSmall || (window.selectedDevice && window.selectedDevice.mounted) ? "#ffb683" : window.muted
                            text: backend.scanError || (window.tooSmall ? "Denne USB-enheten er for liten for den valgte bildefilen."
                                : window.selectedDevice && window.selectedDevice.mounted ? "Enheten er montert. Avmonter partisjonene i filbehandleren først."
                                : window.selectedDevice ? window.selectedDevice.path + "  ·  " + window.selectedDevice.sizeText + (window.selectedDevice.removable ? "  ·  Flyttbar enhet" : "  ·  Ekstern disk — kontroller målet nøye")
                                : "Enhetslisten oppdateres automatisk. Systemdisker er skjult.")
                        }
                    }
                }

                RowLayout {
                    Layout.topMargin: 9; Layout.fillWidth: true; spacing: 12
                    Symbol { kind: "shield"; tint: "#a1b3aa"; scale: 0.8 }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 3
                        Toggle { id: verifyToggle; text: "Verifiser etter skriving"; checked: true; enabled: !backend.busy }
                        Text { Layout.fillWidth: true; text: "Leser USB-innholdet tilbake og sammenligner SHA-256."; color: window.muted; font.pixelSize: 10; wrapMode: Text.WordWrap }
                    }
                    Text { text: "ANBEFALT"; color: "#a1b3aa"; font { pixelSize: 8; letterSpacing: 1.1; weight: Font.DemiBold } }
                }
                Text {
                    Layout.topMargin: 5; Layout.fillWidth: true
                    text: "Direkte diskskriving · Partisjoner og oppstartsmodus følger bildefilen. Bruk USB-kompatible hybrid-ISO-er eller IMG-filer. Vanlige Windows-ISO-er støttes ikke."
                    color: "#7e8490"; font.pixelSize: 10; wrapMode: Text.WordWrap; lineHeight: 1.25
                }

                Rectangle { Layout.topMargin: 10; Layout.fillWidth: true; height: 1; color: window.line }
                RowLayout {
                    Layout.topMargin: 10; Layout.fillWidth: true; spacing: 10
                    Rectangle { width: 7; height: 7; radius: 4; color: backend.stage === "error" ? "#ec8e82" : backend.stage === "complete" ? "#9cd5b6" : window.accent
                        SequentialAnimation on opacity { running: backend.busy; loops: Animation.Infinite; NumberAnimation { to: 0.35; duration: 700 } NumberAnimation { to: 1; duration: 700 } }
                    }
                    Text { text: backend.status; color: window.ink; font { pixelSize: 13; weight: Font.DemiBold } Layout.fillWidth: true }
                    Text { text: Math.round(backend.progress * 100) + "%"; color: backend.stage === "complete" ? "#9cd5b6" : window.muted; font { pixelSize: 15; family: "monospace" } }
                }
                ProgressBar {
                    id: progressBar
                    Layout.fillWidth: true; Layout.topMargin: 5
                    value: backend.progress
                    indeterminate: backend.stage === "authorizing" || backend.stage === "syncing"
                    background: Rectangle { implicitHeight: 7; color: "#2b2e35"; radius: 4 }
                    contentItem: Item {
                        implicitHeight: 7
                        Rectangle { width: progressBar.visualPosition * parent.width; height: 7; radius: 4; color: backend.stage === "complete" ? "#9cd5b6" : window.accent; visible: !progressBar.indeterminate; Behavior on width { NumberAnimation { duration: 150 } } }
                        Rectangle {
                            width: parent.width * 0.24; height: 7; radius: 4; color: window.accent; visible: progressBar.indeterminate
                            SequentialAnimation on x { running: progressBar.indeterminate; loops: Animation.Infinite; NumberAnimation { from: 0; to: progressBar.width * 0.76; duration: 1100; easing.type: Easing.InOutQuad } NumberAnimation { to: 0; duration: 1100; easing.type: Easing.InOutQuad } }
                        }
                    }
                }
                Text { Layout.fillWidth: true; Layout.minimumHeight: 30; text: backend.detail; color: backend.stage === "error" ? "#ecac9e" : window.muted; font.pixelSize: 11; wrapMode: Text.WordWrap }
                RowLayout {
                    Layout.topMargin: 3; Layout.fillWidth: true
                    Button {
                        text: "Vis aktivitetslogg ↗"; flat: true
                        contentItem: Text { text: parent.text; color: parent.hovered ? window.ink : window.muted; font.pixelSize: 11 }
                        background: Rectangle { color: "transparent"; border.color: parent.activeFocus ? window.accent : "transparent"; radius: 4 }
                        onClicked: logDialog.open()
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton { text: "Avbryt"; visible: backend.busy; onClicked: stopDialog.open() }
                    ActionButton {
                        objectName: "writeButton"
                        text: "Skriv til USB  →"; primary: true; implicitHeight: 46
                        enabled: window.ready
                        onClicked: {
                            window.confirmationPath = window.selectedPath
                            window.confirmationIdentity = JSON.stringify(window.selectedDevice)
                            window.confirmVerify = verifyToggle.checked
                            eraseCheck.checked = false
                            confirmDialog.open()
                        }
                    }
                }
                Text { Layout.fillWidth: true; Layout.topMargin: 3; text: "Alt innhold på valgt USB-enhet slettes. Du bekrefter før skrivingen starter."; color: "#777d87"; font.pixelSize: 10; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignRight }
            }
            Item { Layout.preferredHeight: 26 }
        }
    }

    component DarkDialog: Dialog {
        anchors.centerIn: parent
        width: Math.min(window.width - 60, 520)
        modal: true
        padding: 24
        closePolicy: Popup.CloseOnEscape
        background: Rectangle { color: "#202329"; radius: 14; border.color: "#464a53" }
        header: Label { text: parent.title; color: window.ink; font { pixelSize: 21; weight: Font.DemiBold } padding: 24; bottomPadding: 4; wrapMode: Text.WordWrap }
        Overlay.modal: Rectangle { color: "#bd06080d" }
    }

    DarkDialog {
        id: confirmDialog
        objectName: "confirmDialog"
        title: "Slette og skrive til denne enheten?"
        contentItem: ColumnLayout {
            spacing: 17
            Text { Layout.fillWidth: true; text: "Alle partisjoner og filer på enheten blir overskrevet. Kontroller at du har valgt riktig USB-enhet."; color: "#d0b8a4"; wrapMode: Text.WordWrap }
            Rectangle {
                Layout.fillWidth: true; implicitHeight: confirmationDetails.implicitHeight + 28; color: "#15181d"; radius: 8
                ColumnLayout {
                    id: confirmationDetails
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 } spacing: 7
                    Text { Layout.fillWidth: true; text: window.confirmationIdentity ? JSON.parse(window.confirmationIdentity).name : ""; color: window.ink; font.weight: Font.Bold; wrapMode: Text.WordWrap }
                    Text { Layout.fillWidth: true; text: window.confirmationIdentity ? window.confirmationPath + "  ·  " + JSON.parse(window.confirmationIdentity).sizeText : ""; color: window.accent; wrapMode: Text.WordWrap }
                    Text { Layout.fillWidth: true; text: "Bildefil: " + backend.imageName; color: window.muted; elide: Text.ElideMiddle }
                }
            }
            Toggle { id: eraseCheck; objectName: "eraseCheck"; Layout.fillWidth: true; text: "Jeg forstår at alt innhold på denne enheten slettes." }
            Text { Layout.fillWidth: true; text: "Systemet ber deretter om administratorgodkjenning."; color: window.muted; font.pixelSize: 11; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                ActionButton { text: "Tilbake"; onClicked: confirmDialog.close() }
                ActionButton {
                    objectName: "confirmWriteButton"
                    text: "Slett og skriv"; primary: true
                    enabled: eraseCheck.checked && window.confirmationIdentity === JSON.stringify(window.selectedDevice)
                    onClicked: { confirmDialog.close(); backend.start(window.confirmationPath, JSON.parse(window.confirmationIdentity).identity, window.confirmVerify) }
                }
            }
        }
    }

    DarkDialog {
        id: stopDialog
        title: "Avbryte operasjonen?"
        contentItem: ColumnLayout {
            spacing: 20
            Text { Layout.fillWidth: true; text: "En avbrutt skriving kan etterlate USB-enheten ufullstendig. Vent til appen melder at operasjonen er avbrutt før du kobler den fra."; color: window.muted; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                ActionButton { text: "Fortsett"; onClicked: stopDialog.close() }
                ActionButton { text: "Avbryt operasjonen"; primary: true; onClicked: { stopDialog.close(); backend.cancel() } }
            }
        }
    }

    DarkDialog {
        id: logDialog
        title: "Aktivitetslogg"
        width: Math.min(window.width - 60, 720)
        contentItem: ColumnLayout {
            spacing: 16
            ScrollView {
                Layout.fillWidth: true; Layout.preferredHeight: 300
                TextArea { text: backend.log || "Ingen operasjoner ennå."; readOnly: true; selectByMouse: true; wrapMode: TextEdit.Wrap; color: "#c5cbd4"; font { family: "monospace"; pixelSize: 11 } background: Rectangle { color: "#14171c"; radius: 6 } }
            }
            ActionButton { text: "Lukk"; Layout.alignment: Qt.AlignRight; onClicked: logDialog.close() }
        }
    }
}
