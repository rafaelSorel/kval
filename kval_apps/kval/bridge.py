# bridge.py


from kval_app_binder import getBinder
from PyQt5.QtCore import QVariant, QObject, pyqtSlot, QMetaType
from PyQt5.QtDBus import QDBus, QDBusArgument, QDBusReply
import xml.etree.ElementTree as ET

__binder__ = getBinder()

__all__ = ["Binder", "systemLanguage"]


def systemLanguage():
    """
    Extract language from kval application
    """
    return __binder__.getLanguage()


class Binder:
    """
    This is the bridge interface that binds with the main kval process,
    All the coms will go through this binder interface.
    """

    def __init__(self, appid=None):
        self.appBinder = __binder__
        self.appsettings = {}
        self.appinfos = {}
        self.appid = appid
        self.appsettingsfile = None
        self.extract_app_infos()
        self.extract_app_settings()

    def notifyCustomUiTemplate(self, uiTemplatePath):
        """
        Send template path to the application
        """
        return self.appBinder.customUiTemplateNotified(uiTemplatePath)

    def refresh(self):
        """
        Refresh application infos
        """
        self.extract_app_infos()
        self.extract_app_settings()

    def getRemovableDevices(self):
        """
        Extract installed apps from kval
        """
        return self.appBinder.getRemovableDevices()

    def get_installed_apps(self):
        """
        Extract installed apps from kval
        """
        return self.appBinder.get_installed_apps()

    def get_appid(self):
        """
        return the appid
        """
        return self.appid

    def getAppInfo(self, infoKey):
        """
        """
        return self.appinfos.get(infoKey, None)

    def extract_app_infos(self):
        """
        Extract application infos
        """
        if not self.appid:
            return

        appInfoFile = os.path.join(self.getNativePath(), "app.xml")
        if not os.path.exists(appInfoFile):
            LOGGER.error("Could not find " + appInfoFile)
            return

        tree = ET.parse(appInfoFile)
        root = tree.getroot()

        for appInfo in root.iter('app'):
            self.appinfos.update(appInfo.attrib)

    def extract_app_settings(self):
        """
        Extract application settings
        """
        if not self.appid:
            return

        if not os.path.exists(os.path.join(self.getNativePath(), "kval_res/settings.xml")):
            LOGGER.error("No settings found for: " + self.appid)
            return

        self.appsettingsfile = os.path.join(self.getNativePath(),
                                            "kval_res/settings.xml")
        tree = ET.parse(self.appsettingsfile)
        root = tree.getroot()

        for ext in root.iter('setting'):
            self.appsettings[ext.get("id")] = ext.get("default")

    def getSetting(self, setting_id):
        """
        Get settings from file
        """
        if not self.appid:
            return

        if setting_id in self.appsettings:
            LOGGER.debug("setting id [%s], value [%s]" % \
                         (setting_id, self.appsettings[setting_id]))
            return self.appsettings[setting_id]
        else:
            LOGGER.error("Could not find setting id [%s]" % setting_id)
            return None

    def setSetting(self, setting_id, value):
        """
        Get settings from file
        """
        if not self.appid: return
        LOGGER.debug("Set settings")
        self.appsettingsfile = os.path.join(self.getNativePath(),
                                            "kval_res/settings.xml")
        tree = ET.parse(self.appsettingsfile)
        root = tree.getroot()

        for ext in root.iter('setting'):
            if ext.get("id") == setting_id:
                LOGGER.debug("setting id [%s], value [%s]" % (setting_id, value))
                ext.set("default", value)

                # Update local settings dict
                self.appsettings[setting_id] = value
                break

        tree.write(self.appsettingsfile)

    def getLanguage(self):
        """
        Extract language from kval application
        """
        return self.appBinder.getLanguage()

    def getSrvMainAddr(self):
        """
        Extract language from kval application
        """
        return self.appBinder.getSrvAddr()

    def getNativePath(self):
        """
        Get application native path
        """
        if not self.appid: return

        appPath = os.path.join(APPS_PATH, self.appid)
        if not os.path.exists(appPath):
            appPath = os.path.join(APPS_SYSTEM_PATH, self.appid)

        if not os.path.exists(appPath):
            LOGGER.error("application path does not exists: " + appPath)
            return None

        return appPath

    def is_valid(self):
        if self.appBinder.isValid():
            return True
        else:
            return False

    def displayMessageUI(self, header, message, timeout=0, icon=''):
        """
        Display message on UI
        header: header for message
        message: payload
        timeout: display duration/ 0 means default duration
        icon: display message icon / empty means default icon
        """
        self.appBinder.displayMessageUI(header, message, timeout=timeout, icon=icon)

    def addItem(self,
                handle=None,
                item=None,
                url=None,
                isFolder=False,
                totalItems=0):
        """
        Add display Item
        """
        LOGGER.debug("addItem ....")
        if not item or not url:
            LOGGER.error("Need to add item and url")
            return

        item_dict = {}
        item_dict['label'] = item.label
        item_dict['subs'] = ','.join(item.subtitles)
        item_dict['IsPlayable'] = 'true' if item.IsPlayable else 'false'
        item_dict['isFolder'] = 'true' if isFolder else 'false'
        item_dict['url'] = url
        item_dict['path'] = item.path

        item_dict.update(item.artdict)
        item_dict.update(item.properties)
        item_dict.update(item.streamInfo)
        item_dict.update(item.metaInfo)
        item_dict.update(item.contextMenuItems)

        argument = QDBusArgument()
        argument.beginMap(QVariant.String, QVariant.String);
        for key, value in item_dict.iteritems():
            argument.beginMapEntry();
            argument.add(key, QVariant.String)
            strval = value if type(value) is str or type(value) is unicode else str(value)
            argument.add(strval, QVariant.String)
            argument.endMapEntry();
        argument.endMap();

        self.appBinder.addItem(handle, argument, totalItems)

    def executebuiltin(self, function, wait=False):
        """
        Execute builtin functions
        """
        self.appBinder.executebuiltin(function, wait)

    def setResolvedUrl(self, handle, succeeded=False, item=None):
        """
        Set playable resolved URL
        """
        if not succeeded or not item:
            endOfCategory(handle, succeeded=succeeded)
            return

        item_dict = {}
        item_dict['label'] = item.label
        item_dict['subs'] = ','.join(item.subtitles)
        item_dict['IsPlayable'] = 'true' if item.IsPlayable else 'false'
        item_dict['isFolder'] = 'false'
        item_dict['path'] = item.path
        item_dict['audioUri'] = ''
        item_dict['videoUri'] = ''
        if 'audioUri' in item.dualPath and 'videoUri' in item.dualPath:
            item_dict['audioUri'] = item.dualPath['audioUri']
            item_dict['videoUri'] = item.dualPath['videoUri']

        item_dict.update(item.artdict)
        item_dict.update(item.properties)
        item_dict.update(item.streamInfo)
        item_dict.update(item.metaInfo)
        item_dict.update(item.contextMenuItems)

        argument = QDBusArgument()
        argument.beginMap(QVariant.String, QVariant.String);
        for key, value in item_dict.iteritems():
            argument.beginMapEntry();
            argument.add(key, QVariant.String)
            strval = value if type(value) is str or type(value) is unicode else str(value)
            argument.add(strval, QVariant.String)
            argument.endMapEntry();
        argument.endMap();

        self.appBinder.setResolvedUrl(handle, argument)

    def endOfCategory(self, handle, succeeded=True, cacheToDisc=False):
        """
        End of directory session transmission
        """
        LOGGER.info("endOfCategory ....")

        self.appBinder.endOfCategory(handle, succeeded)

