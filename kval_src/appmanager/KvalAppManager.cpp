#define LOG_ACTIVATED
#include <QFile>
#include <QThread>
#include <QMap>
#include <QDate>
#include <QXmlStreamReader>
#include <QtNetwork>
#include <QtDBus/QtDBus>
#include <QProcess>
#include <QTimer>
#include <QCryptographicHash>
#include <QDBusArgument>
#include <QDBusMetaType>

#define LOG_SRC APPSMANAGER
#include "KvalLogging.h"
#include "KvalLocalBrowseManager.h"
#include "KvalAppManager.h"
#include "KvalConfigManager.h"
#include "KvalThreadUtils.h"
#include "KvalPyInvoker.h"

/*------------------------------------------------------------------------------
|    DEFINES
+-----------------------------------------------------------------------------*/
#define SERVICE_APPS_NAME             "org.QtDBus.kvalapps"
#define SERVICE_APPS_INVOKER          "org.QtDBus.kvalappsPyInvoker"
#define SERVICE_APP_BINDER_INVOKER    "org.QtDBus.kvalappsBinderInvoker"

QDBusInterface * g_dbusAppsActiveInterface = nullptr;
APPS_Engine * g_appEngineRef = nullptr;
/*------------------------------------------------------------------------------
|    Functions
+-----------------------------------------------------------------------------*/
/**
 * @brief clientStopScriptTask
 */
void KvalAppManagerBridge::appsStopScriptTask(void)
{
    if(g_dbusAppsActiveInterface)
        g_dbusAppsActiveInterface->call(QDBus::Block,"stopScript");
}

/**
 * @brief AM_AppsPythonInvoker::AM_AppsPythonInvoker
 */
KvalAppManagerBridge::KvalAppManagerBridge()
{
    m_progressDiagIsAborted = false;
    m_yesNoReply = -1;
    m_decReply = -1;
    m_execFunctionStatus = -1;
    m_listReply = -1;
    m_keyboardReply = nullptr;
    m_inputHasReply = false;
    m_owner = nullptr;

    qRegisterMetaType<QStringList>("QStringList");
    qDBusRegisterMetaType<QStringList>();
    qRegisterMetaType<QMap<QString, QString>>("QMap<QString, QString>");
    qRegisterMetaType<QList<QMap<QString, QString> > >("QList<QMap<QString, QString> >");
    qDBusRegisterMetaType<QMap<QString, QString>>();
    qDBusRegisterMetaType<QList<QMap<QString, QString> > >();

}

/**
 * @brief AM_AppsPythonInvoker::setOwner
 * @param owner
 */
void KvalAppManagerBridge::setOwner(KvalAppManager * owner)
{
    m_owner = owner;
}
/**
 * @brief AM_AppsPythonInvoker::add_installed_apps
 * @param argument
 * @param count
 */
void KvalAppManagerBridge::add_installed_apps(
        const QMap<QString, QString> &argument,
        int count)
{
    LOG_INFO(LOG_TAG, "add_installed_apps: %d",count);
    Q_EMIT addInstalledApps(argument, count);
}

/**
 * @brief AM_AppsPythonInvoker::refresh_installed_apps
 */
void KvalAppManagerBridge::refresh_installed_apps()
{
    Q_EMIT refreshInstalledApps();
}

/**
 * @brief AM_AppsPythonInvoker::add_item
 * @param argument
 * @param count
 */
void KvalAppManagerBridge::add_item(const int &handle,
                                     const QMap<QString, QString> &argument,
                                     int count)
{
    if(!_check_handle(handle))
        return;

    LOG_DEBUG(LOG_TAG, "add_item: %d",count);
    Q_EMIT addItem(argument, count);
}

/**
 * @brief AM_AppsPythonInvoker::set_res_url
 * @param handle
 * @param argument
 */
void KvalAppManagerBridge::set_res_url(const int &handle,
                                        const QMap<QString, QString> &argument)
{
    if(!_check_handle(handle))
        return;

    QVariantMap variantArg;
    Q_FOREACH (QString key, argument.keys())
    {
        variantArg[key] = argument[key];
    }
    Q_EMIT setResUrl(variantArg);
}

/**
 * @brief AM_AppsPythonInvoker::end_of_category
 * @param succeeded
 */
void KvalAppManagerBridge::end_of_category( const int &handle,
                                             bool succeeded )
{
    if(!_check_handle(handle))
        return;

    LOG_INFO(LOG_TAG, "succeeded: %u", succeeded);
    Q_EMIT endOfCategory(succeeded);
}

/**
 * @brief AM_AppsPythonInvoker::ok_diag
 * @param title
 * @param text
 */
void KvalAppManagerBridge::ok_diag(const QString &title,
                                    const QString &text)
{
    m_yesNoReply = -1;
    Q_EMIT okDiag(title, text);
}

/**
 * @brief AM_AppsPythonInvoker::yesno_diag
 * @param title
 * @param text
 * @param nolabel
 * @param yeslabel
 * @param replyurl
 */
void KvalAppManagerBridge::yesno_diag(const QString &title,
                                       const QString &text,
                                       const QString &nolabel,
                                       const QString &yeslabel)
{
    m_yesNoReply = -1;
    Q_EMIT yesNoDiag(title, text, nolabel, yeslabel);
}

/**
 * @brief AM_AppsPythonInvoker::input_keyboard
 * @param title
 * @param defaultTxt
 * @param hidden
 */
void KvalAppManagerBridge::input_keyboard(const QString &title,
                                           const QString &defaultTxt,
                                           const bool &hidden)
{
    m_keyboardReply = nullptr;
    m_inputHasReply = false;
    Q_EMIT inputKeyboard(title, defaultTxt, hidden);
}

/**
 * @brief AM_AppsPythonInvoker::input_list
 * @param items
 */
void KvalAppManagerBridge::input_list(const QString &title,
                                       const QStringList &items)
{
    m_inputHasReply = false;
    m_listReply = -1;
    Q_EMIT inputList(title, items);
}

/**
 * @brief AM_AppsPythonInvoker::diag_progress_create
 * @param header
 * @param text
 */
void KvalAppManagerBridge::diag_progress_create(const QString &header,
                                       const QString &text)
{
    Q_EMIT diagProgressCreate(header, text);
}

/**
 * @brief AM_AppsPythonInvoker::diag_progress_update
 * @param position
 * @param text
 */
bool KvalAppManagerBridge::diag_progress_update(const int &position,
                                                 const QString &text)
{
    if(!m_progressDiagIsAborted)
    {
        Q_EMIT diagProgressUpdate(position, text);
    }
    LOG_DEBUG(LOG_TAG, "m_progressDiagIsAborted: %u",m_progressDiagIsAborted);
    return m_progressDiagIsAborted;
}

/**
 * @brief AM_AppsPythonInvoker::get_yesno_reply
 * @return
 */
int KvalAppManagerBridge::get_yesno_reply()
{
    return m_yesNoReply;
}

/**
 * @brief AM_AppsPythonInvoker::diag_progress_close
 */
