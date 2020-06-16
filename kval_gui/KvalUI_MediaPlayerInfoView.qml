import QtQuick 2.0
import QtGraphicalEffects 1.0
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0

Item
{
    id: mediaPlayerInfoView
    signal focusRelinquished()
    signal askForStopNotify()
    enabled: false
    opacity: 0
    focus: false
    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

    property string     currentStreamInPlay : "";
    //Icons Stream properties
    property string     infoAspectImgSrc: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_ASPECT_1_78);
    property string     infoHdImgSrc: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_HD_OFF);
    property string     infoAudioCodecSrc: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_AAC);
    property string     infoVideoCodecImgSrc: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_H264);
    property string     infoResImgSrc: ""
    
    property string     infoDolbyImgSrc: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_DOLBY_OFF);
    property string     infoSubsImgSrc: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_SUBS_OFF);
    property string     currentProgressTime: "";
    property int        currentDurationPercent: 0;
    property int        currentDownloadPercent:0;
    property bool       isPaused: false;
    property int        g_volumePercent: 0
    property int        g_current_duration: 0
    property var        g_nextVideoDisplayDone: null

    function getResTab()
    {
        var resTab = [
        [1, 480, kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_480P), kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_SD)],
        [480, 540, kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_540P), kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_SD)],
        [540, 576, kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_576P), kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_SD)],
        [576, 720, kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_720P), kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_HD)],
        [720, 1080, kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_1080P), kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_HD)],
        [1080, 1440, kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_1440P), kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_4K)],
        [1440, 2160, kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_2160P), kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_4K)] ]
        return resTab
    }

    function getAudioCodecTab()
    {
        var audioCodecTab = [
        ["aac", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_AAC)],
        ["ac3", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_AC3)],
        ["dtshd_hra", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_DTSHDHRA)],
        ["dtshd_ma", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_DTSHDMA)],
        ["dts", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_DTS)],
        ["wma", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_WMA)],
        ["flac", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_FLAC)],
        ["mp2", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_MP2)],
        ["mp3", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_MP3)],
        ["lpcm", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_LPCM)],
        ["opus", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_OPUS)],
        ["vorbis", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_VORBIS)] ]
        return audioCodecTab
    }

    function getVideoCodecTab()
    {
        var videoCodecTab = [
        ["avc1", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_H264)],
        ["h264", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_H264)],
        ["hevc", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_H265)],
        ["h265", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_H265)],
        ["mpeg2", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_MPEG2)],
        ["vc1", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_VC1)],
        ["vp9", kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_CODEC_VP9)] ]
        return videoCodecTab
    }

    function metaDataHasChanged()
    {
        infoHdImgSrc = ""
        infoResImgSrc = ""
        var mediaInfos = streamInfoMetaData.getUpdatedMeta()
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
            var handledRes = getResTab()
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
        var handledAudioDec = getAudioCodecTab()
        for(i = 0; i < handledAudioDec.length; i++)
        {
            if(mediaInfos['Acodec'].indexOf(handledAudioDec[i][0]) !== -1)
            {
                infoAudioCodecSrc = handledAudioDec[i][1]
                break
            }
        }

        //Set Video Codec
        var handledVideoDec = getVideoCodecTab()
        for(i = 0; i < handledVideoDec.length; i++)
        {
            if(mediaInfos['Vcodec'].indexOf(handledVideoDec[i][0]) !== -1)
            {
                infoVideoCodecImgSrc = handledVideoDec[i][1]
                break
            }
        }

        //Set Video Aspect
    }

    function focusRelinquish()
    {
        showInfoBar()
    }

    function setCurrentMediaInfo(mediaName, mediaPlot)
    {
        currentStreamInPlay = mediaName;
        currentMediaName.text = currentStreamInPlay;
        currentMediaTitle.text = currentStreamInPlay;
        currentMediaPlotTxt.text = mediaPlot;
    }

    function invokerInfoBar()
    {
        logger.info("invokerInfoBar...");
        extractStreamPositionDuration()
    }

    function showInfoBar()
    {
        infoViewNextVideoInfos.visible = false
        opacity = 1;
        mediaPlayerInfosItem.opacity = 1
        activeFocusViewManager(mediaPlayerView)
        if(!channelInfoBarTimer.running)
        {
            channelInfoBarTimer.start();
        }
        if(!progressPositionTimer.running)
        {
            progressPositionTimer.start();
            progressPositionTimer.repeat = true;
        }
    }

    function extractStreamPositionDuration()
    {
        var stream_pos_map = mediaPlayer.getCurrentStreamPosition();
        if(stream_pos_map &&
            "position" in stream_pos_map &&
            "duration" in stream_pos_map)
        {
            streamPositionReady(stream_pos_map["position"],
                                stream_pos_map["duration"])
        }
    }

    function reinitValues()
    {
        currentDurationPercent = 0
        currentDownloadPercent = 0
        hideInfoBar()
    }

    function hideInfoBar()
    {
        isPaused = false
        if(channelInfoBarTimer.running) channelInfoBarTimer.stop()
        if(progressPositionTimer.running)
        {
            progressPositionTimer.stop();
            progressPositionTimer.repeat = false;
        }

        infoViewHintItem.visible = false
        if(hintInfoTimer.running) hintInfoTimer.stop()
        if(infoViewNextVideoInfosTimer.running) infoViewNextVideoInfosTimer.stop()
        infoViewNextVideoInfos.visible = false;
        opacity = 0;
        mediaPlayerInfosItem.opacity = 0
        focus = false;
        enabled = false;
        focusRelinquished();
    }

    function restoreFocus()
    {
        opacity = 0;
        mediaPlayerInfosItem.opacity = 0
        activeFocusViewManager(mediaPlayerView)
    }

    function mediaplayerBackKeyPressed()
    {
    }

    function streamPositionReady(curPosition, curDuration)
    {
        String.prototype.lpad = function(padString, length) {
            var str = this;
            while (str.length < length)
                str = padString + str;
            return str;
        }

        //QtMultimedia Methods
        var position = curPosition/1000;
        var posHours = ( Math.floor(position/3600)).toString();
        var posMinutes = (Math.floor((position/60)%60)).toString();
        var posSeconds = (Math.floor(position%60)).toString();
        var positionStr =   posHours.lpad('0', 2)+":"+
                            posMinutes.lpad('0', 2)+":"+
                            posSeconds.lpad('0', 2);

        var duration = curDuration/1000;
        g_current_duration = duration;

        var durHours = ( Math.floor(duration/3600)).toString();
        var durMinutes = (Math.floor((duration/60)%60)).toString();
        var durSeconds = (Math.floor(duration%60)).toString();

        var durationStr =   durHours.lpad('0', 2)+":"+
                            durMinutes.lpad('0', 2)+":"+
                            durSeconds.lpad('0', 2);

        currentMediaDurationText.text = durationStr;
        currentProgressTimeTxt.text = positionStr;
        currentDurationPercent = position*100/duration;
        showInfoBar();
    }
    function streamSeeked()
    {
        isPaused = false;
        showInfoBar()
        infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PLAY);
    }

    function setDownloadValue(value)
    {currentDownloadPercent = value;
    }

    function mediaplayerPlayKeyPressed()
    {
        isPaused = false;
        if(isPaused)
        {infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PAUSE);
        }
        else
        {infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PLAY);
        }
        showInfoBar();
        playCurrentMedia();
    }
    function mediaplayerPauseKeyPressed()
    {
        logger.info("mediaPlayer Pause")
        isPaused = true;
        if(isPaused)
        {infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PAUSE);
        }
        else
        {infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PLAY);
        }
        extractStreamPositionDuration();
        pauseCurrentMedia();
    }
    function mediaplayerStopKeyPressed()
    {
        if(infoViewNextVideoInfosTimer.running)
        {
            infoViewNextVideoInfosTimer.stop()
            g_nextVideoDisplayDone(false)
            return
        }

        askForStopNotify();
        currentMediaDurationText.text = "";
        currentProgressTimeTxt.text = "";
        infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PLAY);
        currentDurationPercent = 0;
        currentDownloadPercent = 0;
    }
    function mediaplayerNextKeyJumpPressed(pos)
    {
        logger.info("mediaPlayer next jump")
        if (mediaProgressDownloadedBar.width < mediaBackProgressBar.width)
        {
            if (  mediaProgressDownloadedBar.width === 0 &&
                  mediaProgressBar.width === 0)
            {
                showInfoBar();
                return
            }
            else if(mediaProgressDownloadedBar.width === 0 &&
                    mediaProgressBar.width > 0)
            {
                logger.info("live content continue")
            }
            else if(mediaProgressDownloadedBar.width - mediaProgressBar.width < 80)
            {
                showInfoBar();
                return
            }
        }
        isPaused = true;
        showInfoBar();
        infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_NEXT_SKIP);
        if (pos === -1)
        {
            mediaPlayer.seek(30, false);
        }
        else
        {
            var jump_position = g_current_duration * (pos/10);
            logger.info("Jump to position: " + jump_position)
            mediaPlayer.jump(jump_position);
        }
    }
    function mediaplayerPreviousKeyJumpPressed()
    {
        logger.info("mediaPlayer back jump")
        isPaused = true;
        showInfoBar();
        infoViewPlayPauseImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PREVIOUS_SKIP);
        mediaPlayer.seek(-30, false);
    }

    function showNextVideoInfos(item, cb)
    {
        infoViewNextVideoLabel.text = item["label"]
        infoViewNextVideoThumb.source = item["thumb"]
        mediaPlayerInfosItem.opacity = 0
        infoViewNextVideoInfos.visible = true
        backRectRefLoading.width = 0
        g_nextVideoDisplayDone = cb
        infoViewNextVideoInfosTimer.start()
    }

    function mediaplayerExitKeyPressed()
    {
        progressPositionTimer.stop();
        progressPositionTimer.repeat = false;
        if(!isPaused)
        {mediaPlayerInfosItem.opacity = 0;
        }

        if(hintInfoTimer.running) hintInfoTimer.stop()
        if(infoViewHintItem.visible) hintDisplayAnimOff.start()
        if(infoViewNextVideoInfosTimer.running || infoViewNextVideoInfos.visible)
        {
            infoViewNextVideoInfosTimer.stop()
            g_nextVideoDisplayDone(false)
        }
    }

    function displayHintNotif()
    {
        if(streamInfoMetaData.getAudioTrackNbr() > 1 || streamInfoMetaData.getSubsTrackNbr() > 0)
            return true

        return false
    }

    //KeyBoard handling
    Keys.onPressed:
    {
        if (event.key === Qt.Key_Up) //Up Button
        { }
        else if (event.key === Qt.Key_Down) //Down Button
        { }
        else if (event.key === Qt.Key_Right) //Right Button
        { }
        else if (event.key === Qt.Key_Left) //Left Button
        { }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {mediaplayerBackKeyPressed()
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {extractStreamPositionDuration()
        }
        else if (event.key === Qt.Key_Escape)
        {mediaplayerExitKeyPressed()
        }
        else if(event.key === Qt.Key_P) //Play Button
        {mediaplayerPlayKeyPressed()
        }
        else if (event.key === Qt.Key_Space) //Pause Button
        {mediaplayerPauseKeyPressed()
        }
        else if (event.key === Qt.Key_S) //Stop Button
        {mediaplayerStopKeyPressed()
        }
        else if (event.key === Qt.Key_I) //blue Button
        {
            hideInfoBar();
            streamInfoMetaData.showMetaData(mediaPlayerView);
        }
        else if (event.key === Qt.Key_F) //skip next Button
        {mediaplayerNextKeyJumpPressed(-1);
        }
        else if (event.key === Qt.Key_W) //skip previous Button
        {mediaplayerPreviousKeyJumpPressed();
        }
        else if (event.key === Qt.Key_U) 
        {volumeUp();
        }
        else if (event.key === Qt.Key_D) 
        {volumeDown();
        }
        else if (event.key === Qt.Key_N) 
        {setMute();
        }
        else if (event.key === Qt.Key_0)
        {mediaplayerNextKeyJumpPressed(0);
        }
        else if (event.key === Qt.Key_1)
        {mediaplayerNextKeyJumpPressed(1);
        }
        else if (event.key === Qt.Key_2)
        {mediaplayerNextKeyJumpPressed(2);
        }
        else if (event.key === Qt.Key_3)
        {mediaplayerNextKeyJumpPressed(3);
        }
        else if (event.key === Qt.Key_4)
        {mediaplayerNextKeyJumpPressed(4);
        }
        else if (event.key === Qt.Key_5)
        {mediaplayerNextKeyJumpPressed(5);
        }
        else if (event.key === Qt.Key_6)
        {mediaplayerNextKeyJumpPressed(6);
        }
        else if (event.key === Qt.Key_7)
        {mediaplayerNextKeyJumpPressed(7);
        }
        else if (event.key === Qt.Key_8)
        {mediaplayerNextKeyJumpPressed(8);
        }
        else if (event.key === Qt.Key_9)
        {mediaplayerNextKeyJumpPressed(9);
        }
        else {}
        event.accepted = true;
    }

    Timer {
        id: channelInfoBarTimer
        interval: 8000;
        running: true;
        repeat: false;
        onTriggered: {
            progressPositionTimer.stop();
            progressPositionTimer.repeat = false;

            if(displayHintNotif())
            {
                hintDisplayAnimOn.start()
                hintInfoTimer.start()
                infoViewHintItem.visible = true
            }

            if(!isPaused)
            {mediaPlayerInfosItem.opacity = 0;
            }
        }
    }
    Timer {
        id: progressPositionTimer
        interval: 1000;
        running: false;
        repeat: false;
        onTriggered:
        {
            if(!isPaused)
            {
                extractStreamPositionDuration();
            }
        }
    }

    Item {
        id: infoViewNextVideoInfos
        anchors.fill: parent
        visible: false

        Timer {
            id: infoViewNextVideoInfosTimer
            interval: 8000;
            running: false;
            repeat: false;
            onTriggered:
            {
                if(g_nextVideoDisplayDone)
                    g_nextVideoDisplayDone(true);
            }
        }

        Rectangle {
            id: backRectRefLoading
            height: backRectRef.height - 2
            width: backRectRef.width
            anchors.top : parent.top
            anchors.topMargin: parent.width * 0.35
            x : backRectRef.x
            visible: parent.visible
            gradient: Gradient {
                GradientStop { position: 1.0; color: "#800f21" }
                GradientStop { position: 0.0; color: "red" }
            }
            opacity: 0.7
            Behavior on width {
                PropertyAnimation{
                    target: backRectRefLoading;
                    properties: "width";
                    from : 0
                    to: backRectRef.width
                    duration: 8000
                }
            }
        }

        Rectangle {
            id: backRectRef
            height: itemBlockImg.height
            width: parent.width * 0.4
            anchors.top : parent.top
            anchors.topMargin: parent.width * 0.35
            x : parent.width - (width*0.9)
            visible: true
            border.width: 1
            border.color: "#585858"
            gradient: Gradient {
                GradientStop { position: 1.0; color: "#141414" }
                GradientStop { position: 0.0; color: "#393939" }
            }
            opacity: 0.7
        }
        Image {
            id: infoViewNextVideoThumb
            anchors.left: itemBlockImg.left
            anchors.leftMargin: itemBlockImg.width * 0.0269
            anchors.right: itemBlockImg.right
            anchors.rightMargin: itemBlockImg.width * 0.0269
            anchors.top: itemBlockImg.top
            anchors.topMargin: itemBlockImg.height * 0.05
            anchors.bottom: itemBlockImg.bottom
            anchors.bottomMargin: itemBlockImg.height * 0.05
            fillMode: Image.Stretch
        }

        Image {
            id: itemBlockImg
            anchors.verticalCenter: backRectRef.verticalCenter
            anchors.left: backRectRef.left
            anchors.leftMargin: -(marginBlock * 0.3)
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LISTRECTANGLEWARE_REF)
        }
        Text{
            id: infoViewNextVideoHead
            anchors.top: itemBlockImg.top
            anchors.topMargin: marginBlock
            anchors.right: parent.right
            anchors.left: itemBlockImg.right
            textFormat: Text.AutoText
            elide:Text.ElideRight
            text: kvalUiConfigManager.retranslate + qsTr("Next Video...")
            font
            {
                family: localAdventFont.name;
                pixelSize: backRectRef.width * 0.04;
            }
            color: "white"
        }
        Text{
            id: infoViewNextVideoLabel
            anchors.top: infoViewNextVideoHead.bottom
            anchors.topMargin: marginBlock*0.5
            anchors.right: parent.right
            anchors.left: itemBlockImg.right
            height: backRectRef.height * 0.7
            textFormat: Text.AutoText
            elide:Text.ElideRight
            wrapMode: Text.WordWrap
            font
            {
                family: localFont.name;
                pixelSize: backRectRef.width * 0.035;
            }
            color: "white"
            opacity: 0.8
        }

    }

    Item {
        id: infoViewHints
        anchors.fill: parent

        Timer {
            id: hintInfoTimer
            interval: 8000;
            running: false;
            repeat: false;
            onTriggered:
            {
                hintDisplayAnimOff.start();
            }
        }

        NumberAnimation {
               id: hintDisplayAnimOn
               target: infoViewHintItem
               running: false
               properties: "x"
               easing {type: Easing.InBack}
               from: mediaPlayerInfoView.width
               to: mediaPlayerInfoView.width - infoViewHintItem.width
               duration: 200
        }

        NumberAnimation {
               id: hintDisplayAnimOff
               target: infoViewHintItem
               running: false
               properties: "x"
               easing {type: Easing.InBack}
               from: mediaPlayerInfoView.width - infoViewHintItem.width
               to: mediaPlayerInfoView.width
               duration: 200
               onStopped: infoViewHintItem.visible = false
        }

        Item {
            id: infoViewHintItem

            x : parent.width
            anchors.top : parent.top
            anchors.topMargin: parent.width * 0.15
            height: parent.height * 0.14
            width: parent.width * 0.25
            visible: false

            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.PLAYER_HINTS_BACK )
            }

            DropShadow {
                anchors.fill: infoViewHintIcon
                horizontalOffset: 0
                verticalOffset: 0
                radius: 8.0
                samples: 17
                color: "black"
                source: infoViewHintIcon
            }

            Image {
                id: infoViewHintIcon
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_MISC_ICO )
            }

            Text {
                id: infoViewHintText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: infoViewHintIcon.right
                anchors.leftMargin: marginBlock * 2
                anchors.right: parent.right

                horizontalAlignment: Text.AlignLeft
                elide:Text.ElideRight
                textFormat: Text.AutoText
                wrapMode: Text.WordWrap

                text: kvalUiConfigManager.retranslate +
                      kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_PLAYBACK_HINT);
                color: "white"
                font {
                    family: localAdventFont.name;
                    pixelSize: parent.width * 0.06
                }
            }
        }
    }

    Item {
        id: mediaPlayerInfosItem

        Text{
            id: currentMediaName
            x: marginBlock*3
            anchors.bottom: downBandRect.top
            textFormat: Text.RichText
            font
            {
                family: localFont.name; pixelSize: 80;
                capitalization: Font.AllUppercase
            }
            color: "white"
            opacity: 0 //Don't display for now
        }
        Item {
            id: localMainRectangle
            width: rootRectangle.width;
            height: rootRectangle.height
            x: rootRectangle.x
            y: rootRectangle.y
        }
    
        Rectangle {
            id: downBandRect
            width: localMainRectangle.width;
            height: Utils.scaled(210);
            anchors.left: localMainRectangle.left;
            anchors.right: localMainRectangle.right;
            anchors.bottom: localMainRectangle.bottom;
            color: "transparent"
            opacity: 0.8
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#363636"
                opacity: 1
            }

            Image {
                anchors.fill: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_VIGNETTE_BOTTOM)
                opacity: 0.7
            }
            Image {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.topMargin: -marginBlock*3
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_RED)
            }
        }
    
        Rectangle {
            id: mediaBackProgressBar
            x: downBandRect.x + marginBlock*20
            y: downBandRect.y + marginBlock*10
            width: downBandRect.width - marginBlock*40
            height: 1
            color: "white"
            opacity: 1
        }
        Rectangle {
            id: mediaProgressDownloadedBar
            anchors.verticalCenter: mediaBackProgressBar.verticalCenter
            anchors.left: mediaBackProgressBar.left
            width: (currentDownloadPercent/100) * mediaBackProgressBar.width
            height: 3
            color: "#c3461b"
            opacity: 1
        }
        Rectangle {
            id: mediaProgressBar
            anchors.verticalCenter: mediaBackProgressBar.verticalCenter
            anchors.left: mediaBackProgressBar.left
            width: (currentDurationPercent/100) * mediaBackProgressBar.width
            height: 5
            color: "#0489B1"
            opacity: 1
        }
    
        Item {
            id: currentMediaProgressTime
            anchors.right : mediaBackProgressBar.left
            anchors.bottom : mediaBackProgressBar.bottom
            anchors.bottomMargin : marginBlock-2
            anchors.rightMargin : marginBlock*2
            width: mediaBackProgressBar.width/7

            Text{
                id: currentProgressTimeTxt
                text: ""
                width: parent.width
                horizontalAlignment: Text.AlignRight
                anchors.verticalCenter: parent.verticalCenter
                font {
                    family: localFont.name;
                    pixelSize: 30;
                }
                color: "white"
            }
        }
        Rectangle {
            id: currentMediaDurationRect
            anchors.left : mediaBackProgressBar.right
            anchors.leftMargin : marginBlock*2
            anchors.bottom : mediaBackProgressBar.bottom
            anchors.bottomMargin : marginBlock-2
            width: mediaBackProgressBar.width/7
            color: "transparent"
            Text{
                id: currentMediaDurationText
                text: ""
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                anchors.verticalCenter: parent.verticalCenter
                font {
                    family: localFont.name;
                    pixelSize: 30;
                }
                color: "white"
            }
        }
    
        Rectangle {
            id: currentMediaNameRect
            anchors.bottom : mediaBackProgressBar.top
            anchors.bottomMargin : marginBlock * 5
            anchors.left : mediaBackProgressBar.left
            anchors.right : mediaBackProgressBar.right
            height: 50
            color: "transparent"
            Text{
                id: currentMediaTitle
                text: ""
                width: parent.width
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                textFormat: Text.RichText
                font {
                    family: localFont.name;
                    pixelSize: 60;
                }
                color: "white"
            }
        }
        Rectangle {
            id: currentMediaPlotRect
            anchors.top : mediaBackProgressBar.bottom
            anchors.topMargin : marginBlock
            anchors.left : mediaBackProgressBar.left
            anchors.right : currentChannelLittleIconsRect.left
            anchors.bottom : currentChannelLittleIconsRect.bottom
            color: "transparent"
            Text{
                id: currentMediaPlotTxt
                text: ""
                width: parent.width*0.8
                height: parent.height
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                opacity: 0.5
                font {
                    family: localFont.name;
                    pixelSize: 28;
                }
                color: "white"
            }
        }

        Rectangle {
            id: currentChannelLittleIconsRect
            height: currentMediaNameRect.height * 0.8
            anchors.bottom : downBandRect.bottom
            anchors.bottomMargin: marginBlock
            anchors.right : downBandRect.right
            anchors.rightMargin : marginBlock * 7.5
    
            width : mediaBackProgressBar.width * 0.3
            color: "transparent"
            opacity: 0.8
            Rectangle {
                id: infoAspectRect
                anchors.right : parent.right
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter
                color: "transparent"
                opacity: 1
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
            Rectangle {
                id: infoAudioCodecImgRect
                anchors.right : infoHdImgRect.left
                anchors.rightMargin : marginBlock+marginBlock/2
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter
                color: "transparent"
                opacity: 1
                Image {
                    id: infoCodecAudioImg
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    source: infoAudioCodecSrc
                }
            }
            Rectangle {
                id: infoVideoCodecImgRect
                anchors.right : infoAudioCodecImgRect.left
                anchors.rightMargin : marginBlock+marginBlock/2
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter
                color: "transparent"
                opacity: 1
                Image {
                    id: infoVideoCodecImg
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    source: infoVideoCodecImgSrc
                }
            }
            Rectangle {
                id: infoResImgRect
                anchors.right : infoVideoCodecImgRect.left
                anchors.rightMargin : marginBlock+marginBlock/2
                height: Utils.scaled(40)
                width: Utils.scaled(102.5)
                anchors.verticalCenter: parent.verticalCenter
                color: "transparent"
                opacity: 1
                Image {
                    id: infoResImg
                    anchors.fill: parent
                    fillMode: Image.Stretch
                    source: infoResImgSrc
                }
            }
        }

        Rectangle {
            id: infoViewLocalTimeRect
            anchors.right : mediaBackProgressBar.right
            anchors.rightMargin : -(marginBlock * 12)
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
                text: g_currentTimeStdFormat
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
                text: g_currentDateVodFormat
                color : "white"
                opacity: 0.5
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(32)
                }
            }
        }

        Rectangle {
            id: infoViewPlayPauseRect
            anchors.left :  mediaBackProgressBar.left
            anchors.top :  infoViewLocalTimeRect.top
            height: Utils.scaled(128)
            width: Utils.scaled(128)
            color: "transparent"
            opacity: 0.7
            Image {
                id: infoViewPlayPauseImg
                anchors.fill:parent
            }
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
            border.width: 2
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
