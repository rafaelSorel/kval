#ifndef QUICKSUBTITLE_H
#define QUICKSUBTITLE_H

#include <QtAV/Subtitle.h>
#include <QtAV>
#include <QtCore/QMutexLocker>

class QuickSubtitleObserver {
public:
    virtual void update(const QImage& image, const QRect& r, int width, int height) = 0;
};

namespace QtAV {

class AVPlayer;
class Subtitle;
/*!
 * \brief The PlayerSubtitle class
 * Bind Subtitle to AVPlayer. Used by SubtitleFilter and QuickSubtitle.
 * Subtitle load priority: user specified file (setFile(...)) > auto load external (autoLoad() must be true) > embedded subtitle
 */
class Q_AV_PRIVATE_EXPORT PlayerSubtitle : public QObject
{
    Q_OBJECT
public:
    PlayerSubtitle(QObject *parent = 0);
    void setPlayer(AVPlayer* player);
    Subtitle* subtitle();
    /*!
     * \brief setFile
     * Load user selected subtitle. The subtitle will not change unless you manually setFile(QString()).
     */
    void setFile(const QString& file);
    QString file() const;
    /*!
     * \brief autoLoad
     * Auto find and load a suitable external subtitle if file() is not empty.
     */
    void setAutoLoad(bool value);
    bool autoLoad() const;
Q_SIGNALS:
    void autoLoadChanged(bool value);
    void fileChanged();
public Q_SLOTS:
    void onEnabledChanged(bool value);
private Q_SLOTS:
    void onPlayerSourceChanged();
    void onPlayerPositionChanged();
    void onPlayerStart();
    void tryReload();
    void tryReloadInternalSub();
    void updateInternalSubtitleTracks(const QVariantList& tracks);
    void processInternalSubtitlePacket(int track, const QtAV::Packet& packet);
    void processInternalSubtitleHeader(const QByteArray &codec, const QByteArray& data); //TODO: remove
private:
    void connectSignals();
    void disconnectSignals();
    void tryReload(int flag); //1: internal, 2: external, 3: internal+external
private:
    bool m_auto;
    bool m_enabled; // TODO: m_enable_external
    AVPlayer *m_player;
    Subtitle *m_sub;
    QString m_file;
    QVariantList m_tracks;
    QVector<Packet> m_current_pkt;
};

}

/*!
 * \brief The QuickSubtitle class
 * high level Subtitle processor for QML. No rendering.
 * Subtitle load priority: user specified file (setFile(...)) > auto load external (autoLoad() must be true) > embedded subtitle
 */
class QuickSubtitle : public QObject, public QtAV::SubtitleAPIProxy
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QObject* player READ player WRITE setPlayer)
    // proxy api
    Q_PROPERTY(QByteArray codec READ codec WRITE setCodec NOTIFY codecChanged)
    Q_PROPERTY(QStringList engines READ engines WRITE setEngines NOTIFY enginesChanged)
    Q_PROPERTY(QString engine READ engine NOTIFY engineChanged)
    Q_PROPERTY(bool fuzzyMatch READ fuzzyMatch WRITE setFuzzyMatch NOTIFY fuzzyMatchChanged)
    //Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QStringList dirs READ dirs WRITE setDirs NOTIFY dirsChanged)
    Q_PROPERTY(QStringList suffixes READ suffixes WRITE setSuffixes NOTIFY suffixesChanged)
    Q_PROPERTY(QStringList supportedSuffixes READ supportedSuffixes NOTIFY supportedSuffixesChanged)
    Q_PROPERTY(qreal delay READ delay WRITE setDelay NOTIFY delayChanged)
    Q_PROPERTY(bool canRender READ canRender NOTIFY canRenderChanged)
    //PlayerSubtitle api
    Q_PROPERTY(bool autoLoad READ autoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    //
    Q_PROPERTY(QString text READ getText)
    // font properties for libass engine
    Q_PROPERTY(QString fontFile READ fontFile WRITE setFontFile NOTIFY fontFileChanged)
    Q_PROPERTY(QString fontsDir READ fontsDir WRITE setFontsDir NOTIFY fontsDirChanged)
    Q_PROPERTY(bool fontFileForced READ isFontFileForced WRITE setFontFileForced NOTIFY fontFileForcedChanged)
public:
    explicit QuickSubtitle(QObject *parent = 0);
    Q_INVOKABLE QString getText() const;
    // observer is only for ass image subtitle
    void addObserver(QuickSubtitleObserver* ob);
    void removeObserver(QuickSubtitleObserver* ob);
    // 0: notify all
    void notifyObservers(const QImage& image, const QRect& r, int width, int height, QuickSubtitleObserver* ob = 0);
    /*!
     * \brief setPlayer
     * if player is set, subtitle will automatically loaded if playing file changed.
     * \param player
     */
    void setPlayer(QObject* player);
    QObject* player();

    // TODO: enableRenderImage + enabled
    void setEnabled(bool value = true); //AVComponent.enabled
    bool isEnabled() const;
    /*!
     * \brief setFile
     * Load user selected subtitle. The subtitle will not change unless you manually setFile(QString()).
     */
    void setFile(const QString& file);
    QString file() const;
    /*!
     * \brief autoLoad
     * Auto find and load a suitable external subtitle if file() is not empty.
     */
    bool autoLoad() const;
    //void setAssFrameSize(int width, int height);
public Q_SLOTS:
    // TODO: enable changed & autoload=> load
    void setAutoLoad(bool value);
Q_SIGNALS:
    void fileChanged();
    void canRenderChanged();
    void loaded(const QString& path);
    void enabledChanged(bool value);
    void autoLoadChanged();

    void codecChanged();
    void enginesChanged();
    void fuzzyMatchChanged();
    void contentChanged();
    //void fileNameChanged();
    void dirsChanged();
    void suffixesChanged();
    void supportedSuffixesChanged();
    void engineChanged();
    void delayChanged();
    void fontFileChanged();
    void fontsDirChanged();
    void fontFileForcedChanged();

private:
    bool m_enable;
    QtAV::AVPlayer *m_player;
    QtAV::PlayerSubtitle *m_player_sub;

    class Filter;
    Filter *m_filter;
    QMutex m_mutex;
    QList<QuickSubtitleObserver*> m_observers;
};

#endif // QUICKSUBTITLE_H
