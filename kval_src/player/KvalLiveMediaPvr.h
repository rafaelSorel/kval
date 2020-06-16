#include <QObject>
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
#include <QLocalServer>
#include <QReadWriteLock>
#include <QPointer>

#include "KvalPlayerPlatform.h"
#include "KvalThreadUtils.h"

using namespace std;

typedef enum PvrModes {
    MEDIA_PVR_INIT_MODE = 0,
    MEDIA_PVR_TRICK_MODE,
    MEDIA_PVR_REC_MODE,
    MEDIA_PVR_MAX_MODE
} MP_MediaPvrModes;


typedef enum DemuxStatus {
    MEDIA_PVR_DEMUX_NONE = 0,
    MEDIA_PVR_DEMUX_INIT,
    MEDIA_PVR_DEMUX_INIT_FALED,
    MEDIA_PVR_DEMUX_CONFIG_FALED,
    MEDIA_PVR_DEMUX_MUX_INPROGRESS,
    MEDIA_PVR_DEMUX_MUX_FALED,
    MEDIA_PVR_DEMUX_ABORTED,
    MEDIA_PVR_DEMUX_MAX
} DemuxStatus;

/** This structure is highly dependent on libav versions,
 * please update on libav update*/
typedef struct SegmentListEntry {
    int index;
    double start_time, end_time;
    int64_t start_pts;
    int64_t offset_pts;
    char *filename;
    struct SegmentListEntry *next;
    int64_t last_duration;
} SegmentListEntry;

typedef struct SegmentEntry {
    int index;
    bool queued;
    bool consumed;
    bool expired;
    double start_time, end_time;
    int64_t start_pts;
    int64_t mplayer_ts;
    int64_t offset_pts;
    int64_t offset_byte;
    int64_t consumed_byte;
    int64_t file_size;
    char filename[128];
} SegmentEntry;

typedef struct RawEntry {
    int index;
    bool consumed;
    bool filled;
    bool expired;
    int64_t prefill_size;
    int64_t offset_read;
    int64_t offset_write;
    int64_t lstbufSize;
    int64_t file_size;
    QFile * fd;
    char filename[128];
} RawEntry;

/** This structure is highly dependent on libav versions,
 * please update on libav update*/
typedef struct SegmentContext {
    const void *dummyclass;  /**< Class for private options. */
    int segment_idx;       ///< index of the segment file to write, starting from 0
    int segment_idx_wrap;  ///< number after which the index wraps
    int segment_idx_wrap_nb;  ///< number of time the index has wraped
    int segment_count;     ///< number of segment files already written
    AVOutputFormat *oformat;
    AVFormatContext *avf;
    char *format;              ///< format to use for output segment files
    char *format_options_str;  ///< format options to use for output segment files
    AVDictionary *format_options;
    char *list;            ///< filename for the segment list file
    int   list_flags;      ///< flags affecting list generation
    int   list_size;       ///< number of entries for the segment list file

    int use_clocktime;    ///< flag to cut segments at regular clock time
    int64_t clocktime_offset; //< clock offset for cutting the segments at regular clock time
    int64_t clocktime_wrap_duration; //< wrapping duration considered for starting a new segment
    int64_t last_val;      ///< remember last time for wrap around detection
    int64_t last_cut;      ///< remember last cut
    int cut_pending;

    char *entry_prefix;    ///< prefix to add to list entry filenames
    int list_type;         ///< set the list type
    AVIOContext *list_pb;  ///< list file put-byte context
    char *time_str;        ///< segment duration specification string
    int64_t time;          ///< segment duration
    int use_strftime;      ///< flag to expand filename with strftime
    int increment_tc;      ///< flag to increment timecode if found

    char *times_str;       ///< segment times specification string
    int64_t *times;        ///< list of segment interval specification
    int nb_times;          ///< number of elments in the times array

    char *frames_str;      ///< segment frame numbers specification string
    int *frames;           ///< list of frame number specification
    int nb_frames;         ///< number of elments in the frames array
    int frame_count;       ///< total number of reference frames
    int segment_frame_count; ///< number of reference frames in the segment

    int64_t time_delta;
    int  individual_header_trailer; /**< Set by a private option. */
    int  write_header_trailer; /**< Set by a private option. */
    char *header_filename;  ///< filename to write the output header to

    int reset_timestamps;  ///< reset timestamps at the begin of each segment
    int64_t initial_offset;    ///< initial timestamps offset, expressed in microseconds
    char *reference_stream_specifier; ///< reference stream specifier
    int   reference_stream_index;
    int   break_non_keyframes;
    int   write_empty;

    int use_rename;
    char temp_list_filename[1024];

    SegmentListEntry cur_entry;
    SegmentListEntry *segment_list_entries;
    SegmentListEntry *segment_list_entries_end;
} SegmentContext;

