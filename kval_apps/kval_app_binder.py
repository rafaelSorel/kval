# -*- coding: utf-8 -*-
import os
import weakref
from kval_utils.weakreflist import WeakList
from PyQt5.QtCore import QVariant, QObject, pyqtSlot, QMetaType
from PyQt5.QtDBus import QDBusInterface, QDBusConnection, QDBus, QDBusArgument, QDBusReply
from time import sleep
import logging
from traceback import format_exc

LOGGER = logging.getLogger('kvalappsbinder')
LOGGER.setLevel(logging.INFO)
# create console handler and set level to debug
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
# create formatter
formatter = logging.Formatter('%(asctime)s #%(threadName)s [%(levelname)s] %(name)s::%(message)s')
# add formatter to ch
ch.setFormatter(formatter)
# add ch to logger
LOGGER.addHandler(ch)
LOGGER.propagate = False

# ------------------------------------
HOME = os.environ['HOME']
HOME_SCRIPTS = os.environ['HOME_SCRIPTS']
APPS_PATH = os.path.join(HOME, 'apps')
APPS_PATH_EXEC = os.path.join(HOME_SCRIPTS, 'apps')

APP_LANGUAGES_PATH_RELEASE = os.path.join(APPS_PATH_EXEC, 'languages')
SERVICE_APPS_NAME = "org.QtDBus.kvalapp"

# ./tools/i18nup.sh apps_bind/languages/
import gettext

gLang = None
_ = None
gDbManager = None
appBinder = None
gBinderMonitor = None


# ---------------------------------------------------------------------
# ---------------------------- Private Classes -------------------------
# ---------------------------------------------------------------------

class GBinderMonitor(QObject):
    """
    Binder monitor class to receive notifications
    from kval program
    """

    def __init__(self):
        if gBinderMonitor:
            raise Exception('What the hell are you doing !')

        super(GBinderMonitor, self).__init__()
        self._refCache = WeakList()

        self._bus = QDBusConnection.sessionBus()
        if not self._bus.registerService("org.QtDBus.kvalappsBinderInvoker"):
            LOGGER.error("ERROR DBUS REGISTRATION %s\n" % self._bus.lastError().message())
        self._bus.registerObject('/binder', self, QDBusConnection.ExportAllSlots)

    def registerObj(self, obj):
        """
       Register Obj
       """
        if obj:
            LOGGER.debug("Register: " + str(obj))
            self._refCache.append(obj)

    def notificationsDispacher(self, methodName, args=None, appid=None):
        """
        Notify all registred obj of the new event
        """
        for regObj in self._refCache:
            try:
                if appid:
                    getappid = getattr(regObj, 'getAppId')
                    if appid != getappid():
                        LOGGER.info("getappid : " + getappid())
                        continue
            except Exception as ex:
                continue

            try:
                method_to_call = getattr(regObj, methodName)
                if args:
                    method_to_call(args)
                else:
                    method_to_call()
            except Exception as ex:
                LOGGER.debug("except: " + ex.message)
                continue

    @pyqtSlot(str, str, str, str, str)
    def onActionSettingWindow(self, appid, moduleId, catIdParent, catId, men):
        arg = {}
        arg['modul'] = moduleId
        arg['category'] = catIdParent
        arg['entry'] = catId
        arg['men'] = men
        self.notificationsDispacher("onClick", args=arg, appid=appid)

    @pyqtSlot(str, str, str)
    def onFocusSettingWindow(self, appid, moduleId, menuEnum):
        arg = {}
        arg['moduleId'] = moduleId
        arg['menuEnum'] = menuEnum
        self.notificationsDispacher("onFocus", args=arg, appid=appid)

    @pyqtSlot(str)
    def onInitSettingWindow(self, appid):
        self.notificationsDispacher("onInit", appid=appid)

    @pyqtSlot()
    def onAbortRequest(self, appid=None):
        self.notificationsDispacher("onAbortRequest", appid=appid)

    @pyqtSlot()
    def onPlayBackStarted(self):
        LOGGER.info("onPlayBackStarted")
        self.notificationsDispacher("onPlayBackStarted")

    @pyqtSlot()
    def onPlayBackStopped(self):
        LOGGER.info("onPlayBackStopped")
        self.notificationsDispacher("onPlayBackStopped")

    @pyqtSlot()
    def onPlayBackEnded(self):
        LOGGER.info("onPlayBackEnded")
        self.notificationsDispacher("onPlayBackEnded")

    @pyqtSlot()
    def onPlaybackFailed(self):
        LOGGER.info("onPlaybackFailed")
        self.notificationsDispacher("onPlaybackFailed")

    @pyqtSlot()
    def onLangChanged(self):
        LOGGER.info("onLangChanged")
        createLanguageHdlr()
        self.notificationsDispacher("onLangChanged")


