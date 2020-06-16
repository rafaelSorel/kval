import QtQuick 2.3
import "KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item {
    id: bootItemScreen

    width: rootRectangle.width;
    height: rootRectangle.height;
    x: rootRectangle.x
    y: rootRectangle.y

    enabled: true

    signal bootScreenReleased()

    NumberAnimation {
       id: bootItemScreenAnimOff

       target: bootItemScreen
       properties: "opacity"
       from: 1
       to: 0
       duration: 100
       onStarted: {
           bootScreenReleased()
       }
       onStopped: {
           opacity = 0
           enabled = false
           focus = false
           bootScreenMainBackImg.source = ""
           bootScreenOverlayImg1.source = ""
           bootScreenOverlayImg2.source = ""
       }
    }

    function _init()
    {

    }

    function onHomeReady()
    {
        if(!enabled) return
        loadingItemAnimOff.start()
    }

    function getBackGroundSrc()
    {
        var currentTime = Qt.formatTime(new Date(), "hh");
        var constMorningBackTab =
        [kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning1.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning2.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning3.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning4.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning5.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning6.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning7.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_MORNING)+"morning8.jpg"]

        var constAfternoonBackTab =
        [kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_AFTERNOON)+"afternoon1.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_AFTERNOON)+"afternoon2.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_AFTERNOON)+"afternoon3.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_AFTERNOON)+"afternoon4.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_AFTERNOON)+"afternoon5.jpg"]

        var constEveningBackTab =
        [kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_EVENING)+"evening1.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_EVENING)+"evening2.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_EVENING)+"evening3.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_EVENING)+"evening4.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_EVENING)+"evening6.jpg",
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BOOT_BACKGROUNG_EVENING)+"evening7.jpg"]

        if(currentTime < 12 && currentTime > 7)
        {
            return constMorningBackTab[Math.floor((
                Math.random() * constMorningBackTab.length))]
        }
        else if(currentTime > 12 && currentTime < 19)
        {
            return constAfternoonBackTab[Math.floor((
                Math.random() * constAfternoonBackTab.length))]
        }
        else
        {
            return constEveningBackTab[Math.floor((
                Math.random() * constEveningBackTab.length))]
        }
    }

    Image {
        id: bootScreenMainBackImg
        anchors.fill: parent
        source: getBackGroundSrc()
        onStatusChanged: {
            if(bootScreenMainBackImg.status === Image.Ready)
            {
                enabled = true
                opacity = 1
                focus = true
                kvalIpMediaAnimOn.start()
            }
        }
        Image {
            id: bootScreenOverlayImg1

            anchors.fill: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND)
            opacity: 0.5
        }
        Image {
            id: bootScreenOverlayImg2

            anchors.fill: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND_BLACK_2)
            opacity: 0.6
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.STARTUP_AFTERNOON_WELCOME)
            opacity: 0.7
        }

    }
    Text {
        anchors.right: parent.right
        anchors.rightMargin: marginBlock*2
        anchors.top: parent.top
        anchors.topMargin: marginBlock
        text: Qt.formatTime(new Date(), "hh:mm");
        color: "white"
        font {
            family: localFontCaps.name;
            pixelSize: bootItemScreen.width * 0.015
        }
        Text {
            anchors.right: parent.right
            anchors.top: parent.bottom
            anchors.topMargin: -marginBlock
            text: Qt.formatDateTime(new Date(), "ddd dd.MM.yyyy")
            color: "#6aa6cb"
            font {
                family: localFont.name
                pixelSize: bootItemScreen.width * 0.015
            }
        }
    }

    Image {
        id: kvalIpMedia
        anchors.centerIn: parent
        source: ""
//        kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KVAL_LOGO_BIG)
        opacity: 1
        scale: 0

        NumberAnimation {
               id: kvalIpMediaAnimOn

               target: kvalIpMedia
               properties: "scale"
               from: 0
               to: 0.6
               duration: 200
               easing {type: Easing.OutBounce; period: 1 }
               onStopped: {
                    loadingItemAnimOn.start()
               }
        }
        NumberAnimation {
               id: kvalIpMediaAnimOff

               target: kvalIpMedia
               properties: "opacity ,scale"
               from: 0.7
               to: 0
               duration: 100
               easing {type: Easing.InBack; period: 1 }
        }
    }

    Item {
        id: loadingItem

        x: bootItemScreen.width * 1.1
        anchors.bottom: parent.bottom
        anchors.bottomMargin: marginBlock*3
        width: parent.width *0.2
        height: parent.height *0.05
        NumberAnimation {
               id: loadingItemAnimOn

               target: loadingItem
               properties: "x, opacity"
               to: bootItemScreen.width*0.78
               duration: 500
               easing {type: Easing.OutBack; overshoot: 1 }
               onStopped: {
                    spinnerLoadAnim.start()
                    spinnerLoad.opacity = 1
               }
        }
        NumberAnimation {
               id: loadingItemAnimOff

               target: loadingItem
               properties: "x, opacity"
               to: bootItemScreen.width * 1.1
               from: bootItemScreen.width*0.78
               duration: 200
               easing {type: Easing.InBack; overshoot: 1.5 }
               onStarted: {
                    spinnerLoadAnim.stop()
                    spinnerLoad.opacity = 0
               }
               onStopped: bootItemScreenAnimOff.start()
        }

        Text {
            id: loadingTxt
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            style: Text.Raised
            styleColor: "black"
            text: kvalUiConfigManager.retranslate + qsTr("Chargement home...")
            color: "white"
            opacity: 0.8
            font {
                family: localAdventFont.name;
                pixelSize: bootItemScreen.height * 0.05
            }
        }
        Image {
            id: spinnerLoad

            anchors.right: loadingTxt.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -marginBlock*0.6
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BUSY_SPINNER)
            scale: 0.5
            opacity: 0
            RotationAnimation on rotation
            {
                id: spinnerLoadAnim
                loops: Animation.Infinite
                direction: RotationAnimation.Clockwise
                from: 0
                to: 360
            }
        }
    }
}
