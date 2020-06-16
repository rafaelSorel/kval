#define LOG_ACTIVATED
#include <QFile>
#include <QThread>
#include <QMap>
#include <QDate>
#include <QXmlStreamReader>
#include <QtNetwork>
#include <QtDBus/QtDBus>
#include <QProcess>
#include <QTimer>
#include <QCryptographicHash>

#define LOG_SRC VODENGINESTR
#include "KvalLogging.h"
#include "KvalVodManager.h"
#include "KvalConfigManager.h"
#include "KvalThreadUtils.h"

#define SERVICE_NAME    "org.QtDBus.vod"

#define CHECK_RUNNING_SCRIPT \
    if(m_dpStreamProcSt == VM_VodEngine::PYTHON_PROC_RUNNING){ \
        LOG_INFO(LOG_TAG, "Already running script..."); \
        return; \
    } \

const char PREVIOUS_PAGE_STR[] = "Page précédente...";
QDBusInterface * g_dbusInterface = nullptr;

/**
 * @brief vodStopScriptTask
 * @return
 */
void VM_VodPythonInvoker::vodStopScriptTask()
{
    if(g_dbusInterface)
        g_dbusInterface->call(QDBus::Block,"stopScript");
}

/**
 * @brief VM_VodPythonInvoker::getCaptchaValue
 * @param Uri
 */
void VM_VodPythonInvoker::getCaptchaValue(QString Uri)
{
    INV("Uri %s", qPrintable(Uri));
    Q_EMIT getCaptchaValueNotify(Uri);
}

QString VM_VodPythonInvoker::checkCaptchaValue()
{
    INV("");
    OUTV("");
    return "";
}

/**
 * @brief VM_VodPythonInvoker::addMovieItem
 * @param movie_title
 * @param movie_info
 * @param movie_url
 * @param movie_icon
 * @param totalItems
 */
void VM_VodPythonInvoker::addMovieItem(QString movie_title,
                                     QString movie_info,
                                     QString movie_url,
                                     QString movie_icon,
                                     QString contextList,
                                     int totalItems)
{
    LOG_INFO(LOG_TAG,"Movie Item Received");
    QStringList infos = movie_info.split('\t');
    QStringList ctxs = contextList.split('\t');

    Q_EMIT movieItemAdd(movie_title, infos,
                      movie_url, movie_icon, 
                      ctxs, totalItems);
}

/**
 * @brief VM_VodPythonInvoker::downloadProgress
 * @param downloadProgress
 */
void VM_VodPythonInvoker::downloadProgress(int downloadProgressVal)
{
    Q_EMIT downloadProgressValue(downloadProgressVal);
}

/**
 * @brief VM_VodPythonInvoker::endOfScript
 */
void VM_VodPythonInvoker::endOfScript()
{
    LOG_INFO(LOG_TAG, ">>>>>>>>>>>>>>>>> endOfScript...");
    Q_EMIT finishScriptExtract(0, QProcess::NormalExit);
}

/**
 * @brief VM_VodPythonInvoker::addSerieItem
 * @param serie_title
 * @param serie_info
 * @param serie_url
 * @param serie_icon
 * @param contextList
 * @param totalItems
 */
void VM_VodPythonInvoker::addSerieItem(QString serie_title,
                                     QString serie_info,
                                     QString serie_url,
                                     QString serie_icon,
                                     int totalItems)
{
    Q_EMIT serieItemAdd(serie_title, serie_info.split('\t'), serie_url, serie_icon,totalItems);
}

/**
 * @brief VM_VodPythonInvoker::ping
 * @param name
 * @param url
 * @param icon
 * @return
 */
QString VM_VodPythonInvoker::addDir(const QString &name,
                                const QString &url,
                                const QString &icon, int totalItems)
{

    return QString("OK");
}

/**
 * @brief VM_VodPythonInvoker::addSerieDir
 * @param name
 * @param url
 * @param icon
 * @param contextList
 * @param totalItems
 * @return
 */
void VM_VodPythonInvoker::addSerieDir(const QString &name,
                                        const QString &url,
                                        const QString &icon,
                                        QString contextList,
                                        int totalItems)
{
    Q_EMIT serieDirAdd(name, url, icon, contextList.split('\t'), totalItems);
}

/**
 * @brief VM_VodPythonInvoker::addItemInfo
 * @param infos_images
 * @param infos_right_box
 * @param infos_left_box
 */
void VM_VodPythonInvoker::addItemInfo(QString infos_images,
                                    QString infos_right_box,
                                    QString infos_left_box)
{
    LOG_INFO(LOG_TAG,"Recv Detailed informations");
    Q_EMIT launchViewInfoReady(infos_images.split('\t'),
                             infos_right_box.split('\t'),
                             infos_left_box.split('\t'));
}

/**
 * @brief VM_VodPythonInvoker::addTrailerItems
 * @param trailernames
 * @param trailerUrls
 */
void VM_VodPythonInvoker::addTrailerItems(QString trailernames,
                                        QString trailerUrls)
{
    LOG_INFO(LOG_TAG,"Recv Trailer List");
    Q_EMIT trailerListItemsReady(trailernames.split('\t'),
                               trailerUrls.split('\t'));
}

/**
 * @brief VM_VodPythonInvoker::addVideosItems
 * @param videoNames
 */
void VM_VodPythonInvoker::addVideosItems(QString videoNames)
{
    LOG_INFO(LOG_TAG,"Recv Videos List");
    Q_EMIT videoListItemsReady(videoNames.split('\t'));
    this->endOfScript();
}

/**
 * @brief VM_VodPythonInvoker::addVideoUrl
 * @param videoUrl
 * @param totalItems
 */
void VM_VodPythonInvoker::addVideoUrl(QString videoUrl, int totalItems)
{
    Q_EMIT videoUrlReady(videoUrl, totalItems);
}

/**
 * @brief VM_VodPythonInvoker::addTrailerInfos
 * @param videoName
 * @param videoUrl
 */
