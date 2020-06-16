import QtQuick 2.2
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.0
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_backgroundImagesDb.js" as BACKGROUNG_DB
import "KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0


Item
{
    id: vodMainView
    enabled: false
    opacity: 0
    focus: false
    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

    NumberAnimation {
           id: vodViewScaleAnimationOn

           target: vodMainView
           properties: "scale"
           from: 1.2
           to: 1
           duration: 50

           onStarted: {
               activeFocusViewManager(vodMainView)
               vodViewOpacityAnimationOn.start()
           }
    }
    NumberAnimation {
           id: vodViewOpacityAnimationOn

           target: vodMainView
           properties: "opacity"
           from: 0
           to: 1
           duration: 50
    }
    NumberAnimation {
           id: vodViewScaleAnimationOff

           target: vodMainView
           properties: "scale"
           from: 1
           to: 1.2
           duration: 50

           onStarted: {
               vodViewOpacityAnimationOff.start()
           }
    }

    NumberAnimation {
           id: vodViewOpacityAnimationOff

           target: vodMainView
           properties: "opacity"
           from: 1
           to: 0
           duration: 50
    }

    //Signals
    signal focusRelinquished()
    signal generateItemListNotify(string url, string extraArg)
    signal generateItemsPage(int index, int pageNbr)
    signal generateHistoryBackNotify()
    signal clearPageHistory()
    signal activateMainScreenView()
    signal deactivateMainView()
    signal generateDetailedItemInfos(int index, bool isSerie)
    signal generateSerieItems(int index, bool contextList)
    signal generateSeasonItems(int index, string seasonNumber, bool contextList)
    signal generateSerieHistoryList(bool isSeason);
    signal addMediaToFavorite(int index);
    signal deleteSelectedFav(int index)
    signal vodMenuReadyNotify()

    //Define Local variables

    property string    headerCategoryName    : "";
    property string    headerSubCategoryName : "";
    property string    headerMediaListName   : "";
    property string    headerVodStr          : "";
    property int       alternateAnimation    :0;
    property int       backAlternateAnimation:0;
    property int       vodBackgroundImageid:1;
    property int       vodCurrentNavigationLevel: 1;
    property int       vodCurrentCatIndex: 1;
    property int       vodCurrentSubCatIndex: 1;
    property int       vodCurrentMediaIndex: 1;
    property int       nextPropIndex : 0;
    property int       previousPropIndex : 0;
    property string    vodMediaListUrl: "";
    property int       vodCurrentPageNbr: 0;
    property bool      nextPropChanged : false;
    property bool      previousPropChanged : false;
    property bool      isSeriesFlag : false;
    property int       seriesIndexHistory : 0;
    property int       serieNavCounter : 0;
    property int       itemIndexHistory:0;
    property int       itemSeasonIndexHistory:0;
    property bool      backgroundAnimEnabled: false;
    property real      categorySubTextOpacity: 0
    property real      categoryIconOpacity: 0
    property real      categoryNbrElementsPages: 0
    property real      categoryTextXmargin: 0
    property bool      g_vodInFlag: false
    property bool      g_vodNewAvailableFile: false
    property bool      isReloadInProgress: false
    property bool      g_vodFileMissing: false
    property string    g_vod_srv_address: ""
    property int       g_cacheDurationValue: -1
    property int       g_bufferingValue: -1
    property bool      g_streamVod: true
    property string    g_contentType: ""


    function getStreamVodValue()
    {
        return g_streamVod;
    }

    function newFileAvailable()
    {g_vodNewAvailableFile = true
    }
    function preloadVodBacks()
    {
        BACKGROUNG_DB.preFillBackGroundArt(
             kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BACKGROUNG_ICON_VOD_ANIMATED));
    }

    function startVodMainView()
    {
        vodCurrentNavigationLevel = 1;
        vodCategoryModel.query = "/categories/category";
        isReloadInProgress = true
        xmlVodQuickSettingModel.reload()
        showVodMainView();
        startAnimationBackGround();
        vodBackgroundRect.opacity = 1;
    }

    function activateVodMainView()
    {
        activeFocusViewManager(vodMainView)
        if(g_vodNewAvailableFile) serverNotificationEngine.updateVodFiles()

        if (g_vod_srv_address !== kvalUiSettingsManager.value("vod", 'srvaddr'))
        {
            g_vod_srv_address = kvalUiSettingsManager.value("vod", 'srvaddr')
            logger.info("set g_vod_srv_address: " + g_vod_srv_address)
            vodEngine.setVodServerAddress(
                        g_vod_srv_address,
                        kvalUiSettingsManager.value('misc', 'boxName'))
        }
        var cacheDurationValue = kvalUiSettingsManager.value("vod", 'cacheduration')
        var bufferingValue = kvalUiSettingsManager.value("vod", 'precache')
        var streamVod = kvalUiSettingsManager.value("vod", 'stream')
        if(streamVod.indexOf("Activer") !== -1)
        {
            streamVod = true;
        }
        else
        {
            streamVod = false;
        }

        if (   ( g_cacheDurationValue !== Number(cacheDurationValue[0]) )
            || ( g_bufferingValue !== Number(bufferingValue[0]) )
            || ( g_streamVod !== streamVod ) )
        {
            g_streamVod = streamVod
            g_bufferingValue = Number(bufferingValue[0])
            g_cacheDurationValue = Number(cacheDurationValue[0])
            logger.info("set g_bufferingValue: " + g_bufferingValue)
            logger.info("set g_cacheDurationValue: " + g_cacheDurationValue)
            logger.info("set g_streamVod: " + g_streamVod)
            vodEngine.setVodSettings(g_bufferingValue,
                                     g_cacheDurationValue,
                                     g_streamVod)
        }
        vodCurrentNavigationLevel = 1;
        vodCategoryModel.query = "/categories/category";
        isReloadInProgress = true
        vodCategoryModel.reload();
        xmlVodQuickSettingModel.reload()
        startAnimationBackGround();
        g_vodInFlag = true
    }

    function stopVodMainView()
    {
        if(vodQuickSettingList.enabled) return;
        reInitSerieNavStatus();
        vodCurrentPageNbr = 0;
        clearPageHistory();
        headerCategoryName = "";
        headerSubCategoryName = "";
        headerMediaListName = "";
        updateHeaderCategoryName();
        hideVodMainView();
        activateMainScreenView();
        vodBackgroundImage2.source = ""
        vodBackgroundImage1.source = ""
        vodBackgroundImage0.source = ""
    }

    function checkArVodActivated()
    {
        var arTok =
        kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_AR_VOD_TOK)
        if(headerVodStr.indexOf(arTok) !== -1)
        {
            return true;
        }
        return false;
    }
    function checkFavoriteVodActivated()
    {
        if(headerVodStr.indexOf("Favoris") !== -1)
        {return true;
        }
        return false;
    }

    function unHideVodMainView() {
        opacity = 1;
        activeFocusViewManager(vodMainView)
    }

    function showVodMainView()
    {
        registerActiveView(vodMainView)
        vodBackgroundRect.opacity = 1;
        vodViewScaleAnimationOn.start()
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
        activeFocusViewManager(vodMainView)
        opacity = 1
    }

    function hideVodMainView() {
        opacity = 0;
        focus = false;
        enabled = false;
        vodBackgroundRect.opacity = 0;
        stopAnimationBackGround()
        focusRelinquished()
    }

    function startAnimationBackGround()
    {
        backgroundAnimEnabled = true;
        vodBackgroundImage1.opacity = 1
        vodBackgroundImage1.source =
                BACKGROUNG_DB.getRandomBackgroundImage("movie");
        vodBackgroundAnim2.start();
    }

    function stopAnimationBackGround()
    {
        backgroundAnimEnabled = false;
        if(vodBackgroundAnim1.running === true)
        {vodBackgroundAnim1.stop()
        }
        else if(vodBackgroundAnim2.running === true)
        {vodBackgroundAnim2.stop()
        }
    }

    function storeBackGroundImage(image)
    {
        if(headerCategoryName.indexOf("Séries") !== -1)
        {BACKGROUNG_DB.storeBackgroundImage("serie", image);
        }
        else if(headerCategoryName.indexOf("Mangas") !== -1)
        {BACKGROUNG_DB.storeBackgroundImage("mangas", image);
        }
        else
        {BACKGROUNG_DB.storeBackgroundImage("movie", image);
        }
    }

    function updateVodItemList()
    {
        if(!enabled) return;
        if(vodItemList.currentIndex < 0) vodItemList.currentIndex = 0;
        vodCategoryList.opacity = 0;
        vodCategoryList.enabled = false;
        vodCategoryList.focus = false;
        if(serieNavCounter)
        {
            vodItemList.positionViewAtIndex(seriesIndexHistory,
                                            ListView.Center)
            vodItemList.currentIndex = seriesIndexHistory;
            seriesIndexHistory = 0;
        }
        else
        {vodItemList.positionViewAtBeginning();
        }
        vodItemList.opacity = 1;
        vodItemList.enabled = true;
        vodItemList.focus = true;
        updateHeaderCategoryName();
    }

    function updateVodCategoryList()
    {
        if(!enabled) return;
        isReloadInProgress = false
        if(vodCategoryList.currentIndex < 0) vodCategoryList.currentIndex = 0;
        vodCategoryList.opacity = 1;
        vodCategoryList.enabled = true;
        vodCategoryList.focus = true;
        vodItemList.opacity = 0;
        vodItemList.enabled = false;
        vodItemList.focus = false;
        infoZoneMetaDataRect.opacity = 0;
        infoZoneSummaryRect.opacity = 0;
        vodViewFetchInfoCover(true);
        updateHeaderCategoryName();
        if(g_vodInFlag)
        {
            logger.info("vodMenuReadyNotify")
            g_vodInFlag = false
            vodMenuReadyNotify()
        }
    }

    function fetchItemsFailed()
    {
        closePopUpNotification();
        startAnimationBackGround();
    }

    function readyItemInfo()
    {
        pyNotificationView.clearview();
        pyNotificationView.updateProgressBar(0);
        opacity = 0;
        enabled = false;
        focus = false;
        vodLaunchView.vodLaunchUpdateHeader(headerVod.text)
        vodLaunchView.startLaunchView();
    }

    function getVodMainCurrentIndex()
    {
        return vodItemList.currentIndex;
    }

    function displayItemList(itemDisplayList)
    {
        startAnimationBackGround();
        if(!itemDisplayList.length)
        {
            //Restore opacity
            opacity = 1;
            reInitSerieNavStatus();
            pyNotificationView.stopProgressBar()
            vodViewRedKeyPressed()
            return
        }

        closePopUpNotification();
        vodItemModel.clear();
        var listPixelSize = marginBlock * 4.3
        var listYShift = marginBlock * 0.8
        if(checkArVodActivated())
        {
            listPixelSize = marginBlock * 4
            listYShift = marginBlock * 0.5
        }
        for(var i = 0; i < itemDisplayList.length ; i++)
        {
            if(itemDisplayList[i] === "Page suivante...")
            {
                vodItemModel.insert(i, {'vodName': itemDisplayList[i],
                                        'vodColor': "#007dfd",
                                        'vodIcon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_NEXT_BLUE),
                                        'vodPixelSize' : listPixelSize ,
                                        'vodYShift' : listYShift});
            }
            else if(itemDisplayList[i] === "Page précédente...")
            {
                vodItemModel.insert(i, {'vodName': itemDisplayList[i],
                                        'vodColor': "#007dfd",
                                        'vodIcon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PREVIOUS_BLUE),
                                        'vodPixelSize' : listPixelSize ,
                                        'vodYShift' : listYShift});
            }
            else if(serieNavCounter != 3 && isSeriesFlag)
            {
                vodItemModel.insert(i, {'vodName': itemDisplayList[i],
                                        'vodColor': "white",
                                        'vodIcon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MOVIE_FOLDER_ICON),
                                        'vodPixelSize' : listPixelSize ,
                                        'vodYShift' : listYShift});
            }
            else
            {
                vodItemModel.insert(i, {'vodName': itemDisplayList[i],
                                        'vodColor': "white",
                                        'vodIcon' : kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VIDEO_2_ICON),
                                        'vodPixelSize' : listPixelSize ,
                                        'vodYShift' : listYShift});
            }
        }
        updateVodItemList();
        vodViewFetchInfoCover(false);
        nextPropChanged = false
        previousPropChanged = false;
        vodChangeNextPreviousProp();
    }

    function setNavHistoryValues(navigationLevel, categoryIndex,
                                 subCatIndex, mediaListIndex)
    {
        if(navigationLevel) vodCurrentNavigationLevel = navigationLevel;
        if(categoryIndex) vodCurrentCatIndex = categoryIndex;
        if(subCatIndex) vodCurrentSubCatIndex = subCatIndex;
        if(mediaListIndex) vodCurrentMediaIndex = mediaListIndex;
    }

    function generateItemList(url, extraArg)
    {
        stopAnimationBackGround();
        generateItemListNotify(url, extraArg);
        popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_DATA));
    }
    
    function popUpNotification(str)
    {
        opacity = 0.4;
        pyNotificationView.requestProgressDisplay(
            kvalUiConfigManager.retranslate + qsTr("Vod Notification"),
            str, 1);
    }

    function closePopUpNotification()
    {
        pyNotificationView.clearview();
        pyNotificationView.updateProgressBar(0);
        opacity = 1;
    }

    function updateNotifProgressValue(value)
    {
        pyNotificationView.updateProgressBar(value)
    }

    function keyBoardCallBack(entry)
    {
        activeFocusViewManager(vodMainView)
        opacity = 1;
        startAnimationBackGround()
        if(!entry.length)
        {
            logger.info("Empty typing")
            return
        }
        var vodName= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodName;
        var vodNameCast= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodNameCast;
        var vodUrl= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodUrl;
        var contentType = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).type;
        vodUrl = vodUrl + "&data=" + entry

        if( contentType === "series" || contentType === "mangas")
        {storeSeriesHistory(vodUrl, "")
        }
        generateItemList(vodUrl, "");
        headerMediaListName = " • " + vodNameCast;
    }

    function searchForMedia(contentType)
    {
        opacity = 0.4;
        enabled = false
        focus = false
        stopAnimationBackGround()
        if( contentType === "films")
        {virtualKeyBoard.showVirtualKeyBoard(vodMainView,
                                             "Rechercher un film ( par titre, acteur, réalisateur )",
                                             "",
                                             IPTV_Constants.ENUM_TEXT_CLEAR);
        }
        else if(contentType === "series")
        {virtualKeyBoard.showVirtualKeyBoard(vodMainView,
                                             "Rechercher une série",
                                             "",
                                             IPTV_Constants.ENUM_TEXT_CLEAR);
        }
        else if(contentType === "mangas")
        {virtualKeyBoard.showVirtualKeyBoard(vodMainView,
                                             "Rechercher un manga",
                                             "",
                                             IPTV_Constants.ENUM_TEXT_CLEAR);
        }
    }

    function reloadCategoryList()
    {
        var navIndex = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodIndex;
        var subContent = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).subContent;
        var contentType = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).type;
        g_contentType = contentType
        var vodName= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodName;
        var vodNameCast= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodNameCast;
        var vodUrl= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodUrl;
        if( contentType === "series" || contentType === "mangas")
        {
            isSeriesFlag = true;
            serieNavCounter = 1;
        }
        generateItemList(vodUrl, "");
        headerMediaListName = " • " + vodNameCast;
    }
    
    function vodCategoryNavigationTree(navIn, navOut)
    {
        var navIndex = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodIndex;
        var subContent = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).subContent;
        var contentType = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).type;
        g_contentType = contentType
        var vodName= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodName;
        var vodNameCast= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodNameCast;
        var vodUrl= xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).vodUrl;
        if(navIn)
        {
            switch (vodCurrentNavigationLevel)
            {
                case IPTV_Constants.ENUM_VOD_ROOT_CATEGORY_LEVEL:
                    vodCategoryModel.query =
                    "/categories/category[" +navIndex+"]/subcategory";
                    setNavHistoryValues( IPTV_Constants.ENUM_VOD_SUBCATEGORY_LEVEL,
                                        navIndex,0, 0)
                    headerCategoryName = " • " + vodName;
                    break;
                case IPTV_Constants.ENUM_VOD_SUBCATEGORY_LEVEL:
                    if(subContent === 'empty')
                    {
                        if( contentType === "series" || contentType === "mangas")
                        {storeSeriesHistory(vodUrl, "")
                        }
                        generateItemList(vodUrl, "");
                        headerMediaListName = " • " + vodNameCast;
                        return
                    }
                    else if(subContent === 'search')
                    {
                        //Call keyboard
                        searchForMedia(contentType)
                        return
                    }
                    else
                    {
                        headerSubCategoryName = " • " + vodNameCast;
                        vodMediaListUrl = vodUrl;
                        vodCategoryModel.query =
                        "/categories/category["+vodCurrentCatIndex+"]/subcategory[" 
                        +navIndex+"]/mediaList";
                        setNavHistoryValues( IPTV_Constants.ENUM_VOD_MEDIALIST_LEVEL,
                                            0, navIndex, 0)
                    }
                    break;
                case IPTV_Constants.ENUM_VOD_MEDIALIST_LEVEL:
                    if( contentType === "series" || contentType === "mangas")
                    {storeSeriesHistory(vodMediaListUrl, vodName)
                    }
                    headerMediaListName = " • " + vodName;
                    generateItemList(vodMediaListUrl, vodName);
                    return;
                default:
                    return;
            }
        }
        else if(navOut)
        {
            switch (vodCurrentNavigationLevel)
            {
                case IPTV_Constants.ENUM_VOD_MEDIALIST_LEVEL:
                    vodCategoryModel.query = "/categories/category[" +vodCurrentCatIndex+"]/subcategory";
                    setNavHistoryValues( IPTV_Constants.ENUM_VOD_SUBCATEGORY_LEVEL, 0 , 1, 0)
                    headerSubCategoryName = "";
                    headerMediaListName = "";
                    break;
                case IPTV_Constants.ENUM_VOD_SUBCATEGORY_LEVEL:
                    vodCategoryModel.query = "/categories/category";
                    setNavHistoryValues( IPTV_Constants.ENUM_VOD_ROOT_CATEGORY_LEVEL, 1 , 1, 1)
                    headerCategoryName = "";
                    break;
                default:
                    return;
            }
        }
        updateHeaderCategoryName();
        isReloadInProgress = true
        vodCategoryModel.reload();
    }

    function storeSeriesHistory(url, extraArg)
    {
        isSeriesFlag = true;
        serieNavCounter = serieNavCounter + 1;
    }

    function reInitSerieNavStatus()
    {
        seriesIndexHistory = 0;
        serieNavCounter = 0;
        itemIndexHistory=0;
        itemSeasonIndexHistory = 0;
        isSeriesFlag = false;
    }

    function updateHeaderCategoryName()
    {
        headerVodStr = headerCategoryName +
                        headerSubCategoryName +
                        headerMediaListName;
    }

    function getCurrentSelectedName()
    {
        return vodItemModel.get(vodItemList.currentIndex).vodName;
    }
    function getCurrentSelectedUri()
    {
        return vodEngine.getUri(vodItemList.currentIndex)
    }

    function vodViewFetchInfoCover(isDir)
    {
        if(isDir)
        {
            vodDisplayCoverReady(xmlVodCategoryModel.get
                                 (vodCategoryList.currentIndex).vodIcon)
            infoZoneTitle.text = xmlVodCategoryModel.get
                                (vodCategoryList.currentIndex).vodName + ' ' +
                                xmlVodCategoryModel.get
                                (vodCategoryList.currentIndex).vodNameCast;
        }
        else 
        {
            infoZoneTitle.text = vodItemModel.get
                            (vodItemList.currentIndex).vodName;
            if(infoZoneTitle.text === "Page suivante...")
            {
                vodDisplayMetaData("");
                vodDisplayCoverReady(kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_NEXT));
            }
            else if(infoZoneTitle.text === "Page précédente...")
            {
                vodDisplayMetaData("");
                vodDisplayCoverReady(kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PREVIOUS));
            }
            else
            {
                vodEngine.getItemInfo(vodItemList.currentIndex);
            }
        }
    }

    function vodDisplayMetaData(metaData)
    {
        if(metaData.length !== 8)
        {
            infoZoneMetaDataRect.opacity = 0;
            infoZoneSummaryRect.opacity = 0;
            return;
        }
        infoZoneMetaDataRect.opacity = 1;
        infoZoneSummaryRect.opacity = 1;

        if(metaData[1] !== "")
        {
            infoZoneDirectorRect.visible = true
            metaDataDirectorText.text =
                               metaData[1].replace(/<(?:.|\n)*?>/gm, '');
        }
        else {
            infoZoneDirectorRect.visible = false
            metaDataDirectorText.text = "..."
        }

        if(metaData[0] !== "") {
            infoZoneYearRect.visible = true
            metaDataYearText.text =
                               metaData[0].replace(/<(?:.|\n)*?>/gm, '');
        }
        else {
            infoZoneYearRect.visible = false
            metaDataYearText.text = "..."
        }

        if(metaData[5] !== "") {
            infoZoneCountryRect.visible = true
            metaDataCountryText.text =
                               metaData[5].replace(/<(?:.|\n)*?>/gm, '');
        }
        else {
            infoZoneCountryRect.visible = false
            metaDataCountryText.text = "...";
        }
        
        if(metaData[3] !== "") {
            infoZoneGenreRect.visible = true
            metaDataGenresText.text =
                               metaData[3].replace(/<(?:.|\n)*?>/gm, '');
        }
        else {
            infoZoneGenreRect.visible = false
            metaDataGenresText.text = "..."
        }

        if(metaData[4] !== "") {
            infoZoneRunTimeRect.visible = true
            metaDataRunTimeText.text = " "+
                               metaData[4].replace(/<(?:.|\n)*?>/gm, '')+" min";
        }
        else{
            infoZoneRunTimeRect.visible = false
            metaDataRunTimeText.text = "..."
        }

        if(metaData[2] !== ""){
            infoZoneCastRect.visible = true
            metaDataCastText.text =
                               metaData[2].replace(/<(?:.|\n)*?>/gm, '');
        }
        else{
            infoZoneCastRect.visible = false
            metaDataCastText.text = "..."
        }

        if(metaData[7] !== ""){
            infoZonePlotRect.visible = true
            metaDataPlotText.text =
                               metaData[7].replace(/<(?:.|\n)*?>/gm, '');
        }
        else{
            infoZonePlotRect.visible = false
            metaDataPlotText.text = "..."
        }
    }

    function vodDisplayCoverReady(imageSrc)
    {
        if(imageSrc !== "")
        {
            if( vodCategoryList.enabled ||
                imageSrc === kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_NEXT) ||
                imageSrc === kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PREVIOUS))
            {
                infoZoneImageRect.opacity = 0
                infoZoneCategoryImageRect.opacity = 1
                infoZoneCategoryImg.source = imageSrc
                return
            }
            infoZoneImageRect.opacity = 1
            infoZoneCategoryImageRect.opacity = 0
            if(!alternateAnimation)
            {
                if(imageSrc === infoZoneImage2.source.toLocaleString()) return
                infoZoneImgItem1.y = 0
                infoZoneImgItem1.width = Utils.scaled(275)
                infoZoneImgItem1.height= Utils.scaled(374)
                infoZoneImage1.fillMode = Image.Stretch;
                infoZoneImage1.source = imageSrc;

                if( checkArVodActivated() && g_contentType === "series")
                {
                    infoZoneImgItem1.width = Utils.scaled(280)
                    infoZoneImgItem1.height= Utils.scaled(159)
                    infoZoneImgItem1.y = marginBlock
                    infoZoneImage1.fillMode = Image.Stretch;
                }
                imageTrans1.start();
                alternateAnimation = 1;
            }
            else
            {
                if(imageSrc === infoZoneImage1.source.toLocaleString()) return
                infoZoneImgItem2.y = 0
                infoZoneImgItem2.width = Utils.scaled(275)
                infoZoneImgItem2.height= Utils.scaled(374)
                infoZoneImage2.fillMode = Image.Stretch;
                infoZoneImage2.source = imageSrc
                if(checkArVodActivated() && g_contentType === "series")
                {
                    infoZoneImgItem2.width = Utils.scaled(280)
                    infoZoneImgItem2.height= Utils.scaled(159)
                    infoZoneImgItem2.y = marginBlock
                    infoZoneImage2.fillMode = Image.Stretch;
                }
                imageTrans2.start();
                alternateAnimation = 0;
            }
        }
    }

    function vodChangeNextPreviousProp()
    {
        if(vodItemModel.get(vodItemList.currentIndex).vodName === 
                                                        "Page suivante...")
        {
            nextPropChanged = true;
            nextPropIndex = vodItemList.currentIndex;
            vodItemModel.setProperty(vodItemList.currentIndex, 
                                    "vodColor",
                                    "white");
            vodItemModel.setProperty(vodItemList.currentIndex, 
                                    "vodIcon",
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_NEXT_SMALL));
            return;
        }
        else if(vodItemModel.get(vodItemList.currentIndex).vodName ===
                                                        "Page précédente...")
        {
            previousPropChanged = true;
            previousPropIndex = vodItemList.currentIndex;
            vodItemModel.setProperty(vodItemList.currentIndex, 
                                    "vodColor",
                                    "white");
            vodItemModel.setProperty(vodItemList.currentIndex, 
                                    "vodIcon",
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PREVIOUS_SMALL));
            return;
        }

        if(nextPropChanged)
        {
            nextPropChanged = false;
            vodItemModel.setProperty(nextPropIndex, 
                                    "vodColor",
                                    "#007dfd");
            vodItemModel.setProperty(nextPropIndex, 
                                    "vodIcon",
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_NEXT_BLUE));
        }
        if(previousPropChanged)
        {
            previousPropChanged = false;
            vodItemModel.setProperty(previousPropIndex, 
                                    "vodColor",
                                    "#007dfd");
            vodItemModel.setProperty(previousPropIndex, 
                                    "vodIcon",
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_PREVIOUS_BLUE));
        }
    }

    function vodViewItemOnSelect()
    {
        var contentType = xmlVodCategoryModel.get
                (vodCategoryList.currentIndex).type;
        if( vodItemModel.get(vodItemList.currentIndex).vodName === "Page suivante...")
        {
            vodCurrentPageNbr = vodCurrentPageNbr + 1;
            stopAnimationBackGround();
            generateItemsPage(vodItemList.currentIndex, vodCurrentPageNbr);
            popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_DATA));
        }
        else if(vodItemModel.get(vodItemList.currentIndex).vodName === "Page précédente...")
        {
            vodCurrentPageNbr = vodCurrentPageNbr - 1;
            stopAnimationBackGround();
            generateItemsPage(vodItemList.currentIndex, vodCurrentPageNbr);
            popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_DATA));
        }
        else if(isSeriesFlag)
        {
            if(serieNavCounter == 3)
            {
                logger.info("Launch Episode ...")
                stopAnimationBackGround();
                vodLaunchView.setActiveIndex(vodItemList.currentIndex);
                popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_START));
                generateDetailedItemInfos(vodItemList.currentIndex, true);
                return;
            }
            else if(serieNavCounter == 1)
            {
                generateSerieItems(vodItemList.currentIndex, true);
                itemIndexHistory = vodItemList.currentIndex;
                stopAnimationBackGround();
                popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_DATA));
            }
            else if(serieNavCounter == 2)
            {
                if(contentType === 'mangas')
                {
                    logger.info("Launch Episode ...")
                    stopAnimationBackGround();
                    vodLaunchView.setActiveIndex(vodItemList.currentIndex);
                    popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_START));
                    generateDetailedItemInfos(vodItemList.currentIndex, true);
                    return;
                }
                var seasonIndex = "1";
                var seasonName = vodItemModel.get
                                (vodItemList.currentIndex).vodName
                if(seasonName.indexOf("Saison") !== -1)
                {
                    seasonIndex = seasonName.substring(seasonName.indexOf
                                                       ("Saison")+7,
                                                       seasonName.length);
                }
                generateSeasonItems(vodItemList.currentIndex, seasonIndex, 
                                    true);
                itemSeasonIndexHistory = vodItemList.currentIndex;
                stopAnimationBackGround();
                popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_DATA));
            }
            else
            {
                generateSerieItems(vodItemList.currentIndex, false);
                stopAnimationBackGround();
                popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_DATA));
            }
            serieNavCounter = serieNavCounter + 1;
        }
        else
        {
            logger.info("Launch Movie ...");
            stopAnimationBackGround();
            vodLaunchView.setActiveIndex(vodItemList.currentIndex);
            popUpNotification(kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING_START));
            generateDetailedItemInfos(vodItemList.currentIndex, false);
        }
    }

    function vodViewOkKeyPressed()
    {
        if(vodQuickSettingList.enabled)
        {vodViewQuickSettingOnSelect()
        }
        else if(vodCategoryList.enabled)
        {vodCategoryNavigationTree(true, false);
        }
        else
        {vodViewItemOnSelect();
        }
    }
    function vodViewUpPressed()
    {
        if(vodQuickSettingList.enabled)
        {
            vodQuickSettingList.decrementCurrentIndex();
        }
        else if(vodCategoryList.enabled)
        {
            vodCategoryList.decrementCurrentIndex();
            xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex, 
                                    "vodColor",
                                    "black");
            xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex, 
                                    "vodColor2",
                                    "black");
            if(vodCategoryList.currentIndex+1 === vodCategoryList.count)
            {
                xmlVodCategoryModel.setProperty(0, "vodColor", "white");
                xmlVodCategoryModel.setProperty(0, "vodColor2", "#6aa6cb");
            }
            else
            {
                xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex+1, 
                                        "vodColor",
                                        "white");
                xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex+1,
                                        "vodColor2",
                                        "#6aa6cb");
            }

            vodViewFetchInfoCover(true);
        }
        else if(vodItemList.enabled)
        {
            vodItemList.decrementCurrentIndex();
            if((vodItemList.currentIndex+1) % IPTV_Constants.ENUM_PAGE_STEP_SIZE === 0)
            {
                vodItemList.positionViewAtIndex(vodItemList.currentIndex,
                                                ListView.End)
            }
            vodViewFetchInfoCover(false);
            vodChangeNextPreviousProp();
        }
    }
    function vodViewDownPressed()
    {
        if(vodQuickSettingList.enabled)
        {
            vodQuickSettingList.incrementCurrentIndex();
        }
        else if(vodCategoryList.enabled)
        {
            vodCategoryList.incrementCurrentIndex();
            xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex, 
                                    "vodColor",
                                    "black");
            xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex, 
                                    "vodColor2",
                                    "black");
            if(vodCategoryList.currentIndex-1 < 0)
            {
                xmlVodCategoryModel.setProperty(vodCategoryList.count - 1, 
                                                "vodColor", "white");
                xmlVodCategoryModel.setProperty(vodCategoryList.count - 1, 
                                                "vodColor2", "#6aa6cb");
            }
            else
            {
                xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex-1, 
                                        "vodColor",
                                        "white");
                xmlVodCategoryModel.setProperty(vodCategoryList.currentIndex-1,
                                        "vodColor2",
                                        "#6aa6cb");
            }
            vodViewFetchInfoCover(true)
        }
        else if(vodItemList.enabled)
        {
            vodItemList.incrementCurrentIndex();
            if(vodItemList.currentIndex % IPTV_Constants.ENUM_PAGE_STEP_SIZE === 0)
            {
                vodItemList.positionViewAtIndex(vodItemList.currentIndex,
                                                ListView.Beginning)
            }
            vodViewFetchInfoCover(false);
            vodChangeNextPreviousProp();
        }
    }
    function vodViewRightPressed()
    {
        if(vodQuickSettingList.enabled)
        {return
        }
        else if(vodCategoryList.enabled)
        {
            if(vodCategoryList.currentIndex+IPTV_Constants.ENUM_PAGE_STEP_SIZE >=
                    vodCategoryList.count)
            {
                vodCategoryList.currentIndex = vodCategoryList.count-1;
                vodCategoryList.positionViewAtIndex(vodCategoryList.currentIndex,
                                                    ListView.End)
            }
            else
            {
                vodCategoryList.currentIndex =
                        vodCategoryList.currentIndex + IPTV_Constants.ENUM_PAGE_STEP_SIZE;
                vodCategoryList.positionViewAtIndex(vodCategoryList.currentIndex,
                                                    ListView.Beginning)
            }
            vodViewFetchInfoCover(true)
        }
        else if(vodItemList.enabled)
        {
            if(vodItemList.currentIndex+IPTV_Constants.ENUM_PAGE_STEP_SIZE
                    >= vodItemList.count)
            {
                vodItemList.currentIndex = vodItemList.count-1;
                vodItemList.positionViewAtIndex(vodItemList.currentIndex,
                                                ListView.Beginning)
            }
            else
            {
                vodItemList.currentIndex = vodItemList.currentIndex +
                                           ( IPTV_Constants.ENUM_PAGE_STEP_SIZE -
                                           (vodItemList.currentIndex % IPTV_Constants.ENUM_PAGE_STEP_SIZE))
                vodItemList.positionViewAtIndex(vodItemList.currentIndex,
                                                ListView.Beginning)
            }
            vodViewFetchInfoCover(false);
            vodChangeNextPreviousProp();
        }
    }
    function vodViewLeftPressed()
    {
        if(vodQuickSettingList.enabled)
        {return
        }
        else if(vodCategoryList.enabled)
        {
            if(vodCategoryList.currentIndex-IPTV_Constants.ENUM_PAGE_STEP_SIZE <= 0)
            {vodCategoryList.currentIndex = 0;
            }
            else
            {vodCategoryList.currentIndex = vodCategoryList.currentIndex -
                                            ((vodCategoryList.currentIndex % IPTV_Constants.ENUM_PAGE_STEP_SIZE) + 1);
            }
            vodCategoryList.positionViewAtIndex(vodCategoryList.currentIndex,
                                            ListView.Beginning)
            vodViewFetchInfoCover(true)
        }
        else if(vodItemList.enabled)
        {
            if(vodItemList.currentIndex - IPTV_Constants.ENUM_PAGE_STEP_SIZE <= 0)
            {return
            }
            else vodItemList.currentIndex = vodItemList.currentIndex -
                                            ((vodItemList.currentIndex % IPTV_Constants.ENUM_PAGE_STEP_SIZE) + 1);
            vodItemList.positionViewAtIndex(vodItemList.currentIndex,
                                            ListView.End)
            vodViewFetchInfoCover(false);
            vodChangeNextPreviousProp();
        }
    }
    function vodViewBackPressed()
    {
        if(vodQuickSettingList.enabled)
        {
            hideQuickSettingList()
        }
        else if(vodCategoryList.enabled)
        {vodCategoryNavigationTree(false, true)
        }
        else
        {
            if(isSeriesFlag)
            {
                serieNavCounter = serieNavCounter - 1;
                if(serieNavCounter == 1)
                {
                    seriesIndexHistory = itemIndexHistory;
                    generateSerieHistoryList(false);
                    return;
                }
                else if(serieNavCounter == 2)
                {
                    seriesIndexHistory = itemSeasonIndexHistory;
                    generateSerieHistoryList(true);
                    return;
                }
            }
            reInitSerieNavStatus();
            headerMediaListName = "";
            vodCurrentPageNbr = 0;
            clearPageHistory();
            updateVodCategoryList();
        }
    }
    function vodViewMenuKeyPressed()
    {
        displayQuickSettingList()
    }
    function vodViewRedKeyPressed()
    {
        if(vodQuickSettingList.enabled) return;
        reInitSerieNavStatus();
        vodCurrentPageNbr = 0;
        clearPageHistory();
        headerCategoryName = "";
        headerSubCategoryName = "";
        headerMediaListName = "";
        updateHeaderCategoryName();
        vodCategoryModel.query = "/categories/category";
        setNavHistoryValues( IPTV_Constants.ENUM_VOD_ROOT_CATEGORY_LEVEL, 1 , 1, 1)
        headerCategoryName = "";
        updateHeaderCategoryName();
        isReloadInProgress = true
        vodCategoryModel.reload();
    }
    function vodViewGreenKeyPressed()
    {
        if(vodQuickSettingList.enabled) return;
        reInitSerieNavStatus();
        vodCurrentPageNbr = 0;
        clearPageHistory();
        headerCategoryName = "";
        headerSubCategoryName = "";
        headerMediaListName = "";
        updateHeaderCategoryName();
        
        vodCategoryModel.query =
        "/categories/category["+2+"]/subcategory";
        setNavHistoryValues( IPTV_Constants.ENUM_VOD_SUBCATEGORY_LEVEL,
                            2,0, 0)
        headerCategoryName = "/ " + vodName;
        updateHeaderCategoryName();
        isReloadInProgress = true
        vodCategoryModel.reload();
    }
    function vodViewYellowKeyPressed()
    {
        if(vodQuickSettingList.enabled) return;
        reInitSerieNavStatus();
        vodCurrentPageNbr = 0;
        clearPageHistory();
        headerCategoryName = "";
        headerSubCategoryName = "";
        headerMediaListName = "";
        updateHeaderCategoryName();
        
        vodCategoryModel.query =
        "/categories/category["+3+"]/subcategory";
        setNavHistoryValues( IPTV_Constants.ENUM_VOD_SUBCATEGORY_LEVEL,
                            3,0, 0)
        headerCategoryName = "/ " + vodName;
        updateHeaderCategoryName();
        isReloadInProgress = true
        vodCategoryModel.reload();
    }
    function vodViewBlueKeyPressed()
    {
        if(vodQuickSettingList.enabled) return;
        reInitSerieNavStatus();
        vodCurrentPageNbr = 0;
        clearPageHistory();
        headerCategoryName = "";
        headerSubCategoryName = "";
        headerMediaListName = "";
        updateHeaderCategoryName();
        
        vodCategoryModel.query =
        "/categories/category["+6+"]/subcategory";
        setNavHistoryValues( IPTV_Constants.ENUM_VOD_SUBCATEGORY_LEVEL,
                            6 ,0, 0)
        headerCategoryName = "/ " + vodName;
        updateHeaderCategoryName();
        isReloadInProgress = true
        vodCategoryModel.reload();
    }

    Keys.onPressed: {
        if(pyNotificationView.checkActiveProgress() || isReloadInProgress)
        {
            logger.info("isReloadInProgress true")
            event.accepted = true;
            return;
        }
        else if (g_vodFileMissing)
        {
            logger.info("Vod File missing")
            if(event.key === Qt.Key_H)
            {stopVodMainView();
            }
            else {}
            event.accepted = true;
            return
        }

        if (event.key === Qt.Key_Up) //Up Button
        {vodViewUpPressed();
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {vodViewDownPressed();
        }
        else if (event.key === Qt.Key_Right) //Right Button
        {vodViewRightPressed()
        }
        else if (event.key === Qt.Key_Left) //Left Button
        {vodViewLeftPressed()
        }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {vodViewBackPressed();
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {vodViewOkKeyPressed();
        }
        else if (event.key === Qt.Key_Escape)
        {stopVodMainView();
        }
        else if(event.key === Qt.Key_R)
        {vodViewRedKeyPressed();
        }
        else if(event.key === Qt.Key_G)
        {vodViewGreenKeyPressed();
        }
        else if(event.key === Qt.Key_Y)
        {vodViewYellowKeyPressed();
        }
        else if(event.key === Qt.Key_B)
        {vodViewBlueKeyPressed();
        }
        else if(event.key === Qt.Key_M)
        {vodViewMenuKeyPressed();
        }
        else if(event.key === Qt.Key_H)
        {stopVodMainView();
        }
        else if (event.key === Qt.Key_Return)
        {
        }
        else {}
        event.accepted = true;
    }

    Rectangle {
        id: vodBackgroundRect
        width: rootRectangle.width
        height: rootRectangle.height
        x: rootRectangle.x;
        y:rootRectangle.y
        color: "transparent"
        Image {
            id: vodBackgroundImage0
            width: parent.width
            height: parent.height
            fillMode: Image.PreserveAspectFit
            source: ""
        }

        Image {
            id: vodBackgroundImage1
            width: parent.width
            height: parent.height
            fillMode: Image.PreserveAspectFit
            NumberAnimation
            {
                id: vodBackgroundAnim1
                target: vodBackgroundImage1;
                property: "opacity";
                from: 0; to: 1; duration: 6000
                onStarted:
                {
                    if(backgroundAnimEnabled)
                    {
                        if(headerCategoryName.indexOf("Séries") !== -1)
                        {vodBackgroundImage1.source = BACKGROUNG_DB.getRandomBackgroundImage("serie");
                        }
                        else if(headerCategoryName.indexOf("Mangas") !== -1)
                        {vodBackgroundImage1.source = BACKGROUNG_DB.getRandomBackgroundImage("mangas");
                        }
                        else
                        {vodBackgroundImage1.source = BACKGROUNG_DB.getRandomBackgroundImage("movie");
                        }
                    }
                }
                onStopped:
                {
                    if(backgroundAnimEnabled)
                    {
                        vodBackgroundImage0.source = vodBackgroundImage1.source;
                        vodBackgroundImage1.opacity = 0;
                        vodBackgroundAnim2.start()
                    }
                }
            }
        }
        Image {
            id: vodBackgroundImage2
            width: parent.width
            height: parent.height
            fillMode: Image.PreserveAspectFit
            NumberAnimation 
            {
                id: vodBackgroundAnim2
                target: vodBackgroundImage2;
                property: "opacity";
                from: 0; to: 1; duration: 6000
                onStarted:
                {
                    if(backgroundAnimEnabled)
                    {
                        if(headerCategoryName.indexOf("Séries") !== -1)
                        {vodBackgroundImage2.source = BACKGROUNG_DB.getRandomBackgroundImage("serie");
                        }
                        else if(headerCategoryName.indexOf("Mangas") !== -1)
                        {vodBackgroundImage2.source = BACKGROUNG_DB.getRandomBackgroundImage("mangas");
                        }
                        else
                        {vodBackgroundImage2.source = BACKGROUNG_DB.getRandomBackgroundImage("movie");
                        }
                    }
                }
                onStopped:
                {
                    if(backgroundAnimEnabled)
                    {
                        vodBackgroundImage0.source = vodBackgroundImage2.source;
                        vodBackgroundImage2.opacity = 0;
                        vodBackgroundAnim1.start()
                    }
                }
            }
        }
    }
    Image {
        width: rootRectangle.width
        height: rootRectangle.height
        x: rootRectangle.x;
        y:rootRectangle.y
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND_2)
        opacity: 0.7
    }
    Image {
        width: rootRectangle.width * 0.45 + 115.13
        height: rootRectangle.height * 0.9 + 132.28
        x: rootRectangle.x - 17.565
        y:rootRectangle.y - 26.14
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Rectangle {
        id: backGroundRect
        //50% of screen width and 87% of screen height
        width: rootRectangle.width * 0.45
        height: rootRectangle.height * 0.9
        x: rootRectangle.x + 40;
        y:rootRectangle.y + 40
        color: "black"
        opacity: 0.5
        radius:5
        clip:true
        Image {
            width:parent.width - 2
            height:parent.height - 2
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_VIGNETTE_FULL_ROUNDED)
            opacity: 1
        }
    }

    Rectangle {
        id: listRect

        width: backGroundRect.width
        height: backGroundRect.height
        x: backGroundRect.x; y: backGroundRect.y
        color: "black"
        opacity: 0
    }

    //ListView Rect Zone
    Rectangle {
        id: mainScrollZone

        width: listRect.width
        height: listRect.height * 0.8
        anchors.centerIn: listRect
        color: "transparent";
        opacity: 0
    }

    //Header Zone
    Rectangle {
        id: mainHeaderZone

        x: listRect.x+marginBlock
        y: listRect.y
        width: listRect.width - marginBlock - marginBlock/2
        height: listRect.height * 0.1
        color: "transparent"
        Image{
            id: backIconAnim
            width: Utils.scaled(40)
            height: Utils.scaled(40)
            anchors.left: parent.left
            anchors.leftMargin: marginBlock*0.5
            anchors.top: parent.top
            anchors.topMargin: marginBlock*2+marginBlock*0.5
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INDICATOR_BACK)
            opacity: 0.7
        }
        Text {
            id: headerVod
            width: parent.width - backIconAnim.width
            x: marginBlock + marginBlock*0.5 + backIconAnim.width
            elide:Text.ElideMiddle
            text: "VOD" + headerVodStr;
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -marginBlock*0.5
            font {
                family: localFont.name;
                pixelSize: marginBlock * 4.4
            }
            color: "white"
        }
    }

    //Bottom Zone
    Rectangle {
        id: bottomZone
        anchors.top: mainScrollZone.bottom
        anchors.left: mainScrollZone.left
        width: listRect.width
        height: listRect.height * 0.1
        color: "transparent"
        Image{
            x: -(marginBlock + marginBlock*0.5)
            id: bottomZoneKeyRed
            width: Utils.scaled(45)
            anchors.bottom: parent.bottom
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_RED_BUTTON)
            Text{
                anchors.left: parent.right
                anchors.leftMargin: marginBlock*0.5
                text: kvalUiConfigManager.retranslate + qsTr("Main")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: marginBlock * 3.5
                }
            }
        }
        Image{
            id: bottomZoneKeyGreen
            width: Utils.scaled(45)
            anchors.bottom: parent.bottom
            anchors.left: bottomZoneKeyRed.right
            anchors.leftMargin: (bottomZone.width*0.1428)
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_GREEN_BUTTON)
            Text{
                anchors.left: parent.right
                anchors.leftMargin: marginBlock*0.5
                text: kvalUiConfigManager.retranslate + qsTr("Films")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: marginBlock * 3.5
                }
            }
        }
        Image{
            id: bottomZoneKeyYellow
            width: Utils.scaled(45)
            anchors.left: bottomZoneKeyGreen.right
            anchors.leftMargin: (bottomZone.width*0.1428)
            anchors.bottom: parent.bottom
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_YELLOW_BUTTON)
            Text{
                anchors.left: parent.right
                anchors.leftMargin: marginBlock*0.5
                text: kvalUiConfigManager.retranslate + qsTr("Séries")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: marginBlock * 3.5
                }
            }
        }
        Image{
            id: bottomZoneKeyBlue
            width: Utils.scaled(45)
            anchors.left: bottomZoneKeyYellow.right
            anchors.leftMargin: (bottomZone.width*0.1428)
            anchors.bottom: parent.bottom
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_BLUE_BUTTON)
            Text{
                anchors.left: parent.right
                anchors.leftMargin: marginBlock*0.5
                text: kvalUiConfigManager.retranslate + qsTr("Mangas")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: marginBlock * 3.5
                }
            }
        }
        Rectangle {
            id: menuIcoRect
            anchors.right : parent.right
            anchors.rightMargin: marginBlock*2
            anchors.bottom : parent.bottom
            height: Utils.scaled(55)
            width : Utils.scaled(111)
            color: "transparent"
            Image{
                height: parent.height
                width: parent.width
                anchors.bottom: parent.bottom
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_MENU_BUTTON)
            }
        }
        Glow {
            id: menuIcoGlow
            anchors.fill: menuIcoRect
            radius: 16
            opacity: 0
            samples: 24
            spread: 0.7
            color: "white"
            transparentBorder: true
            source: menuIcoRect
        }
    }

    Image {
        width: mainPiconZone.width + 127.38
        height: backGroundRect.height - 55 + 125.34
        x: backGroundRect.width + backGroundRect.x - 23.69
        y:rootRectangle.y - 22.67
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }

    //Epg Update Zone
    Rectangle {
        id: mainPiconZone

        x: backGroundRect.width + backGroundRect.x + 40
        y: backGroundRect.y
        width: (rootRectangle.width -
                backGroundRect.width -
                backGroundRect.x - marginBlock*4) - 40

        height: backGroundRect.height - 55
        color: "black"
        opacity: 0.5
        radius:5
        Image {
            width:parent.width - 2
            height:parent.height - 2
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_VIGNETTE_FULL_ROUNDED)
            opacity: 1
        }
    }

    Rectangle {
        id: vofInfoGatherZone

        Rectangle {
            id: infoZoneHeader
    
            x: mainPiconZone.x
            y: mainPiconZone.y
            width: mainPiconZone.width
            height: mainHeaderZone.height
            color: "transparent"
            Text {
                text: g_currentTimeStdFormat
                anchors.right: parent.right
                anchors.rightMargin: marginBlock * 2
                anchors.verticalCenter: parent.verticalCenter
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: marginBlock * 6;
                }
            }
            Text {
                text: g_currentDateVodFormat
                anchors.right: parent.right
                anchors.rightMargin: marginBlock * 18
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                color: "white"
                opacity: 0.8
                font {
                    family: localFont.name;
                    pixelSize: marginBlock * 2.1;
                }
            }
        }
    
        Rectangle {
            id: infoZoneTitleRect
            anchors.top: infoZoneHeader.bottom
            anchors.left: infoZoneHeader.left
            anchors.leftMargin: (marginBlock * 3) + marginBlock/2
            anchors.right: infoZoneHeader.right
            height: marginBlock * 6
            color: "transparent"
            Text {
                id: infoZoneTitle
                width: parent.width - (marginBlock*2)
                height: parent.height
                text: ""
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                color: "white"
                opacity: 1
                textFormat: Text.RichText
                font {
                    family: localFont.name;
                    pixelSize: marginBlock * 5;
                }
            }
        }
        Rectangle {
            id: infoZoneCategoryImageRect
            x: mainPiconZone.x + (mainPiconZone.x * 0.05) - marginBlock
            y: mainScrollZone.y + marginBlock * 8
            width: Utils.scaled(275)
            height: Utils.scaled(374)
            color: "transparent"
            Image{
                id: infoZoneCategoryImg
                width: Utils.scaled(275)
                height: Utils.scaled(374)
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
            }
        }
        Rectangle {
            id: infoZoneImageRect
            x: mainPiconZone.x + (mainPiconZone.x * 0.05) - marginBlock
            y: mainScrollZone.y + marginBlock * 8
            width: Utils.scaled(275)
            height: Utils.scaled(374)
            color: "transparent"
            Image{
                width: infoZoneImgItem1.width + Utils.scaled(35)
                height: infoZoneImgItem1.height + Utils.scaled(32)
                anchors.centerIn: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.POSTER_FRAME)
            }
            Image {
                id: vodCoverRotationSpinner
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BUSY_SPINNER)
                RotationAnimation on rotation
                {
                    id: vodCoverRotationSpinnerAnim
                    running: false
                    loops: Animation.Infinite
                    direction: RotationAnimation.Clockwise
                    from: 0
                    to: 360
                }
            }
            Item {
                id: infoZoneImgItem1
                width: Utils.scaled(275)
                height: Utils.scaled(374)
                anchors.centerIn: parent
                Image {
                    id: infoZoneImage1
                    width: parent.width
                    height: parent.height
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    visible: false
                    onStatusChanged:
                    {
                        if (infoZoneImage1.status === Image.Error)
                        {
                            source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_COVER_IMAGE)
                            vodCoverRotationSpinnerAnim.stop()
                            vodCoverRotationSpinner.opacity = 0
                        }
                        else if(infoZoneImage1.status === Image.Ready)
                        {
                            vodCoverRotationSpinnerAnim.stop()
                            vodCoverRotationSpinner.opacity = 0
                        }
                    }
                }
                Image {
                    id: infoZoneImage1mask
                    width: parent.width
                    height: parent.height
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.POSTER_FRAME_MASK)
                    visible: false
                }
                OpacityMask {
                    id:opacityMskCover
                    anchors.fill: infoZoneImage1
                    source: infoZoneImage1
                    maskSource: infoZoneImage1mask
                }
                NumberAnimation
                {
                    id: imageTrans1
                    target: infoZoneImgItem1
                    property: "opacity";
                    from: 0; to: 1; duration: 100
                    onStarted: {
                        vodCoverRotationSpinnerAnim.start()
                        vodCoverRotationSpinner.opacity = 1
                        infoZoneImgItem2.opacity = 0;
                    }
                }
            }
            Item {
                id: infoZoneImgItem2
                width: Utils.scaled(275)
                height: Utils.scaled(374)
                anchors.centerIn: parent
                Image{
                    id: infoZoneImage2
                    width: parent.width
                    height: parent.height
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    visible: false
                    onStatusChanged: 
                    {
                        if (infoZoneImage2.status === Image.Error)
                        {
                            source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_COVER_IMAGE)
                            vodCoverRotationSpinnerAnim.stop()
                            vodCoverRotationSpinner.opacity = 0
                        }
                        else if(infoZoneImage1.status === Image.Ready)
                        {
                            vodCoverRotationSpinnerAnim.stop()
                            vodCoverRotationSpinner.opacity = 0
                        }
                    }
                }
                Image {
                    id: infoZoneImage2mask
                    width: parent.width
                    height: parent.height
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.POSTER_FRAME_MASK)
                    visible: false
                }
                OpacityMask {
                    id:opacityMskCover2
                    anchors.fill: infoZoneImage2
                    source: infoZoneImage2
                    maskSource: infoZoneImage1mask
                }
                NumberAnimation
                {
                    id: imageTrans2
                    target: infoZoneImgItem2
                    property: "opacity";
                    from: 0; to: 1; duration: 100
                    onStarted: {
                        vodCoverRotationSpinnerAnim.start()
                        vodCoverRotationSpinner.opacity = 1
                        infoZoneImgItem1.opacity = 0;
                    }
                }
            }
        }
        Rectangle {
            id: infoZoneMetaDataRect
    
            anchors.left: infoZoneImageRect.right
            anchors.top: infoZoneImageRect.top
            anchors.bottom: infoZoneImageRect.bottom
            width: mainPiconZone.width - (infoZoneImageRect.width + marginBlock*6)
            color: "transparent"
            opacity: 0
    
            Rectangle {
                id: infoZoneDirectorRect
                anchors.right: parent.right
                anchors.left: parent.left
                anchors.top: parent.top
                height: parent.height / 4
                color: "transparent"
                Text {
                    width: parent.width
                    height: parent.height/2
                    x: parent.x + (marginBlock*2)
                    y: parent.y
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1" //#007dfd
                    text: kvalUiConfigManager.retranslate + qsTr("Réalisateur:")
                    font {
                      bold: true
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
                Text {
                    id: metaDataDirectorText
                    width: parent.width
                    height: parent.height/2
                    x: parent.x + (marginBlock*2)
                    y: parent.y + parent.height/2
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: ""
                    font {
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
            }
            Rectangle {
                id: infoZoneYearRect

                anchors.left: parent.left
                anchors.top: infoZoneDirectorRect.bottom
                anchors.topMargin: marginBlock * 1.9
                width: parent.width/2
                height: parent.height / 4
                color: "transparent"
                Text {
                    id: yearTitle
                    x: parent.x + (marginBlock*2)
                    width: parent.width
                    height: parent.height/2
    
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1" //#007dfd
                    text: kvalUiConfigManager.retranslate + qsTr("Année:")
                    font {
                        bold: true
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
                Text {
                    id: metaDataYearText
                    width: parent.width
                    height: parent.height/2
                    x: parent.x + (marginBlock*2)
                    y: yearTitle.y + yearTitle.contentHeight
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: ""
                    font {
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
            }
            Rectangle {
                id: infoZoneCountryRect

                anchors.top: infoZoneYearRect.top
                anchors.bottom: infoZoneYearRect.bottom
                anchors.left: infoZoneYearRect.right
                width: parent.width/2
                color: "transparent"
                Text {
                    id: countryTitle
                    width: parent.width
                    height: parent.height/2
    
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1" //#007dfd
                    text: kvalUiConfigManager.retranslate + qsTr("Pays:")
                    font {
                        bold: true
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
                Text {
                    id: metaDataCountryText
                    width: parent.width
                    height: parent.height/2
                    y: countryTitle.y + countryTitle.contentHeight
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: ""
                    font {
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
            }
            Rectangle {
                id: infoZoneGenreRect
                anchors.left: parent.left
                anchors.top: infoZoneYearRect.bottom
                anchors.topMargin: marginBlock * 1.9
                width: parent.width
                height: parent.height / 4
                color: "transparent"
                Text {
                    id: genreTitle
                    width: parent.width
                    height: parent.height/2
                    x: parent.x + (marginBlock*2)
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1" //#007dfd
                    text: kvalUiConfigManager.retranslate + qsTr("Genre:")
                    font {
                        bold: true
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
                Text {
                    id: metaDataGenresText
                    width: parent.width - marginBlock
                    height: parent.height/2
                    x: parent.x + (marginBlock*2)
                    y: genreTitle.y + genreTitle.contentHeight
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: ""
                    font {
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
            }
            Rectangle {
                id: infoZoneRunTimeRect

                anchors.left: parent.left
                anchors.top: infoZoneGenreRect.bottom
                anchors.topMargin: marginBlock * 1.9
                width: parent.width
                height: parent.height / 4
                color: "transparent"
                Text {
                    id: durationTitle
                    width: parent.width
                    height: parent.height/2
                    x: parent.x + (marginBlock*2)
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1"
                    text: kvalUiConfigManager.retranslate + qsTr("Durée:")
                    font {
                        bold: true
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
                Text {
                    id: metaDataRunTimeText
                    width: parent.width
                    height: parent.height/2
                    x: parent.x + (marginBlock*2) + durationTitle.contentWidth
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: ""
                    font {
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
            }
        }
    
        Rectangle {
            id: infoZoneSeparatorRect
    
            anchors.left: infoZoneImageRect.left
            anchors.top: infoZoneImageRect.bottom
            anchors.topMargin: marginBlock*3
            height: marginBlock + marginBlock/3
            width: mainPiconZone.width *0.80
            color: "transparent"
            opacity: 1
            Image {
                anchors.fill: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_SEPARATOR_FADED)
                opacity:1
            }
        }
    
        Rectangle {
            id: infoZoneSummaryRect
    
            anchors.left: infoZoneSeparatorRect.left
            anchors.top: infoZoneSeparatorRect.bottom
            anchors.topMargin: marginBlock
            anchors.right: infoZoneSeparatorRect.right
            anchors.rightMargin: -marginBlock*7
            height: mainPiconZone.height * 0.35
            color: "transparent"
            opacity: 0
            Rectangle {
                id: infoZoneCastRect
    
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.right: parent.right
                height: parent.height * 0.2
                color: "transparent"
                opacity: 1
                Text {
                    id: castTitle
                    width: parent.width
                    height: parent.height/2
                    x: parent.x
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1"
                    text: kvalUiConfigManager.retranslate + qsTr("Cast:")
                    font {
                        bold: true
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
                Text {
                    id: metaDataCastText
    
                    anchors.left: parent.left
                    anchors.leftMargin: castTitle.contentWidth + marginBlock
                    width: parent.width - castTitle.contentWidth - marginBlock
                    height: parent.height/2
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: ""
                    font {
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
            }
            Rectangle {
                id: infoZonePlotRect
    
                anchors.left: parent.left
                anchors.top: infoZoneCastRect.bottom
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                color: "transparent"
                opacity: 1
                Text {
                    id: plotTitle
                    width: parent.width
                    height: parent.height
                    x: parent.x
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1"
                    text: kvalUiConfigManager.retranslate + qsTr("Synopsis et détails:")
                    font {
                        bold: true
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
                Text {
                    id: metaDataPlotText
                    anchors.top : parent.top
                    anchors.topMargin: plotTitle.contentHeight
                    anchors.left: parent.left
                    width: parent.width
                    height: parent.height - plotTitle.contentHeight
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignJustify
                    elide: Text.ElideRight
                    color: "white"
                    text: ""
                    font {
                      family: localFont.name;
                      pixelSize: marginBlock * 3.2
                    }
                }
            }
        }
    }
    Item {
        id: vodEmptyMsgZone
        width: mainScrollZone.width * 0.8
        height: mainScrollZone.height * 0.8
        anchors.centerIn: mainScrollZone
        visible: false
        Text {
            id: headEmptyZoneTxt
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            wrapMode: Text.WordWrap
            style: Text.Raised
            styleColor: "black"
            text: ""
            color: 'white'
            clip : true
            font {
                family: localAdventFont.name;
                pixelSize: marginBlock * 4.2
            }
        }
        Text {
            id: bodyEmptyZoneTxt
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            wrapMode: Text.WordWrap
            style: Text.Raised
            color: "white"
            styleColor: "black"
            text: ""
            clip : true
            font {
                family: localAdventFont.name;
                pixelSize: marginBlock * 4.2
            }
        }
        Text {
            id: footerEmptyZoneTxt
            width: parent.width
            height: parent.height
            anchors.top: bodyEmptyZoneTxt.bottom
            anchors.left: bodyEmptyZoneTxt.left
            wrapMode: Text.WordWrap
            style: Text.Raised
            color: "#6aa6cb"
            styleColor: "black"
            text: ""
            clip : true
            font {
                family: localAdventFont.name;
                pixelSize: marginBlock * 3
            }
        }
    }
    //Category ListView
    Component {
            id : mainVodListComponent
            Row {
              Column {
                  Rectangle {
                      id: indexRect
                      x: mainScrollZone.x - (marginBlock*2)
                      width: mainScrollZone.width/14
                      height: mainScrollZone.height/ categoryNbrElementsPages
                      color: "transparent"
                      Image {
                          width: parent.width * 0.80
                          height: parent.height * 0.90
                          y: parent.y
                          anchors.right: parent.right;
                          anchors.rightMargin: marginBlock
                          fillMode: Image.Stretch
                          source: vodPrefixIcon
                          opacity: categoryIconOpacity
                        }
                    }
                }
                Column {
                    Rectangle {
                        x: indexRect.x + categoryTextXmargin
                        width: (mainScrollZone.width) - (indexRect.width)
                        height: mainScrollZone.height/15
                        color: "transparent"
                        Text {
                            id: mainCategoryName
                            width: parent.width - (marginBlock * 8)
                            height: parent.height
                            y: parent.y - 8
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: vodColor
                            text: vodName
                            font {
                              family: localFont.name;
                              pixelSize: marginBlock * 4.3
                            }
                        }
                        Text {
                            x: mainCategoryName.contentWidth + marginBlock
                            y: mainCategoryName.y + marginBlock * 1.4
                            width: parent.width -
                                   (marginBlock * 8) -
                                   mainCategoryName.contentWidth
                            height: parent.height
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: vodColor2
                            text: (vodNameCast !== "") ? "• " + vodNameCast : ""
                            font {
                              family: localAdventFont.name;
                              pixelSize: marginBlock * 4.3
                            }
                        }
                        Text {
                            opacity: categorySubTextOpacity
                            anchors.left: mainCategoryName.left
                            anchors.top: mainCategoryName.bottom
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: vodColor2
                            text: vodSubName
                            font {
                              family: localBoldFont.name;
                              pixelSize: marginBlock * 2.7
                            }
                        }
                    }
                }
            }
        }
    ListModel {
         id: xmlVodCategoryModel
         //Template to Use
         ListElement {
             vodPrefixIcon: ""
             vodName: ""
             vodNameCast: ""
             vodSubName: ""
             vodIndex: ""
             vodIcon: ""
             type: ""
             vodUrl: ""
             subContent: ""
             vodColor: ""
             vodColor2: ""
         }
     }
    XmlListModel {
        id: vodCategoryModel
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_VOD_FILE)
        query: "/categories/category"

        XmlRole {
         name: "vodName";
         query: "@title/string()"
        }
        XmlRole {
         name: "vodNameCast";
         query: "@titleCast/string()"
        }
        XmlRole {
         name: "vodSubName";
         query: "@subtitle/string()"
        }
        XmlRole {
         name: "vodIndex";
         query: "@index/string()"
        }
        XmlRole {
         name: "vodIcon";
         query: "@icon/string()"
        }
        XmlRole {
         name: "type";
         query: "@type/string()"
        }
        XmlRole {
         name: "vodUrl";
         query: "@url/string()"
        }
        XmlRole {
         name: "subContent";
         query: "@content/string()"
        }
        onStatusChanged:
        {
            if (status == XmlListModel.Ready)
            {
                g_vodFileMissing = false
                vodEmptyMsgZone.visible = false
                infoZoneImageRect.visible = true
                if(vodCategoryModel.query === "/categories/category")
                {
                    categoryNbrElementsPages = 8.3
                    categoryIconOpacity = 0
                    categorySubTextOpacity = 1
                    categoryTextXmargin = -(marginBlock*4)
                    vodCategoryList.highlight = highlightMainCategory
                }
                else
                {
                    categoryNbrElementsPages = 15
                    categoryIconOpacity = 1
                    categorySubTextOpacity = 0
                    categoryTextXmargin = +(marginBlock/2)
                    vodCategoryList.highlight = highlightMainSubCategory
                }
                xmlVodCategoryModel.clear()
                for(var i = 0; i < vodCategoryModel.count ; i++)
                {
                        xmlVodCategoryModel.insert(i, 
                        {
                        'vodPrefixIcon': kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MOVIE_FOLDER_ICON),
                        'vodName': kvalUiConfigManager.xmlGetStr(vodCategoryModel.get(i).vodName),
                        'vodNameCast': kvalUiConfigManager.xmlGetStr(vodCategoryModel.get(i).vodNameCast),
                        'vodSubName': kvalUiConfigManager.xmlGetStr(vodCategoryModel.get(i).vodSubName),
                        'vodIndex': vodCategoryModel.get(i).vodIndex,
                        'vodIcon': 'file://' + vodCategoryModel.get(i).vodIcon,
                        'type' : vodCategoryModel.get(i).type,
                        'vodUrl' : vodCategoryModel.get(i).vodUrl,
                        'subContent' : vodCategoryModel.get(i).subContent,
                        'vodColor' : i ? "white" : "black",
                        'vodColor2' : i ? "#6aa6cb" : "black" });
                }
                updateVodCategoryList();
            }
            else if(status == XmlListModel.Error)
            {
                infoZoneImageRect.visible = false
                vodEmptyMsgZone.visible = true
                var errorStr = vodCategoryModel.errorString()
                logger.info("Error reading File: "+ errorStr)
                if ( errorStr.indexOf('No such file or directory') !== -1)
                {
                    headEmptyZoneTxt.text = kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MISSING_VOD_FILE)
                    bodyEmptyZoneTxt.text = '\n\n'+
                            kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MISSING_VOD_HINT)
                    footerEmptyZoneTxt.text =
                            kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_PRESS_HOME_BUTTON_STR)
                    logger.info("File not found")
                    g_vodFileMissing = true
                }
                else
                {logger.info("Set error ")
                }
                if(g_vodInFlag)
                {
                    logger.info("vodMenuReadyNotify")
                    g_vodInFlag = false
                    vodMenuReadyNotify()
                }
                isReloadInProgress = false
            }
        }
    }

    ListView {
        id: vodCategoryList
        anchors.left: mainScrollZone.left
        anchors.right: mainScrollZone.right
        anchors.top: mainScrollZone.top
        anchors.bottom: mainScrollZone.bottom
        enabled: false
        opacity: 0
        focus: false

        model: xmlVodCategoryModel
        highlight: highlightMainCategory

        Rectangle {
            color: 'white'
            anchors.top: vodCategoryList.top
            anchors.horizontalCenter:scrollInternal.horizontalCenter
            opacity: 0.1
            width: 1
            height: (parent.contentHeight > parent.height) ? parent.height : 0
        }
        ScrollBar.vertical: ScrollBar {
            id: scrollInternal
            active: true
            anchors.top: vodCategoryList.top
            anchors.right: vodCategoryList.right
            anchors.rightMargin: marginBlock*2
            anchors.bottom: vodCategoryList.bottom
        }

        highlightFollowsCurrentItem: false
        clip: true
        keyNavigationWraps: true
        interactive: false
        delegate: mainVodListComponent
    }
    Component {
        id: highlightMainSubCategory

        Rectangle {
            id: highlightFavRect
            x: vodCategoryList.currentItem.x
            y: vodCategoryList.currentItem.y
            width: mainScrollZone.width - (marginBlock*4)
            height: 50
            color: "transparent"
            Image {
                x: parent.x
                y: parent.y
                width: mainScrollZone.width - (marginBlock*2)
                anchors.top: parent.top
                anchors.topMargin: -marginBlock - marginBlock/3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -marginBlock - marginBlock/3
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_LIST_FOCUS)
                opacity: 1
                layer.enabled: true
                layer.effect: ColorOverlay
                {
                    color: highlightColor
                }
            }
        }
    }
    Component {
        id: highlightMainCategory

        Rectangle {
            id: highlightFavRect
            x: vodCategoryList.currentItem.x
            y: vodCategoryList.currentItem.y - marginBlock*2
            width: mainScrollZone.width - (marginBlock*4)
            height: 120
            color: "transparent"
            Image {
                x: parent.x
                y: parent.y
                width: mainScrollZone.width - (marginBlock*2)
                anchors.top: parent.top
                anchors.topMargin: -marginBlock - marginBlock/3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -marginBlock - marginBlock/3
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_LIST_FOCUS)
                opacity: 1
            }
        }
    }
    //Item ListView
    Component {
            id : mainVodItemListComponent
            Row {
              Column {
                  Rectangle {
                      id: indexItemRect
                      x: mainScrollZone.x - (marginBlock*2)
                      width: mainScrollZone.width/14
                      height: mainScrollZone.height/15
                      color: "transparent"
                      Image {
                          width: parent.width * 0.80
                          height: parent.height * 0.90
                          y: parent.y
                          anchors.right: parent.right;
                          anchors.rightMargin: marginBlock
                          fillMode: Image.Stretch
                          source: vodIcon
                          opacity:0.6
                        }
                    }
                }
                Column {
                    Rectangle {
                        x: indexItemRect.x + (marginBlock/2)
                        width: (mainScrollZone.width) - (indexItemRect.width)
                        height: mainScrollZone.height/15
                        color: "transparent"
                        Text {
                            id: selectedItem
                            width: parent.width - (marginBlock * 8)
                            height: parent.height
                            y: parent.y - vodYShift
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: vodColor
                            text: vodName
                            textFormat: Text.RichText
                            font {
                              family: localFont.name;
                              pixelSize: vodPixelSize
                            }
                        }
                    }
                }
            }
        }

    ListModel {
         id: vodItemModel
         //Template to Use
         ListElement {
             vodName: ""
             vodIcon: ""
             vodColor: ""
             vodYShift: 8
             vodPixelSize: 43
         }
     }

    ListView {
        id: vodItemList
        anchors.left: mainScrollZone.left
        anchors.right: mainScrollZone.right
        anchors.top: mainScrollZone.top
        anchors.bottom: mainScrollZone.bottom
        enabled: false
        opacity: 0
        focus: false

        model: vodItemModel
        highlight: highlightItem

        Rectangle {
            color: 'white'
            anchors.top: vodItemList.top
            anchors.horizontalCenter:scrollItemInternal.horizontalCenter
            opacity: 0.1
            width: 1
            height: (parent.contentHeight > parent.height) ? parent.height : 0
        }
        ScrollBar.vertical: ScrollBar {
            id: scrollItemInternal
            active: true
            anchors.top: vodItemList.top
            anchors.right: vodItemList.right
            anchors.rightMargin: marginBlock*2
            anchors.bottom: vodItemList.bottom
        }

        highlightFollowsCurrentItem: false
        clip: true
        keyNavigationWraps: true
        interactive: false
        delegate: mainVodItemListComponent
    }

    Component {
        id: highlightItem

        Rectangle {
            id: highlightRect
            x: vodItemList.currentItem.x
            y: vodItemList.currentItem.y
            width: mainScrollZone.width - (marginBlock*5)
            height: 50
            color: "transparent"
            Image {
                x: parent.x
                y: parent.y
                width: mainScrollZone.width - (marginBlock*5)
                anchors.top: parent.top
                anchors.topMargin: -marginBlock - marginBlock/3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -marginBlock- marginBlock/3
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_LIST_FOCUS)
                opacity: 1
                layer.enabled: true
                layer.effect: ColorOverlay
                {
                    color: highlightColor
                }
            }
        }
    }
    //---

    //Quick VOD settings list
    function favItemDeleted()
    {reloadCategoryList()
    }

    function vodViewQuickSettingOnSelect()
    {
        switch (vodQuickSettingList.currentIndex)
        {
            case IPTV_Constants.ENUM_VOD_QUICK_SET_ADD_FAV:
                logger.info("Add prog to favorite")
                if(checkArVodActivated())
                {return;
                }
                if(vodItemList.enabled && !checkFavoriteVodActivated())
                {addMediaToFavorite(vodItemList.currentIndex)
                }
                break;
            case IPTV_Constants.ENUM_VOD_QUICK_SET_CLEAR_CACHE:
                logger.info("Clear cache ...")
                break;
            case IPTV_Constants.ENUM_VOD_QUICK_SET_DEL_SELECTED_FAV:
                if(checkArVodActivated())
                {return;
                }
                if(vodItemList.enabled && checkFavoriteVodActivated())
                {
                    logger.info("delete selected Fav")
                    if(!vodItemList.count)
                    {
                        logger.error("Nothing to delete !!")
                        return
                    }
                    deleteSelectedFav(vodItemList.currentIndex)
                }
                break;
            case IPTV_Constants.ENUM_VOD_QUICK_SET_CLEAR_FAV:
                logger.info("Clear Fav lists ...")
                break;
            case IPTV_Constants.ENUM_VOD_QUICK_SET_MAIN_VOD_SETTINGS:
                logger.info("Go to main vod settings")
                break;
            default:
                return;
        }
    }

    function displayQuickSettingList()
    {
        menuIcoGlow.opacity = 1
        if(vodQuickSettingList.currentIndex < 0) 
            vodQuickSettingList.currentIndex = 0;
        vofInfoGatherZone.opacity = 0.2
        mainPiconZone.opacity = 0.2
        quickSettingRect.opacity = 0.7
        vodQuickSettingList.opacity = 1;
        vodQuickSettingList.enabled = true;
        vodQuickSettingList.focus = true;
    }

    function hideQuickSettingList()
    {
        menuIcoGlow.opacity = 0
        vofInfoGatherZone.opacity = 1
        mainPiconZone.opacity = 0.5
        quickSettingRect.opacity = 0
        vodQuickSettingList.opacity = 0;
        vodQuickSettingList.enabled = false;
        vodQuickSettingList.focus = false;
        if(vodCategoryList.enabled)
        {
            vodCategoryList.enabled = true
            vodCategoryList.focus = true
        }
        else
        {
            vodItemList.enabled = true
            vodItemList.focus = true
        }
    }

    Rectangle {
        id: quickSettingRect
        width: backGroundRect.width*0.5
        height: backGroundRect.height * 0.3
        anchors.bottom: backGroundRect.bottom
        anchors.left: mainPiconZone.left
        anchors.leftMargin: -marginBlock*2
        color: "black"
        opacity: 0
        radius: 8
        border.color: "grey"
        border.width: 1
    }
    Rectangle {
        id: quickSettingListZoneRect
        width: quickSettingRect.width
        height: quickSettingRect.height * 0.9
        anchors.centerIn: quickSettingRect
        color: "transparent"
    }

    ListModel {
         id: quickSettingModel
         //Template to Use
         ListElement {
             settingName: ""
             settingIndex: ""
         }
     }

    XmlListModel {
        id: xmlVodQuickSettingModel
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_VOD_QUICK_SETTING_FILE)
        query: "/quick_settings/vod/setting"

        XmlRole {
         name: "settingName";
         query: "@title/string()"
        }
        XmlRole {
         name: "settingIndex";
         query: "@index/string()"
        }
        onStatusChanged:
        {
            if (status == XmlListModel.Ready)
            {
                quickSettingModel.clear()
                for(var i = 0; i < xmlVodQuickSettingModel.count ; i++)
                {
                    quickSettingModel.insert(i,
                    {
                    'settingName':
                     kvalUiConfigManager.xmlGetStr(xmlVodQuickSettingModel.get(i).settingName),
                    'settingIndex':
                     kvalUiConfigManager.xmlGetStr(xmlVodQuickSettingModel.get(i).settingIndex)
                    });
                }
                logger.info("xmlVodQuickSettingModel update Done")
                vodQuickSettingList.currentIndex = 0
            }
        }
    }

    ListView {
        id: vodQuickSettingList
        anchors.left: quickSettingListZoneRect.left
        anchors.right: quickSettingListZoneRect.right
        anchors.top: quickSettingListZoneRect.top
        anchors.bottom: quickSettingListZoneRect.bottom
        enabled: false
        opacity: 0
        focus: false

        model: quickSettingModel
        highlight: highlightSettingItem
        highlightFollowsCurrentItem: false
        clip: true
        keyNavigationWraps: true
        interactive: false
        delegate: quickSettingListComponent
    }

    Component {
        id: highlightSettingItem

        Rectangle {
            id: highlightSettingRect
            x: vodQuickSettingList.currentItem.x
            y: vodQuickSettingList.currentItem.y
            width: quickSettingRect.width
            height: 45
            color: highlightColor
        }
    }

    Component {
            id : quickSettingListComponent
            Row {
                Column {
                    Rectangle {
                        width: quickSettingListZoneRect.width
                        height: quickSettingListZoneRect.height/5
                        color: "transparent"
                        Text {
                            width: parent.width
                            height: parent.height
                            y: parent.y
                            horizontalAlignment: Text.AlignHCenter
                            color: "white"
                            text: settingName
                            font {
                              family: localFont.name;
                              pixelSize: marginBlock * 2.7
                            }
                        }
                    }
                }
            }
        }
}
