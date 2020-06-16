#include "quicksubtitle.h"
#include <QDir>
#include <QtAV/Filter.h>
#include <QtAV/VideoFrame.h>
#include <QtAV>

using namespace QtAV;

namespace QtAV {
QString getLocalPath(const QString& fullPath)
{
#ifdef Q_OS_MAC
    if (fullPath.startsWith(QLatin1String("file:///.file/id=")) || fullPath.startsWith(QLatin1String("/.file/id=")))
        return absolutePathFromOSX(fullPath);
#endif
    int pos = fullPath.indexOf(QLatin1String("file:"));
    if (pos >= 0) {
        pos += (sizeof("file:") - 1);
        bool has_slash = false;
        while (fullPath.at(pos) == QLatin1Char('/')) {
            has_slash = true;
            ++pos;
        }
        // win: ffmpeg does not supports file:///C:/xx.mov, only supports file:C:/xx.mov or C:/xx.mov
#ifndef Q_OS_WIN // for QUrl
        if (has_slash)
            --pos;
#endif
    }
    // always remove "file:" even thought it works for ffmpeg.but fileName() may be used for QFile which does not file:
    if (pos > 0)
        return fullPath.mid(pos);
    return fullPath;
}

// /xx/oo/a.01.mov => /xx/oo/a.01. native dir separator => /
/*!
 * \brief getSubtitleBasePath
 * \param fullPath path of video or without extension
 * \return absolute path without extension and file://
 */
static QString getSubtitleBasePath(const QString fullPath)
{
    QString path(QDir::fromNativeSeparators(fullPath));
    //path.remove(p->source().scheme() + "://");
    // QString name = QFileInfo(path).completeBaseName();
    // why QFileInfo(path).dir() starts with qml app dir?
    QString name(path);
    int lastSep = path.lastIndexOf(QLatin1Char('/'));
    if (lastSep >= 0) {
        name = name.mid(lastSep + 1);
        path = path.left(lastSep + 1); // endsWidth "/"
    }
    int lastDot = name.lastIndexOf(QLatin1Char('.')); // not path.lastIndexof("."): xxx.oo/xx
    if (lastDot > 0)
        name = name.left(lastDot);
    if (path.startsWith(QLatin1String("file:"))) // can skip convertion here. Subtitle class also convert the path
        path = getLocalPath(path);
    path.append(name);
    return path;
}

PlayerSubtitle::PlayerSubtitle(QObject *parent)
    : QObject(parent)
    , m_auto(true)
    , m_enabled(true)
    , m_player(0)
    , m_sub(new Subtitle(this))
{
}

Subtitle* PlayerSubtitle::subtitle()
{
    return m_sub;
}

void PlayerSubtitle::setPlayer(AVPlayer *player)
{
    if (m_player == player)
        return;
    if (m_player) {
        disconnectSignals();
    }
    m_player = player;
    if (!m_player)
        return;
    connectSignals();
}

void PlayerSubtitle::setFile(const QString &file)
{
    if (m_file != file)
        Q_EMIT fileChanged();
    // always load
    // file was set but now playing with fuzzy match. if file is set again to the same value, subtitle must load that file
    m_file = file;
    if (!m_enabled)
        return;
    m_sub->setFileName(file);
    m_sub->setFuzzyMatch(false);
    m_sub->loadAsync();
}

QString PlayerSubtitle::file() const
{
    return m_file;
}
void PlayerSubtitle::setAutoLoad(bool value)
{
    if (m_auto == value)
        return;
    m_auto = value;
    Q_EMIT autoLoadChanged(value);
}

bool PlayerSubtitle::autoLoad() const
{
    return m_auto;
}

void PlayerSubtitle::onPlayerSourceChanged()
{
    if (!m_auto) {
        m_sub->setFileName(QString());
        return;
    }
    if (!m_enabled)
        return;
    AVPlayer *p = qobject_cast<AVPlayer*>(sender());
    if (!p)
        return;
    m_sub->setFileName(getSubtitleBasePath(p->file()));
    m_sub->setFuzzyMatch(true);
    m_sub->loadAsync();
}

void PlayerSubtitle::onPlayerPositionChanged()
{
    AVPlayer *p = qobject_cast<AVPlayer*>(sender());
    if (!p)
        return;
    m_sub->setTimestamp(qreal(p->position())/1000.0);
}

void PlayerSubtitle::onPlayerStart()
{
    if (!m_enabled)
        return;
    // priority: user file > auto load file > embedded
    if (!m_file.isEmpty()) {
        if (m_file == m_sub->fileName())
            return;
        m_sub->setFileName(m_file);
        m_sub->setFuzzyMatch(false);
        m_sub->loadAsync();
        return;
    }
    if (autoLoad() && !m_sub->fileName().isEmpty())
        return; //already loaded in onPlayerSourceChanged()
    // try embedded subtitles
    const int n = m_player->currentSubtitleStream();
    if (n < 0 || m_tracks.isEmpty() || m_tracks.size() <= n) {
        m_sub->processHeader(QByteArray(), QByteArray()); // reset
        return;
    }
    QVariantMap track = m_tracks[n].toMap();
    QByteArray codec(track.value(QStringLiteral("codec")).toByteArray());
    QByteArray data(track.value(QStringLiteral("extra")).toByteArray());
    m_sub->processHeader(codec, data);
    return;
}

void PlayerSubtitle::onEnabledChanged(bool value)
{
    m_enabled = value;
    if (!m_enabled) {
        disconnectSignals();
        return;
    }
    connectSignals();
    // priority: user file > auto load file > embedded
    if (!m_file.isEmpty()) {
        if (m_sub->fileName() == m_file && m_sub->isLoaded())
            return;
        m_sub->setFileName(m_file);
        m_sub->setFuzzyMatch(false);
        m_sub->loadAsync();
    }
    if (!m_player)
        return;
    if (!autoLoad()) // fallback to internal subtitles
        return;
    m_sub->setFileName(getSubtitleBasePath(m_player->file()));
    m_sub->setFuzzyMatch(true);
    m_sub->loadAsync();
    return;
}

void PlayerSubtitle::tryReload()
{
    tryReload(3);
}

void PlayerSubtitle::tryReloadInternalSub()
{
    tryReload(1);
}

void PlayerSubtitle::tryReload(int flag)
{
    if (!m_enabled)
        return;
    if (!m_player->isPlaying())
        return;
    const int kReloadExternal = 1<<1;
    if (flag & kReloadExternal) {
        //engine or charset changed
        m_sub->processHeader(QByteArray(), QByteArray()); // reset
        m_sub->loadAsync();
        return;
    }
    // kReloadExternal is not set, kReloadInternal is unknown
    //fallback to external sub if no valid internal sub track is set
    const int n = m_player->currentSubtitleStream();
    if (n < 0 || m_tracks.isEmpty() || m_tracks.size() <= n) {
        m_sub->processHeader(QByteArray(), QByteArray()); // reset, null processor
        m_sub->loadAsync();
        return;
    }
    // try internal subtitles
    QVariantMap track = m_tracks[n].toMap();
    QByteArray codec(track.value(QStringLiteral("codec")).toByteArray());
    QByteArray data(track.value(QStringLiteral("extra")).toByteArray());
    m_sub->processHeader(codec, data);
    Packet pkt(m_current_pkt[n]);
    if (pkt.isValid()) {
        processInternalSubtitlePacket(n, pkt);
    }
}

void  PlayerSubtitle::updateInternalSubtitleTracks(const QVariantList &tracks)
{
    m_tracks = tracks;
    m_current_pkt.resize(tracks.size());
}

void PlayerSubtitle::processInternalSubtitlePacket(int track, const QtAV::Packet &packet)
{
    m_sub->processLine(packet.data, packet.pts, packet.duration);
    m_current_pkt[track] = packet;
}

void PlayerSubtitle::processInternalSubtitleHeader(const QByteArray& codec, const QByteArray &data)
{
    m_sub->processHeader(codec, data);
}

void PlayerSubtitle::connectSignals()
{
    if (!m_player)
        return;
    connect(m_player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
    connect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
    connect(m_player, SIGNAL(started()), this, SLOT(onPlayerStart()));
    connect(m_player, SIGNAL(internalSubtitlePacketRead(int,QtAV::Packet)), this, SLOT(processInternalSubtitlePacket(int,QtAV::Packet)));
    connect(m_player, SIGNAL(internalSubtitleHeaderRead(QByteArray,QByteArray)), this, SLOT(processInternalSubtitleHeader(QByteArray,QByteArray)));
    connect(m_player, SIGNAL(internalSubtitleTracksChanged(QVariantList)), this, SLOT(updateInternalSubtitleTracks(QVariantList)));
    // try to reload internal subtitle track. if failed and external subtitle is enabled, fallback to external
    connect(m_player, SIGNAL(subtitleStreamChanged(int)), this, SLOT(tryReloadInternalSub()));
    connect(m_sub, SIGNAL(codecChanged()), this, SLOT(tryReload()));
    connect(m_sub, SIGNAL(enginesChanged()), this, SLOT(tryReload()));
}

void PlayerSubtitle::disconnectSignals()
{
    if (!m_player)
        return;
    disconnect(m_player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
    disconnect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
    disconnect(m_player, SIGNAL(started()), this, SLOT(onPlayerStart()));
    disconnect(m_player, SIGNAL(internalSubtitlePacketRead(int,QtAV::Packet)), this, SLOT(processInternalSubtitlePacket(int,QtAV::Packet)));
    disconnect(m_player, SIGNAL(internalSubtitleHeaderRead(QByteArray,QByteArray)), this, SLOT(processInternalSubtitleHeader(QByteArray,QByteArray)));
    disconnect(m_player, SIGNAL(internalSubtitleTracksChanged(QVariantList)), this, SLOT(updateInternalSubtitleTracks(QVariantList)));
    disconnect(m_sub, SIGNAL(codecChanged()), this, SLOT(tryReload()));
    disconnect(m_sub, SIGNAL(enginesChanged()), this, SLOT(tryReload()));
}

} //namespace QtAV

class QuickSubtitle::Filter : public QtAV::VideoFilter
{
public:
    Filter(Subtitle *sub, QuickSubtitle *parent) :
        VideoFilter(parent)
      , m_empty_image(false)
      , m_sub(sub)
      , m_subject(parent)
    {}
protected:
    virtual void process(Statistics* statistics, VideoFrame* frame) {
        Q_UNUSED(statistics);
        if (!m_sub)
            return;
        if (frame && frame->timestamp() > 0.0) {
            m_sub->setTimestamp(frame->timestamp()); //TODO: set to current display video frame's timestamp
            QRect r;
            QImage image(m_sub->getImage(frame->width(), frame->height(), &r));
            if (image.isNull()) {
                if (m_empty_image)
                    return;
                m_empty_image = true;
            } else {
                m_empty_image = false;
            }
            m_subject->notifyObservers(image, r, frame->width(), frame->height());
        }
    }
private:
    bool m_empty_image;
    Subtitle *m_sub;
    QuickSubtitle *m_subject;
};

QuickSubtitle::QuickSubtitle(QObject *parent) :
    QObject(parent)
  , SubtitleAPIProxy(this)
  , m_enable(true)
  , m_player(0)
  , m_player_sub(new PlayerSubtitle(this))
  , m_filter(0)
{
    AVPlayer *p = qobject_cast<AVPlayer*>(parent);
    if (p) setPlayer(p);

    m_filter = new Filter(m_player_sub->subtitle(), this);
    setSubtitle(m_player_sub->subtitle()); //for proxy
    connect(this, SIGNAL(enabledChanged(bool)), m_player_sub, SLOT(onEnabledChanged(bool))); //////
    connect(m_player_sub, SIGNAL(autoLoadChanged(bool)), this, SIGNAL(autoLoadChanged()));
    connect(m_player_sub, SIGNAL(fileChanged()), this, SIGNAL(fileChanged()));
}

QString QuickSubtitle::getText() const
{
    return m_player_sub->subtitle()->getText();
}

void QuickSubtitle::addObserver(QuickSubtitleObserver *ob)
{
    if (!m_observers.contains(ob)) {
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(lock);
        m_observers.append(ob);
    }
}

void QuickSubtitle::removeObserver(QuickSubtitleObserver *ob)
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    m_observers.removeAll(ob);
}

void QuickSubtitle::notifyObservers(const QImage &image, const QRect &r, int width, int height, QuickSubtitleObserver *ob)
{
    if (ob) {
        ob->update(image, r, width, height);
        return;
    }
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (m_observers.isEmpty())
        return;
    Q_FOREACH (QuickSubtitleObserver* o, m_observers) {
        o->update(image, r, width, height);
    }
}

void QuickSubtitle::setPlayer(QObject *player)
{
    AVPlayer *p = qobject_cast<AVPlayer*>(player);
    if (m_player == p)
        return;
    if (m_player)
        m_filter->uninstall();
    m_player = p;
    if (!p)
        return;
    m_filter->installTo(p);
    // ~Filter() can not call uninstall() unless player is still exists
    // TODO: check AVPlayer null?
    m_player_sub->setPlayer(p);
}

QObject* QuickSubtitle::player()
{
    return m_player;
}

void QuickSubtitle::setEnabled(bool value)
{
    if (m_enable == value)
        return;
    m_enable = value;
    Q_EMIT enabledChanged(value);
    m_filter->setEnabled(m_enable);
    if (!m_enable) { //display nothing
        notifyObservers(QImage(), QRect(), 0, 0);
    }
}

bool QuickSubtitle::isEnabled() const
{
    return m_enable;
}

void QuickSubtitle::setFile(const QString &file)
{
    m_player_sub->setFile(file);
}

QString QuickSubtitle::file() const
{
    return m_player_sub->file();
}

void QuickSubtitle::setAutoLoad(bool value)
{
    m_player_sub->setAutoLoad(value);
}

bool QuickSubtitle::autoLoad() const
{
    return m_player_sub->autoLoad();
}