void VM_VodPythonInvoker::addPlayableUri(QString videoUrl)
{
    LOG_INFO(LOG_TAG,"videoUrl %s", qPrintable(videoUrl));
    Q_EMIT playableUriReady(videoUrl);
}

/**
 * @brief VM_VodPythonInvoker::vodNotification
 * @param progressValue
 * @param notification
 */
void VM_VodPythonInvoker::vodNotification(const QString &notificationHeader ,
                                        const QString &notification)
{
    LOG_INFO(LOG_TAG,"Notification: %s ", qPrintable(notification));
    Q_EMIT displayNotifSignal(notificationHeader, notification);
}

/**
 * @brief VM_VodPythonInvoker::vodNotificationProgress
 * @param notification
 * @param progressValue
 */
void VM_VodPythonInvoker::vodNotificationProgress(const QString &notification, 
                                                int progressValue)
{
    LOG_INFO(LOG_TAG,"Notification: %s --- %d",
             qPrintable(notification), progressValue);
    Q_EMIT displayNotifProgressSignal(notification, progressValue);
}

/**
 * @brief VM_VodEngine::VM_VodEngine
 */
VM_VodEngine::VM_VodEngine():
    m_dpStreamProcSt(PYTHON_PROC_STOPPED),
    m_procActivity(PROC_UNKNOWN),
    m_totalItems(0),
    m_extractedItem(0),
    m_pageNbr(0),
    m_mediaPlayType(MEDIA_UNKNOWN)
{
    LOG_INFO(LOG_TAG, "Instantiate VM_VodEngine");

    if (!QDBusConnection::sessionBus().isConnected())
    {
        LOG_ERROR(LOG_TAG,  "Cannot connect to the D-Bus session bus.\n"
                            "To start it, run:\n"
                            "\teval `dbus-launch --auto-syntax`\n");
        return;
    }
    if (!QDBusConnection::sessionBus().registerService(SERVICE_NAME))
    {
        LOG_ERROR(LOG_TAG, "%s",
                  qPrintable(QDBusConnection::sessionBus().
                             lastError().message()));
    }
    if(!QDBusConnection::sessionBus().registerObject("/",
                                        &m_pythonInvoker,
                                        QDBusConnection::ExportScriptableSlots))
    {
        LOG_ERROR(LOG_TAG, "problem registerObject");
    }

    m_invokerInterface = new QDBusInterface("org.QtDBus.pythonInvoker",
                                            "/", "",
                                            QDBusConnection::sessionBus());

    if (m_invokerInterface->isValid())
    {
        LOG_INFO(LOG_TAG, "Invoker valid");
    }
    else
    {
        LOG_INFO(LOG_TAG, "Invoker not valid");
    }

    g_dbusInterface = m_invokerInterface;
    moveToThread(this);

    connect(&m_pythonInvoker,
            SIGNAL(movieItemAdd(QString, QStringList, QString, QString,
                                QStringList, int)),
            this,
            SLOT(onMovieItemAdd(QString, QStringList, QString, QString, 
                                QStringList, int)));
    connect(&m_pythonInvoker,
            SIGNAL(serieItemAdd(QString,QStringList,QString,QString,int)),
            this,
            SLOT(onSerieItemAdd(QString,QStringList,QString,QString,int)));
    connect(&m_pythonInvoker,
            SIGNAL(serieDirAdd(QString, QString, QString, QStringList, int)),
            this,
            SLOT(onSerieDirAdd(QString, QString, QString, QStringList, int)));
    connect(&m_pythonInvoker,
            SIGNAL(displayNotifProgressSignal(QString, int)),
            this,
            SIGNAL(displayNotifProgressSignal(QString, int)));
    connect(&m_pythonInvoker,
            SIGNAL(displayNotifSignal(QString, QString)),
            this,
            SIGNAL(displayNotifSignal(QString, QString)));
    connect(&m_pythonInvoker,
            SIGNAL(launchViewInfoReady(QStringList, QStringList, QStringList)),
            this,
            SLOT(onLaunchViewInfoReady(QStringList, QStringList, QStringList)));
    connect(&m_pythonInvoker,
            SIGNAL(playableUriReady(QString)),
            this,
            SLOT(onPlayableUriReady(QString)));
    connect(&m_pythonInvoker,
            SIGNAL(trailerListItemsReady(QStringList, QStringList)),
            this,
            SIGNAL(trailerListItemsReady(QStringList, QStringList)));
    connect(&m_pythonInvoker,
            SIGNAL(videoListItemsReady(QStringList)),
            this,
            SIGNAL(videoListItemsReady(QStringList)));
    connect(&m_pythonInvoker,
            SIGNAL(videoUrlReady(QString, int)),
            this,
            SLOT(onVideoUrlReady(QString, int)));
    connect(&m_pythonInvoker,
            SIGNAL(finishScriptExtract(int, QProcess::ExitStatus)),
            this,
            SLOT(onProcessFinishedSlot(int, QProcess::ExitStatus)));
    connect(&m_pythonInvoker,
            SIGNAL(downloadProgressValue(int)),
            this,
            SLOT(onDownloadProgressValueReceived(int)));
    connect(&m_pythonInvoker,
            SIGNAL(getCaptchaValueNotify(QString)),
            this,
            SIGNAL(getCaptchaValueNotify(QString)));
}

/**
 * @brief VM_VodEngine::onDownloadProgressValueReceived
 * @param value
 */
void VM_VodEngine::onDownloadProgressValueReceived(int value)
{
    m_cuDownloadProgressValue = value;
    Q_EMIT downloadProgressValue(value);
}

/**
 * @brief VM_VodEngine::~VM_VodEngine
 */
VM_VodEngine::~VM_VodEngine()
{
    LOG_INFO(LOG_TAG, "Delete VM_VodEngine");

    if(m_invokerInterface)
        delete m_invokerInterface;
}

/**
 * @brief VM_VodEngine::run
 */
