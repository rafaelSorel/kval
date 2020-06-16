#define LOG_ACTIVATED

#include "KvalConnManNetEngine.h"

#define LOG_SRC NETUTILS
#include "KvalLogging.h"


//----------------------------------------------------------------------------
// Macro definition
//----------------------------------------------------------------------------
#define CONNMAN_NETMANAGER_CNX_TIMEOUT   100U

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
 * @brief ConnManNetworkEngine::ConnManNetworkEngine
 */
ConnManNetworkEngine::ConnManNetworkEngine():
    m_currentWifiService{nullptr},
    m_currentEthernetService{nullptr},
    m_current_pass{},
    m_has_ethernet{false},
    m_ethernet_connected{false},
    m_has_wifi{false},
    m_wifi_connected{false}

{
    LOG_INFO(LOG_TAG,"ConnManNetworkEngine initialize");
}

/**
 * @brief ConnManNetworkEngine::~ConnManNetworkEngine
 */
ConnManNetworkEngine::~ConnManNetworkEngine()
{
    LOG_INFO(LOG_TAG, "Delete ConnManNetworkEngine");
}

/**
 * @brief ConnManNetworkEngine::StartConnManNetworkEngine
 */
void ConnManNetworkEngine::Start()
{
    LOG_INFO(LOG_TAG, "StartConnManNetworkEngine");

    m_netManager.reset(new NetworkManager);
    connect (m_netManager.get(), SIGNAL(servicesChanged()),
             this, SLOT(onServiceChanged()));

    m_agent.reset(new UserAgent(this));
    connect(m_agent.get(), SIGNAL(userInputRequested(QString, QVariantMap)),
            this, SLOT(agentRequestedUserInput(QString, QVariantMap)));
    connect(m_agent.get(), SIGNAL(errorReported(QString, QString)), this,
            SLOT(agentReportedError(QString, QString)));
}

/**
 * @brief ConnManNetworkEngine::getActiveInterface
 * @return
 */
const ConnManNetworkEngine::InterfaceProperties ConnManNetworkEngine::getActiveInterface()
{
    InterfaceProperties iF{};
    QPointer<NetworkService> _netService=
            (!m_currentEthernetService.isNull()) ?
                m_currentEthernetService :
                m_currentWifiService;

    if(_netService.isNull()){
        return iF;
    }

    iF.type = (_netService->type() == "Wifi") ?
                kvalInterfaceType::Wifi :
                kvalInterfaceType::Ethernet;

    iF.status = (m_currentEthernetService->connected()) ?
                    kvalConnStatus::Connected :
                    kvalConnStatus::Disconnected ;

    iF.name = m_currentEthernetService->name();

    iF.hwaddr = (m_currentEthernetService->ethernet().contains("Address")) ?
                m_currentEthernetService->ethernet()["Address"].toString() :
                "";
    iF.nameservers = m_currentEthernetService->nameservers();


    return iF;
};

/**
 * @brief ConnManNetworkEngine::getIfIpConfig
 * @param interface
 * @param protocol
 * @return
 */
const ConnManNetworkEngine::IpConfig ConnManNetworkEngine::getIfIpConfig(
        kvalInterfaceType interface,
        kvalNetProtocol protocol)
{
    QPointer<NetworkService> _netService=
            (interface == kvalInterfaceType::Wifi) ?
                m_currentWifiService :
                m_currentEthernetService;

    IpConfig cfg{};
    if(_netService.isNull()){
        return cfg;
    }

    QVariantMap ipConfig= (protocol == kvalNetProtocol::IPv6Protocol) ?
                _netService->ipv6() :
                _netService->ipv4();

    cfg.protocol = protocol;
    if(ipConfig.contains("Method")){
        cfg.method = (ipConfig["Method"] == "dhcp") ?
                    kvalNetMethod::Dhcp :
                    kvalNetMethod::Manual;
    }
    if(ipConfig.contains("Address")){
        cfg.address = ipConfig["Address"].toString();
    }
    if(ipConfig.contains("Netmask")){
        cfg.netmask = ipConfig["Netmask"].toString();
    }
    if(ipConfig.contains("Gateway")){
        cfg.gateway = ipConfig["Gateway"].toString();
    }

    return cfg;
}

