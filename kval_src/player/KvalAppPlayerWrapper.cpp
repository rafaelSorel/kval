#define LOG_ACTIVATED
#include <algorithm>
#include <random>
#include <QObject>
#include <QMap>

#include "appmanager/KvalAppManager.h"
#include "KvalAppPlayerWrapper.h"


#define LOG_SRC PLAYERHELPER
#include "KvalLogging.h"

/*------------------------------------------------------------------------------
|    DEFINES
+-----------------------------------------------------------------------------*/
#define CHECK_RET_MP_ENGINE(x) \
    if (!m_mediaPlayerEngine) { \
       LOG_INFO(LOG_TAG, "No media player."); \
       return x; \
}

#define CHECK_MP_ENGINE \
    if (!m_mediaPlayerEngine) { \
       LOG_INFO(LOG_TAG, "No media player."); \
       return; \
}

/*------------------------------------------------------------------------------
|    Functions
+-----------------------------------------------------------------------------*/


/**
 * @brief PH_MediaPlayerWrapper::PH_MediaPlayerWrapper
 */
KvalAppMediaPlayerWrapper::KvalAppMediaPlayerWrapper():
    m_mediaPlayerEngine(nullptr),
    m_playList(nullptr),
    m_isPlaying(false),
    m_isAudioPlaying(false),
    m_isVideoPlaying(false),
    m_currentPosition(-1),
    m_playBackDelayPolicy(false)
{
    //Extract mediaPlayerEngine obj
    m_mediaPlayerEngine = KvalMediaPlayerEngine::getGlobalRef();
    if (!m_mediaPlayerEngine) {
        LOG_ERROR(LOG_TAG, "No player engine available !");
        return;
    }
    connect(m_mediaPlayerEngine,
            SIGNAL(playbackStarted()),
            this,
            SLOT(onPlaybackStarted()));
    connect(m_mediaPlayerEngine,
            SIGNAL(playbackFailed()),
            this,
            SLOT(onPlaybackFailed()));
    connect(m_mediaPlayerEngine,
            SIGNAL(playbackCompleted()),
            this,
            SLOT(onPlaybackCompleted()));
    connect(m_mediaPlayerEngine,
            SIGNAL(playbackInterrupted(long)),
            this,
            SLOT(onPlaybackInterrupted(long)));
    connect(m_mediaPlayerEngine,
            SIGNAL(uiStopRequested()),
            this,
            SLOT(onUiStopRequested()));
    connect(m_mediaPlayerEngine,
            SIGNAL(mediaPlayerBusy()),
            this,
            SLOT(onMediaPlayerBusy()));


}

/**
 * @brief PH_MediaPlayerWrapper::~PH_MediaPlayerWrapper
 */
KvalAppMediaPlayerWrapper::~KvalAppMediaPlayerWrapper()
{

}

/**
 * @brief PH_MediaPlayerWrapper::onPlaybackStarted
 */
void KvalAppMediaPlayerWrapper::onPlaybackStarted()
{
    LOG_INFO(LOG_TAG, "onPlaybackStarted");
    m_isPlaying = true;
    Q_EMIT playbackStarted();
}

/**
 * @brief PH_MediaPlayerWrapper::onPlaybackStarted
 */
void KvalAppMediaPlayerWrapper::onMediaPlayerBusy()
{
    LOG_INFO(LOG_TAG, "onMediaPlayerBusy");
    m_isPlaying = false;
    if(m_playList)
    {
        m_playList->Clear();
        m_playList = nullptr;
    }
    m_currentPosition = -1;
    Q_EMIT playbackFailed();
}

/**
 * @brief PH_MediaPlayerWrapper::onPlaybackFailed
 */
void KvalAppMediaPlayerWrapper::onPlaybackFailed()
{
    LOG_INFO(LOG_TAG, "onPlaybackFailed");
    m_isPlaying = false;

    if(m_playList)
    {
        m_playList->Clear();
        m_playList = nullptr;
    }
    m_currentPosition = -1;
    Q_EMIT playbackFailed();

}