void VM_VodEngine::run()
{
    LOG_INFO(LOG_TAG, "Run VM_VodEngine Thread...");

    m_getProgressDownloadTimer = new QTimer();
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    connect(&m_dpStream,
            SIGNAL(finished(int, QProcess::ExitStatus)),
            this,
            SLOT(onProcessFinishedSlot(int, QProcess::ExitStatus)));
    connect(m_getProgressDownloadTimer,
            SIGNAL(timeout()),
            this,
            SLOT(grabDownloadProgressValue()));
    exec();
}

/**
 * @brief VM_VodEngine::onPlayableUriReady
 * @param uri
 */
void VM_VodEngine::onPlayableUriReady(QString uri)
{
    QString localFile("file://");
    if(!uri.startsWith("http"))
    {
        LOG_INFO(LOG_TAG, "m_mediaPlayType = MEDIA_LOCAL_PLAYBACK...");
        m_mediaPlayType = MEDIA_LOCAL_PLAYBACK;
        Q_EMIT playableUriReady(localFile.append(uri));
    }
    else
    {
        m_mediaPlayType = MEDIA_HTTP_PLAYBACK;
        Q_EMIT playableUriReady(uri);
    }
}

/**
 * @brief VM_VodEngine::onVideoUrlReady
 * @param url
 * @param totalItems
 */
void VM_VodEngine::onVideoUrlReady(QString url, int totalItems)
{
    LOG_INFO(LOG_TAG, "totalItems: %d", totalItems);
    LOG_INFO(LOG_TAG, "url: %s", qPrintable(url));
}

/**
 * @brief VM_VodEngine::onLaunchViewInfoReady
 */
void VM_VodEngine::onLaunchViewInfoReady(QStringList infos_images,
                                          QStringList infos_right_box,
                                          QStringList infos_left_box)
{
    if(!m_serieTrailerContextUrl.isEmpty())
    {
        m_infos_images = infos_images;
        m_infos_right_box = infos_right_box;
        m_infos_left_box = infos_left_box;
    }
    Q_EMIT launchViewInfoReady(infos_images, infos_right_box, infos_left_box);
}

/**
 * @brief VM_VodEngine::onAddPreviousItem
 */
void VM_VodEngine::onAddPreviousItem()
{
    if(m_extractedItem != 0 || m_pageNbr < 1) return;

    LOG_INFO(LOG_TAG, "movie_title: %s", qPrintable(PREVIOUS_PAGE_STR));
    QStringList previousPageInfos;
    QStringList dummyInfoList;

    previousPageInfos.append(PREVIOUS_PAGE_STR);
    previousPageInfos.append("");
    previousPageInfos.append("");

    m_displayItemListNames.append(PREVIOUS_PAGE_STR);
    m_infosList.append(previousPageInfos);
    m_metaDataList.append(dummyInfoList);
}

/**
 * @brief VM_VodEngine::onMovieItemAdd
 * @param movie_title
 * @param movie_info
 * @param movie_url
 * @param movie_icon
 * @param base_Url
 * @param totalItems
 */
void VM_VodEngine::onMovieItemAdd(QString movie_title, QStringList movie_info, 
                                   QString movie_url, QString movie_icon,
                                   QStringList contextList, int totalItems)
{
    INV();
    m_totalItems = totalItems;
    this->onAddPreviousItem();
    LOG_INFO(LOG_TAG, "movie_title: %s", qPrintable(movie_title));
    QStringList localDisplayListInfos;
    localDisplayListInfos.append(movie_title);
    localDisplayListInfos.append(movie_url);
    localDisplayListInfos.append(movie_icon);
    localDisplayListInfos.append(contextList[0]);
    localDisplayListInfos.append(contextList[1]);
    localDisplayListInfos.append(contextList[2]);
    m_displayItemListNames.append(movie_title);
    if(m_totalItems)
    {
        m_extractedItem = m_extractedItem + 1;
        m_extractedItemPercent = (float)(m_extractedItem*100) / m_totalItems;
        Q_EMIT extractedItemLoadingValue((int)m_extractedItemPercent);
    }
    m_infosList.append(localDisplayListInfos);
    m_metaDataList.append(movie_info);
    OUTV();
}

/**
 * @brief VM_VodEngine::onSerieItemAdd
 * @param serie_title
 * @param serie_info
 * @param serie_url
 * @param serie_icon
 * @param totalItems
 */
void VM_VodEngine::onSerieItemAdd(QString serie_title, QStringList serie_info,
                                   QString serie_url, QString serie_icon,
                                   int totalItems)
{
    INV();
    m_totalItems = totalItems;
    LOG_DEBUG(LOG_TAG, "serie_title: %s", qPrintable(serie_title));
    QStringList localDisplayListInfos;
    localDisplayListInfos.append(serie_title);
    localDisplayListInfos.append(serie_url);
    localDisplayListInfos.append(serie_icon);
    localDisplayListInfos.append(m_serieTrailerContextUrl);
    localDisplayListInfos.append(m_serieInfosContextUrl);
    localDisplayListInfos.append(m_serieInfosContextFav);
    m_displayItemListNames.append(serie_title);
    if(m_totalItems)
    {
        m_extractedItem = m_extractedItem + 1;
        m_extractedItemPercent = (float)(m_extractedItem*100) / m_totalItems;
        Q_EMIT extractedItemLoadingValue((int)m_extractedItemPercent);
    }
    m_infosList.append(localDisplayListInfos);
    m_metaDataList.append(serie_info);
    OUTV();
}

/**
 * @brief VM_VodEngine::onSerieDirAdd
 * @param movie_title
 * @param movie_url
 * @param movie_icon
 * @param totalItems
 */