typedef struct InputStream {
    int file_index;
    AVStream *st;
    int discard;             /* true if stream data should be discarded */
    int user_set_discard;
    int decoding_needed;     /* non zero if the packets must be decoded in 'raw_fifo', see DECODING_FOR_* */
#define DECODING_FOR_OST    1
#define DECODING_FOR_FILTER 2

    AVCodecContext *dec_ctx;
    AVCodec *dec;
    AVFrame *decoded_frame;
    AVFrame *filter_frame; /* a ref of decoded_frame, to be sent to filters */

    int64_t       start;     /* time when read started */
    /* predicted dts of the next packet read for this stream or (when there are
     * several frames in a packet) of the next frame in current packet (in AV_TIME_BASE units) */
    int64_t       next_dts;
    int64_t       dts;       ///< dts of the last packet read for this stream (in AV_TIME_BASE units)

    int64_t       next_pts;  ///< synthetic pts for the next decode frame (in AV_TIME_BASE units)
    int64_t       pts;       ///< current pts of the decoded frame  (in AV_TIME_BASE units)
    int           wrap_correction_done;

    int64_t last_ts;

    int64_t filter_in_rescale_delta_last;

    int64_t min_pts; /* pts with the smallest value in a current stream */
    int64_t max_pts; /* pts with the higher value in a current stream */
    int64_t nb_samples; /* number of samples in the last decoded audio frame before looping */

    double ts_scale;
    int saw_first_ts;
    int showed_multi_packet_warning;
    AVDictionary *decoder_opts;
    AVRational framerate;               /* framerate forced with -r */
    int top_field_first;
    int guess_layout_max;

    int autorotate;
    int resample_height;
    int resample_width;
    int resample_pix_fmt;

    int      resample_sample_fmt;
    int      resample_sample_rate;
    int      resample_channels;
    uint64_t resample_channel_layout;

    int fix_sub_duration;
    struct { /* previous decoded subtitle and related variables */
        int got_output;
        int ret;
        AVSubtitle subtitle;
    } prev_sub;

    struct sub2video {
        int64_t last_pts;
        int64_t end_pts;
        AVFrame *frame;
        int w, h;
    } sub2video;

    int dr1;

    /* stats */
    // combined size of all the packets read
    uint64_t data_size;
    /* number of packets successfully read for this stream */
    uint64_t nb_packets;
    // number of frames/samples retrieved from the decoder
    uint64_t frames_decoded;
    uint64_t samples_decoded;
} InputStream;


class MP_SharedQueue
{
public:
    qint64 dataSize()
    {
        QMutexLocker locker(&mMutex);
        return m_dataSize;
    }
    bool isEmpty()
    {
        QMutexLocker locker(&mMutex);
        return mData.isEmpty();
    }
    bool size()
    {
        QMutexLocker locker(&mMutex);
        return mData.size();
    }
    void enqueue(QByteArray array)
    {
        QMutexLocker locker(&mMutex);
        m_dataSize = m_dataSize + array.size();
        mData.enqueue(array);
    }
    void prepend(QByteArray array)
    {
        QMutexLocker locker(&mMutex);
        mData.push_front(array);
    }

    QByteArray dequeue()
    {
        QMutexLocker locker(&mMutex);
        QByteArray array = mData.dequeue();
        m_dataSize = m_dataSize - array.size();
        return array;
    }
    void clear()
    {
        QMutexLocker locker(&mMutex);
        mData.clear();
    }

private:
    QMutex mMutex;
    int64_t m_dataSize = 0;
    QQueue<QByteArray> mData;
};


class MP_MediaPvr;
class KvalHttpStreamWorker;

/**
 * @brief The MP_MediaPvrProvider class
 */
class MP_MediaPvrProvider : public QObject
{
    Q_OBJECT

public:
    MP_MediaPvrProvider(MP_MediaPvr * owner);
    virtual ~MP_MediaPvrProvider();

    QString getPvrSrvAddr();
    void populate(QByteArray data, bool isActiveTrickMode);
    int send(char *, qint64);
    void Close();

    MP_SharedQueue pvrDataQueue;

Q_SIGNALS:
    void startMuxer();
    void stopTimer();
    void finished();

public Q_SLOTS:
    void onNewConnection();
    void txRx();
    void closingClient();
    void createPvrServer();
    void process();
    bool readData(QByteArray &data);
    void clearPvrData();
    void mediaPlayerReadyNotify();
    void mediaPlayerWait();
    bool checkSeekEnd();
    void setSegmentNumber(int index);

private:
    int fillSegmentQueue();
    bool getRawInfoReader();

    QPointer<MP_MediaPvr> m_owner;
    QThread * m_thread;
    QMutex m_mutex;
    QTimer * m_processTimer;
    MP_SharedQueue m_preLoadDataQueue;
    SegmentEntry m_segActiveEntry;

    QTcpServer * m_server;
    QTcpSocket * m_socket;
    bool m_sessionEstablished =false;
    bool m_stopRequested;
    QString m_streamAddrStr;
    bool m_mediaPlayerReady = true;
};

