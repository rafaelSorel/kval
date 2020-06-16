#ifndef KVALAMLDISPLAYENGINE_H
#define KVALAMLDISPLAYENGINE_H
#include <QMap>
#include <QString>
#include <QObject>
#include <QThread>

#include "KvalDisplayManager.h"

#define D3DPRESENTFLAG_INTERLACED   1
#define D3DPRESENTFLAG_WIDESCREEN   2
#define D3DPRESENTFLAG_PROGRESSIVE  4
#define D3DPRESENTFLAG_MODE3DSBS    8
#define D3DPRESENTFLAG_MODE3DTB    16

class AmlDisplayEngine : public KvalDisplayPlatform::DisplayEngineIf
{
public:
    AmlDisplayEngine() = default;
    virtual ~AmlDisplayEngine() = default;

    virtual QString getCurrentResolution() final;
    virtual bool setResolution(const QString&) final;
    virtual QStringList getAvailableRes() final;

private:
    enum AML_DISPLAY_AXIS_PARAM
    {
      AML_DISPLAY_AXIS_PARAM_X = 0,
      AML_DISPLAY_AXIS_PARAM_Y,
      AML_DISPLAY_AXIS_PARAM_WIDTH,
      AML_DISPLAY_AXIS_PARAM_HEIGHT
    };

    bool setNewResolutionMode(const ResInfo&, const ResInfo&);
    bool modeToResolution(const char *mode, ResInfo *res);
    bool getNativeResolution(ResInfo *res);
    bool setNativeResolution(const ResInfo &res, std::string framebuffer_name);
    bool probeResolutions(std::vector<ResInfo> &resolutions);
    bool getPreferredResolution(ResInfo *res);
    bool setDisplayResolution(const ResInfo &res, std::string framebuffer_name);
    void handleScale(const ResInfo &res);
    void enableFreeScale(const ResInfo &res);
    void setupVideoScaling(const char *mode);
    void handleDisplayStereoMode(const int stereo_mode);
    void disableFreeScale();
    void setFramebufferResolution(const ResInfo &res, std::string framebuffer_name);
    void setFramebufferResolution(int width, int height, std::string framebuffer_name);
    bool IsHdmiConnected();
    bool hasFracRatePolicy();
    int axisValue(AML_DISPLAY_AXIS_PARAM);
};

#endif // KVALAMLDISPLAYENGINE_H
