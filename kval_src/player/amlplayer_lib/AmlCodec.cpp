#include "KvalMiscUtils.h"
#include "AmlCodec.h"
#include "AMLplatform.h"

#define LOG_ACTIVATED
#define LOG_SRC AMLCODEC
#include "KvalLogging.h"


/**
 * @brief AmlCodec::AmlCodec
 */
AmlCodec::AmlCodec()
{
    INV();
    int fd = -1;

#ifndef AML_HARDWARE_DECODER
    apiLevel = ApiLevel::Local;
    if(apiLevel == ApiLevel::Local)
    {
        handle = open("./decodedFile.raw", O_WRONLY);
        OUTV();
        return;
    }
#endif

    fd = open(CODEC_VIDEO_ES_DEVICE, O_WRONLY);
    if (fd < 0)
    {
        LOG_ERROR(LOG_TAG, "AmlCodec open failed");
        OUTV();
        return;
    }

    apiLevel = ApiLevel::S805;
    int version;
    int r = ioctl(fd, AMSTREAM_IOC_GET_VERSION, &version);
    if (r == 0)
    {
        LOG_INFO(LOG_TAG,
                 "amstream version : %d.%d",
                 (version & 0xffff0000) >> 16, version & 0xffff);

        if (version >= 0x20000)
        {
            apiLevel = ApiLevel::S905;
        }
    }
    close(fd);
    OUTV();
}

/**
 * @brief AmlCodec::InternalOpen
 * @param format
 * @param width
 * @param height
 * @param frameRate
 */
