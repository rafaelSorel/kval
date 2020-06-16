#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
extern "C"
{
#include <libavutil/timestamp.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
}

#include <vector>
#include <QThread>
#include <QObject>
#include <QDebug>
#include <QFile>
#include <QtXml/QDomElement>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>
#include <QMutex>
#include <QWaitCondition>
#include <QNetworkReply>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QQueue>
#include <QFile>

#include "KvalLiveMediaPvr.h"
#include "KvalThreadUtils.h"
#include "KvalLiveStreamManager.h"

using namespace std;

class KvalHttpStreamServer : public QObject
{
    Q_OBJECT
public:
    KvalHttpStreamServer(QObject *parent = 0);
    virtual ~KvalHttpStreamServer();

    QString createServer();
    int send(char *, qint64, bool isFlush=false);
    void close();

Q_SIGNALS:
    void serverReady();
    void playerReadyPvrNotify();

public Q_SLOTS:
    void onNewConnection();
    void txRx();
    void closingClient();

private:
    qint64 bytesAvailable() const;

private:
    QTcpServer * m_server{nullptr};
    QVector<QTcpSocket *> m_socket_list{};
    QMutex m_stop_mutex;
    bool sessionEstablished{false};
};

/**
 * @brief The HS_InputStream class
 */
class KvalHttpStreamWorker : public QObject
{
    Q_OBJECT
public:
    enum
    {
        STREAMING_ACCEPTING,
        STREAMING_IN_PROGRESS,
        STREAMING_STOPPING,
        STREAMING_STOPPED
    };

    typedef struct
    {
        int socket;
        int state;
    } streaming_server_t;

    KvalHttpStreamWorker(const QString &);
    virtual ~KvalHttpStreamWorker();

    void Start();
    void Close();
    void execGetRequest();

Q_SIGNALS:
    void httpStreamInterrupted();
    void httpStreamFailed(bool);
    void finished();
    void httpStreamReady(const QString &);
    void abortSession();
    void okDiag(const QString&, const QString&);
    void yesNoDiag(const QString&, const QString&, const QString&, const QString&);
    void yesNoDiagUpdate(const QString&, const QString&);
    void yesNoDiagClose();
    void httpSeeked(int);
    void flushPlayer();
    void segmentDataAvailable();

public Q_SLOTS:
    qreal getTrickModeWritePercent();
    void onNewLinkAvailable();
    void onGetReqTimeout();
    void onServerReady();
    void onlineStateCheck(bool);
    void downloadStatus(qint64, qint64);
    void onPlayerReadyPvrNotify();
    bool Seek(int value, int totalTime, int currentTime);
    void Resume();
    bool Pause();

private Q_SLOTS:
    void pvr_data_ready();
    void data_ready();
    void finish_get();
    void finish_head();
    void onCurrentSessionError(QNetworkReply::NetworkError);

private:
    void extractUrlInfo();
    void startStreamingProc();
    void cleanUp();

private:
    MP_MediaPvr * m_mediaPvr{nullptr};
    QString m_streamAddrStr{};
    KvalHttpStreamServer * m_http_server{nullptr};
    QTimer * m_getReqTimer{nullptr};
    QNetworkReply * m_network_reply{nullptr};
    QString m_main_url_str{};
    QNetworkAccessManager * m_http_manager{nullptr};
    QNetworkConfigurationManager* m_http_config_manager{nullptr};
    streaming_server_t m_server;
    QMutex m_requestAbortMutex{};
    QThread * m_thread{nullptr};
    bool m_httpPendingStop{};
    int m_retry_count{};
    bool m_headRequested{};
    bool m_gotStreamInfo{};
};

/**
 * @brief The HS_HttpEngine class
 */
class KvalHttpStreamManager: public KvalLiveStreamAbstract
{
    Q_OBJECT

public:
    KvalHttpStreamManager();
    virtual ~KvalHttpStreamManager();

Q_SIGNALS:
    void streamReady(const QString &);
    void playbackFailed();
    void unsupportedFormat();
    void okDiag(const QString&, const QString&);
    void yesNoDiag(const QString&, const QString&, const QString&, const QString&);
    void yesNoDiagUpdate(const QString&, const QString&);
    void yesNoDiagClose();
    void httpSeeked(int);
    void flushPlayer();

public Q_SLOTS:
    virtual qreal getTrickWritePercent();
    virtual void onPlayStream(const QString &);
    virtual void onStopStream();
    virtual bool onPauseStream();
    virtual void onResumeStream();
    virtual bool onSeekStream(int value, int totalTime, int currentTime);
    void onHttpStreamInterrupted();
    void onHttpStreamFailed(bool);

private:
    KvalHttpStreamWorker *m_httpMediaProc{nullptr};
};
