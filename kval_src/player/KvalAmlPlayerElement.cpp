#define LOG_ACTIVATED
#include <memory>
#include <QQueue>
#include <QObject>
#include <QThread>

#include "KvalAmlPlayerElement.h"


#define LOG_SRC OMXMEDIAELEMENTSTR
#include "KvalLogging.h"

using namespace std;
#define CHECK_MEDIA_PROCESSOR \
    if (!m_player) { \
       LOG_INFO(LOG_TAG, "No active media player."); \
       return; \
}

//This is used by PH_MediaPlayerWrapper class,
//As it is created only once for the process lifetime
KvalMediaPlayerEngine * g_mediaPlayerEngine = nullptr;

/**
 * @brief PE_MediaPlayerEngine::PE_MediaPlayerEngine
 */
KvalMediaPlayerEngine::KvalMediaPlayerEngine():
    m_seekStart(0),
    m_player(NULL),
    m_interruptAutoRestart(false)
{
    LOG_INFO(LOG_TAG, "Instantiate PE_MediaPlayerEngine...");
    m_thread = new KvalThread("PE_MediaPlayerEngine");
    m_thread->setObjectName("MPlayerEngine");

    // Move to a new thread.
    moveToThread(m_thread);
    connect(this, SIGNAL (finished()), m_thread, SLOT(quit()));
    connect(this, SIGNAL (finished()), this, SLOT(deleteLater()));
    connect(m_thread, SIGNAL (finished()), m_thread, SLOT (deleteLater()));

    m_thread->start();
}

/**
 * @brief PE_MediaPlayerEngine::
 * ~PE_MediaPlayerEngine
 */
KvalMediaPlayerEngine::~KvalMediaPlayerEngine()
{
    LOG_INFO(LOG_TAG, "Delete PE_MediaPlayerEngine...");
    if(m_player)
    {
        delete m_player;
    }
}

/**
 * @brief PE_MediaPlayerEngine::cleanup
 */
void KvalMediaPlayerEngine::cleanup()
{
    if(m_player)
    {
        LOG_INFO(LOG_TAG, "Stop MediaProc...");
        m_player->stop();
        LOG_INFO(LOG_TAG, "MediaProc successfully stopped...");
    }

    Q_EMIT finished();
}

/**
 * @brief PE_MediaPlayerEngine::handleFileSource
 * @return 
 */
QString KvalMediaPlayerEngine::handleFileSource()
{
    return m_handleFileSource;
}

/**
 * @brief PE_MediaPlayerEngine::onSetNewSource
 * @param newSource
 */
void KvalMediaPlayerEngine::onSetNewSource(QString source,
                                       QString source2,
                                       bool isDualStream,
                                       bool isLoop)
{
    if(isActivePlayerState()){
        LOG_INFO(LOG_TAG, "Player busy....");
        Q_EMIT mediaPlayerBusy();
        return;
    }
    LOG_INFO(LOG_TAG, "new media srouces[%s, %s]",
             qPrintable(source),
             qPrintable(source2));
    handleFileOpenMedia(source, source2, isDualStream, isLoop);
}

/**
 * @brief PE_MediaPlayerEngine::
 * createPP_MediaPlayer
 */
void KvalMediaPlayerEngine::createPP_MediaPlayer(bool isDualStream)
{
    LOG_INFO(LOG_TAG, "%s", Q_FUNC_INFO);

    m_player = new PP_MediaPlayer(isDualStream);
    LOG_INFO(LOG_TAG, "New mediaProcThread %p", m_player);
    connect(m_player,
            SIGNAL(playbackStarted()), 
            this,
            SIGNAL(playbackStarted()));
    connect(m_player,
            SIGNAL(playbackFailed()),
            this,
            SLOT(onPlaybackFailed()));
    connect(m_player,
            SIGNAL(playbackCompleted()),
            this,
            SLOT(onPlaybackCompleted()));
    connect(m_player,
            SIGNAL(playbackInterrupted(long)),
            this,
            SLOT(onPlaybackInterrupted(long)));
    connect(m_player,
            SIGNAL(videoSeeked()),
            this, 
            SIGNAL(videoSeeked()));
    connect(m_player,
            SIGNAL(showSubs(QString)),
            this,
            SIGNAL(showSubs(QString)));
    connect(m_player,
            SIGNAL(hideSubs()),
            this,
            SIGNAL(hideSubs()));
}

