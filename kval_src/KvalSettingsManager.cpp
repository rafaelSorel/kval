
#define LOG_ACTIVATED
#include <QXmlStreamReader>
#include <QFile>

#include "KvalMiscUtils.h"
#include "KvalConfigManager.h"
#include "KvalSettingsManager.h"

#define LOG_SRC SETMANAGER
#include "KvalLogging.h"

/*------------------------------------------------------------------------------
|    DEFINES
+-----------------------------------------------------------------------------*/
QPointer<KvalSettingManager> g_settingManager;

/*------------------------------------------------------------------------------
|    Functions
+-----------------------------------------------------------------------------*/
/**
 * @brief setDefaultLanguage
 * @param lang
 * @return
 */
bool KvalSettingManager::setLanguage(const QString& plainlang)
{

    /** ~/opt/system/build.LibreELEC-S905.aarch64-8.2-devel/toolchain/bin/lupdate -no-obsolete kval.pro */
    /** ~/opt/system/build.LibreELEC-S905.aarch64-8.2-devel/toolchain/bin/lconvert languages/kval_en.ts -o languages/kval_en.po */
    /** ~/opt/system/build.LibreELEC-S905.aarch64-8.2-devel/toolchain/bin/lconvert -locations relative languages/kval_en.po -o languages/kval_en.ts */
    /** ~/opt/system/build.LibreELEC-S905.aarch64-8.2-devel/toolchain/bin/lrelease kval.pro */

    LOG_INFO(LOG_TAG,
             "setDefaultLanguage: %s",
             plainlang.toStdString().c_str());

    static QTranslator g_translator;

    QString lang = plainlang;
//    MU_MiscUtils::getDataBetween("(", ")",plainlang);
    QFile lang_file(QString(":/languages/kval_").append(lang).append(".qm"));
    if(!lang_file.exists())
    {
        LOG_ERROR(LOG_TAG,
                  "Could not find: %s",
                  qPrintable(lang_file.fileName()));
        return false;
    }

    // look up e.g. :/languages/kval_fr-FR.qm
    qApp->removeTranslator(&g_translator);
    if (g_translator.load(lang_file.fileName()))
    {
        LOG_INFO(LOG_TAG,
                  "Successfully load: %s",
                  qPrintable(lang_file.fileName()));
        return qApp->installTranslator(&g_translator);
    }

    LOG_INFO(LOG_TAG,
             "Failed load: %s",
             qPrintable(lang_file.fileName()));

    return false;

}
/**
 * @brief SM_SettingsEngine::SM_SettingsEngine
 *
 *   Startup settings Subroutine:
 *   1- settings Manager loads the (settings display) file, read all the entries
 *      and send QVariantList to UI (this list is static and shall
 *      never change during the lifetime of kval)
 *   2- settings Manager will load the (userpreferences.ini file)
 *      and apply what ever is necessary through init on startup rule
 *      (language, screenRes and timezone).
 *   3- All userpreferences must be stored on their translation Id (Qt::tr::source).
 *      For pretty name display, it is up to the UI to translate the source id
 *      before display it.
 *
 *   Runtime settings Subroutine:
 *   1- UI shall always notify the userpreferences value with their (Qt::tr::source).
 *   2- SettingsManager will dispatch actions, latters will be catched by their owners:
 *      Example: (networkManager will catch network settings actions and
 *      shall know what to do with its settings.)
 *   2- "statchoicemenu" and "dynchoicemenu" choice menu user preference
 *      shall be handled as an action.
 */
