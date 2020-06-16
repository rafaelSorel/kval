#define LOG_ACTIVATED
#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <semaphore.h>

#include <QObject>
#include <QThread>
#include <QElapsedTimer>

#include "KvalRtmpStreamManager.h"
#include "KvalThreadUtils.h"
#include "KvalMiscUtils.h"

#include <string.h>
#include <librtmp/log.h>
#include <librtmp/rtmp.h>
#define LOG_SRC RTMPENGINESTR
#include "KvalLogging.h"

using namespace std;
#define GetSockError()  errno

#define  SetAVal(av, cstr) \
    av.av_val = (char *)cstr.c_str(); \
    av.av_len = cstr.length()

#undef AVC
#define AVC(str) \
    { \
        (char *)str, \
        sizeof(str)-1 \
    }

/* librtmp option names are slightly different */
static const struct {
  const char *name;
  AVal key;
} options[] = {
  { "SWFPlayer", AVC("swfUrl") },
  { "PageURL",   AVC("pageUrl") },
  { "PlayPath",  AVC("playpath") },
  { "TcUrl",     AVC("tcUrl") },
  { "IsLive",    AVC("live") },
  { NULL }
};

static int RTMP_level=5;
extern "C" 
{
    static void RS_InputStream_Log(int level, const char *fmt, 
                                        va_list args)
    {
        if (level > RTMP_level)
        {
            return;
        }
        else
        {
            char buf[2048];
            switch(level) 
            {
                default:
                case RTMP_LOGCRIT:    level = LC_LOG_CRITICAL;   break;
                case RTMP_LOGERROR:   level = LC_LOG_ERROR;   break;
                case RTMP_LOGWARNING: level = LC_LOG_WARN;  break;
                case RTMP_LOGINFO:    level = LC_LOG_INFO;  break;
                case RTMP_LOGDEBUG:   level = LC_LOG_DEBUG; break;
                case RTMP_LOGDEBUG2:  level = LC_LOG_DEBUG; break;
            }
    
            vsnprintf(buf, sizeof(buf), fmt, args);
            LOG_INFO(LOG_TAG, "%s", qPrintable(buf));
        }
    }
}

/**
 * @brief RS_InputStream::RS_InputStream
 */
RS_InputStream::RS_InputStream():
    m_thread(new KvalThread("RS_InputStream"))
{
    LOG_INFO(LOG_TAG," Using external libRTMP");
    RTMP_LogLevel level;
    level = RTMP_LOGINFO;
    RTMP_LogSetLevel(level);
    RTMP_level = level;

    RTMP_LogSetCallback(RS_InputStream_Log);

    m_rtmp = RTMP_Alloc();
    RTMP_Init(m_rtmp);

    m_eof = true;
    m_bPaused = false;
    m_sStreamPlaying = NULL;
    m_rtmpPendingStop = false;
    m_port = 0;
    m_server.state = STREAMING_STOPPED;
    m_server.socket = -1;

    // Move to a new thread.
    moveToThread(m_thread);
    m_thread->start();
}

/**
 * @brief RS_InputStream::~RS_InputStream
 */
RS_InputStream::~RS_InputStream()
{
    free(m_sStreamPlaying);
    m_sStreamPlaying = NULL;

    if (m_rtmp)
    {
        RTMP_Free(m_rtmp);
    }
    m_rtmp = NULL;
    m_bPaused = false;
    delete m_thread;
}


/**
 * @brief RS_InputStream::IsEOF
 * @return 
 */
bool RS_InputStream::IsEOF()
{
  return m_eof;
}

/**
 * @brief RS_InputStream::Open
 * @param strFile
 * @return
 */
