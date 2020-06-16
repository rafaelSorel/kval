import QtQuick 2.0
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0
import QtGraphicalEffects 1.0

Item {
    id: popUpView

    //TODO Check the width and height problem and set them dynamically
    width: rootRectangle.width; 
    height: rootRectangle.height;
    x: rootRectangle.x
    y: rootRectangle.y

    enabled: false
    opacity: 0
    focus: false

    property bool g_isNoButFocus: true
    property int g_diag_type: -1
    property string loadPrefix : " ..."
    property bool g_runningSpinner: false
    property bool g_runningItemHighlight: false
    property bool g_ProgressDiagAborted: false
    property var g_backoffEngine: null

    function enableView()
    {
        keyActionTimer.start()
        enabled = true
        focus = true
        opacity = 1
    }

    //Key burst protection timer
    Timer {
        id: keyActionTimer
        interval: 700;
        running: false;
        repeat: false;
    }

    function popUpViewDisplay(severity, msgData)
    {
        switch (severity)
        {
            case IPTV_Constants.ENUM_ERROR_MESSAGE:
                popUpMsgLogo.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MESSAGE_POPUP_ERROR_LOGO);
                break;
            case IPTV_Constants.ENUM_INFO_MESSAGE:
                popUpMsgLogo.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MESSAGE_POPUP_INFO_LOGO);
                break;
            case IPTV_Constants.ENUM_QUESTION_MESSAGE:
                greenButtonKey.opacity = 1;
                redButtonKey.opacity = 1;
                popUpMsgLogo.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MESSAGE_POPUP_QUESTION_LOGO);
                break;
            default:
                logger.error("Unknown severity !!")
                break;
        }
        msgTextContent.text = msgData;
        messageWindowBackground.opacity = 0.8
        enableView()
    }

    function messageLoadingPopUpDisplay(severity, msgData)
    {
        if(g_diag_type > 0) return

        switch (severity)
        {
            case IPTV_Constants.ENUM_ERROR_MESSAGE:
                popUpMsgLogo.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MESSAGE_POPUP_ERROR_LOGO);
                break;
            case IPTV_Constants.ENUM_INFO_MESSAGE:
                popUpMsgLogo.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MESSAGE_POPUP_INFO_LOGO);
                break;
            case IPTV_Constants.ENUM_QUESTION_MESSAGE:
                greenButtonKey.opacity = 1;
                redButtonKey.opacity = 1;
                popUpMsgLogo.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MESSAGE_POPUP_QUESTION_LOGO);
                break;
            default:
                logger.error("Unknown severity !!")
                break;
        }
        msgTextContentAnimated.text = msgData;
        msgTextPrefix.text = loadPrefix;
        messageWindowBackground.opacity = 0.8
        opacity = 1;
        msgLoadingTimer.running = true;
        msgLoadingTimer.repeat = true;
        msgLoadingTimer.start();
    }

    function appInfoDisplay(infos, obj)
    {
        if(!obj.popUpTryGetFocus())
        {
            return
        }
        messageLoadingPopUpHide()
        g_diag_type = IPTV_Constants.ENUM_DIAG_APP_INFO
        enableView()
        appInfosItem.visible = true
        appInfoAppName.text = infos.appName
        appInfoIconImg.source = infos.appIcon
        appInfoBackImg.source = infos.appBack
        appInfoFanart.source = infos.appFanart
        appInfoAppResume.text = infos.appResume
        if(infos.appCat === 'video') appAppCatImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_VID_ICO)
        else if(infos.appCat === 'store') appAppCatImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_STORE_ICO)
        else if(infos.appCat === 'misc') appAppCatImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_MISC_ICO)
        else if(infos.appCat === 'game') appAppCatImg.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_GAMES_ICO)
        var extra_infos = []
        extra_infos.push(infos.appProvider)
        extra_infos.push("version " + infos.appVersion)
        extra_infos.push(infos.appCat)
        appInfoAppExtraInfos.text = "<b>"+extra_infos.join(" • ")+"</b>"
    }

    function okDiag(title, text)
    {
        if( (!getCurrentActiveView().popUpTryGetFocus) ||
            (!getCurrentActiveView().popUpTryGetFocus()))
        {
            appsEngine.diagYesNoReply(false)
            return
        }

        messageLoadingPopUpHide()
        g_diag_type = IPTV_Constants.ENUM_DIAG_OK
        diagSelector.anchors.centerIn = diagOkButtomImg
        diagSelector.visible = true
        diagSelectorAnim.start()
        diagTitle.text = title
        diagText.text = text
        diagOkTxt.text = 'Ok'
        diagOkButtomImg.visible = true
        diag.visible = true
        enableView()
    }

    function yesNoDiag(title, text, nolabel, yeslabel, backoffEngine)
    {
        if( (!getCurrentActiveView().popUpTryGetFocus) ||
            (!getCurrentActiveView().popUpTryGetFocus()))
        {
            if(!backoffEngine)
                appsEngine.diagYesNoReply(false)
            return
        }

        messageLoadingPopUpHide()
        g_backoffEngine = (!backoffEngine) ? null : backoffEngine
        g_diag_type = IPTV_Constants.ENUM_DIAG_YES_NO
        diagSelector.anchors.centerIn = diagNoButtomImg
        diagSelector.visible = true
        diagSelectorAnim.start()
        diagTitle.text = title
        diagText.text = text
        diagYesButtomImg.visible = true
        yesLabelTxt.text = yeslabel
        diagNoButtomImg.visible = true
        noLabelTxt.text = nolabel
        diag.visible = true
        enableView()
    }
    function yesNoDiagUpdate(title, text)
    {
        if(!diag.visible) return

        if(text !== "") diagText.text = text
    }

    function yesNoDiagClose()
    {
        popUpViewHide()
    }

    function progressDiag(title, text)
    {
        if( (!getCurrentActiveView().popUpTryGetFocus) ||
            (!getCurrentActiveView().popUpTryGetFocus()))
        {
            appsEngine.abortProgressDiag()
            return
        }
        messageLoadingPopUpHide()
        g_diag_type = IPTV_Constants.ENUM_DIAG_PROGRESS
        diagSelector.anchors.centerIn = diagOkButtomImg
        g_ProgressDiagAborted = false
        diagSelector.visible = true
        diagSelectorAnim.start()
        diagTitle.text = title
        diagText.text = text
        diagOkTxt.text = qsTr('Annuler')
        diagOkButtomImg.visible = true
        diag.visible = true
        diagProgressBarBack.visible = true
        diagProgressBar.width = 0
        progressBarHelperAnim1.start()
        enableView()
    }
    function progressDiagUpdate(position, text)
    {
        if(!diag.visible) return

        if(text !== "") diagText.text = text
        diagProgressBar.width = (position*diagProgressBarBack.width) / 100
    }

    function progressDiagClose()
    {
        if(!diag.visible) return

        popUpViewHide()
        if( (!getCurrentActiveView().popUpFocusRelinquished) ||
            (!getCurrentActiveView().popUpFocusRelinquished()))
        {
            getCurrentActiveView().popUpFocusRelinquished()
        }
    }

    function listActionMenu(actionMenu, header)
    {
        messageLoadingPopUpHide()
        g_diag_type = IPTV_Constants.ENUM_DIAG_ACTION_LIST
        headerActionMenuTxt.text = header

        var actionNumber = Object.keys(actionMenu).length
        actionMenuBodyImg.height = marginBlock + (actionMenuBodyRef.height * actionNumber)

        actionItemsModelComponent.clear()
        var index = 0
        for (var key in actionMenu)
        {
            actionItemsModelComponent.insert(index, { 'label'     : key,
                                                   'action'   : actionMenu[key]})
            index = index + 1
        }

        actionItemsList.positionViewAtBeginning();
        actionMenuShadowImg.visible = true
        actionMenuBoarderImg.visible = true
        actionMenuHeaderImg.visible = true
        actionMenuBodyImg.visible = true
        actionItemsList.opacity = 1;
        actionItemsList.enabled = true;
        actionItemsList.focus = true;
        g_runningSpinner = true
        enableView()
    }

    function listChoiceMenu(header, items)
    {
        if( (!getCurrentActiveView().popUpTryGetFocus) ||
            (!getCurrentActiveView().popUpTryGetFocus()))
        {
            appsEngine.inputListReply("",-1)
            return
        }

        messageLoadingPopUpHide()
        g_diag_type = IPTV_Constants.ENUM_DIAG_CHOICE_LIST
        headerInputMenuTxt.text = header

        inputMenuBodyImg.height =
                marginBlock + (inputMenuBodyRef.height * ((items.length > 8) ? 8 : items.length))

        inputItemsModelComponent.clear()
        for (var index = 0; index < items.length; index++)
        {
            inputItemsModelComponent.insert(index, { 'label': items[index]})
        }

        inputItemsList.positionViewAtBeginning();
        inputMenuShadowImg.visible = true
        inputMenuBoarderImg.visible = true
        inputMenuHeaderImg.visible = true
        inputMenuBodyImg.visible = true
        inputItemsList.opacity = 1;
        inputItemsList.enabled = true;
        inputItemsList.focus = true;
        g_runningItemHighlight = true
        enableView()
    }

    function popUpViewHide()
    {
        //Global Item display params
        enabled = false;
        opacity = 0;

        g_backoffEngine = null
        g_isNoButFocus = true

        //App Info display Items
        appInfosItem.visible = false

        //Options Menu display Items
        actionMenuShadowImg.visible = false
        actionMenuBoarderImg.visible = false
        actionMenuHeaderImg.visible = false
        actionMenuBodyImg.visible = false
        actionItemsList.opacity = 0
        actionItemsList.enabled = false
        actionItemsList.focus = false
        g_runningSpinner = false

        //Options Menu display Items
        inputMenuShadowImg.visible = false
        inputMenuBoarderImg.visible = false
        inputMenuHeaderImg.visible = false
        inputMenuBodyImg.visible = false
        inputItemsList.opacity = 0
        inputItemsList.enabled = false
        inputItemsList.focus = false
        g_runningItemHighlight = false
        g_ProgressDiagAborted = false

        //Messages window back
        messageWindowBackground.opacity = 0
        greenButtonKey.opacity = 0;
        redButtonKey.opacity = 0;
        focus = false;
        msgTextContent.text = "";
        msgTextContentAnimated.text = "";
        popUpMsgLogo.source = "";
        g_diag_type = -1
        diagProgressBar.width = 0
        diagProgressBarBack.visible = false
        diagSelector.visible = false
        diagOkButtomImg.visible = false
        diagYesButtomImg.visible = false
        diagNoButtomImg.visible = false
        diag.visible = false
        diagSelectorAnim.stop()

        msgTextPrefix.text = ""
    }

    function messageLoadingPopUpHide()
    {
        if(g_diag_type > 0) return
        enabled = false;
        opacity = 0;
        messageWindowBackground.opacity = 0
        greenButtonKey.opacity = 0;
        redButtonKey.opacity = 0;
        focus = false;
        msgTextContent.text = "";
        msgTextContentAnimated.text = "";
        popUpMsgLogo.source = "";
        msgLoadingTimer.running = false;
        msgLoadingTimer.repeat = false;
        msgLoadingTimer.stop();
        msgTextPrefix.text = ""
    }

    function msgPopupViewRightPressed()
    {
        switch(g_diag_type)
        {
            case IPTV_Constants.ENUM_DIAG_YES_NO:
                g_isNoButFocus = true
                diagSelector.anchors.centerIn = diagNoButtomImg
                break;
            case IPTV_Constants.ENUM_DIAG_PROGRESS:
            case IPTV_Constants.ENUM_DIAG_OK:
            case IPTV_Constants.ENUM_DIAG_APP_INFO:
            default:
                break;
        }
    }

    function msgPopupViewLeftPressed()
    {
        switch(g_diag_type)
        {
            case IPTV_Constants.ENUM_DIAG_YES_NO:
                g_isNoButFocus = false
                diagSelector.anchors.centerIn = diagYesButtomImg
                break;
            case IPTV_Constants.ENUM_DIAG_PROGRESS:
            case IPTV_Constants.ENUM_DIAG_OK:
            case IPTV_Constants.ENUM_DIAG_APP_INFO:
            default:
                break;
        }
    }

    function msgPopupViewUpPressed()
    {
        switch(g_diag_type)
        {
            case IPTV_Constants.ENUM_DIAG_ACTION_LIST:
                actionItemsList.decrementCurrentIndex()
                break;
            case IPTV_Constants.ENUM_DIAG_CHOICE_LIST:
                inputItemsList.decrementCurrentIndex()
                break;
            case IPTV_Constants.ENUM_DIAG_YES_NO:
            case IPTV_Constants.ENUM_DIAG_PROGRESS:
            case IPTV_Constants.ENUM_DIAG_OK:
            case IPTV_Constants.ENUM_DIAG_APP_INFO:
            default:
                break;
        }
    }

    function msgPopupViewDownPressed()
    {
        switch(g_diag_type)
        {
            case IPTV_Constants.ENUM_DIAG_ACTION_LIST:
                actionItemsList.incrementCurrentIndex()
                break;
            case IPTV_Constants.ENUM_DIAG_CHOICE_LIST:
                inputItemsList.incrementCurrentIndex()
                break;
            case IPTV_Constants.ENUM_DIAG_YES_NO:
            case IPTV_Constants.ENUM_DIAG_PROGRESS:
            case IPTV_Constants.ENUM_DIAG_OK:
            case IPTV_Constants.ENUM_DIAG_APP_INFO:
            default:
                break;
        }
    }

    function msgPopupViewOkPressed()
    {
        switch(g_diag_type)
        {
            case IPTV_Constants.ENUM_DIAG_YES_NO:
                if(g_isNoButFocus){
                    if (g_backoffEngine)
                        g_backoffEngine.diagWindowCallback(false)
                    else
                        appsEngine.diagYesNoReply(false)
                }
                else{
                    if (g_backoffEngine)
                        g_backoffEngine.diagWindowCallback(true)
                    else
                        appsEngine.diagYesNoReply(true)
                }
                popUpViewHide()
                getCurrentActiveView().popUpFocusRelinquished()
                break;
            case IPTV_Constants.ENUM_DIAG_PROGRESS:
                if(!g_ProgressDiagAborted)
                {
                    diagTitle.text = diagTitle.text + ' : ' + kvalUiConfigManager.retranslate + qsTr("Annulation")
                    appsEngine.abortProgressDiag()
                    g_ProgressDiagAborted = true
                }
                getCurrentActiveView().popUpFocusRelinquished()
                break;
            case IPTV_Constants.ENUM_DIAG_OK:
                popUpViewHide()
                appsEngine.diagYesNoReply(true)
                getCurrentActiveView().popUpFocusRelinquished()
                break;
            case IPTV_Constants.ENUM_DIAG_CHOICE_LIST:
                popUpViewHide()
                appsEngine.inputListReply("",
                                          (!inputItemsList.count) ? -1 :inputItemsList.currentIndex)
                getCurrentActiveView().popUpFocusRelinquished()
                break;
            case IPTV_Constants.ENUM_DIAG_ACTION_LIST:
                popUpViewHide()
                getCurrentActiveView().diagCtxActionCallback((!actionItemsList.count) ? null :
                actionItemsModelComponent.get(actionItemsList.currentIndex).action)
                break;
            case IPTV_Constants.ENUM_DIAG_APP_INFO:
            default:
                popUpViewHide()
                getCurrentActiveView().popUpFocusRelinquished()
                break;
        }
    }

    function msgPopupViewBackPressed()
    {
        switch(g_diag_type)
        {
            case IPTV_Constants.ENUM_DIAG_YES_NO:
                break;
            case IPTV_Constants.ENUM_DIAG_OK:
                break;
            case IPTV_Constants.ENUM_DIAG_PROGRESS:
                if(g_ProgressDiagAborted)
                {progressDiagClose()
                }
                break;
            case IPTV_Constants.ENUM_DIAG_ACTION_LIST:
                popUpViewHide()
                getCurrentActiveView().diagCtxActionCallback(null)
                break;
            case IPTV_Constants.ENUM_DIAG_CHOICE_LIST:
                popUpViewHide()
                appsEngine.inputListReply("", -1)
                getCurrentActiveView().popUpFocusRelinquished()
                break;
            case IPTV_Constants.ENUM_DIAG_APP_INFO:
                popUpViewHide()
                getCurrentActiveView().popUpFocusRelinquished()
                break;
            default:
                break;
        }
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Right) //Right Button
        {
            msgPopupViewRightPressed()
        }
        else if (event.key === Qt.Key_Left) //Left Button
        {
            msgPopupViewLeftPressed()
        }
        else if (event.key === Qt.Key_Up) //Left Button
        {
            msgPopupViewUpPressed()
        }
        else if (event.key === Qt.Key_Down) //Left Button
        {
            msgPopupViewDownPressed()
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {
            if(!keyActionTimer.running)
                msgPopupViewOkPressed()
        }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {
            if(!keyActionTimer.running)
                msgPopupViewBackPressed();
        }
        else {}
        event.accepted = true;
    }

    Timer {
        id: msgLoadingTimer
        interval: 1000;
        running: false;
        repeat: false;
        onTriggered: {
            if(msgTextPrefix.text == " ...")
            {msgTextPrefix.text = " ";
            }
            else if(msgTextPrefix.text == " ")
            {msgTextPrefix.text = " .";
            }
            else if (msgTextPrefix.text == " .")
            {msgTextPrefix.text = " ..";
            }
            else if(msgTextPrefix.text == " ..")
            {msgTextPrefix.text = " ...";
            }
        }
    }

    Item {
        id: appInfosItem
        anchors.fill: parent
        visible: false
        Image {
            id: appInfoFanart
            anchors.top: appInfosWindow.top
            anchors.topMargin: 26 /*Shadow interval*/
            anchors.bottom: appInfosWindow.bottom
            anchors.bottomMargin: 26 /*Shadow interval*/
            anchors.left: appInfosWindow.left
            anchors.leftMargin: 26 /*Shadow interval*/
            anchors.right: appInfosWindow.right
            anchors.rightMargin: 26 /*Shadow interval*/
            opacity: 0.9
        }
        Image {
            id: appInfosWindow
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_APP_INFOS_BACK)
            Item {
                id: appIconZone
                anchors.top: parent.top
                anchors.topMargin: marginBlock * 7
                anchors.left: parent.left
                anchors.leftMargin: marginBlock * 5
                width: 225
                height: 195
                Image {
                    id: appInfoBackImg
                    anchors.fill: parent
                    layer.enabled: true
                    layer.effect: OpacityMask {
                        maskSource: appInfoMaskImg
                    }
                }
                Image {
                    id: appInfoMaskImg
                    anchors.fill: appInfoBackImg
                    fillMode: Image.Stretch
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_APP_INFOS_MASK)
                    visible: false
                }
                Image {
                    id: appInfoIconImg
                    anchors.centerIn: appInfoBackImg
                }
            }
            Text{
                anchors.right: parent.right
                anchors.rightMargin: marginBlock * 4
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock * 2
                color: "white"
                text: "<i>" + kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_APPS_INFORMATIONS) + "</i>"
                opacity: 0.3
                font {
                    family: localFont.name
                    pixelSize: 40
                }
            }

            Text{
                id: appInfoAppName
                anchors.left: appIconZone.right
                anchors.leftMargin: marginBlock * 2
                anchors.top: appIconZone.top
                anchors.topMargin: -marginBlock * 2
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: 50
                }
            }
            Text {
                id: appInfoAppExtraInfos
                anchors.left: appInfoAppName.left
                anchors.top: appInfoAppName.bottom
                color : highlightColor
                font {
                    family: localAdventFont.name
                    pixelSize: 20
                }
            }
            Image {
                id: appAppCatImg
                anchors.verticalCenter: appInfoAppName.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: marginBlock * 4
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_VID_ICO)
                opacity: 0.5
            }

            Rectangle{
                id: appInfoSeparatorLine
                anchors.top: appInfoAppExtraInfos.bottom
                anchors.topMargin: marginBlock * 2
                anchors.left: appInfoAppExtraInfos.left
                height: 1
                width: parent.width * 0.4
                color: "white"
                opacity: 0.8
            }

            Text{
                id: appInfoAppResume
                anchors.left: appInfoAppName.left
                anchors.top: appInfoSeparatorLine.bottom
                anchors.topMargin: marginBlock * 2
                height: appIconZone.height
                width: appIconZone.width * 2.5
                wrapMode: Text.WordWrap
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: 30
                }
            }
        }
    }

    Image {
        id: diag
        anchors.centerIn: parent
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_DIAG_WINDOW)
        visible: false
        Text {
            id: diagTitle
            anchors.top: parent.top
            anchors.topMargin: marginBlock*4.5
            anchors.left: parent.left
            anchors.leftMargin: marginBlock*5
            horizontalAlignment: Text.AlignLeft
            color: "white"
            opacity: 0.7
            elide:Text.ElideRight
            textFormat: Text.AutoText
            font {
              family: localAdventFont.name;
              pixelSize: parent.height * 0.1
            }
        }

