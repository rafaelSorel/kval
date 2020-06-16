import QtQuick 2.14
import QtQuick.XmlListModel 2.0
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item {
    id: searchFirstTemplate
    objectName: "SearchTemplateForm1"

    property real marginFirstTemplate: marginBlock/2 + marginBlock/3
    property var currentObj: null;

    x: mainScreenView.mainChoiceZone().width + (marginBlock*7)
    y: mainScreenView.mainViewHeaderRect().height + (marginBlock*11)
    width: rootRectangle.width * 0.65
    height: mainScreenView.mainChoiceZone().height * 0.75+ marginFirstTemplate*2
    visible: true

    function activateTemplate()
    {
        visible = true
        animateSearchTemplateWidthOn.start()
    }

    function deactivateTemplate(obj)
    {
        currentObj = obj
        animateSearchTemplateWidthOff.start()
        visible = false
    }

    function checkViewReady()
    {
        return false
    }

    function activateRelatedView()
    {
    }
    NumberAnimation {
           id: animateSearchTemplateWidthOn
           target: searchFirstTemplate
           properties: "width"
           from: 0
           to: rootRectangle.width * 0.65
           duration: 50
           easing {type: Easing.OutBack; overshoot: 0.5 }
           onStopped:{
           }
           onStarted:{
               animateSearchTemplateOpacityOn.start()
               textSearchZoneLabel.opacity = 0.7
               searchZoneTxt.opacity = 0.7
           }
    }
    NumberAnimation {
           id: animateSearchTemplateOpacityOn
           target: searchFirstTemplate
           properties: "opacity"
           from: 0
           to: 1
           duration: 50
    }

    NumberAnimation {
           id: animateSearchTemplateWidthOff
           target: searchFirstTemplate
           properties: "width"
           from: rootRectangle.width * 0.65
           to: 0
           duration: 50
           easing {type: Easing.OutBack; overshoot: 0.5 }
           onStarted: {
               textSearchZoneLabel.opacity = 0
               searchZoneTxt.opacity = 0
               animateSearchTemplateOpacityOff.start()
           }
           onStopped: mainScreenView.activateZone(currentObj)
    }
    NumberAnimation {
           id: animateSearchTemplateOpacityOff
           target: searchFirstTemplate
           properties: "opacity"
           from: 1
           to: 0
           duration: 50
    }
    Image {
        width: searchZone.width + searchZone.width*0.134
        height: searchZone.height + searchZone.height*0.13
        anchors.centerIn: searchZone
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.FRAME_SQUARE_BIG)
    }
    Text {
        id: textSearchZoneLabel
        anchors.left: searchZone.left
        anchors.leftMargin: marginFirstTemplate*2
        anchors.bottom: searchZone.top
        text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_TEXT_SEARCH)
        color: "#E6E6E6"
        style: Text.Raised
        styleColor: "black"
        font {
            family: localFontLow.name
            pixelSize: searchZone.width * 0.03
        }
    }
    Rectangle {
        id: searchZone
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0.1
        anchors.right: parent.right
        anchors.rightMargin: parent.width * 0.1
        height: parent.height * 0.17
        radius: 2
        opacity: 0.6
        gradient: Gradient {
            GradientStop { position: 1.0; color: "#424242" }
            GradientStop { position: 0.0; color: "black" }
        }
    }
    Image {
        anchors.verticalCenter: searchZone.verticalCenter
        anchors.right: searchZone.right
        width: searchZone.width * 0.1
        height: searchZone.width * 0.1
        anchors.rightMargin: marginFirstTemplate
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.SEARCH_HOME_ICON)
    }
    Text {
        id: searchZoneTxt
        anchors.left: searchZone.left
        anchors.leftMargin: marginFirstTemplate*2
        anchors.verticalCenter: searchZone.verticalCenter
        text: kvalUiConfigManager.retranslate + kvalUiConfigManager.qmlStr(KvalUiConfigManager.STR_MAIN_SCREEN_TEXT_SEARCH_LABEL)
        color: "#A4A4A4"
        opacity: 0.7
        font {
            family: localFontLow.name
            pixelSize: searchZone.width * 0.045
        }
    }
}

