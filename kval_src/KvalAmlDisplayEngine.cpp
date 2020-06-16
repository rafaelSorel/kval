#define LOG_ACTIVATED

#include <fcntl.h>
#include <sys/ioctl.h>
#include <cmath>
#include <fstream>
#include "linux/fb.h"

#include <QFile>

#include "player/amlplayer_lib/AMLplatform.h"
#include "KvalMiscUtils.h"
#include "KvalAmlDisplayEngine.h"

#define LOG_SRC SETMANAGER
#include "KvalLogging.h"


/**
 * @brief AmlDisplayEngine::setResolution
 * @param resolution
 */
bool AmlDisplayEngine::setResolution(const QString& resolution)
{
    std::vector<ResInfo> resolutions{};

    //Extract all display screen supported resolutions
    if (!probeResolutions(resolutions) || resolutions.empty())
    {
        LOG_INFO(LOG_TAG, "ProbeResolutions failed.");
    }

    ResInfo nativeResolution;
    getNativeResolution(&nativeResolution);
    for(const auto& res: resolutions)
    {
        if( resolution.toStdString() == res.strMode &&
            resolution.toStdString() != nativeResolution.strMode)
        {
            LOG_INFO(LOG_TAG, "Set usermode resolution ...");
            return setNewResolutionMode(res, nativeResolution);
        }
    }
    return false;
}

/**
 * @brief AmlDisplayEngine::getCurrentResolution
 * @return
 */
QString AmlDisplayEngine::getCurrentResolution()
{
    ResInfo nativeResolution;
    getNativeResolution(&nativeResolution);

    return nativeResolution.strMode.c_str();
}
/**
 * @brief CM_DisplayManager::setNewResolutionMode
 * @param strMode lval ref
 * @return
 */
bool AmlDisplayEngine::setNewResolutionMode(
        const ResInfo& userRes,
        const ResInfo& nativeRes)
{

    if(!setNativeResolution(userRes, "fb0"))
    {
        LOG_ERROR(LOG_TAG,
                 "Failed to set new resolution! switch back to old resolution...");
        setNativeResolution(nativeRes, "fb0");
        return false;
    }
    else
    {
        LOG_INFO(LOG_TAG, "Success switching to new res");
        return true;
    }
}

/**
 * @brief AmlDisplayEngine::getAvailableRes
 * @return
 */
QStringList AmlDisplayEngine::getAvailableRes()
{
    QStringList availableRes;
    std::vector<ResInfo> resolutions;
    if (!probeResolutions(resolutions) || resolutions.empty())
    {
        LOG_INFO(LOG_TAG, "ProbeResolutions failed.");
        throw "Fail fetching supported resolution";
    }

    std::for_each(
        std::begin(resolutions),
        std::end(resolutions),
        [&availableRes](const ResInfo& res){availableRes.append(res.strMode.c_str());});

    return availableRes;
}

/**
 * @brief aml_mode_to_resolution
 * @param mode
 * @param res
 * @return
 */
