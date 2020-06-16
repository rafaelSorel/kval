#define LOG_ACTIVATED

#define LOG_SRC OMXMEDIAELEMENTSTR
#include "KvalPlayerManager.h"
#include "KvalConfigManager.h"
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Global var
//----------------------------------------------------------------------------
using namespace QtAV;
QPointer<KvalMediaPlayerEngine> g_mediaPlayerEngine(nullptr);

//----------------------------------------------------------------------------
// Type definitions
//----------------------------------------------------------------------------
#define CHECK_RET_PLAYER(x) \
    if (m_player.isNull()) { \
       LOG_INFO(LOG_TAG, "No av player."); \
       return x; \
}

#define CHECK_PLAYER \
    if (m_player.isNull()) { \
       LOG_INFO(LOG_TAG, "No av player."); \
       return; \
}

//----------------------------------------------------------------------------
// Public Classes
//----------------------------------------------------------------------------

/**
 * @brief PE_MediaPlayerEngine::PE_MediaPlayerEngine
 * @param parent
 */
KvalMediaPlayerEngine::KvalMediaPlayerEngine(QObject* parent)
{
    LOG_INFO(LOG_TAG, "Mplayer engine instantiation");
    m_player.reset(new AVPlayer(parent));
    QStringList hwDecodingSupport{"CUDA", "D3D11",
                                  "DXVA", "VAAPI",
                                  "VideoToolbox", "FFmpeg"};

    m_player->setVideoDecoderPriority(hwDecodingSupport);
    QVariantHash va_opt;
    //"GLX", "X11", "DRM"
    va_opt["display"] = "GLX";
    // "ZeroCopy", "OptimizedCopy", "GenericCopy". Default is "ZeroCopy" if possible
    va_opt["copyMode"] = "ZeroCopy";
    QVariantHash opt;
    //key is decoder name, case sensitive
    opt["VAAPI"] = va_opt;
    m_player->setOptionsForVideoCodec(opt);
    connect(m_player.get(), SIGNAL(started()), SIGNAL(playbackStarted()));
    connect(m_player.get(), SIGNAL(loaded()), this, SLOT(onMediaLoader()));
    connect(m_player.get(), SIGNAL(seekFinished(qint64)), SIGNAL(videoSeeked()));
    connect(m_player.get(), SIGNAL(error(const QtAV::AVError&)), this, SLOT(onPlayerError(const QtAV::AVError&)));
    connect(m_player.get(), SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), this, SLOT(onMediaStatusChanged(QtAV::MediaStatus)));

    g_mediaPlayerEngine = this;
}

/**
 * @brief PE_MediaPlayerEngine::onSetNewSource
 * @param source
 * @param source2
 * @param isDualStream
 * @param isLoop
 */
void KvalMediaPlayerEngine::onSetNewSource(QString source,
                                          QString source2,
                                          bool isDualStream,
                                          bool isLoop)
{
    CHECK_PLAYER
    if(m_player->isPlaying()){
        Q_EMIT mediaPlayerBusy();
        return;
    }
    setSource(source, source2, isDualStream, isLoop);
}

/**
 * @brief PE_MediaPlayerEngine::onMediaLoader
 */
void KvalMediaPlayerEngine::onMediaLoader()
{
    LOG_INFO(LOG_TAG, "Media Loaded ");
    m_metaData->setValuesFromStatistics(m_player->statistics());
}

/**
 * @brief PE_MediaPlayerEngine::onMediaStatusChanged
 * @param status
 */
void KvalMediaPlayerEngine::onMediaStatusChanged(MediaStatus status)
{
    INV("status: %u", status);
    LOG_INFO(LOG_TAG, "Media Status changed: %u", status);

    switch (status) {
        case MediaStatus::LoadingMedia:
        case MediaStatus::LoadedMedia:
            LOG_INFO(LOG_TAG, "Start Playing media...");
            m_player->play();
        break;

        case MediaStatus::EndOfMedia:
            LOG_INFO(LOG_TAG, "Playback ended...");
            local_cleanup();
            Q_EMIT playbackCompleted();
        break;
        case MediaStatus::BufferingMedia:
        case MediaStatus::BufferedMedia:
            LOG_INFO(LOG_TAG, "Media Buffering.");
        break;

        case MediaStatus::StalledMedia:
            LOG_INFO(LOG_TAG, "Playback interrupted ...");
            if(!m_uiStopRequest){
                Q_EMIT playbackInterrupted(streamPosition());
            } else {
                local_cleanup();
                Q_EMIT playbackCompleted();
            }
        break;

        default:
            LOG_ERROR(LOG_TAG, "Something went wrong while loading: %u", status);
            local_cleanup();
            Q_EMIT playbackFailed();
        break;
    }
    OUTV();
}