bool RS_InputStream::Open(const char* strFile)
{
    LOG_INFO(LOG_TAG, "Open[%s]", strFile);
    bool ret = true;
    unsigned int cnx_retry = 0;
    QElapsedTimer timer;
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = 800000000L;

    int rc = this->createStreamServer();
    if(rc)
    {
        LOG_ERROR(LOG_TAG, "Problem creating init streaming server !!!");
        ret = false;
        goto free_and_cleanup;
    }

    if (m_sStreamPlaying)
    {
        free(m_sStreamPlaying);
        m_sStreamPlaying = NULL;
    }

    // libRTMP can and will alter strFile, so take a copy of it
    m_sStreamPlaying = (char*)calloc(strlen(strFile)+1,sizeof(char));
    strcpy(m_sStreamPlaying, strFile);

retry_connect:
    timer.start();
    LOG_INFO(LOG_TAG, "RTMP_SetupURL...");
    if (!RTMP_SetupURL(m_rtmp, m_sStreamPlaying))
    {
        LOG_ERROR(LOG_TAG, "RTMP_SetupURL ERROR...");
        ret = false;
        goto free_and_cleanup;
    }

    LOG_INFO(LOG_TAG, "RTMP_Connect...");
    if (!RTMP_Connect(m_rtmp, NULL) || !RTMP_ConnectStream(m_rtmp, 0))
    {
        LOG_ERROR(LOG_TAG, "RTMP Connect ERROR...");
        if (timer.elapsed() < 1000 && cnx_retry < 10)
        {
            LOG_INFO(LOG_TAG, "RTMP Connect retry...");
            nanosleep(&req, &req);
            cnx_retry++;
            goto retry_connect;
        }
        ret = false;
        goto free_and_cleanup;
    }

    m_eof = false;

    LOG_INFO(LOG_TAG, "Start rtmp thread...");
    return QMetaObject::invokeMethod(this, "serverThread");
    
free_and_cleanup:
    if (m_sStreamPlaying)
    {
        free(m_sStreamPlaying);
        m_sStreamPlaying = NULL;
    }
    cleanUp();

    return ret;
}


/**
 * @brief OMX_MediaProcessor::setRtmpOptionValue
 * @param key
 * @param value
 */
void RS_InputStream::setRtmpOptionValue(QString key, QString value)
{
    LOG_INFO(LOG_TAG, "setRtmpOptionValue key[%s] value[%s]...", 
                                            qPrintable(key), qPrintable(value));
    m_rtmpOptionValues[key] = value;
}

/**
 * @brief RS_InputStream::Close
 */
void RS_InputStream::Close()
{
    INV();
    if(m_server.state != STREAMING_IN_PROGRESS)
    {
        LOG_WARNING(LOG_TAG, "Ask for Close, no streaming in progress !!");
        OUTV();
        return;
    }
    m_rtmpPendingStop = true;

    m_stop_pending_mutex.lock();
    if (m_rtmpPendingStop)
    {
      LOG_INFO(LOG_TAG, "Waiting for the pause command to finish.");
      if(m_wait_stop_pending_cmd.wait(&m_stop_pending_mutex, 5000) == false)
      {
          LOG_WARNING(LOG_TAG, "Waiting for the Stop command timed out");
          if(m_connectedSock != -1)
          {
              close(m_connectedSock);
              m_connectedSock = -1;
          }
          if(m_thread->isRunning())
          {
              m_thread->requestInterruption();
              m_thread->terminate();
          }
          this->cleanUp();
      }
    }
    m_stop_pending_mutex.unlock();
    OUTV();
}

/**
 * @brief connect_to_server
 * @param host
 * @param portnum
 * @return
 */
static int _connect_to_server (const char * host, int portnum)
{
   int sock;
   struct sockaddr_in serv_addr;
   struct hostent *server;

   sock =::socket (AF_INET, SOCK_STREAM, 0);
   if (sock == -1)
   {
        LOG_ERROR(LOG_TAG, "ERROR opening socket");
        return -3;
   }
   server =::gethostbyname (host);
   if (server == NULL)
   {
        LOG_ERROR(LOG_TAG, "no such host");
        close (sock);
        return -2;
   }
   ::memset ((char *) &serv_addr, 0, sizeof (serv_addr));
   serv_addr.sin_family = AF_INET;
   ::memcpy ((char *) &serv_addr.sin_addr.s_addr,
         (char *) server->h_addr, server->h_length);
   serv_addr.sin_port = htons (portnum);


   if (::connect
       (sock, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) == -1) {
      ::close (sock);
      return -1;
   }
   return sock;
}