bool AmlDisplayEngine::modeToResolution(const char *mode, ResInfo *res)
{
    if (!res)
        return false;

    res->iWidth = 0;
    res->iHeight= 0;

    if(!mode)
        return false;

    std::string fromMode = mode;
    MU_MiscUtils::Trim(fromMode);

    // strips, for example, 720p* to 720p
    // the * indicate the 'native' mode of the display
    if (QString::fromStdString(fromMode).endsWith("*"))
    {
        fromMode.erase(fromMode.size() - 1);
    }

    if (!QString::compare(fromMode.c_str(), "panel", Qt::CaseInsensitive))
    {
        res->iWidth = axisValue(AML_DISPLAY_AXIS_PARAM_WIDTH);
        res->iHeight= axisValue(AML_DISPLAY_AXIS_PARAM_HEIGHT);
        res->iScreenWidth = axisValue(AML_DISPLAY_AXIS_PARAM_WIDTH);
        res->iScreenHeight= axisValue(AML_DISPLAY_AXIS_PARAM_HEIGHT);
        res->fRefreshRate = 60;
        res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    }
    else if (!QString::compare(fromMode.c_str(), "4k2ksmpte", Qt::CaseInsensitive) ||
             !QString::compare(fromMode.c_str(), "smpte24hz", Qt::CaseInsensitive))
    {
        res->iWidth = 1920;
        res->iHeight= 1080;
        res->iScreenWidth = 4096;
        res->iScreenHeight= 2160;
        res->fRefreshRate = 24;
        res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    }
    else
    {
        int width = 0, height = 0, rrate = 60;
        char smode = 'p';

        if (sscanf(fromMode.c_str(), "%dx%dp%dhz", &width, &height, &rrate) == 3)
        {
            smode = 'p';
        }
        else if (sscanf(fromMode.c_str(), "%d%[ip]%dhz", &height, &smode, &rrate) >= 2)
        {
            switch (height)
            {
            case 480:
            case 576:
                width = 720;
                break;
            case 720:
                width = 1280;
                break;
            case 1080:
                width = 1920;
                break;
            case 2160:
                width = 3840;
                break;
            }
        }
        else if (sscanf(fromMode.c_str(), "%dcvbs", &height) == 1)
        {
            width = 720;
            smode = 'i';
            rrate = (height == 576) ? 50 : 60;
        }
        else if (sscanf(fromMode.c_str(), "4k2k%d", &rrate) == 1)
        {
            width = 3840;
            height = 2160;
            smode = 'p';
        }
        else
        {
            return false;
        }

        res->iWidth = (width < 3840) ? width : 1920;
        res->iHeight= (height < 2160) ? height : 1080;
        res->iScreenWidth = width;
        res->iScreenHeight = height;
        res->dwFlags = (smode == 'p') ? D3DPRESENTFLAG_PROGRESSIVE : D3DPRESENTFLAG_INTERLACED;

        switch (rrate)
        {
            case 23:
            case 29:
            case 59:
                res->fRefreshRate = (float)((rrate + 1)/1.001);
            break;
            default:
                res->fRefreshRate = (float)rrate;
            break;
        }
    }

    res->fPixelRatio   = 1.0f;
    res->strId = fromMode;
    QString qStrMode;
    res->strMode =qStrMode.sprintf("%dx%d%s@%.2f",
                            res->iScreenWidth,
                            res->iScreenHeight,
                            (res->dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "p",
                            res->fRefreshRate).toStdString();

    //Validate only res above 1080p
    return res->iWidth >= 1920 && res->iHeight>= 1080 ;
}

/**
 * @brief AmlDisplayEngine::getNativeResolution
 * @param res
 * @return
 */
bool AmlDisplayEngine::getNativeResolution(ResInfo *res)
{
  std::string mode;
  MU_MiscUtils::GetString("/sys/class/display/mode", mode);
  bool result = modeToResolution(mode.c_str(), res);

  if (hasFracRatePolicy())
  {
    int fractional_rate;
    MU_MiscUtils::GetInt("/sys/class/amhdmitx/amhdmitx0/frac_rate_policy", fractional_rate);
    if (fractional_rate == 1)
      res->fRefreshRate /= 1.001;
  }

  return result;
}

/**
 * @brief AmlDisplayEngine::handleScale
 * @param res
 */
void AmlDisplayEngine::handleScale(const ResInfo &res)
{
    if (res.iScreenWidth > res.iWidth && res.iScreenHeight > res.iHeight)
        enableFreeScale(res);
    else
        disableFreeScale();
}

/**
 * @brief AmlDisplayEngine::setNativeResolution
 * @param res
 * @param framebuffer_name
 * @return
 */
bool AmlDisplayEngine::setNativeResolution(const ResInfo &res, std::string framebuffer_name)
{
  bool result = false;

  result = setDisplayResolution(res, framebuffer_name);

  handleScale(res);

  return result;
}

/**
 * @brief AmlDisplayEngine::probeResolutions
 * @param resolutions
 * @return
 */
bool AmlDisplayEngine::probeResolutions(std::vector<ResInfo> &resolutions)
{
    std::string valstr, vesastr;

    if (MU_MiscUtils::GetString("/sys/class/amhdmitx/amhdmitx0/disp_cap", valstr) < 0)
      return false;

    if (MU_MiscUtils::GetString("/sys/class/amhdmitx/amhdmitx0/vesa_cap", vesastr) == 0)
      valstr += "\n" + vesastr;

    QStringList probe_str = QString::fromStdString(valstr).split("\n");

    resolutions.clear();
    ResInfo res;
    for(int i = 0; i < probe_str.size(); i++)
    {
        if (    ( (probe_str.at(i).startsWith("4k2k")) &&
                  (aml_support_h264_4k2k() > AML_NO_H264_4K2K)
                )
                || !(probe_str.at(i).startsWith("4k2k"))
           )
        {
            if (!modeToResolution(probe_str.at(i).toStdString().c_str(), &res))
            {
                continue;
            }
            resolutions.push_back(res);

            if (hasFracRatePolicy())
            {
                // Add fractional frame rates: 23.976, 29.97 and 59.94 Hz
                switch ((int)res.fRefreshRate)
                {
                    case 24:
                    case 30:
                    case 60:
                    res.fRefreshRate /= 1.001;
                    QString qStrMode;
                    res.strMode = qStrMode.sprintf("%dx%d%s@%.2f",
                                res.iScreenWidth,
                                res.iScreenHeight,
                                (res.dwFlags & D3DPRESENTFLAG_INTERLACED) ? "i" : "p",
                                res.fRefreshRate).toStdString();
                    resolutions.push_back(res);
                    break;
                }
            }
        }
    }
    return resolutions.size() > 0;
}

/**
 * @brief AmlDisplayEngine::getPreferredResolution
 * @param res
 * @return
 */
bool AmlDisplayEngine::getPreferredResolution(ResInfo *res)
{
    // check display/mode, it gets defaulted at boot
    if (!getNativeResolution(res))
    {
        // punt to 1080p if we get nothing
        modeToResolution("1080p", res);
    }
    return true;
}


/**
 * @brief AmlDisplayEngine::setDisplayResolution
 * @param res
 * @param framebuffer_name
 * @return
 */
bool AmlDisplayEngine::setDisplayResolution(
        const ResInfo &res, std::string framebuffer_name)
{
    std::string mode = res.strId.c_str();
    std::string cur_mode;

    MU_MiscUtils::GetString("/sys/class/display/mode", cur_mode);

    if (hasFracRatePolicy())
    {
        if (cur_mode == mode)
            MU_MiscUtils::SetString("/sys/class/display/mode", "null");

        int fractional_rate = (res.fRefreshRate == std::floor(res.fRefreshRate)) ? 0 : 1;
        MU_MiscUtils::SetInt("/sys/class/amhdmitx/amhdmitx0/frac_rate_policy", fractional_rate);
    }
    else if (cur_mode == mode)
    {
        // Don't set the same mode as current
        return true;
    }

    MU_MiscUtils::SetString("/sys/class/display/mode", mode.c_str());

    setFramebufferResolution(res, framebuffer_name);

    return true;
}

/**
 * @brief AmlDisplayEngine::setupVideoScaling
 * @param mode
 */
void AmlDisplayEngine::setupVideoScaling(const char *mode)
{
    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/blank",      1);
    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/free_scale", 0);
    MU_MiscUtils::SetInt("/sys/class/graphics/fb1/free_scale", 0);
    MU_MiscUtils::SetInt("/sys/class/ppmgr/ppscaler",          0);

    if (strstr(mode, "1080"))
    {
        MU_MiscUtils::SetString("/sys/class/graphics/fb0/request2XScale", "8");
        MU_MiscUtils::SetString("/sys/class/graphics/fb1/scale_axis",     "1280 720 1920 1080");
        MU_MiscUtils::SetString("/sys/class/graphics/fb1/scale",          "0x10001");
    }
    else
    {
        MU_MiscUtils::SetString("/sys/class/graphics/fb0/request2XScale", "16 1280 720");
    }

    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/blank", 0);
}

/**
 * @brief AmlDisplayEngine::enableFreeScale
 * @param res
 */
void AmlDisplayEngine::enableFreeScale(const ResInfo &res)
{
    char fsaxis_str[256] = {0};
    sprintf(fsaxis_str, "0 0 %d %d", res.iWidth-1, res.iHeight-1);
    char waxis_str[256] = {0};
    sprintf(waxis_str, "0 0 %d %d", res.iScreenWidth-1, res.iScreenHeight-1);

    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/free_scale", 0);
    MU_MiscUtils::SetString("/sys/class/graphics/fb0/free_scale_axis", fsaxis_str);
    MU_MiscUtils::SetString("/sys/class/graphics/fb0/window_axis", waxis_str);
    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/scale_width", res.iWidth);
    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/scale_height", res.iHeight);
    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/free_scale", 0x10001);
}

/**
 * @brief disableFreeScale
 */
void disableFreeScale()
{
    // turn off frame buffer freescale
    MU_MiscUtils::SetInt("/sys/class/graphics/fb0/free_scale", 0);
    MU_MiscUtils::SetInt("/sys/class/graphics/fb1/free_scale", 0);
}

/**
 * @brief AmlDisplayEngine::setFramebufferResolution
 * @param res
 * @param framebuffer_name
 */
void AmlDisplayEngine::setFramebufferResolution(const ResInfo &res, std::string framebuffer_name)
{
    setFramebufferResolution(res.iWidth, res.iHeight, framebuffer_name);
}

/**
 * @brief AmlDisplayEngine::setFramebufferResolution
 * @param width
 * @param height
 * @param framebuffer_name
 */
void AmlDisplayEngine::setFramebufferResolution(int width, int height, std::string framebuffer_name)
{
    int fd0;
    std::string framebuffer = "/dev/" + framebuffer_name;

    if ((fd0 = open(framebuffer.c_str(), O_RDWR)) >= 0)
    {
        struct fb_var_screeninfo vinfo;
        if (ioctl(fd0, FBIOGET_VSCREENINFO, &vinfo) == 0)
        {
            vinfo.xres = width;
            vinfo.yres = height;
            vinfo.xres_virtual = 1920;
            vinfo.yres_virtual = 2160;
            vinfo.bits_per_pixel = 32;
            vinfo.activate = FB_ACTIVATE_ALL;
            ioctl(fd0, FBIOPUT_VSCREENINFO, &vinfo);
        }
        close(fd0);
    }
}

/**
 * @brief AmlDisplayEngine::isHdmiConnected
 * @return
 */
bool AmlDisplayEngine::IsHdmiConnected()
{
  int hpd_state;
  MU_MiscUtils::GetInt("/sys/class/amhdmitx/amhdmitx0/hpd_state", hpd_state);
  if (hpd_state == 2)
  {
    return 1;
  }

  return 0;
}

/**
 * @brief AmlDisplayEngine::hasFracRatePolicy
 * @return
 */
bool AmlDisplayEngine::hasFracRatePolicy()
{
  static int has_frac_rate_policy = -1;

  if (has_frac_rate_policy == -1)
      has_frac_rate_policy = MU_MiscUtils::Has("/sys/class/amhdmitx/amhdmitx0/frac_rate_policy");

  return (has_frac_rate_policy == 1);
}

/**
 * @brief AmlDisplayEngine::axisValue
 * @param param
 * @return
 */
int AmlDisplayEngine::axisValue(AML_DISPLAY_AXIS_PARAM param)
{
  std::string axis;
  int value[8];

  MU_MiscUtils::GetString("/sys/class/display/axis", axis);
  sscanf(axis.c_str(),
         "%d %d %d %d %d %d %d %d",
         &value[0],
          &value[1],
          &value[2],
          &value[3],
          &value[4],
          &value[5],
          &value[6],
          &value[7]);

  return value[param];
}
