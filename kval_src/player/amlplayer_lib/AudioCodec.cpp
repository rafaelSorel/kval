extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
}

#include "AudioCodec.h"
#include <algorithm>

#define LOG_ACTIVATED
#define LOG_SRC AMLAUDIOCDCELEMT
#include "KvalLogging.h"


//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
/**
 * @brief AudioCodecElement::AudioCodecElement
 */
AudioCodecElement::AudioCodecElement():
    m_audioSink(new AlsaAudioSinkElement),
    m_aml_audiosink_thread(new KvalThread("AlsaAudioSinkElement")),
    m_aml_reader{nullptr},
    m_av_clock{nullptr},
    m_playPauseMutex{},
    alsa_channels{2},
    inInfo{nullptr},
    soundCodec{nullptr},
    soundCodecContext{nullptr},
    isFirstData{true},
    audioFormat{AudioFormatEnum::Unknown},
    streamChannels{0},
    outputChannels{0},
    sampleRate{0}
{
    INV();

    m_audioSink->moveToThread(m_aml_audiosink_thread);
    m_aml_audiosink_thread->setObjectName("AlsaAudioSinkElement");
    connect(m_aml_audiosink_thread, &QThread::finished, m_audioSink, &QObject::deleteLater);
    m_aml_audiosink_thread->start();

    OUTV();
}

/**
 * @brief AudioCodecElement::~AudioCodecElement
 */
AudioCodecElement::~AudioCodecElement()
{
    INV();

    LOG_INFO(LOG_TAG, "Cleanup audio sink...");
    m_aml_audiosink_thread->quit();
    m_aml_audiosink_thread->wait();
    delete m_aml_audiosink_thread;

    LOG_INFO(LOG_TAG, "+++++++++++++++++ ~AudioCodecElement +++++++++++++++++");

    LOG_INFO(LOG_TAG, "Cleanup...");

    ClearInfosPins();

    if(soundCodecContext)
    {
        LOG_INFO(LOG_TAG, "Cleanup audio codecs...");

        if (soundCodec)
            avcodec_close(soundCodecContext);
        soundCodec = nullptr;

        av_free(soundCodecContext);
        soundCodecContext = nullptr;
    }

    OUTV();
}

/**
 * @brief AudioCodecElement::CloseCodec
 */
void AudioCodecElement::CloseCodec()
{
    INV();
    if (soundCodecContext)
    {
        avcodec_flush_buffers(soundCodecContext);
    }

    if(soundCodecContext)
    {
        LOG_INFO(LOG_TAG, "Cleanup audio codecs...");

        if (soundCodec)
            avcodec_close(soundCodecContext);
        soundCodec = nullptr;

        av_free(soundCodecContext);
        soundCodecContext = nullptr;
    }
    OUTV();
}

/**
 * @brief AudioCodecElement::Close
 */
void AudioCodecElement::Close()
{
    INV();

    if(m_audioSink)
    {
        LOG_INFO(LOG_TAG, "Terminating audioSink.");
        LOG_INFO(LOG_TAG, "Disconnect audioSink slots, start cleaning up..");
        if(!this->disconnect(m_audioSink))
        {
            LOG_ERROR(LOG_TAG, "Could not disconnect audioCodec from audioSink !!");
        }
        m_audioSink->Close();
    }

    LOG_INFO(LOG_TAG, "Terminating audioCodec.");
    LOG_INFO(LOG_TAG, "Disconnect audioCodec slots in order to start cleaning up..");
    if(m_aml_reader)
        m_aml_reader->signalDisconnected(MediaCategoryEnum::Audio);

    Terminate();
    WaitForExecutionState(ExecutionStateEnum::Terminated);
}

/**
 * @brief AudioCodecElement::Open
 * @param audinfo
 * @param av_clock
 * @param aml_reader
 * @param device
 * @return
 */
