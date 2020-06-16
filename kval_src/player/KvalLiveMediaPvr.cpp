#define LOG_ACTIVATED
#include <QFile>
#include <QLocalSocket>
#include <QObject>
#include <QThread>
#include <QStorageInfo>

#include "KvalLocalBrowseManager.h"
#include "KvalLiveMediaPvr.h"

#define LOG_SRC MEDIAPVRSTR
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Macro definition
//----------------------------------------------------------------------------
#define MAX_INTERNAL_TRICK_FILE_SIZE        448 * 1024 * 1024 //448 MB
#define MAX_INTERNAL_PRE_BUF_QUEUE_SIZE     15 * 1024 * 1024   //10 MB
#define MAX_INTERNAL_SEGMENT_QUEUE_SIZE     4 * 1024 * 1024   //4 MB


//----------------------------------------------------------------------------
// Type definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------
static int interrupt_cb(void *unused);
static qint64 CurrentHostCounter(void);
#define RESET_PVR_TIMEOUT(x) do { \
  timeout_start = CurrentHostCounter(); \
  timeout_duration = (x) * timeout_default_duration; \
} while (0)

//----------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------
QAtomicInteger<bool> g_mediaPlayerReaderFlushed {true};
static bool g_abort = false;
static qint64 timeout_start;
static qint64 timeout_default_duration;
static qint64 timeout_duration;

//----------------------------------------------------------------------------
// Public interface, front end
//----------------------------------------------------------------------------
/**
 * @brief CurrentHostCounter
 * @return
 */
static qint64 CurrentHostCounter(void)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return( ((qint64)now.tv_sec * 1000000000LL) + now.tv_nsec );
}

/**
 * @brief interrupt_cb
 * @param unused
 * @return
 */
static int interrupt_cb(void *unused)
{
  (void)unused;
  int ret = 0;

  if (g_abort)
  {
    LOG_ERROR(LOG_TAG, "interrupt_cb - Told to abort");
    ret = 1;
  }
  else if (timeout_duration &&
           CurrentHostCounter() - timeout_start > timeout_duration)
  {
    LOG_ERROR(LOG_TAG, "interrupt_cb - Timed out");
    ret = 1;
  }
  return ret;
}

/**
 * @brief MP_MediaPvrProvider::MP_MediaPvrProvider
 */
MP_MediaPvrProvider::MP_MediaPvrProvider(MP_MediaPvr * owner):
    m_owner(owner)
{
    m_server = nullptr;
    m_socket = nullptr;
    m_processTimer = nullptr;
    m_stopRequested = false;
    m_sessionEstablished = false;
    m_thread = new KvalThread("MP_MediaPvrProvider");
    m_segActiveEntry.index = 0;
    m_segActiveEntry.consumed = false;
    m_segActiveEntry.queued = false;
    m_segActiveEntry.offset_byte = 0;
    memset(&m_segActiveEntry.filename, 0, sizeof(m_segActiveEntry.filename));

    m_thread->setObjectName("MP_MediaPvrProvider");
    moveToThread(m_thread);

    connect(this, SIGNAL (finished()), m_thread, SLOT(quit()));
    connect(this, SIGNAL (finished()), this, SLOT(deleteLater()));
    connect(m_thread, SIGNAL (finished()), m_thread, SLOT (deleteLater()));
    m_thread->start();
}

/**
 * @brief MP_MediaPvrProvider::~MP_MediaPvrProvider
 */
MP_MediaPvrProvider::~MP_MediaPvrProvider()
{
    LOG_INFO(LOG_TAG, "~ Delete MP_MediaPvrProvider ~");
    while(pvrDataQueue.size() > 0)
    {
        QByteArray entry = pvrDataQueue.dequeue();
        entry.clear();
    }
    pvrDataQueue.clear();

    while(m_preLoadDataQueue.size() > 0)
    {
        QByteArray entry = m_preLoadDataQueue.dequeue();
        entry.clear();
    }
    m_preLoadDataQueue.clear();

    if(m_socket)
    {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
    }

    if(m_server)
    {
        m_server->close();
        delete m_server;
    }
}

/**
 * @brief HS_HttpServer::createPvrServer
 * @return
 */
