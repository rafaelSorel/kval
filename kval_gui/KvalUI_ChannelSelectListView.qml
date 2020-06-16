import QtQuick 2.0

import QtGraphicalEffects 1.0
import QtQuick.XmlListModel 2.0
import QtQuick.Controls 2.0
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0

Item {
    id: channelListItem

    enabled: false
    opacity: 0
    focus: false

//------------------------------------------------------------------------------
// Global Vars
//------------------------------------------------------------------------------
    signal focusRelinquished();

    signal setChInfo(string chIndex, string chName, string chPicon)
    signal messageDisplayPopUp(int severity, string msgData);
    signal messageDisplayLoadingPopUp(int severity, string msgData);
    signal messageHideLoadingPopUp();

    //Define Local variables

    property bool       chListinitDoneFlag   : false;
    property string     defaultChannel       : "";
    //Epg Variables
    property string     epgCurrentTitle      : "";
    property string     epgCurrentStart      : "";
    property string     epgCurrentEnd        : "";
    property int        epgCurrentDuration   : 0;
    property string     epgCurrentSummary    : "";
    property string     epgNextProgram       : "";
    property string     epgNextProgramStartLength: "";
    property string     currentChannelPicon  : "";
    property bool       streamPlayStateInProgress      : false;
    property bool       g_loadingInProgress      : false;
    property string     g_currentChNameInPlay : "";
    property string     g_currentChUriInPlay : "";
    property bool       g_liveTvNewAvailableFile : false
    property bool       g_liveTvReloadFile : false
    property int        g_relaunchOnTheSameStream: 0
    property int        g_previousChPosIndex: -1
    property int        g_currentChPosIndex: -1
    property int        g_previousCatPosIndex: -1
    property int        g_currentCatPosIndex: -1
    property bool       g_isRecallProc : false
    property string     g_channelNumber: ''
    property int        g_hint_value: -1
    property bool       g_isFavoriteActive: false
    property bool       g_epgListEnabled: false
    property int        g_seekPosition: 0
    property bool       g_seekInProgress: false
    property var        g_activeChannelNames: []
    property var        g_liveTvHints:
        [kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LIVETV_HINT_1),
         kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LIVETV_HINT_2),
         kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LIVETV_HINT_3)]

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
    function wakeUp()
    {
        registerActiveView(channelSelectionListView)
        kvalUiConfigManager.updateLiveTvSrvAddress()
        returnKeyPressed()
        yellowButtonPressed()
        logger.info(">>>>>>>>>>>>>>>>>>> rootRectangle.width: " + rootRectangle.width)
    }

    function unHideMainView() {
        listGlobalBlock.visible = true
        activeFocusViewManager(channelSelectionListView)
        opacity = 1;
    }
    function setHeaderFooter(header)
    {
        headerChannelList.text = kvalUiConfigManager.retranslate + qsTr("Bouquets") +
                                ((header !== "") ? (" • " + header) : "")
    }

    function hideMainView()
    {
        listGlobalBlock.visible = false
        opacity = 0;
        focus = false;
        enabled = false;
        focusRelinquished();
    }

    function hideMainListView() {
        listGlobalBlock.visible = false
        enabled = false;
        focus = false;
    }

    function popUpTryGetFocus()
    {
        if(!enabled)
        {
            return false
        }
        listGlobalBlock.opacity = 0.3
        enabled = false
        focus = false
        return true
    }
    function popUpFocusRelinquished()
    {
        listGlobalBlock.opacity = 1
        activeFocusViewManager(channelSelectionListView)
    }

    function newFileAvailable()
    {
        g_liveTvNewAvailableFile = true
    }

    function newEpgFileAvailable()
    {
        kvalUiLivetvManager.newEpgFileAvailable();
        serverNotificationEngine.updateEpgFile();
    }

    function epgFileUpdated()
    {
        kvalUiLivetvManager.newEpgFileReady();
    }

    function langChanged()
    {
        xmlChannelQuickSettingModel.reload()
    }

    function checkChannelPicons()
    {
        if(!channelListModel.count) return ""
        currentChannelPicon = kvalUiConfigManager.qmlGetSrvPiconPath(
                              channelListModel.get(channelList.currentIndex).channelName)
        return currentChannelPicon
    }
    function loadStreamInProgress()
    {return g_loadingInProgress;
    }

    function channelListinitState(){
        if(chListinitDoneFlag) return;
        chListinitDoneFlag = true;
        if(channelList.currentIndex < 0) channelList.currentIndex = 0;
    }

    function showChannelListCategory() {
        channelListinitState();
        activeFocusViewManager(channelSelectionListView)
        opacity = 1;
        updateCategoryList();
    }

    function invokeChannelSelectionListView()
    {
        channelEmptyMsgZone.visible = false
        channelSelectClearEpgAndPiconsZones();

        if (g_liveTvReloadFile)
        {
            g_liveTvReloadFile = false
            kvalUiGuiUtils.reloadChannelsList()
            xmlCategoryModel.query = "/categories/category/liveTvFavorite"
            xmlCategoryModel.reload()
        }
        else if(g_liveTvNewAvailableFile)
        {
            g_liveTvNewAvailableFile = false
            kvalUiGuiUtils.reloadChannelsList()
            if(!serverNotificationEngine.updateLiveTvFiles())
            {
                xmlCategoryModel.query = "/categories/category/liveTvFavorite"
                xmlCategoryModel.reload()
            }
        }

        if(enabled && xmlCategoryModel.enabled)
            return;
        else if(enabled && !xmlCategoryModel.enabled)
            updateCategoryList();
        else
            showChannelListCategory();
    }
    function loadInfoView()
    {
        if(streamPlayStateInProgress)
            epgGenerator.updateEpgInfoView(kvalUiLivetvManager.quickGetChannelEpg(g_currentChNameInPlay))

        channelInfoView.showInfoBar();
    }

    function channelListDisplayFavChannels()
    {
        setHeaderFooter("Favoris")
        activeFocusViewManager(channelSelectionListView)
        g_isFavoriteActive = true
        xmlChannelModel.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_FAV_CHANNEL_FILE)
        xmlChannelModel.query = "/favoritesChannels/favorite/channel"
        xmlChannelModel.reload()
    }

    //JavaScripts functions
    function generateChannelEpg(channelName)
    {
        epgUpdaterWorker.sendMessage({'action': 'getEpgFull',
                                      'chName': channelName});
    }

    function setChannelPicons()
    {
        channelCurrentPiconImg.source = checkChannelPicons();
    }

    function getCurrentChannelUrl()
    {
        if(!channelListModel.count) return ""
        return channelListModel.get(channelList.currentIndex).channelUrl;
    }

    function channelSelectClearEpgAndPiconsZones()
    {
        g_hint_value = g_hint_value + 1
        if(g_hint_value === g_liveTvHints.length)
            g_hint_value = 0

        channelCurrentPiconImg.source = ""
        epgGenerator.clearEpgInfo()
        channelSelectionUpdateHintValues()
    }

    function channelSelectionUpdateHintValues()
    {
        channelSelectionListView.epgCurrentTitle =
                kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LIVE_TV_HINT_TITLE)
        channelSelectionListView.epgCurrentSummary =
                g_liveTvHints[g_hint_value]
    }

    function updateCategoryList()
    {
        if(!enabled) return;
        channelSelectClearEpgAndPiconsZones()
        if(categoryList.currentIndex < 0) categoryList.currentIndex = 0;
        channelList.opacity = 0;
        channelList.enabled = false;
        channelList.focus = false;
        categoryList.opacity = 1;
        categoryList.enabled = true;
        categoryList.focus = true;
        setHeaderFooter("")
    }

    function updateChannelList()
    {
        if(!enabled) return;
        if(channelList.currentIndex < 0) channelList.currentIndex = 0;
        categoryList.opacity = 0;
        categoryList.enabled = false;
        categoryList.focus = false;
        channelList.positionViewAtBeginning();
        channelList.opacity = 1;
        channelList.enabled = true;
        channelList.focus = true;
        opacity = 1
        setChannelPicons();
        generateChannelEpg(getCurrentChannelName());
    }

    function channelListFetchEpgData()
    {
        g_epgListEnabled =kvalUiLivetvManager.epgListMode()
        kvalUiLivetvManager.extractChannelEpgList(g_activeChannelNames)
    }

    WorkerScript {
        id: epgUpdaterWorker
        source: "KvalUI_dataloader.js"

        onMessage:
        {
            if(messageObject.reply && messageObject.reply === 'ready')
            {
                if(g_isRecallProc) recallAfterReload();
                else updateChannelList();
            }
            else if(messageObject.epgfull)
            {
                epgGenerator.setLocalEpgValues(messageObject.epgfull)
            }
        }
    }

    function epgDataReady(epgdata, isLastOne)
    {
        var msg = { 'action': 'updateEpg',
                    'model': channelListModel,
                    'epg': epgdata,
                    'viewlist': g_epgListEnabled};
        epgUpdaterWorker.sendMessage(msg);
    }

    function epgDataEnds()
    {
        generateChannelEpg(getCurrentChannelName())
    }

    function updateHeaderCategoryName()
    {
        setHeaderFooter(xmlCategoryModel.get(categoryList.currentIndex).category);
    }

    function getCurrentChannelName()
    {
        if(!channelListModel.count) return ""
        return channelListModel.get(channelList.currentIndex).channelName;
    }
    function getCurrentChannelId()
    {
        if(!channelListModel.count) return ""
        return channelListModel.get(channelList.currentIndex).channelId;
    }

    function displayAllChannels()
    {
    }

    function localLoadingPopUpDisplay(msgType, msgData)
    {
        opacity = 0.2;
        g_loadingInProgress = true;
        messageDisplayLoadingPopUp(msgType,
                                   msgData);
    }

    function playStartSuccess()
    {
        channelInfoView.stopBuffering()
        streamPlayStateInProgress = true;
        g_loadingInProgress = false;
        messageHideLoadingPopUp();
        opacity = 1
        focusRelinquished();
        activeFocusViewManager(channelSelectionListView)
        listGlobalBlock.visible = false
        opacity = 0;
        if(!g_relaunchOnTheSameStream) loadInfoView();
        else g_relaunchOnTheSameStream = 0
    }

    function playStartFailed()
    {
        channelInfoView.hideInfoBar();
        g_relaunchOnTheSameStream = 0
        channelInfoView.stopBuffering()
        streamPlayStateInProgress = false;
        g_loadingInProgress = false;
        messageHideLoadingPopUp();
        listGlobalBlock.visible = true
        opacity = 1;
        focusRelinquished();
        opacity = 1;
        activeFocusViewManager(channelSelectionListView)
    }
    function playStopped()
    {
        g_relaunchOnTheSameStream = 0
        channelInfoView.stopBuffering()
        streamPlayStateInProgress = false;
    }

    function playInterrupted()
    {
        streamPlayStateInProgress = false;
        g_loadingInProgress = false;
        //Relaunch and make 1 try Stream
        if(g_relaunchOnTheSameStream === 1)
        {
            g_relaunchOnTheSameStream = 0
            return
        }
        channelInfoView.startBuffering()
        g_relaunchOnTheSameStream = g_relaunchOnTheSameStream + 1
        kvalUiLiveStreamManager.stopLiveStream()
        kvalUiLiveStreamManager.playLiveStream(g_currentChUriInPlay);
    }
    function progNextTimeout()
    {
        stopCurrentMedia();
        focus = false;
        g_loadingInProgress = true;
        messageDisplayLoadingPopUp( IPTV_Constants.ENUM_INFO_MESSAGE,
                                   kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING) + " " +
                                   getCurrentChannelName());
        storeHistoryChannelInfo()
        kvalUiLiveStreamManager.playLiveStream(g_currentChUriInPlay);
    }

    function storeHistoryChannelInfo()
    {
        g_previousChPosIndex = g_currentChPosIndex
        g_previousCatPosIndex = g_currentCatPosIndex
        g_currentChPosIndex = channelList.currentIndex
        g_currentCatPosIndex = categoryList.currentIndex
        g_currentChNameInPlay = getCurrentChannelName()
        g_currentChUriInPlay = fillTokenWithCody()
    }

    function channelProgNext()
    {
        if( listGlobalBlock.visible ||
            g_loadingInProgress ||
            activePlayingList !== IPTV_Constants.ENUM_ACTIVE_PLAYING_LIVE_CHANNELS
          )
        {
            logger.info("No zap")
            return
        }
        if(progNextTimer.running)
        {
            progNextTimer.stop()
            progNextTimer.running = false
        }
        channelList.incrementCurrentIndex();
        setChannelPicons();
        setChInfo(getCurrentChannelId() ,
                  getCurrentChannelName(),
                  currentChannelPicon);
        loadInfoView();

        progNextTimer.start()
    }

    function channelProgPrevious()
    {
        if(listGlobalBlock.visible) return

        if( opacity ||
            g_loadingInProgress ||
            activePlayingList !== IPTV_Constants.ENUM_ACTIVE_PLAYING_LIVE_CHANNELS
          )
        {return
        }
        if(progNextTimer.running)
        {
            progNextTimer.stop()
            progNextTimer.running = false
        }
        channelList.decrementCurrentIndex();
        setChannelPicons();
        setChInfo(getCurrentChannelId() ,
                  getCurrentChannelName(),
                  currentChannelPicon);
        loadInfoView();

        progNextTimer.start()
    }

    function recallAfterReload()
    {
        g_isRecallProc = false
        channelList.currentIndex = g_previousChPosIndex
        messageDisplayLoadingPopUp( IPTV_Constants.ENUM_INFO_MESSAGE,
                                   kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING) + " " +
                                   getCurrentChannelName());
        storeHistoryChannelInfo()
        setChannelPicons();
        setChInfo(getCurrentChannelId() ,
                  getCurrentChannelName(),
                  currentChannelPicon);
        loadInfoView();

        kvalUiLiveStreamManager.playLiveStream(g_currentChUriInPlay);
    }

    function channelProgRecall()
    {
        logger.info("channelProgRecall")

        logger.info("g_previousCatPosIndex: " + g_previousCatPosIndex)
        logger.info("g_loadingInProgress: "+g_loadingInProgress)
        logger.info("g_isRecallProc: " + g_isRecallProc)
        logger.info("activePlayingList: "  + activePlayingList)
        if( g_previousCatPosIndex === -1 ||
            g_loadingInProgress ||
            g_isRecallProc ||
            activePlayingList !== IPTV_Constants.ENUM_ACTIVE_PLAYING_LIVE_CHANNELS)
            return

        stopCurrentMedia();
        g_isRecallProc = true
        focus = false;
        g_loadingInProgress = true;
        categoryList.currentIndex = g_previousCatPosIndex
        updateHeaderCategoryName();
        xmlChannelModel.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_CHANNEL_FILE)
        xmlChannelModel.query =
            "/categories/category/liveTvFavorite[" +
            xmlCategoryModel.get(categoryList.currentIndex).index +
            "]/channel";
        xmlChannelModel.reload();
    }

    function activateChannelByNumber()
    {
        if( g_loadingInProgress ||
            g_isRecallProc ||
            activePlayingList !== IPTV_Constants.ENUM_ACTIVE_PLAYING_LIVE_CHANNELS)
        {
            logger.info("activateChannelByNumber first return")
            return
        }

        g_isRecallProc = true
        logger.info("g_channelNumber: " + g_channelNumber)
        var position = kvalUiGuiUtils.getChannelPosition(Number(g_channelNumber))
        logger.info("qmlBinder position: " + position)

        if(position[0] === -1 || position[1] === -1)
        {
            g_isRecallProc = false
            g_channelNumber = ""
            channelInfoView.setChNumber(getCurrentChannelId())
            loadInfoView()
            return
        }

        g_previousCatPosIndex = position[0]
        g_previousChPosIndex = position[1]
        stopCurrentMedia();
        focus = false;
        g_loadingInProgress = true;
        categoryList.currentIndex = g_previousCatPosIndex
        updateHeaderCategoryName();
        xmlChannelModel.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_CHANNEL_FILE)
        xmlChannelModel.query =
            "/categories/category/liveTvFavorite[" +
            xmlCategoryModel.get(categoryList.currentIndex).index +
            "]/channel";
        logger.info("xmlCategoryModel index: " + xmlCategoryModel.get(categoryList.currentIndex).index)
        xmlChannelModel.reload();
        g_channelNumber = ''
    }

    function channelSetNumber(digit)
    {
        if(listGlobalBlock.visible) return

        if( g_loadingInProgress ||
            g_isRecallProc ||
            activePlayingList !== IPTV_Constants.ENUM_ACTIVE_PLAYING_LIVE_CHANNELS)
            return

        //Don't go beyond 4 digit
        if(g_channelNumber.length === 4) return
        if(setChannelNumberTimer.running)
        {
            setChannelNumberTimer.stop()
            setChannelNumberTimer.running = false
        }
        g_channelNumber = g_channelNumber + digit
        channelInfoView.setChNumber(g_channelNumber)
        setChannelNumberTimer.start()
    }

    function fillTokenWithCody()
    {
        return getCurrentChannelUrl()
    }

    function channelViewLoadStream(streamName)
    {
        //Avoid launching while loading
        if(loadStreamInProgress())
            return

        //Avoid relaunching the same stream
        if( g_currentChUriInPlay === fillTokenWithCody() &&
            streamPlayStateInProgress === true)
        {
            logger.info("Load the same channel")
            listGlobalBlock.visible = false
            loadInfoView();
            return;
        }

        localLoadingPopUpDisplay( IPTV_Constants.ENUM_INFO_MESSAGE,
                          kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING) + " " + streamName);
        setActivePlayingList( IPTV_Constants.ENUM_ACTIVE_PLAYING_LIVE_CHANNELS);
        //Call Stop function here
        stopCurrentMedia();

        //Extract stream Link
        storeHistoryChannelInfo()
        setChInfo(getCurrentChannelId() ,
                  g_currentChNameInPlay,
                  currentChannelPicon);
        kvalUiLiveStreamManager.playLiveStream(g_currentChUriInPlay);
    }

    function channelSelectionOkButtonPressed()
    {
        if(!listGlobalBlock.visible)
        {
            loadInfoView()
            return
        }
        if(channelQuickSettingList.enabled)
        {channelViewQuickSettingOnSelect()
        }
        else if(channelList.enabled)
        {
            if(!channelList.count) return
            channelViewLoadStream(getCurrentChannelName());
        }
        else if(categoryList.enabled)
        {
            updateHeaderCategoryName();
            xmlChannelModel.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_CHANNEL_FILE)
            xmlChannelModel.query =
                "/categories/category/liveTvFavorite[" +
                xmlCategoryModel.get(categoryList.currentIndex).index +
                "]/channel";
            xmlChannelModel.reload();
        }
    }
    function channelSelectionBackButtonPressed()
    {
        if(!listGlobalBlock.visible)
        {
            channelProgRecall();
            return
        }
        if(channelQuickSettingList.enabled)
        {hideQuickSettingList();
        }
        else if(channelList.enabled)
        {updateCategoryList();
        }
    }

    function channelSelectionOnLeftKeyPressed()
    {
        if(!listGlobalBlock.visible) return

        if(channelQuickSettingList.enabled)
        {return
        }
        if(channelList.enabled)
        {
            if(channelList.currentIndex - 5 <= 0)
            {
                setChannelPicons();
                generateChannelEpg(getCurrentChannelName());
                channelList.currentIndex = 0;
            }
            else channelList.currentIndex = channelList.currentIndex - 5;
        }
    }

    function channelSelectionOnRightKeyPressed()
    {
        if(!listGlobalBlock.visible) return

        if(channelQuickSettingList.enabled)
        {return
        }
        if(channelList.enabled)
        {
            if(channelList.currentIndex+5 >= channelList.count)
            {
                setChannelPicons();
                generateChannelEpg(getCurrentChannelName());
                channelList.currentIndex = channelList.count-1;
            }
            else channelList.currentIndex = channelList.currentIndex + 5;
        }
    }

    function channelSelectionOnUpKeyPressed()
    {
        if(!listGlobalBlock.visible) return

        if(channelQuickSettingList.enabled)
        {channelQuickSettingList.decrementCurrentIndex()
        }
        else if(channelList.enabled)
        {
            channelList.decrementCurrentIndex();
            setChannelPicons();
            generateChannelEpg(getCurrentChannelName());
        }
        else if(categoryList.enabled)
        {categoryList.decrementCurrentIndex();
        }
    }

    function channelSelectionOnDownKeyPressed()
    {
        if(!listGlobalBlock.visible) return

        if(channelQuickSettingList.enabled)
        {channelQuickSettingList.incrementCurrentIndex();
        }
        else if(channelList.enabled)
        {
            channelList.incrementCurrentIndex();
            setChannelPicons();
            generateChannelEpg(getCurrentChannelName());
        }
        else if(categoryList.enabled) categoryList.incrementCurrentIndex();
    }

    function channelSelectionMenuKeyPressed()
    {
        if(!listGlobalBlock.visible) return

        displayQuickChannelSettingList()
    }

    function returnKeyPressed()
    {
        channelInfoView.hideInfoBar();
        switch (currentNavLevel)
        {
            case IPTV_Constants.ENUM_CATEGORIES_ROOT_LEVEL:
                setHeaderFooter("");
                unHideMainView();
                break;
            case IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL:
                if(loadStreamInProgress()) return
                if(!listGlobalBlock.visible)
                {
                    if(channelList.enabled)
                        channelListFetchEpgData()
                    else
                        channelSelectClearEpgAndPiconsZones()
                }
                unHideMainView();
                break;
            default:
                setHeaderFooter("");
                unHideMainView();
                break;
        }
    }

    function yellowButtonPressed()
    {
        if(!listGlobalBlock.visible) return;
        setNavLevel( IPTV_Constants.ENUM_CATEGORIES_STREAMCHANNELS_LEVEL );
        invokeChannelSelectionListView();
    }
    function blueButtonPressed()
    {
        if(vodMainView.enabled) return
        if(mediaPlayerView.enabled) return
        if(!listGlobalBlock.visible)
        {
            channelInfoView.hideInfoBar();
            focus = false;
            streamInfoMetaData.showMetaData(channelSelectionListView);
        }
    }
    function focusRelinquish()
    {
        activeFocusViewManager(channelSelectionListView)
    }
    function greenButtonPressed()
    {
        if(vodMainView.enabled) return
        if(mediaPlayerView.enabled) return
        if(!listGlobalBlock.visible) return

        channelListDisplayFavChannels();
    }
    function hideAllLists()
    {
        hideMainView();
        channelInfoView.disableViewDisplay();
    }

    function channelViewQuickSettingOnSelect()
    {
        switch (channelQuickSettingList.currentIndex)
        {
            case IPTV_Constants.ENUM_LIVE_QUICK_SET_ADD_FAV:
                logger.info("Add Channel from favorite ...")
                if( xmlChannelModel.query !==
                    "/favoritesChannels/favorite/channel" &&
                    !categoryList.enabled)
                {
                    kvalUiGuiUtils.addSelectedChannelToFav(getCurrentChannelId(),
                                                        getCurrentChannelName(),
                                                        getCurrentChannelUrl())
                }
                break;
            case IPTV_Constants.ENUM_LIVE_QUICK_SET_REMOVE_FAV:
                logger.info("Remove Channel from favorite ...")
                if( xmlChannelModel.query ===
                    "/favoritesChannels/favorite/channel")
                {
                    kvalUiGuiUtils.removeSelectedChannelFromFav(
                                getCurrentChannelId())
                    xmlChannelModel.reload()
                }
                break;
            case IPTV_Constants.ENUM_LIVE_QUICK_SET_CLEAR_FAV:
                logger.info("Clear Fav lists ...")
                kvalUiGuiUtils.clearChannelFavoriteList()
                xmlChannelModel.reload()
                break;
            default:
                return;
        }
        hideQuickSettingList()
    }

    function displayQuickChannelSettingList()
    {
        if(channelQuickSettingList.currentIndex < 0)
            channelQuickSettingList.currentIndex = 0;
        channelEpgNextProgZone.opacity = 0.2
        channelEpgSummaryZone.opacity = 0.2
        channelQuickSettingRect.opacity = 0.7
        channelQuickSettingList.opacity = 1;
        channelQuickSettingList.enabled = true;
        channelQuickSettingList.focus = true;
    }

    function hideQuickSettingList()
    {
        channelEpgNextProgZone.opacity = 1
        channelEpgSummaryZone.opacity = 1
        channelQuickSettingRect.opacity = 0
        channelQuickSettingList.opacity = 0;
        channelQuickSettingList.enabled = false;
        channelQuickSettingList.focus = false;
        if(categoryList.enabled)
        {
            categoryList.enabled = true
            categoryList.focus = true
        }
        else
        {
            channelList.enabled = true
            channelList.focus = true
        }
    }

    function exit_button_simulated()
    {
        listGlobalBlock.visible = false
        channelInfoView.hideDownBar()
    }

    function playerFlushed()
    {
        channelInfoView.showTrickModeInfoBar(false)
        g_seekInProgress = false
    }

    function httpSeeked(position)
    {
        logger.info("Seek position: ", position)
        channelInfoView.setCurrentTrickTime(position)
    }
    function pausedForFlush() {}

