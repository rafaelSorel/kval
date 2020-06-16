import QtQuick 2.3
import "KvalUI_constant.js" as IPTV_Constants
import QtGraphicalEffects 1.0
import kval.gui.qml 1.0

Item
{
    id: appsView
    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

    enabled: false
    opacity: 0
    focus: false

//------------------------------------------------------------------------------
// Global Vars
//------------------------------------------------------------------------------

    signal activateMainScreenView()
    signal activateVideoTemplateView()
    signal appsScreenReadyNotify()
    signal appsScreenLaunchApp(string appid, string args)
    signal requesForAppAbort()
    signal focusRelinquished();

    signal appUninstallNotify(string appId)
    signal appUpdateNotify(string appId)
    signal appAddFavNotify(string appId)
    signal appRemoveFavNotify(string appId)

    property int g_appInstalledNbr : 0
    property int g_appStoreInstalledNbr: 0
    property int g_appFavInstalledNbr: 0
    property int g_appMiscInstalledNbr: 0
    property int g_appVidInstalledNbr: 0
    property int g_appGameInstalledNbr: 0
    property int g_appAllInstalledNbr: 0
    property int g_ActiveAppType: -1
    property bool g_waitForMotionGraphics: false
    property var videoComponent: null
    property var g_objComponent: null
    property string g_kvalStoreAppId: "kval.store.app"

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

    function checkAppById(appid)
    {
        var count = appsEngine.getInstalledAppsCount();
        for(var i = 0; i < count; i++)
        {
            if(appid === appsEngine.getAppsMap(i)["name"])
                return true
        }
        return false
    }

    function getActiveAppId()
    {
        return appsModel.get(storeItemList.currentIndex).appId
    }
    function getActiveAppName()
    {
        return appsModel.get(storeItemList.currentIndex).appName
    }
    function getActiveAppFanart()
    {
        return appsModel.get(storeItemList.currentIndex).appFanart
    }
    function getActiveAppIcon()
    {
        return appsModel.get(storeItemList.currentIndex).appIcon
    }
    function getActiveAppCategory()
    {
        return categoryItemList.currentIndex
    }
    function getActiveAppPath()
    {
        return appsModel.get(storeItemList.currentIndex).appPath
    }

    function createAppTemplateObj()
    {
        var ui_type = appsModel.get(storeItemList.currentIndex).appUiType
        if (ui_type === 'video')
        {
            if(videoTemplate.status == Component.Ready)
            {
                logger.info("videoTemplate Ready...")
            }
            else
            {
                logger.info("Create videoTemplate component...")
                videoTemplate.setSource("/qmlFolder/appstemplates/VideoTemplate.qml")
                if (videoTemplate.status !== Component.Ready)
                {
                    videoTemplate.statusChanged.connect(finishCreation);
                }
            }
        }
        else if(ui_type === 'custom')
        {
            logger.info(
                "Ui type custom, wait for application to send its qml file...")
        }
        else
        {
            logger.info("Ui type unknown...")
        }
    }

    function customUiNotify(uiTemplate)
    {
        logger.info("customUiNotify: " + uiTemplate)
        if( appsModel.get(storeItemList.currentIndex).appUiType === 'custom' &&
            !customUiAppTemplate.sourceComponent ) //Load it once at startup of the application
        {
            customUiAppTemplate.setSource("file://" + uiTemplate)
        }
    }

    function registerObjApp(obj)
    {
        g_objComponent = obj
    }

    function unregisterObjApp(obj)
    {
        logger.info(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> unregisterObjApp")
        if(g_objComponent === obj)
            g_objComponent = null
    }

    function getActiveAppTemplateObj()
    {
        if(g_objComponent)
        {
            return g_objComponent
        }
        else if(appsModel.count > 0)
        {
            var ui_type = appsModel.get(storeItemList.currentIndex).appUiType
            if (ui_type === 'video') return videoTemplate.item
            else if(ui_type === 'custom') return customUiAppTemplate.item
            else return null
        }
        else return null
    }

    function refresh()
    {
        if(!opacity) getActiveAppTemplateObj().refresh()
        else activateAppsScreen(false)
    }

    function update(path, replace)
    {
        if(!opacity) getActiveAppTemplateObj().update(path, replace)
    }

    function setViewMode(mode, submode)
    {
        if(getActiveAppTemplateObj())
            getActiveAppTemplateObj().setViewMode(mode, submode)
    }

    function appHasStopped()
    {
        if (!opacity)
        {
            if(getActiveAppTemplateObj())
                getActiveAppTemplateObj().appHasStopped()
        }
        else
        {
            activeFocusViewManager(appsView)
            loadingSpinnerAnim.stop()
        }
    }
    function appHasStarted()
    {
        if (!opacity)
        {
            if(getActiveAppTemplateObj())
                getActiveAppTemplateObj().appHasStarted()
        }
        else
        {
            //Disable view until a reply from application engine, may be give a slot
            //for HOME access button, if something terribly happends in app side
            loadingSpinnerAnim.start()
            enabled = false
            createAppTemplateObj()
        }
    }

    function endOfCategory(succeeded)
    {
        //Check if the apps view is active
        if (!opacity)
        {
            if(getActiveAppTemplateObj())
                getActiveAppTemplateObj().endOfCategory(succeeded)
            return
        }

        loadingSpinnerAnim.stop()
        logger.info("Apps View succeeded: " + succeeded)
        if(!succeeded)
        {
            activeFocusViewManager(appsView)
            if(appsModel.get(storeItemList.currentIndex).appUiType === 'video')
                videoTemplate.sourceComponent = undefined
            else if(appsModel.get(storeItemList.currentIndex).appUiType === 'custom')
                customUiAppTemplate.sourceComponent = undefined
            kvalUiGuiUtils.trimCache()
            return
        }

        //Active the related app view
        if (!getActiveAppTemplateObj()) return

        getActiveAppTemplateObj().activateTemplateView()
        appsBackImg.source = ""
        appsBackOverlay.source = ""
        enabled = false
        opacity = 0
        focus = false
        stopCurrentMedia()
    }

    function popUpTryGetFocus()
    {
        if(!enabled)
        {
            return false
        }
        storeItemList.opacity = 0.5
        enabled = false
        focus = false
        return true
    }
    function popUpFocusRelinquished()
    {
        storeItemList.opacity = 1
        activeFocusViewManager(appsView)
    }

    function diagCtxActionCallback(action)
    {
        storeItemList.opacity = 1
        activeFocusViewManager(appsView)
        if(action) this[action]()
    }
    function abortRunningApp()
    {
        requesForAppAbort()
    }

    function setResUrl(itemInfo)
    {
        if(!opacity && getActiveAppTemplateObj())
            getActiveAppTemplateObj().setResUrl(itemInfo)
    }

    function playBackStarted()
    {
        if(g_waitForMotionGraphics)
        {
            activeFocusViewManager(appsView)
            opacity = 1
            storeItemList.opacity = 1
            g_waitForMotionGraphics = false
            return;
        }

        if(!opacity && getActiveAppTemplateObj())
            getActiveAppTemplateObj().playBackStarted()
    }

    function playBackCompleted()
    {
        if(!opacity && getActiveAppTemplateObj() && getActiveAppTemplateObj().playBackCompleted)
        {
            getActiveAppTemplateObj().playBackCompleted()
        }
    }

    function playBackInterrupted(interruptedPosition)
    {
        if(!opacity && getActiveAppTemplateObj())
            getActiveAppTemplateObj().playBackInterrupted(interruptedPosition)
    }

    function playBackFailed()
    {
        if(g_waitForMotionGraphics)
        {
            appsBackImg.source = 'file:///storage/.kval/apps/motion_graphics/motion_graphics.jpg'
            activeFocusViewManager(appsView)
            opacity = 1
            storeItemList.opacity = 1

            g_waitForMotionGraphics = false
            return;
        }

        if(!opacity && getActiveAppTemplateObj())
            getActiveAppTemplateObj().playBackFailed()
    }

    function initAppScreen()
    {
        g_waitForMotionGraphics = false
        g_appInstalledNbr = 0
        g_appStoreInstalledNbr= 0
        g_appFavInstalledNbr= 0
        g_appMiscInstalledNbr= 0
        g_appVidInstalledNbr= 0
        g_appGameInstalledNbr= 0
        g_appAllInstalledNbr= 0
        categoryModelComponent.clear()
        categoryItemList.currentIndex = 0;
        appsModel.clear()
        refModel.clear()
    }
    function finishCreation()
    {
        logger.info("videoComponent finishCreation")
    }

    function customUiStatusChanged(status)
    {
        if(status === Component.Ready)
        {
            logger.info("custom UI template ready")
            appsEngine.customUiTemplateReady(true)
        }
        else if(status === Component.Error)
        {
            logger.error("custom UI template error")
            appsEngine.customUiTemplateReady(false)
            abortRunningApp()
            endOfCategory(false)
        }
    }

    function getCatListContent()
    {
        var catListContent = [
        {'id'   : IPTV_Constants.ENUM_APPS_VID,
         'text': kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_TV_VIDEO_APPS),
        'icon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_VID_ICO),
        'number' : g_appVidInstalledNbr.toString()},
        {'id'   : IPTV_Constants.ENUM_APPS_GAMES,
        'text': kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_GAMES_APPS) ,
        'icon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_GAMES_ICO),
        'number' : g_appGameInstalledNbr.toString()},
        {'id'   : IPTV_Constants.ENUM_APPS_MISC,
        'text': kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_PROGRAM_APPS),
        'icon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_MISC_ICO),
        'number' : g_appMiscInstalledNbr.toString()},
        {'id'   : IPTV_Constants.ENUM_APPS_FAV,
         'text': kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_FAVORITES_APPS),
        'icon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_FAV_ICO),
        'number' : g_appFavInstalledNbr.toString()},
        {'id'   : IPTV_Constants.ENUM_APPS_STORE,
         'text': kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_STORE_APPS) ,
        'icon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_STORE_ICO),
        'number' : g_appStoreInstalledNbr.toString()},
        {'id'   : IPTV_Constants.ENUM_APPS_ALL,
        'text' : kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_ALL_APPS),
        'icon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_ALL_ICO),
        'number' : g_appAllInstalledNbr.toString()}]
        return catListContent;
    }

    function activateAppsScreen(isHomeRequest)
    {
        kvalUiGuiUtils.trimCache()
        initAppScreen()

        var count = appsEngine.getInstalledAppsCount();
        for(var i = 0; i < count; i++)
        {
            var app = appsEngine.getAppsMap(i)
            if(app["favorite"] === '1') g_appFavInstalledNbr++;
            if(app["category"] === 'video') g_appVidInstalledNbr++;
            else if(app["category"] === 'store') g_appStoreInstalledNbr++;
            else if(app["category"] === 'misc') g_appMiscInstalledNbr++;
            else if(app["category"] === 'game') g_appGameInstalledNbr++;

            g_appAllInstalledNbr++;
            g_appInstalledNbr++;

            refModel.insert(i, {'appId'     : app["name"],
                                'appName'   : app["ui_name"],
                                'appVersion': app["version"],
                                'appResume' : app["resume"],
                                'appIcon'   : app["icon"],
                                'appBack'   : app["backimage"],
                                'appFanart' : app["fanart"],
                                'appCat'    : app["category"],
                                'appProvider': app["provider"],
                                'appUiType' : app["ui_type"],
                                'appFav'    : app["favorite"] === "1" ? true : false,
                                'appUpdate' : app["update"] === "1" ? 1 : 0.5,
                                'appPath'   : app["path"]});
        }

        var categoryListContent = getCatListContent()

        for(i = 0; i < categoryListContent.length; i++ )
        {
            categoryModelComponent.insert(i,
            {'categoryId'      : categoryListContent[i]["id"],
             'categoryIcon'    : categoryListContent[i]["icon"],
             'categoryTxt'     : categoryListContent[i]["text"],
             'categoryTxtNbr'  : categoryListContent[i]["number"]});
        }

        if(!g_appVidInstalledNbr) categoryItemList.currentIndex = categoryListContent.length-1
        reloadAppsList()

        categoryItemList.positionViewAtBeginning();
        categoryItemList.opacity = 1;
        categoryItemList.enabled = true;
        categoryItemList.focus = true;
        categoryItemList.highlight = highlightCatagoryItem

        storeItemList.opacity = 1;
        storeItemList.enabled = false;
        storeItemList.focus = false;
        storeItemList.currentIndex = -1
        storeItemList.highlight = dummyHighlight
        if(!opacity)
        {
            if (isHomeRequest) appsScreenReadyNotify()
            else showAppsScreen()
        }
    }

    function showAppsScreen()
    {
        registerActiveView(appsView)
        g_waitForMotionGraphics = true
        setActivePlayingList( IPTV_Constants.ENUM_ACTIVE_PLAYING_APPLICATIONS );
        setNavLevel( IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL );
        showVideo(kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_MOTION_GRAPHICS),"",false,true)

        //appsBackOverlay.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND_BLACK_2)
        appsBackOverlay.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND_2)
        loadingSpinnerAnim.stop()
    }

    function backToHomeView()
    {
        appsBackImg.source = ""
        appsBackOverlay.source = ""
        loadingSpinnerAnim.stop()
        spinner.visible = false

        activateMainScreenView();
        enabled = false
        opacity = 0
        focus = false
        pyNotificationView.clearview();
        stopCurrentMedia()

        setActivePlayingList( IPTV_Constants.ENUM_ACTIVE_PLAYING_DEFAULT);
        setNavLevel( IPTV_Constants.ENUM_CATEGORIES_ROOT_LEVEL);
    }

    function reloadAppsList()
    {
        var category = categoryModelComponent.get(categoryItemList.currentIndex).categoryId

        appsModel.clear()

        var appCat = 'all'
        if(category === IPTV_Constants.ENUM_APPS_VID) appCat = 'video'
        else if(category === IPTV_Constants.ENUM_APPS_GAMES) appCat = 'game'
        else if(category === IPTV_Constants.ENUM_APPS_MISC) appCat = 'misc'
        else if(category === IPTV_Constants.ENUM_APPS_FAV) appCat = 'favorite'
        else if(category === IPTV_Constants.ENUM_APPS_STORE) appCat = 'store'

        var index = 0;
        for(var i = 0; i < refModel.count ; i++)
        {
            if(category === IPTV_Constants.ENUM_APPS_FAV)
            {
                if(!refModel.get(i).appFav)
                    continue
            }
            else
            {
                if(refModel.get(i).appCat !== appCat && appCat != 'all')
                    continue
            }

            appsModel.insert(index, { 'appId'  : refModel.get(i).appId,
             'appName'  : refModel.get(i).appName,
             'appVersion': refModel.get(i).appVersion,
             'appResume' : refModel.get(i).appResume,
             'appIcon'   : refModel.get(i).appIcon,
             'appBack'   : refModel.get(i).appBack,
             'appFanart' : refModel.get(i).appFanart,
             'appCat'    : refModel.get(i).appCat,
             'appProvider': refModel.get(i).appProvider,
             'appUiType' : refModel.get(i).appUiType,
             'appFav'    : refModel.get(i).appFav,
             'appUpdate' : refModel.get(i).appUpdate,
             'appPath'   : refModel.get(i).appPath});
             index = index + 1
        }
    }

    function appViewUpPressed()
    {
        if(categoryItemList.enabled)
        {
            categoryItemList.decrementCurrentIndex();
            reloadAppsList()
        }
        else if(storeItemList.enabled)
        {
            storeItemList.moveCurrentIndexUp()
        }
    }

    function appViewDownPressed()
    {
        if(categoryItemList.enabled)
        {
            categoryItemList.incrementCurrentIndex();
            reloadAppsList()
        }
        else if(storeItemList.enabled)
        {
            storeItemList.moveCurrentIndexDown()
        }
    }

    function appViewLeftPressed()
    {
        if(storeItemList.enabled)
        {
            storeItemList.moveCurrentIndexLeft()
        }
    }

    function appViewRightPressed()
    {
        if(storeItemList.enabled)
        {
            storeItemList.moveCurrentIndexRight()
        }
    }

    function appViewOkPressed()
    {
        if(categoryItemList.enabled)
        {
            if(!appsModel.count) return
            categoryItemList.enabled = false;
            categoryItemList.focus = false;
            categoryItemList.highlight = dummyHighlight

            storeItemList.enabled = true;
            storeItemList.focus = true;
            storeItemList.highlight = highlightItem
            storeItemList.currentIndex = 0
        }
        else
        {
            if(appsModel.count > 0)
                appsScreenLaunchApp(appsModel.get(storeItemList.currentIndex).appId, "")
        }
    }

    function appViewBackPressed()
    {
        if(storeItemList.enabled)
        {
            categoryItemList.enabled = true;
            categoryItemList.focus = true;
            categoryItemList.highlight = highlightCatagoryItem

            storeItemList.enabled = false;
            storeItemList.focus = false;
            storeItemList.highlight = dummyHighlight
            storeItemList.currentIndex = -1
        }
    }

    function appViewMenuKeyPressed()
    {
        if(loadingSpinnerAnim.running) return

        var header = kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_OPTIONS);
        var actionMenuDict = {}
        if(categoryItemList.enabled)
        {
            return
        }
        else
        {
            header=header+": "+appsModel.get(storeItemList.currentIndex).appName
            actionMenuDict[kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_INFORMATIONS)] = 'appViewAppInfos'
            actionMenuDict[kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_ADD_FAV)] = 'appViewAddFavorite'
            actionMenuDict[kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_UPDATE)] = 'appViewAppUpdate'
            if(appsModel.get(storeItemList.currentIndex).appId !== g_kvalStoreAppId)
            {
                //Do not uninstall store application
                actionMenuDict[kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_UNINSTALL)] = 'appViewAppUninstall'
            }
            actionMenuDict[kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_SETTINGS)] = 'appViewAppSettings'
            if( categoryModelComponent.get(
                categoryItemList.currentIndex).categoryId ===
                IPTV_Constants.ENUM_APPS_FAV)
            {
                actionMenuDict[kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_REMOVE_FAV)] =
                        'appViewRemoveFavorite'
            }
        }
        popUpView.listActionMenu(actionMenuDict, header);
        enabled = false
        focus = false
        storeItemList.opacity = 0.5
    }

    function appViewAppInfos()
    {
        logger.info("appViewAppInfos")
        popUpView.appInfoDisplay(appsModel.get(storeItemList.currentIndex),
                                 appsView);
    }
    function appViewAppUninstall()
    {
        logger.info("appViewAppUninstall")
        appUninstallNotify(appsModel.get(storeItemList.currentIndex).appId)
    }
    function appViewAppUpdate()
    {
        logger.info("appViewAppUpdate")
        appUpdateNotify(appsModel.get(storeItemList.currentIndex).appId)
    }
    function appViewAppSettings()
    {
        logger.info("appViewAppSettings")
    }
    function appViewAddFavorite()
    {
        logger.info("appViewAddFavorite")
        loadingSpinnerAnim.start()
        enabled = false
        focus = false
        appAddFavNotify(appsModel.get(storeItemList.currentIndex).appId)
    }
    function appViewRemoveFavorite()
    {
        logger.info("appViewRemoveFavorite")
        loadingSpinnerAnim.start()
        enabled = false
        focus = false
        appRemoveFavNotify(appsModel.get(storeItemList.currentIndex).appId)
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
        else if (event.key === Qt.Key_H) //Home Button
        {
            backToHomeView();
        }
        else if(event.key === Qt.Key_M)
        {
            appViewMenuKeyPressed()
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
            id: appsBackOverlay
            anchors.fill: parent
            fillMode: Image.Stretch
            opacity: 0.5
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
            scale: 0.8
        }
        Text {
            id: appsTitle
            anchors.left: appsIco.right
            anchors.verticalCenter: appsIco.verticalCenter
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MY_APPS)
            color: "white"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.018
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
            id: appName
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MY_APPS)
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
            anchors.right: appName.left
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
    }

    Item {
        id: categoryZone
        anchors.top: appsHeaderZone.bottom
        anchors.topMargin: marginBlock*5
        anchors.left: parent.left
        anchors.leftMargin: marginBlock*5
        height: parent.height * 0.7
        width: parent.width * 0.25
        scale: 1.15
        Item {
            id: installedIconItem
            anchors.top: parent.top
            anchors.topMargin: marginBlock*4
            anchors.left: parent.left
            anchors.leftMargin: marginBlock*5
            height: parent.height/10
            width: (parent.width-marginBlock*5)*0.2
            opacity: 0.6
            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_INSTALLED_ICO)
            }
        }
        Rectangle {
            anchors.top: installedIconItem.bottom
            anchors.topMargin: marginBlock
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: marginBlock
            width: parent.width-marginBlock*11
            height: 1
            opacity: 0.8
            color: "white"
        }

        Text {
            anchors.verticalCenter: installedIconItem.verticalCenter
            anchors.left: installedIconItem.right
            color: "white"
            text: kvalUiConfigManager.retranslate + qsTr("Installées")
            opacity: 0.8
            font {
              family: localAdventFont.name;
              pixelSize: (parent.width-marginBlock*5) * 0.07
            }
        }
        Text {
            anchors.verticalCenter: installedIconItem.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: marginBlock*5
            color: "white"
            text: g_appInstalledNbr.toString()
            opacity: 0.8
            font {
              family: localAdventFont.name;
              pixelSize: (parent.width-marginBlock*5) * 0.06
            }
        }
    }

    Item {
        id: storeAppsZone
        anchors.top: categoryZone.top
        anchors.topMargin: -marginBlock*2
        anchors.left: categoryZone.right
        anchors.leftMargin: marginBlock * 5
        width: parent.width * 0.7 - marginBlock * 5
        height: parent.height * 0.8
    }
    Component {
        id: highlightCatagoryItem

        Item {
            id: highlightCatRect
            anchors.horizontalCenter: categoryItemList.currentItem.horizontalCenter
            anchors.horizontalCenterOffset: marginBlock*3.5
            anchors.verticalCenter: categoryItemList.currentItem.verticalCenter
            anchors.verticalCenterOffset: marginBlock*3
            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_SELCTOR_CAT)
            }
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation
                {
                    target: highlightCatRect;
                    property: "opacity";
                    from: 1.2; to: 0.2; duration: 800
                }
                NumberAnimation
                {
                    target: highlightCatRect;
                    property: "opacity";
                    from: 0.2; to: 1.2; duration: 800
                }
            }
        }
    }

    Component {
        id : categoryListComponent
        Row {
            id: wrapperRow
            spacing: 2
            Item {
                id: wrapper
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
                Text {
                    id: categ
                    anchors.verticalCenter: iconItem.verticalCenter
                    anchors.left: iconItem.right
                    color: "white"
                    text: categoryTxt
                    opacity: 0.6
                    font {
                      family: localAdventFont.name;
                      pixelSize: parent.width * 0.07
                    }
                }
                Glow {
                    anchors.fill: categ
                    radius: 8
                    opacity: wrapperRow.ListView.isCurrentItem ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: categ
                }
                Text {
                    id: nbr
                    anchors.verticalCenter: iconItem.verticalCenter
                    anchors.right: parent.right
                    color: "white"
                    text: categoryTxtNbr
                    opacity: 0.9
                    font {
                      family: localAdventFont.name;
                      pixelSize: parent.width * 0.06
                    }
                }
                Glow {
                    anchors.fill: nbr
                    radius: 8
                    opacity: wrapperRow.ListView.isCurrentItem ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: nbr
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
        anchors.right: categoryZone.right
        anchors.top: categoryZone.top
        anchors.topMargin: categoryZone.height * 0.15
        anchors.bottom: categoryZone.bottom
        scale: 1.15
        enabled: false
        opacity: 0
        focus: false

        model: categoryModelComponent

        highlight: highlightCatagoryItem
        highlightFollowsCurrentItem: true
        clip: true
        keyNavigationWraps: true
        interactive: false
        orientation: ListView.Vertical
        delegate: categoryListComponent
    }


    Image {
        id: refBlockApp
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_BLOCK_BACK)
        visible: false
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
        id: highlightItem

        Item {
            id: highlightRect
            anchors.horizontalCenter: storeItemList.currentItem.horizontalCenter
            anchors.horizontalCenterOffset: -1
            anchors.verticalCenter: storeItemList.currentItem.verticalCenter
            z: 2
            scale: 1.05
            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_SELECTOR)
            }
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 1; to: 0; duration: 800
                }
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 0; to: 1; duration: 800
                }
            }
        }
    }

    Component {
        id : appListComponent
        Item {
            id: indexAppRect
            width: refBlockApp.width
            height: refBlockApp.height
            scale: GridView.isCurrentItem ? 1.05 : 1

            Image {
                id: appBlockBack
                x: appBlockImg.x + marginBlock*1.1
                y: appBlockImg.y + marginBlock
                height: appBlockImg.height - marginBlock*2
                width: appBlockImg.width * 0.6 - marginBlock*1.5
                source: appBack
            }
            Image {
                id: appBlockImg
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_BLOCK_BACK)
            }
            Image {
                anchors.verticalCenter: appBlockImg.verticalCenter
                anchors.right: appBlockBack.right
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_BLOCK_SEP)
            }
            Image {
                anchors.centerIn: appBlockBack
                source: appIcon
            }
            Item {
                id: resumeZone
                anchors.left: appBlockBack.right
                anchors.leftMargin: marginBlock/2
                anchors.top: appBlockBack.top
                anchors.topMargin: marginBlock*1.5
                anchors.right: appBlockImg.right
                anchors.rightMargin: marginBlock*2
                anchors.bottom: appBlockImg.bottom
                anchors.bottomMargin: marginBlock
                Text {
                    id: selectedApp
                    anchors.left: parent.left
                    anchors.top: parent.top
                    width: parent.width
                    elide: Text.ElideRight
                    color: "white"
                    text: appName
                    font {
                      family: localAdventFont.name;
                      pixelSize: appBlockBack.width * 0.12
                    }
                }
                Image {
                    id: firstFade
                    anchors.top: selectedApp.bottom
                    anchors.topMargin: marginBlock*0.7
                    anchors.left: parent.left
                    anchors.right: parent.right
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_FADED)
                }
                Text {
                    id: versionApp
                    anchors.left: parent.left
                    anchors.top: firstFade.top
                    anchors.topMargin: marginBlock
                    width: parent.width
                    elide: Text.ElideRight
                    color: "white"
                    text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_VERSION) + ": " + appVersion
                    font {
                      family: localAdventFont.name;
                      pixelSize: appBlockBack.width * 0.1
                    }
                }
                Image {
                    id: secondFade
                    anchors.top: versionApp.bottom
                    anchors.topMargin: marginBlock*0.6
                    anchors.left: parent.left
                    anchors.right: parent.right
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_FADED)
                }
                Text {
                    id: resumeApp
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: secondFade.top
                    anchors.topMargin: marginBlock
                    anchors.bottom: upgradeImg.top
                    anchors.bottomMargin: -marginBlock
                    color: "white"
                    width: parent.width
                    elide: Text.ElideRight
                    wrapMode: Text.WordWrap
                    text: appResume
                    font {
                      family: localAdventFont.name;
                      pixelSize: appBlockBack.width * 0.09
                    }
                }
                Image {
                    id: upgradeImg
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: marginBlock/2
                    anchors.right: parent.right
                    anchors.rightMargin: marginBlock/2
                    opacity: appUpdate
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_UPGRADE_ICON)
                }
            }
        }
    }

    ListModel {
         id: refModel
         //Template to Use
         ListElement {
             appId: ""
             appName: ""
             appVersion: ""
             appResume: ""
             appIcon: ""
             appBack: ""
             appFanart: ""
             appCat: ""
             appProvider: ""
             appUiType: ""
             appUpdate: 1
             appFav: false
         }
     }

    ListModel {
         id: appsModel
         //Template to Use
         ListElement {
             appId: ""
             appName: ""
             appVersion: ""
             appResume: ""
             appIcon: ""
             appBack: ""
             appFanart: ""
             appCat: ""
             appProvider: ""
             appUiType: ""
             appUpdate: 1
             appFav: false
             appPath: ""
         }
     }
    Rectangle {
        id: gridViewRect
        anchors.fill: storeAppsZone
        color: "transparent"
        GridView {
            id: storeItemList
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: marginBlock*1.5
            anchors.bottom: parent.bottom
            cellHeight: refBlockApp.height
            cellWidth: refBlockApp.width
            enabled: false
            opacity: 0
            focus: false
            model: appsModel
            highlightFollowsCurrentItem: true
            clip: true
            keyNavigationWraps: true
            interactive: false
            delegate: appListComponent
        }
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: girdItemsMaskImg
        }
    }

    Image {
        id: girdItemsMaskImg
        anchors.fill: gridViewRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_MASKGRID)
        visible: false
    }
}