void VM_VodEngine::onSerieDirAdd(QString movie_title, QString movie_url,
                                  QString movie_icon, QStringList contextList,
                                  int totalItems)
{
    INV();
    if(!m_serieTrailerContextUrl.isEmpty())
    {this->clearSerieData();
    }
    m_totalItems = totalItems;
    LOG_DEBUG(LOG_TAG, "serie title: %s", qPrintable(movie_title));
    QStringList localDisplayListInfos;
    localDisplayListInfos.append(movie_title);
    localDisplayListInfos.append(movie_url);
    localDisplayListInfos.append(movie_icon);
    localDisplayListInfos.append(contextList[0]);
    localDisplayListInfos.append(contextList[1]);
    localDisplayListInfos.append(contextList[2]);
    m_displayItemListNames.append(movie_title);
    if(m_totalItems)
    {
        m_extractedItem = m_extractedItem + 1;
        m_extractedItemPercent = (float)(m_extractedItem*100) / m_totalItems;
        Q_EMIT extractedItemLoadingValue((int)m_extractedItemPercent);
    }
    m_infosList.append(localDisplayListInfos);
    OUTV();
}

/**
 * @brief VM_VodEngine::clearSerieData
 */
void VM_VodEngine::clearSerieData()
{
    m_serieTrailerContextUrl.clear();
    m_serieInfosContextUrl.clear();
    m_serieInfosContextFav.clear();
    m_infos_images.clear();
    m_infos_right_box.clear();
    m_infos_left_box.clear();
}

/**
 * @brief VM_VodEngine::onGenerateItemsNotified
 * @param url
 * @param extraArg
 */
void VM_VodEngine::onGenerateItemsNotified(QString url, 
                                            QString extraArg,
                                            bool isSerie)
{
    INV();
    CHECK_RUNNING_SCRIPT
    QMap<QString, QString> pageUrlHistoryArg;

    if(isSerie) m_procActivity = VM_VodEngine::PROC_WAIT_ITEM_SERIE_LIST;
    else m_procActivity = VM_VodEngine::PROC_WAIT_ITEM_LIST;

    if(!extraArg.isEmpty())
    {
        if (extraArg.endsWith("(VF)")) extraArg="FR";
        else if (extraArg.endsWith("(VOSTFR)")) extraArg="VostFR";
    }

    if(m_pageUrlHistoryArg.count() < m_pageNbr + 1)
    {
        pageUrlHistoryArg["url"] = url;
        pageUrlHistoryArg["extraArg"] = extraArg;
        m_pageUrlHistoryArg.append(pageUrlHistoryArg);
    }

    if(isSerie)
    {extraArg = m_serieInfosContextUrl;
    }
    LOG_INFO(LOG_TAG, "Invoked script: %s",qPrintable(url));
    LOG_INFO(LOG_TAG, "Invoked script extraArg: %s",qPrintable(extraArg));
    this->pythonInvokeProc(url, extraArg);
    OUTV();
}

/**
 * @brief VM_VodEngine::onGenerateItemInfo
 * @param url
 */
void VM_VodEngine::onGenerateItemInfo(QString url)
{
    INV();
    CHECK_RUNNING_SCRIPT
    m_procActivity = VM_VodEngine::PROC_WAIT_INFO_DETAILS;
    LOG_INFO(LOG_TAG, "Invoked script: %s",qPrintable(url));
    this->pythonInvokeProc(url, "");
    OUTV();
}

/**
 * @brief VM_VodEngine::onGenerateSerieItemInfo
 * @param url
 */
void VM_VodEngine::onGenerateSerieItemInfo(QString url)
{
    INV();
    CHECK_RUNNING_SCRIPT
    if( m_infos_images.isEmpty() && m_infos_right_box.isEmpty() && 
        m_infos_left_box.isEmpty())
    {
        m_procActivity = VM_VodEngine::PROC_WAIT_INFO_DETAILS;
        LOG_INFO(LOG_TAG, "Invoked script: %s", qPrintable(url));
        this->pythonInvokeProc(url, "");
    }
    else
    {
        LOG_INFO(LOG_TAG, "launchViewInfoReady()");
        Q_EMIT launchViewInfoReady(m_infos_images, m_infos_right_box, 
                                 m_infos_left_box);
        Q_EMIT readyItemInfo();
    }
}

/**
 * @brief VM_VodEngine::onLaunchTrailerById
 * @param index
 */
void VM_VodEngine::onLaunchTrailerById(int index)
{
    INV();
    CHECK_RUNNING_SCRIPT
    m_procActivity = VM_VodEngine::PROC_WAIT_TRAILERS_LIST;
    QString urlTrailer = m_infosList.at(index).at(3);
    LOG_INFO(LOG_TAG, "Invoked script: %s", qPrintable(urlTrailer));
    this->pythonInvokeProc(urlTrailer, "");
    OUTV();
}

/**
 * @brief VM_VodEngine::onLaunchVideoById
 * @param index
 */
void VM_VodEngine::onLaunchVideoById(int index)
{
    INV();
    CHECK_RUNNING_SCRIPT
    m_procActivity = VM_VodEngine::PROC_WAIT_VIDEO_LIST;
    QString urlVideo = m_infosList.at(index).at(1);
    LOG_INFO(LOG_TAG, "Invoked script: %s",qPrintable(urlVideo));
    this->pythonInvokeProc(urlVideo, "");
    OUTV();
}

/**
 * @brief VM_VodEngine::fetchTrailerPlayableUrl
 * @param url
 */
void VM_VodEngine::fetchTrailerPlayableUrl(QString url)
{
    INV();
    QString pythonProc;
    QString pythonProcArgs;
    m_procActivity = VM_VodEngine::PROC_WAIT_TRAILER_PLAYABLE_URL;

    pythonProc.append("python -O " +
                        QString(CfgObj->get_exec_path(
                        KvalConfigManager::KVAL_YOUTUBE_EXEC_PATH)) +
                      " 6 ");
    pythonProcArgs.append(url);

    LOG_INFO(LOG_TAG, "Invoked script: %s",
             qPrintable(pythonProc.append(pythonProcArgs)));
    m_dpStream.start(pythonProc.append(pythonProcArgs));
    OUTV();
}

/**
 * @brief VM_VodEngine::pythonInvokeProc
 * @param procStr
 */
