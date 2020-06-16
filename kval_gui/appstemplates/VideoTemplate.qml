import QtQuick 2.3
import "../KvalUI_constant.js" as IPTV_Constants
import "navigation_history.js" as IPTV_navhistory
import QtGraphicalEffects 1.0
import kval.gui.qml 1.0

Item
{
    id: videoTemplateView

    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

//------------------------------------------------------------------------------
// Global Vars
//------------------------------------------------------------------------------
    signal activateMainScreenView()
    signal appsScreenReadyNotify()
    signal videoTemplateLaunchApp(string appid, string args)
    signal notifyBuiltinAction(string action)
    signal activeFocus()
    signal activateAppView()
    signal clearTemplate()

    property bool g_refreshRequested: false
    property int  g_mode: 0
    property bool g_userHasChangedmode: false
    property int  g_submode: 0
    property var  g_modeView: null
    property var  g_requestPlayItemInfo: null
    property bool g_urlResProgress: false

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
    function activateTemplateView()
    {
        logger.info("activateVideoTemplateView....")
        registerActiveView(videoTemplate.item)
        if(!g_modeView) g_modeView = gridModeView
        IPTV_navhistory.initHistoryTab(appsView.getActiveAppName())
        g_refreshRequested = false
        loadingSpinnerAnim.stop()

        bottomZoneAppIcon.source = appsView.getActiveAppIcon()
        bottomZoneAppName.text = appsView.getActiveAppName()
        appsBackImg.source = appsView.getActiveAppFanart()
        appsBackOverlay.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND_BLACK_2)

        g_modeView.applyMode()
        itemsReady(null)
    }

    function showView()
    {
        activeFocus()
        activeFocusViewManager(videoTemplate.item)
        opacity = 1
    }

    function hideView()
    {
        enabled = false
        opacity = 0
        focus = false
        loadingSpinnerAnim.stop()
    }

    function clearAllViews()
    {
        gridModeView.clear()
        gridModeView.clearDisplay()
        listModeView.clear()
        listModeView.clearDisplay()
    }

    function refresh()
    {
        g_refreshRequested = true;
        videoTemplateLaunchApp(appsView.getActiveAppId(),
                               IPTV_navhistory.getCurrentNavElement())
    }

    function update(path, replace)
    {
        if(replace)
        {
            IPTV_navhistory.initHistoryTab(appsView.getActiveAppName())
        }
        videoTemplateLaunchApp(appsView.getActiveAppId(), path)
    }
    function popUpTryGetFocus()
    {
        if(!enabled)
        {
            return false
        }
        g_modeView.setOpacity(0.3)
        enabled = false
        focus = false
        return true
    }
    function popUpFocusRelinquished()
    {
        activeFocusViewManager(videoTemplate.item)
        activeFocus()
        g_modeView.setOpacity(1)
    }

    function diagCtxActionCallback(action)
    {
        activeFocusViewManager(videoTemplate.item)
        activeFocus()
        g_modeView.setOpacity(1)
        if(action) notifyBuiltinAction(action)
    }

    function getCurrentLabel()
    {
        return IPTV_navhistory.formatstring(g_modeView.getCurrentLabel())
    }

    function getCurrentPlot()
    {
        return IPTV_navhistory.formatstring(g_modeView.getCurrentPlot())
    }

    function setViewMode(mode, submode)
    {
        if(!g_userHasChangedmode)
        {
            logger.info("User did not change the display mode yet !")
            g_modeView= (mode === IPTV_Constants.ENUM_APPS_GRID_MODE) ?
                        gridModeView :
                        listModeView;
            g_mode = mode
        }
        g_submode = submode
        g_modeView.setViewMode(submode)
    }

    function itemsReady(hisItems)
    {
        if(!hisItems && !g_refreshRequested)
            IPTV_navhistory.storeDisplayModeHistory(g_mode, g_submode)

        //Clear Models
        categoryModelComponent.clear()
        clearAllViews()
        appsHistoryHeader.text = IPTV_navhistory.getLabelHistory()

        //Load new items
        var count = (!hisItems) ? appsEngine.getItemsCount() : hisItems.length
        for(var i = 0; i < count; i++)
        {
            var app = (!hisItems) ? appsEngine.getItemsMap(i) : hisItems[i]
            if(!hisItems) IPTV_navhistory.store_iteminfos(app)

            var label = IPTV_navhistory.formatstring(app["label"])
            var plot = IPTV_navhistory.formatstring(app["plot"])
            var itemDuration = ""
            if("duration" in app)
                itemDuration = IPTV_navhistory.formatduration(app["duration"])

            var itemExtra = []
            if("studio" in app) itemExtra.push(app['studio'])
            if("date" in app) itemExtra.push(app['date'])

            g_modeView.insertElement(i,

                              { 'itemBack'     : app["thumb"],
                                'itemDefBack'  : app["defthumb"],
                                'itemFanart'   : app["fanart"],
                                'itemIcon'     : app["icon"],
                                'itemLabel'    : label,
                                'itemExtra'    : itemExtra.join(" • "),
                                'itemPlot'     : plot,
                                'itemDuration' : itemDuration,
                                'itemHdl'      : app["handle"],
                                'itemUrl'      : app["url"],
                                'itemIsFolder' : (app["isFolder"] === 'true') ?
                                                     true : false})
        }

        if(!hisItems && !g_refreshRequested) IPTV_navhistory.save_items()
        if(g_refreshRequested) IPTV_navhistory.updateCurrentItem()

        var categoryListContent = appsView.getCatListContent()

        for(i = 0; i < categoryListContent.length; i++ )
        {
            categoryModelComponent.insert(i,
            {'categoryId'      : categoryListContent[i]["id"],
             'categoryIcon'    : categoryListContent[i]["icon"],
             'categoryTxt'     : categoryListContent[i]["text"],
             'categoryTxtNbr'  : categoryListContent[i]["number"]});
        }
        categoryItemList.positionViewAtIndex(appsView.getActiveAppCategory(),
                                             ListView.Center)
        categoryItemList.currentIndex = appsView.getActiveAppCategory()
        categoryItemList.opacity = 1;
        g_modeView.activeList()
        g_refreshRequested = false
        fanArtImg.source = g_modeView.getCurrentFanart()
    }

    function appHasStopped()
    {
        if(appsEngine.isActivePlayList() || g_urlResProgress)
        {
            return
        }

        logger.info("appHasStopped()")
        activeFocus()
        loadingSpinnerAnim.stop()
        activeFocusViewManager(videoTemplate.item)
        opacity = 1
    }
    function appHasStarted()
    {
        if(appsEngine.isActivePlayList())
            return

        logger.info("appHasStarted()")
        loadingSpinnerAnim.start()
    }

    function endOfCategory(succeeded)
    {
        logger.info("VideoTemplate::endOfCategory succeeded: " + succeeded)

        if(!succeeded)
        {
            if(appsEngine.isActivePlayList())
            {
                playBackCompleted()
            }
            return
        }

        if(!g_refreshRequested)
        {
            if(g_modeView.isFolderElement())
            {
                IPTV_navhistory.storeNavElement(g_modeView.getCurrentUrl(),
                                                g_modeView.getCurrentLabel())
            }
        }
        appsHistoryHeader.text = IPTV_navhistory.getLabelHistory()
        g_modeView.applyMode()
        itemsReady(null)
    }

    function closeApplication()
    {
        appsView.abortRunningApp()
        g_refreshRequested = false

        g_mode = 0
        g_submode = 0
        g_modeView = null

        g_userHasChangedmode = false
        appsBackImg.source = ""
        fanArtImg.source = ""
        appsBackOverlay.source = ""

        clearAllViews()
        categoryModelComponent.clear()
        IPTV_navhistory.clearHistory()
        enabled = false
        opacity = 0
        focus = false
        pyNotificationView.clearview();
    }

    function reloadAppsList()
    {
        g_modeView.clear()
    }

    function appViewUpPressed()
    {
        if(loadingSpinnerAnim.running) return

        if(categoryItemList.enabled)
        {
            reloadAppsList()
        }
        else if(g_modeView.isListEnabled())
        {
            g_modeView.moveUp()
            fanArtImg.source = g_modeView.getCurrentFanart()
        }
    }

    function appViewDownPressed()
    {
        if(loadingSpinnerAnim.running) return

        if(categoryItemList.enabled)
        {
            reloadAppsList()
        }
        if(!g_modeView.isListEnabled()) return

        g_modeView.moveDown()
        fanArtImg.source = g_modeView.getCurrentFanart()
    }

    function appViewLeftPressed()
    {
        if(loadingSpinnerAnim.running) return

        if(!g_modeView.isListEnabled()) return

        g_modeView.moveLeft()
        fanArtImg.source = g_modeView.getCurrentFanart()
    }

    function appViewRightPressed()
    {

        if(loadingSpinnerAnim.running) return

        if(!g_modeView.isListEnabled()) return

        g_modeView.moveRight()
        fanArtImg.source = g_modeView.getCurrentFanart()
    }

    function appViewOkPressed()
    {
        if(loadingSpinnerAnim.running) return

        if(categoryItemList.enabled)
        {
            categoryItemList.enabled = false;
            categoryItemList.focus = false;
            categoryItemList.highlight = null
        }
        else if(g_modeView.isListEnabled())
        {
            videoTemplateLaunchApp(appsView.getActiveAppId(),
                                   g_modeView.getCurrentUrl())
        }
    }

    function appViewBackPressed()
    {
        if(categoryItemList.enabled)
        {
        }
        else if(g_modeView.isListEnabled())
        {
            if(!IPTV_navhistory.checkRootItem())
            {
                var hisItem = IPTV_navhistory.popNavItem()
                setViewMode(IPTV_navhistory.getDisplayMode(),
                            IPTV_navhistory.getDisplaySubMode())
                appsHistoryHeader.text = IPTV_navhistory.getLabelHistory()
                g_modeView.applyMode()
                itemsReady(hisItem)
            }
            else
            {
                activateAppView()
                closeApplication()
                clearTemplate()
            }
        }
    }

    function appViewMenuKeyPressed()
    {
        if(loadingSpinnerAnim.running) return

        var actionMenuDict = IPTV_navhistory.getActionsDict(g_modeView.getCurrentIndex())
        enabled = false
        focus = false
        g_modeView.setOpacity(0.3)
        var header = kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_OPTIONS) + ": " +
                     g_modeView.getCurrentLabel()
        popUpView.listActionMenu(actionMenuDict, header);
    }

    function appViewListKeyPressed()
    {
        var displaymode = (g_mode === IPTV_Constants.ENUM_APPS_GRID_MODE) ?
                                      IPTV_Constants.ENUM_APPS_LIST_MODE :
                                      IPTV_Constants.ENUM_APPS_GRID_MODE

        g_modeView= (displaymode === IPTV_Constants.ENUM_APPS_GRID_MODE) ?
                                    gridModeView :
                                    listModeView;
        g_mode = displaymode
        g_modeView.setViewMode(g_submode)

        g_userHasChangedmode = true
        g_modeView.applyMode()
        itemsReady(IPTV_navhistory.getCurrentNavItem())
    }

    function setResUrl(itemInfo)
    {
        if(!itemInfo)
            return false

        g_urlResProgress = true
        g_requestPlayItemInfo = itemInfo
        if( (itemInfo['mediatype'] === 'video' || itemInfo['mediatype'] === 'audio') &&
            (itemInfo['IsPlayable'] === 'true') )
        {
            mediaPlayer.enableAutoRestart();
            mediaPlayer.setSeekStart(0);
            if(itemInfo['path'] === "dualstream")
            {
                showVideo(itemInfo["videoUri"], itemInfo["audioUri"], true)
            }
            else
            {
                showVideo(itemInfo['path']);
            }
        }
        return true
    }

    function playBackStarted()
    {
        g_urlResProgress = false
        mediaPlayerView.setDownloadValue(100);
        if(!g_requestPlayItemInfo && appsEngine.isActivePlayList())
        {
            g_requestPlayItemInfo = appsEngine.getCurrentPlayListItem()
        }

        if(g_requestPlayItemInfo)
        {
            mediaPlayerView.setCurrentMediaInfo(
                        ('label' in g_requestPlayItemInfo) ? g_requestPlayItemInfo['label'] : "",
                        ('plot' in g_requestPlayItemInfo) ? g_requestPlayItemInfo['plot'] : "");
        }
        g_requestPlayItemInfo = null

        hideView()
        mediaPlayerView.invokerInfoBar()
    }

    function playBackCompleted()
    {
        logger.info("playBackCompleted...")
        if(appsEngine.isActivePlayList() && !opacity)
        {
            appsEngine.waitForUi()
            var item = appsEngine.getNextPlayListItem()
            mediaPlayerView.showNextVideoInfos(item, nexVideoInfoDisplayDone)
            return
        }

        mediaPlayerView.hideInfoBar();
        loadingSpinnerAnim.stop()
        showView()
    }

    function nexVideoInfoDisplayDone(status)
    {
        if(status && appsEngine.isActivePlayList())
        {
            appsEngine.playNext();
        }
        else
        {
            appsEngine.playListUiAborted()
            mediaPlayerView.hideInfoBar();
            showView()
        }
    }

    function playBackInterrupted(interruptedPosition)
    {
        playBackCompleted()
    }

    function playBackFailed()
    {
        g_urlResProgress = false
        loadingSpinnerAnim.stop()
        mediaPlayerView.hideInfoBar();
        pyNotificationView.requestAlertDisplay("error", null,
            kvalUiConfigManager.retranslate +
            kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VOD_VIDEO_LAUNCH_PROBLEM))
        showView();
    }