/**
 * @brief PH_MediaPlayerWrapper::onPlaybackCompleted
 */
void KvalAppMediaPlayerWrapper::onPlaybackCompleted()
{
    LOG_INFO(LOG_TAG, "onPlaybackCompleted");
    m_isPlaying = false;
    if(!m_playBackDelayPolicy)
        playNext();

    Q_EMIT playbackCompleted();
}


/**
 * @brief PH_MediaPlayerWrapper::onPlaybackInterrupted
 * @param interruptedPosition
 */
void KvalAppMediaPlayerWrapper::onPlaybackInterrupted(long interruptedPosition)
{
    (void)interruptedPosition;
    m_isPlaying = false;
}

/**
 * @brief PH_MediaPlayerWrapper::activateDelayPolicy
 */
void KvalAppMediaPlayerWrapper::waitForUiGo()
{
    m_playBackDelayPolicy = true;
}

/**
 * @brief PH_MediaPlayerWrapper::onUiStopRequested
 */
void KvalAppMediaPlayerWrapper::onUiStopRequested()
{
    LOG_INFO(LOG_TAG, "onUiStopRequested");
    if(m_playList)
    {
        m_playList->Clear();
        m_playList = nullptr;
    }
    m_currentPosition = -1;
    Q_EMIT uiStopRequested();
}

/**
 * @brief PH_MediaPlayerWrapper::play
 * @param source
 * @param argument
 * @param windowed
 * @param startpos
 */
void KvalAppMediaPlayerWrapper::play(QString source,
                              KvalAppMediaPlayList * playlist,
                              QMap<QString, QString> item,
                              bool windowed,
                              int startpos)
{
    if (!source.compare("playlist") && playlist)
    {
        playPlayList(playlist, windowed, startpos);
    }
    else
    {
        playStream(item, windowed);
    }
}

/**
 * @brief PH_MediaPlayerWrapper::setPlayList
 * @param playlist
 */
void KvalAppMediaPlayerWrapper::setPlayList(KvalAppMediaPlayList * playlist)
{
    if(!playlist)
    {
        LOG_ERROR(LOG_TAG, "playlist null");
        return;
    }

    m_playList = playlist;
}

/**
 * @brief PH_MediaPlayerWrapper::playPlayList
 * @param playlist
 * @param windowed
 * @param startpos
 */
void KvalAppMediaPlayerWrapper::playPlayList(KvalAppMediaPlayList * playlist,
                                         bool windowed,
                                         int startpos)
{
    if(!playlist)
    {
        LOG_ERROR(LOG_TAG, "playlist null");
        return;
    }
    m_playList = playlist;

    m_currentPosition = (startpos < 0) ? 0 : startpos;
    LOG_INFO(LOG_TAG, "m_currentPosition: %d", m_currentPosition);
    bool found = false;
    QMap<QString, QString> item = m_playList->GetItem(m_currentPosition, &found);
    if(found)
    {
        playStream(item, windowed);
    }
    else
    {
        LOG_ERROR(LOG_TAG, "playlist empty or wrong position value");
        m_playList->Clear();
        m_playList = nullptr;
        m_currentPosition = -1;
        return;
    }
}

/**
 * @brief PH_MediaPlayerWrapper::getStreamType
 * @param url
 * @return
 */
MEDIA_URL_TYPE_t KvalAppMediaPlayerWrapper::getStreamType(QString url)
{
    if(url.startsWith("http://") || url.startsWith("https://"))
        return MEDIA_TYPE_HTTP;
    else if(url.startsWith("file://"))
        return MEDIA_TYPE_FILE;
    else if(url.startsWith("plugin://"))
        return MEDIA_TYPE_PLUGIN;
    else
        return MEDIA_TYPE_UNKNOWN;
}

/**
 * @brief PH_MediaPlayerWrapper::isActivePlayList
 * @return
 */