void MP_MediaPvrProvider::createPvrServer()
{
    m_server = new QTcpServer(this);
    connect(m_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    m_processTimer = new QTimer;
    connect(m_processTimer, SIGNAL(timeout()), this, SLOT(process()));
    connect(this, SIGNAL(stopTimer()), m_processTimer, SLOT(stop()));

    while(true)
    {
        if(!m_server->listen(QHostAddress::LocalHost))
        {
            LOG_ERROR(LOG_TAG, "Could not start pvr srv");
            continue;
        }
        else
        {
            LOG_INFO(LOG_TAG, "pvr server waiting for connection on port %d", m_server->serverPort());
            m_streamAddrStr = "http://localhost:";
            m_streamAddrStr.append(QString("%1").arg(m_server->serverPort()));
            LOG_INFO(LOG_TAG, "Streaming addr %s", qPrintable(m_streamAddrStr));
            Q_EMIT startMuxer();
            break;
        }
    }
}

/**
 * @brief MP_MediaPvrProvider::onNewConnection
 */
void MP_MediaPvrProvider::onNewConnection()
{
    QMutexLocker locker(&m_mutex);
    LOG_INFO(LOG_TAG, "New Client Ask for Connection...");
    m_socket = m_server->nextPendingConnection();
    LOG_INFO(LOG_TAG, "m_socket %p", m_socket);

    connect(m_socket, SIGNAL(readyRead()), this, SLOT(txRx()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(closingClient()));

    m_sessionEstablished = true;
    LOG_INFO(LOG_TAG, "Out onNewConnection...");

}

/**
 * @brief MP_MediaPvrProvider::txRx
 */
void MP_MediaPvrProvider::txRx()
{
    QMutexLocker locker(&m_mutex);
    LOG_INFO(LOG_TAG, "Recv Client Data...");
    QTcpSocket* readSocket = qobject_cast<QTcpSocket*>(sender());
    QByteArray data = readSocket->read(2048);
    LOG_INFO(LOG_TAG,"Received data from client: \n%s", qPrintable(data.data()));

    readSocket->write("HTTP/1.1 200 OK\r\n");
    readSocket->write("Content-Type: application/octet-stream\r\n");
    readSocket->write("\r\n");
    LOG_INFO(LOG_TAG,"Data sent back ...");

    if(!m_processTimer->isActive())
    {
        m_processTimer->setInterval(20);
        m_processTimer->start();
    }
}

/**
 * @brief MP_MediaPvrProvider::send
 * @param ptrdata
 * @param size
 * @return
 */
int MP_MediaPvrProvider::send(char * ptrdata, qint64 size)
{
    QMutexLocker locker(&m_mutex);

    if(!m_socket)
    {
        LOG_INFO(LOG_TAG, "Socket Not yet ready...");
        return 0;
    }

    if (m_socket->state() != QAbstractSocket::ConnectedState)
    {
        LOG_INFO(LOG_TAG, "Client Socket not valid yet...");
        return -1;
    }

    return m_socket->write(ptrdata, size);
}

/**
 * @brief MP_MediaPvrProvider::closingClient
 */
void MP_MediaPvrProvider::closingClient()
{
    QMutexLocker locker(&m_mutex);
    QTcpSocket* readSocket = qobject_cast<QTcpSocket*>(sender());
    LOG_INFO(LOG_TAG,
             "Client Disconnected state [%u]...",
             readSocket->state());

    readSocket->deleteLater();
    if(readSocket == m_socket)
        m_socket = nullptr;
}

/**
 * @brief MP_MediaPvrProvider::Close
 */
void MP_MediaPvrProvider::Close()
{
    QMutexLocker locker(&m_mutex);
    LOG_INFO(LOG_TAG, "Close segment provider server...");
    m_stopRequested = true;
    if(m_processTimer && m_processTimer->isActive())
    {
        Q_EMIT stopTimer();
    }

    LOG_INFO(LOG_TAG, "Close segment provider server DONE...");
    Q_EMIT finished();
}

/**
 * @brief MP_MediaPvrProvider::process
 */
void MP_MediaPvrProvider::process()
{
    int ret;
    QByteArray localData;

    if(m_stopRequested)
    {
        if(m_processTimer->isActive())
            m_processTimer->stop();

        //Clear all timeout queue
        m_processTimer->disconnect(this);
        return;
    }

    if(m_owner && m_owner->getMuxerStatus() == MEDIA_PVR_DEMUX_CONFIG_FALED)
    {
        LOG_INFO(LOG_TAG, "Restart Muxer");
        m_owner->initMuxerStatus();
        Q_EMIT startMuxer();
        goto injection_process;
    }

    //Try to gather 32K of data at least to speed up the demux task
    while((localData.size() < 0x8000) && (!m_preLoadDataQueue.isEmpty()))
    {
        QByteArray data = m_preLoadDataQueue.dequeue();
        localData.append(data);
    }
    if(!localData.isEmpty())
    {
        ret = send(localData.data(), localData.size());
        if(!ret)
        {
            LOG_INFO(LOG_TAG, "Repush data back in queue");
            m_preLoadDataQueue.prepend(localData);
        }
        if(ret < 0)
        {
            LOG_ERROR(LOG_TAG, "Unable to send data to segmenter !!");
            m_processTimer->stop();
            m_processTimer->disconnect(this);
            return;
        }
    }

injection_process:
    if(m_owner && m_owner->isRawInjectionActive())
    {
        LOG_DEBUG(LOG_TAG, "Raw Injection");
        goto raw_injection;
    }

    else if(m_owner && m_owner->isSegInjectionSwitch())
    {
        goto segment_injection;
    }

    else
    {
        LOG_INFO(LOG_TAG, "No Raw Neither Segement Injection active");
        return;
    }

raw_injection:
    ret = m_owner->fillRawQueue();
    if(ret < 0)
    {
        LOG_DEBUG(LOG_TAG, "Fill Raw errno: %d", errno);
        return;
    }
    m_owner->segDataReady();
    return;


segment_injection:
    ret = fillSegmentQueue();
    if(ret < 0)
    {
        LOG_DEBUG(LOG_TAG, "Fill Segment errno: %d", errno);
        return;
    }

    m_owner->segDataReady();

    return;
}

/**
 * @brief MP_MediaPvrProvider::checkSeekEnd
 */
bool MP_MediaPvrProvider::checkSeekEnd()
{
    QMutexLocker locker(&m_mutex);
    return (m_mediaPlayerReady && g_mediaPlayerReaderFlushed) ? true : false;
}

/**
 * @brief MP_MediaPvrProvider::mediaPlayerReadyNotify
 */
void MP_MediaPvrProvider::mediaPlayerReadyNotify()
{
    QMutexLocker locker(&m_mutex);
    m_mediaPlayerReady = true;
}

/**
 * @brief MP_MediaPvrProvider::mediaPlayerWait
 */
void MP_MediaPvrProvider::mediaPlayerWait()
{
    QMutexLocker locker(&m_mutex);
    m_mediaPlayerReady = false;
}

/**
 * @brief MP_MediaPvrProvider::readData
 * @param data
 * @return
 */
bool MP_MediaPvrProvider::readData(QByteArray &data)
{
    QMutexLocker locker(&m_mutex);

    if(pvrDataQueue.isEmpty())
        return false;

    data = pvrDataQueue.dequeue();
    if(m_owner && m_owner->isSegInjectionSwitch())
    {
        m_segActiveEntry.consumed_byte = m_segActiveEntry.consumed_byte + data.size();
        if(m_segActiveEntry.consumed_byte >= m_segActiveEntry.file_size)
        {
            LOG_INFO(LOG_TAG, "Segment consumed ....");
            m_segActiveEntry.consumed = true;
        }
    }
    LOG_DEBUG(LOG_TAG, "read data: %ld", data.size());
    return true;
}

/**
 * @brief MP_MediaPvrProvider::clearPvrData
 */
void MP_MediaPvrProvider::clearPvrData()
{
    QMutexLocker locker(&m_mutex);
    pvrDataQueue.clear();
}

/**
 * @brief MP_MediaPvrProvider::setSegmentNumber
 * @param index
 */
void MP_MediaPvrProvider::setSegmentNumber(int index)
{
    QMutexLocker locker(&m_mutex);
    int ret = m_owner->fetchSegmentInfo(&m_segActiveEntry, index);
    if(ret < 0)
    {
        LOG_INFO(LOG_TAG, "Fetch Segment errno: %d", errno);
        return;
    }
    LOG_INFO(LOG_TAG, "m_segActiveEntry.index: %d", m_segActiveEntry.index);
    LOG_INFO(LOG_TAG, "m_segActiveEntry.filename: %s", m_segActiveEntry.filename);
    LOG_INFO(LOG_TAG, "m_segActiveEntry.filesize: %ld", m_segActiveEntry.file_size);
}

/**
 * @brief MP_MediaPvrProvider::sendSegment
 * @return
 */
int MP_MediaPvrProvider::fillSegmentQueue()
{
    QMutexLocker locker(&m_mutex);
    int ret;
    static int prebuf_size = 0;
    static qint64 seek_prefill_size = 0;
    static qint64 seekPreFillChunkSize = 0;
    static int burstNbr = 0;
    if(m_stopRequested)
    {
        LOG_INFO(LOG_TAG, "MP_MediaPvrProvider Close Requested...");
        return -EPERM;
    }

    // Wait for player reset in seek situation
    if(!m_mediaPlayerReady)
    {
        LOG_INFO(LOG_TAG, "Wait for player reset...");
        seek_prefill_size = 0;
        burstNbr = 0;
        return -EBUSY;
    }

    if(m_owner)
    {
        ret = m_owner->checkExpiredSegEntries(m_segActiveEntry.index);
        if(ret < 0)
        {
            LOG_INFO(LOG_TAG, "Removed current segment index");
            return ret;
        }
    }

    //Resume player
    if(!m_owner->m_isPausedReq)
        prebuf_size = 0;

    if( ( prebuf_size > 0x80000 ) && m_owner->m_isPausedReq)
    {
        LOG_DEBUG(LOG_TAG, "Pause Requested prebuf_size: %ld...", prebuf_size);
        return -EBUSY;
    }

    if(pvrDataQueue.dataSize() >= MAX_INTERNAL_SEGMENT_QUEUE_SIZE)
    {
        LOG_INFO(LOG_TAG, "Wait for queue to be partially cleared");
        return -EBUSY;
    }

    if(!m_segActiveEntry.index)
    {
        ret = m_owner->fetchSegmentInfo(&m_segActiveEntry, m_segActiveEntry.index + 1);
        if(ret < 0)
        {
            LOG_INFO(LOG_TAG, "Fetch Segment errno: %d", errno);
            return ret;
        }
        LOG_INFO(LOG_TAG, "m_segActiveEntry.index: %d", m_segActiveEntry.index);
        LOG_INFO(LOG_TAG, "m_segActiveEntry.filename: %s", m_segActiveEntry.filename);
        LOG_INFO(LOG_TAG, "m_segActiveEntry.filesize: %ld", m_segActiveEntry.file_size);
    }
    else if(m_segActiveEntry.queued && m_segActiveEntry.consumed)
    {
        ret = m_owner->fetchSegmentInfo(&m_segActiveEntry, m_segActiveEntry.index + 1);
        if(ret < 0)
        {
            LOG_INFO(LOG_TAG, "Fetch Segment errno: %d", errno);
            return ret;
        }
        LOG_INFO(LOG_TAG, "m_segActiveEntry.index: %d", m_segActiveEntry.index);
        LOG_INFO(LOG_TAG, "m_segActiveEntry.filename: %s", m_segActiveEntry.filename);
        LOG_INFO(LOG_TAG, "m_segActiveEntry.filesize: %ld", m_segActiveEntry.file_size);
    }
    else if( m_segActiveEntry.queued && !m_segActiveEntry.consumed )
    {
        LOG_INFO(LOG_TAG,
                 "Segment already queued wait for consumed: %s",
                 m_segActiveEntry.filename);
        return -EAGAIN;
    }

    QFile segmentFile(m_segActiveEntry.filename);
    if(!segmentFile.exists())
    {
        LOG_ERROR(LOG_TAG, "Could not find segment file: %s", m_segActiveEntry.filename);
        return -EEXIST;
    }
    qint64 chunk_size = (qint64)((m_segActiveEntry.file_size /
                          (qint64)(m_segActiveEntry.end_time - m_segActiveEntry.start_time)) / 50);

    if(!g_mediaPlayerReaderFlushed)
    {
        seekPreFillChunkSize = chunk_size;
        chunk_size = m_segActiveEntry.file_size / 5; //5 chunks sends
        burstNbr += 1;
        LOG_INFO(LOG_TAG, "chunk_size: %ld", chunk_size);
        LOG_INFO(LOG_TAG, "seekPreFillChunkSize: %ld", seekPreFillChunkSize);
        // This holds the data size sent to the player to speedup the seeking process
        // it wil be decreased until 0 then resume to normal chunk size.
        // this will avoid to increase memory usage where the player does not need much data to
        // play smoothly.
        seek_prefill_size = chunk_size * (burstNbr - 2);
    }
    else
    {
        if(seek_prefill_size > 0)
        {
            LOG_DEBUG(LOG_TAG, "seek_prefill_size: %ld", seek_prefill_size);
            LOG_DEBUG(LOG_TAG, "Decrease memory enqueuying on the player side...");
            seek_prefill_size -= seekPreFillChunkSize;
            return -EBUSY;
        }
        else
        {
            seek_prefill_size = 0;
            burstNbr=0;
        }
    }

    LOG_DEBUG(LOG_TAG, "chunk_size: %ld", chunk_size);
    segmentFile.open(QIODevice::ReadOnly);
    segmentFile.seek(m_segActiveEntry.offset_byte);
    QByteArray data = segmentFile.read(chunk_size);
    m_segActiveEntry.offset_byte = m_segActiveEntry.offset_byte + data.size();
    LOG_DEBUG(LOG_TAG, "data read size: %d", data.size());
    segmentFile.close();
    prebuf_size += data.size();

    if(m_segActiveEntry.offset_byte >= m_segActiveEntry.file_size)
    {
        LOG_INFO(LOG_TAG, "Segment Queued ....");
        m_segActiveEntry.queued = true;
    }

    pvrDataQueue.enqueue(data);

    return ENOERR;
}

/**
 * @brief MP_MediaPvrProvider::getPvrSrvAddr
 * @return
 */
QString MP_MediaPvrProvider::getPvrSrvAddr()
{
    return m_streamAddrStr;
}

/**
 * @brief MP_MediaPvrProvider::populate
 * @param data
 * @return
 */
void MP_MediaPvrProvider::populate(QByteArray bufferarray, bool isActiveTrickMode)
{
    m_preLoadDataQueue.enqueue(bufferarray);

    LOG_DEBUG(LOG_TAG, "m_preLoadDataQueue.dataSize: %ld", m_preLoadDataQueue.dataSize() / 1024);
    if(isActiveTrickMode)
        return;

    while(m_preLoadDataQueue.dataSize() > MAX_INTERNAL_PRE_BUF_QUEUE_SIZE)
    {
        LOG_DEBUG(LOG_TAG, "MAX_INTERNAL_QUEUE_SIZE reached dequeue");
        m_preLoadDataQueue.dequeue();
    }
}

/**
 * @brief MP_MediaPvr::MP_MediaPvr
 */
MP_MediaPvr::MP_MediaPvr()
{
    ifmt_ctx = NULL;
    ofmt_ctx = NULL;
    last_mux_dts = -1;
    m_pvrFolder = "";
    m_segFilePrefix = "";
    m_captureMode = "";
    m_pvrMode = MEDIA_PVR_INIT_MODE;
    m_isTrickMode = false;
    m_isPausedReq = false;
    m_pauseForFlushReq = false;
    m_write_percent = 0.0;
    m_isTrickOnRemovable = false;
    m_seekVal = 0;
    m_pvrAvailableSpace = (double)MAX_INTERNAL_TRICK_FILE_SIZE;
    m_pvrAvailableSpace -= (double)MAX_INTERNAL_PRE_BUF_QUEUE_SIZE;
    m_pFormatContext = nullptr;
    m_currentSegIdx = 1;
    m_SegCounts = 0;
    m_segInjectionSwitch = false;
    m_rawInjectionActive = true;
    g_mediaPlayerReaderFlushed = true;
    m_mediaPvrProvider = new MP_MediaPvrProvider(this);
    m_pvrSessionConsumedData = 0.0;
    m_mediaPlayerRef = KvalMediaPlayerEngine::getGlobalRef();

    connect(m_mediaPvrProvider, SIGNAL(startMuxer()), this, SLOT(Start()));

    QString trickFilePath;
    QDateTime localDate = QDateTime::currentDateTime();
    QString ufId = localDate.toString("yyyyMMddhhmmss");

    DEVICES_SET removableDevices = KvalDevicesUtils::get_available_devices();
    Q_FOREACH(const KvalMediaDevice& device, removableDevices)
    {
        if(device.m_iDriveType != KvalMediaDevice::SOURCE_TYPE_REMOVABLE)
            continue;

        if(device.recordable_fs == 1)
        {
            m_isTrickOnRemovable = true;
            trickFilePath = device.strPath;
            QStorageInfo partition(trickFilePath);
            m_pvrAvailableSpace = (double)partition.bytesAvailable();
            m_pvrAvailableSpace -= (double)MAX_INTERNAL_PRE_BUF_QUEUE_SIZE;
            break;
        }
    }
    if(!m_isTrickOnRemovable)
    {
        trickFilePath = QString("/storage/.trickpart");
    }

    m_pvrFolder = trickFilePath + "/pvr";
    QDir dir(m_pvrFolder);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    m_segFilePrefix = m_pvrFolder;
    m_rawFilePrefix = m_pvrFolder;

    m_segFilePrefix.append("/").append("segment_"+ufId+"_%04d.ts");
    m_rawFilePrefix.append("/").append("raw_"+ufId+"_%04d.ts");
    LOG_INFO(LOG_TAG, "m_rawFilePrefix: %s", qPrintable(m_rawFilePrefix));

    m_curRawInfoReader = nullptr;
    m_curRawInfoWriter = nullptr;

    m_preEnablementFile = new QFile("/storage/.trickpart/pre_enablement");
    m_preEnablementFile->open(QIODevice::ReadWrite);
    m_preEnablementFile->close();

    m_thread = new KvalThread("MP_MediaPvr");
    m_thread->setObjectName("MP_MediaPvr");
    moveToThread(m_thread);

    connect(this, SIGNAL (finished()), m_thread, SLOT(quit()));
    connect(this, SIGNAL (finished()), this, SLOT(deleteLater()));
    connect(m_thread, SIGNAL (finished()), m_thread, SLOT (deleteLater()));
    m_thread->start();

    QMetaObject::invokeMethod(this, "cleanupPvrFiles");
}

/**
 * @brief MP_MediaPvr::cleanupPvrFiles
 */
void MP_MediaPvr::cleanupPvrFiles()
{
    QMutexLocker locker(&m_rawEntriesLock);
    LOG_INFO(LOG_TAG, "In cleanupPvrFiles");
    QDir dirSeg(m_pvrFolder);
    dirSeg.setNameFilters(QStringList() << "segment_*.ts");
    dirSeg.setFilter(QDir::Files);
    Q_FOREACH(QString dirFile, dirSeg.entryList())
    {
        dirSeg.remove(dirFile);
    }

    QDir dirRaw(m_pvrFolder);
    dirRaw.setNameFilters(QStringList() << "raw_*.ts");
    dirRaw.setFilter(QDir::Files);
    Q_FOREACH(QString dirFile, dirRaw.entryList())
    {
        dirRaw.remove(dirFile);
    }

    //Reavaluate free space
    QStorageInfo partition(m_pvrFolder);
    m_pvrAvailableSpace = (double)partition.bytesAvailable();
    m_pvrAvailableSpace -= (double)MAX_INTERNAL_PRE_BUF_QUEUE_SIZE;
    LOG_INFO(LOG_TAG, "m_pvrAvailableSpace: %f", m_pvrAvailableSpace);
    LOG_INFO(LOG_TAG, "Out cleanupPvrFiles");
}
/**
 * @brief MP_MediaPvr::~MP_MediaPvr
 */
MP_MediaPvr::~MP_MediaPvr()
{
    LOG_INFO(LOG_TAG, "cleanup raws");
    while(m_rawsInfo.size() > 0)
    {
        RawEntry *entry = m_rawsInfo.front();
        m_rawsInfo.pop_front();
        if(entry)
        {
            if(entry->fd)
            {
                if(entry->fd->exists())
                    entry->fd->remove();
                delete entry->fd;
            }
            av_free(entry);
        }
    }

    LOG_INFO(LOG_TAG, "cleanup segments");
    char firstSegmentName[128];
    memset(firstSegmentName, 0, sizeof(firstSegmentName));
    snprintf(firstSegmentName,
             sizeof(firstSegmentName),
             (char*)m_segFilePrefix.toStdString().c_str(),
             0);
    QFile fd(firstSegmentName);
    if (fd.exists())
        fd.remove();

    while(m_segmentsInfo.size() > 0)
    {
        SegmentEntry *entry = m_segmentsInfo.front();
        m_segmentsInfo.pop_front();
        if(entry)
        {
            QFile fd(entry->filename);
            if(fd.exists())
            {
                fd.remove();
            }
            av_free(entry);
        }
    }

    LOG_INFO(LOG_TAG, "~ Delete mediapvr ~");
}

/**
 * @brief MP_MediaPvr::abortPvrRaw
 */
void MP_MediaPvr::abortPvrRaw()
{
    LOG_INFO(LOG_TAG, "abortPvrRaw");
    QMutexLocker locker(&m_rawEntriesLock);
    m_abortRawTask = true;
}

/**
 * @brief MP_MediaPvr::abortPvrSegment
 */
void MP_MediaPvr::abortPvrSegment()
{
    LOG_INFO(LOG_TAG, "abortPvrSegment");
    if(m_muxerStatus == MEDIA_PVR_DEMUX_MUX_INPROGRESS)
    {
        m_abortSegTask = true;
        g_abort = true;

        m_mutexPending.lock();
        if(m_abortSegTask)
        {
            LOG_INFO(LOG_TAG, "Wait for PVR Stop command to finish.");
            m_waitPendingCommand.wait(&m_mutexPending);
        }
        m_mutexPending.unlock();
    }
}

/**
 * @brief MP_MediaPvr::Close
 */
void MP_MediaPvr::Close()
{
    abortPvrRaw();

    abortPvrSegment();

    m_mediaPvrProvider->Close();

    cleanup();
}

/**
 * @brief MP_MediaPvr::cleanup
 */
void MP_MediaPvr::cleanup()
{
    LOG_INFO(LOG_TAG, "cleanup()");
    QMutexLocker locker(&m_pvrMtx);
    Q_EMIT finished();
    LOG_INFO(LOG_TAG, "cleanup() Done");
}

/**
 * @brief MP_MediaPvr::Start
 * @return
 */
void MP_MediaPvr::Start()
{
    QMutexLocker locker(&m_pvrMtx);
    g_abort = false;
    AVOutputFormat *ofmt = NULL;
    QVector <InputStream *> input_streams;
    SegmentEntry * segTempEntry = nullptr;
    AVPacket pkt;
    qint64 ts_offset = 0;
    qint64 totaldatasize = 0;
    qint64 input_pts = 0;
    qint64 segment_pts = 0;
    int current_index = 1;

#if 0
    static int testConfigFail = true;
#endif

    string outfileStr = m_segFilePrefix.toStdString();
    LOG_INFO(LOG_TAG, "outfileStr: %s", outfileStr.c_str());
    string infileStr =  m_mediaPvrProvider->getPvrSrvAddr().toStdString();
    const char *in_filename = infileStr.c_str();
    const char *out_filename = outfileStr.c_str();
    int ret, i;
    int retries = 3;

    DemuxStatus localMuxerStatus = MEDIA_PVR_DEMUX_INIT;
    m_muxerStatus = MEDIA_PVR_DEMUX_INIT;

    av_log_set_level(AV_LOG_ERROR);
    avcodec_register_all();
    av_register_all();
    avformat_network_init();

    AVDictionary* options_dict = NULL;
    const AVIOInterruptCB int_cb = { interrupt_cb, this };
    RESET_PVR_TIMEOUT(40);

    ifmt_ctx = avformat_alloc_context();
    if(m_pvrMode == MEDIA_PVR_TRICK_MODE)
    {
        av_dict_set(&options_dict, "segment_format", "mpegts", 0);
        av_dict_set(&options_dict, "segment_time", "5", 0);
    }

    LOG_INFO(LOG_TAG, "avformat_open_input: %s", in_filename);
    ifmt_ctx->interrupt_callback = int_cb;
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, NULL)) < 0)
    {
        LOG_ERROR(LOG_TAG, "Could not open input file '%s'", in_filename);
        localMuxerStatus = MEDIA_PVR_DEMUX_INIT_FALED;
        goto end;
    }

    while(retries > 0)
    {
        LOG_INFO(LOG_TAG, "avformat_find_stream_info...");
        ret = avformat_find_stream_info(ifmt_ctx, 0);
        if (ret < 0)
        {
            retries = retries - 1;
            LOG_INFO(LOG_TAG, "Wait for more data to extract stream information");
            if(!retries)
            {
                LOG_ERROR(LOG_TAG, "Failed to retrieve input stream information");
                localMuxerStatus = MEDIA_PVR_DEMUX_CONFIG_FALED;
                goto end;
            }
            av_usleep(200000);
            continue;
        }
        localMuxerStatus = MEDIA_PVR_DEMUX_INIT;
        break;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    ts_offset -= ifmt_ctx->start_time;

    avformat_alloc_output_context2(&ofmt_ctx,
                                   NULL,
                                   ((m_pvrMode == MEDIA_PVR_REC_MODE) ? "mp4" : "segment"),
                                   out_filename);
    if (!ofmt_ctx)
    {
        LOG_ERROR(LOG_TAG, "Could not create output context");
        localMuxerStatus = MEDIA_PVR_DEMUX_CONFIG_FALED;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;

    for (i = 0; (unsigned int)i < ifmt_ctx->nb_streams; i++)
    {
        AVStream *in_stream = ifmt_ctx->streams[i];
        InputStream *ist = (InputStream*)av_mallocz(sizeof(*ist));
        ist->saw_first_ts = 0;
        ist->st = in_stream;
        ist->file_index = 0;
        ist->discard = 1;
        ist->nb_samples = 0;
        ist->min_pts = INT64_MAX;
        ist->max_pts = INT64_MIN;
        ist->ts_scale = 1.0;
        ist->autorotate = 1;
        ist->user_set_discard = AVDISCARD_NONE;
        ist->filter_in_rescale_delta_last = AV_NOPTS_VALUE;

        ist->dec_ctx = in_stream->codec;
        ist->dec = (AVCodec*)in_stream->codec->codec;
        ist->nb_packets = 0;
        ist->data_size = 0;

        if(in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            ist->resample_height  = ist->dec_ctx->height;
            ist->resample_width   = ist->dec_ctx->width;
            ist->resample_pix_fmt = ist->dec_ctx->pix_fmt;
            ist->top_field_first = -1;
        }
        if(in_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            ist->resample_sample_fmt     = ist->dec_ctx->sample_fmt;
            ist->resample_sample_rate    = ist->dec_ctx->sample_rate;
            ist->resample_channels       = ist->dec_ctx->channels;
            ist->resample_channel_layout = ist->dec_ctx->channel_layout;
        }
        input_streams.append(ist);

        AVStream *out_stream = add_output_stream(ofmt_ctx, in_stream);

        out_stream->time_base = av_add_q(out_stream->codec->time_base, (AVRational){0, 1});
        LOG_ERROR(LOG_TAG, "out_stream->time_base.num: %d", out_stream->codec->time_base.num);
        LOG_ERROR(LOG_TAG, "out_stream->time_base.den: %d", out_stream->codec->time_base.den);

        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            LOG_ERROR(LOG_TAG, "Could not open output file '%s'", out_filename);
            localMuxerStatus = MEDIA_PVR_DEMUX_CONFIG_FALED;
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, &options_dict);
    if (ret < 0)
    {
        LOG_ERROR(LOG_TAG, "Error occurred when opening output file");
        localMuxerStatus = MEDIA_PVR_DEMUX_CONFIG_FALED;
        goto end;
    }

#if 0
    if(testConfigFail)
    {
        LOG_ERROR(LOG_TAG, "Simulated Error occurred when opening output file");
//        testConfigFail = false;
        localMuxerStatus = MEDIA_PVR_DEMUX_CONFIG_FALED;
        goto end;
    }
#endif

    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    LOG_INFO(LOG_TAG, "write headers OUT");
    LOG_ERROR(LOG_TAG, "AVERROR(EAGAIN): %d", AVERROR(EAGAIN));

    segTempEntry = (SegmentEntry *)av_mallocz(sizeof(*segTempEntry));
    memset(segTempEntry->filename, 0, sizeof(segTempEntry->filename));
    segTempEntry->index = 0;
    segTempEntry->mplayer_ts = -1;

    while(!m_abortSegTask)
    {
        m_muxerStatus = MEDIA_PVR_DEMUX_MUX_INPROGRESS;
        LOG_DEBUG(LOG_TAG, "read frame");
        AVStream *out_stream;
        qint64 duration;

        out_stream = choose_output(ofmt_ctx);
        InputStream *ist = nullptr;

        RESET_PVR_TIMEOUT(30);
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
        {
            if(ret == (int)AVERROR_EOF || ret == (int)AVERROR_EXIT)
            {
                LOG_INFO(LOG_TAG, "End of stream detected ...");
                localMuxerStatus = MEDIA_PVR_DEMUX_ABORTED;
                break;
            }
            else if (ret == AVERROR(EINTR) || ret == AVERROR(EAGAIN))
            {
                // timeout, probably no real error, empty packet
                LOG_ERROR(LOG_TAG, "timeout, probably no real error, empty packet: %d", ret);
                av_usleep(10000);
                continue;
            }
            else
            {
                LOG_ERROR(LOG_TAG, "Error av_read_frame: %d", ret);
                if (ifmt_ctx)
                {
                    avformat_flush(ifmt_ctx);
                }
            }
        }
        LOG_DEBUG(LOG_TAG, "DEMUX ----- pkt.pts: %ld", pkt.pts);
        LOG_DEBUG(LOG_TAG, "DEMUX ----- pkt.dts: %ld", pkt.dts);
        input_pts = pkt.pts;

        ist = input_streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];

        totaldatasize += pkt.size;
        ist->nb_packets++;

        if (ist->nb_packets == 1)
        {
            if (ist->st->nb_side_data)
            {
                av_packet_split_side_data(&pkt);
            }
            for (i = 0; i < ist->st->nb_side_data; i++)
            {
                AVPacketSideData *src_sd = &ist->st->side_data[i];
                uint8_t *dst_data;

                if (av_packet_get_side_data(&pkt, src_sd->type, NULL))
                {
                    continue;
                }

                dst_data = av_packet_new_side_data(&pkt, src_sd->type, src_sd->size);
                if (!dst_data)
                {
                    LOG_ERROR(LOG_TAG, "!dst_data");
                    localMuxerStatus = MEDIA_PVR_DEMUX_ABORTED;
                    break;
                }
                memcpy(dst_data, src_sd->data, src_sd->size);
            }
        }

        if (pkt.dts != AV_NOPTS_VALUE)
            pkt.dts += av_rescale_q(ts_offset, AV_TIME_BASE_Q, ist->st->time_base);
        if (pkt.pts != AV_NOPTS_VALUE)
            pkt.pts += av_rescale_q(ts_offset, AV_TIME_BASE_Q, ist->st->time_base);

        if (pkt.pts != AV_NOPTS_VALUE)
            pkt.pts *= ist->ts_scale;
        if (pkt.dts != AV_NOPTS_VALUE)
            pkt.dts *= ist->ts_scale;

        AVRational fTime_base = (AVRational){ 1, 1 };
        duration = av_rescale_q(0, fTime_base, ist->st->time_base);
        if (pkt.pts != AV_NOPTS_VALUE)
        {
            pkt.pts += duration;
            ist->max_pts = FFMAX(pkt.pts, ist->max_pts);
            ist->min_pts = FFMIN(pkt.pts, ist->min_pts);
        }

        if (pkt.dts != AV_NOPTS_VALUE)
            pkt.dts += duration;

        if (pkt.dts != AV_NOPTS_VALUE)
            ist->last_ts = av_rescale_q(pkt.dts, ist->st->time_base, AV_TIME_BASE_Q);

        av_log(NULL, AV_LOG_DEBUG, "ist->last_ts: %ld \n", ist->last_ts);
        if( ist->st->codec->codec_id == AV_CODEC_ID_AAC &&
            !strcmp(ofmt_ctx->oformat->name, "mp4"))
        {
            LOG_DEBUG(LOG_TAG, "Apply AAC filter for mp4 format");
            AVBitStreamFilterContext* aacBitstreamFilterContext =
                                    av_bitstream_filter_init("aac_adtstoasc");
            av_bitstream_filter_filter(aacBitstreamFilterContext,
                                       ist->st->codec,
                                       NULL,
                                       &pkt.data,
                                       &pkt.size,
                                       pkt.data,
                                       pkt.size,
                                       0);
        }

#if 1
        /* light copy packet */
        pkt.pts = av_rescale_q(pkt.pts, ist->st->time_base, out_stream->time_base);
        pkt.dts = av_rescale_q(pkt.dts, ist->st->time_base, out_stream->time_base);
        pkt.duration = av_rescale_q(pkt.duration, ist->st->time_base, out_stream->time_base);
        pkt.pos = -1;
        segment_pts = pkt.pts;

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0)
        {
            LOG_ERROR(LOG_TAG, "Error muxing packet");
            localMuxerStatus = MEDIA_PVR_DEMUX_ABORTED;
            break;
        }
#else
        /* heavy copy packet for more complex streams */
        process_input_packet(ist, out_stream, &pkt);
#endif

        av_log(NULL, AV_LOG_DEBUG, "ist->nb_packets: %lu \n", ist->nb_packets);
        av_packet_unref(&pkt);

        // Hack part, but the only way to get the segment index, according to
        // avfomat documentation, we are not suppose to access this part,
        // but shuuuuut even though ffmpeg binary access it, So ffmpeg guys ...
        if(m_pvrMode == MEDIA_PVR_TRICK_MODE)
        {
            SegmentContext *seg = (SegmentContext*)ofmt_ctx->priv_data;
            LOG_DEBUG(LOG_TAG, "totaldatasize: %ld", totaldatasize);
            LOG_DEBUG(LOG_TAG,
                      "segIndex = %d::seg_count: %lu",
                      seg->segment_idx,
                      seg->segment_count);

            m_SegCounts = seg->segment_count;

            if(seg->segment_idx > 0)
            {
                if( (current_index != seg->segment_idx ) && (segTempEntry->index > 0) )
                {
                    segTempEntry->mplayer_ts =
                            (qint64)((av_q2d(ist->st->time_base) * input_pts) * 1000);
                    segTempEntry->mplayer_ts &= 0xffffffff;

                    LOG_INFO(LOG_TAG, "player timestamp: %ld", segTempEntry->mplayer_ts);
                    LOG_INFO(LOG_TAG, "input_pts: %ld", input_pts);
                    LOG_INFO(LOG_TAG, "segment_pts: %ld", segment_pts);

                    LOG_INFO(LOG_TAG, "curValidEntry.index = %d", segTempEntry->index);
                    LOG_INFO(LOG_TAG, "segTempEntry.mplayer_ts = %ld", segTempEntry->mplayer_ts);
                    LOG_INFO(LOG_TAG, "curValidEntry.start_time = %f", segTempEntry->start_time);
                    LOG_INFO(LOG_TAG, "curValidEntry.end_time = %f", segTempEntry->end_time);
                    LOG_INFO(LOG_TAG, "curValidEntry.start_pts = %ld", segTempEntry->start_pts);
                    LOG_INFO(LOG_TAG, "curValidEntry.filename = %s", segTempEntry->filename);
                    grow_segment_entries(segTempEntry);
                    //Check pvr space before any new entry
                    monitorPvrSpace();

                    evaluate_bitrate();
                    updateWritePercent();
                }
                current_index = seg->segment_idx;
                seg->time = 10000000;

                memset(segTempEntry, 0x0, sizeof(*segTempEntry));
                segTempEntry->index = seg->cur_entry.index;
                segTempEntry->start_time = seg->cur_entry.start_time;
                segTempEntry->end_time = seg->cur_entry.end_time;
                segTempEntry->start_pts = seg->cur_entry.start_pts;
                segTempEntry->offset_pts = seg->cur_entry.offset_pts;
                memset(segTempEntry->filename, 0, sizeof(segTempEntry->filename));
                strcpy(segTempEntry->filename, seg->cur_entry.filename);
            }
        }
    }
    av_write_trailer(ofmt_ctx);
    if(segTempEntry->start_pts)
        grow_segment_entries(segTempEntry);