/**
 * @brief ConnManNetworkEngine::displayActiveNetTech
 */
void ConnManNetworkEngine::displayActiveNetTech()
{
    Q_FOREACH(const NetworkTechnology* netTech, m_netManager->getTechnologies())
    {
        LOG_DEBUG(LOG_TAG, "============================");
        LOG_DEBUG(LOG_TAG, "name: %s", qPrintable(netTech->name()));
        LOG_DEBUG(LOG_TAG, "    type: %s", qPrintable(netTech->type()));
        LOG_DEBUG(LOG_TAG, "    powered: %u", netTech->powered());
        LOG_DEBUG(LOG_TAG, "    connected: %u", netTech->connected());
        LOG_DEBUG(LOG_TAG, "    objPath: %s", qPrintable(netTech->objPath()));
        LOG_DEBUG(LOG_TAG, "    path: %s", qPrintable(netTech->path()));
        LOG_DEBUG(LOG_TAG, "============================");
    }

}
/**
 * @brief ConnManNetworkEngine::onServiceChanged
 */
void ConnManNetworkEngine::onServiceChanged()
{
    INV();
    displayActiveNetTech();

    //Check ethernet status
    NetworkTechnology * ethernet = m_netManager->getTechnology("ethernet");
    if(ethernet)
    {
        m_has_ethernet = true;
        m_ethernet_connected = ethernet->connected();
        if(m_ethernet_connected)
            set_connected_service(Ethernet);

        //Disable auto change detection for now
        disable_services_auto_detection();
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Ethernet Not Found");
        m_netManager->setTechnologiesEnabled(true);
        m_has_ethernet = false;
        m_ethernet_connected = false;
    }

    //Check Wifi status
    NetworkTechnology * wifi = m_netManager->getTechnology("wifi");
    if(wifi)
    {
        m_has_wifi = true;
        m_wifi_connected = wifi->connected();
        if(m_wifi_connected)
            set_connected_service(Wifi);
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Wifi Not Found");
        m_netManager->setTechnologiesEnabled(true);
        m_has_wifi = false;
        m_wifi_connected = false;
    }
    NetworkService* default_route = m_netManager->defaultRoute();
    LOG_DEBUG(LOG_TAG,
             "default_route name: %s",
             qPrintable(default_route->name()));
    LOG_DEBUG(LOG_TAG,
             "default_route type: %s",
             qPrintable(default_route->type()));
    LOG_DEBUG(LOG_TAG,
             "default_route state: %s",
             qPrintable(default_route->state()));
}

/**
 * @brief ConnManNetworkEngine::disable_services_auto_detection
 */
void ConnManNetworkEngine::disable_services_auto_detection()
{
    INV();
    disconnect (m_netManager.get(),
                SIGNAL(servicesChanged()),
                this,
                SLOT(onServiceChanged()));
    OUTV();
}

/**
 * @brief ConnManNetworkEngine::set_connected_service
 * @param service
 */
void ConnManNetworkEngine::set_connected_service(SupportedServices service)
{
    LOG_INFO(LOG_TAG, "Set connected service: %u", service);
    if(service == Ethernet)
    {
        QVector<NetworkService*> services = m_netManager->getServices("ethernet");
        if(!services.count())
        {
            LOG_INFO(LOG_TAG,
                     "No Ethernet services found, already disconnected ?");
            m_currentEthernetService.clear();
            return;
        }
        if(!m_currentEthernetService.isNull())
            return;

        m_currentEthernetService = services[0];
        connect(m_currentEthernetService, SIGNAL(connectedChanged(bool)),
                 this, SLOT(onEthernetConnectedChanged(bool)));
        connect(m_currentEthernetService, SIGNAL(serviceConnectionStarted()),
                 this, SLOT(onEthernetConnectionStarted()));
        connect(m_currentEthernetService, SIGNAL(connectRequestFailed(QString)),
                 this, SLOT(onEthernetConnectRequestFailed(QString)));

    }
    else if(service == Wifi)
    {
        Q_FOREACH(NetworkService* sWifi, m_netManager->getServices("wifi"))
        {
            if(sWifi && sWifi->connected())
            {
                m_currentWifiService = sWifi;
                LOG_INFO(LOG_TAG,
                         "Found Connected wifi AP: %s",
                         qPrintable(m_currentWifiService->name()));
                return;
            }
        }
        LOG_INFO(LOG_TAG, "No wifi services found,already disconnected ?");
        m_currentWifiService.clear();
    }
}

