import QtQuick 2.3
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0

Item
{
    id: mainScreenViewItem
    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height
    enabled: false
    opacity: 0
    focus: false

    //Signals
    signal focusRelinquished()
    signal mainScreenReadyNotify()
    signal displayVodScreenNotifiy()
    signal displayAppsScreenNotify()
    signal displayMailBoxScreenNotify()
    signal activateVodViewNotify()
    signal activateLiveTvViewNotify()
    signal activateLiveSportViewNotify()
    signal mailListExtractNotify()
    signal mediaBrowserActivateNotify()
    signal displayMediaBrowserScreenNotify()
    signal displaySettingsScreenNotify()
    signal settingsActivateNotify()

    //Define Local variables
    property bool g_appsViewReady: false
    property bool g_vodViewReady: false
    property bool g_channelViewReady: false
    property bool g_mainScreenFirstBoot: false
    property real marginVodFirstTemplate: marginBlock/2 + marginBlock/3
    property bool isVodInfoZoneEnabled: false
    property bool isSearchZoneEnabled: true
    property bool g_mainScreenVodListAvailable: false
    property string unseenMailNbr: '0'
    property string totalMailNbr: '0'
    property var g_supportedtemplates: []
    property bool g_screenReloaded: false
    property int g_previousIndex: 0
    property var g_back_images: ({  0:backgroundImg1,
                                    1:backgroundImg2,
                                    2:backgroundImg3,
                                    3:backgroundImg4,
                                    4:backgroundImg5,
                                    5:backgroundImg6,
                                    6:backgroundImg7,
                                    7:backgroundImg8})

    property var g_templates: ({"searchFirstTemplate": searchFirstTemplate,
                                "vodFirstTemplate":vodFirstTemplate,
                                "liveTvFirstTemplate":liveTvFirstTemplate,
                                "appsFirstTemplate":appsFirstTemplate,
                                "localMediaFirstTemplate":localMediaFirstTemplate,
                                "mailBoxFirstTemplate":mailBoxFirstTemplate,
                                "settingsFirstTemplate":settingsFirstTemplate})

    function get_template(template_idx)
    {
        var template_name = mainScreenViewModel.get(template_idx).template
        if (g_templates[template_name])
            return g_templates[template_name]

        return null
    }

    function mainScreenUsbDeviceDisconnected()
    {
        localMediaFirstTemplate.disconnectedDevice()
    }

    function mainScreenUsbDeviceConnected()
    {
        localMediaFirstTemplate.connectedDevice()
    }

    function mainScreenMailBoxInfosNotify(mailBoxInfos)
    {
        mailBoxFirstTemplate.infosNotify(mailBoxInfos)
        unseenMailNbr = mailBoxInfos[0]
        totalMailNbr = mailBoxInfos[1]
        if (unseenMailNbr !== '0')
        {
            topMsgUnseenMailsTxt.text = unseenMailNbr+" messages non lues"
            topMsgUnseenMailsIconAnimOn.start()
            topMsgUnseenMailsTxtAnimOn.start()
        }
        else
        {
            mainScreenUnseenMailActualizeTimer.stop()
            mainScreenUnseenMailActualizeTimer.repeat = false
            mainScreenUnseenMailActualizeTimer.running = false
            topMsgUnseenMailsIcon.opacity = 0
            topMsgUnseenMailsTxt.text = ""
            topMsgUnseenMailsTxt.opacity = 0
        }
    }

    function gatherAllMainScreenInfos()
    {
        reloadBackImgs()
    }

    function vodScreenReady()
    {
        mainScreenScaleOff()
        displayVodScreenNotifiy()
    }
    function appsScreenReady()
    {
        mainScreenScaleOff()
        displayAppsScreenNotify()
    }

    function mailBoxReady()
    {
        mainScreenScaleOff()
        displayMailBoxScreenNotify()
    }

    function onMediaBrowserViewReady()
    {
        mainScreenScaleOff()
        displayMediaBrowserScreenNotify()
    }
    function onSettingsViewReady()
    {
        mainScreenScaleOff()
        displaySettingsScreenNotify()
    }

    function mainScreenScaleOff()
    {
        mainScreenViewScaleAnimationOff.start()
    }

    function displayServerNotificationMessage(severity, message)
    {
        var messageText = ""
        if( severity.toLowerCase() ===
            kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_HIGH_SEVERITY_SERVER_MESSAGE))
        {
            messageText =   '<font color="#6aa6cb">' +
                            'Message Important: '+
                            '</font>'
        }
        else if (severity.toLowerCase() ===
                 kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VERY_HIGH_SEVERITY_SERVER_MESSAGE))
        {
            messageText =   '<font color="red">' +
                            'Message Très Important: '+
                            '</font>'
        }

        rssText.text = messageText + message
        rssTextAnim.start()
        rssText.opacity = 1
    }

    function hideServerNotificationMessage()
    {
        rssText.opacity = 0
        rssTextAnim.stop()
    }

    function mainScreenHasBooted()
    {
        return g_mainScreenFirstBoot;
    }

    function readyToDisplayMainScreen()
    {
        if(!g_mainScreenFirstBoot)
        {
            logger.info("mainScreenReadyNotify ...");
            g_mainScreenFirstBoot = true
            vodMainView.preloadVodBacks()
            mainScreenReadyNotify()
        }
    }

    function uiReady()
    {
        //Fill of all custom areas
        liveTvFirstTemplate.customInfosReady()
        vodFirstTemplate.customInfosReady()
        readyToDisplayMainScreen()
    }

    function customInfosUpdated()
    {
        liveTvFirstTemplate.customInfosReady()
        vodFirstTemplate.customInfosReady()
        xmlMainScreenCategoryModel.reload()
        g_previousIndex = mainScreenViewList.currentIndex
        g_screenReloaded = true
    }

    function activateMainScreenView()
    {
        registerActiveView(mainScreenView)
        logger.info("activateMainScreenView()")
        reloadBackImgs()
        switch(mainScreenViewList.currentIndex)
        {
            case IPTV_Constants.ENUM_MAIN_SCREEN_VOD_CATEGORY:
                vodFirstTemplate.startVodBackDoreAlternateAnimation()
                break
            case IPTV_Constants.ENUM_MAIN_SCREEN_LIVETV_CATEGORY:
                liveTvFirstTemplate.startLiveTvAlternateAnimation()
                break
            case IPTV_Constants.ENUM_MAIN_SCREEN_LOCAL_MEDIA_CATEGORY:
                break
            default:
                break
        }
        updateMainScreenList()
        mainScreenViewScaleAnimationOn.start()
    }

    function popUpTryGetFocus()
    {
        if(!enabled)
        {
            return false
        }
        searchFirstTemplate.opacity = 0.3
        vodFirstTemplate.opacity = 0.3
        liveTvFirstTemplate.opacity = 0.3
        localMediaFirstTemplate.opacity = 0.3
        mailBoxFirstTemplate.opacity = 0.3
        settingsFirstTemplate.opacity = 0.3
        appsFirstTemplate.opacity = 0.3
        enabled = false
        focus = false
        return true
    }
    function popUpFocusRelinquished()
    {
        searchFirstTemplate.opacity = 1
        vodFirstTemplate.opacity = 1
        liveTvFirstTemplate.opacity = 1
        localMediaFirstTemplate.opacity = 1
        mailBoxFirstTemplate.opacity = 1
        settingsFirstTemplate.opacity = 1
        appsFirstTemplate.opacity = 1
        activeFocusViewManager(mainScreenView)
    }

    function newVodTitleAvailable()
    {g_mainScreenVodListAvailable = true
    }

    function updateMainScreenList()
    {
        if(g_mainScreenVodListAvailable)
        {
            g_mainScreenVodListAvailable = false
            var ret = serverNotificationEngine.updateVodFiles()
            if(!ret)
            {kvalUiGuiUtils.updateCustomsList();
            }
        }
        else
        {
            vodFirstTemplate.updateMediaInfos()
        }
        mainScreenViewList.opacity = 1;
        mainScreenViewList.enabled = true;
        mainScreenViewList.focus = true;
    }
    function reloadBackImgs()
    {
        for(var i = 0; i < mainScreenViewList.count ; i++)
        {
            g_back_images[i].source = mainScreenViewModel.get(i).backJacket
        }
    }
    function cleanUpAllocatedObj()
    {
        var currentBackImg=""
        for(var i = 0; i < mainScreenViewList.count ; i++)
        {
            g_back_images[i].source = ''
        }
        vodFirstTemplate.cleanUp()
        hideServerNotificationMessage()
        mainScreenUnseenMailActualizeTimer.stop()
        mainScreenUnseenMailActualizeTimer.repeat = false
        mainScreenUnseenMailActualizeTimer.running = false
    }

    function hideMainScreenView() {
        opacity = 0;
        focus = false;
        enabled = false;
        cleanUpAllocatedObj()
    }

    function hideMainListView() {
        enabled = false;
        focus = false;
    }

    function highlightDownActionSelectedCategory()
    {
        mainScreenViewModel.setProperty(mainScreenViewList.currentIndex,
                                "shiftY",
                                -marginBlock*3);
        mainScreenViewModel.setProperty(mainScreenViewList.currentIndex-1,
                                "shiftY",
                                -marginBlock*6);
    }

    function highlightUpActionSelectedCategory()
    {
        mainScreenViewModel.setProperty(mainScreenViewList.currentIndex,
                                "shiftY",
                                -marginBlock*3);
        mainScreenViewModel.setProperty(mainScreenViewList.currentIndex+1,
                                        "shiftY",
                                        0);
    }
    function backgroundAnimateDown()
    {
        backGroundImageOpacityDownOff.target = g_back_images[mainScreenViewList.currentIndex-1]
        backGroundImageYDownOn.target = g_back_images[mainScreenViewList.currentIndex]
        backGroundImageOpacityDownOff.start()
    }
    function backgroundAnimateUp()
    {
        backGroundImageOpacityUpOff.target = g_back_images[mainScreenViewList.currentIndex+1]
        backGroundImageYUpOn.target = g_back_images[mainScreenViewList.currentIndex]
        backGroundImageOpacityUpOff.start()
    }

    function activateZone(obj)
    {
        obj.activateTemplate()
    }

    function activateDownCurrentTemplateZone()
    {
        //Normally should never happends, but being paranoiac
        if(mainScreenViewList.currentIndex-1 < 0) return
        var template = get_template(mainScreenViewList.currentIndex)
        var prev_template = get_template(mainScreenViewList.currentIndex-1)

        if(template && prev_template)
        {
            prev_template.deactivateTemplate(template)
        }
    }

    function activateUpCurrentTemplateZone()
    {
        //Normally should never happends, but being paranoiac ...
        if(mainScreenViewList.currentIndex+1 > mainScreenViewList.count) return
        var template = get_template(mainScreenViewList.currentIndex)
        var next_template = get_template(mainScreenViewList.currentIndex+1)
        if(template && next_template)
        {
            next_template.deactivateTemplate(template)
        }
    }

    function mainScreenViewDownKeyPressed()
    {
        if( mainScreenViewList.currentIndex === mainScreenViewList.count - 1 ||
            backGroundImageYDownOn.running || backGroundImageOpacityDownOff.running)
            return
        mainScreenViewList.incrementCurrentIndex();
        highlightDownActionSelectedCategory()
        backgroundAnimateDown()
    }

    function mainScreenViewUpKeyPressed()
    {
        if(!mainScreenViewList.currentIndex ||
            backGroundImageYUpOn.running ||
            backGroundImageOpacityUpOff.running)
            return

        mainScreenViewList.decrementCurrentIndex();
        highlightUpActionSelectedCategory()
        backgroundAnimateUp()
    }

    function exitHomeScreen()
    {
        hideMainScreenView();
    }

    function activateLoadingZone()
    {
        if(!get_template(mainScreenViewList.currentIndex))
            return

        loadingCategory.text = kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_LOADING) +
                               " " +
                               mainScreenViewModel.get(
                               mainScreenViewList.currentIndex).name +
                               " ..."

        currentSelectedCategory.opacity = 0
        loadingCategoryAnimOn.start()
        loadingCategory.opacity = 0
    }

    function activateLoadingZoneStarted()
    {
        var template = get_template(mainScreenViewList.currentIndex)
        if(template)
            template.activateRelatedView()
    }

    function deactivateLoadingZone()
    {
        loadingCategory.opacity = 0
        currentSelectedCategory.opacity = 0.2
        currentSelectedCategory.scale = 1
    }

    function mainScreenViewOkKeyPressed()
    {
        activateLoadingZone()
    }

    function onMailBoxStatus(unseen, total)
    {
        var mailBoxInfos = []
        mailBoxInfos.push(unseen.toString())
        mailBoxInfos.push(total.toString())
        mainScreenMailBoxInfosNotify(mailBoxInfos)
    }

    //Key Actions to change with remote controle Event.
    Keys.onPressed: {
        if (event.key === Qt.Key_Up) //Up Button
        {
            mainScreenViewUpKeyPressed()
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {
            mainScreenViewDownKeyPressed()
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {mainScreenViewOkKeyPressed()
        }
        else {}
        event.accepted = true;
    }

    Item {
        id: backRefsRect
        x: rootRectangle.x
        y: rootRectangle.y
        width: rootRectangle.width
        height: rootRectangle.height
    }

    Timer {
        id: mainScreenUnseenMailActualizeTimer

        interval: 5000;
        running: false;
        repeat: false;
        onTriggered: {
            topMsgUnseenMailsIconAnimOn.start()
            topMsgUnseenMailsTxtAnimOn.start()
        }
    }

    NumberAnimation {
           id: mainScreenViewScaleAnimationOn

           target: mainScreenViewItem
           properties: "scale"
           from: 1.2
           to: 1
           duration: 50

           onStarted: {
               activeFocusViewManager(mainScreenView)
               mainScreenViewOpacityAnimationOn.start()
           }
    }
    NumberAnimation {
           id: mainScreenViewOpacityAnimationOn

           target: mainScreenViewItem
           properties: "opacity"
           from: 0
           to: 1
           duration: 50
    }
    NumberAnimation {
           id: mainScreenViewScaleAnimationOff

           target: mainScreenViewItem
           properties: "scale"
           from: 1
           to: 1.2
           duration: 50

           onStarted: {
               mainScreenViewOpacityAnimationOff.start()
           }
           onStopped: {
               exitHomeScreen()
               deactivateLoadingZone()
           }
    }
    NumberAnimation {
           id: mainScreenViewOpacityAnimationOff

           target: mainScreenViewItem
           properties: "opacity"
           from: 1
           to: 0
           duration: 50
    }

    Item {
        id: backgroundAnimationItem

        Image { id: backgroundImg1; x: backRefsRect.x; y: backRefsRect.y; asynchronous: false
            width: backRefsRect.width; height: backRefsRect.height; opacity:1 }
        Image { id: backgroundImg2; x: backRefsRect.x; y: backRefsRect.y; asynchronous: true
            width: backRefsRect.width; height: backRefsRect.height; opacity:0}
        Image { id: backgroundImg3; x: backRefsRect.x; y: backRefsRect.y; asynchronous: true
            width: backRefsRect.width; height: backRefsRect.height; opacity:0}
        Image { id: backgroundImg4;x: backRefsRect.x; y: backRefsRect.y; asynchronous: true
            width: backRefsRect.width; height: backRefsRect.height; opacity:0}
        Image { id: backgroundImg5; x: backRefsRect.x; y: backRefsRect.y; asynchronous: true
            width: backRefsRect.width; height: backRefsRect.height; opacity:0}
        Image { id: backgroundImg6; x: backRefsRect.x; y: backRefsRect.y; asynchronous: true
            width: backRefsRect.width; height: backRefsRect.height; opacity:0}
        Image { id: backgroundImg7; x: backRefsRect.x; y: backRefsRect.y; asynchronous: true
            width: backRefsRect.width; height: backRefsRect.height; opacity:0}
        Image { id: backgroundImg8; x: backRefsRect.x; y: backRefsRect.y; asynchronous: true
            width: backRefsRect.width; height: backRefsRect.height; opacity:0}
        NumberAnimation {
            id: backGroundImageOpacityUpOff
            properties: "opacity"
            from: 1
            to: 0.1
            duration: 50
            onStarted: activateUpCurrentTemplateZone()
            onStopped: backGroundImageYUpOn.start()
        }
        NumberAnimation {
            id: backGroundImageOpacityDownOff
            properties: "opacity"
            from: 1
            to: 0.1
            duration: 50
            onStarted: activateDownCurrentTemplateZone()
            onStopped: backGroundImageYDownOn.start()
        }
        NumberAnimation {
            id: backGroundImageYDownOn
            properties: "y"
            from: -backRefsRect.height
            to: 0
            duration: 50
            easing {type: Easing.OutBack; overshoot: 0.5 }
            onStarted: {
                backGroundImageYDownOn.target.opacity = 1
                backGroundImageOpacityDownOff.target.opacity = 0
            }
        }
        NumberAnimation {
            id: backGroundImageYUpOn
            properties: "y"
            from: backRefsRect.height
            to: 0
            duration: 50
            easing {type: Easing.OutBack; overshoot: 0.5 }
            onStarted: {
                backGroundImageYUpOn.target.opacity = 1
                backGroundImageOpacityUpOff.target.opacity = 0
            }
        }
        Image {
            id: backgroundOverlay1
            width: backRefsRect.width
            height: backRefsRect.height
            x: backRefsRect.x;
            y: backRefsRect.y
            fillMode: Image.Stretch
            opacity: 0.5
        }
        Image {
            id: backgroundOverlay2
            width: backRefsRect.width
            height: backRefsRect.height
            x: backRefsRect.x
            y:backRefsRect.y
            opacity: 0.6
        }
    }

    //Header Zone
    Item {
        id: mainViewHeaderRectItem
        anchors.left: backRefsRect.left
        anchors.right: backRefsRect.right
        anchors.rightMargin: marginBlock*2
        anchors.top: backRefsRect.top
        anchors.topMargin: marginBlock
        height: backRefsRect.height * 0.06
        Text {
            id: timeHeaderTxt
            anchors.right: parent.right
            anchors.top: parent.top
            text: g_currentTimeStdFormat
            color: "white"
            font {
                family: localFontCaps.name;
                pixelSize: backRefsRect.width * 0.015
            }
        }
        Text {
            anchors.right: timeHeaderTxt.right
            anchors.top: timeHeaderTxt.bottom
            anchors.topMargin: -marginBlock
            text: g_currentDateStdFormat
            color: "#6aa6cb"
            font {
                family: localFont.name
                pixelSize: backRefsRect.width * 0.015
            }
        }
        Image {
            id: homeIcon
            anchors.left: parent.left
            anchors.leftMargin: marginVodFirstTemplate*4
            anchors.bottom: parent.bottom
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_ICON)
            opacity: 0.8
        }
        Text {
            id: homeTxt
            anchors.left: homeIcon.right
            anchors.leftMargin: marginVodFirstTemplate
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -marginVodFirstTemplate/3
            text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_HOME)
            color: "white"
            opacity: 0.8
            font {
                family: localFont.name
                pixelSize: backRefsRect.width * 0.018
            }
        }
        Image {
            id: topMsgUnseenMailsIcon
            anchors.left: homeTxt.right
            anchors.leftMargin: marginVodFirstTemplate * 2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -marginVodFirstTemplate*1.3
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MESSAGE_ICON)
            opacity: 0
            scale: 0
            NumberAnimation {
                   id: topMsgUnseenMailsIconAnimOn
                   target: topMsgUnseenMailsIcon
                   properties: "scale, opacity"
                   from: 0
                   to: 0.9
                   duration: 100
                   easing {type: Easing.OutBack; overshoot: 2 }
            }
        }
        Text {
            id: topMsgUnseenMailsTxt
            anchors.left: topMsgUnseenMailsIcon.right
            anchors.leftMargin: marginVodFirstTemplate
            anchors.bottom: homeTxt.bottom
            text: ""
            color: "white"
            opacity: 0
            scale: 0
            font {
                family: localFont.name
                pixelSize: backRefsRect.width * 0.017
            }
            NumberAnimation {
                   id: topMsgUnseenMailsTxtAnimOn
                   target: topMsgUnseenMailsTxt
                   properties: "scale, opacity"
                   from: 0
                   to: 1
                   duration: 200
                   easing {type: Easing.OutBack; overshoot: 2 }
                   onStopped: {
                       mainScreenUnseenMailActualizeTimer.start()
                   }
            }
        }
    }
    function mainViewHeaderRect() {
        return mainViewHeaderRectItem
    }


    //ListView Rect Zone
    Item {
        id: mainChoiceZoneItem
        x: backRefsRect.x
        y: backRefsRect.y
        width: backRefsRect.width * 0.28
        height: backRefsRect.height * 0.8
        anchors.left: backRefsRect.left
        anchors.top: backRefsRect.top
        anchors.topMargin: backRefsRect.height * 0.15
    }

    function mainChoiceZone() {
        return mainChoiceZoneItem
    }

    Image {
        id: separatorLine
        x: mainChoiceZoneItem.width - (marginBlock*3)
        y: backRefsRect.y
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SEPERATOR_MAIN_VIEW_SCREEN)
    }

    //Favorite ListView
    Component {
            id : mainListComponent
            Item {
                id: wrapper
                width: mainChoiceZoneItem.width
                height: mainChoiceZoneItem.height*0.09
                Rectangle {
                    x: marginBlock
                    width: mainChoiceZoneItem.width
                    height: mainChoiceZoneItem.height*0.09
                    y: shiftY
                    color: "transparent"
                    Behavior on y {
                        NumberAnimation {
                            id: bouncebehavior
                            easing {
                                type: Easing.OutElastic
                                amplitude: 1.0
                                period: 0.5
                            }
                        }
                    }
                    Text {
                        id: mainCategoryTitle
                        anchors.right: parent.right
                        anchors.rightMargin: marginBlock*4
                        anchors.verticalCenter: parent.verticalCenter
                        color: "white"
                        opacity: wrapper.ListView.isCurrentItem ? 1 : 0.4
                        text: name
                        style: wrapper.ListView.isCurrentItem ? Text.Normal : Text.Raised
                        styleColor: "black"
                        font {
                          family: localAdventFont.name;
                          pixelSize: wrapper.ListView.isCurrentItem ?
                                         Utils.scaled(60) :
                                         Utils.scaled(50)
                        }
                    }
                    Glow {
                        id: mainCategoryTitleGlow
                        anchors.fill: mainCategoryTitle
                        radius: 8
                        visible:wrapper.ListView.isCurrentItem ? true : false
                        opacity: wrapper.ListView.isCurrentItem ? 1 : 0
                        samples: 17
                        spread: 0.1
                        color: "white"
                        transparentBorder: true
                        source: mainCategoryTitle
                        Behavior on opacity {
                            SequentialAnimation {
                                loops: Animation.Infinite
                                NumberAnimation
                                {
                                    target: mainCategoryTitleGlow;
                                    property: "opacity";
                                    from: 1; to: 0; duration: 800
                                }
                                NumberAnimation
                                {
                                    target: mainCategoryTitleGlow;
                                    property: "opacity";
                                    from: 0; to: 1; duration: 800
                                }
                            }
                        }
                    }
                    Text {
                        anchors.right: mainCategoryTitle.right
                        anchors.top: mainCategoryTitle.bottom
                        anchors.topMargin: -marginVodFirstTemplate
                        color: "#6aa6cb"
                        opacity: (wrapper.ListView.isCurrentItem ? 1 : 0.4) - 0.4
                        text: secondText
                        style: Text.Raised
                        styleColor: "black"
                        font {
                          family: localAdventFont.name;
                          pixelSize: wrapper.ListView.isCurrentItem ?
                                         Utils.scaled(60*0.45) :
                                         Utils.scaled(50*0.45)
                        }
                    }
                }
            }
        }
    ListModel {
         id: mainScreenViewModel
         //Template to Use
        ListElement {
            name: ""
            template: ""
            secondText: ""
            shiftY: 0
            backJacket: ""
        }
    }

    XmlListModel {
        id: xmlMainScreenCategoryModel
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.XML_MAIN_SCREEN_FILE)
        query: "/categories/category"
        XmlRole {
         name: "title";
         query: "@title/string()"
        }
        XmlRole {
         name: "template";
         query: "@template/string()"
        }
        XmlRole {
         name: "backgroundJacket";
         query: "backgroundJacket/string()"
        }
        XmlRole {
         name: "backgroundOverlay1";
         query: "backgroundOverlay1/string()"
        }
        XmlRole {
         name: "backgroundOverlay2";
         query: "backgroundOverlay2/string()"
        }
        XmlRole {
         name: "logo";
         query: "logo/string()"
        }
        onStatusChanged:
        {
            if (status == XmlListModel.Ready)
            {
                mainScreenViewModel.clear()
                var index = 0
                for(var i = 0; i < xmlMainScreenCategoryModel.count ; i++)
                {
                    if(xmlMainScreenCategoryModel.get(i).title === "Config")
                    {
                        backgroundOverlay1.source =
                                kvalUiConfigManager.translate_path(xmlMainScreenCategoryModel.get(0).backgroundOverlay1)
                        backgroundOverlay2.source =
                                kvalUiConfigManager.translate_path(xmlMainScreenCategoryModel.get(0).backgroundOverlay2)
                        kvalIpMediaLogo.source =
                                kvalUiConfigManager.translate_path(xmlMainScreenCategoryModel.get(0).logo)
                    } else {
                        mainScreenViewModel.insert(index,
                        {
                        'name': kvalUiConfigManager.xmlGetStr(xmlMainScreenCategoryModel.get(i).title),
                        'template': xmlMainScreenCategoryModel.get(i).template,
                        'secondText' : '',
                        'shiftY': index ? 0 : -marginBlock*3,
                        'backJacket':
                        kvalUiConfigManager.translate_path(xmlMainScreenCategoryModel.get(i).backgroundJacket)});
                        index=index+1
                    }
                }
                if(g_screenReloaded)
                {
                    mainScreenViewList.currentIndex = g_previousIndex
                    mainScreenViewModel.setProperty(
                                            mainScreenViewList.currentIndex,
                                            "shiftY",
                                            -marginBlock*3);
                    for (var j = 0; j < mainScreenViewList.currentIndex; j++)
                    {
                        mainScreenViewModel.setProperty(j, "shiftY", -marginBlock*6);
                    }
                    g_screenReloaded = false
                    g_previousIndex = 0
                }
                else
                {
                    gatherAllMainScreenInfos()
                }
            }
            else if(status != XmlListModel.Loading)
            {
                logger.info("OUUUUUUUUUUUUUUUUUUPPPPPPPSSS")
            }
        }
    }

    function mainScreenViewList() {return mainScreenViewList}
    ListView {
        id: mainScreenViewList
        anchors.left: mainChoiceZoneItem.left
        anchors.right: mainChoiceZoneItem.right
        anchors.leftMargin: marginBlock
        anchors.top: mainChoiceZoneItem.top
        anchors.topMargin: marginBlock * 10
        anchors.bottom: mainChoiceZoneItem.bottom
        enabled: true
        opacity: 1
        focus: true

        model: mainScreenViewModel
        highlightFollowsCurrentItem: true
        clip: false
        keyNavigationWraps: false
        interactive: false
        delegate: mainListComponent
    }

    Image {
        id: downBarRss
        anchors.left: backRefsRect.left
        anchors.right: backRefsRect.right
        anchors.bottom: backRefsRect.bottom
        height: backRefsRect.height * 0.035
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_RSS_INFO_BAR)
        opacity: 0.9
    }
    Text {
        id: rssText
        x: backRefsRect.width
        anchors.verticalCenter: downBarRss.verticalCenter
        color: "white"
        font {
            family: localFont.name;
            pixelSize: downBarRss.height * 0.7
        }
    }
    NumberAnimation {
        id: rssTextAnim
        target: rssText
        properties: "x"
        from: backRefsRect.width
        to: -rssText.implicitWidth
        duration: 10000
        loops: Animation.Infinite
    }
    Image {
        id: kvalIpMediaLogo
        anchors.right: separatorLine.right
        anchors.bottom: downBarRss.top

        scale: 0.7
    }

    Text {
        id: currentSelectedCategory

        anchors.right: backRefsRect.right
        anchors.rightMargin: marginBlock*6
        anchors.bottom: downBarRss.top
        text: mainScreenViewModel.get(mainScreenViewList.currentIndex).name
        color: "white"
        opacity: 0.2
        font {
            family: localFontLow.name;
            pixelSize: backRefsRect.height * 0.09
        }
    }

    Text {
        id: loadingCategory

        anchors.right: backRefsRect.right
        anchors.rightMargin: marginBlock*6
        anchors.bottom: downBarRss.top
        scale: 0
        opacity: 0
        color: "white"
        style: Text.Raised
        styleColor: "black"
        font {
            family: localAdventFont.name;
            pixelSize: backRefsRect.height * 0.06
        }
        NumberAnimation {
            id: loadingCategoryAnimOn

            target: loadingCategory
            properties: "scale, opacity"
            from: 0
            to: 1
            easing {type: Easing.OutElastic; overshoot: 1 }
            duration: 250
            onStopped: {
                activateLoadingZoneStarted()
            }
        }
    }
}
