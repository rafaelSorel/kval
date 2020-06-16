# kval_app.py
# -*- coding: utf-8 -*-
import logging
import os
import sys
import threading
import xml.etree.ElementTree as ET
from distutils.version import LooseVersion
from importlib import import_module

from PyQt5.QtCore import QObject, pyqtSlot, QCoreApplication
from PyQt5.QtDBus import QDBusConnection

from kval import DialogBox as Dialog
from kval_app_binder import createBinderMonitor, shareDBManager, createAppBinder, getBinder
from kval_app_binder import createLanguageHdlr, getBinderMonitor
from kval_app_binder import lang_hdlr as _tr
from kval_app_installer import ApplicationInstaller
from kval_exceptions import InstallerException, DatabaseException, CancelException, AppConfigException, LauncherException
import kval_utils

LOGGER = logging.getLogger('kvalapp')
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

from traceback import format_exc

# ------------------------------------
HOME = os.environ['HOME']
HOME_SCRIPTS = os.environ['HOME_SCRIPTS']
SCRIPT_ROOTDIR = os.path.join(HOME_SCRIPTS, 'kval_vod')

APPBINDER_PATH = os.path.join(HOME_SCRIPTS, 'apps')
APPS_PATH = os.path.join(HOME, 'apps')
APPS_SYSTEM_PATH = os.path.join(HOME_SCRIPTS, 'apps')
APPS_DB_PATH = os.path.join(APPS_PATH, 'apps.db')
APPS_DB_SYSTEM_PATH = os.path.join(APPS_SYSTEM_PATH, 'appsystem.db')

sys.path.insert(0, SCRIPT_ROOTDIR)

# ---------------------------------------------------------------------
SERVICE_SERVER_INVOKER_NAME = "org.QtDBus.serverPythonInvoker"
SERVICE_APPS_NAME = "org.QtDBus.kvalapp"
SERVICE_APPS_INVOKER = "org.QtDBus.kvalappsPyInvoker"
# ---------------------------------------------------------------------
USER_AGENT = "Mozilla/5.0 (Windows NT 5.1; rv:15.0) Gecko/20100101 Firefox/15.0.1"

KVAL_ENTRY_EXEC_POINT = "kval.python.appsource"


# ---------------------------------------------------------------------
# ----------------------------Classes ---------------------------------
# ---------------------------------------------------------------------
def launcher_cleaner(fn):
    def inner(obj, appid, args, isupdate=False):
        try:
            return fn(obj, appid, args, isupdate=isupdate)
        except LauncherException as ex:
            obj.kvalAppBinder.displayMessageUI('Error', ex.message)
            obj.kvalAppBinder.endOfCategory("", succeeded=False)
            obj.kvalAppBinder.endOfScript(handle=int(args.split(" ")[0]))
            obj.appProcsAbortStates[threading.current_thread()] = True
            return

    return inner


