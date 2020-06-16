import QtQuick 2.2
import QtGraphicalEffects 1.0
import "KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0
import QtQuick.Controls 2.0

Item {
    id: vodLaunch
    width: rootRectangle.width;
    height: rootRectangle.height;
    x: rootRectangle.x
    y: rootRectangle.y

    signal launchTrailerNotify(string url);
    signal launchVideoNotify(int index);
    signal generateTrailerItemList(int index);
    signal generateVideoItemList(int index);
    signal deactivateMainView();
    signal focusRelinquished();
    signal setCurrentMediaInPlayName(string name, string plot);
    signal launchShahidVodUri(int index);
    signal notifyDeadLinks(string name, string uri);
    signal addMediaToFavorite(int index);

    property int       vodLaunchSetActiveIndex: 0;
    property bool      animationEnabled: false;
    property string    m_previousUri: ""
    property bool      g_isVodLaunched: false
    property string    g_ytAppId: "kval.youtube.app"

    enabled: false
    opacity: 0
    focus: false

    function startLaunchView()
    {
        showLaunchView()
    }

    function showLaunchView()
    {
        registerActiveView(vodLaunchView)
        focusRelinquished()
        activeFocusViewManager(vodLaunchView)
        opacity = 1
    }
    function popUpTryGetFocus()
    {
        if(!enabled)
        {
            return false
        }
        vodLaunchMovieCastRect.opacity = 0.1
        vodLaunchMovieDirectorTitle.opacity = 0.3
        vodLaunchCoverRect.opacity = 0.3
        vodLaunchPrincipalInfoRect.opacity = 0.3
        enabled = false
        focus = false
        return true
    }
    function popUpFocusRelinquished()
    {
        vodLaunchMovieCastRect.opacity = 0.3
        vodLaunchMovieDirectorTitle.opacity = 1
        vodLaunchCoverRect.opacity = 1
        vodLaunchPrincipalInfoRect.opacity = 1
        activeFocusViewManager(vodLaunchView)
    }

    function hideLaunchView()
    {
        appsView.unregisterObjApp(vodLaunchView)
        enabled = false
        opacity = 0
        focus = false
    }

    function inputList(title, items)
    {
        vodLaunchMovieCastRect.opacity = 0.1
        vodLaunchMovieDirectorTitle.opacity = 0.3
        vodLaunchCoverRect.opacity = 0.3
        vodLaunchPrincipalInfoRect.opacity = 0.3
        enabled = false
        focus = false
        popUpView.listChoiceMenu(title, items, vodLaunchView);
    }

    function diagInputListCallback(reply)
    {
        vodLaunchMovieCastRect.opacity = 0.3
        vodLaunchMovieDirectorTitle.opacity = 1
        vodLaunchCoverRect.opacity = 1
        vodLaunchPrincipalInfoRect.opacity = 1
        activeFocusViewManager(vodLaunchView)
        appsEngine.inputListReply("vod.native", reply)
    }

    function okDiagWindowNotify(title, text)
    {
        vodLaunchMovieCastRect.opacity = 0.1
        vodLaunchMovieDirectorTitle.opacity = 0.3
        vodLaunchCoverRect.opacity = 0.3
        vodLaunchPrincipalInfoRect.opacity = 0.3
        enabled = false
        focus = false
        popUpView.okDiag(title, text, vodLaunchView);
    }

    function diagWindowCallback(reply, needreply)
    {
        vodLaunchMovieCastRect.opacity = 0.3
        vodLaunchMovieDirectorTitle.opacity = 1
        vodLaunchCoverRect.opacity = 1
        vodLaunchPrincipalInfoRect.opacity = 1
        activeFocusViewManager(vodLaunchView)
        if(needreply) appsEngine.diagYesNoReply(reply)
    }

    function yesNoDiagWindowNotify(title, text, nolabel, yeslabel)
    {
        vodLaunchMovieCastRect.opacity = 0.1
        vodLaunchMovieDirectorTitle.opacity = 0.3
        vodLaunchCoverRect.opacity = 0.3
        vodLaunchPrincipalInfoRect.opacity = 0.3
        enabled = false
        focus = false
        popUpView.yesNoDiag(title, text, nolabel, yeslabel, vodLaunchView);
    }
    function diagProgressCreate(title, text)
    {
        vodLaunchMovieCastRect.opacity = 0.1
        vodLaunchMovieDirectorTitle.opacity = 0.3
        vodLaunchCoverRect.opacity = 0.3
        vodLaunchPrincipalInfoRect.opacity = 0.3
        enabled = false
        focus = false
        popUpView.progressDiag(title, text, vodLaunchView);
    }
    function diagProgressUpdate(position, text)
    {
        popUpView.progressDiagUpdate(position, text);
    }
    function diagProgressClose()
    {
        popUpView.progressDiagClose();
        diagWindowCallback(true, false)
    }
    function abortProgressDiag()
    {
        appsEngine.abortProgressDiag()
    }

    function setActiveIndex(index)
    {
        vodLaunchSetActiveIndex = index;
    }

    function vodLaunchUpdateHeader(headerStr)
    {
        vodLaunchViewHeaderTxt.text = headerStr;
    }

    function needCaptchaValue(Uri)
    {
        captchaImg.source = Uri
    }

    function fetchTrailerEnd()
    {
        pyNotificationView.clearview();
    }

    function playBackStarted()
    {
        if(vodLaunchTrailerSelectRect.opacity > 0)
        {
            mediaPlayerView.setDownloadValue(100);
        }
        else
        {
            setCurrentMediaInPlayName(vodMainView.getCurrentSelectedName(),vodLaunchPlotValue.text)
        }
        hideLaunchView();
        focusRelinquished();
        mediaPlayerView.invokerInfoBar()
        pyNotificationView.clearview();
        if(g_isVodLaunched) serverNotificationEngine.vodActivityNotify(true)
    }

    function playBackCompleted()
    {
        logger.info("VodLaunchView playBackCompleted()");
        serverNotificationEngine.vodActivityNotify(false)
        mediaPlayerView.hideInfoBar();
        vodEngine.stopNotify();
        showLaunchView();
    }
    function playBackInterrupted(interruptedPosition)
    {
        if (!interruptedPosition)
        {
            playBackCompleted()
            return
        }
        logger.info("VodLaunchView playBackInterrupted(): "+interruptedPosition);
        mediaPlayerView.hideInfoBar();
        stopCurrentMedia();
        mediaPlayer.enableAutoRestart();
        mediaPlayer.setSeekStart(interruptedPosition);
        if(vodMainView.getStreamVodValue())
        {mediaPlayerView.setDownloadValue(100);
        }
        else
        {mediaPlayerView.setDownloadValue(0);
        }
        showVideo(m_previousUri);
    }

    function playBackFailed()
    {
        serverNotificationEngine.vodActivityNotify(false)
        mediaPlayerView.hideInfoBar();
        pyNotificationView.requestAlertDisplay("error",
            kvalUiConfigManager.retranslate + qsTr("Vod Notification"),
            kvalUiConfigManager.retranslate +
            kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VOD_VIDEO_LAUNCH_PROBLEM))
        vodEngine.stopNotify();
        showLaunchView();
    }
    function vodLaunchVideoUri(uri)
    {
        m_previousUri = uri;
        mediaPlayer.enableAutoRestart();
        mediaPlayer.setSeekStart(0);
        if(vodMainView.getStreamVodValue())
        {mediaPlayerView.setDownloadValue(100);
        }
        else
        {mediaPlayerView.setDownloadValue(0);
        }
        showVideo(uri);
    }

    function vodLaunchTrailerListDisplay(names, urls)
    {
        var qualitySrc = ""
        var nameModified = ""
        vodLaunchViewItemModel.clear();
        for(var i = 0; i < names.length ; i++)
        {
            var vidItem = names[i]

            //Extract Quality src
            var qualitySizeSrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LIGHT_QUAL_ICON)
            var vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_720P)
            var vidLang = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VO_LANG_ICON)
            if(vidItem.indexOf("$HD$") !== -1)
            {
                vidItem = vidItem.replace('$HD$', '');
                qualitySrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_HD)
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LIGHT_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_720P)
            }
            else
            {
                vidItem = vidItem.replace('$SD$', '');
                qualitySrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_HD)
            }

            vodLaunchViewItemModel.insert(i, {  'vodTrailerCat' : '',
                                                'vodTrailerName': vidItem,
                                                'vodTrailerUrl': urls[i],
                                                'vodTrailerQualitySrc': qualitySrc,
                                                'vodTrailerQualSizeSrc': qualitySizeSrc,
                                                'vidsign': '',
                                                'vodRes': vidRes,
                                                'vodLang': vidLang});
        }
        updateTrailersItemList();
    }

    function vodLaunchVideoListDisplay(videoItems)
    {
        var qualitySrc = ""
        var nameModified = ""
        var paidIcon = ""
        var currentlang = ''
        var vodTrailerCatRef = 'multi'
        var vodTrailerCatDisplay = 'Version multi langue'

        vodLaunchViewItemModel.clear();
        for(var i = 0; i < videoItems.length ; i++)
        {
            var vidItem = videoItems[i]

            //Extract Quality src
            if(vidItem.indexOf("$4K$") !== -1)
            {
                vidItem = vidItem.replace('$4K$', '');
                qualitySrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_4K)
            }
            else if(vidItem.indexOf("$3D$") !== -1)
            {
                vidItem = vidItem.replace('$3D$', '');
                qualitySrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_3D)
            }
            else if(vidItem.indexOf("$HD$") !== -1)
            {
                vidItem = vidItem.replace('$HD$', '');
                qualitySrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_HD)
            }
            else
            {
                vidItem = vidItem.replace('$SD$', '');
                qualitySrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICON_SD)
            }

            //extract res type
            var qualitySizeSrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DVD_QUAL_ICON)
            var vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_576P)
            if(vidItem.indexOf("2160p full") !== -1)
            {
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLURAY_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_2160P)
            }
            else if(vidItem.indexOf("2160p light") !== -1 || vidItem.indexOf("2160p") !== -1)
            {
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLURAY_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_2160P)
            }
            else if(vidItem.indexOf("[3d") !== -1)
            {
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLURAY_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_1080P)
            }
            else if(vidItem.indexOf("1080p full") !== -1)
            {
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLURAY_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_1080P)
            }
            else if(vidItem.indexOf("720p full") !== -1)
            {
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.BLURAY_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_720P)
            }
            else if(vidItem.indexOf("1080p light") !== -1 || vidItem.indexOf("1080p") !== -1)
            {
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LIGHT_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_1080P)
            }
            else if(vidItem.indexOf("720p light") !== -1 || vidItem.indexOf("720p") !== -1)
            {
                qualitySizeSrc =kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LIGHT_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_720P)
            }
            else
            {
                qualitySizeSrc = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DVD_QUAL_ICON)
                vidRes = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RES_576P)
            }

            //extract lang type
            var vidLang = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VF_LANG_ICON)
            if(vidItem.indexOf("MULTI") !== -1)
            {
                currentlang = "multi"
                vidLang = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MULTI_LANG_ICON)
            }
            else if(vidItem.indexOf("[VOSTFR]") !== -1 || vidItem.indexOf("[VOST FR]") !== -1)
            {
                currentlang = "vost"
                vidLang = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VOST_LANG_ICON)
            }
            else if(vidItem.indexOf("[VO]") !== -1 || vidItem.indexOf("[AR]") !== -1)
            {
                currentlang = "vo"
                vidLang = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VO_LANG_ICON)
            }
            else
            {
                currentlang = "vf"
                vidLang = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.VF_LANG_ICON)
            }

            var vidsign = vidItem.split('(').pop().split(')')[0];

            vodLaunchViewItemModel.insert(i, {  'vodTrailerCat' : '',
                                                'vodTrailerName': vodLaunchViewTitleText.text,
                                                'vodTrailerQualitySrc': qualitySrc,
                                                'vodTrailerQualSizeSrc': qualitySizeSrc,
                                                'vidsign': '('+vidsign+')',
                                                'vodRes': vidRes,
                                                'vodLang': vidLang});
        }
        updateVideoItemList();
    }

    function updateVideoItemList()
    {
        showLaunchView();
        if(!enabled) return;
        if(vodLaunchItemList.currentIndex < 0) vodLaunchItemList.currentIndex = 0;
        vodLaunchPrincipalInfoRect.opacity = 0.1;
        if(vodLaunchVideoHideAnim.running == true)
        {
            animationEnabled = false
            vodLaunchVideoImageGlowHideAnim.stop();
            vodLaunchVideoImageHideAnim.stop();
            vodLaunchVideoGlowHideAnim.stop()
            vodLaunchVideoHideAnim.stop()
            vodLaunchVideoPlayImage.opacity = 1
            vodLaunchVideoIconGlow.opacity = 1
            vodLaunchVideoTextGlow.opacity = 1
            vodLaunchVideoText.opacity = 1
        }
        if(vodLaunchVideoShowAnim.running == true)
        {
            animationEnabled = false
            vodLaunchVideoImageGlowShowAnim.stop();
            vodLaunchVideoImageShowAnim.stop();
            vodLaunchVideoGlowShowAnim.stop()
            vodLaunchVideoShowAnim.stop()
            vodLaunchVideoPlayImage.opacity = 1
            vodLaunchVideoIconGlow.opacity = 1
            vodLaunchVideoTextGlow.opacity = 1
            vodLaunchVideoText.opacity = 1
        }
        itemListRect.anchors.bottom = vodLaunchVideoRect.bottom;
        vodLaunchItemList.positionViewAtIndex(0, ListView.Beginning)
        itemListRect.opacity = 0.7
        vodLaunchItemList.opacity = 1;
        vodLaunchItemList.enabled = true;
        vodLaunchItemList.focus = true;
    }

    function updateTrailersItemList()
    {
        showLaunchView();
        if(!enabled) return;
        if(vodLaunchItemList.currentIndex < 0) vodLaunchItemList.currentIndex = 0;
        vodLaunchPrincipalInfoRect.opacity = 0.1;
        if(vodLaunchTrailerHideAnim.running == true)
        {
            animationEnabled = false
            vodLaunchTrailerImageGlowHideAnim.stop();
            vodLaunchTrailerImageHideAnim.stop();
            vodLaunchTrailerGlowHideAnim.stop()
            vodLaunchTrailerHideAnim.stop()
            vodLaunchTrailerPlayIcon.opacity = 1
            vodLaunchTrailerIconGlow.opacity = 1
            vodLaunchTrailerTextGlow.opacity = 1
            vodLaunchTrailerText.opacity = 1
        }
        if(vodLaunchTrailerShowAnim.running == true)
        {
            animationEnabled = false
            vodLaunchTrailerImageGlowShowAnim.stop();
            vodLaunchTrailerImageShowAnim.stop();
            vodLaunchTrailerGlowShowAnim.stop()
            vodLaunchTrailerShowAnim.stop()
            vodLaunchTrailerPlayIcon.opacity = 1
            vodLaunchTrailerIconGlow.opacity = 1
            vodLaunchTrailerTextGlow.opacity = 1
            vodLaunchTrailerText.opacity = 1
        }
        itemListRect.anchors.bottom = vodLaunchTrailerRect.bottom;
        itemListRect.opacity = 0.7
        vodLaunchItemList.opacity = 1;
        vodLaunchItemList.enabled = true;
        vodLaunchItemList.focus = true;
    }

    function hideTrailersItemList()
    {
        if(vodLaunchItemList.currentIndex < 0) vodLaunchItemList.currentIndex = 0;
        vodLaunchPrincipalInfoRect.opacity = 1;
        itemListRect.opacity = 0
        vodLaunchItemList.opacity = 0;
        vodLaunchItemList.enabled = false;
        vodLaunchItemList.focus = false;
        showLaunchView();
    }

    function vodLaunchMetaPaintDisplay(metaData)
    {
        if(metaData.length !== 2)
        {
            logger.error("Problem in PaintDisplay length tab !!!")
            return;
        }

        if(metaData[0] !== "")
        {
            if(metaData[0].indexOf("http") === -1)
            {vodLaunchViewCoverImage.source = "file://"+metaData[0];
            }
            else
            {vodLaunchViewCoverImage.source = metaData[0];
            }
        }
        else vodLaunchViewCoverImage.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_COVER_IMAGE);
        if(metaData[1] !== "")
        {
            vodLaunchBackDoreImage.fillMode = Image.PreserveAspectFit;
            logger.info(metaData[1])
            vodLaunchBackDoreImage.source = metaData[1];
            vodMainView.storeBackGroundImage(metaData[1]);
        }
        else
        {
            vodLaunchBackDoreImage.fillMode = Image.Stretch
            vodLaunchBackDoreImage.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_BACKDORE_PNG)
        }
    }

    function vodLaunchMetaRightRectDisplay(metaData)
    {
        if(metaData.length !== 5)
        {
            logger.error("Problem in right rect length tab !!!")
            return;
        }

        if(metaData[0] !== "") vodLaunchViewTitleText.text =
                                vodMainView.getCurrentSelectedName();
        else vodLaunchViewTitleText.text = "..."

        if(metaData[1] !== "")
        {
            vodLaunchRateTxt.text = "Rating: " + metaData[1];
            drawRatingStars(metaData[1])
        }
        else
        {
            vodLaunchRateTxt.text = "6.2"
            drawRatingStars(6.2)
        }

        if(metaData[2] !== "") vodLaunchPlotValue.text =
                               metaData[2].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchPlotValue.text = "...";

        if(metaData[3] !== "") vodLaunchCastValue.text =
                               metaData[3].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchCastValue.text = "..."

        if(metaData[4] !== "") vodLaunchWriterValue.text = " "+
                               metaData[4].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchWriterValue.text = "..."
    }
    function vodLaunchMetaLeftRectDisplay(metaData)
    {
        if(metaData.length !== 6)
        {
            logger.error("Problem in Left Rect length tab !!!")
            return;
        }
        if(vodMainView.isSeriesFlag) vodLaunchMovieDirectorTitle.text = "Studio: "
        else vodLaunchMovieDirectorTitle.text = "Réalisateur: "

        if(metaData[0] !== "") vodLaunchMovieDirectorValue.text =
                               metaData[0].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchMovieDirectorValue.text = "..."

        if(metaData[1] !== "") vodLaunchMovieGenreValue.text =
                               metaData[1].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchMovieGenreValue.text = "..."

        if(metaData[2] !== "") vodLaunchMovieYearValue.text =
                               metaData[2].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchMovieYearValue.text = "...";

        if(metaData[3] !== "") vodLaunchMoviePremieredValue.text =
                               metaData[3].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchMoviePremieredValue.text = "..."

        if(metaData[4] !== "") vodLaunchMovieVotesValue.text =
                               metaData[4].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchMovieVotesValue.text = "..."

        if(metaData[5] !== "") vodLaunchMovieDurationValue.text =
                               metaData[5].replace(/<(?:.|\n)*?>/gm, '');
        else vodLaunchMovieDurationValue.text = "..."
    }
    function drawRatingStars(rating)
    {
        var starsRef = Math.floor( rating )
        switch (starsRef)
        {
            case 0:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_0)
                break;
            case 1:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_1)
                break;
            case 2:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_2)
                break;
            case 3:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_3)
                break;
            case 4:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_4)
                break;
            case 5:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_5)
                break;
            case 6:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_6)
                break;
            case 7:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_7)
                break;
            case 8:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_8)
                break;
            case 9:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_9)
                break;
            case 10:
                vodLaunchViewRateImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_RATING_10)
                break;
            default:
                break;
        }
    }

    //HighJacked application engine function to run youtube plugin
    function setResUrl(itemInfo)
    {
        if(!itemInfo) return

        if( (itemInfo['mediatype'] === 'video' || itemInfo['mediatype'] === 'audio') &&
            (itemInfo['IsPlayable'] === 'true') )
        {
            mediaPlayer.enableAutoRestart();
            mediaPlayer.setSeekStart(0);
            mediaPlayerView.setCurrentMediaInfo(
                        ('label' in itemInfo) ? itemInfo['label'] : "",
                        ('plot' in itemInfo) ? itemInfo['plot'] : "");

            if(itemInfo['path'] === "dualstream")
            {
                showVideo(itemInfo["videoUri"], itemInfo["audioUri"], true)
            }
            else
            {
                showVideo(itemInfo['path']);
            }
        }
    }
    function appHasStopped() { }
    function appHasStarted() { }

    function endOfCategory(succeeded)
    {
        logger.info("Applicaion succeeded: " + succeeded)
        pyNotificationView.requestProgressDisplay(
                    kvalUiConfigManager.retranslate + qsTr("Vod Notification"),
                    kvalUiConfigManager.retranslate + qsTr("Startup..."), 90);
        if(!succeeded)
        {
            logger.info("succeded failed")
            pyNotificationView.requestAlertDisplay("warning",
                kvalUiConfigManager.retranslate + qsTr("Vod Notification"),
                kvalUiConfigManager.retranslate + qsTr("Trailer Not found, try another video."));
        }
    }

    function vodLaunchOkKeyPressed()
    {
        logger.info("vodLaunchOkKeyPressed")
        if(vodDeadLinkRect.opacity > 0)
        {
            vodDeadLinkRect.opacity = 0
            redKeyGlow.opacity = 0
            notifyDeadLinks(vodMainView.getCurrentSelectedName(),
                            vodMainView.getCurrentSelectedUri());
            return;
        }

        if(vodLaunchItemList.enabled)
        {
            if(!vodLaunchItemList.count) return
            stopCurrentMedia();
            setActivePlayingList( IPTV_Constants.ENUM_ACTIVE_PLAYING_VOD);
            pyNotificationView.requestProgressDisplay(
                        kvalUiConfigManager.retranslate + qsTr("Vod Notification"),
                        kvalUiConfigManager.retranslate + qsTr("Startup..."), 1);

            if(vodLaunchTrailerSelectRect.opacity > 0)
            {
                g_isVodLaunched = false
                setNavLevel( IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL);
                if(!appsView.checkAppById(g_ytAppId))
                {
                    pyNotificationView.requestAlertDisplay("warning",
                    kvalUiConfigManager.retranslate + qsTr("Vod Notification"),
                    kvalUiConfigManager.retranslate + qsTr("Trailers unavailable, you need to install youtube application."));
                    return
                }

                appsView.registerObjApp(vodLaunchView)
                appsEngine.launchApp(g_ytAppId,
                                     "plugin://kval.youtube.app/play/?video_id=" +
                                     vodLaunchViewItemModel.get
                                     (vodLaunchItemList.currentIndex).vodTrailerUrl +
                                     "&stdalone=1")
                return
            }
            else
            {
                logger.info("Make Choice ...")
                g_isVodLaunched = true
                setNavLevel( IPTV_Constants.ENUM_CATEGORIES_VOD_LEVEL);
                hideTrailersItemList();
                launchVideoNotify(vodLaunchItemList.currentIndex);
                return
            }
        }
        else
        {
            if(vodLaunchTrailerSelectRect.opacity > 0)
            {
                animationEnabled = true
                vodLaunchTrailerHideAnim.start();
                vodLaunchTrailerGlowHideAnim.start();
                vodLaunchTrailerImageGlowHideAnim.start();
                vodLaunchTrailerImageHideAnim.start();
                generateTrailerItemList(vodLaunchSetActiveIndex)
            }
            else
            {
                animationEnabled = true
                vodLaunchVideoHideAnim.start();
                vodLaunchVideoGlowHideAnim.start();
                vodLaunchVideoImageGlowHideAnim.start();
                vodLaunchVideoImageHideAnim.start();
                if(vodMainView.checkArVodActivated())
                {launchShahidVodUri(vodMainView.getVodMainCurrentIndex())
                }
                else
                {generateVideoItemList(vodLaunchSetActiveIndex)
                }
            }
        }
        deactivateMainView();
        enabled = false;
        focus = false;
    }
    function vodLaunchUpKeyPressed()
    {
        if(vodLaunchItemList.enabled)
        {
            vodLaunchItemList.decrementCurrentIndex();
            if(!vodLaunchItemList.currentIndex) vodLaunchItemList.positionViewAtBeginning()
        }
        else
        {
            vodLaunchVideoSelectRect.opacity = 0
            vodLaunchVideoIconGlow.opacity = 0
            vodLaunchVideoTextGlow.opacity = 0
            vodLaunchTrailerSelectRect.opacity = 0.8
            vodLaunchTrailerIconGlow.opacity = 1
            vodLaunchTrailerTextGlow.opacity = 1
        }
    }

    function vodLaunchDownKeyPressed()
    {
        if(vodLaunchItemList.enabled)
        {
            vodLaunchItemList.incrementCurrentIndex();
            if(!vodLaunchItemList.currentIndex) vodLaunchItemList.positionViewAtBeginning()
        }
        else
        {
            vodLaunchVideoSelectRect.opacity = 0.8
            vodLaunchVideoIconGlow.opacity = 1
            vodLaunchVideoTextGlow.opacity = 1
            vodLaunchTrailerSelectRect.opacity = 0
            vodLaunchTrailerIconGlow.opacity = 0
            vodLaunchTrailerTextGlow.opacity = 0
        }
    }

    function vodLaunchBackKeyPressed()
    {
        if(vodDeadLinkRect.opacity > 0)
        {
            vodDeadLinkRect.opacity = 0
            redKeyGlow.opacity = 0
            return
        }
        if(vodLaunchItemList.enabled)
        {
            hideTrailersItemList();
            launchVideoNotify(-1);
        }
        else
        {
            hideLaunchView();
            vodMainView.startAnimationBackGround();
            vodMainView.showVodMainView();
        }
    }
    function vodLaunchRedKeyPressed()
    {
        if(vodDeadLinkRect.opacity > 0) return
        vodDeadLinkRect.opacity = 0.8
        redKeyGlow.opacity = 1
        return
    }
    function vodLaunchYellowKeyPressed()
    {
        addMediaToFavorite(vodLaunchSetActiveIndex)
    }

    Keys.onPressed: {
        if(pyNotificationView.checkActiveProgress())
        {
                if (event.key === Qt.Key_Backspace)
                {
                    // @TODO: Use window for questions
//                    pyNotificationView.showQuestionText(
//                    kvalUiConfigManager.retranslate + qsTr("Question"),
//                    kvalUiConfigManager.retranslate + qsTr("Would you like to cancel the current loading ?"),
//                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MESSAGE_POPUP_QUESTION_LOGO))
                }
                event.accepted = true;
                return
        }
        // @TODO: Use window for questions
//        if(pyNotificationView.checkActiveQuestion())
//        {
//            if (event.key === Qt.Key_Backspace) //Back Button
//            {
//                // TODO: Use window
////                pyNotificationView.removeQuestionBanner();
//            }
//            else if (event.key === Qt.Key_O) //Ok Button
//            {
//                vodEngine.abortMovieLoading()
//                // TODO: Use window
////                pyNotificationView.removeQuestionBanner();
//            }
//            event.accepted = true;
//            return;
//        }
        if (event.key === Qt.Key_Up) //Up Button
        {vodLaunchUpKeyPressed();
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {vodLaunchDownKeyPressed();
        }
        else if (event.key === Qt.Key_Right) //Right Button
        {
        }
        else if (event.key === Qt.Key_Left) //Left Button
        {
        }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {vodLaunchBackKeyPressed();
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {vodLaunchOkKeyPressed();
        }
        else if (event.key === Qt.Key_R) //Red Button
        {vodLaunchRedKeyPressed();
        }
        else if (event.key === Qt.Key_Y) //Yellow Button
        {vodLaunchYellowKeyPressed();
        }
        else if(event.key === Qt.Key_H)
        {
            hideLaunchView();
            vodMainView.stopVodMainView();
            pyNotificationView.clearview();
        }
        else if (event.key === Qt.Key_Return ||
                 event.key === Qt.Key_G ||
                 event.key === Qt.Key_B ||
                 event.key === Qt.Key_Escape){}
        else {}
        event.accepted = true;
    }

    Image {
        id: vodLaunchBackDoreImage
        width: parent.width
        height: parent.height
        fillMode: Image.Stretch
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
    Rectangle {
        id: vodLaunchRect
        width: parent.width - (marginBlock*4)
        height: parent.height - (marginBlock*4)
        anchors.centerIn: parent
        color: "black"
        radius: 8
        border.color: "grey"
        border.width: 1
        opacity: 0.3
    }
    Image {
        width:vodLaunchRect.width - 2
        height:vodLaunchRect.height - 2
        anchors.centerIn: vodLaunchRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_VIGNETTE_FULL_ROUNDED)
        opacity: 0.6
    }
    Rectangle {
        id: headerRect
        anchors.left: vodLaunchRect.left
        anchors.right: vodLaunchRect.right
        anchors.top: vodLaunchRect.top
        anchors.topMargin: marginBlock * 2
        height: vodLaunchRect.height * 0.08
        color: "transparent";
        Text{
            id: vodLaunchViewHeaderTxt

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: marginBlock * 8
            color: "white"
            opacity: 0.7
            font {
                family: localFont.name;
                pixelSize: marginBlock*4.5
            }
        }
        Text{
            id: vodLaunchViewHeaderTime
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: marginBlock * 8
            color: "white"
            text: g_currentTimeStdFormat
            font {
                family: localFont.name;
                pixelSize: marginBlock*7
            }
        }
        Text{
            id: vodLaunchViewHeaderDate
            anchors.right: parent.right
            anchors.rightMargin: vodLaunchViewHeaderTime.contentWidth +
                                 marginBlock * 9
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: marginBlock/3
            horizontalAlignment: Text.AlignRight
            color: "white"
            text: g_currentDateVodFormat
            font {
                family: localFont.name;
                pixelSize: marginBlock*2.4
            }
        }
    }
    Rectangle {
        id: separationLine
        anchors.left: headerRect.left
        anchors.right: headerRect.right
        anchors.top: headerRect.bottom
        anchors.leftMargin: marginBlock*8
        anchors.rightMargin: marginBlock*8
        height: 1
        color: "white"
        opacity: 0.3
    }

    Rectangle {
        id: vodLaunchMovieCastRect
        anchors.right: vodLaunchCoverRect.left
        anchors.rightMargin: - (marginBlock*2)
        anchors.bottom: vodLaunchCoverRect.bottom
        anchors.bottomMargin: marginBlock * 3
        width: parent.width * 0.2083
        height: parent.height * 0.3703
        border.color: "white"
        border.width: 1
        radius: marginBlock * 2
        opacity: 0.3
        color: highlightColor
    }
    Text {
        id: vodLaunchMovieDirectorTitle
        anchors.left: vodLaunchMovieCastRect.left
        anchors.leftMargin: marginBlock * 2
        anchors.top: vodLaunchMovieCastRect.top
        anchors.topMargin: marginBlock *2
        width: vodLaunchMovieCastRect.width
        opacity: 1
        text: kvalUiConfigManager.retranslate + qsTr("Réalisateur: ")
        color: "white"
        font {
            family: localFont.name;
            pixelSize: marginBlock*3
        }
        Text {
            id: vodLaunchMovieDirectorValue
            anchors.left: vodLaunchMovieDirectorTitle.left
            anchors.top: vodLaunchMovieDirectorTitle.bottom
            width: vodLaunchMovieDirectorTitle.width
            elide: Text.ElideRight
            text: ""
            color: "white"
            opacity: 0.6
            font {
                family: localFont.name;
                pixelSize: marginBlock*2.6
            }
        }
        Text {
            id: vodLaunchMovieGenreTitle
            anchors.left: vodLaunchMovieDirectorTitle.left
            anchors.top: vodLaunchMovieDirectorValue.bottom
            width: vodLaunchMovieCastRect.width
            opacity: 1
            text: kvalUiConfigManager.retranslate + qsTr("Genre: ")
            color: "white"
            font {
                family: localFont.name;
                pixelSize: marginBlock*3
            }
            Text {
                id: vodLaunchMovieGenreValue
                anchors.left: vodLaunchMovieGenreTitle.left
                anchors.top: vodLaunchMovieGenreTitle.bottom
                width: vodLaunchMovieGenreTitle.width - vodLaunchMovieGenreTitle.contentWidth
                elide: Text.ElideRight
                text: ""
                color: "white"
                opacity: 0.6
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*2.6
                }
            }
        }
        Text {
            id: vodLaunchMovieYearTitle
            anchors.left: vodLaunchMovieGenreTitle.left
            anchors.top: vodLaunchMovieGenreTitle.bottom

            anchors.topMargin: vodLaunchMovieGenreValue.contentHeight
            width: vodLaunchMovieCastRect.width
            opacity: 1
            text: kvalUiConfigManager.retranslate + qsTr("Année: ")
            color: "white"
            font {
                family: localFont.name;
                pixelSize: marginBlock*3
            }
            Text {
                id: vodLaunchMovieYearValue
                anchors.left: parent.left
                anchors.leftMargin: parent.contentWidth
                anchors.bottom: parent.bottom
                elide: Text.ElideRight
                text: ""
                color: "white"
                opacity: 0.6
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*2.6
                }
            }
        }
        Text {
            id: vodLaunchMoviePremieredTitle
            anchors.left: vodLaunchMovieYearTitle.left
            anchors.top: vodLaunchMovieYearTitle.bottom
            anchors.topMargin: marginBlock*3
            width: vodLaunchMovieCastRect.width
            opacity: 1
            text: kvalUiConfigManager.retranslate + qsTr("Date de sortie: ")
            color: "white"
            font {
                family: localFont.name;
                pixelSize: marginBlock*3
            }
            Text {
                id: vodLaunchMoviePremieredValue
                anchors.left: parent.left
                anchors.leftMargin: parent.contentWidth
                anchors.bottom: parent.bottom
                elide: Text.ElideRight
                text: ""
                color: "white"
                opacity: 0.6
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*2.6
                }
            }
        }
        Text {
            id: vodLaunchMovieVotesTitle
            anchors.left: vodLaunchMoviePremieredTitle.left
            anchors.top: vodLaunchMoviePremieredTitle.bottom
            width: vodLaunchMovieCastRect.width
            opacity: 1
            text: kvalUiConfigManager.retranslate + qsTr("Votes: ")
            color: "white"
            font {
                family: localFont.name;
                pixelSize: marginBlock*3
            }
            Text {
                id: vodLaunchMovieVotesValue
                anchors.left: parent.left
                anchors.leftMargin: parent.contentWidth
                anchors.bottom: parent.bottom
                elide: Text.ElideRight
                text: ""
                color: "white"
                opacity: 0.6
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*2.6
                }
            }
        }
        Text {
            id: vodLaunchMovieDurationTitle
            anchors.left: vodLaunchMovieVotesTitle.left
            anchors.top: vodLaunchMovieVotesTitle.bottom
            width: vodLaunchMovieCastRect.width
            opacity: 1
            text: kvalUiConfigManager.retranslate + qsTr("Durée: ")
            color: "white"
            font {
                family: localFont.name;
                pixelSize: marginBlock*3
            }
            Text {
                id: vodLaunchMovieDurationValue
                anchors.left: parent.left
                anchors.leftMargin: parent.contentWidth
                anchors.bottom: parent.bottom
                elide: Text.ElideRight
                text: ""
                color: "white"
                opacity: 0.6
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*2.6
                }
            }
        }
    }


    Item {
        id: vodLaunchCoverRect
        anchors.left: vodLaunchRect.left
        anchors.leftMargin: marginBlock * 45
        anchors.top: vodLaunchRect.top
        anchors.topMargin: headerRect.height + marginBlock * 10
        width: parent.width * 0.1781
        height: parent.height * 0.475
        Image{
            width: parent.width + (marginBlock*4) + (marginBlock*0.4)
            height: parent.height + (marginBlock*4)
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.POSTER_FRAME)
        }
        Item {
            width: parent.width
            height: parent.height
            anchors.centerIn: parent
            Image{
                id: vodLaunchViewCoverImage
                width: parent.width
                height: parent.height
                fillMode: Image.Stretch
                smooth: true
                visible: false
                onStatusChanged:
                {
                    if (vodLaunchViewCoverImage.status === Image.Error)
                    {source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.DEFAULT_COVER_IMAGE)
                    }
                }
            }
            Image {
                id: vodLaunchViewCovermask
                width: parent.width
                height: parent.height
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.POSTER_FRAME_MASK)
                smooth: true
                visible: false
            }
            OpacityMask {
                id:opacityMskCover
                anchors.fill: vodLaunchViewCoverImage
                source: vodLaunchViewCoverImage
                maskSource: vodLaunchViewCovermask
            }
        }
    }
    Item {
        id: vodLaunchPrincipalInfoRect
        anchors.left: vodLaunchCoverRect.right
        anchors.leftMargin: marginBlock * 12
        anchors.top: vodLaunchCoverRect.top
        anchors.topMargin: -marginBlock*5
        height: parent.height * 0.8 - marginBlock * 3
        width: rootRectangle.width -
               (vodLaunchCoverRect.width + vodLaunchCoverRect.x) -
               (marginBlock * 20)

        Text{
            id: vodLaunchViewTitleText
            width: vodLaunchPrincipalInfoRect.width - (marginBlock * 2)
            anchors.left: parent.left
            anchors.leftMargin: marginBlock * 2
            anchors.top: parent.top
            anchors.topMargin: - marginBlock
            horizontalAlignment: Text.AlignLeft
            color: "white"
            elide: Text.ElideRight
            textFormat: Text.RichText
            font {
                family: localFont.name;
                pixelSize: marginBlock*5.4
            }
        }
        Rectangle {
            id: vodLaunchRateRect
            anchors.left: vodLaunchViewTitleText.left
            anchors.top: vodLaunchViewTitleText.bottom
            width: marginBlock*25.2
            height: (marginBlock*4.8)
            color: "transparent"
            Image {
                id: vodLaunchViewRateImg
                anchors.fill: parent
            }
            Text {
                id: vodLaunchRateTxt
                anchors.left: vodLaunchViewRateImg.right
                anchors.leftMargin: marginBlock
                anchors.bottom: vodLaunchViewRateImg.bottom
                horizontalAlignment: Text.AlignRight
                color: "white"
                text: ""
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*2.4
                }
            }
        }
        Text{
            id: vodLaunchPlotTitle
            anchors.left: vodLaunchRateRect.left
            anchors.top: vodLaunchRateRect.bottom
            anchors.topMargin: marginBlock*2
            horizontalAlignment: Text.AlignLeft
            color: "#0489B1"
            text: kvalUiConfigManager.retranslate + qsTr("Synopsis et détails: ")
            font {
                family: localFontCaps.name;
                pixelSize: marginBlock*4.2
            }
        }
        Glow {
            anchors.fill: vodLaunchPlotTitle
            radius: 5
            opacity: 1
            samples: 16
            spread: 0.5
            color: "#0489B1"
            transparentBorder: true
            source: vodLaunchPlotTitle
        }
        Text{
            id: vodLaunchPlotValue
            anchors.left: vodLaunchPlotTitle.left
            anchors.top: vodLaunchPlotTitle.bottom
            width: parent.width - marginBlock * 2
            horizontalAlignment: Text.AlignJustify
            maximumLineCount: 7
            elide: Text.ElideRight
            color: "white"
            text: ""
            wrapMode: Text.WordWrap
            font {
                family: localFontLow.name;
                pixelSize: marginBlock*3.2
            }
        }
        Text{
            id: vodLaunchCastTitle
            anchors.left: vodLaunchPlotValue.left
            anchors.top: vodLaunchPlotValue.bottom
            anchors.topMargin: marginBlock*3
            horizontalAlignment: Text.AlignLeft
            color: "#0489B1"
            text: kvalUiConfigManager.retranslate + qsTr("Cast: ")
            font {
                family: localFontCaps.name;
                pixelSize: marginBlock*4.2
            }
        }
        Glow {
            anchors.fill: vodLaunchCastTitle
            radius: 5
            opacity: 1
            samples: 16
            spread: 0.5
            color: "#0489B1"
            transparentBorder: true
            source: vodLaunchCastTitle
        }
        Text{
            id: vodLaunchCastValue
            anchors.left: vodLaunchCastTitle.left
            anchors.top: vodLaunchCastTitle.bottom
            width: parent.width - marginBlock * 2
            horizontalAlignment: Text.AlignLeft
            maximumLineCount:4
            elide: Text.ElideRight
            color: "white"
            text: ""
            wrapMode: Text.WordWrap
            font {
                family: localFontLow.name;
                pixelSize: marginBlock*3.2
            }
        }
        Text{
            id: vodLaunchWriterTitle
            anchors.left: vodLaunchPlotValue.left
            anchors.top: vodLaunchCastValue.bottom
            anchors.topMargin: marginBlock*3
            horizontalAlignment: Text.AlignLeft
            maximumLineCount:3
            color: "#0489B1"
            text: kvalUiConfigManager.retranslate + qsTr("Writer: ")
            font {
                family: localFontCaps.name;
                pixelSize: marginBlock*4.2
            }
        }
        Glow {
            anchors.fill: vodLaunchWriterTitle
            radius: 5
            opacity: 1
            samples: 16
            spread: 0.5
            color: "#0489B1"
            transparentBorder: true
            source: vodLaunchWriterTitle
        }
        Text{
            id: vodLaunchWriterValue
            anchors.left: vodLaunchWriterTitle.left
            anchors.top: vodLaunchWriterTitle.bottom
            width: parent.width - marginBlock * 2
            horizontalAlignment: Text.AlignLeft
            maximumLineCount: 1
            color: "white"
            text: ""
            wrapMode: Text.WordWrap
            font {
                family: localFontLow.name;
                pixelSize: marginBlock*3.2
            }
        }
    }
    /*-------------------------------------------------------------------------
    |                   Launch trailer rect
    -------------------------------------------------------------------------*/
    Image {
        anchors.centerIn: vodLaunchTrailerRect
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_BANNER_VOD_CHOICE)
    }

    Rectangle {
        id: vodLaunchTrailerRect
        anchors.top: vodLaunchCoverRect.bottom
        anchors.right: vodLaunchCoverRect.right
        anchors.topMargin: marginBlock
        width: vodLaunchCoverRect.width
        height: marginBlock*4
        color: "transparent"
        border.color: "white"
        border.width: 1
        radius: 5
        opacity: 0.9
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#585858" }
            GradientStop { position: 1.0; color: "black" }
        }
    }
    Rectangle {
        id: vodLaunchTrailerSelectRect
        anchors.fill: vodLaunchTrailerRect
        border.color: "white"
        border.width: 1
        radius: 5
        opacity: 0.9
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#58D3F7" }
            GradientStop { position: 1.0; color: "#0B4C5F" }
        }
    }
    Glow {
        id: vodLaunchTrailerIconGlow
        anchors.fill: vodLaunchTrailerPlayIcon
        radius: 8
        opacity: 1
        samples: 16
        spread: 0.6
        color: "white"
        transparentBorder: true
        source: vodLaunchTrailerPlayIcon
        NumberAnimation
        {
            id: vodLaunchTrailerImageGlowHideAnim
            target: vodLaunchTrailerIconGlow;
            property: "opacity";
            from: 1; to: 0.1; duration: 200
        }
        NumberAnimation
        {
            id: vodLaunchTrailerImageGlowShowAnim
            target: vodLaunchTrailerIconGlow;
            property: "opacity";
            from: 0; to: 1; duration: 200
        }
    }
    Image{
        id: vodLaunchTrailerPlayIcon
        anchors.left: vodLaunchTrailerRect.left
        anchors.leftMargin: marginBlock * 4
        anchors.verticalCenter: vodLaunchTrailerRect.verticalCenter
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_START_VIDEO_V1)
        NumberAnimation
        {
            id: vodLaunchTrailerImageHideAnim
            target: vodLaunchTrailerPlayIcon;
            property: "opacity";
            from: 1; to: 0; duration: 200
        }
        NumberAnimation
        {
            id: vodLaunchTrailerImageShowAnim
            target: vodLaunchTrailerPlayIcon;
            property: "opacity";
            from: 0; to: 1; duration: 200
        }
    }
    Glow {
        id: vodLaunchTrailerTextGlow
        anchors.fill: vodLaunchTrailerText
        radius: 5
        opacity: 1
        samples: 20
        spread: 0.5
        color: "white"
        transparentBorder: true
        source: vodLaunchTrailerText
        NumberAnimation
        {
            id: vodLaunchTrailerGlowHideAnim
            target: vodLaunchTrailerTextGlow;
            property: "opacity";
            from: 1; to: 0; duration: 200
        }
        NumberAnimation
        {
            id: vodLaunchTrailerGlowShowAnim
            target: vodLaunchTrailerTextGlow;
            property: "opacity";
            from: 0; to: 1; duration: 200
        }
    }
    Text{
        id: vodLaunchTrailerText
        anchors.horizontalCenter: vodLaunchTrailerRect.horizontalCenter
        anchors.horizontalCenterOffset: marginBlock
        anchors.verticalCenter: vodLaunchTrailerRect.verticalCenter
        anchors.verticalCenterOffset: -(marginBlock*0.2)
        color: "white"
        text: kvalUiConfigManager.retranslate + qsTr("Lancer Le Trailer")
        font {
            family: localFont.name;
            pixelSize: marginBlock*3
        }
        NumberAnimation
        {
            id: vodLaunchTrailerHideAnim
            target: vodLaunchTrailerText;
            property: "opacity";
            from: 1; to: 0; duration: 200
            onStopped:
            {
                if(animationEnabled)
                {
                    vodLaunchTrailerGlowShowAnim.start()
                    vodLaunchTrailerShowAnim.start()
                    vodLaunchTrailerImageGlowShowAnim.start();
                    vodLaunchTrailerImageShowAnim.start();
                }
            }
        }
        NumberAnimation
        {
            id: vodLaunchTrailerShowAnim
            target: vodLaunchTrailerText;
            property: "opacity";
            from: 0; to: 1; duration: 200
            onStopped:
            {
                if(animationEnabled)
                {
                    vodLaunchTrailerGlowHideAnim.start()
                    vodLaunchTrailerHideAnim.start()
                    vodLaunchTrailerImageGlowHideAnim.start();
                    vodLaunchTrailerImageHideAnim.start();
                }
            }
        }
    }
    /*-------------------------------------------------------------------------
    |                   Launch Video rect
    -------------------------------------------------------------------------*/
    Image {
        anchors.centerIn: vodLaunchVideoRect
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_BANNER_VOD_CHOICE)
    }
    Rectangle {
        id: vodLaunchVideoRect
        anchors.top: vodLaunchTrailerRect.bottom
        anchors.right: vodLaunchTrailerRect.right
        anchors.topMargin: marginBlock
        width: vodLaunchTrailerRect.width
        height: marginBlock*4
        color: "transparent"
        border.color: "white"
        border.width: 1
        radius: 5
        opacity: 0.9
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#585858" }
            GradientStop { position: 1.0; color: "black" }
        }
    }
    Rectangle {
        id: vodLaunchVideoSelectRect
        anchors.fill: vodLaunchVideoRect
        border.color: "white"
        border.width: 1
        radius: 5
        opacity: 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#58D3F7" }
            GradientStop { position: 1.0; color: "#0B4C5F" }
        }
    }
    Glow {
        id: vodLaunchVideoIconGlow
        anchors.fill: vodLaunchVideoPlayImage
        radius: marginBlock*0.8
        opacity: 0
        samples: marginBlock*1.6
        spread: marginBlock*0.06
        color: "white"
        transparentBorder: true
        source: vodLaunchVideoPlayImage
        NumberAnimation
        {
            id: vodLaunchVideoImageGlowHideAnim
            target: vodLaunchVideoIconGlow;
            property: "opacity";
            from: 1; to: 0.1; duration: 200
        }
        NumberAnimation
        {
            id: vodLaunchVideoImageGlowShowAnim
            target: vodLaunchVideoIconGlow;
            property: "opacity";
            from: 0; to: 1; duration: 200
        }
    }
    Image{
        id: vodLaunchVideoPlayImage
        anchors.left: vodLaunchVideoRect.left
        anchors.leftMargin: marginBlock * 4
        anchors.verticalCenter: vodLaunchVideoRect.verticalCenter
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.ICO_START_VIDEO_V1)
        NumberAnimation
        {
            id: vodLaunchVideoImageHideAnim
            target: vodLaunchVideoPlayImage;
            property: "opacity";
            from: 1; to: 0; duration: 200
        }
        NumberAnimation
        {
            id: vodLaunchVideoImageShowAnim
            target: vodLaunchVideoPlayImage;
            property: "opacity";
            from: 0; to: 1; duration: 200
        }
    }
    Glow {
        id: vodLaunchVideoTextGlow
        anchors.fill: vodLaunchVideoText
        radius: marginBlock*0.5
        opacity: 0
        samples: marginBlock*2
        spread: marginBlock*0.05
        color: "white"
        transparentBorder: true
        source: vodLaunchVideoText
        NumberAnimation
        {
            id: vodLaunchVideoGlowHideAnim
            target: vodLaunchVideoTextGlow;
            property: "opacity";
            from: 1; to: 0; duration: 200
        }
        NumberAnimation
        {
            id: vodLaunchVideoGlowShowAnim
            target: vodLaunchVideoTextGlow;
            property: "opacity";
            from: 0; to: 1; duration: 200
        }
    }
    Text{
        id: vodLaunchVideoText
        anchors.horizontalCenter: vodLaunchVideoRect.horizontalCenter
        anchors.horizontalCenterOffset: marginBlock
        anchors.verticalCenter: vodLaunchVideoRect.verticalCenter
        anchors.verticalCenterOffset: -(marginBlock*0.2)
        color: "white"
        text: kvalUiConfigManager.retranslate + qsTr("Lancer Le Film...")
        font {
            family: localFont.name;
            pixelSize: marginBlock*3
        }
        NumberAnimation
        {
            id: vodLaunchVideoHideAnim
            target: vodLaunchVideoText;
            property: "opacity";
            from: 1; to: 0; duration: 200
            onStopped:
            {
                if(animationEnabled)
                {
                    vodLaunchVideoGlowShowAnim.start()
                    vodLaunchVideoShowAnim.start()
                    vodLaunchVideoImageGlowShowAnim.start();
                    vodLaunchVideoImageShowAnim.start();
                }
            }
        }
        NumberAnimation
        {
            id: vodLaunchVideoShowAnim
            target: vodLaunchVideoText;
            property: "opacity";
            from: 0; to: 1; duration: 200
            onStopped:
            {
                if(animationEnabled)
                {
                    vodLaunchVideoGlowHideAnim.start()
                    vodLaunchVideoHideAnim.start()
                    vodLaunchVideoImageGlowHideAnim.start();
                    vodLaunchVideoImageHideAnim.start();
                }
            }
        }
    }
    /*-------------------------------------------------------------------------
    |                   Back To List Rect
    -------------------------------------------------------------------------*/
    Rectangle {
        id: bottomZone
        anchors.bottom: vodLaunchRect.bottom
        anchors.left: vodLaunchRect.left
        width: vodLaunchRect.width * 0.4
        height: vodLaunchRect.height * 0.1
        color: "transparent"
        Image{
            id: bottomZoneKeyRed
            x: marginBlock * 2
            width: marginBlock*4.5
            anchors.bottom: parent.bottom
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_RED_BUTTON)
            Text{
                id: redKeyTxt
                anchors.left: parent.right
                anchors.leftMargin: marginBlock
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock/2
                text: kvalUiConfigManager.retranslate + qsTr("Problème Lancement Vidéo")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*3.5
                }
            }
            Glow {
                id: redKeyGlow
                anchors.fill: redKeyTxt
                radius: 5
                opacity: 0
                samples: 16
                spread: 0.5
                color: "white"
                transparentBorder: true
                source: redKeyTxt
            }
        }
        Image{
            id: bottomZoneKeyBlue
            anchors.left: bottomZoneKeyRed.right
            anchors.leftMargin: marginBlock*2.5 + redKeyTxt.contentWidth
            anchors.bottom: parent.bottom
            width: marginBlock*4.5
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_YELLOW_BUTTON)
            Text{
                anchors.left: parent.right
                anchors.leftMargin: marginBlock/2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock/2
                text: kvalUiConfigManager.retranslate + qsTr("Ajouter au favoris")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: marginBlock*3.5
                }
            }
        }
    }
    Rectangle {
        id: itemListRect
        width: parent.width * 0.425
        height: parent.height * 0.555
        anchors.bottom: vodLaunchTrailerRect.bottom
        anchors.left: vodLaunchTrailerRect.right
        anchors.leftMargin: marginBlock*2
        color: "black"
        opacity: 0
        radius: 8
        border.color: "white"
        border.width: 1
        Image {
            width:parent.width - 2
            height:parent.height - 2
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_VIGNETTE_FULL_ROUNDED)
            opacity: 0.7
        }
    }
    Rectangle {
        id: itemListRectScrollZone
        width: parent.width * 0.425
        height: parent.height * 0.52
        anchors.centerIn: itemListRect
        color: "transparent"
    }
    Rectangle {
        id: itemListRectScrollZoneRefs
        x: parent.width * 0.4375
        y: parent.height * 0.2269
        width: parent.width * 0.365
        height: parent.height * 0.5
        color: "transparent"
    }

    Component {
        id : mainVodItemListComponent

        Item {
            id: wrapper
            width: itemListRectScrollZone.width
            height: (vodTrailerCat === "") ?
                        itemListRectScrollZone.height/10 :
                        (itemListRectScrollZone.height/10) * 2
            Item {
                id: vodTrailerCatTicket
                anchors.left: parent.left
                anchors.leftMargin: marginBlock * 1.5
                anchors.top: parent.top
                width: parent.width
                height: (vodTrailerCat === "") ? 0 : parent.height/2
                visible: (vodTrailerCat === "") ? false : true

                Text {
                    id: vodTrailerCatTxt
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: marginBlock
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "#0489B1"
                    text: vodTrailerCat
                    font {
                      family: localAdventFont.name;
                      pixelSize: marginBlock*3
                    }
                }
                Glow {
                    anchors.fill: vodTrailerCatTxt
                    radius: 8
                    opacity: 1
                    samples: 17
                    spread: 0.1
                    color: "#0489B1"
                    transparentBorder: true
                    source: vodTrailerCatTxt
                }
            }

            Item {
                id: videoInfosItem

                anchors.left: parent.left
                anchors.top: (vodTrailerCat === "") ? parent.top : vodTrailerCatTicket.bottom
                width: parent.width * 0.4
                height: (vodTrailerCat === "") ? parent.height : parent.height/2
                Image {
                    id: qualImg
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width * 0.25
                    height: parent.height * 0.8
                    fillMode: Image.PreserveAspectFit
                    source: vodTrailerQualitySrc
                }

                Image {
                    id: qualSizeImg
                    anchors.left: qualImg.right
                    anchors.leftMargin: -marginBlock
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width * 0.25
                    height: parent.height * 0.7
                    fillMode: Image.PreserveAspectFit
                    source: vodTrailerQualSizeSrc
                }

                Image {
                    id: resImg
                    anchors.left: qualSizeImg.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width * 0.25
                    height: parent.height * 0.7
                    fillMode: Image.PreserveAspectFit
                    source: vodRes
                }

                Image {
                    anchors.left: resImg.right
                    anchors.leftMargin: -marginBlock*0.5
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width * 0.25
                    height: parent.height * 0.7
                    fillMode: Image.PreserveAspectFit
                    source: vodLang
                }
            }
            Image {
                id: signImage
                anchors.left: videoInfosItem.right
                anchors.leftMargin: -marginBlock*2
                anchors.verticalCenter: videoInfosItem.verticalCenter
                scale: 0.6
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.GTIP_POINT)
            }
            Item {
                id: textTitleRect
                anchors.left: parent.left
                anchors.leftMargin: videoInfosItem.width + (marginBlock * 1.5)
                anchors.right: parent.right
                anchors.rightMargin: marginBlock * 2
                anchors.top: (vodTrailerCat === "") ? parent.top : vodTrailerCatTicket.bottom
                height: (vodTrailerCat === "") ? parent.height : parent.height/2
                Text {
                    id: vidsigntext
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    text: vidsign
                    font {
                      family: localAdventFont.name
                      bold: true
                      pixelSize: marginBlock*1.7
                    }
                }
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: (vidsign==="") ? marginBlock : marginBlock * 3.5
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: vodTrailerName
                    font {
                      family: localAdventFont.name
                      pixelSize: marginBlock*3.2
                    }
                }
            }
        }
    }
    ListModel {
         id: vodLaunchViewItemModel
         ListElement {
             vodTrailerCat : ""
             vodTrailerName: ""
             vodTrailerUrl: ""
             vodTrailerQualitySrc: ""
             vodTrailerQualSizeSrc: ""
             vidsign: ""
             vodRes: ""
             vodLang: ""
         }
     }
    Rectangle {
        id: listViewRect
        anchors.fill: itemListRectScrollZone
        color: "transparent"
        opacity: vodLaunchItemList.opacity

        ListView {
            id: vodLaunchItemList
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: marginBlock * 1.5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock
            enabled: false
            opacity: 0
            focus: false
            Rectangle {
                color: 'white'
                anchors.top: vodLaunchItemList.top
                anchors.horizontalCenter:scrollInternal.horizontalCenter
                opacity: 0.1
                width: 1
                height: (parent.contentHeight > parent.height) ? parent.height : 0
            }
            ScrollBar.vertical: ScrollBar {
                id: scrollInternal
                active: true
                anchors.top: vodLaunchItemList.top
                anchors.right: vodLaunchItemList.right
                anchors.rightMargin: marginBlock
                anchors.bottom: vodLaunchItemList.bottom
            }

            model: vodLaunchViewItemModel
            highlight: highlightVodLaunchItem
            highlightFollowsCurrentItem: false
            clip: true
            keyNavigationWraps: true
            interactive: false
            delegate: mainVodItemListComponent
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

    Component {
        id: highlightVodLaunchItem

        Item {
            id: highlightRect
            anchors.bottom: vodLaunchItemList.currentItem.bottom
            anchors.bottomMargin: marginBlock * 0.3
            width: itemListRect.width - (marginBlock*3)
            height: itemListRectScrollZone.height/10
            Image {
                x: parent.x
                y: parent.y
                width: itemListRect.width - (marginBlock*1.5)
                anchors.top: parent.top
                anchors.topMargin: -marginBlock
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -marginBlock
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
    Rectangle {
        id: vodDeadLinkRect
        width: marginBlock*64
        height: marginBlock*15
        anchors.left: bottomZone.left
        anchors.leftMargin: (marginBlock * 5) + (marginBlock/2)
        anchors.bottom: bottomZone.top
        anchors.bottomMargin: -(marginBlock * 4)
        color: "black";
        border.color: "grey"
        radius:5
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1C1C1C" }
            GradientStop { position: 1.0; color: "black" }
        }
        border.width: 1
        opacity: 0
        Text{
            id: vodDeadLinkExplainText
            x: marginBlock
            horizontalAlignment: Text.AlignLeft
            width: parent.width - marginBlock
            color: "white"
            wrapMode: Text.WordWrap
            textFormat: Text.StyledText
            font {
                bold: true
                family: localFont.name;
                pixelSize: marginBlock*2.6
            }
            text: kvalUiConfigManager.retranslate + qsTr("Veuillez vous assurer que tous les liens concernant ce programme ne fonctionnent plus avant d'envoyer toute notification ..."+
                  "<br>Appuyer sur <font color=\"#0489B1\" ><b>'Ok'</b></font> pour envoyer et <font color=\"#0489B1\"><b>'Back'</b></font> pour revenir.")
        }
    }
}