bool KvalAppMediaPlayerWrapper::isActivePlayList()
{
    return !(!m_playList || !m_playList->size() || (m_currentPosition+1) >= m_playList->size());
}

/**
 * @brief PH_MediaPlayerWrapper::nextPlayListItem
 * @return
 */
QVariantMap KvalAppMediaPlayerWrapper::nextPlayListItem()
{
    QVariantMap rval;
    bool found;
    QMap<QString, QString> tempmap = m_playList->GetItem(m_currentPosition+1, &found);
    Q_FOREACH (QString key, tempmap.keys())
    {
        rval[key] = tempmap[key];
    }
    return rval;
}

/**
 * @brief PH_MediaPlayerWrapper::currentPlayListItem
 * @return
 */
QVariantMap KvalAppMediaPlayerWrapper::currentPlayListItem()
{
    LOG_INFO(LOG_TAG, ">>>>>>>>>> currentPlayListItem: %d", m_currentPosition);
    QVariantMap rval;
    bool found;
    QMap<QString, QString> tempmap = m_playList->GetItem(m_currentPosition, &found);
    Q_FOREACH (QString key, tempmap.keys())
    {
        rval[key] = tempmap[key];
    }
    return rval;
}

/**
 * @brief PH_MediaPlayerWrapper::playStream
 * @param item
 * @param windowed
 */
void KvalAppMediaPlayerWrapper::playStream(QMap<QString, QString> item,
                                       bool windowed)
{
    CHECK_MP_ENGINE
    //Not used for now, all media are playing full screen.
    (void)windowed;
    if(m_playList)
    {
        m_playList->setPosition(m_currentPosition);
    }
    try
    {
        if ( ( (!item["mediatype"].compare("video")) ||
               (!item["mediatype"].compare("audio")) )
             &&
                (!item["IsPlayable"].compare("true")) )
        {
            MEDIA_URL_TYPE_t type = getStreamType(item["url"]);
            if(type == MEDIA_TYPE_HTTP || type == MEDIA_TYPE_FILE)
            {
                LOG_INFO(LOG_TAG, "Direct Media play...");
                if(!item["path"].compare("dualstream"))
                {
                    QMetaObject::invokeMethod(m_mediaPlayerEngine,
                                              "onSetNewSource",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString, item["videoUri"]),
                                              Q_ARG(QString, item["audioUri"]),
                                              Q_ARG(bool, true));
                }
                else
                {
                    QMetaObject::invokeMethod(m_mediaPlayerEngine,
                                              "onSetNewSource",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString, item["url"]),
                                              Q_ARG(QString, ""),
                                              Q_ARG(bool, false));
                }
            }
            else if(type == MEDIA_TYPE_PLUGIN)
            {
                LOG_INFO(LOG_TAG,
                         "Need to resolve the url...: %s",
                         qPrintable(item["url"]));
                QMetaObject::invokeMethod(g_appEngineRef,
                                          "runPlugin",
                                          Q_ARG(QString, item["url"]));
            }
            else
            {
                LOG_ERROR(LOG_TAG,
                          "Unknown media type url: %s",
                          item["url"].toStdString().c_str());
            }
        }
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "exeception while parsing item");
        return;
    }

}

/**
 * @brief PH_MediaPlayerWrapper::stop
 */
void KvalAppMediaPlayerWrapper::stop()
{
    CHECK_MP_ENGINE
    QMetaObject::invokeMethod(m_mediaPlayerEngine,
                              "onStopCurrentMedia",
                              (m_mediaPlayerEngine->isActivePlayerState()) ?
                                Qt::DirectConnection : Qt::QueuedConnection);
}

/**
 * @brief PH_MediaPlayerWrapper::pause
 */
void KvalAppMediaPlayerWrapper::pause()
{
    CHECK_MP_ENGINE
    QMetaObject::invokeMethod(m_mediaPlayerEngine,
                              "onPauseCurrentMedia",
                              Qt::QueuedConnection);
}

/**
 * @brief PH_MediaPlayerWrapper::playNext
 */