void KvalAppManagerBridge::diag_progress_close()
{
    m_progressDiagIsAborted = false;
    Q_EMIT diagProgressClose();
}

/**
 * @brief AM_AppsPythonInvoker::displayMsgUi
 * @param severity
 * @param payload
 */
void KvalAppManagerBridge::displayMsgUi(const QString &severity,
                                         const QString &payload)
{
    Q_EMIT displayAppsMsgSig(severity, payload);
}

/**
 * @brief AM_AppsPythonInvoker::execute_func
 * @param function
 * @param needStatus
 */
void KvalAppManagerBridge::execute_func(const QString &function,
                                         const bool &needStatus)
{
    m_execFunctionStatus = -1;
    Q_EMIT executeFunc(function, "", needStatus);
}

/**
 * @brief AM_AppsPythonInvoker::get_exec_func_status
 * @return
 */
int KvalAppManagerBridge::get_exec_func_status()
{
    return m_execFunctionStatus;
}

/**
 * @brief AM_AppsPythonInvoker::input_has_reply
 * @return
 */
bool KvalAppManagerBridge::input_has_reply()
{
    return m_inputHasReply;
}

/**
 * @brief AM_AppsPythonInvoker::end_of_script
 * @param handle
 */
void KvalAppManagerBridge::end_of_script(const int &handle)
{
    if(!_check_handle(handle))
        return;

    LOG_INFO(LOG_TAG, "appHasStopped");
    Q_EMIT appHasStopped();
}

/**
 * @brief AM_AppsPythonInvoker::get_input_keyboard_result
 * @return
 */
QString KvalAppManagerBridge::get_input_keyboard_result()
{
    return m_keyboardReply;
}

/**
 * @brief AM_AppsPythonInvoker::get_language
 * @return
 */
QString KvalAppManagerBridge::get_language()
{
    return KvalSettingManager::Instance()->value(
                KVALSETTING_GROUP_GENERAL,
                KVALSETTING_KEY_LANGUAGE).toString();
}

/**
 * @brief AM_AppsPythonInvoker::get_srv_addr
 * @return
 */
QString KvalAppManagerBridge::get_srv_addr()
{
    return KvalSettingManager::Instance()->value(
                KVALSETTING_GROUP_LIVE,
                KVALSETTING_LIVE_KEY_SRVADDR).toString();
}

/**
 * @brief AM_AppsPythonInvoker::get_input_list_result
 * @return
 */
int  KvalAppManagerBridge::get_input_list_result()
{
    return m_listReply;
}

/**
 * @brief AM_AppsPythonInvoker::_check_handle
 * @param handle
 * @return
 */