KvalSettingManager::KvalSettingManager():
    m_settings{QSharedPointer<QSettings>::create(
               CfgObj->get_path(KvalConfigManager::KVAL_SETTINGS_INI_FILE),
               QSettings::IniFormat)},
    m_settingsValueMtx{}
{
    LOG_INFO(LOG_TAG, "Instantiate SM_SettingsEngine");

    m_actionDispatcher = {
        {KVALSETTING_GROUP_GENERAL , &KvalSettingManager::settingsManGenUserReq},
        {KVALSETTING_GROUP_NETWORK , &KvalSettingManager::settingsManNetUserReq},
        {KVALSETTING_GROUP_VOD , &KvalSettingManager::settingsManVodUserReq},
        {KVALSETTING_GROUP_LIVE , &KvalSettingManager::settingsManLiveUserReq},
    };

    m_userReplyDispatcher = {
        {KVALSETTING_GROUP_GENERAL , &KvalSettingManager::settingsManGenUserReply},
        {KVALSETTING_GROUP_NETWORK , &KvalSettingManager::settingsManNetUserReply},
        {KVALSETTING_GROUP_VOD , &KvalSettingManager::settingsManVodUserReply},
        {KVALSETTING_GROUP_LIVE , &KvalSettingManager::settingsManLiveUserReply},
    };
    m_actionMenuDispatcher = {
        {KVALSETTING_KEY_LANGUAGE, &KvalSettingManager::updateDefaultLang},
        {KVALSETTING_KEY_TIMEZONE, &KvalSettingManager::updateDefaultTimeZone},
        {KVALSETTING_KEY_SCREENRES, &KvalSettingManager::updateDefaultScreenRes},
        {KVALSETTING_KEY_TIMEREFRESH , &KvalSettingManager::updateDateAndTime},
    };
}

/**
 * @brief KvalSettingManager::initialize
 */
void KvalSettingManager::initialize()
{
    // Local connections
    connect(this, &KvalSettingManager::settingsManGenUserReq,
            this, &KvalSettingManager::onUserActionReq,
            Qt::QueuedConnection);
    connect(this, &KvalSettingManager::settingsManNetUserReply,
            &m_networkManager, &NetworkManager::onUserReply,
            Qt::DirectConnection);

    // To Network manager
    connect(this, &KvalSettingManager::settingsManNetUserReq,
            &m_networkManager, &NetworkManager::onUserActionReq,
            Qt::DirectConnection);

    // From Network manager
    connect(&m_networkManager, SIGNAL(dynMenuReply(const QString&, const QVariantList&)),
            this, SIGNAL(dynMenuReply(const QString&, const QVariantList&)),
            Qt::QueuedConnection);
    connect(&m_networkManager, SIGNAL(netValueChanged(const QString&, const QString&, const QVariant&)),
            this, SLOT(onSetValue(const QString&, const QString&, const QVariant&)),
            Qt::QueuedConnection);
    connect(&m_networkManager, SIGNAL(notifyAlert(const QString&, const QString&)),
            this, SIGNAL(displayMsg(const QString&, const QString&)),
            Qt::QueuedConnection);
    connect(&m_networkManager, SIGNAL(notifyProgress(const QString&, qreal)),
            this, SIGNAL(displayProgress(const QString&, qreal)),
            Qt::QueuedConnection);
    connect(&m_networkManager, SIGNAL(textEntryRequest(const QString&, bool)),
            this, SIGNAL(textEntryRequest(const QString&, bool)),
            Qt::QueuedConnection);

    // To Display manager
    connect(this, SIGNAL(settingsManGenUserReply(const QString&, const QString&, const QVariant&)),
            &m_displayManager, SLOT(onUserReply(const QString&, const QString&, const QVariant&)),
            Qt::DirectConnection);

    // From Display manager
    connect(&m_displayManager, SIGNAL(displayMsg(const QString&, const QString&)),
            this, SIGNAL(displayMsg(const QString&, const QString&)),
            Qt::QueuedConnection);
    connect(&m_displayManager, SIGNAL(yesNoDiag(const QString&,const QString&,const QString&,const QString&)),
            this, SIGNAL(yesNoDiag(const QString&,const QString&,const QString&,const QString&)),
            Qt::QueuedConnection);
    connect(&m_displayManager, SIGNAL(yesNoDiagUpdate(const QString&,const QString&)),
            this, SIGNAL(yesNoDiagUpdate(const QString&,const QString&)),
            Qt::QueuedConnection);
    connect(&m_displayManager, SIGNAL(yesNoDiagClose()),
            this, SIGNAL(yesNoDiagClose()),
            Qt::QueuedConnection);
    connect(&m_displayManager, SIGNAL(displayMsg(const QString&, const QString&)),
            this, SIGNAL(displayMsg(const QString&, const QString&)),
            Qt::QueuedConnection);
    connect(&m_displayManager, SIGNAL(displayValueChanged(const QString&, const QString&, const QVariant&)),
            this, SLOT(onSetValue(const QString&, const QString&, const QVariant&)),
            Qt::QueuedConnection);

    m_elapsedTime.start();
    loadSettingsDisplay();
    m_networkManager.updateNetworkSettings();
    applyStartupValues();
}

