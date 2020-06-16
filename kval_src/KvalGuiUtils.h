#include <QAbstractListModel>
#include <QStringList>
#include <QStringListModel>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#include "player/KvalRtmpStreamManager.h"
#endif

#include "KvalMiscUtils.h"

/**
 * @brief The QE_QmlBinder class
 */
class KvalGuiUtils : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KvalGuiUtils)

public:
    KvalGuiUtils();
    virtual ~KvalGuiUtils();

    Q_INVOKABLE QList<int> getChannelPosition(int channelNbr);
    Q_INVOKABLE void reloadChannelsList();
    Q_INVOKABLE void addSelectedChannelToFav(const QString&,const QString&,const QString&);
    Q_INVOKABLE void removeSelectedChannelFromFav(const QString &chIndex);
    Q_INVOKABLE void clearChannelFavoriteList();
    Q_INVOKABLE void updateCustomsList();
    Q_INVOKABLE void stopWelcomeScreenProc();
    Q_INVOKABLE void trimCache();

    Q_INVOKABLE QList<QString> getliveCustomMainZoneInfo() const
    {return m_liveCustomMainZoneInfo;
    }
    Q_INVOKABLE QList<QString> getliveCustomFirstBanZoneInfo() const
    {return m_liveCustomFirstBanZoneInfo;
    }
    Q_INVOKABLE QList<QString> getliveCustomSecondBanZoneInfo() const
    {return m_liveCustomSecondBanZoneInfo;
    }
    Q_INVOKABLE QList<QString> getliveCustomSecondBanAnimZoneImg() const
    {return m_liveCustomSecondBanAnimZoneImg;
    }
    Q_INVOKABLE QList<QString> getliveCustomSecondBanAnimZoneTxt() const
    {return m_liveCustomSecondBanAnimZoneTxt;
    }
    Q_INVOKABLE QList<QString> getliveCustomThirdBanZoneInfo() const
    {return m_liveCustomThirdBanZoneInfo;
    }
    Q_INVOKABLE QList<QString> getliveCustomThirdBanAnimZoneImg() const
    {return m_liveCustomThirdBanAnimZoneImg;
    }
    Q_INVOKABLE QList<QString> getliveCustomThirdBanAnimZoneTxt() const
    {return m_liveCustomThirdBanAnimZoneTxt;
    }

    Q_INVOKABLE QList<QString> getvodCustomTopLeftZoneInfo() const
    {return m_vodCustomTopLeftZoneInfo;
    }
    Q_INVOKABLE QList<QString> getvodCustomBottomLeftZoneInfo() const
    {return m_vodCustomBottomLeftZoneInfo;
    }
    Q_INVOKABLE QList<QString> getvodCustombottomMiddleZoneInfo() const
    {return m_vodCustombottomMiddleZoneInfo;
    }
    Q_INVOKABLE QList<QString> getvodCustomTopRightZoneInfo() const
    {return m_vodCustomTopRightZoneInfo;
    }
    Q_INVOKABLE QList<QString> getvodCustomMiddleRightZoneInfo() const
    {return m_vodCustomMiddleRightZoneInfo;
    }
    Q_INVOKABLE QList<QString> getvodCustomBottomRightZoneInfo() const
    {return m_vodCustomBottomRightZoneInfo;
    }

Q_SIGNALS:
    void epgReady();
    void cutomLiveTvParsed();
    void cutomVodParsed();
    void displayMsg(const QString& header, const QString& msg);

private:
    int clearFavList();
    int delChannelFromFavXml(const QString &);
    int addChannelToFavXml(const QString&,const QString&,const QString&);
    int checkFavList(int);
    void analyseXmlLiveChFile();
    void analyseXmlLiveCusomFile();
    void analyseXmlVODCusomFile();

private:
    QList<QString> m_liveCustomMainZoneInfo;
    QList<QString> m_liveCustomFirstBanZoneInfo;
    QList<QString> m_liveCustomSecondBanZoneInfo;
    QList<QString> m_liveCustomSecondBanAnimZoneImg;
    QList<QString> m_liveCustomSecondBanAnimZoneTxt;
    QList<QString> m_liveCustomThirdBanZoneInfo;
    QList<QString> m_liveCustomThirdBanAnimZoneImg;
    QList<QString> m_liveCustomThirdBanAnimZoneTxt;

    QList<QString> m_vodCustomTopLeftZoneInfo;
    QList<QString> m_vodCustomBottomLeftZoneInfo;
    QList<QString> m_vodCustombottomMiddleZoneInfo;
    QList<QString> m_vodCustomTopRightZoneInfo;
    QList<QString> m_vodCustomMiddleRightZoneInfo;
    QList<QString> m_vodCustomBottomRightZoneInfo;

    QList<QList<int> > m_channelstree;
    QList<int> m_favoriteChannelsIdx;
};
#ifdef ENABLE_LIRC_SYSTEM_CONTROLE
/**
 * @brief The QE_QmlBinder class
 */
class QERemoteControleKeyEmitter : public QThread
{
    Q_OBJECT
public:
    explicit QERemoteControleKeyEmitter(QObject * receiver);
    ~QERemoteControleKeyEmitter();
    void run();

Q_SIGNALS:
    void sendKey(unsigned int);

private:
    QObject * m_mainQuickWindow;
    QHash<const char*, unsigned int> m_lircQtKeys;
};
#endif //ENABLE_LIRC_SYSTEM_CONTROLE

/**
 * @brief The QE_QmlCustomView class
 */
class QE_QmlCustomView : public QQuickView
{
    Q_OBJECT
public:
    QE_QmlCustomView();
    ~QE_QmlCustomView();

    bool event(QEvent * event);

public Q_SLOTS:
    void processKey(unsigned int);

};

/**
 * @brief The QE_QMLLogger class
 */
class QE_QMLLogger : public QObject
{
   Q_OBJECT
public:
   static QE_QMLLogger& instance() {
      static QE_QMLLogger instance;
      return instance;
   }
   
   Q_INVOKABLE void debug(QString s) const;

   Q_INVOKABLE bool verbose(QString s) const;

   Q_INVOKABLE bool info(QString s) const;

   Q_INVOKABLE bool warn(QString s) const;

   Q_INVOKABLE bool error(QString s) const;

   Q_INVOKABLE bool critical(QString s) const;

   static void registerObject(QQmlContext* context);

private:
   QE_QMLLogger() : QObject() {}
};
