#ifndef OMX_VODENGINE_H
#define OMX_VODENGINE_H

#include "/usr/include/python3.7m/Python.h"
#include <string>
#include <list>
#include <QThread>
#include <QObject>
#include <QtCore/QObject>
#include <QProcess>
#include <QMap>
#include <QtDBus/QtDBus>
#include <QMutex>
#include <QWaitCondition>
#include "KvalThreadUtils.h"

void vodStopScriptTask();

/**
 * @brief The VM_VodPythonInvoker class
 */
class VM_VodPythonInvoker: public QObject
{
    Q_OBJECT

public:
    static void vodStopScriptTask(void);

Q_SIGNALS:
    void movieItemAdd(QString, QStringList, QString, QString, QStringList, int);
    void serieItemAdd(QString, QStringList, QString, QString, int);
    void serieDirAdd(QString, QString, QString, QStringList, int);
    void displayNotifSignal(QString, QString);
    void displayNotifProgressSignal(QString, int);
    void launchViewInfoReady(QStringList, QStringList, QStringList);
    void playableUriReady(QString);
    void trailerListItemsReady(QStringList, QStringList);
    void videoListItemsReady(QStringList);
    void videoUrlReady(QString, int);
    void finishScriptExtract(int, QProcess::ExitStatus);
    void downloadProgressValue(int);
    void getCaptchaValueNotify(QString);

public Q_SLOTS:
    Q_SCRIPTABLE QString addDir(const QString &name,
                                const QString &url,
                                const QString &icon,
                                int totalItems);

    Q_SCRIPTABLE void addSerieDir(  const QString &name,
                                    const QString &url,
                                    const QString &icon,
                                    QString contextList,
                                    int totalItems);

    Q_SCRIPTABLE void vodNotification(const QString &notificationHeader,
                                      const QString &notification);

    Q_SCRIPTABLE void vodNotificationProgress(  const QString &notification, 
                                                int progressValue);

    Q_SCRIPTABLE void addMovieItem( QString movie_title,
                                    QString movie_info,
                                    QString movie_url,
                                    QString movie_icon,
                                    QString contextList,
                                    int totalItems);

    Q_SCRIPTABLE void addSerieItem( QString serie_title,
                                    QString serie_info,
                                    QString serie_url,
                                    QString serie_icon,
                                    int totalItems);

    Q_SCRIPTABLE void addItemInfo(  QString infos_images,
                                    QString infos_right_box,
                                    QString infos_left_box);

    Q_SCRIPTABLE void addTrailerItems(QString trailernames,
                                     QString trailerUrls);

    Q_SCRIPTABLE void addPlayableUri(QString videoUrl);
    Q_SCRIPTABLE void addVideosItems(QString videoNames);
    Q_SCRIPTABLE void addVideoUrl(QString videoUrl, int totalItems);
    Q_SCRIPTABLE void endOfScript();
    Q_SCRIPTABLE void downloadProgress(int downloadProgressVal);
    Q_SCRIPTABLE void getCaptchaValue(QString);
    Q_SCRIPTABLE QString checkCaptchaValue();

private:
    int m_currentTotalItems;
    QMutex m_captchaWaitMutex;
    QWaitCondition m_waitPendingCaptcha;
};

/**
 * @brief The VM_VodEngine class
 */
class VM_VodEngine : public QThread
{
    Q_OBJECT
public:
    enum
    {
        PYTHON_PROC_RUNNING,
        PYTHON_PROC_STOPPED
    };

    enum
    {
        PROC_UNKNOWN = 0,
        PROC_WAIT_ITEM_LIST,
        PROC_WAIT_ITEM_SERIE_LIST,
        PROC_WAIT_INFO_DETAILS,
        PROC_WAIT_TRAILERS_LIST,
        PROC_WAIT_TRAILER_PLAYABLE_URL,
        PROC_WAIT_VIDEO_LIST,
        PROC_WAIT_VIDEO_PLAYABLE_URL,
        PROC_WAIT_STREAM_URI,
        PROC_NOTIFY_DEAD_LINK,
        PROC_WAIT_ADD_TO_FAV,
        PROC_WAIT_DEL_FROM_FAV,
    };
    enum
    {
        MEDIA_UNKNOWN = 0,
        MEDIA_HTTP_PLAYBACK,
        MEDIA_LOCAL_PLAYBACK
    };

    VM_VodEngine();
    ~VM_VodEngine();