/**
 * @brief SM_SettingsEngine::~SM_SettingsEngine
 */
KvalSettingManager::~KvalSettingManager()
{
    LOG_INFO(LOG_TAG, "Delete SM_SettingsEngine...");
    m_settings->sync();
}

/**
 * @brief KvalSettingManager::categories
 * @return
 */
QVariantList KvalSettingManager::categories()
{
    return m_categories;
}

/**
 * @brief KvalSettingManager::applyStartupValues
 * @return
 */
void KvalSettingManager::applyStartupValues()
{
    QMap<QString,
        std::function<void(KvalSettingManager&, const QString&)>>
        _startupMap {
            {KVALSETTING_KEY_LANGUAGE, &KvalSettingManager::setLanguage},
            {KVALSETTING_KEY_SCREENRES, &KvalSettingManager::setscreenRes},
            {KVALSETTING_KEY_TIMEZONE, &KvalSettingManager::settimeZone} };

    Q_FOREACH(auto key, _startupMap.keys()){
        m_settings->beginGroup(KVALSETTING_GROUP_GENERAL);
        auto val = m_settings->value(key, QVariant());
        m_settings->endGroup();
        _startupMap[key](*this, val.toString());
    }
}
/**
 * @brief KvalSettingManager::loadSettingsDisplay
 * @return
 */
bool KvalSettingManager::loadSettingsDisplay(void)
{

    QString _fpath = CfgObj->get_path(
                            KvalConfigManager::XML_SETTINGS_DISPLAY_FILE);
    QFile file(_fpath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Couldn't open settings display file: " << _fpath;
        return false;
    }

    QXmlStreamReader xmlsReader(&file);

    auto readSettingItems = [&](QVariantMap& cat) {
        QVariantList items;
        while(xmlsReader.readNextStartElement()){
            if(xmlsReader.name() == "item"){
                QVariantMap item;
                std::for_each(
                    std::begin(xmlsReader.attributes()),
                    std::end(xmlsReader.attributes()),
                    [&item](const QXmlStreamAttribute& at){
                    item.insert(at.name().toString(), at.value().toString());});
                items.append(item);
                value(cat["id"].toString(),
                        item["id"].toString(),
                        item["default"]);

            }
            xmlsReader.skipCurrentElement();
        }
        cat.insert("items", items);
    };

    auto readSettingCategory = [&](){
        while(xmlsReader.readNextStartElement()){
            if(xmlsReader.name() != "category"){
                xmlsReader.skipCurrentElement();
                continue;
            }
            QVariantMap category;
            std::for_each(
                std::begin(xmlsReader.attributes()),
                std::end(xmlsReader.attributes()),
                [&category](const QXmlStreamAttribute& at){
                category.insert(at.name().toString(), at.value().toString());});
            readSettingItems(category);
            m_categories.append(category);
        }
    };

    if (xmlsReader.readNextStartElement()) {
        if (xmlsReader.name() != "settings"){
            LOG_ERROR(LOG_TAG, "Not a settings xml file !");
            file.close();
            return false;
        }
        readSettingCategory();
    }

    file.close();
    return true;
}
/**
 * @brief KvalSettingManager::value
 * @param key
 * @param defaultValue
 * @return
 */