void VM_VodEngine::pythonInvokeProc(QString arg1, QString arg2)
{
    m_dpStreamProcSt = PYTHON_PROC_RUNNING;
    if(m_procActivity == VM_VodEngine::PROC_WAIT_ITEM_LIST ||
       m_procActivity == VM_VodEngine::PROC_WAIT_ITEM_SERIE_LIST)
    {
        m_displayItemListNames.clear();
        m_infosList.clear();
        m_metaDataList.clear();
    }
    m_invokerInterface->call(QDBus::NoBlock,"invokeArgs", arg1, arg2);
}

/**
 * @brief VM_VodEngine::onProcessFinishedSlot
 * @param finished
 * @param status
 */
void VM_VodEngine::onProcessFinishedSlot(int finished,
                                          QProcess::ExitStatus status)
{
    INV("finished: %d, status: %d", finished, status);
    m_dpStreamProcSt = PYTHON_PROC_STOPPED;
    m_totalItems = 0;
    m_extractedItemPercent = 0;
    m_extractedItem = 0;

    LOG_INFO(LOG_TAG, "Python Script End Proc Activity: %d", m_procActivity);
    switch (m_procActivity)
    {
        case VM_VodEngine::PROC_WAIT_ITEM_LIST:
            if(finished) Q_EMIT pbFetchingItems();
            else Q_EMIT readyItemList(m_displayItemListNames);
            break;
        case VM_VodEngine::PROC_WAIT_ITEM_SERIE_LIST:
            if(finished) Q_EMIT pbFetchingItems();
            else Q_EMIT readyItemList(m_displayItemListNames);
            break;
        case VM_VodEngine::PROC_WAIT_INFO_DETAILS:
            Q_EMIT readyItemInfo();
            break;
        case VM_VodEngine::PROC_WAIT_TRAILERS_LIST:
            break;
        case VM_VodEngine::PROC_WAIT_TRAILER_PLAYABLE_URL:
            Q_EMIT trailerScriptEnd();
            break;
        case VM_VodEngine::PROC_WAIT_STREAM_URI:
            if(m_mediaPlayType == MEDIA_LOCAL_PLAYBACK)
            {
                LOG_INFO(LOG_TAG, "Start m_getProgressDownloadTimer...");
                m_cuDownloadProgressValue = 0;
                m_getProgressDownloadTimer->setSingleShot(true);
                m_getProgressDownloadTimer->start(10000);
            }
            break;
        case VM_VodEngine::PROC_WAIT_VIDEO_LIST:
            break;
        case VM_VodEngine::PROC_NOTIFY_DEAD_LINK:
            break;
        case VM_VodEngine::PROC_WAIT_ADD_TO_FAV:
            break;
        case VM_VodEngine::PROC_WAIT_DEL_FROM_FAV:
            Q_EMIT favItemDeleted();
            break;
        default:
            LOG_WARNING(LOG_TAG, "Unknown processed Activity: %d", 
                        m_procActivity);
            break;
    }
    OUTV();
}

/**
 * @brief VM_VodEngine::getIconById
 * @param index
 * @return
 */
QString VM_VodEngine::getIconById(int index)
{
    if(m_infosList.isEmpty()|| m_infosList.at(index).at(2).isEmpty())
    {
        return CfgObj->get_path(CfgObj->DEFAULT_VOD_MOVIE_COVER);
    }
    return m_infosList.at(index).at(2);
}

/**
 * @brief VM_VodEngine::getMetaById
 * @param index
 * @return 
 */
QStringList VM_VodEngine::getMetaById(int index)
{
    QStringList localMetaData;
    if(m_metaDataList.isEmpty() || m_metaDataList.count() < index)
    {
        return localMetaData;
    }
    return m_metaDataList.at(index);
}

/**
 * @brief VM_VodEngine::getUrlById
 * @param id
 * @return
 */
QString VM_VodEngine::getUrlById(int id)
{
    if(m_infosList.isEmpty())
    {return "''";
    }
    return m_infosList.at(id).at(1);
}

/**
 * @brief VM_VodEngine::onGetInfoById
 * @param index
 */
void VM_VodEngine::onGetInfoById(int index)
{
    Q_EMIT extractedInfoReady(getIconById(index), getMetaById(index));
}

/**
 * @brief VM_VodEngine::setPageNbr
 * @param pageNbr
 */
void VM_VodEngine::setPageNbr(int pageNbr)
{
    m_pageNbr = pageNbr;
}

/**
 * @brief VM_VodEngine::onClearPageHistory
 */
void VM_VodEngine::onClearPageHistory()
{
    INV();
    LOG_INFO(LOG_TAG, "onClearPageHistory()");
    m_pageNbr = 0;
    m_pageUrlHistoryArg.clear();
    OUTV();
}

/**
 * @brief VM_VodEngine::getItemInfoUrlByIndex
 * @param index
 * @return 
 */
QString VM_VodEngine::getItemInfoUrlByIndex(int index)
{
    return m_infosList.at(index).at(4);
}

/**
 * @brief VM_VodEngine::getPreviousHistoryUrl
 * @return
 */
QString VM_VodEngine::getPreviousHistoryUrl()
{
    return m_pageUrlHistoryArg.at(m_pageNbr).value("url");
}

/**
 * @brief VM_VodEngine::getPreviousHistoryExtraArg
 * @return
 */
QString VM_VodEngine::getPreviousHistoryExtraArg()
{
    return m_pageUrlHistoryArg.at(m_pageNbr).value("extraArg");
}

/**
 * @brief VM_VodEngine::storeContextById
 * @param index
 */
void VM_VodEngine::onStoreContextById(int index)
{
    m_serieTrailerContextUrl = m_infosList.at(index).at(3);
    m_serieInfosContextUrl = m_infosList.at(index).at(4);
    m_serieInfosContextFav = m_infosList.at(index).at(5);
    m_serieHistoryListDir = m_infosList;
}

/**
 * @brief VM_VodEngine::storeContextById
 * @param index
 */
void VM_VodEngine::onStoreSeasonById(int index)
{
    m_serieTrailerContextUrl = m_infosList.at(index).at(3);
    m_serieInfosContextUrl = m_infosList.at(index).at(4);
    m_serieInfosContextFav = m_infosList.at(index).at(5);
    m_serieHistorySeasonListDir = m_infosList;
}