end:
    if( m_muxerStatus == MEDIA_PVR_DEMUX_MUX_INPROGRESS)
    {
        m_muxerStatus = MEDIA_PVR_DEMUX_ABORTED;
        localMuxerStatus = MEDIA_PVR_DEMUX_ABORTED;
    }

    LOG_INFO(LOG_TAG, "clean input streams");
    //Clean up input streams

    while(!input_streams.isEmpty())
    {
        InputStream *outValue = input_streams.front();
        input_streams.pop_front();
        if(outValue)
        {
            av_frame_free(&outValue->decoded_frame);
            av_frame_free(&outValue->filter_frame);
            av_dict_free(&outValue->decoder_opts);
            avsubtitle_free(&outValue->prev_sub.subtitle);
            av_frame_free(&outValue->sub2video.frame);
            av_freep(outValue);
        }
    }

    LOG_INFO(LOG_TAG, "free seg entry");
    if(segTempEntry)
    {
        av_free(segTempEntry);
    }

    m_segListLock.lock();
    for(i=0; i<m_segmentsInfo.size(); i++)
    {
        m_pvrSessionConsumedData += m_segmentsInfo[i]->file_size;
    }
    m_segListLock.unlock();

    updatePreEnablement();

    LOG_INFO(LOG_TAG, "clear input ctx");
    avformat_close_input(&ifmt_ctx);

    LOG_INFO(LOG_TAG, "close output");
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
    {
        avio_close(ofmt_ctx->pb);
    }

    LOG_INFO(LOG_TAG, "free output context");
    if(ofmt_ctx)
    {
        avformat_free_context(ofmt_ctx);
    }

    m_mutexPending.lock();
     LOG_INFO(LOG_TAG, "Wake the Stop command.");
     m_abortSegTask = false;
     m_waitPendingCommand.wakeAll();
    m_mutexPending.unlock();

    m_muxerStatus.store(localMuxerStatus);
    LOG_INFO(LOG_TAG, "OUT Start ...");
}