QVariant KvalSettingManager::value(
        const QString &group,
        const QString &key,
        const QVariant &defaultValue)
{
    QMutexLocker _lck(&m_settingsValueMtx);
    m_settings->beginGroup(group);
    if(!m_settings->contains(key)){
        m_settings->setValue(key, defaultValue);
    }

    QVariant val = m_settings->value(key, defaultValue);
    m_settings->endGroup();
    return val;
}

/**
 * @brief KvalSettingManager::setValue
 */
void KvalSettingManager::onSetValue(
        const QString& group,
        const QString& key,
        const QVariant& value)
{
    QMutexLocker _lck(&m_settingsValueMtx);
    m_settings->beginGroup(group);
    m_settings->setValue(key, value);
    m_settings->endGroup();

    qInfo() << "update " << key  << " : " << value;
    Q_EMIT valueUpdated(group, key, value);
}

/**
 * @brief KvalSettingManager::onSetValueFailed
 * @param group
 * @param key
 */
void KvalSettingManager::onSetValueFailed(
        const QString& group,
        const QString& key)
{
    qWarning() << "value set failed: " << key;
    Q_EMIT valueUpdated(group, key, value(group, key));
}
/**
 * @brief KvalSettingManager::onReqAction
 * @param group
 * @param key
 */
void KvalSettingManager::onReqAction(
        const QString& group,
        const QString& key,
        const QVariant &value)
{
    LOG_INFO(LOG_TAG,
             "on req action: (%s, %s, %s)",
             qPrintable(group),
             qPrintable(key),
             qPrintable(value.toString()));

    if(m_actionDispatcher.contains(group)){
        Q_EMIT m_actionDispatcher[group](*this, group, key, value);
    }
}

/**
 * @brief KvalSettingManager::onKeyToggle
 * @param group
 * @param key
 */
void KvalSettingManager::onKeyToggle(
        const QString& group,
        const QString& key)
{
    LOG_INFO(LOG_TAG,
             "on Key Toggle: (%s, %s)",
             qPrintable(group),
             qPrintable(key));

    QVariant _v = !(value(group, key).toBool());

    if(m_actionDispatcher.contains(group)){
        Q_EMIT m_actionDispatcher[group](*this, group, key, _v);
    }
}
/**
 * @brief KvalSettingManager::onUserReply
 * @param group
 * @param key
 * @param value
 */
void KvalSettingManager::onUserReply(const QString& group,
                                    const QString& key,
                                    const QVariant &value)
{
    if(m_userReplyDispatcher.contains(group)){
        Q_EMIT m_userReplyDispatcher[group](*this, group, key, value);
    }
}

/**
 * @brief KvalSettingManager::onGenUserReq
 * @param key
 */
void KvalSettingManager::onUserActionReq(const QString& group,
                                      const QString& key,
                                      const QVariant& value)
{
    qInfo() << group;
    if(m_actionMenuDispatcher.contains(key)){
        m_actionMenuDispatcher[key](*this, group, key, value.toString());
    }
}

/**
 * @brief KvalSettingManager::onReqDynVal
 * @param group
 * @param key
 */
void KvalSettingManager::onReqDynVal(const QString& group, const QString& key)
{

     LOG_INFO(LOG_TAG,
              "onReqDynVal(%s, %s)",
              qPrintable(group),
              qPrintable(key));

    if(group == KVALSETTING_GROUP_GENERAL) {
        this->onUserMenuReq(key);
    }
    else if(group == KVALSETTING_GROUP_NETWORK){
        m_networkManager.onUserMenuReq(key);
    }
    else {
        Q_EMIT dynMenuReply(key, QVariantList());
    }
}
/**
 * @brief KvalSettingManager::onUserMenuReq
 * @param group
 * @param key
 */