/**
 * @brief PE_MediaPlayerEngine::local_cleanup
 */
void KvalMediaPlayerEngine::local_cleanup()
{
    m_source.clear();
    m_dualStream = false;
    m_uiStopRequest = false;
    m_audio_tracks.clear();
    m_subs_tracks.clear();
    m_seekStart = 0;
    m_autoRestart = false;
}
/**
 * @brief PE_MediaPlayerEngine::onPlayerError
 * @param err
 */
void KvalMediaPlayerEngine::onPlayerError(const QtAV::AVError& err)
{
    INV("err: %u", err.error());
    LOG_ERROR(LOG_TAG, "error on media player: %s", qPrintable(err.string()));
    Q_EMIT playbackFailed();
    OUTV("");
}

/**
 * @brief PE_MediaPlayerEngine::setSource
 * @param source
 * @param source2
 * @param isDualStream
 * @param isLoop
 */
void KvalMediaPlayerEngine::setSource(QString source,
                                     QString source2,
                                     bool isDualStream,
                                     bool isLoop)
{
    Q_UNUSED(source2);
    m_source = source;
    m_dualStream = isDualStream;
    m_uiStopRequest = false;
    m_audio_tracks.clear();
    m_subs_tracks.clear();
    m_metaData.reset(new KvalPlayerMetaData);

    m_player->setStartPosition(m_seekStart);
    m_player->setFile(source);
    m_player->setRepeat(isLoop ? -1 : 0);
    if (!m_player->load())
    {
        LOG_ERROR(LOG_TAG, "Unable to load media source !");
        Q_EMIT playbackFailed();
        return;
    }
}

/**
 * @brief PP_MediaPlayer::jump
 * @param position
 * @return
 */
bool KvalMediaPlayerEngine::jump(qint64 position)
{
    LOG_INFO(LOG_TAG, "Jump %lldms.", position);
    return seek(position, true);
}

/**
 * @brief PE_MediaPlayerEngine::setVolume
 * @param value
 */
void KvalMediaPlayerEngine::setVolume(long value)
{
    CHECK_PLAYER

    AudioOutput *ao = m_player->audio();
    if (!ao || !ao->isAvailable())
        return;

    m_volume = ao->volume();
    LOG_INFO(LOG_TAG, ">>>>>>>>>>>>> set volume: %f", m_volume+value);
    ao->setVolume(m_volume + value);
}

/**
 * @brief PE_MediaPlayerEngine::volume
 * @return
 */
long KvalMediaPlayerEngine::volume()
{
    CHECK_RET_PLAYER(0)

    AudioOutput *ao = m_player->audio();
    if (!ao || !ao->isAvailable())
        return 0;

    m_volume = ao->volume();
    LOG_INFO(LOG_TAG, ">>>>>>>>>>>>> m_volume: %f", m_volume);
    return m_volume;
}

/**
 * @brief PE_MediaPlayerEngine::setMute
 * @param muted
 */
void KvalMediaPlayerEngine::setMute(bool muted)
{
    CHECK_PLAYER

    AudioOutput *ao = m_player->audio();
    if (!ao || !ao->isAvailable())
        return;
    return ao->setMute(muted);
}

/**
 * @brief PE_MediaPlayerEngine::muted
 * @return
 */
bool KvalMediaPlayerEngine::muted()
{
    CHECK_RET_PLAYER(false)

    AudioOutput *ao = m_player->audio();
    if (!ao || !ao->isAvailable())
        return false;
    return ao->isMute();
}

/**
 * @brief PE_MediaPlayerEngine::avplayer
 * @return
 */
AVPlayer *KvalMediaPlayerEngine::avplayer()
{
    return m_player.get();
}

/**
 * @brief PE_MediaPlayerEngine::isActivePlayerState
 * @return
 */
