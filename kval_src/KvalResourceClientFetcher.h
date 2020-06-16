#ifndef OMX_SERVERNOTIFICATIONENGINE_H
#define OMX_SERVERNOTIFICATIONENGINE_H
#include <QThread>
#include <QObject>
#include <QtCore/QObject>
#include <QProcess>
#include <QMap>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusInterface>

#include "KvalThreadUtils.h"

void clientStopScriptTask();

/**
 * @brief The SC_SubsClientPythonInvoker class
 */
class SC_SubsClientPythonInvoker: public QObject
{
    Q_OBJECT

public:
    static void clientStopScriptTask();

Q_SIGNALS:
    void newNotificationMsg(QString, QString, QString);
    void newFirmwareReceivedInProgress();
    void newFirmwareReceivedStatus(QString, QString, QString);
    void mailBoxInfosRecv(QStringList);
    void endOfClientProcessing();
    void newMailItem(QStringList, int);
    void displayInternalSrvNotifSignal(QString, QString);
    void mailBoxEmptyNotify();
    void notifNewXmlFileAvailableNotif(QString);

public Q_SLOTS:
    Q_SCRIPTABLE void endOfClientScript();
    Q_SCRIPTABLE void notifRecvMsg( const QString &severity,
                                    const QString &message,
                                    const QString &sendDate);
    Q_SCRIPTABLE void notifNewFirmware();
    Q_SCRIPTABLE void notifNewFirmwareExtracted(const QString &status,
                                                const QString &filename,
                                                const QString &sendDate);
    Q_SCRIPTABLE void notifMailBoxInfos(const QStringList &infos);
    Q_SCRIPTABLE void addMailItem(const QStringList &mailInfos,
                                  int totalItems);
    Q_SCRIPTABLE void internalServerNotification(const QString &notificationHeader ,
                                                const QString &notification);
    Q_SCRIPTABLE void mailBoxEmpty();
    Q_SCRIPTABLE void notifNewXmlFileAvailable(QString);
};


/**
 * @brief The SC_SubsClientEngine class
 */
class SC_SubsClientEngine : public QThread
{
    Q_OBJECT
public:
    enum
    {
        PROC_INACTIVE = 0,
        PROC_WAIT_MAIL_ITEM_LIST,
    };

    SC_SubsClientEngine();
    ~SC_SubsClientEngine();

    void run();

Q_SIGNALS:
    void newNotificationMsg(QString, QString, QString);
    void newFirmwareReceivedInProgress();
    void newFirmwareReceivedStatus(QString, QString, QString);
    void mailBoxInfosRecv(QStringList);
    void readyMailItemList(QStringList);
    void displayInternalSrvNotifSignal(QString, QString);
    void mailBoxEmptyNotify();
    void vodFileAvailable();
    void liveTvFileAvailable();
    void epgFileAvailable();
    void epgFileUpdated();
    void mainScreenVodFileAvailable();

public Q_SLOTS:
    void onStartClientDaemonNotify();
    void onExtractMailsFromCache();
    void onEndOfClientProcessing();
    void onNewMailItem(QStringList, int);
    void onSetSeenFlagById(int);
    void onDeleteMailById(int);
    void onDeleteAllMails();
    void onNotifNewXmlFileAvailableNotif(QString fileName);
    int onUpdateVodFiles();
    int onUpdateLiveTvFiles();
    void onUpdateEpgFile();
    void onNotifyDeadLinks(QString, QString);
    void onVodActivityNotify(bool);
    void onVodServerAddressNotify(QString, QString);
    void onLiveTvServerAddressNotify(QString, QString);
    void onActiveLiveTvFeatureNotify(QString);
    void onActiveVodFeatureNotify(QString);

private:
    SC_SubsClientPythonInvoker m_serverPythonInvoker;
    QDBusInterface * m_invokerInterface;
    unsigned int m_clientProcActivity;

    QList<QString> m_displayMailItemList;
};

/**
 * @brief The SC_SubsClientElements class
 */
class SC_SubsClientElements : public QObject
{
    Q_OBJECT
public:
    SC_SubsClientElements();
    ~SC_SubsClientElements();

    Q_INVOKABLE void startClientDaemon(void);
    Q_INVOKABLE void extractMailList();
    Q_INVOKABLE void setSeenFlagById(int index);
    Q_INVOKABLE void deleteMailById(int index);
    Q_INVOKABLE void deleteAllMails();
    Q_INVOKABLE int updateLiveTvFiles();
    Q_INVOKABLE int updateVodFiles();
    Q_INVOKABLE void updateEpgFile();
    Q_INVOKABLE void notifyDeadLinks(QString, QString);
    Q_INVOKABLE void notifyNewFirmwareOnUsbSupport(QString, QString);
    Q_INVOKABLE void vodActivityNotify(bool);
    Q_INVOKABLE void setVodServerAddress(QString, QString);
    Q_INVOKABLE void setLiveTvServerAddress(QString, QString);
    Q_INVOKABLE void activeLiveTvFeature(QString);
    Q_INVOKABLE void activeVodFeature(QString);

Q_SIGNALS:
    void startClientDaemonNotify();
    void extractMailsFromCache();
    void newNotificationMsg(QString severity,
                            QString message,
                            QString sendDate);
    void newFirmwareReceivedInProgress();
    void newFirmwareReceivedStatus(QString status,
                                   QString filename,
                                   QString sendDate);
    void mailBoxInfosRecv(QStringList mailBoxInfos);
    void readyMailItemList(QStringList mailDisplayList);
    void displayInternalSrvNotifSignal(QString header, QString body);
    void mailBoxEmptyNotify();
    void vodFileAvailable();
    void liveTvFileAvailable();
    void mainScreenVodFileAvailable();
    void epgFileAvailable();
    void epgFileUpdated();
    void firmwareOK(void);

private:
    //Local Attributes
    SC_SubsClientEngine * m_serverNotifEngine;
//    Downloader * m_downloader;
};

#endif // OMX_SERVERNOTIFICATIONENGINE_H
