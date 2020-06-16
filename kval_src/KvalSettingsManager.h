#ifndef OMX_SETTINGSENGINE_H
#define OMX_SETTINGSENGINE_H

#include <QObject>
#include <QTranslator>
#include <QFile>
#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QString>
#include <QMetaObject>
#include <QHostInfo>
#include <QtCore/QObject>
#include <QStringList>
#include <QDir>
#include <QMap>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QNetworkConfigurationManager>
#include <QTranslator>
#include <QSettings>

#include "KvalSettingsKeys.h"
#include "KvalThreadUtils.h"
#include "KvalSettingsPlatform.h"

using namespace KvalSettingsPlatform;
/**
 * @brief The SM_SettingsEngine class
 */
class KvalSettingManager: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KvalSettingManager);

public:
    static KvalSettingManager *Instance(){
        static KvalSettingManager instance;
        return &instance;
    }
    void initialize();
    void setValue(const QString&, const QVariant&);
    QVariant value(
            const QString&,
            const QString&,
            const QVariant &defaultValue = QVariant());

    QVariantList categories();
    QVariant getTimeZone();

    static void _trval(const QVariant& val, bool& res) {
        res= val.toBool();
    }
    static void _trval(const QVariant& val, QString& res) {
        res = val.toString();
    }
    static void _trval(const QVariant& val, int& res) {
        res = val.toInt();
    }


protected:
    KvalSettingManager();
    virtual ~KvalSettingManager();

Q_SIGNALS:
    void settingsManLiveUserReq(const QString&, const QString&, const QVariant&);
    void settingsManVodUserReq(const QString&, const QString&, const QVariant&);
    void settingsManNetUserReq(const QString&, const QString&, const QVariant&);
    void settingsManGenUserReq(const QString&, const QString&, const QVariant&);
    void settingsManLiveUserReply(const QString&, const QString&, const QVariant&);
    void settingsManVodUserReply(const QString&, const QString&, const QVariant&);
    void settingsManNetUserReply(const QString&, const QString&, const QVariant&);
    void settingsManGenUserReply(const QString&, const QString&, const QVariant&);
    void dynMenuReply(const QString&, const QVariantList&);
    void valueUpdated(const QString&, const QString&, const QVariant&);

    void textEntryRequest(const QString&, bool);
    void displayProgress(const QString&, qreal);
    void displayMsg(const QString&, const QString&);
    void yesNoDiag(const QString&,const QString&,const QString&,const QString&);
    void yesNoDiagUpdate(const QString&, const QString&);
    void yesNoDiagClose();
    void endSetSetting();
    void languageChanged();

public Q_SLOTS:
    void onSetValue(const QString&, const QString&, const QVariant&);
    void onSetValueFailed(const QString&, const QString&);
    void onReqAction(const QString&, const QString&, const QVariant &value = QVariant());
    void onKeyToggle(const QString&, const QString&);
    void onUserReply(const QString&, const QString&, const QVariant &);
    void onReqDynVal(const QString&, const QString&);
    void settimeZone(const QString&);
    void setscreenRes(const QString&);

private Q_SLOTS:
    void onUserActionReq(const QString&, const QString&, const QVariant&);
    void onUserMenuReq(const QString&);

    QString getboxName();
    QString getstartSince();
    QString getSoftVersion();
    QString getserialNumber();

private:
    using Dispatcher =
    QMap<QString,std::function<void(KvalSettingManager&,const QString&,
                                                        const QString&,
                                                        const QVariant&)>>;

    void updateDefaultLang(const QString&, const QString&, const QVariant&);
    void updateDefaultTimeZone(const QString&, const QString&, const QVariant&);
    void updateDefaultScreenRes(const QString&, const QString&, const QVariant&);
    void updateDateAndTime(const QString&,const QString&,const QVariant&);

    bool loadSettingsDisplay(void);
    void applyStartupValues(void);
    bool setLanguage(const QString&);

private:
    QSharedPointer<QSettings> m_settings{nullptr};
    NetworkManager m_networkManager;
    DisplayManager m_displayManager;
    TimeZoneManager m_timezone;

    Dispatcher m_actionDispatcher{};
    Dispatcher m_actionMenuDispatcher{};
    Dispatcher m_userReplyDispatcher{};

    QVariantList m_categories{};
    QElapsedTimer m_elapsedTime;
    QString m_firmwareStatusFile;
    QMutex m_settingsValueMtx;
};

/**
 * @brief The SM_SettingsElements class
 */
class KvalSettingManagerElement : public QObject
{
    Q_OBJECT

public:

    Q_INVOKABLE void refreshSettingsValue();
    Q_INVOKABLE QVariant value(const QString&,const QString&);
    Q_INVOKABLE void setValue(const QString&, const QString&, const QVariant&);
    Q_INVOKABLE void reqaction(const QString&, const QString&);
    Q_INVOKABLE void reqMenuAction(const QString&, const QString&, const QVariant&);
    Q_INVOKABLE void toggleChoice(const QString&, const QString&);
    Q_INVOKABLE void userReplyNotify(const QString&, const QString&, const QVariant&);
    Q_INVOKABLE void dynvalues(const QString&, const QString&);
    Q_INVOKABLE QVariantList getCategoiesDisplay(void);

    KvalSettingManagerElement();
    virtual ~KvalSettingManagerElement();

Q_SIGNALS:
    void dynMenuReply(const QString& key, const QVariantList& list);
    void displayProgress(const QString& msg, qreal val);
    void displayMsg(const QString& head, const QString& body);
    void yesNoDiag(const QString& title, const QString& body, const QString& no,const QString& yes);
    void yesNoDiagUpdate(const QString& title, const QString& body);
    void yesNoDiagClose();
    void endSetSetting();
    void languageChanged();
    void valueUpdated(const QString& grp, const QString& key, const QVariant& val);
    void textEntryRequest(const QString& defval, bool ishidden);

private:
    KvalSettingManager *m_settingsManager;
    KvalThread *m_settingsManagerThread;
};

#endif // OMX_SETTINGSENGINE_H
