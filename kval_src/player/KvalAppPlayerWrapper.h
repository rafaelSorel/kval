#pragma once

#include <memory>
#include <QObject>
#include <QMap>
#include <QVector>

#include "KvalPlayerPlatform.h"

class KvalAppMediaPlayList;

typedef enum _MEDIA_URL_TYPE
{
    MEDIA_TYPE_UNKNOWN = 0,
    MEDIA_TYPE_HTTP,
    MEDIA_TYPE_FILE,
    MEDIA_TYPE_PLUGIN,
    MEDIA_TYPE_COUNT
}MEDIA_URL_TYPE_t;

class KvalAppMediaPlayerWrapper: public QObject
{
    Q_OBJECT

public:
    KvalAppMediaPlayerWrapper();
    virtual ~KvalAppMediaPlayerWrapper();

     void play(QString source,
               KvalAppMediaPlayList * playlist,
               QMap<QString, QString> item,
               bool windowed,
               int startpos);
     void setPlayList(KvalAppMediaPlayList * playlist);
     void stop();
     void pause();
     void playNext();
     void playPrevious();
     void playSelected(int selected);
     bool isPlaying();
     bool isPlayingAudio();
     bool isPlayingVideo();
     QString playingFileName();
     int streamPosition();
     void seek(int seekTime);
     void setSubs(QString path);
     int streamLength();
     static MEDIA_URL_TYPE_t getStreamType(QString url);
     bool isActivePlayList();
     QVariantMap nextPlayListItem();
     QVariantMap currentPlayListItem();
     void waitForUiGo();

Q_SIGNALS:
     void playbackStarted();
     void playbackFailed();
     void playbackCompleted();
     void uiStopRequested();

public Q_SLOTS:
     void onPlaybackStarted();
     void onPlaybackFailed();
     void onPlaybackCompleted();
     void onPlaybackInterrupted(long interruptedPosition);
     void onUiStopRequested();
     void onMediaPlayerBusy();

private:
     KvalMediaPlayerEngine * m_mediaPlayerEngine;
     KvalAppMediaPlayList * m_playList;
     bool m_isPlaying;
     bool m_isAudioPlaying;
     bool m_isVideoPlaying;
     int  m_currentPosition;
     bool m_playBackDelayPolicy;

     void playStream(QMap<QString, QString> item, bool windowed);
     void playPlayList(KvalAppMediaPlayList * playlist, bool windowed, int startpos);
};

/**
 * @brief The PH_MediaPlayList class
 */
class KvalAppMediaPlayList: public QObject
{
    Q_OBJECT

public:
    KvalAppMediaPlayList();
    virtual ~KvalAppMediaPlayList();

    //void Insert(const PH_MediaPlayList& playlist, int iPosition = -1);
    //void Insert(const QList<QMap<QString, QString> > items, int iPosition = -1);
    void Insert(const QMap<QString, QString> &item, int iPosition = -1);

    int FindOrder(int iOrder) const;
    QString GetName();
    void Remove(int position);
    bool Swap(int position1, int position2);

    void Clear();
    int size() const;

    void Shuffle(int iPosition = 0);
    void UnShuffle();
    bool IsShuffled() const { return m_bShuffled; }

    void SetPlayed(bool bPlayed) { m_bWasPlayed = true; }
    bool WasPlayed() const { return m_bWasPlayed; }

    int GetPlayable() const { return m_iPlayableItems; }

    void UpdateItem(const QMap<QString, QString> item);
    QMap<QString, QString> GetItem(int position, bool * found);
    int currentPosition();
    void setPosition(int position);


protected:
    int m_id;
    int m_currentPosition;
    QString m_strPlayListName;
    QString m_strBasePath;
    int m_iPlayableItems;
    bool m_bShuffled;
    bool m_bWasPlayed;
    QVector<QMap<QString, QString> > m_playListItems;
    QVector<QMap<QString, QString> > m_playListItemsUnshuffl;


private:
    //void AnnounceRemove(int pos);
    //void AnnounceClear();
    //void AnnounceAdd(const QMap<QString, QString>& item, int pos);
};
