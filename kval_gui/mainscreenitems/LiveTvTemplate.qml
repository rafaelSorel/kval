import QtQuick 2.3
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item {
    id: liveTvFirstTemplate
    objectName: "LiveTvTemplateForm1"

    signal activateLiveTvViewNotify()
    property real marginFirstTemplate: marginBlock/2 + marginBlock/3
    property var frChannelNameArray: []
    property var frChannelPiconsArray: []
    property var arChannelPiconsArray: []
    property var arChannelNameArray: []
    property int g_currentFrPicIdx: 0
    property int g_currentArPicIdx: 0
    property var currentObj: null

    x: mainScreenView.mainChoiceZone().width + (marginBlock*7)
    y: mainScreenView.mainViewHeaderRect().height + (marginBlock*12)
    width: rootRectangle.width * 0.65
    height: mainScreenView.mainChoiceZone().height * 0.75+ marginFirstTemplate*2


    function activateTemplate()
    {
        visible = true
        preLoadData()
        animateLiveTvZoneHeightOn.start()
        startLiveTvAlternateAnimation()
    }

    function deactivateTemplate(obj)
    {
        currentObj = obj
        animateThirdTvBannerWidthOff.start()
        stopLiveTvAlternateAnimation()
        visible = false
    }

    function checkViewReady()
    {
        return true
    }

    function activateRelatedView()
    {
        stopLiveTvAlternateAnimation()
        activateLiveTvViewNotify()
        mainScreenView.mainScreenScaleOff()
    }

    function preLoadData()
    {
        frChannelName.text = frChannelNameArray[g_currentFrPicIdx]
        frChannelPicon.source = frChannelPiconsArray[g_currentFrPicIdx]

        arChannelPicon.source = arChannelPiconsArray[g_currentArPicIdx]
        arChannelName.text = arChannelNameArray[g_currentArPicIdx]
    }

    function startLiveTvAlternateAnimation()
    {
        mainScreenFrPiconAlternateTimer.start()
        mainScreenFrPiconAlternateTimer.repeat = true
        mainScreenArPiconAlternateTimer.start()
        mainScreenArPiconAlternateTimer.repeat = true
    }
    function stopLiveTvAlternateAnimation()
    {
        mainScreenFrPiconAlternateTimer.stop()
        mainScreenFrPiconAlternateTimer.running = false
        mainScreenFrPiconAlternateTimer.repeat = false
        mainScreenArPiconAlternateTimer.stop()
        mainScreenArPiconAlternateTimer.running = false
        mainScreenArPiconAlternateTimer.repeat = false
    }

    function customInfosReady()
    {
        var mainInfoZone = kvalUiGuiUtils.getliveCustomMainZoneInfo()
        if(mainInfoZone.length === 0) return
        mainImage.source = mainInfoZone[0]
        overlayMainImage.source = mainInfoZone[1]
        liveTvDesc.text = mainInfoZone[2]
        liveTvDesc.color = mainInfoZone[3]
        infoOverlayImg.source = mainInfoZone[4]
        liveTvHomeIcon.text = mainInfoZone[5]
        liveTvHomeIcon.color = mainInfoZone[6]

        var firstBanInfoZone = kvalUiGuiUtils.getliveCustomFirstBanZoneInfo()
        firstTvBannerLabel.text = firstBanInfoZone[0]
        firstTvBannerLabel.color= firstBanInfoZone[1]
        firtBannerTopRightMiniImage.source= firstBanInfoZone[2]
        firtBannerBackImage.source= firstBanInfoZone[3]
        firstBannerForeImage.source= firstBanInfoZone[4]
        firstBannerForeImage2.source= firstBanInfoZone[5]
        firstBannerEticket.text= firstBanInfoZone[6]
        firstBannerEticket.color= firstBanInfoZone[7]

        var secondBanZoneInfo = kvalUiGuiUtils.getliveCustomSecondBanZoneInfo()
        secondTvBannerLabel.text = secondBanZoneInfo[0]
        secondTvBannerLabel.color= secondBanZoneInfo[1]
        secondBannerBackImage.source= secondBanZoneInfo[2]
        secondBannerForeImage.source= secondBanZoneInfo[3]
        frChannelTvImg.text= secondBanZoneInfo[4]
        frChannelTvImg.color= secondBanZoneInfo[5]
        frChannelPiconsArray =
                kvalUiGuiUtils.getliveCustomSecondBanAnimZoneImg()
        frChannelNameArray = kvalUiGuiUtils.getliveCustomSecondBanAnimZoneTxt()
        frChannelName.color =
                frChannelNameArray[frChannelNameArray.length - 1]

        var thirdBanZoneInfo = kvalUiGuiUtils.getliveCustomThirdBanZoneInfo()
        thirdTvBannerLabel.text = thirdBanZoneInfo[0]
        thirdTvBannerLabel.color = thirdBanZoneInfo[1]
        thirdTvBannerBackImage.source = thirdBanZoneInfo[2]
        thirdTvBannerForeImage.source= thirdBanZoneInfo[3]
        arChannelTvImg.text= thirdBanZoneInfo[4]
        arChannelTvImg.color= thirdBanZoneInfo[5]
        arChannelPiconsArray =
                kvalUiGuiUtils.getliveCustomThirdBanAnimZoneImg()
        arChannelNameArray = kvalUiGuiUtils.getliveCustomThirdBanAnimZoneTxt()
        arChannelName.color =
                frChannelNameArray[frChannelNameArray.length - 1]
    }

    Timer {
        id: mainScreenFrPiconAlternateTimer
        interval: 2500;
        running: false;
        repeat: true;
        onTriggered: {
            animateFrChannelOpacityOff.start()
        }
    }
    Timer {
        id: mainScreenArPiconAlternateTimer
        interval: 3000;
        running: false;
        repeat: true;
        onTriggered: {
            animateArChannelOpacityOff.start()
        }
    }

    Rectangle {
        id: liveTvZone

        width: parent.width * 0.35
        height: 0
        color: "transparent"
        clip: true
        NumberAnimation {
               id: animateLiveTvZoneHeightOn
               target: liveTvZone
               properties: "height"
               from: 0
               to: liveTvFirstTemplate.height
               duration: 20
               easing {type: Easing.OutBack; overshoot: 0.8 }
               onStopped:{
                   animateLiveTvfirtBannerWidthOn.start()
               }
               onStarted: {
                   liveTvHomeIcon.opacity = 1
                   liveTvDesc.opacity = 1
               }
        }
        NumberAnimation {
               id: animateLiveTvHeightOff
               target: liveTvZone
               properties: "height"
               from: liveTvFirstTemplate.height
               to: 0
               duration: 20
               easing {type: Easing.InOutBack; overshoot: 1.5 }
               onStopped: {
                   liveTvHomeIcon.opacity = 0
                   liveTvDesc.opacity = 0
                   visible = false
                   mainScreenView.activateZone(currentObj)
                   visible = false
               }
        }
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.6
        }
        Image {
            id: mainImage

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            fillMode: Image.PreserveAspectFit
            opacity: 0.65
        }
        Image {
            id: overlayMainImage

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            opacity: 0.6
        }
        Text {
            id: liveTvDesc

            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate/2
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate*2
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate*2
            opacity: 0
            wrapMode: Text.WordWrap
            style: Text.Raised
            styleColor: "black"
            font {
                family: localAdventFont.name
                pixelSize: parent.height * 0.05
            }
        }
        Image {
            id: infoOverlayImg

            anchors.top: parent.top
            anchors.bottom :parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: -marginFirstTemplate*12
            rotation: 180
            opacity: 0.4
        }
        Text {
            id: liveTvHomeIcon

            anchors.top: parent.top
            anchors.topMargin: -marginFirstTemplate*2
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate*2
            opacity: 0
            font {
                family: localLiveTvFont.name
                pixelSize: parent.height * 0.4
            }
        }
    }
    Image {
        width: liveTvZone.width * 1.13
        height: liveTvZone.height * 1.13
        anchors.centerIn: liveTvZone
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Text {
        id: firstTvBannerLabel

        anchors.left: firstTvBanner.left
        anchors.leftMargin: marginFirstTemplate*2
        anchors.bottom: firstTvBanner.top
        style: Text.Raised
        styleColor: "black"
        opacity: 0
        font {
            family: localFont.name
            pixelSize: firstTvBanner.width * 0.05
        }
    }
    Item {
        id: firstTvBanner
        width: 0
        height: liveTvZone.height * 0.7 - (marginFirstTemplate*3)
        anchors.left: liveTvZone.right
        anchors.leftMargin: marginFirstTemplate
        anchors.top: liveTvZone.top
        clip: true
        NumberAnimation {
               id: animateLiveTvfirtBannerWidthOn
               target: firstTvBanner
               properties: "width"
               from: 0
               to: liveTvFirstTemplate.width * 0.65 - marginFirstTemplate/2
               duration: 20
               easing {type: Easing.OutBack; overshoot: 0.8 }
               onStarted: {
               }
               onStopped: {
                   animateSecondTvBannerWidthOn.start()
                   firstTvBannerLabel.opacity = 1
                   firstBannerEticket.opacity = 1
               }
        }
        NumberAnimation {
               id: animateLiveTvfirtBannerWidthOff
               target: firstTvBanner
               properties: "width"
               from: liveTvFirstTemplate.width * 0.65 - marginFirstTemplate/2
               to: 0
               duration: 20
               easing {type: Easing.InBack; overshoot: 0.8 }
               onStarted: {
                   firstTvBannerLabel.opacity = 0
                   firstBannerEticket.opacity = 0
               }
               onStopped: {
                   animateLiveTvHeightOff.start()
               }
        }
        Image {
            id: firtBannerTopRightMiniImage
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate
            width: parent.width * 0.065
            height: parent.width * 0.065
        }
        Image {
            id: firtBannerBackImage
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            fillMode: Image.PreserveAspectFit
        }
        Image {
            id: firstBannerForeImage
            anchors.left: parent.left
            anchors.leftMargin: parent.width*0.15
            anchors.right: parent.right
            anchors.rightMargin: parent.width*0.15
            anchors.top: parent.top
            anchors.topMargin: parent.height * 0.15
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * 0.15
            fillMode: Image.PreserveAspectFit
            opacity: 0.85
        }
        Image {
            id: firstBannerForeImage2
            anchors.fill: parent
            opacity: 0.6
        }
        Text {
            id: firstBannerEticket
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate
            anchors.bottom: parent.bottom
            opacity: 0
            style: Text.Raised
            styleColor: "black"
            font {
                family: localAdventFont.name
                pixelSize: parent.width * 0.06
            }
        }

    }
    Image {
        width: firstTvBanner.width * 1.13
        height: firstTvBanner.height * 1.13
        anchors.centerIn: firstTvBanner
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }

    Item {
        id: secondTvBanner
        width: 0
        height: liveTvZone.height * 0.3 + marginFirstTemplate*2
        anchors.left: firstTvBanner.left
        anchors.top: firstTvBanner.bottom
        anchors.topMargin: marginFirstTemplate
        clip: true
        NumberAnimation {
               id: animateSecondTvBannerWidthOn
               target: secondTvBanner
               properties: "width"
               from: 0
               to: firstTvBanner.width * 0.5 - marginFirstTemplate/2
               duration: 20
               easing {type: Easing.OutBack; overshoot: 0.5 }
               onStopped: {
                   animateThirdTvBannerWidthOn.start()
                   secondTvBannerLabel.opacity = 1
               }
        }
        NumberAnimation {
               id: animateSecondTvBannerWidthOff
               target: secondTvBanner
               properties: "width"
               from: firstTvBanner.width * 0.5 - marginFirstTemplate/2
               to: 0
               duration: 20
               easing {type: Easing.InBack; overshoot: 1 }
               onStarted: {
                   secondTvBannerLabel.opacity = 0
               }
               onStopped: animateLiveTvfirtBannerWidthOff.start()
        }
        Image {
            id: secondBannerBackImage
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            fillMode: Image.PreserveAspectFit
            opacity: 0.2
        }
        Image {
            id: secondBannerForeImage
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            opacity: 0.65
        }
        Text {
            id: frChannelTvImg
            anchors.top: parent.top
            anchors.topMargin: -marginFirstTemplate
            anchors.left: parent.left
            font {
                family: localLiveTvFont.name
                pixelSize: parent.width * 0.4
            }
            opacity: 0.8
        }
        Text {
            id: frChannelName
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate/2
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate

            style: Text.Raised
            styleColor: "black"
            font {
                family: localAdventFont.name
                pixelSize: parent.width * 0.08
            }
            opacity:0.8
        }

        Image {
            id: frChannelPicon
            x: secondTvBanner.width * 0.35
            y: secondTvBanner.width * 0.06
            width: secondTvBanner.width * 0.6
            height: secondTvBanner.height * 0.6
            fillMode: Image.PreserveAspectFit
            NumberAnimation {
                   id: animateFrChannelOpacityOff
                   target: frChannelPicon
                   properties: "opacity"
                   from: 1
                   to: 0
                   duration: 200
                   onStopped:{
                       if(g_currentFrPicIdx+1 === frChannelPiconsArray.length) g_currentFrPicIdx = 0
                       else g_currentFrPicIdx = g_currentFrPicIdx + 1
                       frChannelName.text = frChannelNameArray[g_currentFrPicIdx]
                       frChannelPicon.source = frChannelPiconsArray[g_currentFrPicIdx]
                       animateFrChannelXOn.start()
                   }
            }
            NumberAnimation {
                   id: animateFrChannelXOn
                   target: frChannelPicon
                   properties: "y"
                   from: -secondTvBanner.width * 0.06
                   to: secondTvBanner.width * 0.06
                   duration: 200
                   easing {type: Easing.OutBack; overshoot: 1.8 }
                   onStarted: frChannelPicon.opacity = 1
                }
            }
    }
    Image {
        width: secondTvBanner.width * 1.13
        height: secondTvBanner.height * 1.13
        anchors.centerIn: secondTvBanner
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Text {
        id: secondTvBannerLabel
        anchors.left: secondTvBanner.left
        anchors.leftMargin: marginFirstTemplate*2
        anchors.top: secondTvBanner.bottom
        style: Text.Raised
        styleColor: "black"
        opacity: 0
        font {
            family: localFont.name
            pixelSize: secondTvBanner.width * 0.08
        }
    }

    Item {
        id: thirdTvBanner
        width: 0
        height: secondTvBanner.height
        anchors.left: secondTvBanner.right
        anchors.leftMargin: marginFirstTemplate
        anchors.top: secondTvBanner.top
        clip: true
        NumberAnimation {
               id: animateThirdTvBannerWidthOn
               target: thirdTvBanner
               properties: "width"
               from: 0
               to: firstTvBanner.width * 0.5 - marginFirstTemplate/2
               duration: 20
               easing {type: Easing.OutBack; overshoot: 0.5 }
               onStarted: {
               }
               onStopped: {
                   thirdTvBannerLabel.opacity = 1
               }
        }
        NumberAnimation {
               id: animateThirdTvBannerWidthOff
               target: thirdTvBanner
               properties: "width"
               from: firstTvBanner.width * 0.5 - marginFirstTemplate/2
               to: 0
               duration: 0
               easing {type: Easing.InBack; overshoot: 0.5 }
               onStarted: {
                   thirdTvBannerLabel.opacity = 0
               }
               onStopped: animateSecondTvBannerWidthOff.start()
        }
        Image {
            id: thirdTvBannerBackImage
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            fillMode: Image.PreserveAspectFit
            opacity: 0.2
        }
        Image {
            id: thirdTvBannerForeImage
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            opacity: 0.65
        }
        Text {
            id: arChannelTvImg
            anchors.top: parent.top
            anchors.topMargin: -marginFirstTemplate
            anchors.left: parent.left
            font {
                family: localLiveTvFont.name
                pixelSize: parent.width * 0.4
            }
            opacity:0.8
        }
        Text {
            id: arChannelName
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate/2
            x: thirdTvBanner.width -
               arChannelName.implicitWidth -
               marginFirstTemplate
            style: Text.Raised
            styleColor: "black"
            opacity:0.8
            font {
                family: localAdventFont.name
                pixelSize: parent.width * 0.08
            }
        }
        Image {
            id: arChannelPicon
            x: thirdTvBanner.width * 0.35
            y: thirdTvBanner.width * 0.06
            width: thirdTvBanner.width * 0.6
            height: thirdTvBanner.height * 0.6
            fillMode: Image.PreserveAspectFit
            NumberAnimation {
                   id: animateArChannelOpacityOff
                   target: arChannelPicon
                   properties: "opacity"
                   from: 1
                   to: 0
                   duration: 200
                   onStopped:{
                       if(g_currentArPicIdx+1 === arChannelPiconsArray.length) g_currentArPicIdx = 0
                       else g_currentArPicIdx = g_currentArPicIdx + 1
                       arChannelPicon.source = arChannelPiconsArray[g_currentArPicIdx]
                       arChannelName.text = arChannelNameArray[g_currentArPicIdx]
                       animateArChannelXOn.start()
                   }
            }
            NumberAnimation {
                   id: animateArChannelXOn
                   target: arChannelPicon
                   properties: "y"
                   from: -thirdTvBanner.width * 0.06
                   to: thirdTvBanner.width * 0.06
                   duration: 100
                   easing {type: Easing.OutBack; overshoot: 1 }
                   onStarted: arChannelPicon.opacity = 1
                }
            }
    }
    Image {
        width: thirdTvBanner.width * 1.13
        height: thirdTvBanner.height * 1.13
        anchors.centerIn: thirdTvBanner
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Text {
        id: thirdTvBannerLabel
        anchors.left: thirdTvBanner.left
        anchors.leftMargin: marginFirstTemplate*2
        anchors.top: thirdTvBanner.bottom
        style: Text.Raised
        styleColor: "black"
        opacity: 0
        font {
            family: localFont.name
            pixelSize: thirdTvBanner.width * 0.08
        }
    }
}
