#define LOG_ACTIVATED
#include <cstring>
#include <semaphore.h>
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <queue>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

#include <QStringList>
#include <QOpenGLContext>
#include <QElapsedTimer>
#include <QUrl>
#include <QSettings>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#define LOG_SRC OMXMEDIAPROCSTR
#include "KvalLogging.h"
#include "KvalConfigManager.h"
#include "KvalAmlPlayerCore.h"
/*------------------------------------------------------------------------------
|    DEFINES
+-----------------------------------------------------------------------------*/
#define ENABLE_HDMI_CLOCK_SYNC false
#define FONT_PATH           "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
#define FONT_PATH_ITALIC    "/usr/share/fonts/truetype/freefont/FreeSansOblique.ttf"
#define FONT_SIZE              (0.055f)
#define CENTERED               false
#define BUFFERING_TIMEOUT_S    3

QPointer<PP_MediaPlayer> g_mediaPlayerRef;
/*------------------------------------------------------------------------------
|    Functions
+-----------------------------------------------------------------------------*/

/**
 * @brief PP_MediaPlayer::PP_MediaPlayer
 * @param provider
 */
PP_MediaPlayer::PP_MediaPlayer(bool isDualStream) :
    m_aml_reader2(nullptr),
    m_state(STATE_INACTIVE),
    m_has_video(false),
    m_has_audio(false),
    m_has_subtitle(false),
    m_pendingStop(false),
    m_flush_asked(false),
    startpts(0),
    m_seekStart(0),
    m_isLocalFile(false),
    m_enable_auto_restart(false),
    m_Volume(0),
    m_audio_index_use(0),
    m_subs_index_use(0),
    m_dualStream(isDualStream),
    m_loop(false),
    m_threadVec{&m_aml_reader_thread,
                &m_aml_reader2_thread,
                &m_aml_video_thread,
                &m_aml_video_clk_thread,
                &m_aml_audio_thread,
                &m_aml_subs_thread}
{
    qRegisterMetaType<PP_MediaPlayer::PP_MediaPlayerError>
           ("PP_MediaPlayer::PP_MediaPlayerError");
    qRegisterMetaType<PP_MediaPlayer::PP_MediaPlayerState>
           ("PP_MediaPlayer::PP_MediaPlayerState");
    qRegisterMetaType<BufferSPTR>("BufferSPTR");
    qRegisterMetaType<PinInfoSPTR>("PinInfoSPTR");

    m_aml_reader = new AMLReader;
    m_aml_reader_thread = new KvalThread("AMLReader");;
    m_aml_reader->moveToThread(m_aml_reader_thread);
    m_aml_reader_thread->setObjectName("AMLReader");
    connect(m_aml_reader_thread, &QThread::finished, m_aml_reader, &QObject::deleteLater);
    m_aml_reader_thread->start();

    if(m_dualStream)
    {
        m_aml_reader2 = new AMLReader;
        m_aml_reader2_thread = new KvalThread("AMLReader2");
        m_aml_reader2->moveToThread(m_aml_reader2_thread);
        m_aml_reader2_thread->setObjectName("AMLReader2");
        connect(m_aml_reader2_thread, &QThread::finished, m_aml_reader2, &QObject::deleteLater);
        m_aml_reader2_thread->start();
    }

    m_player_video = new AmlVideoSinkElement;
    m_aml_video_thread = new KvalThread("AmlVideoSinkElement");
    m_player_video->moveToThread(m_aml_video_thread);
    m_aml_video_thread->setObjectName("AmlVideoSinkElement");
    connect(m_aml_video_thread, &QThread::finished, m_player_video, &QObject::deleteLater);
    m_aml_video_thread->start();

    m_player_audio = new AudioCodecElement;
    m_aml_audio_thread = new KvalThread("AudioCodecElement");
    m_player_audio->moveToThread(m_aml_audio_thread);
    m_aml_audio_thread->setObjectName("AudioCodecElement");
    connect(m_aml_audio_thread, &QThread::finished, m_player_audio, &QObject::deleteLater);
    m_aml_audio_thread->start();

    m_player_subs = new SubtitleDecoderElement;
    m_aml_subs_thread = new KvalThread("SubtitleDecoderElement");
    m_player_subs->moveToThread(m_aml_subs_thread);
    m_aml_subs_thread->setObjectName("SubtitleDecoderElement");
    connect(m_aml_subs_thread, &QThread::finished, m_player_subs, &QObject::deleteLater);
    m_aml_subs_thread->start();


    m_thread = new KvalThread("PP_MediaPlayer");
    m_thread->setObjectName("PP_MediaPlayer");
    // Move to a new thread.
    moveToThread(m_thread);
    connect(this, SIGNAL(finished()), m_thread, SLOT(quit()), Qt::DirectConnection);
    connect(m_thread, &QThread::finished, this, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    connect(m_aml_reader,
            SIGNAL (reset_player()),
            this,
            SLOT(onResetPlayer()),
            Qt::DirectConnection);
    connect(m_player_video,
            SIGNAL (reset_player()),
            this,
            SLOT(onResetPlayer()),
            Qt::DirectConnection);
    connect(m_player_subs->SubsRenderer(),
            SIGNAL(showSubs(QString)),
            this,
            SIGNAL(showSubs(QString)),
            Qt::DirectConnection);
    connect(m_player_subs->SubsRenderer(),
            SIGNAL(hideSubs()),
            this,
            SIGNAL(hideSubs()),
            Qt::DirectConnection);

    m_cmpVect = {
            m_player_subs,
            m_player_audio,
            m_player_video,
            m_aml_reader,
            m_aml_reader2};

    g_mediaPlayerRef = this;
    m_thread->start();
}

/**
 * @brief PP_MediaPlayer::~PP_MediaPlayer
 */
PP_MediaPlayer::~PP_MediaPlayer()
{
    cleanup();
}

/**
 * @brief PP_MediaPlayer::filename
 * @return
 */
QString PP_MediaPlayer::filename()
{
   return m_filename;
}

/**
 * @brief PP_MediaPlayer::streams
 * @return
 */
QStringList PP_MediaPlayer::streams()
{
   // TODO: Reimplement.
   return QStringList();
}

/**
 * @brief PP_MediaPlayer::setFilename Sets the filename
 *
 * @param filename
 * @return
 */
bool PP_MediaPlayer::setFilename(QString filename, QString filename2, bool loop)
{
    QMutexLocker locker(&m_sendCmd);
    if (!checkCurrentThread())
        return false;

    m_loop = loop;
    if(loop)
    {
        if(m_aml_reader)
            m_aml_reader->activateLoop();
        if(m_aml_reader2)
            m_aml_reader2->activateLoop();
    }
    bool open_status=   (m_dualStream) ?
                        setFilenameInt(filename, filename2) :
                        setFilenameInt(filename);
    if(open_status == false)
    {
        LOG_INFO(LOG_TAG, "Open failed %u");
        QMetaObject::invokeMethod(this, "open_failed");
    }
    return open_status;
}

/**
 * @brief PP_MediaPlayer::open_failed
 */
void PP_MediaPlayer::open_failed()
{
    INV();
    LOG_WARNING(LOG_TAG, "Open file failed Q_EMIT finished signal ...");
    closePlayers();
    Q_EMIT finished();
    OUTV();
}

/**
 * @brief PP_MediaPlayer::extractStreamInfos
 * @return
 */
QVariantMap PP_MediaPlayer::extractStreamInfos()
{
    INV();
    LOG_INFO(LOG_TAG, "Extract Stream Informations...");
    QStringList audio_langs;
    QStringList subs_langs;

    auto audio_aml_reader = (m_dualStream) ? m_aml_reader2 : m_aml_reader;
    //Gather default in play values
    if(m_has_video)
    {
        VideoPinInfo * videoinfo =
            (VideoPinInfo *)m_aml_reader->GetHints(MediaCategoryEnum::Video);
        m_hints_video = videoinfo->hints;
    }
    if(m_has_audio)
    {
        AudioPinInfo * audioinfo=
                (AudioPinInfo *)audio_aml_reader->GetHints(MediaCategoryEnum::Audio);

        m_hints_audio = audioinfo->hints;
    }
    if(m_has_subtitle)
    {
        SubtitlePinInfo * subsinfo =
        (SubtitlePinInfo *)m_aml_reader->GetHints(MediaCategoryEnum::Subtitle);
        m_hints_subs = subsinfo->hints;
    }

    //Create variant map values
    QVariantMap hints_value;
    hints_value["resolution"] = "";
    hints_value["bitrate"] = "- Kbps";
    hints_value["aspect"] = "";
    hints_value["Vcodec"] = "";
    hints_value["Acodec"] = "";
    hints_value["Alang"] = "";
    hints_value["Scodec"] = "";
    hints_value["Slang"] = "";
    hints_value["length"] = "";
    hints_value["audioNbr"] = 0;
    hints_value["subsNbr"] = 0;

    LOG_INFO(LOG_TAG, "------------------------------------------------");
    if(m_has_video)
    {
        hints_value["resolution"] = QString("%1").arg(m_hints_video.width) + "x" +
                                    QString("%1").arg(m_hints_video.height);

        if(m_hints_video.bitrate)
        {
            int bitrate = (int)m_hints_video.bitrate / 1024;
            hints_value["bitrate"] = QString("%1").arg(bitrate) + " Kbps";
        }
        hints_value["aspect"] = QString("%1").arg(m_hints_video.aspect);
        hints_value["Vcodec"] = m_hints_video.codec_name.c_str();
        LOG_INFO(LOG_TAG, "width [%d]", m_hints_video.width);
        LOG_INFO(LOG_TAG, "height [%d]", m_hints_video.height);
        LOG_INFO(LOG_TAG, "bitrate [%d]", m_hints_video.bitrate);
        LOG_INFO(LOG_TAG, "aspect Ratio [%f]", m_hints_video.aspect);
        LOG_INFO(LOG_TAG, "videoCodecName [%s]", m_hints_video.codec_name.c_str());
    }
    if(m_has_audio)
    {
        for(int i=0; i<audio_aml_reader->AudioStreamCount();i++)
        {
            CAMLStreamInfo * hints_audio =
                    audio_aml_reader->GetHintsIndex(MediaCategoryEnum::Audio, i);
            if(hints_audio->languageStr[0])
            {
                audio_langs.append(tr(hints_audio->languageStr));
            }
            else
            {
                audio_langs.append(QString(tr("Piste Audio")) + QString(" %1").arg(i+1));
            }
        }
        LOG_INFO(LOG_TAG, "audio_langs Count [%u]", audio_langs.length());
        hints_value["Acodec"] = m_hints_audio.codec_name.c_str();
        hints_value["Alang"] = m_hints_audio.language;
        LOG_INFO(LOG_TAG, "audioCodecName [%s]", m_hints_audio.codec_name.c_str());
        LOG_INFO(LOG_TAG, "audioLanguage [%s]", m_hints_audio.language);
    }
    else
    {
        audio_langs.append(tr("Audio Unavailable"));
    }
    if(m_has_subtitle)
    {
        for(int i=0; i<m_aml_reader->SubtitleStreamCount();i++)
        {
            CAMLStreamInfo * hints_subs =
                    m_aml_reader->GetHintsIndex(MediaCategoryEnum::Subtitle,i);
            if(hints_subs->languageStr[0])
            {
                subs_langs.append(tr(hints_subs->languageStr));
            }
            else
            {
                subs_langs.append(tr("Piste Sous-titres")  + QString(" %1").arg(i+1));
            }
        }
        subs_langs.append(tr("Deactivate"));

        hints_value["Scodec"] = m_hints_subs.codec_name.c_str();
        hints_value["Slang"] = m_hints_subs.language;
        LOG_INFO(LOG_TAG, "subsCodecName [%s]", m_hints_subs.codec_name.c_str());
        LOG_INFO(LOG_TAG, "subsLanguage [%s]", m_hints_subs.language);
    }
    else
    {
        subs_langs.append(tr("Substitles Unavailable"));
    }
    LOG_INFO(LOG_TAG, "streamLength [%d]", m_aml_reader->GetStreamLength());
    QTimeSpan streamDuration;
    streamDuration.setFromMSecs(m_aml_reader->GetStreamLength());
    QString streamDurationStr = streamDuration.toString("hh:mm:ss");
    LOG_INFO(LOG_TAG, "streamDuration: %s", qPrintable(streamDurationStr));
    hints_value["length"] = streamDurationStr;
    LOG_INFO(LOG_TAG, "------------------------------------------------");


    hints_value["audioNbr"] = audio_langs.length();
    hints_value["subsNbr"] = subs_langs.length();
    hints_value["audioTracks"] = audio_langs;
    hints_value["subsTracks"] = subs_langs;

    OUTV();
    return hints_value;
}

/**
 * @brief PP_MediaPlayer::setFilenameInt
 * @param filename
 * @return
 */
inline bool PP_MediaPlayer::setFilenameInt(QString filename, QString filename2)
{
    INV("filename: %s, filename2: %s", qPrintable(filename), qPrintable(filename2));
    switch (m_state) {
        case STATE_INACTIVE:
            break;
        case STATE_STOPPED:
            break;
        case STATE_PAUSED:
        case STATE_PLAYING:
            OUTV("false");
            return false;
    }

    QUrl url(filename);
    if (url.isLocalFile() && filename.startsWith("file://"))
    {
        m_isLocalFile = true;
        filename = url.path();
    }
    LOG_INFO(LOG_TAG, "open URL [%s]", qPrintable(filename));

    std::string avOptions;
    try {
        if (!m_aml_reader->Open(filename.toStdString(), avOptions))
        {
            LOG_ERROR(LOG_TAG, "Failed to open source.");
            return false;
        }
    } catch(Exception &e){
        LOG_ERROR(LOG_TAG,
                    "Exception while opening reader %s", e.what());
        OUTV("false");
        return false;
    }


    if(m_dualStream && m_aml_reader2)
    {
        try {
            if (!m_aml_reader2->Open(filename2.toStdString(), ""))
            {
                LOG_ERROR(LOG_TAG, "Failed to open source2.");
                return false;
            }
        } catch(Exception &e){
            LOG_ERROR(LOG_TAG,
                        "Exception while opening reader %s", e.what());
            OUTV("false");
            return false;
        }
    }

    m_filename = filename;

    m_has_video = m_aml_reader->VideoStreamCount();
    m_has_audio =   m_audio_index_use < 0 ?
                    false :
                    ( (m_dualStream) ?  m_aml_reader2->AudioStreamCount() :
                                        m_aml_reader->AudioStreamCount() );

    LOG_INFO(LOG_TAG,
             "videoStreamCount: %d, AudioStreamCount: %d",
             m_has_video,
             m_has_audio);

    AudioPinInfo * audioinfo = nullptr;
    VideoPinInfo * videoinfo = nullptr;
    if(m_has_video)
    {
        LOG_INFO(LOG_TAG, "Set Active video Stream to 0");
        m_aml_reader->SetActiveStream(MediaCategoryEnum::Video, 0);
        videoinfo =
            (VideoPinInfo *)m_aml_reader->GetHints(MediaCategoryEnum::Video);
        m_hints_video =videoinfo->hints;
    }
    if(m_has_audio)
    {
        LOG_INFO(LOG_TAG, "Set Active audio Stream to %d", m_audio_index_use);
        (m_dualStream) ?
        m_aml_reader2->SetActiveStream(MediaCategoryEnum::Audio, m_audio_index_use) :
        m_aml_reader->SetActiveStream(MediaCategoryEnum::Audio, m_audio_index_use) ;
        audioinfo= (m_dualStream) ?
                    (AudioPinInfo *)m_aml_reader2->GetHints(MediaCategoryEnum::Audio) :
                    (AudioPinInfo *)m_aml_reader->GetHints(MediaCategoryEnum::Audio);
        m_hints_audio =audioinfo->hints;
    }

    SubtitlePinInfo * subsinfo = nullptr;
    m_has_subtitle  = m_aml_reader->SubtitleStreamCount();
    if(m_has_subtitle)
    {
        LOG_INFO(LOG_TAG, "Set Active subs Stream to %d", m_subs_index_use);
        m_aml_reader->SetActiveStream(MediaCategoryEnum::Subtitle,
                                      m_subs_index_use);
        subsinfo=
        (SubtitlePinInfo *)m_aml_reader->GetHints(MediaCategoryEnum::Subtitle);
        m_hints_subs = subsinfo->hints;
    }

    if( ( m_hints_video.width < 1 ||
          m_hints_video.height < 1 ) &&
        ( m_has_video ) )
    {
        LOG_ERROR(LOG_TAG, "Error Video Size, !!!");
        OUTV("false");
        return false;
    }

    if (m_has_video)
    {
        LOG_INFO(LOG_TAG, "Opening Video Player...");
        try {
            if (!m_player_video->Open(
                        videoinfo,              /* video info hints */
                        m_av_clock,            /* Created clock obj */
                        m_aml_video_clk_thread, /* thread clk obj */
                        m_aml_reader,           /* reader pointer */
                        true                    /* auto flush disabler */
                ))
            {
                LOG_ERROR(LOG_TAG, "Failed to open Video Player");
                OUTV("false");
                return false;
            }
        } catch(Exception &e){
            LOG_ERROR(LOG_TAG,
                      "Exception while opening video player %s", e.what());
            OUTV("false");
            return false;
        }
    }
    if(m_has_audio)
    {
        LOG_INFO(LOG_TAG, "Opening Audio Player...");
        try {
            if (!m_player_audio->Open(
                        audioinfo,        /* audio info hints */
                        m_av_clock,       /* clock obj */
                        (m_dualStream) ? m_aml_reader2 : m_aml_reader,     /* reader obj */
                        "default"         /* Output playable hardware */
            ))
            {
                LOG_ERROR(LOG_TAG, "Failed to Open Audio Player");
                OUTV("false");
                return false;
            }
        } catch(Exception &e){
            LOG_ERROR(LOG_TAG,
                      "Exception while opening audio player %s", e.what());
            OUTV("false");
            return false;
        }
    }
    if(m_has_subtitle)
    {
        LOG_INFO(LOG_TAG, "Opening Subtitle Player...");
        try {
            if (!m_player_subs->Open(
                            m_aml_reader->SubtitleStreamCount(), /* subs stream count */
                            subsinfo,         /* Subtitle info hints */
                            m_av_clock,       /* clock obj */
                            m_aml_reader      /* reader obj */
                ))
            {
                LOG_ERROR(LOG_TAG, "Failed to Open subs Player");
                OUTV("false");
                return false;
            }
        } catch(Exception &e){
            LOG_ERROR(LOG_TAG,
                      "Exception while opening subs player %s", e.what());
            OUTV("false");
            return false;
        }

    }

    setState(STATE_STOPPED);
    LOG_INFO(LOG_TAG, "Decoders were well opened...");
    OUTV("true");
    return true;
}

/**
 * @brief PP_MediaPlayer::seekStartPlay
 * @param position
 */
void PP_MediaPlayer::seekStartPlay(qint64 position)
{
    m_seekStart = position;
    return;
}

/**
 * @brief PP_MediaPlayer::switchAudio
 * @param index
 */
void PP_MediaPlayer::switchAudio(int index)
{
    INV();
    int audio_count = (m_dualStream) ?
                      m_aml_reader2->AudioStreamCount() :
                      m_aml_reader->AudioStreamCount();
    if(audio_count <= 1 || index > audio_count)
    {
        LOG_INFO(LOG_TAG, "Only one Audio track available");
        return;
    }
    m_audio_index_use = index;

    (m_dualStream) ?
    m_aml_reader2->SetActiveStream(MediaCategoryEnum::Audio,m_audio_index_use) :
    m_aml_reader->SetActiveStream(MediaCategoryEnum::Audio,m_audio_index_use);
    this->seek(0, false);
    OUTV();
}

/**
 * @brief PP_MediaPlayer::switchSubs
 * @param index
 */
void PP_MediaPlayer::switchSubs(int index)
{
    INV();
    int subs_count = m_aml_reader->SubtitleStreamCount();
    if(subs_count <= 1 || index > subs_count)
    {
        LOG_INFO(LOG_TAG, "Only one Subs track available");
        return;
    }
    m_subs_index_use = index;
    m_aml_reader->SetActiveStream(MediaCategoryEnum::Subtitle, m_subs_index_use);
    this->seek(0, false);
    OUTV();
}

/**
 * @brief PP_MediaPlayer::onResetPlayer
 */
void PP_MediaPlayer::onResetPlayer()
{
    INV();
    LOG_INFO(LOG_TAG, "Reset player request catched...");
    if(m_flush_asked)
        return;

    m_flush_asked = true;
    OUTV();
}

/**
 * @brief PP_MediaPlayer::play
 * @return
 */
bool PP_MediaPlayer::play()
{
    QMutexLocker locker(&m_sendCmd);
    if (!checkCurrentThread())
        return false;

    switch (m_state)
    {
        case STATE_INACTIVE:
            OUTV("true");
            return true;
        case STATE_PAUSED:
            break;
        case STATE_PLAYING:
            OUTV("true");
            return true;
        case STATE_STOPPED:
        {
            setState(STATE_PLAYING);
            LOG_INFO(LOG_TAG, "Starting thread.");
            OUTV("true");
            return QMetaObject::invokeMethod(this, "mediaDecoding");
        }
        default:
            OUTV("false");
            return false;
    }

    setState(STATE_PLAYING);
    SetDecodersState(Play);

    LOG_INFO(LOG_TAG, "Play command issued.");
    OUTV("true");
    return true;
}

/**
 * @brief PP_MediaPlayer::stop
 * @return
 */
bool PP_MediaPlayer::stop()
{
    INV();
    LOG_INFO(LOG_TAG, "Stop Command...");
    m_sendCmd.lock();
    if (!checkCurrentThread() || !m_aml_reader)
    {
        LOG_INFO(LOG_TAG, "Players already closed ...");
        m_sendCmd.unlock();
        OUTV("false");
        return false;
    }

    switch (m_state)
    {
        case STATE_INACTIVE:
            m_sendCmd.unlock();
            OUTV("false");
            return false;
        case STATE_PAUSED:
        case STATE_PLAYING:
            break;
        case STATE_STOPPED:
            m_sendCmd.unlock();
            OUTV("true");
            return true;
        default:
            OUTV("false");
            m_sendCmd.unlock();
            return false;
    }

    //Call the stopExec here to unlock any stucked av_lib call.
    if(m_aml_reader) m_aml_reader->stopExec();
    if(m_aml_reader2) m_aml_reader2->stopExec();

    m_sendCmd.unlock();


    //Stop main decoding thread
    m_pendingStop = true;

    m_mutexPending.lock();
    if(m_pendingStop)
    {
        LOG_INFO(LOG_TAG, "Wait for the Stop command to finish.");
        m_waitPendingCommand.wait(&m_mutexPending);
    }
    m_mutexPending.unlock();

    Q_EMIT finished();
    LOG_INFO(LOG_TAG, "Out Stop...");
    OUTV("true");
    return true;
}

/**
 * @brief PP_MediaPlayer::pause
 * @return
 */
bool PP_MediaPlayer::pause()
{
    INV();
    LOG_INFO(LOG_TAG, "Pause Command Issue...");
    QMutexLocker locker(&m_sendCmd);
    if (!checkCurrentThread())
        return false;

    switch (m_state)
    {
        case STATE_INACTIVE:
        case STATE_STOPPED:
            OUTV("true");
            return true;
        case STATE_PAUSED:
            OUTV("true");
            return true;
        case STATE_PLAYING:
          break;
        default:
            OUTV("false");
            return false;
    }

    setState(STATE_PAUSED);
    SetDecodersState(Pause);

    LOG_INFO(LOG_TAG, "Pause command issued.");

    OUTV("true");
    return true;
}

void PP_MediaPlayer::pvrseek()
{
    LOG_INFO(LOG_TAG, "Pvr Seek");
    m_pvrSeek = true;
}

/**
 * @brief PP_MediaPlayer::seek
 * @param position
 * @return
 */
bool PP_MediaPlayer::seek(qint64 position, bool isJump, bool isInternalJump)
{
    LOG_INFO(LOG_TAG, "Seek %lldms.", position);
    QMutexLocker locker(&m_seekMtx);
    qint64 newPosition = position;
    qint64 currentTime = 0;
    if (!isJump)
    {
        currentTime = streamPosition()/1000;
        LOG_INFO(LOG_TAG, "currentTime: %lld", currentTime);
        newPosition = currentTime + position;
        LOG_INFO(LOG_TAG, "newPosition: %lld", newPosition);
    }

    m_dec_state = Pause;
    setState(STATE_PAUSED);
    if(m_aml_reader)
    {
        m_aml_reader->seekRequest();
        m_aml_reader->SetState(Pause);
    }
    if(m_aml_reader2)
    {
        m_aml_reader2->seekRequest();
        m_aml_reader2->SetState(Pause);
    }

    if (m_player_audio && m_has_audio)
    {
        m_player_audio->SetState(Pause);
        if(m_player_audio->audioDecoder() && m_has_audio)
            m_player_audio->audioDecoder()->SetState(Pause);
    }


    if (m_player_video && m_has_video)
        m_player_video->SetState(Pause);

    if (m_player_subs && m_has_subtitle)
    {
        LOG_DEBUG(LOG_TAG, "Seek: subtitle pause.");
        if(m_has_subtitle)
            m_player_subs->SetState(Pause);

        if(m_player_subs->SubsRenderer())
            m_player_subs->SubsRenderer()->SetState(Pause);
    }

    if (m_player_audio && m_has_audio)
    {
        m_player_audio->Flush();
        if(m_player_audio->audioDecoder() && m_has_audio)
            m_player_audio->audioDecoder()->Flush();
    }

    if (m_player_video && m_has_video)
        m_player_video->Flush();

    if (m_player_subs && m_has_subtitle)
    {
        LOG_DEBUG(LOG_TAG, "Seek: subtitle flush.");
        m_player_subs->Flush();

        if(m_player_subs->SubsRenderer())
            m_player_subs->SubsRenderer()->Flush();

    }

    if(m_aml_reader)
    {
        if(isInternalJump)
        {
            LOG_INFO(LOG_TAG, "reader Reset.");
            m_aml_reader->Reset();
        }
        else
        {
            LOG_INFO(LOG_TAG, "reader flush.");
            m_aml_reader->Flush();
            LOG_INFO(LOG_TAG, "Seek: source seek.");
            m_aml_reader->Seek(newPosition);
        }
    }
    if(m_aml_reader2)
    {
        if(isInternalJump)
        {
            LOG_INFO(LOG_TAG, "reader Reset.");
            m_aml_reader2->Reset();
        }
        else
        {
            m_aml_reader2->Flush();
            LOG_INFO(LOG_TAG, "Seek: source2 seek.");
            m_aml_reader2->Seek(newPosition);
        }
    }

    m_dec_state = Play;
    if (m_player_video && m_has_video)
        m_player_video->SetState(Play);

    if (m_player_audio && m_has_audio)
    {
        m_player_audio->SetState(Play);
        if(m_player_audio->audioDecoder() && m_has_audio)
            m_player_audio->audioDecoder()->SetState(Play);
    }

    if (m_player_subs && m_has_subtitle)
    {
        m_player_subs->SetState(Play);

        if(m_player_subs->SubsRenderer())
            m_player_subs->SubsRenderer()->SetState(Play);
    }

    if(m_aml_reader)
    {
        m_aml_reader->SetState(Play);
        m_aml_reader->seekRequestEnd();
    }
    if(m_aml_reader2)
    {
        m_aml_reader2->SetState(Play);
        m_aml_reader2->seekRequestEnd();
    }

    setState(STATE_PLAYING);

    if(!m_flush_asked)
    {
        //Inform MMI the end of the seek event
        Q_EMIT videoSeeked();
    }

    OUTV("true");
    return true;
}

/**
 * @brief PP_MediaPlayer::SetDecodersState
 * @param value
 */
void PP_MediaPlayer::SetDecodersState(MediaState value)
{
    INV("value: %u", value);

    QMutexLocker locker(&m_seekMtx);

    //Set Global decoding state
    m_dec_state = value;

    if (m_player_video && m_has_video)
        m_player_video->SetState(value);

    if (m_aml_reader)
        m_aml_reader->SetState(value);

    if (m_aml_reader2)
        m_aml_reader2->SetState(value);

    if(m_player_audio->audioDecoder() && m_has_audio)
        m_player_audio->audioDecoder()->SetState(value);

    if (m_player_audio && m_has_audio)
        m_player_audio->SetState(value);

    if (m_player_subs && m_has_subtitle)
    {
        m_player_subs->SetState(value);

        if(m_player_subs->SubsRenderer())
            m_player_subs->SubsRenderer()->SetState(value);
    }

    if(m_aml_reader)
    {
        m_aml_reader->seekRequestEnd();
    }
    OUTV();
}
/**
 * @brief PP_MediaPlayer::jump
 * @param position
 * @return
 */
bool PP_MediaPlayer::jump(qint64 position)
{
    LOG_INFO(LOG_TAG, "Jump %lldms.", position);
    m_seekStart = position;
    seek(position, true);
    return true;
}

/**
 * @brief PP_MediaPlayer::streamPosition
 * @return
 */
qint64 PP_MediaPlayer::streamPosition()
{
    double result;

    if (m_player_audio && m_has_audio)
    {
        result = m_player_audio->audioDecoder()->Clock();
        LOG_INFO(LOG_TAG, "audio position: %f", result);
    }
    else if (m_player_video && m_has_video)
    {
        result = m_player_video->Clock();
        LOG_INFO(LOG_TAG, "video position: %f", result);
    }
    else
    {
        result = 0;
    }

    return (qint64)result*1000;
}

/**
 * @brief PP_MediaPlayer::enableAutoRestart
 */
void PP_MediaPlayer::enableAutoRestart()
{
    LOG_INFO(LOG_TAG,"Enable interruption restart...");
    m_enable_auto_restart = true;
}

/**
 * @brief PP_MediaPlayer::setVolume
 * @param volume
 */
void PP_MediaPlayer::setVolume(long volume)
{
    LOG_INFO(LOG_TAG, "volume: %lld", volume);
    if(m_player_audio  && m_has_audio)
    {
        m_player_audio->audioDecoder()->SetAlsaVolume(volume);
    }
    OUTV();
}

/**
 * @brief PP_MediaPlayer::volume
 * @return
 */
long PP_MediaPlayer::volume()
{
    if(m_player_audio  && m_has_audio)
    {
        return m_player_audio->audioDecoder()->GetVolume();
    }
    return 0;
}

/**
 * @brief PP_MediaPlayer::setMute
 * @param muted
 */
void PP_MediaPlayer::setMute(bool muted)
{
    if(m_player_audio  && m_has_audio)
    {
        m_player_audio->audioDecoder()->SetAlsaSwitchMute(muted);
    }
}

/**
 * @brief PP_MediaPlayer::muted
 * @return
 */
bool PP_MediaPlayer::muted()
{
    if(m_player_audio  && m_has_audio)
    {
        return m_player_audio->audioDecoder()->GetMuted();
    }
    return false;
}

/**
 * @brief PP_MediaPlayer::IsEndOfStream
 * @return
 */
inline bool PP_MediaPlayer::IsEndOfStream()
{
    INV();
    QMutexLocker locker(&m_seekMtx);
    bool result;

    bool audioIsIdle = true;
    if(m_player_audio  && m_has_audio)
    {
        if (m_player_audio->audioDecoder())
        {
            if (m_player_audio->audioDecoder()->State() != Pause)
                audioIsIdle = false;
        }
    }
    bool videoIsIdle = true;
    if (m_player_video && m_has_video)
    {
        if (m_player_video->State() != Pause)
            videoIsIdle = false;
    }
    if (m_dec_state != Pause && audioIsIdle && videoIsIdle)
        result = true;

    else result = false;

    OUTV("%u", result);
    return result;
}

/**
 * @brief PP_MediaPlayer::mediaDecoding
 */
void PP_MediaPlayer::mediaDecoding()
{
    INV();
    LOG_INFO(LOG_TAG, "Decoding thread started.");

    //Store the current stream length in case of broken connexion
    qint64 current_stream_length = streamLength();
    qint64 breakPosition = 0;

    if(m_isLocalFile || m_enable_auto_restart)
    {
        if (m_seekStart != 0 && m_aml_reader->CanSeek())
        {
            LOG_INFO(LOG_TAG, "Start video at %fs", (double)m_seekStart);
            seek(m_seekStart, true);
        }
    }

    SetDecodersState(Play);

    Q_EMIT playbackStarted();

    while (!m_pendingStop)
    {
        if (IsEndOfStream())
        {
            if(m_loop)
            {
                seek(0, true, true);
            }
            else
            {
                breakPosition = streamPosition();
                break;
            }
        }
        if(m_pvrSeek)
        {
            seek(0, true, true);
            m_pvrSeek = false;
        }
        if(m_flush_asked)
        {
            seek(0, false, false);
            m_flush_asked=false;
        }
        else
        {
            usleep(100);
        }
    }
    LOG_INFO(LOG_TAG, "Previous Decoding thread terminated Cleaning up...");

    closePlayers();

    setState(STATE_STOPPED);

    if(!m_pendingStop)
    {
        if(m_enable_auto_restart) /* || m_isLocalFile */
        {
            //If differences is 10 seconds consider an interruption
            if(current_stream_length - breakPosition > 10000)
            {
                LOG_INFO(LOG_TAG,
                "playback interrupted stream_length [%lu] breakPosition [%lu]",
                current_stream_length, breakPosition);
                Q_EMIT playbackInterrupted(breakPosition);
            }
            else
            {
                LOG_INFO(LOG_TAG, "playback Completed !!");
                Q_EMIT playbackCompleted();
            }
        }
        else
        {
            LOG_INFO(LOG_TAG, "playback interrupted !!");
            Q_EMIT playbackInterrupted(0);
        }
    }
    else
    {
        LOG_INFO(LOG_TAG, "playback completed !!");
        Q_EMIT playbackCompleted();
    }

    m_mutexPending.lock();
    if (m_pendingStop)
    {
        LOG_INFO(LOG_TAG, "Wake the Stop command.");
        m_pendingStop = false;
        m_waitPendingCommand.wakeAll();
    }
    else
    {
        Q_EMIT finished();
    }
    m_mutexPending.unlock();

    OUTV();
}

/**
 * @brief PP_MediaPlayer::closePlayers
 */
void PP_MediaPlayer::closePlayers()
{
    INV();
    QMutexLocker locker(&m_sendCmd);

    if(m_aml_reader && !m_pendingStop)
    {
        m_aml_reader->stopExec();
    }

    if(m_aml_reader2 && !m_pendingStop)
    {
        m_aml_reader2->stopExec();
    }

    for (auto _cmp: m_cmpVect) {
        if (_cmp) {
            LOG_INFO(LOG_TAG, "Close [%s] ", _cmp->name() );
            _cmp->Close();
        }
    }
    m_cmpVect.clear();

    LOG_INFO(LOG_TAG, "Players: Closed");
    OUTV();
}


/**
 * @brief PP_MediaPlayer::setSpeed
 * @param iSpeed
 */
inline void PP_MediaPlayer::setSpeed(int iSpeed)
{
}

/**
 * @brief PP_MediaPlayer::flushStreams
 * @param pts
 */
void PP_MediaPlayer::flushStreams(double pts)
{
}

/**
 * @brief PP_MediaPlayer::checkCurrentThread
 * @return
 */
inline bool PP_MediaPlayer::checkCurrentThread()
{
   if (QThread::currentThreadId() == m_thread->getThreadId())
   {
      LOG_ERROR(LOG_TAG, "Do not invoke in the object's thread!");
      return false;
   }
   return true;
}

/**
 * @brief PP_MediaPlayer::videoBitRate
 * @return
 */
int PP_MediaPlayer::videoBitRate()
{
    if (!m_aml_reader)
       return 0;

    LOG_INFO(LOG_TAG, "bitrate [%d]", m_aml_reader->GetVideoBitrate());
    return m_aml_reader->GetVideoBitrate();
}

/**
 * @brief PP_MediaPlayer::streamLength
 * @return
 */
qint64 PP_MediaPlayer::streamLength()
{
    if (!m_aml_reader)
       return -1;
    return m_aml_reader->GetStreamLength();

}

/**
 * @brief PP_MediaPlayer::streamLength
 * @return
 */
bool PP_MediaPlayer::setAspectRatio(AspectRatio value)
{
    INV("value: %u", value);
    bool ret = false;
    if(!m_has_video || !m_player_video)
    {
        OUTV("false");
        return ret;
    }

    ret = m_player_video->setAspectRatio(value);
    OUTV("%u", ret);
    return ret;
}

/**
 * @brief PP_MediaPlayer::cleanup
 */
void PP_MediaPlayer::cleanup()
{
   LOG_INFO(LOG_TAG, "Cleaning up...");
    for (auto &_thr: m_threadVec) {
        if (*_thr) {
            LOG_INFO(LOG_TAG, "Clean up [%p] ", (*_thr)->getThreadId());
            (*_thr)->quit();
            (*_thr)->wait();
            delete *_thr;
        }
    }
    m_threadVec.clear();
    LOG_INFO(LOG_TAG, "Cleanup done");
}
