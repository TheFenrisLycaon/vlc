/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T

import Heimdall.MainInterface
import Heimdall.Dialogs
import Heimdall.Widgets as Widgets
import Heimdall.Util
import Heimdall.Style

Item {
    id: root

    //---------------------------------------------------------------------------------------------
    // Properties
    //---------------------------------------------------------------------------------------------

    property Item bgContent: null

    //---------------------------------------------------------------------------------------------
    // Signal
    //---------------------------------------------------------------------------------------------

    signal restoreFocus();

    //---------------------------------------------------------------------------------------------
    // Events
    //---------------------------------------------------------------------------------------------

    Component.onCompleted: if (DialogErrorModel.repeatedMessageCount){
                               hideErrorPopupTimer.restart()
                               errorPopup.state = "visible"
                           }

    Component.onDestruction: {
        if (questionDialog.dialogId !== null) {
            heimdallDialog.dismiss(questionDialog.dialogId)
            questionDialog.dialogId = null
        } if (loginDialog.dialogId !== null) {
            heimdallDialog.dismiss(loginDialog.dialogId)
            loginDialog.dialogId = null
        }
    }

    //---------------------------------------------------------------------------------------------
    // Functions
    //---------------------------------------------------------------------------------------------

    function ask(text, acceptCb, rejectCb, buttons) {
        customDialog.ask(text, acceptCb, rejectCb, buttons)
    }

    //---------------------------------------------------------------------------------------------
    // Connections
    //---------------------------------------------------------------------------------------------

    HeimdallDialog
    {
        id: heimdallDialog

        Component.onCompleted: {
            HeimdallDialogModel.provider = heimdallDialog
        }

        Component.onDestruction: {
            // Only clear the provider if we are still the current one.
            if (HeimdallDialogModel.provider === heimdallDialog)
                HeimdallDialogModel.provider = null
        }

        onLogin: (dialogId, title, text, defaultUsername, askStore) => {
            loginDialog.dialogId = dialogId
            loginDialog.title = title
            loginDialog.defaultUsername = defaultUsername
            loginDialog.open()
        }

        onQuestion: (dialogId, title, text, type, cancel, action1, action2) => {
            questionDialog.dialogId = dialogId
            questionDialog.title = title
            questionDialog.text = text
            questionDialog.cancelTxt = cancel
            questionDialog.action1Txt = action1
            questionDialog.action2Txt = action2
            questionDialog.open()
        }

        onProgress: (dialogId, title, text, indeterminate, position, cancel) => {
            progressDialog.dialogId = dialogId
            progressDialog.title = title
            progressDialog.text = text
            progressDialog.cancelTxt = cancel
            progressDialog.position = position
            progressDialog.interderminate = indeterminate
            progressDialog.open()
        }

        onProgressUpdated: (dialogId, position, text) => {
            if (progressDialog.dialogId !== dialogId) {
                console.warn("progress event on an inexisting dialog")
                return
            }
            progressDialog.text = text
            progressDialog.position = position
        }

        onCancelled: (dialogId) => {
            if (questionDialog.dialogId === dialogId) {
                questionDialog.close()
                questionDialog.dialogId = null
                dismiss(dialogId)
            } else if (loginDialog.dialogId === dialogId)  {
                loginDialog.close()
                loginDialog.dialogId = null
                dismiss(dialogId)
            } else if (progressDialog.dialogId === dialogId) {
                progressDialog.close()
                progressDialog.dialogId = null
                dismiss(dialogId)
            } else {
                dismiss(dialogId)
            }
        }
    }

    Connections
    {
        target: DialogErrorModel

        function onCountChanged() {
            hideErrorPopupTimer.restart()
            errorPopup.state = "visible"
        }
    }

    //---------------------------------------------------------------------------------------------
    // Childs
    //---------------------------------------------------------------------------------------------

    Widgets.DrawerExt {
        id: errorPopup

        property string messageText
        property int repeatedMessageCount: 0

        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: HeimdallStyle.margin_small
        }

        edge: Widgets.DrawerExt.Edges.Bottom
        width: contentItem?.layoutWidth ?? 0
        height: contentItem?.height ?? 0
        z: 10

        ColorContext {
            id: errorMsgTheme
            palette: HeimdallStyle.palette
            colorSet: ColorContext.Window
        }

        component: Rectangle {
            color: errorMsgTheme.bg.negative

            height: messageText.implicitHeight + HeimdallStyle.margin_normal
            radius: HeimdallStyle.fontHeight_normal / 2

            property real layoutWidth: layout.width

            Accessible.role: Accessible.AlertMessage
            Accessible.name: qsTr("error popup")

            RowLayout {
                id: layout

                spacing: HeimdallStyle.margin_xsmall
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                Widgets.IconLabel {
                    text: HeimdallIcons.info
                    color: errorMsgTheme.fg.primary
                    Layout.leftMargin: HeimdallStyle.margin_xxsmall
                }

                T.Label {
                    id: messageText

                    Layout.maximumWidth: root.width * 0.5
                    Layout.leftMargin: HeimdallStyle.margin_xxsmall
                    Layout.rightMargin: HeimdallStyle.margin_xxsmall

                    text: (DialogErrorModel.repeatedMessageCount > 1 ? '[' + DialogErrorModel.repeatedMessageCount + '] ' : '')
                          + DialogErrorModel.notificationText

                    wrapMode: Text.WrapAnywhere
                    font.pixelSize: HeimdallStyle.fontSize_normal
                    font.bold: true
                    color: errorMsgTheme.fg.primary
                }

                Widgets.TextToolButton {
                    id: detailsBtn

                    text: qsTr("Show Details")

                    colorContext.colorSet: ColorContext.ButtonAccent

                    onClicked: {
                        hideErrorPopupTimer.stop()
                        errorPopup.state = "hidden"
                        DialogErrorModel.resetRepeatedMessageCount()
                        DialogsProvider.messagesDialog(1)
                    }
                }

                Widgets.IconToolButton {
                    id: closeBtn
                    font.pixelSize: HeimdallStyle.icon_normal
                    text: HeimdallIcons.clear
                    description: qsTr("Dismiss")
                    Layout.rightMargin: HeimdallStyle.margin_xxsmall

                    color: closeBtn.colorContext.fg.primary
                    backgroundColor: closeBtn.colorContext.bg.negative

                    onClicked: {
                        hideErrorPopupTimer.stop()
                        errorPopup.state = "hidden"
                        DialogErrorModel.resetRepeatedMessageCount()
                    }
                }
            }

            Widgets.RoundedRectangleShadow {
                yOffset: HeimdallStyle.dp(4, HeimdallStyle.scale)
                blurRadius: HeimdallStyle.dp(3, HeimdallStyle.scale)

                opacity: 0.2

                visible: (color.a > 0.0)

                color: Qt.darker(parent.color)
            }
        }

        state: "hidden"

        Timer {
            id: hideErrorPopupTimer
            interval: 5000
            repeat: false
            onTriggered: {
                errorPopup.state = "hidden"
                DialogErrorModel.resetRepeatedMessageCount()
            }
        }
    }

    ModalDialog {
        id: loginDialog

        //use var here as DialogId is a QGadget and passed by value
        property var dialogId: null
        property string defaultUsername: ""

        onAboutToHide: restoreFocus()
        rootWindow: root.bgContent

        contentItem: GridLayout {
            columns: 2

            readonly property ColorContext colorContext: ColorContext {
                id: loginContentTheme
                palette: HeimdallStyle.palette
                colorSet: ColorContext.Window
            }

            Text {
                text: qsTr("User")
                color: loginContentTheme.fg.primary
                font.pixelSize: HeimdallStyle.fontSize_normal
            }

            Widgets.TextFieldExt {
                id: username

                focus: true
                text: loginDialog.defaultUsername

                Layout.fillWidth:true

                Navigation.downItem: password
                Keys.priority: Keys.AfterItem
                Keys.onPressed: (event) => Navigation.defaultKeyAction(event)
            }

            Text {
                text: qsTr("Password")
                color: loginContentTheme.fg.primary
                font.pixelSize: HeimdallStyle.fontSize_normal
            }

            Widgets.TextFieldExt {
                id: password

                echoMode: TextInput.Password
                font.pixelSize: HeimdallStyle.fontSize_normal
                Layout.fillWidth:true

                Navigation.upItem: username
                Navigation.downItem: savePassword
                Keys.priority: Keys.AfterItem
                Keys.onPressed: (event) => Navigation.defaultKeyAction(event)
            }

            Text {
                text: qsTr("Save password")
                color: loginContentTheme.fg.primary
                font.pixelSize: HeimdallStyle.fontSize_normal
            }
            Widgets.CheckBoxExt {
                id: savePassword

                Navigation.upItem: password
                Navigation.downItem: loginButtons
                Keys.priority: Keys.AfterItem
                Keys.onPressed: (event) => Navigation.defaultKeyAction(event)
            }
        }

        footer: FocusScope {
            id: loginButtons
            implicitHeight: HeimdallStyle.icon_normal

            readonly property ColorContext colorContext: ColorContext {
                id: loginFooterTheme
                palette: HeimdallStyle.palette
                colorSet: ColorContext.Window
            }

            Rectangle {
                color: loginFooterTheme.bg.primary
                anchors.fill: parent

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: HeimdallStyle.margin_xxsmall
                    anchors.rightMargin: HeimdallStyle.margin_xxsmall

                    Widgets.TextToolButton {
                        id: loginCancel
                        Layout.fillWidth: true
                        text: qsTr("cancel")

                        Navigation.upItem: savePassword
                        Navigation.rightItem: loginOk
                        Keys.priority: Keys.AfterItem
                        Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

                        onClicked: {
                            loginDialog.reject()
                            loginDialog.close()
                        }
                    }

                    Widgets.TextToolButton {
                        id: loginOk
                        Layout.fillWidth: true
                        text: qsTr("Ok")
                        focus: true

                        Navigation.upItem: savePassword
                        Navigation.leftItem: loginCancel
                        Keys.priority: Keys.AfterItem
                        Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

                        onClicked: {
                            loginDialog.accept()
                            loginDialog.close()
                        }
                    }
                }
            }
        }

        onAccepted: {
            if (loginDialog.dialogId !== null) {
                heimdallDialog.post_login(loginDialog.dialogId, username.text, password.text, savePassword.checked)
                loginDialog.dialogId = null
            }
        }
        onRejected: {
            if (loginDialog.dialogId !== null) {
                heimdallDialog.dismiss(loginDialog.dialogId)
                loginDialog.dialogId = null
            }
        }
    }

    ModalDialog {
        id: progressDialog

        property var dialogId: null

        property string text : ""
        property string cancelTxt: ""
        property bool interderminate: false
        property real position: 0.0

        onAboutToHide: restoreFocus()
        rootWindow: root.bgContent

        contentItem:  ColumnLayout {

            readonly property ColorContext colorContext: ColorContext {
                id: progressContentTheme
                palette: HeimdallStyle.palette
                colorSet: ColorContext.Window
            }

            Text {
                focus: false
                font.pixelSize: HeimdallStyle.fontSize_normal
                color: progressContentTheme.fg.primary
                text: progressDialog.text
                wrapMode: Text.WordWrap
            }

            Widgets.ProgressBarExt {
                Layout.fillWidth:true

                from: 0.0
                to: 1.0

                indeterminate: progressDialog.interderminate
                value: progressDialog.position
            }
        }


        footer: FocusScope {
            focus: true
            implicitHeight: HeimdallStyle.icon_normal

            readonly property ColorContext colorContext: ColorContext {
                palette: HeimdallStyle.palette
                colorSet: ColorContext.Window
            }

            Rectangle {
                color: questionDialog.colorContext.bg.primary
                anchors.fill: parent
                anchors.leftMargin: HeimdallStyle.margin_xxsmall
                anchors.rightMargin: HeimdallStyle.margin_xxsmall

                Widgets.TextToolButton {
                    anchors.right: parent.right
                    focus: true
                    visible: text !== ""
                    text: progressDialog.cancelTxt

                    onClicked: {
                        heimdallDialog.dismiss(progressDialog.dialogId)
                        progressDialog.dialogId = null
                        progressDialog.close()
                    }
                }
            }
        }
    }

    ModalDialog {
        id: questionDialog

        //use var here as DialogId is a QGadget and passed by value
        property var dialogId: null
        property alias text: content.text
        property alias cancelTxt: cancel.text
        property alias action1Txt: action1.text
        property alias action2Txt: action2.text

        rootWindow: root.bgContent

        onAboutToHide: restoreFocus()

        contentItem: Text {
            id: content
            focus: false
            font.pixelSize: HeimdallStyle.fontSize_normal
            color: questionDialog.colorContext.fg.primary
            wrapMode: Text.WordWrap
        }

        footer: FocusScope {
            focus: true
            id: questionButtons
            implicitHeight: HeimdallStyle.icon_normal

            readonly property ColorContext colorContext: ColorContext {
                palette: HeimdallStyle.palette
                colorSet: ColorContext.Window
            }

            Rectangle {
                color: questionDialog.colorContext.bg.primary
                anchors.fill: parent
                anchors.leftMargin: HeimdallStyle.margin_xxsmall
                anchors.rightMargin: HeimdallStyle.margin_xxsmall

                RowLayout {
                    anchors.fill: parent

                    Widgets.TextToolButton {
                        id: cancel

                        focus: true
                        visible: cancel.text !== ""

                        Layout.fillWidth: true

                        Navigation.rightItem: action1
                        Keys.priority: Keys.AfterItem
                        Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

                        onClicked: {
                            heimdallDialog.dismiss(questionDialog.dialogId)
                            questionDialog.dialogId = null
                            questionDialog.close()
                        }
                    }

                    Widgets.TextToolButton {
                        id: action1

                        visible: action1.text !== ""

                        Layout.fillWidth: true

                        Navigation.leftItem: cancel
                        Navigation.rightItem: action2
                        Keys.priority: Keys.AfterItem
                        Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

                        onClicked: {
                            heimdallDialog.post_action1(questionDialog.dialogId)
                            questionDialog.dialogId = null
                            questionDialog.close()
                        }
                    }

                    Widgets.TextToolButton {
                        id: action2
                        visible: action2.text !== ""

                        Layout.fillWidth: true

                        Navigation.leftItem: action1
                        Keys.priority: Keys.AfterItem
                        Keys.onPressed: (event) => Navigation.defaultKeyAction(event)

                        onClicked: {
                            heimdallDialog.post_action2(questionDialog.dialogId)
                            questionDialog.dialogId = null
                            questionDialog.close()
                        }
                    }
                }
            }
        }
    }

    CustomDialog {
        id: customDialog
        rootWindow: root.bgContent
        onAboutToHide: restoreFocus()
    }

    Loader {
        id: toolbarEditorDialogLoader
        active: false
        source: "qrc:///qt/qml/Heimdall/Dialogs/ToolbarEditorDialog.qml"

        Connections {
            target: DialogsProvider

            function onShowToolbarEditorDialog() {
                toolbarEditorDialogLoader.active = true
                toolbarEditorDialogLoader.item.open()
            }
        }
    }
}
