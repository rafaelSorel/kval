# kval_app_installer.py
#-*- coding: utf-8 -*-
import urllib, re, os, sys, shutil, string
import urllib2
import logging
import subprocess
from hashlib import md5, sha1
from time import gmtime, strftime, sleep
import time
from zipfile import ZipFile
from traceback import print_exc

from PyQt5.QtCore import QMutex, QWaitCondition
from PyQt5.QtDBus import QDBusInterface, QDBusConnection, QDBus, QDBusReply

from kval_utils.sqlite3worker import Sqlite3Worker
import xml.etree.ElementTree as ET
from kval import DialogProgress, Crypt
from kval_exceptions import InstallerException, DatabaseException, KvalException, CancelException
from kval_app_binder import lang_hdlr as _

LOGGER = logging.getLogger('kvalinstaller')
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

#------------------------------------
HOME = os.environ['HOME']
HOME_SCRIPTS = os.environ['HOME_SCRIPTS']
SCRIPT_ROOTDIR = os.path.join(HOME_SCRIPTS, 'kval_vod')

APPS_PATH = os.path.join(HOME, 'apps')

sys.path.insert(0, SCRIPT_ROOTDIR)
import StreamLauncherDownloader as sldownloader

#---------------------------------------------------------------------
USER_AGENT = "Mozilla/5.0 (Windows NT 5.1; rv:15.0) Gecko/20100101 Firefox/15.0.1"
MIRRORS = [""]
#----------------------------Classes ---------------------------------
#---------------------------------------------------------------------

