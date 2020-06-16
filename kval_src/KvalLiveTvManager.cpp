#define LOG_ACTIVATED

#include "KvalLiveTvManager.h"

#define LOG_SRC KVAL_LIVETV_MANAGER
#include "KvalLogging.h"

struct KvalLiveTvManager::LiveTvAlerts {

    LiveTvAlerts():
        m_alerts{
        {EpgLoadError,"Something went wrong while activating Epg !"},
        {UsbLoadError,"Something went wrong while activating Live tv !"},
        {InvalidServerAddr,"Invalid server address !"},
        {ServerLoadError,"Something went wrong while activating Live tv !"}}
        {};
    ~LiveTvAlerts() = default;

    enum AlertType {
        Invalid =0,

        EpgLoadSuccess,
        EpgLoadError,
        EpgFileNotFound,

        InvalidServerAddr,
        NotSupportedLiveTvFile,
        MalFormattedLiveTvFile,
        ServerLoadError,

        UsbLoadSuccess,
        UsbLoadError,
        Count
    };

    void regalert(AlertType _k) {
        m_current = _k;
    }
    const QString alert() const {
        if(m_current != Invalid)
            return m_alerts[m_current];
        return "";
    }

private:
    const QMap<AlertType, QString> m_alerts;
    AlertType m_current{Invalid};
};


KvalLiveTvManager::KvalLiveTvManager(QObject *parent) :
    QObject(parent),
    m_alerts(new LiveTvAlerts)
{
    connect(KvalSettingManager::Instance(),
            SIGNAL(livetvEpgToggle(const QVariant&)),
            this, SLOT(onEpgToggle(const QVariant&)));

    connect(KvalSettingManager::Instance(),
            SIGNAL(settingsManLiveUserReq(const QString&, const QString&, const QVariant&)),
            this, SLOT(onUserActionReq(const QString&, const QString&, const QVariant&)),
            Qt::QueuedConnection);

    connect(&m_epgEngine, SIGNAL(qmlEpgReady(const QVariantMap&)),
            this, SIGNAL(qmlEpgReady(const QVariantMap&)),
            Qt::QueuedConnection);
    connect(&m_epgEngine, SIGNAL(qmlEpgEnds()),
            this, SIGNAL(qmlEpgEnds()),
            Qt::QueuedConnection);

    m_dispatcher = {
        {KVALSETTING_LIVE_KEY_EPG , &KvalLiveTvManager::toggleEpg},
        {KVALSETTING_LIVE_KEY_EPGCACHE , &KvalLiveTvManager::setEpgCache},
        {KVALSETTING_LIVE_KEY_EPGDISPLAY , &KvalLiveTvManager::toggleEpgList},
        {KVALSETTING_LIVE_KEY_USBLOAD , &KvalLiveTvManager::usbLiveListLoad},
        {KVALSETTING_LIVE_KEY_ACTIVATE , &KvalLiveTvManager::serverLiveListLoad},
    };
}

/**
 * @brief KvalLiveTvManager::~KvalLiveTvManager
 */
KvalLiveTvManager::~KvalLiveTvManager(){
    delete m_alerts;
}

/**
 * @brief KvalLiveTvManager::Start
 */
void KvalLiveTvManager::Start()
{
    auto _settings = KvalSettingManager::Instance();
    QTimeZone _tz(_settings->getTimeZone().toByteArray());
    m_epgEngine.setTimeZoneOffset(
                (_tz.offsetFromUtc(QDateTime::currentDateTime()))/3600);
    Q_FOREACH(const QString& _k, m_dispatcher.keys()) {
        m_dispatcher[_k](*this, _settings->value(KVALSETTING_GROUP_LIVE, _k));
    }
}

/**
 * @brief KvalLiveTvManager::Stop
 */
void KvalLiveTvManager::Stop()
{
    m_epgEngine.Stop();
}

/**
 * @brief KvalLiveTvManager::processEpgFile
 */
