#define LOG_ACTIVATED
#define LOG_SRC KVAL_LIVESTREAM_MANAGER
#include "KvalLogging.h"
#include "KvalLiveStreamManager.h"
#include "KvalHttpStreamManager.h"
#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#include "KvalRtmpStreamManager.h"
#endif

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------
#define CHECK_LIVE_STREAM_SESSION \
    if(   !m_livePlayBackSession  \
       || !m_livePlayBackSession->liveStreamObj) \
        return; \

#define CHECK_RET_LIVE_STREAM_SESSION(x) \
    if(   !m_livePlayBackSession \
       || !m_livePlayBackSession->liveStreamObj) \
        return x; \

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Global var
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Public Classes
//----------------------------------------------------------------------------

/**
 * @brief KvalLiveStreamManager::KvalLiveStreamManager
 * @param parent
 */
KvalLiveStreamManager::KvalLiveStreamManager(QObject *parent) :
    QObject(parent),
    m_liveStreamEngines{
    {KVAL_LIVESTREAM_HTTP, nullptr},
    {KVAL_LIVESTREAM_RTMP, nullptr},
    {KVAL_LIVESTREAM_FTP, nullptr},
    {KVAL_LIVESTREAM_P2P, nullptr}}
{
    registerLiveStreamEngines();
    m_uriScheme = {
        {{"http", "https"}, KVAL_LIVESTREAM_HTTP},
        {{"ftp", "tftp"}, KVAL_LIVESTREAM_FTP},
        {{"rtmp", "rtmpe"}, KVAL_LIVESTREAM_RTMP},
        {{"p2p", "tracker", "sop", "acestream"}, KVAL_LIVESTREAM_P2P},
    };

    m_livePlayBackSession.reset(nullptr);
}

/**
 * @brief KvalLiveStreamManager::registerLiveStreamEngines
 * @return
 */
bool KvalLiveStreamManager::registerLiveStreamEngines()
{
    LOG_INFO(LOG_TAG, "In");

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
    QSharedPointer<KvalLiveStreamAbstract> rtmpStreamEngine{new KvalRtmpStreamManager};
    KvalThread *rtmpStreamManagerThread{new KvalThread("KvalRtmpStreamManager")};
    SharedStreamEngineConf rtmpStreamConf(new KvalLiveStreamEngineConf(
                    rtmpStreamEngine,
                    rtmpStreamManagerThread));
    m_liveStreamEngines.insert(KVAL_LIVESTREAM_RTMP, rtmpStreamConf);
#endif

    QSharedPointer<KvalLiveStreamAbstract> httpStreamEngine{new KvalHttpStreamManager};
    KvalThread *httpStreamManagerThread{new KvalThread("KvalHttpStreamManager")};
    SharedStreamEngineConf httpStreamConf(new KvalLiveStreamEngineConf(
                    httpStreamEngine,
                    httpStreamManagerThread));
    m_liveStreamEngines.insert(KVAL_LIVESTREAM_HTTP, httpStreamConf);

    std::function<void(SharedStreamEngineConf&)> startEngine =
            [this](SharedStreamEngineConf& sharedEngineConf)
    {
        if(sharedEngineConf.isNull()){
            return;
        }
        KvalLiveStreamEngineConf *engineConf = sharedEngineConf.get();
        engineConf->engine->moveToThread(engineConf->thread);
        connect(engineConf->engine.get(), SIGNAL(playbackFailed()),
                this, SIGNAL(playbackFailed()));
        connect(engineConf->engine.get(), SIGNAL(unsupportedFormat()),
                this, SLOT(onUnsupportedFormat()));
        connect(engineConf->engine.get(), SIGNAL(streamReady(const QString&)),
                this, SLOT(onLiveStreamReady(const QString&)));
        connect(engineConf->engine.get(), SIGNAL(okDiag(QString, QString)),
                this, SIGNAL(okDiag(QString,QString)));
        connect(engineConf->engine.get(), SIGNAL(yesNoDiag(QString,QString,QString,QString)),
                this, SIGNAL(yesNoDiag(QString,QString,QString,QString)));
        connect(engineConf->engine.get(), SIGNAL(yesNoDiagUpdate(QString,QString)),
                this, SIGNAL(yesNoDiagUpdate(QString,QString)));
        connect(engineConf->engine.get(), SIGNAL(yesNoDiagClose()),
                this, SIGNAL(yesNoDiagClose()));
        connect(engineConf->engine.get(), SIGNAL(httpSeeked(int)),
                this, SIGNAL(httpSeeked(int)));
        connect(engineConf->engine.get(), SIGNAL(flushPlayer()),
                this, SIGNAL(flushPlayer()));
        engineConf->thread->start();
    };

    for_each(begin(m_liveStreamEngines), end(m_liveStreamEngines), startEngine);

    return true;
}

