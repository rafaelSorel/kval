
#include "KvalLocalBrowseManager.h"
#include "KvalConfigManager.h"

#define LOG_ACTIVATED
#define LOG_SRC MEDIA_BROWSER
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Type definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------
DEVICES_SET _available_devices{};

//----------------------------------------------------------------------------
// Local variables
//----------------------------------------------------------------------------
static const char* INFO_DISPLAY_STRING = "Info";
static const char* USB_DISCONNECTED_DISPLAY_STRING =
        QT_TRANSLATE_NOOP("BM_MediaBrowserElements","Support Usb déconnecté");
static const char* USB_MOUNT_SUCCES_DISPLAY_STRING =
        QT_TRANSLATE_NOOP("BM_MediaBrowserElements",
                          "Support Usb connecté avec succés.");
static const char* USB_SCAN_CURRENT_FOLD_DISPLAY_STRING =
        QT_TRANSLATE_NOOP("BM_MediaBrowserElements",
                          "Scan du Dossier courant...");
static const char* USB_SCAN_CURRENT_FOLD_OVER_DISPLAY_STRING =
        QT_TRANSLATE_NOOP("BM_MediaBrowserElements",
                          "Scan terminé...");
//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

/**
 * @brief BM_UsbUtils::run
 */
void KvalDevicesUtils::Start()
{
    INV();
    LOG_INFO(LOG_TAG, "Selected UDev as storage provider");
    m_mounted_volumes = QStorageInfo::mountedVolumes();
    scan_volumes();

    Q_FOREACH(const KvalMediaDevice& device, m_removable_set)
    {
        LOG_INFO(LOG_TAG, "name: %s", qPrintable(device.strName));
        LOG_INFO(LOG_TAG, "path: %s", qPrintable(device.strPath));
        LOG_INFO(LOG_TAG, "devNode: %s", qPrintable(device.devNode));
        LOG_INFO(LOG_TAG, "fs_type: %s", qPrintable(device.fsType));
    }

    m_running = true;
    bm_usb_utils_daemon();
}

/**
 * @brief BM_UsbUtils::get_mounted_disks
 * @return
 */
DEVICES_SET KvalDevicesUtils::get_mounted_disks()
{
    return m_removable_set;
}

/**
 * @brief BM_UsbUtils::is_removable
 * @param device_path
 * @return
 */
bool KvalDevicesUtils::is_removable(const QString &device_path)
{
    INV("device_path: %s" , qPrintable(device_path));
//@ TODO: Check removable device on different os type.
#if defined(Q_OS_LINUX)
    if (device_path.startsWith("/media"))
        return true;
#elif defined(Q_OS_WINDOWS)
    return true;
#elif defined(Q_OS_OSX)
    return true;
#else
    return false;
#endif
    return false;
}
/**
 * @brief BM_UsbUtils::scan_volumes
 */
void KvalDevicesUtils::scan_volumes()
{
    INV();

    QStringList system_volumes{ "/storage", "/flash", "/tmp", "/" };

    auto _create_media_src = [this](const QStorageInfo &storage){

        KvalMediaDevice media_src;
        media_src.devNode = storage.device();
        media_src.strName = storage.name();
        media_src.strPath = storage.rootPath();
        media_src.fsType = storage.fileSystemType();
        media_src.m_ignore = true;

        media_src.m_iDriveType= (this->is_removable(media_src.strPath)) ?
                    KvalMediaDevice::SOURCE_TYPE_REMOVABLE :
                    KvalMediaDevice::SOURCE_TYPE_LOCAL;

        if( media_src.fsType.contains("ext4")     ||
            media_src.fsType.contains("ntfs")     ||
            media_src.fsType.contains("fuseblk")  ||
            media_src.fsType.contains("fat") )
        {
            media_src.recordable_fs = true;
        }
        return media_src;
    };

    Q_FOREACH (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            if (std::find(system_volumes.begin(),
                          system_volumes.end(),
                          storage.name()) == system_volumes.end())
            {
                auto device = _create_media_src(storage);
                if (std::find(m_removable_set.begin(),
                              m_removable_set.end(),
                              device) == m_removable_set.end())
                {
                    m_removable_set.push_back(device);
                }
            }
        }
    }

    _available_devices = m_removable_set;
}

