#define LOG_ACTIVATED

#include <functional>
#include <QNetworkInterface>
#include "KvalNetworkManager.h"
#include "KvalConfigManager.h"

#if defined(AML_TARGET)
#include "KvalConnManNetEngine.h"
#else
#include "KvalDesktopNetEngine.h"
#endif

#define LOG_SRC SETMANAGER
#include "KvalLogging.h"

#define KVAL_NETWORK_MANAGER_CHECK_ENGINE \
    if(m_networkEngine.isNull()){ \
        LOG_ERROR(LOG_TAG, "No Active Network Engine."); \
        return; }

namespace KvalNetworkPlatform {
/**
 * @brief NetworkManager::NetworkManager
 */
NetworkManager::NetworkManager()
{
    registerNetworkEngines();
    if(m_networkEngine.isNull()){
        LOG_ERROR(LOG_TAG, "Network not handled.");
        return;
    }
    qRegisterMetaType<QVector<WlanAccessPoint>>("QVector<WlanAccessPoint>");

    m_networkEngineThread = new KvalThread(m_networkEngine.get());
    connect(m_networkEngine.get(), SIGNAL(notifyAlert(const QString&, const QString&)),
            this, SIGNAL(notifyAlert(const QString&, const QString&)),
            Qt::QueuedConnection);
    connect(m_networkEngine.get(), SIGNAL(notifyProgress(const QString&, qreal)),
            this, SIGNAL(notifyProgress(const QString&, qreal)),
            Qt::QueuedConnection);
    connect(m_networkEngine.get(), SIGNAL(notifyWlanAps(const QVector<WlanAccessPoint>&)),
            this, SLOT(onWlanApsAvailable(const QVector<WlanAccessPoint>&)),
            Qt::QueuedConnection);
    connect(m_networkEngine.get(), SIGNAL(apRequestPassPhrase()),
            this, SLOT(onRequestPassPhrase()),
            Qt::QueuedConnection);


    LOG_INFO(LOG_TAG, "start thead");
    m_networkEngineThread->start();
}

/**
 * @brief NetworkManager::~NetworkManager
 */
NetworkManager::~NetworkManager()
{
    if(m_networkEngineThread){
        LOG_INFO(LOG_TAG, "Stop Network Manager thread...");
        m_networkEngineThread->stop();
        delete m_networkEngineThread;
    }
}

/**
 * @brief NetworkManager::registerNetworkEngines
 */
void NetworkManager::registerNetworkEngines()
{
    LOG_ERROR(LOG_TAG, "registerNetworkEngines...");

#if defined (Q_OS_LINUX)
#ifdef AMLOGIC_TARGET
    m_networkEngine.reset(new ConnManNetworkEngine());
#endif //AMLOGIC_TARGET
    m_networkEngine.reset(new DesktopNetworkEngine());

#elif defined (Q_OS_OSX)
#elif defined (Q_OS_WINDOWS)
#else
#endif// Q_OS_LINUX

}

/**
 * @brief NetworkManager::updateNetworkSettings
 */
void NetworkManager::updateNetworkSettings()
{
    LOG_INFO(LOG_TAG, "updateNetworkSettings");
    refreshIfDetail();
}

/**
 * @brief NetworkManager::refreshIfDetail
 * @param netSettingsGroup
 */
void NetworkManager::refreshIfDetail()
{
    KVAL_NETWORK_MANAGER_CHECK_ENGINE

    LOG_INFO(LOG_TAG, "refreshIfDetail");
    QMap<QString, std::function<void(NetworkManager&,
                                     const QString&,
                                     const InterfaceProperties&)> >
    _interfaceMap {
        {KVALSETTING_NETWORK_KEY_IF, &NetworkManager::updateConnectedInterface},
        {KVALSETTING_NETWORK_KEY_STATUS,  &NetworkManager::updateNetStatus},
        {KVALSETTING_NETWORK_KEY_MAC, &NetworkManager::updateMacAddress},
        {KVALSETTING_NETWORK_KEY_WIFISCAN, &NetworkManager::updateWifiAp},
        {KVALSETTING_NETWORK_KEY_DNS1, &NetworkManager::updatePrimaryDns},
        {KVALSETTING_NETWORK_KEY_DNS2, &NetworkManager::updateSecondDns}
    };

    const InterfaceProperties iF = m_networkEngine->getActiveInterface();
    LOG_INFO(LOG_TAG, "Network Interface name: %s", qPrintable(iF.name));

    Q_FOREACH(auto k, _interfaceMap.keys()){
        _interfaceMap[k](*this, k, iF);
    }

    auto setIpv4 = [&](const QString& key, const IpConfig& cfg){
        notifyValueChanged(KVALSETTING_GROUP_NETWORK, key,
                (cfg.protocol==kvalNetProtocol::IPv4Protocol) ?cfg.address:"");

    };
    auto setIpv6 = [&](const QString& key, const IpConfig& cfg){
        notifyValueChanged(KVALSETTING_GROUP_NETWORK, key,
                (cfg.protocol==kvalNetProtocol::IPv6Protocol) ? cfg.address:"");
    };
    auto setSubnet = [&](const QString& key, const IpConfig& cfg){
        notifyValueChanged(KVALSETTING_GROUP_NETWORK, key, cfg.netmask);
    };
    auto setGateway = [&](const QString& key, const IpConfig& cfg){
        notifyValueChanged(KVALSETTING_GROUP_NETWORK, key, cfg.gateway);
    };

    const IpConfig ipv4Cfg =
        m_networkEngine->getIfIpConfig(iF.type, kvalNetProtocol::IPv4Protocol);
    const IpConfig ipv6Cfg =
        m_networkEngine->getIfIpConfig(iF.type, kvalNetProtocol::IPv6Protocol);

    QMap<QString, std::function<void(const QString&, const IpConfig&)>>
    _ipConfigMap {
        {KVALSETTING_NETWORK_KEY_IPV4, setIpv4},
        {KVALSETTING_NETWORK_KEY_SUBNET, setSubnet},
        {KVALSETTING_NETWORK_KEY_GATEWAY, setGateway},
    };
    Q_FOREACH(auto k, _ipConfigMap.keys()){
        _ipConfigMap[k](k, ipv4Cfg);
    }

    setIpv6(KVALSETTING_NETWORK_KEY_IPV6, ipv6Cfg);
}
/**
 * @brief NetworkManager::updatestatus
 */
void NetworkManager::updateNetStatus(const QString& key,
                                const NetworkEngineIf::InterfaceProperties& iF)
{
    LOG_INFO(LOG_TAG, "connection status %u", iF.status);
    notifyValueChanged(KVALSETTING_GROUP_NETWORK, key,
                       (iF.status == kvalConnStatus::Connected) ?
                           "Connected" :
                           "Disconnected");

}

/**
 * @brief NetworkManager::updateConnectedInterface
 * @param key
 */
void NetworkManager::updateConnectedInterface(
        const QString& key,
        const InterfaceProperties& iF)
{
    LOG_INFO(LOG_TAG, "connection status %s", qPrintable(iF.name));
    QString ifType{};
    switch(iF.type){
    case kvalInterfaceType::Ethernet:
        ifType = "Ethernet";
        break;
    case kvalInterfaceType::Wifi:
        ifType = "WiFi";
        break;
    default:
        break;
    }
    notifyValueChanged(KVALSETTING_GROUP_NETWORK, key, ifType);
}

/**
 * @brief NetworkManager::updateWifiAp
 * @param key
 */
void NetworkManager::updateWifiAp(const QString& key,
                                  const InterfaceProperties& iF)
{
    if(iF.type == kvalInterfaceType::Wifi)
        notifyValueChanged(KVALSETTING_GROUP_NETWORK, key,iF.name);
}

/**
 * @brief NetworkManager::updateMacAddress
 */
void NetworkManager::updateMacAddress(const QString& key,
                                      const InterfaceProperties& iF)
{
    notifyValueChanged(KVALSETTING_GROUP_NETWORK, key,iF.hwaddr);
}

/**
 * @brief NetworkManager::updatePrimaryDns
 * @param key
 * @param iF
 */
void NetworkManager::updatePrimaryDns(const QString& key,
                               const InterfaceProperties& iF)
{
    notifyValueChanged(KVALSETTING_GROUP_NETWORK, key,
                       (iF.nameservers.size()>0) ? iF.nameservers[0] : "");
}

/**
 * @brief NetworkManager::updateSecondDns
 * @param key
 * @param iF
 */
void NetworkManager::updateSecondDns(const QString& key,
                               const InterfaceProperties& iF)
{
    notifyValueChanged(KVALSETTING_GROUP_NETWORK, key,
                       (iF.nameservers.size()>1) ? iF.nameservers[1] : "");
}

/**
 * @brief NetworkManager::onUserActionReq
 * @param group
 * @param key
 * @param value
 */
void NetworkManager::onUserActionReq(const QString& group,
                                      const QString& key,
                                      const QVariant& value)
{
    LOG_INFO(LOG_TAG,
             "Network manager onUserActionReq(%s, %s, %s)",
             qPrintable(group),
             qPrintable(key),
             qPrintable(value.toString()));

    if(key == KVALSETTING_NETWORK_KEY_IF)
    {
        qInfo() << "Switch interface to: " << value;
        bool status{false};
        kvalInterfaceType ifType= (key.toLower() == "ethernet") ?
                    kvalInterfaceType::Ethernet :
                    kvalInterfaceType::Wifi;

        QMetaObject::invokeMethod(m_networkEngine.get(),
                                  "setInterface",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(bool, status),
                                  Q_ARG(kvalInterfaceType, ifType));
        if(status){
            qInfo() << "Switching success to: " << value;
            // @TODO: update settings value to the new value
        }
        else {
            qInfo() << "Switching fails to: " << value;
        }
    }
    else if(key == KVALSETTING_NETWORK_KEY_WIFISCAN)
    {
        QString _inprogress = tr("Connexion ");
        _inprogress += value.toString();
        Q_EMIT notifyProgress(_inprogress, 0);
        QMetaObject::invokeMethod(
                    m_networkEngine.get(),
                    "apConnectRequest",
                    Qt::QueuedConnection,
                    Q_ARG(QString, value.toString()));
    }
}

/**
 * @brief KvalSettingManager::onUserMenuReq
 * @param group
 * @param key
 */
void NetworkManager::onUserMenuReq(const QString& key)
{
    LOG_INFO(LOG_TAG,"Network manager onUserMenuReq(%s)", qPrintable(key));

    if(key == KVALSETTING_NETWORK_KEY_WIFISCAN)
    {
        Q_EMIT notifyProgress(tr("Scanning wireless network ..."), 0);
        // Engine needs to reply even no aps found
        // The reply will deblock the ui, as it waits for a reponse
        QMetaObject::invokeMethod(
                    m_networkEngine.get(),
                    "onScanWlan",
                    Qt::QueuedConnection);
    }
}

/**
 * @brief NetworkManager::onUserReply
 * @param group
 * @param key
 * @param value
 */
void NetworkManager::onUserReply(
        const QString& group,
        const QString& key,
        const QVariant& value)
{
    LOG_INFO(LOG_TAG,"onUserReply(%s)", qPrintable(key));
    if(group != KVALSETTING_GROUP_NETWORK){
        return;
    }

    if (key == KVALSETTING_NETWORK_KEY_WIFISCAN) {
        QMetaObject::invokeMethod(
                    m_networkEngine.get(),
                    "apConnectReply",
                    Qt::QueuedConnection,
                    Q_ARG(QString, value.toString()));
    }
}

/**
 * @brief NetworkManager::onWlanScan
 * @param key
 */
void NetworkManager::onWlanApsAvailable(
        const QVector<WlanAccessPoint>& availableAps)
{
    auto wdp = [](bool isenc, uint strength){
        QString ico{"file://"};
        ico+= CfgObj->get_path(KvalConfigManager::SETTINGS_WIFI_ICON_PREFIX);
        if(strength < 20) ico += "_4";
        else if(strength < 50) ico += "_3";
        else if(strength < 75) ico += "_2";
        else ico += "_1";
        auto suffix= (isenc) ? "_lock.png" : ".png";
        ico +=suffix;
        return ico;
    };
    QVariantList displayAps{};

    if(!availableAps.isEmpty()) {
        // Construct the display list
        std::for_each(
             std::begin(availableAps),
             std::end(availableAps),
             [&](const WlanAccessPoint& ap){
                    displayAps.append(QVariantMap{
                            {"val", ap.essid},
                            {"def", false},
                            {"val2", ap.encryption},
                            {"icon", wdp(ap.secured ,ap.signal)} } );
            });
        Q_EMIT notifyAlert("success", tr("Wireless Network Scan Complete"));
    }
    Q_EMIT dynMenuReply(KVALSETTING_NETWORK_KEY_WIFISCAN, displayAps);
}
/**
 * @brief NetworkManager::notifyValueChanged
 * @param group
 * @param key
 * @param val
 */
void NetworkManager::notifyValueChanged(
        const QString& group,
        const QString& key,
        const QString& val)
{
    qDebug() << "update val: " << val;
    Q_EMIT netValueChanged(group, key, val);
}

/**
 * @brief NetworkManager::onRequestPassPhrase
 */
void NetworkManager::onRequestPassPhrase()
{
    qInfo() << "pass phrase requested...";
    Q_EMIT textEntryRequest(tr("Enter the wifi password..."), true);

}

} //namespace KvalNetworkPlatform