bool KvalMediaPlayerEngine::isActivePlayerState()
{
    CHECK_RET_PLAYER(false)

    return m_player->isPlaying();
}

/**
 * @brief PE_MediaPlayerEngine::streamPosition
 * @return
 */
qint64 KvalMediaPlayerEngine::streamPosition()
{
    CHECK_RET_PLAYER(-1)

    return m_player->position();
}

/**
 * @brief PE_MediaPlayerEngine::streamLength
 * @return
 */
qint64 KvalMediaPlayerEngine::streamLength()
{
    CHECK_RET_PLAYER(-1)
    return m_player->duration();
}

/**
 * @brief PE_MediaPlayerEngine::seek
 * @param position
 * @param isJump
 * @return
 */
bool KvalMediaPlayerEngine::seek(qint64 position, bool isJump)
{
    LOG_INFO(LOG_TAG, "Seek %lldms.", position);
    if(!m_player->isSeekable())
    {
        LOG_INFO(LOG_TAG, "Media is unseakable !");
        return false;
    }

    qint64 newPosition = position;
    qint64 currentTime = 0;
    if (!isJump)
    {
        currentTime = streamPosition()/1000;
        LOG_INFO(LOG_TAG, "currentTime: %lld", currentTime);
        newPosition = currentTime + position;
        LOG_INFO(LOG_TAG, "newPosition: %lld", newPosition);
    }

    m_player->setPosition(newPosition*1000);
    OUTV("true");
    return true;
}

/**
 * @brief PE_MediaPlayerEngine::getCurrentStreamPosition
 * @return
 */
QVariantMap KvalMediaPlayerEngine::getCurrentStreamPosition()
{
    QVariantMap streamPosMap{
        {"position", streamPosition()},
        {"duration", streamLength()}};

    return streamPosMap;
}

/**
 * @brief PP_MediaPlayer::extractStreamInfos
 * @return
 */
QVariantMap KvalMediaPlayerEngine::extractStreamInfos()
{
    INV();
    LOG_INFO(LOG_TAG, "Extract Stream Informations...");
    m_metaData->setValuesFromStatistics(m_player->statistics());
    KvalPlayerMetaData *metaData = m_metaData.data();
    //Create variant map values
    QStringList audio_langs;
    QStringList subs_langs;
    QVariantMap hints_value {
        {"resolution", ""},
        {"bitrate", ""},
        {"aspect", ""},
        {"Vcodec", ""},
        {"Acodec", ""},
        {"Alang", ""},
        {"Scodec", ""},
        {"Slang", ""},
        {"length", metaData->duration()},
        {"audioNbr", m_player->audioStreamCount()},
        {"subsNbr", (m_player->subtitleStreamCount() > 0) ? m_player->subtitleStreamCount()+1 : 1},
        {"audioTracks", QStringList{tr("Audio Unavailable")}},
        {"subsTracks",  QStringList{tr("Substitles Unavailable")} }
    };

    //Gather default in play values
    if(m_player->videoStreamCount() > 0)
    {
        QString val;
        QTextStream res(&val, QIODevice::WriteOnly);
        res << metaData->resolution().toSize().width()
            << "x"
            << metaData->resolution().toSize().height();
        res.flush();
        LOG_INFO(LOG_TAG, "Resolution [%s]", qPrintable(val));
        hints_value["resolution"] = val;
        res.reset();
        val.clear();

        res << (metaData->videoBitRate().toInt() / 1024) << " Kbps";
        res.flush();
        hints_value["bitrate"] = val;

        hints_value["aspect"] = metaData->pixelAspectRatio();
        hints_value["Vcodec"] = metaData->videoCodec();
    }
    if(m_player->audioStreamCount() > 0){
        m_audio_tracks.clear();
        hints_value["Acodec"] = metaData->audioCodec();
        hints_value["Alang"] = metaData->language();
        QVariantList audio_tracks= m_player->internalAudioTracks();
        Q_FOREACH(const QVariant& track, audio_tracks)
        {
            AudioTrack audioTrack(qvariant_cast<QVariantMap>(track));
            m_audio_tracks.append(audioTrack);
            qDebug() << audioTrack;
            audio_langs.append(tr(audioTrack.langFull.toStdString().c_str()));
        }
        hints_value["audioTracks"] = audio_langs;
    }
    if(m_player->subtitleStreamCount() > 0){

        hints_value["Scodec"] = metaData->subTitle();
        hints_value["Slang"] = metaData->language();
        QVariantList subs_tracks= m_player->internalSubtitleTracks();
        Q_FOREACH(const QVariant& track, subs_tracks)
        {
            SubsTrack subsTrack(qvariant_cast<QVariantMap>(track));
            m_subs_tracks.append(subsTrack);
            qDebug() << subsTrack;
            subs_langs.append(subsTrack.langFull);
        }
        LOG_INFO(LOG_TAG, "currentSubtitleStream: %d", m_player->currentSubtitleStream());
        subs_langs.append(tr("Deactivate"));
        hints_value["subsTracks"] = subs_langs;
    }

    auto _ftime = [](qint64 _ms){
        qint64 _s = _ms/1000;
        int hours = _s/3600;
        int minutes = (_s%3600)/60;
        int seconds = (_s%3600)%60;
        QString _fmt;
        QTextStream _ts(&_fmt);
        _ts.setRealNumberPrecision(2);
        _ts << hours << ':' << minutes << ':' << seconds;
        return _fmt;
    };

    LOG_INFO(LOG_TAG, "streamLength [%d]", streamLength());
    QString streamDurationStr = _ftime(streamLength());
    LOG_INFO(LOG_TAG, "streamDuration: %s", qPrintable(streamDurationStr));
    hints_value["length"] = streamDurationStr;
    LOG_INFO(LOG_TAG, "------------------------------------------------");

    OUTV();
    return hints_value;
}

