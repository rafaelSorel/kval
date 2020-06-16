import QtQuick 2.3
import "KvalUI_constant.js" as IPTV_Constants
import QtGraphicalEffects 1.0
import kval.gui.qml 1.0

Item
{
    id: mailBoxView

    enabled: false
    opacity: 0
    focus: false
    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

    signal mailBoxDisplayReadyNotify()
    signal setMailSeenFlagNotify(int index)
    signal activateMainScreenView()
    signal notifyMainScreenMailBoxStatus(int unseen, int total)
    signal deleteCurrentMailNotify(int index)
    signal deleteAllMailsNotify()

    property int        g_mailSeenNbr : 0
    property int        g_mailTotalNbr : 0
    property int        g_latestFocusIndex: 0

    function showMailBoxView()
    {
        registerActiveView(mailBoxMainView)
        activeFocusViewManager(mailBoxMainView)
        opacity = 1
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
        activeFocusViewManager(mailBoxMainView)
        opacity = 1
    }

    function backToHomeView()
    {
        mailBoxBack.source = ""
        mailBoxBackOverlay.source = ""
        notifyMainScreenMailBoxStatus(g_mailSeenNbr, g_mailTotalNbr)
        activateMainScreenView();
        enabled = false
        opacity = 0
        focus = false
        pyNotificationView.clearview();
    }

    function onMailBoxEmpty()
    {
        mailBoxModel.clear()
        mailBoxModel.insert(0, {'mailIndex': '',
                                'mailTitle': '<center><b><i>Messagerie Vide</center></b></i>',
                                'mailListInfo' : '',
                                'mailDate': '',
                                'mailSeverity' : '',
                                'mailColor': "black",
                                'mailColor2': "black",
                                'mailContent': '',
                                'mailSeenFlag' : 1,
                                'mailSeenFlagIcon' : ''});
        g_mailSeenNbr = 0
        g_mailTotalNbr = 0
        mailBoxListView.currentIndex = 0
        mailBoxPositionViewAt(0)
        mailBoxSetMailSeenFlag()
        mailBoxSetMailInfos()
    }

    function fillMailBoxList(mailBoxInfos)
    {
        mailBoxModel.clear()
        var reg = new RegExp("[;]+", "g");
        var unseenMailsNbr = 0
        for(var i = 0; i < mailBoxInfos.length ; i++)
        {
            var opacityFlag = 0.8
            var tab = mailBoxInfos[i].split(reg);
            var mailSeverity = tab[2]

            if (mailSeverity === kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_HIGH_SEVERITY_SERVER_MESSAGE))
                mailSeverity = "Important"
            if (mailSeverity === kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_VERY_HIGH_SEVERITY_SERVER_MESSAGE))
                mailSeverity = "Très Important"
            else mailSeverity = "Normal"

            var mailSeenFlag = tab[3].toString().toLowerCase()
            if (mailSeenFlag === 'false')
            {
                unseenMailsNbr = unseenMailsNbr + 1
                opacityFlag = 1
            }
            else
            {opacityFlag = 0.5
            }

            mailBoxModel.insert(i, {'mailIndex': (i+1).toString(),
                                    'mailTitle': tab[0],
                                    'mailListInfo' : tab[1]+" • "+mailSeverity,
                                    'mailDate': tab[1],
                                    'mailSeverity' : mailSeverity ,
                                    'mailColor': i ? "white" : "black",
                                    'mailColor2': i ? "#6aa6cb" : "black",
                                    'mailContent': tab[4],
                                    'mailSeenFlag' : opacityFlag,
                                    'mailSeenFlagIcon' :
                                    i ? kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG) :
                                        kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG_SELECTED)});
        }
        g_mailSeenNbr = unseenMailsNbr
        g_mailTotalNbr = mailBoxInfos.length
        mailBoxSetMailSeenFlag()
        mailBoxSetMailInfos()
        mailBoxPositionViewAt(0)
    }

    function mailBoxPositionViewAt(posIndex)
    {
        mailBoxListView.positionViewAtIndex(posIndex,
                                            ListView.Top)
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailColor",
                                "black");
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailColor2",
                                "black");
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailSeenFlagIcon",
                                 kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG_SELECTED));
    }

    function mailBoxSetMailInfos()
    {
        mailBoxCurrentMailTitleTxt.text =
                mailBoxModel.get(mailBoxListView.currentIndex).mailTitle
        mailBoxCurrentMailRecvDateTxt.text =
                mailBoxModel.get(mailBoxListView.currentIndex).mailDate
        mailBoxCurrentMailSeverityTxt.text =
                mailBoxModel.get(mailBoxListView.currentIndex).mailSeverity
        mailBoxCurrentMailMsgTxt.text =
                mailBoxModel.get(mailBoxListView.currentIndex).mailContent

        if(enabled) return
        mailBoxBack.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_BACKGROUND)
        mailBoxBackOverlay.source = kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.OVERLAY_BACKGROUND_2)
        mailBoxDisplayReadyNotify()
    }

    function mailBoxSetMailSeenFlag()
    {
        if(!g_mailTotalNbr) return
        if(mailBoxModel.get(mailBoxListView.currentIndex).mailSeenFlag !== 1)
            return
        g_mailSeenNbr = g_mailSeenNbr - 1
        setMailSeenFlagNotify(mailBoxListView.currentIndex)
    }

    function mailBoxViewUpKeyPressed()
    {
        if(mailBoxListView.count === 1) return
        mailBoxListView.decrementCurrentIndex()
        mailBoxSetMailInfos()
        mailBoxSetMailSeenFlag()
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailColor",
                                "black");
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailColor2",
                                "black");
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailSeenFlagIcon",
                                 kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG_SELECTED));

        if(mailBoxListView.currentIndex+1 === mailBoxListView.count)
        {
            mailBoxModel.setProperty(0, "mailColor", "white");
            mailBoxModel.setProperty(0, "mailColor2", "#6aa6cb");
            mailBoxModel.setProperty(0, "mailSeenFlag", 0.5);
            mailBoxModel.setProperty(0, "mailSeenFlagIcon",
                                     kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG));
        }
        else
        {
            mailBoxModel.setProperty(mailBoxListView.currentIndex+1,
                                    "mailColor",
                                    "white");
            mailBoxModel.setProperty(mailBoxListView.currentIndex+1,
                                    "mailColor2",
                                    "#6aa6cb");
            mailBoxModel.setProperty(mailBoxListView.currentIndex+1,
                                     "mailSeenFlag",
                                     0.5);
            mailBoxModel.setProperty(mailBoxListView.currentIndex+1,
                                     "mailSeenFlagIcon",
                                     kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG));
        }
    }

    function mailBoxViewDownKeyPressed()
    {
        if(mailBoxListView.count === 1) return
        mailBoxListView.incrementCurrentIndex()
        mailBoxSetMailInfos()
        mailBoxSetMailSeenFlag()
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailColor",
                                "black");
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailColor2",
                                "black");
        mailBoxModel.setProperty(mailBoxListView.currentIndex,
                                "mailSeenFlagIcon",
                                 kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG_SELECTED));

        if(mailBoxListView.currentIndex-1 < 0)
        {
            mailBoxModel.setProperty(mailBoxListView.count - 1,
                                            "mailColor", "white");
            mailBoxModel.setProperty(mailBoxListView.count - 1,
                                            "mailColor2", "#6aa6cb");
            mailBoxModel.setProperty(mailBoxListView.count - 1,
                                            "mailSeenFlag", 0.5);
            mailBoxModel.setProperty(mailBoxListView.count - 1,
                                    "mailSeenFlagIcon",
                                     kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG));
        }
        else
        {
            mailBoxModel.setProperty(mailBoxListView.currentIndex-1,
                                    "mailColor",
                                    "white");
            mailBoxModel.setProperty(mailBoxListView.currentIndex-1,
                                    "mailColor2",
                                    "#6aa6cb");
            mailBoxModel.setProperty(mailBoxListView.currentIndex-1,
                                            "mailSeenFlag", 0.5);
            mailBoxModel.setProperty(mailBoxListView.currentIndex - 1,
                                    "mailSeenFlagIcon",
                                     kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_SEEN_FLAG));
        }
    }

    function deleteCurrentMailEntry()
    {
        g_latestFocusIndex = mailBoxListView.currentIndex
        deleteCurrentMailNotify(mailBoxListView.currentIndex)
    }

    function deleteAllMails()
    {deleteAllMailsNotify()
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Up) //Up Button
        {mailBoxViewUpKeyPressed();
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {mailBoxViewDownKeyPressed();
        }
        else if (event.key === Qt.Key_H) //Home Button
        {backToHomeView();
        }
        else if (event.key === Qt.Key_R) //Red Button
        {deleteCurrentMailEntry();
        }
        else if (event.key === Qt.Key_Y) //Yellow Button
        {deleteAllMails();
        }
        else {}
        event.accepted = true;
    }
    Rectangle {
        id: mailBoxListViewRef

        x: 0
        y: mailBoxView.height * 0.12
        width: mailBoxView.width * 0.5
        height: mailBoxView.height * 0.8
        color: "transparent"
    }

    Item {
        id: mailBoxBackGound
        anchors.fill: parent
        Image {
            id: mailBoxBack
            anchors.fill: parent
        }
        Image {
            id: mailBoxBackOverlay
            anchors.fill: parent
        }
    }

    Item {
        id: mailBoxHeaderZone
        anchors.top: parent.top
        anchors.topMargin: marginBlock
        anchors.left: parent.left
        anchors.leftMargin: marginBlock*2
        anchors.right: parent.right
        anchors.rightMargin: marginBlock*2
        height: parent.height * 0.1
        Text {
            id: mailBoxTimeHeaderTxt
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
            anchors.right: mailBoxTimeHeaderTxt.right
            anchors.top: mailBoxTimeHeaderTxt.bottom
            anchors.topMargin: -marginBlock
            text: g_currentDateStdFormat
            color: "#6aa6cb"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.015
            }
        }
        Image{
            id: mailBoxIco

            anchors.left: parent.left
            anchors.leftMargin: -marginBlock*2
            anchors.top: parent.top
            anchors.topMargin: -marginBlock*3
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.MAILBOX_BOX_ICON)
            scale: 0.6
        }
        Text {
            id: mailBoxTitle

            anchors.left: mailBoxIco.right
            anchors.leftMargin: -marginBlock*2
            anchors.top: parent.top
            text: kvalUiConfigManager.retranslate +
                  kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_MAILBOX)
            color: "white"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.025
            }
        }
        Text {
            id: mailBoxTotalNbrMails

            anchors.left: mailBoxTitle.left
            anchors.top: mailBoxTitle.bottom
            anchors.topMargin: -marginBlock
            color: "white"
            text:   '<b>'+g_mailTotalNbr.toString()+'</b>'+
                    kvalUiConfigManager.retranslate + qsTr(' messages')+' • '+
                    '<b>'+g_mailSeenNbr.toString() +'</b>'+
                    kvalUiConfigManager.retranslate + qsTr(' non lues')
            font {
                family: localFont.name
                pixelSize: parent.width * 0.015
            }
        }
    }

    Item {
        id: mailBoxBottomZone

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: parent.width
        height: parent.height * 0.1

        Image {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * 0.5

            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_RSS_INFO_BAR)
            opacity: 0.5
        }
        Image{
            id: mailBoxRedKeyIco

            anchors.bottom: parent.bottom
            anchors.bottomMargin: -marginBlock
            anchors.right: parent.right
            anchors.rightMargin: parent.width * 0.4
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_RED_BUTTON)

            Text{
                id: mailBoxDeleteCurrentTxt

                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock
                anchors.left: parent.right
                anchors.leftMargin: marginBlock/2
                text: kvalUiConfigManager.retranslate + qsTr("Supprimer Message Courant")
                color: "white"
                font {
                    family: localAdventFont.name;
                    pixelSize: mailBoxBottomZone.height * 0.25
                }
            }
        }
        Image{
            id: mailBoxYellowKeyIco

            anchors.bottom: parent.bottom
            anchors.bottomMargin: -marginBlock
            anchors.left: mailBoxRedKeyIco.right
            anchors.leftMargin: marginBlock * 3 +
                                mailBoxDeleteCurrentTxt.implicitWidth
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.KEY_YELLOW_BUTTON)

            Text{
                anchors.bottom: parent.bottom
                anchors.bottomMargin: marginBlock
                anchors.left: parent.right
                anchors.leftMargin: marginBlock/2
                text: kvalUiConfigManager.retranslate + qsTr("Supprimer Tout les Messages")
                color: "white"
                font {
                    family: localAdventFont.name;
                    pixelSize: mailBoxBottomZone.height * 0.25
                }
            }
        }
    }

    Image {
        id: separatorLine
        anchors.top: mailBoxHeaderZone.bottom
        anchors.bottom: mailBoxView.bottom
        anchors.left: mailBoxListViewRef.right
        anchors.leftMargin: -marginBlock * 4
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SEPERATOR_MAIN_VIEW_SCREEN)
    }
    Image {
        id: kvalIpMedia
        anchors.left: separatorLine.left
        anchors.bottom: mailBoxBottomZone.top
        anchors.bottomMargin: -marginBlock*6
        scale: 0.7
    }

    Item {
        id: mailBoxInformationZone

        anchors.top: mailBoxListViewRef.top
        anchors.left: mailBoxListViewRef.right
        height: mailBoxListViewRef.height * 0.8
        width: mailBoxListViewRef.width

        Text {
            id: mailBoxCurrentMailTitleTxt

            anchors.top: parent.top
            anchors.topMargin: marginBlock
            anchors.left: parent.left
            anchors.leftMargin: marginBlock * 4
            color: "#6aa6cb"
            font {
              family: localFont.name;
              pixelSize: (1+(parent.width * 0.03)) +
                         ((1 + (parent.width * 0.03)) * 0.333)
            }
        }
        Text {
            id: mailBoxCurrentMailRecvDateTxt

            anchors.top: mailBoxCurrentMailTitleTxt.bottom
            anchors.topMargin: marginBlock
            anchors.left: mailBoxCurrentMailTitleTxt.left
            color: "white"
            font {
              family: localFont.name;
              pixelSize: (1+(parent.width * 0.025)) +
                         ((1 + (parent.width * 0.025)) * 0.333)
            }
        }
        Text {
            id: mailBoxCurrentMailSeverityTxt

            anchors.top: mailBoxCurrentMailRecvDateTxt.top
            anchors.right: parent.right
            anchors.rightMargin: marginBlock * 6
            color: "white"
            font {
              family: localBoldFont.name;
              pixelSize: (1+(parent.width * 0.025)) +
                         ((1 + (parent.width * 0.025)) * 0.333)
            }
        }
        Text {
            id: mailBoxCurrentMailMsgTxt

            anchors.top: mailBoxCurrentMailRecvDateTxt.bottom
            anchors.topMargin: marginBlock * 6
            anchors.left: mailBoxCurrentMailRecvDateTxt.left
            width: parent.width - (marginBlock * 6)
            wrapMode: Text.WordWrap
            color: "white"
            font {
              family: localFont.name;
              pixelSize: (1+(parent.width * 0.028)) +
                         ((1 + (parent.width * 0.028)) * 0.333)
            }
        }
    }

    Component {
            id : mailBoxComponentDelegate
            Row {
                Column {
                    Rectangle {
                        x: marginBlock
                        width: mailBoxListViewRef.width
                        height: mailBoxListViewRef.height/9
                        color: "transparent"
                        opacity: mailSeenFlag
                        Text {
                            id: mainMailTitle

                            width: parent.width - (marginBlock * 8)
                            height: parent.height
                            y: parent.y + marginBlock
                            x: marginBlock * 2
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: mailColor
                            text: mailIndex + '    ' + mailTitle
                            font {
                              family: localFont.name;
                              pixelSize: (1+(mailBoxListViewRef.width * 0.027))+
                                         ((1 + (mailBoxListViewRef.width * 0.027)) * 0.333)
                            }
                        }
                        Text {
                            anchors.left: mainMailTitle.left
                            anchors.leftMargin: marginBlock * 5
                            anchors.top: mainMailTitle.bottom
                            anchors.topMargin: -marginBlock * 5
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: mailColor2
                            text: mailListInfo
                            font {
                              family: localBoldFont.name;
                              pixelSize: (1+(mailBoxListViewRef.width * 0.023))+
                                         ((1 + (mailBoxListViewRef.width * 0.023)) * 0.333)
                            }
                        }
                        Image {
                            anchors.right: parent.right
                            anchors.rightMargin: marginBlock * 12
                            anchors.verticalCenter: parent.verticalCenter
                            source: mailSeenFlagIcon
                            scale: 0.8
                        }
                    }
                }
            }
        }

    ListModel {
         id: mailBoxModel
         //Template to Use
         ListElement {
             mailIndex: ""
             mailTitle: ""
             mailListInfo: ""
             mailSeverity: ""
             mailDate: ""
             mailColor: ""
             mailColor2: ""
             mailContent: ""
             mailSeenFlag : 1
             mailSeenFlagIcon: ""
         }
     }

    ListView {
        id: mailBoxListView
        anchors.fill: mailBoxListViewRef
        enabled: true
        opacity: 1
        focus: true

        model: mailBoxModel
        highlight: highlightMailBoxComponent
        highlightFollowsCurrentItem: false
        clip: true
        keyNavigationWraps: true
        interactive: false
        delegate: mailBoxComponentDelegate
    }

    Component {
        id: highlightMailBoxComponent

        Rectangle {
            id: highlightFavRect

            x: mailBoxListView.currentItem.x
            y: mailBoxListView.currentItem.y
            width: mailBoxListViewRef.width
            height: mailBoxListViewRef.height/9 + marginBlock
            color: "transparent"
            Image {
                x: parent.x
                y: parent.y
                width: mailBoxListViewRef.width - (marginBlock*2)
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
