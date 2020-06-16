import QtQuick 2.14
import QtQuick.Window 2.1
import QtGraphicalEffects 1.0
import kval.gui.qml 1.0

import "KvalUI_Utils.js" as Utils
import "KvalUI_constant.js" as IPTV_Constants
import "mainscreenitems"
import "appstemplates"


Rectangle {
    id: rootRectangle
    //Main Rect
    color: "transparent"
    focus:  false
    enabled: false
//    scale: 0.6

    //Global properties
    FontLoader { id: localFont; source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_TEXT_FONT) }
    FontLoader { id: localFontLow; source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_TEXT_FONT_LOW) }
    FontLoader { id: localFontCaps; source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_TEXT_FONT_CAPS) }
    FontLoader { id: localBoldFont; source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_TEXT_BOLD_FONT) }
    FontLoader { id: localAdventFont; source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ADVENT_TEXT_FONT) }
    FontLoader { id: localMangasFont; source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MANGAS_TEXT_FONT) }
    FontLoader { id: localLiveTvFont; source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LIVE_TV_GALLERY_FONT) }

    property int marginBlock                : Utils.scaled(10);
    property color highlightColor           : "#149baf"; //"#dd314b"
    property color highlightColorGrad       : "#0B4C5F"; //"#8d1022"
    property color backColor                : "#444243";
    property color backColorGrad            : "#252525";

    property int currentNavLevel            : IPTV_Constants.ENUM_CATEGORIES_ROOT_LEVEL;
    property int activePlayingList          : IPTV_Constants.ENUM_ACTIVE_PLAYING_DEFAULT;
    property string g_currentTimeStdFormat  : Qt.formatTime(new Date(),"hh:mm")
    property string g_currentDateStdFormat  : Qt.formatDateTime(new Date(),
                                                               "ddd dd.MM.yyyy")
    property string g_currentDateVodFormat  : Qt.formatDateTime(new Date(),
                                                               "ddd" + 
                                                               "\n" + 
                                                               "dd, MMM")
    property bool g_userStopRequested       : false
    property bool g_guiInitialized          : false

    //Properties for testing purposes
    property int zapNumber                  : 0
    property bool testOn                    : false
    property var g_currentActiveView         : null

    Timer {
        id: timeUpdate
        interval: 30000
        repeat: true
        running: true
        onTriggered:
        {
            g_currentTimeStdFormat = Qt.formatTime(new Date(),"hh:mm")
            g_currentDateStdFormat = Qt.formatDateTime(new Date(), "ddd dd.MM.yyyy")
            g_currentDateVodFormat = Qt.formatDateTime(new Date(), "ddd" + 
                                                       "\n" + 
                                                       "dd, MMM")
        }
    }

    Timer {
        id: zapTest
        interval: 7000
        repeat: true
        running: true
        onTriggered:
        {
            if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL)
            {
                if(!testOn) return
                channelSelectionListView.exit_button_simulated();
                channelSelectionListView.channelProgNext();
                zapNumber++
                logger.info(">>>>>>>> zapNumber: "+zapNumber)
            }
        }
    }

    KvalUiConfigManager {
        id: kvalUiConfigManager
        Component.onCompleted:{
            logger.info("KvalUiConfigManager Ready.")
            if(!g_guiInitialized)
            {
                g_guiInitialized = true
                logger.info("UI intialized ...");
                restoreFocusToMain();
                mainScreenView.uiReady()
            }
        }
    }


    UI_PlayerElement {
        id: mediaPlayer
        onPlaybackStarted: onPlayBackStarted();
        onPlaybackFailed: onPlayBackFailed();
        onPlaybackCompleted: onPlayBackCompleted();
        onPlaybackInterrupted: onPlayBackInterrupted(interruptedPosition);
        onShowSubs: videoSubtitleSurface.showSubs(subsLine);
        onHideSubs: videoSubtitleSurface.hideSubs();
        onStreamPositionReady :
        {
            if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL ||
               currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL ||
               currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_MEDIA_BROWSER_LEVEL)
            {mediaPlayerView.streamPositionReady(position, length);
            }
        }
        onVideoSeeked :
        {
            if(streamInfoMetaData.enabled)
            {
                streamInfoMetaData.extractStreamBaseMetaData();
            }
            else
            {
                if( currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL ||
                    currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL ||
                    currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_MEDIA_BROWSER_LEVEL)
                {mediaPlayerView.streamSeeked();
                }
                else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL)
                {
                    channelSelectionListView.playerFlushed()
                }
            }
        }
    }

    KvalUI_SubsSurfaceView {
        id: videoSubtitleSurface
    }

    KvalUI_BootScreen {
        id: bootScreen
        onBootScreenReleased: {
            mainScreenView.activateMainScreenView()
            serverNotificationEngine.startClientDaemon()
        }
    }

    UI_SettingsElements {
        id: kvalUiSettingsManager
        onWifiApCurrentReady: {
            pyNotificationView.updateProgressBar(progressVal);
            settingsMainView.apUpdateCurrent(apInfos)
        }
        onWifiScanEnds: {
            pyNotificationView.requestAlertDisplay(
                        kvalUiConfigManager.retranslate + qsTr("Info"),
                        kvalUiConfigManager.retranslate + qsTr("Wireless Network Scan Complete."));
            settingsMainView.displayWlanApList()
        }
        onDisplayProgress: {
            pyNotificationView.updateStrProgressBar(msg, progressVal);
        }
        onDisplayMsg: {
            pyNotificationView.requestAlertDisplay(header, msg);
        }

        onYesNoDiag: popUpView.yesNoDiag(title, text, nolabel, yeslabel, settingsMainView)
        onYesNoDiagUpdate: popUpView.yesNoDiagUpdate(title,text);
        onYesNoDiagClose: popUpView.yesNoDiagClose();

        onEndSetSetting: {
            settingsMainView.endSetSetting()
            g_currentTimeStdFormat = Qt.formatTime(new Date(),"hh:mm")
            g_currentDateStdFormat = Qt.formatDateTime(new Date(), "ddd dd.MM.yyyy")
            g_currentDateVodFormat = Qt.formatDateTime(new Date(), "ddd" + 
                                                       "\n" + 
                                                       "dd, MMM")
        }
        onWifiPassReq: {
            settingsMainView.settingViewRequestWifiPass()
        }
        onLanguageChanged:{
            logger.info("onLanguageChanged.")
            appsEngine.langChanged();
            kvalUiConfigManager.langChanged();
            kvalUiGuiUtils.updateCustomsList();
            localMediaFirstTemplate.langChanged()
            settingsMainView.langChanged()
            channelSelectionListView.langChanged()
        }
    }

    UI_QmlElement {
        id: qmlBinder

        onEpgReady: epgGenerator.epgReady = true;
        onStartStream: showVideo(streamLink);
        onPlaybackFailed: onPlayBackFailed();
        onDisplayMsg: pyNotificationView.requestAlertDisplay(header, msg);
        onOkDiag: popUpView.okDiag(title, text);
        onYesNoDiag: popUpView.yesNoDiag(title, text, nolabel, yeslabel, channelSelectionListView);
        onYesNoDiagUpdate: popUpView.yesNoDiagUpdate(title,text);
        onYesNoDiagClose: popUpView.yesNoDiagClose();
        onHttpSeeked: channelSelectionListView.httpSeeked(pos)
        onFlushPlayer: channelSelectionListView.pausedForFlush()

        onCutomLiveTvParsed: mainScreenView.customInfosUpdated();
    }

    KvalUiLiveTvManager{
        id: kvalUiLivetvManager
        onQmlEpgReady: channelSelectionListView.epgDataReady(epgdata)
        onQmlEpgEnds: channelSelectionListView.epgDataEnds()
    }
    KvalUI_MainScreenView {
        id: mainScreenView

        onMainScreenReadyNotify: {
            bootScreen.onHomeReady()
        }

        onDisplayVodScreenNotifiy: vodMainView.showVodMainView()
        onDisplayMailBoxScreenNotify: mailBoxMainView.showMailBoxView()
        onDisplayMediaBrowserScreenNotify: mediaBrowserMainView.showMediaBrowserView()
        onDisplaySettingsScreenNotify: settingsMainView.showSettingsView()
        onDisplayAppsScreenNotify: appsView.showAppsScreen()

        SearchTemplate{
            id: searchFirstTemplate
        }
        VodTemplate {
            id: vodFirstTemplate
            onActivateVodViewNotify:
                vodMainView.activateVodMainView()
        }
        LiveTvTemplate {
            id: liveTvFirstTemplate
            onActivateLiveTvViewNotify: channelSelectionListView.wakeUp()
        }
        DevicesTemplate{
            id: localMediaFirstTemplate
            onMediaBrowserActivateNotify:
                mediaBrowserMainView.activateMediaBrowserView()
        }
        MessagesTemplate{
            id: mailBoxFirstTemplate
            onMailListExtractNotify:
                serverNotificationEngine.extractMailList()
        }
        SettingsTemplate{
            id: settingsFirstTemplate
            onSettingsActivateNotify:
                settingsMainView.activateSettingsView()
        }
        AppsTemplate{
            id: appsFirstTemplate
            onAppsExtractNotify: appsView.activateAppsScreen(true)
        }
    }

    KvalUI_AppsView {
        id: appsView
        onActivateMainScreenView: mainScreenView.activateMainScreenView()
        onAppsScreenReadyNotify: mainScreenView.appsScreenReady()
        onActivateVideoTemplateView: videoTemplate.activateVideoTemplateView()
        onAppsScreenLaunchApp: appsEngine.launchApp(appid, args)
        onRequesForAppAbort: appsEngine.requestAppAbort()
        onFocusRelinquished: restoreFocusToMain()
        onAppUninstallNotify: appsEngine.uninstallApp(appId)
        onAppUpdateNotify: appsEngine.updateApp(appId)
        onAppAddFavNotify: appsEngine.addAppToFav(appId)
        onAppRemoveFavNotify:appsEngine.removeAppFromFav(appId)
    }
    UI_AppsElement {
        id: appsEngine
        onEndOfCategory: if(g_currentActiveView.endOfCategory) g_currentActiveView.endOfCategory(succeeded)
        onEndOfSettingsEntries: if(g_currentActiveView.endOfSettingsEntries) g_currentActiveView.endOfSettingsEntries(succeeded)
        onAppHasStopped: if(g_currentActiveView.appHasStopped) g_currentActiveView.appHasStopped()
        onAppHasStarted: if(g_currentActiveView.appHasStarted) g_currentActiveView.appHasStarted()
        onCustomUiTemplateSig: if(g_currentActiveView.customUiNotify) g_currentActiveView.customUiNotify(uiTemplate)
        onRefresh: if(g_currentActiveView.refresh) g_currentActiveView.refresh();
        onUpdate: if(g_currentActiveView.update) g_currentActiveView.update(path, replace);
        onSetViewMode: if(g_currentActiveView.setViewMode) g_currentActiveView.setViewMode(mode, submode);
        onSetResUrl: if(g_currentActiveView.setResUrl) g_currentActiveView.setResUrl(itemInfo);
        onSystemReboot: {
            blurUi.source = g_currentActiveView
            blurUi.visible = true
            rebootItem.visible = true
        }

        onDisplayAppsMsgSig: pyNotificationView.requestAlertDisplay(header,body)
        onOkDiag: popUpView.okDiag(title, text)
        onYesNoDiag: popUpView.yesNoDiag(title, text, nolabel, yeslabel)
        onDiagProgressCreate: popUpView.progressDiag(title, text)
        onDiagProgressUpdate: popUpView.progressDiagUpdate(position, text)
        onDiagProgressClose: popUpView.progressDiagClose()
        onInputKeyboard: virtualKeyBoard.showVirtualKeyBoardFromApps(defaultTxt,"",
                                                                     (!hidden) ?
                                                                    IPTV_Constants.ENUM_TEXT_CLEAR :
                                                                    IPTV_Constants.ENUM_TEXT_HIDDEN);
        onInputList: popUpView.listChoiceMenu(title, items)
    }

    Loader {
        id: videoTemplate
        onLoaded: {
            logger.info("videoTemplate Successfully loaded UI : " + source)
            visible = false
        }
        Connections {
            target: videoTemplate.item

            onActiveFocus:
            {
                if(activeFocusViewManager(videoTemplate.item))
                {
                    videoTemplate.focus = true
                    videoTemplate.enabled = true
                }
                videoTemplate.visible = true
            }
            onActivateMainScreenView:
            {
                mainScreenView.activateMainScreenView()
                appsView.initAppScreen()
            }
            onActivateAppView:
            {
                appsView.activateAppsScreen(false)
            }
            onClearTemplate:
            {
                videoTemplate.sourceComponent = undefined
                kvalUiGuiUtils.trimCache()
            }

            onVideoTemplateLaunchApp: appsEngine.launchApp(appid, args)
            onNotifyBuiltinAction: appsEngine.notifyBuiltinAction(action)
        }
    }

    Loader {
        id: customUiAppTemplate
        onLoaded: {
            logger.info("customUiAppTemplate Successfully loaded UI : " + source)
            visible = false
        }
        onStatusChanged: {
            logger.info("status Changed: " + status)
            appsView.customUiStatusChanged(status);
        }

        Connections {
            target: customUiAppTemplate.item

            onActiveFocus:
            {
                logger.info("customUiAppTemplate onActiveFocus")
                if(activeFocusViewManager(customUiAppTemplate.item))
                {
                    customUiAppTemplate.focus = true
                    customUiAppTemplate.enabled = true
                }
                customUiAppTemplate.visible = true
            }
            onActivateMainScreenView:
            {
                mainScreenView.activateMainScreenView()
                appsView.initAppScreen()
            }
            onActivateAppView:
            {
                appsView.activateAppsScreen(false)
            }
            onClearTemplate:
            {
                customUiAppTemplate.sourceComponent = undefined
                appsEngine.clearCustomTemplate()
                kvalUiGuiUtils.trimCache()
            }
            onCustomTemplateLaunchApp: appsEngine.launchApp(appid, args)
            onNotifyBuiltinAction: appsEngine.notifyBuiltinAction(action)
        }
    }

    UI_SubsClientElement{
        id: serverNotificationEngine
        onNewNotificationMsg: {
            if (mainScreenView.enabled)
            {mainScreenView.displayServerNotificationMessage(severity, message)
            }
            else if(mailBoxMainView.enabled)
            {serverNotificationEngine.extractMailList()
            }
            else
            {serverNotificationCenter.displayServerNotificationMessage(severity,
                                                                        message)
            }
        }
        onNewFirmwareReceivedInProgress: {
            serverNotificationCenter.newFirmwareNotif()
        }
        onNewFirmwareReceivedStatus: {
            serverNotificationCenter.newFirmwareNotifStatus(status)
        }
        onFirmwareOK: {
            serverNotificationCenter.firmwareOK()
        }
        onMailBoxInfosRecv: {
            serverNotificationCenter.mailBoxInfosRecv(mailBoxInfos)
            mainScreenView.mainScreenMailBoxInfosNotify(mailBoxInfos)
        }
        onReadyMailItemList: {
            mailBoxMainView.fillMailBoxList(mailDisplayList)
        }
        onDisplayInternalSrvNotifSignal: {
            pyNotificationView.requestAlertDisplay(header, body)
        }
        onMailBoxEmptyNotify: {
            mailBoxMainView.onMailBoxEmpty()
        }
        onLiveTvFileAvailable : channelSelectionListView.newFileAvailable()
        onVodFileAvailable : vodMainView.newFileAvailable()
        onMainScreenVodFileAvailable: mainScreenView.newVodTitleAvailable()
        onEpgFileAvailable: channelSelectionListView.newEpgFileAvailable()
        onEpgFileUpdated: channelSelectionListView.epgFileUpdated()
    }
    UI_MediaBrowserElement{
        id: mediaBrowserElement
        onUsbUtilNotifyConnectedDevice :{
            mainScreenView.mainScreenUsbDeviceConnected()
        }
        onUsbUtilNotifyDisconnectedDevice :{
            mainScreenView.mainScreenUsbDeviceDisconnected()
            if(mediaBrowserMainView.enabled)
                mediaBrowserMainView.mediaBrowserBackToHomeView()
        }
        onNewFirmwareReceivedStatus: {
            serverNotificationCenter.newFirmwareUsbNotifStatus(status)
            serverNotificationEngine.notifyNewFirmwareOnUsbSupport(status, 
                                                                   filename)
        }
        onDisplayProgress: {
            pyNotificationView.updateStrProgressBar(msg, progressVal);
        }
        onDisplayMsg: {
            pyNotificationView.requestAlertDisplay(header, msg);
        }
    }

    UI_VodElement{
        id: vodEngine
        onReadyItemList :
        {vodMainView.displayItemList(itemDisplayList);
        }
        onExtractedItemLoadingValue:
        {
            vodMainView.updateNotifProgressValue(loadprogressValue);
        }
        onExtractedInfoReady:
        {
            vodMainView.vodDisplayMetaData(metaData);
            vodMainView.vodDisplayCoverReady(infoIcon);
        }
        onPbFetchingItems : vodMainView.fetchItemsFailed();
        onTrailerScriptEnd: vodLaunchView.fetchTrailerEnd();
        onDisplayNotifProgressSignal :
        {pyNotificationView.updateStrProgressBar(notifStr, progressValue);
        }
        onDisplayNotifSignal:
        {pyNotificationView.requestAlertDisplay(headerMsg, mainMsg)
        }
        onReadyItemInfo :
        {vodMainView.readyItemInfo();
        }
        onLaunchViewInfoReady :
        {
            vodLaunchView.vodLaunchMetaPaintDisplay(artDataInfo);
            vodLaunchView.vodLaunchMetaRightRectDisplay(rightDataInfo);
            vodLaunchView.vodLaunchMetaLeftRectDisplay(leftDataInfo);
        }
        onPlayableUriReady:
        {
            vodLaunchView.vodLaunchVideoUri(playableUri);
        }
        onTrailerListItemsReady :
        {vodLaunchView.vodLaunchTrailerListDisplay(trailerNames, trailerUrls);
        }
        onVideoListItemsReady:
        {vodLaunchView.vodLaunchVideoListDisplay(videoItems);
        }
        onDownloadProgressValue:
        {mediaPlayerView.setDownloadValue(value)
        }
        onFavItemDeleted:
        {vodMainView.favItemDeleted();
        }
        onGetCaptchaValueNotify:
        {vodLaunchView.needCaptchaValue(Uri);
        }
    }

    KvalUI_VodView {
        id: vodMainView
        onFocusRelinquished: parent.focus = true
        onGenerateItemListNotify: vodEngine.generateItems(url, extraArg);
        onGenerateItemsPage : vodEngine.generateItemPage(index, pageNbr);
        onClearPageHistory: vodEngine.clearPageHistory();
        onGenerateHistoryBackNotify:vodEngine.generateHistoryBack();
        onActivateMainScreenView: mainScreenView.activateMainScreenView()
        onDeactivateMainView: removeFocusfromMain();
        onGenerateDetailedItemInfos: vodEngine.generateItemInfo(index, isSerie)
        onGenerateSerieItems: vodEngine.generateSerieItems(index, contextList)
        onGenerateSeasonItems: vodEngine.generateSeasonItems(index, seasonNumber,
                                                             contextList)
        onGenerateSerieHistoryList: vodEngine.getSerieHistoryList(isSeason)
        onAddMediaToFavorite: vodEngine.addMediaToFav(index)
        onDeleteSelectedFav: vodEngine.deleteMediaFromFav(index)
        onVodMenuReadyNotify: mainScreenView.vodScreenReady()
    }
    
    KvalUI_VodLaunchView {
        id: vodLaunchView
        onLaunchTrailerNotify: vodEngine.launchTrailer(url)
        onGenerateTrailerItemList: vodEngine.generateTrailerList(index)
        onGenerateVideoItemList: vodEngine.generateVideoList(index)
        onDeactivateMainView: removeFocusfromMain();
        onFocusRelinquished: restoreFocusToMain();
        onSetCurrentMediaInPlayName: mediaPlayerView.setCurrentMediaInfo(name, plot)
        onLaunchVideoNotify: vodEngine.generateVideoUrl(index)
        onLaunchShahidVodUri: vodEngine.getShahidPlayableUri(index)
        onNotifyDeadLinks: serverNotificationEngine.notifyDeadLinks(name, uri)
        onAddMediaToFavorite: vodEngine.addMediaToFav(index)
    }

    KvalUI_MediaPlayerInfoView {
        id: mediaPlayerView
        onAskForStopNotify: {
            mediaPlayerView.reinitValues();
            stopCurrentMedia();
            if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL)
            {
                vodEngine.stopNotify()
                vodLaunchView.showLaunchView()
            }
            else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL)
            {
//                appsView.stopNotify()
            }
            else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_MEDIA_BROWSER_LEVEL)
            {mediaBrowserMainView.showMediaBrowserView()
            }
        }
    }

    KvalUI_ChannelSelectListView {
        id: channelSelectionListView

        onFocusRelinquished : restoreFocusToMain();
        onMessageHideLoadingPopUp : popUpView.messageLoadingPopUpHide();
        onMessageDisplayPopUp:
            popUpView.messagePopUpDisplay(severity, msgData);
        onMessageDisplayLoadingPopUp:
            popUpView.messageLoadingPopUpDisplay(severity, msgData);
        onSetChInfo: channelInfoView.setCurrentChInfo(chIndex, chName, chPicon);
    }

    KvalUI_channelInfoView {
        id: channelInfoView
        onFocusRelinquished: parent.focus = true
    }

    KvalUI_EpgGenerator {
        id: epgGenerator
    }

    Rectangle {
        id: backgroundProgressRect
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        color: "black"
        opacity: 0
    }

    KvalUI_StreamInfoMetaData {
        id: streamInfoMetaData

        onFocusRelinquished: {
            if( currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL ||
                currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_MEDIA_BROWSER_LEVEL ||
                currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL)
            {
                mediaPlayerView.restoreFocus()
            }
            else parent.focus = true;
        }
        onMessageDisplayPopUp:{
            popUpView.messagePopUpDisplay(severity, msgData);
        }
        onMessageHidePopUp : popUpView.messagePopUpHide();
        onMetaDataChangeNotify:
        {
            if( currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL ||
                currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_MEDIA_BROWSER_LEVEL ||
                currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL)
            {
                 mediaPlayerView.metaDataHasChanged();
            }
            else
            {
                channelInfoView.metaDataHasChanged();
            }
        }
    }

    KvalUI_MailBoxView {
        id: mailBoxMainView
        onMailBoxDisplayReadyNotify: mainScreenView.mailBoxReady()
        onSetMailSeenFlagNotify: serverNotificationEngine.setSeenFlagById(index)
        onActivateMainScreenView: mainScreenView.activateMainScreenView()
        onNotifyMainScreenMailBoxStatus: mainScreenView.onMailBoxStatus(unseen, total)
        onDeleteCurrentMailNotify : serverNotificationEngine.deleteMailById(index)
        onDeleteAllMailsNotify: serverNotificationEngine.deleteAllMails()
    }
    KvalUI_FileMediaBrowser {
        id: mediaBrowserMainView
        onMediaBrowserReadyNotify: mainScreenView.onMediaBrowserViewReady()
        onActivateMainScreenView: mainScreenView.activateMainScreenView()
        onFocusRelinquished: restoreFocusToMain();
    }
    KvalUI_SettingsMainView {
        id: settingsMainView
        onSettingsReadyNotify: mainScreenView.onSettingsViewReady()
        onActivateMainScreenView: mainScreenView.activateMainScreenView()
    }
    KvalUI_VirtualKeyBoard {
        id: virtualKeyBoard
    }
    KvalUI_ServerNotificationCenter {
        id: serverNotificationCenter
    }
    KvalUI_PyNotificationView {
        id: pyNotificationView
    }
    KvalUI_MessagePopUp {
        id: popUpView
    }

    FastBlur {
        id: blurUi
        anchors.fill: parent
        visible: false
        radius: 38
    }

    Item {
        id: rebootItem
        anchors.fill: parent
        visible: false

        Image {
            id: rebootIco
            anchors.centerIn: parent
            opacity: 0.7
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAIN_REBOOT_ICON)
        }
        Text{
            id: rebootText
            anchors.top: rebootIco.bottom
            anchors.topMargin: marginBlock * 2
            anchors.horizontalCenter: parent.horizontalCenter
            text: kvalUiConfigManager.retranslate + qsTr("Reboot In Progress ...")
            color: "white"
            opacity: 0.7
            font {
                family: localAdventFont.name;
                italic: true
                bold: true
                pixelSize: parent.width * 0.02
            }
        }
    }


    function activeFocusViewManager(viewObj)
    {
        if(popUpView.enabled)
        {
            popUpView.enabled = true
            popUpView.focus = true
            return false
        }
        else
        {
            viewObj.enabled = true
            viewObj.focus = true
            return true
        }
    }

    function registerActiveView(activeViewObj)
    {
        g_currentActiveView = activeViewObj
    }
    function getCurrentActiveView()
    {
        return g_currentActiveView
    }

    function removeFocusfromMain()
    {
        focus = false;
        enabled = false;
    }
    function restoreFocusToMain()
    {
        focus = true;
        enabled = true;
    }
    function showVideo(videoUri, audioUri, isDualStram, isLoop)
    {
        mediaPlayer.setSource(videoUri,
                              ((!isDualStram) ? "" : audioUri),
                              ((!isDualStram) ? false : true),
                              ((!isLoop) ? false : true))
    }
    function setNavLevel(navLevel)
    {
        currentNavLevel = navLevel;
    }
    function setActivePlayingList(activePlayList)
    {
        activePlayingList = activePlayList;
    }
    //---
    function onPlayBackCompleted()
    {
        streamInfoMetaData.initMetaDatas()
        if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL)
        {
            channelSelectionListView.playStopped();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL)
        {
            g_userStopRequested = false
            vodLaunchView.playBackCompleted();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL)
        {
            appsView.playBackCompleted();
        }
    }

    function onPlayBackInterrupted(interruptedPosition)
    {
        if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL)
        {
            channelSelectionListView.playInterrupted();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL)
        {
            vodLaunchView.playBackInterrupted(interruptedPosition);
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL)
        {
            appsView.playBackInterrupted(interruptedPosition);
        }
    }

    function onPlayBackStarted()
    {
        logger.info("Playing Signal received!");
        g_userStopRequested = false
        if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL)
        {
            streamInfoMetaData.switchLiveAspectRatio();
            channelSelectionListView.playStartSuccess();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL)
        {
            logger.info("play started CATEGORIES_VOD_LEVEL")
            streamInfoMetaData.switchVodAspectRatio();
            vodLaunchView.playBackStarted();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL)
        {
            logger.info("play started CATEGORIES_APPLICATIONS_LEVEL")
            streamInfoMetaData.switchVodAspectRatio();
            appsView.playBackStarted();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_MEDIA_BROWSER_LEVEL)
        {
            streamInfoMetaData.switchVodAspectRatio();
            logger.info("play started CATEGORIES_MEDIA_BROWSER_LEVEL")
            mediaBrowserMainView.mediaBrowserPlayBackStarted();
        }
        streamInfoMetaData.extractStreamBaseMetaData();
    }
    
    function onPlayBackFailed()
    {
        logger.info("Play Failed Signal received!");
        streamInfoMetaData.initMetaDatas()
        if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL)
        {
            channelSelectionListView.playStartFailed();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL)
        {
            vodLaunchView.playBackFailed();
        }
        else if(currentNavLevel === IPTV_Constants.ENUM_CATEGORIES_APPLICATIONS_LEVEL)
        {
            appsView.playBackFailed();
        }
    }

    function stopCurrentMedia()
    {
        logger.info("stopCurrentMedia");
        g_userStopRequested = true
        kvalUiLiveStreamManager.stopLiveStream()
        mediaPlayer.stop();
        channelSelectionListView.streamPlayStateInProgress = false;
    }
    
    function playCurrentMedia()
    {
        logger.info("play ...");
        mediaPlayer.play();
    }
    
    function pauseCurrentMedia()
    {
        mediaPlayer.pause();
    }

    //KeyBoard handling
    Keys.onPressed:
    {
        if (event.key === Qt.Key_H)
        {
            testOn = false;
            if(vodMainView.enabled) return
            stopCurrentMedia();
            channelSelectionListView.hideAllLists();
            mainScreenView.activateMainScreenView()
            pyNotificationView.clearview();
            streamInfoMetaData.hideMetaData()
        }
        else if (event.key === Qt.Key_U)
        {channelInfoView.volumeUp();
        }
        else if (event.key === Qt.Key_D)
        {channelInfoView.volumeDown();
        }
        else if (event.key === Qt.Key_N)
        {channelInfoView.setMute();
        }
        else return;
        event.accepted = true;
    }
}