    void run();

Q_SIGNALS:
    void readyItemList(QList<QString>);
    void extractedItemLoadingValue(int);
    void extractedInfoReady(QString, QStringList);
    void pbFetchingItems();
    void displayNotifProgressSignal(QString, int);
    void displayNotifSignal(QString, QString);
    void readyItemInfo();
    void launchViewInfoReady(QStringList, QStringList, QStringList);
    void playableUriReady(QString);
    void trailerListItemsReady(QStringList, QStringList);
    void downloadProgressValue(int);
    void videoListItemsReady(QStringList);
    void favItemDeleted();
    void getCaptchaValueNotify(QString);
    void trailerScriptEnd();

public Q_SLOTS:
    void onGenerateItemsNotified(QString, QString, bool);
    void onProcessFinishedSlot(int , QProcess::ExitStatus);
    QString getIconById(int);
    QStringList getMetaById(int index);
    void onSerieDirAdd(QString, QString, QString, QStringList,int);
    void onMovieItemAdd(QString, QStringList, QString, QString, QStringList, int);
    void onSerieItemAdd(QString, QStringList, QString, QString, int );
    void onGetInfoById(int);
    QString getUrlById(int id);
    void onClearPageHistory();
    void setPageNbr(int pageNbr);
    QString getPreviousHistoryUrl();
    QString getPreviousHistoryExtraArg();
    QString getItemInfoUrlByIndex(int);
    void onGenerateItemInfo(QString);
    void onGenerateSerieItemInfo(QString);
    void onLaunchTrailerById(int index);
    void onLaunchVideoById(int index);
    void fetchTrailerPlayableUrl(QString url);
    void onStoreContextById(int);
    void onStoreSeasonById(int);
    void onLaunchViewInfoReady(QStringList, QStringList, QStringList);
    void onGetSerieHistoryList();
    void onGetSerieSeasonHistoryList();
    void onVideoUrlReady(QString, int);
    void onReplyUrlChoice(int);
    void stopCurrentMedia();
    void grabDownloadProgressValue();
    void onPlayableUriReady(QString);
    void onGetShahidPlayableUri(int);
    void onAddMediaToFav(int index);
    void onDeleteMediaFromFav(int index);
    void onClearVodCache();
    void onClearVodDownloads();
    void onSetVodSettings(int, int, bool);
    void onAbortMovieLoading();
    void onDownloadProgressValueReceived(int value);
    void onVodServerInfosNotify(QString, QString);

private:
    void pythonInvokeProc(QString, QString);
    void clearSerieData();
    void onAddPreviousItem();

    //Local Attributes
    int m_cuDownloadProgressValue;
    QProcess m_dpStream;
    QProcess m_imgConverter;
    unsigned int m_dpStreamProcSt;
    VM_VodPythonInvoker m_pythonInvoker;
    QList<QString> m_displayDirListNames;
    QList<QString> m_displayItemListNames;
    QList<QStringList> m_infosList;
    QList<QStringList> m_serieHistoryListDir;
    QList<QStringList> m_serieHistorySeasonListDir;
    QList<QStringList> m_metaDataList;
    bool m_isItemList;
    unsigned int m_procActivity;
    int m_totalItems;
    float m_extractedItemPercent;
    int m_extractedItem;
    QList<QMap<QString, QString> >m_pageUrlHistoryArg;
    QString m_serieTrailerContextUrl;
    QString m_serieInfosContextUrl;
    QString m_serieInfosContextFav;

    int m_pageNbr;
    QStringList m_infos_images;
    QStringList m_infos_right_box;
    QStringList m_infos_left_box;
    QDBusInterface * m_invokerInterface;
    QTimer * m_getProgressDownloadTimer;
    unsigned int m_mediaPlayType;
};

/**
 * @brief The EM_EpgEngine class
 */
class VM_VodElement : public QObject
{
    Q_OBJECT
public:
    VM_VodElement();
    ~VM_VodElement();

    Q_INVOKABLE void generateItems(QString url, QString extraArg);
    Q_INVOKABLE void getItemInfo(int index);
    Q_INVOKABLE void generateItemPage(int index, int pageNbr);
    Q_INVOKABLE void clearPageHistory();
    Q_INVOKABLE void generateItemInfo(int index, bool isSerie);
    Q_INVOKABLE void launchTrailer(QString url);
    Q_INVOKABLE void generateTrailerList(int index);
    Q_INVOKABLE void generateVideoList(int index);
    Q_INVOKABLE void generateSerieItems(int index,bool useContextList);
    Q_INVOKABLE void generateSeasonItems(int index, QString seasonNumber, 
                                         bool useContextList);
    Q_INVOKABLE void getSerieHistoryList(bool isSeason);
    Q_INVOKABLE void generateVideoUrl(int index);
    Q_INVOKABLE void stopNotify();
    Q_INVOKABLE void getShahidPlayableUri(int index);
    Q_INVOKABLE void addMediaToFav(int index);
    Q_INVOKABLE void deleteMediaFromFav(int index);
    Q_INVOKABLE void clearVodCache();
    Q_INVOKABLE void clearVodDownloads();
    Q_INVOKABLE void setVodSettings(int, int, bool);
    Q_INVOKABLE QString getUri(int index);
    Q_INVOKABLE void abortMovieLoading();
    Q_INVOKABLE void setVodServerAddress(QString, QString);

Q_SIGNALS:
    void generateDirNotify(int);
    void readyDirList(QList<QString> dirDisplayList);
    void readyItemList(QList<QString> itemDisplayList);
    void extractedItemLoadingValue(int loadprogressValue);
    void getInfoById(int);
    void extractedInfoReady(QString infoIcon, QStringList metaData);
    void pbFetchingItems();
    void displayNotifProgressSignal(QString notifStr, int progressValue);
    void displayNotifSignal(QString headerMsg, QString mainMsg);
    void readyItemInfo();
    void launchViewInfoReady(QStringList artDataInfo, 
                             QStringList rightDataInfo, 
                             QStringList leftDataInfo);
    void playableUriReady(QString playableUri);
    void trailerListItemsReady(QStringList trailerNames, 
                               QStringList trailerUrls);
    void storeContextById(int);
    void videoListItemsReady(QStringList videoItems);
    void downloadProgressValue(int value);
    void favItemDeleted();
    void getCaptchaValueNotify(QString Uri);
    void trailerScriptEnd();

private:
    //Local Attributes
    VM_VodEngine * m_vodEngine;
};

#endif // OMX_VODENGINE_H
