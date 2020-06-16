import QtQuick 2.3
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item
{
    id: listViewRef

    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

//------------------------------------------------------------------------------
// Global Vars
//------------------------------------------------------------------------------
    property int g_listmode             : IPTV_Constants.ENUM_APPS_BOXWARE_MODE
    property Item itemsZoneRef          : null
    property ListView g_currentListView : null

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
    function setViewMode(listMode)
    {
        g_listmode = listMode
    }

    function applyMode()
    {
        g_currentListView = itemsList
        g_currentListView.currentIndex = 0
        g_currentListView.positionViewAtBeginning();
    }

    function refreshListModeDisplay()
    {
        g_currentListView.currentIndex = 0
        g_currentListView.opacity = 1
        g_currentListView.enabled = true
        g_currentListView.focus = true
    }
    function activeList()
    {
        refreshListModeDisplay()
    }
    function clear()
    {
        itemModel.clear()
    }
    function clearDisplay()
    {
        itemsList.opacity = 0
        itemsList.enabled = false
        itemsList.focus = false
    }
    function insertElement(index, element)
    {
        itemModel.insert(index, element)
    }
    function getCurrentUrl()
    {
        return itemModel.get(g_currentListView.currentIndex).itemUrl
    }
    function getCurrentLabel()
    {
        return itemModel.get(g_currentListView.currentIndex).itemLabel
    }
    function getCurrentPlot()
    {
        return itemModel.get(g_currentListView.currentIndex).itemPlot
    }
    function getCurrentFanart()
    {
        return itemModel.get(g_currentListView.currentIndex).itemFanart
    }
    function isFolderElement()
    {
        logger.info("LIST MODE isFolderElement")
        return itemModel.get(g_currentListView.currentIndex).itemIsFolder
    }
    function isListEnabled()
    {
        return g_currentListView.enabled
    }
    function setOpacity(opacity)
    {
        g_currentListView.opacity = opacity
    }
    function getCurrentIndex()
    {
        return g_currentListView.currentIndex
    }
    function moveUp()
    {
        g_currentListView.decrementCurrentIndex()
    }
    function moveDown()
    {
        g_currentListView.incrementCurrentIndex();
    }
    function moveRight()
    {
    }
    function moveLeft()
    {
    }

//------------------------------------------------------------------------------
// Items
//------------------------------------------------------------------------------

    Image {
        id: refBlockApp
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_BOXWARE_REF)
        visible: false
    }
    Image {
        id: refBlockRectWareApp
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_RECTANGLEWARE_REF)
        visible: false
    }

    Component {
        id: dummyHighlight
        Item {
            x:0
            y:0
            width: 1
            height: 1
        }
    }

    Item {
        id: itemsZone

        x: itemsZoneRef.x  - marginBlock*8
        y: itemsZoneRef.y - marginBlock*2
        width: itemsZoneRef.width + marginBlock*16
        height: itemsZoneRef.height + marginBlock*4
    }

    Component {
        id: highlightItem
        Item {
            id: highlightRect
            anchors.horizontalCenter: itemsList.currentItem.horizontalCenter
            anchors.horizontalCenterOffset: -marginBlock*3
            anchors.verticalCenter: itemsList.currentItem.verticalCenter
            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LISTRECT_SELECTOR)
                visible: (g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ? false : true
            }
        }
    }
    Component {
        id : itemsListDelegateComponent
        Row {
            id: wrapperRow
            z: ListView.isCurrentItem ? 1 : 0
            Item {
                id: itemElement
                width: itemsZone.width
                height: itemBlockImg.height

                Item {
                    id: elementImageItem
                    width: itemBlockImg.width
                    height: itemBlockImg.height
                    scale: wrapperRow.ListView.isCurrentItem ? 1.2 : 1
                    Behavior on scale {
                        PropertyAnimation {
                            target: elementImageItem;
                            properties: "scale";
                            duration: 50;
                        }
                    }

                    Item {
                        id: itemBlockBackground
                        anchors.left: itemBlockImg.left
                        anchors.leftMargin: itemBlockImg.width *
                                            (itemIsFolder ? 0.089 :
                                            ((g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                            0.05 : 0.0269))
                        anchors.right: itemBlockImg.right
                        anchors.rightMargin: itemBlockImg.width *
                                             (itemIsFolder ? 0.089 :
                                             ((g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                              0.05 : 0.0269))
                        anchors.top: itemBlockImg.top
                        anchors.topMargin: itemBlockImg.height *
                                           ((g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ? 0.0285 : 0.05)
                        anchors.bottom: itemBlockImg.bottom
                        anchors.bottomMargin: itemBlockImg.height *
                                              ((g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ? 0.0285 : 0.05)
                        Image {
                            anchors.fill: parent
                            fillMode: Image.Stretch
                            source: itemBack
                            onStatusChanged: if (status === Image.Error) source = itemDefBack
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: itemsMaskImg
                            }
                        }
                        Image {
                            id: itemsMaskImg
                            anchors.fill: parent
                            fillMode: Image.Stretch
                            source: (g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_COVERWARE_MASK) :
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_RECTANGLEWARE_MASK)
                            visible: false
                        }
                    }
                    Image {
                        id: itemBlockImg
                        anchors.centerIn: parent
                        source: itemIsFolder ?
                                kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LISTBOXWARE_REF) :
                                ((g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LISTCOVERWARE_REF) :
                                    kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_LISTRECTANGLEWARE_REF))

                    }
                    Image {
                        width: itemBlockBackground.width * 0.8
                        height: itemBlockBackground.height * 0.8
                        anchors.centerIn: itemBlockBackground
                        fillMode: (sourceSize.width > itemBlockBackground.width ||
                                   sourceSize.height > itemBlockBackground.height) ?
                                   Image.PreserveAspectFit :
                                   Image.Pad
                        source: itemIcon
                    }
                    Rectangle {
                        id: ticket
                        anchors.right: itemBlockBackground.right
                        anchors.rightMargin: marginBlock*0.5
                        anchors.bottom: itemBlockBackground.bottom
                        anchors.bottomMargin: marginBlock*0.5
                        width: ticketTxt.width * 1.2
                        height: (g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                itemBlockBackground.height * 0.09 :
                                itemBlockBackground.height * 0.15
                        opacity: itemDuration ? 0.7 : 0
                        color: "black"
                    }
                    Text {
                        id: ticketTxt
                        anchors.centerIn: ticket
                        text: itemDuration
                        color: "white"
                        font {
                            family: localAdventFont.name
                            pixelSize: ticket.height * 0.7
                        }
                    }
                }

                Item {
                    id: elementLabelItem
                    anchors.left: elementImageItem.right
                    anchors.leftMargin: marginBlock * 4
                    width: parent.width - elementImageItem.width - (marginBlock*4)
                    height: elementImageItem.height
                    Text {
                        id: label
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.topMargin: marginBlock*1.5
                        width: parent.width - (marginBlock * 4)
                        horizontalAlignment: Text.AlignLeft
                        color: "white"
                        opacity: wrapperRow.ListView.isCurrentItem ? 1 : 0.7
                        elide:Text.ElideRight
                        textFormat: Text.AutoText
                        text: itemLabel
                        font {
                          family: localAdventFont.name;
                          pixelSize: (g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                      parent.height * 0.12 :
                                      parent.height * 0.18
                        }
                    }
                    Glow {
                        anchors.fill: label
                        radius: 8
                        opacity: wrapperRow.ListView.isCurrentItem ? 1 : 0
                        samples: 17
                        spread: 0.1
                        color: "white"
                        transparentBorder: true
                        source: label
                    }
                    Text {
                        id: extraInfos
                        anchors.left: parent.left
                        anchors.top: label.bottom
                        width: parent.width - (marginBlock * 4)
                        horizontalAlignment: Text.AlignLeft
                        color: "white"
                        elide:Text.ElideRight
                        textFormat: Text.AutoText
                        opacity: wrapperRow.ListView.isCurrentItem ? 0.8 : 0.6
                        wrapMode: Text.WordWrap
                        text: itemExtra
                        font {
                          family: localAdventFont.name;
                          pixelSize: (g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                      parent.height * 0.06 :
                                      parent.height * 0.12
                        }
                    }
                    Text {
                        id: plot
                        anchors.left: parent.left
                        anchors.top: label.bottom
                        anchors.topMargin: marginBlock * 4
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: marginBlock
                        width: parent.width - (marginBlock * 6)
                        horizontalAlignment: Text.AlignLeft
                        color: "white"
                        elide:Text.ElideRight
                        textFormat: Text.AutoText
                        opacity: wrapperRow.ListView.isCurrentItem ? 0.8 : 0.6
                        wrapMode: Text.WordWrap
                        text: itemPlot
                        font {
                          family: localAdventFont.name;
                          pixelSize: (g_listmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE) ?
                                      parent.height * 0.07 :
                                      parent.height * 0.13
                        }
                    }
                }
            }
        }
    }

    ListModel {
         id: itemModel
         ListElement {
             itemBack: ""
             itemDefBack: ""
             itemFanart: ""
             itemIcon: ""
             itemLabel: ""
             itemExtra: ""
             itemDuration: ""
             itemPlot: ""
             itemHdl: ""
             itemUrl: ""
             itemIsFolder: false
         }
    }
    Rectangle {
        id: listViewRect
        anchors.fill: itemsZone
        color: "transparent"
        opacity: itemsList.opacity

        ListView {
            id: itemsList
            anchors.left: parent.left
            anchors.leftMargin: marginBlock * 3
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: marginBlock * 4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock * 4
            enabled: false
            opacity: 0
            focus: false

            model: itemModel

            preferredHighlightBegin: parent.x + marginBlock
            preferredHighlightEnd: parent.x + parent.height * 0.4
            highlightFollowsCurrentItem: true
            highlightMoveVelocity: 1000
            highlightRangeMode: ListView.ApplyRange
            clip: false
            keyNavigationWraps: false
            interactive: false

            delegate: itemsListDelegateComponent

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
}
