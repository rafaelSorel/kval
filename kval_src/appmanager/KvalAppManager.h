#ifndef KVAL_APPSMANAGER_H
#define KVAL_APPSMANAGER_H

#include <atomic>
#include <QThread>
#include <QObject>
#include <QtCore/QObject>
#include <QProcess>
#include <QMap>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusInterface>
#include <QDBusArgument>

#include "player/KvalAppPlayerWrapper.h"
#include "KvalAppEmbedFuncs.h"
#include "KvalThreadUtils.h"

class KvalAppManagerElement;
class KvalAppManager;
extern KvalAppManager * g_appEngineRef;

typedef enum AppsEngineCustomUiStatus {
    APP_UI_STATUS_UNKNOWN = 0,
    APP_UI_STATUS_LOADED,
    APP_UI_STATUS_ERROR,
    APP_UI_STATUS_MAX
} AppsEngineCustomUiStatus;

/**
 * @brief The SC_SubsClientPythonInvoker class
 */
class KvalAppManagerBridge: public QObject
{
    Q_OBJECT

public:

    KvalAppManagerBridge();
    virtual ~KvalAppManagerBridge() {}

    static void appsStopScriptTask(void);
    void setOwner(KvalAppManager * owner);
    void abortProgressDiag(){ m_progressDiagIsAborted=true; }
    void diagYesNoReply(bool reply){ m_yesNoReply = reply; }
    void diagDecryptReply(bool reply){ m_decReply = reply; }
    void execFunctionReply(bool reply){ m_execFunctionStatus = reply; }
    void inputReply(QString reply)
    {
        m_keyboardReply = reply;
        m_inputHasReply = true;
    }
    void inputListReply(int reply)
    {
        m_listReply = reply;
        m_inputHasReply = true;
    }


Q_SIGNALS:
    void displayAppsMsgSig(QString, QString);
    void addInstalledApps(QMap<QString, QString> app, int totalAppsCount);
    void addItem(QMap<QString, QString> app, int totalAppsCount);
    void setResUrl(QVariantMap itemInfo);
    void endOfCategory(bool succeeded);
    void okDiag(QString, QString);
    void yesNoDiag(QString, QString, QString, QString);
    void diagProgressCreate(QString, QString);
    void diagProgressUpdate(int, QString);
    void diagProgressClose();
    void executeFunc(QString, QString, bool);
    void inputKeyboard(QString, QString, bool);
    void inputList(QString,QStringList);
    void appHasStopped();
    void refreshInstalledApps();
    void endOfSettingsEntries(bool succeeded);

public Q_SLOTS: //Appliations Core Slots
    Q_SCRIPTABLE void add_installed_apps(const QMap<QString, QString> &argument, int count);
    Q_SCRIPTABLE void refresh_installed_apps();
    Q_SCRIPTABLE void end_of_script(const int &handle);
    Q_SCRIPTABLE QString get_language();
    Q_SCRIPTABLE QString get_srv_addr();
    Q_SCRIPTABLE void execute_func(const QString &function,const bool &needStatus);
    Q_SCRIPTABLE int get_exec_func_status();
    Q_SCRIPTABLE QList<QMap<QString, QString>> get_removable_devices();
    Q_SCRIPTABLE void register_window_setting_diag(const QString &appId);
    Q_SCRIPTABLE void add_menu_item_setting(const QString &appId,
                                            const QMap<QString, QString> &argument);
    Q_SCRIPTABLE void add_config_item_setting(const QString &appId,
                                              const QString &men,
                                              const QMap<QString, QString> &argument,
                                              const int &count);

    Q_SCRIPTABLE void end_of_entries(const QString &appId, bool succeeded);
    Q_SCRIPTABLE void decrypt_cmd(const QString &sig,
                                  const QString &keypath,
                                  const QString &src,
                                  const QString &dst);
    Q_SCRIPTABLE int get_decrypt_reply();
    Q_SCRIPTABLE void notify_custom_ui_template(const QString &uiTemplate);
    Q_SCRIPTABLE int get_ui_template_status(const QString &uiTemplate);

public Q_SLOTS: //application Ui Slots
    Q_SCRIPTABLE void add_item(const int &handle,
                               const QMap<QString, QString> &argument,
                               int count);
    Q_SCRIPTABLE void displayMsgUi(const QString &severity,
                                   const QString &payload);
    Q_SCRIPTABLE void end_of_category( const int &handle, bool succeeded );