bool KvalLiveTvManager::processEpgFile(void)
{
    QString epgfilepath = CfgObj->get_path(KvalConfigManager::KVAL_EPG_FILE);
    return m_epgEngine.Start(epgfilepath.toStdString().c_str());
}
/**
 * @brief KvalLiveTvManager::onUserActionReq
 * @param group
 * @param key
 * @param value
 */
void KvalLiveTvManager::onUserActionReq(const QString& group,
                                      const QString& key,
                                      const QVariant& value)
{
    LOG_INFO(LOG_TAG,
             "Live Tv manager onUserActionReq(%s, %s, %s)",
             qPrintable(group),
             qPrintable(key),
             qPrintable(value.toString()));

    if(m_dispatcher.contains(key)) {
        if(!m_dispatcher[key](*this, value)) {
            LOG_ERROR(LOG_TAG,
                      "Something went wrong while setting [%s]",
                      qPrintable(key));
            QMetaObject::invokeMethod(
                        KvalSettingManager::Instance(), "onSetValueFailed",
                        Qt::QueuedConnection,
                        Q_ARG(QString, group),
                        Q_ARG(QString, key));
            Q_EMIT notifyAlert("error",
                             tr(m_alerts->alert().toStdString().c_str()));
        }
        else {
            QMetaObject::invokeMethod(
                        KvalSettingManager::Instance(), "onSetValue",
                        Qt::QueuedConnection, Q_ARG(QString, group),
                        Q_ARG(QString, key), Q_ARG(QVariant, value));
        }
    }
}
/**
 * @brief KvalEpgEngine::onEpgToggle
 * @param val
 */
bool KvalLiveTvManager::toggleEpg(const QVariant& val)
{
    bool ret{true};
    qInfo() << "toggleEpg: " << val;
    KvalSettingManager::_trval(val, m_epgEnabled);

    qInfo() << "m_epgEnabled: " << m_epgEnabled;
    if(   (m_epgEnabled)
       && (m_epgEngine.status() == KvalEpgEngine::EPG_NOT_AVAILABLE_ST) ){
        ret = processEpgFile();
        if(!ret){
            m_alerts->regalert(LiveTvAlerts::EpgLoadError);
        }
    }

    return ret;
}

/**
 * @brief KvalLiveTvManager::toggleEpg
 * @param val
 */
bool KvalLiveTvManager::toggleEpgList(const QVariant& val)
{
    qInfo() << "toggleEpgList: " << val;
    bool _bl;
    KvalSettingManager::_trval(val, _bl);
    m_epgListMode.store(_bl);
    return true;
}

/**
 * @brief KvalLiveTvManager::setEpgCache
 * @param val
 */
bool KvalLiveTvManager::setEpgCache(const QVariant& val)
{
    qInfo() << "setEpgCache: " << val;
    KvalSettingManager::_trval(val, m_epgCacheSize);
    return true;
}

/**
 * @brief KvalLiveTvManager::usbLiveListLoad
 * @param val
 */
bool KvalLiveTvManager::usbLiveListLoad(const QVariant& val)
{
    qInfo() << "usbLiveListLoad: " << val;
    //@ TODO
    m_alerts->regalert(LiveTvAlerts::UsbLoadError);
    return false;
}

/**
 * @brief KvalLiveTvManager::serverLiveListLoad
 * @param val
 */
bool KvalLiveTvManager::serverLiveListLoad(const QVariant& val)
{
    bool ret{false};
    qInfo() << "serverLiveListLoad: " << val;
    auto _srvval =KvalSettingManager::Instance()->value(
                        KVALSETTING_GROUP_LIVE, KVALSETTING_LIVE_KEY_SRVADDR);
    if(!_srvval.isValid()){
        m_alerts->regalert(LiveTvAlerts::InvalidServerAddr);
    }
    else {
        //@ TODO: Get livetv file from distant srv
        m_alerts->regalert(LiveTvAlerts::ServerLoadError);
    }

    return ret;
}

/**
 * @brief KvalLiveTvManager::isEpgAvailable
 * @return
 */