/**
 * @brief RS_InputStream::createStreamServer
 */
int RS_InputStream::createStreamServer()
{
    LOG_INFO(LOG_TAG, "Instantiate StreamServer.");
    struct sockaddr_in addr;
    int sockfd;
    int i = 0;
    int sockTemp;
    while (1)
    {
        i = rand () % (65536 - 3000) + 3000;	////3000 to 65535
        sockTemp = _connect_to_server ("127.0.0.1", i);
        if (sockTemp == -1)
        {
            break;
        }
        else if (sockTemp >= 0)
        {
            ::close(sockTemp);
        }
    }
    m_port = i;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1)
    {
        LOG_ERROR(LOG_TAG, "%s, couldn't create socket", __FUNCTION__);
        return -EINVAL;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");//htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) ==
    -1)
    {
        LOG_ERROR(LOG_TAG, "%s, TCP bind failed for port number: %d",
                 __FUNCTION__,
                 m_port);
        return -EINVAL;
    }

    if (listen(sockfd, 10) == -1)
    {
        LOG_ERROR(LOG_TAG, "%s, listen failed", __FUNCTION__);
        close(sockfd);
        return -EINVAL;
    }

    m_server.socket = sockfd;
    return ENOERR;
}

/**
 * @brief RS_InputStream::serverThread
 */
void RS_InputStream::serverThread()
{
    INV();
    m_server.state = STREAMING_ACCEPTING;
    QString streamAddrStr("http://localhost:");
    streamAddrStr.append(QString("%1").arg(m_port));
    LOG_INFO(LOG_TAG, "Streaming addr %s", qPrintable(streamAddrStr));
    Q_EMIT rtmpStreamReady(streamAddrStr);
    int sockfd = -1;

    while(m_server.state == STREAMING_ACCEPTING)
    {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(struct sockaddr_in);
        sockfd = accept(m_server.socket,
                            (struct sockaddr *) &addr,
                            &addrlen);
        if(sockfd > 0)
        {
            // Create a new process and transfer the control to that
            LOG_INFO(LOG_TAG, "%s: accepted connection from %s", __FUNCTION__,
            inet_ntoa(addr.sin_addr));
            break;
        }
        else
        {
            LOG_DEBUG(LOG_TAG, "%s: accept failed", __FUNCTION__);
        }
    }
    m_connectedSock = sockfd;
    this->processTcpRequest(sockfd);
    OUTV();
}

/**
 * @brief RS_InputStream::processTcpRequest
 * @param sockfd
 */
void RS_InputStream::processTcpRequest(int sockfd)
{
    INV();
    char buf[512] = { 0 };      // answer buffer
    char header[2048] = { 0 };  // request header
    char *filename = NULL;      // GET request: file name
    int len;

    size_t nRead = 0;
    char srvhead[] =
            "\r\nServer: HTTP-RTMP Stream Server 2.0 \r\n";
//    char *status = "404 Not Found";
    m_server.state = STREAMING_IN_PROGRESS;

    // timeout for http requests
    fd_set fds;
    struct timeval tv;

    memset(&tv, 0, sizeof(struct timeval));
    tv.tv_sec = 60;

    // go through request lines
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    if(select(sockfd + 1, &fds, NULL, NULL, &tv) <= 0)
    {
        LOG_ERROR(LOG_TAG, "Request timeout/select failed, ignoring request");
        return;
    }
    else
    {
        nRead = recv(sockfd, header, 2047, 0);
        header[2047] = '\0';
        LOG_INFO(LOG_TAG, "%s: header: %s",
                 __FUNCTION__,
                 header);

// TODO check range starts from 0 and asking till the end.
#if 0
        if (strstr(header, "Range: bytes=") != 0)
        {
            LOG_ERROR(LOG_TAG,
                      "%s, Range request not supported\n", __FUNCTION__);
            len = sprintf(buf,
                          "HTTP/1.0 416 Requested Range Not Satisfiable%s\r\n",
                          srvhead);
            send(sockfd, buf, len, 0);
            return;
        }
#endif
        if(strncmp(header, "GET", 3) == 0 && nRead > 4)
        {
            filename = header + 4;
            // filter " HTTP/..." from end of request
            char *p = filename;
            while(*p != '\0')
            {
                if(*p == ' ')
                {
                    *p = '\0';
                    break;
                }
                p++;
            }
        }
    }
    len = sprintf(buf,
                  "HTTP/1.0 200 OK%sContent-Type: video/flv\r\n\r\n",
                  srvhead);
    LOG_INFO(LOG_TAG,"%s, reply header %s", __FUNCTION__, buf);
    send(sockfd, buf, len, 0);
    close(m_server.socket);
    m_server.socket = 0;
    this->Read(sockfd);
    OUTV();
}