/**
 * @brief MP_MediaPvr::getMuxerStatus
 * @return
 */
int MP_MediaPvr::getMuxerStatus()
{
    return m_muxerStatus.load();
}

/**
 * @brief MP_MediaPvr::initMuxerStatus
 */
void MP_MediaPvr::initMuxerStatus()
{
    m_muxerStatus = MEDIA_PVR_DEMUX_NONE;
}

/**
 * @brief MP_MediaPvr::setPvrMode
 * @param pvrMode
 */
void MP_MediaPvr::setPvrMode(MP_MediaPvrModes pvrMode)
{
    m_pvrMode = pvrMode;
    if(m_pvrMode == MEDIA_PVR_TRICK_MODE)
    {
        m_captureMode = "segment";
    }
    else if(m_pvrMode == MEDIA_PVR_REC_MODE)
    {
        m_captureMode = "mp4";
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Unhandled Pvr Mode !!");
    }
}

/**
 * @brief MP_MediaPvr::Pause
 * @param isFlushRequest
 * @return
 */
bool MP_MediaPvr::Pause()
{
    INV();
    QMutexLocker locker(&m_rawEntriesLock);
    LOG_INFO(LOG_TAG, "Activate trickmode and pause...");

    m_mediaPlayerRef = KvalMediaPlayerEngine::getGlobalRef();

    if( m_abortRawTask || !m_mediaPlayerRef )
        return false;

    if(!m_isTrickMode && !m_isTrickOnRemovable)
    {
        if(getPreEnablementValue() >= 0x280000000)
        {
            Q_EMIT okDiag(tr("Live Playback"),
                        tr("Please use an external storage device for live playback feature."));
            return false;
        }

        Q_EMIT okDiag(tr("Live Playback"),
                    tr("It is recommended that you use an external storage device "
                       "for playback recording !<br>In the meantime, "
                       "Kval will offer 500MB of internal storage temporarely !"));
    }

    if(!m_isTrickMode)
    {
        if(m_mediaPlayerRef)
            m_mediaPlayerRef->activatePvrTimeout();
        QMetaObject::invokeMethod(m_mediaPvrProvider, "createPvrServer");
    }

    m_isTrickMode = true;
    if(!m_isPausedReq)
    {
        m_mediaPlayerRef->pause();
        m_isPausedReq = true;
    }

    OUTV("m_isTrickMode: %u", (bool)m_isTrickMode);
    return m_isTrickMode;
}

/**
 * @brief MP_MediaPvr::getRemainsSegTimes
 * @param entry
 * @return
 */
qint64 MP_MediaPvr::getRemainsSegTimes(SegmentEntry * entry)
{
    qint64 remaintimes = 0;
    for(int i = 0; i < m_segmentsInfo.size(); i++)
    {
        if(entry->index < m_segmentsInfo[i]->index)
            continue;

        remaintimes += ((qint64)(m_segmentsInfo[i]->end_time * 1000) -
                       (qint64)(m_segmentsInfo[i]->start_time * 1000));

    }
    LOG_INFO(LOG_TAG, "remaintimes: %ld", remaintimes);
    return remaintimes;
}

/**
 * @brief MP_MediaPvr::getSegmentBestEffort
 * @param player_timestamp
 * @return
 */
int MP_MediaPvr::getSegmentBestEffort(int *seekedPos, int pvrPlayTime)
{
    QMutexLocker locker(&m_segListLock);
    QVectorIterator<SegmentEntry *> segEntriesIter(m_segmentsInfo);

    if(m_muxerStatus == MEDIA_PVR_DEMUX_ABORTED)
        return -EBADF;

    if(!m_segmentsInfo.size())
        return -EEXIST;

    qint64 player_position= 0;
    qint64 player_timestamp = 0;
    if(m_mediaPlayerRef)
        player_position = m_mediaPlayerRef->streamPosition();
    player_timestamp = player_position + (*seekedPos * 1000);
    LOG_INFO(LOG_TAG, "player_position: %ld", player_position);
    LOG_INFO(LOG_TAG, "player_timestamp: %ld", player_timestamp);

    if(!m_segInjectionSwitch && (player_timestamp < segEntriesIter.peekNext()->mplayer_ts))
    {
        *seekedPos = -pvrPlayTime;
        return segEntriesIter.peekNext()->index;
    }
    else if((m_segInjectionSwitch) &&
            (player_timestamp < (qint64)(segEntriesIter.peekNext()->start_time * 1000)))
    {
        *seekedPos = -pvrPlayTime;
        return segEntriesIter.peekNext()->index;
    }

    while(segEntriesIter.hasNext())
    {
        SegmentEntry * segEntry = segEntriesIter.next();
        LOG_INFO(LOG_TAG, "segEntry index: %d", segEntry->index);
        if(!m_segInjectionSwitch)
        {
            if(segEntriesIter.hasNext())
            {
                SegmentEntry *nextSegEntry = segEntriesIter.peekNext();
                if( (player_timestamp > segEntry->mplayer_ts) &&
                    (player_timestamp < nextSegEntry->mplayer_ts) )
                {
                    qint64 averageTs = (nextSegEntry->mplayer_ts + segEntry->mplayer_ts) / 2;
                    if(player_timestamp > averageTs)
                    {
                        *seekedPos = qRound(static_cast<double>((nextSegEntry->mplayer_ts - player_position) / 1000));
                        return nextSegEntry->index;
                    }
                    else
                    {
                        *seekedPos = qRound(static_cast<double>((segEntry->mplayer_ts - player_position) / 1000));
                        return segEntry->index;
                    }
                }
            }
            else
            {
                *seekedPos = qRound(static_cast<double>((segEntry->mplayer_ts - player_position) / 1000));
                return segEntry->index;
            }
        }
        else
        {
            if( (player_timestamp > (qint64)(segEntry->start_time * 1000)) &&
                (player_timestamp < (qint64)(segEntry->end_time * 1000)) )
            {
                LOG_INFO(LOG_TAG, "(%d)::segEntry startTime: %f", segEntry->index, segEntry->start_time);
                LOG_INFO(LOG_TAG, "(%d)::segEntry end_time: %f", segEntry->index, segEntry->end_time);
                qint64 averageTs = ((qint64)(segEntry->start_time * 1000) +
                                     (qint64)(segEntry->end_time * 1000)) / 2;

                if(player_timestamp > averageTs && segEntriesIter.hasNext())
                {
                    SegmentEntry *nextSegEntry = segEntriesIter.peekNext();
                    LOG_INFO(LOG_TAG, "(%d)::nextSegEntry startTime: %f", nextSegEntry->index, nextSegEntry->start_time);
                    LOG_INFO(LOG_TAG, "(%d)::nextSegEntry end_time: %f", nextSegEntry->index, nextSegEntry->end_time);

                    qint64 remaintimes = getRemainsSegTimes(nextSegEntry);
                    if(remaintimes < 15000)
                    {
                        *seekedPos =
                        qRound(static_cast<double>(((segEntry->start_time * 1000) - player_position) / 1000));
                        return segEntry->index;
                    }

                    else
                    {
                        *seekedPos =
                        qRound(static_cast<double>(((nextSegEntry->start_time * 1000) - player_position) / 1000));
                        return nextSegEntry->index;
                    }
                }
                else
                {
                    qint64 remaintimes = getRemainsSegTimes(segEntry);
                    if(remaintimes < 15000 && segEntriesIter.hasPrevious())
                    {
                        *seekedPos =
                        qRound(static_cast<double>(((segEntriesIter.peekPrevious()->start_time * 1000) - player_position) / 1000));
                        return segEntriesIter.peekPrevious()->index;
                    }
                    else
                    {
                        *seekedPos =
                        qRound(static_cast<double>(((segEntry->start_time * 1000) - player_position) / 1000));
                        return segEntry->index;
                    }
                }
            }
        }
    }

    *seekedPos = 0;
    return -EEXIST;
}


