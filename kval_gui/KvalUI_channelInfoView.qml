import QtQuick 2.0
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0
import QtGraphicalEffects 1.0

Item
{
    id: channelInfoView
    signal focusRelinquished()
    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

    enabled: false
    opacity: 0
    focus: false

    property string     currentStreamInPlay : "";
    property string     infoViewCurrentTime : "";
    property string     infoViewCurrentDate : "";
    //Epg Variables
    property string     currentEpgTitle      : "";
    property string     currentEpgTiming      : "";
    property int        epgCurrentDuration   : 0;
    property string     nextEpgTitle         : "";
    property string     nextEpgStartEnd      : "";
    property string     nextEpgDuration      : "";
    //Icons Stream properties
    property string     infoAspectImgSrc: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_ASPECT_1_78)
    property string     infoHdImgSrc: ""
    property string     infoAudioCodecSrc: ""
    property string     infoVideoCodecImgSrc: ""
    property string     infoResImgSrc: ""
    property string     infoResTxtSrc : ""
    property int        g_volumePercent: 100
    property bool       g_trickModeActivated: false
    property bool       g_trickModePaused: false

    property int        g_play_timer : 0
    property string     g_play_time: "00:00:00"
    property int        g_rec_timer : 0
    property string     g_rec_time : "00:00:00"
    property real       g_trickWritePercent: 0.0

    function metaDataHasChanged()
    {
        var mediaInfos = streamInfoMetaData.getUpdatedMeta()
        infoResTxtSrc = mediaInfos['resolution'];

        infoHdImgSrc = ""
        infoResImgSrc = ""
        var i = 0
        if(!mediaInfos)
        {
            logger.info("mediaInfos Not available yet !!")
            return
        }

        //Set Resolution
        var resTab = mediaInfos['resolution'].split("x")
        var height = 720
        if(resTab.length > 0)
        {
            height = parseInt(resTab[1], 10)
            var handledRes = mediaPlayerView.getResTab()
            for(i = 0; i < handledRes.length; i++)
            {
                if(height > handledRes[i][0] && height <= handledRes[i][1])
                {
                    infoResImgSrc = handledRes[i][2]
                    infoHdImgSrc = handledRes[i][3];
                    break
                }
            }
        }

        //Set Audio Codec
        var handledAudioDec = mediaPlayerView.getAudioCodecTab()
        for(i = 0; i < handledAudioDec.length; i++)
        {
            if(mediaInfos['Acodec'].indexOf(handledAudioDec[i][0]) !== -1)
            {
                infoAudioCodecSrc = handledAudioDec[i][1]
                break
            }
        }

        //Set Video Codec
        var handledVideoDec = mediaPlayerView.getVideoCodecTab()
        for(i = 0; i < handledVideoDec.length; i++)
        {
            if(mediaInfos['Vcodec'].indexOf(handledVideoDec[i][0]) !== -1)
            {
                infoVideoCodecImgSrc = handledVideoDec[i][1]
                break
            }
        }
    }

    function setTransitionRectState(state)
    {
        if(state === 0)
        {
            logger.info("Hide transition Rect");
            transitionRect.enabled = false;
            transitionRect.focus = false;
            transitionRect.opacity = 0;
        }
        else if(state === 1)
        {
            logger.info("Show transition Rect");
            transitionRect.focus = true;
            transitionRect.enabled = true;
            transitionRect.opacity = 1;
        }
    }

    function setCurrentChInfo(chIndex, chName, chPicon)
    {
        //Disable trick mode display
        if(trickModeProgressTimer.running) trickModeProgressTimer.stop()
        g_trickModeActivated = false
        g_trickModePaused = false
        trickModeItem.visible = false
        infoViewTrickRect.visible = false
        g_play_timer = 0
        g_play_time = "00:00:00"
        g_rec_timer = 0
        g_rec_time = "00:00:00"
        g_trickWritePercent = 0
        //---

        if(chIndex.indexOf("-1") !== -1)
        {channelNbrTxt.text = "";
        }
        else
        {channelNbrTxt.text = chIndex + " |";
        }
        currentStreamInPlay = chName;
        channelName.text = currentStreamInPlay;
        currentChPicon.source = chPicon;
    }

    function trickModeActivated()
    {
        return g_trickModeActivated
    }

    function getTotalTrickTime()
    {
        if(!g_trickModeActivated) return false
        return g_rec_timer
    }

    function setCurrentTrickTime(value)
    {
        if(!g_trickModeActivated) return false
        showSeek(value)

        g_play_timer = g_play_timer + value;
        if(g_play_timer < 0) g_play_timer = 0;
        if(g_play_timer > g_rec_timer) g_play_timer = g_rec_timer - 1;
    }

    function getCurrentTrickTime()
    {
        if(!g_trickModeActivated) return false
        return g_play_timer
    }

    function showSeek(value)
    {
        trickModeItem.visible = true
        g_trickModePaused = true;
        timeItem.visible = true
        downBandItem.visible = true
        infoViewChannelNbrItem.visible = true
        downBandItem.visible = true
        if(!trickModeProgressTimer.running)
            trickModeProgressTimer.start()

        opacity = 1;
        if(channelInfoBarTimer.running) channelInfoBarTimer.stop();
        infoViewTrickRect.visible = true
        infoViewTrickImg.source = (value < 0) ?
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PREVIOUS_SKIP):
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_NEXT_SKIP);
    }

    function showTrickModeInfoBar(isPause)
    {
        g_trickModeActivated = true
        trickModeItem.visible = true
        g_trickModePaused = isPause
        infoViewCurrentTime = Qt.formatTime(new Date(), "hh:mm");
        infoViewCurrentDate = Qt.formatDate(new Date(), "ddd \n dd. MMM");
        timeItem.visible = true
        downBandItem.visible = true
        infoViewChannelNbrItem.visible = true
        if(!trickModeProgressTimer.running)
            trickModeProgressTimer.start()

        opacity = 1;
        if(g_trickModePaused)
        {
            if(channelInfoBarTimer.running) channelInfoBarTimer.stop();
            infoViewTrickRect.visible = true
            infoViewTrickImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PAUSE);
        }
        else
        {
            infoViewTrickRect.visible = true
            infoViewTrickImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PLAY);
            channelInfoBarTimer.start();
        }
    }

    function showInfoBar()
    {
        if(g_trickModeActivated)
        {
            showTrickModeInfoBar(g_trickModePaused)
            return
        }

        logger.info("showInfoBar ...");
        infoViewCurrentTime = Qt.formatTime(new Date(), "hh:mm");
        infoViewCurrentDate = Qt.formatDate(new Date(), "ddd \n dd. MMM");
        timeItem.visible = true
        downBandItem.visible = true
        infoViewChannelNbrItem.visible = true
        opacity = 1;

        channelInfoBarTimer.start();
    }

    function hideInfoBar()
    {
        if(channelInfoBarTimer.running) channelInfoBarTimer.stop();
        opacity = 0.0;
        focus = false;
        enabled = false;
//        focusRelinquished();
    }

    function timerHideLocally()
    {
        if(channelInfoBarTimer.running) channelInfoBarTimer.stop();
        timeItem.visible = false
        downBandItem.visible = false
        infoViewChannelNbrItem.visible = false
    }

    function setChNumber(chNbr)
    {
        opacity = 1
        infoViewChannelNbrItem.visible = true
        channelNbrTxt.text = chNbr + " |";
    }

    function volumeUp()
    {
        opacity = 1
        if(volumeBarTimer.running) volumeBarTimer.stop();
        if(mediaPlayer.muted()) mediaPlayer.setMute(false);
        mediaPlayer.setVolume(2)
        g_volumePercent = mediaPlayer.volume()
        volumeIcon.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VOLUME_ICON)
        volumeItem.visible = true
        volumeBarTimer.start()
    }

    function volumeDown()
    {
        opacity = 1
        if(volumeBarTimer.running) volumeBarTimer.stop();
        if(mediaPlayer.muted()) mediaPlayer.setMute(false);
        mediaPlayer.setVolume(-2)
        g_volumePercent = mediaPlayer.volume()
        logger.info("volume: "+g_volumePercent)
        volumeIcon.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VOLUME_ICON)
        volumeItem.visible = true
        volumeBarTimer.start()
    }
    function setMute()
    {
        opacity = 1
        if(volumeBarTimer.running) volumeBarTimer.stop();
        if(mediaPlayer.muted())
        {
            mediaPlayer.setMute(false);
            volumeIcon.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VOLUME_ICON)
            volumeItem.visible = true
            volumeBarTimer.start()
        }
        else 
        {
            mediaPlayer.setMute(true);
            volumeIcon.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MUTE_ICON)
            volumeItem.visible = true
        }
    }

    function disableViewDisplay()
    {
        if(trickModeProgressTimer.running) trickModeProgressTimer.stop()
        g_trickModeActivated = false
        g_trickModePaused = false
        trickModeItem.visible = false
        g_play_timer = 0
        g_play_time = "00:00:00"
        g_rec_timer = 0
        g_rec_time = "00:00:00"
        g_trickWritePercent = 0

        if(volumeBarTimer.running) volumeBarTimer.stop();
        timerHideLocally()
        //if(mediaPlayer.muted()) return
        volumeItem.visible = false
    }

    function hideDownBar()
    {
        if(volumeBarTimer.running) volumeBarTimer.stop();
        timerHideLocally()
        //if(mediaPlayer.muted()) return
        volumeItem.visible = false
    }
    function startBuffering()
    {
        logger.info("Start Buffering...")
        if(trickModeProgressTimer.running) trickModeProgressTimer.stop()
        g_trickModeActivated = false
        g_trickModePaused = false
        trickModeItem.visible = false
        g_play_timer = 0
        g_play_time = "00:00:00"
        g_rec_timer = 0
        g_rec_time = "00:00:00"
        g_trickWritePercent = 0

        if(opacity === 0) opacity = 1
        bufferingRotationSpinnerAnim.start()
        bufferingRect.visible = true
    }

    function stopBuffering()
    {
        logger.info("Stop Buffering...")
        bufferingRotationSpinnerAnim.stop()
        bufferingRect.visible = false
    }

    Item {
        id: bufferingRect
        width: Utils.scaled(400)
        height: Utils.scaled(60)
        anchors.left: parent.left
        anchors.leftMargin: marginBlock * 4
        anchors.top: parent.top
        anchors.topMargin: marginBlock * 4
        visible: false
        Rectangle {
            anchors.fill: parent
            color: "black"
            radius: 5
            opacity: 0.8
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#1C1C1C" }
                GradientStop { position: 1.0; color: "black" }
            }
        }
        Image {
            id: bufferingSpinner
            anchors.left: parent.left
            anchors.leftMargin: marginBlock * 1.5
            anchors.verticalCenter: parent.verticalCenter
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BUSY_SPINNER)
            fillMode: Image.PreserveAspectFit
            scale: 0.6
            opacity: 1
            RotationAnimation on rotation
            {
                id: bufferingRotationSpinnerAnim
                running: false
                loops: Animation.Infinite
                direction: RotationAnimation.Clockwise
                from: 0
                to: 360
            }
        }

        Text{
            id: bufferingText
            anchors.left: bufferingSpinner.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: marginBlock * 0.6
            
            text: kvalUiConfigManager.retranslate + qsTr(" Chargement ...")
            color: "white"
            opacity: 1
            font {
                family: localAdventFont.name;
                pixelSize: parent.width * 0.13
            }
        }
        Image {
            width: parent.width *1.12
            height: parent.height *1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
    }


    Rectangle {
        id: transitionRect

        width: rootRectangle.width
        height: rootRectangle.height
        x: 0
        y: 0
        color: "black" //"#1C1C1C"
        focus:  false
        opacity: 0
    }

    Timer {
        id: channelInfoBarTimer
        interval: 8000;
        running: false;
        repeat: false;
        onTriggered: {
            timerHideLocally();
        }
    }

    Timer {
        id: trickModeProgressTimer
        interval: 1000;
        running: false;
        repeat: true;
        onTriggered: {
            g_trickWritePercent = kvalUiLiveStreamManager.getTrickWritePercent()
            String.prototype.lpad = function(padString, length) {
                var str = this;
                while (str.length < length)
                    str = padString + str;
                return str;
            }
            var hours = 0
            var minutes = 0
            var seconds = 0
            if(!g_trickModePaused)
            {
                g_play_timer = g_play_timer + 1
                g_play_timer %= 3600;
                hours = Math.floor(g_play_timer / 3600).toString()
                minutes = Math.floor(g_play_timer / 60).toString()
                seconds = (g_play_timer % 60).toString()
                g_play_time = hours.lpad('0', 2)+":"+minutes.lpad('0', 2)+":" + seconds.lpad('0', 2);
            }

            g_rec_timer = g_rec_timer + 1;
            g_rec_timer %= 3600;
            hours = Math.floor(g_rec_timer / 3600).toString()
            minutes = Math.floor(g_rec_timer / 60).toString();;
            seconds = (g_rec_timer % 60).toString();;
            g_rec_time = hours.lpad('0', 2)+":"+minutes.lpad('0', 2)+":"+seconds.lpad('0', 2);
        }
    }

    Timer {
        id: volumeBarTimer

        interval: 3000;
        running: false;
        repeat: false;

        onTriggered: {
            if(volumeBarTimer.running) volumeBarTimer.stop();
            volumeItem.visible = false
        }
    }

    Item {
        id: downBandItem
        visible: false
        Item {
            id: localMainRectangle
            width: rootRectangle.width;
            height: rootRectangle.height
            x: rootRectangle.x
            y: rootRectangle.y
        }
        Item {
            id: trickModeItem
            visible: false
            anchors.right: downBandRect.right
            anchors.rightMargin: marginBlock * 0.5
            anchors.bottom: downBandRect.top
            anchors.bottomMargin: marginBlock * 0.5
            width: downBandRect.width * 0.3
            height: downBandRect.height * 0.2
            DropShadow {
                 anchors.fill: trickModeRect
                 horizontalOffset: 0
                 verticalOffset: 0
                 radius: 8.0
                 samples: 17
                 color: "black"
                 source: trickModeRect
             }

            Rectangle {
                id: trickModeRect
                anchors.fill: parent
                radius: 5
                color: "#800f21"
            }
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width * g_trickWritePercent
                radius: Utils.scaled(5)
                color: "#e2314b"
            }
            Item {
                id: infoViewTrickRect
                anchors.left :  parent.left
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height * 0.9
                width: parent.width * 0.1
                opacity: 0.8
                visible: false
                Image {
                    id: infoViewTrickImg
                    anchors.centerIn :parent
                    width: parent.width
                    height : parent.height
                    fillMode: Image.PreserveAspectFit
                }
            }
            Text {
                id: trickTxt
                anchors.left: infoViewTrickRect.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: marginBlock * 0.35
                text: kvalUiConfigManager.retranslate + qsTr("LIVE PLAYBACK") + "  "
                color: "white"
                opacity: 0.8
                font
                {
                    family: localAdventFont.name;
                    bold: true
                    pixelSize: parent.height * 0.7;
                }
            }
            Item {
                id: recCurTimeTxt
                anchors.left: trickTxt.right
                width: parent.width * 0.23
                height: parent.height

                Text {
                    text: g_play_time
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: marginBlock * 0.35
                    anchors.left: parent.left
                    anchors.right: parent.right
                    color: "white"
                    opacity: 0.8
                    font
                    {
                        family: localAdventFont.name;
                        bold: true
                        pixelSize: trickTxt.font.pixelSize
                    }
                }
            }
            Item {
                id: trickTimeSepTxt
                anchors.left: recCurTimeTxt.right
                anchors.leftMargin: -marginBlock * 0.4
                width: parent.width * 0.05
                height: parent.height
                Text {
                    text: "-"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: marginBlock * 0.35
                    color: "white"
                    opacity: 0.8
                    font
                    {
                        family: localAdventFont.name;
                        bold: true
                        pixelSize: trickTxt.font.pixelSize
                    }
                }
            }
            Item {
                anchors.left: trickTimeSepTxt.right
                width: parent.width * 0.2
                height: parent.height
                Text {
                    text: g_rec_time
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: marginBlock * 0.35
                    anchors.left: parent.left
                    anchors.right: parent.right
                    opacity: 0.8
                    color: "white"
                    font
                    {
                        family: localAdventFont.name;
                        bold: true
                        pixelSize: trickTxt.font.pixelSize
                    }
                }
            }
        }

        Rectangle {
            id: downBandRect
            width: localMainRectangle.width;
            height: Utils.scaled(240)
            anchors.left: localMainRectangle.left;
            anchors.right: localMainRectangle.right;
            anchors.bottom: localMainRectangle.bottom;
            color: "black"
            opacity: 0.8
        }
        Item {
            id: currentPiconRect
            width: Utils.scaled(270)
            height: Utils.scaled(200)
            anchors.left: downBandRect.left;
            anchors.top: downBandRect.top;
            Image {
                id: backImgPicon
                anchors.centerIn: parent
                width: parent.width * 0.9
                height: parent.height * 0.9
                fillMode: Image.PreserveAspectFit
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.PICON_CHANNEL_ABS_PATH) +
                        "back" +
                        ".png";
            }
            Image {
                id: currentChPicon

                anchors.horizontalCenter: backImgPicon.horizontalCenter
                anchors.verticalCenter: backImgPicon.verticalCenter
                anchors.verticalCenterOffset: (sourceSize.height > backImgPicon.height) ?
                                              (-backImgPicon.height *0.05) :0
                width: (sourceSize.height > backImgPicon.height) ?
                       (backImgPicon.width * 0.9) : backImgPicon.width
                height: (sourceSize.height > backImgPicon.height) ?
                        (backImgPicon.height * 0.6) : backImgPicon.height
                fillMode: Image.PreserveAspectFit
                onStatusChanged: {
                    if(status === Image.Error)
                    {
                        currentChPicon.source =
                        kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.PICON_CHANNEL_ABS_PATH) +
                                "default" +
                                ".png";
                    }
                }
            }
        }

        Rectangle {
            id: channelEpgProgressBar

            x: currentPiconRect.x + currentPiconRect.width
            y: currentPiconRect.y + 100
            width: (downBandRect.width) - (currentPiconRect.width) - (marginBlock*3)
            height: 1
            color: "white"
            opacity: 1
        }

        Rectangle {
            id: channelEpgCurrentElapsedBar
            anchors.verticalCenter: channelEpgProgressBar.verticalCenter
            anchors.left: channelEpgProgressBar.left
            width: (epgCurrentDuration/100) * channelEpgProgressBar.width
            height: 5
            color: "white"
            opacity: 1
        }

        Rectangle {
            id: currentEpgTimeRect
    
            anchors.top : downBandRect.top
            anchors.bottom : channelEpgProgressBar.top
            anchors.right : channelEpgProgressBar.right
            width: channelEpgProgressBar.width/7
            color: "transparent"
            Text{
                text: currentEpgTiming
                width: parent.width
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                font {
                    family: localFont.name;
                    pixelSize: 30;
                }
                color: "white"
            }
        }
        Rectangle {
            id: currentEpgTitleRect
    
            anchors.top : downBandRect.top
            anchors.bottom : channelEpgProgressBar.top
            anchors.left : channelEpgProgressBar.left
            anchors.right : currentEpgTimeRect.left
            color: "transparent"
            opacity: 1
            Text{
                text: currentEpgTitle
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                font {
                    family: localFont.name;
                    pixelSize: 54;
                }
                color: "white"
            }
        }
    
        Rectangle {
            id: nextEpgTimeRect
            anchors.top : channelEpgProgressBar.down
            anchors.bottom : currentChannelLittleIconsRect.top
            anchors.right : channelEpgProgressBar.right
            height: currentEpgTimeRect.height*0.6
            width: currentEpgTimeRect.width
            color: "transparent"
            Text{
                text: nextEpgDuration
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                font {
                    family: localFont.name;
                    pixelSize: 30;
                }
                opacity: 0.5
                color: "white"
            }
        }
        Rectangle {
            id: nextEpgTitleRect
    
            anchors.bottom : currentChannelLittleIconsRect.top
            anchors.top : channelEpgProgressBar.bottom
            anchors.left : channelEpgProgressBar.left
            anchors.right : nextEpgTimeRect.left
            color: "transparent"
            Text{
                id: epgNextstartEndTxt
                text: nextEpgStartEnd
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                font {
                    family: localFont.name;
                    pixelSize: 35;
                }
                opacity: 0.5
                color: "white"
            }
        }
        Text{
            text: nextEpgTitle
            width: nextEpgTitleRect.width - epgNextstartEndTxt.contentWidth
            anchors.verticalCenter: nextEpgTitleRect.verticalCenter
            x: nextEpgTitleRect.x + epgNextstartEndTxt.contentWidth + marginBlock
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            font {
                family: localFont.name;
                pixelSize: 35;
            }
            opacity: 0.5
            color: "white"
        }
    
        Item {
            id: currentChannelLittleIconsRect
            height: currentEpgTitleRect.height * 0.8
            anchors.bottom : downBandRect.bottom
            anchors.bottomMargin: marginBlock
            anchors.right : downBandRect.right
            anchors.rightMargin : marginBlock * 7.5
            width : channelEpgProgressBar.width * 0.3

            opacity: 0.8
            Item {
                id: infoAspectRect
                anchors.right : parent.right
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    id: infoAspectImg
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    source: infoAspectImgSrc
                }
            }
            Item {
                id: infoHdImgRect
                anchors.right : infoAspectRect.left
                anchors.rightMargin : marginBlock+marginBlock/2
                height: Utils.scaled(50)
                width: Utils.scaled(50)
                anchors.verticalCenter: parent.verticalCenter
                scale: 0.8

                Image {
                    id: infoHdImg
                    anchors.centerIn: parent
                    source: infoHdImgSrc
                }
            }
            Item {
                id: infoAudioCodecImgRect
                anchors.right : infoHdImgRect.left
                anchors.rightMargin : marginBlock+marginBlock/2
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    id: infoCodecAudioImg
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    source: infoAudioCodecSrc
                }
            }
            Item {
                id: infoVideoCodecImgRect
                anchors.right : infoAudioCodecImgRect.left
                anchors.rightMargin : marginBlock+marginBlock/2
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter
                Image {
                    id: infoVideoCodecImg
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    source: infoVideoCodecImgSrc
                }
            }
            Item {
                id: infoResImgRect
                anchors.right : infoVideoCodecImgRect.left
                anchors.rightMargin : marginBlock+marginBlock/2
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    id: infoResImg
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    source: infoResImgSrc
                }
            }
        }
    }
    
    Item {
        id: timeItem
        visible: false

        Item {
            id: channelEpgProgressItem

            x: currentPiconRect.x + currentPiconRect.width
            y: currentPiconRect.y + 100
            width: (downBandRect.width) - (currentPiconRect.width) - (marginBlock*3)
            height: 1
        }

        Rectangle {
            id: infoViewLocalTimeRect
            anchors.right : channelEpgProgressItem.right
            y: Utils.scaled(50)
            height: Utils.scaled(100)
            width: Utils.scaled(250)
            color: "black"
            opacity: 0.8
            Text {
                id: infoViewLocalTimeTxt
                width: parent.width - marginBlock
                height: parent.height - marginBlock
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -marginBlock*2
                horizontalAlignment: Text.AlignLeft
                text: infoViewCurrentTime
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(90)
                }
            }
        }
        Rectangle {
            id: infoViewLocalDateRect
            anchors.right : infoViewLocalTimeRect.left
            y: Utils.scaled(53.5)
            height: Utils.scaled(93)
            width: Utils.scaled(150)
            color: highlightColor
            opacity: 0.8
            Text {
                id: infoViewLocalDateTxt
                width: parent.width - marginBlock
                height: parent.height
                horizontalAlignment: Text.AlignRight
                text: infoViewCurrentDate
                color : "white"
                opacity: 0.5
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(32)
                }
            }
        }
    }
    Item {
        id: infoViewChannelNbrItem
        anchors.left : parent.left
        y: infoViewLocalDateRect.y
        height: infoViewLocalDateRect.height
        anchors.leftMargin: marginBlock * 4
        width: parent.width * 0.25

        Text{
            id: channelNbrTxt
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: marginBlock
            style: Text.Outline
            styleColor: "grey"
            color: "white"
            opacity: 0.7
            font {
                family: localAdventFont.name;
                pixelSize: parent.height * 0.7
            }
        }
        DropShadow {
             anchors.fill: channelNbrTxt
             horizontalOffset: 0
             verticalOffset: 0
             opacity: 0.7
             radius: 8.0
             samples: 17
             color: "black"
             source: channelNbrTxt
         }

        Text{
            id: channelName
            anchors.left: channelNbrTxt.right
            anchors.verticalCenter: parent.verticalCenter
            width: rootRectangle.width * 0.4
            elide: Text.ElideRight
            style: Text.Outline
            styleColor: "grey"
            color: "white"
            opacity: 0.7
            font
            {
                family: localAdventFont.name;
                pixelSize: channelNbrTxt.font.pixelSize
                capitalization: Font.AllUppercase
            }
        }
        DropShadow {
             anchors.fill: channelName
             horizontalOffset: 0
             verticalOffset: 0
             opacity: 0.7
             radius: 8.0
             samples: 17
             color: "black"
             source: channelName
         }
    }
    Item {
        id: volumeItem

        anchors.left : parent.left
        anchors.leftMargin: marginBlock * 4
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -marginBlock*2
        height: parent.height * 0.45
        width: parent.width * 0.013
        opacity: 0.9
        visible: false

        Rectangle {
            anchors.fill: parent
            border.width: Utils.scaled(2)
            border.color: "grey"
            color: "transparent"
            radius: 2
            Rectangle {
                id: volumeBarRect
    
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
                anchors.left: parent.left
                anchors.leftMargin: 2
                width: parent.width - 4
                height: (g_volumePercent * (parent.height-4)) / 100
                color: "white"
                radius: 2
            }
        }

        Image {
            id: volumeIcon
            anchors.bottom: parent.top
            anchors.bottomMargin: -marginBlock*2
            anchors.left: parent.left
            anchors.leftMargin: -marginBlock*6
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VOLUME_ICON)
            scale: 0.5
        }
    }
}