    Q_SCRIPTABLE void ok_diag(const QString &title,
                              const QString &text);
    Q_SCRIPTABLE void yesno_diag(const QString &title,
                                   const QString &text,
                                   const QString &nolabel,
                                   const QString &yeslabel);
    Q_SCRIPTABLE void diag_progress_create(const QString &header,
                                           const QString &text);
    Q_SCRIPTABLE bool diag_progress_update(const int &position,
                                           const QString &text);
    Q_SCRIPTABLE void diag_progress_close();
    Q_SCRIPTABLE int get_yesno_reply();
    Q_SCRIPTABLE void input_keyboard(const QString &title,
                                     const QString &defaultTxt,
                                     const bool &hidden);
    Q_SCRIPTABLE void input_list(const QString &title,
                                 const QStringList &items);
    Q_SCRIPTABLE QString get_input_keyboard_result();
    Q_SCRIPTABLE int get_input_list_result();
    Q_SCRIPTABLE bool input_has_reply();

public Q_SLOTS: //application PP_MediaPlayer Slots
    Q_SCRIPTABLE void mediaplayer_play(const QString &source,
                                       const QMap<QString, QString> &argument,
                                       const bool &windowed,
                                       const int &startpos);
    Q_SCRIPTABLE void mediaplayer_activateplaylist();
    Q_SCRIPTABLE void mediaplayer_stop();
    Q_SCRIPTABLE void mediaplayer_pause();
    Q_SCRIPTABLE void mediaplayer_playnext();
    Q_SCRIPTABLE void mediaplayer_playprevious();
    Q_SCRIPTABLE void mediaplayer_playselected(const int &selected);
    Q_SCRIPTABLE bool mediaplayer_isplaying();
    Q_SCRIPTABLE bool mediaplayer_isplayingaudio();
    Q_SCRIPTABLE bool mediaplayer_isplayingvideo();
    Q_SCRIPTABLE QString mediaplayer_filename();
    Q_SCRIPTABLE int mediaplayer_streamposition();
    Q_SCRIPTABLE void mediaplayer_seek(const int &seekTime);
    Q_SCRIPTABLE void mediaplayer_setsubsfile(const QString &path);
    Q_SCRIPTABLE int mediaplayer_streamlength();

    Q_SCRIPTABLE int get_playlist_position();
    Q_SCRIPTABLE int get_playlist_size();
    Q_SCRIPTABLE void clear_playlist();
    Q_SCRIPTABLE void shuffle_playlist(const int &index);
    Q_SCRIPTABLE void unshuffle_playlist();
    Q_SCRIPTABLE void remove_playlist_element(const int &index);
    Q_SCRIPTABLE void add_playlist_element(const QMap<QString, QString> &item, const int &index);
    Q_SCRIPTABLE void set_res_url(const int &handle, const QMap<QString, QString> &argument);


private:
    bool _check_handle(int handle);
    bool m_progressDiagIsAborted;
    int m_execFunctionStatus;
    int m_yesNoReply;
    int m_decReply;
    bool m_inputHasReply;
    int m_listReply;
    QString m_keyboardReply;
    KvalAppManager * m_owner;
};

/**
 * @brief The AM_AppsEngine class
 */
class KvalAppManager : public QThread
{
    Q_OBJECT

public:
    enum
    {
        PROC_INACTIVE = 0,
        PROC_WAIT_MAIL_ITEM_LIST,
    };

    KvalAppManager(KvalAppManagerElement * uiElement);
    ~KvalAppManager();
    static KvalAppManager * getGlobalRef() { return g_appEngineRef; }

    void run();
    int getCurrentHandler(){
        QMutexLocker locker(&m_handlerLocker);
        return m_current_handler;
    }
    bool isActivePlayList() {
        return (!m_mediaPlayList) ? false : m_mediaPlayerWrapper->isActivePlayList();
    }

    AppsEngineCustomUiStatus checkTemplateStatus(QString uiTemplate)
    {
        if(!uiTemplate.compare(m_appCustomUiTemplate))
            return static_cast<AppsEngineCustomUiStatus>(
                        m_appCustomUiTemplateStatus.loadRelaxed());
        return APP_UI_STATUS_UNKNOWN;
    }