/**
 * @brief VM_VodEngine::onGetSerieHistoryList
 */
void VM_VodEngine::onGetSerieHistoryList()
{
    LOG_INFO(LOG_TAG, "history called ...");
    m_displayItemListNames.clear();
    m_infosList.clear();
    m_metaDataList.clear();
    QStringList localContextList;
    for(int i = 0; i<m_serieHistoryListDir.count(); i++)
    {
        localContextList.append(m_serieHistoryListDir.at(i).at(3));
        localContextList.append(m_serieHistoryListDir.at(i).at(4));
        localContextList.append(m_serieHistoryListDir.at(i).at(5));
        this->onSerieDirAdd(m_serieHistoryListDir.at(i).at(0),
                            m_serieHistoryListDir.at(i).at(1),
                            m_serieHistoryListDir.at(i).at(2),
                            localContextList,
                            m_serieHistoryListDir.count());
        localContextList.clear();
    }
    Q_EMIT readyItemList(m_displayItemListNames);
    m_serieHistoryListDir.clear();
    m_serieHistorySeasonListDir.clear();
    m_totalItems = 0;
    m_extractedItemPercent = 0;
    m_extractedItem = 0;
}

/**
 * @brief VM_VodEngine::onGetSerieSeasonHistoryList
 */
void VM_VodEngine::onGetSerieSeasonHistoryList()
{
    LOG_INFO(LOG_TAG, "Season history called ...");
    m_displayItemListNames.clear();
    m_infosList.clear();
    m_metaDataList.clear();
    QStringList localContextList;
    for(int i = 0; i<m_serieHistorySeasonListDir.count(); i++)
    {
        localContextList.append(m_serieHistorySeasonListDir.at(i).at(3));
        localContextList.append(m_serieHistorySeasonListDir.at(i).at(4));
        localContextList.append(m_serieHistorySeasonListDir.at(i).at(5));
        this->onSerieDirAdd(m_serieHistorySeasonListDir.at(i).at(0),
                            m_serieHistorySeasonListDir.at(i).at(1),
                            m_serieHistorySeasonListDir.at(i).at(2),
                            localContextList,
                            m_serieHistorySeasonListDir.count());
        localContextList.clear();
    }
    Q_EMIT readyItemList(m_displayItemListNames);
    m_serieHistorySeasonListDir.clear();
    m_totalItems = 0;
    m_extractedItemPercent = 0;
    m_extractedItem = 0;
}

/**
 * @brief VM_VodEngine::grabDownloadProgressValue
 */
void VM_VodEngine::grabDownloadProgressValue()
{
    if( m_procActivity == PROC_WAIT_STREAM_URI &&
        m_mediaPlayType == MEDIA_LOCAL_PLAYBACK &&
        m_cuDownloadProgressValue != 100)
    {
        m_invokerInterface->call(QDBus::NoBlock,"grabDownloadProgressVal");
        m_getProgressDownloadTimer->setSingleShot(true);
        m_getProgressDownloadTimer->start(10000);
    }
}

/**
 * @brief VM_VodEngine::onReplyUrlChoice
 * @param index
 */
void VM_VodEngine::onReplyUrlChoice(int index)
{
    CHECK_RUNNING_SCRIPT
    m_procActivity = PROC_WAIT_STREAM_URI;
    m_invokerInterface->call(QDBus::NoBlock,"video_choice", index);
}

/**
 * @brief VM_VodEngine::onGetShahidPlayableUri
 */
void VM_VodEngine::onGetShahidPlayableUri(int index)
{
    CHECK_RUNNING_SCRIPT
    m_procActivity = PROC_WAIT_VIDEO_LIST;
    this->pythonInvokeProc(m_infosList.at(index).at(1), "");
}

/**
 * @brief VM_VodEngine::stopCurrentMedia
 */
void VM_VodEngine::stopCurrentMedia()
{
    CHECK_RUNNING_SCRIPT
    if( m_procActivity == PROC_WAIT_STREAM_URI &&
        m_mediaPlayType == MEDIA_LOCAL_PLAYBACK)
    {
        LOG_INFO(LOG_TAG, "Stop Download and remove current media...");
        m_invokerInterface->call(QDBus::NoBlock,"video_stop_remove_downloads");
    }
    m_procActivity = PROC_UNKNOWN;
    m_mediaPlayType = MEDIA_UNKNOWN;
}

/**
 * @brief VM_VodEngine::onAddMediaToFav
 * @param index
 */
void VM_VodEngine::onAddMediaToFav(int index)
{
    INV("index: %d", index);
    CHECK_RUNNING_SCRIPT
    m_procActivity = PROC_WAIT_ADD_TO_FAV;
    LOG_INFO(LOG_TAG, "m_infosList.at(index).at(5): ", 
             qPrintable(m_infosList.at(index).at(5)));
    this->pythonInvokeProc(m_infosList.at(index).at(5), "");
    OUTV();
}

/**
 * @brief VM_VodEngine::onDeleteMediaFromFav
 * @param index
 */
void VM_VodEngine::onDeleteMediaFromFav(int index)
{
    INV("index: %d", index);
    CHECK_RUNNING_SCRIPT
    m_procActivity = PROC_WAIT_DEL_FROM_FAV;
    QString deleteReqArg(m_infosList.at(index).at(5));
    //patch request arg mode
    deleteReqArg.replace(QString("mode=22"), QString("mode=24"));
    LOG_INFO(LOG_TAG, "deleteReqArg: ", qPrintable(deleteReqArg));
    this->pythonInvokeProc(deleteReqArg, "");
    OUTV();
}

/**
 * @brief VM_VodEngine::onClearVodCache
 * @param index
 */
