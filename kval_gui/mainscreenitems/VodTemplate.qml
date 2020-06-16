import QtQuick 2.3
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item {
    id: vodFirstTemplate
    objectName: "VodTemplateForm1"

    signal activateVodViewNotify();
    property real marginFirstTemplate: marginBlock/2 + marginBlock/3
    property var gripPointTab:
        [kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.GTIP_POINT_FIRST),
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.GTIP_POINT_MIDDLE),
         kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.GTIP_POINT_LAST)]
    property int g_movieImageCurrentIndex: 0
    property int g_tvShowImageCurrentIndex: 0
    property int g_mangasImageCurrentIndex: 0
    property var movieNamesArray: []
    property var movieImageArray: []
    property var movieRatesArray: []
    property var tvShowNamesArray: []
    property var tvShowImageArray: []
    property var tvShowRatesArray: []
    property var mangasNamesArray: []
    property var mangasImageArray: []
    property var mangasRatesArray: []
    property var currentObj: null

    x: mainScreenView.mainChoiceZone().width + (marginBlock*7)
    y: mainScreenView.mainViewHeaderRect().height + (marginBlock*11)
    width: rootRectangle.width * 0.65
    height: mainScreenView.mainChoiceZone().height * 0.75+ marginFirstTemplate*2

    function activateRelatedView()
    {
        activateVodViewNotify();
    }

    function activateTemplate()
    {
        visible = true
        activateAnimations()
        startVodBackDoreAlternateAnimation()
    }

    function checkViewReady()
    {
        return true
    }

    function deactivateTemplate(obj)
    {
        currentObj = obj
        vodFirstTemplate.stopVodBackDoreAlternateAnimation()
        vodFirstTemplate.deactivateAnimations()
        visible = false
    }

    function cleanUp()
    {
        tvShowBackDoreImg1.source = ""
        tvShowBackDoreImg2.source = ""
        movieBackDoreImg1.source = ""
        movieBackDoreImg2.source = ""
        mangasBackDoreImg1.source = ""
        mangasBackDoreImg2.source = ""
        g_movieImageCurrentIndex = 0
        g_tvShowImageCurrentIndex = 0
        g_mangasImageCurrentIndex = 0
        stopVodBackDoreAlternateAnimation()
    }

    function startVodBackDoreAlternateAnimation()
    {
        mainScreenAlternateBackDoreTimer.start()
        mainScreenAlternateBackDoreTimer.repeat = true
        mainScreenAlternateTvShowBackDoreTimer.start()
        mainScreenAlternateTvShowBackDoreTimer.repeat = true
        mainScreenAlternateMangasBackDoreTimer.start()
        mainScreenAlternateMangasBackDoreTimer.repeat = true
    }
    function stopVodBackDoreAlternateAnimation()
    {
        mainScreenAlternateBackDoreTimer.running = false
        mainScreenAlternateBackDoreTimer.repeat = false
        mainScreenAlternateBackDoreTimer.stop()
        mainScreenAlternateTvShowBackDoreTimer.running = false
        mainScreenAlternateTvShowBackDoreTimer.repeat = false
        mainScreenAlternateTvShowBackDoreTimer.stop()
        mainScreenAlternateMangasBackDoreTimer.running = false
        mainScreenAlternateMangasBackDoreTimer.repeat = false
        mainScreenAlternateMangasBackDoreTimer.stop()
    }

    function activateAnimations()
    {
        animateMovieBackDoreOpacityOn.start()
    }

    function deactivateAnimations()
    {
        animateMoviesInfoBackDoreOpacityOff.start()
        animateTvShowsInfoBackDoreOpacityOff.start()
        animateMangasInfoBackDoreOpacityOff.start()
    }

    function customInfosReady()
    {
        movieImageArray = []
        movieNamesArray = []
        movieRatesArray = []
        var i, j = 0

        var vodTopLeftZone = kvalUiGuiUtils.getvodCustomTopLeftZoneInfo()
        if(vodTopLeftZone.length === 0) return
        leftBigVodRectLabel.text = vodTopLeftZone[0]
        leftBigVodRectLabel.color = vodTopLeftZone[1]
        for(i = 2, j = 0 ; i < vodTopLeftZone.length ; i++, j++)
        {
            movieNamesArray[j] = vodTopLeftZone[i]
            i += 1
            vodMovieTitle.color = vodTopLeftZone[i]
            i += 1
            movieImageArray[j] = vodTopLeftZone[i]
            i += 1
            movieRatesArray[j] = vodTopLeftZone[i]
            i += 1
            ratingMovieTxt.color = vodTopLeftZone[i]
        }

        var vodBottomLeftZone = kvalUiGuiUtils.getvodCustomBottomLeftZoneInfo()
        bottomLeftVodRectLabel.text = vodBottomLeftZone[0]
        bottomLeftVodRectLabel.color = vodBottomLeftZone[1]
        for(i = 2, j = 0 ; i < vodBottomLeftZone.length ; i++, j++)
        {
            tvShowNamesArray[j] = vodBottomLeftZone[i]
            i += 1
            tvShowsTitleTxt.color = vodBottomLeftZone[i]
            i += 1
            tvShowImageArray[j] = vodBottomLeftZone[i]
            i += 1
            tvShowRatesArray[j] = vodBottomLeftZone[i]
            i += 1
            ratingTvShowTxt.color = vodBottomLeftZone[i]
        }

        var vodBottomMiddleZone = kvalUiGuiUtils.getvodCustombottomMiddleZoneInfo()
        bottomRightVodRectLabel.text = vodBottomMiddleZone[0]
        bottomRightVodRectLabel.color = vodBottomMiddleZone[1]
        for(i = 2, j = 0 ; i < vodBottomMiddleZone.length ; i++, j++)
        {
            mangasNamesArray[j] = vodBottomMiddleZone[i]
            i += 1
            mangasTitleTxt.color = vodBottomMiddleZone[i]
            i += 1
            mangasImageArray[j] = vodBottomMiddleZone[i]
            i += 1
            mangasRatesArray[j] = vodBottomMiddleZone[i]
            i += 1
            ratingMangasShowTxt.color = vodBottomMiddleZone[i]
        }

        var vodTopRightZone = kvalUiGuiUtils.getvodCustomTopRightZoneInfo()
        topLeftBackImg.source = vodTopRightZone[0]
        overlayImgMovies.source = vodTopRightZone[1]
        topLeftIconImg.source = vodTopRightZone[2]
        moviesTitleTxt.text = vodTopRightZone[3]
        moviesTitleTxt.color = vodTopRightZone[4]
        moviesNbrTxt.text = vodTopRightZone[5]
        moviesNbrTxt.color = vodTopRightZone[6]

        var vodMiddleRightZone = kvalUiGuiUtils.getvodCustomMiddleRightZoneInfo()
        rightMiddleVodBackImg.source = vodMiddleRightZone[0]
        overlayImgTvShows.source = vodMiddleRightZone[1]
        tvShowsIconImg.source = vodMiddleRightZone[2]
        tvShowsNbrTitleTxt.text = vodMiddleRightZone[3]
        tvShowsNbrTitleTxt.color = vodMiddleRightZone[4]
        tvShowsNbrTxt.text = vodMiddleRightZone[5]
        tvShowsNbrTxt.color = vodMiddleRightZone[6]

        var vodBottomRightZone = kvalUiGuiUtils.getvodCustomBottomRightZoneInfo()
        rightBottomVodBackImg.source = vodBottomRightZone[0]
        overlayImgMangas.source = vodBottomRightZone[1]
        mangasIcon.text = vodBottomRightZone[2]
        mangasIcon.color = vodBottomRightZone[3]
        mangasNbrTitleTxt.text = vodBottomRightZone[4]
        mangasNbrTitleTxt.color = vodBottomRightZone[5]
        mangasNbrTxt.text = vodBottomRightZone[6]
        mangasNbrTxt.color = vodBottomRightZone[7]

        updateMediaInfos()
    }

    function updateMediaInfos()
    {
        fillRecentMoviesInfos()
        fillRecentTvShowInfos()
        fillRecentMangasInfos()
    }

    function fillRecentMoviesInfos()
    {
        //Prefill movie infos
        if (movieImageArray.length === 0) return

        movieBackDoreImg1.source = movieImageArray[g_movieImageCurrentIndex] ?
                                   movieImageArray[g_movieImageCurrentIndex] : ""

        vodMovieTitle.text = movieNamesArray[g_movieImageCurrentIndex] ?
                             movieNamesArray[g_movieImageCurrentIndex] : ""

        ratingMovieTxt.text = movieRatesArray[g_movieImageCurrentIndex] ?
                              movieRatesArray[g_movieImageCurrentIndex] : ""

        g_movieImageCurrentIndex = g_movieImageCurrentIndex+1

        movieBackDoreImg2.source = movieImageArray[g_movieImageCurrentIndex] ?
                                   movieImageArray[g_movieImageCurrentIndex] : ""
    }
    function fillRecentTvShowInfos()
    {
        if (tvShowImageArray.length === 0) return
        //Prefill movie infos
        tvShowBackDoreImg1.source = tvShowImageArray[0]
        tvShowsTitleTxt.text = tvShowNamesArray[0]
        ratingTvShowTxt.text = tvShowRatesArray[0]
        tvShowBackDoreImg2.source = tvShowImageArray[1]
    }
    function fillRecentMangasInfos()
    {
        if (mangasImageArray.length === 0) return
        //Prefill movie infos
        mangasBackDoreImg1.source = mangasImageArray[0]
        mangasTitleTxt.text = mangasNamesArray[0]
        ratingMangasShowTxt.text = mangasRatesArray[0]
        mangasBackDoreImg2.source = mangasImageArray[1]
    }


    Timer {
        id: mainScreenAlternateBackDoreTimer
        interval: 3000;
        running: false;
        repeat: true;
        onTriggered: {
            if(!movieBackDoreImg1.x)
            {
                animateMovieBackDoreImg1Off.start()
                animateMovieBackDoreImg2On.start()
            }
            else
            {
                animateMovieBackDoreImg2Off.start()
                animateMovieBackDoreImg1On.start()
            }
        }
    }
    Timer {
        id: mainScreenAlternateTvShowBackDoreTimer
        interval: 3500;
        running: false;
        repeat: true;
        onTriggered: {
            if(!tvShowBackDoreImg1.x)
            {
                animateTvShowBackDoreImg1Off.start()
                animateTvShowBackDoreImg2On.start()
            }
            else
            {
                animateTvShowBackDoreImg2Off.start()
                animateTvShowBackDoreImg1On.start()
            }
        }
    }
    Timer {
        id: mainScreenAlternateMangasBackDoreTimer
        interval: 2500;
        running: false;
        repeat: true;
        onTriggered: {
            if(!mangasBackDoreImg1.x)
            {
                animateMangasBackDoreImg1Off.start()
                animateMangasBackDoreImg2On.start()
            }
            else
            {
                animateMangasBackDoreImg2Off.start()
                animateMangasBackDoreImg1On.start()
            }
        }
    }

    Text {
        id: leftBigVodRectLabel
        anchors.left: leftBigVodRect.left
        anchors.leftMargin: marginFirstTemplate
        anchors.bottom: leftBigVodRect.top
        anchors.bottomMargin: marginFirstTemplate/2
        style: Text.Raised
        styleColor: "black"
        opacity: 0
        font {
            family: localFontLow.name
            pixelSize: leftBigVodRect.width * 0.04
        }
    }
    Item {
        id: leftBigVodRect
        anchors.top: parent.top
        anchors.left: parent.left
        width: 0
        height: (parent.height/3)*2 + marginFirstTemplate
        clip: true
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.6
        }
        NumberAnimation {
               id: animateMovieBackDoreOpacityOn
               target: leftBigVodRect
               properties: "width"
               from: 0
               to: vodFirstTemplate.width * 0.65 + marginFirstTemplate
               duration: 20
               easing {type: Easing.OutBack; overshoot: 0.8 }
               onStopped:{
                   animateMoviesInfoBackDoreOpacityOn.start()
                   animateTvShowsInfoBackDoreOpacityOn.start()
                   animateMangasInfoBackDoreOpacityOn.start()
                   animateTvShowBackDoreOpacityOn.start()
               }
               onStarted: {
                   ratingMovieTxt.opacity = 1
                   vodMovieTitle.opacity = 1
                   leftBigVodRectLabel.opacity = 1
               }
        }
        NumberAnimation {
               id: animateMovieBackDoreOpacityOff
               target: leftBigVodRect
               properties: "width"
               from: vodFirstTemplate.width * 0.65 + marginFirstTemplate
               to: 0
               duration: 20
               easing {type: Easing.InBack; overshoot: 1.5 }
               onStarted: {
                   ratingMovieTxt.opacity = 0
                   vodMovieTitle.opacity = 0
                   leftBigVodRectLabel.opacity = 0
               }
               onStopped: {
                   mainScreenView.activateZone(currentObj)
                   vodFirstTemplate.visible = false
               }
        }
        Image {
            id: movieBackDoreImg1
            width: parent.width
            height: parent.height
            fillMode: Image.Stretch
            NumberAnimation {
                   id: animateMovieBackDoreImg1On
                   target: movieBackDoreImg1
                   properties: "x"
                   from: -leftBigVodRect.width
                   to: 0
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStarted: {
                       vodMovieTitle.text = movieNamesArray[g_movieImageCurrentIndex]
                       ratingMovieTxt.text = movieRatesArray[g_movieImageCurrentIndex]
                       gripPointSelected.source = gripPointTab[g_movieImageCurrentIndex]
                   }
            }
            NumberAnimation {
                   id: animateMovieBackDoreImg1Off
                   target: movieBackDoreImg1
                   properties: "x"
                   from: 0
                   to: leftBigVodRect.width
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStopped:{
                       g_movieImageCurrentIndex= (g_movieImageCurrentIndex === (movieImageArray.length-1)) ? 0 : g_movieImageCurrentIndex+1;
                       movieBackDoreImg1.source = movieImageArray[g_movieImageCurrentIndex]
                   }
            }
        }
        Image {
            id: movieBackDoreImg2
            x: -leftBigVodRect.width
            width: leftBigVodRect.width
            height: parent.height
            fillMode: Image.Stretch
            NumberAnimation {
                   id: animateMovieBackDoreImg2On
                   target: movieBackDoreImg2
                   properties: "x"
                   from: -leftBigVodRect.width
                   to: 0
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStarted: {
                       vodMovieTitle.text = movieNamesArray[g_movieImageCurrentIndex]
                       ratingMovieTxt.text = movieRatesArray[g_movieImageCurrentIndex]
                       gripPointSelected.source = gripPointTab[g_movieImageCurrentIndex]
                   }
            }
            NumberAnimation {
                   id: animateMovieBackDoreImg2Off
                   target: movieBackDoreImg2
                   properties: "x"
                   from: 0
                   to: leftBigVodRect.width
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStopped:{
                        g_movieImageCurrentIndex= (g_movieImageCurrentIndex === (movieImageArray.length-1)) ? 0 : g_movieImageCurrentIndex+1;
                        movieBackDoreImg2.source = movieImageArray[g_movieImageCurrentIndex]
                   }
            }
        }

        Image {
            id: gripPointSelected

            anchors.top: parent.top
            anchors.topMargin: marginBlock/2
            anchors.right: parent.right
            anchors.rightMargin: marginBlock/2
            source: gripPointTab[g_movieImageCurrentIndex]
        }

        Image {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.width * 0.07
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_RSS_INFO_BAR)
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "transparent"
                height: parent.height * 0.8
                clip: true
                Text {
                    id: vodMovieTitle
                    y: -marginFirstTemplate - marginFirstTemplate/3
                    anchors.left: parent.left
                    anchors.leftMargin: marginFirstTemplate
                    opacity: 0
                    font {
                        family: localFont.name
                        pixelSize: parent.width * 0.05
                    }
                }
                Text {
                    id:ratingMovieTxt
                    anchors.right: parent.right
                    anchors.rightMargin: marginBlock
                    anchors.bottom: vodMovieTitle.bottom
                    anchors.bottomMargin: marginBlock/3
                    opacity: 0
                    font {
                        family: localFont.name
                        pixelSize: parent.width * 0.04
                    }
                }
                Image {
                    id: starRate
                    anchors.right: ratingMovieTxt.left
                    anchors.rightMargin: marginBlock/2
                    anchors.bottom: vodMovieTitle.bottom
                    anchors.bottomMargin: marginBlock/2 + marginBlock/3
                    width: parent.width * 0.04
                    height: parent.width * 0.04
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.STAR_RATE)
                }
            }
        }
    }

    Image {
        width: leftBigVodRect.width + leftBigVodRect.width*0.13
        height: leftBigVodRect.height + leftBigVodRect.height*0.13
        anchors.centerIn: leftBigVodRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Text {
        id: bottomLeftVodRectLabel
        anchors.left: bottomLeftVodRect.left
        anchors.leftMargin: marginFirstTemplate
        anchors.top: bottomLeftVodRect.bottom
        anchors.topMargin: marginFirstTemplate/2
        style: Text.Raised
        styleColor: "black"
        opacity: 0
        font {
            family: localFontLow.name
            pixelSize: leftBigVodRect.width * 0.04
        }
    }

    Item {
        id: bottomLeftVodRect
        anchors.top: leftBigVodRect.bottom
        anchors.topMargin: marginFirstTemplate
        anchors.left: parent.left
        width: 0
        height: parent.height/3
        clip:true

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.6
        }
        NumberAnimation {
               id: animateTvShowBackDoreOpacityOn
               target: bottomLeftVodRect
               properties: "width"
               from: 0
               to: vodFirstTemplate.width * (0.65/2)
               duration: 20
               easing {type: Easing.OutBack ; period: 1.5 }
               onStopped: animateMangaBackDoreOpacityOn.start()
               onStarted: {
                   tvShowsTitleTxt.opacity = 1
                   ratingTvShowTxt.opacity = 1
                   bottomLeftVodRectLabel.opacity = 1
               }
        }
        NumberAnimation {
               id: animateTvShowBackDoreOpacityOff
               target: bottomLeftVodRect
               properties: "width"
               from: vodFirstTemplate.width * (0.65/2)
               to: 0
               duration: 20
               easing {type: Easing.InBack; overshoot: 1 }
               onStarted: {
                   tvShowsTitleTxt.opacity = 0
                   ratingTvShowTxt.opacity = 0
                   bottomLeftVodRectLabel.opacity = 0
               }
        }
        Image {
            id: tvShowBackDoreImg1
            width: bottomLeftVodRect.width
            height: parent.height
            fillMode: Image.Stretch
            NumberAnimation {
                   id: animateTvShowBackDoreImg1On
                   target: tvShowBackDoreImg1
                   properties: "x"
                   from: -bottomLeftVodRect.width
                   to: 0
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStarted:
                   {
                       tvShowsTitleTxt.text = tvShowNamesArray[g_tvShowImageCurrentIndex]
                       ratingTvShowTxt.text = tvShowRatesArray[g_tvShowImageCurrentIndex]
                       tvShowGripPointSelected.source = gripPointTab[g_tvShowImageCurrentIndex]
                   }
            }
            NumberAnimation {
                   id: animateTvShowBackDoreImg1Off
                   target: tvShowBackDoreImg1
                   properties: "x"
                   from: 0
                   to: bottomLeftVodRect.width
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStopped:{
                       g_tvShowImageCurrentIndex= (g_tvShowImageCurrentIndex === (tvShowImageArray.length-1)) ? 0 : g_tvShowImageCurrentIndex+1;
                       tvShowBackDoreImg1.source = tvShowImageArray[g_tvShowImageCurrentIndex]
                }
            }
        }
        Image {
            id: tvShowBackDoreImg2
            x: -bottomLeftVodRect.width
            width: bottomLeftVodRect.width
            height: parent.height
            fillMode: Image.Stretch
            NumberAnimation {
                   id: animateTvShowBackDoreImg2On
                   target: tvShowBackDoreImg2
                   properties: "x"
                   from: -bottomLeftVodRect.width
                   to: 0
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStarted:
                   {
                       tvShowsTitleTxt.text = tvShowNamesArray[g_tvShowImageCurrentIndex]
                       ratingTvShowTxt.text = tvShowRatesArray[g_tvShowImageCurrentIndex]
                       tvShowGripPointSelected.source = gripPointTab[g_tvShowImageCurrentIndex]
                   }
            }
            NumberAnimation {
                   id: animateTvShowBackDoreImg2Off
                   target: tvShowBackDoreImg2
                   properties: "x"
                   from: 0
                   to: bottomLeftVodRect.width
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStopped:{
                       g_tvShowImageCurrentIndex= (g_tvShowImageCurrentIndex === (tvShowImageArray.length-1)) ? 0 : g_tvShowImageCurrentIndex+1;
                       tvShowBackDoreImg2.source = tvShowImageArray[g_tvShowImageCurrentIndex]
                }
            }
        }

        Image {
            id: tvShowGripPointSelected

            anchors.top: parent.top
            anchors.topMargin: marginBlock/2
            anchors.right: parent.right
            anchors.rightMargin: marginBlock/2
            source: gripPointTab[g_tvShowImageCurrentIndex]
        }
        Image {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.width * 0.1
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_RSS_INFO_BAR)
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "transparent"
                height: parent.height * 0.8
                clip: true
                Text {
                    id: tvShowsTitleTxt
                    y: -marginFirstTemplate/3
                    anchors.left: parent.left
                    anchors.leftMargin: marginFirstTemplate
                    color: "white"
                    opacity: 0
                    font {
                        family: localFont.name
                        pixelSize: parent.width * 0.06
                    }
                }
                Text {
                    id:ratingTvShowTxt
                    anchors.right: parent.right
                    anchors.rightMargin: marginBlock
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: marginBlock/10
                    color: "white"
                    opacity:0
                    font {
                        family: localFont.name
                        pixelSize: parent.width * 0.06
                    }
                }
                Image {
                    id: tvShowStarRate
                    anchors.right: ratingTvShowTxt.left
                    anchors.rightMargin: marginBlock/4
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: marginBlock/2
                    width: parent.width * 0.06
                    height: parent.width * 0.06
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.STAR_RATE)
                }
            }
        }
    }
    Image {
        width: bottomLeftVodRect.width *1.13
        height: bottomLeftVodRect.height *1.13
        anchors.centerIn: bottomLeftVodRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Text {
        id: bottomRightVodRectLabel
        anchors.left: bottomRightVodRect.left
        anchors.leftMargin: marginFirstTemplate
        anchors.top: bottomRightVodRect.bottom
        anchors.topMargin: marginFirstTemplate/2
        style: Text.Raised
        styleColor: "black"
        opacity: 0
        font {
            family: localFontLow.name
            pixelSize: leftBigVodRect.width * 0.04
        }
    }

    Item {
        id: bottomRightVodRect
        anchors.top: leftBigVodRect.bottom
        anchors.topMargin: marginFirstTemplate
        anchors.left: bottomLeftVodRect.right
        anchors.leftMargin: marginFirstTemplate
        width: 0
        height: parent.height/3
        clip: true

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.6
        }
        NumberAnimation {
               id: animateMangaBackDoreOpacityOn
               target: bottomRightVodRect
               properties: "width"
               from: 0
               to: vodFirstTemplate.width * (0.65/2)
               duration: 50
               easing {type: Easing.OutElastic ; period: 1.0 }
               onStarted: {
                   mangasTitleTxt.opacity = 1
                   ratingMangasShowTxt.opacity = 1
                   bottomRightVodRectLabel.opacity = 1
               }
        }
        NumberAnimation {
               id: animateMangaBackDoreOpacityOff
               target: bottomRightVodRect
               properties: "width"
               from: vodFirstTemplate.width * (0.65/2)
               to: 0
               duration: 50
               easing {type: Easing.InBack ; overshoot: 1.5 }
               onStarted: {
                   mangasTitleTxt.opacity = 0
                   ratingMangasShowTxt.opacity = 0
                   bottomRightVodRectLabel.opacity = 0
               }
        }
        Image {
            id: mangasBackDoreImg1
            width: bottomRightVodRect.width
            height: parent.height
            fillMode: Image.Stretch
            NumberAnimation {
                   id: animateMangasBackDoreImg1On
                   target: mangasBackDoreImg1
                   properties: "x"
                   from: -bottomRightVodRect.width
                   to: 0
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStarted:
                   {
                       mangasTitleTxt.text = mangasNamesArray[g_mangasImageCurrentIndex]
                       ratingMangasShowTxt.text = tvShowRatesArray[g_mangasImageCurrentIndex]
                       mangasGripPointSelected.source = gripPointTab[g_mangasImageCurrentIndex]
                   }
            }
            NumberAnimation {
                   id: animateMangasBackDoreImg1Off
                   target: mangasBackDoreImg1
                   properties: "x"
                   from: 0
                   to: bottomRightVodRect.width
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStopped:{
                       g_mangasImageCurrentIndex= (g_mangasImageCurrentIndex === (mangasImageArray.length-1)) ? 0 : g_mangasImageCurrentIndex+1;
                       mangasBackDoreImg1.source = mangasImageArray[g_mangasImageCurrentIndex]
                }
            }
        }
        Image {
            id: mangasBackDoreImg2
            x: -bottomRightVodRect.width
            width: bottomRightVodRect.width
            height: parent.height
            fillMode: Image.Stretch
            NumberAnimation {
                   id: animateMangasBackDoreImg2On
                   target: mangasBackDoreImg2
                   properties: "x"
                   from: -bottomRightVodRect.width
                   to: 0
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStarted:
                   {
                       mangasTitleTxt.text = mangasNamesArray[g_mangasImageCurrentIndex]
                       ratingMangasShowTxt.text = tvShowRatesArray[g_mangasImageCurrentIndex]
                       mangasGripPointSelected.source = gripPointTab[g_mangasImageCurrentIndex]
                   }
            }
            NumberAnimation {
                   id: animateMangasBackDoreImg2Off
                   target: mangasBackDoreImg2
                   properties: "x"
                   from: 0
                   to: bottomRightVodRect.width
                   duration: 500
                   easing {type: Easing.OutBounce; period: 1 }
                   onStopped:{
                       g_mangasImageCurrentIndex= (g_mangasImageCurrentIndex === (mangasImageArray.length-1)) ? 0 : g_mangasImageCurrentIndex+1;
                       mangasBackDoreImg2.source = mangasImageArray[g_mangasImageCurrentIndex]
                }
            }
        }
        Image {
            id: mangasGripPointSelected

            anchors.top: parent.top
            anchors.topMargin: marginBlock/2
            anchors.right: parent.right
            anchors.rightMargin: marginBlock/2
            source: gripPointTab[g_mangasImageCurrentIndex]
        }
        Image {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.width * 0.1
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_RSS_INFO_BAR)
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "transparent"
                height: parent.height * 0.8
                clip: true
                Text {
                    id: mangasTitleTxt
                    y: -marginFirstTemplate/3
                    anchors.left: parent.left
                    anchors.leftMargin: marginBlock
                    opacity: 0
                    font {
                        family: localFont.name
                        pixelSize: parent.width * 0.06
                    }
                }
                Text {
                    id:ratingMangasShowTxt
                    anchors.right: parent.right
                    anchors.rightMargin: marginBlock
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: marginBlock/10
                    opacity: 0
                    font {
                        family: localFont.name
                        pixelSize: parent.width * 0.06
                    }
                }
                Image {
                    anchors.right: ratingMangasShowTxt.left
                    anchors.rightMargin: marginBlock/4
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: marginBlock/2
                    width: parent.width * 0.06
                    height: parent.width * 0.06
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.STAR_RATE)
                }
            }
        }
    }
    Image {
        width: bottomRightVodRect.width *1.13
        height: bottomRightVodRect.height *1.13
        anchors.centerIn: bottomRightVodRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Rectangle {
        id: rightTopVodRect
        anchors.top: leftBigVodRect.top
        anchors.left: leftBigVodRect.right
        anchors.leftMargin: marginFirstTemplate
        width: 0
        height: parent.height/3
        color: "transparent"
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.8
        }
        NumberAnimation {
               id: animateMoviesInfoBackDoreOpacityOn
               target: rightTopVodRect
               properties: "width"
               from: 0
               to: vodFirstTemplate.width - leftBigVodRect.width
               duration: 20
               easing {type: Easing.OutElastic ; period: 1.5 }
               onStarted:
               {
                   moviesNbrTxt.opacity = 1
                   moviesTitleTxt.opacity = 1
               }
        }
        NumberAnimation {
               id: animateMoviesInfoBackDoreOpacityOff
               target: rightTopVodRect
               properties: "width"
               from: vodFirstTemplate.width - leftBigVodRect.width
               to: 0
               duration: 20
               easing {type: Easing.InBack ; overshoot: 1.5 }
               onStarted: {
               }
               onStopped: {
                   moviesNbrTxt.opacity = 0
                   moviesTitleTxt.opacity = 0
                   animateMangaBackDoreOpacityOff.start()
                   animateTvShowBackDoreOpacityOff.start()
                   animateMovieBackDoreOpacityOff.start()
               }
        }
        Image {
            id: topLeftBackImg
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.width * 0.08
            fillMode: Image.Stretch
            rotation: 180
        }
        Image {
            id: overlayImgMovies
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            rotation: 180
            opacity: 0.7
        }
        Image {
            id: topLeftIconImg
            width: parent.width*0.6
            height: parent.width*0.6
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: marginFirstTemplate
            anchors.horizontalCenter: parent.horizontalCenter
            fillMode: Image.Stretch
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.width * 0.08
            color: "transparent"
            Text {
                id: moviesTitleTxt
                anchors.left: parent.left
                anchors.leftMargin: marginBlock
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -marginFirstTemplate/2
                opacity:0
                font {
                    family: localFont.name
                    pixelSize: parent.width * 0.06
                }
            }
            Text {
                id: moviesNbrTxt
                anchors.right: parent.right
                anchors.rightMargin: marginBlock
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -marginFirstTemplate/2
                opacity:0
                font {
                    family: localFont.name
                    pixelSize: parent.width * 0.06
                }
            }
        }
    }
    Image {
        width: rightTopVodRect.width + rightTopVodRect.width*0.13
        height: rightTopVodRect.height + rightTopVodRect.height*0.13
        anchors.centerIn: rightTopVodRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Rectangle {
        id: rightMiddleVodRect
        anchors.top: rightTopVodRect.bottom
        anchors.topMargin: marginFirstTemplate
        anchors.left: rightTopVodRect.left
        width: 0
        height: rightTopVodRect.height
        color: "transparent"
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.8
        }
        NumberAnimation {
               id: animateTvShowsInfoBackDoreOpacityOn
               target: rightMiddleVodRect
               properties: "width"
               from: 0
               to: vodFirstTemplate.width - leftBigVodRect.width
               duration: 20
               easing {type: Easing.OutBack ; overshoot: 0.8 }
               onStarted: {
                   tvShowsNbrTitleTxt.opacity = 1
                   tvShowsNbrTxt.opacity = 1
               }
        }
        NumberAnimation {
               id: animateTvShowsInfoBackDoreOpacityOff
               target: rightMiddleVodRect
               properties: "width"
               from: vodFirstTemplate.width - leftBigVodRect.width
               to: 0
               duration: 20
               easing {type: Easing.InBack ; overshoot: 1.5 }
               onStopped:{
                   tvShowsNbrTxt.opacity = 0
                   tvShowsNbrTitleTxt.opacity = 0
               }
        }
        Image {
            id: rightMiddleVodBackImg
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.width * 0.08
            fillMode: Image.Stretch
        }
        Image {
            id: overlayImgTvShows
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            rotation: 180
            opacity: 0.7
        }
        Image {
            id: tvShowsIconImg
            width: parent.width*0.6
            height: parent.width*0.6
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: marginFirstTemplate
            anchors.horizontalCenter: parent.horizontalCenter
            fillMode: Image.Pad
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.width * 0.08
            color: "transparent"
            Text {
                id: tvShowsNbrTitleTxt
                anchors.left: parent.left
                anchors.leftMargin: marginBlock
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -marginFirstTemplate/2
                opacity: 0
                font {
                    family: localFont.name
                    pixelSize: parent.width * 0.06
                }
            }
            Text {
                id: tvShowsNbrTxt
                anchors.right: parent.right
                anchors.rightMargin: marginBlock
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -marginFirstTemplate/2
                opacity: 0
                font {
                    family: localFont.name
                    pixelSize: parent.width * 0.06
                }
            }
        }
    }
    Image {
        width: rightMiddleVodRect.width + rightMiddleVodRect.width*0.13
        height: rightMiddleVodRect.height + rightMiddleVodRect.height*0.13
        anchors.centerIn: rightMiddleVodRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Rectangle {
        id: rightBottomVodRect
        anchors.top: rightMiddleVodRect.bottom
        anchors.topMargin: marginFirstTemplate
        anchors.left: rightTopVodRect.left
        width: 0
        height: rightTopVodRect.height
        color: "transparent"
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.8
        }
        NumberAnimation {
               id: animateMangasInfoBackDoreOpacityOn
               target: rightBottomVodRect
               properties: "width"
               from: 0
               to: vodFirstTemplate.width - leftBigVodRect.width
               duration: 20
               easing {type: Easing.OutElastic ; period: 1.5 }
               onStopped: {
                   mangasNbrTitleTxt.opacity = 1
                   mangasNbrTxt.opacity = 1
                   mangasIcon.opacity = 1
               }
        }
        NumberAnimation {
               id: animateMangasInfoBackDoreOpacityOff
               target: rightBottomVodRect
               properties: "width"
               from: vodFirstTemplate.width - leftBigVodRect.width
               to: 0
               duration: 20
               easing {type: Easing.InBack ; overshoot: 1 }
               onStopped:{
                   mangasNbrTitleTxt.opacity = 0
                   mangasNbrTxt.opacity = 0
                   mangasIcon.opacity = 0
               }
        }
        Image {
            id: rightBottomVodBackImg
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.width * 0.08
            fillMode: Image.Stretch
        }
        Image {
            id: overlayImgMangas
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            fillMode: Image.Stretch
            rotation: 180
            opacity: 0.7
        }
        Text {
            id: mangasIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -marginFirstTemplate*2
            anchors.horizontalCenter: parent.horizontalCenter
            style: Text.Raised
            opacity: 0
            styleColor: "black"
            font {
                family: localMangasFont.name
                pixelSize: parent.width * 0.18
            }
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.width * 0.08
            color: "transparent"
            Text {
                id: mangasNbrTitleTxt
                anchors.left: parent.left
                anchors.leftMargin: marginBlock
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -marginFirstTemplate/2
                opacity: 0
                font {
                    family: localFont.name
                    pixelSize: parent.width * 0.06
                }
            }
            Text {
                id: mangasNbrTxt
                anchors.right: parent.right
                anchors.rightMargin: marginBlock
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -marginFirstTemplate/2
                opacity: 0
                font {
                    family: localFont.name
                    pixelSize: parent.width * 0.06
                }
            }
        }
    }
    Image {
        width: rightBottomVodRect.width + rightBottomVodRect.width*0.13
        height: rightBottomVodRect.height + rightBottomVodRect.height*0.13
        anchors.centerIn: rightBottomVodRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
}

