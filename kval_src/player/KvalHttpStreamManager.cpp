#define LOG_ACTIVATED
#include <QFile>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "KvalHttpStreamManager.h"
#include "KvalThreadUtils.h"

#define LOG_SRC HTTPENGINESTR
#include "KvalLogging.h"

/**
 * @brief HS_HttpServer::HS_HttpServer
 * @param parent
 */
KvalHttpStreamServer::KvalHttpStreamServer(QObject *parent) : QObject(parent)
{}

/**
 * @brief HS_HttpServer::~myHTTPserver
 */
KvalHttpStreamServer::~KvalHttpStreamServer()
{
    LOG_INFO(LOG_TAG, ">>>>>>>>> Delete HS_HttpServer");
    while(m_socket_list.size() > 0)
    {
        QTcpSocket * lsocket = m_socket_list.front();
        m_socket_list.pop_front();
        if(lsocket)
        {
            LOG_INFO(LOG_TAG, "socket delete later ...");
            lsocket->deleteLater();
        }
    }
    if(m_server)
    {
        LOG_INFO(LOG_TAG, "delete server ...");
        delete m_server;
        m_server = nullptr;
    }
    LOG_INFO(LOG_TAG, "<<<<<<<<< HS_HttpServer Deleted");
}

/**
 * @brief HS_HttpServer::createServer
 * @return
 */
QString KvalHttpStreamServer::createServer()
{
    m_server = new QTcpServer(this);
    connect(m_server,
            SIGNAL(newConnection()),
            this,
            SLOT(onNewConnection()));

    while(1)
    {
        if(!m_server->listen(QHostAddress::LocalHost))
        {
            LOG_ERROR(LOG_TAG, "Web server could not start");
            continue;
        }
        else
        {
            LOG_INFO(LOG_TAG,
                     "Web server is waiting for a connection on port %d",
                     m_server->serverPort());
            QString streamAddrStr("http://localhost:");
            streamAddrStr.append(QString("%1").arg(m_server->serverPort()));
            LOG_INFO(LOG_TAG, "Streaming addr %s", qPrintable(streamAddrStr));
            return streamAddrStr;
        }
    }
}

/**
 * @brief HS_HttpServer::close
 */
void KvalHttpStreamServer::close()
{
    QMutexLocker locker(&m_stop_mutex);
    LOG_INFO(LOG_TAG, "Close http server...");
    for(int i=0; i<m_socket_list.size();i++)
    {
        m_socket_list[i]->disconnect(this);
        m_socket_list[i]->abort();
    }
    m_server->close();
}

/**
 * @brief HS_HttpServer::onNewConnection
 */
