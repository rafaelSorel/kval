#ifndef OMX_MEDIAPROCESSORELEMENT_H
#define OMX_MEDIAPROCESSORELEMENT_H

#include <QQuickItem>
#include <QObject>
#include <QThread>
#include "KvalAmlPlayerCore.h"

class KvalMediaPlayerEngine;
extern KvalMediaPlayerEngine * g_mediaPlayerEngine;

/**
 * @brief The PE_MediaPlayerEngine class
 */
class KvalMediaPlayerEngine : public QObject
{
    Q_OBJECT

public:
    KvalMediaPlayerEngine();
    virtual ~KvalMediaPlayerEngine();

    void run();
    QString handleFileSource();
    void cleanup();
    void handleFileStopCurrentDecodeThread();
    static KvalMediaPlayerEngine * getGlobalRef() { return g_mediaPlayerEngine; }
    static bool configureAudioSystem() {
        return AlsaAudioSinkElement::AlsaConfigSoundSystem();
    }

    PP_MediaPlayer* handleFileMediaProcessor()
    {
        return m_player;
    }
    bool isActivePlayerState()
    {
        if(m_player)
            return m_player->isActiveState();

        return false;
    }

    qint64 streamPosition();
    qint64 streamLength();
    long m_seekStart;
    void setVolume(long volume);
    long volume();
    void setMute(bool muted);
    bool muted();
    int  videoBitRate();
    void enableInterruptAutoRestart();
    QVariantMap extractStreamInfos();

public Q_SLOTS:
    void onSetNewSource(QString, QString, bool, bool);
    void onStopCurrentMedia();
    void onPlayCurrentMedia();
    void onPauseCurrentMedia();
    void onSeekCurrentMedia(long, bool);
    void onJumpCurrentMedia(long);
    void onPlaybackInterrupted(long);
    void onPlaybackFailed();
    void onPlaybackCompleted();
    void onGetStreamPosition();
    void onSwitchAudio(int index);
    void onSwitchSubs(int index);
    void setAspectRatio(int value);
    void pvrseek() { if(m_player) m_player->pvrseek(); }
    void setPvrCb(void *cb){ if(m_player) m_player->setPvrCb(cb); }
    void activatePvrTimeout(void) { if(m_player) m_player->activatePvrTimeout(); }

Q_SIGNALS:
    void handleFileReady(unsigned int status);
    void uiStopRequested();
    void playbackStarted();
    void playbackFailed();
    void playbackInterrupted(long);
    void playbackCompleted();
    void streamPositionReady(long, long);
    void videoSeeked();
    void showSubs(QString);
    void hideSubs();
    void finished();
    void mediaPlayerBusy();

private:
    PP_MediaPlayer * m_player;
    QThread * m_thread;
    QString m_handleFileSource;
    QMutex m_mediaProcMtx;
    bool m_interruptAutoRestart;

private Q_SLOTS:
    void createPP_MediaPlayer(bool);
    bool handleFileOpenMedia(QString filepath, QString source2, bool isDualStream, bool isLoop);
};

/**
 * @brief The PE_MediaPlayerElement class
 */
class KvalMediaPlayerElement : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay)

public:
    explicit KvalMediaPlayerElement(QQuickItem* parent = 0);
    ~KvalMediaPlayerElement();

    bool autoPlay();
    void setAutoPlay(bool autoPlay);

public Q_SLOTS:
    Q_INVOKABLE void setSource(QString source,
                               QString source2,
                               bool isDualStream = false,
                               bool isLoop = false);

    Q_INVOKABLE bool play();
    Q_INVOKABLE bool stop();
    Q_INVOKABLE bool pause();
    Q_INVOKABLE bool seek(long position, bool isFlushInternal);
    Q_INVOKABLE bool jump(long position);

    Q_INVOKABLE void setVolume(long volume);
    Q_INVOKABLE long volume();
    Q_INVOKABLE int  videoBitRate();
    Q_INVOKABLE void setMute(bool muted);
    Q_INVOKABLE bool muted();

    Q_INVOKABLE void getCurrentStreamPosition();
    Q_INVOKABLE void setSeekStart(long millis);
    Q_INVOKABLE void switchAudioTrack(int index);
    Q_INVOKABLE void switchSubsTrack(int index);
    Q_INVOKABLE void enableAutoRestart();
    Q_INVOKABLE void setAspectRatio(int value);
    Q_INVOKABLE QVariantMap extractStreamInfos();

Q_SIGNALS:
    void sourceChanged(QString, QString, bool, bool);

    void playbackStarted();
    void playbackFailed();
    void playbackCompleted();
    void playbackInterrupted(long interruptedPosition);
    void stopCurrentMedia();
    void getStreamPosition();
    void pauseCurrentMedia();
    void playCurrentMedia();
    void seekCurrentMedia(long, bool);
    void jumpCurrentMedia(long);
    void streamPositionReady(long position, long length);
    void videoSeeked();
    void showSubs(QString subsLine);
    void hideSubs();
    void switchAudio(int);
    void switchSubs(int);

private:
    QString m_source;
    bool m_autoPlay = true;
    volatile bool m_pendingOpen;
    KvalMediaPlayerEngine * m_mPlayerEngine;
};

#endif // OMX_MEDIAPROCESSORELEMENT_H