void KvalSettingManager::onUserMenuReq(const QString& key)
{
    LOG_INFO(LOG_TAG,
             "onUserMenuReq(%s)",
             qPrintable(key));

    if(key == KVALSETTING_KEY_SCREENRES) {
        QVariantList rvs{};
        auto _lst = m_displayManager.resolutions();
        auto _cur = m_displayManager.getCurrentResolution();
        std::for_each(std::begin(_lst),
                  std::end(_lst),
                  [&](const QString& res){
                  rvs.append(QVariantMap{{"val", res}, {"def", res==_cur}});});

        Q_EMIT dynMenuReply(key, rvs);
    }
}

/**
 * @brief KvalSettingManager::updateDefaultTimeZone
 * @param group
 * @param key
 * @param value
 */
void KvalSettingManager::updateDefaultTimeZone(
        const QString& group,
        const QString& key,
        const QVariant& value)
{
    settimeZone(value.toString());
    onSetValue(group, key, value);
}

/**
 * @brief KvalSettingManager::updateDefaultScreenRes
 * @param group
 * @param key
 * @param value
 */
void KvalSettingManager::updateDefaultScreenRes(
        const QString& group,
        const QString& key,
        const QVariant& value)
{
    Q_UNUSED(group)
    Q_UNUSED(key)
    m_displayManager.updateScreenRes(value.toString());
}

/**
 * @brief SM_SettingsEngine::setlanguage
 * @return 
 */
void KvalSettingManager::updateDefaultLang(
        const QString& group,
        const QString& key,
        const QVariant& value)
{
    qInfo();
    auto res = setLanguage(value.toString());
    if(!res) {
        Q_EMIT displayMsg("Error" , tr("Non supporté dans cette version !"));
        return;
    }

    onSetValue(group, key, value);
    Q_EMIT languageChanged();
}

/**
 * @brief KvalSettingManager::updateDateAndTime
 * @param group
 * @param key
 * @param value
 */
void KvalSettingManager::updateDateAndTime(
        const QString& group,
        const QString& key,
        const QVariant& value)
{
    Q_UNUSED(group)
    Q_UNUSED(key)
    Q_UNUSED(value)
    Q_EMIT displayProgress(tr("Mise a jours date-heure ..."), 30);
    bool status = m_timezone.refreshDateAndTime();

    if(status) {
        Q_EMIT displayMsg("success" , tr("Mise à jour avec succès."));
    }
    else {
        Q_EMIT displayMsg("error" , tr("Problème mise à jour date et heure."));
    }
    Q_EMIT endSetSetting();
}
/**
 * @brief SM_SettingsEngine::settimezone
 */
void KvalSettingManager::settimeZone(const QString& value)
{
    m_timezone.set(value);
}

/**
 * @brief SM_SettingsEngine::setscreenRes
 */
void KvalSettingManager::setscreenRes(const QString& value)
{
    m_displayManager.setResolution(value);
    onSetValue(KVALSETTING_GROUP_GENERAL,
               KVALSETTING_KEY_SCREENRES,
               m_displayManager.getCurrentResolution());
}

/**
 * @brief KvalSettingManager::getTimeZone
 * @return
 */
QVariant KvalSettingManager::getTimeZone()
{
    return value(KVALSETTING_GROUP_GENERAL, KVALSETTING_KEY_TIMEZONE);
}

/**
 * @brief SM_SettingsEngine::getboxName
 * @return 
 */
QString KvalSettingManager::getboxName()
{
    //@TODO: part of network manager
    LOG_DEBUG(LOG_TAG, "Host Name %s",
             qPrintable(QHostInfo::localHostName()));
    return QHostInfo::localHostName();
}

/**
 * @brief SM_SettingsEngine::getstartSince
 * @return 
 */
