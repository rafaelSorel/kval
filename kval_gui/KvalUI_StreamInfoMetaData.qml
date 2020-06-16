import QtQuick 2.2
import QtGraphicalEffects 1.0
import "KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item
{
    id: channelInfoMetaDataView
    width: rootRectangle.width;
    height: rootRectangle.height
    x: rootRectangle.x + (rootRectangle.width * 0.05)
    y: rootRectangle.y - (rootRectangle.height * 0.05)
    enabled: false
    opacity: 0
    focus: false
    scale: 1.1

    signal focusRelinquished()
    signal messageDisplayPopUp(int severity, string msgData);
    signal messageHidePopUp();
    signal metaDataChangeNotify()

    property string currentStreamName : "";
    property string currentStreamRes : "";
    property int    currentStreamAspectRatio : 0;
    property int    currentStreamAudioId : 0;
    property int    currentStreamSubsId : 0;
    property string currentStreamBitRate : "";
    property string currentStreamRating : "";
    property string currentStreamStrength : "";
    property int    audioTrackNbr : 0;
    property bool   popUpActive : false;
    property int    currentStreamWidth : 0;
    property int    currentStreamHeight : 0;
    property int    currentStreamAspectId : 0;
    property int    g_subsNbr: 0
    property var    streamMetaData: null
    property var    g_audioTracks: []
    property var    g_subsTracks: []

    property var    g_objCall: null

    function initMetaDatas()
    {
        currentStreamBitRate = ""
        currentStreamRes = ""
        currentStreamAspectRatio = 0
        currentStreamAudioId = 0
        currentStreamSubsId = 0
        bluePanelAspectRatioCmp.clear()
        bluePanelAudiosCmp.clear()
        bluePanelSubsCmp.clear()
        audioTrackNbr = 0
        g_subsNbr = 0
        g_audioTracks = []
        g_subsTracks = []
    }

    function getAudioTrackNbr()
    {
        return audioTrackNbr;
    }

    function getSubsTrackNbr()
    {
        return g_subsNbr;
    }

    function getUpdatedMeta()
    {
        return streamMetaData
    }

    function extractStreamBaseMetaData()
    {
        streamMetaData = mediaPlayer.extractStreamInfos()
        extractStreamMetaData(false)
        metaDataChangeNotify();
    }

    function extractStreamMetaData(refetch)
    {
        if(refetch)
            streamMetaData = mediaPlayer.extractStreamInfos()
        logger.info("StreamMetaData received ...")
        currentStreamRes = streamMetaData["resolution"]
        currentStreamBitRate = streamMetaData["bitrate"]
        audioTrackNbr = streamMetaData["audioNbr"]
        g_subsNbr = streamMetaData["subsNbr"]
        g_audioTracks = streamMetaData["audioTracks"]
        g_subsTracks = streamMetaData["subsTracks"]
    }

    function switchVodAspectRatio()
    {
        currentStreamAspectId = 0
        currentStreamAspectRatio = 0
        //notify new metaData
        metaDataChangeNotify();
    }

    function switchLiveAspectRatio()
    {
        currentStreamAspectId = 0
        currentStreamAspectRatio = 0
        //notify new metaData
        metaDataChangeNotify();
    }

    function getAspectRatioValues()
    {
        var aspectRatioArray =
        [
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_NORMAL),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_FULL_STRETCH),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_4_3),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_16_9),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_NONLINEAR),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_NORMAL_NOSCALEUP),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_4_3_IGNORE),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_4_3_LETTER_BOX),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_4_3_PAN_SCAN),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_4_3_COMBINED),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_16_9_IGNORE),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_16_9_LETTER_BOX),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_16_9_PAN_SCAN),
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VIDEO_WIDEOPTION_16_9_COMBINED)
        ]
        return aspectRatioArray;
    }

    function showMetaData(obj)
    {
        g_objCall = obj
        if(activePlayingList === IPTV_Constants.ENUM_ACTIVE_PLAYING_DEFAULT)
        {
            currentStream = ""
            currentStreamBitRate = ""
            currentStreamRes = ""
            audioTrackNbr = 0
        }

        else if(activePlayingList === IPTV_Constants.ENUM_ACTIVE_PLAYING_VOD)
        {
            var streamName = mediaPlayerView.currentStreamInPlay
            currentStreamName = streamName.replace(/\<(.+?)\>/g, "");
        }
        else
        {
            currentStreamName = channelInfoView.currentStreamInPlay;
        }
        var bitRateStr = "- Kbps"
        var bitrate = mediaPlayer.videoBitRate()
        if (bitrate)
        {
            logger.info("Extracted bitrate: " + bitrate)
            bitrate = Math.floor(bitrate / 1024)
            bitRateStr = bitrate.toString() + " Kbps"
        }
        currentStreamBitRate =bitRateStr
        bitRateInfoTimer.start()
        bitRateInfoTimer.repeat = true;

        extractStreamMetaData(true)
        fillMainModel()

        //Change display properties
        enabled = true
        focus = true
        opacity = 1
    }

    function fillMainModel()
    {
        bluePanelMainModelCmp.clear()
        var i = 0;
        var catListContent = [
            {'name'      : kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_STREAM_BITRATE)},
            {'name'      : kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_STREAM_RESOLUTION)},
            {'name'      : kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_ASPECT_RATIO)},
            {'name'      : kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_AUDIO_TRACKS)},
            {'name'      : kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_SUBS)}
        ]
        for(i = 0; i < catListContent.length; i++ )
        {
            bluePanelMainModelCmp.insert(i, {'name'      : catListContent[i]["name"]});
        }

        bluePanelMainList.positionViewAtBeginning();
        bluePanelMainList.opacity = 1;
        bluePanelMainList.enabled = true;
        bluePanelMainList.focus = true;
        bluePanelMainList.currentIndex = 0;
        bluePanelMainList.highlight = bluePanelMainListHighlight

        //Fill Aspect Ration tab
        var aspectRatioArray = getAspectRatioValues()
        bluePanelAspectRatioCmp.clear()
        for(i = 0; i < aspectRatioArray.length; i++ )
        {
            bluePanelAspectRatioCmp.insert(i, {'name'      : aspectRatioArray[i]});
        }
        bluePanelAspectRatioList.positionViewAtIndex(currentStreamAspectId, ListView.Center);
        bluePanelAspectRatioList.opacity = 1;
        bluePanelAspectRatioList.enabled = false;
        bluePanelAspectRatioList.focus = false;
        bluePanelAspectRatioList.currentIndex = 0;
        bluePanelAspectRatioList.highlight = dummyHighlight

        //Fill Audios tab
        bluePanelAudiosCmp.clear()
        audioTrackNbr = getAudioTrackNbr()
        for(i = 0; i < audioTrackNbr; i++ )
        {
            bluePanelAudiosCmp.insert(i, {'index' : i,
                                          'name': g_audioTracks[i] } );
        }

        bluePanelAudiosList.positionViewAtIndex(currentStreamAudioId, ListView.Center);
        bluePanelAudiosList.opacity = 1;
        bluePanelAudiosList.enabled = false;
        bluePanelAudiosList.focus = false;
        bluePanelAudiosList.currentIndex = 0;
        bluePanelAudiosList.highlight = dummyHighlight

        //Fill Subs tab
        bluePanelSubsCmp.clear()
        for(i = 0; i < g_subsNbr; i++ )
        {
            bluePanelSubsCmp.insert(i, {'index' : i, 'name' : g_subsTracks[i]});
        }

        bluePanelSubsList.positionViewAtIndex(currentStreamSubsId, ListView.Center);
        bluePanelSubsList.opacity = 1;
        bluePanelSubsList.enabled = false;
        bluePanelSubsList.focus = false;
        bluePanelSubsList.currentIndex = 0;
        bluePanelSubsList.highlight = dummyHighlight
    }

    function bluePanelViewUpPressed()
    {
        if(bluePanelMainList.enabled)
        {
            bluePanelMainList.decrementCurrentIndex();
        }
    }

    function bluePanelDownPressed()
    {
        if(bluePanelMainList.enabled)
        {
            bluePanelMainList.incrementCurrentIndex()
        }
    }

    function bluePanelRightPressed()
    {
        if(bluePanelAspectRatioList.enabled)
        {
            bluePanelAspectRatioList.incrementCurrentIndex()
            currentStreamAspectId =bluePanelAspectRatioList.currentIndex
            mediaPlayer.setAspectRatio(bluePanelAspectRatioList.currentIndex);
            currentStreamAspectRatio = bluePanelAspectRatioList.currentIndex
            metaDataChangeNotify();
        }
        else if(bluePanelAudiosList.enabled)
        {
            if(currentStreamAudioId === (audioTrackNbr-1)) return
            bluePanelAudiosList.incrementCurrentIndex()
            mediaPlayer.switchAudioTrack(bluePanelAudiosList.currentIndex)
            currentStreamAudioId = bluePanelAudiosList.currentIndex
        }
        else if(bluePanelSubsList.enabled)
        {
            if(currentStreamSubsId === (g_subsNbr-1)) return
            bluePanelSubsList.incrementCurrentIndex()
            currentStreamSubsId = bluePanelSubsList.currentIndex
            if(currentStreamSubsId < (g_subsNbr-1))
            {
                mediaPlayer.switchSubsTrack(bluePanelSubsList.currentIndex)
                videoSubtitleSurface.switchOnSubs()
            }
            else
            {
                videoSubtitleSurface.switchOffSubs()
            }
        }
    }

    function bluePanelLeftPressed()
    {
        if(bluePanelAspectRatioList.enabled)
        {
            if(!currentStreamAspectId) return
            bluePanelAspectRatioList.decrementCurrentIndex()
            currentStreamAspectId =bluePanelAspectRatioList.currentIndex
            mediaPlayer.setAspectRatio(bluePanelAspectRatioList.currentIndex);
            currentStreamAspectRatio = bluePanelAspectRatioList.currentIndex
            metaDataChangeNotify();
        }
        else if(bluePanelAudiosList.enabled)
        {
            if(!currentStreamAudioId) return
            bluePanelAudiosList.decrementCurrentIndex()
            mediaPlayer.switchAudioTrack(bluePanelAudiosList.currentIndex)
            currentStreamAudioId = bluePanelAudiosList.currentIndex
        }
        else if(bluePanelSubsList.enabled)
        {
            if(!currentStreamSubsId) return
            bluePanelSubsList.decrementCurrentIndex()
            currentStreamSubsId = bluePanelSubsList.currentIndex
            if(currentStreamSubsId < (g_subsNbr-1))
            {
                mediaPlayer.switchSubsTrack(bluePanelSubsList.currentIndex)
                videoSubtitleSurface.switchOnSubs()
            }
            else
            {
                videoSubtitleSurface.switchOffSubs()
            }
        }
    }

    function bluePanelOkPressed()
    {
        //Aspect Ratio selection
        if(bluePanelMainList.enabled && bluePanelMainList.currentIndex === 2)
        {
            bluePanelMainList.positionViewAtBeginning();
            bluePanelMainList.opacity = 1;
            bluePanelMainList.enabled = false;
            bluePanelMainList.focus = false;
            bluePanelMainList.highlight = dummyHighlight

            bluePanelAspectRatioList.positionViewAtIndex(currentStreamAspectRatio, ListView.Center);
            bluePanelAspectRatioList.opacity = 1;
            bluePanelAspectRatioList.enabled = true;
            bluePanelAspectRatioList.focus = true;
            bluePanelAspectRatioList.currentIndex = currentStreamAspectRatio;
            bluePanelAspectRatioList.highlight = bluePanelAspectRatioListHighlight
        }
        else if(bluePanelMainList.enabled && bluePanelMainList.currentIndex === 3)
        {
            bluePanelMainList.positionViewAtBeginning();
            bluePanelMainList.opacity = 1;
            bluePanelMainList.enabled = false;
            bluePanelMainList.focus = false;
            bluePanelMainList.highlight = dummyHighlight

            bluePanelAudiosList.positionViewAtIndex(currentStreamAudioId, ListView.Center);
            bluePanelAudiosList.opacity = 1;
            bluePanelAudiosList.enabled = true;
            bluePanelAudiosList.focus = true;
            bluePanelAudiosList.currentIndex = currentStreamAudioId;
            bluePanelAudiosList.highlight = bluePanelAudiosListHighlight
        }
        else if(bluePanelMainList.enabled && bluePanelMainList.currentIndex === 4)
        {
            bluePanelMainList.positionViewAtBeginning();
            bluePanelMainList.opacity = 1;
            bluePanelMainList.enabled = false;
            bluePanelMainList.focus = false;
            bluePanelMainList.highlight = dummyHighlight

            bluePanelSubsList.positionViewAtIndex(currentStreamSubsId, ListView.Center);
            bluePanelSubsList.opacity = 1;
            bluePanelSubsList.enabled = true;
            bluePanelSubsList.focus = true;
            bluePanelSubsList.currentIndex = currentStreamSubsId;
            bluePanelSubsList.highlight = bluePanelSubsListHighlight
        }
    }

    function bluePanelBackPressed()
    {
        if(!bluePanelMainList.enabled)
        {

            bluePanelAspectRatioList.enabled = false;
            bluePanelAspectRatioList.focus = false;
            bluePanelAspectRatioList.highlight = dummyHighlight

            bluePanelAudiosList.enabled = false;
            bluePanelAudiosList.focus = false;
            bluePanelAudiosList.highlight = dummyHighlight

            bluePanelSubsList.enabled = false;
            bluePanelSubsList.focus = false;
            bluePanelSubsList.highlight = dummyHighlight

            bluePanelMainList.opacity = 1;
            bluePanelMainList.enabled = true;
            bluePanelMainList.focus = true;
            bluePanelMainList.highlight = bluePanelMainListHighlight
        }
    }

    Keys.onPressed:
    {
        if (event.key === Qt.Key_Up) //Up Button
        {
            bluePanelViewUpPressed()
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {
            bluePanelDownPressed()
        }
        else if (event.key === Qt.Key_Right) //Right Button
        {
            bluePanelRightPressed()
        }
        else if (event.key === Qt.Key_Left) //Left Button
        {
            bluePanelLeftPressed()
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {
            bluePanelOkPressed()
        }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {
            bluePanelBackPressed();
        }

        if (event.key === Qt.Key_Escape)
        {
            backButtonPressed();
        }
        else if (event.key === Qt.Key_Return)
        {
        }
        else
            return;
        event.accepted = true;
    }

    Timer {
        id: bitRateInfoTimer
        interval: 2000;
        running: false;
        repeat: false;
        onTriggered:
        {
            var bitRateStr = "- Kbps"
            var bitrate = mediaPlayer.videoBitRate()
            if (bitrate)
            {
                bitrate = Math.floor(bitrate / 1024)
                bitRateStr = bitrate.toString() + " Kbps"
            }
            currentStreamBitRate =bitRateStr
        }
    }


    function hideMetaData()
    {
        if(bitRateInfoTimer.running)
        {
            bitRateInfoTimer.stop();
            bitRateInfoTimer.repeat = false;
        }

        enabled = false
        opacity = 0
        focus = false
    }

    function backButtonPressed()
    {
        if(!popUpActive)
        {
            hideMetaData();
            g_objCall.focusRelinquish();
            return;
        }
    }

    Image{
        id: bluePanelBackImg
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_BACK)
    }

    Item {
        id: bluePanelStreamNameZone
        anchors.bottom: bluePanelMainZone.top
        anchors.bottomMargin: marginBlock*0.2
        anchors.left: bluePanelMainZone.left
        anchors.right: bluePanelBitRateZone.right
        height: bluePanelMainZone.height / 5
        Text{
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width * 0.9
            textFormat: Text.AutoText
            elide: Text.ElideRight
            color: "white"
            text:  currentStreamName
            font { family: localAdventFont.name; pixelSize: parent.height * 0.55 }
            opacity: 0.7
        }
    }

    Item {
        id: bluePanelMainZone
        anchors.bottom: bluePanelBackImg.bottom
        anchors.bottomMargin: marginBlock*6.5
        anchors.left: bluePanelBackImg.left
        anchors.leftMargin: marginBlock*6.5
        height: bluePanelBackImg.height * 0.25
        width: parent.width * 0.093
        Rectangle {
            anchors.fill: parent
            color: "yellow"
            opacity: 0
        }
    }
    Item {
        id: bluePanelBitRateZone
        anchors.top: bluePanelMainZone.top
        anchors.left: bluePanelMainZone.right
        anchors.leftMargin: marginBlock*0.2
        height: bluePanelMainZone.height/5
        width: bluePanelMainZone.width * 3.12
        Text{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            textFormat: Text.AutoText
            elide: Text.ElideRight
            color: "white"
            text:  currentStreamBitRate
            font { family: localAdventFont.name; pixelSize: parent.height * 0.5 }
        }
    }
    Item {
        id: bluePanelResolutionZone
        anchors.top: bluePanelBitRateZone.bottom
        anchors.left: bluePanelBitRateZone.left
        anchors.right: bluePanelBitRateZone.right
        anchors.topMargin: marginBlock*0.2
        height: (bluePanelMainZone.height/5) - (marginBlock*0.4)
        Text{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            textFormat: Text.AutoText
            elide: Text.ElideRight
            color: "white"
            text:  currentStreamRes
            font { family: localAdventFont.name; pixelSize: parent.height * 0.5 }
        }
    }
    Item {
        id: bluePanelAspectRatioZone
        anchors.top: bluePanelResolutionZone.bottom
        anchors.left: bluePanelResolutionZone.left
        anchors.right: bluePanelResolutionZone.right
        anchors.topMargin: marginBlock*0.4
        height: (bluePanelMainZone.height/5) - (marginBlock*0.4)
    }
    Item {
        id: bluePanelAudiosZone
        anchors.top: bluePanelAspectRatioZone.bottom
        anchors.left: bluePanelAspectRatioZone.left
        anchors.right: bluePanelAspectRatioZone.right
        anchors.topMargin: marginBlock*0.4
        height: (bluePanelMainZone.height/5) - (marginBlock*0.4)
    }
    Item {
        id: bluePanelSubsZone
        anchors.top: bluePanelAudiosZone.bottom
        anchors.left: bluePanelAudiosZone.left
        anchors.right: bluePanelAudiosZone.right
        anchors.topMargin: marginBlock*0.4
        height: (bluePanelMainZone.height/5) - (marginBlock*0.4)
    }



    Component {
        id: dummyHighlight
        Item {
            x:0
            y:0
            width: 1
            height: 1
        }
    }

    Component {
        id: bluePanelMainListHighlight

        Item {
            anchors.top: bluePanelMainList.currentItem.top
            anchors.topMargin: marginBlock*0.2
            anchors.bottom: bluePanelMainList.currentItem.bottom
            anchors.bottomMargin: marginBlock*0.2
            anchors.left: bluePanelMainList.currentItem.left
            anchors.right: bluePanelMainList.currentItem.right

            Image {
                anchors.fill: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_SELECTOR)
            }
        }
    }
    Component {
        id : bluePanelMainListComponent
        Row {
            id: wrapperRow
            Item {
                id: wrapper
                width: bluePanelMainZone.width
                height: bluePanelMainZone.height / 5
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: marginBlock * 2
                    text: name
                    color: "white"
                    font {
                        family: localAdventFont.name;
                        pixelSize: parent.height * 0.5;
                    }
                }
            }
        }
    }
    ListView {
        id: bluePanelMainList
        anchors.left: bluePanelMainZone.left
        anchors.right: bluePanelMainZone.right
        anchors.top: bluePanelMainZone.top
        anchors.bottom: bluePanelMainZone.bottom
        enabled: false
        opacity: 0
        focus: false

        model: bluePanelMainModelCmp

        highlightFollowsCurrentItem: false
        clip: true
        interactive: false
        orientation: ListView.Vertical
        delegate: bluePanelMainListComponent
    }
    ListModel {
         id: bluePanelMainModelCmp
         //Template to Use
         ListElement {
             name: ""
         }
    }


    Component {
        id: bluePanelAspectRatioListHighlight

        Item {
            anchors.top: bluePanelAspectRatioList.currentItem.top
            anchors.bottom: bluePanelAspectRatioList.currentItem.bottom
            anchors.left: bluePanelAspectRatioList.currentItem.left
            anchors.right: bluePanelAspectRatioList.currentItem.right

            Image {
                anchors.fill: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_SELECTOR)
            }
        }
    }
    Component {
        id : bluePanelAspectRatioListComponent
        Row {
            id: wrapperRow
            Item {
                id: wrapper
                width: aspectName.contentWidth * 1.5
                height: bluePanelAspectRatioZone.height
                Text {
                    id: aspectName
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    text: name
                    color: "white"
                    font {
                        family: localAdventFont.name;
                        pixelSize: parent.height * 0.5;
                    }
                    opacity: (index === currentStreamAspectId) ? 1 : 0.6
                }
                Glow {
                    anchors.fill: aspectName
                    radius: 6
                    opacity: (index === currentStreamAspectId) ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: aspectName
                }

                Image {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_SEPARATOR)
                }
            }
        }
    }
    ListView {
        id: bluePanelAspectRatioList
        anchors.left: bluePanelAspectRatioZone.left
        anchors.right: bluePanelAspectRatioZone.right
        anchors.top: bluePanelAspectRatioZone.top
        anchors.bottom: bluePanelAspectRatioZone.bottom
        enabled: false
        opacity: 0
        focus: false

        model: bluePanelAspectRatioCmp

        highlightFollowsCurrentItem: false
        clip: true
        interactive: false
        orientation: ListView.Horizontal
        delegate: bluePanelAspectRatioListComponent
    }
    ListModel {
         id: bluePanelAspectRatioCmp
         //Template to Use
         ListElement {
             name: ""
         }
    }

    Component {
        id: bluePanelAudiosListHighlight

        Item {
            anchors.top: bluePanelAudiosList.currentItem.top
            anchors.bottom: bluePanelAudiosList.currentItem.bottom
            anchors.left: bluePanelAudiosList.currentItem.left
            anchors.right: bluePanelAudiosList.currentItem.right

            Image {
                anchors.fill: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_SELECTOR)
            }
        }
    }
    Component {
        id : bluePanelAudiosListComponent
        Row {
            id: wrapperRow
            Item {
                id: wrapper
                width:  (audioTrackNbr > 4) ?
                        (bluePanelAudiosZone.width / 4) :
                        (bluePanelAudiosZone.width / ((!audioTrackNbr) ? 1 : audioTrackNbr))
                height: bluePanelAudiosZone.height
                Text {
                    id: audioName

                    width: parent.width - (marginBlock*2)
                    elide: Text.ElideRight
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.Center
                    text: name
                    color: "white"
                    opacity: (index === currentStreamAudioId) ? 1 : 0.6
                    font {
                        family: localAdventFont.name;
                        pixelSize: parent.height * 0.5;
                    }
                }
                Glow {
                    anchors.fill: audioName
                    radius: 6
                    opacity: (index === currentStreamAudioId) ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: audioName
                }
                Image {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_SEPARATOR)
                    visible: (index === (audioTrackNbr-1) ) ? false : true
                }
            }
        }
    }
    ListView {
        id: bluePanelAudiosList
        anchors.left: bluePanelAudiosZone.left
        anchors.right: bluePanelAudiosZone.right
        anchors.top: bluePanelAudiosZone.top
        anchors.bottom: bluePanelAudiosZone.bottom
        enabled: false
        opacity: 0
        focus: false

        model: bluePanelAudiosCmp

        highlightFollowsCurrentItem: false
        clip: true
        interactive: false
        orientation: ListView.Horizontal
        delegate: bluePanelAudiosListComponent
    }
    ListModel {
         id: bluePanelAudiosCmp
         //Template to Use
         ListElement {
             index: 0
             name: ""
         }
    }

    Component {
        id: bluePanelSubsListHighlight

        Item {
            anchors.top: bluePanelSubsList.currentItem.top
            anchors.bottom: bluePanelSubsList.currentItem.bottom
            anchors.left: bluePanelSubsList.currentItem.left
            anchors.right: bluePanelSubsList.currentItem.right

            Image {
                anchors.fill: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_SELECTOR)
            }
        }
    }
    Component {
        id : bluePanelSubsListComponent
        Row {
            id: wrapperRow
            Item {
                id: wrapper
                width:  (g_subsNbr > 4) ?
                        (bluePanelSubsZone.width / 4) :
                        (bluePanelSubsZone.width / ((!g_subsNbr) ? 1 : g_subsNbr))
                height: bluePanelSubsZone.height

                Text {
                    id: subsName

                    width: parent.width - (marginBlock*2)
                    elide: Text.ElideRight
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.Center
                    text: name
                    color: "white"
                    opacity: (index === currentStreamSubsId) ? 1 : 0.6
                    font {
                        family: localAdventFont.name;
                        pixelSize: parent.height * 0.5;
                    }
                }
                Glow {
                    anchors.fill: subsName
                    radius: 6
                    opacity: (index === currentStreamSubsId) ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: subsName
                }
                Image {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLUE_PANEL_SEPARATOR)
                    visible: (index === (g_subsNbr-1) ) ? false : true
                }
            }
        }
    }
    ListView {
        id: bluePanelSubsList
        anchors.left: bluePanelSubsZone.left
        anchors.right: bluePanelSubsZone.right
        anchors.top: bluePanelSubsZone.top
        anchors.bottom: bluePanelSubsZone.bottom
        enabled: false
        opacity: 0
        focus: false

        model: bluePanelSubsCmp

        highlightFollowsCurrentItem: false
        clip: true
        interactive: false
        orientation: ListView.Horizontal
        delegate: bluePanelSubsListComponent
    }
    ListModel {
         id: bluePanelSubsCmp
         //Template to Use
         ListElement {
             index: 0
             name: ""
         }
    }

}