/**
 * @brief ConnManNetworkEngine::scanWlanIfc
 * @return
 */
void ConnManNetworkEngine::onScanWlan(void)
{
    INV();
    QVector<WlanAccessPoint> ap_list{};
    if(!m_has_wifi)
    {
        LOG_ERROR(LOG_TAG, "Connman: Wifi not available");
        Q_EMIT notifyAlert("error", tr("Wlan not available !"));
        Q_EMIT notifyWlanAps(ap_list);
        return;
    }
    auto populateaps = [&ap_list](const NetworkService* srv){
        if(!srv) return;
        ap_list.append(WlanAccessPoint{
                           srv->name(),
                           (srv->securityType() == NetworkService::SecurityNone)
                            ? false : true,
                           srv->security().join("/"),
                           QString::number(srv->strength())});
        LOG_INFO(LOG_TAG, "++++++++++++++++++++++++++++++++++++++++");
        LOG_INFO(LOG_TAG, "essid: %s", qPrintable(srv->name()));
        LOG_INFO(LOG_TAG, "securityType: %u", srv->securityType());
        LOG_INFO(LOG_TAG, "security: %s", qPrintable(srv->security().join("/")));
        LOG_INFO(LOG_TAG, "strength: %u", srv->strength());
        LOG_INFO(LOG_TAG, "state: %s", qPrintable(srv->state()));
        LOG_INFO(LOG_TAG, "error: %s", qPrintable(srv->error()));
        LOG_INFO(LOG_TAG, "autoConnect: %u", srv->autoConnect());
        LOG_INFO(LOG_TAG, "path: %s", qPrintable(srv->path()));
    };

    QVector<NetworkService*> services = m_netManager->getServices("wifi");
    std::for_each(std::begin(services),std::end(services), populateaps);

    Q_EMIT notifyWlanAps(ap_list);
    OUTV();
}

/**
 * @brief ConnManNetworkEngine::apConnectRequest
 * @param ssid
 */
void ConnManNetworkEngine::apConnectRequest(const QString& ssid)
{
    if(!m_currentWifiService.isNull())
    {
        if(m_currentWifiService->name() != ssid)
        {
            LOG_INFO(LOG_TAG,
                     "Disconnect previous wifi AP: %s",
                     qPrintable(m_currentWifiService->name()));

            m_currentWifiService->disconnect(this);
            m_currentWifiService->requestDisconnect();
            int retry_number = 1000;
            while(m_currentWifiService->connected() && retry_number-- > 0)
            {
                usleep(1000);
            }
        }
        if( m_currentWifiService->connected() &&
            m_currentWifiService->name() != ssid)
        {
            Q_EMIT notifyAlert("Error",
                           tr("Unable to disconnect previous Access point !"));
            return;
        }
    }

    Q_FOREACH(const NetworkService* service, m_netManager->getServices("wifi"))
    {
        if(service->name() == ssid)
        {
            LOG_INFO(LOG_TAG, "Found Essid");
            m_currentWifiService = const_cast<NetworkService*>(service);
            break;
        }
    }

    LOG_INFO(LOG_TAG, "essid Name %s", qPrintable(m_currentWifiService->name()));
    createWifiConnectionTimer();
    QMetaObject::invokeMethod(this,
                              "onRequestConnect",
                              Qt::QueuedConnection,
                              Q_ARG(SupportedServices, Wifi));
}