void KvalAppMediaPlayerWrapper::playNext()
{
    if(!m_playList)
    {
        LOG_INFO(LOG_TAG, "No Active play list");
        return;
    }

    m_playBackDelayPolicy = false;
    bool found = false;
    m_currentPosition = m_currentPosition + 1;
    QMap<QString, QString> item = m_playList->GetItem(m_currentPosition, &found);
    if(found)
    {
        playStream(item, false);
    }
    else
    {
        LOG_INFO(LOG_TAG, "playlist empty or wrong position value: %d", m_currentPosition);
        m_playList->Clear();
        m_playList = nullptr;
        m_currentPosition = -1;
    }
}

/**
 * @brief PH_MediaPlayerWrapper::playPrevious
 */
void KvalAppMediaPlayerWrapper::playPrevious()
{
    if(!m_playList)
    {
        LOG_INFO(LOG_TAG, "No Active play list");
        return;
    }

    bool found = false;
    m_currentPosition = m_currentPosition - 1;
    QMap<QString, QString> item = m_playList->GetItem(m_currentPosition, &found);
    if(found)
    {
        playStream(item, false);
    }
    else
    {
        LOG_INFO(LOG_TAG, "playlist empty or wrong position value: %d", m_currentPosition);
        m_playList->Clear();
        m_playList = nullptr;
        m_currentPosition = -1;
    }
}

/**
 * @brief PH_MediaPlayerWrapper::playSelected
 * @param selected
 */
void KvalAppMediaPlayerWrapper::playSelected(int selected)
{
    bool found = false;
    if(!m_playList)
    {
        LOG_INFO(LOG_TAG, "No Active play list");
        return;
    }

    QMap<QString, QString> item = m_playList->GetItem(selected, &found);
    if(found)
    {
        playStream(item, false);
    }
    else
    {
        LOG_ERROR(LOG_TAG, "playlist empty or wrong position value: %d", selected);
        m_playList->Clear();
        m_playList = nullptr;
        m_currentPosition = -1;
    }
}

/**
 * @brief PH_MediaPlayerWrapper::isPlaying
 * @return
 */
bool KvalAppMediaPlayerWrapper::isPlaying()
{
    return m_isPlaying;
}

/**
 * @brief PH_MediaPlayerWrapper::isPlayingAudio
 * @return
 */
bool KvalAppMediaPlayerWrapper::isPlayingAudio()
{
    return m_isAudioPlaying;
}

/**
 * @brief PH_MediaPlayerWrapper::isPlayingVideo
 * @return
 */
bool KvalAppMediaPlayerWrapper::isPlayingVideo()
{
    return m_isVideoPlaying;
}

/**
 * @brief PH_MediaPlayerWrapper::playingFileName
 * @return
 */
QString KvalAppMediaPlayerWrapper::playingFileName()
{
    return "";
}

/**
 * @brief PH_MediaPlayerWrapper::streamPosition
 * @return
 */
int KvalAppMediaPlayerWrapper::streamPosition()
{
    CHECK_RET_MP_ENGINE(-1)
    return (int)m_mediaPlayerEngine->streamPosition();
}

/**
 * @brief PH_MediaPlayerWrapper::seek
 * @param seekTime
 */
void KvalAppMediaPlayerWrapper::seek(int seekTime)
{
    CHECK_MP_ENGINE
    QMetaObject::invokeMethod(m_mediaPlayerEngine,
                              "onJumpCurrentMedia",
                              Qt::QueuedConnection,
                              Q_ARG(int, seekTime));
}

/**
 * @brief PH_MediaPlayerWrapper::setSubs
 * @param path
 */
void KvalAppMediaPlayerWrapper::setSubs(QString path)
{
    //@todo:: Implement this.
    (void)path;
    return;
}

/**
 * @brief PH_MediaPlayerWrapper::streamLength
 * @return
 */
int KvalAppMediaPlayerWrapper::streamLength()
{
    CHECK_RET_MP_ENGINE(-1)
    return (int)m_mediaPlayerEngine->streamLength();
}

