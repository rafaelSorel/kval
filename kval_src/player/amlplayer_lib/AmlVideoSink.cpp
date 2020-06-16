#include <QObject>
#include "KvalThreadUtils.h"
#include "AmlVideoSink.h"
#include "utils/BitstreamConverter.h"

#define LOG_ACTIVATED
#define LOG_SRC AMLVIDEOSINK
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Public APIs
//----------------------------------------------------------------------------
/**
 * @brief am_packet_allocate
 * @return
 */
am_packet_t *am_packet_allocate(void)
{
    am_packet_t * ampkt = (am_packet_t *)malloc(sizeof(am_packet_t));
    if(!ampkt)
    {
        LOG_ERROR(LOG_TAG, "ENOMMEM am_packet_t");
        return nullptr;
    }
    memset(ampkt, 0, sizeof(am_packet_t));
    memset(&ampkt->avpkt, 0, sizeof(AVPacket));
    return ampkt;
}

/**
 * @brief am_packet_release
 * @param ampkt
 */
void am_packet_release(am_packet_t * ampkt)
{
    if(!ampkt) return;
    av_packet_unref(&ampkt->avpkt);
    free(ampkt);
}

/**
 * @brief AmlVideoSinkClock::AmlVideoSinkClock
 * @param info
 * @param codecPTR
 */
AmlVideoSinkClock::AmlVideoSinkClock(
        AmlVideoSinkElement * owner,
        PinInfoSPTR info,
        AmlCodec* codecPTR,
        bool autoFlushDisabled):
    m_owner(owner),
    PTS_FREQ(90000),
    codecPTR(codecPTR),
    frameRate{},
    m_autoFlushDisabled(autoFlushDisabled),
    m_clock{0.0}
{
    INV("owner: %p, info: %p, codecPTR: %p", owner, info.get(), codecPTR);
    if (codecPTR == nullptr)
        throw ArgumentNullException("codecPTR == nullptr !!");

    LOG_INFO(LOG_TAG, "AmlVideoSinkClock Created ...");
    OUTV();
}

/**
 * @brief AmlVideoSinkClock::~AmlVideoSinkClock
 */
AmlVideoSinkClock::~AmlVideoSinkClock()
{
    INV();
    LOG_INFO(LOG_TAG, "++++++++++++++ ~AmlVideoSinkClock ++++++++++++++");
    OUTV();
}

/**
 * @brief AmlVideoSinkClock::Clock
 * @return
 */
double AmlVideoSinkClock::Clock()
{
    INV();

    if(m_clock)
    {
        OUTV("m_clock: %f", m_clock);
        return m_clock;
    }

    if (codecPTR->IsOpen())
    {
        double current_pts = codecPTR->GetCurrentPts();
        OUTV("current_pts: %f", current_pts);
        return current_pts;
    }
    else
    {
        OUTV("0");
        return 0;
    }
}

/**
 * @brief AmlVideoSinkClock::Close
 */
void AmlVideoSinkClock::Close()
{
    INV();
    LOG_INFO(LOG_TAG, "Terminating av clock.");
    OUTV();
}

/**
 * @brief AmlVideoSinkClock::onClockBufferAvailable
 * @param buffer
 */
void AmlVideoSinkClock::onClockBufferAvailable(BufferSPTR buffer)
{
    INV("buffer: %p", buffer.get());

    //Fill the local clock value
    m_clock = buffer->TimeStamp();

    if (m_owner && m_owner->State() == Play)
    {
        ProcessClockBuffer(buffer);
    }
    OUTV();
}

/**
 * @brief AmlVideoSinkClock::FrameRate
 * @return
 */
double AmlVideoSinkClock::FrameRate() const
{
    INV();
    OUTV("frameRate: %f", frameRate);
    return frameRate;
}

/**
 * @brief AmlVideoSinkClock::SetFrameRate
 * @param value
 */
void AmlVideoSinkClock::SetFrameRate(double value)
{
    INV("value: %f", value);
    LOG_INFO(LOG_TAG, "frameRate: %f", value);
    frameRate = value;
    OUTV();
}

/**
 * @brief AmlVideoSinkClock::ProcessClockBuffer
 * @param buffer
 */
