import QtQuick 2.3
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item {
    id: settingsFirstTemplate
    objectName: "SettingsTemplateForm1"

    signal settingsActivateNotify()
    property real marginFirstTemplate: marginBlock/2 + marginBlock/3
    property var currentObj: null

    x: mainScreenView.mainChoiceZone().width + (marginBlock*7)
    y: mainScreenView.mainViewHeaderRect().height + (marginBlock*11)
    width: rootRectangle.width * 0.65
    height: mainScreenView.mainChoiceZone().height * 0.75+ marginFirstTemplate*2

    visible: true

    function activateTemplate()
    {
        visible = true
        systemSettingRectAnimOn.start()
        networkSettingRectAnimOn.start()
        profileSettingRectAnimOn.start()
        vodSettingRectAnimOn.start()
        liveTvSettingRectAnimOn.start()
        infoSettingRectAnimOn.start()
    }

    function deactivateTemplate(obj)
    {
        currentObj = obj
        systemSettingRectAnimOff.start()
        networkSettingRectAnimOff.start()
        profileSettingRectAnimOff.start()
        vodSettingRectAnimOff.start()
        liveTvSettingRectAnimOff.start()
        infoSettingRectAnimOff.start()
        visible = false
    }

    function checkViewReady()
    {
        return true
    }

    function activateRelatedView()
    {
        settingsActivateNotify()
    }


    Item {
        id: systemSettingRect

        anchors.left: parent.left
        anchors.leftMargin: parent.height*0.2
        anchors.top: parent.top
        anchors.topMargin: parent.height*0.1
        width: parent.width * 0.2
        height: parent.height * 0.4
        scale: 0
        NumberAnimation {
            id: systemSettingRectAnimOn

            target: systemSettingRect
            properties: "scale"
            from: 0
            to: 1
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
            onStarted: settingsFirstTemplate.visible=true
        }
        NumberAnimation {
            id: systemSettingRectAnimOff

            target: systemSettingRect
            properties: "scale"
            from: 1
            to: 0
            easing {type: Easing.InBack; overshoot: 0.8 }
            duration: 50
            onStopped: {
                mainScreenView.activateZone(currentObj)
                settingsFirstTemplate.visible = false
            }
        }

        Rectangle {
            anchors.fill: parent
            color: 'black'
            opacity: 0.6
            radius: 5
        }
        Image {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLACK)
            rotation: 180
            opacity: 0.5
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SETTINGS_SYSTEM_ICON)
        }
        Image {
            width: parent.width *1.13
            height: parent.height *1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 0.5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.5
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_SETTINGS_SYSTEM_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.1
            }
        }
    }
    Item {
        id: networkSettingRect

        anchors.left: systemSettingRect.right
        anchors.leftMargin: marginFirstTemplate
        anchors.top: parent.top
        anchors.topMargin: parent.height*0.1
        width: parent.width * 0.25
        height: parent.height * 0.4
        scale: 0
        NumberAnimation {
            id: networkSettingRectAnimOn

            target: networkSettingRect
            properties: "scale"
            from: 0
            to: 1
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
        }
        NumberAnimation {
            id: networkSettingRectAnimOff

            target: networkSettingRect
            properties: "scale"
            from: 1
            to: 0
            easing {type: Easing.InBack; overshoot: 0.8 }
            duration: 50
        }

        Rectangle {
            anchors.fill: parent
            color: 'black'
            opacity: 0.6
            radius: 5
        }
        Image {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLUE)
            rotation: 180
            opacity: 0.5
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SETTINGS_NETWORK_ICON)
        }
        Image {
            width: parent.width *1.13
            height: parent.height *1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 0.5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.5
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_SETTINGS_NETWORK_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.1
            }
        }
    }
    Item {
        id: profileSettingRect

        anchors.left: networkSettingRect.right
        anchors.leftMargin: marginFirstTemplate
        anchors.top: parent.top
        anchors.topMargin: parent.height*0.1
        width: parent.width * 0.3
        height: parent.height * 0.4
        scale: 0
        NumberAnimation {
            id: profileSettingRectAnimOn

            target: profileSettingRect
            properties: "scale"
            from: 0
            to: 1
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 30
        }
        NumberAnimation {
            id: profileSettingRectAnimOff

            target: profileSettingRect
            properties: "scale"
            from: 1
            to: 0
            easing {type: Easing.InBack; overshoot: 0.8 }
            duration: 50
        }


        Rectangle {
            anchors.fill: parent
            color: 'black'
            opacity: 0.6
            radius: 5
        }
        Image {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLACK)
            rotation: 180
            opacity: 0.5
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SETTINGS_PROFILE_ICON)
        }
        Image {
            width: parent.width *1.13
            height: parent.height *1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 0.5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.5
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_SETTINGS_PROFILE_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.1
            }
        }
    }
    Item {
        id: vodSettingRect

        anchors.left: systemSettingRect.left
        anchors.top: systemSettingRect.bottom
        anchors.topMargin: marginFirstTemplate
        width: parent.width * 0.3
        height: parent.height * 0.4
        scale: 0
        NumberAnimation {
            id: vodSettingRectAnimOn

            target: vodSettingRect
            properties: "scale"
            from: 0
            to: 1
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 20
        }
        NumberAnimation {
            id: vodSettingRectAnimOff

            target: vodSettingRect
            properties: "scale"
            from: 1
            to: 0
            easing {type: Easing.InBack; overshoot: 0.8 }
            duration: 50
        }

        Rectangle {
            anchors.fill: parent
            color: 'black'
            opacity: 0.6
            radius: 5
        }
        Image {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLUE)
            rotation: 180
            opacity: 0.5
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SETTINGS_VOD_ICON)
        }
        Image {
            width: parent.width *1.13
            height: parent.height *1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 0.5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.5
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_SETTINGS_VOD_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.1
            }
        }
    }
    Item {
        id: liveTvSettingRect

        anchors.left: vodSettingRect.right
        anchors.leftMargin: marginFirstTemplate
        anchors.top: vodSettingRect.top
        width: parent.width * 0.2
        height: parent.height * 0.4
        scale: 0
        NumberAnimation {
            id: liveTvSettingRectAnimOn

            target: liveTvSettingRect
            properties: "scale"
            from: 0
            to: 1
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 30
        }
        NumberAnimation {
            id: liveTvSettingRectAnimOff

            target: liveTvSettingRect
            properties: "scale"
            from: 1
            to: 0
            easing {type: Easing.InBack; overshoot: 0.8 }
            duration: 20
        }

        Rectangle {
            anchors.fill: parent
            color: 'black'
            opacity: 0.6
            radius: 5
        }
        Image {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLACK)
            rotation: 180
            opacity: 0.5
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SETTINGS_LIVETV_ICON)
        }
        Image {
            width: parent.width *1.13
            height: parent.height *1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 0.5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.5
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_SETTINGS_LIVETV_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.1
            }
        }
    }
    Item {
        id: infoSettingRect

        anchors.left: liveTvSettingRect.right
        anchors.leftMargin: marginFirstTemplate
        anchors.top: liveTvSettingRect.top
        width: parent.width * 0.25
        height: parent.height * 0.4
        scale: 0
        NumberAnimation {
            id: infoSettingRectAnimOn

            target: infoSettingRect
            properties: "scale"
            from: 0
            to: 1
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
        }
        NumberAnimation {
            id: infoSettingRectAnimOff

            target: infoSettingRect
            properties: "scale"
            from: 1
            to: 0
            easing {type: Easing.InBack; overshoot: 0.8 }
            duration: 20
        }

        Rectangle {
            anchors.fill: parent
            color: 'black'
            opacity: 0.6
            radius: 5
        }
        Image {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLUE)
            rotation: 180
            opacity: 0.5
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SETTINGS_INFO_ICON_BIG)
        }
        Image {
            width: parent.width *1.13
            height: parent.height *1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 0.5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.5
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_SETTINGS_INFO_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.1
            }
        }
    }
}