/**
 * @brief PE_MediaPlayerEngine::onPlaybackInterrupted
 * @param pos
 */
void KvalMediaPlayerEngine::onPlaybackInterrupted(long pos)
{
    Q_EMIT playbackInterrupted(pos);
    LOG_INFO(LOG_TAG, "onPlaybackInterrupted ...");
    QMutexLocker locker(&m_mediaProcMtx);
    m_player = NULL;
}

void KvalMediaPlayerEngine::onPlaybackFailed()
{
    Q_EMIT playbackFailed();
    LOG_INFO(LOG_TAG, "onPlaybackFailed ...");
    QMutexLocker locker(&m_mediaProcMtx);
    m_player = NULL;
}

/**
 * @brief PE_MediaPlayerEngine::playbackCompleted
 * @param pos
 */
void KvalMediaPlayerEngine::onPlaybackCompleted()
{
    Q_EMIT playbackCompleted();
    LOG_INFO(LOG_TAG, "onPlaybackCompleted ...");
    QMutexLocker locker(&m_mediaProcMtx);
    m_player = NULL;
}

/**
 * @brief PE_MediaPlayerEngine::handleFileOpenMedia
 * @param filepath
 * @return 
 */
bool KvalMediaPlayerEngine::handleFileOpenMedia(QString filepath,
                                            QString source2,
                                            bool isDualStream,
                                            bool isLoop)
{
    createPP_MediaPlayer(isDualStream);
    if(m_interruptAutoRestart)
        m_player->enableAutoRestart();
    m_player->seekStartPlay(m_seekStart);
    //Reinit seek start to avoid a false activation on the next start
    m_seekStart = 0;
    m_interruptAutoRestart = false;
    if (!m_player->setFilename(filepath, source2, isLoop))
    {
        LOG_INFO(LOG_TAG, "send playbackFailed ...");
        Q_EMIT playbackFailed();
        QMutexLocker locker(&m_mediaProcMtx);
        m_player = NULL;
        return false;
    }
    m_player->play();
    return true;
}

/**
 * @brief PE_MediaPlayerEngine::onPlayCurrentMedia
 */
void KvalMediaPlayerEngine::onPlayCurrentMedia()
{
    CHECK_MEDIA_PROCESSOR
    LOG_INFO(LOG_TAG, "onPlayCurrentMedia");
    m_player->play();
}

/**
 * @brief PE_MediaPlayerEngine::onPauseCurrentMedia
 */
void KvalMediaPlayerEngine::onPauseCurrentMedia()
{
    CHECK_MEDIA_PROCESSOR
    LOG_INFO(LOG_TAG, "onPauseCurrentMedia");
    m_player->pause();
}

/**
 * @brief PE_MediaPlayerEngine::handleFileSource
 * @return 
 */
void KvalMediaPlayerEngine::onGetStreamPosition()
{
    CHECK_MEDIA_PROCESSOR
    Q_EMIT streamPositionReady((unsigned)m_player->streamPosition(),
                             (unsigned)m_player->streamLength());
}

/**
 * @brief PE_MediaPlayerEngine::streamPosition
 * @return
 */
qint64 KvalMediaPlayerEngine::streamPosition()
{
    QMutexLocker locker(&m_mediaProcMtx);
    if (!m_player)
    {
        return -1;
    }

    return m_player->streamPosition();
}

/**
 * @brief PE_MediaPlayerEngine::streamLength
 * @return
 */
qint64 KvalMediaPlayerEngine::streamLength()
{
    QMutexLocker locker(&m_mediaProcMtx);
    if (!m_player)
    {
        return -1;
    }

    return m_player->streamLength();
}

/**
 * @brief PE_MediaPlayerEngine::onSeekCurrentMedia
 */
void KvalMediaPlayerEngine::onSeekCurrentMedia(long millis, bool isFlushInternal)
{
    CHECK_MEDIA_PROCESSOR
    LOG_INFO(LOG_TAG, "onSeekCurrentMedia");
    m_player->seek(millis, false, isFlushInternal);
}

