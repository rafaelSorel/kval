#ifndef KVALLIVESTREAMMANAGER_H
#define KVALLIVESTREAMMANAGER_H

#include <QObject>
#include <QUrl>
#include "KvalThreadUtils.h"

/**
 * @brief The KvalLiveStreamAbstract class
 */
class KvalLiveStreamAbstract: public QObject
{
    Q_OBJECT
public:
    KvalLiveStreamAbstract() = default;
    virtual ~KvalLiveStreamAbstract() = default;

public Q_SLOTS:
    virtual qreal getTrickWritePercent() = 0;
    virtual void onPlayStream(const QString &) = 0;
    virtual void onStopStream() = 0;
    virtual bool onPauseStream() = 0;
    virtual void onResumeStream() = 0;
    virtual bool onSeekStream(int value, int totalTime, int currentTime) = 0;
};

/**
 * @brief The KvalLiveStreamManager class
 */
class KvalLiveStreamManager : public QObject
{
    Q_OBJECT

public:
    KvalLiveStreamManager(QObject *parent = nullptr);
    virtual ~KvalLiveStreamManager();

    Q_INVOKABLE bool playLiveStream(const QString& liveStreamUri);
    Q_INVOKABLE void stopLiveStream() const;
    Q_INVOKABLE bool pauseLiveStream();
    Q_INVOKABLE void resumeLiveStream();
    Q_INVOKABLE bool seekCurrentStream(int value, int totalTime, int currentTime);
    Q_INVOKABLE qreal getTrickWritePercent();

    enum KvalLiveStreamSupportedFormat {
        KVAL_LIVESTREAM_UNKONW,
        KVAL_LIVESTREAM_HTTP,
        KVAL_LIVESTREAM_RTMP,
        KVAL_LIVESTREAM_FTP,
        KVAL_LIVESTREAM_P2P,
        KVAL_LIVESTREAM_COUNT
    };


public Q_SLOTS:
    void onLiveStreamReady(const QString &);
    void onUnsupportedFormat();

Q_SIGNALS:
    void startStream(const QString &streamLink);
    void httpPauseStream();
    void httpResumeStream();
    void rtmpStreamNotify(const QString &);
    void rtmpStreamStop();
    void httpStreamNotify(const QString &);
    void httpStreamStop() const;
    void playbackFailed();
    void okDiag(const QString& title, const QString& text);
    void yesNoDiag(const QString& title,
                   const QString& text,
                   const QString& nolabel,
                   const QString& yeslabel);
    void yesNoDiagUpdate(const QString& title, const QString& text);
    void yesNoDiagClose();
    void httpSeeked(int pos);
    void flushPlayer();

private:
    struct KvalLiveStreamEngineConf
    {
        KvalLiveStreamEngineConf(
                QSharedPointer<KvalLiveStreamAbstract> engine,
                KvalThread *thread): engine{engine}, thread{thread}
        {}

        ~KvalLiveStreamEngineConf(){
            if(engine){
                engine->onStopStream();
            }
            if(thread){
                thread->stop();
                delete thread;
            }
        }
        QSharedPointer<KvalLiveStreamAbstract> engine;
        KvalThread * thread{nullptr};
    };

    /**
     * @brief The KvalPlayBackLiveSession struct
     */
    struct KvalPlayBackLiveSession
    {
        QSharedPointer<KvalLiveStreamEngineConf> liveStreamObj{nullptr};
        QString originalStreamUri{};
        QUrl parsedStreamUri{};
        KvalLiveStreamSupportedFormat streamFormat{KVAL_LIVESTREAM_UNKONW};
        bool isActiveSession{false};
    };

    bool registerLiveStreamEngines(void);

private:
    using SupportedFormat = KvalLiveStreamSupportedFormat;
    using SharedStreamEngineConf =  QSharedPointer<KvalLiveStreamEngineConf>;
    QScopedPointer<KvalPlayBackLiveSession> m_livePlayBackSession;
    QMap<KvalLiveStreamSupportedFormat, SharedStreamEngineConf> m_liveStreamEngines;
    QMap<QStringList, SupportedFormat> m_uriScheme;
};

/**
 * @brief The KvalLiveStreamManagerElement class
 */
class KvalLiveStreamManagerElement : public KvalLiveStreamManager
{
    Q_OBJECT

public:
    KvalLiveStreamManagerElement(QObject *parent = nullptr):
        KvalLiveStreamManager(parent) {}
    virtual ~KvalLiveStreamManagerElement() = default;

};
#endif // KVALLIVESTREAMMANAGER_H
