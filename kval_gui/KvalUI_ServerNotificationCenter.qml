import QtQuick 2.3
import "KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item
{
    id: notificationCenterView

    enabled: false
    focus: false
    opacity: 0
    width: rootRectangle.width;
    height: rootRectangle.height;
    x: rootRectangle.x
    y: rootRectangle.y

    property string unseenMailNbr: '0'


    function mailBoxInfosRecv(mailBoxInfos)
    {unseenMailNbr = mailBoxInfos[0]
    }

    function newFirmwareNotif()
    {
        notifMessageTxt.text =
                "Une mise à jour est en cours de téléchargement ..."
        notifZoneDisplayTimer.stop()
        notifZoneDisplayTimer.repeat = false
        notifZoneDisplayTimer.running = false
        notificationZoneDisplayOn.start()
    }

    function newFirmwareNotifStatus(status)
    {
        if (status === 'success')
        {
            notifMessageTxt.text = "Mise jour téléchargé avec succés..."
        }
        else
        {
            notifMessageTxt.text =
            "Une erreur c'est produite lors du téléchargement de la mise jour ..."
        }
        notifZoneDisplayTimer.start()
    }

    function firmwareOK()
    {
        notifMessageTxt.text
        = "Veuillez redémarrer votre box pour appliquer la mise à jour ..."
        notificationZoneDisplayOn.start()
        notifZoneDisplayTimer.start()
    }
    function newFirmwareUsbNotifStatus(status)
    {
        notifMessageTxt.text = "Mise jour Disponible dans le support USB ..."
        notificationZoneDisplayOn.start()
        notifZoneDisplayTimer.start()
    }

    function displayServerNotificationMessage(severity, message)
    {
        notifMessageTxt.text = 'Nouveau message Reçu.' +
              '<font color="#6aa6cb"> ' +
              unseenMailNbr +
              ' Messages non lues'+
              '</font>'
        notificationZoneDisplayOn.start()
        notifZoneDisplayTimer.start()
    }

    Timer {
        id: notifZoneDisplayTimer

        interval: 7000;
        running: false;
        repeat: false;
        onTriggered: {
            notificationZoneDisplayOff.start()
            notifZoneDisplayTimer.repeat = false
            notifZoneDisplayTimer.running = false
        }
    }

    Image {
        id: notificationZone
        x: -notificationCenterView.width * 0.6
        anchors.bottom: parent.bottom

        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.NOTIFICATION_MSG_BACK)
        Image {
            id: messageIcon
            anchors.left: parent.left
            anchors.leftMargin: marginBlock
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -marginBlock
            source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.HOME_MESSAGE_ICON)
            opacity: 0.8
            scale: 0.8
        }

        Text {
            id: notifMessageTxt
            anchors.left: messageIcon.right
            anchors.leftMargin: marginBlock
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock*0.3
            color: "white"

            font {
                family: localFont.name;
                pixelSize: notificationCenterView.width * 0.014
            }
        }
        NumberAnimation {
            id: notificationZoneDisplayOn
            target: notificationZone
            properties: "x"
            from: -notificationCenterView.width * 0.6
            to: 0
            duration: 200
            onStarted: {
                notificationCenterView.opacity = 1
            }
        }
        NumberAnimation {
            id: notificationZoneDisplayOff
            target: notificationZone
            properties: "x"
            from: 0
            to: -notificationCenterView.width * 0.6
            duration: 200
            onStopped: notificationCenterView.opacity = 0
        }
    }
}