bool KvalAppManagerBridge::_check_handle(int handle)
{
    if(m_owner->getCurrentHandler() != handle)
    {
        LOG_WARNING(LOG_TAG, "Not the right handle: %d", handle);
        return false;
    }

    return true;
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_play
 * @param source
 * @param argument
 * @param windowed
 * @param startpos
 */
void KvalAppManagerBridge::mediaplayer_play(const QString &source,
                                            const QMap<QString, QString> &argument,
                                            const bool &windowed,
                                            const int &startpos)
{
    m_owner->getMediaPlayerWrapper()->play(source,
                                          m_owner->getPlayList(),
                                          argument,
                                          windowed,
                                          startpos);
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_activateplaylist
 */
void KvalAppManagerBridge::mediaplayer_activateplaylist()
{
    m_owner->getMediaPlayerWrapper()->setPlayList(m_owner->getPlayList());
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_stop
 */
void KvalAppManagerBridge::mediaplayer_stop()
{
    m_owner->getMediaPlayerWrapper()->stop();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_pause
 */
void KvalAppManagerBridge::mediaplayer_pause()
{
    m_owner->getMediaPlayerWrapper()->pause();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_playnext
 */
void KvalAppManagerBridge::mediaplayer_playnext()
{
    m_owner->getMediaPlayerWrapper()->playNext();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_playprevious
 */
void KvalAppManagerBridge::mediaplayer_playprevious()
{
    m_owner->getMediaPlayerWrapper()->playPrevious();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_playselected
 * @param selected
 */
void KvalAppManagerBridge::mediaplayer_playselected(const int &selected)
{
    m_owner->getMediaPlayerWrapper()->playSelected(selected);
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_isplaying
 * @return
 */
bool KvalAppManagerBridge::mediaplayer_isplaying()
{
    return m_owner->getMediaPlayerWrapper()->isPlaying();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_isplayingaudio
 * @return
 */
bool KvalAppManagerBridge::mediaplayer_isplayingaudio()
{
    return m_owner->getMediaPlayerWrapper()->isPlayingAudio();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_isplayingvideo
 * @return
 */
bool KvalAppManagerBridge::mediaplayer_isplayingvideo()
{
    return m_owner->getMediaPlayerWrapper()->isPlayingVideo();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_filename
 * @return
 */
QString KvalAppManagerBridge::mediaplayer_filename()
{
    return m_owner->getMediaPlayerWrapper()->playingFileName();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_streamposition
 * @return
 */
int KvalAppManagerBridge::mediaplayer_streamposition()
{
    return m_owner->getMediaPlayerWrapper()->streamPosition();
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_seek
 * @param seekTime
 */
void KvalAppManagerBridge::mediaplayer_seek(const int &seekTime)
{
    return m_owner->getMediaPlayerWrapper()->seek(seekTime);
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_setsubsfile
 * @param path
 */
void KvalAppManagerBridge::mediaplayer_setsubsfile(const QString &path)
{
    return m_owner->getMediaPlayerWrapper()->setSubs(path);
}

/**
 * @brief AM_AppsPythonInvoker::mediaplayer_streamlength
 * @return
 */
int KvalAppManagerBridge::mediaplayer_streamlength()
{
    return m_owner->getMediaPlayerWrapper()->streamLength();
}


/**
 * @brief AM_AppsPythonInvoker::get_playlist_position
 * @param playListId
 * @return
 */
int KvalAppManagerBridge::get_playlist_position()
{
    return m_owner->getPlayList()->currentPosition();
}

/**
 * @brief AM_AppsPythonInvoker::get_playlist_size
 * @param playListId
 * @return
 */
int KvalAppManagerBridge::get_playlist_size()
{
    return m_owner->getPlayList()->size();
}

/**
 * @brief AM_AppsPythonInvoker::remove_playlist_element
 * @param playListId
 * @param index
 */
void KvalAppManagerBridge::remove_playlist_element(const int &index)
{
    m_owner->getPlayList()->Remove(index);
}

/**
 * @brief AM_AppsPythonInvoker::add_playlist_element
 * @param playListId
 * @param url
 * @param item
 * @param index
 */
void KvalAppManagerBridge::add_playlist_element(const QMap<QString, QString> &item,
                                                 const int &index)
{
    m_owner->getPlayList()->Insert(item, index);
}

/**
 * @brief AM_AppsPythonInvoker::clear_playlist
 * @param playListId
 */
void KvalAppManagerBridge::clear_playlist()
{
    m_owner->getPlayList()->Clear();
}

/**
 * @brief AM_AppsPythonInvoker::shuffle_playlist
 */
void KvalAppManagerBridge::shuffle_playlist(const int &index)
{
    m_owner->getPlayList()->Shuffle(index);
}

/**
 * @brief AM_AppsPythonInvoker::unshuffle_playlist
 */
void KvalAppManagerBridge::unshuffle_playlist()
{
    m_owner->getPlayList()->UnShuffle();
}

/**
 * @brief AM_AppsPythonInvoker::get_removable_devices
 * @return
 */
QList<QMap<QString, QString>> KvalAppManagerBridge::get_removable_devices()
{
    DEVICES_SET removableDevices = KvalDevicesUtils::get_available_devices();
    QList<QMap<QString, QString>> removables;
    Q_FOREACH(const KvalMediaDevice& device, removableDevices)
    {
        if(device.m_iDriveType == KvalMediaDevice::SOURCE_TYPE_REMOVABLE)
        {
            removables.append({{"name", device.strName},
                                {"node", device.devNode},
                                {"path", device.strPath}});
        }
    }

    return removables;
}

/**
 * @brief AM_AppsPythonInvoker::register_window_setting_diag
 * @param appId
 */
void KvalAppManagerBridge::register_window_setting_diag(const QString &appId)
{
    LOG_INFO(LOG_TAG, "registerWindowSettingDiag");
    m_owner->registerWindowSettingDiag(appId);
}

/**
 * @brief AM_AppsPythonInvoker::decrypt_cmd
 * @param keypath
 * @param src
 * @param dst
 */
void KvalAppManagerBridge::decrypt_cmd(const QString &sig,
                                        const QString &keypath,
                                        const QString &src,
                                        const QString &dst)
{
    m_decReply = -1;
    QMetaObject::invokeMethod(m_owner, "decryptAppCmd",
                              Q_ARG(QString, sig),
                              Q_ARG(QString, keypath),
                              Q_ARG(QString, src),
                              Q_ARG(QString, dst));
}

/**
 * @brief AM_AppsPythonInvoker::get_decrypt_reply
 * @return
 */
int KvalAppManagerBridge::get_decrypt_reply()
{
    return m_decReply;
}

/**
 * @brief AM_AppsPythonInvoker::add_menu_item_setting
 * @param appId
 * @param argument
 */
void KvalAppManagerBridge::add_menu_item_setting(const QString &appId,
                                        const QMap<QString, QString> &argument)
{
    if (!m_owner->checkWindowSettingDiagAppId(appId))
    {
        LOG_ERROR(LOG_TAG, "appId not found: %s", qPrintable(appId));
        return;
    }
    LOG_INFO(LOG_TAG, "addMenuItems");
    m_owner->addMenuItems(appId, argument);
}

void KvalAppManagerBridge::add_config_item_setting(const QString &appId,
                                              const QString &men,
                                              const QMap<QString, QString> &argument,
                                              const int &count)
{
    if (!m_owner->checkWindowSettingDiagAppId(appId))
    {
        LOG_ERROR(LOG_TAG, "appId not found: %s", qPrintable(appId));
        return;
    }

    LOG_INFO(LOG_TAG, "add_config_item_setting");
    return m_owner->addItemSetting(men, argument, count);
}

/**
 * @brief AM_AppsPythonInvoker::end_of_entries
 * @param appId
 */
void KvalAppManagerBridge::end_of_entries(const QString &appId, bool succeeded)
{
    if (!m_owner->checkWindowSettingDiagAppId(appId))
    {
        LOG_ERROR(LOG_TAG, "appId not found: %s", qPrintable(appId));
        return;
    }
    LOG_INFO(LOG_TAG, "endOfSettingsEntries");
    Q_EMIT endOfSettingsEntries(succeeded);
}

/**
 * @brief AM_AppsPythonInvoker::notify_custom_ui_template
 * @param uiTemplate
 */
void KvalAppManagerBridge::notify_custom_ui_template(const QString &uiTemplate)
{
    QMetaObject::invokeMethod(m_owner, "customUiTemplateNotif", Q_ARG(QString, uiTemplate));
}

/**
 * @brief AM_AppsPythonInvoker::get_ui_template_status
 * @param uiTemplate
 */
int KvalAppManagerBridge::get_ui_template_status(const QString &uiTemplate)
{
    AppsEngineCustomUiStatus status = m_owner->checkTemplateStatus(uiTemplate);
    switch (status) {
        case APP_UI_STATUS_UNKNOWN:
            LOG_INFO(LOG_TAG, "Template UI not yet ready !!");
            return (int)(-APP_UI_STATUS_LOADED);
        break;
        case APP_UI_STATUS_LOADED:
            LOG_INFO(LOG_TAG, "Template UI ready :)");
            return (int)APP_UI_STATUS_LOADED;
        break;
        case APP_UI_STATUS_ERROR:
            return (int)APP_UI_STATUS_ERROR;
        break;
        case APP_UI_STATUS_MAX:
        default:
            return (int)(-APP_UI_STATUS_MAX);
        break;
}

}

/**
 * @brief AM_AppsEngine::AM_AppsEngine
 */
KvalAppManager::KvalAppManager(KvalAppManagerElement * uiElement)
{
    LOG_INFO(LOG_TAG, "Instantiate AM_AppsEngine");
    m_totalCount = 0;
    m_extractedItems = 0;
    m_startAppsBinderTimer = nullptr;
    m_uiElement = uiElement;
    m_current_handler = 999;
    m_appsBuiltinFuncs = new EF_EmbedFuncs();
    m_appsBuiltinFuncs->setUiElementRef(m_uiElement);
    m_appsPythonInvoker.setOwner(this);
    m_mediaPlayList = new KvalAppMediaPlayList;
    m_mediaPlayerWrapper = new KvalAppMediaPlayerWrapper();
#ifdef KVAL_SECURE
    m_encSecureEngine = SecureEngine::getGlobalRef();
#endif
    m_appCustomUiTemplate = "";

    if (!QDBusConnection::sessionBus().isConnected())
    {
        LOG_ERROR(LOG_TAG,  "Cannot connect to the D-Bus session bus.\n"
                            "To start it, run:\n"
                            "\teval `dbus-launch --auto-syntax`\n");
        return;
    }
    if (!QDBusConnection::sessionBus().registerService(SERVICE_APPS_NAME))
    {
        LOG_ERROR(LOG_TAG, "%s",
                  qPrintable(QDBusConnection::sessionBus().
                             lastError().message()));
    }

    if(!QDBusConnection::sessionBus().registerObject("/apps",
                                        &m_appsPythonInvoker,
                                        QDBusConnection::ExportScriptableSlots))
    {
        LOG_ERROR(LOG_TAG, "problem register m_appsPythonInvoker Object");
    }

    m_invokerInterface = new QDBusInterface(SERVICE_APPS_INVOKER,
                                            "/", "",
                                            QDBusConnection::sessionBus());

    if (!m_invokerInterface->isValid())
    {
        LOG_ERROR(LOG_TAG, "Apps Invoker not yet valid");
    }

    m_binderInvokerInterface = new QDBusInterface(SERVICE_APP_BINDER_INVOKER,
                                                  "/binder", "",
                                                  QDBusConnection::sessionBus());

    g_dbusAppsActiveInterface = m_invokerInterface;
    moveToThread(this);

    //Connect python invoker
    connect(&m_appsPythonInvoker,
            SIGNAL(displayAppsMsgSig(QString, QString)),
            this,
            SIGNAL(displayAppsMsgSig(QString, QString)));
    connect(&m_appsPythonInvoker,
            SIGNAL(addInstalledApps(QMap<QString, QString>, int)),
            this,
            SLOT(onInstalledAppsAdd(QMap<QString, QString>, int)));
    connect(&m_appsPythonInvoker,
            SIGNAL(refreshInstalledApps()),
            this,
            SLOT(onRefreshInstalledApps()));
    connect(&m_appsPythonInvoker,
            SIGNAL(addItem(QMap<QString, QString>, int)),
            this,
            SLOT(onItemAdd(QMap<QString, QString>, int)));
    connect(&m_appsPythonInvoker,
            SIGNAL(endOfCategory(bool)),
            this,
            SIGNAL(endOfCategory(bool)));
    connect(&m_appsPythonInvoker,
            SIGNAL(endOfSettingsEntries(bool)),
            this,
            SIGNAL(endOfSettingsEntries(bool)));
    connect(&m_appsPythonInvoker,
            SIGNAL(okDiag(QString, QString)),
            this,
            SIGNAL(okDiag(QString, QString)));
    connect(&m_appsPythonInvoker,
            SIGNAL(yesNoDiag(QString, QString, QString, QString)),
            this,
            SIGNAL(yesNoDiag(QString, QString, QString, QString)));
    connect(&m_appsPythonInvoker,
            SIGNAL(diagProgressCreate(QString, QString)),
            this,
            SIGNAL(diagProgressCreate(QString, QString)));
    connect(&m_appsPythonInvoker,
            SIGNAL(diagProgressUpdate(int, QString)),
            this,
            SIGNAL(diagProgressUpdate(int, QString)));
    connect(&m_appsPythonInvoker,
            SIGNAL(diagProgressClose()),
            this,
            SIGNAL(diagProgressClose()));
    connect(&m_appsPythonInvoker,
            SIGNAL(setResUrl(QVariantMap)),
            this,
            SIGNAL(setResUrl(QVariantMap)));
    connect(&m_appsPythonInvoker,
            SIGNAL(executeFunc(QString, QString, bool)),
            this,
            SLOT(onExecuteFunc(QString, QString, bool)));
    connect(&m_appsPythonInvoker,
            SIGNAL(inputKeyboard(QString, QString, bool)),
            this,
            SIGNAL(inputKeyboard(QString, QString, bool)));
    connect(&m_appsPythonInvoker,
            SIGNAL(inputList(QString, QStringList)),
            this,
            SIGNAL(inputList(QString, QStringList)));
    connect(&m_appsPythonInvoker,
            SIGNAL(appHasStopped()),
            this,
            SIGNAL(appHasStopped()));

    //Connect player wrapper
    connect(m_mediaPlayerWrapper,
            SIGNAL(playbackStarted()),
            this,
            SLOT(onPlaybackStarted()));
    connect(m_mediaPlayerWrapper,
            SIGNAL(playbackFailed()),
            this,
            SLOT(onPlaybackFailed()));
    connect(m_mediaPlayerWrapper,
            SIGNAL(playbackCompleted()),
            this,
            SLOT(onPlaybackCompleted()));
    connect(m_mediaPlayerWrapper,
            SIGNAL(uiStopRequested()),
            this,
            SLOT(onUiStopRequested()));
}

/**
 * @brief AM_AppsEngine::tryReboot
 */
void KvalAppManager::tryReboot()
{
    LOG_INFO(LOG_TAG, "tryReboot");
    QDBusInterface logind{"org.freedesktop.login1",
                          "/org/freedesktop/login1",
                          "org.freedesktop.login1.Manager",
                          QDBusConnection::systemBus()};
    const auto message = logind.callWithArgumentList(QDBus::Block, "CanReboot", {});
    QDBusPendingReply< QString > canReboot = message;

    if (canReboot.isError())
    {
        const auto error = canReboot.error();
        LOG_ERROR(LOG_TAG,
                  "Asynchronous call finished with error: %s (%s)",
                  error.name().toStdString().c_str(),
                  error.message().toStdString().c_str());
        return;
    }
    if (canReboot.value() == "yes")
    {
        LOG_INFO(LOG_TAG, "canReboot yes");
#ifdef AMLOGIC_TARGET
        QDBusPendingReply<> reboot = logind.callWithArgumentList(QDBus::Block, "Reboot", {true, });
        if (reboot.isError())
        {
            const auto error = reboot.error();
            LOG_ERROR(LOG_TAG,
                      "Asynchronous call finished with error: %s (%s)",
                      error.name().toStdString().c_str(),
                      error.message().toStdString().c_str());
        }
#endif
    }
    else
    {
        LOG_ERROR(LOG_TAG,
                  "Can't reboot: CanReboot() result is 'yes'");
    }
}

/**
 * @brief AM_AppsEngine::decryptAppCmd
 * @param keypath
 * @param src
 * @param dst
 */
void KvalAppManager::decryptAppCmd( QString sig,
                                    QString keypath,
                                    QString src,
                                    QString dst)
{
    bool status = true;
#ifdef KVAL_SECURE
    status = m_encSecureEngine->vDec(sig, keypath, src, dst);
#endif
    m_appsPythonInvoker.diagDecryptReply(status);
}

/**
 * @brief AM_AppsEngine::onLangChanged
 */
void KvalAppManager::onLangChanged()
{
    if(m_binderInvokerInterface)
        m_binderInvokerInterface->call(QDBus::NoBlock, "onLangChanged");
}

/**
 * @brief AM_AppsEngine::onPlaybackStarted
 */
void KvalAppManager::onPlaybackStarted()
{
    if(m_binderInvokerInterface)
        m_binderInvokerInterface->call(QDBus::NoBlock, "onPlayBackStarted");
}

/**
 * @brief AM_AppsEngine::onPlaybackFailed
 */
void KvalAppManager::onPlaybackFailed()
{
    if(m_binderInvokerInterface)
        m_binderInvokerInterface->call(QDBus::NoBlock, "onPlaybackFailed");
}

/**
 * @brief AM_AppsEngine::onPlaybackCompleted
 */
void KvalAppManager::onPlaybackCompleted()
{
    if(m_binderInvokerInterface)
        m_binderInvokerInterface->call(QDBus::NoBlock, "onPlayBackEnded");
}

/**
 * @brief AM_AppsEngine::onUiStopRequested
 */
void KvalAppManager::onUiStopRequested()
{
    if(m_binderInvokerInterface)
        m_binderInvokerInterface->call(QDBus::NoBlock, "onPlayBackStopped");
}

/**
 * @brief AM_AppsEngine::~AM_AppsEngine
 */
KvalAppManager::~KvalAppManager()
{
    LOG_INFO(LOG_TAG, "Delete AM_AppsEngine...");
    if (m_binderInvokerInterface->isValid())
    {
        LOG_INFO(LOG_TAG, "Notify onAbortRequest ...");
        m_binderInvokerInterface->call(QDBus::NoBlock, "onAbortRequest");
    }

    if(m_startAppsBinderTimer)
    {
        m_startAppsBinderTimer->stop();
        delete m_startAppsBinderTimer;
    }
    if(m_appsBuiltinFuncs)
        delete m_appsBuiltinFuncs;

    if(m_invokerInterface)
        delete m_invokerInterface;

    if(m_binderInvokerInterface)
        delete m_binderInvokerInterface;

    if(m_mediaPlayList)
        delete m_mediaPlayList;

    if(m_mediaPlayerWrapper)
        delete m_mediaPlayerWrapper;
}

/**
 * @brief AM_AppsEngine::checkInvokerInterface
 */
void KvalAppManager::checkInvokerInterface()
{
    if (m_invokerInterface->isValid())
    {
        LOG_INFO(LOG_TAG, "Start Apps Binder Daemon ...");
        m_invokerInterface->call(QDBus::NoBlock, "startAppsBinderDeamon");
        return;
    }
    if(!m_startAppsBinderTimer)
    {
        m_startAppsBinderTimer = new QTimer(this);
        connect(m_startAppsBinderTimer,
                SIGNAL(timeout()),
                this,
                SLOT(checkInvokerInterface()));
    }
    m_startAppsBinderTimer->setSingleShot(true);
    m_startAppsBinderTimer->setInterval(50);
    m_startAppsBinderTimer->start();
}

/**
 * @brief AM_AppsEngine::onExecuteFunc
 * @param function
 * @param needStatus
 */
void KvalAppManager::onExecuteFunc(QString execStr, QString appId, bool needStatus)
{
    LOG_INFO(LOG_TAG,
             "function: %s, needStatus: %u",
             qPrintable(execStr), needStatus);
    if(!appId.isEmpty())
    {
        LOG_INFO(LOG_TAG, "Set activeAppId [%s]...", qPrintable(appId));
        m_activeAppId = appId;
    }
    m_execNeedStatus = needStatus;
    m_appsBuiltinFuncs->Execute(execStr);
    LOG_INFO(LOG_TAG, "Called Exexcute");
    OUTV();
}

/**
 * @brief AM_AppsEngine::constructArgs
 * @param arg
 * @return
 */
QString KvalAppManager::constructArgs(QString arg)
{
    QMutexLocker locker(&m_handlerLocker);
    INV("arg: %s", qPrintable(arg));
    int new_handle = qrand() % 1000;
    while(m_current_handler == new_handle)
    {
        new_handle = qrand() % 1000;
    }

    m_current_handler = new_handle;
    LOG_INFO(LOG_TAG, "m_current_handler: %d", m_current_handler);

    QString handler = QString::number(m_current_handler);
    OUTV("handler: %s", qPrintable(handler));

    return arg.isEmpty() ? handler : handler.append(" ").append(arg);
}

/**
 * @brief AM_AppsEngine::onAbortProgressDiag
 */
void KvalAppManager::onAbortProgressDiag()
{
    m_appsPythonInvoker.abortProgressDiag();
}

/**
 * @brief AM_AppsEngine::onDiagYesNoReply
 * @param reply
 */
void KvalAppManager::onDiagYesNoReply(bool reply)
{
    m_appsPythonInvoker.diagYesNoReply(reply);
}

/**
 * @brief AM_AppsEngine::onInputReply
 * @param appId
 * @param reply
 */
void KvalAppManager::onInputReply(QString appId, QString reply)
{
    (void)appId;
    m_appsPythonInvoker.inputReply(reply);
}

/**
 * @brief AM_AppsEngine::onInputListReply
 * @param appId
 * @param reply
 */
void KvalAppManager::onInputListReply(QString appId, int reply)
{
    (void)appId;
    m_appsPythonInvoker.inputListReply(reply);
}

/**
 * @brief AM_AppsEngine::onRequestAppAbort
 */
void KvalAppManager::onRequestAppAbort()
{
    //Invalidate the current handler
    constructArgs("");

    //Call for abort
    m_invokerInterface->call(QDBus::NoBlock, "StopAppProc", m_activeAppId);
}

/**
 * @brief AM_AppsEngine::onAddStoreAddress
 */
void KvalAppManager::onAddStoreAddress(QString address)
{
    m_invokerInterface->call(QDBus::NoBlock,
                             "set_appsstore_server_address",
                             address);
}

/**
 * @brief AM_AppsEngine::onUninstallApp
 */
void KvalAppManager::onUninstallApp(QString appId)
{
    m_invokerInterface->call(QDBus::NoBlock, "uninstallApplication", appId);
}

/**
 * @brief AM_AppsEngine::onUpdateApp
 */
void KvalAppManager::onUpdateApp(QString appId)
{
    m_invokerInterface->call(QDBus::NoBlock, "updateApplication", appId, constructArgs(""));
    Q_EMIT appHasStarted();
}

/**
 * @brief AM_AppsEngine::onAddAppToFav
 */
void KvalAppManager::onAddAppToFav(QString appId)
{
    constructArgs("");
    m_invokerInterface->call(QDBus::NoBlock,
                             "favoriteApp",
                             appId,
                             true,
                             m_current_handler);
}

/**
 * @brief AM_AppsEngine::onRemoveAppFromFav
 */
void KvalAppManager::onRemoveAppFromFav(QString appId)
{
    constructArgs("");
    m_invokerInterface->call(QDBus::NoBlock,
                             "favoriteApp",
                             appId,
                             false,
                             m_current_handler);
}

/**
 * @brief AM_AppsEngine::runPlugin
 * @param path
 */
void KvalAppManager::runPlugin(QString path)
{
    m_invokerInterface->call(QDBus::NoBlock,
                             "launchApp",
                             m_activeAppId,
                             constructArgs(path));
    Q_EMIT appHasStarted();
}

/**
 * @brief AM_AppsEngine::installApp
 * @param appId
 * @param appUrl
 * @param appHash
 */
void KvalAppManager::installApp(QString appId, QString appUrl, QString appVersion, QString appHash)
{
    m_invokerInterface->call(QDBus::NoBlock,
                             "installApplication",
                             appId, appUrl, appVersion, appHash);
}

/**
 * @brief AM_AppsEngine::launchApplication
 * @param appId
 * @param args
 */
void KvalAppManager::launchApplication(QString appId, QString args)
{
    m_activeAppId = appId;
    QString constracted_args = constructArgs(args);
    LOG_INFO(LOG_TAG,
             "launch application [%s] with args [%s] ...",
             qPrintable(appId),
             qPrintable(constracted_args));

    Q_EMIT appHasStarted();
    m_invokerInterface->call(QDBus::NoBlock,
                             "launchApp",
                             appId,
                             constracted_args);
}

/**
 * @brief AM_AppsEngine::run
 */
void KvalAppManager::run()
{
    LOG_INFO(LOG_TAG, "Run AM_AppsEngine Thread...");
    exec();
}

/**
 * @brief AM_AppsEngine::onRefreshInstalledApps
 */
void KvalAppManager::onRefreshInstalledApps()
{
    m_totalCount = 0;
    m_installedApps.clear();
    m_extractedItems = 0;
}

/**
 * @brief AM_AppsEngine::onInstalledAppsAdd
 * @param app
 * @param totalItems
 */
void KvalAppManager::onInstalledAppsAdd(QMap<QString, QString> app,
                                             int totalAppsCount)
{
    INV();
    m_totalCount = totalAppsCount;

    LOG_INFO(LOG_TAG, "Add app: %s", qPrintable(app["ui_name"]));
    m_installedApps.append(app);
    if(m_totalCount)
    {
        m_extractedItems = m_extractedItems+ 1;
    }
    if(m_extractedItems == m_totalCount)
    {
        LOG_INFO(LOG_TAG, "Got all apps");
        m_extractedItems = 0;
    }

    OUTV();
}

/**
 * @brief AM_AppsEngine::onItemAdd
 * @param item
 * @param totalAppsCount
 */
void KvalAppManager::onItemAdd(QMap<QString, QString> item,
                                    int totalAppsCount)
{
    INV();
    if (!m_extractedItems)
    {
        //Cleanup previous session data
        m_categoryItems.clear();
    }
    m_totalCount = totalAppsCount;

    LOG_DEBUG(LOG_TAG, "Add Item: %s", qPrintable(item["label"]));

    QVariantMap rval;
    Q_FOREACH (QString key, item.keys())
    {
        rval[key] = item[key];
    }
    m_categoryItems.append(rval);

    if(m_totalCount)
    {m_extractedItems = m_extractedItems + 1;
    }
    if(m_extractedItems == m_totalCount)
    {
        LOG_INFO(LOG_TAG, "Got all items");
        m_extractedItems = 0;
    }

    OUTV();
}

/**
 * @brief AM_AppsEngine::addMenuItems
 * @param appId
 */
void KvalAppManager::addMenuItems(QString appId, QMap<QString, QString> menuDict)
{
    (void)appId;
    QVariantMap rval;
    Q_FOREACH (QString key, menuDict.keys())
    {
        rval[key] = menuDict[key];
    }
    m_appItemMenuSettings.append(rval);
}

/**
 * @brief AM_AppsEngine::addItemSetting
 * @param item
 * @param count
 */
void KvalAppManager::addItemSetting(QString men, QMap<QString, QString> item, int count)
{
    INV();
    static unsigned int recvCount = 0;
    static unsigned int totalCount = 0;
    static QList<QMap<QString, QString> > _appTmpItemSettings;

    if (!recvCount)
    {
        LOG_INFO(LOG_TAG, "recvCount cleared");
        _appTmpItemSettings.clear();
        m_appItemSettings.clear();
        totalCount = count;
        m_menuEnum = men;
    }

    _appTmpItemSettings.append(item);

    if(totalCount)
    {
        recvCount = recvCount + 1;
    }
    if(recvCount == totalCount)
    {
        LOG_INFO(LOG_TAG, "Got all settings items, m_menuEnum: %s", qPrintable(m_menuEnum));
        recvCount = 0;
        int cat_counter = 0;
        QVariantMap setting;
        QVariantList categories;
        for(int i=0; i < _appTmpItemSettings.count(); i++)
        {
            if(!cat_counter)
            {
                setting.clear();
                categories.clear();
                LOG_INFO(LOG_TAG, "ms::%s", qPrintable(_appTmpItemSettings[i]["ms"]));
                LOG_INFO(LOG_TAG, "cat::%s", qPrintable(_appTmpItemSettings[i]["cat"]));
                setting["ms"] = _appTmpItemSettings[i]["ms"];
                setting["label"] = _appTmpItemSettings[i]["msLabel"];
            }
            cat_counter = cat_counter + 1;

            QVariantMap rval;
            Q_FOREACH (QString key, _appTmpItemSettings[i].keys())
            {
                if( !key.compare("ms") ||
                    !key.compare("msLabel") ||
                    !key.compare("msorder") ||
                    !key.compare("order") ||
                    !key.compare("catCount")
                    )
                {
                    continue;
                }
                rval[key] = _appTmpItemSettings[i][key];
            }
            categories.append(rval);
            if(cat_counter == _appTmpItemSettings[i]["catCount"].toInt())
            {
                cat_counter = 0;
                setting["settings"] = categories;
                m_appItemSettings.append(setting);
            }
        }
        LOG_INFO(LOG_TAG, "count settingtree: %d", m_appItemSettings.count());
    }

    OUTV();
}

/**
 * @brief AM_AppsEngine::initAppWindow
 * @param appId
 * @return
 */
bool KvalAppManager::initAppSettingWindow(QString appId)
{
    if(!m_registredWindowAppIds.contains(appId))
    {
        return false;
    }

    m_appItemMenuSettings.clear();
    m_appItemSettings.clear();
    m_binderInvokerInterface->call(QDBus::NoBlock,"onInitSettingWindow", appId);
    return true;
}

/**
 * @brief AM_AppsEngine::focusAppSettingWindow
 * @param appId
 * @param moduleId
 * @param menuEnum
 * @return
 */
bool KvalAppManager::focusAppSettingWindow(QString appId, QString moduleId, QString menuEnum)
{
    if(!m_registredWindowAppIds.contains(appId))
    {
        return false;
    }

    m_appItemMenuSettings.clear();
    m_appItemSettings.clear();
    m_binderInvokerInterface->call(QDBus::NoBlock,"onFocusSettingWindow",appId, moduleId, menuEnum);
    return true;
}

/**
 * @brief AM_AppsEngine::actionAppSettingClick
 * @param appId
 * @param moduleId
 * @param catParentId
 * @param catId
 * @return
 */
bool KvalAppManager::actionAppSettingClick(QString appId,
                                           QString moduleId,
                                           QString catId,
                                           QString entryId,
                                           QString men)
{
    if(!m_registredWindowAppIds.contains(appId))
    {
        return false;
    }

    m_binderInvokerInterface->call(QDBus::NoBlock,
                                   "onActionSettingWindow",
                                   appId,
                                   moduleId,
                                   catId,
                                   entryId,
                                   men);
    return true;
}

/**
 * @brief AM_AppsEngine::checkServiceMenuCode
 * @return
 */
bool KvalAppManager::checkServiceMenuCode(QString entry)
{
#ifdef KVAL_SECURE
    return m_encSecureEngine->cckSrv(entry);
#else
    (void)entry;
    return true;
#endif
}

/**
 * @brief AM_AppsEngine::customUiTemplateNotif
 * @param uiTemplate
 */
void KvalAppManager::customUiTemplateNotif(QString uiTemplate)
{
    m_appCustomUiTemplate = uiTemplate;
    Q_EMIT customUiTemplateSig(uiTemplate);
}

/**
 * @brief AM_AppsEngine::getSettingItems
 * @return
 */
QVariantList KvalAppManager::getSettingItems(QString men)
{
    if(m_menuEnum.compare(men))
    {
        LOG_ERROR(LOG_TAG,
                  "menu enum missmatch: m_menuEnum:%s Vs men:%s",
                  qPrintable(m_menuEnum),
                  qPrintable(men));
        QVariantList dummy;
        return dummy;
    }
    return m_appItemSettings;
}

/**
 * @brief AM_AppsEngine::getSettingMenuItems
 * @return
 */
QVariantList KvalAppManager::getSettingMenuItems()
{
    return m_appItemMenuSettings;
}

/**
 * @brief AM_AppsEngine::getItemsCount
 * @return
 */
int KvalAppManager::getItemsCount()
{
    INV();
    int count = m_categoryItems.count();
    OUTV("Items count: %d", count);
    return count;
}

/**
 * @brief AM_AppsEngine::getItemsMap
 * @param index
 * @return
 */
QVariantMap KvalAppManager::getItemsMap(int index)
{
    INV("index: %d", index);

    return qvariant_cast<QVariantMap>(m_categoryItems.at(index));
}

/**
 * @brief AM_AppsEngine::getItems
 * @return
 */
QVariantList KvalAppManager::getItems()
{
    return m_categoryItems;
}

/**
 * @brief AM_AppsEngine::getInstalledAppsCount
 * @return
 */
int KvalAppManager::getInstalledAppsCount()
{
    INV();
    int count = m_installedApps.count();
    OUTV("installed apps count: %d", count);
    return count;
}

/**
 * @brief AM_AppsEngine::getInstalledAppsByIndex
 * @param index
 * @return
 */
QVariantMap KvalAppManager::getInstalledAppsByIndex(int index)
{
    INV("index: %d", index);

    QVariantMap rval;
    QMap<QString, QString> tempmap = m_installedApps.at(index);
    Q_FOREACH (QString key, tempmap.keys())
    {
        rval[key] = tempmap[key];
    }
    OUTV();
    return rval;
}

/**
 * @brief AM_AppsEngine::onStartAppsDaemonNotify
 */
void KvalAppManager::onStartAppsDaemonNotify()
{
    INV();
    m_invokerInterface->call(QDBus::NoBlock,"startAppsBinderDeamon");
    OUTV();
}

/**
 * @brief AM_AppsEngine::onSetAppsStoreAddress
 * @param srvAddr
 */
void KvalAppManager::onSetAppsStoreAddress(QString srvAddr)
{
    INV("Live Tv server Address: %s", qPrintable(srvAddr));
    m_invokerInterface->call(QDBus::NoBlock,
                            "set_appsstore_server_address",
                             srvAddr);
    OUTV();
}

/**
 * @brief AM_AppsElement::AM_AppsElement
 */
KvalAppManagerElement::KvalAppManagerElement()
{
    INV();
    LOG_INFO(LOG_TAG, "Instantiate AM_AppsElement");
    m_appsNotifEngine = new KvalAppManager(this);
    g_appEngineRef = m_appsNotifEngine;
    m_appsNotifEngine->start();
    QMetaObject::invokeMethod(m_appsNotifEngine, "checkInvokerInterface");

    connect(m_appsNotifEngine,
            SIGNAL(displayAppsMsgSig(QString, QString)),
            this,
            SIGNAL(displayAppsMsgSig(QString, QString)));
    connect(m_appsNotifEngine,
            SIGNAL(endOfCategory(bool)),
            this,
            SIGNAL(endOfCategory(bool)));
    connect(m_appsNotifEngine,
            SIGNAL(endOfSettingsEntries(bool)),
            this,
            SIGNAL(endOfSettingsEntries(bool)));
    connect(m_appsNotifEngine,
            SIGNAL(okDiag(QString, QString)),
            this,
            SIGNAL(okDiag(QString, QString)));
    connect(m_appsNotifEngine,
            SIGNAL(yesNoDiag(QString, QString, QString, QString)),
            this,
            SIGNAL(yesNoDiag(QString, QString, QString, QString)));
    connect(m_appsNotifEngine,
            SIGNAL(diagProgressCreate(QString, QString)),
            this,
            SIGNAL(diagProgressCreate(QString, QString)));
    connect(m_appsNotifEngine,
            SIGNAL(diagProgressUpdate(int, QString)),
            this,
            SIGNAL(diagProgressUpdate(int, QString)));
    connect(m_appsNotifEngine,
            SIGNAL(diagProgressClose()),
            this,
            SIGNAL(diagProgressClose()));
    connect(m_appsNotifEngine,
            SIGNAL(setResUrl(QVariantMap)),
            this,
            SIGNAL(setResUrl(QVariantMap)));
    connect(m_appsNotifEngine,
            SIGNAL(inputKeyboard(QString, QString, bool)),
            this,
            SIGNAL(inputKeyboard(QString, QString, bool)));
    connect(m_appsNotifEngine,
            SIGNAL(appHasStopped()),
            this,
            SIGNAL(appHasStopped()));
    connect(m_appsNotifEngine,
            SIGNAL(appHasStarted()),
            this,
            SIGNAL(appHasStarted()));
    connect(m_appsNotifEngine,
            SIGNAL(inputList(QString, QStringList)),
            this,
            SIGNAL(inputList(QString, QStringList)));
    connect(m_appsNotifEngine,
            SIGNAL(customUiTemplateSig(QString)),
            this,
            SIGNAL(customUiTemplateSig(QString)));

    OUTV();
}

/**
 * @brief AM_AppsElement::~AM_AppsElement
 */
KvalAppManagerElement::~KvalAppManagerElement()
{
    LOG_INFO(LOG_TAG, "Delete AM_AppsElement...");
    if(m_appsNotifEngine)
    {
        m_appsNotifEngine->quit();
        m_appsNotifEngine->wait();
        delete m_appsNotifEngine;
    }
}

/**
 * @brief AM_AppsElement::langChanged
 */
void KvalAppManagerElement::langChanged()
{
    QMetaObject::invokeMethod(m_appsNotifEngine, "onLangChanged");
}

/**
 * @brief AM_AppsElement::launchApp
 * @param appId
 * @param args
 */
void KvalAppManagerElement::launchApp(QString appId, QString args)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "launchApplication",
                              Q_ARG(QString, appId),
                              Q_ARG(QString, args));
}

/**
 * @brief AM_AppsElement::diagYesNoReply
 * @param reply
 */
void KvalAppManagerElement::diagYesNoReply(bool reply)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onDiagYesNoReply",
                              Q_ARG(bool, reply));
}

/**
 * @brief AM_AppsElement::inputReply
 * @param appId
 * @param reply
 */
void KvalAppManagerElement::inputReply(QString appId, QString reply)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onInputReply",
                              Q_ARG(QString, appId),
                              Q_ARG(QString, reply));
}

/**
 * @brief AM_AppsElement::inputListReply
 * @param appId
 * @param reply
 */
void KvalAppManagerElement::inputListReply(QString appId, int reply)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onInputListReply",
                              Q_ARG(QString, appId),
                              Q_ARG(int, reply));
}

/**
 * @brief AM_AppsElement::notifyBuiltinAction
 * @param action
 */
void KvalAppManagerElement::notifyBuiltinAction(QString action, QString appId)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onExecuteFunc",
                              Q_ARG(QString, action),
                              Q_ARG(QString, appId),
                              Q_ARG(bool, false));
}

/**
 * @brief AM_AppsElement::requestAppAbort
 */
void KvalAppManagerElement::requestAppAbort()
{
    QMetaObject::invokeMethod(m_appsNotifEngine, "onRequestAppAbort");
}

/**
 * @brief AM_AppsElement::addStoreAddress
 * @param address
 */
void KvalAppManagerElement::addStoreAddress(QString address)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onAddStoreAddress",
                              Q_ARG(QString, address));
}

/**
 * @brief AM_AppsElement::uninstallApp
 * @param appId
 */
void KvalAppManagerElement::uninstallApp(QString appId)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onUninstallApp",
                              Q_ARG(QString, appId));
}