/**
 * @brief MP_MediaPvr::Seek
 * @param value
 * @param totalTime
 * @param currentTime
 * @return
 */
bool MP_MediaPvr::Seek(int value, int totalTime, int currentTime)
{
    LOG_INFO(LOG_TAG, "Seek called");
    Q_UNUSED(totalTime);

    m_mediaPlayerRef = KvalMediaPlayerEngine::getGlobalRef();

    if(!m_isTrickMode ||
        m_isPausedReq ||
        !m_segmentsInfo.size() ||
        !m_mediaPvrProvider->checkSeekEnd() ||
        !m_mediaPlayerRef )
    {
        LOG_INFO(LOG_TAG, "Could not seek in this context.");
        return false;
    }

    m_seekVal = value;

    LOG_INFO(LOG_TAG, "Seek position: %d", value);
    int seekedPos = value;
    int segIdx = getSegmentBestEffort(&seekedPos, currentTime);
    if(segIdx<0)
    {
        LOG_ERROR(LOG_TAG, "No segment found for your seek");
        return false;
    }
    LOG_INFO(LOG_TAG, "New Seek position: %d", seekedPos);
    LOG_INFO(LOG_TAG, "Got segment index: %d", segIdx);
//    LOG_INFO(LOG_TAG,
//             "  start_time: %ld",
//             (qint64)(m_segmentsInfo[i]->start_time * 1000));
//    LOG_INFO(LOG_TAG,
//             "  end_time: %ld",
//             (qint64)(m_segmentsInfo[i]->end_time * 1000));

    Q_EMIT httpSeeked(seekedPos);

    LOG_INFO(LOG_TAG, "Disable raw entries...");
    if(m_rawInjectionActive)
    {
        m_rawEntriesLock.lock();
        m_rawInjectionActive = false;
        m_rawEntriesLock.unlock();
    }

    LOG_INFO(LOG_TAG, "clear pvr data provider");
    m_mediaPvrProvider->mediaPlayerWait();
    m_mediaPvrProvider->clearPvrData();

    LOG_INFO(LOG_TAG, "enable seg injection");
    m_mediaPvrProvider->setSegmentNumber(segIdx);
    m_segInjectionSwitch = true;
    g_mediaPlayerReaderFlushed = false;

    LOG_INFO(LOG_TAG, "seek media player...");
    m_mediaPlayerRef->setPvrCb(reinterpret_cast<void*>(MP_MediaPvr::mediaPlayerReaderFlushed));
    m_mediaPlayerRef->pvrseek();

    m_seekVal = 0;

    LOG_INFO(LOG_TAG, "m_seekVal : %d", (int)m_seekVal);
    return true;
}

/**
 * @brief MP_MediaPvr::Resume
 */
void MP_MediaPvr::Resume()
{
    INV();
    LOG_INFO(LOG_TAG, "Resume called");

    m_mediaPlayerRef = KvalMediaPlayerEngine::getGlobalRef();
    if(!m_mediaPlayerRef)
        return;

    LOG_INFO(LOG_TAG, "Resume...");
    if(m_isPausedReq)
    {
        m_isPausedReq = false;
        m_mediaPlayerRef->play();
    }

    OUTV();
}

/**
 * @brief MP_MediaPvr::getTrickModeWritePercent
 * @return
 */
qreal MP_MediaPvr::getTrickModeWritePercent()
{
    return m_write_percent;
}

/**
 * @brief MP_MediaPvr::getPreEnablementValue
 * @return
 */
double MP_MediaPvr::getPreEnablementValue()
{
    if(!m_preEnablementFile->isOpen())
        m_preEnablementFile->open(QIODevice::ReadOnly);
    m_preEnablementFile->seek(0);
    QByteArray bufferarray = m_preEnablementFile->read(64);
    bool status = false;
    double dValue = bufferarray.toDouble(&status);
    if(!status)
    {
        LOG_WARNING(LOG_TAG, "Unable to read the preenablement file !");
        m_preEnablementFile->close();
        return 0.0;
    }

    LOG_INFO(LOG_TAG, "preenablement file read size: %f", dValue);
    m_preEnablementFile->close();
    return dValue;
}

/**
 * @brief MP_MediaPvr::updatePreEnablement
 */
void MP_MediaPvr::updatePreEnablement()
{
    if(m_isTrickOnRemovable)
    {
        LOG_INFO(LOG_TAG, "No limit as a trick mode is active on removable");
        return;
    }
    if(!m_pvrSessionConsumedData)
    {
        LOG_INFO(LOG_TAG, "No trick mode data written during this session");
        return;
    }
    if(!m_preEnablementFile->isOpen())
        m_preEnablementFile->open(QIODevice::ReadWrite);

    m_preEnablementFile->seek(0);
    QByteArray bufferarray = m_preEnablementFile->read(64);
    m_preEnablementFile->close();
    m_preEnablementFile->resize(0);

    bool status = false;
    double dValue = 0.0;
    dValue = bufferarray.toDouble(&status);
    if(!status)
    {
        LOG_WARNING(LOG_TAG, "Unable to read the preenablement file !");
    }
    LOG_INFO(LOG_TAG, "preenablement file read size: %f", dValue);
    LOG_INFO(LOG_TAG, "m_pvrSessionConsumedData: %f", m_pvrSessionConsumedData);
    if(m_pvrSessionConsumedData > MAX_INTERNAL_TRICK_FILE_SIZE)
        m_pvrSessionConsumedData = MAX_INTERNAL_TRICK_FILE_SIZE;
    dValue = dValue + m_pvrSessionConsumedData;
    LOG_INFO(LOG_TAG, "final value: %f", dValue);
    m_preEnablementFile->open(QIODevice::ReadWrite);
    m_preEnablementFile->seek(0);
    QByteArray valueAsBytes = QByteArray::number(dValue);
    LOG_INFO(LOG_TAG, "valueAsBytes: '%s'", valueAsBytes.toStdString().c_str());
    m_preEnablementFile->write(valueAsBytes);
    m_preEnablementFile->close();

    delete m_preEnablementFile;
}

/**
 * @brief MP_MediaPvr::fetchSegmentInfo
 * @param entry
 * @return
 */
int MP_MediaPvr::fetchSegmentInfo(SegmentEntry * entry, int index)
{
    int status = -EBUSY;
    int i = 0;
    QMutexLocker locker(&m_segListLock);
    if(m_muxerStatus == MEDIA_PVR_DEMUX_ABORTED)
        return -EBADF;

    for(i = 0; i < m_segmentsInfo.size(); i++)
    {
        if(m_segmentsInfo[i]->index == index)
        {
            memcpy(entry, m_segmentsInfo[i], sizeof(*entry));
            entry->queued = false;
            entry->consumed = false;
            entry->offset_byte = 0;
            entry->consumed_byte = 0;
            status = ENOERR;
            goto status_return;
        }
    }

    LOG_INFO(LOG_TAG, "Index[%d] Not Found...", index);
    status = -EBADF;

status_return:
    return status;
}

/**
 * @brief MP_MediaPvr::getSegCount
 * @return
 */
int MP_MediaPvr::getSegCount()
{
    return m_SegCounts;
}

/**
 * @brief MP_MediaPvr::sendDataToPlayer
 * @param data
 * @return
 */
void MP_MediaPvr::segDataReady()
{
    Q_EMIT dataAvailable();
}

/**
 * @brief read
 * @return
 */
bool MP_MediaPvr::readPvrData(QByteArray &bufferarray, bool * flush)
{
    *flush = false;
    return m_mediaPvrProvider->readData(bufferarray);
}

/**
 * @brief MP_MediaPvr::httpServerFlushed
 */
void MP_MediaPvr::httpServerFlushed()
{
    LOG_INFO(LOG_TAG, "httpServerFlushed...");
    if(!m_seekVal)
        return;

    LOG_INFO(LOG_TAG, "seek media player...");
    m_mediaPlayerRef = KvalMediaPlayerEngine::getGlobalRef();
    if(m_mediaPlayerRef)
    {
        m_mediaPlayerRef->setPvrCb(reinterpret_cast<void*>(MP_MediaPvr::mediaPlayerReaderFlushed));
        m_mediaPlayerRef->pvrseek();
    }

    LOG_INFO(LOG_TAG, "Disable raw entries...");
    m_rawEntriesLock.lock();
    m_rawInjectionActive = false;
    m_rawEntriesLock.unlock();

    LOG_INFO(LOG_TAG, "clear pvr data provider");
    m_mediaPvrProvider->clearPvrData();

    LOG_INFO(LOG_TAG, "enable seg injection");
    m_segInjectionSwitch = true;
    m_seekVal = 0;
}

/**
 * @brief MP_MediaPvr::updateWritePercent
 */
void MP_MediaPvr::updateWritePercent()
{
    qint64 total_written_size = getRawEntriesDataSize() + getSegEntriesDataSize();
    if (total_written_size >= m_pvrAvailableSpace)
    {
        LOG_INFO(LOG_TAG, "Write Loop To the beginning");
        m_write_percent = 1.0;
    }
    else
    {
        m_write_percent = (double)total_written_size / (double)m_pvrAvailableSpace;
    }
}

/**
 * @brief MP_MediaPvr::getRawEntriesDataSize
 * @return
 */
qint64 MP_MediaPvr::getRawEntriesDataSize()
{
    QMutexLocker locker(&m_rawEntriesLock);
    qint64 entriesDataSize = 0;
    for(int i = 0; i < m_rawsInfo.size(); i++)
    {
        entriesDataSize += m_rawsInfo[i]->file_size;
    }
    return entriesDataSize;
}

/**
 * @brief MP_MediaPvr::getRawEntriesDataSize
 * @return
 */
qint64 MP_MediaPvr::getSegEntriesDataSize()
{
    QMutexLocker locker(&m_segListLock);
    qint64 entriesDataSize = 0;
    for(int i = 0; i < m_segmentsInfo.size(); i++)
    {
        entriesDataSize += m_segmentsInfo[i]->file_size;
    }
    return entriesDataSize;
}

/**
 * @brief MP_MediaPvr::pvrOnRawFiles
 * @param bufferarray
 * @return
 */