class AppDbManager:
    """
    Installed Application database management class
    """

    def __init__(self, db_path: str = "data base path"):
        self.sql_worker = kval_utils.Sqlite3Worker(db_path)

    def check_installed_dep(self, dep_id, dep_version):
        """
        Check if dependency has already been installed
        """
        installed_deps = \
            self.sql_worker.execute("select * from dependencies where depid like ?",
                                    (dep_id,))
        if not len(installed_deps) > 0:
            LOGGER.info("Dependency not insalled !")
            return False

        try:
            for installed_dep in installed_deps:
                if LooseVersion(installed_dep["depversion"]) >= LooseVersion(dep_version):
                    LOGGER.info("Already installed deps")
                    return True
        except:
            LOGGER.info("Dependency not insalled !")
            return False

        LOGGER.info("Dependency not insalled !")
        return False

    def check_installed_app(self,
                            appid: str = "application unique identifier",
                            appversion: str = "application version") -> bool:
        """
        Check if application has already been installed
        """
        installed_apps = self.sql_worker.execute("select * from apps where name like ?", (appid,))
        if not len(installed_apps) > 0:
            LOGGER.info("app not found !")
            raise DatabaseException("No application found")

        try:
            for installed_app in installed_apps:
                if LooseVersion(installed_app["version"]) >= LooseVersion(appversion):
                    LOGGER.info("Already installed apps")
                    return True
                else:
                    return False
        except:
            raise DatabaseException("No application found")
        else:
            raise DatabaseException("No application found")

    def add_app_to_favorite(self, appId):
        """
        Add application to favorite
        """
        self.sql_worker.execute( \
            "UPDATE apps SET favorite = 1 where name like ?", (appId,))

        return True

    def remove_app_from_favorite(self, appId):
        """
        Remove application from favorite
        """
        self.sql_worker.execute( \
            "UPDATE apps SET favorite = 0 where name like ?", (appId,))

        return True

    def add_installed_dep(self, appinfos):
        """
        Add dependencies has already been installed
        """
        results = self.sql_worker.execute("select * from dependencies where depid like ?",
                                          (appinfos["id"],))

        if results:
            for result in results:
                if LooseVersion(result["depversion"]) >= LooseVersion(appinfos["version"]):
                    LOGGER.info("Already installed apps")
                    return True

        # @todo: Check older version and may be remove them
        self.sql_worker.execute("insert into dependencies (depid, depversion) values(?, ?)",
                                (appinfos["id"], appinfos["version"]))
        return True

    def get_appname_from_id(self, appId):
        """
        Return Name from Id
        """
        installed_apps = self.sql_worker.execute( \
            "select * from apps where name like ?", (appId,))
        return installed_apps[0]["ui_name"]

    def remove_installed_app(self, appid):
        """
        Remove all application occurence from application database
        """
        self.sql_worker.execute("delete from apps where name like ?", (appid,))

    def add_installed_app(self, appinfos):
        """
        Add dependencies has already been installed
        """
        installed_apps = self.sql_worker.execute("select * from apps where name like ?",
                                                 (appinfos["id"],))
        if installed_apps:
            LOGGER.info('Check version of the installed app...')
            try:
                if LooseVersion(installed_apps[0]["version"]) >= \
                        LooseVersion(appinfos["version"]):
                    LOGGER.info("Already installed app: " + appinfos["id"])
                    return True
            except:
                pass

        self.sql_worker.execute( \
            "insert into apps \
        ('name', 'ui_name', 'version', 'resume', 'icon', 'backimage', 'fanart', 'category', \
        'provider', 'ui_type', 'update', 'favorite') \
        values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, 0)",
            (appinfos["id"],
             appinfos["name"],
             appinfos["version"],
             appinfos["resume"],
             appinfos["icon"],
             appinfos["back"],
             appinfos["fanart"],
             appinfos["category"],
             appinfos["provider-name"],
             appinfos["ui-type"]))

        return True

    def get_installed_apps(self):
        """
        Get installed apps
        """
        installed_apps = self.sql_worker.execute("select * from apps")
        if not installed_apps:
            LOGGER.info('No applications found in database')
            return

        apps_tab = []
        for app in installed_apps:
            LOGGER.info('Add application: ' + app['ui_name'])
            apps_dict = {}
            apps_dict["name"] = app['name']
            apps_dict["ui_name"] = app['ui_name']
            apps_dict["version"] = app['version']
            apps_dict["icon"] = app['icon']
            apps_dict["resume"] = app['resume']
            apps_dict["backimage"] = app['backimage']
            apps_dict["fanart"] = app['fanart']
            apps_dict["category"] = app['category']
            apps_dict["provider"] = app['provider']
            apps_dict["ui_type"] = app['ui_type']
            apps_dict["update"] = str(app['update'])
            apps_dict["favorite"] = str(app['favorite'])
            apps_dict["path"] = os.path.join(APPS_PATH, app['name'])
            apps_tab.append(apps_dict)

        return apps_tab


