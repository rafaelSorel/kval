import QtQuick 2.0
import "KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item {
    id: virtualKeyBoard
    width: rootRectangle.width;
    height: rootRectangle.height;
    x: rootRectangle.x
    y: rootRectangle.y

    enabled:false
    focus:false
    opacity:0

    signal keyEvent(string event);
    signal returnEvent();
    signal backspaceEvent();
    property real highlightGirdWidth : 80
    property int mainKeyBoardPointSize: 24 + 8
    property real keyBoardButtonOpacity: 0.9
    property real higlightControlItemWidth: 0
    property bool isCapsActivated: false
    property bool isSymbolsActivated: false
    property var objectCallBack: null
    property string defaultBackString: ""
    property string g_defaultText: ""
    property string g_virtualKeyBoardFontName: localFont.name
    property int g_textDrawType: IPTV_Constants.ENUM_TEXT_CLEAR
    property string g_realText: ""
    property bool g_appRequest: false

    function showVirtualKeyBoard(objCallBack,
                                 defaultBackStr,
                                 defaultText,
                                 textDrawType)
    {
        objectCallBack = objCallBack;
        g_defaultText = defaultText
        defaultBackString = defaultBackStr
        g_textDrawType = textDrawType
        g_realText = ""
        setDefaultBackTextEdit()
        buildKeyboard(false, false)
        enabled = true
        focus = true
        opacity = 1
    }

    function showVirtualKeyBoardFromApps(defaultBackStr,
                                         defaultText,
                                         textDrawType)
    {
        if( (!getCurrentActiveView().popUpTryGetFocus) ||
            (!getCurrentActiveView().popUpTryGetFocus()))
        {
            appsEngine.inputReply("", "")
            return
        }
        g_appRequest = true
        showVirtualKeyBoard(getCurrentActiveView(),
                            defaultBackStr,
                            defaultText,
                            textDrawType)
    }

    function setDefaultBackTextEdit()
    {
        if(textEditableZone.text.length) return
        if(g_defaultText === "")
        {
            textEditableZone.text = defaultBackString
            textEditableZone.color = "grey"
            textEditableZone.opacity = 0.8
            textEditableZone.font.italic = true
            textEditableZone.cursorPosition = 0
        }
        else
        {
            textEditableZone.text = g_defaultText
            g_realText = g_defaultText
            g_defaultText = ""
            textEditableZone.color = "white"
            textEditableZone.opacity = 1
            textEditableZone.font.italic = false
            textEditableZone.cursorPosition = g_realText.length
        }
    }

    function unSetDefaultBackTextEdit()
    {
        if(textEditableZone.text !== defaultBackString) return
        textEditableZone.text = ""
        textEditableZone.color = "white"
        textEditableZone.opacity = 1
        textEditableZone.font.italic = false
    }

    function hideVirtualKeyBoard()
    {
        enabled = false
        focus = false
        opacity = 0
        textEditableZone.text = ""
    }

    function buildKeyboard(isCaps, isSymbols)
    {

        keyBoardItemModel.clear()
        var alphabeticAzertyKeys = ['0','1','2','3','4','5','6','7','8','9',
                                  'a','z','e','r','t','y','u','i','o','p',
                                  'q','s','d','f','g','h','j','k','l', 'm',
                                  'w','x','c','v','b','n'];

        var alphabeticSymbolsKeys = [')','!','@','#','$','%','^','&','*','(',
                                  '[',']','{','}','-','_','=','+',';',':',
                                  '\'','"',',','.','<','>','/','?','\\', '|',
                                  '`','~','','£','§','µ'];

        var alphabeticAzertyCornerKeys = 
                                    ['','é','','','','','è','ê','ç','à',
                                  'A','Z','E','R','T','Y','U','I','O','P',
                                  'Q','S','D','F','G','H','J','K','L', 'M',
                                  'W','X','C','V','B','N'];
        isSymbolsActivated = false
        if(isCaps)
        {
            isCapsActivated = true
            for(var i = 0; i < alphabeticAzertyKeys.length ; i++)
            {keyBoardItemModel.insert(i,{'cellText': alphabeticAzertyCornerKeys[i],
                                        'cellCornerText': alphabeticAzertyKeys[i] });
            }
        }
        else if(isSymbols)
        {
            isSymbolsActivated = true
            for(i = 0; i < alphabeticAzertyKeys.length ; i++)
            {keyBoardItemModel.insert(i,{'cellText': alphabeticSymbolsKeys[i],
                                        'cellCornerText': '' });
            }
        }
        else
        {
            isCapsActivated = false
            for(i = 0; i < alphabeticAzertyKeys.length ; i++)
            {keyBoardItemModel.insert(i,{'cellText': alphabeticAzertyKeys[i],
                                        'cellCornerText': alphabeticAzertyCornerKeys[i]});
            }
        }
    }

    function setCharAt(str, index, chr)
    {
        if (index > str.length - 1) return str;
        return str.substr(0, index) + chr + str.substr(index + 1);
    }

    function addCharAt(str, index, chr)
    {
        if (!str.length) return chr
        else if (index > str.length - 1) return str + chr;
        return str.substr(0, index) + chr + str.substr(index);
    }

    function keyBoardKeyRightPressed()
    {
        if(keyBoardGridList.enabled)
        {
            if(spaceKeyRectSelected.opacity)
            {
                spaceKeyRectSelected.opacity = 0
                leftKeyRectSelected.opacity = 1
            }
            else if(leftKeyRectSelected.opacity)
            {
                leftKeyRectSelected.opacity = 0
                rightKeyRectSelected.opacity = 1
            }
            else if(keyBoardGridList.currentIndex + 1 === keyBoardGridList.count)
            {
                highlightGirdWidth = 0
                backSpaceKeyRectSelected.opacity = 1
            }
            else
            {keyBoardGridList.moveCurrentIndexRight()
            }
        }
        else
        {
            switch (keyBoardContorlList.currentIndex)
            {
                case 0 :
                    keyBoardGridList.currentIndex = 0;
                    highlightGirdWidth = 80
                    break;
                case 1:
                    keyBoardGridList.currentIndex = 10;
                    highlightGirdWidth = 80
                    break;
                case 2:
                    keyBoardGridList.currentIndex = 20;
                    highlightGirdWidth = 80
                    break
                case 3:
                    keyBoardGridList.currentIndex = 30;
                    highlightGirdWidth = 80
                    break
                case 4:
                    spaceKeyRectSelected.opacity = 1
                    break
                default:
                    break;
            }
            keyBoardGridList.enabled = true
            keyBoardGridList.focus = true
            keyBoardContorlList.enabled = false
            keyBoardContorlList.focus = false
            higlightControlItemWidth = 0
        }
    }

    function keyBoardKeyLeftPressed()
    {
        if(backSpaceKeyRectSelected.opacity)
        {
            keyBoardGridList.currentIndex = keyBoardGridList.count-1
            backSpaceKeyRectSelected.opacity = 0
            highlightGirdWidth = 80
        }
        else if(rightKeyRectSelected.opacity)
        {
            rightKeyRectSelected.opacity = 0
            leftKeyRectSelected.opacity = 1
        }
        else if(leftKeyRectSelected.opacity)
        {
            leftKeyRectSelected.opacity = 0
            spaceKeyRectSelected.opacity = 1
        }
        else if(spaceKeyRectSelected.opacity)
        {
            keyBoardContorlList.enabled = true
            keyBoardContorlList.focus = true
            keyBoardGridList.enabled = false
            keyBoardGridList.focus = false
            spaceKeyRectSelected.opacity = 0
            higlightControlItemWidth = keyBoardContorleBackRect.width
            keyBoardContorlList.currentIndex = 4
        }
        else
        {
            switch (keyBoardGridList.currentIndex)
            {
                case 0 :
                    keyBoardContorlList.currentIndex = 0;
                    highlightGirdWidth = 0
                    higlightControlItemWidth = keyBoardContorleBackRect.width
                    keyBoardContorlList.enabled = true
                    keyBoardContorlList.focus = true
                    keyBoardGridList.enabled = false
                    keyBoardGridList.focus = false
                    break;
                case 10:
                    keyBoardContorlList.currentIndex = 1;
                    highlightGirdWidth = 0
                    higlightControlItemWidth = keyBoardContorleBackRect.width
                    keyBoardContorlList.enabled = true
                    keyBoardContorlList.focus = true
                    keyBoardGridList.enabled = false
                    keyBoardGridList.focus = false
                    break;
                case 20:
                    keyBoardContorlList.currentIndex = 2;
                    highlightGirdWidth = 0
                    higlightControlItemWidth = keyBoardContorleBackRect.width
                    keyBoardContorlList.enabled = true
                    keyBoardContorlList.focus = true
                    keyBoardGridList.enabled = false
                    keyBoardGridList.focus = false
                    break
                case 30:
                    keyBoardContorlList.currentIndex = 3;
                    highlightGirdWidth = 0
                    higlightControlItemWidth = keyBoardContorleBackRect.width
                    keyBoardContorlList.enabled = true
                    keyBoardContorlList.focus = true
                    keyBoardGridList.enabled = false
                    keyBoardGridList.focus = false
                    break
                default:
                    keyBoardGridList.moveCurrentIndexLeft()
                    break;
            }
        }
    }

    function keyBoardKeyDownPressed()
    {
        if(keyBoardGridList.enabled)
        {
            if(backSpaceKeyRectSelected.opacity)
            {
                backSpaceKeyRectSelected.opacity = 0
                rightKeyRectSelected.opacity = 1
            }
            else if(leftKeyRectSelected.opacity  ||
                    rightKeyRectSelected.opacity ||
                    spaceKeyRectSelected.opacity)
            {
            }
            else if(keyBoardGridList.currentIndex >= 30)
            {
                highlightGirdWidth = 0
                spaceKeyRectSelected.opacity = 1
            }
            else if(keyBoardGridList.currentIndex >= 26 && 
                    keyBoardGridList.currentIndex <= 29)
            {
                highlightGirdWidth = 0
                backSpaceKeyRectSelected.opacity = 1
            }
            else
            {keyBoardGridList.moveCurrentIndexDown()
            }
        }
        else
        {keyBoardContorlList.incrementCurrentIndex()
        }
    }

    function keyBoardKeyUpPressed()
    {
        if(keyBoardGridList.enabled)
        {
            if(spaceKeyRectSelected.opacity)
            {
                spaceKeyRectSelected.opacity = 0
                keyBoardGridList.currentIndex=keyBoardGridList.count - 1
                highlightGirdWidth = 80
            }
            else if(backSpaceKeyRectSelected.opacity)
            {
                backSpaceKeyRectSelected.opacity = 0
                keyBoardGridList.currentIndex= 
                        (keyBoardGridList.currentIndex === keyBoardGridList.count - 1) 
                        ? 26 : keyBoardGridList.currentIndex
                highlightGirdWidth = 80
            }
            else if(leftKeyRectSelected.opacity ||
                    rightKeyRectSelected.opacity)
            {
                leftKeyRectSelected.opacity = 0
                rightKeyRectSelected.opacity = 0
                backSpaceKeyRectSelected.opacity = 1
            }
            else
            {keyBoardGridList.moveCurrentIndexUp()
            }
        }
        else
        {keyBoardContorlList.decrementCurrentIndex()
        }
    }

    function keyBoardKeyOkPressed()
    {
        if(keyBoardGridList.enabled)
        {
            var editableText = '';
            unSetDefaultBackTextEdit()
            if(spaceKeyRectSelected.opacity)
            {
                if(g_textDrawType === IPTV_Constants.ENUM_TEXT_HIDDEN)
                {
                    editableText = '•'
                    g_realText = addCharAt( g_realText,
                                            textEditableZone.cursorPosition,
                                            ' ');
                }
                else if(g_textDrawType === IPTV_Constants.ENUM_TEXT_CLEAR)
                {
                    editableText = ' '
                    g_realText = addCharAt( g_realText,
                                            textEditableZone.cursorPosition,
                                            editableText);
                }

                var currentIncPos = textEditableZone.cursorPosition
                textEditableZone.text = addCharAt(textEditableZone.text, 
                                                  textEditableZone.cursorPosition, 
                                                  editableText);
                textEditableZone.cursorPosition = currentIncPos+1
            }
            else if(backSpaceKeyRectSelected.opacity)
            {
                var currentPos = textEditableZone.cursorPosition - 1
                g_realText =setCharAt(g_realText,
                                      textEditableZone.cursorPosition-1,
                                      '');
                textEditableZone.text = setCharAt(textEditableZone.text, 
                                                  textEditableZone.cursorPosition-1, 
                                                  '');
                textEditableZone.cursorPosition = currentPos
            }
            else if(leftKeyRectSelected.opacity)
            {
                textEditableZone.cursorPosition = 
                        textEditableZone.cursorPosition -1
            }
            else if(rightKeyRectSelected.opacity)
            {
                textEditableZone.cursorPosition = 
                        textEditableZone.cursorPosition + 1
            }
            else
            {
                var currentIncPos = textEditableZone.cursorPosition
                if(g_textDrawType === IPTV_Constants.ENUM_TEXT_HIDDEN)
                {
                    editableText = '•'
                    g_realText = addCharAt(g_realText, 
                                        textEditableZone.cursorPosition, 
                                        keyBoardItemModel.get(keyBoardGridList.currentIndex).cellText);
                }
                else if(g_textDrawType === IPTV_Constants.ENUM_TEXT_CLEAR)
                {
                    editableText = keyBoardItemModel.get(keyBoardGridList.currentIndex).cellText
                    g_realText = addCharAt(g_realText, 
                                            textEditableZone.cursorPosition, 
                                            editableText);
                }
                textEditableZone.text = addCharAt(textEditableZone.text, 
                                                  textEditableZone.cursorPosition, 
                                                  editableText);
                textEditableZone.cursorPosition = currentIncPos+1
            }
            setDefaultBackTextEdit()
        }
        else
        {
            switch (keyBoardContorlList.currentIndex)
            {
                case 0 :
                    if(textEditableZone.text === defaultBackString) textEditableZone.text = ""
                    if(g_appRequest)
                    {
                        g_appRequest = false
                        getCurrentActiveView().popUpFocusRelinquished()
                        appsEngine.inputReply("", g_realText)
                    }
                    else
                    {
                        objectCallBack.keyBoardCallBack(g_realText)
                    }
                    hideVirtualKeyBoard();
                    break;
                case 1:
                    if(isCapsActivated) buildKeyboard(false, false)
                    else buildKeyboard(true, false)
                    break;
                case 2:
                    if(isSymbolsActivated) 
                    {
                        keyBoardControlModel.setProperty(keyBoardContorlList.currentIndex, 
                                                "cellText",
                                                "Symboles");
                        buildKeyboard(false, false)
                    }
                    else 
                    {
                        keyBoardControlModel.setProperty(keyBoardContorlList.currentIndex, 
                                                "cellText",
                                                "ABC");
                        buildKeyboard(false, true)
                    }
                    break
                case 3:
                    break
                default:
                    break;
            }
        }
    }

    function keyBoardKeyBackPressed()
    {
        textEditableZone.text = ""
        if(g_appRequest)
        {
            g_appRequest = false
            getCurrentActiveView().popUpFocusRelinquished()
            appsEngine.inputReply("", textEditableZone.text)
        }
        else
        {
            objectCallBack.keyBoardCallBack(textEditableZone.text)
        }
        hideVirtualKeyBoard();
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Up) //Up Button
        {keyBoardKeyUpPressed()
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {keyBoardKeyDownPressed()
        }
        else if (event.key === Qt.Key_Right) //Right Button
        {keyBoardKeyRightPressed()
        }
        else if (event.key === Qt.Key_Left) //Left Button
        {keyBoardKeyLeftPressed()
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {keyBoardKeyOkPressed()
        }
        else if (event.key === Qt.Key_Backspace) //Ok Button
        {keyBoardKeyBackPressed()
        }
        else {}
        event.accepted = true;
    }

    Rectangle {
        id: keyBoardLayout
        width: 1200
        height: 460
        anchors.centerIn: parent
        color: "transparent"
//        radius: 10
//        border.color: "grey"
//        border.width: 1
//        opacity: 0.75
    }

    Rectangle {
        id: textEditRect
        height: 75
        anchors.right: keyBoardLayout.right
        anchors.left: keyBoardLayout.left
        anchors.bottom: keyBoardLayout.top
        anchors.bottomMargin: marginBlock
        gradient: Gradient {
            GradientStop { position: 1.0; color: "#424242" }
            GradientStop { position: 0.0; color: "black" }
        }
        radius: 10
        border.color: "grey"
        border.width: 1
        opacity: 0.9
    }

    TextEdit {
        id: textEditableZone
        anchors.centerIn: textEditRect
        color: "white"
        focus: true
        cursorVisible: true
        font.pixelSize: 24 + 8
    }

    Rectangle {
        id: keyBoardBackground
        width: 900
        height: 450
        anchors.right: keyBoardLayout.right
        anchors.top: keyBoardLayout.top
        anchors.topMargin: marginBlock
        color: "transparent"
    }

    Rectangle {
        id: backSpaceKeyRect
        width: 350
        height: 80
        radius: 10
        border.color: "grey"
        border.width: 1
        anchors.right: keyBoardBackground.right
        anchors.rightMargin: marginBlock
        anchors.bottom: keyBoardBackground.bottom
        anchors.bottomMargin: marginBlock * 10
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#585858" }
            GradientStop { position: 1.0; color: "black" }
        }
        opacity: keyBoardButtonOpacity
    }
    Rectangle {
        id: backSpaceKeyRectSelected
        width: 350
        height: 80
        radius: 10
        opacity: 0
        border.color: "grey"
        border.width: 1
        anchors.centerIn: backSpaceKeyRect
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#58D3F7" }
            GradientStop { position: 1.0; color: "#0B4C5F" }
        }
    }
    Text {
        text: kvalUiConfigManager.retranslate + qsTr("Retour Arrière")
        font.pixelSize: mainKeyBoardPointSize
        font.family: g_virtualKeyBoardFontName
        font.bold: true
        anchors.centerIn: backSpaceKeyRect
        color: "white"
    }

    Rectangle {
        id: spaceKeyRect
        width: 530
        height: 80
        radius: 10
        border.color: "grey"
        border.width: 1
        anchors.left: keyBoardBackground.left
        anchors.bottom: keyBoardBackground.bottom
        anchors.bottomMargin: marginBlock
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#585858" }
            GradientStop { position: 1.0; color: "black" }
        }
        opacity:keyBoardButtonOpacity
    }
    Rectangle {
        id: spaceKeyRectSelected
        width: 530
        height: 80
        radius: 10
        opacity: 0
        border.color: "grey"
        border.width: 1
        anchors.centerIn: spaceKeyRect
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#58D3F7" }
            GradientStop { position: 1.0; color: "#0B4C5F" }
        }
    }
    Text {
        text: kvalUiConfigManager.retranslate + qsTr("Espace")
        font.pixelSize: mainKeyBoardPointSize
        font.family: g_virtualKeyBoardFontName
        font.bold: true
        anchors.centerIn: spaceKeyRect
        color: "white"
    }

    Rectangle {
        id: leftKeyRect
        width: 170
        height: 80
        radius: 10
        border.color: "grey"
        border.width: 1
        anchors.left: spaceKeyRect.right
        anchors.leftMargin: marginBlock
        anchors.bottom: spaceKeyRect.bottom
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#585858" }
            GradientStop { position: 1.0; color: "black" }
        }
        opacity:keyBoardButtonOpacity
    }
    Rectangle {
        id: leftKeyRectSelected
        width: 170
        height: 80
        radius: 10
        opacity: 0
        border.color: "grey"
        border.width: 1
        anchors.centerIn: leftKeyRect
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#58D3F7" }
            GradientStop { position: 1.0; color: "#0B4C5F" }
        }
    }
    Text {
        text: "<"
        font.pixelSize: mainKeyBoardPointSize
        anchors.centerIn: leftKeyRectSelected
        color: "white"
        font.family: g_virtualKeyBoardFontName
        font.bold: true
    }

    Rectangle {
        id: rightKeyRect
        width: 170
        height: 80
        radius: 10
        border.color: "grey"
        border.width: 1
        anchors.left: leftKeyRect.right
        anchors.leftMargin: marginBlock
        anchors.bottom: leftKeyRect.bottom
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#585858" }
            GradientStop { position: 1.0; color: "black" }
        }
        opacity:keyBoardButtonOpacity
    }
    Rectangle {
        id: rightKeyRectSelected
        width: 170
        height: 80
        radius: 10
        opacity: 0
        border.color: "grey"
        border.width: 1
        anchors.centerIn: rightKeyRect
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#58D3F7" }
            GradientStop { position: 1.0; color: "#0B4C5F" }
        }
    }
    Text {
        text: ">"
        font.pixelSize: 26 + 9
        anchors.centerIn: rightKeyRectSelected
        color: "white"
        font.family: g_virtualKeyBoardFontName
        font.bold: true
    }

    Grid {
        anchors.left: keyBoardBackground.left
        anchors.right: keyBoardBackground.right
        anchors.top: keyBoardBackground.top
        anchors.bottom: keyBoardBackground.bottom
        rows: 4; columns: 10; spacing: 10

        Repeater { model: 36
            Rectangle {
                id: lettreCompRect
                width: 80
                height: 80
                radius: 10
                border.color: "grey"
                border.width: 1
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#585858" }
                    GradientStop { position: 1.0; color: "black" }
                }
                opacity: keyBoardButtonOpacity
            }
        }
    }
    ListModel {
         id: keyBoardItemModel
         //Template to Use
         ListElement {
             cellText: ""
             cellCornerText: ""
         }
     }
    Component {
            id: appDelegate
            Item {
                width: 80
                height: 80
                Rectangle {
                    id: lettreCompRect
                    width: 80
                    height: 80
                    color: "transparent"
                }
                Text {
                    text: cellText
                    font.pixelSize: mainKeyBoardPointSize
                    anchors.centerIn: lettreCompRect
                    color: "white"
                    font.family: g_virtualKeyBoardFontName
                    font.bold: true
                }
                Text {
                    text: cellCornerText
                    anchors.top: lettreCompRect.top
                    anchors.left: lettreCompRect.left
                    anchors.leftMargin: marginBlock
                    font.pixelSize: 18 + 6
                    color: "white"
                    font.family: g_virtualKeyBoardFontName
                    font.bold: true
                }
            }
        }
    Component {
            id: highlightGridView
            Rectangle {
                x: keyBoardGridList.currentItem.x
                y: keyBoardGridList.currentItem.y
                width: highlightGirdWidth
                height: 80
                radius: 10
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#58D3F7" }
                    GradientStop { position: 1.0; color: "#0B4C5F" }
                }
            }
        }
    GridView {
        id: keyBoardGridList
        anchors.left: keyBoardBackground.left
        anchors.right: keyBoardBackground.right
        anchors.top: keyBoardBackground.top
        anchors.bottom: keyBoardBackground.bottom
        cellWidth: 90
        cellHeight: 90

        focus :true
        enabled: true
        model: keyBoardItemModel
        keyNavigationWraps: false
        delegate: appDelegate
        highlight: highlightGridView
        highlightFollowsCurrentItem: false
        interactive: false
     }

    //keyboard control key
    Rectangle {
        id: keyBoardContorleBackRect
        width: keyBoardLayout.width - 900 - (marginBlock*2)
        height: 450
        anchors.left: keyBoardLayout.left
        anchors.leftMargin: marginBlock
        anchors.top: keyBoardLayout.top
        anchors.topMargin: marginBlock
        color: "transparent"
    }

    ListModel {
         id: keyBoardControlModel
         ListElement {
             cellText: "Enter" //kvalUiConfigManager.retranslate + qsTr("Entrée")
         }
         ListElement {
             cellText: "Maj" //kvalUiConfigManager.retranslate + qsTr("Maj")
         }
         ListElement {
             cellText: "Symbols" //kvalUiConfigManager.retranslate + qsTr("Symboles")
         }
         ListElement {
             cellText: "Lang" //kvalUiConfigManager.retranslate + qsTr("Langue")
         }
         ListElement {
             cellText: "Ip" //kvalUiConfigManager.retranslate + qsTr("Adresse Ip")
         }
     }
    Item {
        width: keyBoardContorleBackRect.width; 
        height: keyBoardContorleBackRect.height
        anchors.left: keyBoardContorleBackRect.left
        anchors.right: keyBoardContorleBackRect.right
        anchors.top: keyBoardContorleBackRect.top
        anchors.bottom: keyBoardContorleBackRect.bottom

        Column {
            spacing: 10
            Repeater { model: 5
                Rectangle {
                    width: keyBoardContorleBackRect.width
                    height: 80
                    radius: 10
                    border.color: "grey"
                    border.width: 1
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#585858" }
                        GradientStop { position: 1.0; color: "black" }
                    }
                    opacity: keyBoardButtonOpacity
                }
            }
        }
    }
    Component {
            id: keyBoardControlDelegate
            Item {
                width: keyBoardContorleBackRect.width
                height: 80
                Rectangle {
                    id: lettreCompRect
                    width: keyBoardContorleBackRect.width
                    height: 80
                    color: "transparent"
                }
                Text {
                    text: cellText
                    font.pixelSize: mainKeyBoardPointSize
                    anchors.centerIn: lettreCompRect
                    color: "white"
                    font.family: g_virtualKeyBoardFontName
                    font.bold: true
                }
            }
        }

    ListView {
        id: keyBoardContorlList
        anchors.left: keyBoardContorleBackRect.left
        anchors.right: keyBoardContorleBackRect.right
        anchors.top: keyBoardContorleBackRect.top
        anchors.bottom: keyBoardContorleBackRect.bottom
        enabled: false
        opacity: 1
        focus: false
        spacing: 10

        model: keyBoardControlModel
        highlight: highlightContorlItem
        highlightFollowsCurrentItem: false
        clip: true
        keyNavigationWraps: true
        interactive: false
        delegate: keyBoardControlDelegate
    }

    Component {
        id: highlightContorlItem

        Rectangle {
            x: keyBoardContorlList.currentItem.x
            y: keyBoardContorlList.currentItem.y
            width: higlightControlItemWidth
            height: 80
            radius: 10
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#58D3F7" }
                GradientStop { position: 1.0; color: "#0B4C5F" }
            }
        }
    }
}
