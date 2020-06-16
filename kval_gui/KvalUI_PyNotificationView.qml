import QtQuick 2.2
import QtQuick.Window 2.1
import QtGraphicalEffects 1.0
import QtQuick.Controls.Styles 1.2
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_Utils.js" as Utils
import kval.gui.qml 1.0


Item {
    id: alertview
    width: rootRectangle.width; 
    height: rootRectangle.height;
    x: rootRectangle.x
    y: rootRectangle.y

    enabled: false
    opacity: 1
    focus: false
    visible: true

    property var lpath: kvalUiConfigManager.qmlGetPath
    property var  g_alertType: {
        "error":
        {header: "Error Notification",
            icon: lpath(KvalUiConfigManager.ALERT_LOGO_PNG),
            color1: "#ae0000",
            color2: "#510001"},
        "info":
        {header: "Information Notification",
            icon: lpath(KvalUiConfigManager.INFO_LOGO_PNG),
            color1: "#0057fe",
            color2: "#001e58"},
        "progress":
        {header: "",
            icon: lpath(KvalUiConfigManager.INFO_LOGO_PNG),
            color1: "#0057fe",
            color2: "#001e58"},
        "success":
        {header: "Success Notification",
            icon: lpath(KvalUiConfigManager.SUCCESS_LOGO_PNG),
            color1: "#35901d",
            color2: "#005609"},
        "warning":
        {header: "Warning Notification",
            icon: lpath(KvalUiConfigManager.ALERT_LOGO_PNG),
            color1: "#ff6600",
            color2: "#502000"}}

    function displayview()
    {
        visible = true
    }

    function clearview()
    {
        internalclearview()
    }

    function checkActiveProgress()
    {
        return loadingSpinner.running()
    }

    function stopProgressBar()
    {
        if(loadingSpinner.running())
            internalclearview();
    }

    function internalclearview()
    {
        if(alertitem.visible)
            alertitem.deactivate()

//        visible = false

        if(loadingSpinner.running())
            loadingSpinner.stop()

        if(notifTextAnim.running)
            notifTextAnim.stop()

        if(notifPopUpNotifTimer.running)
            notifPopUpNotifTimer.stop();

    }

    function requestAlertDisplay(alert, header, body)
    {
        if(!mainScreenView.mainScreenHasBooted())
            return

        var alertmap = g_alertType['info']
        var fAlert = alert.toString().toLowerCase()

        if (fAlert in g_alertType){
            alertmap = g_alertType[fAlert]
        }

        displayAlert((!header) ? alertmap.header : header, body, alertmap)
        displayview()
    }

    function requestProgressDisplay(header, body, numval)
    {
        if(!mainScreenView.mainScreenHasBooted())
            return

        var alertmap = g_alertType['progress']
        displayLoading((!header) ? alertmap.header : header,
                       body, alertmap, numval)
        displayview()
    }

    function displayAlert(header, body, alertmap)
    {
        if(!mainScreenView.mainScreenHasBooted())
            return

        if(notifPopUpNotifTimer.running)
            notifPopUpNotifTimer.stop();

        if(loadingSpinner.running())
            loadingSpinner.stop()

        notifText.text = header;
        notifTextMsg.text = body
        notifRect.color1 = alertmap.color1
        notifRect.color2 = alertmap.color2
        notifIcon.source = alertmap.icon;
        notifIcon.visible = true
        if(!alertitem.visible)
            alertitem.activate()
        notifPopUpNotifTimer.start();
    }

    function updateProgressBar(progressValue)
    {
        //TODO: Do we need to handle a progress bar in alert notifications ?
    }

    function displayLoading(header, body, alertmap, numval)
    {
        if(!mainScreenView.mainScreenHasBooted())
            return

        if(notifPopUpNotifTimer.running)
            notifPopUpNotifTimer.stop();

        //@TODO: handle progress ring with "numval" arg...
        notifText.text = header;
        notifTextMsg.text = body

        if(loadingSpinner.running())
            return

        notifRect.color1 = alertmap.color1
        notifRect.color2 = alertmap.color2
        notifIcon.visible = false
        if(!alertitem.visible)
            alertitem.activate()
        loadingSpinner.start()
    }

    Timer {
        id: notifPopUpNotifTimer
        interval: 10000;
        running: false;
        repeat: false;
        onTriggered: {
            logger.info("notifPopUpNotifTimer triggered")
            if(notifTextAnim.running)
                notifTextAnim.stop()

            internalclearview()
            notifPopUpNotifTimer.running = false;
            notifPopUpNotifTimer.repeat = false;
        }
    }

    Item {
        id: alertitem
        function activate() {
            animon.start()
        }
        function deactivate() {
            animoff.start()
        }
        width: parent.width * 0.3
        height: parent.height * 0.08
        anchors.top: parent.top
        anchors.topMargin: marginBlock * 2
        anchors.horizontalCenter: parent.horizontalCenter
        scale: 0
        visible: false
        ParallelAnimation {
            id: animon
            NumberAnimation {
                target: alertitem;
                property: "scale";
                from:0; to: 0.95;
                duration: 200;
                easing {type: Easing.OutBack; overshoot: 1.8 }
            }
            NumberAnimation {
                target: alertitem;
                property: "opacity";
                from:0; to: 1;
                duration: 200
            }
            onStarted: { alertitem.visible = true }
        }
        ParallelAnimation {
            id: animoff
            NumberAnimation {
                target: alertitem;
                property: "scale";
                from: 0.95; to: 0;
                duration: 200;
                easing {type: Easing.InBack; overshoot: 1.8 }
            }
            NumberAnimation {
                target: alertitem;
                property: "opacity";
                from:1; to: 0;
                duration: 200
            }
            onStopped:{ alertitem.visible = false }
        }

        DropShadow {
            anchors.fill: notifRect
            horizontalOffset: 0
            verticalOffset: 0
            radius: 8.0
            samples: 17
            color: "black"
            source: notifRect
            opacity: 0.3
        }

        Rectangle {
            id: notifRect
            anchors.fill: parent
            property var color1: ""
            property var color2: ""
            clip: true
            radius: 5
            opacity: 0.5
            gradient: Gradient {
                GradientStop { position: 1.0; color: notifRect.color2 }
                GradientStop { position: 0.0; color: notifRect.color1 }
            }
        }


        Item {
            id: itemicon
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: marginBlock
            width: parent.height * 0.9
            height: parent.height * 0.9

            Image{
                id: notifIcon
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                scale: 0.7
            }
            Item {
                id: loadingSpinner
                function running(){
                    return (anim1.running || anim2.running)
                }
                function start() {
                    if (loadingSpinner.running())
                        return
                    loadingSpinner.visible = true
                }
                function stop() {
                    loadingSpinner.visible = false
                }

                width: parent.width * 0.8
                height: width * 0.5
                anchors.centerIn: parent
                visible: false
                Item {
                    id: semicirc
                    anchors.fill: parent
                    clip:true

                    DropShadow {
                        anchors.fill: circ
                        horizontalOffset: 0
                        verticalOffset: 0
                        radius: 8.0
                        samples: 17
                        color: "black"
                        source: circ
                        opacity: 0.7
                    }

                    Rectangle{
                        id: circ
                        width: parent.width
                        height: parent.width
                        anchors.centerIn: parent
                        color: "transparent"
                        border.width: 4
                        radius:1000
                        border.color: "white"
                    }

                    RotationAnimation on rotation
                    {
                        id: anim1
                        running: loadingSpinner.visible
                        loops: Animation.Infinite
                        direction: RotationAnimation.Clockwise
                        from: 0
                        to: 360
                        duration: 600
                    }
                }
                Item {
                    id: semicirc2
                    width: parent.width *0.65
                    height: width*0.5
                    anchors.centerIn: parent
                    clip:true
                    DropShadow {
                        anchors.fill: circ2
                        horizontalOffset: 0
                        verticalOffset: 0
                        radius: 8.0
                        samples: 17
                        color: "black"
                        source: circ2
                        opacity: 0.7
                    }
                    Rectangle{
                        id: circ2
                        width: parent.width
                        height: parent.width
                        anchors.centerIn: parent
                        color: "transparent"
                        border.width: 4
                        radius:1000
                        border.color: "white"
                    }

                    RotationAnimation on rotation
                    {
                        id: anim2
                        running: loadingSpinner.visible
                        loops: Animation.Infinite
                        direction: RotationAnimation.Counterclockwise
                        from: 360
                        to: 0
                        duration: 1000
                    }
                }
            }
        }

        Item {
            id: notificationTextArea
            anchors.left: itemicon.right
            anchors.leftMargin: marginBlock * 2
            anchors.right: parent.right
            anchors.rightMargin: marginBlock
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            opacity: 1
            clip: true
            Item {
                id: headitem
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height * 0.3

                Text{
                    id: notifText
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: marginBlock
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                    color: "white"
                    style: Text.Raised
                    styleColor: "#424242"
                    font {
                        bold: true
                        family: localFont.name;
                        pixelSize: Utils.scaled(24)
                    }
                }
            }
            Item {
                id: bodyitem
                anchors.top: headitem.bottom
                anchors.left: headitem.left
                anchors.right: headitem.right
                height: parent.height * 0.7

                Text{
                    id: notifTextMsg
                    anchors.verticalCenter: parent.verticalCenter
                    color: "white"
                    style: Text.Raised
                    styleColor: "#424242"
                    font {
                        bold: true
                        family: localFont.name;
                        pixelSize: Utils.scaled(18)
                    }
                    SequentialAnimation {
                        id: notifTextAnim
                        running:
                            (notifTextMsg.contentWidth > bodyitem.width) ?
                                true : false
                        loops: Animation.Infinite
                        onStopped: notifTextMsg.x = notifText.x

                        NumberAnimation
                        {
                            target: notifTextMsg;
                            property: "x";
                            from: notifTextMsg.x;
                            to: notifTextMsg.x -
                                (notifTextMsg.contentWidth - bodyitem.width) ;
                            duration: 4000
                        }
                        NumberAnimation
                        {
                            target: notifTextMsg;
                            property: "x";
                            to: notifTextMsg.x;
                            from: notifTextMsg.x -
                                (notifTextMsg.contentWidth - bodyitem.width) ;
                            duration: 100
                        }
                    }
                }
            }
        }
    }
}