//------------------------------------------------------------------------------
// Keys Management
//------------------------------------------------------------------------------
    Keys.onPressed: {
        if (g_liveTvReloadFile)
        {
            logger.info("Live Tv file missing")
            if (event.key === Qt.Key_H) return
            else
            {
                event.accepted = true;
                return
            }
        }

        if(event.key === Qt.Key_P) //Play Button
        {
            if(streamPlayStateInProgress && channelInfoView.trickModeActivated())
            {
                kvalUiLiveStreamManager.resumeLiveStream()
                channelInfoView.showTrickModeInfoBar(false)
            }
        }
        else if (event.key === Qt.Key_Space) //Pause Button
        {
            if(streamPlayStateInProgress)
            {
                if(kvalUiLiveStreamManager.pauseLiveStream(false))
                {
                    channelInfoView.showTrickModeInfoBar(true)
                }
            }
        }
        else if (event.key === Qt.Key_W) //skip previous Button
        {
            if( streamPlayStateInProgress &&
                channelInfoView.trickModeActivated() &&
                !g_seekInProgress)
            {
                g_seekInProgress = true
                g_seekPosition = -10;
                g_seekInProgress =
                    kvalUiLiveStreamManager.seekCurrentStream(g_seekPosition,
                                            channelInfoView.getTotalTrickTime(),
                                            channelInfoView.getCurrentTrickTime());
            }
        }
        else if (event.key === Qt.Key_F) //skip previous Button
        {
            if( streamPlayStateInProgress &&
                channelInfoView.trickModeActivated() &&
                !g_seekInProgress)
            {
                g_seekPosition = 10;
                g_seekInProgress =
                    kvalUiLiveStreamManager.seekCurrentStream(g_seekPosition,
                                            channelInfoView.getTotalTrickTime(),
                                            channelInfoView.getCurrentTrickTime());

            }
        }

        //---

        else if(event.key === Qt.Key_Return)
        {
            returnKeyPressed()
        }
        else if (event.key === Qt.Key_Escape)
        {
            if(loadStreamInProgress())
                stopCurrentMedia()
            listGlobalBlock.visible = false
            channelInfoView.hideDownBar()
        }
        else if (event.key === Qt.Key_Y)
        {yellowButtonPressed();
        }
        else if (event.key === Qt.Key_I)
        {blueButtonPressed();
        }
        else if (event.key === Qt.Key_G)
        {greenButtonPressed();
        }
        else if (event.key === Qt.Key_Up) //Up Button
        {channelSelectionOnUpKeyPressed()
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {channelSelectionOnDownKeyPressed()
        }
        else if (event.key === Qt.Key_Right) //Right Button
        {channelSelectionOnRightKeyPressed()
        }
        else if (event.key === Qt.Key_Left) //Left Button
        {channelSelectionOnLeftKeyPressed()
        }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {channelSelectionBackButtonPressed();
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {channelSelectionOkButtonPressed();
        }
        else if(event.key === Qt.Key_M)
        {channelSelectionMenuKeyPressed();
        }
        else if (event.key === Qt.Key_Plus)
        {channelProgNext();
        }
        else if (event.key === Qt.Key_Minus)
        {channelProgPrevious();
        }
        else if (event.key === Qt.Key_0)
        {channelSetNumber('0');
        }
        else if (event.key === Qt.Key_1)
        {channelSetNumber('1');
        }
        else if (event.key === Qt.Key_2)
        {channelSetNumber('2');
        }
        else if (event.key === Qt.Key_3)
        {channelSetNumber('3');
        }
        else if (event.key === Qt.Key_4)
        {channelSetNumber('4');
        }
        else if (event.key === Qt.Key_5)
        {channelSetNumber('5');
        }
        else if (event.key === Qt.Key_6)
        {channelSetNumber('6');
        }
        else if (event.key === Qt.Key_7)
        {channelSetNumber('7');
        }
        else if (event.key === Qt.Key_8)
        {channelSetNumber('8');
        }
        else if (event.key === Qt.Key_9)
        {channelSetNumber('9');
        }

        //Only for testing purpose (trigger self zapping test)
        else if (event.key === Qt.Key_T)
        {testOn = true;
        }

        else return;
        event.accepted = true;
    }

//------------------------------------------------------------------------------
// Items
//------------------------------------------------------------------------------
    Timer {
        id: progNextTimer

        interval: 800;
        running: false;
        repeat: false;
        onTriggered: {
            progNextTimeout()
        }
    }

    Timer {
        id: setChannelNumberTimer

        interval: 1500;
        running: false;
        repeat: false;
        onTriggered: {
            activateChannelByNumber()
        }
    }

    Item
    {
        id: listGlobalBlock
        anchors.fill: parent
        visible: false

        Image {
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LIVE_TV_BACK_IMG)
            anchors.fill: parent
            opacity: streamPlayStateInProgress ? 0.0 : 0.3
        }

        Rectangle {
            id: backGroundRect
            //67% of screen width and 87% of screen height
            width: rootRectangle.width * 0.67
            height: rootRectangle.height * 0.9
            x: rootRectangle.x + marginBlock*4
            y: rootRectangle.y + marginBlock*4
            border.width: 1
            border.color: "#141414"
            gradient: Gradient {
                GradientStop { position: 1.0; color: "black" }
                GradientStop { position: 0.0; color: "#141414" }
            }

            opacity: 0.8
            Image {
                width: parent.width *1.13
                height: parent.height *1.13
                anchors.centerIn: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
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
        //Header Zone
        Rectangle {
            id: headerZone

            x: listRect.x
            y: listRect.y
            width: listRect.width
            height: listRect.height * 0.1
            color: "transparent"
            Text {
                id: headerChannelList
                x: 10
                anchors.verticalCenter: parent.verticalCenter
                color: "white"
                opacity: 0.8
                font {
                    family: localAdventFont.name;
                    pixelSize: Utils.scaled(45)
                }
            }
            Text {
                id: headerZoneDate
                text: g_currentDateStdFormat
                y : 10
                anchors.right: parent.right
                anchors.leftMargin : 10
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(30)
                }

            }
            Text {
                text: g_currentTimeStdFormat
                y: headerZoneDate.y + 30
                anchors.right: parent.right
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(40)
                }
            }
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
        //Bottom Zone
        Item {
            id: bottomZone
            anchors.top: mainScrollZone.bottom
            anchors.left: mainScrollZone.left
            width: listRect.width
            height: listRect.height * 0.1
            Image{
                id: bottomZoneKeyGreen
                x: marginBlock
                width: Utils.scaled(45)
                anchors.bottom: parent.bottom
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_GREEN_BUTTON)
                Text{
                    x: marginBlock * 6
                    text: kvalUiConfigManager.retranslate + qsTr("Chaines Favoris")
                    color: "white"
                    font {
                        family: localFont.name;
                        pixelSize: Utils.scaled(35)
                    }
                }
            }
            Image{
                id: bottomZoneKeyYellow
                x: marginBlock  + (bottomZone.width*0.23)
                anchors.bottom: parent.bottom
                width: Utils.scaled(45)
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_YELLOW_BUTTON)
                Text{
                    x: marginBlock * 6
                    text: kvalUiConfigManager.retranslate + qsTr("Chaines Live")
                    color: "white"
                    font {
                        family: localFont.name;
                        pixelSize: Utils.scaled(35)
                    }
                }
            }
            Item {
                anchors.right : parent.right
                anchors.rightMargin: marginBlock*2
                anchors.bottom : parent.bottom
                height: Utils.scaled(55)
                width : Utils.scaled(111)
                Image{
                    height: parent.height
                    width: parent.width
                    anchors.bottom: parent.bottom
                    fillMode: Image.Stretch
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_MENU_BUTTON)
                }
            }
        }
        //Epg Update Zone
        Rectangle {
            id: mainPiconZone

            x: backGroundRect.width + backGroundRect.x
            y: backGroundRect.y+ marginBlock*3
            width: (rootRectangle.width -
                    backGroundRect.width -
                    (backGroundRect.x * Utils.scaled(2)))
            height: backGroundRect.height - marginBlock*6
            gradient: Gradient {
                GradientStop { position: 0.0; color: highlightColor }
                GradientStop { position: 1.0; color: highlightColorGrad }
            }
            opacity: 0.7
            Image {
                width: parent.width *1.13
                height: parent.height *1.13
                anchors.centerIn: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
            }
        }

        Rectangle {
            id: channelPiconZone

            x: backGroundRect.width + backGroundRect.x
            y: backGroundRect.y + (marginBlock*3)
            width: rootRectangle.width -
                    backGroundRect.width -
                    (backGroundRect.x * Utils.scaled(2))
            height: backGroundRect.height - (marginBlock*6)
            color: highlightColor
            opacity: 0
            enabled : false
        }

        Rectangle {
            id: channelListRect

            width: backGroundRect.width
            height: backGroundRect.height
            x: backGroundRect.x; y: backGroundRect.y
            color: "black"
            opacity: 0
            enabled : false
        }

        Rectangle {
            id: channelScrollZone

            width: channelListRect.width
            height: channelListRect.height * 0.8
            anchors.centerIn: channelListRect
            opacity: 0
        }

        //Category ListView
        Component {
                id : categoryComponent
                Row {
                  Column {
                      Rectangle {
                          id: indexRect
                          width: channelScrollZone.width*0.0666
                          height: channelScrollZone.height*0.0666
                          color: "transparent"
                          Image {
                              width: parent.width * 0.55
                              height: parent.height * 0.95
                              anchors.bottom: parent.bottom;
                              anchors.bottomMargin: -marginBlock*0.5;
                              anchors.right: parent.right;
                              anchors.rightMargin: marginBlock
                              fillMode: Image.Stretch
                              source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FOLDER_ICON)
                            }
                        }
                    }
                    Column {
                        Rectangle {
                            x: indexRect.x
                            width: (channelScrollZone.width) - (indexRect.width)
                            height: channelScrollZone.height*0.0666
                            color: "transparent"
                            Text {
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                anchors.top: parent.top
                                color: "white"
                                text: category
                                font {
                                  family: localFont.name;
                                  pixelSize: Utils.scaled(42)
                                }
                            }
                        }
                    }
                }
            }
        XmlListModel {
            id: xmlCategoryModel
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KVAL_XML_LIVE_TV_FILE)
            query: "/categories/category/liveTvFavorite"

            XmlRole {
             name: "category";
             query: "@title/string()"
            }
            XmlRole {
             name: "index";
             query: "@index/string()"
            }
            onStatusChanged:
            {
                channelEmptyMsgZone.visible = false
                if (status == XmlListModel.Ready)
                {
                    logger.info("LiveChannel xmlCategoryModel update Done")
                    updateCategoryList();
                }
                else if(status == XmlListModel.Error)
                {
                    channelEmptyMsgZone.visible = true
                    var errorStr = xmlCategoryModel.errorString()
                    logger.info("Error reading File: "+ errorStr)
                    if ( errorStr.indexOf('No such file or directory') !== -1)
                    {
                        headEmptyZoneTxt.text = kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MISSING_LIVETV_FILE)
                        bodyEmptyZoneTxt.text =
                                '\n\n'+
                                kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MISSING_LIVE_TV_HINT)
                        footerEmptyZoneTxt.text =
                                kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_PRESS_HOME_BUTTON_STR)
                        logger.info("File not found")
                    }
                    else
                    {logger.info("Set error ")
                    }
                    g_liveTvReloadFile = true
                }
            }
        }
        Component {
            id: highlightCategory
            Rectangle {
                anchors.horizontalCenter: categoryList.currentItem.horizontalCenter
                anchors.horizontalCenterOffset: -marginBlock
                anchors.verticalCenter: categoryList.currentItem.verticalCenter
                anchors.verticalCenterOffset: marginBlock * 0.5
                width: channelScrollZone.width * 0.95
                height: channelScrollZone.height * 0.06
                radius: 6

                gradient: Gradient {
                    GradientStop { position: 0.0; color: highlightColor }
                    GradientStop { position: 1.0; color: highlightColorGrad }
                }
            }
        }
        Rectangle {
            id: listViewRectCat
            anchors.fill: channelScrollZone
            color: "transparent"
            opacity: categoryList.opacity

            ListView {
                id: categoryList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: marginBlock * 3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock * 3

                enabled: false
                opacity: 0
                focus: false
                Rectangle {
                    color: 'white'
                    anchors.top: parent.top
                    anchors.horizontalCenter:scrollInternal.horizontalCenter
                    opacity: 0.2
                    width: 1
                    height: (parent.contentHeight > parent.height) ? parent.height : 0
                }
                ScrollBar.vertical: ScrollBar {
                    id: scrollInternal
                    active: true
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.rightMargin: marginBlock*2
                    anchors.bottom: parent.bottom
                }

                model: xmlCategoryModel
                highlight: highlightCategory
                highlightFollowsCurrentItem: false
                clip: false
                keyNavigationWraps: true
                interactive: false
                delegate: categoryComponent
            }
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: listItemsMaskImg
            }
        }
        //---

        ListModel {
             id: channelListModel
             ListElement {
                 channelName: ""
                 channelId: ""
                 channelUrl: ""
                 channelProgress: 0
                 channelEpg: ""
             }
         }

        //Channel ListView
        XmlListModel {
            id: xmlChannelModel
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_CHANNEL_FILE)

            XmlRole {
             name: "channelName";
             query: "@title/string()"
            }
            XmlRole {
             name: "channelId";
             query: "@id/string()"
            }
            XmlRole {
             name: "channelUrl";
             query: "@url/string()"
            }
            onStatusChanged: {
                if (status == XmlListModel.Ready)
                {
                    logger.info("live channel xmlChannelModel update Done")
                    if(xmlChannelModel.query === "/favoritesChannels/favorite/channel")
                    {
                        if(!xmlChannelModel.count)
                        {
                            channelEmptyMsgZone.visible = true
                            headEmptyZoneTxt.text = kvalUiConfigManager.retranslate +
                                kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_EMPTY_FAV_LIST_STR)
                            bodyEmptyZoneTxt.text = '\n' + g_liveTvHints[1]
                            footerEmptyZoneTxt.text = ''
                        }
                        else
                        {
                            channelEmptyMsgZone.visible = false
                        }
                    }
                    g_activeChannelNames = []
                    var channels = []
                    for(var i = 0; i < xmlChannelModel.count ; i++)
                    {
                        g_activeChannelNames.push(xmlChannelModel.get(i).channelName)
                        channels.push({'channelName': xmlChannelModel.get(i).channelName,
                                      'channelId': xmlChannelModel.get(i).channelId,
                                      'channelUrl': xmlChannelModel.get(i).channelUrl})
                    }

                    epgUpdaterWorker.sendMessage({'action': 'addData',
                                                  'model': channelListModel,
                                                  'channels': channels});

                    channelListFetchEpgData()
                    logger.info("xmlChannelModel update Done")

                }
                else if(status == XmlListModel.Error)
                {
                    var errorStr = xmlChannelModel.errorString()
                    logger.info("Error reading File: "+ errorStr)
                    if ( errorStr.indexOf('No such file or directory') !== -1)
                    {logger.info("File not found")
                    }
                    else
                    {logger.info("Set error ")
                    }
                    g_liveTvReloadFile = true
                }
                else
                {
                    logger.error("Undefined status: "+status)
                }
            }
        }
        Component {
            id : channelComponent
            Row {
                id: wrapperRow
                Column {
                    Item {
                      id: indexRect
                      x: marginBlock
                      width: channelScrollZone.width*0.0666
                      height: channelScrollZone.height*0.0666
                      Text {
                          id: currentChannelListView
                          anchors.right: parent.right
                          anchors.bottom: parent.bottom
                          anchors.top: parent.top
                          text: channelId;
                          color: "white"
                          font {
                              family: localFont.name;
                              pixelSize: Utils.scaled(42)
                            }
                        }
                    }
                }
                Column {
                    Item {
                          x: marginBlock * 3
                          width: channelScrollZone.width*0.0833
                          height: channelScrollZone.height*0.0666
                          opacity: channelProgress ? 1 : 0;
                          Rectangle {
                              id: backEpgProgress
                              anchors.verticalCenter: parent.verticalCenter
                              anchors.verticalCenterOffset: marginBlock * 0.5
                              anchors.horizontalCenter: parent.horizontalCenter
                              width: parent.width
                              height: parent.height * 0.25
                              border.color: wrapperRow.ListView.isCurrentItem ? "white" : highlightColor
                              border.width: 2
                              color: "transparent"
                          }
                          Rectangle {
                              anchors.verticalCenter: backEpgProgress.verticalCenter
                              anchors.left: backEpgProgress.left
                              width: (channelProgress/100) * backEpgProgress.width
                              height: backEpgProgress.height
                              color: wrapperRow.ListView.isCurrentItem ? "white" : highlightColor
                          }
                    }
                }

                Column {
                    Item {
                        id: channelNameRect
                        x: marginBlock * 5
                        width: (channelScrollZone.width) - (marginBlock * 9)
                        height: channelScrollZone.height*0.0666
                        Text {
                            id: chName
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                            anchors.top: parent.top
                            elide:Text.ElideRight
                            color: "white"
                            text: channelName
                            font {
                              family: localFont.name;
                              pixelSize: Utils.scaled(42)
                            }
                        }
                        Text {
                            anchors.left: chName.right
                            horizontalAlignment: Text.AlignLeft
                            anchors.leftMargin: marginBlock * 2
                            width: (parent.width * 0.8) - (marginBlock * 2) - chName.contentWidth
                            anchors.bottom: chName.bottom
                            color: wrapperRow.ListView.isCurrentItem ? "white" : highlightColor
                            text: "(" + channelEpg + ")"
                            opacity: channelProgress ? 1 : 0;
                            elide:Text.ElideRight
                            font {
                              family: localAdventFont.name;
                              pixelSize: Utils.scaled(30)
                            }
                        }
                    }
                }
            }
        }
        Component {
            id: highlight

            Rectangle {
                anchors.horizontalCenter: channelList.currentItem.horizontalCenter
                anchors.horizontalCenterOffset: -marginBlock*6.5
                anchors.verticalCenter: channelList.currentItem.verticalCenter
                anchors.verticalCenterOffset: marginBlock * 0.5
                width: channelScrollZone.width * 0.95
                height: channelScrollZone.height * 0.06
                radius: 6

                gradient: Gradient {
                    GradientStop { position: 0.0; color: highlightColor }
                    GradientStop { position: 1.0; color: highlightColorGrad }
                }
            }
        }

        Rectangle {
            id: listViewRect
            anchors.fill: channelScrollZone
            color: "transparent"
            opacity: channelList.opacity

            ListView {
                id: channelList
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: marginBlock * 3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock * 3

                model: channelListModel

                Rectangle {
                    color: 'white'
                    anchors.top: parent.top
                    anchors.horizontalCenter:scrollChannelInternal.horizontalCenter
                    opacity: 0.2
                    width: 1
                    height: (parent.contentHeight > parent.height) ? parent.height : 0
                }
                ScrollBar.vertical: ScrollBar {
                    id: scrollChannelInternal
                    active: true
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.rightMargin: marginBlock*2
                    anchors.bottom: parent.bottom
                }

                highlight: highlight
                highlightFollowsCurrentItem: false
                clip: false
                keyNavigationWraps: true
                interactive: false

                focus: true
                delegate: channelComponent
            }
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: listItemsMaskImg
            }
        }
        Image {
            id: listItemsMaskImg
            anchors.fill: listViewRect
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_MASKGRID)
            visible: false
        }

        //---

        Item {
            id: channelCurrentPicon

            x: ((channelPiconZone.x) + ((channelPiconZone.width/2) -
                                        ((channelCurrentPicon.width)/2)))
            y: channelPiconZone.y + marginBlock*4
            width: Utils.scaled(270)
            height: Utils.scaled(200)

            Image {
                id: backImgPicon
                width: parent.width
                height: parent.height
                fillMode: Image.PreserveAspectFit
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.PICON_CHANNEL_ABS_PATH) +
                        "back" +
                        ".png";
            }

            Image {
                id: channelCurrentPiconImg

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
                        channelCurrentPiconImg.source =
                        kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.PICON_CHANNEL_ABS_PATH) +
                                "default" +
                                ".png";
                    }
                }
            }
        }
        Item {
            id: channelCurrentEpgTitle

            x: channelPiconZone.x
            y: channelPiconZone.height * 0.35
            width: channelPiconZone.width
            height: marginBlock * 5
            Text{
                text: epgCurrentTitle
                width: parent.width * 0.93
                elide: Text.ElideRight
                anchors.centerIn: parent
                style: Text.Raised
                styleColor: "black"
                font {
                    family: localAdventFont.name;
                    pixelSize: Utils.scaled(46)
                    capitalization: Font.AllUppercase;
                }
                color: "white"
            }
        }
        Item {
            id: channelCurrentEpgProgressZone

            x: channelPiconZone.x
            y: channelPiconZone.height * 0.425
            width: channelPiconZone.width
            height: channelPiconZone.height * 0.05
        }
        Rectangle {
            anchors.left: channelCurrentEpgProgressZone.left
            anchors.top: channelCurrentEpgProgressZone.top
            anchors.bottom: channelCurrentEpgProgressZone.bottom
            width: channelPiconZone.width *0.2
            color: "transparent"
            opacity: 1
            Text{
                text: epgCurrentStart
                anchors.centerIn: parent
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(35)
                }
                color: "white"
            }
        }
        Rectangle {
            anchors.right: channelCurrentEpgProgressZone.right
            anchors.top: channelCurrentEpgProgressZone.top
            anchors.bottom: channelCurrentEpgProgressZone.bottom
            width: channelPiconZone.width *0.2
            color: "transparent"
            opacity: 1
            Text {
                text: epgCurrentEnd
                anchors.centerIn: parent
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(35)
                }
                color: "white"
            }
        }

        Rectangle {
            id: channelPiconEpgbar
            anchors.centerIn: channelCurrentEpgProgressZone
            anchors.verticalCenterOffset: 3
            width: channelPiconZone.width * 0.6
            height: 1
            color: "white"
            opacity: 1
        }
        Rectangle {
            id: channelPiconEpgProgressbar
            anchors.verticalCenter: channelPiconEpgbar.verticalCenter
            anchors.left: channelPiconEpgbar.left
            width: (epgCurrentDuration/100) * channelPiconEpgbar.width
            height: 5
            color: "white"
            opacity: 1
        }
        Rectangle {
            id: channelEpgSummaryZone
            y:channelPiconEpgProgressbar.y + marginBlock*3
            width: channelPiconZone.width * 0.8
            height: channelPiconZone.height * 0.5
            anchors.left: channelPiconZone.left
            anchors.right: channelPiconZone.right
            color: "transparent"
            Text {
                width: channelPiconZone.width * 0.93
                height: channelPiconZone.height * 0.5
                wrapMode: Text.WordWrap
                style: Text.Raised
                styleColor: "black"
                text: epgCurrentSummary
                anchors.centerIn: parent
                clip : true
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(30)
                }
                color: "white"
            }
        }
        Item {
            id: channelEpgNextProgZone

            y: channelEpgSummaryZone.y + channelEpgSummaryZone.height + marginBlock*0.5
            width: channelPiconZone.width * 0.8
            height: channelPiconZone.height * 0.1
            anchors.left: channelPiconZone.left
            anchors.right: channelPiconZone.right
            Text {
                text: epgNextProgram
                width: channelPiconZone.width * 0.93
                height: channelPiconZone.height * 0.1
                elide: Text.ElideRight
                anchors.centerIn: parent
                clip : true
                font {
                    family: localAdventFont.name;
                    pixelSize: Utils.scaled(30)
                    //capitalization: Font.AllUppercase;
                }
                color: "white"
            }
        }

        Item {
            id: channelEmptyMsgZone
            width: channelListRect.width * 0.8
            height: channelListRect.height * 0.6
            anchors.centerIn: channelListRect
            visible: false
            Text {
                id: headEmptyZoneTxt
                width: parent.width
                height: parent.height
                anchors.centerIn: parent
                wrapMode: Text.WordWrap
                style: Text.Raised
                styleColor: "black"
                text: kvalUiConfigManager.retranslate +
                      kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_EMPTY_FAV_LIST_STR)
                color: 'white'
                clip : true
                font {
                    family: localAdventFont.name;
                    pixelSize: Utils.scaled(42)
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
                    pixelSize: Utils.scaled(42)
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
                    pixelSize: Utils.scaled(30)
                }
            }
        }

        Rectangle {
            id: channelQuickSettingRect
            width: backGroundRect.width*0.3
            height: backGroundRect.height * 0.2
            anchors.bottom: backGroundRect.bottom
            anchors.left: channelEpgNextProgZone.left
            color: "black"
            opacity: 0
            radius: Utils.scaled(8)
            border.color: "grey"
            border.width: 1
            Image {
                width: parent.width *1.13
                height: parent.height *1.13
                anchors.centerIn: parent
                fillMode: Image.Stretch
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
            }
        }
        Rectangle {
            id: quickSettingListZoneRect
            width: channelQuickSettingRect.width
            height: channelQuickSettingRect.height * 0.9
            anchors.centerIn: channelQuickSettingRect
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
            id: xmlChannelQuickSettingModel
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_VOD_QUICK_SETTING_FILE)
            query: "/quick_settings/live_tv/setting"

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
                    for(var i = 0; i < xmlChannelQuickSettingModel.count ; i++)
                    {
                        quickSettingModel.insert(i,
                        {
                        'settingName':
                         kvalUiConfigManager.xmlGetStr(xmlChannelQuickSettingModel.get(i).settingName),
                        'settingIndex':
                         kvalUiConfigManager.xmlGetStr(xmlChannelQuickSettingModel.get(i).settingIndex)
                        });
                    }
                    logger.info("xmlChannelQuickSettingModel update Done")
                    channelQuickSettingList.currentIndex = 0
                }
            }
        }

        ListView {
            id: channelQuickSettingList
            anchors.left: quickSettingListZoneRect.left
            anchors.right: quickSettingListZoneRect.right
            anchors.top: quickSettingListZoneRect.top
            anchors.bottom: quickSettingListZoneRect.bottom
            enabled: false
            opacity: 0
            focus: false

            model: quickSettingModel
            highlight: highlightChannelSettingItem
            highlightFollowsCurrentItem: false
            clip: true
            keyNavigationWraps: true
            interactive: false
            delegate: quickSettingListComponent
        }

        Component {
            id: highlightChannelSettingItem

            Rectangle {
                id: highlightSettingRect
                x: channelQuickSettingList.currentItem.x
                y: channelQuickSettingList.currentItem.y
                width: channelQuickSettingRect.width
                height: Utils.scaled(45)
                color: highlightColor
            }
        }

        Component {
                id : quickSettingListComponent
                Row {
                    Column {
                        Rectangle {
                            width: quickSettingListZoneRect.width
                            height: quickSettingListZoneRect.height*0.3333
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
                                  pixelSize: Utils.scaled(30)
                                }
                            }
                        }
                    }
                }
            }
    }
}