class AppBinder:
    def __init__(self):
        if appBinder:
            raise Exception('What the hell are you doing !')

        self.appBinder = QDBusInterface(SERVICE_APPS_NAME,
                                        '/apps',
                                        '',
                                        QDBusConnection.sessionBus())
        if self.appBinder.isValid():
            LOGGER.info('appBinder valid dbus interface ')
        else:
            LOGGER.error('appBinder Not valid dbus interface ')

    def getComHdlr(self):
        if not self.appBinder.isValid():
            raise Exception('Not ready binder com hdlr !')

        return self.appBinder

    def customUiTemplateNotified(self, uiTemplatePath):
        """
        Send template path to main proc
        """
        self.appBinder.call(QDBus.NoBlock, 'notify_custom_ui_template', uiTemplatePath)
        while True:
            # check reply execution
            res = self.appBinder.call(QDBus.Block, 'get_ui_template_status', uiTemplatePath)
            reply = QDBusReply(res)
            if not reply.isValid():
                LOGGER.info("Reply Not Valid")
                break

            if reply.value() < 0:
                sleep(0.01)
                continue

            LOGGER.info("execute function reply: " + str(reply.value()))
            LOGGER.info("Reply: " + str(reply.value()))
            return True if reply.value() == 1 else False

    def getRemovableDevices(self):
        """
        Extract installed apps from kval
        """
        res = self.appBinder.call(QDBus.Block, 'get_removable_devices')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Reply not valid from kval")
            return None

        LOGGER.info("Reply: " + str(reply.value()))
        return reply.value()

    def get_installed_apps(self):
        """
        Extract installed apps from kval
        """
        return gDbManager.get_installed_apps() if gDbManager else None

    def getLanguage(self):
        """
        Extract language from kval application
        """
        res = self.appBinder.call(QDBus.Block, 'get_language')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Reply not valid from kval")
            return 'fr-FR'

        LOGGER.info("Reply: " + str(reply.value()))
        return reply.value()

    def getSrvAddr(self):
        """
        Extract Main server address
        """
        res = self.appBinder.call(QDBus.Block, 'get_srv_addr')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Reply not valid from kval")
            return None

        LOGGER.info("Reply: " + str(reply.value()))
        return reply.value()

    def displayMessageUI(self, header, message, timeout=0, icon=''):
        """
        Display message on UI
        header: header for message
        message: payload
        timeout: display duration/ 0 means default duration
        icon: display message icon / empty means default icon
        """
        self.appBinder.call(QDBus.NoBlock, 'displayMsgUi', header, message)

    def addItem(self, handle, argument, totalItems=0):
        """
        Add display Item
        """
        LOGGER.debug("addItem ....")
        self.appBinder.call(QDBus.NoBlock,
                            'add_item',
                            handle,
                            argument,
                            totalItems)

    def executebuiltin(self, function, wait=False):
        """
        Execute builtin functions
        """
        self.appBinder.call(QDBus.NoBlock, 'execute_func', function, wait)
        if not wait: return

        while True:
            # check reply execution
            res = self.appBinder.call(QDBus.Block, 'get_exec_func_status')
            reply = QDBusReply(res)
            if not reply.isValid():
                LOGGER.error("Reply not valid from kval")
                sleep(0.1)
                continue

            if reply.value() < 0:
                sleep(0.1)
                continue

            LOGGER.info("execute function reply: " + str(reply.value()))
            LOGGER.info("Reply: " + str(reply.value()))
            return True if reply.value() is 1 else False

    def setResolvedUrl(self, handle, argument):
        """
        Set playable resolved URL
        """
        self.appBinder.call(QDBus.NoBlock, 'set_res_url', handle, argument)

    def endOfCategory(self, handle, succeeded):
        """
        End of directory session transmission
        """
        LOGGER.info("endOfCategory ....")
        self.appBinder.call(QDBus.NoBlock,
                            'end_of_category',
                            handle,
                            succeeded)

    def endOfScript(self, handle=None):
        """
        Notify that app has stopped for any reason.
        """
        self.appBinder.call(QDBus.NoBlock, 'end_of_script', handle)

    def refresh_installed_apps(self, apps_tab):
        """
        Refresh installed applications
        """
        self.appBinder.call(QDBus.NoBlock, 'refresh_installed_apps')
        self.add_installed_apps(apps_tab)
        self.executebuiltin("Container.Refresh")

    def add_installed_apps(self, apps_tab):
        """
        Add installed applications
        """
        for apps_dict in apps_tab:
            argument = QDBusArgument()
            argument.beginMap(QVariant.String, QVariant.String);
            for key, value in apps_dict.iteritems():
                argument.beginMapEntry();
                argument.add(key, QVariant.String)
                argument.add(value, QVariant.String)
                argument.endMapEntry();
            argument.endMap();

            self.appBinder.call(QDBus.NoBlock, 'add_installed_apps', argument, len(apps_tab))


# ---------------------------------------------------------------------
# ----------------------------Private Helpers -------------------------
# ---------------------------------------------------------------------

def lang_hdlr(str):
    """
    Return language handler
    """
    global _
    return _(str).decode('utf-8')


def createLanguageHdlr():
    """
    Create language handler,
    this will be used by application manager on python side
    """
    global gLang
    lang = appBinder.getLanguage()
    appLangFormat = lang.split('-')[0]

    LOGGER.info("system language: " + str(lang))
    LOGGER.info("appLangFormat language: " + str(appLangFormat))

    lang_path_folder = APP_LANGUAGES_PATH_RELEASE
    try:
        gLang = gettext.translation('base',
                                    localedir=lang_path_folder,
                                    languages=[appLangFormat])
    except:
        LOGGER.info("format_exc: " + str(format_exc))
        gLang = gettext.translation('base',
                                    localedir=lang_path_folder,
                                    languages=['en'])

    gLang.install()
    global _
    _ = gLang.gettext


def createBinderMonitor():
    """
    Set the monitor binder for receving infos from kval
    """
    global gBinderMonitor
    gBinderMonitor = GBinderMonitor()


def createAppBinder():
    """
    Create app binder
    """
    global appBinder
    appBinder = AppBinder()


def shareDBManager(obj):
    """
    share db manager
    """
    global gDbManager
    gDbManager = weakref.proxy(obj)


def getBinderMonitor():
    """
    Get binder monitor
    """
    if not gBinderMonitor:
        createBinderMonitor()

    return gBinderMonitor


def getBinder():
    """
    Get application binder
    """
    if not appBinder:
        createAppBinder()

    return appBinder
