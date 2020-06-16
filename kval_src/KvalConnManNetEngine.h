#ifndef KVAL_CONNMAN_NET_ENGINE_H
#define KVAL_CONNMAN_NET_ENGINE_H

#include <QMap>
#include <QString>
#include <QObject>
#include <QThread>

#include <networkmanager.h>
#include <networktechnology.h>
#include <networkservice.h>
#include <useragent.h>
#include "KvalNetworkManager.h"

constexpr char InvalidArguments[] = "Invalid arguments";
constexpr char PermissionDenied[] = "Permission denied";
constexpr char PassphraseRequired[] = "Passphrase required";
constexpr char NotRegistered[] = "Not registered";
constexpr char NotUnique[] = "Not unique";
constexpr char NotSupported[] = "Not supported";
constexpr char NotImplemented[] = "Not implemented";
constexpr char NotFound[] = "Not found";
constexpr char NoCarrier[] = "No carrier";
constexpr char InProgress[] = "In progress";
constexpr char AlreadyExists[] = "Already exists";
constexpr char AlreadyEnabled[] = "Already enabled";
constexpr char AlreadyDisabled[] = "Already disabled";
constexpr char AlreadyConnected[] = "Already connected";
constexpr char NotConnected[] = "Not connected";
constexpr char OperationAborted[] = "Operation aborted";
constexpr char OperationTimeout[] = "Operation timeout";
constexpr char InvalidProperty[] = "Invalid property";
constexpr char InvalidService[] = "Invalid service";

//User Agent Errors
constexpr char OutOfRange[] = "out-of-range";
constexpr char PingMissing[] = "pin-missing";
constexpr char DhcpFailed[] = "dhcp-failed";
constexpr char ConnectFailed[] = "connect-failed";
constexpr char LoginFailed[] = "login-failed";
constexpr char AuthFailed[] = "auth-failed";
constexpr char InvalidKey[] = "invalid-key";

/**
 * @brief The ConnManNetworkEngine class
 */
class ConnManNetworkEngine: public KvalNetworkPlatform::NetworkEngineIf
{
    Q_OBJECT

public:
    enum SupportedServices {
        Ethernet,
        Wifi
    };
    Q_ENUM(SupportedServices)

    enum WifiCnxStatus {
        CNX_OK,
        CNX_NOK,
        CNX_PASS,
        CNX_USER_PASS,
        CNX_TIMEOUT
    };

    ConnManNetworkEngine();
    ~ConnManNetworkEngine();

    WifiCnxStatus connectToSsid(QString, QString, bool);

    QString extractCurrentSsid();
    void refresh_services();

    virtual const InterfaceProperties getActiveInterface() override;
    virtual const IpConfig getIfIpConfig(kvalInterfaceType, kvalNetProtocol) override;

public Q_SLOTS:
    virtual void Start() override;
    virtual void apConnectRequest(const QString&) override;
    virtual void apConnectReply(const QString&) override;
    virtual void apConnectAbort() override;
    virtual void onScanWlan(void) override;
    virtual bool setInterface(kvalInterfaceType) final override;

private Q_SLOTS:
    void onServiceChanged();
    void onConnectedChanged(bool);
    void onServiceConnectionStarted();
    void onConnectRequestFailed(const QString&);
    void onEthernetConnectedChanged(bool status);
    void onEthernetConnectionStarted();
    void onEthernetConnectRequestFailed(QString error);
    void agentRequestedUserInput(QString, QVariantMap);
    void agentReportedError(QString path, QString error);
    void onRequestConnect(unsigned int service);
    void onCheckWifiConnection();
    void onCheckEthernetConnection();

private:
    bool disconnect_services(SupportedServices);
    void connect_services(SupportedServices);
    void set_connected_service(SupportedServices service);
    void disable_services_auto_detection();
    void displayActiveNetTech();
    void createWifiConnectionTimer();
    void createEthernetConnectionTimer();
    inline void _abortWifiConnectionTimer();
    inline void _abortEthernetConnectionTimer();

private:
    QScopedPointer<NetworkManager> m_netManager;
    QPointer<NetworkService> m_currentWifiService;
    QPointer<NetworkService> m_currentEthernetService;
    QScopedPointer<UserAgent> m_agent;
    QString m_current_pass;
    bool m_has_ethernet;
    bool m_ethernet_connected;
    bool m_has_wifi;
    bool m_wifi_connected;
    QString m_error_str;
    QWaitCondition m_waitPassCmd;
    QMutex m_waitPassMtx;
    QMutex m_servicesCheckMtx;
    QScopedPointer<QTimer> m_wifiConnectionTimer{nullptr};
    QScopedPointer<QTimer> m_ethernetConnectionTimer{nullptr};
};

#endif // KVAL_CONNMAN_NET_ENGINE_H