/**
 * @brief ConnManNetworkEngine::apConnectReply
 * @param pass
 */
void ConnManNetworkEngine::apConnectReply(const QString& pass)
{
    LOG_INFO(LOG_TAG, "Passphrase Reply %s", qPrintable(pass));

    if (m_agent && m_wifiConnectionTimer.isNull())
    {
        QVariantMap reply;
        reply.insert("Passphrase", pass);
        m_agent->sendUserReply(reply);
        createWifiConnectionTimer();
    }
}

/**
 * @brief ConnManNetworkEngine::createWifiConnectionTimer
 */
void ConnManNetworkEngine::createWifiConnectionTimer()
{
    m_wifiConnectionTimer.reset(new QTimer);
    m_wifiConnectionTimer->setInterval(CONNMAN_NETMANAGER_CNX_TIMEOUT);
    m_wifiConnectionTimer->start();
    connect(m_wifiConnectionTimer.get(), SIGNAL(timeout()),
            this, SLOT(onCheckWifiConnection()));
}

/**
 * @brief ConnManNetworkEngine::createEthernetConnectionTimer
 */
void ConnManNetworkEngine::createEthernetConnectionTimer()
{
    m_ethernetConnectionTimer.reset(new QTimer);
    m_ethernetConnectionTimer->setInterval(CONNMAN_NETMANAGER_CNX_TIMEOUT);
    m_ethernetConnectionTimer->start();
    connect(m_ethernetConnectionTimer.get(), SIGNAL(timeout()),
            this, SLOT(onCheckEthernetConnection()));
}


/**
 * @brief ConnManNetworkEngine::apConnectAbort
 */
void ConnManNetworkEngine::apConnectAbort()
{
    LOG_INFO(LOG_TAG, "Abort wifi in progress connection if any...");
    if(!m_wifiConnectionTimer.isNull() && m_wifiConnectionTimer->isActive()){
        _abortWifiConnectionTimer();
    }
}
/**
 * @brief ConnManNetworkEngine::onCheckWifiConnection
 */
void ConnManNetworkEngine::onCheckWifiConnection()
{
    static int max_retries = 100;
    static int retry_number = 0;
    if(m_currentWifiService.isNull()){
        _abortWifiConnectionTimer();
        retry_number=0;
        return;
    }

    if(m_currentWifiService->connected()) {
        LOG_INFO(LOG_TAG, "Wifi connection success ");
        _abortWifiConnectionTimer();
        retry_number=0;
        Q_EMIT notifyAlert("success",
                         tr("Connected to ")+m_currentWifiService->name());
    }
    else if (retry_number > max_retries) {
        _abortWifiConnectionTimer();
        retry_number=0;
        Q_EMIT notifyAlert("error", tr("Connection Timeout."));

    }
    else {
        LOG_INFO(LOG_TAG, "Wifi connection in progress ...");
        Q_EMIT notifyProgress (
        QString(tr("Connexion ") + m_currentWifiService->name()+ " ..."),
        100 * static_cast<qreal>(++retry_number*CONNMAN_NETMANAGER_CNX_TIMEOUT /
                                 max_retries*CONNMAN_NETMANAGER_CNX_TIMEOUT) );
    }
}

/**
 * @brief ConnManNetworkEngine::onCheckEthernetConnection
 */
void ConnManNetworkEngine::onCheckEthernetConnection()
{
    static int max_retries = 100;
    static int retry_number = 0;
    if(m_currentEthernetService.isNull()){
        _abortEthernetConnectionTimer();
        retry_number=0;
        return;
    }

    if(!m_currentEthernetService.isNull() && m_currentEthernetService->connected()) {
        LOG_INFO(LOG_TAG, "Ethernet connection success ");
        _abortEthernetConnectionTimer();
        retry_number=0;
    }
    else{
        LOG_INFO(LOG_TAG, "Ethernet connection in progress ...");
        Q_EMIT notifyProgress (
        QString(tr("Connexion ") + m_currentEthernetService->name()+ " ..."),
        100 * static_cast<qreal>(++retry_number*CONNMAN_NETMANAGER_CNX_TIMEOUT /
                                 max_retries*CONNMAN_NETMANAGER_CNX_TIMEOUT) );
    }
}
/**
 * @brief ConnManNetworkEngine::abortWifiConnectionTimer
 */