/**
 * @brief The MP_MediaPvr class
 */
class MP_MediaPvr : public QObject
{
    Q_OBJECT
public:
    MP_MediaPvr();
    virtual ~MP_MediaPvr();

    void setPvrMode(MP_MediaPvrModes pvrMode);
    void cleanup();

    bool readPvrData(QByteArray &bufferarray, bool * flush);
    bool avSeekToKeyFrame(qint64 * trickReadCurrentPos);
    bool isSegInjectionSwitch();
    bool isRawInjectionActive();
    void initMuxerStatus();
    int getMuxerStatus();

    static void mediaPlayerReaderFlushed();
    bool m_pvrReady = false;
    atomic_bool m_isPausedReq{false};


Q_SIGNALS:
    void okDiag(const QString &, const QString &);
    void yesNoDiag(const QString &, const QString &, const QString &, const QString &);
    void yesNoDiagUpdate(const QString &,const QString &);
    void yesNoDiagClose();
    void httpSeeked(int);
    void flushPlayer();
    void finished();
    void dataAvailable();
    void resume();
    void seek(int,int,int);

public Q_SLOTS:
    void Start();
    void Close();
    bool Pause();
    bool Seek(int value, int totalTime, int currentTime);
    void Resume();
    qreal getTrickModeWritePercent();
    bool checkMP_MediaPvr(QByteArray bufferarray);
    int getSegCount();
    int fetchSegmentInfo(SegmentEntry * entry, int index);
    void segDataReady();
    void httpServerFlushed();
    void updateWritePercent();
    bool chechRawDataAvailable();
    int fillRawQueue();
    void mediaPlayerReadyNotify();
    void cleanupPvrFiles();
    int checkExpiredSegEntries(int readIndex);

private:
    bool pvrOnRawFiles(QByteArray bufferarray);
    bool writeToRawFile(QByteArray &bufferarray);
    void updatePreEnablement();
    double getPreEnablementValue();
    int evaluate_bitrate();
    int grow_segment_entries(SegmentEntry *segTempEntry);
    int grow_raw_entries(RawEntry *rawTempEntry);
    bool getRawInfoReader();
    void abortPvrRaw();
    void abortPvrSegment();
    int getSegmentBestEffort(int *seekedPos, int pvrPlayTime);
    qint64 getRawEntriesDataSize();
    qint64 getSegEntriesDataSize();
    int monitorPvrSpace();
    int checkExpiredRawEntries();
    qint64 getRemainsSegTimes(SegmentEntry * entry);

    /** Segmenter */
    AVStream *add_output_stream(AVFormatContext *output_format_context, AVStream *input_stream);
    AVStream * choose_output(AVFormatContext *ofmt_ctx);
    int process_input_packet(InputStream *ist, AVStream *ost, AVPacket *pkt);
    int do_streamcopy(InputStream *ist, AVStream *ost, AVPacket *pkt);
    void write_frame(AVFormatContext *s, AVPacket *pkt, AVStream *ost);

    QVector<SegmentEntry *> m_segmentsInfo;
    QVector<RawEntry *> m_rawsInfo;
    RawEntry * m_curRawInfoReader;
    RawEntry * m_curRawInfoWriter;
    QThread * m_thread;
    QMutex m_pvrMtx;
    QMutex m_mutexPending;
    QWaitCondition m_waitPendingCommand;
    QMutex m_segListLock;
    QMutex m_rawEntriesLock;
    AVFormatContext *ifmt_ctx = NULL;
    AVFormatContext *ofmt_ctx = NULL;
    qint64 last_mux_dts;
    QString m_pvrFolder;
    QString m_segFilePrefix;
    QString m_rawFilePrefix;
    QString m_captureMode;
    MP_MediaPvrModes m_pvrMode;
    QPointer<KvalMediaPlayerEngine> m_mediaPlayerRef;

    AVFormatContext * m_pFormatContext;
    atomic_bool m_rawInjectionActive;
    atomic_bool m_isTrickMode;

    atomic_bool m_pauseForFlushReq;
    atomic_llong m_seekVal;
    QFile * m_trickModeFile;
    QFile * m_preEnablementFile;
    atomic<qreal> m_write_percent;
    double m_pvrSessionConsumedData;
    bool m_isTrickOnRemovable;
    double m_pvrAvailableSpace;
    atomic_int m_SegCounts;
    atomic_int m_currentSegIdx;
    MP_MediaPvrProvider * m_mediaPvrProvider;
    bool m_abortSegTask = false;
    bool m_abortRawTask = false;
    atomic<DemuxStatus> m_muxerStatus{MEDIA_PVR_DEMUX_NONE};
    atomic_bool m_segInjectionSwitch;
    atomic_bool m_mediaPlayerReaderFlushed;
    atomic_int m_byteRateMs{0};

    /** Old implementation, deprecated */
    QString m_trickModeFilePath;

};