/**
 * @brief PE_MediaPlayerEngine::onJumpCurrentMedia
 * @param millis
 */
void KvalMediaPlayerEngine::onJumpCurrentMedia(long millis)
{
    CHECK_MEDIA_PROCESSOR
    LOG_INFO(LOG_TAG, "onJumpCurrentMedia");
    m_player->jump(millis);
}

/**
 * @brief PE_MediaPlayerEngine::onSwitchAudio
 */
void KvalMediaPlayerEngine::onSwitchAudio(int index)
{
    CHECK_MEDIA_PROCESSOR
    LOG_INFO(LOG_TAG, "onSwitchAudio");
    m_player->switchAudio(index);
}

/**
 * @brief PE_MediaPlayerEngine::onSwitchSubs
 * @param index
 */
void KvalMediaPlayerEngine::onSwitchSubs(int index)
{
    CHECK_MEDIA_PROCESSOR
    LOG_INFO(LOG_TAG, "onSwitchSubs");
    m_player->switchSubs(index);
}

/**
 * @brief PE_MediaPlayerEngine::onStopCurrentMedia
 */
void KvalMediaPlayerEngine::onStopCurrentMedia()
{
    LOG_INFO(LOG_TAG, "onStopCurrentMedia");
    QMutexLocker locker(&m_mediaProcMtx);
    CHECK_MEDIA_PROCESSOR
    Q_EMIT uiStopRequested();
    m_player->stop();
    m_player = NULL;
}

/**
 * @brief PE_MediaPlayerEngine::setVolume
 * @param volume
 */
void KvalMediaPlayerEngine::setVolume(long volume)
{
    QMutexLocker locker(&m_mediaProcMtx);
    CHECK_MEDIA_PROCESSOR
    m_player->setVolume(volume);
}

/**
 * @brief PE_MediaPlayerEngine::volume
 * @param linear
 * @return 
 */
long KvalMediaPlayerEngine::volume()
{
    if (!m_player)
    {
       LOG_INFO(LOG_TAG, "No active media player.");
       return -1; 
    }
    return m_player->volume();
}

/**
 * @brief PE_MediaPlayerEngine::videoBitRate
 * @return
 */
int KvalMediaPlayerEngine::videoBitRate()
{
    if (!m_player)
    {
       LOG_INFO(LOG_TAG, "No active media player.");
       return -1;
    }
    return m_player->videoBitRate();
}

/**
 * @brief PE_MediaPlayerEngine::extractStreamInfos
 */
QVariantMap KvalMediaPlayerEngine::extractStreamInfos()
{
    if (!m_player)
    {
        QVariantMap dummy;
        LOG_INFO(LOG_TAG, "No active media player.");
        return dummy;
    }
    return m_player->extractStreamInfos();
}

/**
 * @brief PE_MediaPlayerEngine::setMute
 * @param muted
 */
void KvalMediaPlayerEngine::setMute(bool muted)
{
    CHECK_MEDIA_PROCESSOR
    m_player->setMute(muted);
}

/**
 * @brief PE_MediaPlayerEngine::muted
 * @return 
 */
bool KvalMediaPlayerEngine::muted()
{
    if (!m_player)
    {
       LOG_INFO(LOG_TAG, "No active media player.");
       return false; 
    }
    return m_player->muted();
}

/**
 * @brief PE_MediaPlayerEngine::enableInterruptAutoRestart
 */
void KvalMediaPlayerEngine::enableInterruptAutoRestart()
{
    m_interruptAutoRestart = true;
}

/**
 * @brief PE_MediaPlayerEngine::setAspectRatio
 * @param value
 */
void KvalMediaPlayerEngine::setAspectRatio(int value)
{
    if (!m_player)
    {
       LOG_INFO(LOG_TAG, "No active media player.");
       return;
    }
    m_player->setAspectRatio((AspectRatio)value);
}

/**
 * @brief PE_MediaPlayerElement::PE_MediaPlayerElement
 * @param parent
 */