class InvokerAppBinder(QObject):
    """
    Interfacing with Cpp kval main applications
    """

    def __init__(self):
        super(InvokerAppBinder, self).__init__()
        self.kvalAppBinder = None
        self.store_srv_addr = None
        self.store_apps = None
        self.app_proc = None
        self.db_manager = AppDbManager(APPS_DB_PATH)
        self.system_db_manager = AppDbManager(APPS_DB_SYSTEM_PATH)
        self.appProcsAbortStates = {}
        self.appsDaemonsRunning = {}
        self._inserted_path = None
        shareDBManager(self.db_manager)

    @pyqtSlot(str)
    def set_appsstore_server_address(self, address):
        LOGGER.info('set_store_server_address: ' + address)
        dialog = Dialog()
        if not dialog.yesNoBox(_tr("Validation"),
                               _tr("Could you please validate:<br><b>{addr}</b>".format(addr=address)),
                               nolabel=_tr("No"),
                               yeslabel=_tr("Yes")):
            return

        dialog.okBox(_tr("Installation..."),
                     _tr("The installation will startup, don't <b>shutdown</b> or <b>disconnect</b> the kval box"))

        try:
            appid = address.split("/")[-1].split(".")[0]
            LOGGER.info("appid: " + appid)
        except:
            LOGGER.error("Could Not Extract appid from: " + address)
            self.kvalAppBinder.displayMessageUI('Error',
                                                _tr("Wrong store address."))
            return

        self.installApplication(appid, address, None)

    @pyqtSlot()
    def stopScript(self):
        LOGGER.info("Stop python app binder Daemon script...")
        kval_app.quit()
        sys.exit(0)

    @pyqtSlot(str, str, str, str)
    def installApplication(self, appId, appuri, appversion, apphash):
        """
        Install Application
        """
        # stop any daemon if exists for this application
        srvMainAddress = self.kvalAppBinder.getSrvAddr()
        appinstaller = ApplicationInstaller(self.db_manager)

        LOGGER.debug("Install: " + appId)
        LOGGER.debug("Version: " + appversion)

        apphash = None if not apphash or apphash == "" else apphash
        appversion = None if not appversion or appversion == "" else appversion

        try:
            appinstaller.preAppInstall(appId, appversion)
            self._stopDaemon(appId)
            appinstaller.installerStart(appId,
                                        appuri,
                                        appversion=appversion,
                                        apphash=apphash,
                                        srv=srvMainAddress)
        except InstallerException as ex:
            self.kvalAppBinder.displayMessageUI(_tr("Error"), ex.message)
            pass
        except CancelException as ex:
            self.kvalAppBinder.displayMessageUI(_tr("Error"), ex.message)
            pass
        except Exception:
            self.kvalAppBinder.displayMessageUI(_tr('Error'), _tr("Application install problem"))
            pass
        else:
            self.kvalAppBinder.refresh_installed_apps(self.db_manager.get_installed_apps())
            self.kvalAppBinder.displayMessageUI(_tr('Info'), _tr("Success install application."))
            self._reload(appId, os.path.join(APPS_PATH, appId))
            LOGGER.info("Launch daemon if needed for: " + appId)
            app_proc = threading.Thread(target=self._threaded_app_daemon, args=(appId,))
            app_proc.setDaemon(True)
            app_proc.start()

    @pyqtSlot(str)
    def uninstallApplication(self, appId):
        """
        Uninstall Application
        """
        app_name = self.db_manager.get_appname_from_id(appId)
        if not app_name:
            LOGGER.error("Could not extract appname from: " + appId)
            return

        dialog = Dialog()
        if not dialog.yesNoBox(_tr("Uninstall {appname} ?").format(appname=app_name),
                               _tr("Do you really want to uninstall {appname} ?").format(appname=app_name),
                               nolabel=_tr("No"),
                               yeslabel=_tr("Yes")):
            LOGGER.info("User Choose No to uninstall: " + appId)
            return

        LOGGER.info("uninstall: " + appId)

        try:
            appinstaller = ApplicationInstaller(self.db_manager)
            appinstaller.uninstallerStart(appId)
        except InstallerException as ex:
            self.kvalAppBinder.displayMessageUI(_tr("Error"), ex.message)
            pass
        except Exception:
            self.kvalAppBinder.displayMessageUI(_tr("Error"), _tr("Error occured !"))
            pass
        else:
            self.kvalAppBinder.refresh_installed_apps(self.db_manager.get_installed_apps())
            self.kvalAppBinder.displayMessageUI(_tr("Info"),
                                                _tr("Success uninstall {appname}").format(appname=app_name))

    @pyqtSlot(str, bool, int)
    def favoriteApp(self, appId, isAdd, handle):
        """
        Handle add/remove app from favorite
        """
        if isAdd:
            LOGGER.info("Add to favorite: " + appId)
            self.db_manager.add_app_to_favorite(appId)
        else:
            LOGGER.info("Remove From favorite: " + appId)
            self.db_manager.remove_app_from_favorite(appId)

        self.kvalAppBinder.refresh_installed_apps(
            self.db_manager.get_installed_apps())

    @pyqtSlot()
    def startAppsBinderDeamon(self):
        """
        Start Application Interface
        """
        createLanguageHdlr()
        self.kvalAppBinder = getBinder()
        try:
            self.kvalAppBinder.add_installed_apps(self.db_manager.get_installed_apps())
        except Exception as ex:
            LOGGER.error('Unable to load applications: ' + ex.message)
            pass
        try:
            self._run_apps_daemons(self.system_db_manager.get_installed_apps(), True)
        except Exception as ex:
            LOGGER.error('Unable to load daemon system db manager: ' + ex.message)
            pass
        try:
            self._run_apps_daemons(self.db_manager.get_installed_apps())
        except Exception as ex:
            LOGGER.error('Unable to load daemon db manager: ' + ex.message)
            pass

    def _run_apps_daemons(self, apps, system=False):
        """
        Check application daemon services
        """
        for app in apps:
            app_proc = threading.Thread(target=self._threaded_app_daemon,
                                        args=(app['name'], system))
            app_proc.setDaemon(True)
            app_proc.start()

    def _stopDaemon(self, appid):
        """
        Check and stop daemon from specific appid
        """
        if appid in self.appsDaemonsRunning:
            if self.appsDaemonsRunning[appid].is_alive():
                getBinderMonitor().onAbortRequest(appid=appid)
                LOGGER.info("Stop running damon for: " + appid)
                self.appsDaemonsRunning[appid].join()
                LOGGER.info("Stopped running daemon for: " + appid)

            del self.appsDaemonsRunning[appid]

    def _add_dep_to_syspath(self, appid):
        """
        Add dependencies to syspath
        """
        LOGGER.debug("_add_dep_to_syspath: " + appid)
        if APPBINDER_PATH not in sys.path:
            sys.path.insert(0, APPBINDER_PATH)

        app_folder_path = os.path.join(APPS_PATH, appid) or os.path.join(APPS_SYSTEM_PATH, appid)
        json_file_config = os.path.join(app_folder_path, "app.json")
        try:
            app_config = kval_utils.format_config(json_file_config)
        except FileNotFoundError as ex:
            LOGGER.error(f"FileNotFoundError for {json_file_config} !")
            raise
        except AppConfigException as ex:
            LOGGER.error(f"AppConfigException: {ex.message}")
            raise

        for dep in app_config.dependencies:
            LOGGER.debug(f"Handle {dep.id}")
            self._add_dep_to_syspath(dep.id)

        if app_config.launcher.package:
            lib_path = os.path.join(app_folder_path, app_config.launcher.package)
            if lib_path not in sys.path:
                sys.path.insert(0, lib_path)
                self._inserted_path.append(lib_path)

        LOGGER.debug("all deps were added to syspath")

    def _extract_daemonexec_infos(self, appid, system=False):
        """
        Extract daemon point exec
        """
        app_folder_path = os.path.join(APPS_PATH, appid) or os.path.join(APPS_SYSTEM_PATH, appid)

        appxmlfile = os.path.join(app_folder_path, "app.xml")
        source = open(appxmlfile)
        tree = ET.parse(source)
        root = tree.getroot()

        for ext in root.iter('extension'):
            if ext.get("point") == "kval.daemon":
                execpoint = ext.get("library")
                execpoint = execpoint.replace('.py', '')
                execpoint = execpoint.replace("/", ".")
                startupLevel = ext.get("start")
                source.close()
                if app_folder_path not in sys.path:
                    sys.path.insert(0, app_folder_path)
                return execpoint, startupLevel

        source.close()
        return None, None

    def _extract_exec_point(self, appid):
        """
        Extract execution script
        """
        app_folder_path = os.path.join(APPS_PATH, appid)
        appxmlfile = os.path.join(app_folder_path, "app.xml")
        with open(appxmlfile, "r") as source:
            tree = ET.parse(source)
            root = tree.getroot()
            if app_folder_path not in sys.path:
                sys.path.insert(0, app_folder_path)

            for ext in filter(lambda x: x.get("point") == KVAL_ENTRY_EXEC_POINT, root.iter('extension')):
                return ext.get("library").replace('.py', '').replace("/", ".")

    def _extract_update_point(self, appid):
        """
        Extract execution script
        """
        app_folder_path = os.path.join(APPS_PATH, appid)
        appxmlfile = os.path.join(app_folder_path, "app.xml")
        source = open(appxmlfile)
        tree = ET.parse(source)
        root = tree.getroot()

        if app_folder_path not in sys.path:
            sys.path.insert(0, app_folder_path)

        for ext in root.iter('extension'):
            if ext.get("point") == "kval.update":
                execpoint = ext.get("library")
                execpoint = execpoint.replace('.py', '')
                execpoint = execpoint.replace("/", ".")
                source.close()
                return execpoint

        source.close()
        return None

    def _load(self, module):
        """
        Reload the module and return it.
        The module must have been successfully imported before.
        """
        if module in sys.modules:
            LOGGER.info(f"Delete {module} from sys.modules")
            del sys.modules[module]

        LOGGER.debug('import module process: ' + module)
        return import_module(module)

    def _reload(self, module, path):
        """
        Reload the module and return it.
        The module must have been successfully imported before.
        """
        for k, v in sys.modules.items():
            if path in str(v):
                LOGGER.error("delete %s from sys.modules" % k)
                del sys.modules[k]

    def _threaded_app_daemon(self, appid, system=False):
        """
        Start Daemon application
        """
        apps_folder_path = os.path.join(APPS_PATH, appid) or os.path.join(APPS_SYSTEM_PATH, appid)
        if not os.path.exists(apps_folder_path):
            return

        LOGGER.info('Extract Daemon path and startup level for %s...' % appid)
        try:
            execpath, startLevel = self._extract_daemonexec_infos(appid, system)
        except Exception as ex:
            LOGGER.error("Error _extract_daemonexec_infos: " + ex.message)
            return

        if not execpath:
            LOGGER.error("No daemon point for: " + appid)
            return

        # Extract all dependencies and add them to syspath
        try:
            self._add_dep_to_syspath(appid, system)
        except (FileNotFoundError, AppConfigException) as ex:
            LOGGER.error(f"_add_dep_tosyspath({appid}) error: {str(ex)}")
            return

        LOGGER.info('Startup Daemon for app [%s]...' % appid)
        self.appsDaemonsRunning[appid] = threading.current_thread()

        try:
            app = self._load(execpath)
            app.startup()
        except:
            LOGGER.error("Problem exec " + appid)
            LOGGER.error(format_exc())
            return

        LOGGER.info(f"========= End Of {appid} Daemon ==========")

    @launcher_cleaner
    def _threaded_app_start(self, appid, args, isupdate=False):
        """
        """
        # Check appId
        LOGGER.debug('_threaded_app_start')
        apps_folder_path = os.path.join(APPS_PATH, appid)
        if not os.path.exists(apps_folder_path):
            raise LauncherException(_tr('Application Introuvable !!'))

        self.is_abort_requested = False
        # Extract all dependencies and add them to syspath
        LOGGER.debug('Extract all dependencies and add them to syspath')
        try:
            self._inserted_path = []
            self._add_dep_to_syspath(appid)
        except (FileNotFoundError, AppConfigException) as ex:
            LOGGER.error(f"Exception on add_dep_to_syspath: {str(ex)}")
            raise LauncherException(_tr('Application Erreur !!'))

        # clear argvs
        sys.argv = [sys.argv[0]]

        # set args
        if args:
            sys.argv.extend(arg for arg in args.split(" "))

        LOGGER.debug('exec path process')
        try:
            execpath = self._extract_exec_point(appid) if not isupdate else self._extract_update_point(appid)
            if not execpath:
                error_msg = _tr('Problème lancement application.') \
                    if not isupdate else _tr('Problème mise à jours de l\'application.')
                raise LauncherException(error_msg)
        except LauncherException as ex:
            raise
        except Exception as ex:
            raise LauncherException(_tr('Application Erreur !!'))

        try:
            app = self._load(execpath)
            app.startup()
        except:
            LOGGER.error("Problem exec app: ")
            LOGGER.error(format_exc())
            if self.appProcsAbortStates[threading.current_thread()]:
                LOGGER.warning("Already Aborted return")
                return
            raise LauncherException(_tr('Application Erreur !!'))

        if apps_folder_path in sys.path:
            sys.path.remove(apps_folder_path)

        for app_path in self._inserted_path:
            sys.path.remove(app_path)

        LOGGER.info('========= END OF SCRIPT ==========')
        if self.appProcsAbortStates[threading.current_thread()]:
            LOGGER.warning("Already Aborted return")
            return

        self.kvalAppBinder.endOfScript(handle=int(args.split(" ")[0]))
        self.appProcsAbortStates[threading.current_thread()] = True
        LOGGER.info("Thread finished ...")

    @pyqtSlot(str, str)
    def updateApplication(self, appid, args):
        """
        update Application
        """
        LOGGER.info("update: " + appid)
        self.app_proc = threading.Thread(target=self._threaded_app_start,
                                         args=(appid, args, True))
        self.app_proc.start()
        self.appProcsAbortStates[self.app_proc] = False

    @pyqtSlot(str, str)
    def launchApp(self, appid, args):
        """
        Application launcher
        """
        self.app_proc = threading.Thread(target=self._threaded_app_start,
                                         args=(appid, args))
        self.app_proc.start()
        self.appProcsAbortStates[self.app_proc] = False

    @pyqtSlot(str)
    def StopAppProc(self, appid):
        """
        Abort app proc
        """
        LOGGER.info("Abort App Proc !")
        if self.app_proc.is_alive():
            self.appProcsAbortStates[self.app_proc] = True
            LOGGER.info("Found alive application proc, terminate it !")
            # Terminate
            # self.raise_exception()
            self.app_proc.join(2)
            LOGGER.info(">>>> terminated")

    def raise_exception(self):
        import ctypes

        thread_id = self.app_proc.ident
        active_id = self.get_active_id()
        if active_id:
            thread_id = active_id

        res = ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, ctypes.py_object(SystemExit))
        if res > 1:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, 0)
            LOGGER.info('Exception raise failure')

    def get_active_id(self):
        # returns id of the respective thread
        if hasattr(self.app_proc, '_thread_id'):
            return self.app_proc._thread_id
        for id, thread in threading._active.items():
            if thread is self.app_proc:
                LOGGER.info('Got Thread id')
                return id
        return None


if __name__ == "__main__":

    global kval_app
    kval_app = QCoreApplication(sys.argv)
    bus = QDBusConnection.sessionBus()

    if not bus.isConnected():
        LOGGER.error("Cannot connect to the D-Bus session bus.\n"
                     "To start it, run:\n"
                     "\teval `dbus-launch --auto-syntax`\n")
        sys.exit(1)

    if not bus.registerService(SERVICE_APPS_INVOKER):
        LOGGER.error("%s\n" % bus.lastError().message())
        sys.exit(1)

    invokerApp = InvokerAppBinder()
    bus.registerObject('/', invokerApp, QDBusConnection.ExportAllSlots)
    createAppBinder()
    createBinderMonitor()
    sys.exit(kval_app.exec_())
