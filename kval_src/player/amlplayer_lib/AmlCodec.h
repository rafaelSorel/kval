#pragma once

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Exception.h"
#include "Mutex.h"
#include "Pin.h"
#include "Rectangle.h"


// codec_type.h
typedef struct {
    unsigned int    format;  ///< video format, such as H264, MPEG2...
    unsigned int    width;   ///< video source width
    unsigned int    height;  ///< video source height
    unsigned int    rate;    ///< video source frame duration
    unsigned int    extra;   ///< extra data information of video stream
    unsigned int    status;  ///< status of video stream
    unsigned int    ratio;   ///< aspect ratio of video source
    void *          param;   ///< other parameters for video decoder
    unsigned long long    ratio64;   ///< aspect ratio of video source
} dec_sysinfo_t;

#define TRICKMODE_NONE  0x00
#define TRICKMODE_I     0x01
#define TRICKMODE_FFFB  0x02

// vformat.h
typedef enum {
    VIDEO_DEC_FORMAT_UNKNOW,
    VIDEO_DEC_FORMAT_MPEG4_3,
    VIDEO_DEC_FORMAT_MPEG4_4,
    VIDEO_DEC_FORMAT_MPEG4_5,
    VIDEO_DEC_FORMAT_H264,
    VIDEO_DEC_FORMAT_MJPEG,
    VIDEO_DEC_FORMAT_MP4,
    VIDEO_DEC_FORMAT_H263,
    VIDEO_DEC_FORMAT_REAL_8,
    VIDEO_DEC_FORMAT_REAL_9,
    VIDEO_DEC_FORMAT_WMV3,
    VIDEO_DEC_FORMAT_WVC1,
    VIDEO_DEC_FORMAT_SW,
    VIDEO_DEC_FORMAT_AVS,
    VIDEO_DEC_FORMAT_H264_4K2K,
    VIDEO_DEC_FORMAT_HEVC,
    VIDEO_DEC_FORMAT_VP9,
    VIDEO_DEC_FORMAT_MAX
} vdec_type_t;

typedef enum {
    VFORMAT_UNKNOWN = -1,
    VFORMAT_MPEG12 = 0,
    VFORMAT_MPEG4,
    VFORMAT_H264,
    VFORMAT_MJPEG,
    VFORMAT_REAL,
    VFORMAT_JPEG,
    VFORMAT_VC1,
    VFORMAT_AVS,
    VFORMAT_SW,
    VFORMAT_H264MVC,
    VFORMAT_H264_4K2K,
    VFORMAT_HEVC,
    VFORMAT_H264_ENC,
    VFORMAT_JPEG_ENC,
    VFORMAT_VP9,

    /*add new here before.*/
    VFORMAT_MAX,
    VFORMAT_UNSUPPORT = VFORMAT_MAX
} vformat_t;

typedef enum {
    AFORMAT_UNKNOWN = -1,
    AFORMAT_MPEG = 0,
    AFORMAT_PCM_S16LE = 1,
    AFORMAT_AAC = 2,
    AFORMAT_AC3 = 3,
    AFORMAT_ALAW = 4,
    AFORMAT_MULAW = 5,
    AFORMAT_DTS = 6,
    AFORMAT_PCM_S16BE = 7,
    AFORMAT_FLAC = 8,
    AFORMAT_COOK = 9,
    AFORMAT_PCM_U8 = 10,
    AFORMAT_ADPCM = 11,
    AFORMAT_AMR = 12,
    AFORMAT_RAAC = 13,
    AFORMAT_WMA = 14,
    AFORMAT_WMAPRO = 15,
    AFORMAT_PCM_BLURAY = 16,
    AFORMAT_ALAC = 17,
    AFORMAT_VORBIS = 18,
    AFORMAT_AAC_LATM = 19,
    AFORMAT_APE = 20,
    AFORMAT_EAC3 = 21,
    AFORMAT_PCM_WIFIDISPLAY = 22,
    AFORMAT_DRA = 23,
    AFORMAT_SIPR = 24,
    AFORMAT_TRUEHD = 25,
    AFORMAT_MPEG1 = 26, //AFORMAT_MPEG-->mp3,AFORMAT_MPEG1-->mp1,AFROMAT_MPEG2-->mp2
    AFORMAT_MPEG2 = 27,
    AFORMAT_WMAVOI = 28,
    AFORMAT_UNSUPPORT,
    AFORMAT_MAX

} aformat_t;