/**
 * @brief AM_AppsElement::updateApp
 * @param appId
 */
void KvalAppManagerElement::updateApp(QString appId)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onUpdateApp",
                              Q_ARG(QString, appId));
}

/**
 * @brief AM_AppsElement::addAppToFav
 * @param appId
 */
void KvalAppManagerElement::addAppToFav(QString appId)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onAddAppToFav",
                              Q_ARG(QString, appId));
}

/**
 * @brief AM_AppsElement::removeAppFromFav
 * @param appId
 */
void KvalAppManagerElement::removeAppFromFav(QString appId)
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onRemoveAppFromFav",
                              Q_ARG(QString, appId));
}


/**
 * @brief AM_AppsElement::abortProgressDiag
 */
void KvalAppManagerElement::abortProgressDiag()
{
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onAbortProgressDiag");
}

/**
 * @brief AM_AppsElement::getItemsCount
 * @return
 */
int KvalAppManagerElement::getItemsCount()
{
    return m_appsNotifEngine->getItemsCount();
}

bool KvalAppManagerElement::isActivePlayList()
{
    return m_appsNotifEngine->isActivePlayList();
}

/**
 * @brief AM_AppsElement::getNextPlayListItem
 * @return
 */
QVariantMap KvalAppManagerElement::getNextPlayListItem()
{
    return m_appsNotifEngine->getNextPlayListItem();
}

