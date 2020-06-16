import QtQuick 2.0
import kval.gui.qml 1.0
import "KvalUI_Utils.js" as Utils

Item
{
    id: subtitleSurfaceView
    opacity: 1

    function getSubsVisibilityStatus()
    {
        return currentSubsText.visible
    }

    function switchOffSubs()
    {
        currentSubsText.visible = false
    }

    function switchOnSubs()
    {
        currentSubsText.visible = true
    }

    function switchSubsVisibility()
    {
        if(currentSubsText.visible === true) currentSubsText.visible = false
        else currentSubsText.visible = true
    }

    function showSubs(subsLine)
    {
        currentSubsText.text = subsLine
        currentSubsText.opacity = 1
    }
    function hideSubs()
    {
        currentSubsText.opacity = 0;
    }
    Rectangle {
        id: localMainRectangle
        width: rootRectangle.width;
        height: rootRectangle.height
        x: rootRectangle.x
        y: rootRectangle.y
        color: "transparent"
    }

    Rectangle {
        id: subtitleRect
        x: localMainRectangle.width * 0.1
        y: rootRectangle.height - Utils.scaled(200)
        width: localMainRectangle.width * 0.8;
        height: Utils.scaled(200)
        color: "transparent"
        Text{
            id: currentSubsText
            width: parent.width
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            visible: true
            font
            {
                bold: true
                family: localFont.name
                pixelSize: Utils.scaled(45)
            }
            style: Text.Outline
            wrapMode: Text.WordWrap
            styleColor: "black"
            color: "white"
            opacity: 1
        }
    }
}