void AmlCodec::InternalOpen(VideoFormatEnum format,
                            int width,
                            int height,
                            double frameRate)
{
    INV("format: %u, width: %d, height: %d, framerate: %f",
        format, width, height, frameRate);

#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    if (!OpenAmlVideo())
    {
        LOG_ERROR(LOG_TAG, "Cannot open amlvideo device !");
        return;
    }

    if (apiLevel < ApiLevel::S905)
    {
        if (width > 1920 || height > 1080)
        {
            LOG_ERROR(LOG_TAG,
                      "The video resolution is not supported on this platform");
            OUTV();
            return;
        }
    }

    this->format = format;
    this->width = width;
    this->height = height;
    this->frameRate = frameRate;

    // Open codec
    int flags = O_WRONLY;
    switch (format)
    {
        case VideoFormatEnum::Hevc:
        case VideoFormatEnum::VP9:
            handle = open(CODEC_VIDEO_ES_HEVC_DEVICE, flags);
            break;

        default:
            handle = open(CODEC_VIDEO_ES_DEVICE, flags);
            break;
    }

    if (handle < 0)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG,
                  "AmlCodec open failed");
        OUTV();
        return;
    }

    // Set video format
    vformat_t amlFormat = (vformat_t)0;
    dec_sysinfo_t am_sysinfo = { 0 };
    am_sysinfo.param = (void*)(EXTERNAL_PTS);

    // Note: Testing has shown that the ALSA clock requires the +1
    am_sysinfo.rate = 96000.0 / frameRate + 1;
    switch (format)
    {
        case VideoFormatEnum::Mpeg2:
            LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/MPEG2");

            amlFormat = VFORMAT_MPEG12;
            am_sysinfo.format = VIDEO_DEC_FORMAT_UNKNOW;
            break;

        case VideoFormatEnum::Mpeg4V3:
            LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/MPEG4V3");

            amlFormat = VFORMAT_MPEG4;
            am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_3;
            break;

        case VideoFormatEnum::Mpeg4:
            LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/MPEG4");

            amlFormat = VFORMAT_MPEG4;
            am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
            break;

        case VideoFormatEnum::Avc:
        {
            if ((width > 1920 || height > 1080) &&
                (aml_support_h264_4k2k() == AML_HAS_H264_4K2K))
            {
                LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/H264_4K2K");
                amlFormat = VFORMAT_H264_4K2K;
                am_sysinfo.format = VIDEO_DEC_FORMAT_H264_4K2K;
            }
            else
            {
                LOG_INFO(LOG_TAG, "AmlVideoSink - VIDEO/H264");
                amlFormat = VFORMAT_H264;
                am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
            }
        }
        break;

        case VideoFormatEnum::Hevc:
            LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/HEVC");

            amlFormat = VFORMAT_HEVC;
            am_sysinfo.format = VIDEO_DEC_FORMAT_HEVC;
            break;

        case VideoFormatEnum::VC1:
            LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/VC1");
            amlFormat = VFORMAT_VC1;
            am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
            break;

        case VideoFormatEnum::VP9:
            LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/VP9");
            amlFormat = VFORMAT_VP9;
            am_sysinfo.format = VIDEO_DEC_FORMAT_VP9;
            break;

        default:
            LOG_INFO(LOG_TAG,"AmlVideoSink - VIDEO/UNKNOWN(%d)", (int)format);
            close(handle);
            codecMutex.Unlock();
            return;
    }

    if (amlFormat == VFORMAT_VC1)
    {
        MU_MiscUtils::SetInt("/sys/module/di/parameters/bypass_prog", 0);
    }
    else
    {
        MU_MiscUtils::SetInt("/sys/module/di/parameters/bypass_prog", 1);
    }

    // S905
    am_ioctl_parm parm = { 0 };
    int r;
    if (apiLevel >= ApiLevel::S905) // S905
    {
        parm.cmd = AMSTREAM_SET_VFORMAT;
        parm.data_vformat = amlFormat;
        r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
        if (r < 0)
        {
            close(handle);
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG,"AMSTREAM_IOC_SET failed");
            OUTV();
            return;
        }
    }
    else //S805
    {
        r = ioctl(handle, AMSTREAM_IOC_VFORMAT, amlFormat);
        if (r < 0)
        {
            close(handle);
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG,"AMSTREAM_IOC_SET failed");
            OUTV();
            return;
        }
    }

    r = ioctl(handle, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo);
    if (r < 0)
    {
        close(handle);
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG,"AMSTREAM_IOC_SYSINFO failed");
        OUTV();
        return;
    }

    // Control device
    cntl_handle = open(CODEC_CNTL_DEVICE, O_RDWR);
    if (cntl_handle < 0)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG,"open CODEC_CNTL_DEVICE failed");
        OUTV();
        return;
    }

    if (apiLevel >= ApiLevel::S905) //S905
    {
        parm = { 0 };
        parm.cmd = AMSTREAM_PORT_INIT;

        r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
        if (r != 0)
        {
            close(handle);
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG,"AMSTREAM_PORT_INIT failed");
            OUTV();
            return;
        }
    }
    else //S805
    {
        r = ioctl(handle, AMSTREAM_IOC_PORT_INIT, 0);
        if (r != 0)
        {
            close(handle);
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG,"AMSTREAM_IOC_PORT_INIT failed");
            OUTV();
            return;
        }
    }

    r = ioctl(cntl_handle, AMSTREAM_IOC_SYNCENABLE, (unsigned long)1);
    if (r != 0)
    {
        close(cntl_handle);
        close(handle);
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG,"AMSTREAM_IOC_SYNCENABLE failed");
        OUTV();
        return;
    }

    r = ioctl(cntl_handle,
              AMSTREAM_IOC_SET_VIDEO_DISABLE,
              (unsigned long)VIDEO_DISABLE_NONE);
    if (r != 0)
    {
        close(cntl_handle);
        close(handle);
        LOG_ERROR(LOG_TAG,
                  "AMSTREAM_IOC_SET_VIDEO_DISABLE VIDEO_DISABLE_NONE failed.");
        OUTV();
        return;
    }

    uint32_t screenMode = (uint32_t)VIDEO_WIDEOPTION_NORMAL;
    r = ioctl(cntl_handle, AMSTREAM_IOC_SET_SCREEN_MODE, &screenMode);
    if (r != 0)
    {
        close(cntl_handle);
        close(handle);
        std::string err =
                "AMSTREAM_IOC_SET_SCREEN_MODE VIDEO_WIDEOPTION_NORMAL failed ("
                + std::to_string(r) + ").";
        LOG_ERROR(LOG_TAG,"error: %s", err.c_str());
        OUTV();
        return;
    }

    r = ioctl(cntl_handle, AMSTREAM_IOC_TRICKMODE, (unsigned long)TRICKMODE_NONE);
    if (r != 0)
    {
        std::string err =   "AMSTREAM_IOC_TRICKMODE failed (" +
                            std::to_string(r) + ").";
        LOG_ERROR(LOG_TAG,"error: %s", err.c_str());
    }

    r = ioctl(cntl_handle, AMSTREAM_IOC_SET_FREERUN_MODE, 0);
    if (r != 0)
    {
        std::string err =   "AMSTREAM_IOC_SET_FREERUN_MODE failed (" +
                            std::to_string(r) + ").";
        LOG_ERROR(LOG_TAG,"error: %s", err.c_str());
    }

    // Disable tsync as we are playing video disconnected from audio.
    MU_MiscUtils::SetInt("/sys/class/tsync/enable", 0);

    // Debug info
    LOG_INFO(LOG_TAG,"w: %d h: %d ", width, height);
    LOG_INFO(LOG_TAG,"fps: %f ", frameRate);
    LOG_INFO(LOG_TAG,"am_sysinfo.rate: %d ", am_sysinfo.rate);
    LOG_INFO(LOG_TAG,"");

    isOpen = true;
    OUTV();
}

