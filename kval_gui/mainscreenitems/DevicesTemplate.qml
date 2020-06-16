import QtQuick 2.3
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import "../KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0

Item {
    id: localMediaFirstTemplate
    objectName: "DevicesTemplateForm1"

    signal mediaBrowserActivateNotify()
    property real marginFirstTemplate: marginBlock*0.5 + marginBlock*0.3333
    property bool g_connectedUsbDevice: false
    property var currentObj: null

    x: mainScreenView.mainChoiceZone().width + (marginBlock*7)
    y: mainScreenView.mainViewHeaderRect().height + (marginBlock*11)
    width: rootRectangle.width * 0.65
    height: mainScreenView.mainChoiceZone().height*0.75 + marginFirstTemplate*2
    visible: false


    function activateTemplate()
    {
        visible = true
        mainLocalMediaItemAnimOn.start()
    }

    function deactivateTemplate(obj)
    {
        currentObj = obj
        localMediaPhotosAnimOff.start()
        localMediaMusicAnimOff.start()
        localMediaVideosAnimOff.start()
        visible = false
    }

    function langChanged()
    {
        if(g_connectedUsbDevice)
            mainLocalMediaItemConnectedInfoTxt.text =
                kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_USB_CONNECTED)
        else
            mainLocalMediaItemConnectedInfoTxt.text =
                kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_USB_NOT_CONNECTED)

    }

    function checkViewReady()
    {
        if(!g_connectedUsbDevice) return false
        return true
    }

    function activateRelatedView()
    {
        mediaBrowserActivateNotify()
    }

    function connectedDevice()
    {
        g_connectedUsbDevice = true
        mainLocalMediaItemConnectedInfoTxt.text =
                kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_USB_CONNECTED)
    }

    function disconnectedDevice()
    {
        g_connectedUsbDevice = false
        mainLocalMediaItemConnectedInfoTxt.text =
                kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_USB_NOT_CONNECTED)
    }

    Item {
        id: mainLocalMediaItem

        x: parent.width * 0.125
        width: parent.width * 0.75 + marginFirstTemplate * 2
        height: 0
        opacity: 0
        NumberAnimation {
            id: mainLocalMediaItemAnimOn

            target: mainLocalMediaItem
            properties: "height, opacity"
            from: 0
            to: localMediaFirstTemplate.height *0.7
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
            onStarted: localMediaFirstTemplate.visible = true
            onStopped: {
                localMediaMusicAnimOn.start()
                localMediaPhotosAnimOn.start()
                localMediaVideosAnimOn.start()
            }
        }
        NumberAnimation {
            id: mainLocalMediaItemAnimOff

            target: mainLocalMediaItem
            properties: "height, opacity"
            to: 0
            from: localMediaFirstTemplate.height *0.7
            easing {type: Easing.InBack; overshoot: 1 }
            duration: 50
            onStopped: {
                localMediaFirstTemplate.visible = false
                mainScreenView.activateZone(currentObj)
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.6
            radius: 5
        }
        Image {
            width: mainLocalMediaItem.width + mainLocalMediaItem.width*0.134
            height: mainLocalMediaItem.height + mainLocalMediaItem.height*0.13
            anchors.centerIn: mainLocalMediaItem
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            id: mainLocalMediaItemConnectedInfoTxt

            anchors.top: mainLocalMediaItem.top
            anchors.topMargin: marginFirstTemplate * 2
            anchors.left: mainLocalMediaItem.left
            anchors.leftMargin: marginFirstTemplate * 4
            anchors.right: mainLocalMediaItem.right
            anchors.rightMargin: marginFirstTemplate * 2

            color: "white"
            text: kvalUiConfigManager.retranslate +
                  kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_USB_NOT_CONNECTED)
            style: Text.Raised
            styleColor: "black"
            elide:Text.ElideLeft
            wrapMode: Text.WordWrap
            font {
              family: localAdventFont.name;
              pixelSize: mainLocalMediaItem.height * 0.09
            }
        }
        Image {
            anchors.top: mainLocalMediaItem.top
            anchors.topMargin: marginFirstTemplate * 10
            anchors.left: mainLocalMediaItem.left
            anchors.leftMargin: marginFirstTemplate * 15
            scale: Utils.scaled(1)
            opacity: 0.8
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.USB_HOME_ICON)
        }
        Image {
            anchors.top: mainLocalMediaItem.top
            anchors.topMargin: marginFirstTemplate * 10
            anchors.right: mainLocalMediaItem.right
            anchors.rightMargin: marginFirstTemplate * 15
            scale: Utils.scaled(1)
            opacity: 0.8
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HDD_HOME_ICON)
        }
        Text {
            anchors.bottom: mainLocalMediaItem.bottom
            anchors.bottomMargin: marginFirstTemplate * 2
            anchors.left: mainLocalMediaItem.left
            anchors.leftMargin: marginFirstTemplate * 4
            anchors.right: mainLocalMediaItem.right
            anchors.rightMargin: marginFirstTemplate * 2

            color: "#6aa6cb"
            text: kvalUiConfigManager.retranslate +
                  kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_USB_INFO_CONTENT)
            style: Text.Raised
            styleColor: "black"
            elide:Text.ElideLeft
            wrapMode: Text.WordWrap
            font {
              family: localAdventFont.name;
              pixelSize: mainLocalMediaItem.height * 0.08
            }
        }
    }

    Item {
        id: localMediaPhotos

        x: mainLocalMediaItem.x
        y: mainLocalMediaItem.height + marginFirstTemplate
        height: 0
        opacity: 0
        width: parent.width * 0.25
        NumberAnimation {
            id: localMediaPhotosAnimOn

            target: localMediaPhotos
            properties: "height, opacity"
            from: 0
            to: localMediaFirstTemplate.height * 0.31
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
        }
        NumberAnimation {
            id: localMediaPhotosAnimOff

            target: localMediaPhotos
            properties: "height, opacity"
            to: 0
            from: localMediaFirstTemplate.height * 0.31
            easing {type: Easing.InBack; overshoot: 1 }
            duration: 50
            onStopped: {
                mainLocalMediaItemAnimOff.start()
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            radius: 5
            opacity: 0.7
        }
        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            fillMode: Image.PreserveAspectFit
            opacity: 0.7
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_LOCAL_MEDIA_PICS_BACK)
        }
        Image {
            anchors.top: parent.top
            anchors.topMargin: -marginFirstTemplate*4
            anchors.horizontalCenter: parent.horizontalCenter
//            scale: Utils.scaled(1)
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_LOCAL_MEDIA_PICS)
        }
        Image {
            width: parent.width + parent.width*0.13
            height: parent.height + parent.height*0.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_PIC_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.12
            }
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate
            color: "#6aa6cb"
            text: ""
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.12
            }
        }
    }
    Item {
        id: localMediaMusic

        x: mainLocalMediaItem.x +
           parent.width * 0.25 +
           marginFirstTemplate
        y: mainLocalMediaItem.height +
           marginFirstTemplate
        height: 0
        opacity: 0
        width: parent.width * 0.25
        NumberAnimation {
            id: localMediaMusicAnimOn

            target: localMediaMusic
            properties: "height, opacity"
            from: 0
            to: localMediaFirstTemplate.height * 0.31
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
        }
        NumberAnimation {
            id: localMediaMusicAnimOff

            target: localMediaMusic
            properties: "height, opacity"
            to: 0
            from: localMediaFirstTemplate.height * 0.31
            easing {type: Easing.InBack; overshoot: 1 }
            duration: 50
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            radius: 5
            opacity: 0.7
        }
        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            fillMode: Image.PreserveAspectFit
            scale: Utils.scaled(1)
            opacity: 0.7
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_LOCAL_MEDIA_MUSIC_BACK)
        }
        Image {
            anchors.top: parent.top
            anchors.topMargin: -marginFirstTemplate*4
            anchors.horizontalCenter: parent.horizontalCenter
            scale: Utils.scaled(1)
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_LOCAL_MEDIA_MUSIC)
        }
        Image {
            width: parent.width + parent.width*0.13
            height: parent.height + parent.height*0.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_MUSIC_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.12
            }
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate
            color: "#6aa6cb"
            text: ""
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.12
            }
        }
    }
    Item {
        id: localMediaVideos

        x: mainLocalMediaItem.x +
           parent.width * 0.5 +
           marginFirstTemplate * 2
        y: mainLocalMediaItem.height +
           marginFirstTemplate
        height: 0
        opacity: 0
        width: parent.width * 0.25
        NumberAnimation {
            id: localMediaVideosAnimOn

            target: localMediaVideos
            properties: "height, opacity"
            from: 0
            to: localMediaFirstTemplate.height * 0.31
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
        }
        NumberAnimation {
            id: localMediaVideosAnimOff

            target: localMediaVideos
            properties: "height, opacity"
            to: 0
            from: localMediaFirstTemplate.height * 0.31
            easing {type: Easing.InBack; overshoot: 1 }
            duration: 50
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            radius: 5
            opacity: 0.7
        }
        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            fillMode: Image.PreserveAspectFit
            scale: Utils.scaled(1)
            opacity: 0.7
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_LOCAL_MEDIA_VIDEOS_BACK)
        }
        Image {
            anchors.top: parent.top
            anchors.topMargin: -marginFirstTemplate*4
            anchors.horizontalCenter: parent.horizontalCenter
            scale: Utils.scaled(1)
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_LOCAL_MEDIA_VIDEOS)
        }
        Image {
            width: parent.width + parent.width*0.13
            height: parent.height + parent.height*0.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate
            color: "white"
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_VIDEOS_LABEL_NAME)
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.12
            }
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate
            color: "#6aa6cb"
            text: ""
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.12
            }
        }
    }
}