/**
 * @brief BM_UsbUtils::bm_usb_utils_daemon
 */
void KvalDevicesUtils::bm_usb_utils_daemon()
{
    INV();

    while(m_running)
    {
        auto mounted_volumes = QStorageInfo::mountedVolumes();
        if (mounted_volumes.size() > m_mounted_volumes.size())
        {
            LOG_INFO(LOG_TAG, "New device has been connected to the system");
            scan_volumes();
            notify_new_connected_device();
            m_mounted_volumes = mounted_volumes;
        }
        else if(mounted_volumes.size() < m_mounted_volumes.size())
        {
            LOG_INFO(LOG_TAG, "A device has been disconnected from the system");
            m_removable_set.clear();
            scan_volumes();
            notify_disconnected_device();
            m_mounted_volumes = mounted_volumes;
        }

        QThread::usleep(50*1000);
    }
}


/**
 * @brief BM_UsbUtils::Stop
 */
void KvalDevicesUtils::Stop()
{
    INV();
    m_running = false;
    OUTV();
}

/**
 * @brief BM_UsbUtils::notify_disconnected_device
 */
void KvalDevicesUtils::notify_disconnected_device()
{
    Q_EMIT displayMsg(INFO_DISPLAY_STRING ,
                    USB_DISCONNECTED_DISPLAY_STRING);
    Q_EMIT usbUtilNotifyDisconnectedDevice();
}
/**
 * @brief BM_UsbUtils::notify_connected_devices
 */
void KvalDevicesUtils::notify_new_connected_device()
{
    Q_EMIT usbUtilNotifyConnectedDevice();
    Q_EMIT displayMsg(INFO_DISPLAY_STRING, USB_MOUNT_SUCCES_DISPLAY_STRING);
    check_for_update_package();
}

/**
 * @brief BM_UsbUtils::check_for_update_package
 */
void KvalDevicesUtils::check_for_update_package()
{
    //Check firmware update
    Q_FOREACH(const KvalMediaDevice& device, m_removable_set)
    {
        QDir rootUsbDir(device.strPath);

        QStringList nameFilters;
        QStringList rootFolderContNames;
        QFileInfoList rootFolderContInfo;
        QStringList firmFolderContNames;
        QFileInfoList firmFolderContInfo;

        rootFolderContNames =  rootUsbDir.entryList(nameFilters,
                                        QDir::AllEntries | QDir::NoDotAndDotDot,
                                        QDir::DirsFirst);
        rootFolderContInfo =  rootUsbDir.entryInfoList(nameFilters,
                                        QDir::AllEntries | QDir::NoDotAndDotDot,
                                        QDir::DirsFirst);

        for(int i=0; i<rootFolderContNames.length(); i++)
        {
            if(!rootFolderContNames[i].contains("_firmware"))
            {
                continue;
            }
            QDir firmwareUpDir(rootFolderContInfo[i].absoluteFilePath());
            firmFolderContNames =  firmwareUpDir.entryList(nameFilters,
                                    QDir::AllEntries | QDir::NoDotAndDotDot,
                                    QDir::DirsFirst);
            firmFolderContInfo =  firmwareUpDir.entryInfoList(nameFilters,
                                    QDir::AllEntries | QDir::NoDotAndDotDot,
                                    QDir::DirsFirst);
            for(int j=0; j<firmFolderContNames.length(); j++)
            {
                if(!firmFolderContNames[j].contains("_firmware"))
                {
                    continue;
                }
                QString destPath(
                CfgObj->get_path(
                KvalConfigManager::KVAL_LOCAL_DOWNLOAD_FIRM_FOLDER));
                destPath.append(firmFolderContNames[j]);
                LOG_INFO(LOG_TAG, "destPath: %s",
                         qPrintable(destPath));

                QFile::copy(firmFolderContInfo[j].absoluteFilePath(),
                             destPath);
                Q_EMIT newFirmwareReceivedStatus("success",
                                destPath,
                                "");
                return;
            }
        }
    }

}