inline void ConnManNetworkEngine::_abortWifiConnectionTimer()
{
    if(!m_wifiConnectionTimer.isNull() && m_wifiConnectionTimer->isActive()){
        m_wifiConnectionTimer->stop();
        m_wifiConnectionTimer.reset(nullptr);
    }
}

/**
 * @brief ConnManNetworkEngine::_abortEthernetConnectionTimer
 */
inline void ConnManNetworkEngine::_abortEthernetConnectionTimer()
{
    if(!m_ethernetConnectionTimer.isNull() && m_ethernetConnectionTimer->isActive()){
        m_ethernetConnectionTimer->stop();
        m_ethernetConnectionTimer.reset(nullptr);
    }
}
/**
 * @brief ConnManNetworkEngine::agentReportedError
 * @param path
 * @param error
 */
void ConnManNetworkEngine::agentReportedError(QString path, QString error)
{
    INV("path: %s, error: %s", qPrintable(path), qPrintable(error));

    LOG_INFO(LOG_TAG,
             "path: %s, error: %s",
             qPrintable(path), qPrintable(error));

    if (error == InvalidKey) {
        _abortWifiConnectionTimer();
        Q_EMIT notifyAlert("error", tr("Mot de passe incorrect !"));
     }
    else if (error == OutOfRange) {
        _abortWifiConnectionTimer();
        Q_EMIT notifyAlert("error", tr("Accès impossible !"));
    }
    else if (error == PingMissing) { }
    else if (error == DhcpFailed) { }
    else if (error == ConnectFailed) {
        _abortWifiConnectionTimer();
        Q_EMIT notifyAlert("error", QString(tr("Erreur connexion ") +
                                         m_currentWifiService->name()));
    }
}


/**
 * @brief ConnManNetworkEngine::agentRequestedUserInput
 * @param path
 * @param field
 */
void ConnManNetworkEngine::agentRequestedUserInput(QString path,
                                                     QVariantMap field)
{
    LOG_INFO(LOG_TAG, "path: %s", qPrintable(path));

    if (field.contains("Passphrase"))
    {
        LOG_INFO(LOG_TAG, "Passphrase requested, Wait for user reply...");
        _abortWifiConnectionTimer();
        Q_EMIT apRequestPassPhrase();
    }
}

/**
 * @brief ConnManNetworkEngine::extractCurrentSsid
 * @return
 */
QString ConnManNetworkEngine::extractCurrentSsid()
{
    if(m_currentWifiService.isNull())
    {
        LOG_INFO(LOG_TAG, "No Wifi services available");
        return "";
    }
    return m_currentWifiService->name();
}

/**
 * @brief ConnManNetworkEngine::setInterface
 * @param ifType
 * @return
 */
bool ConnManNetworkEngine::setInterface(kvalInterfaceType ifType)
{
    if(ifType == kvalInterfaceType::Ethernet){
        disconnect_services(Wifi);
        connect_services(Ethernet);
    }
    else if(ifType == kvalInterfaceType::Wifi){
        disconnect_services(Ethernet);
        connect_services(Wifi);
    }
    else {
        Q_EMIT notifyAlert("error", tr("Interface type not handled !"));
        return false;
    }
    return true;
}

/**
 * @brief ConnManNetworkEngine::disconnect_services
 * @param service
 * @return
 */