void VM_VodEngine::onClearVodCache()
{
    QProcess clearVodCache;
    Q_EMIT displayNotifProgressSignal("Vider le Cache ...", 50);
    QString cmd = "python -O " +
            QString(CfgObj->get_exec_path(
            KvalConfigManager::KVAL_VOD_EXEC_CLEAR_CACHE));
    LOG_INFO(LOG_TAG, ">>>>>>>>>>>>>>>>>>>>>>>>> cmd: %s",qPrintable(cmd));

    clearVodCache.start(cmd);

    Q_EMIT displayNotifSignal("Info", tr("Cache Vidé"));
    clearVodCache.waitForFinished();
}

/**
 * @brief VM_VodEngine::onClearVodDownloads
 * @param index
 */
void VM_VodEngine::onClearVodDownloads()
{
    QProcess clearVodDownloads;
    Q_EMIT displayNotifProgressSignal("Supprimé les téléchargements ...", 50);
    clearVodDownloads.start("python -O " +
                        QString(CfgObj->get_exec_path(
                        KvalConfigManager::KVAL_VOD_EXEC_CLEAR_DOWNLOADS)));
    Q_EMIT displayNotifSignal("Info", tr("Téléchargements Supprimé"));
    clearVodDownloads.waitForFinished();
}

/**
 * @brief VM_VodEngine::onSetVodSettings
 * @param cacheValue
 * @param bufferingValue
 */
void VM_VodEngine::onSetVodSettings(int bufferingValue,
                                     int cacheValue,
                                     bool streamVod)
{
    LOG_INFO(LOG_TAG,
             "cacheValue: %d, bufferingValue: %d, streamVod: %u",
             cacheValue, bufferingValue, streamVod);
    m_invokerInterface->call(QDBus::Block,"setVodPySettings", 
                             bufferingValue, 
                             cacheValue,
                             streamVod);
    LOG_INFO(LOG_TAG,"out");
}

/**
 * @brief VM_VodEngine::onAbortMovieLoading
 */
void VM_VodEngine::onAbortMovieLoading()
{
    m_invokerInterface->call(QDBus::NoBlock,"abortMovieLoading");
}

/**
 * @brief VM_VodEngine::onVodServerInfosNotify
 * @param srvAddr
 * @param boxName
 */
void VM_VodEngine::onVodServerInfosNotify(QString srvAddr, QString boxName)
{
    INV("VOD server Address: %s, boxName: %s",
        qPrintable(srvAddr), qPrintable(boxName));
    m_invokerInterface->call(QDBus::NoBlock,
                            "set_vod_server_address",
                             srvAddr, boxName);
    OUTV();
}

/**
 * @brief VM_VodElement::abortMovieLoading
 */
void VM_VodElement::abortMovieLoading()
{
    m_vodEngine->onAbortMovieLoading();
}

/**
 * @brief VM_VodElement::setVodServerAddress
 * @param srvAddr
 * @param boxName
 */
void VM_VodElement::setVodServerAddress(QString srvAddr,
                                                  QString boxName)
{
    INV("VOD server Address: %s, boxName: %s",
        qPrintable(srvAddr), qPrintable(boxName));
    QMetaObject::invokeMethod(m_vodEngine,
                              "onVodServerInfosNotify",
                              Q_ARG(QString, srvAddr),
                              Q_ARG(QString, boxName));
    OUTV();
}

/**
 * @brief VM_VodElement::getUri
 * @param index
 * @return
 */
QString VM_VodElement::getUri(int index)
{
    return m_vodEngine->getUrlById(index);
}

/**
 * @brief VM_VodElement::clearVodCache
 */
void VM_VodElement::clearVodCache()
{
    QMetaObject::invokeMethod(m_vodEngine, "onClearVodCache");
}

/**
 * @brief VM_VodElement::clearVodDownloads
 */
void VM_VodElement::clearVodDownloads()
{
    QMetaObject::invokeMethod(m_vodEngine, "onClearVodDownloads");
}

/**
 * @brief VM_VodElement::setVodSettings
 * @param cacheDuration
 * @param bufferingVal
 */
void VM_VodElement::setVodSettings(int bufferingVal,
                                          int cacheDuration,
                                          bool streamVod)
{
    m_vodEngine->onSetVodSettings(bufferingVal, cacheDuration, streamVod);
}

/**
 * @brief VM_VodElement::deleteMediaFromFav
 * @param index
 */
void VM_VodElement::deleteMediaFromFav(int index)
{
    INV("index: %d", index);
    m_vodEngine->onDeleteMediaFromFav(index);
    OUTV();
}

/**
 * @brief VM_VodElement::addMediaToFav
 * @param index
 */
void VM_VodElement::addMediaToFav(int index)
{
    INV("index: %d", index);
    m_vodEngine->onAddMediaToFav(index);
    OUTV();
}

/**
 * @brief VM_VodElement::stopNotify
 */
void VM_VodElement::stopNotify()
{
    m_vodEngine->stopCurrentMedia();
}

/**
 * @brief VM_VodElement::generateVideoUrl
 * @param index
 */
void VM_VodElement::generateVideoUrl(int index)
{
    m_vodEngine->onReplyUrlChoice(index);
}

/**
 * @brief VM_VodElement::getShahidPlayableUri
 * @param uri
 */
void VM_VodElement::getShahidPlayableUri(int index)
{
    m_vodEngine->onGetShahidPlayableUri(index);
}

/**
 * @brief VM_VodElement::generateSerieItems
 * @param index
 * @param pageNbr
 */
void VM_VodElement::generateSerieItems(int index, bool useContextList)
{
    if(useContextList)
    {m_vodEngine->onStoreContextById(index);
    }
    m_vodEngine->onGenerateItemsNotified(m_vodEngine->getUrlById(index),"",true);
}

/**
 * @brief VM_VodElement::generateSeasonItems
 * @param index
 * @param useContextList
 */
void VM_VodElement::generateSeasonItems(int index, QString seasonNumber,
                                               bool useContextList)
{
    if(useContextList)
    {m_vodEngine->onStoreSeasonById(index);
    }
    QString urlToSend(m_vodEngine->getUrlById(index));
    urlToSend.append("&season=");
    urlToSend.append(seasonNumber);
    m_vodEngine->onGenerateItemsNotified(urlToSend, "", true);
}

