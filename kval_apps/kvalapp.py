# kvalapp.py

# -*- coding: utf-8 -*-
import logging
import os
from distutils.version import LooseVersion

from kval_exceptions import InstallerException, DatabaseException, CancelException, AppConfigException, \
    LauncherException
import kval_utils
import intkval

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

APPS_PATH = os.path.join(HOME, 'kval_apps')
APPS_DB_PATH = os.path.join(APPS_PATH, 'apps.db')


# ---------------------------------------------------------------------
# ---------------------------------------------------------------------

# ---------------------------------------------------------------------
# ----------------------------Classes ---------------------------------
# ---------------------------------------------------------------------


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
        installed_deps = self.sql_worker.execute(f"select * from dependencies where depid like {dep_id}")
        if not installed_deps:
            LOGGER.info("Dependency not insalled !")
            return False

        try:
            for installed_dep in installed_deps:
                if LooseVersion(installed_dep["depversion"]) >= LooseVersion(dep_version):
                    LOGGER.info("Already installed deps")
                    return True
        except Exception as ex:
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
        installed_apps = self.sql_worker.execute(f"select * from apps where name like {appid}")
        if not len(installed_apps):
            LOGGER.info("app not found !")
            raise DatabaseException("No application found")

        try:
            for installed_app in installed_apps:
                if LooseVersion(installed_app["version"]) >= LooseVersion(appversion):
                    LOGGER.info("Already installed apps")
                    return True
                else:
                    return False
        except Exception:
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
            LOGGER.info('Add application: ' + app['appid'])
            apps_dict = {'appid': app['appid'], 'version': app['version']}
            apps_tab.append(apps_dict)

        LOGGER.info("Return applications...")
        return apps_tab

    def __del__(self):
        self.sql_worker.close()


class AppManager:
    """
    This class is a delegate of the main cpp Application Manager,
    It takes care of installing, uninstall,update ,extract and so on...
    """
    def __init__(self):
        print(f"database applications path: {APPS_DB_PATH}")
        self.db_manager = AppDbManager(APPS_DB_PATH)
        self.appProcsAbortStates = {}
        self.appsDaemonsRunning = {}
        self._inserted_path = None

    def get_installed_apps(self):
        return self.db_manager.get_installed_apps()

def translate_config(app_id):
    app_folder_path = os.path.join(APPS_PATH, app_id)
    _fconfig = os.path.join(app_folder_path, "appconfig.json")
    try:
        appcfg = kval_utils.format_config(_fconfig)
    except FileNotFoundError as ex:
        LOGGER.error(f"FileNotFoundError for {_fconfig} !")
        raise
    except AppConfigException as ex:
        LOGGER.error(f"AppConfigException: {ex.message}")
        raise

    LOGGER.info(f"set cpp app: {appcfg.id}, {appcfg.version}")
    app = intkval.Application(appcfg.id, appcfg.version)

    app.set_launcher(intkval.Launcher(appcfg.launcher.main,
                                      appcfg.launcher.daemon,
                                      appcfg.launcher.update,
                                      appcfg.launcher.package))

    for dep in appcfg.dependencies:
        app.add_dep(intkval.Dependency(dep.id, dep.version, dep.uri))

    return app


def extract_available_apps():
    """
    Extract Installed application from db,
    TODO: if db is not accessible or corrupted,
          try to recover it from application installation path
    Returns:
        Applications list
    """
    _app = AppManager()
    _apps = _app.get_installed_apps()

    avail_apps = []
    for app in _apps:
        try:
            _cpp_app_fmt = translate_config(app['appid'])
            avail_apps.append(_cpp_app_fmt)
        except (FileNotFoundError, AppConfigException, Exception):
            LOGGER.error(f"Something went wrong with {app.get('appid')}, try fetch next applications...")
            pass

    return avail_apps


# import threading
# class KvalStoreDaemon(threading.Thread):
#
#     def __init__(self):
#         threading.Thread.__init__(self, daemon=True)
#         self.stopped = False
#
#     def stop(self):
#         self.stopped = True
#
#     def run(self):
#         while not self.stopped:
#             print('_service_::run', 'WAITING For stop', 1)
#             time.sleep(1)
#
# daemon = KvalStoreDaemon()
# daemon.start()
#
# print("wait for thread stopped")
#
# daemon.join()
#
# print("Daemon thread stopped")
