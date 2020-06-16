#pragma once

#include "Codec.h"
#include "LockedQueue.h"

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>


#include <vector>

#include "AmlVideoSink.h"
#include "AudioCodec.h"
#include "Codec.h"
#include "AMLComponent.h"
#include "Pin.h"
#include "IClock.h"

class AudioCodecElement;

/**
 * @brief The AlsaAudioSinkElement class
 */
class AlsaAudioSinkElement : public AMLComponent
{
    Q_OBJECT

public:

    AlsaAudioSinkElement();
    virtual ~AlsaAudioSinkElement();

    bool Open(AudioPinInfo * aud_info,
              AudioCodecElement* audio_codec,
              AmlVideoSinkClock* av_clock,
              std::string device);
    virtual void Close() override;

    double AudioAdjustSeconds() const;
    void SetAudioAdjustSeconds(double value);

    double Clock() const;
    virtual void Flush() override;
    bool setupNewDecoder(AudioPinInfo * aud_info);
    bool GetMuted();
    int GetVolume();
    void SetAlsaVolume(long);
    void SetAlsaSwitchMute(bool);
    static bool AlsaConfigSoundSystem();

Q_SIGNALS:
    void clockBufferAvailable(BufferSPTR);

public Q_SLOTS:
    void onBufferAvailable(BufferSPTR);

protected:
    virtual void Initialize() override;
    virtual void DoWork() override;
    virtual void State_Executing() override;
    virtual void Terminating() override;
    virtual void ChangeState(MediaState oldState, MediaState newState) override;

private:
    AudioCodecElement * m_audio_codec;
    AmlVideoSinkClock * m_av_clock;
    AudioPinInfo * audioinfo;

    const int AUDIO_FRAME_BUFFERCOUNT = 8;

    //plughw /* playback device (HDMI, SPDIF, AV OUTPUT)*/
    std::string m_device = "default";
    std::string m_sid = "PCM";


    AVCodecID codec_id = AV_CODEC_ID_NONE;
    const int alsa_channels = 2;
    unsigned int sampleRate = 0;
    snd_pcm_t* handle = nullptr;
    snd_pcm_sframes_t frames;


    double lastTimeStamp = -1;
    bool canPause = true;
    bool isFirstData = true;
    AudioFormatEnum audioFormat = AudioFormatEnum::Unknown;
    int streamChannels = 0;
    bool isFirstBuffer = true;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t buffer_size;

    double audioAdjustSeconds = 0.0;
    double clock = 0.0;
    int FRAME_SIZE = 0;
    bool doResumeFlag = false;
    bool doPauseFlag = false;
    bool doStopFlag = false;
    QMutex playPauseMutex;
    bool m_muted;
    int m_volume;

    bool SetupAlsa(int frameSize);
    void ProcessBuffer(PcmDataBufferSPTR pcmBuffer);
    void extractAlsaVolumeConfig();

};

typedef std::shared_ptr<AlsaAudioSinkElement> AlsaAudioSinkElementSPTR;