bool AudioCodecElement::Open(AudioPinInfo * audinfo,
                             AmlVideoSinkClock* av_clock,
                             AMLReader *aml_reader,
                             std::string device)
{
    INV("audinfo: %p, av_clock: %p, aml_reader: %p, device: %s",
        audinfo, av_clock, aml_reader, device.c_str());

    if (audinfo->Category() !=MediaCategoryEnum::Audio)
    {
        LOG_ERROR(LOG_TAG, "Not connected to an audio pin.");
        OUTV("false");
        return false;
    }

    LOG_INFO(LOG_TAG, "Create Connections.");

    m_aml_reader    = aml_reader;
    inInfo          = audinfo;
    if(av_clock)
        m_av_clock = av_clock;

    connect(m_aml_reader,
            SIGNAL(bufferAvailable(BufferSPTR)),
            this,
            SLOT(onBufferAvailable(BufferSPTR)));
    connect(m_aml_reader,
            SIGNAL(bufferAvailable_A(BufferSPTR)),
            this,
            SLOT(onBufferAvailable(BufferSPTR)));
    connect(m_aml_reader,
            SIGNAL(audioStreamCodecChanged()),
            this,
            SLOT(setupNewCodec()),
            Qt::DirectConnection);
    connect(this,
            SIGNAL(sendBackBuffer(BufferSPTR)),
            m_aml_reader,
            SLOT(onBufferReturned(BufferSPTR)),
            Qt::DirectConnection);

    audioFormat = audinfo->Format;
    sampleRate = audinfo->SampleRate;
    streamChannels = audinfo->Channels;

    LOG_INFO(LOG_TAG,
             "audinfo->SampleRate=%d, audinfo->Channels=%d",
             audinfo->SampleRate,
             audinfo->Channels);
    SetupCodec();
    isFirstData = false;

    SetName("AudioCodec");
    Execute();
    WaitForExecutionState(ExecutionStateEnum::Idle);

    LOG_INFO(LOG_TAG, "m_audioSink Open.");
    if(!m_audioSink->Open(audinfo, this, m_av_clock, device))
    {
        LOG_ERROR(LOG_TAG, "Could not open audiosink device !");
        OUTV("false");
        return false;
    }

    OUTV("true");
    return true;
}

/**
 * @brief AudioCodecElement::SetupCodec
 * @return
 */
bool AudioCodecElement::SetupCodec()
{
    INV();
    switch (audioFormat)
    {
    case AudioFormatEnum::Aac:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
        break;

    case AudioFormatEnum::Ac3:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_AC3);
        break;

    case AudioFormatEnum::Dts:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_DTS);
        break;

    case AudioFormatEnum::MpegLayer2:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_MP2);
        break;

    case AudioFormatEnum::Mpeg2Layer3:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_MP3);
        break;

    case AudioFormatEnum::WmaPro:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_WMAPRO);
        break;

    case AudioFormatEnum::DolbyTrueHD:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_TRUEHD);
        break;

    case AudioFormatEnum::EAc3:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_EAC3);
        break;

    case AudioFormatEnum::Opus:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
        break;

    case AudioFormatEnum::Vorbis:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_VORBIS);
        break;

    case AudioFormatEnum::PcmDvd:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_PCM_DVD);
        break;

    case AudioFormatEnum::Flac:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_FLAC);
        break;

    case AudioFormatEnum::PcmS24LE:
        soundCodec = avcodec_find_decoder(AV_CODEC_ID_PCM_S24LE);
        break;

    default:
        LOG_ERROR(LOG_TAG,
                  "Audio format %d is not supported.",
                  (int)audioFormat);
        OUTV("false");
        return false;
    }

    if (!soundCodec)
    {
        LOG_ERROR(LOG_TAG, "codec not found");
        OUTV();
        OUTV("false");
        return false;
    }


    soundCodecContext = avcodec_alloc_context3(soundCodec);
    if (!soundCodecContext)
    {
        LOG_ERROR(LOG_TAG, "avcodec_alloc_context3 failed");
        OUTV();
        OUTV("false");
        return false;
    }

    soundCodecContext->channels = alsa_channels;
    soundCodecContext->sample_rate = sampleRate;
    // AV_SAMPLE_FMT_S16P; //AV_SAMPLE_FMT_FLTP;
    soundCodecContext->request_sample_fmt = AV_SAMPLE_FMT_FLTP;
    soundCodecContext->request_channel_layout = AV_CH_LAYOUT_STEREO;
    if (inInfo->ExtraData)
    {
        soundCodecContext->extradata_size = inInfo->ExtraData->size();
        soundCodecContext->extradata = &inInfo->ExtraData->operator[](0);
    }
    /* open it */
    if (avcodec_open2(soundCodecContext, soundCodec, NULL) < 0)
    {
        LOG_ERROR(LOG_TAG, "could not open codec");
        OUTV();
        OUTV("false");
        return false;
    }
    OUTV("true");
    return true;
}

