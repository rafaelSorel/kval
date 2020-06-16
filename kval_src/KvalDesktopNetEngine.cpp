#define LOG_ACTIVATED

#include "KvalDesktopNetEngine.h"

#if defined (Q_OS_LINUX)
#include <resolv.h>
#include <arpa/inet.h>
#endif

#define LOG_SRC NETUTILS
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Macro definition
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Type definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Class interface, front end
//----------------------------------------------------------------------------

/**
 * @brief DesktopNetworkEngine::DesktopNetworkEngine
 */
DesktopNetworkEngine::DesktopNetworkEngine()
{
    LOG_INFO(LOG_TAG, "Instantiate DesktopNetworkEngine");
}

/**
 * @brief DesktopNetworkEngine::~DesktopNetworkEngine
 */
DesktopNetworkEngine::~DesktopNetworkEngine()
{
    LOG_INFO(LOG_TAG, "~Destroy DesktopNetworkEngine");
}

/**
 * @brief DesktopNetworkEngine::setInterface
 * @param ifType
 * @return
 */
bool DesktopNetworkEngine::setInterface(kvalInterfaceType ifType)
{
    Q_UNUSED(ifType)
    Q_EMIT notifyAlert("warning",
                     tr("Could not change interface on this environnement."));
    return false;
}

/**
 * @brief DesktopNetworkEngine::apConnectRequest
 * @param essid
 */
void DesktopNetworkEngine::apConnectRequest(const QString& essid)
{
    Q_UNUSED(essid)
    qInfo() << "============ apConnectRequest: " << essid;
    Q_EMIT notifyAlert("warning",
                     tr("Could not connect on this environnement."));
//    QThread::sleep(4);
//    Q_EMIT apRequestPassPhrase();
}

/**
 * @brief DesktopNetworkEngine::apConnectReply
 * @param pass
 */
void DesktopNetworkEngine::apConnectReply(const QString& pass)
{
    Q_UNUSED(pass)
    Q_EMIT notifyAlert("warning",
                     tr("Could not connect on this environnement."));
//    qInfo() << "============ Got the password: " << pass;
//    QThread::sleep(4);
//    Q_EMIT notifyAlert("success", tr("Connected to xxxxx"));
}


/**
 * @brief DesktopNetworkEngine::onScanWlan
 * @return
 */
void DesktopNetworkEngine::onScanWlan(void)
{
    QVector<WlanAccessPoint> aps{};
    QThread::sleep(4);
    Q_EMIT notifyAlert("warning",
                     tr("Could not perform Wlan scan on this environnement."));
    Q_EMIT notifyWlanAps(aps);

//    auto populateaps = [&](int& val){
//        Q_UNUSED(val)
//        aps.append(WlanAccessPoint{"mywlan", true, 80, "wpa2"});
//        aps.append(WlanAccessPoint{"This is home", false, 20, ""});
//        aps.append(WlanAccessPoint{"élaboration intact", true, 50, "wep"});
//    };

//    QVector<int> services{3};
//    std::for_each(std::begin(services),std::end(services), populateaps);

//    Q_EMIT notifyWlanAps(aps);
}
/**
 * @brief ConnManNetworkEngine::getActiveInterface
 * @return
 */
const DesktopNetworkEngine::InterfaceProperties DesktopNetworkEngine::getActiveInterface()
{
    InterfaceProperties iF{};

    Q_FOREACH(const QNetworkInterface& qIf, QNetworkInterface::allInterfaces())
    {
        if (    qIf.flags().testFlag(QNetworkInterface::IsUp)
             && qIf.flags().testFlag(QNetworkInterface::IsRunning)
             && !qIf.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            iF.type = static_cast<kvalInterfaceType>(qIf.type());
            iF.status = kvalConnStatus::Connected;
            iF.name = qIf.name();
            iF.hwaddr = qIf.hardwareAddress();
            LOG_INFO(LOG_TAG, "iF.hwaddr: %s", qPrintable(iF.hwaddr));
            iF.nameservers = extractNameServers();
            return iF;
        }
    }
    return iF;
};

/**
 * @brief DesktopNetworkEngine::extractNameServers
 * @return
 */
const QStringList DesktopNetworkEngine::extractNameServers()
{
    QStringList _nameServers{};
#if defined (Q_OS_LINUX)
    res_init();
    if(_res.nscount){
        for(auto i=0; i < _res.nscount; ++i ){
            int ipAddr = _res.nsaddr_list[i].sin_addr.s_addr;
            char str[INET_ADDRSTRLEN];
            inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN);
            _nameServers.append(str);
        }
    }
#endif // Q_OS_LINUX
    return _nameServers;
}
/**
 * @brief ConnManNetworkEngine::getIfIpConfig
 * @param interface
 * @param protocol
 * @return
 */
const DesktopNetworkEngine::IpConfig DesktopNetworkEngine::getIfIpConfig(
        kvalInterfaceType interface,
        kvalNetProtocol protocol)
{
    IpConfig cfg{};
    Q_FOREACH(const QNetworkInterface& qIf, QNetworkInterface::allInterfaces())
    {
        if (qIf.type() == interface)
        {
            Q_FOREACH(const QNetworkAddressEntry& entry, qIf.addressEntries())
            {
                if(entry.ip().protocol() == protocol){
                    cfg.protocol = protocol;
                    cfg.method = kvalNetMethod::Dhcp; //@TODO: get the real value
                    cfg.address = entry.ip().toString();
                    cfg.gateway = entry.broadcast().toString();
                    cfg.netmask = entry.netmask().toString();
                    return cfg;
                }
            }
        }
    }

    return cfg;
}

/**
 * @brief DesktopNetworkEngine::Start
 */
void DesktopNetworkEngine::Start()
{

}