/**
 * @brief BM_MediaBrowserUtils::getSupportedImageFormats
 * @return 
 */
const QStringList& KvalSupportedMedia::getSupportedImageFormats()
{
   static QStringList supported;
   if (supported.isEmpty()) {
      QList<QByteArray> mimes = QImageReader::supportedMimeTypes();
      Q_FOREACH (QByteArray a, mimes)
         supported.append(QString(a));
   }

   return supported;
}

/**
 * @brief BM_MediaBrowserUtils::getSupportedImageExtensions
 * @return 
 */
const QStringList& KvalSupportedMedia::getSupportedImageExtensions()
{
   static QStringList ret;
   if (!ret.isEmpty())
      return ret;

   const QStringList supported = 
           KvalSupportedMedia::getSupportedImageFormats();

   Q_FOREACH (QString a, supported) {
      QStringList mimes = getMimeToExtMap().values(a);
      if (mimes.isEmpty()) {
         qDebug("Can't find extension for %s.", qPrintable(a));
         continue;
      }

      Q_FOREACH (QString b, mimes)
         ret.append(b.toLocal8Bit());
   }

   return ret;
}

/**
 * @brief BM_MediaBrowserUtils::getSupportedVideoExtensions
 * @return 
 */
const QStringList& KvalSupportedMedia::getSupportedVideoExtensions()
{
   static QStringList supported;
   if (supported.isEmpty()) {
      supported.append("mp4");
      supported.append("mov");
      supported.append("mkv");
      supported.append("ts");
      supported.append("avi");
      supported.append("flv");
      supported.append("m4v");
      supported.append("mpeg");
      supported.append("mpg");
      supported.append("mpe");
      supported.append("ogg");
      supported.append("rm");
      supported.append("swf");
      supported.append("wmv");
      supported.append("cam");
   }

   return supported;
}

/**
 * @brief BM_MediaBrowserUtils::getSupportedAudioExtensions
 * @return 
 */
const QStringList& KvalSupportedMedia::getSupportedAudioExtensions()
{
   static QStringList supported;
   if (supported.isEmpty()) {
      supported.append("mp3");
      supported.append("ogg");
      supported.append("wma");
      supported.append("wav");
      supported.append("m4a");

      // TODO: Add the other supported extensions.
   }

   return supported;
}

/**
 * @brief BM_MediaBrowserUtils::getFilterFromExts
 * @param extensions
 * @return 
 */
const QStringList KvalSupportedMedia::getFilterFromExts(const QStringList& 
                                                           extensions)
{
   QStringList ret;
   Q_FOREACH (QString a, extensions)
      ret.append(QString("*.") + a);
   return ret;
}

/**
 * @brief BM_MediaBrowserUtils::isSupportedAudio
 * @param file
 * @return 
 */
bool KvalSupportedMedia::isSupportedAudio(QString file)
{
   QString ext = getFileExtension(file);
   if (ext.isNull())
      return false;

   QStringList supported = getSupportedAudioExtensions();
   return supported.contains(ext);
}

/**
 * @brief BM_MediaBrowserUtils::isSupportedVideo
 * @param file
 * @return 
 */
bool KvalSupportedMedia::isSupportedVideo(QString file)
{
   QString ext = getFileExtension(file);
   if (ext.isNull())
      return false;

   QStringList supported = getSupportedVideoExtensions();
   return supported.contains(ext);
}

/**
 * @brief BM_MediaBrowserUtils::isSupportedImage
 * @param file
 * @return 
 */
bool KvalSupportedMedia::isSupportedImage(QString file)
{
   QString ext = getFileExtension(file);
   if (ext.isNull())
      return false;

   QStringList supported = getSupportedImageExtensions();
   return supported.contains(ext);
}