/**
 * @brief AmlCodec::OpenAmlVideo
 * @return
 */
bool AmlCodec::OpenAmlVideo(void)
{
    return true;

    PosixFilePtr amlVideoFile = std::make_shared<PosixFile>();
    if (!amlVideoFile->Open("/dev/video10", O_RDONLY | O_NONBLOCK))
    {
        LOG_ERROR(LOG_TAG,
                  "Cannot open V4L amlvideo device /dev/video10: %s",
                  strerror(errno));
        return false;
    }

    m_amlVideoFile = amlVideoFile;
    m_defaultVfmMap = GetVfmMap("default");
    SetVfmMap("default", "decoder ppmgr deinterlace amlvideo amvideo");

    MU_MiscUtils::SetInt("/sys/module/amlvideodri/parameters/freerun_mode", 3);

    return true;
}

/**
 * @brief AmlCodec::GetVfmMap
 * @param name
 * @return
 */
std::string AmlCodec::GetVfmMap(QString name)
{
    std::string vfmMap;
    MU_MiscUtils::GetString("/sys/class/vfm/map", vfmMap);
    QString QvfmMap = vfmMap.c_str();

    QStringList sections = QvfmMap.split('\n');

    QString sectionMap;
    for (int i = 0; i < sections.size(); ++i)
    {
        if (sections[i].startsWith(name + " {"))
        {
            LOG_INFO(LOG_TAG, "Got section map");
            sectionMap = sections[i];
            break;
        }
    }

    int openingBracePos = sectionMap.indexOf('{') + 1;
    sectionMap = (sectionMap.toStdString().substr(openingBracePos,
                                        sectionMap.size() - openingBracePos - 1)).c_str();
    sectionMap.replace("(0)", "");

    return sectionMap.toStdString();
}

/**
 * @brief AmlCodec::SetVfmMap
 * @param name
 * @param map
 */
void AmlCodec::SetVfmMap(const std::string &name, const std::string &map)
{
  MU_MiscUtils::SetString("/sys/class/vfm/map", "rm " + name);
  MU_MiscUtils::SetString("/sys/class/vfm/map", "add " + name + " " + map);
}
/**
 * @brief AmlCodec::SetSpeed
 * @param mode
 */
void AmlCodec::SetSpeed(unsigned int mode)
{
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();
    int r = ioctl(cntl_handle, AMSTREAM_IOC_TRICKMODE, (unsigned long)mode);
    if (r < 0)
    {
        std::string err =   "AMSTREAM_IOC_TRICKMODE failed (" +
                            std::to_string(r) + ").";
        LOG_ERROR(LOG_TAG,"error: %s", err.c_str());
    }
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::setVideoMode
 * @param value
 */
void AmlCodec::setVideoMode(AspectRatio value)
{
    INV("value: %u", value);
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();
    uint32_t screenMode = (uint32_t)value;
    int r = ioctl(cntl_handle, AMSTREAM_IOC_SET_SCREEN_MODE, &screenMode);
    if (r != 0)
    {
        std::string err =   "AMSTREAM_IOC_SET_SCREEN_MODE failed (" +
                            std::to_string(r) + ").";
        LOG_ERROR(LOG_TAG,"error: %s", err.c_str());
    }
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::InternalClose
 */
void AmlCodec::InternalClose()
{
    INV();
    int r;
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        close(handle);
        OUTV();
        return;
    }
#endif

    r = ioctl(cntl_handle, AMSTREAM_IOC_CLEAR_VIDEO, 0);
    if (r < 0)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG,"AMSTREAM_IOC_CLEAR_VIDEO failed");
        OUTV();
        return;
    }
    r = close(cntl_handle);
    if (r < 0)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG,"close cntl_handle failed");
        OUTV();
        return;
    }

    r = close(handle);
    if (r < 0)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG,"close handle failed");
        OUTV();
        return;
    }

    //m_amlVideoFile.reset();
    //SetVfmMap("default", m_defaultVfmMap);

    cntl_handle = -1;
    handle = -1;
    isOpen = false;
    OUTV();
}