/**
 * @brief KvalLiveStreamManager::~KvalLiveStreamManager
 */
KvalLiveStreamManager::~KvalLiveStreamManager()
{
    LOG_INFO(LOG_TAG, "~KvalLiveStreamManager");
    m_livePlayBackSession.reset(nullptr);
    m_liveStreamEngines.clear();
}

/**
 * @brief QE_QmlBinder::pauseLiveStream
 * @return
 */
bool KvalLiveStreamManager::pauseLiveStream()
{
    CHECK_RET_LIVE_STREAM_SESSION(false)
    bool retVal{false};
    QMetaObject::invokeMethod(m_livePlayBackSession->liveStreamObj->engine.get(),
                              "onPauseStream",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, retVal));
    return retVal;
}

/**
 * @brief QE_QmlBinder::seekCurrentStream
 * @param value
 * @param totalTime
 * @param currentTime
 * @return
 */
bool KvalLiveStreamManager::seekCurrentStream(int value,
                                              int totalTime,
                                              int currentTime)
{
    CHECK_RET_LIVE_STREAM_SESSION(false)
    bool retVal{false};
    QMetaObject::invokeMethod(m_livePlayBackSession->liveStreamObj->engine.get(),
                              "onSeekStream",
                              Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, retVal),
                              Q_ARG(int, value),
                              Q_ARG(int, totalTime),
                              Q_ARG(int, currentTime));
    return retVal;
}

/**
 * @brief KvalLiveStreamManager::resumeLiveStream
 */
void KvalLiveStreamManager::resumeLiveStream()
{
    CHECK_LIVE_STREAM_SESSION
    QMetaObject::invokeMethod(m_livePlayBackSession->liveStreamObj->engine.get(),
                              "onResumeStream",
                              Qt::QueuedConnection);
}

/**
 * @brief KvalLiveStreamManager::getTrickWritePercent
 * @return
 */
qreal KvalLiveStreamManager::getTrickWritePercent()
{
    CHECK_RET_LIVE_STREAM_SESSION(0.0)
    return m_livePlayBackSession->liveStreamObj->engine->getTrickWritePercent();
}

/**
 * @brief KvalLiveStreamManager::stopLiveStream
 */
void KvalLiveStreamManager::stopLiveStream() const
{
    CHECK_LIVE_STREAM_SESSION
    QMetaObject::invokeMethod(m_livePlayBackSession->liveStreamObj->engine.get(),
                              "onStopStream",
                              Qt::QueuedConnection);
}

/**
 * @brief QE_QmlBinder::onUnsupportedFormat
 */
void KvalLiveStreamManager::onUnsupportedFormat()
{
    Q_EMIT startStream(m_livePlayBackSession->originalStreamUri);
    m_livePlayBackSession.reset(nullptr);
}

/**
 * @brief KvalLiveStreamManager::playLiveStream
 * @param channelUrl
 * @return
 */
bool KvalLiveStreamManager::playLiveStream(const QString& liveStreamUri)
{
    LOG_INFO(LOG_TAG, "playLiveStream %s", qPrintable(liveStreamUri));

    m_livePlayBackSession.reset(new KvalPlayBackLiveSession);

    QUrl streamUri(liveStreamUri, QUrl::TolerantMode);
    if(!streamUri.isValid()){
        LOG_ERROR(LOG_TAG,
                  "Not a valid stream uri: %s",
                  qPrintable(liveStreamUri));
        return false;
    }

    QString uriScheme = streamUri.scheme();
    Q_FOREACH(const QStringList schemeList, m_uriScheme.keys()){
        if (schemeList.contains(uriScheme)){
            SupportedFormat _format = m_uriScheme.value(schemeList);
            SharedStreamEngineConf liveStreamEngine = m_liveStreamEngines[_format];
            if(liveStreamEngine.isNull()){
                LOG_ERROR(LOG_TAG, "Stream obj not available!");
                return false;
            }
            QMetaObject::invokeMethod(liveStreamEngine->engine.get(),
                                      "onPlayStream",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, streamUri.toString()));
            m_livePlayBackSession->liveStreamObj = liveStreamEngine;
            m_livePlayBackSession->originalStreamUri = liveStreamUri;
            m_livePlayBackSession->parsedStreamUri = streamUri;
            m_livePlayBackSession->streamFormat = _format;
            return true;
        }
    }

    LOG_ERROR(LOG_TAG,
              "Unable to parse stream url: %s",
              qPrintable(liveStreamUri));
    return false;
}

/**
 * @brief KvalLiveStreamManager::onLiveStreamReady
 * @param pipeName
 */
void KvalLiveStreamManager::onLiveStreamReady(const QString &pipeName)
{
    LOG_INFO(LOG_TAG, "onLiveStreamReady pipeName = %s", qPrintable(pipeName));
    Q_EMIT startStream(pipeName);
}
