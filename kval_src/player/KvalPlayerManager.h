#ifndef KVAL_DESKTOPMEDIAPLAYER_H
#define KVAL_DESKTOPMEDIAPLAYER_H

#include <QObject>
#include <QQuickItem>
#include <QHash>
#include <QDebug>
#include <QtAV>
#include "KvalPlayerMetadata.h"
#include "KvalPlayerUtils.h"
#include "quicksubtitle.h"

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------
class KvalMediaPlayerEngine;

//----------------------------------------------------------------------------
// Global var
//----------------------------------------------------------------------------
using namespace QtAV;
extern QPointer<KvalMediaPlayerEngine> g_mediaPlayerEngine;

//----------------------------------------------------------------------------
// Type definitions
//----------------------------------------------------------------------------

// TODO: remove it from here
enum AspectRatio
{
    VIDEO_WIDEOPTION_NORMAL = 0,
    VIDEO_WIDEOPTION_FULL_STRETCH = 1,
    VIDEO_WIDEOPTION_4_3 = 2,
    VIDEO_WIDEOPTION_16_9 = 3,
    VIDEO_WIDEOPTION_NONLINEAR = 4,
    VIDEO_WIDEOPTION_NORMAL_NOSCALEUP = 5,
    VIDEO_WIDEOPTION_4_3_IGNORE = 6,
    VIDEO_WIDEOPTION_4_3_LETTER_BOX = 7,
    VIDEO_WIDEOPTION_4_3_PAN_SCAN = 8,
    VIDEO_WIDEOPTION_4_3_COMBINED = 9,
    VIDEO_WIDEOPTION_16_9_IGNORE = 10,
    VIDEO_WIDEOPTION_16_9_LETTER_BOX = 11,
    VIDEO_WIDEOPTION_16_9_PAN_SCAN = 12,
    VIDEO_WIDEOPTION_16_9_COMBINED = 13,
    VIDEO_WIDEOPTION_MAX = 14
};

//----------------------------------------------------------------------------
// Public Classes
//----------------------------------------------------------------------------

/**
 * @brief The PE_MediaPlayerEngine class
 */
class KvalMediaPlayerEngine : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KvalMediaPlayerEngine);
    Q_PROPERTY(AVPlayer *avplayer READ avplayer NOTIFY playerChanged)

public:
    KvalMediaPlayerEngine(QObject* parent = 0);
    virtual ~KvalMediaPlayerEngine() = default;

    Q_INVOKABLE void setSource(QString source, QString source2,
                               bool isDualStream = false,
                               bool isLoop = false);
    Q_INVOKABLE QVariantMap getCurrentStreamPosition();
    Q_INVOKABLE QVariantMap extractStreamInfos();
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE int videoBitRate();
    Q_INVOKABLE void switchAudioTrack(int index);
    Q_INVOKABLE void switchSubsTrack(int index);
    Q_INVOKABLE bool seek(qint64 position, bool isJump);
    Q_INVOKABLE bool jump(qint64 position);
    Q_INVOKABLE void enableAutoRestart() { m_autoRestart = true; }
    Q_INVOKABLE void setSeekStart(qint64 millis) { m_seekStart = millis; }
    Q_INVOKABLE void setVolume(long value);
    Q_INVOKABLE long volume();
    Q_INVOKABLE void setMute(bool muted);
    Q_INVOKABLE bool muted();

    static bool configureAudioSystem() { return true; }
    static KvalMediaPlayerEngine * getGlobalRef() { return g_mediaPlayerEngine; }

    AVPlayer *avplayer();
    bool isActivePlayerState();
    qint64 streamPosition();
    qint64 streamLength();

private Q_SLOTS:
    void onPlayerError(const QtAV::AVError& err);
    void onMediaLoader();
    void onMediaStatusChanged(QtAV::MediaStatus status);

public Q_SLOTS:
    void onSetNewSource(QString source, QString source2,
                  bool isDualStream = false,
                  bool isLoop = false);

      void setPvrCb(void *cb){
          //@ TODO: Update method
          Q_UNUSED(cb);
      }
      void activatePvrTimeout(){
          //@ TODO: Update method
      }
      void pvrseek(){
          //@ TODO: Update method
      }
//    void setAspectRatio(int value);

Q_SIGNALS:
    void uiStopRequested();
    void playbackStarted();
    void playbackFailed();
    void playbackInterrupted(long interruptedPosition);
    void playbackCompleted();
    void streamPositionReady(long position, long length);
    void videoSeeked();
    void showSubs(QString);
    void hideSubs();
    void mediaPlayerBusy();
    void playerChanged();

private:
    void local_cleanup(void);

private:
    QScopedPointer<AVPlayer> m_player;
    bool m_uiStopRequest{false};
    bool m_autoRestart{false};
    qreal m_volume;
    qint64 m_seekStart{0};
    QString m_source{};
    bool m_dualStream{false};
    QList<AudioTrack> m_audio_tracks;
    QList<SubsTrack> m_subs_tracks;
    QScopedPointer<KvalPlayerMetaData> m_metaData;
};

/**
 * @brief The PE_MediaPlayerElement class
 */
class KvalMediaPlayerElement : public KvalMediaPlayerEngine
{
    Q_OBJECT

public:

    KvalMediaPlayerElement(QObject* parent = 0);
    virtual ~KvalMediaPlayerElement() = default;

};

#endif // KVAL_DESKTOPMEDIAPLAYER_H