void KvalHttpStreamServer::onNewConnection()
{
    QMutexLocker locker(&m_stop_mutex);
    LOG_INFO(LOG_TAG, "Client connection...");
    QTcpSocket *m_socket = m_server->nextPendingConnection();

    m_socket_list.append(m_socket);
    LOG_INFO(LOG_TAG, "m_socket %p", m_socket);
    LOG_INFO(LOG_TAG, "m_socket_list Size: %d", m_socket_list.size());

    connect(m_socket, SIGNAL(readyRead()), this, SLOT(txRx()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(closingClient()));
    if (!sessionEstablished)
        Q_EMIT serverReady();
    else
        Q_EMIT playerReadyPvrNotify();
    sessionEstablished = true;
    LOG_INFO(LOG_TAG, "Out onNewConnection...");
}

/**
 * @brief HS_HttpServer::txRx
 */
void KvalHttpStreamServer::txRx()
{
    QMutexLocker locker(&m_stop_mutex);
    LOG_INFO(LOG_TAG, "Recv Client Data...");
    LOG_INFO(LOG_TAG, "m_socket_list Size: %d", m_socket_list.size());
    QTcpSocket* readSocket = qobject_cast<QTcpSocket*>(sender());
    QByteArray data = readSocket->read(2048);
    LOG_INFO(LOG_TAG,"Received data from client: \n%s", qPrintable(data.data()));

    readSocket->write("HTTP/1.1 200 OK\r\n");
    readSocket->write("Content-Type: application/octet-stream\r\n");
    readSocket->write("\r\n");
    LOG_INFO(LOG_TAG,"Data sent back ...");
}

/**
 * @brief HS_HttpServer::closingClient
 */
void KvalHttpStreamServer::closingClient()
{
    QMutexLocker locker(&m_stop_mutex);
    QTcpSocket* readSocket = qobject_cast<QTcpSocket*>(sender());
    LOG_INFO(LOG_TAG,
             "Client Disconnected state [%u]...",
             readSocket->state());
    for(int i=0; i < m_socket_list.size();i++)
    {
        if (m_socket_list[i] == readSocket)
        {
            LOG_INFO(LOG_TAG,"Remove client from list");
            m_socket_list.remove(i);
            readSocket->deleteLater();
        }
    }
}

/**
 * @brief HS_HttpServer::send
 * @param ptrArray
 * @param size
 * @return
 */
int KvalHttpStreamServer::send(char * ptrdata, qint64 size, bool isFlush)
{
    QMutexLocker locker(&m_stop_mutex);
    int gWrittenSize = -1;
    for(int i=0; i < m_socket_list.size();i++)
    {
        int writtenSize = -1;
        if (m_socket_list[i]->state() != QAbstractSocket::ConnectedState)
        {
            LOG_INFO(LOG_TAG, "Client Socket not valid yet...");
            continue;
        }
        writtenSize = m_socket_list[i]->write(ptrdata, size);
        if(isFlush)
        {
            LOG_INFO(LOG_TAG, "Flush Client Socket ...");
            while(m_socket_list[i]->flush())
            {
                LOG_INFO(LOG_TAG, "Flush...");
            }
            LOG_INFO(LOG_TAG, "Client Socket Flushed...");
        }
        if(gWrittenSize < writtenSize) gWrittenSize = writtenSize;
    }

    return gWrittenSize;
}

/**
 * @brief HS_InputStream::HS_InputStream
 */
KvalHttpStreamWorker::KvalHttpStreamWorker(const QString &main_url_str):
    m_mediaPvr{new MP_MediaPvr},
    m_streamAddrStr{},
    m_http_server{nullptr},
    m_getReqTimer{nullptr},
    m_network_reply{nullptr},
    m_main_url_str{main_url_str},
    m_http_manager{nullptr},
    m_http_config_manager{nullptr},
    m_server{STREAMING_STOPPED, -1},
    m_requestAbortMutex{},
    m_thread{new KvalThread("HS_InputStream")},
    m_httpPendingStop{false},
    m_retry_count{2},
    m_headRequested{false},
    m_gotStreamInfo{false}
{
    INV("Instantiate HS_InputStream");

    m_mediaPvr->setPvrMode(MEDIA_PVR_TRICK_MODE);
    m_thread->setObjectName("HS_InputStream");

    moveToThread(m_thread);

    connect(this, SIGNAL (finished()), m_thread, SLOT(quit()));
    connect(this, SIGNAL (finished()), this, SLOT(deleteLater()));
    connect(m_thread, SIGNAL (finished()), m_thread, SLOT (deleteLater()));
    connect(m_mediaPvr,
            SIGNAL(okDiag(QString, QString)),
            this,
            SIGNAL(okDiag(QString, QString)));
    connect(m_mediaPvr,
            SIGNAL(yesNoDiag(QString,QString,QString,QString)),
            this,
            SIGNAL(yesNoDiag(QString,QString,QString,QString)));
    connect(m_mediaPvr,
            SIGNAL(yesNoDiagUpdate(QString,QString)),
            this,
            SIGNAL(yesNoDiagUpdate(QString,QString)));
    connect(m_mediaPvr,
            SIGNAL(yesNoDiagClose()),
            this,
            SIGNAL(yesNoDiagClose()));
    connect(m_mediaPvr,
            SIGNAL(httpSeeked(int)),
            this,
            SIGNAL(httpSeeked(int)));
    connect(m_mediaPvr,
            SIGNAL(flushPlayer()),
            this,
            SIGNAL(flushPlayer()));
    connect(m_mediaPvr,
            SIGNAL(dataAvailable()),
            this,
            SLOT(pvr_data_ready()));
    connect(m_mediaPvr,
            SIGNAL(seek(int,int,int)),
            this,
            SLOT(Seek(int,int,int)),
            Qt::QueuedConnection);
    connect(m_mediaPvr,
            SIGNAL(resume()),
            this,
            SLOT(Resume()),
            Qt::QueuedConnection);


    m_thread->start();
    LOG_INFO(LOG_TAG,"Instantiated HS_InputStream");
    OUTV();
}

/**
 * @brief HS_InputStream::~HS_InputStream
 */
KvalHttpStreamWorker::~KvalHttpStreamWorker()
{
    LOG_INFO(LOG_TAG, "HS_InputStream deleted %p", this);
    this->cleanUp();
}

/**
 * @brief HS_InputStream::Start
 */
void KvalHttpStreamWorker::Start()
{
    QMetaObject::invokeMethod(this, "onNewLinkAvailable");
}

/**
 * @brief HS_InputStream::onNewLinkAvailable
 * @param link
 */
void KvalHttpStreamWorker::onNewLinkAvailable()
{
    INV("link: %s", qPrintable(m_main_url_str));
    QMutexLocker locker(&m_requestAbortMutex);
    if(m_httpPendingStop)
    {
        m_server.state = STREAMING_STOPPING;
        if(m_http_server) m_http_server->close();
        if(m_mediaPvr) m_mediaPvr->Close();
        Q_EMIT finished();
        return;
    }

    m_http_config_manager = new QNetworkConfigurationManager;
    m_http_manager = new QNetworkAccessManager;
    m_getReqTimer = new QTimer(this);
    connect(m_getReqTimer, SIGNAL(timeout()), this, SLOT(onGetReqTimeout()));
    connect(m_http_config_manager,
            SIGNAL(onlineStateChanged(bool)),
            this,
            SLOT(onlineStateCheck(bool)));

    LOG_INFO(LOG_TAG, "New Link: %s", qPrintable(m_main_url_str));
    this->extractUrlInfo();
}

/**
 * @brief HS_InputStream::onGetReqTimeout
 */
void KvalHttpStreamWorker::onGetReqTimeout()
{
    QMutexLocker locker(&m_requestAbortMutex);
    LOG_INFO(LOG_TAG, "Head request Timeout Abort!!");

    m_server.state = STREAMING_STOPPING;

    m_network_reply->disconnect();
    m_network_reply->abort();
    m_network_reply->deleteLater();

    if(!m_httpPendingStop)
    {
        Q_EMIT httpStreamFailed(0);
    }
    if(m_http_server)
    {
        m_http_server->close();
    }
    if(m_mediaPvr)
    {
        m_mediaPvr->Close();
    }

    Q_EMIT finished();
}

/**
 * @brief HS_InputStream::extractUrlInfo
 */
void KvalHttpStreamWorker::extractUrlInfo()
{
    LOG_INFO(LOG_TAG, "Check URL Infos...");

    m_getReqTimer->setSingleShot(true);
    m_getReqTimer->setInterval(10000);
    m_getReqTimer->start();

    m_network_reply = m_http_manager->head(QNetworkRequest(QUrl(m_main_url_str)));

    if(!m_network_reply)
    {
        LOG_ERROR(LOG_TAG, "network reply obj null !");
        m_server.state = STREAMING_STOPPING;
        Q_EMIT httpStreamFailed(0);
        Q_EMIT finished();
        return;
    }
    connect(m_network_reply, SIGNAL(finished()), this, SLOT(finish_head()));
}

/**
 * @brief HS_InputStream::finish_head
 */
void KvalHttpStreamWorker::finish_head()
{
    QMutexLocker locker(&m_requestAbortMutex);
    LOG_INFO(LOG_TAG, "finish_head called");
    QString redirectUrl;
    QVariant contentLength;
    qint64 length = 0;
    bool notSupportedStream = false;
    if(m_getReqTimer->isActive())
    {
        m_getReqTimer->stop();
    }

    if(m_httpPendingStop)
    {
        goto error_nothandled;
    }

    if (m_network_reply->error() != QNetworkReply::NoError)
    {
        LOG_WARNING(LOG_TAG, "m_network_reply->error(): %u", m_network_reply->error());
        if(m_headRequested)
        {
            LOG_ERROR(LOG_TAG, "Unable to open stream for info...");
            notSupportedStream = true;
            goto error_nothandled;
        }

        LOG_WARNING(LOG_TAG, "Unable to perform a head request, Try get request...");
        m_headRequested = true;
        m_network_reply->disconnect(this);
        m_network_reply->close();
        m_network_reply->deleteLater();

        m_getReqTimer->setSingleShot(true);
        m_getReqTimer->setInterval(10000);
        m_getReqTimer->start();

        m_network_reply = m_http_manager->get(QNetworkRequest(QUrl(m_main_url_str)));
        if(!m_network_reply)
        {
            LOG_ERROR(LOG_TAG, "network reply obj null !");
            m_server.state = STREAMING_STOPPING;
            Q_EMIT httpStreamFailed(0);
            if(m_http_server) m_http_server->close();
            if(m_mediaPvr) m_mediaPvr->Close();
            Q_EMIT finished();
            return;
        }

        connect(m_network_reply, SIGNAL(finished()), this, SLOT(finish_head()));
        connect(m_network_reply, SIGNAL(readyRead()), this, SLOT(finish_head()));
        return;
    }

    m_headRequested = true;
    m_gotStreamInfo = true;
    LOG_INFO(LOG_TAG, "Check headers...");
    redirectUrl =m_network_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
    contentLength = m_network_reply->header(QNetworkRequest::ContentLengthHeader);
    if (contentLength.isValid())
    {
        length = contentLength.toLongLong();
        LOG_WARNING(LOG_TAG, "content length headers present %llu", length);
        if(length)
        {
            LOG_WARNING(LOG_TAG, "This type of file is not handled yet");
            Q_EMIT okDiag(tr("Stream not fully supported"),
                        tr("This is not a live stream, "
                           "if you wants to play correctly such a stream, "
                           "please use <b>VOD</b> feature or <b>multimedia</b> feature."
                           "<br>In the meantime you will not be able to use trickmode keys (Pause, Play...)."));
            notSupportedStream = true;
            goto error_nothandled;
        }
    }
    //Cleanup old networkreply obj
    m_network_reply->disconnect(this);
    if(m_network_reply->isRunning())
    {
        m_network_reply->abort();
    }
    m_network_reply->close();
    m_network_reply->deleteLater();
    if(!redirectUrl.isEmpty())
    {
        LOG_INFO(LOG_TAG, "URL Redirected: %s", qPrintable(redirectUrl));
        m_main_url_str = redirectUrl;
    }
    this->startStreamingProc();
    return;

error_nothandled:
    m_server.state = STREAMING_STOPPING;
    LOG_INFO(LOG_TAG, "http connection Failed...");
    if(!m_httpPendingStop)
    {
        Q_EMIT httpStreamFailed(notSupportedStream);
    }
    m_network_reply->disconnect(this);
    m_network_reply->close();
    m_network_reply->deleteLater();
    if(m_http_server) m_http_server->close();
    if(m_mediaPvr) m_mediaPvr->Close();
    Q_EMIT finished();
    LOG_INFO(LOG_TAG, "finished emitted...");
    return;
}

/**
 * @brief HS_InputStream::startStreamingProc
 */
void KvalHttpStreamWorker::startStreamingProc()
{
    LOG_INFO(LOG_TAG, "Start Streaming proc: %s", qPrintable(m_main_url_str));
    m_http_server = new KvalHttpStreamServer;
    connect(m_http_server, SIGNAL(serverReady()), this, SLOT(onServerReady()), Qt::QueuedConnection);
    connect(m_http_server,
            SIGNAL(playerReadyPvrNotify()),
            this,
            SLOT(onPlayerReadyPvrNotify()),
            Qt::QueuedConnection);
    QString streamAddrStr = m_http_server->createServer();
    m_streamAddrStr = streamAddrStr;
    if(streamAddrStr.isEmpty())
    {
        LOG_ERROR(LOG_TAG, "Problem creating init streaming server !!!");
        Q_EMIT abortSession();
        return;
    }
    Q_EMIT httpStreamReady(streamAddrStr);
    m_server.state = STREAMING_IN_PROGRESS;
}

/**
 * @brief HS_InputStream::onlineStateCheck
 * @param isOnline
 */
void KvalHttpStreamWorker::onlineStateCheck(bool isOnline)
{
    LOG_INFO(LOG_TAG, "onlineStateChanged %u ...", isOnline);
    if (!isOnline)
    {
        Q_EMIT abortSession();
    }
}

/**
 * @brief HS_InputStream::onServerReady
 */
void KvalHttpStreamWorker::onServerReady()
{
    LOG_INFO(LOG_TAG, "Server Ready, start fetching data...");
    this->execGetRequest();
}

/**
 * @brief HS_InputStream::onPlayerReadyPvrNotify
 */
void KvalHttpStreamWorker::onPlayerReadyPvrNotify()
{
    m_mediaPvr->mediaPlayerReadyNotify();
}

/**
 * @brief HS_InputStream::execGetRequest
 */
void KvalHttpStreamWorker::execGetRequest()
{
    QMutexLocker locker(&m_requestAbortMutex);
    if(m_httpPendingStop)
    {
        LOG_ERROR(LOG_TAG, "Stop requested before starting the streaming server !");
        m_server.state = STREAMING_STOPPING;
        m_mediaPvr->Close();
        m_http_server->close();
        Q_EMIT finished();
        return;
    }

    m_getReqTimer->setSingleShot(true);
    m_getReqTimer->setInterval(10000);
    m_getReqTimer->start();

    m_network_reply = m_http_manager->get(QNetworkRequest(QUrl(m_main_url_str)));
    LOG_INFO(LOG_TAG,"m_network_reply: %p", m_network_reply);

    connect(m_network_reply,
            SIGNAL(error(QNetworkReply::NetworkError)),
            this,
            SLOT(onCurrentSessionError(QNetworkReply::NetworkError)),
            Qt::QueuedConnection);
    connect(m_network_reply,
            SIGNAL(downloadProgress(qint64, qint64)),
            this,
            SLOT(downloadStatus(qint64, qint64)));
    connect(m_network_reply,
            SIGNAL(readyRead()),
            this,
            SLOT(data_ready()));
    connect(m_network_reply,
            SIGNAL(finished()),
            this,
            SLOT(finish_get()),
            Qt::QueuedConnection);
    connect(this,
            SIGNAL(abortSession()),
            m_network_reply,
            SLOT(abort()));
}

void KvalHttpStreamWorker::downloadStatus(qint64 bytesReceived, qint64 bytesTotal)
{
    LOG_DEBUG(LOG_TAG, "bytesReceived: %llu / %llu", bytesReceived, bytesTotal);
}

/**
 * @brief HS_InputStream::pvr_data_ready
 */
void KvalHttpStreamWorker::pvr_data_ready()
{
    if(m_httpPendingStop)
    {
        LOG_WARNING(LOG_TAG, "Stop requested");
        return;
    }

    bool needToFlush;
    QByteArray bufferarray;
    m_mediaPvr->readPvrData(bufferarray, &needToFlush);
    if(!bufferarray.size() && !needToFlush)
    {
        LOG_INFO(LOG_TAG, "Empty Segment bufferSize");
        return;
    }

    if(m_http_server->send(bufferarray.data(), bufferarray.size(), needToFlush) < 0)
    {
        QMutexLocker locker(&m_requestAbortMutex);
        LOG_ERROR(LOG_TAG, "Sending failed");
        m_server.state = STREAMING_STOPPING;
        m_network_reply->abort();
        LOG_ERROR(LOG_TAG, "m_network_reply session aborted ...");
    }
}

/**
 * @brief HS_InputStream::data_ready
 */
void KvalHttpStreamWorker::data_ready()
{
    if(m_getReqTimer->isActive())
    {
        LOG_INFO(LOG_TAG, "data_ready Abort timer...");
        m_getReqTimer->stop();
    }

    if(m_server.state != STREAMING_IN_PROGRESS)
    {
        LOG_WARNING(LOG_TAG, "server not yet ready for streaming");
        return;
    }

    if(m_httpPendingStop)
    {
        LOG_WARNING(LOG_TAG, "Stop requested");
        return;
    }

    QVariant contentLength = m_network_reply->header(QNetworkRequest::ContentLengthHeader);
    if (contentLength.isValid())
    {
        qint64 length = contentLength.toLongLong();
        LOG_WARNING(LOG_TAG, "content length headers present %lld", length);
        if(length)
        {
            LOG_WARNING(LOG_TAG, "This type of file is not handled on live stream");
            Q_EMIT okDiag(tr("Stream not fully supported"),
                        tr("This is not a live stream, "
                           "if you wants to play correctly such a stream, "
                           "please use <b>VOD</b> feature or <b>multimedia</b> feature."
                           "<br>In the meantime you will not be able to use trickmode keys (Pause, Play...)."));
            Q_EMIT httpStreamFailed(true);
            QMutexLocker locker(&m_requestAbortMutex);
            m_server.state = STREAMING_STOPPING;
            m_network_reply->abort();
            return;
        }
    }

    int bufferSize = m_network_reply->bytesAvailable();
    if(!bufferSize)
    {
        LOG_WARNING(LOG_TAG, "Zero size ...");
        return;
    }


    QByteArray bufferarray = m_network_reply->read(bufferSize);

    bool ret = m_mediaPvr->checkMP_MediaPvr(bufferarray);
    if(!ret)
        return;

    LOG_DEBUG(LOG_TAG, "bufferSize: %u", bufferarray.size());
    if(m_http_server->send(bufferarray.data(), bufferarray.size()) < 0)
    {
        QMutexLocker locker(&m_requestAbortMutex);
        LOG_ERROR(LOG_TAG, "Sending failed");
        m_server.state = STREAMING_STOPPING;
        m_network_reply->abort();
        LOG_ERROR(LOG_TAG, "m_network_reply session aborted ...");
    }
}

/**
 * @brief HS_InputStream::getWritePercent
 * @return
 */
qreal KvalHttpStreamWorker::getTrickModeWritePercent()
{
    return m_mediaPvr->getTrickModeWritePercent();
}

/**
 * @brief HS_InputStream::onCurrentSessionError
 */
void KvalHttpStreamWorker::onCurrentSessionError(QNetworkReply::NetworkError)
{
    LOG_ERROR(LOG_TAG,
              "Got a Network error: %s",
              qPrintable(m_network_reply->errorString()));
}

/**
 * @brief HS_InputStream::finish_get
 */
void KvalHttpStreamWorker::finish_get()
{
    LOG_INFO(LOG_TAG, "finish_get called");
    if(m_getReqTimer->isActive())
    {
        m_getReqTimer->stop();
    }

    if (m_network_reply->error() == QNetworkReply::NoError)
    {
        QString redirectUrl =
        m_network_reply->attribute(
                    QNetworkRequest::RedirectionTargetAttribute).toString();
        if(!redirectUrl.isEmpty())
        {
            LOG_INFO(LOG_TAG, "URL Redirected: %s", qPrintable(redirectUrl));
            m_main_url_str = redirectUrl;

            //Cleanup ol networkreply obj
            m_network_reply->close();
            m_network_reply->deleteLater();

            //Execute the new get request
            this->execGetRequest();
            return;
        }
        else
        {
            if(!m_httpPendingStop && m_retry_count)
            {
                LOG_INFO(LOG_TAG, "Give a retry ...");
                m_network_reply->deleteLater();
                this->execGetRequest();
                m_retry_count = m_retry_count - 1;
                return;
            }
        }
    }
    m_server.state = STREAMING_STOPPING;
    if(!m_httpPendingStop)
    {
        LOG_INFO(LOG_TAG, "http connection is interrupted...");
        Q_EMIT httpStreamInterrupted();
        m_network_reply->disconnect();
        m_network_reply->abort();
    }
    m_mediaPvr->Close();
    m_http_server->close();
    m_network_reply->deleteLater();
    Q_EMIT finished();
    LOG_INFO(LOG_TAG, "finished emitted...");
}

/**
 * @brief HS_InputStream::Close
 */
void KvalHttpStreamWorker::Close()
{
    INV();
    LOG_INFO(LOG_TAG, "Close called");
    QMutexLocker locker(&m_requestAbortMutex);
    m_httpPendingStop = true;
    if(m_server.state != STREAMING_IN_PROGRESS)
    {
        LOG_WARNING(LOG_TAG, "Ask for Close, no streaming in progress !!");
        OUTV();
        return;
    }

    LOG_INFO(LOG_TAG, "Abort current http connection...");
    Q_EMIT abortSession();

    OUTV();
}

/**
 * @brief HS_InputStream::Pause
 * @return
 */
bool KvalHttpStreamWorker::Pause()
{
    INV();
    LOG_INFO(LOG_TAG, "Pause called");
    if(m_server.state != STREAMING_IN_PROGRESS)
    {
        LOG_WARNING(LOG_TAG, "Ask for Pause, no streaming in progress !!");
        OUTV("false");
        return false;
    }

    return m_mediaPvr->Pause();
}

/**
 * @brief HS_InputStream::Seek
 * @param value
 * @return
 */
bool KvalHttpStreamWorker::Seek(int value, int totalTime, int currentTime)
{
    LOG_INFO(LOG_TAG, "Seek called");
    if(m_server.state != STREAMING_IN_PROGRESS)
    {
        LOG_WARNING(LOG_TAG, "Ask for Seek, no streaming in progress !!");
        OUTV("false");
        return false;
    }

    return m_mediaPvr->Seek(value, totalTime, currentTime);
}

/**
 * @brief HS_InputStream::Resume
 */
void KvalHttpStreamWorker::Resume()
{
    INV();
    LOG_INFO(LOG_TAG, "Resume called");
    if(m_server.state != STREAMING_IN_PROGRESS)
    {
        LOG_WARNING(LOG_TAG, "Ask for Resume, no streaming in progress !!");
        OUTV();
        return;
    }

    m_mediaPvr->Resume();
    OUTV();
}

/**
 * @brief HS_InputStream::cleanUp
 */
void KvalHttpStreamWorker::cleanUp()
{
    INV();
    LOG_INFO(LOG_TAG, "cleanUp...");

    m_server.state = STREAMING_STOPPED;

    if(m_http_manager)
    {
        m_http_manager->clearAccessCache();
        delete m_http_manager;
        m_http_manager = 0;
    }
    if(m_http_config_manager)
    {
        delete m_http_config_manager;
        m_http_config_manager = 0;
    }
    if(m_getReqTimer)
    {
        delete m_getReqTimer;
        m_getReqTimer = 0;
    }
    if(m_http_server)
    {
        delete m_http_server;
        m_http_server = 0;
    }

    LOG_INFO(LOG_TAG, "Cleanup done");
    OUTV();
}

/**
 * @brief HS_HttpEngine::HS_HttpEngine
 */
KvalHttpStreamManager::KvalHttpStreamManager():
    m_httpMediaProc{nullptr}
{
    LOG_INFO(LOG_TAG, "Instantiate KvalHttpStreamManager()");
}

/**
 * @brief HS_HttpEngine::~HS_HttpEngine
 */
KvalHttpStreamManager::~KvalHttpStreamManager()
{
    LOG_INFO(LOG_TAG, "Delete KvalHttpStreamManager()");
}

/**
 * @brief HS_HttpEngine::onPlayStream
 * @param httpStream
 */
void KvalHttpStreamManager::onPlayStream(const QString &httpStream)
{
    LOG_INFO(LOG_TAG, "onOpenStreamNotify() %s", qPrintable(httpStream));
    m_httpMediaProc = new KvalHttpStreamWorker(httpStream);

    connect(m_httpMediaProc,
            SIGNAL(httpStreamReady(QString)),
            this,
            SIGNAL(streamReady(QString)));
    connect(m_httpMediaProc,
            SIGNAL(httpStreamInterrupted()),
            this,
            SLOT(onHttpStreamInterrupted()),
            Qt::BlockingQueuedConnection);
    connect(m_httpMediaProc,
            SIGNAL(httpStreamFailed(bool)),
            this,
            SLOT(onHttpStreamFailed(bool)));
    connect(m_httpMediaProc,
            SIGNAL(okDiag(QString, QString)),
            this,
            SIGNAL(okDiag(QString, QString)));
    connect(m_httpMediaProc,
            SIGNAL(yesNoDiag(QString,QString,QString,QString)),
            this,
            SIGNAL(yesNoDiag(QString,QString,QString,QString)));
    connect(m_httpMediaProc,
            SIGNAL(yesNoDiagUpdate(QString,QString)),
            this,
            SIGNAL(yesNoDiagUpdate(QString,QString)));
    connect(m_httpMediaProc,
            SIGNAL(yesNoDiagClose()),
            this,
            SIGNAL(yesNoDiagClose()));
    connect(m_httpMediaProc,
            SIGNAL(httpSeeked(int)),
            this,
            SIGNAL(httpSeeked(int)));
    connect(m_httpMediaProc,
            SIGNAL(flushPlayer()),
            this,
            SIGNAL(flushPlayer()));


    m_httpMediaProc->Start();

    LOG_INFO(LOG_TAG, "m_httpMediaProc %p created", m_httpMediaProc);
}

/**
 * @brief HS_HttpEngine::getTrickWritePercent
 * @return
 */
qreal KvalHttpStreamManager::getTrickWritePercent()
{
    INV();
    if(!m_httpMediaProc)
    {
        LOG_WARNING(LOG_TAG, "No m_httpMediaProc available");
        return 0.0;
    }
    return m_httpMediaProc->getTrickModeWritePercent();
    OUTV();
}

/**
 * @brief HS_HttpEngine::onHttpStreamInterrupted
 */
void KvalHttpStreamManager::onHttpStreamInterrupted()
{
    LOG_INFO(LOG_TAG, "m_httpMediaProc self interruption !");
    m_httpMediaProc = NULL;
}

/**
 * @brief HS_HttpEngine::onStopStream
 */
void KvalHttpStreamManager::onStopStream()
{
    INV();
    if(!m_httpMediaProc)
    {
        LOG_WARNING(LOG_TAG, "m_httpMediaProc has already been deleted");
        return;
    }
    m_httpMediaProc->Close();
    m_httpMediaProc = NULL;
    OUTV();
}

/**
 * @brief HS_HttpEngine::onPauseStream
 */
bool KvalHttpStreamManager::onPauseStream()
{
    INV();
    if(!m_httpMediaProc)
    {
        LOG_WARNING(LOG_TAG, "No m_httpMediaProc available");
        return false;
    }

    OUTV();
    return m_httpMediaProc->Pause();
}

/**
 * @brief HS_HttpEngine::onSeekStream
 * @param value
 * @return
 */
bool KvalHttpStreamManager::onSeekStream(int value, int totalTime, int currentTime)
{
    INV();
    if(!m_httpMediaProc)
    {
        LOG_WARNING(LOG_TAG, "No m_httpMediaProc available");
        return false;
    }

    OUTV();
    return m_httpMediaProc->Seek(value, totalTime, currentTime);
}

/**
 * @brief HS_HttpEngine::onResumeStream
 */
void KvalHttpStreamManager::onResumeStream()
{
    INV();
    if(!m_httpMediaProc)
    {
        LOG_WARNING(LOG_TAG, "No m_httpMediaProc available");
        return;
    }
    m_httpMediaProc->Resume();
    OUTV();
}

/**
 * @brief HS_HttpEngine::onHttpStreamFailed
 */
void KvalHttpStreamManager::onHttpStreamFailed(bool unsupported)
{
    INV();
    if(!unsupported)
    {
        Q_EMIT playbackFailed();
    }
    else
    {
        Q_EMIT unsupportedFormat();
    }

    m_httpMediaProc = NULL;
    OUTV();
}
