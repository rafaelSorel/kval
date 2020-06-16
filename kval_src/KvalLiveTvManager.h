#ifndef KVALLIVETVMANAGER_H
#define KVALLIVETVMANAGER_H

#include <QObject>
#include <QFile>
#include <QThread>
#include <QMap>
#include <QDate>
#include <QXmlStreamReader>
#include <QtNetwork>

#include "KvalConfigManager.h"
#include "KvalEpgEngine.h"
#include "KvalSettingsManager.h"
#include "KvalThreadUtils.h"

class KvalLiveTvManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KvalLiveTvManager)

public:
    explicit KvalLiveTvManager(QObject *parent = nullptr);
    ~KvalLiveTvManager();

    bool isEpgAvailable();
    bool peekEpgListMode();
    QVariantMap getChEpg(const QString& chname);

Q_SIGNALS:
    void qmlEpgReady(const QVariantMap& epgdata);
    void qmlEpgEnds();
    void notifyAlert(const QString&, const QString&);

public Q_SLOTS:
    void Start();
    void Stop();
    void onUserActionReq(const QString&, const QString&, const QVariant&);
    bool processEpgFile(void);
    void onGetEpgChList(const QStringList&);

private:
    using LDispatcher =
    QMap<QString, std::function<bool(KvalLiveTvManager&, const QVariant&)>>;
    LDispatcher m_dispatcher{};

    KvalEpgEngine m_epgEngine;
    bool m_epgEnabled{false};
    int m_epgCacheSize{1};
    std::atomic_bool m_epgListMode{false};
    struct LiveTvAlerts;
    LiveTvAlerts *m_alerts;

private:
    bool toggleEpg(const QVariant& val);
    bool toggleEpgList(const QVariant&);
    bool setEpgCache(const QVariant&);
    bool usbLiveListLoad(const QVariant&);
    bool serverLiveListLoad(const QVariant&);
};


/**
 * @brief The EM_EpgEngine class
 */
class KvalLiveTvManagerElement : public QObject
{
    Q_OBJECT
public:
    KvalLiveTvManagerElement();
    ~KvalLiveTvManagerElement();

    Q_INVOKABLE QVariantMap quickGetChannelEpg(const QString& channelName);
    Q_INVOKABLE void newEpgFileAvailable();
    Q_INVOKABLE void newEpgFileReady();
    Q_INVOKABLE void extractChannelEpgList(QStringList chList);
    Q_INVOKABLE bool epgListMode();

Q_SIGNALS:
    void qmlEpgReady(const QVariantMap& epgdata);
    void notifyAlert(const QString& type, const QString& msg);
    void qmlEpgEnds();

private:
    //Local Attributes
    QString m_current_ch_name;
    QList<QString> m_dummyChannelEpgElementInfo;
    KvalLiveTvManager m_livetvManager;
    KvalThread* m_livetvManagerTh;
};

#endif // KVALLIVETVMANAGER_H