//        Rectangle {
//            anchors.top: parent.top
//            anchors.topMargin: parent.height * 0.35
//            anchors.left: parent.left
//            anchors.leftMargin: marginBlock*5
//            anchors.right: parent.right
//            anchors.rightMargin: marginBlock*3
//            anchors.bottom: parent.bottom
//            anchors.bottomMargin: parent.height * 0.35
//            color: "yellow"
//            opacity: 0.3
//        }

        Item {
            anchors.top: parent.top
            anchors.topMargin: parent.height * 0.3
            anchors.left: parent.left
            anchors.leftMargin: marginBlock*5
            anchors.right: parent.right
            anchors.rightMargin: marginBlock*3
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height * 0.35

            Text {
                id: diagText
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                color: "white"
                wrapMode: Text.WordWrap
                elide:Text.ElideRight
                textFormat: Text.AutoText
                font {
                  family: localAdventFont.name;
                  pixelSize: diag.height * 0.08
                }
            }
        }
        Image {
            id: diagOkButtomImg
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock*4
            anchors.horizontalCenter: parent.horizontalCenter
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_DIAG_BUTTON)
            visible: false
            Text {
                id: diagOkTxt
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: marginBlock*0.5
                color: "white"
                textFormat: Text.AutoText
                font {
                  family: localAdventFont.name;
                  pixelSize: parent.height * 0.5
                }
            }
        }
        Image {
            id: diagYesButtomImg
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock*4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: -width
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_DIAG_BUTTON)
            visible: false
            Text {
                id: yesLabelTxt
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: marginBlock*0.5
                color: "white"
                textFormat: Text.AutoText
                font {
                  family: localAdventFont.name;
                  pixelSize: parent.height * 0.5
                }
            }
        }
        Image {
            id: diagNoButtomImg
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock*4
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: width
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_DIAG_BUTTON)
            visible: false
            Text {
                id: noLabelTxt
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: marginBlock*0.5
                color: "white"
                textFormat: Text.AutoText
                font {
                  family: localAdventFont.name;
                  pixelSize: parent.height * 0.5
                }
            }
        }
        Image {
            id: diagProgressBarBack
            anchors.bottom: diagOkButtomImg.top
//            anchors.bottomMargin: marginBlock * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_PROGRESS_BAR_BACK)
            visible: false
            Rectangle {
                id: diagProgressBar
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                height: parent.height * 0.8
                opacity: 0.8
                color: 'white'
                Image {
                    id: progressBarHelper
                    anchors.verticalCenter: parent.verticalCenter
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_PROGRESS_BAR_HELPER)
                    ParallelAnimation {
                        id: progressBarHelperAnim1
                        running: false
                        NumberAnimation
                        {
                            target: progressBarHelper;
                            property: "x";
                            from: diagProgressBar.x;
                            to: diagProgressBar.width*0.5 - progressBarHelper.width;
                            duration: 400
                        }
                        NumberAnimation
                        {
                            target: progressBarHelper;
                            property: "opacity";
                            from: 0; to: 1; duration: 400
                        }
                        onStarted:
                        {
                            if (progressBarHelper.width*2 > diagProgressBar.width)
                                progressBarHelper.visible = false
                            else progressBarHelper.visible = true
                        }

                        onStopped:
                        {
                            if(diagProgressBarBack.visible)
                                progressBarHelperAnim2.start()
                        }
                    }
                    ParallelAnimation {
                        id: progressBarHelperAnim2
                        running: false
                        NumberAnimation
                        {
                            target: progressBarHelper;
                            property: "x";
                            from: (diagProgressBar.width*0.5)-(progressBarHelper.width);
                            to: diagProgressBar.width+progressBarHelper.width;
                            duration: 800
                        }
                        NumberAnimation
                        {
                            target: progressBarHelper;
                            property: "opacity";
                            from: 1; to: 0; duration: 400
                        }
                        onStopped:{
                            if(diagProgressBarBack.visible)
                                progressBarHelperAnim1.start()
                        }
                    }
                }

            }
        }

        Image {
            id: diagSelector
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_DIAG_SELECTOR)
            visible: false
        }
    }

    SequentialAnimation {
        id: diagSelectorAnim
        running: false
        loops: Animation.Infinite
        NumberAnimation
        {
            target: diagSelector;
            property: "opacity";
            from: 1; to: 0.3; duration: 800
        }
        NumberAnimation
        {
            target: diagSelector;
            property: "opacity";
            from: 0.3; to: 1; duration: 800
        }
    }

    Rectangle {
        id: messageWindowBackground
        width: 920
        height: 220
        anchors.centerIn: parent
        color: "black";
        border.color: "white"
        border.width: 1
        opacity: 0
        Image{
            id: greenButtonKey
            opacity:0
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: marginBlock * 24
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_GREEN_BUTTON)
            Text{
                anchors.left: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: marginBlock
                text: kvalUiConfigManager.retranslate + qsTr("Ok")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(28)
                }
            }
        }
        Image{
            id: redButtonKey
            opacity:0
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: marginBlock * 12
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_RED_BUTTON)
            Text{
                anchors.left: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: marginBlock
                text: kvalUiConfigManager.retranslate + qsTr("Cancel")
                color: "white"
                font {
                    family: localFont.name;
                    pixelSize: Utils.scaled(30)
                }
            }
        }
    }

    Rectangle {
        width: 100
        height: 100
        anchors.top: messageWindowBackground.top
        anchors.left: messageWindowBackground.left
        color: "transparent"
        Image {
            id: popUpMsgLogo
            anchors.centerIn: parent;
            source: "";
        }
    }

    Rectangle {
        width: messageWindowBackground.width
        height: messageWindowBackground.height * 0.7
        anchors.bottom: messageWindowBackground.bottom
        anchors.left: messageWindowBackground.left
        anchors.right: messageWindowBackground.right
        color: "transparent"
        Text {
            id: msgTextContent
            width: parent.width
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: "";
            wrapMode: Text.WordWrap
            clip: true
            font {
                family: localFont.name;
                pixelSize: 46
            }
            color: "white"
        }
        Text {
            id: msgTextContentAnimated
            anchors.centerIn: parent
            text: "";
            font {
                family: localFont.name;
                pixelSize: 46
            }
            color: "white"
        }
        Text {
            id: msgTextPrefix
            x: msgTextContentAnimated.contentWidth + msgTextContentAnimated.x
            y: msgTextContentAnimated.y
            text: "";
            font {
                family: localFont.name;
                pixelSize: 46
            }
            color: "white"
        }
    }

    Item {
        id: actionMenuItem
        anchors.centerIn : parent
        Image {
            id: actionMenuBodyRef
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_CTX_BODY)
            visible: false
        }
        Image {
            id: actionMenuShadowImg
            anchors.horizontalCenter: actionMenuBoarderImg.horizontalCenter
            anchors.top: actionMenuBoarderImg.top
            anchors.topMargin: -actionMenuBodyImg.height*0.1
            anchors.bottom: actionMenuBoarderImg.bottom
            anchors.bottomMargin: -actionMenuBodyImg.height*0.1
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_CTX_SHADOW)
            visible: false
        }
        Image {
            id: actionMenuBoarderImg
            anchors.left: actionMenuHeaderImg.left
            anchors.leftMargin: -2
            anchors.right: actionMenuHeaderImg.right
            anchors.rightMargin: -2
            anchors.top: actionMenuHeaderImg.top
            anchors.topMargin: -2
            anchors.bottom: actionMenuBodyImg.bottom
            anchors.bottomMargin: -2
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_CTX_BORDER)
            visible: false
        }
        Image {
            id: actionMenuHeaderImg
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -marginBlock * 20
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_CTX_HEADER)
            Text {
                id: headerActionMenuTxt
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: marginBlock *0.5
                anchors.right: parent.right
                anchors.rightMargin: marginBlock *0.5
                elide:Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                opacity: 0.7
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.5
                }
            }
            visible: false
        }
        Image {
            id: actionMenuBodyImg
            anchors.top: actionMenuHeaderImg.bottom
            anchors.left: actionMenuHeaderImg.left
            height: actionMenuBodyRef.height
            width: actionMenuBodyRef.width
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_CTX_BODY)
            visible: false
        }

        Component {
            id: highlightCtxItem
            Item {
                id: highlightCtxRect
                anchors.horizontalCenter: actionItemsList.currentItem.horizontalCenter
                anchors.verticalCenter: actionItemsList.currentItem.verticalCenter
                Image {
                    anchors.centerIn: parent
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_CTX_MENU_SELECTOR)
                }
                SequentialAnimation {
                    id: spinnerHighlight
                    running: g_runningSpinner
                    loops: Animation.Infinite
                    NumberAnimation
                    {
                        target: highlightCtxRect;
                        property: "opacity";
                        from: 1.2; to: 0.2; duration: 800
                    }
                    NumberAnimation
                    {
                        target: highlightCtxRect;
                        property: "opacity";
                        from: 0.2; to: 1.2; duration: 800
                    }
                }
            }
        }

        Component {
            id : actionItemsComponent
            Row {
                id: wrapperRow
                Item {
                    width: actionMenuBodyRef.width
                    height: actionMenuBodyRef.height
                    Text {
                        id: textLabel
                        width: parent.width - marginBlock*3
                        height: parent.height
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        elide:Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: label
                        scale: wrapperRow.ListView.isCurrentItem ? 1.1 : 1
                        opacity: 0.7
                        color: "white"
                        font {
                            family: localAdventFont.name
                            pixelSize: parent.height * 0.5
                        }
                    }
                }
            }
        }
        ListModel {
             id: actionItemsModelComponent
             //Template to Use
             ListElement {
                 label: ""
                 action: ""
             }
        }
        ListView {
            id: actionItemsList
            anchors.left: actionMenuBodyImg.left
            anchors.right: actionMenuBodyImg.right
            anchors.top: actionMenuBodyImg.top
            anchors.bottom: actionMenuBodyImg.bottom
            enabled: false
            opacity: 0
            focus: false

            model: actionItemsModelComponent

            highlight: highlightCtxItem
            highlightFollowsCurrentItem: true
            clip: false
            keyNavigationWraps: true
            interactive: false
            orientation: ListView.Vertical
            delegate: actionItemsComponent
        }
    }

    Item {
        id: inputMenuItem
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -marginBlock * 4
        Image {
            id: inputMenuBodyRef
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LIST_MENU_BODY)
            visible: false
        }
        Image {
            id: inputMenuShadowImg
            anchors.horizontalCenter: inputMenuBoarderImg.horizontalCenter
            anchors.top: inputMenuBoarderImg.top
            anchors.topMargin: -inputMenuBodyImg.height*0.1
            anchors.bottom: inputMenuBoarderImg.bottom
            anchors.bottomMargin: -inputMenuBodyImg.height*0.1
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LIST_MENU_SHADOW)
            visible: false
        }
        Image {
            id: inputMenuBoarderImg
            anchors.left: inputMenuHeaderImg.left
            anchors.leftMargin: -2
            anchors.right: inputMenuHeaderImg.right
            anchors.rightMargin: -2
            anchors.top: inputMenuHeaderImg.top
            anchors.topMargin: -2
            anchors.bottom: inputMenuBodyImg.bottom
            anchors.bottomMargin: -2
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LIST_MENU_BORDER)
            visible: false
        }
        Image {
            id: inputMenuHeaderImg
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -marginBlock * 20
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LIST_MENU_HEADER)
            Text {
                id: headerInputMenuTxt
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: marginBlock *0.5
                anchors.right: parent.right
                anchors.rightMargin: marginBlock *0.5
                elide:Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                opacity: 0.7
                color: "white"
                font {
                    family: localAdventFont.name
                    pixelSize: parent.height * 0.5
                }
            }
            visible: false
        }
        Image {
            id: inputMenuBodyImg
            anchors.top: inputMenuHeaderImg.bottom
            anchors.left: inputMenuHeaderImg.left
            height: inputMenuBodyRef.height
            width: inputMenuBodyRef.width
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LIST_MENU_BODY)
            visible: false
        }

        Component {
            id: highlightInputItem
            Item {
                id: highlightInputRect
                anchors.horizontalCenter: inputItemsList.currentItem.horizontalCenter
                anchors.verticalCenter: inputItemsList.currentItem.verticalCenter

                Image {
                    anchors.centerIn: parent
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LIST_MENU_SELECTOR)
                }
                SequentialAnimation {
                    running: g_runningItemHighlight
                    loops: Animation.Infinite
                    NumberAnimation
                    {
                        target: highlightInputRect;
                        property: "opacity";
                        from: 1.2; to: 0.2; duration: 800
                    }
                    NumberAnimation
                    {
                        target: highlightInputRect;
                        property: "opacity";
                        from: 0.2; to: 1.2; duration: 800
                    }
                }
            }
        }

        Component {
            id : inputItemsComponent
            Row {
                id: wrapperRow
                Item {
                    width: inputMenuBodyRef.width
                    height: inputMenuBodyRef.height
                    Text {
                        id: textLabel
                        width: parent.width - marginBlock*3
                        height: parent.height
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        elide:Text.ElideRight
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text: label
                        opacity: 0.7
                        color: "white"
                        font {
                            family: localAdventFont.name
                            pixelSize: parent.height * 0.55
                        }
                    }
                }
            }
        }
        ListModel {
             id: inputItemsModelComponent
             //Template to Use
             ListElement {
                 label: ""
             }
        }
        ListView {
            id: inputItemsList
            anchors.left: inputMenuBodyImg.left
            anchors.right: inputMenuBodyImg.right
            anchors.top: inputMenuBodyImg.top
            anchors.bottom: inputMenuBodyImg.bottom
            enabled: false
            opacity: 0
            focus: false

            model: inputItemsModelComponent

            highlight: highlightInputItem
            highlightFollowsCurrentItem: true
            clip: true
            keyNavigationWraps: true
            interactive: false
            orientation: ListView.Vertical
            delegate: inputItemsComponent
        }
    }

}