    void clearCustomTemplate() {
        m_appCustomUiTemplate = "";
        m_appCustomUiTemplateStatus.storeRelaxed(APP_UI_STATUS_UNKNOWN);
    }

    void customUiTemplateStChanged(AppsEngineCustomUiStatus st) {
        m_appCustomUiTemplateStatus.storeRelaxed(st);
    }

    QVariantMap getNextPlayListItem() { return m_mediaPlayerWrapper->nextPlayListItem(); }
    QVariantMap getCurrentPlayListItem() { return m_mediaPlayerWrapper->currentPlayListItem(); }
    KvalAppMediaPlayerWrapper * getMediaPlayerWrapper() { return m_mediaPlayerWrapper; }
    QDBusInterface * getBinderInvoker() { return m_binderInvokerInterface; }
    KvalAppMediaPlayList * getPlayList() { return m_mediaPlayList; }
    void waitForUi() { m_mediaPlayerWrapper->waitForUiGo(); }
    void playNext() { m_mediaPlayerWrapper->playNext(); }
    void playListUiAborted() { m_mediaPlayerWrapper->onUiStopRequested(); }
    void registerWindowSettingDiag(QString appId) { m_registredWindowAppIds.append(appId); }
    bool checkWindowSettingDiagAppId(QString appId)
    {
        if (!m_registredWindowAppIds.contains(appId))
            return false;
        return true;
    }

Q_SIGNALS:
    void displayAppsMsgSig(QString, QString);
    void endOfCategory ( bool succeeded );
    void okDiag(QString, QString);
    void yesNoDiag(QString, QString, QString, QString);
    void diagProgressCreate(QString, QString);
    void diagProgressUpdate(int, QString);
    void diagProgressClose();
    void setResUrl(QVariantMap itemInfo);
    void inputKeyboard(QString, QString, bool);
    void appHasStopped();
    void inputList(QString, QStringList);
    void appHasStarted();
    void endOfSettingsEntries(bool succeeded);
    void customUiTemplateSig(QString uiTemplate);

public Q_SLOTS:
    void tryReboot();
    void decryptAppCmd( QString sig, QString keypath, QString src, QString dst);
    void onSetAppsStoreAddress(QString);
    void onStartAppsDaemonNotify();
    void onInstalledAppsAdd(QMap<QString, QString> app, int totalAppsCount);
    void onItemAdd(QMap<QString, QString> item, int totalAppsCount);
    int getInstalledAppsCount();
    int getItemsCount();
    QVariantList getItems();
    QVariantList getSettingItems(QString men);
    QVariantList getSettingMenuItems();
    QVariantMap getInstalledAppsByIndex(int index);
    QVariantMap getItemsMap(int index);
    void checkInvokerInterface();
    void launchApplication(QString appId, QString args);
    void onAbortProgressDiag();
    void onDiagYesNoReply(bool reply);
    void onExecuteFunc(QString execStr, QString appId, bool needStatus);
    void onInputReply(QString appId, QString reply);
    void onInputListReply(QString appId, int reply);
    void onRequestAppAbort();
    void onAddStoreAddress(QString);
    void onUninstallApp(QString);
    void onUpdateApp(QString);
    void onAddAppToFav(QString);
    void onRemoveAppFromFav(QString);
    void onRefreshInstalledApps();
    void onLangChanged();
    void addMenuItems(QString appId, QMap<QString, QString> menuDict);
    void addItemSetting(QString men, QMap<QString, QString> item, int count);
    bool initAppSettingWindow(QString appId);
    bool focusAppSettingWindow(QString appId, QString moduleId, QString menuEnum);
    bool actionAppSettingClick(QString, QString, QString, QString, QString);
    bool checkServiceMenuCode(QString);
    void customUiTemplateNotif(QString);


    //Player wrapper slots
    void onUiStopRequested();
    void onPlaybackCompleted();
    void onPlaybackFailed();
    void onPlaybackStarted();

    //Built in app function
    void runPlugin(QString path);
    void installApp(QString appId, QString appUrl, QString appVersion, QString appHash);

private:
    QString constructArgs(QString arg);
    int m_current_handler;
    QStringList m_registredWindowAppIds;

