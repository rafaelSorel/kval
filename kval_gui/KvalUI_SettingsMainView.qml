import QtQuick 2.3
import "KvalUI_constant.js" as IPTV_Constants
import "KvalUI_Utils.js" as Utils
import QtGraphicalEffects 1.0
import QtQuick.XmlListModel 2.0
import QtQuick.Controls 2.0
import kval.gui.qml 1.0

Item
{
    id: settingsView
    property var lpath: kvalUiConfigManager.qmlGetPath
    property var trpath: kvalUiConfigManager.translate_path
    property var reTr: kvalUiConfigManager.retranslate
    property var tr: kvalUiConfigManager.qmlStr
    property var trid: kvalUiConfigManager.xmlGetStr

    enabled: false
    opacity: 1
    focus: false
    visible: false

    x: rootRectangle.x
    y: rootRectangle.y
    width: rootRectangle.width
    height: rootRectangle.height

    signal settingsReadyNotify()
    signal activateMainScreenView()
    signal notifyNewLiveCody(string value)
    signal notifyNewVodCody(string value)

    property bool   g_settingsFromApp: false
    property string g_mainSettingAppId: ""
    property string g_mainSettingAction: ""
    property var    g_settingItems: null
    property var    g_settingMenuItems: null
    property bool   g_waitForMenuItems: false
    property bool   g_settingsReloaded: false
    property bool   g_itemsSettingsHighlighed: false
    property bool   g_serviceMenuEntryActivated: false
    property string g_serviceMenuSecCode: ""
    readonly property var g_typeActionMap: {
        "text": virtualKeyboardReq,
        "action": actionReq,
        "bool": boolToggleReq,
        "choicemenu": staticMenuReq,
        "dynchoicemenu": dynamicMenuReq,
    }
    readonly property var g_iconMaps: {
        'choicemenu': lpath(KvalUiConfigManager.SETTINGS_SELECT_ICON),
        'dynchoicemenu': lpath(KvalUiConfigManager.SETTINGS_SELECT_ICON),
        'info': lpath(KvalUiConfigManager.SETTINGS_INFO_ICON_SMALL),
        'text': lpath(KvalUiConfigManager.SETTINGS_EDIT_ICON),
        'action': lpath(KvalUiConfigManager.SETTINGS_ACTION_ICON),
        'button': lpath(KvalUiConfigManager.SETTINGS_ACTION_ICON),
        'folder': lpath(KvalUiConfigManager.SETTINGS_ACTION_ICON),
        'bool': {
            true: lpath(KvalUiConfigManager.SETTINGS_ON_ICON),
            'true': lpath(KvalUiConfigManager.SETTINGS_ON_ICON),
            false: lpath(KvalUiConfigManager.SETTINGS_OFF_ICON),
            'false': lpath(KvalUiConfigManager.SETTINGS_OFF_ICON) }
    }
    property var categories: ({})

    //--------------------------------------------------------------------------
    // Helpers
    //--------------------------------------------------------------------------

    function group(){
        return catmodel.get(settingsMainListView.currentIndex)
    }
    function child(){
        return settingsItemsModel.get(settingsItemsListView.currentIndex)
    }
    function choice(){
        return itemChoiceModel.get(itemChoiceListView.currentIndex)
    }

    //--------------------------------------------------------------------------
    // View manipulation functions (visibility, enabling, focus...)
    //--------------------------------------------------------------------------

    function showSettingsView()
    {
        fillSettingItems(group().uid)
        registerActiveView(settingsMainView)
        activeFocusViewManager(settingsMainView)
        visible = true
        opacity = 1
    }

    function backToHomeView()
    {
        activateMainScreenView();
        settingsBack.source = ""
        settingsBackOverlay2.source = ""
        g_serviceMenuSecCode = ""
        if(g_serviceMenuEntryActivated)
        {
            g_serviceMenuEntryActivated = false
            g_settingMenuItems = null
            xmlMainSettingsModel.reload()
        }
        enabled = false
        visible = false
        focus = false
        pyNotificationView.clearview();
    }

    function activateSettingsView()
    {
        constructListModel()
        registerActiveView(settingsMainView)
        settingsBack.source = lpath(KvalUiConfigManager.SETTINGS_BACKGROUND)
        settingsBackOverlay2.source = lpath(KvalUiConfigManager.OVERLAY_BACKGROUND_BLACK_2)

        if(g_settingsFromApp)
        {
            logger.info("main Setting App: " + g_mainSettingAppId)
            if( ( g_settingMenuItems ) ||
                ( !appsEngine.initSettingWindow(g_mainSettingAppId) ) )
            {
                logger.info("settingsReadyNotify(): " + g_mainSettingAppId)
                settingsReadyNotify()
            }
            else
            {
                g_waitForMenuItems = true
                appSettingsWindowTimer.start()
            }
        }
        else
        {
            settingsReadyNotify()
        }
    }

    function popUpTryGetFocus()
    {
        if(!enabled) return false

        opacity = 0.4
        enabled = false
        focus = false
        return true
    }

    function popUpFocusRelinquished()
    {
        opacity = 1
        enabled = true
        focus = true
    }

    function langChanged()
    {
        g_settingMenuItems = null
        g_settingsReloaded = true
        // Reload settings
        constructListModel()
    }

    function endSetSetting()
    {
        if (!settingsView.enabled) return
//        kvalUiSettingsManager.refreshSettingsValue()
        itemChoiceZone.visible = false
        itemChoiceListView.opacity = 0
        itemChoiceListView.enabled = false
        itemChoiceListView.focus = false
        settingsItemsListView.enabled = true
        settingsItemsListView.focus = true
        settingsItemsListView.opacity = 1
//        fillSettingItems(group().uid)

    }

    // This function is called onces when category
    // list has been successfully parsed by settingsManager
    function constructListModel()
    {
        var localCategories = kvalUiSettingsManager.getCategoiesDisplay()
        if(!localCategories || !localCategories.length) {
            logger.error("Unabale to load settings !")
            return
        }

        catmodel.clear()
        g_settingsFromApp = false
        g_mainSettingAction = ""
        g_mainSettingAppId = ""

        for(var i = 0; i < localCategories.length ; i++)
        {
            var category = localCategories[i]
            logger.info('category: ' + category['id'])
            if(category['source'] === "external") {
                logger.info("External application needed ...")
                g_settingsFromApp = true
                g_mainSettingAppId = category['sourceref']
                continue
            }
            catmodel.insert(i, {
                'uid': category['id'],
                'label': ('label' in category) ? category['label'] : '',
                'icon' : ('icon' in category) ? trpath(category['icon']) : '',
                'src': ('source' in category) ? category['source'] : '',
                'sourceref': ('sourceref' in category) ? category['sourceref'] : '' } )

            //Fill global scope categories
            categories[category['id']] = category['items']
        }

        if(g_settingsReloaded) {
            g_settingsReloaded = false
        }
        else {
            settingsMainListView.model = catmodel
            settingsMainListView.highlight = highlightMainSettingsList
            settingsItemsListView.highlight = dummyHighlight
            settingsMainListView.currentIndex = 0
            settingsMainListView.positionViewAtBeginning();
            settingsMainListView.enabled = true
            settingsMainListView.focus = true
            settingsItemsListView.enabled = false
            settingsItemsListView.focus = false
            g_itemsSettingsHighlighed = false
        }

    }

    function fillSettingItems(categoryId)
    {
        if(!(categoryId in categories)){
            logger.info("!!!!!!! Unable to find: " + categoryId)
            return
        }

        var items = categories[categoryId]
        settingsItemsModel.clear()
        for(var i = 0; i < items.length ; i++)
        {
            var item = items[i]
            var value = item['default'];
            if(item['type'] !== 'action'){
                value = kvalUiSettingsManager.value(categoryId, item['id'])
            }

            var itemOpacity = getItemOpacity(item['id'], value)
            var itemIco = (item['type'] in g_iconMaps) ?
                            g_iconMaps[item['type']] :
                            lpath(KvalUiConfigManager.SETTINGS_SETTING_ICON)

            if(item['type'] === "bool") {
                itemIco = g_iconMaps[item['type']][value]
                //Stringify
                value = ""
            }


            settingsItemsModel.insert(i, {
            'uid': item['id'],
            'catid': categoryId,
            'cat': (!i) ? trid(group().label) : "",
            'name': item['label'],
            'type': item['type'],
            'opac' : itemOpacity,
            'value': value,
            'values': item['values'],
            'icon' : itemIco,
            'tip' : "",
            'option': ""});
        }
    }

    function getItemOpacity(uid, value)
    {
        var opacity = 1
        var opacitySettings ={
            "wifiScan": {"if":  "Ethernet"},
            "proxyaddr": {"proxy":  "Deactivated"},
            "proxyport": {"proxy":  "Deactivated"},
            "activateVodServer": {"vodSrvAddr":  ""},
            "activateLiveTvServer": {"liveTvSrvAddr":  ""},}
        if (uid in opacitySettings){
            //@TODO Fix me
            opacity= (value === opacitySettings[uid][1]) ? 0.3: 1
        }
        return opacity
    }

    //--------------------------------------------------------------------------
    // External application setting functions
    //--------------------------------------------------------------------------
    function apUpdateCurrent(apInfos)
    {
        var encryptionTypetext = ""
        var signalIcon = ""
        var lockSuffix = ".png"
        var signalStrength = ""
        if(apInfos[2] === "on")
        {
            encryptionTypetext = " • " + apInfos[3]
            lockSuffix = "_lock.png"
        }

        var quality = apInfos[4].substring(0, 2)

        if(quality < 20) signalStrength = "_4"
        else if(quality < 50) signalStrength = "_3"
        else if(quality < 75) signalStrength = "_2"
        else signalStrength = "_1"

        var iconPath =  lpath(KvalUiConfigManager.SETTINGS_WIFI_ICON_PREFIX) +
                        signalStrength +
                        lockSuffix

        itemChoiceModel.insert(apInfos[0],
                    {
                    'itcolor':        "white",
                    'itval':         apInfos[1],
                    'itcolor2' :     "white",
                    'isdefault':          apInfos[2],
                    'itval2' :      encryptionTypetext,
                    'iticon':         iconPath});
    }

    function updateAppSettingModel()
    {
        for (var i =0; i < g_settingMenuItems.length; i++)
        {
            if( g_settingMenuItems[i]['modul'].indexOf(
                group().uid) !== -1)
            {
                appsEngine.focusAppSettingWindow(g_mainSettingAppId,
                                                 g_settingMenuItems[i]['modul'],
                                                 g_settingMenuItems[i]['men'])
                break
            }
        }
    }

    function _getCurrentAppMenuEnum()
    {
        var menuEnum = -1;
        for (var i =0; i < g_settingMenuItems.length; i++)
        {
            if( g_settingMenuItems[i]['modul'].indexOf(
                group().uid) !== -1)
            {
                menuEnum = g_settingMenuItems[i]['men']
                break
            }
        }
        return menuEnum
    }

    function endOfSettingsEntries(succeeded)
    {
        logger.info("settingsMainView::endOfCategory succeeded: " + succeeded)
        if(!succeeded){
            settingsReadyNotify()
            return
        }

        if(g_waitForMenuItems && !g_settingMenuItems)
        {
            logger.info("settingsItemsListView.count: " + settingsItemsListView.count)
            appSettingsWindowTimer.stop()
            g_waitForMenuItems = false
            g_settingMenuItems = appsEngine.getSettingMenuItems()
            for (var i =0; i < g_settingMenuItems.length; i++)
            {
                logger.debug("men: " + g_settingMenuItems[i]['men'])
                logger.debug("modul: " + g_settingMenuItems[i]['name'])
                if( g_settingMenuItems[i]['men'] === '2' &&
                    g_settingMenuItems[i]['hidden'] === 'true' &&
                    !g_serviceMenuEntryActivated)
                    continue

                category.append(
                {
                'uid': g_settingMenuItems[i]['modul'],
                'label': g_settingMenuItems[i]['name'],
                'icon': ('icon' in g_settingMenuItems[i]) ? g_settingMenuItems[i]['icon'] : "",
                'source': 'external'
                });
            }
//            xmlItemsSettingsModel.reload()
        }
        else
        {
            var prevPos = settingsItemsListView.currentIndex
            var menuEnum  = _getCurrentAppMenuEnum()
            settingsItemsModel.clear()
            g_settingItems = appsEngine.getSettingItems(menuEnum)
            var indexIns = 0
            for (i =0; i < g_settingItems.length; i++)
            {
                var catSettings = g_settingItems[i]['settings']
                for(var j = 0; j < catSettings.length; j++)
                {
                    if('hidden' in catSettings[j] &&
                        catSettings[j]['hidden'] === 'true')
                    {
                        logger.info("hidden settings category: " + catSettings[j]['name'])
                        continue
                    }
                    var itemIco = (catSettings[j]['type'] in g_iconMaps) ?
                            g_iconMaps[model.get(i).itemType] :
                            lpath(KvalUiConfigManager.SETTINGS_SETTING_ICON)

                    if(catSettings[j]['type'] === "bool")
                    {
                        itemIco = (catSettings[j]['value'] in itemIco) ?
                                    itemIco[catSettings[j]['value']]:
                                    lpath(KvalUiConfigManager.SETTINGS_OFF_ICON)
                    }

                    settingsItemsModel.insert(indexIns,
                                {
                                'catid': g_settingItems[i]['ms'],
                                'cat': (!j) ? g_settingItems[i]['label'] : "",
                                'id': catSettings[j]['cat'],
                                'name': catSettings[j]['name'],
                                'type': catSettings[j]['type'],
                                'value': catSettings[j]['value'],
                                'values': catSettings[j]['values'],
                                'action': catSettings[j]['action'],
                                'option': "",
                                'tip': catSettings[j]['InfoText'],
                                'opac': 1,
                                'icon': itemIco
                                });
                    indexIns = indexIns+1
                }
            }
            settingsItemsListView.currentIndex = prevPos
        }

        settingsReadyNotify()
    }

    function appHasStopped()
    {
        logger.info("settingsMainView::appHasStopped")
    }

    //--------------------------------------------------------------------------
    // External reply functions
    //--------------------------------------------------------------------------

    function sViewValueUpdated(grp, key, val)
    {
        logger.info("Update: " + val)
        if(group().uid === grp && child().uid === key) {
            if(child().type === "bool") {
                child().icon = g_iconMaps[child().type][val]
            }
            else {
                child().value = val
            }
        }
    }

    function keyBoardCallBack(entry)
    {
        opacity = 1;
        enabled = true
        focus = true
        logger.info("keyBoardCallBack entry: " + entry)
        if(child().type === "text"){
            kvalUiSettingsManager.setValue(group().uid, child().uid, entry)
        }
        else {
            kvalUiSettingsManager.userReplyNotify(group().uid, child().uid, entry)
        }
    }

    function sViewDynaMenuReply(key, list)
    {
        if(!visible) {
            logger.info("settings menu not active !")
            return
        }

        enabled = true
        if(child().uid !== key){
            logger.info("dyn menu reply on a non focus child, thow it.")
            return
        }

        itemChoiceModel.clear()
        sViewPopulateChoiceMenu(list)
    }

    function sViewPopulateChoiceMenu(list)
    {
        if(!list.length)
            return

        itemChoiceModel.clear()
        for (var i = 0; i < list.length; i++ )
        {
            var entry = list[i]
            itemChoiceModel.insert(i,
                        {
                        'itcolor': "white",
                        'itval': ("val" in entry) ? entry["val"] : "",
                        'itcolor2': "white",
                        'isdefault': ("def" in entry) ? entry["def"] : false,
                        'itval2' : ("val2" in entry) ? " • " + entry["val2"] : "",
                        'iticon': ("icon" in entry) ? entry["icon"] : ""});
        }
        if(i < 9) backItemChoice.height = mainSettingsItemsZone.height * i/10
        else backItemChoice.height = mainSettingsItemsZone.height *0.9


        itemChoiceZone.visible = true
        itemChoiceListView.opacity = 1
        itemChoiceListView.enabled = true
        itemChoiceListView.focus = true
        settingsItemsListView.enabled = false
        settingsItemsListView.focus = false
        settingsItemsListView.opacity = 0.3
    }

    function diagWindowCallback(reply)
    {
        logger.info("diagWindowCallback notify kvalUiSettingsManager: " + reply)
        kvalUiSettingsManager.userReplyNotify(group().uid, child().uid, reply)
    }

    function diagWindowClosed()
    {
        logger.info("diagWindow closed")
        enabled = true
        focus = true
        opacity = 1
        itemChoiceZone.visible = false
        itemChoiceListView.opacity = 0
        itemChoiceListView.enabled = false
        itemChoiceListView.focus = false
        settingsItemsListView.enabled = true
        settingsItemsListView.focus = true
        settingsItemsListView.opacity = 1
    }

    //--------------------------------------------------------------------------
    // Item Actions request functions
    //--------------------------------------------------------------------------

    function virtualKeyboardReq(){
        userTextEntryReq("", false)
    }

    function userTextEntryReq(defval, ishidden)
    {
        opacity = 0.4
        enabled = false
        focus = false
        virtualKeyBoard.showVirtualKeyBoard(
                    settingsMainView,
                    defval,
                    child().value,
                    ishidden
                    ? IPTV_Constants.ENUM_TEXT_HIDDEN
                    : IPTV_Constants.ENUM_TEXT_CLEAR);
    }

    function actionReq() {
        // @TODO Block the settings view and wait for cpp reply
        // May be try to add some timer to avoid a forever blockout.
        kvalUiSettingsManager.reqaction(group().uid, child().uid)
    }

    function boolToggleReq() {
        // @TODO Block the settings view and wait for cpp reply
        // May be try to add some timer to avoid a forever blockout.
        child().icon=
            (child().icon === lpath(KvalUiConfigManager.SETTINGS_ON_ICON))?
                            lpath(KvalUiConfigManager.SETTINGS_OFF_ICON) :
                            lpath(KvalUiConfigManager.SETTINGS_ON_ICON)
        kvalUiSettingsManager.toggleChoice(group().uid, child().uid)
    }

    function staticMenuReq()
    {
        var values = trid(child().values)
        var reg = new RegExp("[|]+", "g");
        var arr = values.split(reg);
        var maparr = []
        for(var i in arr){
            maparr.push({"val": arr[i]})
        }

        sViewPopulateChoiceMenu(maparr)
    }

    function dynamicMenuReq()
    {
        // disable the view
        enabled = false
        kvalUiSettingsManager.dynvalues(group().uid, child().uid)
    }

    function settingsViewHandleSetById()
    {
        logger.info("settingsViewHandleSetById: "+choice().itval)
        kvalUiSettingsManager.reqMenuAction(group().uid, child().uid, choice().itval);
        endSetSetting()
    }

    function settingsViewHandleItems()
    {
        if(child().opac !== 1)
            return

        if(group().src === 'external')
        {
            appsEngine.actionAppSettingClick(
                    g_mainSettingAppId,
                    group().uid,
                    child().catid,
                    child().uid,
                    _getCurrentAppMenuEnum())
        }
        else {
            if (child().type in g_typeActionMap) {
                g_typeActionMap[child().type]()
            }
        }
    }

    //--------------------------------------------------------------------------
    // Key board navigation actions
    //--------------------------------------------------------------------------

    function onkeyup()
    {
        if(settingsMainListView.enabled)
        {
            settingsMainListView.decrementCurrentIndex()

            if(group().src === 'external')
            {
                updateAppSettingModel()
            }
            else
            {
                fillSettingItems(group().uid)
            }
        }
        else if(settingsItemsListView.enabled)
        {
            settingsItemsListView.decrementCurrentIndex()
            if(!settingsItemsListView.currentIndex) settingsItemsListView.positionViewAtBeginning()
        }
        else
        {
            itemChoiceListView.decrementCurrentIndex()
        }
    }

    function onkeydown()
    {
        if(settingsMainListView.enabled)
        {
            settingsMainListView.incrementCurrentIndex()
            if(group().src === 'external')
            {
                updateAppSettingModel()
            }
            else
            {
                fillSettingItems(group().uid)
            }
        }
        else if(settingsItemsListView.enabled)
        {
            settingsItemsListView.incrementCurrentIndex()
            if(!settingsItemsListView.currentIndex) settingsItemsListView.positionViewAtBeginning()
        }
        else
        {
            itemChoiceListView.incrementCurrentIndex()
        }
    }

    function onkeyok()
    {
        if(settingsMainListView.enabled)
        {
            settingsMainListView.enabled = false
            settingsMainListView.focus = false
            settingsItemsListView.enabled = true
            settingsItemsListView.focus = true
            settingsMainListView.highlight = dummyHighlight
            settingsItemsListView.highlight = highlightItemSettingsList
            g_itemsSettingsHighlighed = true
        }
        else if(settingsItemsListView.enabled)
        {
            settingsViewHandleItems()
        }
        else
        {
            settingsViewHandleSetById()
        }
    }

    function onkeyback()
    {
        if(settingsItemsListView.enabled)
        {
            settingsMainListView.enabled = true
            settingsMainListView.focus = true
            settingsItemsListView.enabled = false
            settingsItemsListView.focus = false
            g_itemsSettingsHighlighed = false
            settingsItemsListView.highlight = dummyHighlight
            settingsMainListView.highlight = highlightMainSettingsList
            settingsItemsListView.currentIndex = 0
            settingsItemsListView.positionViewAtIndex(0, ListView.Beginning)
        }
        else if(itemChoiceZone.enabled)
        {
            itemChoiceZone.visible = false
            itemChoiceListView.opacity = 0
            itemChoiceListView.enabled = false
            itemChoiceListView.focus = false
            settingsItemsListView.enabled = true
            settingsItemsListView.focus = true
            settingsItemsListView.opacity = 1
        }
    }

    function onkeynumeric(entry)
    {
        if(g_serviceMenuEntryActivated) return

        g_serviceMenuSecCode = g_serviceMenuSecCode + entry
        if(appsEngine.checkServiceMenuCode(g_serviceMenuSecCode))
        {
            g_serviceMenuEntryActivated = true
            g_settingMenuItems = null
            g_settingsReloaded = true
            xmlMainSettingsModel.reload()
        }
    }

    //--------------------------------------------------------------------------
    // Keys mapping
    //--------------------------------------------------------------------------

    Keys.onPressed: {
        if (event.key === Qt.Key_Up) //Up Button
        {onkeyup();
        }
        else if (event.key === Qt.Key_Down) //Down Button
        {onkeydown();
        }
        else if (event.key === Qt.Key_O) //Ok Button
        {onkeyok();
        }
        else if (event.key === Qt.Key_H) //Home Button
        {backToHomeView();
        }
        else if (event.key === Qt.Key_Backspace) //back Button
        {onkeyback()
        }
        else if (event.key === Qt.Key_0)
        {onkeynumeric('0');
        }
        else if (event.key === Qt.Key_1)
        {onkeynumeric('1');
        }
        else if (event.key === Qt.Key_2)
        {onkeynumeric('2');
        }
        else if (event.key === Qt.Key_3)
        {onkeynumeric('3');
        }
        else if (event.key === Qt.Key_4)
        {onkeynumeric('4');
        }
        else if (event.key === Qt.Key_5)
        {onkeynumeric('5');
        }
        else if (event.key === Qt.Key_6)
        {onkeynumeric('6');
        }
        else if (event.key === Qt.Key_7)
        {onkeynumeric('7');
        }
        else if (event.key === Qt.Key_8)
        {onkeynumeric('8');
        }
        else if (event.key === Qt.Key_9)
        {onkeynumeric('9');
        }
        else if (event.key === Qt.Key_Y) //Yellow Button
        {
        }
        else {}
        event.accepted = true;
    }

    //--------------------------------------------------------------------------
    // Timers
    //--------------------------------------------------------------------------

    Timer {
        id: appSettingsWindowTimer
        interval: 3000;
        running: false;
        repeat: false;
        onTriggered:
        {
            if(g_waitForMenuItems)
                settingsReadyNotify();
        }
    }

    //--------------------------------------------------------------------------
    // Display Items
    //--------------------------------------------------------------------------

    Item {
        id: settingsBackGound
        anchors.fill: parent
        Image {
            id: settingsBack
            anchors.fill: parent
        }
        Image {
            id: settingsBackOverlay2
            anchors.fill: parent
            opacity: 0.6
        }
    }
    Item {
        id: mainSettingsChoiceZone
        height: parent.height * 0.7
        width: parent.width * 0.2
        anchors.left: parent.left
        anchors.leftMargin: marginBlock * 5
        anchors.verticalCenter: parent.verticalCenter
    }
    Item {
        id: mainSettingsItemsZone
        height: parent.height * 0.7
        anchors.left: mainSettingsSeparatorLine.right
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
    }
    Item {
        id: mainSettingsSeparatorLine
        anchors.left: mainSettingsChoiceZone.right
        anchors.verticalCenter: parent.verticalCenter
    }

    Item {
        id: settingsHeaderZone
        anchors.top: parent.top
        anchors.topMargin: marginBlock
        anchors.left: parent.left
        anchors.leftMargin: marginBlock*2
        anchors.right: parent.right
        anchors.rightMargin: marginBlock*2
        height: parent.height * 0.1
        Item {
            id: spinner
            anchors.centerIn: parent
            visible: false
            Image {
                id: spinnerBack
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_SPINNER_BACK)
                scale: 0.5
                opacity: 0.6
            }
            Image {
                id: spinnerImg
                anchors.centerIn: parent
                source: kvalUiConfigManager.qmlGetPath(KvalUiConfigManager.APPS_VIEW_SPINNER)
                scale: 0.5
                RotationAnimation on rotation
                {
                    id: loadingSpinnerAnim
                    running: false
                    loops: Animation.Infinite
                    direction: RotationAnimation.Clockwise
                    from: 0
                    to: 360
                    duration: 600
                    onStarted: {
                        spinner.visible = true
                    }
                    onStopped: {
                        spinner.visible = false
                    }
                }
            }
        }
        Text {
            id: settingsTimeHeaderTxt
            anchors.right: parent.right
            anchors.top: parent.top
            color: "white"
            text: g_currentTimeStdFormat
            font {
                family: localFontCaps.name;
                pixelSize: parent.width * 0.015
            }
        }
        Text {
            anchors.right: settingsTimeHeaderTxt.right
            anchors.top: settingsTimeHeaderTxt.bottom
            anchors.topMargin: -marginBlock
            text: g_currentDateStdFormat
            color: "#6aa6cb"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.015
            }
        }
        Image{
            id: settingsIco
            anchors.left: parent.left
            anchors.top: parent.top
            source: lpath(KvalUiConfigManager.SETTINGS_ICON)
            scale: 0.7
        }
        Text {
            id: settingsTitle

            anchors.left: settingsIco.right
            anchors.leftMargin: marginBlock * 0.5
            anchors.verticalCenter: settingsIco.verticalCenter
            text: reTr + tr(KvalUiConfigManager.STR_KVAL_PARAM)
            color: "white"
            font {
                family: localFont.name
                pixelSize: parent.width * 0.03
            }
        }
    }

    Component {
            id : settingsMainDelegate
            Item {
                id: wrapper
                x: marginBlock
                width: mainSettingsChoiceZone.width
                height: mainSettingsChoiceZone.height/9

                Image {
                    id: settingIco
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: -marginBlock*0.5
                    horizontalAlignment: Text.AlignLeft
                    source: icon
                    scale: wrapper.ListView.isCurrentItem ? 1.05 : 1
                    opacity: wrapper.ListView.isCurrentItem ? 1 : 0.5
                }

                Text {
                    id: settingId
                    anchors.left: parent.left
                    anchors.leftMargin: settingIco.sourceSize.width + marginBlock
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    color: "white"
                    text: trid(label)
                    font {
                      family: localAdventFont.name;
                      pixelSize: mainSettingsChoiceZone.width * 0.1
                    }
                    opacity: wrapper.ListView.isCurrentItem ? 1 : 0.5
                }
                Glow {
                    anchors.fill: settingId
                    radius: 8
                    opacity: wrapper.ListView.isCurrentItem ? 1 : 0
                    samples: 17
                    spread: 0.1
                    color: "white"
                    transparentBorder: true
                    source: settingId
                }
            }
        }
    ListModel {
         id: catmodel
         ListElement {
             uid: ""
             label: ""
             icon: ""
             src: ""
             srcref: ""
         }
     }

    ListView {
        id: settingsMainListView
        anchors.top: mainSettingsChoiceZone.top
        anchors.topMargin: marginBlock * 4
        anchors.bottom: mainSettingsChoiceZone.bottom
        anchors.left: mainSettingsChoiceZone.left
        anchors.right: mainSettingsChoiceZone.right
        enabled: true
        opacity: 1
        focus: true

        model: catmodel
        highlight: highlightMainSettingsList
        highlightFollowsCurrentItem: false
        clip: false
        keyNavigationWraps: true
        interactive: false
        delegate: settingsMainDelegate
    }

    Component {
        id: highlightMainSettingsList

        Item {
            id: highlightCatRect
            anchors.horizontalCenter: settingsMainListView.currentItem.horizontalCenter
            anchors.verticalCenter: settingsMainListView.currentItem.verticalCenter
            width: settingsMainListView.currentItem.width * 1.05
            height: settingsMainListView.currentItem.height
            Rectangle {
                id: selectRect
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "transparent"
                border.color: "white"
                border.width: 2
                opacity: 0.5
            }
            Glow {
                anchors.fill: selectRect
                radius: 8
                opacity: 1
                samples: 25
                spread: 0.4
                color: "white"
                transparentBorder: true
                source: selectRect
            }
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation
                {
                    target: highlightCatRect;
                    property: "opacity";
                    from: 1.2; to: 0.2; duration: 800
                }
                NumberAnimation
                {
                    target: highlightCatRect;
                    property: "opacity";
                    from: 0.2; to: 1.2; duration: 800
                }
            }
        }
    }

    Component {
            id : settingsItemsDelegate
            Item {
                id: wrapper
                x: marginBlock*3
                width: mainSettingsItemsZone.width
                height: (cat === "") ?
                            mainSettingsItemsZone.height/10 :
                            (mainSettingsItemsZone.height/10) * 2

                Item {
                    id: catTicket
                    x: parent.x
                    anchors.top: parent.top
                    width: parent.width
                    height: (cat === "") ? 0 : parent.height/2
                    visible: (cat === "") ? false : true

                    Text {
                        id: catTxt
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        color: "#0489B1"
                        text: cat
                        font {
                          family: localAdventFont.name;
                          pixelSize: parent.height * 0.55
                        }
                    }
                    Glow {
                        anchors.fill: catTxt
                        radius: 8
                        opacity: 1
                        samples: 17
                        spread: 0.1
                        color: "#0489B1"
                        transparentBorder: true
                        source: catTxt
                    }
                    Image {
                        anchors.top: catTxt.bottom
                        anchors.topMargin: marginBlock
                        anchors.left: parent.left
                        anchors.leftMargin: -(parent.width * 0.1)
                        anchors.right: parent.right
                        anchors.rightMargin: parent.width * 0.1
                        source: lpath(KvalUiConfigManager.SETTINGS_SEPARATOR_FADED)
                    }
                }

                Item {
                    x: marginBlock*7
                    anchors.top: (cat === "") ?
                                     parent.top :
                                     catTicket.bottom
                    width: parent.width
                    height: (cat === "") ? parent.height : parent.height/2

                    Text {
                        id: settingItemTitle
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        color: "white"
                        text: trid(name)
                        opacity: (opac < 1) ? opac :
                                 ((wrapper.ListView.isCurrentItem && g_itemsSettingsHighlighed) ? 1 : 0.7)
                        font {
                          family: localAdventFont.name;
                          pixelSize: parent.height * 0.45
                        }
                    }
                    Glow {
                        anchors.fill: settingItemTitle
                        radius: 8
                        opacity: (wrapper.ListView.isCurrentItem && g_itemsSettingsHighlighed) ? 1 : 0
                        samples: 17
                        spread: 0.1
                        color: "white"
                        transparentBorder: true
                        source: settingItemTitle
                    }
                    Text {
                        id: settingItemValueTxt

                        anchors.right: parent.right
                        anchors.rightMargin: marginBlock*22
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: marginBlock * 0.3
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        color: "#6aa6cb"
                        text: reTr + qsTr(value)
                        visible: (type === 'bool' || type === 'button') ?
                                     false : true
                        font {
                          family: localAdventFont.name;
                          pixelSize: parent.height * 0.43
                        }
                    }

                    Image {
                        id: infoIcon
                        anchors.right: parent.right
                        anchors.rightMargin: marginBlock*14
                        anchors.verticalCenter: parent.verticalCenter
                        source: icon
                        scale: 0.8
                    }
                }
            }
        }
    ListModel {
         id: settingsItemsModel
         ListElement {
             uid: ""
             catid : ""
             cat: ""
             name: ""
             type: ""
             value: ""
             values: ""
             action: ""
             option: ""
             tip: ""
             opac: 1
             icon: ""
         }
     }
    
    Rectangle {
        id: listViewRect
        anchors.fill: mainSettingsItemsZone
        color: "transparent"
        opacity: settingsItemsListView.opacity

        ListView {
            id: settingsItemsListView
            anchors.top: parent.top
            anchors.topMargin: marginBlock
            anchors.bottom: parent.bottom
            anchors.bottomMargin: marginBlock
            anchors.left: parent.left
            anchors.right: parent.right
            enabled: false
            opacity: 1
            focus: false

            model: settingsItemsModel
            highlightFollowsCurrentItem: false

            Rectangle {
                color: 'white'
                anchors.top: settingsItemsListView.top
                anchors.horizontalCenter:scrollInternal.horizontalCenter
                opacity: 0.1
                width: 1
                height: (parent.contentHeight > parent.height) ? parent.height : 0
            }
            ScrollBar.vertical: ScrollBar {
                id: scrollInternal
                active: true
                anchors.top: settingsItemsListView.top
                anchors.right: settingsItemsListView.right
                anchors.rightMargin: marginBlock*2
                anchors.bottom: settingsItemsListView.bottom
            }

            clip: false
            keyNavigationWraps: true
            interactive: false
            delegate: settingsItemsDelegate
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
        source: lpath(KvalUiConfigManager.APPS_VIEW_MASKGRID)
        visible: false
    }

    Component {
        id: highlightItemSettingsList

        Item {
            id: highlightItemSetting
            anchors.bottom: settingsItemsListView.currentItem.bottom
            anchors.bottomMargin: marginBlock * 0.3
            width: mainSettingsItemsZone.width
            height: mainSettingsItemsZone.height * 0.1
            Rectangle {
                id: selectRect
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: marginBlock * 6
                anchors.right: parent.right
                anchors.rightMargin: marginBlock * 4
                color: "transparent"
                border.color: "white"
                border.width: 2
                opacity: 0.5
            }
            Glow {
                anchors.fill: selectRect
                radius: 8
                opacity: 1
                samples: 25
                spread: 0.4
                color: "white"
                transparentBorder: true
                source: selectRect
            }
            SequentialAnimation {
                running: true
                loops: Animation.Infinite
                NumberAnimation
                {
                    target: highlightItemSetting;
                    property: "opacity";
                    from: 1.2; to: 0.2; duration: 800
                }
                NumberAnimation
                {
                    target: highlightItemSetting;
                    property: "opacity";
                    from: 0.2; to: 1.2; duration: 800
                }
            }
        }
    }


    Rectangle {
        id: bottomZoneSeparator
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.07
        anchors.left: parent.left
        anchors.leftMargin: marginBlock*2
        anchors.right: parent.right
        anchors.rightMargin: marginBlock*2
        height: 1
        color: "white"
        opacity: 0.6
    }

    Item {
        id: mainSettingsItemTip
        anchors.top: mainSettingsItemsZone.bottom
        anchors.topMargin: marginBlock *0.5
        anchors.bottom: bottomZoneSeparator.top
        anchors.bottomMargin: marginBlock *0.5
        anchors.left: mainSettingsItemsZone.left
        anchors.leftMargin: marginBlock * 6
        anchors.right: mainSettingsItemsZone.right
        anchors.rightMargin: marginBlock*2

        Image {
            id: infoItemHintIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            scale: 0.8
            opacity: (child) ? ((child().tip === "") ? 0 : 1) : 0
            source: lpath(KvalUiConfigManager.APPS_VIEW_MISC_ICO )
        }

        Text {
            id: mainSettingItemTipTxt
            anchors.left: infoItemHintIcon.right
            anchors.leftMargin: marginBlock
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            elide: Text.ElideRight
            textFormat: Text.AutoText
            wrapMode: Text.WordWrap
            color: "white"
            opacity: 0.8
            text: (child && child().tip) ? trid(child().tip) : ""
            font {
              family: localFont.name;
              italic: true
              pixelSize: parent.height * 0.25
            }
        }
    }

    Item {
        id: itemChoiceZone

        width: mainSettingsItemsZone.width * 0.6
        height: mainSettingsItemsZone.height *0.9
        anchors.right: parent.right
        anchors.rightMargin: marginBlock
        anchors.top: mainSettingsItemsZone.top
        visible: false

        Rectangle{
            id: backItemChoice

            anchors.top: parent.top
            anchors.left: parent.left
            width: parent.width
            height: parent.height
            color: "black"
            opacity: 0.75
            radius: 3
            border.color: "grey"
            border.width: 1
            Image {
                width: parent.width * 1.13
                height: parent.height * 1.13
                anchors.centerIn: parent
                fillMode: Image.Stretch
                source: lpath(KvalUiConfigManager.FRAME_SQUARE_BIG)
            }
        }
    }


    Component {
            id : itemChoiceDelegate
            Row {
                Column {
                    Rectangle {
                        x: marginBlock
                        width: itemChoiceZone.width
                        height: itemChoiceZone.height/9
                        color: "transparent"

                        Text {
                            id: mainTitle

                            anchors.left: parent.left
                            anchors.leftMargin: marginBlock*2
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: itcolor
                            text: itval
                            font {
                              family: localAdventFont.name;
                              pixelSize: (1+(itemChoiceZone.width * 0.025)) +
                                         ((1 + (itemChoiceZone.width * 0.025)) * 0.333)
                            }
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: mainTitle.implicitWidth +
                                                marginBlock*2.5
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            color: itcolor2
                            text: itval2
                            opacity: 0.6
                            font {
                              family: localAdventFont.name;
                              pixelSize: (1+(itemChoiceZone.width * 0.02)) +
                                         ((1 + (itemChoiceZone.width * 0.02)) * 0.333)
                            }
                        }
                        Image {
                            anchors.right: parent.right
                            anchors.rightMargin: marginBlock*4
                            anchors.verticalCenter: parent.verticalCenter
                            source: iticon
                        }
                    }
                }
            }
        }
    ListModel {
         id: itemChoiceModel
         ListElement {
             itcolor: ""
             itval: ""
             itcolor2: ""
             isdefault: false
             itval2: ""
             iticon: ""
         }
     }
    ListView {
        id: itemChoiceListView
        anchors.top: itemChoiceZone.top
        anchors.bottom: itemChoiceZone.bottom
        anchors.left: itemChoiceZone.left
        anchors.right: itemChoiceZone.right
        enabled: false
        opacity: 0
        focus: false

        model: itemChoiceModel
        highlight: highlightItemChoiceList
        highlightFollowsCurrentItem: false
        clip: true
        keyNavigationWraps: true
        interactive: false
        delegate: itemChoiceDelegate
    }

    Component {
        id: highlightItemChoiceList

        Item {
            y: itemChoiceListView.currentItem.y
            width: itemChoiceZone.width + marginBlock*2
            height: itemChoiceZone.height/9

            Image {
                width: itemChoiceZone.width - (marginBlock*2)
                anchors.top: parent.top
                anchors.topMargin: -marginBlock - marginBlock/3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -marginBlock - marginBlock/3
                fillMode: Image.Stretch
                source: lpath(KvalUiConfigManager.OVERLAY_LIST_FOCUS)
                opacity: 0.8
                layer.enabled: true
                layer.effect: ColorOverlay
                {
                    color: highlightColor
                }
            }
        }
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
}