/**
 * @brief The buf_status struct
 */
struct buf_status {
    int size;
    int data_len;
    int free_len;
    unsigned int read_pointer;
    unsigned int write_pointer;
};

/**
 * @brief The am_ioctl_parm struct
 */
struct am_ioctl_parm {
    union {
        unsigned int data_32;
        unsigned long long data_64;
        vformat_t data_vformat;
        aformat_t data_aformat;
        char data[8];
    };
    unsigned int cmd;
    char reserved[4];
};

/**
 * @brief The vdec_status struct
 */
struct vdec_status {
    unsigned int width;
    unsigned int height;
    unsigned int fps;
    unsigned int error_count;
    unsigned int status;
};

/**
 * @brief The adec_status struct
 */
struct adec_status {
    unsigned int channels;
    unsigned int sample_rate;
    unsigned int resolution;
    unsigned int error_count;
    unsigned int status;
};

/**
 * @brief The userdata_poc_info_t struct
 */
struct userdata_poc_info_t {
    unsigned int poc_info;
    unsigned int poc_number;
};

/**
 * @brief The am_ioctl_parm_ex struct
 */
struct am_ioctl_parm_ex {
    union {
        struct buf_status status;
        struct vdec_status vstatus;
        struct adec_status astatus;

        struct userdata_poc_info_t data_userdata_info;
        char data[24];

    };
    unsigned int cmd;
    char reserved[4];
};

#define AMSTREAM_IOC_MAGIC 'S'
#define AMSTREAM_IOC_SYSINFO _IOW((AMSTREAM_IOC_MAGIC), 0x0a, int)
#define AMSTREAM_IOC_VPAUSE _IOW((AMSTREAM_IOC_MAGIC), 0x17, int)
#define AMSTREAM_IOC_SYNCTHRESH _IOW((AMSTREAM_IOC_MAGIC), 0x19, int)
#define AMSTREAM_IOC_CLEAR_VIDEO _IOW((AMSTREAM_IOC_MAGIC), 0x1f, int)
#define AMSTREAM_IOC_SYNCENABLE _IOW((AMSTREAM_IOC_MAGIC), 0x43, int)
#define AMSTREAM_IOC_SET_VIDEO_DISABLE _IOW((AMSTREAM_IOC_MAGIC), 0x49, int)
#define AMSTREAM_IOC_GET_VIDEO_AXIS   _IOR((AMSTREAM_IOC_MAGIC), 0x4b, int)
#define AMSTREAM_IOC_SET_VIDEO_AXIS   _IOW((AMSTREAM_IOC_MAGIC), 0x4c, int)
#define AMSTREAM_IOC_SET_SCREEN_MODE _IOW((AMSTREAM_IOC_MAGIC), 0x59, int)
#define AMSTREAM_IOC_GET _IOWR((AMSTREAM_IOC_MAGIC), 0xc1, struct am_ioctl_parm)
#define AMSTREAM_IOC_SET _IOW((AMSTREAM_IOC_MAGIC), 0xc2, struct am_ioctl_parm)
#define AMSTREAM_IOC_GET_EX _IOWR((AMSTREAM_IOC_MAGIC), 0xc3, struct am_ioctl_parm_ex)
#define AMSTREAM_IOC_AVTHRESH _IOW((AMSTREAM_IOC_MAGIC), 0x18, int)
#define AMSTREAM_IOC_TRICKMODE _IOW((AMSTREAM_IOC_MAGIC), 0x12, int)
#define AMSTREAM_IOC_SET_FREERUN_MODE _IOW((AMSTREAM_IOC_MAGIC), 0x88, int)

#define AMSTREAM_SET_VFORMAT 0x105
#define AMSTREAM_SET_TSTAMP 0x10E
#define AMSTREAM_PORT_INIT 0x111
#define AMSTREAM_SET_PCRSCR 0x118
#define AMSTREAM_GET_VPTS 0x805
#define AMSTREAM_SET_VIDEO_DELAY_LIMIT_MS 0x11A

#define AMSTREAM_GET_EX_VB_STATUS 0x900
#define AMSTREAM_GET_EX_VDECSTAT 0x902

#define AMSTREAM_IOC_GET_VERSION _IOR((AMSTREAM_IOC_MAGIC), 0xc0, int)

// AMSTREAM_IOC_SET_VIDEO_DISABLE
#define VIDEO_DISABLE_NONE    0
#define VIDEO_DISABLE_NORMAL  1
#define VIDEO_DISABLE_FORNEXT 2