void KvalMediaPlayerEngine::play() { m_player->togglePause(); }

/**
 * @brief PE_MediaPlayerEngine::pause
 */
void KvalMediaPlayerEngine::pause()
{
    m_player->togglePause();
}

/**
 * @brief PE_MediaPlayerEngine::stop
 */
void KvalMediaPlayerEngine::stop()
{
    m_uiStopRequest = true;
    m_player->stop();
    Q_EMIT uiStopRequested();
}

/**
 * @brief PE_MediaPlayerEngine::switchAudioTrack
 * @param index
 */
void KvalMediaPlayerEngine::switchAudioTrack(int index)
{
    INV("index: %d", index);
    LOG_INFO(LOG_TAG, "index: %d", index);
    if(m_player->audioStreamCount() <= 1 || index > m_player->audioStreamCount())
    {
        LOG_INFO(LOG_TAG, "Only one Audio track available");
        return;
    }

    if(!m_player->setAudioStream(index))
    {
        LOG_ERROR(LOG_TAG, "Unable to switch audio stream !");
    }
    OUTV();
}

/**
 * @brief PE_MediaPlayerEngine::switchAudioTrack
 * @param index
 */
void KvalMediaPlayerEngine::switchSubsTrack(int index)
{
    INV("index: %d", index);
    LOG_INFO(LOG_TAG, "currentSubtitleStream: %d", m_player->currentSubtitleStream());
    LOG_INFO(LOG_TAG, "switch subs: %d", index);
    if(m_player->subtitleStreamCount() <= 1 || index > m_player->subtitleStreamCount())
    {
        LOG_INFO(LOG_TAG, "Only one Subs track available");
        return;
    }

    if(!m_player->setSubtitleStream(index))
    {
        LOG_ERROR(LOG_TAG, "Unable to switch subs stream !");
    }

    OUTV();
}

/**
 * @brief PE_MediaPlayerEngine::videoBitRate
 * @return
 */
int KvalMediaPlayerEngine::videoBitRate()
{
    if(isActivePlayerState()){
        m_metaData->setValuesFromStatistics(m_player->statistics());
        int bitrate = m_metaData.data()->videoBitRate().toInt();
        LOG_INFO(LOG_TAG, "Extracted bitrate: %d", bitrate);
        return bitrate;
    }
    return 0;
}
/**
 * @brief PE_MediaPlayerElement::PE_MediaPlayerElement
 * @param parent
 */
KvalMediaPlayerElement::KvalMediaPlayerElement(QObject* parent) :
    KvalMediaPlayerEngine(parent)
{
    qInfo() << "PE_MediaPlayerElement created";
}