KvalMediaPlayerElement::KvalMediaPlayerElement(QQuickItem *parent) :
    QQuickItem(parent),
    m_pendingOpen(false)
{
    LOG_INFO(LOG_TAG, "Instantiate PE_MediaPlayerEngine...");

    m_mPlayerEngine = new KvalMediaPlayerEngine();
    g_mediaPlayerEngine = m_mPlayerEngine;

    //Connections
    connect(m_mPlayerEngine,
            SIGNAL(playbackStarted()),
            this,
            SIGNAL(playbackStarted()));
    connect(m_mPlayerEngine,
            SIGNAL(playbackFailed()),
            this, 
            SIGNAL(playbackFailed()));
    connect(m_mPlayerEngine,
            SIGNAL(playbackCompleted()),
            this,
            SIGNAL(playbackCompleted()));
    connect(m_mPlayerEngine,
            SIGNAL(playbackInterrupted(long)),
            this,
            SIGNAL(playbackInterrupted(long)));
    connect(this,
            SIGNAL(stopCurrentMedia()),
            m_mPlayerEngine,
            SLOT(onStopCurrentMedia()));
    connect(this,
            SIGNAL(playCurrentMedia()),
            m_mPlayerEngine,
            SLOT(onPlayCurrentMedia()));
    connect(this,
            SIGNAL(pauseCurrentMedia()),
            m_mPlayerEngine,
            SLOT(onPauseCurrentMedia()));
    connect(this,
            SIGNAL(seekCurrentMedia(long, bool)),
            m_mPlayerEngine,
            SLOT(onSeekCurrentMedia(long, bool)));
    connect(this,
            SIGNAL(jumpCurrentMedia(long)),
            m_mPlayerEngine,
            SLOT(onJumpCurrentMedia(long)));
    connect(this,
            SIGNAL(getStreamPosition()),
            m_mPlayerEngine,
            SLOT(onGetStreamPosition()));
    connect(this,
            SIGNAL(switchAudio(int)),
            m_mPlayerEngine,
            SLOT(onSwitchAudio(int)));
    connect(this,
            SIGNAL(switchSubs(int)),
            m_mPlayerEngine,
            SLOT(onSwitchSubs(int)));
    connect(this,
            SIGNAL(sourceChanged(QString, QString, bool, bool)),
            m_mPlayerEngine,
            SLOT(onSetNewSource(QString, QString, bool, bool)));
    connect(m_mPlayerEngine,
            SIGNAL(streamPositionReady(long, long)),
            this,
            SIGNAL(streamPositionReady(long, long)));
    connect(m_mPlayerEngine,
            SIGNAL(videoSeeked()),
            this, 
            SIGNAL(videoSeeked()));
    connect(m_mPlayerEngine,
            SIGNAL(showSubs(QString)),
            this,
            SIGNAL(showSubs(QString)));
    connect(m_mPlayerEngine,
            SIGNAL(hideSubs()),
            this,
            SIGNAL(hideSubs()));
}

/**
 * @brief PE_MediaPlayerElement::~PE_MediaPlayerElement
 */
KvalMediaPlayerElement::~KvalMediaPlayerElement()
{
    LOG_INFO(LOG_TAG, "Delete PE_MediaPlayerElement");
    if(m_mPlayerEngine)
    {
        m_mPlayerEngine->cleanup();
    }
}

/**
 * @brief PE_MediaPlayerElement::setSource
 * @param source
 */
void KvalMediaPlayerElement::setSource(QString source,
                                   QString source2,
                                   bool isDualStream,
                                   bool isLoop)
{
    m_source = source;
    Q_EMIT sourceChanged(source, source2, isDualStream, isLoop);
    return;
}

/**
 * @brief PE_MediaPlayerElement::autoPlay
 * @return 
 */
bool KvalMediaPlayerElement::autoPlay()
{
    return m_autoPlay;
}

/**
 * @brief PE_MediaPlayerElement::setAutoPlay
 * @param autoPlay
 */
void KvalMediaPlayerElement::setAutoPlay(bool autoPlay)
{
    LOG_INFO(LOG_TAG, "Setting autoPlay.");
    m_autoPlay = autoPlay;
}

/**
 * @brief PE_MediaPlayerElement::play
 * @return 
 */
bool KvalMediaPlayerElement::play()
{
    Q_EMIT playCurrentMedia();
    return true;
}

/**
 * @brief PE_MediaPlayerElement::stop
 * @return 
 */
