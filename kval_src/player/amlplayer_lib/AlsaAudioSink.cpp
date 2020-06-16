#include "AlsaAudioSink.h"

#define LOG_ACTIVATED
#define LOG_SRC ALSAAUDIOSINK
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

/**
 * @brief AlsaAudioSinkElement::AlsaAudioSinkElement
 */
AlsaAudioSinkElement::AlsaAudioSinkElement()
{
    INV();
    m_volume = 65;
    m_muted = false;
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::~AlsaAudioSinkElement
 */
AlsaAudioSinkElement::~AlsaAudioSinkElement()
{
    INV();
    LOG_INFO(LOG_TAG, "Cleanup...");
    if (handle)
    {
        LOG_INFO(LOG_TAG, "snd_pcm_close");
        snd_pcm_drop(handle);
        snd_pcm_close(handle);
        snd_config_update_free_global();
        handle = nullptr;
    }
    ClearInfosPins();
    LOG_INFO(LOG_TAG,
             "+++++++++++++++++ ~AlsaAudioSinkElement +++++++++++++++++");
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::AlsaConfigSoundSystem
 * @return
 */
bool AlsaAudioSinkElement::AlsaConfigSoundSystem()
{
    INV();
    LOG_INFO(LOG_TAG, "AlsaConfigSoundSystem ...");

    //default config
    snd_pcm_t* handle = nullptr;
    const int alsa_channels = 2;
    const int frameSize = 1024;
    unsigned int sampleRate = 48000;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t buffer_size;
    const int AUDIO_FRAME_BUFFERCOUNT = 8;
    std::string m_device = "default";
    std::string m_sid = "PCM";
    //---

    const int MIN_FRAME_SIZE = 512;
    if (sampleRate == 0)
    {
        LOG_ERROR(LOG_TAG, "sampleRate: 0");
        OUTV("false");
        return false;
    }

    int err;
    //SND_PCM_NONBLOCK
    if ((err = snd_pcm_open(&handle,
                            m_device.c_str(),
                            SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        LOG_ERROR(LOG_TAG, "snd_pcm_open error: %s", snd_strerror(err));
        OUTV("false");
        return false;
    }

    LOG_INFO(LOG_TAG, "frameSize: %d", frameSize);

    int FRAME_SIZE = frameSize;
    if (FRAME_SIZE < MIN_FRAME_SIZE)
    {
        FRAME_SIZE = MIN_FRAME_SIZE;
        LOG_INFO(LOG_TAG,
                 "FrameSize too small. Using default value (%d)",
                 FRAME_SIZE);
    }
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    period_size = FRAME_SIZE * alsa_channels * sizeof(short);
    buffer_size = AUDIO_FRAME_BUFFERCOUNT * period_size;

    (snd_pcm_hw_params_malloc(&hw_params));
    (snd_pcm_hw_params_any(handle, hw_params));
    (snd_pcm_hw_params_set_access(handle,
                                  hw_params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED));
    (snd_pcm_hw_params_set_format(handle,
                                  hw_params,
                                  SND_PCM_FORMAT_S16_LE));
    (snd_pcm_hw_params_set_rate_near(handle,
                                     hw_params,
                                     &sampleRate,
                                     NULL));
    (snd_pcm_hw_params_set_channels(handle,
                                    hw_params,
                                    alsa_channels));
    (snd_pcm_hw_params_set_buffer_size_near(handle,
                                            hw_params,
                                            &buffer_size));
    (snd_pcm_hw_params_set_period_size_near(handle,
                                            hw_params,
                                            &period_size,
                                            NULL));
    (snd_pcm_hw_params(handle, hw_params));
    snd_pcm_hw_params_free(hw_params);

    snd_pcm_sw_params_malloc(&sw_params);
    snd_pcm_sw_params_current(handle, sw_params);
    snd_pcm_sw_params_set_start_threshold(handle,
                                          sw_params, buffer_size - period_size);
    snd_pcm_sw_params_set_avail_min(handle, sw_params, period_size);
    snd_pcm_sw_params(handle, sw_params);
    snd_pcm_sw_params_free(sw_params);

    snd_pcm_prepare(handle);
    snd_pcm_drop(handle);
    snd_pcm_close(handle);
    snd_config_update_free_global();

    //Reset Volume
    snd_mixer_t *m_handle;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t* elem;
    long min, max;
    long volume_set_value;

    if(snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_open");
        goto return_cleanup;
    }
    if (snd_mixer_attach(m_handle, m_device.c_str()) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_attach");
        goto return_cleanup;
    }

    if (snd_mixer_selem_register(m_handle, NULL, NULL) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_selem_register");
        goto return_cleanup;
    }

    if (snd_mixer_load(m_handle) < 0)
    {
        LOG_ERROR(LOG_TAG,"Error snd_mixer_load");
        goto return_cleanup;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, m_sid.c_str());
    elem = snd_mixer_find_selem(m_handle, sid);
    if (elem == NULL)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_find_selem");
        goto return_cleanup;
    }

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    volume_set_value = (65 * max) / 100 ;

    LOG_DEBUG(LOG_TAG, "volume_set_value: %lld", volume_set_value);
    LOG_DEBUG(LOG_TAG, "min: %lld", min);
    LOG_DEBUG(LOG_TAG, "max: %lld", max);

    snd_mixer_selem_set_playback_volume_all(elem, volume_set_value);

return_cleanup:
    snd_mixer_close(m_handle);

    OUTV("true");
    return true;
}

/**
 * @brief AlsaAudioSinkElement::extractAlsaVolumeConfig
 */
void AlsaAudioSinkElement::extractAlsaVolumeConfig()
{
    INV();

    SetAlsaSwitchMute(false);

    snd_mixer_t *m_handle;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t* elem;
    long min, max;
    long volume = 0;

    if(snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_open");
        goto return_cleanup;
    }
    if (snd_mixer_attach(m_handle, m_device.c_str()) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_attach");
        goto return_cleanup;
    }

    if (snd_mixer_selem_register(m_handle, NULL, NULL) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_selem_register");
        goto return_cleanup;
    }

    if (snd_mixer_load(m_handle) < 0)
    {
        LOG_ERROR(LOG_TAG,"Error snd_mixer_load");
        goto return_cleanup;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, m_sid.c_str());
    elem = snd_mixer_find_selem(m_handle, sid);
    if (elem == NULL)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_find_selem");
        goto return_cleanup;
    }

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    LOG_INFO(LOG_TAG, "min: %lld, max: %lld", min, max);

    snd_mixer_selem_get_playback_volume(elem,
                                        SND_MIXER_SCHN_FRONT_LEFT,
                                        &volume);
    m_volume = (int)( (volume * 100 ) / max );

    LOG_INFO(LOG_TAG, "m_volume: %lld", m_volume);

return_cleanup:
    snd_mixer_close(m_handle);
    snd_config_update_free_global();
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::SetAlsaSwitchMute
 * @param card
 * @param selem_name
 */
void AlsaAudioSinkElement::SetAlsaSwitchMute(bool muted)
{
    INV("muted: %u", muted);
    const char *sys_mute = "/sys/class/amaudio/mute_unmute";
    int fd_sys_mute = open(sys_mute, O_CREAT | O_RDWR | O_TRUNC, 0664);
    if (fd_sys_mute >= 0)
    {
        write(fd_sys_mute, muted ? "1" : "0" , 0x1);
        close(fd_sys_mute);
        m_muted =muted;
    }

    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::SetAlsaVolume
 * @param card
 * @param selem_name
 * @param volume
 */
void AlsaAudioSinkElement::SetAlsaVolume(long volume)
{
    INV("volume: %lld", volume);
    snd_mixer_t *m_handle;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t* elem;
    long min, max;
    long new_volume;
    long volume_set_value;

    if(snd_mixer_open(&m_handle, SND_MIXER_ELEM_SIMPLE) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_open");
        goto return_cleanup;
    }
    if (snd_mixer_attach(m_handle, m_device.c_str()) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_attach");
        goto return_cleanup;
    }

    if (snd_mixer_selem_register(m_handle, NULL, NULL) < 0)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_selem_register");
        goto return_cleanup;
    }

    if (snd_mixer_load(m_handle) < 0)
    {
        LOG_ERROR(LOG_TAG,"Error snd_mixer_load");
        goto return_cleanup;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, m_sid.c_str());
    elem = snd_mixer_find_selem(m_handle, sid);
    if (elem == NULL)
    {
        LOG_ERROR(LOG_TAG, "Error snd_mixer_find_selem");
        goto return_cleanup;
    }

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    new_volume = m_volume + volume;
    if(new_volume > 100)
    {
        m_volume = 100;
        goto return_cleanup;
    }
    else if(new_volume < 0)
    {
        m_volume = 0;
        goto return_cleanup;
    }
    volume_set_value = (new_volume * max) / 100 ;
    LOG_DEBUG(LOG_TAG, "volume_set_value: %lld", volume_set_value);
    LOG_DEBUG(LOG_TAG, "min: %lld", min);
    LOG_DEBUG(LOG_TAG, "max: %lld", max);
    snd_mixer_selem_set_playback_volume_all(elem, volume_set_value);

    m_volume = new_volume;
    LOG_INFO(LOG_TAG, "New Volume: %d", m_volume);

return_cleanup:
    snd_mixer_close(m_handle);
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::GetMuted
 * @return
 */
bool AlsaAudioSinkElement::GetMuted()
{
    return m_muted;
}

/**
 * @brief AlsaAudioSinkElement::GetVolume
 * @return
 */
int AlsaAudioSinkElement::GetVolume()
{
    return m_volume;
}

/**
 * @brief AlsaAudioSinkElement::setupNewDecoder
 * @param aud_info
 * @return
 */
bool AlsaAudioSinkElement::setupNewDecoder(AudioPinInfo * aud_info)
{
    INV("aud_info: %p", aud_info);

    if (aud_info->Category() != MediaCategoryEnum::Audio)
    {
        LOG_ERROR(LOG_TAG, "Not connected to an audio pin.");
        OUTV("false");
        return false;
    }

    if (handle)
    {
        snd_pcm_drop(handle);
        snd_pcm_close(handle);
        snd_config_update_free_global();
        handle = nullptr;
    }

    audioinfo       = aud_info;
    audioFormat = audioinfo->Format;
    sampleRate = audioinfo->SampleRate;
    streamChannels = audioinfo->Channels;
    isFirstData = false;
    isFirstBuffer = true;

    return true;
}

/**
 * @brief AlsaAudioSinkElement::Close
 */
void AlsaAudioSinkElement::Close()
{
    INV();

    Terminate();
    WaitForExecutionState(ExecutionStateEnum::Terminated);

//    Q_EMIT finished();
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::Open
 * @param aud_info
 * @param audio_codec
 * @param av_clock
 * @param device
 * @return
 */
bool AlsaAudioSinkElement::Open(AudioPinInfo * aud_info,
                                AudioCodecElement* audio_codec,
                                AmlVideoSinkClock* av_clock,
                                std::string audio_device)
{
    INV("audinfo: %p, av_clock: %p, audio_codec: %p, audio_device: %s",
        aud_info, av_clock, audio_codec, audio_device.c_str());

    if (aud_info->Category() != MediaCategoryEnum::Audio)
    {
        LOG_ERROR(LOG_TAG, "Not connected to an audio pin.");
        OUTV("false");
        return false;
    }

    audioinfo       = aud_info;
    m_audio_codec   = audio_codec;
    m_device        = audio_device;
    if(av_clock)
    {
        m_av_clock = av_clock;
        connect(this,
                SIGNAL(clockBufferAvailable(BufferSPTR)),
                m_av_clock,
                SLOT(onClockBufferAvailable(BufferSPTR)));
    }


    connect(m_audio_codec,
            SIGNAL(bufferAvailable(BufferSPTR)),
            this,
            SLOT(onBufferAvailable(BufferSPTR)));

    audioFormat = audioinfo->Format;
    sampleRate = audioinfo->SampleRate;
    streamChannels = audioinfo->Channels;
    isFirstData = false;
    extractAlsaVolumeConfig();

    SetName("AudioSink");
    Execute();
    WaitForExecutionState(ExecutionStateEnum::Idle);

    OUTV("true");
    return true;
}

/**
 * @brief AlsaAudioSinkElement::SetupAlsa
 * @param frameSize
 * @return
 */
bool AlsaAudioSinkElement::SetupAlsa(int frameSize)
{
    INV("frameSize: %d", frameSize);
    const int MIN_FRAME_SIZE = 512;
    if (sampleRate == 0)
    {
        LOG_ERROR(LOG_TAG, "sampleRate: 0");
        OUTV("false");
        return false;
    }

    int err;
    //SND_PCM_NONBLOCK
    if ((err = snd_pcm_open(&handle,
                            m_device.c_str(),
                            SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        LOG_ERROR(LOG_TAG, "snd_pcm_open error: %s", snd_strerror(err));
        OUTV("false");
        return false;
    }

    LOG_INFO(LOG_TAG, "frameSize: %d", frameSize);

    FRAME_SIZE = frameSize;
    if (FRAME_SIZE < MIN_FRAME_SIZE)
    {
        FRAME_SIZE = MIN_FRAME_SIZE;
        LOG_INFO(LOG_TAG,
                 "FrameSize too small. Using default value (%d)",
                 FRAME_SIZE);
    }
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    period_size = FRAME_SIZE * alsa_channels * sizeof(short);
    buffer_size = AUDIO_FRAME_BUFFERCOUNT * period_size;

    (snd_pcm_hw_params_malloc(&hw_params));
    (snd_pcm_hw_params_any(handle, hw_params));
    (snd_pcm_hw_params_set_access(handle,
                                  hw_params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED));
    (snd_pcm_hw_params_set_format(handle,
                                  hw_params,
                                  SND_PCM_FORMAT_S16_LE));
    (snd_pcm_hw_params_set_rate_near(handle,
                                     hw_params,
                                     &sampleRate,
                                     NULL));
    (snd_pcm_hw_params_set_channels(handle,
                                    hw_params,
                                    alsa_channels));
    (snd_pcm_hw_params_set_buffer_size_near(handle,
                                            hw_params,
                                            &buffer_size));
    (snd_pcm_hw_params_set_period_size_near(handle,
                                            hw_params,
                                            &period_size,
                                            NULL));
    (snd_pcm_hw_params(handle, hw_params));
    snd_pcm_hw_params_free(hw_params);

    snd_pcm_sw_params_malloc(&sw_params);
    snd_pcm_sw_params_current(handle, sw_params);
    snd_pcm_sw_params_set_start_threshold(handle,
                                          sw_params, buffer_size - period_size);
    snd_pcm_sw_params_set_avail_min(handle, sw_params, period_size);
    snd_pcm_sw_params(handle, sw_params);
    snd_pcm_sw_params_free(sw_params);

    snd_pcm_prepare(handle);
    snd_config_update_free_global();
    OUTV("true");
    return true;
}

/**
 * @brief AlsaAudioSinkElement::ProcessBuffer
 * @param pcmBuffer
 */
void AlsaAudioSinkElement::ProcessBuffer(PcmDataBufferSPTR pcmBuffer)
{
    INV("pcmBuffer: %p", pcmBuffer.get());

    QMutexLocker locker(&playPauseMutex);
    if(doStopFlag)
    {
        LOG_INFO(LOG_TAG, "doStopFlag invoked");
        return;
    }
    if (doResumeFlag)
    {
        //LOG_INFO(LOG_TAG, "snd_pcm_pause: handle=%p, enable=0", handle);
        snd_pcm_pause(handle, 0);
        //LOG_INFO(LOG_TAG, "snd_pcm_pause: returned.");
        doResumeFlag = false;
    }

    PcmData* pcmData = pcmBuffer->GetPcmData();
    if (isFirstBuffer)
    {
        if(!SetupAlsa(pcmData->Samples))
        {
            LOG_ERROR(LOG_TAG, "Could not setup sound dec, will play without sound !!");
            return;
        }
        isFirstBuffer = false;
    }

    short data[alsa_channels * pcmData->Samples];
    if (pcmData->Format == PcmFormat::Int16)
    {
        if (alsa_channels != 2)
        {
            LOG_ERROR(LOG_TAG, "alsa_channels != 2");
            OUTV();
            return;
        }
        if (pcmData->Channels < 2)
        {
            LOG_ERROR(LOG_TAG, "pcmData->Channels < 2");
            OUTV();
            return;
        }

        int stride = pcmData->Channels * sizeof(short);
        unsigned char* source = (unsigned char*)pcmData->Channel[0];
        int index = 0;
        for (int i = 0; i < pcmData->Samples; ++i)
        {
            short* samples = (short*)(source + i * stride);

            for (int j = 0; j < alsa_channels; ++j)
            {
                if (j < pcmData->Channels)
                {
                    data[index++] = *(samples++);
                }
                else
                {
                    data[index++] = 0;
                }
            }
        }
    }
    else if (pcmData->Format == PcmFormat::Int16Planes)
    {
        if (alsa_channels != 2)
        {
            LOG_ERROR(LOG_TAG, "alsa_channels != 2");
            OUTV();
            return;
        }
        short* channels[alsa_channels] = { 0 };
        int channelCount = pcmData->Channels;
        switch (channelCount)
        {
            case 0:
            {
                LOG_ERROR(LOG_TAG, "Unexpected zero channel count");
                OUTV();
                return;
            }

            case 1:
                // Output the mono channel over both stereo channels
                channels[0] = (short*)pcmData->Channel[0];
                channels[1] = (short*)pcmData->Channel[0];
                break;

            case 2:
                channels[0] = (short*)pcmData->Channel[0];
                channels[1] = (short*)pcmData->Channel[1];
                break;

            default:
                channels[0] = (short*)pcmData->Channel[0];
                channels[1] = (short*)pcmData->Channel[2];
                break;
        }

        int index = 0;
        for (int i = 0; i < pcmData->Samples; ++i)
        {
            for (int j = 0; j < alsa_channels; ++j)
            {
                short* samples = channels[j];
                data[index++] = samples[i];
            }
        }
    }
    else if (pcmData->Format == PcmFormat::Float32Planes)
    {
        if (alsa_channels != 2)
        {
            LOG_ERROR(LOG_TAG, "alsa_channels != 2");
            OUTV();
            return;
        }
        if (pcmData->Channels < 1)
        {
            LOG_ERROR(LOG_TAG, "pcmData->Channels < 1");
            OUTV();
            return;
        }

        int channelCount = pcmData->Channels;
        if (!channelCount)
        {
            LOG_ERROR(LOG_TAG, "Unexpected zero channel count");
            OUTV();
            return;
        }

        int index = 0;
        for (int i = 0; i < pcmData->Samples; ++i)
        {
            float left;
            float right;
            switch(pcmData->Channels)
            {
                case 1:
                {
                    float* leftSamples = (float*)pcmData->Channel[0];
                    float* rightSamples = (float*)pcmData->Channel[0];

                    left = leftSamples[i];
                    right = rightSamples[i];
                    break;
                }
                case 2:
                {
                    float* leftSamples = (float*)pcmData->Channel[0];
                    float* rightSamples = (float*)pcmData->Channel[1];

                    left = leftSamples[i];
                    right = rightSamples[i];
                    break;
                }
                case 3:
                {
                    float* leftSamples = (float*)pcmData->Channel[0];
                    float* rightSamples = (float*)pcmData->Channel[1];
                    float* centerSamples = (float*)pcmData->Channel[2];

                    const float CENTER_WEIGHT = 0.1666666666666667f;

                    left = (leftSamples[i] * (1.0f - CENTER_WEIGHT)) +
                            (centerSamples[i] * CENTER_WEIGHT);
                    right = (rightSamples[i] * (1.0f - CENTER_WEIGHT)) +
                            (centerSamples[i] * CENTER_WEIGHT);
                    break;
                }
                case 6:
                {
                    float* leftSamples = (float*)pcmData->Channel[0];
                    float* rightSamples = (float*)pcmData->Channel[1];
                    float* centerSamples = (float*)pcmData->Channel[2];
                    float* subWooferSamples = (float*)pcmData->Channel[3];
                    float* sideLeftSamples = (float*)pcmData->Channel[4];
                    float* sideRightSamples = (float*)pcmData->Channel[5];

                    const float CENTER_WEIGHT = 0.1666666666666667f;
                    const float SUBWOOFER_WEIGHT = 0.1666666666666667f;
                    const float SIDELEFT_WEIGHT =  0.0833333333333f;
                    const float SIDERIGHT_WEIGHT = 0.0833333333333f;

                    left = (leftSamples[i] *
                            (1.0f - CENTER_WEIGHT - SUBWOOFER_WEIGHT - SIDELEFT_WEIGHT)) +
                            (centerSamples[i] * CENTER_WEIGHT) +
                            (subWooferSamples[i] * SUBWOOFER_WEIGHT) +
                            (sideLeftSamples[i] * SIDELEFT_WEIGHT);

                    right = (rightSamples[i] *
                             (1.0f - CENTER_WEIGHT - SUBWOOFER_WEIGHT - SIDERIGHT_WEIGHT)) +
                            (centerSamples[i] * CENTER_WEIGHT) +
                            (subWooferSamples[i] * SUBWOOFER_WEIGHT) +
                            (sideRightSamples[i] * SIDERIGHT_WEIGHT);;
                    break;
                }
                default:
                {
                    float* leftSamples = (float*)pcmData->Channel[0];
                    float* rightSamples = (float*)pcmData->Channel[1];

                    left = leftSamples[i];
                    right = rightSamples[i];
                    break;
                }
            }

            if (left > 1.0f)
                left = 1.0f;
            else if (left < -1.0f)
                left = -1.0f;

            if (right > 1.0f)
                right = 1.0f;
            else if (right < -1.0f)
                right = -1.0f;


            data[index++] = (short)(left * 0x7fff);
            data[index++] = (short)(right * 0x7fff);
        }
    }
    else if (pcmData->Format == PcmFormat::Int32)
    {
        if (alsa_channels != 2)
        {
            LOG_ERROR(LOG_TAG, "alsa_channels != 2");
            OUTV();
            return;
        }
        if (pcmData->Channels < 2)
        {
            LOG_ERROR(LOG_TAG, "pcmData->Channels < 2");
            OUTV();
            return;
        }
        // Signed 32 bit, interleaved
        int srcIndex = 0;
        int dstIndex = 0;

        int* source = (int*)pcmData->Channel[0];
        //short* dest = data;

        for (int i = 0; i < pcmData->Samples; ++i)
        {
            int left = source[srcIndex++];
            int right = source[srcIndex++];

            srcIndex += (pcmData->Channels - 2);

            data[dstIndex++] = (short)(left >> 16);
            data[dstIndex++] = (short)(right >> 16);
        }
    }
    else if (pcmData->Format == PcmFormat::Int32Planes)
    {
        if (alsa_channels != 2)
        {
            LOG_ERROR(LOG_TAG, "alsa_channels != 2");
            OUTV();
            return;
        }
        int* channels[alsa_channels] = { 0 };
        int channelCount = pcmData->Channels;
        switch (channelCount)
        {
            case 0:
            {
                LOG_ERROR(LOG_TAG, "Unexpected zero channel count");
                OUTV();
                return;
            }

            case 1:
                // Output the mono channel over both stereo channels
                channels[0] = (int*)pcmData->Channel[0];
                channels[1] = (int*)pcmData->Channel[0];
                break;

            case 2:
                channels[0] = (int*)pcmData->Channel[0];
                channels[1] = (int*)pcmData->Channel[1];
                break;

            default:
                channels[0] = (int*)pcmData->Channel[0];
                channels[1] = (int*)pcmData->Channel[2];
                break;
        }

        int index = 0;
        for (int i = 0; i < pcmData->Samples; ++i)
        {
            for (int j = 0; j < alsa_channels; ++j)
            {
                int* samples = channels[j];
                data[index++] = (short)(samples[i] >> 16);
            }
        }
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Not Supported !");
        OUTV();
        return;
    }

#if 0
    uint8_t * data_out;
    int data_out_len = 0;
    int samples_out = 0;
    m_audio_codec->Resample((uint8_t **)pcmData->Channel,
                            pcmData->Samples,
                            data_out,
                            &data_out_len,
                            &samples_out);
#endif

    // Update the reference clock
    /*
    From ALSA docs:
    For playback the delay is defined as the time that a frame that
    is written to the PCM stream shortly after this call will take
    to be actually audible. It is as such the overall latency from
    the write call to the final DAC.
    */
    snd_pcm_sframes_t delay;
    //LOG_INFO(LOG_TAG, "snd_pcm_delay: handle=%p, &delay=%p", handle, &delay);
    double adjust = 0;

    if (snd_pcm_delay(handle, &delay) != 0)
    {
        LOG_ERROR(LOG_TAG, "snd_pcm_delay failed.");
    }
    else
    {
        //LOG_INFO(LOG_TAG, "snd_pcm_delay: returned");
        // The sample currently playing is previous in time to this frame,
        // so adjust negatively.
        adjust = -(delay / (double)sampleRate);
    }

    if (pcmBuffer->TimeStamp() > 0)
    {
        double time = pcmBuffer->TimeStamp() + adjust + audioAdjustSeconds;
        clock = time;
        ClockDataBufferSPTR clockBuffer =
                std::make_shared<ClockDataBuffer>(nullptr);
        clockBuffer->SetTimeStamp(time);
        Q_EMIT clockBufferAvailable(clockBuffer);
    }

    // Send data to ALSA
    int totalFramesWritten = 0;
    while (totalFramesWritten < pcmData->Samples)
    {
        /*
        From ALSA docs:
        If the blocking behaviour is selected and it is running, then
        routine waits until all	requested frames are played or put to
        the playback ring buffer. The returned number of frames can be
        less only if a signal or underrun occurred.
        */
        void* ptr = data + (totalFramesWritten * alsa_channels * sizeof(short));
        snd_pcm_sframes_t framesToWrite = pcmData->Samples - totalFramesWritten;
        //LOG_INFO(LOG_TAG, "snd_pcm_writei: handle=%p, ptr=%p, frames=%ld", handle, ptr, framesToWrite);
        snd_pcm_sframes_t frames = snd_pcm_writei(handle,
            ptr,
            framesToWrite);
        //LOG_INFO(LOG_TAG, "snd_pcm_writei: returned frames=%ld", frames);
        if (frames != framesToWrite)
        {
            if (frames == 0)
            {
                // ALSA will never recover when the return result is 0
                LOG_INFO(LOG_TAG, "snd_pcm_writei: unexpected zero (0) result.");
                break;
            }
            else
            {
                LOG_INFO(LOG_TAG, "snd_pcm_writei failed: %s", snd_strerror(frames));
                //LOG_INFO(LOG_TAG, "snd_pcm_recover: handle=%p, err=%ld, silent=1", handle, frames);
                snd_pcm_recover(handle, frames, 1);
                //LOG_INFO(LOG_TAG, "snd_pcm_recover: returned");
                LOG_INFO(LOG_TAG, "snd_pcm_recover");
            }
        }
        else
        {
            totalFramesWritten += frames;
        }
    }

    if (doPauseFlag)
    {
        //LOG_INFO(LOG_TAG, "handle: %p, enable: 1", handle);
        snd_pcm_pause(handle, 1);
        //LOG_INFO(LOG_TAG, "snd_pcm_pause: returned");
        doPauseFlag = false;
    }
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::AudioAdjustSeconds
 * @return
 */
double AlsaAudioSinkElement::AudioAdjustSeconds() const
{
    INV();
    OUTV("audioAdjustSeconds: %f", audioAdjustSeconds);
    return audioAdjustSeconds;
}

/**
 * @brief AlsaAudioSinkElement::SetAudioAdjustSeconds
 * @param value
 */
void AlsaAudioSinkElement::SetAudioAdjustSeconds(double value)
{
    INV("value: %f", value);
    audioAdjustSeconds = value;
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::Clock
 * @return
 */
double AlsaAudioSinkElement::Clock() const
{
    INV();
    OUTV("clock: %f", clock);
    return clock;
}

/**
 * @brief AlsaAudioSinkElement::Flush
 */
void AlsaAudioSinkElement::Flush()
{
    INV();
    AMLComponent::Flush();
    if (handle)
    {
        snd_pcm_drop(handle);
        snd_pcm_prepare(handle);
    }
    returnAllBuffers();
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::Initialize
 */
void AlsaAudioSinkElement::Initialize()
{
    INV();
    LOG_INFO(LOG_TAG, "Initialize...");
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::State_Executing
 */
void AlsaAudioSinkElement::State_Executing()
{
    INV();
    LOG_INFO(LOG_TAG,
        "State_Executing Started... AlsaAudioSinkElement_Thread in work !");
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::DoWork
 */
void AlsaAudioSinkElement::DoWork()
{
    INV();
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::onBufferAvailable
 * @param buffer
 */
void AlsaAudioSinkElement::onBufferAvailable(BufferSPTR buffer)
{
    INV();

    if(!CheckCurrentState())
    {
        return;
    }

    switch (buffer->Type())
    {
        case BufferTypeEnum::Marker:
        {
            MarkerBufferSPTR markerBuffer =
                    std::static_pointer_cast<MarkerBuffer>(buffer);
            LOG_INFO(LOG_TAG,
                     "got marker buffer Marker: %d",
                     (int)markerBuffer->Marker());

            switch (markerBuffer->Marker())
            {
                case MarkerEnum::EndOfStream:
                    if(!m_audio_codec->isSeekRequest())
                        SetState(Pause);
                    break;
                case MarkerEnum::Discontinue:
                {
                    m_returnAllBuffersMtx.lock();
                    if(m_returnAllBuffers)
                        m_returnAllBuffers = false;
                    m_returnAllBuffersCmd.wakeAll();
                    m_returnAllBuffersMtx.unlock();
                    break;
                }
                default:
                    LOG_ERROR(LOG_TAG,
                              "Unhandled buffer marker type: %u",
                              markerBuffer->Marker());
                    break;
            }
            break;
        }
        case BufferTypeEnum::PcmData:
        {
            if(m_returnAllBuffers)
            {
                LOG_DEBUG(LOG_TAG,"Drop PCM data ...");
            }
            else
            {
                PcmDataBufferSPTR pcmBuffer =
                        std::static_pointer_cast<PcmDataBuffer>(buffer);
                ProcessBuffer(pcmBuffer);
            }
            break;
        }
        default:
            LOG_ERROR(LOG_TAG,
                      "Unexpected buffer type: %u",
                      buffer->Type());
            OUTV();
            return;
    }

    LOG_DEBUG(LOG_TAG, "buffer usecount: %lu", buffer.use_count());
    if (ExecutionState() < ExecutionStateEnum::Idle)
        SetExecutionState(ExecutionStateEnum::Idle);
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::Terminating
 */
void AlsaAudioSinkElement::Terminating()
{
    INV();
    if (handle)
    {
        LOG_INFO(LOG_TAG, ">>>>>>>>>>>>>>>>>>> snd_pcm_close");
        snd_pcm_drop(handle);
        snd_pcm_close(handle);
        snd_config_update_free_global();
        handle = nullptr;
    }
    OUTV();
}

/**
 * @brief AlsaAudioSinkElement::ChangeState
 * @param oldState
 * @param newState
 */
void AlsaAudioSinkElement::ChangeState(MediaState oldState, MediaState newState)
{
    INV("oldState: %u, newState: %u", oldState, newState);
    QMutexLocker locker(&m_wait_state_change);
    AMLComponent::ChangeState(oldState, newState);

    switch (newState)
    {
        case Play:
        {
            playPauseMutex.lock();
            if(handle)
            {
                doPauseFlag = false;
                doResumeFlag = true;
                snd_pcm_pause(handle, 0);
            }
            playPauseMutex.unlock();
            m_wait_state_cmd.wakeAll();
            break;
        }

        case Pause:
        {
            playPauseMutex.lock();
            if(handle)
            {
                doPauseFlag = true;
                doResumeFlag = false;
                snd_pcm_pause(handle, 1);
            }
            playPauseMutex.unlock();

            break;
        }
        case Stop:
        {
            playPauseMutex.lock();
            doStopFlag = true;
            if(handle)
            {
                snd_pcm_pause(handle, 1);
            }
            playPauseMutex.unlock();
            m_wait_state_cmd.wakeAll();
            break;
        }
        default:
            break;
    }

    OUTV();
}