QString KvalSettingManager::getstartSince()
{
    qint64 mscDuration = m_elapsedTime.elapsed();
    QString res;

    LOG_INFO(LOG_TAG, "mscDuration: %lld", mscDuration);
    int duration = (int) (mscDuration / 1000);
    int seconds = (int) (duration % 60);
    duration /= 60;
    int minutes = (int) (duration % 60);
    duration /= 60;
    int hours = (int) (duration % 24);
    int days = (int) (duration / 24);
    if((hours == 0)&&(days == 0))
        res = res.asprintf("%02d:%02d", minutes, seconds);
    if (days == 0)
      res = res.asprintf("%02d:%02d:%02d", hours, minutes, seconds);
    res = res.asprintf("%dJ %02dH %02dMin %02dSec",
                      days, hours, minutes, seconds);

    LOG_DEBUG(LOG_TAG, "res: %s", qPrintable(res));
    return res;
}

/**
 * @brief SM_SettingsEngine::getSoftVersion
 * @return 
 */
QString KvalSettingManager::getSoftVersion()
{
#ifdef AMLOGIC_TARGET
    QFile File("/etc/os-release");
#else
    QFile File("./os-release");
#endif
    QString name;
    QString version;
    QString versionId;
    QString result = "";


    if (File.open(QIODevice::ReadOnly))
    {
       QTextStream in(&File);
       while (!in.atEnd())
       {
          QString line = in.readLine();
          if((QString(line)).split("=").first().remove("\n") == "NAME")
          {
              name = (QString(line)).split("=").last().remove("\n");
          }
          else if((QString(line)).split("=").first().remove("\n") == "VERSION")
          {
              version = (QString(line)).split("=").last().remove("\n");
          }
          else if((QString(line)).split("=").first().remove("\n") == "VERSION_ID")
          {
              versionId = (QString(line)).split("=").last().remove("\n");
          }
       }
       result = name +  "-" + version + "-" + versionId;
       result = result.remove('"');
       File.close();
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Unable to open status file");
    }
    return result;
}

/**
 * @brief SM_SettingsEngine::getserialNumber
 * @return 
 */
QString KvalSettingManager::getserialNumber()
{
    QByteArray qusid;
#ifdef AMLOGIC_TARGET
    char usid[16];
    memset(usid, 0, sizeof(usid));
    if (!aml_get_usid(usid))
    {
        LOG_ERROR(LOG_TAG, "Enable to extract usid !");
    }
    else
    {
        QByteArray usridstr(usid, 16);
        qusid = usridstr.mid(4,8);
    }
#else
    qusid = "";
#endif

    return QString(qusid.toHex().toStdString().c_str());
}

/**
 * @brief SM_SettingsElements::refreshSettingsValue
 */
void KvalSettingManagerElement::refreshSettingsValue()
{
}

/**
 * @brief KvalSettingManagerElement::value
 * @param key
 * @param defaultVal
 * @return
 */
QVariant KvalSettingManagerElement::value(
        const QString &group,
        const QString &key)
{
    return m_settingsManager->value(group, key);
}

/**
 * @brief KvalSettingManagerElement::setValue
 * @param group
 * @param key
 * @param value
 */
void KvalSettingManagerElement::setValue(
        const QString &group,
        const QString &key,
        const QVariant &value)
{
    QMetaObject::invokeMethod(m_settingsManager,
                              "onSetValue",
                              Qt::QueuedConnection,
                              Q_ARG(QString, group),
                              Q_ARG(QString, key),
                              Q_ARG(QVariant, value));
}

/**
 * @brief KvalSettingManagerElement::actionReq
 */
void KvalSettingManagerElement::reqaction(
        const QString &group,
        const QString &key)
{
    QMetaObject::invokeMethod(m_settingsManager,
                              "onReqAction",
                              Qt::QueuedConnection,
                              Q_ARG(QString, group),
                              Q_ARG(QString, key));
}

/**
 * @brief KvalSettingManagerElement::actionReq
 */
