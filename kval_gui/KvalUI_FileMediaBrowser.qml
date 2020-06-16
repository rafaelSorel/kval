import QtQuick 2.3
import "KvalUI_constant.js" as IPTV_Constants
import QtGraphicalEffects 1.0
import kval.gui.qml 1.0

Item
{
    id: mediaBrowserView

    enabled: false
    opacity: 0
    focus: false
    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

    signal fileSelectedNotify(string fileAbsPath)
    signal mediaBrowserReadyNotify()
    signal activateMainScreenView()
    signal focusRelinquished();

    property string     g_currentFolder : ""
    property string     g_currentFolderPath : ""
    property string     g_parentFolderPath : ""
    property int        g_count : 0
    property int        g_componentLoadInc : 0

    function showMediaBrowserView()
    {
        setNavLevel( IPTV_Constants.ENUM_CATEGORIES_MEDIA_BROWSER_LEVEL);
        registerActiveView(mediaBrowserMainView)
        activeFocusViewManager(mediaBrowserMainView)
        opacity = 1

    }
    
    function playBackCompleted()
    {
        logger.info("mediaBrowserView playBackCompleted()");
        mediaPlayerView.hideInfoBar();
        showMediaBrowserView()
    }

    function hideMediaBrowserView()
    {
        enabled = false
        opacity = 0
        focus = false
    }

    function popUpTryGetFocus()
    {
        if(!enabled)
        {
            return false
        }
        opacity = 0.5
        enabled = false
        focus = false
        return true
    }
    function popUpFocusRelinquished()
    {
        activeFocusViewManager(mediaBrowserMainView)
        opacity = 1
    }

    function mediaBrowserBackToHomeView()
    {
        mediaBrowserBack.source = ""
        mediaBrowserBackOverlay.source = ""
        mediaBrowserBackOverlay2.source = ""
        infoZoneImageThumb.source = ""
        contentBack.source = ""
        activateMainScreenView()
        hideMediaBrowserView()
        pyNotificationView.clearview();
    }

    function mediaBrowserPlayBackStarted()
    {
        logger.info("mediaBrowserPlayBackStarted")
        mediaPlayerView.setDownloadValue(100);
        hideMediaBrowserView()
        mediaPlayerView.invokerInfoBar()
    }

    function mediaBrowserShowLocalMedia(mediaPath) 
    {
        mediaPlayerView.setCurrentMediaInfo(
        mediaBrowserFolderModel.get(mediaBrowserListView.currentIndex).fileName, 
                                    "")
        var mediaUri = "file://" + mediaPath;
        mediaBrowserShowUrlMedia(mediaUri);
    }
    function showImage(imageUri) 
    {
        stopCurrentMedia()
        logger.info("showImage: "+ imageUri)
//        imageOutput.showImage(imageUri);
    }

    function mediaBrowserShowUrlMedia(mediaUri)
    {
        if (mediaBrowserElement.isSupportedAudio(mediaUri))
        {showVideo(mediaUri);
        }
        else if (mediaBrowserElement.isSupportedImage(mediaUri))
        {showImage(mediaUri);
        }
        else if (mediaBrowserElement.isSupportedVideo(mediaUri))
        {showVideo(mediaUri);
        }
        else
        {
            logger.info("Can't handle this media, sorry.");
        }
    }
    
    function extractAdeIcon(fileNamee)
    {
        if (mediaBrowserElement.isSupportedAudio(fileNamee))
            return kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_ICON_AUDIO);
        else if (mediaBrowserElement.isSupportedVideo(fileNamee))
            return kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_ICON_VIDEO);
        else if (mediaBrowserElement.isSupportedImage(fileNamee))
            return kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_ICON_IMAGE);
        else
            return kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_ICON_UNKNOWN);
    }

    function fillListView(currentInFolderList)
    {
        mediaBrowserFolderModel.clear()
        for(var i = 0; i < currentInFolderList.length ; i++)
        {
            var currentFileModified = mediaBrowserElement.getFileModified(i)
            var currentFileSize = formatSizeUnits(mediaBrowserElement.getFileSize(i))
            var currentfileInfo = currentFileModified
            var currentFileIcon = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_ICON_DIR)
            if(!mediaBrowserElement.isDirectory(i))
            {
                currentfileInfo = currentFileModified+ ' • ' + currentFileSize
                currentFileIcon = extractAdeIcon(currentInFolderList[i])
            }


            mediaBrowserFolderModel.insert(i, {
                                    'fileName': currentInFolderList[i],
                                    'fileModified' : currentFileModified,
                                    'fileSize': currentFileSize,
                                    'fileInfo' : currentfileInfo,
                                    'mediaColor': i ? "white" : "black",
                                    'mediaColor2': i ? "#6aa6cb" : "black",
                                    'itemIcon': currentFileIcon});
        }
        delegateReady()
    }

    function activateMediaBrowserView()
    {
        var currentList = mediaBrowserElement.getListByPath(
                    mediaBrowserElement.getUsbMountDir());
        fillListView(currentList)
        mediaBrowserBack.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_BACKGROUND)
        mediaBrowserBackOverlay.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND)
        mediaBrowserBackOverlay2.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND_BLACK_2)
    }

    function mediaBrowserSetCurrentItemInfos()
    {
        g_currentFolder =
        mediaBrowserElement.getCurrentDir(mediaBrowserListView.currentIndex)
        mediaBrowserCurrentItemTitleTxt.text = 
        mediaBrowserFolderModel.get(mediaBrowserListView.currentIndex).fileName
        mediaBrowserCurrentItemDateTxt.text = 
        mediaBrowserFolderModel.get(mediaBrowserListView.currentIndex).fileModified
        if (mediaBrowserElement.isDirectory(mediaBrowserListView.currentIndex))
        {
            mediaBrowserCurrentItemSizeTxt.text = ""
            infoZoneImageThumb.source = ''
        }
        else
        {
            mediaBrowserCurrentItemSizeTxt.text = 
            mediaBrowserFolderModel.get(mediaBrowserListView.currentIndex).fileSize

            var currentfilePath = 
            mediaBrowserElement.getSelectedPath(mediaBrowserListView.currentIndex)

            if(mediaBrowserElement.isSupportedImage(currentfilePath))
            {
                infoZoneVideoReady.visible = false
                infoZoneImageThumb.source = 'file://' + currentfilePath
            }
            else if(mediaBrowserElement.isSupportedVideo(currentfilePath))
            {
                contentText.text = 'pour lancer le contenue Video'
                contentBack.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_VIDEO_BACK)
                infoZoneImageThumb.source = ''
                infoZoneVideoReady.visible = true
            }
            else if(mediaBrowserElement.isSupportedAudio(currentfilePath))
            {
                contentText.text = 'pour lancer le contenue Audio'
                contentBack.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_MUSIC_BACK)
                infoZoneImageThumb.source = ''
                infoZoneVideoReady.visible = true
            }
            else
            {
                infoZoneVideoReady.visible = false
                infoZoneImageThumb.source = ''
            }
        }
    }

    function formatSizeUnits(fileSize)
    {
          var bytes = Number(fileSize)
          if      (bytes>=1073741824) {bytes=(bytes/1073741824).toFixed(2)+' GB';}
          else if (bytes>=1048576)    {bytes=(bytes/1048576).toFixed(2)+' MB';}
          else if (bytes>=1024)       {bytes=(bytes/1024).toFixed(2)+' KB';}
          else if (bytes>1)           {bytes=bytes+' bytes';}
          else if (bytes===1)          {bytes=bytes+' byte';}
          else                        {bytes='0 byte';}
          return bytes;
    }
    function delegateReady()
    {
        mediaBrowserListView.currentIndex = 0
        mediaBrowserListView.positionViewAtIndex(0, ListView.Beginning)
        mediaBrowserSetCurrentItemInfos()
    }

    function mediaBrowserOkKeyPressed()
    {
        setActivePlayingList( IPTV_Constants.ENUM_ACTIVE_PLAYING_MEDIA_BROWSER);
        var absPath =
        mediaBrowserElement.getAbsolutePath(mediaBrowserListView.currentIndex)
        if (mediaBrowserElement.isDirectory(mediaBrowserListView.currentIndex))
        {
            var currentList = mediaBrowserElement.getListByPath(
                        mediaBrowserElement.getSelectedPath(mediaBrowserListView.currentIndex));
            fillListView(currentList)
        }
        else
        {mediaBrowserShowLocalMedia(mediaBrowserElement.getSelectedPath(mediaBrowserListView.currentIndex))
        }
    }

    function mediaBrowserDownKeyPressed()
    {
        if(mediaBrowserListView.count === 1) return
        mediaBrowserListView.incrementCurrentIndex()
        mediaBrowserSetCurrentItemInfos()
        mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex,
                                "mediaColor",
                                "black");
        mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex,
                                "mediaColor2",
                                "black");

        if(mediaBrowserListView.currentIndex-1 < 0)
        {
            mediaBrowserFolderModel.setProperty(mediaBrowserListView.count - 1,
                                            "mediaColor", "white");
            mediaBrowserFolderModel.setProperty(mediaBrowserListView.count - 1,
                                            "mediaColor2", "#6aa6cb");
        }
        else
        {
            mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex-1,
                                    "mediaColor",
                                    "white");
            mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex-1,
                                    "mediaColor2",
                                    "#6aa6cb");
        }
    }
    function mediaBrowserUpKeyPressed()
    {
        if(mediaBrowserListView.count === 1) return
        mediaBrowserListView.decrementCurrentIndex()
        mediaBrowserSetCurrentItemInfos()
        mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex,
                                "mediaColor",
                                "black");
        mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex,
                                "mediaColor2",
                                "black");
        if(mediaBrowserListView.currentIndex+1 === mediaBrowserListView.count)
        {
            mediaBrowserFolderModel.setProperty(0, "mediaColor", "white");
            mediaBrowserFolderModel.setProperty(0, "mediaColor2", "#6aa6cb");
        }
        else
        {
            mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex+1,
                                    "mediaColor",
                                    "white");
            mediaBrowserFolderModel.setProperty(mediaBrowserListView.currentIndex+1,
                                    "mediaColor2",
                                    "#6aa6cb");
        }
    }
    function mediaBrowserBackKeyPressed()
    {
        var currentDir = 
        mediaBrowserElement.getCurrentDir(mediaBrowserListView.currentIndex)
        if( currentDir.length <= 11)
        {
            logger.info("We reach root ...")
            return
        }
        var currentList = mediaBrowserElement.getListByPath("..");
        fillListView(currentList)
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Up) //Up Button
        {mediaBrowserUpKeyPressed()
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {mediaBrowserDownKeyPressed()
        }
        else if (event.key === Qt.Key_H) //Down Button
        {mediaBrowserBackToHomeView()
        }
        else if (event.key === Qt.Key_Backspace) //Down Button
        {mediaBrowserBackKeyPressed()
        }
        else if (event.key === Qt.Key_R) //Down Button
        {
        }
        else if (event.key === Qt.Key_Y) //Down Button
        {
        }
        else if (event.key === Qt.Key_O)
        {mediaBrowserOkKeyPressed()
        }

        else {}
        event.accepted = true;
    }
    Rectangle {
        id: mediaBrowserListViewRef

        x: 0
        y: mediaBrowserView.height * 0.12
        width: mediaBrowserView.width * 0.5
        height: mediaBrowserView.height * 0.8
        color: "transparent"
    }

    Item {
        id: mediaBrowserBackGound
        anchors.fill: parent
        Image {
            id: mediaBrowserBack
            anchors.fill: parent
        }
        Image {
            id: mediaBrowserBackOverlay
            anchors.fill: parent
            opacity: 0.7
        }
        Image {
            id: mediaBrowserBackOverlay2
            anchors.fill: parent
            opacity: 0.6
            onStatusChanged: {
                if (status === Image.Ready) mediaBrowserReadyNotify()
            }
        }
    }

    Item {
        id: mediaBrowserHeaderZone
        anchors.top: parent.top
        anchors.topMargin: marginBlock
        anchors.left: parent.left
        anchors.leftMargin: marginBlock*2
        anchors.right: parent.right
        anchors.rightMargin: marginBlock*2
        height: parent.height * 0.1
        Text {
            id: mediaBrowserTimeHeaderTxt
            anchors.right: parent.right
            anchors.top: parent.top
            text: g_currentTimeStdFormat
            color: "white"
            font {
                family: localFontCaps.name;
                pixelSize: parent.width * 0.015
            }
        }
        Text {
            anchors.right: mediaBrowserTimeHeaderTxt.right
            anchors.top: mediaBrowserTimeHeaderTxt.bottom
            anchors.topMargin: -marginBlock
            text: g_currentDateStdFormat
            color: "#6aa6cb"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.015
            }
        }
        Image{
            id: mediaBrowserIco

            anchors.left: parent.left
            anchors.leftMargin: -marginBlock*2
            anchors.top: parent.top
            anchors.topMargin: marginBlock
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_ICON_HDD)
        }
        Text {
            id: mediaBrowserTitle

            anchors.left: mediaBrowserIco.right
            anchors.top: parent.top
            text: kvalUiConfigManager.retranslate + qsTr("Multimedia contents")
            color: "white"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.025
            }
        }
        Text {
            id: mediaBrowserTotalItemsTxt

            anchors.left: mediaBrowserTitle.left
            anchors.top: mediaBrowserTitle.bottom
            anchors.topMargin: -marginBlock
            color: "white"
            text:   '<b>'+mediaBrowserFolderModel.count+'</b>'+
                    kvalUiConfigManager.retranslate + qsTr(' éléments') + ' • '
            font {
                family: localFont.name
                pixelSize: parent.width * 0.015
            }
        }
        Text {
            anchors.left: mediaBrowserTotalItemsTxt.right
            anchors.top: mediaBrowserTitle.bottom
            anchors.topMargin: -marginBlock
            width: mediaBrowserListViewRef.width * 0.7
            elide: Text.ElideMiddle
            color: "#6aa6cb"
            text:  g_currentFolder
            font {
                family: localFont.name
                pixelSize: parent.width * 0.015
            }
        }
    }

    Item {
        id: mediaBrowserBottomZone

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: parent.width
        height: parent.height * 0.1
        visible: false

        Image {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * 0.5

            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_RSS_INFO_BAR)
            opacity: 0.5
        }
        Image{
            id: mediaBrowserRedKeyIco

            anchors.bottom: parent.bottom
            anchors.bottomMargin: -marginBlock
            anchors.right: parent.right
            anchors.rightMargin: parent.width * 0.4
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_RED_BUTTON)

            Text{
                id: mediaBrowserDeleteCurrentTxt

                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock
                anchors.left: parent.right
                anchors.leftMargin: marginBlock/2
                text: kvalUiConfigManager.retranslate + qsTr("Label 1")
                color: "white"
                font {
                    family: localAdventFont.name;
                    pixelSize: mediaBrowserBottomZone.height * 0.25
                }
            }
        }
        Image{
            id: mediaBrowserYellowKeyIco

            anchors.bottom: parent.bottom
            anchors.bottomMargin: -marginBlock
            anchors.left: mediaBrowserRedKeyIco.right
            anchors.leftMargin: marginBlock * 3 +
                                mediaBrowserDeleteCurrentTxt.implicitWidth
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_YELLOW_BUTTON)

            Text{
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock
                anchors.left: parent.right
                anchors.leftMargin: marginBlock/2
                text: kvalUiConfigManager.retranslate + qsTr("Label 2")
                color: "white"
                font {
                    family: localAdventFont.name;
                    pixelSize: mediaBrowserBottomZone.height * 0.25
                }
            }
        }
    }

    Image {
        id: separatorLine
        anchors.top: mediaBrowserHeaderZone.bottom
        anchors.bottom: mediaBrowserView.bottom
        anchors.left: mediaBrowserListViewRef.right
        anchors.leftMargin: -marginBlock * 4
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SEPERATOR_MAIN_VIEW_SCREEN)
    }
    Image {
        anchors.left: separatorLine.left
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -marginBlock
        scale: 0.7
    }

    Item {
        id: mediaBrowserInformationZone

        anchors.top: mediaBrowserListViewRef.top
        anchors.left: mediaBrowserListViewRef.right
        height: mediaBrowserListViewRef.height * 0.8
        width: mediaBrowserListViewRef.width

        Text {
            id: mediaBrowserCurrentItemTitleTxt

            anchors.top: parent.top
            anchors.topMargin: marginBlock
            anchors.left: parent.left
            anchors.leftMargin: marginBlock * 4
            color: "#6aa6cb"
            width: parent.width - marginBlock * 4
            elide: Text.ElideRight
            font {
              family: localFont.name
              pixelSize: (1+(parent.width * 0.03)) +
                         ((1 + (parent.width * 0.03)) * 0.333)
            }
        }
        Text {
            id: mediaBrowserCurrentItemDateTxt

            anchors.top: mediaBrowserCurrentItemTitleTxt.bottom
            anchors.topMargin: marginBlock
            anchors.left: mediaBrowserCurrentItemTitleTxt.left
            color: "white"
            font {
              family: localFont.name;
              pixelSize: (1+(parent.width * 0.025)) +
                         ((1 + (parent.width * 0.025)) * 0.333)
            }
        }
        Text {
            id: mediaBrowserCurrentItemSizeTxt

            anchors.top: mediaBrowserCurrentItemDateTxt.top
            anchors.right: parent.right
            anchors.rightMargin: marginBlock * 6
            color: "white"
            font {
              family: localBoldFont.name;
              pixelSize: (1+(parent.width * 0.025)) +
                         ((1 + (parent.width * 0.025)) * 0.333)
            }
        }

        Item {
            id: mediaThumbs

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: mediaBrowserInformationZone.top
            anchors.topMargin: marginBlock * 15
            width: parent.width *0.8
            height: parent.height *0.65
            Image {
                id: infoZoneImageThumb

                asynchronous: true
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                fillMode: Image.PreserveAspectFit
            }
            Item{
                id: infoZoneVideoReady
                anchors.fill: parent
                visible: false
                Image {
                    id: contentBack

                    anchors.fill: parent
                    asynchronous: true
                    opacity: 0.6
                }
                Rectangle {
                    anchors.fill: parent
                    color: 'black'
                    opacity: 0.7
                    radius: 5
                }
                Image {
                    id: playIco
                    anchors.centerIn: parent
                    fillMode: Image.Pad
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PLAY)
                    Text{
                        anchors.top: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: 'white'
                        text: kvalUiConfigManager.retranslate + qsTr('Appuyer sur OK')
                        font {
                          family: localFont.name;
                          pixelSize: (1+(playIco.width * 0.18)) +
                                     ((1 + (playIco.width * 0.18)) * 0.333)
                        }
                        Text{
                            id: contentText

                            anchors.top: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: 'white'
                            font {
                              family: localFont.name;
                              pixelSize: (1+(playIco.width * 0.18)) +
                                         ((1 + (playIco.width * 0.18)) * 0.333)
                            }
                        }
                    }
                }
                Image {
                    width: parent.width *1.13
                    height: parent.height *1.13
                    anchors.centerIn: parent
                    fillMode: Image.Stretch
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
                }
            }
        }
    }

    Component {
            id : mediaBrowserComponentDelegate

            Row {
                Column {
                    Rectangle {
                        width: mediaBrowserListViewRef.width
                        height: mediaBrowserListViewRef.height/9
                        color: "transparent"
                        Image {
                            id: mainIconPrefix

                            x: marginBlock * 2
                            y: parent.y - marginBlock * 2.5
                            scale: 0.46
                            opacity: 0.7
                            Component.onCompleted: {
                                if (itemIcon === kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MEDIA_BROWSER_ICON_DIR))
                                {
                                    x = marginBlock
                                    y = parent.y - marginBlock
                                }
                                else
                                {
                                    x = marginBlock * 2
                                    y = parent.y - marginBlock*2.5
                                }
                            }
                            source: itemIcon
                        }
                        Text {
                            id: mediaTitle

                            anchors.left: parent.left
                            anchors.leftMargin: marginBlock * 14
                            y: parent.y + marginBlock*0.6
                            width: parent.width * 0.8
                            height: parent.height

                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: mediaColor
                            text: fileName
                            font {
                              family: localFont.name;
                              pixelSize: (1+(mediaBrowserListViewRef.width * 0.027)) +
                                         ((1 + (mediaBrowserListViewRef.width * 0.027)) * 0.333)
                            }
                        }
                        Text {
                            anchors.left: mediaTitle.left
                            anchors.top: mediaTitle.bottom
                            anchors.topMargin: -marginBlock * 5
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: mediaColor2
                            text: fileInfo
                            font {
                              family: localFont.name;
                              pixelSize: (1+(mediaBrowserListViewRef.width * 0.023)) +
                                         ((1 + (mediaBrowserListViewRef.width * 0.023)) * 0.333)

                            }
                        }
                    }
                }
            }
        }

    ListModel {
         id: mediaBrowserFolderModel
         //Template to Use
         ListElement {
             fileName: ""
             fileModified: ""
             fileInfo: ""
             fileSize: ""
             mediaColor: ""
             mediaColor2: ""
             itemIcon: ""
         }
     }

    ListView {
        id: mediaBrowserListView
        anchors.fill: mediaBrowserListViewRef
        enabled: true
        opacity: 1
        focus: true

        model: mediaBrowserFolderModel
        highlight: highlightmediaBrowserComponent
        highlightFollowsCurrentItem: false
        clip: true
        keyNavigationWraps: true
        interactive: false
        delegate: mediaBrowserComponentDelegate
    }

    Component {
        id: highlightmediaBrowserComponent

        Rectangle {
            id: highlightFavRect

            x: mediaBrowserListView.currentItem.x
            y: mediaBrowserListView.currentItem.y
            width: mediaBrowserListViewRef.width
            height: mediaBrowserListViewRef.height/9 + marginBlock
            color: "transparent"
            Image {
                x: parent.x
                y: parent.y
                width: mediaBrowserListViewRef.width - (marginBlock*2)
                anchors.top: parent.top
                anchors.topMargin: -marginBlock - marginBlock/3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -marginBlock - marginBlock/3
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_LIST_FOCUS)
                opacity: 0.8
            }
        }
    }
}
