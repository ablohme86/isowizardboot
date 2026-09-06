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
    title: qsTr("IsoWizardBoot — USB image writer")
    color: window.page
    font.family: "Noto Sans"
    font.pixelSize: 13

    readonly property bool lightTheme: settings.theme === "light"
    readonly property color page: lightTheme ? "#f7f7f4" : "#101216"
    readonly property color sidebarColor: lightTheme ? "#eeefea" : "#17191e"
    readonly property color card: lightTheme ? "#ffffff" : "#1b1e24"
    readonly property color field: lightTheme ? "#fafaf7" : "#13161b"
    readonly property color ink: lightTheme ? "#242a2f" : "#f3f0e9"
    readonly property color muted: lightTheme ? "#616973" : "#92969f"
    readonly property color accent: lightTheme ? "#b35a20" : "#ffac69"
    readonly property color line: lightTheme ? "#dbddd6" : "#30333a"
    readonly property color quiet: lightTheme ? "#687079" : "#777d88"
    readonly property color secondary: lightTheme ? "#4c5860" : "#bec8cc"
    readonly property color hover: lightTheme ? "#e1e3dc" : "#33363e"
    readonly property color buttonBase: lightTheme ? "#e9eae4" : "#272a31"
    readonly property color border: lightTheme ? "#c7cbc2" : "#3b3f48"
    readonly property color activeBackground: lightTheme ? "#f5e6d8" : "#302923"
    readonly property color success: lightTheme ? "#27744a" : "#9cd5b6"
    readonly property color successBackground: lightTheme ? "#dcecdf" : "#284138"
    readonly property color successMuted: lightTheme ? "#466b55" : "#a1b3aa"
    readonly property color error: lightTheme ? "#b34437" : "#ecac9e"
    readonly property color warning: lightTheme ? "#92501f" : "#d0b8a4"
    readonly property color badge: lightTheme ? "#e9ede7" : "#20252a"
    readonly property color badgeBorder: lightTheme ? "#d1d9cc" : "#343b40"
    readonly property color popupColor: lightTheme ? "#ffffff" : "#262a32"
    readonly property color progressTrack: lightTheme ? "#dee2d9" : "#2b2e35"
    readonly property color disabledText: lightTheme ? "#81877e" : "#696c74"
    readonly property color buttonInk: lightTheme ? "#ffffff" : "#1b1713"
    readonly property color buttonPressed: lightTheme ? "#914113" : "#e49454"
    readonly property color buttonHover: lightTheme ? "#c86b29" : "#ffbc86"
    readonly property color overlayColor: lightTheme ? "#800d1520" : "#bd06080d"

    property string selectedPath: ""
    property var selectedDevice: null
    property string confirmationPath: ""
    property string confirmationIdentity: ""
    property bool confirmVerify: true
    readonly property bool tooSmall: selectedDevice !== null && backend.imageSize > selectedDevice.size
    readonly property bool ready: backend.imageSize > 0 && selectedDevice !== null && !tooSmall && !selectedDevice.mounted && !backend.busy

    function updateSelection() {
        let selected = null;
        for (let i = 0; i < backend.devices.length; ++i) {
            if (backend.devices[i].path === selectedPath) {
                selected = backend.devices[i];
                usbSelect.currentIndex = i;
                break;
            }
        }
        if (selected === null) {
            selectedPath = "";
            usbSelect.currentIndex = -1;
        }
        selectedDevice = selected;
    }

    Connections {
        target: backend
        function onDevicesChanged() {
            window.updateSelection();
        }
    }

    onClosing: function (close) {
        if (backend.busy) {
            close.accepted = false;
            stopDialog.open();
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
            let c = getContext("2d");
            c.reset();
            c.strokeStyle = tint;
            c.fillStyle = tint;
            c.lineWidth = 1.6;
            c.lineCap = "round";
            c.lineJoin = "round";
            c.beginPath();
            if (kind === "disc") {
                c.arc(12, 12, 9, 0, Math.PI * 2);
                c.moveTo(15, 12);
                c.arc(12, 12, 3, 0, Math.PI * 2);
                c.moveTo(7, 7);
                c.lineTo(9, 5);
            } else if (kind === "usb") {
                c.rect(7, 9, 10, 12);
                c.rect(9, 2, 6, 7);
                c.moveTo(11, 5);
                c.lineTo(13, 5);
                c.moveTo(10, 17);
                c.lineTo(14, 17);
            } else if (kind === "refresh") {
                c.arc(12, 12, 8, -0.6, 4.8);
                c.moveTo(19, 3);
                c.lineTo(20, 8);
                c.lineTo(15, 8);
            } else if (kind === "arrow") {
                c.moveTo(4, 12);
                c.lineTo(20, 12);
                c.moveTo(14, 6);
                c.lineTo(20, 12);
                c.lineTo(14, 18);
            } else if (kind === "check") {
                c.moveTo(5, 12);
                c.lineTo(10, 17);
                c.lineTo(20, 6);
            } else {
                c.moveTo(12, 2);
                c.lineTo(21, 6);
                c.lineTo(20, 15);
                c.quadraticCurveTo(18, 20, 12, 23);
                c.quadraticCurveTo(6, 20, 4, 15);
                c.lineTo(3, 6);
                c.closePath();
                c.moveTo(8, 12);
                c.lineTo(11, 15);
                c.lineTo(16, 9);
            }
            c.stroke();
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
            color: control.enabled ? (control.primary ? window.buttonInk : window.ink) : window.disabledText
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 8
            color: !control.enabled ? window.buttonBase : control.primary ? (control.down ? window.buttonPressed : control.hovered ? window.buttonHover : window.accent) : control.hovered ? window.hover : window.buttonBase
            border.color: control.activeFocus ? window.accent : control.primary && control.enabled ? window.accent : window.border
            border.width: control.activeFocus ? 2 : 1
            Behavior on color {
                ColorAnimation {
                    duration: 100
                }
            }
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
            color: toggle.checked ? window.accent : window.field
            border.color: toggle.activeFocus ? window.ink : toggle.checked ? window.accent : window.border
            Symbol {
                anchors.centerIn: parent
                width: 20
                height: 20
                scale: 0.65
                kind: "check"
                tint: window.buttonInk
                visible: toggle.checked
            }
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
        title: qsTr("Choose a disk image")
        nameFilters: [qsTr("Disk images (*.iso *.img *.ISO *.IMG)")]
        fileMode: FileDialog.OpenFile
        onAccepted: backend.selectImage(selectedFile)
    }

    Rectangle {
        id: sidebar
        width: 206
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        color: window.sidebarColor
        Rectangle {
            width: 1
            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            color: window.line
        }
        ColumnLayout {
            anchors {
                fill: parent
                margins: 24
            }
            spacing: 0
            RowLayout {
                spacing: 10
                Rectangle {
                    width: 33
                    height: 39
                    radius: 9
                    color: window.accent
                    Symbol {
                        anchors.centerIn: parent
                        kind: "usb"
                        tint: window.buttonInk
                    }
                }
                Column {
                    Text {
                        text: "IsoWizard"
                        font {
                            pixelSize: 15
                            weight: Font.Bold
                            letterSpacing: -0.4
                        }
                        color: window.ink
                    }
                    Text {
                        text: "BOOT"
                        font {
                            pixelSize: 12
                            weight: Font.Medium
                            letterSpacing: 4
                        }
                        color: window.accent
                    }
                }
            }
            Text {
                Layout.topMargin: 48
                text: qsTr("WORKSPACE")
                color: window.quiet
                font {
                    pixelSize: 9
                    weight: Font.Bold
                    letterSpacing: 1.3
                }
            }
            Rectangle {
                Layout.topMargin: 14
                Layout.fillWidth: true
                height: 42
                radius: 7
                color: window.activeBackground
                Row {
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left
                        leftMargin: 10
                    }
                    spacing: 8
                    Symbol {
                        kind: "usb"
                        scale: 0.8
                    }
                    Text {
                        text: qsTr("Write USB")
                        color: window.accent
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            Text {
                Layout.topMargin: 32
                text: qsTr("THREE SIMPLE STEPS")
                color: window.quiet
                font {
                    pixelSize: 9
                    weight: Font.Bold
                    letterSpacing: 1.3
                }
            }
            Repeater {
                model: [qsTr("Choose an image"), qsTr("Choose a USB device"), qsTr("Write and boot")]
                delegate: RowLayout {
                    required property int index
                    required property string modelData
                    Layout.topMargin: 19
                    spacing: 10
                    readonly property bool passed: index === 0 ? backend.imageSize > 0 : index === 1 ? window.selectedDevice !== null : backend.stage === "complete"
                    Rectangle {
                        width: 22
                        height: 22
                        radius: 11
                        color: parent.passed ? window.successBackground : window.buttonBase
                        Text {
                            anchors.centerIn: parent
                            text: parent.parent.passed ? "✓" : (index + 1)
                            color: parent.parent.passed ? window.success : window.quiet
                            font.pixelSize: 11
                        }
                    }
                    Text {
                        text: modelData
                        color: parent.passed ? window.secondary : window.quiet
                        font.pixelSize: 11
                    }
                }
            }
            Item {
                Layout.fillHeight: true
            }
            ActionButton {
                objectName: "settingsButton"
                Layout.fillWidth: true
                Layout.bottomMargin: 18
                text: qsTr("Settings")
                onClicked: settingsDialog.open()
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: window.line
            }
            RowLayout {
                Layout.topMargin: 20
                spacing: 7
                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: window.success
                }
                Text {
                    text: qsTr("Local. Simple. Ready.")
                    color: window.secondary
                    font.pixelSize: 10
                }
            }
            Text {
                Layout.topMargin: 8
                text: "IsoWizardBoot 1.0  /  Linux"
                color: window.quiet
                font.pixelSize: 10
            }
        }
    }

    ScrollView {
        id: scroll
        anchors {
            left: sidebar.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            width: scroll.availableWidth
            spacing: 0
            Item {
                Layout.preferredHeight: 29
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 34
                Layout.rightMargin: 34
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("A FRESH START BEGINS HERE")
                        color: window.accent
                        font {
                            pixelSize: 10
                            weight: Font.DemiBold
                            letterSpacing: 1.7
                        }
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        implicitWidth: 86
                        implicitHeight: 25
                        radius: 12
                        color: window.badge
                        border.color: window.badgeBorder
                        Text {
                            anchors.centerIn: parent
                            text: "ISO → USB"
                            color: window.secondary
                            font {
                                pixelSize: 10
                                weight: Font.Medium
                            }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Make your USB ready to boot.")
                    wrapMode: Text.WordWrap
                    color: window.ink
                    font {
                        pixelSize: window.width >= 990 ? 28 : 23
                        weight: Font.DemiBold
                        letterSpacing: -0.7
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Choose an image. Connect a USB device. We'll handle the writing.")
                    wrapMode: Text.WordWrap
                    color: window.muted
                    font.pixelSize: 12
                }

                Rectangle {
                    Layout.topMargin: 19
                    Layout.fillWidth: true
                    implicitHeight: imageContent.implicitHeight + 36
                    color: drop.containsDrag ? window.activeBackground : window.card
                    radius: 12
                    border.color: drop.containsDrag ? window.accent : window.line
                    DropArea {
                        id: drop
                        anchors.fill: parent
                        enabled: !backend.busy
                        onDropped: function (event) {
                            if (event.hasUrls && event.urls.length === 1)
                                backend.selectImage(event.urls[0]);
                        }
                    }
                    ColumnLayout {
                        id: imageContent
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            margins: 18
                        }
                        spacing: 14
                        RowLayout {
                            spacing: 9
                            Text {
                                text: "01"
                                color: window.accent
                                font {
                                    pixelSize: 11
                                    weight: Font.Bold
                                }
                            }
                            Text {
                                text: qsTr("Image file")
                                color: window.ink
                                font {
                                    pixelSize: 14
                                    weight: Font.DemiBold
                                }
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                            Text {
                                text: ".ISO / .IMG"
                                color: window.quiet
                                font {
                                    pixelSize: 9
                                    letterSpacing: 1
                                }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14
                            Rectangle {
                                width: 48
                                height: 48
                                radius: 10
                                color: window.activeBackground
                                Symbol {
                                    anchors.centerIn: parent
                                    kind: "disc"
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text {
                                    Layout.fillWidth: true
                                    text: backend.imageName || qsTr("Choose or drop an image here")
                                    elide: Text.ElideMiddle
                                    color: window.ink
                                    font {
                                        pixelSize: 13
                                        weight: Font.Medium
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: backend.imageName ? backend.imageSizeText + "  ·  " + backend.imagePath : qsTr("Uncompressed ISO and IMG files")
                                    elide: Text.ElideMiddle
                                    color: window.muted
                                    font.pixelSize: 11
                                }
                            }
                            ActionButton {
                                text: qsTr("Choose file …")
                                enabled: !backend.busy
                                onClicked: imageDialog.open()
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.topMargin: 5
                    Layout.fillWidth: true
                    implicitHeight: deviceContent.implicitHeight + 36
                    color: window.card
                    radius: 12
                    border.color: window.line
                    ColumnLayout {
                        id: deviceContent
                        anchors {
                            left: parent.left
                            right: parent.right
                            top: parent.top
                            margins: 18
                        }
                        spacing: 12
                        RowLayout {
                            spacing: 9
                            Text {
                                text: "02"
                                color: window.accent
                                font {
                                    pixelSize: 11
                                    weight: Font.Bold
                                }
                            }
                            Text {
                                text: qsTr("USB device")
                                color: window.ink
                                font {
                                    pixelSize: 14
                                    weight: Font.DemiBold
                                }
                            }
                            Item {
                                Layout.fillWidth: true
                            }
                            Text {
                                text: backend.devices.length + qsTr(" found")
                                color: window.muted
                                font.pixelSize: 11
                            }
                            Button {
                                implicitWidth: 28
                                implicitHeight: 28
                                enabled: !backend.busy
                                padding: 2
                                Accessible.name: qsTr("Refresh USB devices")
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Refresh the device list")
                                background: Rectangle {
                                    radius: 5
                                    color: parent.hovered ? window.hover : "transparent"
                                    border.color: parent.activeFocus ? window.accent : "transparent"
                                }
                                contentItem: Symbol {
                                    kind: "refresh"
                                    tint: window.muted
                                    scale: 0.7
                                }
                                onClicked: backend.refreshDevices()
                            }
                        }
                        ComboBox {
                            id: usbSelect
                            Layout.fillWidth: true
                            implicitHeight: 46
                            enabled: !backend.busy && backend.devices.length > 0
                            model: backend.devices
                            textRole: "label"
                            currentIndex: -1
                            Accessible.name: qsTr("Choose the USB device to overwrite")
                            onActivated: function (index) {
                                window.selectedPath = backend.devices[index].path;
                                window.updateSelection();
                            }
                            contentItem: Text {
                                leftPadding: 14
                                rightPadding: 34
                                text: usbSelect.currentIndex >= 0 ? usbSelect.displayText : backend.devices.length ? qsTr("Choose a USB device …") : qsTr("Connect a USB device …")
                                color: usbSelect.currentIndex >= 0 ? window.ink : window.muted
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                                verticalAlignment: Text.AlignVCenter
                            }
                            indicator: Text {
                                x: parent.width - 27
                                y: 12
                                text: "⌄"
                                color: window.muted
                                font.pixelSize: 17
                            }
                            background: Rectangle {
                                radius: 8
                                color: window.field
                                border.color: usbSelect.activeFocus ? window.accent : window.border
                            }
                            popup: Popup {
                                y: usbSelect.height + 5
                                width: usbSelect.width
                                padding: 5
                                implicitHeight: Math.min(contentItem.implicitHeight + 10, 220)
                                background: Rectangle {
                                    color: window.popupColor
                                    border.color: window.border
                                    radius: 8
                                }
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: usbSelect.popup.visible ? usbSelect.delegateModel : null
                                    currentIndex: usbSelect.highlightedIndex
                                    ScrollIndicator.vertical: ScrollIndicator {}
                                }
                            }
                            delegate: ItemDelegate {
                                required property var modelData
                                required property int index
                                width: usbSelect.width - 10
                                contentItem: Text {
                                    text: modelData.label
                                    color: window.ink
                                    font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 5
                                    color: parent.highlighted ? window.activeBackground : "transparent"
                                }
                                highlighted: usbSelect.highlightedIndex === index
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            font.pixelSize: 11
                            color: backend.scanError || window.tooSmall || (window.selectedDevice && window.selectedDevice.mounted) ? window.warning : window.muted
                            text: backend.scanError || (window.tooSmall ? qsTr("This USB device is too small for the selected image.") : window.selectedDevice && window.selectedDevice.mounted ? qsTr("The device is mounted. Unmount its partitions in your file manager first.") : window.selectedDevice ? window.selectedDevice.path + "  ·  " + window.selectedDevice.sizeText + (window.selectedDevice.removable ? qsTr("  ·  Removable device") : qsTr("  ·  External disk — check the target carefully")) : qsTr("Devices refresh automatically. System disks are hidden."))
                        }
                    }
                }

                RowLayout {
                    Layout.topMargin: 9
                    Layout.fillWidth: true
                    spacing: 12
                    Symbol {
                        kind: "shield"
                        tint: window.successMuted
                        scale: 0.8
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3
                        Toggle {
                            id: verifyToggle
                            text: qsTr("Verify after writing")
                            checked: true
                            enabled: !backend.busy
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Reads back the USB contents and compares SHA-256 hashes.")
                            color: window.muted
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                        }
                    }
                    Text {
                        text: qsTr("RECOMMENDED")
                        color: window.successMuted
                        font {
                            pixelSize: 8
                            letterSpacing: 1.1
                            weight: Font.DemiBold
                        }
                    }
                }
                Text {
                    Layout.topMargin: 5
                    Layout.fillWidth: true
                    text: qsTr("Direct disk writing · Partitions and boot mode follow the image. Use USB-compatible hybrid ISOs or IMG files. Standard Windows ISOs are not supported.")
                    color: window.quiet
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                    lineHeight: 1.25
                }

                Rectangle {
                    Layout.topMargin: 10
                    Layout.fillWidth: true
                    height: 1
                    color: window.line
                }
                RowLayout {
                    Layout.topMargin: 10
                    Layout.fillWidth: true
                    spacing: 10
                    Rectangle {
                        width: 7
                        height: 7
                        radius: 4
                        color: backend.stage === "error" ? window.error : backend.stage === "complete" ? window.success : window.accent
                        SequentialAnimation on opacity {
                            running: backend.busy
                            loops: Animation.Infinite
                            NumberAnimation {
                                to: 0.35
                                duration: 700
                            }
                            NumberAnimation {
                                to: 1
                                duration: 700
                            }
                        }
                    }
                    Text {
                        text: backend.status
                        color: window.ink
                        font {
                            pixelSize: 13
                            weight: Font.DemiBold
                        }
                        Layout.fillWidth: true
                    }
                    Text {
                        text: Math.round(backend.progress * 100) + "%"
                        color: backend.stage === "complete" ? window.success : window.muted
                        font {
                            pixelSize: 15
                            family: "monospace"
                        }
                    }
                }
                ProgressBar {
                    id: progressBar
                    Layout.fillWidth: true
                    Layout.topMargin: 5
                    value: backend.progress
                    indeterminate: backend.stage === "authorizing" || backend.stage === "syncing"
                    background: Rectangle {
                        implicitHeight: 7
                        color: window.progressTrack
                        radius: 4
                    }
                    contentItem: Item {
                        implicitHeight: 7
                        Rectangle {
                            width: progressBar.visualPosition * parent.width
                            height: 7
                            radius: 4
                            color: backend.stage === "complete" ? window.success : window.accent
                            visible: !progressBar.indeterminate
                            Behavior on width {
                                NumberAnimation {
                                    duration: 150
                                }
                            }
                        }
                        Rectangle {
                            width: parent.width * 0.24
                            height: 7
                            radius: 4
                            color: window.accent
                            visible: progressBar.indeterminate
                            SequentialAnimation on x {
                                running: progressBar.indeterminate
                                loops: Animation.Infinite
                                NumberAnimation {
                                    from: 0
                                    to: progressBar.width * 0.76
                                    duration: 1100
                                    easing.type: Easing.InOutQuad
                                }
                                NumberAnimation {
                                    to: 0
                                    duration: 1100
                                    easing.type: Easing.InOutQuad
                                }
                            }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.minimumHeight: 30
                    text: backend.detail
                    color: backend.stage === "error" ? window.error : window.muted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.topMargin: 3
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("View activity log ↗")
                        flat: true
                        contentItem: Text {
                            text: parent.text
                            color: parent.hovered ? window.ink : window.muted
                            font.pixelSize: 11
                        }
                        background: Rectangle {
                            color: "transparent"
                            border.color: parent.activeFocus ? window.accent : "transparent"
                            radius: 4
                        }
                        onClicked: logDialog.open()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    ActionButton {
                        text: qsTr("Cancel")
                        visible: backend.busy
                        onClicked: stopDialog.open()
                    }
                    ActionButton {
                        objectName: "writeButton"
                        text: qsTr("Write to USB  →")
                        primary: true
                        implicitHeight: 46
                        enabled: window.ready
                        onClicked: {
                            window.confirmationPath = window.selectedPath;
                            window.confirmationIdentity = JSON.stringify(window.selectedDevice);
                            window.confirmVerify = verifyToggle.checked;
                            eraseCheck.checked = false;
                            confirmDialog.open();
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 3
                    text: qsTr("All data on the selected USB device will be erased. You'll confirm before writing.")
                    color: window.quiet
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignRight
                }
            }
            Item {
                Layout.preferredHeight: 26
            }
        }
    }

    component DarkDialog: Dialog {
        anchors.centerIn: parent
        width: Math.min(window.width - 60, 520)
        modal: true
        padding: 24
        closePolicy: Popup.CloseOnEscape
        background: Rectangle {
            color: window.card
            radius: 14
            border.color: window.border
        }
        header: Label {
            text: parent.title
            color: window.ink
            font {
                pixelSize: 21
                weight: Font.DemiBold
            }
            padding: 24
            bottomPadding: 4
            wrapMode: Text.WordWrap
        }
        Overlay.modal: Rectangle {
            color: window.overlayColor
        }
    }

    DarkDialog {
        id: settingsDialog
        objectName: "settingsDialog"
        title: qsTr("Settings")
        contentItem: ColumnLayout {
            spacing: 18
            Text {
                text: qsTr("Make yourself at home.")
                color: window.muted
                font.pixelSize: 13
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: window.line
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 7
                Text {
                    text: qsTr("Languages")
                    color: window.ink
                    font {
                        pixelSize: 15
                        weight: Font.DemiBold
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Choose your preferred language. Changes apply immediately.")
                    color: window.muted
                    wrapMode: Text.WordWrap
                    font.pixelSize: 11
                }
                RowLayout {
                    Layout.topMargin: 5
                    Layout.fillWidth: true
                    spacing: 10
                    ActionButton {
                        objectName: "englishButton"
                        Layout.fillWidth: true
                        text: "English"
                        primary: settings.language === "en"
                        onClicked: settings.language = "en"
                    }
                    ActionButton {
                        objectName: "norwegianButton"
                        Layout.fillWidth: true
                        text: "Norsk bokmål"
                        primary: settings.language === "no-NB"
                        onClicked: settings.language = "no-NB"
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 7
                Text {
                    text: qsTr("Appearance")
                    color: window.ink
                    font {
                        pixelSize: 15
                        weight: Font.DemiBold
                    }
                }
                Text {
                    text: qsTr("Choose a theme for your workspace.")
                    color: window.muted
                    font.pixelSize: 11
                }
                RowLayout {
                    Layout.topMargin: 5
                    Layout.fillWidth: true
                    spacing: 10
                    ActionButton {
                        objectName: "darkThemeButton"
                        Layout.fillWidth: true
                        text: qsTr("Dark")
                        primary: settings.theme === "dark"
                        onClicked: settings.theme = "dark"
                    }
                    ActionButton {
                        objectName: "lightThemeButton"
                        Layout.fillWidth: true
                        text: qsTr("Light")
                        primary: settings.theme === "light"
                        onClicked: settings.theme = "light"
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Your preferences are saved automatically.")
                color: window.muted
                wrapMode: Text.WordWrap
                font.pixelSize: 11
            }
            ActionButton {
                text: qsTr("Close")
                Layout.alignment: Qt.AlignRight
                onClicked: settingsDialog.close()
            }
        }
    }

    DarkDialog {
        id: confirmDialog
        objectName: "confirmDialog"
        title: qsTr("Erase and write to this device?")
        contentItem: ColumnLayout {
            spacing: 17
            Text {
                Layout.fillWidth: true
                text: qsTr("Every partition and file on this device will be overwritten. Check that you've selected the correct USB device.")
                color: window.warning
                wrapMode: Text.WordWrap
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: confirmationDetails.implicitHeight + 28
                color: window.field
                radius: 8
                ColumnLayout {
                    id: confirmationDetails
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        margins: 14
                    }
                    spacing: 7
                    Text {
                        Layout.fillWidth: true
                        text: window.confirmationIdentity ? JSON.parse(window.confirmationIdentity).name : ""
                        color: window.ink
                        font.weight: Font.Bold
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: window.confirmationIdentity ? window.confirmationPath + "  ·  " + JSON.parse(window.confirmationIdentity).sizeText : ""
                        color: window.accent
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Image: ") + backend.imageName
                        color: window.muted
                        elide: Text.ElideMiddle
                    }
                }
            }
            Toggle {
                id: eraseCheck
                objectName: "eraseCheck"
                Layout.fillWidth: true
                text: qsTr("I understand that all data on this device will be erased.")
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("The system will then ask for administrator approval.")
                color: window.muted
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                Item {
                    Layout.fillWidth: true
                }
                ActionButton {
                    text: qsTr("Go back")
                    onClicked: confirmDialog.close()
                }
                ActionButton {
                    objectName: "confirmWriteButton"
                    text: qsTr("Erase and write")
                    primary: true
                    enabled: eraseCheck.checked && window.confirmationIdentity === JSON.stringify(window.selectedDevice)
                    onClicked: {
                        confirmDialog.close();
                        backend.start(window.confirmationPath, JSON.parse(window.confirmationIdentity).identity, window.confirmVerify);
                    }
                }
            }
        }
    }

    DarkDialog {
        id: stopDialog
        title: qsTr("Cancel the operation?")
        contentItem: ColumnLayout {
            spacing: 20
            Text {
                Layout.fillWidth: true
                text: qsTr("Cancelling a write can leave the USB device incomplete. Wait for the app to report that the operation was cancelled before disconnecting it.")
                color: window.muted
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                Item {
                    Layout.fillWidth: true
                }
                ActionButton {
                    text: qsTr("Keep going")
                    onClicked: stopDialog.close()
                }
                ActionButton {
                    text: qsTr("Cancel operation")
                    primary: true
                    onClicked: {
                        stopDialog.close();
                        backend.cancel();
                    }
                }
            }
        }
    }

    DarkDialog {
        id: logDialog
        title: qsTr("Activity log")
        width: Math.min(window.width - 60, 720)
        contentItem: ColumnLayout {
            spacing: 16
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                TextArea {
                    text: backend.log || qsTr("No operations yet.")
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    color: window.secondary
                    font {
                        family: "monospace"
                        pixelSize: 11
                    }
                    background: Rectangle {
                        color: window.field
                        radius: 6
                    }
                }
            }
            ActionButton {
                text: qsTr("Close")
                Layout.alignment: Qt.AlignRight
                onClicked: logDialog.close()
            }
        }
    }
}