/**
 * @brief BM_MediaBrowserUtils::listSupportedImageFormats
 */
void KvalSupportedMedia::listSupportedImageFormats()
{
   QList<QString> supported = getSupportedImageFormats();
   Q_FOREACH (QString s, supported)
      qDebug("Supported format: %s.", qPrintable(s));
}

/**
 * @brief BM_MediaBrowserUtils::listSupportedImageExtensions
 */
void KvalSupportedMedia::listSupportedImageExtensions()
{
   QList<QString> supported = getSupportedImageExtensions();
   Q_FOREACH (QString s, supported)
      qDebug("Supported extension: %s.", qPrintable(s));
}

/**
 * @brief BM_MediaBrowserUtils::getMimeToExtMap
 * @return 
 */
const QHash<QString, QString>& KvalSupportedMedia::getMimeToExtMap()
{
   static QMutex mutex;
   QMutexLocker locker(&mutex);

   static QHash<QString, QString> map;
   if (map.isEmpty()) {
      map.insertMulti("image/bmp", "bmp");
      map.insertMulti("image/cgm", "cgm");
      map.insertMulti("image/g3fax", "g3");
      map.insertMulti("image/gif", "gif");
      map.insertMulti("image/ief", "ief");
      map.insertMulti("image/jpeg", "jpeg");
      map.insertMulti("image/jpeg", "jpg");
      map.insertMulti("image/jpeg", "jpe");
      map.insertMulti("image/ktx", "ktx");
      map.insertMulti("image/png", "png");
      map.insertMulti("image/prs.btif", "btif");
      map.insertMulti("image/sgi", "sgi");
      map.insertMulti("image/svg+xml", "svgz");
      map.insertMulti("image/svg+xml", "svg");
      map.insertMulti("image/tiff", "tiff");
      map.insertMulti("image/tiff", "tif");
      map.insertMulti("image/vnd.adobe.photoshop", "psd");
      map.insertMulti("image/vnd.dece.graphic", "uvi");
      map.insertMulti("image/vnd.dece.graphic", "uvvi");
      map.insertMulti("image/vnd.dece.graphic", "uvg");
      map.insertMulti("image/vnd.dece.graphic", "uvvg");
      map.insertMulti("image/vnd.dvb.subtitle", "sub");
      map.insertMulti("image/vnd.djvu", "djvu djv");
      map.insertMulti("image/vnd.djvu", "djv");
      map.insertMulti("image/vnd.dwg", "dwg");
      map.insertMulti("image/vnd.dxf", "dxf");
      map.insertMulti("image/vnd.fastbidsheet", "fbs");
      map.insertMulti("image/vnd.fpx", "fpx");
      map.insertMulti("image/vnd.fst", "fst");
      map.insertMulti("image/vnd.fujixerox.edmics-mmr", "mmr");
      map.insertMulti("image/vnd.fujixerox.edmics-rlc", "rlc");
      map.insertMulti("image/vnd.ms-modi", "mdi");
      map.insertMulti("image/vnd.ms-photo", "wdp");
      map.insertMulti("image/vnd.net-fpx", "npx");
      map.insertMulti("image/vnd.wap.wbmp", "wbmp");
      map.insertMulti("image/vnd.xiff", "xif");
      map.insertMulti("image/webp", "webp");
      map.insertMulti("image/x-3ds", "3ds");
      map.insertMulti("image/x-cmu-raster", "ras");
      map.insertMulti("image/x-cmx", "cmx");
      map.insertMulti("image/x-freehand", "fh");
      map.insertMulti("image/x-freehand", "fhc");
      map.insertMulti("image/x-freehand", "fh4");
      map.insertMulti("image/x-freehand", "fh5");
      map.insertMulti("image/x-freehand", "fh7");
      map.insertMulti("image/x-icon", "ico");
      map.insertMulti("image/x-mrsid-image", "sid");
      map.insertMulti("image/x-pcx", "pcx");
      map.insertMulti("image/x-pict", "pct");
      map.insertMulti("image/x-pict", "pic");
      map.insertMulti("image/x-portable-anymap", "pnm");
      map.insertMulti("image/x-portable-bitmap", "pbm");
      map.insertMulti("image/x-portable-graymap", "pgm");
      map.insertMulti("image/x-portable-pixmap", "ppm");
      map.insertMulti("image/x-rgb", "rgb");
      map.insertMulti("image/x-tga", "tga");
      map.insertMulti("image/x-xbitmap", "xbm");
      map.insertMulti("image/x-xpixmap", "xpm");
      map.insertMulti("image/x-xwindowdump", "xwd");
   }

   return map;
}