bool KvalLiveTvManager::isEpgAvailable()
{
    return (   (m_epgEnabled)
            && (m_epgEngine.status() == KvalEpgEngine::EPG_AVAILABLE_ST) );
}

/**
 * @brief KvalLiveTvManager::getChEpg
 * @param chname
 * @return
 */
QVariantMap KvalLiveTvManager::getChEpg(const QString& chname)
{
    QVariantMap chepg;
    m_epgEngine.generateUiChEpg(chname, chepg);
    return chepg;
}

/**
 * @brief KvalLiveTvManager::onGetEpgChList
 * @param chlist
 */
void KvalLiveTvManager::onGetEpgChList(const QStringList& chlist)
{
    if(m_epgEnabled) {
        m_epgEngine.generateEpgChList(chlist);
    }
}

/**
 * @brief KvalLiveTvManager::peekEpgListing
 * @return
 */
bool KvalLiveTvManager::peekEpgListMode()
{
    return m_epgListMode.load();
}
/**
 * @brief EM_EpgElement::EM_EpgElement
 */
KvalLiveTvManagerElement::KvalLiveTvManagerElement():
    m_livetvManagerTh{new KvalThread(&m_livetvManager)}
{
    LOG_INFO(LOG_TAG, "Initialize ...");

    connect(&m_livetvManager, SIGNAL(qmlEpgReady(const QVariantMap&)),
            this, SIGNAL(qmlEpgReady(const QVariantMap&)),
            Qt::QueuedConnection);
    connect(&m_livetvManager, SIGNAL(qmlEpgEnds()),
            this, SIGNAL(qmlEpgEnds()),
            Qt::QueuedConnection);
    connect(&m_livetvManager, SIGNAL(notifyAlert(const QString&, const QString&)),
            this, SIGNAL(notifyAlert(const QString&, const QString&)),
            Qt::QueuedConnection);

    m_livetvManagerTh->start();
    QMetaObject::invokeMethod(&m_livetvManager, "Start");
}

/**
 * @brief EM_EpgElement::~EM_EpgElement
 */
KvalLiveTvManagerElement::~KvalLiveTvManagerElement()
{
    LOG_INFO(LOG_TAG, "Delete EM_EpgElement ...");
    m_livetvManager.Stop();
    if(m_livetvManagerTh){
        m_livetvManagerTh->stop();
        delete m_livetvManagerTh;
    }
}

/**
 * @brief KvalLiveTvManagerElement::epgListMode
 * @return
 */
bool KvalLiveTvManagerElement::epgListMode()
{
    return m_livetvManager.peekEpgListMode();
}
/**
 * @brief EM_EpgElement::processEpgFile
 */
void KvalLiveTvManagerElement::newEpgFileAvailable()
{
    // @ TODO Stop parsing epg file if in progress
    qInfo() << "New epg file available, stop parsing if in progess";
}

/**
 * @brief EM_EpgElement::newEpgFileReady
 */
void KvalLiveTvManagerElement::newEpgFileReady()
{
    QMetaObject::invokeMethod(&m_livetvManager, "processEpgFile");
}

/**
 * @brief EM_EpgElement::getChannelEpgBlock
 * @param channelName
 * @return
 */
QVariantMap KvalLiveTvManagerElement::quickGetChannelEpg(
        const QString& channelName)
{
    QVariantMap currentEpg{};
    LOG_INFO(LOG_TAG, "quickGetChannelEpg [%s]...", qPrintable(channelName));
    if(m_livetvManager.isEpgAvailable())
    {
        return m_livetvManager.getChEpg(channelName);
    }

    return QVariantMap();
}

/**
 * @brief EM_EpgElement::extractChannelEpgList
 * @param chList
 */
void KvalLiveTvManagerElement::extractChannelEpgList(QStringList chList)
{
    if(!m_livetvManager.isEpgAvailable())
    {
        LOG_DEBUG(LOG_TAG, "EPG Not available yet ...");
        return;
    }

    QMetaObject::invokeMethod(&m_livetvManager,
                              "onGetEpgChList",
                              Qt::QueuedConnection,
                              Q_ARG(QStringList, chList));
}
