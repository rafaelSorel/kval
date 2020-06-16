import QtQuick 2.3
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import "../KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0

Item {
    id: appsFirstTemplate
    objectName: "AppsTemplateForm1"

    signal appsExtractNotify()
    property real marginFirstTemplate: marginBlock*0.5 + marginBlock*0.3333
    property var currentObj: null

    x: mainScreenView.mainChoiceZone().width + (marginBlock*15)
    y: mainScreenView.mainViewHeaderRect().height + (marginBlock*15)
    width: rootRectangle.width * 0.65
    height: mainScreenView.mainChoiceZone().height * 0.75 + marginFirstTemplate*2

    function checkViewReady()
    {
        return true
    }

    function activateTemplate()
    {
        visible = true
        app1VidZoneAnimOn.start()
        app2VidZoneAnimOn.start()
        app3VidZoneAnimOn.start()
        app1MiscZoneAnimOn.start()
        separationLine.opacity = 0.3
    }

    function deactivateTemplate(obj)
    {
        currentObj = obj
        app1VidZoneAnimOff.start()
        app2VidZoneAnimOff.start()
        app3VidZoneAnimOff.start()
        app1MiscZoneAnimOff.start()
        separationLine.opacity = 0.0
        visible = false
        mainScreenView.activateZone(currentObj)
    }

    function activateRelatedView()
    {
        appsExtractNotify()
    }

    Item {
        id: videoAppZone

        anchors.left: parent.left
        anchors.leftMargin: marginFirstTemplate * 10
        anchors.top: parent.top
        anchors.topMargin: marginFirstTemplate * 3
        width: appsFirstTemplate.width * 0.7
        height: appsFirstTemplate.height * 0.4

        Item {
            id: app1VidZone

            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 5
            anchors.top: parent.top
            anchors.topMargin: marginFirstTemplate
            width: appsFirstTemplate.width * 0.2
            height: appsFirstTemplate.height * 0.35
            scale: 0
            NumberAnimation {
                id: app1VidZoneAnimOn

                target: app1VidZone
                properties: "scale"
                from: 0
                to: 1
                easing {type: Easing.OutBack; overshoot: 1 }
                duration: 20
            }
            NumberAnimation {
                id: app1VidZoneAnimOff

                target: app1VidZone
                properties: "scale"
                from: 1
                to: 0
                easing {type: Easing.InBack; overshoot: 0.8 }
                duration: 50
            }

            Image {
                id: lineBackImg
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: -marginFirstTemplate*0.5
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate*4.5
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LINE1_BACKGROUND)
                opacity: 0.8
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
                opacity: 0.2
            }
            Image {
                id: screenappforeground3
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_FOREGROUND_3)
            }
            Image {
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_SHININGCORNER_3)
            }

            Image {
                anchors.centerIn: lineBackImg
                scale: Utils.scaled(0.5)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.YT_LOGO_APP)
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: marginFirstTemplate * 0.5
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate * 0.5
                color: "white"
                text: 'YouTube'
                style: Text.Raised
                styleColor: "black"
                font {
                  family: localAdventFont.name;
                  pixelSize: parent.height * 0.1
                }
            }
        }
        Item {
            id: app2VidZone

            anchors.left: app1VidZone.right
            anchors.leftMargin: marginFirstTemplate*4
            anchors.top: parent.top
            anchors.topMargin: marginFirstTemplate
            width: appsFirstTemplate.width * 0.2
            height: appsFirstTemplate.height * 0.35
            scale: 0
            NumberAnimation {
                id: app2VidZoneAnimOn

                target: app2VidZone
                properties: "scale"
                from: 0
                to: 1
                easing {type: Easing.OutBack; overshoot: 1 }
                duration: 20
            }
            NumberAnimation {
                id: app2VidZoneAnimOff

                target: app2VidZone
                properties: "scale"
                from: 1
                to: 0
                easing {type: Easing.InBack; overshoot: 0.8 }
                duration: 50
            }

            Image {
                id: app2VidBackImg
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate*4.5
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SPORT_WALLPAPER_BACKGROUND)
                opacity: 0.9
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
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_FOREGROUND_1)
            }
            Image {
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_SHININGCORNER_1)
            }
            Image {
                anchors.centerIn: app2VidBackImg
                scale: Utils.scaled(0.6)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LIVE_SPORT_LOGO)
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: marginFirstTemplate * 0.5
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate * 0.5
                color: "white"
                text: "Live sport"
                style: Text.Raised
                styleColor: "black"
                font {
                  family: localAdventFont.name;
                  pixelSize: parent.height * 0.1
                }
            }
        }
        Item {
            id: app3VidZone

            anchors.left: app2VidZone.right
            anchors.leftMargin: marginFirstTemplate*4
            anchors.top: parent.top
            anchors.topMargin: marginFirstTemplate
            width: appsFirstTemplate.width * 0.2
            height: appsFirstTemplate.height * 0.35
            scale: 0
            NumberAnimation {
                id: app3VidZoneAnimOn

                target: app3VidZone
                properties: "scale"
                from: 0
                to: 1
                easing {type: Easing.OutBack; overshoot: 1 }
                duration: 20
            }
            NumberAnimation {
                id: app3VidZoneAnimOff

                target: app3VidZone
                properties: "scale"
                from: 1
                to: 0
                easing {type: Easing.InBack; overshoot: 0.8 }
                duration: 50
            }

            Image {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate*4.5
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APP_PLUS_BACKGROUND)
                opacity: 0.7
            }
            Image {
                id: app3VidZoneOverlay
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate
                fillMode: Image.Stretch

                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLACK)
                rotation: 180
                opacity: 0.1
            }
            Image {
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_FOREGROUND_2)
            }

            Image {
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_SHININGCORNER_2)
            }


            Image {
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ADDON_ADD_ICON)
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: marginFirstTemplate * 0.5
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate * 0.5
                color: "white"
                text: kvalUiConfigManager.retranslate + qsTr("Plus")
                style: Text.Raised
                styleColor: "black"
                font {
                  family: localAdventFont.name;
                  pixelSize: parent.height * 0.1
                }
            }
        }

    }
    Rectangle {
        id: separationLine
        anchors.left: videoAppZone.left
        anchors.leftMargin: marginFirstTemplate*2
        anchors.right: videoAppZone.right
        anchors.top: videoAppZone.bottom
        height: 1
        color: "white"
        opacity: 0.0
        layer.enabled: true
        layer.effect: Glow {
            radius: 8
            samples: 17
            spread: 0.5
            color: "#6aa6cb"
            transparentBorder: true
        }
    }
    Item {
        id: miscAppZone

        anchors.left: parent.left
        anchors.leftMargin: marginFirstTemplate * 10
        anchors.top: separationLine.bottom
        anchors.topMargin: marginFirstTemplate * 3
        width: appsFirstTemplate.width * 0.7
        height: appsFirstTemplate.height * 0.4
        Item {
            id: app1MiscZone

            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 5
            anchors.top: parent.top
            anchors.topMargin: marginFirstTemplate
            width: appsFirstTemplate.width * 0.2
            height: appsFirstTemplate.height * 0.35
            scale: 0
            NumberAnimation {
                id: app1MiscZoneAnimOn

                target: app1MiscZone
                properties: "scale"
                from: 0
                to: 1
                easing {type: Easing.OutBack; overshoot: 1 }
                duration: 20
            }
            NumberAnimation {
                id: app1MiscZoneAnimOff

                target: app1MiscZone
                properties: "scale"
                from: 1
                to: 0
                easing {type: Easing.InBack; overshoot: 0.8 }
                duration: 50
            }

            Image {
                id: app1MiscZoneBackImg
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: -marginFirstTemplate*0.5
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate*4.5
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.WEATHER_APP_BACK)
                opacity: 0.9
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
                opacity: 0.2
            }
            Image {
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_FOREGROUND_2)
            }
            Image {
                anchors.centerIn: parent
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SCREEN_APP_SHININGCORNER_2)
                opacity: 0.6
            }

            Image {
                anchors.centerIn: app1MiscZoneBackImg
                scale: Utils.scaled(1)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.WEATHER_APP_ICON)
            }
            Text {
                anchors.left: parent.left
                anchors.leftMargin: marginFirstTemplate * 0.5
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginFirstTemplate * 0.5
                color: "white"
                text: 'Yahoo Weather'
                style: Text.Raised
                styleColor: "black"
                font {
                  family: localAdventFont.name;
                  pixelSize: parent.height * 0.1
                }
            }
        }
    }
}

