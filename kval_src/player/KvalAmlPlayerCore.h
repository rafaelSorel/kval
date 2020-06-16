#ifndef OMX_MEDIAPROCESSOR_H
#define OMX_MEDIAPROCESSOR_H

#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVariantMap>

#include <stdexcept>
#include <memory>

#include "KvalThreadUtils.h"
#include "amlplayer_lib/AMLReader.h"
#include "amlplayer_lib/AlsaAudioSink.h"
#include "amlplayer_lib/AmlVideoSink.h"
#include "amlplayer_lib/AudioCodec.h"
#include "amlplayer_lib/SubtitleCodecElement.h"

using namespace std;
class OMXPlayerSubtitles;
class OMXSubtitleQml_renderer;
class PP_MediaPlayer;

extern QPointer<PP_MediaPlayer> g_mediaPlayerRef;

/**
 * @brief The PP_MediaPlayer class
 */
class PP_MediaPlayer : public QObject
{
    Q_OBJECT

public:
    enum PP_MediaPlayerState {
        STATE_STOPPED,
        STATE_INACTIVE,
        STATE_PAUSED,
        STATE_PLAYING
    };
    typedef enum {
        CONF_FLAGS_FORMAT_NONE, 
        CONF_FLAGS_FORMAT_SBS, 
        CONF_FLAGS_FORMAT_TB 
    } FORMAT_3D_T;

    enum PP_MediaPlayerError {
        ERROR_CANT_OPEN_FILE,
        ERROR_WRONG_THREAD
    };

    PP_MediaPlayer(bool isDualStream);
    ~PP_MediaPlayer();

    static QPointer<PP_MediaPlayer> getGlobalRef() { return g_mediaPlayerRef; }
    bool setFilename(QString filename, QString filename2 = "", bool loop=false);
    QString filename();
    QStringList streams();

    qint64 streamPosition();
    int videoBitRate();

    bool hasAudio();
    bool hasVideo();
    bool isActiveState()
    {
        return (m_state >= STATE_PAUSED) ? true : false;
    }

    qint64 streamLength();

    inline bool hasSubtitle() {
        return m_has_subtitle;
    }

    PP_MediaPlayerState state();

    void enableAutoRestart();
    void setVolume(long volume);
    long volume();

    void setMute(bool muted);
    bool muted();

    QVariantMap extractStreamInfos();
    QString getCurrentPosition();

    bool setAspectRatio(AspectRatio);

    KvalThread * getMainThread() { return m_thread; }

public Q_SLOTS:
    bool play();
    bool stop();
    bool pause();
    void pvrseek();
    bool seek(qint64 position, bool isJump, bool isInternalJump = false);
    bool jump(qint64 position);
    void seekStartPlay(qint64 position);
    void switchAudio(int index);
    void switchSubs(int index);
    void onResetPlayer();
    void setPvrCb(void* cb){ if(m_aml_reader) m_aml_reader->setPvrCb(cb); }
    void activatePvrTimeout(void) { if(m_aml_reader) m_aml_reader->activatePvrTimeout(); }

Q_SIGNALS:
    void playbackStarted();
    void playbackFailed();
    void playbackCompleted();
    void playbackInterrupted(long);

    void videoSeeked();
    void showSubs(QString);
    void hideSubs();

    void finished();

private Q_SLOTS:
    void mediaDecoding();
    void open_failed();
    void cleanup();

private:
    bool setFilenameInt(QString filename, QString filename2 = "");
    void setState(PP_MediaPlayerState state);
    void setSpeed(int iSpeed);
    void flushStreams(double pts);
    bool checkCurrentThread();
    void SetDecodersState(MediaState value);
    void closePlayers();
    bool IsEndOfStream();

private:
    AMLReader* m_aml_reader{nullptr};
    AMLReader* m_aml_reader2{nullptr};
    AmlVideoSinkClock* m_av_clock{nullptr};
    AmlVideoSinkElement* m_player_video{nullptr};
    AudioCodecElement* m_player_audio{nullptr};
    SubtitleDecoderElement* m_player_subs{nullptr};

    KvalThread* m_thread{nullptr};
    KvalThread* m_aml_reader_thread{nullptr};
    KvalThread* m_aml_reader2_thread{nullptr};
    KvalThread* m_aml_video_thread{nullptr};
    KvalThread* m_aml_video_clk_thread{nullptr};
    KvalThread* m_aml_audio_thread{nullptr};
    KvalThread* m_aml_subs_thread{nullptr};
    QString m_filename;

    volatile PP_MediaPlayerState m_state;

    MediaState m_dec_state{Pause};

    int m_has_video;
    int m_has_audio;
    int m_has_subtitle;

    bool m_pendingStop;
    bool m_flush_asked;
    bool m_pvrSeek = false;

    QMutex m_sendCmd;
    QMutex m_seekMtx;
    QMutex m_mutexPending;
    QWaitCondition m_waitPendingCommand;


    CAMLStreamInfo m_hints_audio;
    CAMLStreamInfo m_hints_video;
    CAMLStreamInfo m_hints_subs;

    double startpts;
    long m_seekStart;
    bool m_isLocalFile;
    bool m_enable_auto_restart; //Typically force the seek start for VOD videos
    long m_Volume;
    int m_audio_index_use;
    int m_subs_index_use;
    bool m_dualStream;
    bool m_loop;
    std::vector<AMLComponent*> m_cmpVect{};
    std::vector<KvalThread**> m_threadVec{};

};

/**
 * @brief PP_MediaPlayer::hasAudio
 * @return 
 */
inline bool PP_MediaPlayer::hasAudio()
{
    return m_has_audio;
}

/**
 * @brief PP_MediaPlayer::hasVideo
 * @return 
 */
inline bool PP_MediaPlayer::hasVideo()
{
    return m_has_video;
}

/**
 * @brief PP_MediaPlayer::state
 * @return 
 */
inline PP_MediaPlayer::PP_MediaPlayerState PP_MediaPlayer::state()
{
    return m_state;
}

/**
 * @brief PP_MediaPlayer::setState
 * @param state
 */
inline void PP_MediaPlayer::setState(PP_MediaPlayerState state)
{
   m_state = state;
}

#endif // OMX_MEDIAPROCESSOR_H