class ApplicationInstaller():
    """
    Install Application internally into the fs
        1- Check URL size
        2- Check Local storage space
        3- Download application
        4- Extract applicaion
        5- check dependencies
        6- download check and install deps
        7- Install application
    """
    UPDATE_KEY_URL = 'https://%s/d/updates/sk/%s_k.enc'
    UPDATE_SIG_URL = 'https://%s/d/updates/sk/%s_s.enc'
    TEMP = '%s/temp/' % APPS_PATH
    def __init__(self, dbmanager, isApp=True):
        self.dbmanager = dbmanager
        self.useragent = USER_AGENT
        self.downloadWaitMutex = QMutex()
        self.downloadWaitCondition = QWaitCondition()
        self.lastDirectDownloadedFile = None
        self.app_info = {}
        self.dep_info = {}

    def preAppInstall(self, appid, appversion):
        """
        Check if we procced
        """
        if not os.path.exists(APPS_PATH):
            LOGGER.error("Could not find: " + APPS_PATH)
            raise InstallerException(_("Application install problem"))

        #Check if app is installed
        LOGGER.info("Check if app is installed...")
        try:
            if self.dbmanager.check_installed_app(appid, appversion):
                ui_name = self.dbmanager.get_appname_from_id(appid)
                LOGGER.info("Already installed app :" + ui_name)
                raise InstallerException( \
                        _("<b>{appname}</b> already installed with the last version: <b>{appversion}</b>").\
                                                        format( appname=ui_name,
                                                                appversion=appversion))
            else:
                LOGGER.info("Remove installed app :" + appid)
                self.uninstallerStart(appid)

        except InstallerException as ex:
            raise InstallerException(ex.message)
        except DatabaseException as ex:
            LOGGER.error("DatabaseException: " + ex.message)
        except Exception as ex:
            LOGGER.error("Exception: " + ex.message)
            raise InstallerException(_("Application install problem"))

    def uninstallerStart(self, appname):
        """
        Uninstall application
        """
        #check application physical location
        if not os.path.exists(os.path.join(APPS_PATH, appname)):
            LOGGER.error("Could not find: " + os.path.join(APPS_PATH, appname))
            raise InstallerException(_("Could not find application !"))

        #remove folder
        shutil.rmtree(os.path.join(APPS_PATH, appname))

        #remove application from database
        try:
            self.dbmanager.remove_installed_app(appname)
        except:
            LOGGER.error("Could not find: " + os.path.join(APPS_PATH, appname))
            raise InstallerException(_("Could not remove application from database !"))

    def installerDep(self, appname, appuri, appversion=None, apphash=None):
        """
        Install dependencies
        """
        if isinstance(appname, unicode):
            LOGGER.info("appname unicode")
            appname = appname.encode('utf-8')

        if not os.path.exists(self.TEMP):
            os.makedirs(self.TEMP)

        try:
            dst_file = self.download_file(appuri,
                                          os.path.join(self.TEMP, appuri.split('/')[-1]),
                                          None, apphash=apphash, silent=True)

            if dst_file is None:
                raise InstallerException(_("Application install problem"))
        except Exception as ex:
            if os.path.exists(self.TEMP):
                shutil.rmtree(self.TEMP)
            raise ex

        #Extraction step
        try:
            self._extract_application(dst_file, APPS_PATH)
            dst_file = dst_file.split('/')[-1] if '/' in dst_file else dst_file
            apps_folder_path = os.path.join(APPS_PATH,
            re.compile("(.+?)(?:-[0-9.]+)*\.zip").findall(dst_file)[0])
        except:
            LOGGER.error("Could not extract base path:" + dst_file)
            raise InstallerException(_("Could not extract application"))

        LOGGER.info("apps_folder_path: " + apps_folder_path)

        #Check extracted data step
        try:
            infos = self._check_application_struct(apps_folder_path)
            self._check_application_dependencies(apps_folder_path)
        except Exception as ex:
            raise ex

        #Add to data base step
        self.dbmanager.add_installed_dep(infos)

    def installerStart(self, appname, appuri, appversion=None, apphash=None, srv=None):
        """
        Download and install the application
        """
        LOGGER.info("install: " + appname)
        if isinstance(appname, unicode):
            LOGGER.info("appname unicode")
            appname = appname.encode('utf-8')

        if not os.path.exists(self.TEMP):
            os.makedirs(self.TEMP)

        dialog = DialogProgress()
        dialog.create(_("Installation..."),
                      _("Download {appname} in progress ...").format(appname=appname))
        dialog.update(10)

        if not appuri:
            LOGGER.info("No location has been specified, used default mirrors")
            for mirror in MIRRORS:
                if appversion:
                    appuri = mirror + "/" + appname + "-" + appversion + ".zip"
                else:
                    appuri = mirror + "/" + appname + ".zip"

        #Download step
        key_url = self.UPDATE_KEY_URL % (srv , self._get_host_hash())
        sig_url = self.UPDATE_SIG_URL % (srv , self._get_host_hash())
        try:
            key_path = self.download_file(key_url,
                                          os.path.join(self.TEMP, key_url.split('/')[-1]),
                                          None, apphash=None, silent=True)
            sig_path = self.download_file(sig_url,
                                          os.path.join(self.TEMP, sig_url.split('/')[-1]),
                                          None, apphash=None, silent=True)
            app_path = self.download_file(appuri,
                                          os.path.join(self.TEMP, appuri.split('/')[-1]),
                                          dialog, apphash=apphash)
        except CancelException as ex:
            raise CancelException(ex.message)
        except Exception as ex:
            if os.path.exists(self.TEMP): shutil.rmtree(self.TEMP)
            dialog.close()
            raise ex

        if app_path is None or key_path is None or sig_path is None:
            if os.path.exists(self.TEMP): shutil.rmtree(self.TEMP)
            dialog.close()
            raise InstallerException(_("Application install problem"))

        if dialog.iscanceled():
            if os.path.exists(self.TEMP): shutil.rmtree(self.TEMP)
            dialog.close()
            raise CancelException(_("Installation cancelled by user"))

        dialog.update(20, _("Extract {appname} in progress.").format(appname=appname))

        #Extraction step
        try:
            kvalcrypt = Crypt()
            dst_file = os.path.join(APPS_PATH, appname+'.zip')
            if not kvalcrypt.decrypt(sig_path, key_path, app_path, dst_file):
                raise InstallerException(_("Application install problem"))

            self._extract_application(dst_file, APPS_PATH)
            dst_file = dst_file.split('/')[-1] if '/' in dst_file else dst_file
            apps_folder_path = os.path.join(APPS_PATH,
            re.compile("(.+?)(?:-[0-9.]+)*\.zip").findall(dst_file)[0])
        except:
            LOGGER.error("Could not extract base path:" + dst_file)
            if os.path.exists(self.TEMP): shutil.rmtree(self.TEMP)
            dialog.close()
            raise InstallerException(_("Could not extract application"))

        LOGGER.info("apps_folder_path: " + apps_folder_path)
        if dialog.iscanceled():
            if os.path.exists(self.TEMP): shutil.rmtree(self.TEMP)
            dialog.close()
            raise CancelException(_("Installation cancelled by user"))


        dialog.update(50, _("Check {appname} in progress.").format(appname=appname))

        #Check extracted data step
        try:
            infos = self._check_application_struct(apps_folder_path)
            self._check_application_dependencies(apps_folder_path)
        except Exception as ex:
            if os.path.exists(self.TEMP): shutil.rmtree(self.TEMP)
            dialog.close()
            raise ex


        self.lastDirectDownloadedFile = None
        dialog.update(80, _("Install {appname} in progress.").format(appname=appname))

        #Add to data base step
        self.dbmanager.add_installed_app(infos)

        dialog.close()
        if os.path.exists(self.TEMP): shutil.rmtree(self.TEMP)

    def _checkFreeSpaceForApp(self, appsize):
        """
        Check free space where to install the application
        """
        statvfs = os.statvfs(APPS_PATH)

        #statvfs.f_frsize * statvfs.f_blocks     # Size of filesystem in bytes
        #statvfs.f_frsize * statvfs.f_bfree      # Actual number of free bytes
        #statvfs.f_frsize * statvfs.f_bavail     # Number of free bytes that ordinary users
        free_space_size = int(statvfs.f_frsize) * int(statvfs.f_bfree)
        final_appsize = int(appsize)*3
        if final_appsize > free_space_size:
            LOGGER.error('Not enough space to install app %d vs %d: ' \
                            % (appsize, free_space_size))
            return False

        return True

    def _get_host_hash(self):
        '''
        Hash confidential data
        '''
        hostname = os.uname()[1]
        hashed = sha1(md5(hostname).hexdigest()).hexdigest()
        return hashed

    def download_file(self, source, destination, download_dlg, apphash=None, silent=False):
        try:
            local_file = open(destination, 'wb')
            response = urllib2.urlopen(urllib2.quote(source, safe=':/'))
            total_size = int(response.info().getheader('Content-Length').strip())
            minutes = 0
            seconds = 0
            rest = 0
            speed = 1
            start = time.time()
            size = 0
            part_size = 0
            last_percent = 0
            while 1:
                part = response.read(32768)
                part_size += len(part)
                if time.time() > start + 2:
                    speed = int((part_size - size) / (time.time() - start) / 1024)
                    start = time.time()
                    size = part_size
                    rest = total_size - part_size
                    minutes = rest / 1024 / speed / 60
                    seconds = rest / 1024 / speed - minutes * 60
                percent = int(part_size * 100.0 / total_size)
                if silent == False:
                    download_dlg.update(percent,
                                        _("Filename") + \
                                        ':  %s' % source.rsplit('/', 1)[1] + '<br>' + \
                                        _("Download speed") + ':  %d KB/s' % speed + '<br>' + \
                                        _("Time remaining") + ':  %d m %d s' % (minutes, seconds))
                    if download_dlg.iscanceled():
                        os.remove(destination)
                        local_file.close()
                        response.close()
                        raise CancelException(_("Installation cancelled by user"))
                else:
                    if percent > last_percent + 5:
                        LOGGER.info('download_file(' + destination + ') %d percent with %d KB/s' % (percent, speed))
                        last_percent = percent
                if not part:
                    break
                local_file.write(part)
            local_file.close()
            response.close()
            subprocess.call('sync', shell=True, stdin=None, stdout=None, stderr=None)
        except Exception, e:
            LOGGER.error('download_file(' + source + ', ' + destination + ') ERROR: (' + repr(e) + ')')
            raise InstallerException(_("Application download problem !"))

        if not self._check_downloaded_file(destination, apphash):
            LOGGER.error('Hash check failed')
            self.lastDirectDownloadedFile = None
            raise InstallerException(_("Application download problem !"))

        return destination

    def _check_application_struct(self, apps_folder_path):
        """
        Check application folder structure and contents
        """
        #check apps.xml file
        appxmlfile = os.path.join(apps_folder_path, "app.xml")
        if not os.path.exists(appxmlfile):
            LOGGER.error("Could not find " + appxmlfile)
            raise InstallerException(_("Application format missmatch !"))

        tree = ET.parse(appxmlfile)
        root = tree.getroot()
        self.app_info = root.attrib
        LOGGER.info("name " + self.app_info["name"])
        LOGGER.info("id " + self.app_info["id"])
        LOGGER.info("version " + self.app_info["version"])
        LOGGER.info("provider-name " + self.app_info["provider-name"])
        LOGGER.info("ui-type " + self.app_info["ui-type"] if "ui-type" in self.app_info else "")

        #looks for assets
        self.app_info["icon"] = ''
        self.app_info["back"] = ''
        self.app_info["fanart"] = ''

        try:
            self.app_info["icon"] = "file://"+os.path.join(apps_folder_path,
                                              root.iter('icon').next().text)
            self.app_info["back"] = "file://"+os.path.join(apps_folder_path,
                                              root.iter('back').next().text)
            self.app_info["fanart"] = "file://"+os.path.join(apps_folder_path,
                                              root.iter('fanart').next().text)
        except:
            LOGGER.info("default assets")
            pass

        #looks for resume fr_FR for now,
        #@todo: Change language upon main application current language
        self.app_info["resume"] = ''
        try:
            for desc in root.iter('description'):
                if desc.get("lang") == "fr_fr":
                    self.app_info["resume"] = desc.text
                    break
        except:
            pass

        return self.app_info

    def _check_application_dependencies(self, apps_folder_path):

        appxmlfile = os.path.join(apps_folder_path, "app.xml")
        tree = ET.parse(appxmlfile)
        root = tree.getroot()

        #looks for dependencies
        try:
            for dep in root.iter('import'):
                LOGGER.info("dep " + str(dep.get("addon")))
                LOGGER.info("version " + str(dep.get("version")))
                LOGGER.info("baselink " + str(dep.get("baselink")))
                LOGGER.info("optional " + str(dep.get("optional")))
                if not self.dbmanager.check_installed_dep(dep.get("addon"), dep.get("version")):
                    appinstaller = ApplicationInstaller(self.dbmanager)
                    LOGGER.info("Install %s", dep.get("addon"))
                    appinstaller._install_dependency(dep.get("addon"),
                                                    dep.get("version"),
                                                    dep.get("baselink"),
                                                    dep.get("optional"))
                else:
                    LOGGER.info("dependency [%s] already installed" % dep.get("addon"))
        except InstallerException as ex:
            LOGGER.error("Problem install: " + dep.get("addon"))
            raise ex
        except Exception as ex:
            from traceback import print_exc
            print print_exc()
            LOGGER.info("No dependencies")
            pass


    def _install_dependency(self,
                            scriptId,
                            scriptVersion,
                            scriptBaseLink,
                            isOptional):
        """
        Install application dependency
        """

        appuri = None
        if scriptBaseLink and scriptBaseLink is not "":
            if scriptVersion is not "":
                appuri = scriptBaseLink + "/" +scriptId + "-" + scriptVersion + ".zip"
            else:
                appuri = scriptBaseLink + "/" +scriptId + ".zip"

        LOGGER.info("Install dependency: " + appuri)
        self.installerDep(scriptId, appuri, scriptVersion)

    def _extract_application(self, zipfilepath, extractionpath, password=None):
        """
        Extract application in zip mode
        this allows the use of password if needed
        """
        try:
            zip_ref = ZipFile(zipfilepath, 'r')
            zip_ref.extractall(extractionpath, pwd=password)
            zip_ref.close()
            #Remove zip file from flash
            if os.path.exists(zipfilepath):
                os.remove(zipfilepath)
        except:
            LOGGER.error('Problem extracting zip file: ' + str(zipfilepath))
            raise InstallerException(_("Could not extract application"))

        subprocess.call('sync', shell=True, stdin=None, stdout=None, stderr=None)
        LOGGER.info('success extracting %s in %s' %(zipfilepath, extractionpath))
        return True

    def downloadCallback(self, cursize=None):
        """
        Download callback status
        """
        LOGGER.info( 'callback invoker size: '+ str(cursize))
        try :
            download_info = self.downloader.getInfos()
            if download_info['status'] == sldownloader.CDownload.DOWNLOADING :
                downloadProgress =  (int(download_info['downloaded'])*100/ \
                                    int(download_info['size']))
                LOGGER.info('downloadProgress: '+str(downloadProgress))
            elif download_info['status'] == sldownloader.CDownload.FINISHED :
                LOGGER.info( 'download Finished extract file name: '+
                                str(download_info['localfilename']) )
                splittedPath = download_info['localfilename'].split('/')
                self.lastDirectDownloadedFile = download_info['localfilename']
                self.downloadWaitMutex.lock()
                self.downloadWaitCondition.wakeAll()
                self.downloadWaitMutex.unlock()
            else :
                LOGGER.info( 'download status : '+download_info['status'] )
        except :
            LOGGER.error( '!! Someting went wrong in download !!' )
            self.downloadWaitMutex.lock()
            self.downloadWaitCondition.wakeAll()
            self.downloadWaitMutex.unlock()

    def _check_downloaded_file(self, filepath, fileCheckSum) :
        """
        Check file checksum
        """
        if os.path.exists(filepath) :
            #checksum the downloaded file
            LOGGER.info( "checksum the downloaded file ...")
            if fileCheckSum is None:
                LOGGER.info( "checksum has not been specified, don't check.")
                return True

            if md5(open(filepath).read()).hexdigest() == fileCheckSum :
                LOGGER.info( "checksum success ")
                return True
            else:
                LOGGER.info( "checksum Failed !!" )
                os.remove(filepath)
        #Else problem occured
        LOGGER.info("app download Failed !!")
        return False

