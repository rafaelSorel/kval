#ifndef KVAL_BROWSERMANAGER_H
#define KVAL_BROWSERMANAGER_H
#include <QObject>
#include <QThread>
#include <QStringList>
#include <QStorageInfo>
#include <QDir>
#include <QVector>
#include <QFile>
#include <QMap>
#include <QDate>
#include <QDateTime>
#include <QTimer>
#include <QImageReader>
#include <QMutex>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QDebug>

#include "KvalThreadUtils.h"

class KvalDevicesUtils;
struct KvalMediaDevice;

typedef QVector<KvalMediaDevice> DEVICES_SET;
extern DEVICES_SET _available_devices;

struct KvalMediaDevice
{
  enum SourceType
  {
    SOURCE_TYPE_UNKNOWN = 0,
    SOURCE_TYPE_LOCAL,
    SOURCE_TYPE_REMOTE,
    SOURCE_TYPE_REMOVABLE,
    SOURCE_TYPE_MAX,
  };

  KvalMediaDevice() :
      m_iDriveType{SOURCE_TYPE_UNKNOWN},
      m_iHasLock{0},
      m_iBadPwdCount{0},
      m_ignore{false},
      m_allowSharing{true},
      recordable_fs{false}  {}

  virtual ~KvalMediaDevice() = default;

  bool operator==(const KvalMediaDevice &right) const{
        return (   (right.strName == this->strName)
                && (right.strPath == this->strPath) );
  }


  QString devNode;
  QString fsType; /// Filesytem type: NTFS, ext4, vFat ...
  QString strName; ///< Name of the share, can be choosen freely.
  QString strStatus; ///< Status of the share (eg has disk etc.)
  QString strDiskUniqueId; ///< removable:// + DVD Label + DVD ID for resume point storage, if available
  QString strPath; ///< Path of the share, eg. iso9660:// or F:

  SourceType m_iDriveType;
  QString m_strLockCode;  ///< Input code for Lock UI to verify, can be chosen freely.
  int m_iHasLock;
  int m_iBadPwdCount; ///< Number of wrong passwords user has entered since share was last unlocked

  QString m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  QStringList vecPaths;
  bool m_ignore; /// <Do not store in xml
  bool m_allowSharing; /// <Allow browsing of source from UPnP / WebServer
  bool recordable_fs;
};

/**
 * @brief The BM_MediaBrowserUtils class
 */
class KvalDevicesUtils: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KvalDevicesUtils);

    friend struct KvalMediaDevice;
public:
    KvalDevicesUtils() = default;
    virtual ~KvalDevicesUtils() = default;

    DEVICES_SET get_mounted_disks();
    static DEVICES_SET get_available_devices() { return _available_devices; }

public Q_SLOTS:
    void Start();
    void Stop();

Q_SIGNALS:
    void usbUtilNotifyConnectedDevice();
    void usbUtilNotifyDisconnectedDevice();
    void newFirmwareReceivedInProgress();
    void newFirmwareReceivedStatus(QString status,
                                   QString filename,
                                   QString sendDate);
    void displayProgress(QString, float);
    void displayMsg(QString header, QString msg);

private:
    QList<QStorageInfo> m_mounted_volumes{};
    DEVICES_SET m_removable_set;
    bool m_deviceConnectedFlag;
    bool m_running;

private:
    void scan_volumes();
    void notify_disconnected_device();
    void notify_new_connected_device();
    void check_for_update_package();
    void bm_usb_utils_daemon();
    bool is_removable(const QString &device_path);
};

/**
 * @brief The BM_MediaBrowserUtils class
 */
struct KvalSupportedMedia
{
   static const QStringList& getSupportedImageFormats();
   static const QStringList& getSupportedImageExtensions();
   static const QStringList& getSupportedVideoExtensions();
   static const QStringList& getSupportedAudioExtensions();

   static const QStringList getFilterFromExts(const QStringList& extensions);

   static bool isSupportedVideo(QString file);
   static bool isSupportedImage(QString file);
   static bool isSupportedAudio(QString file);

   static void listSupportedImageFormats();
   static void listSupportedImageExtensions();

   static const QHash<QString, QString>& getMimeToExtMap();

   static QString getFileExtension(const QString& file);
};

/**
 * @brief The BM_MediaBrowserElements class
 */
class KvalMediaBrowserElements : public QObject
{
    Q_OBJECT
public:
    KvalMediaBrowserElements();
    virtual ~KvalMediaBrowserElements();

    Q_INVOKABLE QString getUsbMountDir();
    Q_INVOKABLE QStringList getActiveDevices();
    Q_INVOKABLE QString getPathFromUri(QString uri);
    Q_INVOKABLE QString getFileExtension(QString file);
    Q_INVOKABLE int getCount(QString path);
    Q_INVOKABLE QStringList getListByPath(QString path);
    Q_INVOKABLE QString getFileModified(int index);
    Q_INVOKABLE QString getSelectedPath(int index);
    Q_INVOKABLE QString getAbsolutePath(int index);
    Q_INVOKABLE QString getCurrentDir(int index);
    Q_INVOKABLE unsigned long getFileSize(int index);
    Q_INVOKABLE bool isDirectory(int index);

    // File methods
    Q_INVOKABLE bool isSupportedImage(QString file);
    Q_INVOKABLE bool isSupportedVideo(QString file);
    Q_INVOKABLE bool isSupportedAudio(QString file);
 
    // Gallery methods.
    Q_INVOKABLE QString getNextImage(QString imageAbsPath);
    Q_INVOKABLE QString getPrevImage(QString imageAbsPath);
    Q_INVOKABLE QStringList getSupportedImageExtensions();

Q_SIGNALS:
    void usbUtilNotifyConnectedDevice();
    void usbUtilNotifyDisconnectedDevice();
    void usbUtilNotifyFirmwareUpdate();
    void newFirmwareReceivedInProgress();
    void newFirmwareReceivedStatus(QString status,
                                   QString filename,
                                   QString sendDate);
    void displayProgress(QString msg, float progressVal);
    void displayMsg(QString header, QString msg);

private:
    QDir m_currentActiveDir;
    QFileInfoList m_currentListInfo;
    QStringList m_currentDisplayedList;
    KvalThread * m_usbUtilsThread;
    KvalDevicesUtils * m_usbUtils;
};

#endif // KVAL_BROWSERMANAGER_H