/**
 * @brief BM_MediaBrowserUtils::getFileExtension
 * @param file
 * @return 
 */
QString KvalSupportedMedia::getFileExtension(const QString& file)
{
   QFileInfo fileInfo(file);
   return fileInfo.suffix();
}

/**
 * @brief BM_MediaBrowserElements::BM_MediaBrowserElements
 * @param parent
 */
KvalMediaBrowserElements::KvalMediaBrowserElements():
    m_currentActiveDir{},
    m_currentListInfo{},
    m_currentDisplayedList{},
    m_usbUtilsThread{new KvalThread("KvalDevicesUtils")},
    m_usbUtils{new KvalDevicesUtils}
{

    m_usbUtils->moveToThread(m_usbUtilsThread);
    connect(m_usbUtilsThread, &QThread::finished, m_usbUtils, &QObject::deleteLater);
    m_usbUtilsThread->start();

    connect(m_usbUtils,
           SIGNAL(usbUtilNotifyConnectedDevice()),
           this,
           SIGNAL(usbUtilNotifyConnectedDevice()));
    connect(m_usbUtils,
           SIGNAL(usbUtilNotifyDisconnectedDevice()),
           this,
           SIGNAL(usbUtilNotifyDisconnectedDevice()));
    connect(m_usbUtils,
           SIGNAL(newFirmwareReceivedInProgress()),
           this,
           SIGNAL(newFirmwareReceivedInProgress()));
    connect(m_usbUtils,
           SIGNAL(newFirmwareReceivedStatus(QString, QString, QString)),
           this,
           SIGNAL(newFirmwareReceivedStatus(QString, QString, QString)));
    connect(m_usbUtils,
           SIGNAL(displayProgress(QString, float)),
           this,
           SIGNAL(displayProgress(QString, float)));
    connect(m_usbUtils,
           SIGNAL(displayMsg(QString, QString)),
           this,
           SIGNAL(displayMsg(QString, QString)));

    QMetaObject::invokeMethod(m_usbUtils, "Start");

}

/**
 * @brief BM_MediaBrowserElements::~BM_MediaBrowserElements
 */
KvalMediaBrowserElements::~KvalMediaBrowserElements()
{
    LOG_INFO(LOG_TAG, "Delete BM_MediaBrowserElements...");
    if(m_usbUtils)
    {
        m_usbUtils->Stop();
        m_usbUtilsThread->quit();
        m_usbUtilsThread->wait();
        delete m_usbUtilsThread;
    }
}

/**
 * @brief BM_MediaBrowserElements::getActiveDevices
 * @return
 */
QStringList KvalMediaBrowserElements::getActiveDevices()
{
    DEVICES_SET devices = m_usbUtils->get_mounted_disks();
    QStringList devices_path;

    Q_FOREACH(const KvalMediaDevice& device, devices)
    {
        LOG_INFO(LOG_TAG, "device: %s", qPrintable(device.strPath));
        devices_path << device.strPath;
    }
    return devices_path;
}

/**
 * @brief BM_MediaBrowserElements::getHomeDir
 * @return 
 */