/**
 * @brief AudioCodecElement::setupNewCodec
 */
void AudioCodecElement::setupNewCodec()
{
    INV();

    //Get the new hints for the new audio channel
    inInfo= (AudioPinInfo *)m_aml_reader->GetHints(MediaCategoryEnum::Audio);

    //Setup new codec configuration
    audioFormat = inInfo->Format;
    sampleRate = inInfo->SampleRate;
    streamChannels = inInfo->Channels;

    LOG_INFO(LOG_TAG,
             "New data audinfo->SampleRate=%d, audinfo->Channels=%d",
             inInfo->SampleRate,
             inInfo->Channels);

    CloseCodec();
    SetupCodec();
    isFirstData = false;

    //Set New Decoder parameters
    if(!m_audioSink->setupNewDecoder(inInfo))
    {
        LOG_ERROR(LOG_TAG, "Problem setting up new decoder !!");
    }

    OUTV();
}

/**
 * @brief count_bits
 * @param value
 */
static unsigned count_bits(int64_t value)
{
  unsigned bits = 0;
  for(;value;++bits)
    value &= value - 1;
  return bits;
}

/**
 * @brief AudioCodecElement::BuildChannelMap
 */
void AudioCodecElement::BuildChannelMap()
{
    int64_t layout;

    int bits = count_bits(soundCodecContext->channel_layout);
    if (bits == soundCodecContext->channels)
    {
        layout = soundCodecContext->channel_layout;
    }
    else
    {
        LOG_INFO(LOG_TAG,
                 "FFmpeg reported %d channels, but the layout contains %d ignoring",
                 soundCodecContext->channels,
                 bits);

        layout = av_get_default_channel_layout(soundCodecContext->channels);
    }

    if (layout & AV_CH_FRONT_LEFT           ) LOG_INFO(LOG_TAG, "AV_CH_FRONT_LEFT") ;
    if (layout & AV_CH_FRONT_RIGHT          ) LOG_INFO(LOG_TAG, "AV_CH_FRONT_RIGHT") ;
    if (layout & AV_CH_FRONT_CENTER         ) LOG_INFO(LOG_TAG, "AV_CH_FRONT_CENTER") ;
    if (layout & AV_CH_LOW_FREQUENCY        ) LOG_INFO(LOG_TAG, "AV_CH_LOW_FREQUENCY") ;
    if (layout & AV_CH_BACK_LEFT            ) LOG_INFO(LOG_TAG, "AV_CH_BACK_LEFT") ;
    if (layout & AV_CH_BACK_RIGHT           ) LOG_INFO(LOG_TAG, "AV_CH_BACK_RIGHT") ;
    if (layout & AV_CH_FRONT_LEFT_OF_CENTER ) LOG_INFO(LOG_TAG, "AV_CH_FRONT_LEFT_OF_CENTER") ;
    if (layout & AV_CH_FRONT_RIGHT_OF_CENTER) LOG_INFO(LOG_TAG, "AV_CH_FRONT_RIGHT_OF_CENTER") ;
    if (layout & AV_CH_BACK_CENTER          ) LOG_INFO(LOG_TAG, "AV_CH_BACK_CENTER") ;
    if (layout & AV_CH_SIDE_LEFT            ) LOG_INFO(LOG_TAG, "AV_CH_SIDE_LEFT") ;
    if (layout & AV_CH_SIDE_RIGHT           ) LOG_INFO(LOG_TAG, "AV_CH_SIDE_RIGHT") ;
    if (layout & AV_CH_TOP_CENTER           ) LOG_INFO(LOG_TAG, "AV_CH_TOP_CENTER") ;
    if (layout & AV_CH_TOP_FRONT_LEFT       ) LOG_INFO(LOG_TAG, "AV_CH_TOP_FRONT_LEFT") ;
    if (layout & AV_CH_TOP_FRONT_CENTER     ) LOG_INFO(LOG_TAG, "AV_CH_TOP_FRONT_CENTER") ;
    if (layout & AV_CH_TOP_FRONT_RIGHT      ) LOG_INFO(LOG_TAG, "AV_CH_TOP_FRONT_RIGHT") ;
    if (layout & AV_CH_TOP_BACK_LEFT        ) LOG_INFO(LOG_TAG, "AV_CH_TOP_BACK_LEFT") ;
    if (layout & AV_CH_TOP_BACK_CENTER      ) LOG_INFO(LOG_TAG, "AV_CH_TOP_BACK_CENTER") ;
    if (layout & AV_CH_TOP_BACK_RIGHT       ) LOG_INFO(LOG_TAG, "AV_CH_TOP_BACK_RIGHT") ;
}