/**
 * @brief AmlCodec::Open
 * @param format
 * @param width
 * @param height
 * @param frameRate
 */
void AmlCodec::Open(VideoFormatEnum format,
                    int width,
                    int height,
                    double frameRate)
{
    INV("format: %u, width: %d, height: %d, framerate: %f",
        format, width, height, frameRate);
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    if (width < 1)
    {
        LOG_ERROR(LOG_TAG, "Wrong width value !");
        OUTV();
        return;
    }

    if (height < 1)
    {
        LOG_ERROR(LOG_TAG, "Wrong height value !");
        OUTV();
        return;
    }

    if (frameRate < 1)
    {
        LOG_ERROR(LOG_TAG, "Wrong frameRate value !");
        OUTV();
        return;
    }

    codecMutex.Lock();
    if (isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is already open !");
        OUTV();
        return;
    }
    InternalOpen(format, width, height, frameRate);
    codecMutex.Unlock();
    Resume();
    OUTV();
}

/**
 * @brief AmlCodec::Close
 */
void AmlCodec::Close()
{
    INV();
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        InternalClose();
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();
    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open");
        OUTV();
        return;
    }
    InternalClose();
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::GetInt
 * @param path
 * @param val
 * @return
 */
int AmlCodec::GetInt(const std::string& path, int& val)
{
  int fd = open(path.c_str(), O_RDONLY);
  int ret = 0;
  if (fd >= 0)
  {
    char bcmd[16];
    if (read(fd, bcmd, sizeof(bcmd)) < 0)
      ret = -1;
    else
      val = strtol(bcmd, NULL, 16);

    close(fd);
  }
  if (ret)
    LOG_INFO(LOG_TAG, "Error reading %s", path.c_str());

  return ret;
}

/**
 * @brief AmlCodec::SetInt
 * @param path
 * @param val
 * @return
 */
int AmlCodec::SetInt(const std::string& path, const int val)
{
  int fd = open(path.c_str(), O_RDWR, 0644);
  int ret = 0;
  if (fd >= 0)
  {
    char bcmd[16];
    sprintf(bcmd, "%d", val);
    if (write(fd, bcmd, strlen(bcmd)) < 0)
      ret = -1;
    close(fd);
  }
  if (ret)
    LOG_ERROR(LOG_TAG, "Error writing %s", path.c_str());

  return ret;
}

/**
 * @brief AmlCodec::Reset
 */
void AmlCodec::Reset()
{
    INV();
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    Pause();
    codecMutex.Lock();
    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open !");
        OUTV();
        return;
    }
    VideoFormatEnum format = this->format;
    int width = this->width ;
    int height = this->height ;
    double frameRate = this->frameRate;

    int blackout_policy = -1;
    GetInt("/sys/class/video/blackout_policy", blackout_policy);
    SetInt("/sys/class/video/blackout_policy", 0);

    InternalClose();
    InternalOpen(format, width, height, frameRate);

    SetInt("/sys/class/video/blackout_policy", blackout_policy);
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::GetCurrentPts
 * @return
 */
double AmlCodec::GetCurrentPts()
{
    INV();
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return 0.0;
    }
