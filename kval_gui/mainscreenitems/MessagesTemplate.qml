import QtQuick 2.3
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item {
    id: mailBoxFirstTemplate
    objectName: "MessagesTemplateForm1"

    signal mailListExtractNotify()
    property var currentObj: null
    property real marginFirstTemplate: marginBlock/2 + marginBlock/3
    property string unseenMailNbr: '0'
    property string totalMailNbr: '0'

    x: mainScreenView.mainChoiceZone().width + (marginBlock*7)
    y: mainScreenView.mainViewHeaderRect().height + (marginBlock*11)
    width: rootRectangle.width * 0.65
    height: mainScreenView.mainChoiceZone().height * 0.75+ marginFirstTemplate*2
    visible: false

    function activateTemplate()
    {
        visible = true
        mailBoxMainItemAnimOn.start()
    }

    function deactivateTemplate(obj)
    {
        currentObj = obj
        notReadMessagesItemAnimOff.start()
        totalMailsItemAnimOff.start()
        visible = false
    }

    function checkViewReady()
    {
        return true
    }

    function activateRelatedView()
    {
        mailListExtractNotify()
    }

    function infosNotify(mailBoxInfos)
    {
        unseenMailNbr = mailBoxInfos[0]
        totalMailNbr = mailBoxInfos[1]
    }

    Item {
        id: mailBoxMainItem

        x: parent.width * 0.125
        y: parent.height * 0.125
        width: parent.width * 0.35
        height: 0
        NumberAnimation {
            id: mailBoxMainItemAnimOn

            target: mailBoxMainItem
            properties: "height, opacity"
            from: 0
            to: mailBoxFirstTemplate.height * 0.7 + marginFirstTemplate
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
            onStarted: mailBoxFirstTemplate.opacity = 1
            onStopped: {
                notReadMessagesItemAnimOn.start()
                totalMailsItemAnimOn.start()
            }
        }
        NumberAnimation {
            id: mailBoxMainItemAnimOff

            target: mailBoxMainItem
            properties: "height, opacity"
            from: mailBoxFirstTemplate.height * 0.7 + marginFirstTemplate
            to: 0
            easing {type: Easing.InBack; overshoot: 1 }
            duration: 50
            onStopped: {
                mailBoxFirstTemplate.opacity = 0
                mainScreenView.activateZone(currentObj)
            }
        }

        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MAILBOX_COMP1_BACK)
            opacity: 0.5
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.4
            radius: 5
        }
        Image {
            width: parent.width * 1.13
            height: parent.height * 1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Text {
            anchors.top: parent.top
            anchors.topMargin: marginFirstTemplate * 3
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 3
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate * 3

            text: kvalUiConfigManager.retranslate +
                  kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_MAILBOX_ACTIVE)

            elide:Text.ElideLeft
            wrapMode: Text.WordWrap

            color: "white"
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: mailBoxMainItem.height * 0.07
            }
        }
        Image {
            anchors.top: parent.top
            anchors.topMargin: marginFirstTemplate * 10
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 2
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MAILBOX_IMG)
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 3
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate * 3
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate * 3

            text: kvalUiConfigManager.retranslate +
                  kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_MAILBOX_INFO)
            elide:Text.ElideLeft
            wrapMode: Text.WordWrap

            color: "#6aa6cb"
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: mailBoxMainItem.height * 0.06
            }
        }
    }
    Item {
        id: notReadMessagesItem
        x: parent.width * 0.125 +
           parent.width * 0.35 +
           marginFirstTemplate
        y: parent.height * 0.125
        width: 0
        height: parent.height * 0.35
        opacity: 0

        NumberAnimation {
            id: notReadMessagesItemAnimOn

            target: notReadMessagesItem
            properties: "width, opacity"
            from: 0
            to: mailBoxFirstTemplate.width * 0.35
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
        }
        NumberAnimation {
            id: notReadMessagesItemAnimOff

            target: notReadMessagesItem
            properties: "width, opacity"
            from: mailBoxFirstTemplate.width * 0.35
            to: 0
            easing {type: Easing.InBack; overshoot: 1 }
            duration: 50
            onStopped: mailBoxMainItemAnimOff.start()
        }

        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate*4.5
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LINE2_GREY_BACKGROUND)
            opacity: 0.6
        }
        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLUE)
            rotation: 180
            opacity: 0.65
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.4
            radius: 5
        }
        Image {
            width: parent.width * 1.13
            height: parent.height * 1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MAILBOX_ONE_MAIL)
            Image {
                id: unreadMsgBubble
                anchors.top: parent.top
                anchors.topMargin: marginFirstTemplate*7.8
                anchors.right: parent.right
                anchors.rightMargin: marginFirstTemplate * 5
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MAILBOX_MSG_BUBBLE)
                opacity: 0.8
            }
            Text {
                anchors.horizontalCenter: unreadMsgBubble.horizontalCenter
                anchors.verticalCenter: unreadMsgBubble.verticalCenter
                anchors.verticalCenterOffset: marginFirstTemplate * 0.2
                rotation: 2
                text: unseenMailNbr
                color: "white"
                font {
                  family: localAdventFont.name;
                  pixelSize: unreadMsgBubble.height * 0.4
                }
            }
            Image {
                anchors.top: parent.top
                anchors.topMargin: marginFirstTemplate*7.8
                anchors.right: parent.right
                anchors.rightMargin: marginFirstTemplate * 5
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MAILBOX_MSG_BUBBLE)
                opacity: 0.4
            }
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.3
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate
            text: kvalUiConfigManager.retranslate +
                  kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_MAILBOX_UNREAD_MSG)
            color: "white"
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: mailBoxMainItem.height * 0.05
            }
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate * 0.3
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate
            text: unseenMailNbr
            color: "#6aa6cb"
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: mailBoxMainItem.height * 0.05
            }
        }
    }
    Item {
        id: totalMailsItem
        x: parent.width * 0.125 + parent.width * 0.35 + marginFirstTemplate
        y: parent.height * 0.125 + notReadMessagesItem.height + marginFirstTemplate
        width: 0
        height: parent.height * 0.35
        opacity: 0

        NumberAnimation {
            id: totalMailsItemAnimOn

            target: totalMailsItem
            properties: "width, opacity"
            from: 0
            to: mailBoxFirstTemplate.width * 0.35
            easing {type: Easing.OutBack; overshoot: 1 }
            duration: 50
        }
        NumberAnimation {
            id: totalMailsItemAnimOff

            target: totalMailsItem
            properties: "width, opacity"
            from: mailBoxFirstTemplate.width * 0.35
            to: 0
            easing {type: Easing.InBack; overshoot: 1 }
            duration: 50
        }

        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate*4.5
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.LINE2_BACKGROUND)
            rotation: 180
            opacity: 0.6
        }
        Image {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.INFO_OVERLAY_BLACK)
            rotation: 180
            opacity: 0.65
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.4
            radius: 5
        }
        Image {
            anchors.centerIn: parent
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MAILBOX_TOTAL_MAILS)
            opacity: 0.9
        }

        Image {
            width: parent.width * 1.13
            height: parent.height * 1.13
            anchors.centerIn: parent
            fillMode: Image.Stretch
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate*0.3
            anchors.left: parent.left
            anchors.leftMargin: marginFirstTemplate
            text: kvalUiConfigManager.retranslate +
                  kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_MAILBOX_TOTAL_RECV_MSG)
            color: "white"
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: mailBoxMainItem.height * 0.05
            }
        }
        Text {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginFirstTemplate*0.3
            anchors.right: parent.right
            anchors.rightMargin: marginFirstTemplate
            text: totalMailNbr
            color: "#6aa6cb"
            style: Text.Raised
            styleColor: "black"
            font {
              family: localAdventFont.name;
              pixelSize: mailBoxMainItem.height * 0.05
            }
        }
    }
}