/**
 * @brief AudioCodecElement::Resample
 * @param data_in
 * @param sample_in
 * @param data_out
 * @param data_out_len
 * @param samples_out
 */
void AudioCodecElement::Resample(uint8_t** data_in,
                                 int sample_in,
                                 uint8_t * data_out,
                                 int * data_out_len,
                                 int * samples_out)
{
    SwrContext* swr =
        swr_alloc_set_opts(NULL,
                           AV_CH_LAYOUT_STEREO,                 // output
                           AV_SAMPLE_FMT_S16,                   // output
                           44100,                               // output
                           soundCodecContext->channel_layout,   // input
                           soundCodecContext->sample_fmt,       // input
                           soundCodecContext->sample_rate,      // input
                           0,
                           NULL);
    if (!swr)
    {
        LOG_ERROR(LOG_TAG, "error: swr_alloc_set_opts()");
        return;
    }
    swr_init(swr);
    int out_buffer_size = av_samples_get_buffer_size(NULL,2 ,44100,AV_SAMPLE_FMT_S16, 1);
    LOG_INFO(LOG_TAG, "out_buffer_size: %d", out_buffer_size);

    int out_samples = av_rescale_rnd(swr_get_delay(swr, soundCodecContext->sample_rate) +
                                     sample_in,
                                     44100,
                                     soundCodecContext->sample_rate,
                                     AV_ROUND_UP);
    av_samples_alloc(&data_out,
                     NULL,
                     2,
                     out_samples,
                     AV_SAMPLE_FMT_S16,
                     0);
    int got_samples = swr_convert(swr,
                              &data_out,
                              out_samples,
                              (const uint8_t**)data_in,
                              sample_in);

    if(got_samples > 0)
    {
        LOG_INFO(LOG_TAG, "out_samples: %d", out_samples);

        *samples_out = out_samples;
        *data_out_len = out_buffer_size;
    }
    else
    {
        *data_out_len = 0;
        *samples_out = 0;
    }

//    av_freep(&output);
}

/**
 * @brief AudioCodecElement::ProcessBuffer
 * @param buffer
 * @param frame
 */
