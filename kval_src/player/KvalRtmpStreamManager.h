#include <string>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <semaphore.h>

#include <vector>
#include <QThread>
#include <QObject>
#include <QDebug>
#include <QFile>
#include <QtXml/QDomElement>
#include <QNetworkAccessManager>
#include <QMutex>
#include <QWaitCondition>

#include "KvalThreadUtils.h"
#include "KvalLiveStreamManager.h"

#include <librtmp/log.h>
#include <librtmp/rtmp.h>

using namespace std;

/**
 * @brief The RS_InputStream class
 */
class RS_InputStream : public QObject
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

    RS_InputStream();
    virtual ~RS_InputStream();
    bool    Open(const char* strFile);
    void    Close();
    int64_t Seek(int64_t offset, int whence);
    bool SeekTime(int iTimeInMsec);
    bool Pause(double dTime);
    bool    IsEOF();
    int64_t GetLength();
    void setRtmpOptionValue(QString, QString);
    void stopCurrentThread();

Q_SIGNALS:
    void rtmpStreamReady(QString);
    void rtmpCleanupComplete(RS_InputStream*);

private Q_SLOTS:
    void serverThread();

private:
    int Read(int);
    int createStreamServer();
    void processTcpRequest(int);
    void cleanUp();

    KvalThread *m_thread;
    QMutex m_stop_pending_mutex;
    QWaitCondition m_wait_stop_pending_cmd;
    sem_t m_stop_pending_sem;
    streaming_server_t m_server;
    bool       m_eof;
    bool       m_bPaused;
    char*      m_sStreamPlaying;
    std::vector<std::string> m_optionvalues;
    bool m_rtmpPendingStop;
    int m_port;
    int m_connectedSock;
    QHash<QString, QString> m_rtmpOptionValues;
    RTMP *m_rtmp;
};

/**
 * @brief The RS_RtmpEngine class
 */
class KvalRtmpStreamManager: public KvalLiveStreamAbstract
{
    Q_OBJECT
public:
    KvalRtmpStreamManager();
    virtual ~KvalRtmpStreamManager();

Q_SIGNALS:
    void streamReady(QString);
    void playbackFailed();
    void unsupportedFormat();
    void okDiag(const QString&, const QString&);
    void yesNoDiag(const QString&, const QString&, const QString&, const QString&);
    void yesNoDiagUpdate(const QString&, const QString&);
    void yesNoDiagClose();
    void httpSeeked(int);
    void flushPlayer();

public Q_SLOTS:
    virtual qreal getTrickWritePercent() {return 0.0; }
    virtual void onPlayStream(const QString &);
    virtual void onStopStream();
    virtual bool onPauseStream(){ return false; }
    virtual void onResumeStream() {}
    virtual bool onSeekStream(int value, int totalTime, int currentTime){
        Q_UNUSED(value)
        Q_UNUSED(totalTime)
        Q_UNUSED(currentTime)
        return false;
    }

    void onRtmpCleanupComplete(RS_InputStream*);

private:
    RS_InputStream * m_rtmpMediaProc;
    QString m_outputPipe;
    QString m_previousCreatedPipe;
};