/**
 * @brief AM_AppsElement::getCurrentPlayListItem
 * @return
 */
QVariantMap KvalAppManagerElement::getCurrentPlayListItem()
{
    return m_appsNotifEngine->getCurrentPlayListItem();
}
/**
 * @brief AM_AppsElement::waitForUi
 */
void KvalAppManagerElement::waitForUi()
{
    m_appsNotifEngine->waitForUi();
}

/**
 * @brief AM_AppsElement::playNext
 */
void KvalAppManagerElement::playNext()
{
    m_appsNotifEngine->playNext();
}

/**
 * @brief AM_AppsElement::playListUiAborted
 */
void KvalAppManagerElement::playListUiAborted()
{
    m_appsNotifEngine->playListUiAborted();
}

/**
 * @brief AM_AppsElement::initSettingWindow
 * @param appId
 * @return
 */
bool KvalAppManagerElement::initSettingWindow(QString appId)
{
    return m_appsNotifEngine->initAppSettingWindow(appId);
}

/**
 * @brief AM_AppsElement::focusAppSettingWindow
 * @param appId
 * @param moduleId
 * @param menuEnum
 * @return
 */
bool KvalAppManagerElement::focusAppSettingWindow(QString appId, QString moduleId, QString menuEnum)
{
    return m_appsNotifEngine->focusAppSettingWindow(appId, moduleId, menuEnum);
}

