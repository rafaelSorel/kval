#define LOG_ACTIVATED

#include <QGuiApplication>
#include <QScreen>
#include <fstream>
#include <QMap>
#include <QPair>

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#include <sys/mount.h>
#endif

#include "KvalConfigManager.h"

#define LOG_SRC CONFIGMGR
#include "KvalLogging.h"

KvalConfigManager * g_cfg_manager = nullptr;


/**
 * @brief CM_ManagerResources::getEnumFromStr
 * @param id
 * @return
 */
int KvalConfigManagerResources::getEnumFromStr(QString id)
{
    const QMetaObject &mo = KvalConfigManagerResources::staticMetaObject;
    int index = mo.indexOfEnumerator("str_ids");
    QMetaEnum metaEnum = mo.enumerator(index);
    return metaEnum.keyToValue((const char *)id.toStdString().c_str());
}

/**
 * @brief KvalConfigManager::KvalConfigManager
 */
KvalConfigManager::KvalConfigManager()
{
    QProcessEnvironment proc_env;
    m_home_path = proc_env.systemEnvironment().value("HOME", "/storage/.kval");
    m_script_home_path = proc_env.systemEnvironment().value("HOME_SCRIPTS",
                                                            "/usr/bin/kval");

    //Create trickmode partition
    QString trick_mount = get_base_res_path() + "kvalpvr";
    QDir dir(trick_mount);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#if defined (AMLOGIC_TARGET)
    char trick_part[] = "/dev/cache";
    umount2(trick_mount.toStdString().c_str(), MNT_FORCE);
    if( -1 == mount(trick_part, trick_mount.toStdString().c_str(), "ext4", MS_SILENT, NULL))
    {
        LOG_ERROR(LOG_TAG, "Trickpart mount failed with error: %s", strerror(errno));
    }
#endif

#elif defined (Q_OS_WINDOWS)
    LOG_ERROR(LOG_TAG, "Trickpart mount failed with error: %s", strerror(errno));
#endif
}

/**
 * @brief CM_ConfigManager::qmlGetStr
 * @param id
 * @return
 */
QString KvalConfigManager::qmlGetStr(str_ids id)
{
    return cfg_str_tab.value(id);
}

/**
 * @brief CM_ConfigManager::get_base_res_path
 * @return
 */
QString KvalConfigManager::get_base_res_path()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
    return "/storage/.kval";
#elif defined(Q_OS_WINDOW)
#else
    return m_home_path;
#endif
}

/**
 * @brief CM_ConfigManager::translate_path
 * @param path
 * @return
 */
QString KvalConfigManager::translate_path(const QString &path)
{
    QDir res_path{get_base_res_path() + "/kval_res"};
    QMap<QString, QDir> path_token {
            {"KVAL_RES_PATH", res_path} };

    QString tmp_path{path};

    for(auto it = path_token.cbegin(); it != path_token.cend(); ++it)
    {
        if (tmp_path.contains(it.key()) && it.value().isReadable())
        {
            tmp_path.replace(it.key(), it.value().path());
            return tmp_path;
        }
    }
    return path;
}
/**
 * @brief CM_ConfigManager::qmlGetPath
 * @param id
 * @return
 */
QString KvalConfigManager::qmlGetPath(path_ids id)
{
    QString finalPath = QString("file://") +
                            get_base_res_path() +
                            cfg_path_tab.value(id);
    return finalPath;
}

/**
 * @brief CM_ConfigManager::xmlGetStr
 * @param id
 * @return
 */
QString KvalConfigManager::xmlGetStr(QString &id)
{
    int enum_id = getEnumFromStr(id);
    if(enum_id < 0)
    {
        return id;
    }
    return qmlGetStr((str_ids)enum_id);
}

/**
 * @brief CM_ConfigManager::get_path
 * @param id
 * @return
 */
QString KvalConfigManager::get_path(path_ids id)
{
    return get_base_res_path() + cfg_path_tab.value(id);
}

/**
 * @brief CM_ConfigManager::get_exec_path
 * @param id
 * @return
 */
QString KvalConfigManager::get_exec_path(path_ids id)
{
    return (m_script_home_path + cfg_exc_path_tab.value(id) );
}

/**
 * @brief CM_ConfigManagerElement::qmlStr
 * @param id
 * @return
 */
QString KvalConfigManagerElement::qmlStr(str_ids id)
{
    return CfgObj->qmlGetStr(id);
}

/**
 * @brief CM_ConfigManagerElement::qmlGetPath
 * @param id
 * @return
 */
QString KvalConfigManagerElement::qmlGetPath(path_ids id)
{
    return CfgObj->qmlGetPath(id);
}

/**
 * @brief CM_ConfigManagerElement::xmlGetStr
 * @param id
 * @return
 */
QString KvalConfigManagerElement::xmlGetStr(QString id)
{
    return CfgObj->xmlGetStr(id);
}

/**
 * @brief CM_ConfigManagerElement::qmlGetSrvPiconPath
 * @param channelName
 * @return
 */
QString KvalConfigManagerElement::qmlGetSrvPiconPath(QString channelName)
{
    QString serv_base_path = "https://" +
                             m_livetv_srv_address +
                             "/d/livetv/picons/" +
                             channelName.replace(" ", "_") +
                             ".png";
    LOG_DEBUG(LOG_TAG, "picons srv path: %s", serv_base_path.toStdString().c_str());
    return serv_base_path;
}

/**
 * @brief CM_ConfigManagerElement::langChanged
 */
void KvalConfigManagerElement::langChanged()
{
    Q_EMIT languageChanged();
}

/**
 * @brief CM_ConfigManagerElement::updateLiveTvSrvAddress
 */
void KvalConfigManagerElement::updateLiveTvSrvAddress()
{
    m_livetv_srv_address =
        KvalSettingManager::Instance()->value(KVALSETTING_GROUP_LIVE,
                                              KVALSETTING_LIVE_KEY_SRVADDR).toString();
}

/**
 * @brief CM_ConfigManagerElement::getRetranslate
 * @return
 */
QString KvalConfigManagerElement::getRetranslate()
{
    return "";
}