/**
 * @brief PH_MediaPlayList::PH_MediaPlayList
 */
KvalAppMediaPlayList::KvalAppMediaPlayList()
{
    m_strPlayListName = "";
    m_strBasePath = "";
    m_iPlayableItems = -1;
    m_bShuffled = false;
    m_bWasPlayed = false;
    m_currentPosition = -1;
}

/**
 * @brief PH_MediaPlayList::~PH_MediaPlayList
 */
KvalAppMediaPlayList::~KvalAppMediaPlayList(void)
{
    m_playListItems.clear();
    m_currentPosition = -1;
}

/**
 * @brief Insert
 * @param item
 * @param iPosition
 */
void KvalAppMediaPlayList::Insert(const QMap<QString, QString> &item, int iPosition)
{
    if(iPosition < 0)
    {
        m_playListItems.append(item);
    }
    else
    {
        m_playListItems.insert(iPosition, item);
    }
}

/**
 * @brief PH_MediaPlayList::GetName
 * @return
 */
QString KvalAppMediaPlayList::GetName()
{
    return m_strPlayListName;
}

/**
 * @brief PH_MediaPlayList::Remove
 * @param position
 */
void KvalAppMediaPlayList::Remove(int position)
{
    if(!size())
        return;

    m_playListItems.remove(position);
}

/**
 * @brief PH_MediaPlayList::Swap
 * @param position1
 * @param position2
 * @return
 */
bool KvalAppMediaPlayList::Swap(int position1, int position2)
{
    (void)position1;
    (void)position2;
    return false;
}

/**
 * @brief PH_MediaPlayList::Clear
 */
void KvalAppMediaPlayList::Clear()
{
    m_playListItems.clear();
    m_currentPosition = -1;
}

/**
 * @brief PH_MediaPlayList::size
 * @return
 */
int KvalAppMediaPlayList::size() const
{
    return m_playListItems.size();
}

/**
 * @brief PH_MediaPlayList::Shuffle
 * @param iPosition
 */
void KvalAppMediaPlayList::Shuffle(int iPosition)
{
    if(!size() || iPosition > size())
        return;

    if(iPosition < 0)
        iPosition = 0;

    m_playListItemsUnshuffl.swap(m_playListItems);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::shuffle(m_playListItems.begin(), m_playListItems.end(), mt);
    m_bShuffled = true;
}

/**
 * @brief PH_MediaPlayList::UnShuffle
 */
void KvalAppMediaPlayList::UnShuffle()
{
    if(!size())
        return;

    m_playListItems.swap(m_playListItemsUnshuffl);
    m_bShuffled = false;
}

/**
 * @brief PH_MediaPlayList::UpdateItem
 * @param item
 */
void KvalAppMediaPlayList::UpdateItem(const QMap<QString, QString> item)
{
    if(!size())
        return;

    for(int i = 0; i < m_playListItems.size(); i++)
    {
        if(!item["url"].compare(item["url"]))
        {
            m_playListItems.replace(i, item);
            break;
        }
    }
}

/**
 * @brief PH_MediaPlayList::GetItem
 * @param position
 * @param found
 * @return
 */
QMap<QString, QString> KvalAppMediaPlayList::GetItem(int position, bool * found)
{
    if(size() > 0 && position < size() && position >= 0)
    {
        *found = true;
        return m_playListItems.at(position);
    }

    QMap<QString, QString> dummyitem;
    *found = false;
    return dummyitem;
}

/**
 * @brief PH_MediaPlayList::setPosition
 */
void KvalAppMediaPlayList::setPosition(int position)
{
    if(size() > 0 && position < size() && position >= 0)
    {
        m_currentPosition = position;
    }
    else
    {
        m_currentPosition = -1;
    }
}

/**
 * @brief PH_MediaPlayList::currentPosition
 * @return
 */
int KvalAppMediaPlayList::currentPosition()
{
    return m_currentPosition;
}