/**
 * @brief AM_AppsElement::actionClick
 * @param appId
 * @param moduleId
 * @param catParentId
 * @param catId
 * @return
 */
bool KvalAppManagerElement::actionAppSettingClick(QString appId,
                                 QString moduleId,
                                 QString catId,
                                 QString entryId,
                                 QString men)
{
    return m_appsNotifEngine->actionAppSettingClick(appId, moduleId, catId, entryId, men);
}

/**
 * @brief AM_AppsElement::checkServiceMenuCode
 * @param entry
 * @return
 */
bool KvalAppManagerElement::checkServiceMenuCode(QString entry)
{
    return m_appsNotifEngine->checkServiceMenuCode(entry);
}

/**
 * @brief AM_AppsElement::customUiTemplateReady
 * @return
 */
void KvalAppManagerElement::customUiTemplateReady(bool status)
{
    return m_appsNotifEngine->customUiTemplateStChanged((status) ?
                                                        APP_UI_STATUS_LOADED :
                                                        APP_UI_STATUS_ERROR);
}

/**
 * @brief AM_AppsElement::clearCustomTemplate
 */
void KvalAppManagerElement::clearCustomTemplate()
{
    return m_appsNotifEngine->clearCustomTemplate();
}

/**
 * @brief AM_AppsElement::getAppsMap
 * @param index
 * @return
 */