bool ConnManNetworkEngine::disconnect_services(SupportedServices service)
{
    INV("service: %u", service);

    LOG_INFO(LOG_TAG, "Disconnect Service: %u", service);
    bool status = true;
    int retry_number = 1000;
    if(service == Ethernet)
    {
        if(m_currentEthernetService.isNull())
        {
            LOG_INFO(LOG_TAG,
                     "No Ethernet service found, already disconnected");
            status = true;
            goto return_cleanup;
        }
        m_currentEthernetService->requestDisconnect();
        while(m_currentEthernetService->connected() &&
              retry_number-- > 0)
        {
            LOG_INFO(LOG_TAG, "ethernet Disconnection request ...");
            usleep(10000);
            Q_EMIT notifyProgress(QString(tr("Déconnexion ethernet ...")),
                                    (0.1F * ((1000-retry_number))));
        }

        if(m_currentEthernetService->connected())
        {
            LOG_ERROR(LOG_TAG, "Timeout Ethernet Disconnected");
            status = false;
            goto return_cleanup;
        }

        LOG_INFO(LOG_TAG, "Ethernet Disconnected");
        status = true;
        goto return_cleanup;
    }
    else if(service == Wifi)
    {
        if(m_currentWifiService.isNull())
        {
            LOG_INFO(LOG_TAG, "No Wifi services found,already disconnected");
            status = true;
            goto return_cleanup;
        }

        m_currentWifiService->requestDisconnect();
        while(m_currentWifiService->connected() &&
              retry_number-- > 0)
        {
            LOG_INFO(LOG_TAG, "wifi Disconnection request ...");
            usleep(10000);
            Q_EMIT notifyProgress(QString(tr("Déconnexion ")+
                                            m_currentWifiService->name()+
                                            " ..."),
                                    (0.1F * ((1000-retry_number))));
        }
        if(m_currentWifiService->connected())
        {
            LOG_ERROR(LOG_TAG, "Timeout Wifi Disconnected");
            status = false;
            goto return_cleanup;
        }

        LOG_INFO(LOG_TAG, "Wifi Disconnected");
        status = true;
        goto return_cleanup;
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Not handled service: %u !!", service);
    }

return_cleanup:
    OUTV("status: %u", status);
    return status;
}

/**
 * @brief ConnManNetworkEngine::onRequestConnect
 * @param service
 */
void ConnManNetworkEngine::onRequestConnect(unsigned int service)
{
    if((SupportedServices)service == Ethernet)
    {
        LOG_INFO(LOG_TAG, "ethernet onRequestConnect Service");
        if(m_currentEthernetService)
            m_currentEthernetService->requestConnect();
    }
    else if((SupportedServices)service == Wifi)
    {
        LOG_INFO(LOG_TAG, "wifi onRequestConnect Service");
        m_currentWifiService->disconnect(this);
        connect (m_currentWifiService, SIGNAL(connectedChanged(bool)),
                 this, SLOT(onConnectedChanged(bool)));
        connect (m_currentWifiService, SIGNAL(serviceConnectionStarted()),
                 this, SLOT(onServiceConnectionStarted()));
        connect (m_currentWifiService, SIGNAL(connectRequestFailed(const QString&)),
                 this, SLOT(onConnectRequestFailed(const QString&)));

        m_currentWifiService->requestConnect();
    }
}

/**
 * @brief ConnManNetworkEngine::refresh_services
 */
void ConnManNetworkEngine::refresh_services()
{
    INV();
    NetworkService* default_route = m_netManager->defaultRoute();
    LOG_INFO(LOG_TAG,
             "default_route name: %s",
             qPrintable(default_route->name()));
    LOG_INFO(LOG_TAG,
             "default_route type: %s",
             qPrintable(default_route->type()));
    LOG_INFO(LOG_TAG,
             "default_route state: %s",
             qPrintable(default_route->state()));

    OUTV();
}

/**
 * @brief ConnManNetworkEngine::connect_services
 * @param service
 */