QString KvalMediaBrowserElements::getUsbMountDir()
{
    DEVICES_SET devices = m_usbUtils->get_mounted_disks();

    //@ TODO: Handle more than one mounted disks
    Q_FOREACH(const KvalMediaDevice& device, devices)
    {
        LOG_INFO(LOG_TAG, "device: %s", qPrintable(device.strPath));
        auto device_dir = QDir(device.strPath);
        device_dir.cdUp();
        return device_dir.path();
    }

    return "";
}

/**
 * @brief BM_MediaBrowserElements::getPathFromUri
 * Returns the path given a local (file://) URI.
 * Otherwise a null string is returned.
 * @param uri
 * @return
 */
QString KvalMediaBrowserElements::getPathFromUri(QString uri)
{
   if (!uri.startsWith("file://", Qt::CaseSensitive))
      return uri;

   return uri.mid(7, uri.size() - 7);
}

/**
 * @brief BM_MediaBrowserElements::getFileExtension 
 * Returns the file extension. file can be abs or relative.
 * @param file
 * @return
 */
QString KvalMediaBrowserElements::getFileExtension(QString file)
{
   return KvalSupportedMedia::getFileExtension(file);
}

/**
 * @brief BM_MediaBrowserElements::isSupportedAudio
 * @param file
 * @return 
 */
bool KvalMediaBrowserElements::isSupportedAudio(QString file)
{
   return KvalSupportedMedia::isSupportedAudio(file);
}

/**
 * @brief BM_MediaBrowserElements::isSupportedVideo
 * @param file
 * @return 
 */
bool KvalMediaBrowserElements::isSupportedVideo(QString file)
{
   return KvalSupportedMedia::isSupportedVideo(file);
}

/**
 * @brief BM_MediaBrowserElements::isSupportedImage
 * @param file
 * @return 
 */
bool KvalMediaBrowserElements::isSupportedImage(QString file)
{
   return KvalSupportedMedia::isSupportedImage(file);
}

/**
 * @brief BM_MediaBrowserElements::getPrevImage 
 * Returns the alphabetically next image in the same directory.
 * @param imageAbsPath
 * @return
 */
QString KvalMediaBrowserElements::getNextImage(QString imageAbsPath)
{
   QFileInfo fileInfo(imageAbsPath);
   QDir dir = fileInfo.absoluteDir();

   // List the files in the directory.
   const QStringList& extensions = KvalSupportedMedia::getSupportedImageExtensions();
   QFileInfoList files = 
           dir.entryInfoList(KvalSupportedMedia::getFilterFromExts(extensions), 
                             QDir::Files, QDir::Name);
   if (files.size() <= 0)
      return QString();

   int index = files.lastIndexOf(QFileInfo(imageAbsPath));
   if (index < -1)
      return QString();

   index = (index + 1)%files.size();
   return files.at(index).absoluteFilePath();
}

/**
 * @brief BM_MediaBrowserElements::getPrevImage 
 * Returns the alphabetically previous image in the same directory.
 * @param imageAbsPath
 * @return
 */
QString KvalMediaBrowserElements::getPrevImage(QString imageAbsPath)
{
   QFileInfo fileInfo(imageAbsPath);
   QDir dir = fileInfo.absoluteDir();

   // List the files in the directory.
   const QStringList& extensions = 
           KvalSupportedMedia::getSupportedImageExtensions();
   QFileInfoList files = 
           dir.entryInfoList(KvalSupportedMedia::getFilterFromExts(extensions), 
                             QDir::Files, QDir::Name);
   if (files.size() <= 0)
      return QString();

   int index = files.lastIndexOf(QFileInfo(imageAbsPath));
   if (index < -1)
      return QString();

   index = (index <= 0) ? files.size() - 1 : index - 1;
   return files.at(index).absoluteFilePath();
}

/**
 * @brief BM_MediaBrowserElements::getSupportedImageExtensions 
 * Returns a list of the supported image extensions on the current platform.
 * @return
 */
QStringList KvalMediaBrowserElements::getSupportedImageExtensions()
{
   return KvalSupportedMedia::getSupportedImageExtensions();
}

/**
 * @brief BM_MediaBrowserElements::getCount
 * @param imageAbsPath
 * @return 
 */