void AmlVideoSinkClock::ProcessClockBuffer(BufferSPTR buffer)
{
    INV("buffer: %p", buffer.get());

    if(!codecPTR->IsOpen())
        return;

    // truncate to 32bit
    uint64_t pts = (uint64_t)(buffer->TimeStamp() * PTS_FREQ);
    pts &= 0xffffffff;

    uint64_t vpts = (uint64_t)(codecPTR->GetCurrentPts() * PTS_FREQ);
    vpts &= 0xffffffff;

    double drift = ((double)vpts - (double)pts) / (double)PTS_FREQ;
    double driftFrames = drift * frameRate;

    if ( (driftFrames >= 15.0 || driftFrames <= -15.0) &&
         (!m_autoFlushDisabled) )
    {
        LOG_INFO(LOG_TAG, "Too many framebuffers dropped -- Flush.");
        return;
    }
    // To minimize clock jitter, only adjust the clock if it
    // deviates more than +/- 2 frames
    if (driftFrames >= 2.0 || driftFrames <= -2.0)
    {
        codecPTR->SetCurrentPts(buffer->TimeStamp());
        LOG_INFO(LOG_TAG,
                 "Adjust PTS - pts=%f vpts=%f drift=%f (%f frames)",
                 pts / (double)PTS_FREQ,
                 vpts / (double)PTS_FREQ,
                 drift,
                 driftFrames);
    }
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::AmlVideoSinkElement
 */
AmlVideoSinkElement::AmlVideoSinkElement()
{
    INV();
    endOfStreamTimer = nullptr;
    m_stop_timer = false;
    m_bitparser = NULL;
    m_bitstream = NULL;

    OUTV();
}

/**
 * @brief AmlVideoSinkElement::~AmlVideoSinkElement
 */
AmlVideoSinkElement::~AmlVideoSinkElement()
{
    INV();
    if (m_bitstream)
      delete m_bitstream, m_bitstream = NULL;

    if (m_bitparser)
      delete m_bitparser, m_bitparser = NULL;

    if(endOfStreamTimer)
        delete endOfStreamTimer;
    LOG_INFO(LOG_TAG, "+++++++++++++++ ~AmlVideoSinkElement +++++++++++++++");
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::resetPlayer
 */
void AmlVideoSinkElement::resetPlayer()
{
    LOG_INFO(LOG_TAG, "resetPlayer ...");
    Q_EMIT reset_player();
}

/**
 * @brief AmlVideoSinkElement::set_display_axis
 * @param recovery
 * @return
 */
bool AmlVideoSinkElement::set_display_axis(bool recovery)
{
    INV("recovery: %u", recovery);
    int fd;
    //int fd1;
    int fd_video_axis, fd_video_disable, fd_video_screenmode;
    const char *path = "/sys/class/display/axis";
    //char *path1 = "/sys/class/graphics/fb0/blank";
    const char *videoaxis_patch = "/sys/class/video/axis";
    const char *videodisable_patch = "/sys/class/video/disable_video";
    const char *videoscreenmode_patch = "/sys/class/video/screen_mode";
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0664);
    //fd1 = open(path1, O_CREAT|O_RDWR | O_TRUNC, 0664);
    fd_video_axis = open(videoaxis_patch,
                         O_CREAT|O_RDWR | O_TRUNC, 0664);
    fd_video_disable = open(videodisable_patch,
                            O_CREAT|O_RDWR | O_TRUNC, 0664);
    fd_video_screenmode = open(videoscreenmode_patch,
                               O_CREAT|O_RDWR | O_TRUNC, 0664);
    if (fd >= 0)
    {

        if (fd_video_axis >= 0 )
        {
             write(fd_video_axis, "0 0 -1 -1", strlen("0 0 -1 -1"));
             close(fd_video_axis);
        }

        if (fd_video_disable >= 0 )
        {
             write(fd_video_disable, "0", strlen("0"));
             close(fd_video_disable);
        }
        if (fd_video_screenmode >= 0 )
        {
             write(fd_video_screenmode, "1", strlen("1"));
             close(fd_video_screenmode);
        }

        OUTV("true");
        return true;
    }

    OUTV("false");
    return false;
}

void AmlVideoSinkElement::Qtimer_Expired()
{
    INV("");
    timerMutex.Lock();

    if(isEndOfStream && (State() != Pause) )
    {
#ifndef AML_HARDWARE_DECODER
        SetState(Pause);
        isEndOfStream = false;
#else
        double pts = amlCodec.GetCurrentPts();
        // If the pts is the same as last time (clock tick = 1/4 s),
        // then assume that playback is done.
        if (pts == eosPts)
        {
            LOG_INFO(LOG_TAG,
                     "isEndOfStream: true, pts: %f, eosPts: %f",
                     pts,
                     eosPts);
            SetState(Pause);
            isEndOfStream = false;
            eosPts = -1;
        }
        else
        {
            eosPts = pts;
        }
#endif
    }
    timerMutex.Unlock();

    LOG_DEBUG(LOG_TAG, "Timer expired.");
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::SetupHardware
 */
void AmlVideoSinkElement::SetupHardware()
{
    INV();
    amlCodec.Open(videoinfo->Format,
                  videoinfo->Width,
                  videoinfo->Height,
                  videoinfo->FrameRate);
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::NaluFormatStartCodes
 * @param codec
 * @param in_extradata
 * @return
 */
bool AmlVideoSinkElement::NaluFormatStartCodes(VideoFormatEnum codec,
                                        std::vector<unsigned char> in_extradata)
{
    INV("codec: %u", codec);
    switch(codec)
    {
        case VideoFormatEnum::Avc:
        case VideoFormatEnum::Hevc:
        {
            // valid avcC atom data always starts with the value 1 (version),
            // otherwise annexb
            if (in_extradata.size() > 7 && in_extradata[0] == 1)
            {
                OUTV("true");
                return true;
            }
        }
        default:
            break;
    }
    OUTV("false");
    return false;
}

/**
 * @brief AmlVideoSinkElement::ProcessBuffer
 * @param buffer
 */
void AmlVideoSinkElement::ProcessBuffer(AVPacketBufferSPTR buffer)
{
    INV("buffer: %p", buffer.get());
    QMutexLocker locker(&playPauseMutex);

    AVPacket* pkt = buffer->GetAVPacket();
    if (!m_aml_reader->IsActive(MediaCategoryEnum::Video, pkt->stream_index))
    {
        LOG_INFO(LOG_TAG, "Video data not for me.");
        return;
    }

    unsigned char* nalHeader = (unsigned char*)pkt->data;
    if (isFirstVideoPacket)
    {
        if(NaluFormatStartCodes(videoFormat, this->extraData))
        {
            isAnnexB = true;
        }
        isFirstVideoPacket = false;
        isShortStartCode = false;
        LOG_INFO(LOG_TAG,"isAnnexB: %u", isAnnexB);
        LOG_INFO(LOG_TAG,"isShortStartCode: %u", isShortStartCode);
    }

    uint64_t pts = 0;
    if (pkt->pts != AV_NOPTS_VALUE)
    {
        double timeStamp = av_q2d(buffer->TimeBase()) * pkt->pts;
        pts = (uint64_t)(timeStamp * PTS_FREQ);
        estimatedNextPts = pkt->pts + pkt->duration;
        lastTimeStamp = timeStamp;
    }

    isExtraDataSent = false;

    switch (videoFormat)
    {
        case VideoFormatEnum::Mpeg2:
        {
            SendCodecData(pts, pkt->data, pkt->size);
            break;
        }

        case VideoFormatEnum::Mpeg4:
        {
            unsigned char* video_extra_data = &extraData[0];
            int video_extra_data_size = extraData.size();
            SendCodecData(0, video_extra_data, video_extra_data_size);
            SendCodecData(pts, pkt->data, pkt->size);
            break;
        }

        case VideoFormatEnum::Mpeg4V3:
        {
            Divx3Header(videoinfo->Width,
                        videoinfo->Height,
                        pkt->size);
            SendCodecData(pts,
                          &videoExtraData[0],
                          videoExtraData.size());
            SendCodecData(0, pkt->data, pkt->size);
            break;
        }

        case VideoFormatEnum::Avc:
        case VideoFormatEnum::Hevc:
        {
            if (isAnnexB)
            {
                // Five least significant bits of first
                // NAL unit byte signify nal_unit_type.
                int nal_unit_type;
                const int nalHeaderLength = 4;
                while (nalHeader < (pkt->data + pkt->size))
                {
                    switch (videoFormat)
                    {
                        case VideoFormatEnum::Avc:
                        {
                            // Copy AnnexB data if NAL unit type is 5
                            nal_unit_type = nalHeader[nalHeaderLength] & 0x1F;
                            if (!isExtraDataSent || nal_unit_type == 5)
                            {
                                ConvertH264ExtraDataToAnnexB();
                                SendCodecData(pts,
                                              &videoExtraData[0],
                                               videoExtraData.size());
                            }
                            isExtraDataSent = true;
                        }
                        break;

                        case VideoFormatEnum::Hevc:
                        {
                            nal_unit_type =
                                    (nalHeader[nalHeaderLength] >> 1) & 0x3f;
                            /* prepend extradata to IRAP frames */
                            if (!isExtraDataSent ||
                                (nal_unit_type >= 16 && nal_unit_type <= 23))
                            {
                                HevcExtraDataToAnnexB();
                                SendCodecData(0,
                                              &videoExtraData[0],
                                        videoExtraData.size());
                            }
                            isExtraDataSent = true;
                        }
                        break;

                        default:
                            LOG_ERROR(LOG_TAG,"Unexpected video format");
                            goto return_cleanup;
                    }

                    // Overwrite header NAL length
                    // with startcode '0x00000001' in *BigEndian*
                    int nalLength = nalHeader[0] << 24;
                    nalLength |= nalHeader[1] << 16;
                    nalLength |= nalHeader[2] << 8;
                    nalLength |= nalHeader[3];
                    if (nalLength < 0 || nalLength > pkt->size)
                    {
                        LOG_ERROR(LOG_TAG,
                                 "Invalid NAL length=%d, pkt->size=%d",
                                 nalLength,
                                 pkt->size);
                        goto return_cleanup;
                    }
                    nalHeader[0] = 0;
                    nalHeader[1] = 0;
                    nalHeader[2] = 0;
                    nalHeader[3] = 1;
                    nalHeader += nalLength + 4;
                }
            }

//            DUMP_HEX(LC_LOG_INFO, pkt->data, pkt->size);
//            LOG_INFO(LOG_TAG, "---------------------------------");
            if (!SendCodecData(pts, pkt->data, pkt->size))
            {
                // Resend extra data on codec reset
                isExtraDataSent = false;
                LOG_ERROR(LOG_TAG, "SendData Failed.");
            }
            break;
        }

        case VideoFormatEnum::VC1:
        {
            SendCodecData(pts, pkt->data, pkt->size);
            break;
        }

        case VideoFormatEnum::VP9:
        {
            am_packet_t * ampkt= am_packet_allocate();
            if(!ampkt) break;
            ampkt->data = pkt->data;
            ampkt->data_size = pkt->size;
            //DUMP_HEX(LC_LOG_INFO, ampkt->data, ampkt->data_size);
            //LOG_INFO(LOG_TAG, "---------------------------------");

            vp9_update_frame_header(ampkt);
            pkt->data = ampkt->data;
            pkt->size = ampkt->data_size;

            SendCodecData(pts, pkt->data, pkt->size);
            am_packet_release(ampkt);
            break;
        }

        default:
            LOG_ERROR(LOG_TAG, "Not Supported video format.");
            goto return_cleanup;
    }

return_cleanup:
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::setAspectRatio
 * @param value
 * @return
 */
bool AmlVideoSinkElement::setAspectRatio(AspectRatio value)
{
    INV("value: %u", value);

    if(!amlCodec.IsOpen())
    {
        OUTV("false");
        return false;
    }

    amlCodec.setVideoMode(value);
    OUTV("true");
    return true;
}

/**
 * @brief AmlVideoSinkElement::SendCodecData
 * @param pts
 * @param data
 * @param length
 * @return
 */
bool AmlVideoSinkElement::SendCodecData(unsigned long pts,
                                        unsigned char* data,
                                        int length)
{

    INV("pts: %lu, data: %p, length: 0x%x", pts, data, length);

    bool result = true;
    if (State() == Stop)
    {
        return 0;
    }

    if (pts > 0)
    {
        amlCodec.CheckinPts(pts);
    }

    int maxAttempts = 150;
    int offset = 0;

    while (offset < length)
    {
        if (State() == Stop)
        {
            result = false;
            break;
        }
        int count = amlCodec.WriteData(data + offset, length - offset);
        if (count > 0)
        {
            offset += count;
        }
        else
        {
            LOG_DEBUG(LOG_TAG, "codec_write failed (%x).", count);
            maxAttempts -= 1;
            if (maxAttempts <= 0)
            {
                LOG_ERROR(LOG_TAG,"codec_write max attempts exceeded.");
                LOG_INFO(LOG_TAG, "Reset Hw decoders...");
                amlCodec.Reset();
                LOG_INFO(LOG_TAG, "Reset Player...");
                resetPlayer();
                result = false;
                break;
            }
        }
    }

    //usleep(5000);
    OUTV("result: %u", result);
    return result;
}
/**
 * @brief AmlVideoSinkElement::Clock
 * @return
 */
double AmlVideoSinkElement::Clock()
{
    INV();
    if (amlCodec.IsOpen())
    {
        double current_pts = amlCodec.GetCurrentPts();
        OUTV("current_pts: %f", current_pts);
        return current_pts;
    }
    else
    {
        OUTV("0");
        return 0;
    }
}

/**
 * @brief AudioCodecElement::Close
 */
void AmlVideoSinkElement::Close()
{
    INV();

    LOG_INFO(LOG_TAG, "Terminating m_player_video.");
    m_av_clock->Close();

    LOG_INFO(LOG_TAG,
             "Disconnect m_player_video slots in order to start cleaning up..");

    if(m_aml_reader)
        m_aml_reader->signalDisconnected(MediaCategoryEnum::Video);

    Terminate();
    WaitForExecutionState(ExecutionStateEnum::Terminated);

    OUTV();
}

/**
 * @brief AmlVideoSinkElement::Open
 * @param vidinfo
 * @param aml_reader
 * @param av_clock
 * @return
 */
bool AmlVideoSinkElement::Open(VideoPinInfo * vidinfo,
                               AmlVideoSinkClock*& av_clock,
                               KvalThread*& av_clock_th,
                               AMLReader *aml_reader,
                               bool autoFlushDisabled)
{
    INV("vidinfo: %p, aml_reader: %p, av_clock: %p",
        vidinfo, aml_reader, av_clock);

    if (vidinfo->Category() != MediaCategoryEnum::Video)
    {
        LOG_ERROR(LOG_TAG,
                 "AmlVideoSink: Not connected to a video pin.");
        OUTV("false");
        return false;
    }

    m_aml_reader    = aml_reader;
    videoinfo       = vidinfo;

    connect(m_aml_reader,
            SIGNAL(bufferAvailable(BufferSPTR)),
            this,
            SLOT(onBufferAvailable(BufferSPTR)));
    connect(m_aml_reader,
            SIGNAL(bufferAvailable_V(BufferSPTR)),
            this,
            SLOT(onBufferAvailable(BufferSPTR)));
    connect(this,
            SIGNAL(sendBackBuffer(BufferSPTR)),
            m_aml_reader,
            SLOT(onBufferReturned(BufferSPTR)),
            Qt::DirectConnection);

    info = std::make_shared<PinInfo>(MediaCategoryEnum::Clock, 0, -1);
    try{
        m_av_clock = new AmlVideoSinkClock(this, info, &amlCodec, autoFlushDisabled);
    } catch (ArgumentNullException &e) {
        LOG_CRITICAL(LOG_TAG, "ArgumentNullException: %s", e.what());
        throw e;
    }
    m_av_clock_thread = new KvalThread("AmlVideoSinkClock");
    m_av_clock->moveToThread(m_av_clock_thread);
    m_av_clock_thread->setObjectName("AmlVideoSinkClock");
    connect(m_av_clock_thread, &QThread::finished, m_av_clock, &QObject::deleteLater);
    m_av_clock_thread->start();

    LOG_INFO(LOG_TAG, "AmlVideoSink: m_av_clock: %p", m_av_clock);

    av_clock = m_av_clock;
    av_clock_th = m_av_clock_thread;

    videoFormat = videoinfo->Format;
    extraData = *(videoinfo->ExtraData);
    m_av_clock->SetFrameRate(videoinfo->FrameRate);
    LOG_INFO(LOG_TAG,
             "AmlVideoSink: ExtraData size=%ld",
             (long int)extraData.size());
    SetupHardware();
    isFirstData = false;

    SetName(std::string("VideoSink"));
    Execute();
    WaitForExecutionState(ExecutionStateEnum::Idle);

    OUTV("true");
    return true;
}

/**
 * @brief AmlVideoSinkElement::Initialize
 */
void AmlVideoSinkElement::Initialize()
{
    INV();

    LOG_INFO(LOG_TAG, "Start AvSink Timer...");
    endOfStreamTimer = new QTimer;
    connect(endOfStreamTimer, SIGNAL(timeout()), this, SLOT(Qtimer_Expired()));
    connect(this, SIGNAL(stopTimer()), endOfStreamTimer, SLOT(stop()));

    endOfStreamTimer->setInterval(500);
    endOfStreamTimer->start();
    OUTV();
}

/**
 * @brief onVidBufferReady
 */
void AmlVideoSinkElement::onBufferAvailable(BufferSPTR buffer)
{
    INV();
    //Check If stop has been issued
    if(!CheckCurrentState())
    {
        LOG_DEBUG(LOG_TAG, "Send back the video buffer...");
        Q_EMIT sendBackBuffer(buffer);
        return;
    }

    switch(buffer->Type())
    {
        case BufferTypeEnum::Marker:
        {
            MarkerBufferSPTR markerBuffer =
                    std::static_pointer_cast<MarkerBuffer>(buffer);
            LOG_INFO(LOG_TAG,
                     "Got marker buffer Marker=%d",
                     (int)markerBuffer->Marker());

            switch (markerBuffer->Marker())
            {
                case MarkerEnum::EndOfStream:
                    if(!m_aml_reader->isSeekRequest())
                        isEndOfStream = true;
                    break;

                case MarkerEnum::Discontinue:
                    m_returnAllBuffersMtx.lock();
                    if(m_returnAllBuffers)
                        m_returnAllBuffers = false;
                    m_returnAllBuffersCmd.wakeAll();
                    m_returnAllBuffersMtx.unlock();
                    break;

                default:
                    break;
            }
            break;
        }

        case BufferTypeEnum::AVPacket:
        {
            LOG_DEBUG(LOG_TAG,"AmlVideoSink: Got a buffer.");
            AVPacketBufferSPTR avPacketBuffer =
                    std::static_pointer_cast<AVPacketBuffer>(buffer);
            if(m_returnAllBuffers)
            {
                LOG_DEBUG(LOG_TAG,"return AVPacket...");
            }
            else
            {
                ProcessBuffer(avPacketBuffer);
            }
            break;
        }

        default:
        {
            LOG_ERROR(LOG_TAG,"Unexpected buffer type.");
        }
    }

    LOG_DEBUG(LOG_TAG, "Send back the video buffer...");
    Q_EMIT sendBackBuffer(buffer);
    if (ExecutionState() < ExecutionStateEnum::Idle)
        SetExecutionState(ExecutionStateEnum::Idle);
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::DoWork
 */
void AmlVideoSinkElement::DoWork()
{
    INV();
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::DoWork
 */
void AmlVideoSinkElement::State_Executing()
{
    INV();
    LOG_INFO(LOG_TAG,
             "State_Executing Started... AmlVideoSinkElement_Thread in work !");
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::ChangeState
 * @param oldState
 * @param newState
 */
void AmlVideoSinkElement::ChangeState(MediaState oldState,
                                      MediaState newState)
{
    INV("oldState: %u, newState: %u", oldState, newState);
    QMutexLocker locker(&m_wait_state_change);
    AMLComponent::ChangeState(oldState, newState);

    switch (State())
    {
        case Play:
        {
            playPauseMutex.lock();
            if (amlCodec.IsOpen())
            {
                amlCodec.Resume();
            }
            playPauseMutex.unlock();
            m_wait_state_cmd.wakeAll();
            break;
        }

        case Pause:
        {
            playPauseMutex.lock();
            if (amlCodec.IsOpen())
            {
                amlCodec.Pause();
            }
            playPauseMutex.unlock();
            break;
        }
        case Stop:
        {
            playPauseMutex.lock();
            if (amlCodec.IsOpen())
            {
                amlCodec.Pause();
            }
            playPauseMutex.unlock();
            m_wait_state_cmd.wakeAll();
            break;
        }

        default:
            break;
    }
    LOG_INFO(LOG_TAG, "Out of changeState");
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::Flush
 */
void AmlVideoSinkElement::Flush()
{
    INV();
    timerMutex.Lock();
    playPauseMutex.lock();

    LOG_INFO(LOG_TAG,"AmlVideoSinkElement: calling base.");
    AMLComponent::Flush();
    if (amlCodec.IsOpen())
    {
        LOG_INFO(LOG_TAG,"Reset.");
        amlCodec.Reset();
    }

    isEndOfStream = false;

    playPauseMutex.unlock();
    timerMutex.Unlock();
    returnAllBuffers();
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::Terminating
 */
void AmlVideoSinkElement::Terminating()
{
    INV();
    LOG_DEBUG(LOG_TAG,"timerMutex wait.");
    timerMutex.Lock();
    LOG_DEBUG(LOG_TAG,"playPauseMutex wait.");
    playPauseMutex.lock();
    Q_EMIT stopTimer();
    if(amlCodec.IsOpen())
    {
        amlCodec.Resume();
        amlCodec.Close();
    }
    LOG_DEBUG(LOG_TAG,"playPauseMutex unlock.");
    playPauseMutex.unlock();
    LOG_DEBUG(LOG_TAG,"timerMutex unlock.");
    timerMutex.Unlock();

    ClearInfosPins();
    LOG_INFO(LOG_TAG,"OUT Terminating.");
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::Divx3Header
 * @param width
 * @param height
 * @param packetSize
 */
void AmlVideoSinkElement::Divx3Header(int width, int height, int packetSize)
{
    INV("width: %d, height: %d, packetSize: %d", width, height, packetSize);

    /** Bitstream info */
    videoExtraData.clear();
    videoExtraData.push_back(0x00);
    videoExtraData.push_back(0x00);
    videoExtraData.push_back(0x00);
    videoExtraData.push_back(0x01);
    unsigned i = (width << 12) | (height & 0xfff);
    videoExtraData.push_back(0x20);
    videoExtraData.push_back((i >> 16) & 0xff);
    videoExtraData.push_back((i >> 8) & 0xff);
    videoExtraData.push_back(i & 0xff);
    videoExtraData.push_back(0x00);
    videoExtraData.push_back(0x00);
    const unsigned char divx311_chunk_prefix[] =
    {
        0x00, 0x00, 0x00, 0x01,
        0xb6, 'D', 'I', 'V', 'X', '3', '.', '1', '1'
    };

    for (size_t i = 0; i < sizeof(divx311_chunk_prefix); ++i)
    {
        videoExtraData.push_back(divx311_chunk_prefix[i]);
    }

    videoExtraData.push_back((packetSize >> 24) & 0xff);
    videoExtraData.push_back((packetSize >> 16) & 0xff);
    videoExtraData.push_back((packetSize >> 8) & 0xff);
    videoExtraData.push_back(packetSize & 0xff);
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::ConvertH264ExtraDataToAnnexB
 */
void AmlVideoSinkElement::ConvertH264ExtraDataToAnnexB()
{
    INV();
    void* video_extra_data = &extraData[0];
    int video_extra_data_size = extraData.size();
    videoExtraData.clear();

    if (video_extra_data_size > 0)
    {
        /** @Refers to:
         * http://aviadr1.blogspot.com/2010/05/
         * h264-extradata-partially-explained-for.html */
        unsigned char* extraData = (unsigned char*)video_extra_data;
        const int spsStart = 6;
        int spsLen = extraData[spsStart] * 256 + extraData[spsStart + 1];
        videoExtraData.push_back(0);
        videoExtraData.push_back(0);
        videoExtraData.push_back(0);
        videoExtraData.push_back(1);
        for (int i = 0; i < spsLen; ++i)
        {
            videoExtraData.push_back(extraData[spsStart + 2 + i]);
        }
        // 2byte sbs len, 1 byte pps start code
        int ppsStart = spsStart + 2 + spsLen + 1;
        int ppsLen = extraData[ppsStart] * 256 + extraData[ppsStart + 1];
        videoExtraData.push_back(0);
        videoExtraData.push_back(0);
        videoExtraData.push_back(0);
        videoExtraData.push_back(1);
        for (int i = 0; i < ppsLen; ++i)
        {
            videoExtraData.push_back(extraData[ppsStart + 2 + i]);
        }
    }
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::HevcExtraDataToAnnexB
 */
void AmlVideoSinkElement::HevcExtraDataToAnnexB()
{
    INV();
    void* video_extra_data = &extraData[0];
    int video_extra_data_size = extraData.size();
    videoExtraData.clear();

    if (video_extra_data_size > 0)
    {
        unsigned char* extraData = (unsigned char*)video_extra_data;

        /** @Refers to:
         *  http://fossies.org/linux/ffmpeg/libavcodec/hevc_mp4toannexb_bsf.c */
        int offset = 21;
        int length_size = (extraData[offset++] & 3) + 1;
        length_size = length_size;// silence compiler warning
        int num_arrays = extraData[offset++];
        for (int i = 0; i < num_arrays; i++)
        {
            int type = extraData[offset++] & 0x3f;
            type = type; // silence compiler warning
            int cnt = extraData[offset++] << 8;
            cnt |= extraData[offset++];
            for (int j = 0; j < cnt; j++)
            {
                videoExtraData.push_back(0);
                videoExtraData.push_back(0);
                videoExtraData.push_back(0);
                videoExtraData.push_back(1);
                int nalu_len = extraData[offset++] << 8;
                nalu_len |= extraData[offset++];
                for (int k = 0; k < nalu_len; ++k)
                {
                    videoExtraData.push_back(extraData[offset++]);
                }
            }
        }
    }
    OUTV();
}

/**
 * @brief AmlVideoSinkElement::vp9_update_frame_header
 * @param pkt
 * @return
 */
int AmlVideoSinkElement::vp9_update_frame_header(am_packet_t *pkt)
{

    int dsize = pkt->data_size;
    unsigned char *buf = pkt->data;
    unsigned char marker;
    int frame_number;
    int cur_frame, cur_mag, mag, index_sz, offset[9], size[8], tframesize[9];
    int mag_ptr;
    int ret;
    unsigned char *old_header = NULL;
    int total_datasize = 0;

    pkt->avpkt.data = pkt->data;
    pkt->avpkt.size = pkt->data_size;

    if (buf == NULL) return 0; /*something error. skip add header*/
    marker = buf[dsize - 1];
    if ((marker & 0xe0) == 0xc0) {
        frame_number = (marker & 0x7) + 1;
        mag = ((marker >> 3) & 0x3) + 1;
        index_sz = 2 + mag * frame_number;
        LOG_DEBUG(LOG_TAG, " frame_number : %d, mag : %d; index_sz : %d\n", frame_number, mag, index_sz);
        offset[0] = 0;
        mag_ptr = dsize - mag * frame_number - 2;
        if (buf[mag_ptr] != marker) {
            LOG_DEBUG(LOG_TAG, " Wrong marker2 : 0x%X --> 0x%X\n", marker, buf[mag_ptr]);
            return 0;
        }
        mag_ptr++;
        for (cur_frame = 0; cur_frame < frame_number; cur_frame++) {
            size[cur_frame] = 0; // or size[0] = bytes_in_buffer - 1; both OK
            for (cur_mag = 0; cur_mag < mag; cur_mag++) {
                size[cur_frame] = size[cur_frame]  | (buf[mag_ptr] << (cur_mag*8) );
                mag_ptr++;
            }
            offset[cur_frame+1] = offset[cur_frame] + size[cur_frame];
            if (cur_frame == 0)
                tframesize[cur_frame] = size[cur_frame];
            else
                tframesize[cur_frame] = tframesize[cur_frame - 1] + size[cur_frame];
            total_datasize += size[cur_frame];
        }
    } else {
        frame_number = 1;
        offset[0] = 0;
        size[0] = dsize; // or size[0] = bytes_in_buffer - 1; both OK
        total_datasize += dsize;
        tframesize[0] = dsize;
    }
    if (total_datasize > dsize) {
        LOG_DEBUG(LOG_TAG, "DATA overflow : 0x%X --> 0x%X\n", total_datasize, dsize);
        return 0;
    }
    if (frame_number >= 1) {
        /*
        if only one frame ,can used headers.
        */
        int need_more = total_datasize + frame_number * 16 - dsize;

        av_buffer_unref(&pkt->avpkt.buf);
        ret = av_grow_packet(&(pkt->avpkt), need_more);
        if (ret < 0) {
            LOG_DEBUG(LOG_TAG, "ERROR!!! grow_packet for apk failed.!!!\n");
            return ret;
        }

        pkt->data = pkt->avpkt.data;
        pkt->data_size = pkt->avpkt.size;
    }
    for (cur_frame = frame_number - 1; cur_frame >= 0; cur_frame--) {
        AVPacket *avpkt = &(pkt->avpkt);
        int framesize = size[cur_frame];
        int oldframeoff = tframesize[cur_frame] - framesize;
        int outheaderoff = oldframeoff + cur_frame * 16;
        uint8_t *fdata = avpkt->data + outheaderoff;
        uint8_t *old_framedata = avpkt->data + oldframeoff;
        memmove(fdata + 16, old_framedata, framesize);
        framesize += 4;/*add 4. for shift.....*/

        /*add amlogic frame headers.*/
        fdata[0] = (framesize >> 24) & 0xff;
        fdata[1] = (framesize >> 16) & 0xff;
        fdata[2] = (framesize >> 8) & 0xff;
        fdata[3] = (framesize >> 0) & 0xff;
        fdata[4] = ((framesize >> 24) & 0xff) ^0xff;
        fdata[5] = ((framesize >> 16) & 0xff) ^0xff;
        fdata[6] = ((framesize >> 8) & 0xff) ^0xff;
        fdata[7] = ((framesize >> 0) & 0xff) ^0xff;
        fdata[8] = 0;
        fdata[9] = 0;
        fdata[10] = 0;
        fdata[11] = 1;
        fdata[12] = 'A';
        fdata[13] = 'M';
        fdata[14] = 'L';
        fdata[15] = 'V';
        framesize -= 4;/*del 4 to real framesize for check.....*/
       if (!old_header) {
           ///nothing
       } else if (old_header > fdata + 16 + framesize) {
           LOG_DEBUG(LOG_TAG, "data has gaps,set to 0\n");
           memset(fdata + 16 + framesize, 0, (old_header - fdata + 16 + framesize));
       } else if (old_header < fdata + 16 + framesize) {
           LOG_DEBUG(LOG_TAG,
                     "ERROR!!! data over writed!!!! over write %d\n",
                     fdata + 16 + framesize - old_header);
       }
       old_header = fdata;
    }
    return 0;
}

