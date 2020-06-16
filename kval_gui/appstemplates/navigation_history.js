
var _navigation_history_apps_infos_tab = [];
var _navigation_history_nav_screens_tab = [];
var _navigation_history_label_tab = [];
var _navigation_history_uri_tab = [];
var _navigation_history_displaymode_tab = [];
var _navigation_history_displaysubmode_tab = [];

function clearHistory()
{
    _navigation_history_apps_infos_tab = []
    _navigation_history_nav_screens_tab = []
    _navigation_history_label_tab = []
    _navigation_history_uri_tab = []
    _navigation_history_displaymode_tab = []
    _navigation_history_displaysubmode_tab = []
}

function initHistoryTab(appName)
{
    _navigation_history_apps_infos_tab = []
    _navigation_history_nav_screens_tab = []
    _navigation_history_label_tab = []
    _navigation_history_uri_tab = []
    _navigation_history_displaymode_tab = []
    _navigation_history_displaysubmode_tab = []

    _navigation_history_label_tab.push(appName)
    _navigation_history_uri_tab.push("")
}

function storeDisplayModeHistory(mode, submode)
{
    _navigation_history_displaymode_tab.push(mode)
    _navigation_history_displaysubmode_tab.push(submode)
}

function storeNavElement(url, label)
{
    _navigation_history_label_tab.push(label)
    _navigation_history_uri_tab.push(url)
}

function store_iteminfos(appinfos)
{
    _navigation_history_apps_infos_tab.push(appinfos)
}

function save_items()
{
    _navigation_history_nav_screens_tab.push(_navigation_history_apps_infos_tab)
    _navigation_history_apps_infos_tab = []
}

function updateCurrentItem()
{
    _navigation_history_nav_screens_tab.pop()
    _navigation_history_nav_screens_tab.push(_navigation_history_apps_infos_tab)
    _navigation_history_apps_infos_tab = []
}

function checkRootItem()
{
    if((_navigation_history_uri_tab.length-1) <= 0)
        return true

    return false
}

function popNavItem()
{
    if(_navigation_history_uri_tab.length > 1)
        _navigation_history_uri_tab.pop()
    if(_navigation_history_label_tab.length > 1)
        _navigation_history_label_tab.pop()
    if(_navigation_history_nav_screens_tab.length > 1)
        _navigation_history_nav_screens_tab.pop()
    if(_navigation_history_displaymode_tab.length > 1)
        _navigation_history_displaymode_tab.pop()
    if(_navigation_history_displaysubmode_tab.length > 1)
        _navigation_history_displaysubmode_tab.pop()

    if(_navigation_history_nav_screens_tab.length > 1)
        return _navigation_history_nav_screens_tab[
                    _navigation_history_nav_screens_tab.length-1]

    return _navigation_history_nav_screens_tab[
                _navigation_history_nav_screens_tab.length-1]
}

function getCurrentNavItem()
{
    return _navigation_history_nav_screens_tab[
                _navigation_history_nav_screens_tab.length-1]
}

function getCurrentNavElement()
{
    return _navigation_history_uri_tab[_navigation_history_uri_tab.length-1]
}

function getLabelHistory()
{
    return _navigation_history_label_tab.join(" • ")
}

function getDisplayMode()
{
    return _navigation_history_displaymode_tab[
                        _navigation_history_displaymode_tab.length-1]

}

function getDisplaySubMode()
{
    return _navigation_history_displaysubmode_tab[
                        _navigation_history_displaysubmode_tab.length-1]
}

function getActionsDict(index)
{
    var items = _navigation_history_nav_screens_tab[
                        _navigation_history_nav_screens_tab.length-1][index]

    var constructed_actions = {}
    for (var key in items)
    {
        if(key.indexOf("CtxMenu_", 0) !== -1)
        {
            var new_key = key.substring(8, key.length)
            constructed_actions[new_key] = items[key]
        }
    }
    return constructed_actions;
}

function formatstring(str)
{
    if(!str) return ""

    str = str.split('[CR]').join('<br>')
    str = str.split('\n').join('<br>')
    str = str.split('[UPPERCASE]').join('')
    str = str.split('[/UPPERCASE]').join(' ')
    return str
}

function formatduration(duration)
{
    var totalSeconds = Number(duration)
    var hours = n(Math.floor(totalSeconds / 3600))
    totalSeconds %= 3600;
    var minutes = n(Math.floor(totalSeconds / 60))
    var seconds = n(totalSeconds % 60)

    var result = ''
    if(hours > 0)
        result = hours.toString() + ":"
    if(minutes>0)
        result = result + minutes.toString() + ":"
        result = result + seconds.toString()

    return result
}

function n(n){
    return n > 9 ? "" + n: "0" + n;
}

