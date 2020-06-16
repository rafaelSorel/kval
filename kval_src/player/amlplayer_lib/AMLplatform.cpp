#define LOG_ACTIVATED

#include "linux/fb.h"
#include <sys/ioctl.h>
#include <cmath>

#include "AMLplatform.h"
#include "KvalMiscUtils.h"

#define LOG_SRC AMLPLATFORM
#include "KvalLogging.h"

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Type definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Public interface
//-----------------------------------------------------------------------------

/**
 * @brief aml_wired_present
 * @return
 */
bool aml_wired_present()
{
  static int has_wired = -1;
  if (has_wired == -1)
  {
    std::string test;
    if (MU_MiscUtils::GetString("/sys/class/net/eth0/operstate", test) != -1)
      has_wired = 1;
    else
      has_wired = 0;
  }
  return has_wired == 1;
}

/**
 * @brief aml_support_hevc
 * @return
 */
bool aml_support_hevc()
{
  static int has_hevc = -1;

  if (has_hevc == -1)
  {
    std::string valstr;
    if(MU_MiscUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
      has_hevc = 0;
    else
      has_hevc = (valstr.find("hevc:") != std::string::npos) ? 1: 0;
  }
  return (has_hevc == 1);
}

/**
 * @brief aml_support_hevc_4k2k
 * @return
 */
bool aml_support_hevc_4k2k()
{
  static int has_hevc_4k2k = -1;
  if (has_hevc_4k2k == -1)
  {
    QRegExp regexp("hevc:.*4k");
    std::string valstr;
    if (MU_MiscUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
      has_hevc_4k2k = 0;
    else
      has_hevc_4k2k = (regexp.exactMatch(QString::fromStdString(valstr))) ? 1 : 0;
  }
  return (has_hevc_4k2k == 1);
}

/**
 * @brief aml_support_hevc_10bit
 * @return
 */
bool aml_support_hevc_10bit()
{
  static int has_hevc_10bit = -1;
  if (has_hevc_10bit == -1)
  {
    QRegExp regexp("hevc:.*10bit");
    std::string valstr;
    if (MU_MiscUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
      has_hevc_10bit = 0;
    else
      has_hevc_10bit = (regexp.exactMatch(QString::fromStdString(valstr))) ? 1 : 0;
  }
  return (has_hevc_10bit == 1);
}

/**
 * @brief aml_support_h264_4k2k
 * @return
 */
AML_SUPPORT_H264_4K2K aml_support_h264_4k2k()
{
  static AML_SUPPORT_H264_4K2K has_h264_4k2k = AML_SUPPORT_H264_4K2K_UNINIT;

  if (has_h264_4k2k == AML_SUPPORT_H264_4K2K_UNINIT)
  {
    std::string valstr;
    if (MU_MiscUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
      has_h264_4k2k = AML_NO_H264_4K2K;
    else if (valstr.find("h264:4k") != std::string::npos)
      has_h264_4k2k = AML_HAS_H264_4K2K_SAME_PROFILE;
    else if (valstr.find("h264_4k2k:") != std::string::npos)
      has_h264_4k2k = AML_HAS_H264_4K2K;
    else
      has_h264_4k2k = AML_NO_H264_4K2K;
  }
  return has_h264_4k2k;
}

/**
 * @brief aml_support_vp9
 * @return
 */
bool aml_support_vp9()
{
    static int has_vp9 = -1;

    if (has_vp9 == -1)
    {
        //CRegExp regexp;
        //regexp.RegComp("vp9:.*compressed");
        std::string valstr;
        if (MU_MiscUtils::GetString("/sys/class/amstream/vcodec_profile", valstr) != 0)
        {
            has_vp9 = 0;
        }
        else
        {
            QString fileContents = valstr.c_str();
            if(fileContents.contains("vp9"))
                has_vp9 = 1;
            else
                has_vp9 = 0;
        }
    }
    return (has_vp9 == 1);
}

/**
 * @brief aml_set_audio_passthrough
 * @param passthrough
 */
void aml_set_audio_passthrough(bool passthrough)
{
  MU_MiscUtils::SetInt("/sys/class/audiodsp/digital_raw", passthrough ? 2:0);
}

/**
 * @brief aml_probe_hdmi_audio
 */
void aml_probe_hdmi_audio()
{
  // Audio {format, channel, freq, cce}
  // {1, 7, 7f, 7}
  // {7, 5, 1e, 0}
  // {2, 5, 7, 0}
  // {11, 7, 7e, 1}
  // {10, 7, 6, 0}
  // {12, 7, 7e, 0}

  int fd = open("/sys/class/amhdmitx/amhdmitx0/edid", O_RDONLY);
  if (fd >= 0)
  {
    char valstr[1024] = {0};

    read(fd, valstr, sizeof(valstr) - 1);
    valstr[strlen(valstr)] = '\0';
    close(fd);

    QStringList probe_str = QString::fromStdString(valstr).split("\n");

    for(int i=0; i < probe_str.size(); i++)
    {
      if (probe_str.at(i).contains("Audio"))
      {
        for (int j = i + 1; j < probe_str.size(); ++j)
        {
          if(probe_str.at(j).contains("{1,"))
            LOG_INFO(LOG_TAG, " PCM found {1,\n");
          else if (probe_str.at(j).contains("{2,"))
            LOG_INFO(LOG_TAG, " AC3 found {2,\n");
          else if (probe_str.at(j).contains("{3,"))
            LOG_INFO(LOG_TAG, " MPEG1 found {3,\n");
          else if (probe_str.at(j).contains("{3,"))
            LOG_INFO(LOG_TAG, " MP3 found {4,\n");
          else if (probe_str.at(j).contains("{5,"))
            LOG_INFO(LOG_TAG, " MPEG2 found {5,\n");
          else if (probe_str.at(j).contains("{6,"))
            LOG_INFO(LOG_TAG, " AAC found {6,\n");
          else if (probe_str.at(j).contains("{7,"))
            LOG_INFO(LOG_TAG, " DTS found {7,\n");
          else if (probe_str.at(j).contains("{8,"))
            LOG_INFO(LOG_TAG, " ATRAC found {8,\n");
          else if (probe_str.at(j).contains("{9,"))
            LOG_INFO(LOG_TAG, " One_Bit_Audio found {9,\n");
          else if (probe_str.at(j).contains("{10,"))
            LOG_INFO(LOG_TAG, " Dolby found {10,\n");
          else if (probe_str.at(j).contains("{11,"))
            LOG_INFO(LOG_TAG, " DTS_HD found {11,\n");
          else if (probe_str.at(j).contains("{12,"))
            LOG_INFO(LOG_TAG, " MAT found {12,\n");
          else if (probe_str.at(j).contains("{13,"))
            LOG_INFO(LOG_TAG, " ATRAC found {13,\n");
          else if (probe_str.at(j).contains("{14,"))
            LOG_INFO(LOG_TAG, " WMA found {14,\n");
          else
            break;
        }
        break;
      }
    }
  }
}

/**
 * @brief aml_get_usid
 * @param val
 * @return
 */
bool aml_get_usid(char * val)
{
    int fd0;
    char usid[16];
    bool status = false;
    memset(usid, 0, sizeof(usid));

    if ((fd0 = open("/dev/efuse", O_RDONLY)) >= 0)
    {
        struct efusekey_info efuseinfo;
        memset(efuseinfo.keyname, 0, sizeof(efuseinfo.keyname));
        memcpy(efuseinfo.keyname, "usid", strlen("usid"));

        if (ioctl(fd0, EFUSE_INFO_GET, &efuseinfo) == 0)
        {
            LOG_INFO(LOG_TAG, " efuseinfo.offset: %d", efuseinfo.offset);
            LOG_INFO(LOG_TAG, " efuseinfo.size: %d", efuseinfo.size);

            if(lseek(fd0, efuseinfo.offset, SEEK_SET) == efuseinfo.offset)
            {
                if(read(fd0, usid, efuseinfo.size) > 0)
                {
                    DUMP_HEX(LC_LOG_INFO, (uint8_t*)usid, efuseinfo.size);
                    status = true;
                    memcpy(val, usid, sizeof(usid));
                }
            }
        }
        close(fd0);
    }
    return status;
}

/**
 * @brief aml_get_btl_uuid
 * @param val
 * @return
 */
bool aml_get_btl_uuid(char * filename, char * val)
{
    FILE *fd;
    unsigned char *buf_in;
    size_t fd_size, result;
    char uuid[16];
    memset(uuid, 0, sizeof(uuid));

    fd = fopen(filename, "r");
    if (!fd)
    {
        LOG_ERROR(LOG_TAG, "Can't open input file !");
        return false;
    }

    fseek (fd, 0, SEEK_END);
    fd_size = ftell(fd);
    if (fd_size < 0X80 + 0x10)
    {
        LOG_ERROR(LOG_TAG, "Input file too small to read header!");
        fclose(fd);
        return false;
    }

    buf_in = (unsigned char*) malloc(fd_size);
    if(!buf_in)
    {
        LOG_ERROR(LOG_TAG, "Cannot allocate input buffer !");
        fclose(fd);
        return false;
    }

    fseek (fd, 0, SEEK_SET);
    result = fread(buf_in, 1, fd_size, fd);
    if (result != fd_size)
    {
        LOG_ERROR(LOG_TAG, "Cannot read entire file !\n");
        free(buf_in);
        fclose(fd);
        return false;
    }

    memcpy(val, (buf_in + 0X80), 16U);
    LOG_INFO(LOG_TAG, "UUID val: ");
    DUMP_HEX(LC_LOG_INFO, (uint8_t*)val, 16);

    free(buf_in);
    fclose(fd);
    return true;
}

