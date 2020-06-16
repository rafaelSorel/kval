#ifndef KVALDISPLAYMANAGER_H
#define KVALDISPLAYMANAGER_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QString>
#include <QStringList>
#include <QScopedPointer>
#include <QPointer>
#include <QSettings>
#include <QNetworkInterface>

#include "KvalSettingsKeys.h"
#include "KvalThreadUtils.h"

namespace KvalDisplayPlatform {

class DisplayManager;

/**
 * @brief The ResolutionInfo struct
 */
struct ResolutionInfo
{
    ResolutionInfo(
            int width = 1280,
            int height = 720,
            float aspect = 0,
            const std::string &mode = "");
    float DisplayRatio() const;
    void printRes(const ResolutionInfo& res);
    int iWidth{};
    int iHeight{};
    int iScreenWidth{};
    int iScreenHeight{};
    uint32_t dwFlags{};
    float fPixelRatio{};
    float fRefreshRate{};
    std::string strMode{};
    std::string strOutput{};
    std::string strId{};
};


/**
 * @brief The NetworkEngineIftract class
 */
class DisplayEngineIf : public QObject
{
    Q_OBJECT

    friend DisplayManager;

public:
    DisplayEngineIf() = default;
    virtual ~DisplayEngineIf() = default;

    virtual QString getCurrentResolution() = 0;
    virtual bool setResolution(const QString&) = 0;
    virtual QStringList getAvailableRes() = 0;

Q_SIGNALS:
    void msgNotify(const QString&, const QString&);

protected:
    using ResInfo = ResolutionInfo;
};

/**
 * @brief The KvalDisplayManager class
 */
class DisplayManager : public QObject
{
    Q_OBJECT
public:
    DisplayManager();
    virtual ~DisplayManager() = default;
    bool Start();

    QStringList resolutions();
    bool setResolution(const QString&);
    QString getCurrentResolution();
    bool updateScreenRes(const QString&);

Q_SIGNALS:
    void displayValueChanged(const QString&, const QString&, const QVariant&);
    void displayMsg(const QString&, const QString&);
    void yesNoDiag(const QString&,const QString&,const QString&,const QString&);
    void yesNoDiagUpdate(const QString&,const QString&);
    void yesNoDiagClose();
    void stopScreenResTimer();
    void stoptimer();
    void starttimer(int);

public Q_SLOTS:
    void onUserReply(const QString&, const QString&, const QVariant&);

private Q_SLOTS:
    void onTimeout();

private:
    void registerDisplayEngines();

private:
    QScopedPointer<DisplayEngineIf> m_displayEngine;
    QTimer m_timer;
    QString m_currentRes{};
    QString m_newRes{};
    unsigned int m_countdown;
};

} //namespace KvalDisplayPlatform
#endif // KVALDISPLAYMANAGER_H