void KvalSettingManagerElement::reqMenuAction(
        const QString &group,
        const QString &key,
        const QVariant &value)
{
    QMetaObject::invokeMethod(m_settingsManager,
                              "onReqAction",
                              Qt::QueuedConnection,
                              Q_ARG(QString, group),
                              Q_ARG(QString, key),
                              Q_ARG(QVariant, value));
}

/**
 * @brief KvalSettingManagerElement::toggleChoice
 * @param group
 * @param key
 */
void KvalSettingManagerElement::toggleChoice(
        const QString &group,
        const QString &key)
{
    QMetaObject::invokeMethod(m_settingsManager,
                              "onKeyToggle",
                              Qt::QueuedConnection,
                              Q_ARG(QString, group),
                              Q_ARG(QString, key));
}
/**
 * @brief KvalSettingManagerElement::userReplyNotify
 * @param group
 * @param key
 * @param value
 */
void KvalSettingManagerElement::userReplyNotify(
        const QString &group,
        const QString &key,
        const QVariant &value)
{
    QMetaObject::invokeMethod(m_settingsManager,
                              "onUserReply",
                              Qt::QueuedConnection,
                              Q_ARG(QString, group),
                              Q_ARG(QString, key),
                              Q_ARG(QVariant, value));
}

/**
 * @brief KvalSettingManagerElement::dynvalues
 * @param group
 * @param key
 * @return
 */
void KvalSettingManagerElement::dynvalues(
        const QString &group,
        const QString &key)
{
    qInfo() << "dynvalues requested ...";
    QMetaObject::invokeMethod(m_settingsManager,
                              "onReqDynVal",
                              Qt::QueuedConnection,
                              Q_ARG(QString, group),
                              Q_ARG(QString, key));
}

/**
 * @brief KvalSettingManagerElement::getCategoiesDisplay
 * @return
 */
QVariantList KvalSettingManagerElement::getCategoiesDisplay()
{
    return m_settingsManager->categories();
}

/**
 * @brief SM_SettingsElements::SM_SettingsElements
 */
KvalSettingManagerElement::KvalSettingManagerElement():
    m_settingsManager{KvalSettingManager::Instance()},
    m_settingsManagerThread(new KvalThread(m_settingsManager))
{
    connect(m_settingsManager, SIGNAL(dynMenuReply(const QString&, const QVariantList&)),
            this, SIGNAL(dynMenuReply(const QString&, const QVariantList&)),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(displayMsg(const QString&, const QString&)),
            this, SIGNAL(displayMsg(const QString&, const QString&)),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(displayProgress(const QString&, qreal)),
            this, SIGNAL(displayProgress(const QString&, qreal)),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(yesNoDiag(const QString&,const QString&,const QString&,const QString&)),
            this, SIGNAL(yesNoDiag(const QString&,const QString&,const QString&,const QString&)),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(yesNoDiagUpdate(const QString&,const QString&)),
            this, SIGNAL(yesNoDiagUpdate(const QString&,const QString&)),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(yesNoDiagClose()),
            this, SIGNAL(yesNoDiagClose()),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(languageChanged()),
            this, SIGNAL(languageChanged()),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(valueUpdated(const QString&, const QString&, const QVariant&)),
            this, SIGNAL(valueUpdated(const QString&, const QString&, const QVariant&)),
            Qt::QueuedConnection);
    connect(m_settingsManager, SIGNAL(textEntryRequest(const QString&, bool)),
            this, SIGNAL(textEntryRequest(const QString&, bool)),
            Qt::QueuedConnection);

    m_settingsManagerThread->start();
}

/**
 * @brief KvalSettingManagerElement::~KvalSettingManagerElement
 */
KvalSettingManagerElement::~KvalSettingManagerElement()
{
    LOG_INFO(LOG_TAG, "Delete KvalSettingManagerElement...");
    m_settingsManagerThread->stop();
    delete m_settingsManagerThread;
}