//------------------------------------------------------------------------------
// Keys Management
//------------------------------------------------------------------------------

    Keys.onPressed: {
        if (event.key === Qt.Key_Up) //Up Button
        {
            appViewUpPressed()
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {
            appViewDownPressed()
        }
        else if (event.key === Qt.Key_Right) //Right Button
        {
            appViewRightPressed()
        }
        else if (event.key === Qt.Key_Left) //Left Button
        {
            appViewLeftPressed()
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {
            appViewOkPressed()
        }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {
            appViewBackPressed();
        }
        else if(event.key === Qt.Key_M)
        {
            appViewMenuKeyPressed();
        }
        else if (event.key === Qt.Key_Return)
        {
            appViewListKeyPressed();
        }

        else if (event.key === Qt.Key_H) //Home Button
        {
            closeApplication();
            activateMainScreenView();
            clearTemplate()
        }
        else if (event.key === Qt.Key_R) //Red Button
        {
        }
        else if (event.key === Qt.Key_Y) //Yellow Button
        {
        }
        else {}
        event.accepted = true;
    }

//------------------------------------------------------------------------------
// Items
//------------------------------------------------------------------------------

    Item {
        id: appsBackGound
        anchors.fill: parent
        Image {
            id: appsBackImg
            anchors.fill: parent
            fillMode: Image.Stretch
        }
        Image {
            id: fanArtImg
            anchors.fill: parent
            fillMode: Image.Stretch
            asynchronous: true
            onStatusChanged: {
                if(status === Image.Ready) fanartShowAnimation.start()
            }
            onSourceChanged: fanartHideAnimation.start()
        }
        NumberAnimation
        {
            id: fanartShowAnimation
            target: fanArtImg;
            property: "opacity";
            from: 0; to: 1; duration: 200
        }
        NumberAnimation
        {
            id: fanartHideAnimation
            target: fanArtImg;
            property: "opacity";
            from: 1; to: 0; duration: 200
        }

        Image {
            id: appsBackOverlay
            anchors.fill: parent
            fillMode: Image.Stretch
            opacity: 0.9
        }
    }

    Item {
        id: appsHeaderZone
        anchors.top: parent.top
        anchors.topMargin: marginBlock
        anchors.left: parent.left
        anchors.leftMargin: marginBlock*2
        anchors.right: parent.right
        anchors.rightMargin: marginBlock*2
        height: parent.height * 0.1
        Item {
            id: spinner
            anchors.centerIn: parent
            visible: false
            Image {
                id: spinnerBack
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_SPINNER_BACK)
                scale: 0.5
                opacity: 0.6
            }
            Image {
                id: spinnerImg
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_SPINNER)
                scale: 0.5
                RotationAnimation on rotation
                {
                    id: loadingSpinnerAnim
                    running: false
                    loops: Animation.Infinite
                    direction: RotationAnimation.Clockwise
                    from: 0
                    to: 360
                    duration: 600
                    onStarted: {
                        spinner.visible = true
                    }
                    onStopped: {
                        spinner.visible = false
                    }
                }
            }
        }
        Text {
            id: appsTimeHeaderTxt
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
            anchors.right: appsTimeHeaderTxt.right
            anchors.top: appsTimeHeaderTxt.bottom
            anchors.topMargin: -marginBlock
            text: g_currentDateStdFormat
            color: "#6aa6cb"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.015
            }
        }
        Image{
            id: appsIco
            anchors.left: parent.left
            anchors.top: parent.top
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_ICONS)
            scale: 0.9
        }
        Text {
            id: appsTitle
            anchors.left: appsIco.right
            anchors.top: appsIco.top
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MY_APPS)
            color: "white"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.018
            }
        }
        Text {
            id: appsHistoryHeader
            anchors.left: appsIco.right
            anchors.bottom: appsIco.bottom
            width: parent.width *0.4
            elide:Text.ElideMiddle
            color: "white"
            opacity: 0.8
            font {
                family: localAdventFont.name
                pixelSize: parent.width * 0.015
            }
        }
    }

    Item {
        id: bottomZone
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: marginBlock*2
        anchors.right: parent.right
        anchors.rightMargin: marginBlock*2
        height: parent.height * 0.07
        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left
            height: 1
            color: "white"
            opacity: 0.6
        }
        Text {
            id: bottomZoneAppName
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            color: "white"
            opacity: 0.6
            font {
                family: localAdventFont.name
                pixelSize: parent.height * 0.5
            }
        }
        Image{
            id: bottomZoneAppIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -marginBlock*0.5
            anchors.right: bottomZoneAppName.left
            anchors.rightMargin: -marginBlock*2
            scale: 0.5
            opacity: 0.6
        }

        Image{
            id: backButton
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: parent.width *0.02
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_MENU_ICON)
            Text {
                anchors.centerIn: parent
                text: 'BACK'
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.35
                }
            }
            Text {
                id: backButtonText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.right
                anchors.leftMargin: -marginBlock*0.7
                text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_BACK)
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.5
                }
            }
        }
        Image{
            id: menuButton
            anchors.top: parent.top
            anchors.left: backButton.right
            anchors.leftMargin: backButtonText.width*1.2
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_MENU_ICON)
            Text {
                anchors.centerIn: parent
                text: 'MENU'
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.35
                }
            }
            Text {
                id: menuButtonText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.right
                anchors.leftMargin: -marginBlock*0.7
                text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_OPTIONS)
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.5
                }
            }
        }
        Image{
            id: displayModeButton
            anchors.top: parent.top
            anchors.left: menuButton.right
            anchors.leftMargin: menuButtonText.width*1.2
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_MENU_ICON)
            Text {
                anchors.centerIn: parent
                text: 'LIST'
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.35
                }
            }
            Text {
                id: displayModeButtonText
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.right
                anchors.leftMargin: -marginBlock*0.7
                text: 'Mode'
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.5
                }
            }
        }

    }

    Item {
        id: categoryZone
        anchors.top: appsHeaderZone.bottom
        anchors.topMargin: marginBlock*2
        anchors.left: parent.left
        height: parent.height * 0.7
        width: parent.width * 0.05
    }

    Item {
        id: itemsZone
        anchors.top: appsHeaderZone.bottom
        anchors.topMargin: marginBlock
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0.15
        width: parent.width * 0.8
        height: parent.height * 0.8
    }

    Component {
        id : categoryListComponent
        Row {
            id: wrapperRow
            spacing: 2
            Item {
                id: indexAppRect
                width: categoryZone.width - marginBlock*5
                height: categoryZone.height / 10
                Item {
                    id: iconItem
                    anchors.top: parent.top
                    anchors.topMargin: marginBlock*3
                    anchors.left: parent.left
                    anchors.leftMargin: marginBlock*5
                    height: parent.height
                    width: parent.width*0.2
                    opacity: 0.6
                    Image {
                        id: ico
                        anchors.centerIn: parent
                        source: categoryIcon
                    }
                    Glow {
                        anchors.fill: ico
                        radius: 8
                        opacity: wrapperRow.ListView.isCurrentItem ? 1 : 0
                        samples: 17
                        spread: 0.1
                        color: "white"
                        transparentBorder: true
                        source: ico
                    }
                }
            }
        }
    }

    ListModel {
         id: categoryModelComponent
         //Template to Use
         ListElement {
             categoryIcon: ""
             categoryTxt: ""
             categoryTxtNbr: ""
         }
     }

    ListView {
        id: categoryItemList
        anchors.left: categoryZone.left
        anchors.leftMargin: -marginBlock
        anchors.right: categoryZone.right
        anchors.top: categoryZone.top
        anchors.topMargin: categoryZone.height * 0.15
        anchors.bottom: categoryZone.bottom
        enabled: false
        opacity: 0
        focus: false

        model: categoryModelComponent

        highlightFollowsCurrentItem: true
        clip: true
        keyNavigationWraps: true
        interactive: false
        orientation: ListView.Vertical
        delegate: categoryListComponent
    }

    GridMode {
        id: gridModeView
        itemsZoneRef: itemsZone
    }

    ListMode {
        id: listModeView
        itemsZoneRef: itemsZone
    }
//    FastBlur {
//         anchors.fill: gridModeView
//         source: gridModeView
//         radius: 32
//    }

}
