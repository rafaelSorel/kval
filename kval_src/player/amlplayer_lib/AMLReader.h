#pragma once

#include "AMLStreamInfo.h"
#include "Codec.h"
#include "AMLComponent.h"
#include "Pin.h"
#include "EventListener.h"

#include <string>
#include <map>
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QPointer>


extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
}

#include <memory>
class AMLReader;
extern QPointer<AMLReader> g_amlReaderRef;

class BitstreamStats
{

public:
  // in order not to cause a performance hit, we should only check the clock when
  // we reach m_nEstimatedBitrate bits.
  // if this value is 1, we will calculate bitrate on every sample.
  BitstreamStats(unsigned int nEstimatedBitrate=(10240*8) /*10Kbit*/);
  virtual ~BitstreamStats();

  void AddSampleBytes(unsigned int nBytes);
  void AddSampleBits(unsigned int nBits);

  inline double GetBitrate()    const { return m_dBitrate; }
  inline double GetMaxBitrate() const { return m_dMaxBitrate; }
  inline double GetMinBitrate() const { return m_dMinBitrate; }

  void Start();
  void CalculateBitrate();

private:
  double m_dBitrate;
  double m_dMaxBitrate;
  double m_dMinBitrate;
  unsigned int m_nBitCount;
  unsigned int m_nEstimatedBitrate; // when we reach this amount of bits we check current bitrate.
  int64_t m_tmStart;
  static int64_t m_tmFreq;
};

class AVException : public Exception
{
    static std::string GetErrorString(int error)
    {
        char buffer[AV_ERROR_MAX_STRING_SIZE + 1];
        char* message = av_make_error_string(buffer,
                                             AV_ERROR_MAX_STRING_SIZE, error);
        // Return a copy that survives after buffer goes out of scope
        return std::string(message);
    }

public:
    AVException(int error)
        : Exception(GetErrorString(error).c_str())
    {}
};



struct Chapter
{
    std::string Title;
    double TimeStamp;
};

typedef std::vector<Chapter> ChapterList;
typedef std::shared_ptr<ChapterList> ChapterListSPTR;
typedef void (*CallBackFlushed)(void);

#define NewSPTR std::make_shared
#define NewUPTR std::make_unique

/**
 * @brief The AMLReader class
 */
class AMLReader : public AMLComponent
{
    Q_OBJECT

Q_SIGNALS:
    void bufferAvailable(BufferSPTR);
    void bufferAvailable_V(BufferSPTR);
    void bufferAvailable_A(BufferSPTR);
    void bufferAvailable_S(BufferSPTR);
    void audioStreamCodecChanged();
    void reset_player();

public Q_SLOTS:
    void onBufferReturned(BufferSPTR);
    int64_t getCurrentPts();
    /**This cb will be used to notify pvr that reader has been reseted and it can starts send data*/
    void setPvrCb(void *cb);
    /**This cb will increase av_frame_read timeout, the connection with server side may broke
       and as the user can rewind to play smoothly, do not stop the player in the meantime */
    void activatePvrTimeout();
    bool Reset();

public:
    AMLReader();
    virtual ~AMLReader();
    virtual void Initialize() override;
    virtual void State_Executing() override;
    virtual void ChangeState(MediaState oldState, MediaState newState) override;
    virtual void Terminating() override;
    virtual void Close() override;

    static QPointer<AMLReader> getGlobalRef() { return g_amlReaderRef; }
    const ChapterListSPTR Chapters() const;

    bool Open(std::string url, std::string avOptions);
    bool SetActiveStream(MediaCategoryEnum, unsigned int);
    void stopExec();
    

    int GetVideoBitrate();
    void FlushInternal();
    void Seek(double timeStamp);
    bool CanSeek();
    void activateLoop() { m_loopMotionGraphics = true; }

    void signalDisconnected(MediaCategoryEnum);
    int  AudioStreamCount() { return m_audio_count; }
    int  VideoStreamCount() { return m_video_count; }
    int  SubtitleStreamCount() { return m_subtitle_count; }
    int  GetStreamLength();
    bool IsActive(MediaCategoryEnum type, int stream_index);

    PinInfo * GetHints(MediaCategoryEnum type);
    CAMLStreamInfo * GetHintsIndex(MediaCategoryEnum type, unsigned int index);

    //Seek request handling
    void seekRequest() { m_seekRequested = true; }
    void seekRequestEnd() { m_seekRequested = false; }
    bool isSeekRequest() { return m_seekRequested; }

protected:
    int                                 m_video_index;
    int                                 m_audio_index;
    int                                 m_subtitle_index;
    int                                 m_video_count;
    int                                 m_audio_count;
    int                                 m_subtitle_count;
    int                                 m_unknow_media_count;
    bool                                m_bMatroska;
    bool                                m_bAVI;
    bool                                m_audio_codec_changed;
    bool                                m_subs_codec_changed;

private:
    BitstreamStats m_videoStats;
    const int                           BUFFER_COUNT = 64; //enough for 4k video
    std::atomic_bool                    m_video_discon= {false};
    std::atomic_bool                    m_audio_discon= {false};
    std::atomic_bool                    m_subs_discon= {false};
    std::atomic<int64_t>                m_player_pts{-1};
    std::atomic<int64_t>                m_flush_pts{-1};
    std::atomic_bool                    m_isPvrTimeout{false};
    CallBackFlushed                     cbFlushed;

    bool                                m_stop_request = false;

    std::string                         m_url;
    AVFormatContext                    *m_pFormatContext;
    AVIOContext                        *m_ioContext;
    ThreadSafeQueue<BufferSPTR>         availableBuffers;
    std::vector<PinInfoSPTR>            streamList;
    std::vector<uint64_t>               streamNextPts;
    ChapterListSPTR                     chapters = NewSPTR<ChapterList>();
    uint64_t                            lastPts = 0;
    double                              duration = -1;


    QMutex                              m_activeStreamMtx;
    QMutex m_mutexPending;
    QWaitCondition m_waitPendingCommand;

private:
    void PrintDictionary(const AVDictionary*) const;
    void SetupPins();
    bool SetActiveStreamInternal(MediaCategoryEnum, unsigned int);
    bool setInternalHints(AVStream*, CAMLStreamInfo*);
    std::string GetStreamCodecName(AVStream *stream);
    std::string GetStreamLanguage(MediaCategoryEnum type, int stream_index);
    bool m_loopMotionGraphics;
};

typedef std::shared_ptr<AMLReader> AMLReaderSPTR;