bool MP_MediaPvr::pvrOnRawFiles(QByteArray bufferarray)
{
    QMutexLocker locker(&m_rawEntriesLock);
    if(m_abortRawTask)
        return false;

    if(!m_isTrickMode)
        return true;

    if(!m_rawInjectionActive)
        return false;

    LOG_DEBUG(LOG_TAG, "writeToRawFile...");
    writeToRawFile(bufferarray);
    return false;
}

/**
 * @brief MP_MediaPvr::chechRawDataAvailable
 * @return
 */
bool MP_MediaPvr::chechRawDataAvailable()
{
    QMutexLocker locker(&m_rawEntriesLock);
    if(m_abortRawTask)
        return false;

    if(!m_curRawInfoReader->consumed)
        return true;
    else if ((m_curRawInfoReader->index + 1) < m_rawsInfo.size())
        return true;
    else
        return false;
}

/**
 * @brief MP_MediaPvr::getRawInfoReader
 * @return
 */
bool MP_MediaPvr::getRawInfoReader()
{
    //First Raw read
    if(!m_curRawInfoReader)
    {
        if(m_rawsInfo.size())
        {
            m_curRawInfoReader = m_rawsInfo[0];
            return true;
        }
        else
        {
            return false;
        }
    }

    if(m_curRawInfoReader->consumed)
    {
        for(int i = 0; i < m_rawsInfo.size(); i++)
        {
            if(m_rawsInfo[i]->index == m_curRawInfoReader->index+1)
            {
                LOG_INFO(LOG_TAG, "m_curRawInfoReader.index: %d", m_curRawInfoReader->index);
                if(m_curRawInfoReader->fd->exists())
                {
                    m_curRawInfoReader->fd->remove();
                }
                LOG_INFO(LOG_TAG,
                          "m_curRawInfoReader.offset_read: %d",
                          m_curRawInfoReader->offset_read);
                LOG_INFO(LOG_TAG,
                          "m_curRawInfoReader.file_size: %d",
                          m_curRawInfoReader->file_size);
                qint64 prefill_size = m_curRawInfoReader->prefill_size;
                m_curRawInfoReader = m_rawsInfo[i];
                m_curRawInfoReader->prefill_size = prefill_size;
                return true;
            }
        }
    }
    else if (!m_curRawInfoReader->consumed)
    {
        return true;
    }

    return false;
}

/**
 * @brief MP_MediaPvr::checkExpiredSegEntries
 * @return
 */
int MP_MediaPvr::checkExpiredSegEntries(int readIndex)
{
    QMutexLocker locker(&m_segListLock);
    bool seekNeeded = false;
    while(m_segmentsInfo.size() > 0)
    {
        if(m_segmentsInfo[0]->expired)
        {
            if(readIndex == m_segmentsInfo[0]->index)
                seekNeeded = true;

            SegmentEntry *entry = m_segmentsInfo.front();
            m_segmentsInfo.pop_front();
            if(entry)
            {
                QFile fd(entry->filename);
                if(fd.exists())
                {
                    fd.remove();
                }
                av_free(entry);
            }
            continue;
        }
        break;
    }

    if(seekNeeded)
    {
        // Set this to False value so that segment reader stop reading file that has been deleted
        // Until the next seek event is executed
        m_segInjectionSwitch = false;
        Q_EMIT resume();
        Q_EMIT seek(0,0,0);
        return -EBUSY;
    }

    return ENOERR;
}
/**
 * @brief MP_MediaPvr::checkExpiredRawEntries
 * @return
 */
int MP_MediaPvr::checkExpiredRawEntries()
{
    for(int i = 0; i < m_rawsInfo.size(); i++)
    {
        if(m_rawsInfo[i]->expired)
        {
            m_rawInjectionActive = false;
            break;
        }
    }

    if(!m_rawInjectionActive)
    {
        while(m_rawsInfo.size() > 0)
        {
            RawEntry *entry = m_rawsInfo.front();
            m_rawsInfo.pop_front();
            if(entry)
            {
                if(entry->fd)
                {
                    if(entry->fd->exists())
                        entry->fd->remove();
                    delete entry->fd;
                }
                av_free(entry);
            }
        }

        Q_EMIT resume();
        Q_EMIT seek(0,0,0);
        return -EBUSY;
    }

    return ENOERR;
}

/**
 * @brief MP_MediaPvr::fillRawQueue
 * @return
 */
int MP_MediaPvr::fillRawQueue()
{
    LOG_DEBUG(LOG_TAG, "fillRawQueue");
    QMutexLocker locker(&m_rawEntriesLock);
    static int prebuf_size = 0;

    if(m_abortRawTask || m_seekVal)
    {
        return false;
    }

    int ret = checkExpiredRawEntries();
    if(ret < 0)
    {
        return ret;
    }

    if(!m_isPausedReq)
    {
        prebuf_size = 0;
    }

    if( ( prebuf_size > 0x80000 ) && m_isPausedReq)
    {
        LOG_DEBUG(LOG_TAG, "Pause Requested prebuf_size: %ld...", prebuf_size);
        return -EBUSY;
    }

    if(m_mediaPvrProvider->pvrDataQueue.dataSize() >= MAX_INTERNAL_SEGMENT_QUEUE_SIZE)
    {
        LOG_ERROR(LOG_TAG, "Wait for queue to be partially cleared");
        return -EAGAIN;
    }


    if(!getRawInfoReader())
    {
        LOG_DEBUG(LOG_TAG, "rawInfoReader unavailable");
        return -EAGAIN;
    }
    int read_size = (!m_byteRateMs.load()) ?
                        (m_curRawInfoReader->lstbufSize) :
                        (m_byteRateMs.load() * 20);


    if(!m_curRawInfoReader->fd->isOpen())
        m_curRawInfoReader->fd->open(QIODevice::ReadOnly);

    m_curRawInfoReader->fd->seek(m_curRawInfoReader->offset_read);
    QByteArray bufferarray = m_curRawInfoReader->fd->read(read_size);
    m_curRawInfoReader->offset_read = m_curRawInfoReader->offset_read + bufferarray.size();

    if( (m_curRawInfoReader->offset_read >= m_curRawInfoReader->file_size) &&
        m_curRawInfoReader->filled )
    {
        m_curRawInfoReader->consumed = true;
    }

    prebuf_size += bufferarray.size();
    m_curRawInfoReader->fd->close();

    if(!bufferarray.size())
    {
        return -EAGAIN;
    }

    LOG_DEBUG(LOG_TAG, "enqueue: %d", bufferarray.size());
    m_mediaPvrProvider->pvrDataQueue.enqueue(bufferarray);

    return ENOERR;
}

/**
 * @brief MP_MediaPvr::writeToRawFile
 * @param bufferarray
 * @return
 */
bool MP_MediaPvr::writeToRawFile(QByteArray &bufferarray)
{
    if(!m_curRawInfoWriter)
    {
        RawEntry *rawEntryTemp = (RawEntry*)av_mallocz(sizeof(RawEntry));
        m_curRawInfoWriter = rawEntryTemp;
        memset(m_curRawInfoWriter, 0, sizeof(*m_curRawInfoWriter));

        snprintf(m_curRawInfoWriter->filename,
                 sizeof(m_curRawInfoWriter->filename),
                (char*)m_rawFilePrefix.toStdString().c_str(),
                m_curRawInfoWriter->index);

        m_curRawInfoWriter->fd = new QFile(m_curRawInfoWriter->filename);
        grow_raw_entries(m_curRawInfoWriter);
    }
    else if(m_curRawInfoWriter->filled)
    {
        LOG_DEBUG(LOG_TAG,
                 "index: %d::m_curRawInfoWriter->consumed: %d",
                 m_curRawInfoWriter->index,
                 m_curRawInfoWriter->consumed);

        int prev_index = m_curRawInfoWriter->index;
        RawEntry *rawEntryTemp = (RawEntry*)av_mallocz(sizeof(RawEntry));
        m_curRawInfoWriter = rawEntryTemp;
        memset(m_curRawInfoWriter, 0x0, sizeof(*m_curRawInfoWriter));
        m_curRawInfoWriter->index = prev_index + 1;
        snprintf(m_curRawInfoWriter->filename,
                 sizeof(m_curRawInfoWriter->filename),
                (char*)m_rawFilePrefix.toStdString().c_str(),
                m_curRawInfoWriter->index);

        m_curRawInfoWriter->fd = new QFile(m_curRawInfoWriter->filename);
        grow_raw_entries(m_curRawInfoWriter);
    }

    m_curRawInfoWriter->lstbufSize = bufferarray.size();
    if(!m_curRawInfoWriter->fd->isOpen())
        m_curRawInfoWriter->fd->open(QIODevice::ReadWrite);

    //Trick Mode activated write to a file
    m_curRawInfoWriter->fd->seek(m_curRawInfoWriter->offset_write);
    m_curRawInfoWriter->fd->write(bufferarray);
    m_curRawInfoWriter->fd->close();
    m_curRawInfoWriter->offset_write = m_curRawInfoWriter->offset_write + bufferarray.size();

    if(m_rawInjectionActive && (m_byteRateMs > 0)) /* ~10 second raw file */
    {
        if (((m_curRawInfoWriter->offset_write) / (m_byteRateMs * 1000) ) > 10)
        {
            m_curRawInfoWriter->filled = true;
            m_curRawInfoWriter->file_size = m_curRawInfoWriter->offset_write;
        }
    }

    return true;
}

/**
 * @brief MP_MediaPvr::checkMP_MediaPvr
 * @param bufferarray
 * @return
 */
bool MP_MediaPvr::checkMP_MediaPvr(QByteArray bufferarray)
{
    LOG_DEBUG(LOG_TAG, "network data size: %u", bufferarray.size());
    m_mediaPvrProvider->populate(bufferarray, m_isTrickMode);

    return pvrOnRawFiles(bufferarray);
}

/**
 * @brief MP_MediaPvr::evaluate_bitrate
 * @return
 */
int MP_MediaPvr::evaluate_bitrate()
{
    QMutexLocker locker(&m_segListLock);
    if(m_muxerStatus == MEDIA_PVR_DEMUX_ABORTED)
        return -EBADF;

    qint64 segFilesSize = 0;
    qint64 segFilesMsDuration = 0;

    for(int i = 0; i < m_segmentsInfo.size(); ++i)
    {
        segFilesSize = segFilesSize + m_segmentsInfo[i]->file_size;
        segFilesMsDuration = segFilesMsDuration +
                ( ( m_segmentsInfo[i]->end_time - m_segmentsInfo[i]->start_time ) * 1000 );
    }

    if(segFilesMsDuration > 0)
    {
        m_byteRateMs = (int)(segFilesSize/segFilesMsDuration);
        LOG_INFO(LOG_TAG,
                 "===================== m_byteRateMs: %d =====================",
                 m_byteRateMs.load());
    }

    return ENOERR;
}

/**
 * @brief MP_MediaPvr::monitorPvrSpace
 * @return
 */
int MP_MediaPvr::monitorPvrSpace()
{
    //Block all raws and segments access for write
    QMutexLocker rawlocker(&m_rawEntriesLock);
    QMutexLocker seglocker(&m_segListLock);

    qint64 usedPvrSpace = 0;
    for(int i = 0; i < m_segmentsInfo.size(); i++)
    {
        usedPvrSpace += m_segmentsInfo[i]->file_size;
    }

    for(int i = 0; i < m_rawsInfo.size(); i++)
    {
        usedPvrSpace += m_rawsInfo[i]->file_size;
    }

    LOG_INFO(LOG_TAG, "===================== usedPvrSpace: %ld", usedPvrSpace);
    if(((qint64)m_pvrAvailableSpace - usedPvrSpace) > MAX_INTERNAL_PRE_BUF_QUEUE_SIZE)
    {
        LOG_INFO(LOG_TAG, "PVRMonitor::Good to go...");
        return ENOERR;
    }

    LOG_INFO(LOG_TAG, "PVRMonitor::low space, cleanup segments...");
    qint64 freedSpace = MAX_INTERNAL_PRE_BUF_QUEUE_SIZE;
    if(m_segInjectionSwitch)
    {
        for(int i = 0; i < m_segmentsInfo.size() ; i++)
        {
            LOG_INFO(LOG_TAG, "PVRMonitor::Set Seg id[%d] as expired...", m_segmentsInfo[i]->index);
            freedSpace -= m_segmentsInfo[i]->file_size;
            m_segmentsInfo[i]->expired = true;
            if(freedSpace <= 0)
                break;
        }
    }
    else if(m_rawInjectionActive)
    {
        for(int i = 0; i < m_rawsInfo.size(); i++)
        {
            if(m_rawsInfo[i]->filled)
            {
                LOG_INFO(LOG_TAG, "PVRMonitor::Set raw id[%d] as expired...", m_rawsInfo[i]->index);
                freedSpace -= m_rawsInfo[i]->file_size;
                m_rawsInfo[i]->expired = true;
            }
            else
            {
                LOG_INFO(LOG_TAG, "PVRMonitor::Reach last raw...");
            }
            if(freedSpace <= 0)
                break;
        }
    }

    return ENOERR;
}