/**
 * @brief RS_InputStream::Read
 * @param buf
 * @param buf_size
 * @return 
 */
int RS_InputStream::Read(int sockfd)
{
    INV();
    LOG_INFO(LOG_TAG,"Start read stream ...");
    if (!m_rtmp)
        return -1;

    int bufferSize = 1024 * 1024;
    char * buffer = NULL;
    buffer = (char *) malloc(bufferSize);
    int nRead = 0;
    int nWritten = 0;

    do
    {
        nRead = RTMP_Read(m_rtmp, buffer, bufferSize);
        if(m_rtmpPendingStop)
        {
            LOG_INFO(LOG_TAG, "m_rtmpPendingStop true STOP ...");
            break;
        }
        if (nRead > 0)
        {
            if((nWritten = send(sockfd, buffer, nRead, 0)) < 0)
            {
                LOG_ERROR(LOG_TAG, "%s, sending failed, error: %d",
                __FUNCTION__,
                GetSockError());
                break;
            }
        }
        else
        {LOG_WARNING(LOG_TAG, "zero read!");
        }
    }while(m_server.state == STREAMING_IN_PROGRESS &&
           nRead > -1 &&
           RTMP_IsConnected(m_rtmp) && nWritten >= 0 &&
           !RTMP_IsTimedout(m_rtmp) && !m_rtmpPendingStop);

    LOG_INFO(LOG_TAG, "Rtmp free_and_cleanup...");
    m_server.state = STREAMING_STOPPING;
    if(m_connectedSock)
    {
        close(m_connectedSock);
        m_connectedSock = -1;
    }
    //Start the new stream after closing the previous one as some server does
    //not accept more than one connection from the same client.
    m_stop_pending_mutex.lock();
    m_wait_stop_pending_cmd.wakeAll();
    m_stop_pending_mutex.unlock();
    if(buffer)
    {free(buffer);
    }
    LOG_INFO(LOG_TAG, "Rtmp Decoding stop cleanup...");
    this->cleanUp();

    OUTV();
    return false;
}

/**
 * @brief RS_InputStream::cleanUp
 */
void RS_InputStream::cleanUp()
{
    INV();
    if (m_rtmp)
    {
        LOG_INFO(LOG_TAG, "RTMP_Close...");
        RTMP_Close(m_rtmp);
    }
    m_optionvalues.clear();
    m_eof = true;
    m_bPaused = false;
    LOG_INFO(LOG_TAG, "Socket Close...");
    if (m_server.socket)
    {
        close(m_server.socket);
    }
    LOG_INFO(LOG_TAG, "Socket Closed...");
    m_server.state = STREAMING_STOPPED;

    Q_EMIT rtmpCleanupComplete(this);
    OUTV();
}

/**
 * @brief RS_InputStream::stopCurrentThread
 */
void RS_InputStream::stopCurrentThread()
{
    INV();
    if(m_thread->isRunning())
    {
        if(!m_thread->isInterruptionRequested())
        {
            m_thread->quit();
            m_thread->wait();
        }
    }
    OUTV();
}