void AudioCodecElement::ProcessBuffer(AVPacketBufferSPTR buffer)
{
    INV("buffer: %p ", buffer.get());
    QMutexLocker locker(&m_playPauseMutex);
    if(!soundCodecContext)
    {
        LOG_ERROR(LOG_TAG, "Context not allocated");
        return;
    }

    AVPacket* pkt = buffer->GetAVPacket();
    if(!pkt)
    {
        LOG_ERROR(LOG_TAG, "pkt NULL");
        OUTV();
        return;
    }
    AVFrame* decoded_frame = m_av_frame->GetAVFrame();
    if(!decoded_frame)
    {
        LOG_ERROR(LOG_TAG, "decoded_frame NULL");
        OUTV();
        return;
    }

    // Decode audio
    int bytesDecoded = 0;
    while (IsExecuting() && bytesDecoded < pkt->size)
    {
        int got_frame = 0;
        int len = avcodec_decode_audio4(soundCodecContext, decoded_frame, &got_frame, pkt);

        if (len < 0)
        {
            // Report the error, but otherwise ignore it.
            char errmsg[1024] = { 0 };
            av_strerror(len, errmsg, 1024);
            LOG_ERROR(LOG_TAG, "Error while decoding: %s", errmsg);
            break;
        }
        else
        {
            bytesDecoded += len;
        }
        LOG_DEBUG(LOG_TAG,
                 "decoded audio frame OK (len=%x, pkt.size=%x)",
                 len,
                 buffer->GetAVPacket()->size);

        LOG_DEBUG(LOG_TAG,
                 "decoded_frame->format = %u)",
                 decoded_frame->format);

        // Convert audio to ALSA format
        if (got_frame)
        {
            // Copy out the PCM data because libav fills the frame
            // with re-used data pointers.
            PcmFormat format;
            bool isInterleaved;
            switch (decoded_frame->format)
            {
            case AV_SAMPLE_FMT_S16:
                format = PcmFormat::Int16;
                isInterleaved = true;
                break;

            case AV_SAMPLE_FMT_S32:
                format = PcmFormat::Int32;
                isInterleaved = true;
                break;

            case AV_SAMPLE_FMT_FLT:
                format = PcmFormat::Float32;
                isInterleaved = true;
                break;

            case AV_SAMPLE_FMT_S16P:
                format = PcmFormat::Int16Planes;
                isInterleaved = false;
                break;

            case AV_SAMPLE_FMT_S32P:
                format = PcmFormat::Int32Planes;
                isInterleaved = false;
                break;

            case AV_SAMPLE_FMT_FLTP:
                format = PcmFormat::Float32Planes;
                isInterleaved = false;
                break;

            default:
                LOG_ERROR(LOG_TAG,
                          "Sample format (%d) not supported.",
                          decoded_frame->format);
                OUTV();
                return;
            }
            //BuildChannelMap();
            PcmDataBufferSPTR pcmDataBuffer{nullptr};
            try {
                pcmDataBuffer = std::make_shared<PcmDataBuffer>(
                    nullptr,
                    format,
                    decoded_frame->channels,
                    decoded_frame->nb_samples);
            } catch(NotSupportedException &e) {
                LOG_CRITICAL(LOG_TAG,
                        "Unable to creates the pcmDataBuffer, format not supported !");
                return;
            }

            if (buffer->GetAVPacket()->pts != AV_NOPTS_VALUE)
            {
                pcmDataBuffer->SetTimeStamp(
                    av_frame_get_best_effort_timestamp(m_av_frame->GetAVFrame()) *
                    av_q2d(buffer->TimeBase()));
            }
            else
            {
                pcmDataBuffer->SetTimeStamp(-1);
            }

            if (isInterleaved)
            {
                PcmData* pcmData = pcmDataBuffer->GetPcmData();
                memcpy(pcmData->Channel[0],
                        decoded_frame->data[0],
                        pcmData->ChannelSize);
            }
            else
            {

                // left, right, center
                const int DOWNMIX_MAX_CHANNELS = 6;
                void* channels[DOWNMIX_MAX_CHANNELS] = { 0 };
                if (decoded_frame->channels == 1)
                {
                    // Mono
                    int monoChannelIndex = av_get_channel_layout_channel_index(
                        decoded_frame->channel_layout,
                        AV_CH_FRONT_CENTER);
                    channels[0] = (void*)decoded_frame->data[monoChannelIndex];
                }
                else if (decoded_frame->channels == 2)
                {
                    int leftChannelIndex = av_get_channel_layout_channel_index(
                        decoded_frame->channel_layout,
                        AV_CH_FRONT_LEFT);
                    if (leftChannelIndex < 0)
                    {
                        LOG_ERROR(LOG_TAG, "AV_CH_FRONT_LEFT failed.");
                        return;
                    }

                    int rightChannelIndex = av_get_channel_layout_channel_index(
                        decoded_frame->channel_layout,
                        AV_CH_FRONT_RIGHT);
                    if (rightChannelIndex < 0)
                    {
                        LOG_ERROR(LOG_TAG, "AV_CH_FRONT_RIGHT failed.");
                        return;
                    }
                    channels[0] = (void*)decoded_frame->data[leftChannelIndex];
                    channels[1] = (void*)decoded_frame->data[rightChannelIndex];
                }
#if 1 //5.1 channels
                else if (decoded_frame->channels == 6)
                {
                    int64_t layout = 0;
                    int bits = count_bits(soundCodecContext->channel_layout);
                    if (bits == soundCodecContext->channels)
                    {
                        layout = soundCodecContext->channel_layout;
                    }
                    else
                    {
                        LOG_DEBUG(LOG_TAG,
                                 "FFmpeg reported %d channels, but the layout contains %d ignoring",
                                 soundCodecContext->channels,
                                 bits);

                        layout = av_get_default_channel_layout(soundCodecContext->channels);
                    }


                    if(layout & AV_CH_FRONT_LEFT)
                    {
                        int leftChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_FRONT_LEFT);
                        if (leftChannelIndex < 0)
                        {
                            LOG_ERROR(LOG_TAG, "AV_CH_FRONT_LEFT failed.");
                            return;
                        }
                        channels[0] = (void*)decoded_frame->data[leftChannelIndex];
                    }
                    if(layout & AV_CH_FRONT_RIGHT)
                    {
                        int rightChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_FRONT_RIGHT);
                        if (rightChannelIndex < 0)
                        {
                            LOG_ERROR(LOG_TAG, "AV_CH_FRONT_RIGHT failed.");
                            return;
                        }
                        channels[1] = (void*)decoded_frame->data[rightChannelIndex];
                    }
                    if(layout & AV_CH_FRONT_CENTER)
                    {
                        int centerChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_FRONT_CENTER);
                        if (centerChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_FRONT_CENTER failed.");
                            return;
                        }
                        channels[2] = (void*)decoded_frame->data[centerChannelIndex];
                    }
                    if(layout & AV_CH_SIDE_LEFT)
                    {
                        int backLeftChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_SIDE_LEFT);
                        if (backLeftChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_SIDE_LEFT failed.");
                            return;
                        }
                        channels[4] = (void*)decoded_frame->data[backLeftChannelIndex];

                    }
                    else if(layout & AV_CH_BACK_LEFT)
                    {
                        int backLeftChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_BACK_LEFT);
                        if (backLeftChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_BACK_LEFT failed.");
                            return;
                        }
                        channels[4] = (void*)decoded_frame->data[backLeftChannelIndex];

                    }
                    else
                    {
                        int backLeftChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_FRONT_LEFT);
                        if (backLeftChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_FRONT_LEFT failed.");
                            return;
                        }
                        channels[4] = (void*)decoded_frame->data[backLeftChannelIndex];
                    }


                    if(layout & AV_CH_SIDE_RIGHT)
                    {
                        int backRightChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_SIDE_RIGHT);
                        if (backRightChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_SIDE_RIGHT failed.");
                            return;
                        }
                        channels[5] = (void*)decoded_frame->data[backRightChannelIndex];

                    }
                    else if(layout & AV_CH_BACK_RIGHT)
                    {
                        int backRightChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_BACK_RIGHT);
                        if (backRightChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_BACK_RIGHT failed.");
                            return;
                        }
                        channels[5] = (void*)decoded_frame->data[backRightChannelIndex];

                    }
                    else
                    {
                        int backRightChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_FRONT_RIGHT);
                        if (backRightChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_FRONT_RIGHT failed.");
                            return;
                        }
                        channels[5] = (void*)decoded_frame->data[backRightChannelIndex];

                    }
                    if(layout & AV_CH_LOW_FREQUENCY)
                    {
                        int SubWooferChannelIndex = av_get_channel_layout_channel_index(
                            decoded_frame->channel_layout,
                            AV_CH_LOW_FREQUENCY);
                        if (SubWooferChannelIndex < 0)
                        {
                            LOG_INFO(LOG_TAG, "AV_CH_LOW_FREQUENCY failed.");
                            return;
                        }
                        channels[3] = (void*)decoded_frame->data[SubWooferChannelIndex];
                    }
                }
