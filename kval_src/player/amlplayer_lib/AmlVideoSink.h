#pragma once
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <QObject>
#include <QTimer>

#include "KvalThreadUtils.h"
#include "Codec.h"
#include "AMLComponent.h"
#include "Pin.h"
#include "Timer.h"
#include "AmlCodec.h"
#include "AMLReader.h"
#include "utils/BitstreamConverter.h"

typedef struct am_packet {
    AVPacket      avpkt;
    unsigned char *data;
    int           data_size;
} am_packet_t;


class AmlVideoSinkElement;

class AmlVideoSinkClock : public AMLComponent
{
    Q_OBJECT

public:
    AmlVideoSinkClock(AmlVideoSinkElement* owner,
                      PinInfoSPTR info,
                      AmlCodec* codecPTR,
                      bool autoFlushDisabled);
    virtual ~AmlVideoSinkClock();

    virtual void Close() override;
    double Clock();
    double FrameRate() const;
    void SetFrameRate(double value);

Q_SIGNALS:
    void finished();

public Q_SLOTS:
    void onClockBufferAvailable(BufferSPTR);

private:
    void ProcessClockBuffer(BufferSPTR buffer);

private:
    AmlVideoSinkElement * m_owner;
    const uint64_t PTS_FREQ = 90000;
    AmlCodec* codecPTR;
    double frameRate = 0;
    bool m_autoFlushDisabled;
    double m_clock = 0.0;
};
typedef std::shared_ptr<AmlVideoSinkClock> AmlVideoSinkClockSPTR;

/**
 * @brief The AmlVideoSinkElement class
 */
class AmlVideoSinkElement : public AMLComponent
{
    Q_OBJECT

public:
    AmlVideoSinkElement();
    virtual ~AmlVideoSinkElement();

    bool Open(VideoPinInfo*, AmlVideoSinkClock*&, KvalThread*& , AMLReader*, bool);
    virtual void Close() override;

    double Clock();
    AmlVideoSinkClock * V_Clock(){ return m_av_clock; }
    virtual void Flush() override;
    bool set_display_axis(bool recovery);
    void resetPlayer();
    bool setAspectRatio(AspectRatio value);

Q_SIGNALS:
    void sendBackBuffer(BufferSPTR);
    void stopTimer();
    void reset_player();

public Q_SLOTS:
    void onBufferAvailable(BufferSPTR);
    void Qtimer_Expired();

protected:
    virtual void Initialize() override;
    virtual void DoWork() override;
    virtual void State_Executing() override;
    virtual void ChangeState(MediaState oldState, MediaState newState) override;
    virtual void Terminating() override;

private:
    CBitstreamParser *m_bitparser;
    CBitstreamConverter *m_bitstream;

    //omx_bitstream_ctx m_sps_pps_context;
    uint32_t          m_sps_pps_size;
    QTimer  * endOfStreamTimer;

    QMutex m_exec_change;
    AMLReader * m_aml_reader = nullptr;
    AmlVideoSinkClock * m_av_clock = nullptr;
    KvalThread* m_av_clock_thread = nullptr;

    const uint64_t PTS_FREQ = 90000;
    std::vector<unsigned char> videoExtraData;

    double lastTimeStamp = -1;
    bool isFirstVideoPacket = true;
    bool isAnnexB = false;
    bool isExtraDataSent = false;
    int64_t estimatedNextPts = 0;

    VideoFormatEnum videoFormat = VideoFormatEnum::Unknown;
    VideoPinInfo * videoinfo = nullptr;
    PinInfoSPTR info;

    bool isFirstData = true;
    std::vector<unsigned char> extraData;

    uint64_t clockPts = 0;
    uint64_t lastClockPts = 0;
    double eosPts = -1;

    double clock = 0;
    bool isShortStartCode = false;
    bool isEndOfStream = false;

    bool m_stop_timer;
    Mutex timerMutex;
    QMutex playPauseMutex;

    AmlCodec amlCodec;

    void SetupHardware();
    void ProcessBuffer(AVPacketBufferSPTR buffer);
    bool SendCodecData(unsigned long pts, unsigned char* data, int length);
    bool SendCodecAmlData(unsigned long pts, unsigned char* data, int length);

    //static void WriteToFile(const char* path, const char* value);
    void Divx3Header(int width, int height, int packetSize);
    int vp9_update_frame_header(am_packet_t *pkt);
    void ConvertH264ExtraDataToAnnexB();
    bool BitstreamConvertInitAVC(void *in_extradata, int in_extrasize);
    void HevcExtraDataToAnnexB();
    bool NaluFormatStartCodes(VideoFormatEnum codec,
                              std::vector<unsigned char> in_extradata);

};

typedef std::shared_ptr<AmlVideoSinkElement> AmlVideoSinkElementSPTR;
