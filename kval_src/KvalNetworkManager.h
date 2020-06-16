#ifndef KVALNETWORKMANAGER_H
#define KVALNETWORKMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QScopedPointer>
#include <QPointer>
#include <QSettings>
#include <QNetworkInterface>

#include "KvalSettingsKeys.h"
#include "KvalThreadUtils.h"

namespace KvalNetworkPlatform {

class NetworkManager;
/**
 * @brief The NetworkEngineIftract class
 */
class NetworkEngineIf : public QObject
{
    Q_OBJECT

    friend NetworkManager;

protected:
    using kvalInterfaceType = QNetworkInterface::InterfaceType;
    using kvalNetProtocol = QAbstractSocket::NetworkLayerProtocol;
    enum class kvalNetMethod { Dhcp, Manual, Count };
    enum class kvalConnStatus { Connected, Disconnected, Count };

    struct InterfaceProperties {
        kvalInterfaceType type{kvalInterfaceType::Unknown}; // Ethernet, Wifi...
        kvalConnStatus status{kvalConnStatus::Disconnected}; // connected, diconnected
        QString name{}; // access point for wifi
        QString hwaddr{}; // mac address
        QStringList nameservers{}; // Dns servers configuration
    };

    struct IpConfig {
        kvalNetProtocol protocol{kvalNetProtocol::UnknownNetworkLayerProtocol};
        kvalNetMethod method{kvalNetMethod::Dhcp};
        QString address{"0.0.0.0"};
        QString netmask{"0.0.0.0"};
        QString gateway{"0.0.0.0"};
    };

    struct WlanAccessPoint {
        WlanAccessPoint() = default;
        explicit WlanAccessPoint(const QString essid, bool secured,
                        uint signal, const QString encryption):
            essid(essid), secured(secured),
            signal(signal), encryption(encryption) {}
        WlanAccessPoint(const WlanAccessPoint& rhs):
            essid(rhs.essid), secured(rhs.secured),
            signal(rhs.signal), encryption(rhs.encryption) {}
        WlanAccessPoint(WlanAccessPoint&& rhs):
            essid(rhs.essid), secured(rhs.secured),
            signal(rhs.signal), encryption(rhs.encryption) {}
        QString essid;
        bool secured;
        uint signal;
        QString encryption;
    };

public:
    NetworkEngineIf() = default;
    virtual ~NetworkEngineIf() = default;

    virtual const InterfaceProperties getActiveInterface() = 0;
    virtual const IpConfig getIfIpConfig(kvalInterfaceType, kvalNetProtocol) = 0;

Q_SIGNALS:
    void notifyAlert(const QString&, const QString&);
    void notifyProgress(const QString&, qreal);
    void notifyWlanAps(const QVector<WlanAccessPoint>&);
    void apRequestPassPhrase();

public Q_SLOTS:
    virtual void Start() = 0;
    virtual bool setInterface(kvalInterfaceType) = 0;
    virtual void onScanWlan() = 0;
    virtual void apConnectRequest(const QString&) = 0;
    virtual void apConnectReply(const QString&) = 0 ;
    virtual void apConnectAbort() = 0 ;
};

/**
 * @brief The NetworkManager class
 */
class NetworkManager: public QObject
{
    Q_OBJECT
public:
    NetworkManager();
    virtual ~NetworkManager();
    void updateNetworkSettings();

    using InterfaceProperties = NetworkEngineIf::InterfaceProperties;
    using IpConfig = NetworkEngineIf::IpConfig;
    using kvalNetProtocol = NetworkEngineIf::kvalNetProtocol;
    using kvalInterfaceType = NetworkEngineIf::kvalInterfaceType;
    using kvalConnStatus = NetworkEngineIf::kvalConnStatus;
    using WlanAccessPoint = NetworkEngineIf::WlanAccessPoint;
    Q_ENUM(kvalInterfaceType)

Q_SIGNALS:
    void netValueChanged(const QString&, const QString&, const QVariant&);
    void dynMenuReply(const QString&, const QVariantList&);
    void notifyAlert(const QString&, const QString&);
    void notifyProgress(const QString&, qreal);
    void textEntryRequest(const QString&, bool);

public Q_SLOTS:
    void onUserActionReq(const QString&, const QString&, const QVariant&);
    void onUserMenuReq(const QString&);
    void onUserReply(const QString&,const QString&, const QVariant&);
    void onWlanApsAvailable(const QVector<WlanAccessPoint>&);
    void onRequestPassPhrase();

private:
    void registerNetworkEngines(void);

    void refreshIfDetail();
    void updateConnectedInterface(const QString&, const InterfaceProperties&);
    void updateNetStatus(const QString&, const InterfaceProperties&);
    void updateWifiAp(const QString&, const InterfaceProperties&);
    void updateMacAddress(const QString&, const InterfaceProperties&);
    void updateIpv4(const QString&, const InterfaceProperties&);
    void updateIpv6(const QString&, const InterfaceProperties&);
    void updateSubnetMask(const QString&, const InterfaceProperties&);
    void updateGatewayAddr(const QString&, const InterfaceProperties&);
    void updatePrimaryDns(const QString&, const InterfaceProperties&);
    void updateSecondDns(const QString&, const InterfaceProperties&);
    void notifyValueChanged(const QString&,const QString&, const QString&);



private:
    KvalThread *m_networkEngineThread;
    QScopedPointer<NetworkEngineIf> m_networkEngine;
};

} //namespace KvalNetworkPlatform
#endif // KVALNETWORKMANAGER_H