bool KvalMediaPlayerElement::stop()
{
    if(m_mPlayerEngine->isActivePlayerState())
    {
        LOG_INFO(LOG_TAG, "Call Stop directly");
        m_mPlayerEngine->onStopCurrentMedia();
    }
    else
    {
        LOG_INFO(LOG_TAG, "Send Stop cmd to worker thread");
        Q_EMIT stopCurrentMedia();
    }
    return true;
}

/**
 * @brief PE_MediaPlayerElement::pause
 * @return 
 */
bool KvalMediaPlayerElement::pause()
{
    Q_EMIT pauseCurrentMedia();
    return true;
}

/**
 * @brief PE_MediaPlayerElement::getCurrentStreamPosition
 */
void KvalMediaPlayerElement::getCurrentStreamPosition()
{
    Q_EMIT getStreamPosition();
}

/**
 * @brief PE_MediaPlayerElement::seek
 * @param millis
 * @return 
 */
bool KvalMediaPlayerElement::seek(long millis, bool isFlushInternal)
{
    LOG_INFO(LOG_TAG, "seek current Media ...");
    Q_EMIT seekCurrentMedia(millis, isFlushInternal);
    return true;
}

/**
 * @brief PE_MediaPlayerElement::jump
 * @param millis
 * @return
 */
bool KvalMediaPlayerElement::jump(long millis)
{
    LOG_INFO(LOG_TAG, "Jump current Media ...");
    Q_EMIT jumpCurrentMedia(millis);
    return true;
}

/**
 * @brief PE_MediaPlayerElement::setSeekStart
 * @param millis
 */
void KvalMediaPlayerElement::setSeekStart(long millis)
{
    LOG_INFO(LOG_TAG, "seek Start set: %d", millis);
    m_mPlayerEngine->m_seekStart = millis;
}

/**
 * @brief PE_MediaPlayerElement::enableAutoRestart
 */
void KvalMediaPlayerElement::enableAutoRestart()
{
    LOG_INFO(LOG_TAG, "enable interrupt auto restart");
    m_mPlayerEngine->enableInterruptAutoRestart();
}

/**
 * @brief PE_MediaPlayerElement::setAspectRatio
 * @param value
 */
void KvalMediaPlayerElement::setAspectRatio(int value)
{
    INV("value: %d", value);
    LOG_INFO(LOG_TAG, "set Aspect Ratio");
    if(m_mPlayerEngine)
        QMetaObject::invokeMethod(m_mPlayerEngine,
                                  "setAspectRatio",
                                  Q_ARG(int, value));
    OUTV();
}

/**
 * @brief PE_MediaPlayerElement::switchAudioTrack
 */
void KvalMediaPlayerElement::switchAudioTrack(int index)
{
    LOG_INFO(LOG_TAG, "Switch Audio: %d", index);
    Q_EMIT switchAudio(index);
}

/**
 * @brief PE_MediaPlayerElement::switchSubsTrack
 */
void KvalMediaPlayerElement::switchSubsTrack(int index)
{
    LOG_INFO(LOG_TAG, "switch subs: %d", index);
    Q_EMIT switchSubs(index);
}

void KvalMediaPlayerElement::setVolume(long volume)
{
    m_mPlayerEngine->setVolume(volume);
}

/**
 * @brief PE_MediaPlayerElement::volume
 * @return
 */
long KvalMediaPlayerElement::volume()
{
    return m_mPlayerEngine->volume();
}

/**
 * @brief PE_MediaPlayerElement::setMute
 * @param muted
 */
void KvalMediaPlayerElement::setMute(bool muted)
{
    m_mPlayerEngine->setMute(muted);
}

/**
 * @brief PE_MediaPlayerElement::muted
 * @return
 */
bool KvalMediaPlayerElement::muted()
{
    return m_mPlayerEngine->muted();
}

/**
 * @brief PE_MediaPlayerElement::videoBitRate
 * @return
 */
int KvalMediaPlayerElement::videoBitRate()
{
    return m_mPlayerEngine->videoBitRate();
}

QVariantMap KvalMediaPlayerElement::extractStreamInfos()
{
    return m_mPlayerEngine->extractStreamInfos();
}
