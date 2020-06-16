#ifndef KVAL_DESKTOP_NET_ENGINE_H
#define KVAL_DESKTOP_NET_ENGINE_H

#include <QMap>
#include <QString>
#include <QObject>
#include <QThread>
#include "KvalNetworkManager.h"


/**
 * @brief The DesktopNetworkEngine class
 */
class DesktopNetworkEngine: public KvalNetworkPlatform::NetworkEngineIf
{
    Q_OBJECT

public:

    DesktopNetworkEngine();
    virtual ~DesktopNetworkEngine();
    virtual const InterfaceProperties getActiveInterface() override;
    virtual const IpConfig getIfIpConfig( kvalInterfaceType, kvalNetProtocol) override;

public Q_SLOTS:
    virtual void Start() final override;
    virtual bool setInterface(kvalInterfaceType) final override;
    virtual void onScanWlan() final override;
    virtual void apConnectRequest(const QString&) final override;
    virtual void apConnectReply(const QString&) final override;
    virtual void apConnectAbort() final override {};

private:
    const QStringList extractNameServers();
};

#endif // KVAL_DESKTOP_NET_ENGINE_H