#endif
                else
                {
                    int leftChannelIndex = av_get_channel_layout_channel_index(
                        decoded_frame->channel_layout,
                        AV_CH_FRONT_LEFT);
                    if (leftChannelIndex < 0)
                    {
                        LOG_ERROR(LOG_TAG, "AV_CH_FRONT_LEFT failed.");
                        return;
                    }

                    int rightChannelIndex = av_get_channel_layout_channel_index(
                        decoded_frame->channel_layout,
                        AV_CH_FRONT_RIGHT);
                    if (rightChannelIndex < 0)
                    {
                        LOG_ERROR(LOG_TAG, "AV_CH_FRONT_RIGHT failed.");
                        return;
                    }

                    int centerChannelIndex = av_get_channel_layout_channel_index(
                        decoded_frame->channel_layout,
                        AV_CH_FRONT_CENTER);
                    if (centerChannelIndex < 0)
                    {
                        LOG_INFO(LOG_TAG, "AV_CH_FRONT_CENTER failed.");
                        return;
                    }
                    channels[0] = (void*)decoded_frame->data[leftChannelIndex];
                    channels[1] = (void*)decoded_frame->data[rightChannelIndex];
                    channels[2] = (void*)decoded_frame->data[centerChannelIndex];
                }

                int channelCount = std::min(DOWNMIX_MAX_CHANNELS, decoded_frame->channels);

                for (int i = 0; i < channelCount; ++i)
                {
                    if(!channels[i])
                    {
                        if(i>0)
                        {
                            channels[i] = channels[i-1];
                        }
                        else
                            return;
                    }

                    if(!pcmDataBuffer.get()) return;
                    PcmData* pcmData = pcmDataBuffer->GetPcmData();
                    if(!pcmData) return;

                    memcpy(pcmData->Channel[i],
                           channels[i],
                           pcmData->ChannelSize);
                }
            }

            Q_EMIT bufferAvailable(pcmDataBuffer);
        }
    }
    OUTV();
}