/**
 * @brief VM_VodElement::getSerieHistoryList
 */
void VM_VodElement::getSerieHistoryList(bool isSeason)
{
    if(isSeason)
    {m_vodEngine->onGetSerieSeasonHistoryList();
    }
    else
    {m_vodEngine->onGetSerieHistoryList();
    }
}

/**
 * @brief VM_VodElement::clearPageHistory
 */
void VM_VodElement::clearPageHistory()
{
    INV();
    m_vodEngine->onClearPageHistory();
    OUTV();
}
/**
 * @brief VM_VodElement::launchTrailer
 * @param index
 */
void VM_VodElement::launchTrailer(QString url)
{
    LOG_INFO(LOG_TAG, "url %s",qPrintable(url));
    m_vodEngine->fetchTrailerPlayableUrl(url);
}

/**
 * @brief VM_VodElement::launchTrailer
 * @param index
 */
void VM_VodElement::generateTrailerList(int index)
{
    m_vodEngine->onLaunchTrailerById(index);
}

/**
 * @brief VM_VodElement::generateVideoList
 * @param index
 */
void VM_VodElement::generateVideoList(int index)
{
    m_vodEngine->onLaunchVideoById(index);
}

/**
 * @brief VM_VodElement::generateItemInfo
 * @param index
 */
void VM_VodElement::generateItemInfo(int index, bool isSerie)
{
    if(isSerie)
    {
        m_vodEngine->onGenerateSerieItemInfo(m_vodEngine->getItemInfoUrlByIndex
                                             (index));
    }
    else
    {
        m_vodEngine->onGenerateItemInfo(m_vodEngine->getItemInfoUrlByIndex
                                        (index));
    }
}

/**
 * @brief VM_VodElement::getItemInfo
 * @param index
 * @return
 */
void VM_VodElement::getItemInfo(int index)
{
    Q_EMIT getInfoById(index);
}

/**
 * @brief VM_VodElement::generateItems
 * @param url
 * @param extraArg
 */
void VM_VodElement::generateItems(QString url, QString extraArg)
{
    m_vodEngine->setPageNbr(0);
    m_vodEngine->onGenerateItemsNotified(url, extraArg, false);
}

/**
 * @brief VM_VodElement::generateItemPage
 * @param index
 * @param pageNbr
 */
void VM_VodElement::generateItemPage(int index, int pageNbr)
{
    m_vodEngine->setPageNbr(pageNbr);
    if(index == 0)
    {
        LOG_INFO(LOG_TAG, "Previous Page called");
        m_vodEngine->onGenerateItemsNotified(m_vodEngine->getPreviousHistoryUrl(),
                                             m_vodEngine->getPreviousHistoryExtraArg(),
                                             false);
        return;
    }
    m_vodEngine->onGenerateItemsNotified(m_vodEngine->getUrlById(index), "", false);
}

/**
 * @brief VM_VodElement::VM_VodElement
 */
VM_VodElement::VM_VodElement()
{
    LOG_INFO(LOG_TAG, "Instantiate VM_VodElement");
    m_vodEngine = new VM_VodEngine();
    m_vodEngine->start();
    connect(m_vodEngine,
            SIGNAL(readyItemList(QList<QString>)),
            this,
            SIGNAL(readyItemList(QList<QString>)));
    connect(this,
            SIGNAL(getInfoById(int)),
            m_vodEngine,
            SLOT(onGetInfoById(int)));
    connect(m_vodEngine,
            SIGNAL(extractedItemLoadingValue(int)),
            this,
            SIGNAL(extractedItemLoadingValue(int)));
    connect(m_vodEngine,
            SIGNAL(extractedInfoReady(QString, QStringList)),
            this,
            SIGNAL(extractedInfoReady(QString, QStringList)));
    connect(m_vodEngine,
            SIGNAL(pbFetchingItems()),
            this,
            SIGNAL(pbFetchingItems()));
    connect(m_vodEngine,
            SIGNAL(displayNotifProgressSignal(QString, int)),
            this,
            SIGNAL(displayNotifProgressSignal(QString, int)));
    connect(m_vodEngine,
            SIGNAL(readyItemInfo()),
            this,
            SIGNAL(readyItemInfo()));
    connect(m_vodEngine,
            SIGNAL(launchViewInfoReady(QStringList, QStringList, QStringList)),
            this,
            SIGNAL(launchViewInfoReady(QStringList, QStringList, QStringList)));
    connect(m_vodEngine,
            SIGNAL(playableUriReady(QString)),
            this,
            SIGNAL(playableUriReady(QString)));
    connect(m_vodEngine,
            SIGNAL(trailerListItemsReady(QStringList, QStringList)),
            this,
            SIGNAL(trailerListItemsReady(QStringList, QStringList)));
    connect(m_vodEngine,
            SIGNAL(displayNotifSignal(QString, QString)),
            this,
            SIGNAL(displayNotifSignal(QString, QString)));
    connect(m_vodEngine,
            SIGNAL(videoListItemsReady(QStringList)),
            this,
            SIGNAL(videoListItemsReady(QStringList)));
    connect(m_vodEngine,
            SIGNAL(downloadProgressValue(int)),
            this,
            SIGNAL(downloadProgressValue(int)));
    connect(m_vodEngine,
            SIGNAL(favItemDeleted()),
            this,
            SIGNAL(favItemDeleted()));
    connect(m_vodEngine,
            SIGNAL(getCaptchaValueNotify(QString)),
            this,
            SIGNAL(getCaptchaValueNotify(QString)));
    connect(m_vodEngine,
            SIGNAL(trailerScriptEnd()),
            this,
            SIGNAL(trailerScriptEnd()));
}

/**
 * @brief VM_VodElement::~VM_VodElement
 */
VM_VodElement::~VM_VodElement()
{
    LOG_INFO(LOG_TAG, "Delete VM_VodElement");
    if(m_vodEngine)
    {
        m_vodEngine->quit();
        m_vodEngine->wait();
        delete m_vodEngine;
    }
}
