import QtQuick 2.3
import QtGraphicalEffects 1.0
import "../KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item
{
    id: gridViewRef

    x: parent.x
    y: parent.y
    width: parent.width
    height: parent.height

//------------------------------------------------------------------------------
// Global Vars
//------------------------------------------------------------------------------
    property Item itemsZoneRef               : null;
    property int g_gridmode : IPTV_Constants.ENUM_APPS_BOXWARE_MODE
    property GridView g_currentListView : null
    property int g_gridAnimationDuration : 550

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
    function setViewMode(gridMode)
    {
        g_gridmode = gridMode
    }

    function applyMode()
    {
        if(g_gridmode === IPTV_Constants.ENUM_APPS_BOXWARE_MODE)
        {
            g_currentListView = itemsList
        }
        else if(g_gridmode === IPTV_Constants.ENUM_APPS_RECTANGLEWARE_MODE)
        {
            g_currentListView = itemsRectList
        }
        else if(g_gridmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE)
        {
            g_currentListView = itemsCoverList
        }
        g_currentListView.currentIndex = 0
        g_currentListView.positionViewAtBeginning();
    }

    function refreshGridModeDisplay()
    {
        if(g_gridmode === IPTV_Constants.ENUM_APPS_BOXWARE_MODE)
        {
            itemsRectList.visible = false
            itemsRectList.enabled = false
            itemsRectList.focus = false
            itemsRectList.model = 0

            itemsCoverList.visible = false
            itemsCoverList.enabled = false
            itemsCoverList.focus = false
            itemsCoverList.model = 0

            itemsList.model = itemModel
            g_currentListView = itemsList
        }
        else if(g_gridmode === IPTV_Constants.ENUM_APPS_RECTANGLEWARE_MODE)
        {
            itemsList.visible = false
            itemsList.enabled = false
            itemsList.focus = false
            itemsList.model = 0

            itemsCoverList.visible = false
            itemsCoverList.enabled = false
            itemsCoverList.focus = false
            itemsCoverList.model = 0

            itemsRectList.model = itemModel
            g_currentListView = itemsRectList
        }
        else if(g_gridmode === IPTV_Constants.ENUM_APPS_COVERWARE_MODE)
        {
            itemsList.visible = false
            itemsList.enabled = false
            itemsList.focus = false
            itemsList.model = 0

            itemsRectList.visible = false
            itemsRectList.enabled = false
            itemsRectList.focus = false
            itemsRectList.model = 0

            itemsCoverList.model = itemModel
            g_currentListView = itemsCoverList
        }

        g_currentListView.visible = true
        g_currentListView.enabled = true
        g_currentListView.focus = true
    }

    function activeList()
    {
        refreshGridModeDisplay()
    }
    function clear()
    {
        itemModel.clear()
    }
    function clearDisplay()
    {
        itemsList.visible = false
        itemsList.enabled = false
        itemsList.focus = false
        itemsRectList.visible = false
        itemsRectList.enabled = false
        itemsRectList.focus = false
        itemsCoverList.visible = false
        itemsCoverList.enabled = false
        itemsCoverList.focus = false

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
        g_currentListView.moveCurrentIndexUp()
    }
    function moveDown()
    {
        g_currentListView.moveCurrentIndexDown()
    }
    function moveRight()
    {
        g_currentListView.moveCurrentIndexRight()
    }
    function moveLeft()
    {
        g_currentListView.moveCurrentIndexLeft()
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
    Image {
        id: refBlockCoverWareApp
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_COVERWARE_REF)
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
            anchors.verticalCenter: itemsList.currentItem.verticalCenter
            scale: 1.16
            z: 2
            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_CAT_SELECTOR)
            }
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 1; to: 0.3; duration: 800
                }
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 0.3; to: 1; duration: 800
                }
            }
        }
    }
    Component {
        id: highlightRectWareItem
        Item {
            id: highlightRect
            anchors.horizontalCenter: itemsRectList.currentItem.horizontalCenter
            anchors.verticalCenter: itemsRectList.currentItem.verticalCenter
            scale: 1.16
            z: 2
            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_RECTWARE_SELECTOR)
            }
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 1; to: 0.3; duration: 800
                }
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 0.3; to: 1; duration: 800
                }
            }
        }
    }
    Component {
        id: highlightCoverWareItem
        Item {
            id: highlightRect
            anchors.horizontalCenter: itemsCoverList.currentItem.horizontalCenter
            anchors.verticalCenter: itemsCoverList.currentItem.verticalCenter
            scale: 1.14
            z: 2
            Image {
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_COVERWARE_SELECTOR)
            }
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 1; to: 0.3; duration: 800
                }
                NumberAnimation
                {
                    target: highlightRect;
                    property: "opacity";
                    from: 0.3; to: 1; duration: 800
                }
            }
        }
    }

    Component {
        id : itemsListBoxWareComponent

        Item {
            id: wrapper
            width: itemBlockRectWareImg.width
            height: itemBlockRectWareImg.height
            scale: GridView.isCurrentItem ? 1.16 : 1
            z: GridView.isCurrentItem ? 1 : 0

            Behavior on scale {
                PropertyAnimation {
                    target: wrapper;
                    properties: "scale";
                    duration: 50;
                }
            }

            Item {
                id: itemBlockRectWareBack
                anchors.left: itemBlockRectWareImg.left
                anchors.leftMargin: itemBlockRectWareImg.width * 0.089
                anchors.right: itemBlockRectWareImg.right
                anchors.rightMargin: itemBlockRectWareImg.width * 0.089
                anchors.top: itemBlockRectWareImg.top
                anchors.topMargin: itemBlockRectWareImg.height * 0.0496
                anchors.bottom: itemBlockRectWareImg.bottom
                anchors.bottomMargin: itemBlockRectWareImg.height * 0.2
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
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_COVERWARE_MASK)
                    visible: false
                }
            }
            Image {
                id: itemBlockRectWareImg
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_BOXWARE_REF)
            }
            Image {
                width: itemBlockRectWareBack.width - marginBlock*0.3
                height: itemBlockRectWareBack.height - marginBlock*0.3
                anchors.centerIn: itemBlockRectWareBack
                fillMode: (sourceSize.width > itemBlockRectWareBack.width ||
                           sourceSize.height > itemBlockRectWareBack.height) ?
                           Image.PreserveAspectFit :
                           Image.Pad
                source: itemIcon
            }
            Rectangle {
                id: ticket
                anchors.right: itemBlockRectWareBack.right
                anchors.rightMargin: marginBlock*0.5
                anchors.bottom: itemBlockRectWareBack.bottom
                anchors.bottomMargin: marginBlock*0.5
                width: ticketTxt.width * 1.16
                height: itemBlockRectWareBack.height * 0.15
                opacity: itemDuration ? 0.7 : 0
                color: "black"
                scale: wrapper.GridView.isCurrentItem ? 1.16 : 1
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
            Item {
                anchors.top: itemBlockRectWareBack.bottom
                anchors.topMargin: marginBlock
                anchors.left: itemBlockRectWareBack.left
                anchors.leftMargin: marginBlock
                width: itemBlockRectWareBack.width - marginBlock
                height: (itemBlockRectWareBack.height * 0.3) - marginBlock
                Text {
                    id: label
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignLeft
                    color: "white"
                    elide:Text.ElideRight
                    textFormat: Text.AutoText
                    text: itemLabel
                    font {
                      family: localAdventFont.name;
                      pixelSize: itemBlockRectWareImg.height * 0.1
                    }
                }
                Glow {
                    anchors.fill: label
                    radius: 8
                    opacity: wrapper.GridView.isCurrentItem ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: label
                }
            }
        }
    }

    Component {
        id : itemsListRectangleWareComponent

        Item {
            id: wrapper
            width: itemBlockRectWareImg.width
            height: itemBlockRectWareImg.height
            scale: GridView.isCurrentItem ? 1.16 : 1
            z: GridView.isCurrentItem ? 1 : 0

            Behavior on scale {
                PropertyAnimation {
                    target: wrapper;
                    properties: "scale";
                    duration: 50;
                }
            }

            Item {
                id: itemBlockRectWareBack
                anchors.left: itemBlockRectWareImg.left
                anchors.leftMargin: itemBlockRectWareImg.width * 0.0269
                anchors.right: itemBlockRectWareImg.right
                anchors.rightMargin: itemBlockRectWareImg.width * 0.0269
                anchors.top: itemBlockRectWareImg.top
                anchors.topMargin: itemBlockRectWareImg.height * 0.0496
                anchors.bottom: itemBlockRectWareImg.bottom
                anchors.bottomMargin: itemBlockRectWareImg.height * 0.2
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
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_RECTANGLEWARE_MASK)
                    visible: false
                }
            }
            Image {
                id: itemBlockRectWareImg
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_RECTANGLEWARE_REF)
            }
            Image {
                width: itemBlockRectWareBack.width - marginBlock*0.3
                height: itemBlockRectWareBack.height - marginBlock*0.3
                anchors.centerIn: itemBlockRectWareBack
                fillMode: (sourceSize.width > itemBlockRectWareBack.width ||
                           sourceSize.height > itemBlockRectWareBack.height) ?
                           Image.PreserveAspectFit :
                           Image.Pad
                source: itemIcon
            }
            Rectangle {
                id: ticket
                anchors.right: itemBlockRectWareBack.right
                anchors.rightMargin: marginBlock*0.5
                anchors.bottom: itemBlockRectWareBack.bottom
                anchors.bottomMargin: marginBlock*0.5
                width: ticketTxt.width * 1.16
                height: itemBlockRectWareBack.height * 0.15
                opacity: itemDuration ? 0.7 : 0
                color: "black"
                scale: wrapper.GridView.isCurrentItem ? 1.16 : 1
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
            Item {
                anchors.top: itemBlockRectWareBack.bottom
                anchors.topMargin: marginBlock
                anchors.left: itemBlockRectWareBack.left
                anchors.leftMargin: marginBlock
                width: itemBlockRectWareBack.width - marginBlock
                height: (itemBlockRectWareBack.height * 0.3) - marginBlock
                Text {
                    id: label
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignLeft
                    color: "white"
                    elide:Text.ElideRight
                    textFormat: Text.AutoText
                    text: itemLabel
                    font {
                      family: localAdventFont.name;
                      pixelSize: itemBlockRectWareImg.height * 0.1
                    }
                }
                Glow {
                    anchors.fill: label
                    radius: 8
                    visible: false
                    opacity: wrapper.GridView.isCurrentItem ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: label
                }
            }
        }
    }

    Component {
        id : itemsListCoverWareComponent

        Item {
            id: wrapper
            width: itemBlockCoverWareImg.width
            height: itemBlockCoverWareImg.height
            scale: GridView.isCurrentItem ? 1.14 : 1
            z: GridView.isCurrentItem ? 1 : 0
            Behavior on scale {
                PropertyAnimation {
                    target: wrapper;
                    properties: "scale";
                    duration: 50;
                }
            }

            Item {
                id: itemBlockCoverWareBack
                anchors.left: itemBlockCoverWareImg.left
                anchors.leftMargin: itemBlockCoverWareImg.width * 0.05
                anchors.right: itemBlockCoverWareImg.right
                anchors.rightMargin: itemBlockCoverWareImg.width * 0.05
                anchors.top: itemBlockCoverWareImg.top
                anchors.topMargin: itemBlockCoverWareImg.height * 0.0285
                anchors.bottom: itemBlockCoverWareImg.bottom
                anchors.bottomMargin: itemBlockCoverWareImg.height * 0.128

                Image {
                    anchors.fill: parent
//                    fillMode: (sourceSize.width > itemBlockCoverWareBack.width ||
//                               sourceSize.height > itemBlockCoverWareBack.height) ?
//                               Image.PreserveAspectFit :
//                               Image.Pad

                    fillMode: Image.Stretch
                    source: itemBack
                    asynchronous: true
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
                    source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_COVERWARE_MASK)
                    visible: false
                }
            }
            Image {
                id: itemBlockCoverWareImg
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_COVERWARE_REF)
            }
            Image {
                width: itemBlockCoverWareBack.width - marginBlock*0.3
                height: itemBlockCoverWareBack.height - marginBlock*0.3
                anchors.centerIn: itemBlockCoverWareBack
                fillMode: (sourceSize.width > itemBlockCoverWareBack.width ||
                           sourceSize.height > itemBlockCoverWareBack.height) ?
                           Image.PreserveAspectFit :
                           Image.Pad
                source: itemIcon
            }
            Rectangle {
                id: ticket
                anchors.right: itemBlockCoverWareBack.right
                anchors.rightMargin: marginBlock*0.5
                anchors.bottom: itemBlockCoverWareBack.bottom
                anchors.bottomMargin: marginBlock*0.5
                width: ticketTxt.width * 1.16
                height: itemBlockCoverWareBack.height * 0.1
                opacity: itemDuration ? 0.7 : 0
                color: "black"
                scale: wrapper.GridView.isCurrentItem ? 1.16 : 1
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
            Item {
                anchors.top: itemBlockCoverWareBack.bottom
                anchors.topMargin: marginBlock*0.5
                anchors.left: itemBlockCoverWareBack.left
                anchors.leftMargin: marginBlock
                width: itemBlockCoverWareBack.width - marginBlock
                height: (itemBlockCoverWareBack.height * 0.3) - marginBlock
                Text {
                    id: label
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignLeft
                    color: "white"
                    elide:Text.ElideRight
                    textFormat: Text.AutoText
                    text: itemLabel
                    font {
                      family: localAdventFont.name;
                      pixelSize: itemBlockCoverWareImg.height * 0.08
                    }
                }
                Glow {
                    anchors.fill: label
                    radius: 8
                    opacity: wrapper.GridView.isCurrentItem ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: label
                }
            }
        }
    }

    ListModel {
         id: itemModel
         ListElement {
             itemBack: ""
             itemDefBack: ""
             itemMask: ""
             itemBlockRect: ""
             itemFanart: ""
             itemIcon: ""
             itemLabel: ""
             itemDuration: ""
             itemPlot: ""
             itemHdl: ""
             itemUrl: ""
             itemIsFolder: false
         }
    }
    Rectangle {
        id: gridViewRect
        anchors.fill: itemsZone
        color: "transparent"

        GridView {
            id: itemsList
            anchors.left: parent.left
            anchors.leftMargin: marginBlock*3
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: marginBlock*4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock*4
            cellHeight: refBlockApp.height
            cellWidth: refBlockApp.width
            enabled: false
            visible: false
            focus: false

            highlight: highlightItem
            preferredHighlightBegin: parent.x + parent.height * 0.2
            preferredHighlightEnd: parent.x + parent.height * 0.5
            highlightFollowsCurrentItem: true
            highlightRangeMode: GridView.ApplyRange
            clip: false
            keyNavigationWraps: false
            interactive: false

            delegate: itemsListBoxWareComponent
        }
        GridView {
            id: itemsRectList
            anchors.left: parent.left
            anchors.leftMargin: marginBlock*3
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: marginBlock*4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock*4
            cellHeight: refBlockRectWareApp.height
            cellWidth: refBlockRectWareApp.width * 1.08

            enabled: false
            visible: false
            focus: false

            highlight: highlightRectWareItem
            preferredHighlightBegin: parent.x + parent.height * 0.2
            preferredHighlightEnd: parent.x + parent.height * 0.5
            highlightFollowsCurrentItem: true
            highlightRangeMode: GridView.ApplyRange
            clip: false
            keyNavigationWraps: false
            interactive: false

            delegate: itemsListRectangleWareComponent
        }
        GridView {
            id: itemsCoverList
            anchors.left: parent.left
            anchors.leftMargin: marginBlock*3
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: marginBlock*4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock*4
            cellHeight: refBlockCoverWareApp.height
            cellWidth: refBlockCoverWareApp.width * 1.1
            enabled: false
            visible: false
            focus: false

            highlight: highlightCoverWareItem
            preferredHighlightBegin: parent.x + parent.height * 0.4
            preferredHighlightEnd: parent.x + parent.height * 0.2
            highlightFollowsCurrentItem: true
            highlightRangeMode: GridView.ApplyRange
            clip: false
            keyNavigationWraps: false
            interactive: false

            delegate: itemsListCoverWareComponent
        }

        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: girdItemsMaskImg
        }
    }


    Image {
        id: girdItemsMaskImg
        anchors.fill: gridViewRect
        fillMode: Image.Stretch
        source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_MASKGRID)
        visible: false
    }
}