/**
 * @brief AudioCodecElement::Initialize
 */
void AudioCodecElement::Initialize()
{
    INV();

    // Create buffer(s)
    LOG_INFO(LOG_TAG, "Create Audio Frame buffer...");
    m_av_frame = std::make_shared<AVFrameBuffer>(nullptr);
    OUTV();
}

/**
 * @brief AudioCodecElement::State_Executing
 */
void AudioCodecElement::State_Executing()
{
    INV();
    LOG_INFO(LOG_TAG,
             "State_Executing Started... AudioCodecElement_Thread in work !");
    OUTV();
}

/**
 * @brief AudioCodecElement::DoWork
 */
void AudioCodecElement::DoWork()
{
    INV();
    OUTV();
}

/**
 * @brief AudioCodecElement::onBufferAvailable
 * @param buffer
 */
void AudioCodecElement::onBufferAvailable(BufferSPTR buffer)
{
    INV("buffer: %p", buffer.get());

    if(!CheckCurrentState())
    {
        LOG_DEBUG(LOG_TAG, "Send back the audio buffer...");
        Q_EMIT sendBackBuffer(buffer);
        return;
    }
    switch (buffer->Type())
    {
        case BufferTypeEnum::AVPacket:
        {
            AVPacketBufferSPTR avPacketBuffer =
                    std::static_pointer_cast<AVPacketBuffer>(buffer);
            if (!m_returnAllBuffers &&
                m_aml_reader->IsActive(MediaCategoryEnum::Audio,
                                avPacketBuffer->GetAVPacket()->stream_index))
            {
                ProcessBuffer(avPacketBuffer);
            }
            break;
        }
        case BufferTypeEnum::Marker:
        {
            MarkerBufferSPTR markerBuffer =
                    std::static_pointer_cast<MarkerBuffer>(buffer);
            switch (markerBuffer->Marker())
            {
                case MarkerEnum::EndOfStream:
                {
                    if(!m_aml_reader->isSeekRequest())
                    {
                        // Send EOS buffer to alsa obj
                        MarkerBufferSPTR eosBuffer =
                                std::make_shared<MarkerBuffer>(
                                    nullptr,
                                    MarkerEnum::EndOfStream);
                        Q_EMIT bufferAvailable(eosBuffer);
                        SetState(Pause);
                    }
                    break;
                }
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
                    // ignore unknown
                    break;
            }
            break;
        }
        default:
            // Ignore
            break;
    }
    LOG_DEBUG(LOG_TAG, "Send back the audio buffer...");
    Q_EMIT sendBackBuffer(buffer);
    if (ExecutionState() < ExecutionStateEnum::Idle)
        SetExecutionState(ExecutionStateEnum::Idle);
    OUTV();
}

/**
 * @brief AudioCodecElement::ChangeState
 * @param oldState
 * @param newState
 */
void AudioCodecElement::ChangeState(MediaState oldState, MediaState newState)
{
    INV("oldState: %u, newState: %u", oldState, newState);
    QMutexLocker locker(&m_wait_state_change);
    AMLComponent::ChangeState(oldState, newState);
    switch (newState)
    {
        case Play:
        case Stop:
        {
            m_wait_state_cmd.wakeAll();
            break;
        }
        case Pause:
        default:
            break;
    }

    OUTV();
}

/**
 * @brief AudioCodecElement::Flush
 */
void AudioCodecElement::Flush()
{
    INV();

    m_playPauseMutex.lock();
    AMLComponent::Flush();
    if (soundCodecContext)
    {
        avcodec_flush_buffers(soundCodecContext);
    }
    m_playPauseMutex.unlock();
    returnAllBuffers();
    OUTV();
}