#endif

    codecMutex.Lock();
    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open !");
        OUTV();
        return -1;
    }
    unsigned int vpts;
    int ret;
    if (apiLevel >= ApiLevel::S905) // S905
    {
        am_ioctl_parm parm = { 0 };
        parm.cmd = AMSTREAM_GET_VPTS;
        ret = ioctl(handle, AMSTREAM_IOC_GET, (unsigned long)&parm);
        if (ret < 0)
        {
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG, "AMSTREAM_GET_VPTS failed !");
            OUTV();
            return -1;
        }
        vpts = parm.data_32;
    }
    else    // S805
    {
        ret = ioctl(handle, AMSTREAM_IOC_VPTS, (unsigned long)&vpts);
        if (ret < 0)
        {
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_VPTS failed !");
            OUTV();
            return -1;
        }
    }
    codecMutex.Unlock();
    OUTV();
    return vpts / (double)PTS_FREQ;
}

/**
 * @brief AmlCodec::SetCurrentPts
 * @param value
 */
void AmlCodec::SetCurrentPts(double value)
{
    INV("value: %f", value);
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();
    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open !");
        OUTV();
        return;
    }
    // truncate to 32bit
    unsigned long pts = (unsigned long)(value * PTS_FREQ);
    pts &= 0xffffffff;

    if (apiLevel >= ApiLevel::S905) // S905
    {
        am_ioctl_parm parm = { 0 };
        parm.cmd = AMSTREAM_SET_PCRSCR;
        parm.data_32 = (unsigned int)(pts);
        int ret = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
        if (ret < 0)
        {
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG, "AMSTREAM_SET_PCRSCR failed !");
            OUTV();
            return;
        }
        else
        {
            //LOG_INFO(LOG_TAG,"parm.data_32=%u", parm.data_32);
        }
    }
    else // S805
    {
        int ret = ioctl(handle, AMSTREAM_IOC_SET_PCRSCR, pts);
        if (ret < 0)
        {
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_SET_PCRSCR failed !");
            OUTV();
            return;
        }
    }
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::Pause
 */
void AmlCodec::Pause()
{
    INV();
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();
    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open !");
        OUTV();
        return;
    }
    int ret = ioctl(cntl_handle, AMSTREAM_IOC_VPAUSE, 1);
    if (ret < 0)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_VPAUSE (1) failed");
        OUTV();
        return;
    }
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::Resume
 */
void AmlCodec::Resume()
{
    INV();
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();
    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open");
        OUTV();
        return;
    }
    int ret = ioctl(cntl_handle, AMSTREAM_IOC_VPAUSE, 0);
    if (ret < 0)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_VPAUSE (0) failed");
        OUTV();
        return;
    }
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::GetBufferStatus
 * @return
 */
buf_status AmlCodec::GetBufferStatus()
{
    INV();
    buf_status status;
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        status.data_len = 1024;
        OUTV();
        return status;
    }
#endif
    codecMutex.Lock();

    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open");
        OUTV();
        return status;
    }
    if (apiLevel >= ApiLevel::S905) // S905
    {
        am_ioctl_parm_ex parm = { 0 };
        parm.cmd = AMSTREAM_GET_EX_VB_STATUS;
        int r = ioctl(handle, AMSTREAM_IOC_GET_EX, (unsigned long)&parm);
        codecMutex.Unlock();
        if (r < 0)
        {
            LOG_ERROR(LOG_TAG, "AMSTREAM_GET_EX_VB_STATUS failed.");
            OUTV();
            return status;
        }
        memcpy(&status, &parm.status, sizeof(status));
    }
    else // S805
    {
        am_io_param am_io;
        int r = ioctl(handle, AMSTREAM_IOC_VB_STATUS, (unsigned long)&am_io);
        codecMutex.Unlock();
        if (r < 0)
        {
            LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_VB_STATUS failed");
            OUTV();
            return status;
        }
        memcpy(&status, &am_io.status, sizeof(status));
    }
    OUTV("status: %p", status);
    return status;
}

/**
 * @brief AmlCodec::GetVdecStatus
 * @return
 */
vdec_status AmlCodec::GetVdecStatus()
{
    INV();
    vdec_status status;
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return status;
    }
#endif
    codecMutex.Lock();

    if (!isOpen)
    {
        codecMutex.Unlock();
        return status;
    }
    am_ioctl_parm_ex parm = { 0 };
    parm.cmd = AMSTREAM_GET_EX_VDECSTAT;
    int r = ioctl(handle, AMSTREAM_IOC_GET_EX, (unsigned long)&parm);
    codecMutex.Unlock();
    if (r < 0)
    {
        LOG_ERROR(LOG_TAG, "AMSTREAM_GET_EX_VB_STATUS failed");
        OUTV();
        return status;
    }

    memcpy(&status, &parm.status, sizeof(status));
    OUTV("status: %p", status);
    return status;
}

