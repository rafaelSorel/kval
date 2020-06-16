#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

#include "AMLReader.h"
#include "AlsaAudioSink.h"
#include "AmlVideoSink.h"
#include "Codec.h"
#include "AMLComponent.h"
#include "Pin.h"

class AlsaAudioSinkElement;
/**
 * @brief The AudioCodecElement class
 */
class AudioCodecElement : public AMLComponent
{
    Q_OBJECT

public:
    AudioCodecElement();
    virtual ~AudioCodecElement();

    bool Open(AudioPinInfo * audinfo,
              AmlVideoSinkClock* av_clock,
              AMLReader *aml_reader,
              std::string device);

    virtual void Close() override;
    void SetVolume(float fVolume) { }
    virtual void Initialize() override;
    virtual void State_Executing() override;
    virtual void DoWork() override;
    virtual void ChangeState(MediaState oldState, MediaState newState) override;
    virtual void Flush() override;
    bool isSeekRequest() { return m_aml_reader->isSeekRequest(); }
    void Resample(uint8_t** data_in,
                  int sample_in,
                  uint8_t * data_out,
                  int * data_out_len,
                  int * samples_out);
    AlsaAudioSinkElement * audioDecoder() { return m_audioSink; }

Q_SIGNALS:
    void bufferAvailable(BufferSPTR);
    void sendBackBuffer(BufferSPTR);

public Q_SLOTS:
    void onBufferAvailable(BufferSPTR);
    void setupNewCodec();

private:
    void BuildChannelMap();
    bool SetupCodec();
    void ProcessBuffer(AVPacketBufferSPTR buffer);
    void CloseCodec();

    AlsaAudioSinkElement * m_audioSink = nullptr;
    KvalThread *m_aml_audiosink_thread;
    AMLReader * m_aml_reader = nullptr;
    AmlVideoSinkClock * m_av_clock = nullptr;
    QMutex m_playPauseMutex;
    const int alsa_channels = 2;
    AudioPinInfo * inInfo =nullptr;
    AVCodec* soundCodec = nullptr;
    AVCodecContext* soundCodecContext = nullptr; //OK
    bool isFirstData = true;
    AudioFormatEnum audioFormat = AudioFormatEnum::Unknown;
    int streamChannels = 0;
    int outputChannels = 0;
    int sampleRate = 0;
    AVFrameBufferSPTR m_av_frame; //OK

};

typedef std::shared_ptr<AudioCodecElement> AudioCodecElementSPTR;