// S805
#define AMSTREAM_IOC_VFORMAT _IOW(AMSTREAM_IOC_MAGIC, 0x04, int)
#define AMSTREAM_IOC_PORT_INIT _IO(AMSTREAM_IOC_MAGIC, 0x11)
#define AMSTREAM_IOC_TSTAMP _IOW(AMSTREAM_IOC_MAGIC, 0x0e, unsigned long)
#define AMSTREAM_IOC_VPTS _IOR(AMSTREAM_IOC_MAGIC, 0x41, unsigned long)
#define AMSTREAM_IOC_SET_PCRSCR _IOW(AMSTREAM_IOC_MAGIC, 0x4a, unsigned long)
#define AMSTREAM_IOC_VB_STATUS _IOR(AMSTREAM_IOC_MAGIC, 0x08, unsigned long)

/**
 * @brief The am_io_param struct
 */
struct am_io_param {
    union {
        int data;
        int id;//get bufstatus? //or others
    };

    int len; //buffer size;

    union {
        char buf[1];
        struct buf_status status;
        struct vdec_status vstatus;
        struct adec_status astatus;
    };
};


/**
 * @brief The ApiLevel enum
 */
enum class ApiLevel
{
    Unknown = 0,
    Local,
    S805,
    S905
};

class PosixFile
{
public:
  PosixFile() :
    m_fd(-1)
  {
  }

  PosixFile(int fd) :
    m_fd(fd)
  {
  }

  ~PosixFile()
  {
    if (m_fd >= 0)
     close(m_fd);
  }

  bool Open(const std::string &pathName, int flags)
  {
    m_fd = open(pathName.c_str(), flags);
    return m_fd >= 0;
  }

  int GetDescriptor() const { return m_fd; }

  int IOControl(unsigned long request, void *param)
  {
    return ioctl(m_fd, request, param);
  }

private:
  int m_fd;
};

typedef std::shared_ptr<PosixFile> PosixFilePtr;

/**
 * @brief The AmlCodec class
 */
class AmlCodec
{
    const unsigned long PTS_FREQ = 90000;

    const long EXTERNAL_PTS = (1);
    const long SYNC_OUTSIDE = (2);
    const long USE_IDR_FRAMERATE = 0x04;
    const long UCODE_IP_ONLY_PARAM = 0x08;
    const long MAX_REFER_BUF = 0x10;
    const long ERROR_RECOVERY_MODE_IN = 0x20;

    const char* CODEC_VIDEO_ES_DEVICE = "/dev/amstream_vbuf";
    const char* CODEC_VIDEO_ES_HEVC_DEVICE = "/dev/amstream_hevc";
    const char* CODEC_CNTL_DEVICE = "/dev/amvideo";
    typedef int CODEC_HANDLE;

    //codec_para_t codec = { 0 };
    bool isOpen = false;
    Mutex codecMutex;
    CODEC_HANDLE handle;
    CODEC_HANDLE cntl_handle;
    VideoFormatEnum format;
    int width;
    int height;
    double frameRate;
    ApiLevel apiLevel;
    PosixFilePtr m_amlVideoFile;
    std::string m_defaultVfmMap;


    void InternalOpen(VideoFormatEnum format,
                      int width,
                      int height,
                      double frameRate);
    void InternalClose();
    bool OpenAmlVideo(void);
    std::string GetVfmMap(QString name);
    void SetVfmMap(const std::string &name, const std::string &map);

public:
    bool IsOpen() const
    {
        return isOpen;
    }

    AmlCodec();
    ~AmlCodec() { }

    void Open(VideoFormatEnum format, int width, int height, double frameRate);
    void Close();
    void Reset();
    double GetCurrentPts();
    void SetCurrentPts(double value);
    void Pause();
    void Resume();
    buf_status GetBufferStatus();
    vdec_status GetVdecStatus();
    //bool SendData(unsigned long pts, unsigned char* data, int length);
    void SetVideoAxis(Int32Rectangle rectangle);
    Int32Rectangle GetVideoAxis();
    void SetSyncThreshold(unsigned long pts);
    void CheckinPts(unsigned long pts);
    int WriteData(unsigned char* data, int length);
    void setVideoMode(AspectRatio value);
    void SetSpeed(unsigned int mode);
    int SetInt(const std::string& path, const int val);
    int GetInt(const std::string& path, int& val);
};