/**
 * @brief AmlCodec::SetVideoAxis
 * @param rectangle
 */
void AmlCodec::SetVideoAxis(Int32Rectangle rectangle)
{
    INV("rectangle: %p",rectangle);
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();

    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open");
        OUTV();
        return;
    }

    int params[4]{ rectangle.X,
        rectangle.Y,
        rectangle.X + rectangle.Width,
        rectangle.Y + rectangle.Height };
    int ret = ioctl(cntl_handle, AMSTREAM_IOC_SET_VIDEO_AXIS, &params);
    codecMutex.Unlock();
    if (ret < 0)
    {
        LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_SET_VIDEO_AXIS failed");
        OUTV();
        return;
    }
    OUTV();
}

/**
 * @brief AmlCodec::GetVideoAxis
 * @return
 */
Int32Rectangle AmlCodec::GetVideoAxis()
{
    INV();
    Int32Rectangle rect;
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return rect;
    }
#endif
    codecMutex.Lock();

    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open");
        OUTV();
        return rect;
    }

    int params[4] = { 0 };
    int ret = ioctl(cntl_handle, AMSTREAM_IOC_GET_VIDEO_AXIS, &params);
    codecMutex.Unlock();
    if (ret < 0)
    {
        LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_GET_VIDEO_AXIS failed");
        OUTV();
        return rect;
    }

    Int32Rectangle result(params[0],
        params[1],
        params[2] - params[0],
        params[3] - params[1]);

    OUTV("result: %p", result);
    return result;
}

/**
 * @brief AmlCodec::SetSyncThreshold
 * @param pts
 */
void AmlCodec::SetSyncThreshold(unsigned long pts)
{
    INV("pts: %lu", pts);
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();
    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open");
        OUTV();
        return;
    }

    int ret = ioctl(cntl_handle, AMSTREAM_IOC_SYNCTHRESH, pts);
    codecMutex.Unlock();
    if (ret < 0)
    {
        LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_SYNCTHRESH failed");
        OUTV();
        return;
    }
    OUTV();
}

/**
 * @brief AmlCodec::CheckinPts
 * @param pts
 */
void AmlCodec::CheckinPts(unsigned long pts)
{
    INV("pts: %lu", pts);
#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return;
    }
#endif
    codecMutex.Lock();

    if (!isOpen)
    {
        codecMutex.Unlock();
        LOG_ERROR(LOG_TAG, "The codec is not open");
        OUTV();
        return;
    }

    // truncate to 32bit
    pts &= 0xffffffff;

    if (apiLevel >= ApiLevel::S905) // S905
    {
        am_ioctl_parm parm = { 0 };
        parm.cmd = AMSTREAM_SET_TSTAMP;
        parm.data_32 = (unsigned int)pts;

        int r = ioctl(handle, AMSTREAM_IOC_SET, (unsigned long)&parm);
        if (r < 0)
        {
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG, "AMSTREAM_SET_TSTAMP failed");
            OUTV();
            return;
        }
    }
    else // S805
    {
        int r = ioctl(handle, AMSTREAM_IOC_TSTAMP, pts);
        if (r < 0)
        {
            codecMutex.Unlock();
            LOG_ERROR(LOG_TAG, "AMSTREAM_IOC_TSTAMP failed");
            OUTV();
            return;
        }
    }
    codecMutex.Unlock();
    OUTV();
}

/**
 * @brief AmlCodec::WriteData
 * @param data
 * @param length
 * @return
 */
int AmlCodec::WriteData(unsigned char* data, int length)
{
    INV("data: %p, length: %d", data, length);
    if (data == nullptr)
    {
        LOG_ERROR(LOG_TAG, "data NULL");
        OUTV();
        return -1;
    }

    if (length < 1)
    {
        LOG_ERROR(LOG_TAG, "length < 1");
        OUTV();
        return -1;
    }

#ifndef AML_HARDWARE_DECODER
    if(apiLevel == ApiLevel::Local)
    {
        OUTV();
        return length;
    }
#endif

    // This is done unlocked because it blocks
    int ret = write(handle, data, length);
    OUTV("ret: %d", ret);
    return ret;
}