/**
 * @brief RS_InputStream::Seek
 * @param offset
 * @param whence
 * @return 
 */
int64_t RS_InputStream::Seek(int64_t offset, int whence)
{
    if (whence == 1)
        return 0;
    else
        return -1;
}

/**
 * @brief RS_InputStream::SeekTime
 * @param iTimeInMsec
 * @return 
 */
bool RS_InputStream::SeekTime(int iTimeInMsec)
{
  LOG_INFO(LOG_TAG, "RTMP Seek to %i requested", iTimeInMsec);

  if (m_rtmp && RTMP_SendSeek(m_rtmp, iTimeInMsec))
    return true;

  return false;
}

/**
 * @brief RS_InputStream::GetLength
 * @return 
 */
int64_t RS_InputStream::GetLength()
{
    return -1;
}

/**
 * @brief RS_InputStream::Pause
 * @param dTime
 * @return 
 */
bool RS_InputStream::Pause(double dTime)
{
    m_bPaused = !m_bPaused;
    LOG_INFO(LOG_TAG, "RTMP Pause %s requested", 
             m_bPaused ? "TRUE" : "FALSE");

    if (m_rtmp)
    {
        RTMP_Pause(m_rtmp, m_bPaused);
    }

    return true;
}


/**
 * @brief RS_RtmpEngine::RS_RtmpEngine
 */
KvalRtmpStreamManager::KvalRtmpStreamManager():
    m_rtmpMediaProc(nullptr)
{
    LOG_INFO(LOG_TAG, "Instantiate RS_RtmpEngine()");
}

/**
 * @brief RS_RtmpEngine::~RS_RtmpEngine
 */
KvalRtmpStreamManager::~KvalRtmpStreamManager()
{
    LOG_INFO(LOG_TAG, "Delete RS_RtmpEngine()");
}

/**
 * @brief RS_RtmpEngine::onPlayStream
 * @param rtmpStream
 */
void KvalRtmpStreamManager::onPlayStream(const QString &rtmpStream)
{
    LOG_INFO(LOG_TAG, "onOpenStreamNotify() %s", qPrintable(rtmpStream));
    if(!m_rtmpMediaProc)
    {
        m_rtmpMediaProc = new RS_InputStream();
        connect(m_rtmpMediaProc, 
                SIGNAL(rtmpStreamReady(QString)),
                this, 
                SIGNAL(streamReady(QString)));
        connect(m_rtmpMediaProc, 
                SIGNAL(rtmpCleanupComplete(RS_InputStream*)),
                this, 
                SLOT(onRtmpCleanupComplete(RS_InputStream*)));

        int rc = m_rtmpMediaProc->Open(rtmpStream.toStdString().c_str());
        if(rc == false)
        {
            Q_EMIT playbackFailed();
            LOG_ERROR(LOG_TAG, "Could not open stream: %s",
                     qPrintable(rtmpStream));
        }
    }
}

/**
 * @brief RS_RtmpEngine::onRtmpCleanupComplete
 */
void KvalRtmpStreamManager::onRtmpCleanupComplete(RS_InputStream* rtmpStreamObj)
{
    INV();
    if(m_rtmpMediaProc == rtmpStreamObj)
    {
        LOG_INFO(LOG_TAG, "rtmpMediaProc delete current...");
        m_rtmpMediaProc->stopCurrentThread();
        delete m_rtmpMediaProc;
        m_rtmpMediaProc = NULL;
    }
    else
    {
        LOG_INFO(LOG_TAG, "rtmpMediaProc delete previous ...");
        rtmpStreamObj->stopCurrentThread();
        delete rtmpStreamObj;
        rtmpStreamObj = NULL;
    }
    OUTV();
}

/**
 * @brief RS_RtmpEngine::onStopStream
 */
void KvalRtmpStreamManager::onStopStream()
{
    INV();
    if(!m_rtmpMediaProc) return;
    m_rtmpMediaProc->Close();
    m_rtmpMediaProc = NULL;
    OUTV();
}