void ConnManNetworkEngine::connect_services(SupportedServices service)
{
    INV("service: %u", service);
    LOG_INFO(LOG_TAG, "Connect Service: %u", service);
    if(service == Ethernet)
    {
        if(m_currentEthernetService.isNull())
            set_connected_service(Ethernet);

        if(!m_currentEthernetService.isNull())
        {
            createWifiConnectionTimer();
            QMetaObject::invokeMethod(this,
                                      "onRequestConnect",
                                      Qt::QueuedConnection,
                                      Q_ARG(SupportedServices, Ethernet));
        }
    }
    else if(service == Wifi)
    {
        LOG_INFO(LOG_TAG, "request Wifi Connection");
        if(m_currentWifiService.isNull())
        {
            LOG_ERROR(LOG_TAG, "No wifi service available");
        }
    }
}

/**
 * @brief ConnManNetworkEngine::onEthernetConnectRequestFailed
 * @param error
 */
void ConnManNetworkEngine::onEthernetConnectRequestFailed(QString error)
{
    LOG_INFO(LOG_TAG,
             "Ethernet Failed Connection: %s",
             qPrintable(error));
}

/**
 * @brief ConnManNetworkEngine::onEthernetConnectionStarted
 */
void ConnManNetworkEngine::onEthernetConnectionStarted()
{
    LOG_INFO(LOG_TAG, "Ethernet Connection started");
}

/**
 * @brief ConnManNetworkEngine::onEthernetConnectedChanged
 * @param status
 */
void ConnManNetworkEngine::onEthernetConnectedChanged(bool status)
{
    LOG_INFO(LOG_TAG,
             "Ethernet status: %u", status);
    LOG_INFO(LOG_TAG,
             "Ethernet connected: %u",
             m_currentEthernetService->connected());
    LOG_INFO(LOG_TAG,
             "Ethernet saved: %u",
             m_currentEthernetService->saved());
}

/**
 * @brief ConnManNetworkEngine::onConnectRequestFailed
 * @param error
 */
void ConnManNetworkEngine::onConnectRequestFailed(const QString& error)
{
    LOG_INFO(LOG_TAG,
             "%s: Failed Connection: %s",
             qPrintable(m_currentWifiService->name()),
             qPrintable(error));

    //@TODO Perform a retry and check how many retry
    //      and what error codes to handle
    if (error == OperationAborted) {
        m_currentWifiService->requestConnect();
        return;
     }
    else if (error == InvalidArguments) { }
    else if (error == PermissionDenied) { }
    else if (error == PassphraseRequired) { }
    else if (error == NotRegistered) { }
    else if (error == NotUnique) { }
    else if (error == NotSupported) { }
    else if (error == NotImplemented) { }
    else if (error == NotFound) { }
    else if (error == NoCarrier) { }
    else if (error == InProgress) { return; }
    else if (error == AlreadyExists) { }
    else if (error == AlreadyEnabled) { }
    else if (error == AlreadyDisabled) { }
    else if (error == AlreadyConnected) { }
    else if (error == NotConnected) { }
    else if (error == OperationTimeout) { }
    else if (error == InvalidProperty) { }
    else if (error == InvalidService) { }

    _abortWifiConnectionTimer();
    Q_EMIT notifyAlert("error", tr(error.toStdString().c_str()));
}

/**
 * @brief ConnManNetworkEngine::onServiceConnectionStarted
 */
void ConnManNetworkEngine::onServiceConnectionStarted()
{
    LOG_INFO(LOG_TAG,
             "%s: Connection started",
             qPrintable(m_currentWifiService->name()));
}

/**
 * @brief ConnManNetworkEngine::onConnectedChanged
 * @param status
 */
void ConnManNetworkEngine::onConnectedChanged(bool status)
{
    LOG_INFO(LOG_TAG,
             "%s: status: %u",
             qPrintable(m_currentWifiService->name()), status);
    LOG_INFO(LOG_TAG,
             "currentWifiService->connected(): %u",
             m_currentWifiService->connected());
    if(m_currentWifiService->connected())
    {
        m_currentWifiService->setAutoConnect(true);
        LOG_INFO(LOG_TAG,
                 "currentWifiService->saved(): %u",
                 m_currentWifiService->saved());
    }
}