/**
 * @brief MP_MediaPvr::grow_segment_entries
 * @return
 */
int MP_MediaPvr::grow_segment_entries(SegmentEntry *segTempEntry)
{
    QMutexLocker locker(&m_segListLock);
    if(m_muxerStatus == MEDIA_PVR_DEMUX_ABORTED)
        return -EBADF;

    if(!segTempEntry)
    {
        LOG_ERROR(LOG_TAG, "segTempEntry NULL !!");
        return -EINVAL;
    }

    //Add the new entry
    SegmentEntry * segNewEntry;
    segNewEntry = (SegmentEntry *)av_mallocz(sizeof(*segNewEntry));
    memset(segNewEntry, 0x0, sizeof(*segNewEntry));
    memset(segNewEntry->filename, 0, sizeof(segNewEntry->filename));

    segNewEntry->index = segTempEntry->index;
    segNewEntry->mplayer_ts = segTempEntry->mplayer_ts;
    segNewEntry->queued = false;
    segNewEntry->consumed = false;
    segNewEntry->expired = false;
    segNewEntry->start_time = segTempEntry->start_time;
    segNewEntry->end_time = segTempEntry->end_time;
    segNewEntry->start_pts = segTempEntry->start_pts;
    segNewEntry->offset_pts = segTempEntry->offset_pts;
    segNewEntry->offset_byte = 0;
    segNewEntry->consumed_byte = 0;
    
    strncpy(segNewEntry->filename,
            m_pvrFolder.toStdString().c_str(),
            m_pvrFolder.size());
    strcat(segNewEntry->filename, "/");
    strncat(segNewEntry->filename,
            segTempEntry->filename,
            strlen(segTempEntry->filename));
    segNewEntry->filename[strlen(segNewEntry->filename)] = 0;

    QFile segFile(segNewEntry->filename);
    segFile.open(QIODevice::ReadOnly);
    segNewEntry->file_size = segFile.size();
    segFile.close();
    m_segmentsInfo.append(segNewEntry);
    return ENOERR;
}

/**
 * @brief MP_MediaPvr::grow_raw_entries
 * @param rawTempEntry
 * @return
 */
int MP_MediaPvr::grow_raw_entries(RawEntry *rawTempEntry)
{
    if(!rawTempEntry)
    {
        LOG_ERROR(LOG_TAG, "rawTempEntry NULL !!");
        return -EINVAL;
    }

    m_rawsInfo.append(rawTempEntry);
    return ENOERR;
}

/**
 * @brief MP_MediaPvr::isSegInjectionSwitch
 * @return
 */
bool MP_MediaPvr::isSegInjectionSwitch()
{
    return m_segInjectionSwitch;
}

/**
 * @brief MP_MediaPvr::isRawInjectionActive
 * @return
 */
bool MP_MediaPvr::isRawInjectionActive()
{
    return m_rawInjectionActive;
}

/**
 * @brief MP_MediaPvr::mediaPlayerReaderFlushed
 * @return
 */
void MP_MediaPvr::mediaPlayerReaderFlushed()
{
    LOG_INFO(LOG_TAG, "media player reader flushed");
    g_mediaPlayerReaderFlushed = true;
}

/**
 * @brief MP_MediaPvr::mediaPlayerReadyNotify
 */
void MP_MediaPvr::mediaPlayerReadyNotify()
{
    m_mediaPvrProvider->mediaPlayerReadyNotify();
}

/**
 * @brief MP_MediaPvr::avSeekToKeyFrame
 * @return
 */
bool MP_MediaPvr::avSeekToKeyFrame(qint64 * trickReadCurrentPos)
{
    if(m_pFormatContext)
    {
        LOG_DEBUG(LOG_TAG, "Already opened file stream");
        return true;
    }

    av_log_set_level(AV_LOG_QUIET);
    m_pFormatContext = avformat_alloc_context();
    m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;
    int ret = avformat_open_input(&m_pFormatContext,
                                  m_trickModeFilePath.toStdString().c_str() ,
                                  NULL,
                                  NULL);
    if (ret < 0)
    {
        LOG_INFO(LOG_TAG,"avformat_open_input failed");
        avformat_free_context(m_pFormatContext);
        m_pFormatContext = NULL;
        return false;
    }

    ret = avformat_find_stream_info(m_pFormatContext, NULL);
    if (ret < 0)
    {
        LOG_ERROR(LOG_TAG, "Could not find stream info !!");
        avformat_free_context(m_pFormatContext);
        m_pFormatContext = NULL;
        return false;
    }

    LOG_INFO(LOG_TAG, "Open success file stream");

    int seekret = avformat_seek_file(m_pFormatContext,
                                     -1,
                                     INT64_MIN,
                                     *trickReadCurrentPos,
                                     INT64_MAX,
                                     AVSEEK_FLAG_BYTE | AVSEEK_FLAG_BACKWARD);
    if(seekret < 0)
    {
        LOG_ERROR(LOG_TAG,
                 "av_seek_frame failed, Seeking failed: %d",
                 seekret);
    }
    else
    {
        //look for valid frame to start read from when after seeking
        while(true)
        {
            AVPacket* avpkt;
            avpkt = (AVPacket*)calloc(1, sizeof(*avpkt));
            av_init_packet(avpkt);

            int ret = av_read_frame(m_pFormatContext, avpkt);
            if (ret < 0)
            {
                LOG_ERROR(LOG_TAG, "Error av_read_frame: %d", ret);
                av_packet_unref(avpkt);
                break;
            }

            if(avpkt->pos > 0)
            {
                *trickReadCurrentPos = avpkt->pos;
                LOG_INFO(LOG_TAG, ">>>>>>>>> frame_seek_position: %lld", *trickReadCurrentPos);
                LOG_INFO(LOG_TAG, ">>>>>>>>> pts: %lld", avpkt->pts);
                av_packet_unref(avpkt);
                break;
            }
            av_packet_unref(avpkt);
        }
    }

    avformat_free_context(m_pFormatContext);
    m_pFormatContext = NULL;
    return true;

}


/**
 * @brief MP_MediaPvr::add_output_stream
 * @param output_format_context
 * @param input_stream
 * @return
 */
AVStream *MP_MediaPvr::add_output_stream(AVFormatContext *output_format_context,
                                      AVStream *input_stream)
{
    AVCodec *dec = NULL;
    AVRational sar;
    quint64 extra_size;
    int copy_tb = -1;

    AVCodecContext *input_codec_context;
    AVCodecContext *output_codec_context;
    AVStream *output_stream;

    output_stream = avformat_new_stream(output_format_context, dec);
    if (!output_stream)
    {
        fprintf(stderr, "Could not allocate stream\n");
        return 0;
    }

    output_stream->id = 0;

    input_codec_context = input_stream->codec;
    output_codec_context = output_stream->codec;

    extra_size = (quint64)input_codec_context->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE;

    if (extra_size > INT_MAX)
    {
        LOG_ERROR(LOG_TAG, "extra_size > INT_MAX");
        return 0;
    }

    output_codec_context->codec_id   = input_codec_context->codec_id;
    output_codec_context->codec_type = input_codec_context->codec_type;
    output_codec_context->codec_tag = 0;

    if (!output_codec_context->codec_tag)
    {
        unsigned int codec_tag;
        if (!output_format_context->oformat->codec_tag ||
            av_codec_get_id(output_format_context->oformat->codec_tag,
                            input_codec_context->codec_tag) == output_codec_context->codec_id ||
                            !av_codec_get_tag2(output_format_context->oformat->codec_tag,
                                               input_codec_context->codec_id, &codec_tag))
        {
            output_codec_context->codec_tag = input_codec_context->codec_tag;
        }
    }

    output_codec_context->bit_rate       = input_codec_context->bit_rate;
    output_codec_context->rc_max_rate    = input_codec_context->rc_max_rate;
    output_codec_context->rc_buffer_size = input_codec_context->rc_buffer_size;
    output_codec_context->field_order    = input_codec_context->field_order;
    if (input_codec_context->extradata_size)
    {
        output_codec_context->extradata      = (uint8_t *)av_mallocz(extra_size);
        if (!output_codec_context->extradata)
        {
            LOG_ERROR(LOG_TAG, "unable to alloc extra size");
            return 0;
        }
        memcpy(output_codec_context->extradata,
               input_codec_context->extradata,
               input_codec_context->extradata_size);
    }
    output_codec_context->extradata_size= input_codec_context->extradata_size;
    output_codec_context->bits_per_coded_sample  = input_codec_context->bits_per_coded_sample;
    output_codec_context->time_base = input_stream->time_base;

    if(!(output_format_context->oformat->flags & AVFMT_VARIABLE_FPS)
        && strcmp(output_format_context->oformat->name, "mp4"))
    {
        if((  copy_tb < 0 && input_codec_context->time_base.den
                          && av_q2d(input_codec_context->time_base)*
                                    input_codec_context->ticks_per_frame >
                                        av_q2d(input_stream->time_base)
                          && av_q2d(input_stream->time_base) < 1.0/500 )
           || copy_tb == 0)
        {
            output_codec_context->time_base = input_codec_context->time_base;
            output_codec_context->time_base.num *= input_codec_context->ticks_per_frame;
        }
    }

    av_reduce(&output_codec_context->time_base.num, &output_codec_context->time_base.den,
                output_codec_context->time_base.num, output_codec_context->time_base.den, INT_MAX);


    switch (input_codec_context->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            output_codec_context->channel_layout = input_codec_context->channel_layout;
            output_codec_context->sample_rate = input_codec_context->sample_rate;
            output_codec_context->channels = input_codec_context->channels;
            output_codec_context->frame_size = input_codec_context->frame_size;
            output_codec_context->block_align = input_codec_context->block_align;
            output_codec_context->initial_padding    = input_codec_context->delay;
            output_codec_context->profile            = input_codec_context->profile;
            output_codec_context->audio_service_type = input_codec_context->audio_service_type;

            if ((input_codec_context->block_align == 1 ||
                 input_codec_context->block_align == 1152 ||
                 input_codec_context->block_align == 576) &&
                input_codec_context->codec_id == AV_CODEC_ID_MP3)
            {
                output_codec_context->block_align = 0;
            }
            if (input_codec_context->codec_id == AV_CODEC_ID_AC3)
            {
                output_codec_context->block_align = 0;
            }
            if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
                output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }

            break;
        case AVMEDIA_TYPE_VIDEO:
            output_codec_context->pix_fmt            = input_codec_context->pix_fmt;
            output_codec_context->colorspace         = input_codec_context->colorspace;
            output_codec_context->color_range        = input_codec_context->color_range;
            output_codec_context->color_primaries    = input_codec_context->color_primaries;
            output_codec_context->color_trc          = input_codec_context->color_trc;
            output_codec_context->width              = input_codec_context->width;
            output_codec_context->height             = input_codec_context->height;
            output_codec_context->has_b_frames       = input_codec_context->has_b_frames;
            sar = input_codec_context->sample_aspect_ratio;

            output_stream->sample_aspect_ratio = sar;
            output_stream->avg_frame_rate = input_stream->avg_frame_rate;
            output_stream->r_frame_rate = input_stream->r_frame_rate;
            break;
    default:
        break;
    }


    return output_stream;
}
#if 1


/**
 * @brief MP_MediaPvr::do_streamcopy
 * @param ist
 * @param pkt
 */