QVariantMap KvalAppManagerElement::getItemsMap(int index)
{
    return m_appsNotifEngine->getItemsMap(index);
}

/**
 * @brief AM_AppsElement::getItems
 * @return
 */
QVariantList KvalAppManagerElement::getItems()
{
    return m_appsNotifEngine->getItems();
}

/**
 * @brief AM_AppsElement::getSettingItems
 * @return
 */
QVariantList KvalAppManagerElement::getSettingItems(QString men)
{
    return m_appsNotifEngine->getSettingItems(men);
}

/**
 * @brief AM_AppsElement::getSettingMenuItems
 * @return
 */
QVariantList KvalAppManagerElement::getSettingMenuItems()
{
    return m_appsNotifEngine->getSettingMenuItems();
}

/**
 * @brief AM_AppsElement::getInstalledAppsCount
 * @return
 */
int KvalAppManagerElement::getInstalledAppsCount()
{
    return m_appsNotifEngine->getInstalledAppsCount();
}

/**
 * @brief AM_AppsElement::getAppsMap
 * @param index
 * @return
 */
QVariantMap KvalAppManagerElement::getAppsMap(int index)
{
    return m_appsNotifEngine->getInstalledAppsByIndex(index);
}

/**
 * @brief AM_AppsElement::setAppsStoreAddress
 * @param srvAddr
 */
void KvalAppManagerElement::setAppsStoreAddress(QString srvAddr)
{
    INV("VOD server Address: %s", qPrintable(srvAddr));
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onSetAppsStoreAddress",
                              Q_ARG(QString, srvAddr));
    OUTV();
}

/**
 * @brief AM_AppsElement::startClientDaemon
 */
void KvalAppManagerElement::startAppsDaemon()
{
    INV();
    QMetaObject::invokeMethod(m_appsNotifEngine,
                              "onStartAppsDaemonNotify");

    OUTV();
}
