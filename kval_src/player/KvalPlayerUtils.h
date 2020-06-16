#ifndef KVAL_PLAYERUTILS_H
#define KVAL_PLAYERUTILS_H
#include <QObject>
#include <QMap>
#include <QDebug>

extern QMap<QString, QString> iso639LangMap;

/**
 * @brief The PlayerTrack struct
 */
struct PlayerTrack
{
    enum TrackType { video, audio, subs, count };
    TrackType trackType{count};
    int id{};
    int stream_index{};
    QString codec{};
    QString extra{};
    QString filepath{};
    QString langTriplet{};
    QString langFull{};
    QString title{};

    PlayerTrack(const PlayerTrack& rhs);

    PlayerTrack(TrackType type, const QVariantMap& track);

    friend QDebug operator<< (QDebug d, const PlayerTrack &track) {
        QMap<TrackType, QString> displayMap{
            {TrackType::video, "Video Track"},
            {TrackType::audio, "Audio Track"},
            {TrackType::subs, "Subs Track"}
        };
        d << "======================== "
          << displayMap[track.trackType]
          << " ========================" << endl;
        d << "id: " << track.id << endl;
        d << "stream_index: " << track.stream_index << endl;
        d << "codec: " << track.codec << endl;
        d << "file: " << track.filepath << endl;
        d << "codec: " << track.codec << endl;
        d << "language: " << track.langFull << endl;

        return d;
    }

private:
    template<typename T>
    T _cvt(const QVariantMap& var_map, QString&& key){
        if(var_map.contains(key) && var_map[key].isValid()){
            T val = qvariant_cast<T>(var_map[key]);
            return val;
        }

        T def_val{};
        return def_val;
    }
};

/**
 * @brief The AudioTrack struct
 */
struct AudioTrack: PlayerTrack
{
    AudioTrack(const QVariantMap& track):
        PlayerTrack(TrackType::audio, track){};
    AudioTrack(const AudioTrack& track):
        PlayerTrack(track){};
};

/**
 * @brief The SubsTrack struct
 */
struct SubsTrack: PlayerTrack
{
    SubsTrack(const QVariantMap& track):
        PlayerTrack(TrackType::subs, track){};
    SubsTrack(const SubsTrack& track):
        PlayerTrack(track){};
};

#endif // KVAL_PLAYERUTILS_H