int MP_MediaPvr::do_streamcopy(InputStream *ist, AVStream *ost, AVPacket *pkt)
{
    qint64 start_time = 0;
    qint64 ost_tb_start_time =
            av_rescale_q(start_time, AV_TIME_BASE_Q, ost->time_base);
    AVPacket opkt;

    av_init_packet(&opkt);

    if (pkt->pts != AV_NOPTS_VALUE)
        opkt.pts = av_rescale_q(pkt->pts, ist->st->time_base, ost->time_base) - ost_tb_start_time;
    else
        opkt.pts = AV_NOPTS_VALUE;

    if (pkt->dts == AV_NOPTS_VALUE)
        opkt.dts = av_rescale_q(ist->dts, AV_TIME_BASE_Q, ost->time_base);
    else
        opkt.dts = av_rescale_q(pkt->dts, ist->st->time_base, ost->time_base);
    opkt.dts -= ost_tb_start_time;

    if (ost->codec->codec_type == AVMEDIA_TYPE_AUDIO && pkt->dts != AV_NOPTS_VALUE)
    {
        int duration = av_get_audio_frame_duration(ist->dec_ctx, pkt->size);
        if(!duration)
            duration = ist->dec_ctx->frame_size;
        opkt.dts = opkt.pts = av_rescale_delta(ist->st->time_base, pkt->dts,
                                               (AVRational){1, ist->dec_ctx->sample_rate},
                                               duration,
                                               &ist->filter_in_rescale_delta_last,
                                               ost->time_base) - ost_tb_start_time;
    }

    opkt.duration = av_rescale_q(pkt->duration, ist->st->time_base, ost->time_base);
    opkt.flags    = pkt->flags;

    if (  ost->codec->codec_id != AV_CODEC_ID_H264
       && ost->codec->codec_id != AV_CODEC_ID_MPEG1VIDEO
       && ost->codec->codec_id != AV_CODEC_ID_MPEG2VIDEO
       && ost->codec->codec_id != AV_CODEC_ID_VC1
       )
    {
        AVCodecParserContext *parser = av_parser_init(ost->codec->codec_id);
        int ret = av_parser_change(parser, ost->codec,
                             &opkt.data, &opkt.size,
                             pkt->data, pkt->size,
                             pkt->flags & AV_PKT_FLAG_KEY);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_FATAL, "av_parser_change failed \n");
            return -EINVAL;
        }
        if (ret)
        {
            opkt.buf = av_buffer_create(opkt.data, opkt.size, av_buffer_default_free, NULL, 0);
            if (!opkt.buf)
                return -EINVAL;
        }
    }
    else
    {
        opkt.data = pkt->data;
        opkt.size = pkt->size;
    }
    av_copy_packet_side_data(&opkt, pkt);

    write_frame(ofmt_ctx, &opkt, ost);
    return ENOERR;
}

/**
 * @brief MP_MediaPvr::write_frame
 * @param s
 * @param pkt
 * @param ost
 */
void MP_MediaPvr::write_frame(AVFormatContext *s, AVPacket *pkt, AVStream *ost)
{
    AVCodecContext          *avctx = ost->codec;
    int ret;

#if 0
    if ((avctx->codec_type == AVMEDIA_TYPE_VIDEO && video_sync_method == VSYNC_DROP) ||
        (avctx->codec_type == AVMEDIA_TYPE_AUDIO && audio_sync_method < 0))
        pkt->pts = pkt->dts = AV_NOPTS_VALUE;

    /*
     * Audio encoders may split the packets --  #frames in != #packets out.
     * But there is no reordering, so we can limit the number of output packets
     * by simply dropping them here.
     * Counting encoded video frames needs to be done separately because of
     * reordering, see do_video_out()
     */
    if (!(avctx->codec_type == AVMEDIA_TYPE_VIDEO && avctx->codec)) {
        if (ost->frame_number >= ost->max_frames) {
            av_packet_unref(pkt);
            return;
        }
        ost->frame_number++;
    }

    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        int i;
        uint8_t *sd = av_packet_get_side_data(pkt, AV_PKT_DATA_QUALITY_STATS,
                                              NULL);
        ost->quality = sd ? AV_RL32(sd) : -1;
        ost->pict_type = sd ? sd[4] : AV_PICTURE_TYPE_NONE;

        for (i = 0; i<FF_ARRAY_ELEMS(ost->error); i++) {
            if (sd && i < sd[5])
                ost->error[i] = AV_RL64(sd + 8 + 8*i);
            else
                ost->error[i] = -1;
        }

        if (ost->frame_rate.num && ost->is_cfr)
        {
            if (pkt->duration > 0)
                av_log(NULL, AV_LOG_WARNING, "Overriding packet duration by frame rate, this should not happen\n");
            pkt->duration = av_rescale_q(1, av_inv_q(ost->frame_rate),
                                         ost->st->time_base);
        }
    }
#endif

    if (pkt->size == 0 && pkt->side_data_elems == 0)
        return;

    if (!ost->codecpar->extradata && avctx->extradata)
    {
        ost->codecpar->extradata =
                (uint8_t*)av_malloc(avctx->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
        if (!ost->codecpar->extradata)
        {
            av_log(NULL, AV_LOG_ERROR, "Could not allocate extradata buffer to copy parser data.\n");
            return;
        }
        ost->codecpar->extradata_size = avctx->extradata_size;
        memcpy(ost->codecpar->extradata, avctx->extradata, avctx->extradata_size);
    }

#if 0
    if (!(s->oformat->flags & AVFMT_NOTIMESTAMPS))
    {
        if (pkt->dts != AV_NOPTS_VALUE &&
            pkt->pts != AV_NOPTS_VALUE &&
            pkt->dts > pkt->pts) {
            av_log(s, AV_LOG_WARNING, "Invalid DTS: %"PRId64" PTS: %"PRId64" in output stream %d, replacing by guess\n",
                   pkt->dts, pkt->pts,
                   ost->index);
            pkt->pts =
            pkt->dts = pkt->pts + pkt->dts + ost->last_mux_dts + 1
                     - FFMIN3(pkt->pts, pkt->dts, ost->last_mux_dts + 1)
                     - FFMAX3(pkt->pts, pkt->dts, ost->last_mux_dts + 1);
        }
     if(
        (avctx->codec_type == AVMEDIA_TYPE_AUDIO || avctx->codec_type == AVMEDIA_TYPE_VIDEO) &&
        pkt->dts != AV_NOPTS_VALUE &&
        !(avctx->codec_id == AV_CODEC_ID_VP9 && ost->stream_copy) &&
        ost->last_mux_dts != AV_NOPTS_VALUE) {
      qint64 max = ost->last_mux_dts + !(s->oformat->flags & AVFMT_TS_NONSTRICT);
      if (pkt->dts < max) {
        int loglevel = max - pkt->dts > 2 || avctx->codec_type == AVMEDIA_TYPE_VIDEO ? AV_LOG_WARNING : AV_LOG_DEBUG;
        av_log(s, loglevel, "Non-monotonous DTS in output stream "
               "%d:%d; previous: %"PRId64", current: %"PRId64"; ",
               ost->file_index, ost->st->index, ost->last_mux_dts, pkt->dts);
        if (exit_on_error) {
            av_log(NULL, AV_LOG_FATAL, "aborting.\n");
            exit_program(1);
        }
        av_log(s, loglevel, "changing to %"PRId64". This may result "
               "in incorrect timestamps in the output file.\n",
               max);
        if(pkt->pts >= pkt->dts)
            pkt->pts = FFMAX(pkt->pts, max);
        pkt->dts = max;
      }
     }
    }
    ost->last_mux_dts = pkt->dts;

    ost->data_size += pkt->size;
    ost->packets_written++;
#endif

    pkt->stream_index = ost->index;

    ret = av_interleaved_write_frame(s, pkt);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "av_interleaved_write_frame(): %d \n", ret);
    }
    av_packet_unref(pkt);
}

/**
 * @brief process_input_packet
 * @param ist
 * @param pkt
 * @return
 */
int MP_MediaPvr::process_input_packet(InputStream *ist, AVStream *ost, AVPacket *pkt)
{
    int got_output = 0;

    AVPacket avpkt;
    if (!ist->saw_first_ts)
    {
        av_log(NULL, AV_LOG_ERROR,
               "============ saw_first_ts ============ \n");

        ist->dts = ist->st->avg_frame_rate.num ? - ist->dec_ctx->has_b_frames * AV_TIME_BASE / av_q2d(ist->st->avg_frame_rate) : 0;
        ist->pts = 0;
        if (pkt && pkt->pts != AV_NOPTS_VALUE && !ist->decoding_needed) {
            ist->dts += av_rescale_q(pkt->pts, ist->st->time_base, AV_TIME_BASE_Q);
            ist->pts = ist->dts; //unused but better to set it to a value thats not totally wrong
        }
        ist->saw_first_ts = 1;
    }

    if (ist->next_dts == AV_NOPTS_VALUE)
        ist->next_dts = ist->dts;
    if (ist->next_pts == AV_NOPTS_VALUE)
        ist->next_pts = ist->pts;

    if (!pkt)
    {
        /* EOF handling */
        av_init_packet(&avpkt);
        avpkt.data = NULL;
        avpkt.size = 0;
        av_log(NULL, AV_LOG_WARNING, "goto handle_eof \n");
    }
    else
    {
        avpkt = *pkt;
    }

    if (pkt->dts != AV_NOPTS_VALUE)
    {
        ist->next_dts = ist->dts = av_rescale_q(pkt->dts, ist->st->time_base, AV_TIME_BASE_Q);
        if (ist->dec_ctx->codec_type != AVMEDIA_TYPE_VIDEO || !ist->decoding_needed)
            ist->next_pts = ist->pts = ist->dts;
    }

    /* handle stream copy */
    ist->dts = ist->next_dts;
    switch (ist->dec_ctx->codec_type)
    {
        case AVMEDIA_TYPE_AUDIO:
            ist->next_dts += ((qint64)AV_TIME_BASE * ist->dec_ctx->frame_size) /
                             ist->dec_ctx->sample_rate;
            break;
        case AVMEDIA_TYPE_VIDEO:
            if (ist->framerate.num)
            {
                // TODO: Remove work-around for c99-to-c89 issue 7
                AVRational time_base_q = AV_TIME_BASE_Q;
                qint64 next_dts = av_rescale_q(ist->next_dts, time_base_q, av_inv_q(ist->framerate));
                ist->next_dts = av_rescale_q(next_dts + 1, av_inv_q(ist->framerate), time_base_q);
            }
            else if (pkt->duration)
            {
                ist->next_dts += av_rescale_q(pkt->duration, ist->st->time_base, AV_TIME_BASE_Q);
            }
            else if(ist->dec_ctx->framerate.num != 0)
            {
                int ticks= av_stream_get_parser(ist->st) ? av_stream_get_parser(ist->st)->repeat_pict + 1 : ist->dec_ctx->ticks_per_frame;
                ist->next_dts += ((qint64)AV_TIME_BASE *
                                  ist->dec_ctx->framerate.den * ticks) /
                                  ist->dec_ctx->framerate.num / ist->dec_ctx->ticks_per_frame;
            }
            break;
        case AVMEDIA_TYPE_DATA:
        case AVMEDIA_TYPE_SUBTITLE:
        case AVMEDIA_TYPE_ATTACHMENT:
        case AVMEDIA_TYPE_NB:
        default:
            break;
    }
    ist->pts = ist->dts;
    ist->next_pts = ist->next_dts;

    do_streamcopy(ist, ost, pkt);

    return got_output;
}
#endif
/**
 * @brief choose_output
 * @param ofmt_ctx
 * @return
 */
AVStream * MP_MediaPvr::choose_output(AVFormatContext *ofmt_ctx)
{
    unsigned int i;
    qint64 opts_min = INT64_MAX;
    AVStream *ost_min = NULL;

    for (i = 0; i < ofmt_ctx->nb_streams; i++)
    {
        AVStream *ost = ofmt_ctx->streams[i];
        qint64 opts = (ost->cur_dts == AV_NOPTS_VALUE) ?
                        INT64_MIN :
                        av_rescale_q(ost->cur_dts, ost->time_base, AV_TIME_BASE_Q);
        if (ost->cur_dts == AV_NOPTS_VALUE)
        {
            LOG_INFO(LOG_TAG,
            "cur_dts is invalid (this is harmless if it occurs once at the start per stream)");
        }

        if (opts < opts_min)
        {
            av_log(NULL, AV_LOG_DEBUG, "Stream finished \n");
            opts_min = opts;
            ost_min = ost;
        }
    }
    return ost_min;
}