int KvalMediaBrowserElements::getCount(QString path)
{
    QDir dir(this->getPathFromUri(path));
    return dir.count();
}

/**
 * @brief BM_MediaBrowserElements::getListByPath
 * @param path
 * @return 
 */
QStringList KvalMediaBrowserElements::getListByPath(QString path)
{
    m_currentDisplayedList.clear();
    m_currentListInfo.clear();
    m_currentActiveDir.setCurrent(getPathFromUri(path));
    LOG_INFO(LOG_TAG, "====> path: %s", qPrintable(path));

    Q_EMIT displayProgress(USB_SCAN_CURRENT_FOLD_DISPLAY_STRING, 10);
    QString currentDir = path;
    if(!m_currentListInfo.isEmpty())
    {
        LOG_INFO(LOG_TAG, ">>>>>>>>> m_currentListInfo not empty");
        currentDir = getCurrentDir(0);
        LOG_INFO(LOG_TAG, "currentDir: %s", qPrintable(currentDir));
    }

    QStringList nameFilters;
    if( currentDir.length() <= 11 )
    {
        m_currentDisplayedList = m_currentActiveDir.entryList(nameFilters,
                                        QDir::AllEntries | QDir::NoDotAndDotDot,
                                        QDir::DirsFirst);
        Q_EMIT displayProgress(USB_SCAN_CURRENT_FOLD_DISPLAY_STRING, 60);
        m_currentListInfo =  m_currentActiveDir.entryInfoList(nameFilters,
                                        QDir::AllEntries | QDir::NoDotAndDotDot,
                                        QDir::DirsFirst);
    }
    else
    {
        m_currentDisplayedList = m_currentActiveDir.entryList(nameFilters,
                                            QDir::AllEntries | QDir::NoDot,
                                            QDir::DirsFirst);
        Q_EMIT displayProgress(USB_SCAN_CURRENT_FOLD_DISPLAY_STRING, 60);
        m_currentListInfo =  m_currentActiveDir.entryInfoList(nameFilters,
                                            QDir::AllEntries | QDir::NoDot,
                                            QDir::DirsFirst);
    }
    Q_EMIT displayProgress(USB_SCAN_CURRENT_FOLD_DISPLAY_STRING, 95);
    usleep(200*1000);
    Q_EMIT displayMsg(INFO_DISPLAY_STRING,
                    USB_SCAN_CURRENT_FOLD_OVER_DISPLAY_STRING);
    return m_currentDisplayedList;
}

/**
 * @brief BM_MediaBrowserElements::getFileModified
 * @param index
 * @return 
 */
QString KvalMediaBrowserElements::getFileModified(int index)
{
    QDateTime lastModified = m_currentListInfo[index].lastModified();
    return lastModified.toString("ddd mm yyy hh:mm");
}

/**
 * @brief BM_MediaBrowserElements::getFileSize
 * @param index
 * @return 
 */
unsigned long KvalMediaBrowserElements::getFileSize(int index)
{
    return m_currentListInfo[index].size();
}

/**
 * @brief BM_MediaBrowserElements::isDirectory
 * @param index
 * @return 
 */
bool KvalMediaBrowserElements::isDirectory(int index)
{
    return m_currentListInfo[index].isDir();
}

/**
 * @brief BM_MediaBrowserElements::getSelectedPath
 * @param index
 * @return 
 */
QString KvalMediaBrowserElements::getSelectedPath(int index)
{
    return m_currentListInfo[index].absoluteFilePath();
}

/**
 * @brief BM_MediaBrowserElements::getParentPath
 * @param index
 * @return 
 */
QString KvalMediaBrowserElements::getAbsolutePath(int index)
{
    return m_currentListInfo[index].absolutePath();
}

/**
 * @brief BM_MediaBrowserElements::getCurrentDir
 * @param index
 * @return
 */
QString KvalMediaBrowserElements::getCurrentDir(int index)
{
    return m_currentListInfo[index].dir().currentPath() ;
}