    QString m_activeAppId;
    QTimer * m_startAppsBinderTimer;
    KvalAppManagerBridge m_appsPythonInvoker;
    QDBusInterface * m_invokerInterface;
    QDBusInterface * m_binderInvokerInterface;
    unsigned int m_totalCount;
    unsigned int m_extractedItems;
    bool m_execNeedStatus;
    QList<QMap<QString, QString> > m_installedApps;
    QVariantList m_categoryItems;
    QVariantList m_appItemSettings;
    QString m_menuEnum;
    QVariantList m_appItemMenuSettings;
    EF_EmbedFuncs * m_appsBuiltinFuncs;
    KvalAppManagerElement * m_uiElement;
    KvalAppMediaPlayList * m_mediaPlayList;
    KvalAppMediaPlayerWrapper * m_mediaPlayerWrapper;
#ifdef KVAL_SECURE
    SecureEngine * m_encSecureEngine;
#endif
    QMutex m_handlerLocker;
    QString m_appCustomUiTemplate;
    QAtomicInt m_appCustomUiTemplateStatus{APP_UI_STATUS_UNKNOWN};
};
typedef KvalAppManager APPS_Engine;

/**
 * @brief The AM_AppsElement class
 */
class KvalAppManagerElement : public QObject
{
    Q_OBJECT
public:
    KvalAppManagerElement();
    ~KvalAppManagerElement();

    Q_INVOKABLE void startAppsDaemon(void);
    Q_INVOKABLE void setAppsStoreAddress(QString);
    Q_INVOKABLE int getInstalledAppsCount(void);
    Q_INVOKABLE QVariantMap getAppsMap(int index);
    Q_INVOKABLE int getItemsCount(void);
    Q_INVOKABLE void langChanged();
    Q_INVOKABLE QVariantMap getItemsMap(int index);
    Q_INVOKABLE QVariantList getSettingItems(QString men);
    Q_INVOKABLE QVariantList getSettingMenuItems();
    Q_INVOKABLE void launchApp(QString, QString);
    Q_INVOKABLE void abortProgressDiag();
    Q_INVOKABLE void diagYesNoReply(bool);
    Q_INVOKABLE void inputReply(QString, QString);
    Q_INVOKABLE void inputListReply(QString, int);
    Q_INVOKABLE void notifyBuiltinAction(QString action, QString appId="");
    Q_INVOKABLE void requestAppAbort();
    Q_INVOKABLE void addStoreAddress(QString);
    Q_INVOKABLE void uninstallApp(QString);
    Q_INVOKABLE void updateApp(QString);
    Q_INVOKABLE void addAppToFav(QString);
    Q_INVOKABLE void removeAppFromFav(QString);
    Q_INVOKABLE bool isActivePlayList();
    Q_INVOKABLE QVariantMap getNextPlayListItem();
    Q_INVOKABLE QVariantMap getCurrentPlayListItem();
    Q_INVOKABLE void waitForUi();
    Q_INVOKABLE void playNext();
    Q_INVOKABLE void playListUiAborted();
    Q_INVOKABLE bool initSettingWindow(QString);
    Q_INVOKABLE bool focusAppSettingWindow(QString appId, QString moduleId, QString menuEnum);
    Q_INVOKABLE bool actionAppSettingClick(QString, QString, QString, QString, QString);
    Q_INVOKABLE bool checkServiceMenuCode(QString);
    Q_INVOKABLE void clearCustomTemplate();
    Q_INVOKABLE void customUiTemplateReady(bool);
    Q_INVOKABLE QVariantList getItems();

Q_SIGNALS:
    void startAppsDaemonNotify();
    void displayAppsMsgSig(QString header, QString body);
    void endOfCategory(bool succeeded);
    void okDiag(QString title, QString text);
    void yesNoDiag(QString title,
                   QString text,
                   QString nolabel,
                   QString yeslabel);
    void diagProgressCreate(QString title, QString text);
    void diagProgressUpdate(int position, QString text);
    void diagProgressClose();
    void setResUrl(QVariantMap itemInfo);
    void inputKeyboard(QString title, QString defaultTxt, bool hidden);
    void inputList(QString title, QStringList items);
    void appHasStarted();
    void appHasStopped();
    void endOfSettingsEntries(bool succeeded);
    void customUiTemplateSig(QString uiTemplate);

    //Builtin functions signals
    void systemReboot();
    void refresh();
    void update(QString path, bool replace);
    void setViewMode(int mode, int submode);

private:
    //Local Attributes
    KvalAppManager * m_appsNotifEngine;
};

typedef KvalAppManagerElement UI_Element;

#endif // KVAL_APPSMANAGER_H
