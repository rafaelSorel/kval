
#include "KvalPlayerUtils.h"

QMap<QString, QString> iso639LangMap =
{
    {"aar",   "Afar"},
    {"afr",   "Afrikaans"},
    {"aka",   "Akan"},
    {"amh",   "Amharique"},
    {"arg",   "Aragonais"},
    {"ara",   "Arabe"},
    {"asm",   "Assamais"},
    {"ava",   "Avar"},
    {"aym",   "Aymara"},
    {"aze",   "Azéri"},
    {"bak",   "Bachkir"},
    {"bel",   "Biélorusse"},
    {"bul",   "Bulgare"},
    {"bih",   "Bihari"},
    {"bis",   "Bichelamar"},
    {"bam",   "Bambara"},
    {"ben",   "Bengali"},
    {"bre",   "Breton"},
    {"bos",   "Bosnien"},
    {"cat",   "Catalan"},
    {"che",   "Tchétchène"},
    {"cha",   "Chamorro"},
    {"cos",   "Corse"},
    {"cre",   "Cri"},
    {"cze",   "Tchèque"},
    {"ces",   "Tchèque"},
    {"chu",   "Vieux-slave"},
    {"chv",   "Tchouvache"},
    {"wel",   "Gallois"},
    {"cym",   "Gallois"},
    {"dan",   "Danois"},
    {"ger",   "Allemand"},
    {"deu",   "Allemand"},
    {"ell",   "Grecmoderne"},
    {"gre",   "Grecmoderne"},
    {"eng",   "Anglais"},
    {"spa",   "Espagnol"},
    {"fas",   "Persan"},
    {"per",   "Persan"},
    {"fin",   "Finnois"},
    {"fra",   "Français"},
    {"fre",   "Français"},
    {"gle",   "Irlandais"},
    {"gla",   "Écossais"},
    {"glg",   "Galicien"},
    {"heb",   "Hébreu"},
    {"hin",   "Hindi"},
    {"hrv",   "Croate"},
    {"scr",   "Croate"},
    {"hun",   "Hongrois"},
    {"hye",   "Arménien"},
    {"arm",   "Arménien"},
    {"ind",   "Indonésien"},
    {"ice",   "Islandais"},
    {"isl",   "Islandais"},
    {"ita",   "Italien"},
    {"jpn",   "Japonais"},
    {"kat",   "Géorgien"},
    {"geo",   "Géorgien"},
    {"kua",   "Kuanyama"},
    {"kaz",   "Kazakh"},
    {"kal",   "Groenlandais"},
    {"kor",   "Coréen"},
    {"kau",   "Kanouri"},
    {"kas",   "Cachemiri"},
    {"kur",   "Kurde"},
    {"lat",   "Latin"},
    {"ltz",   "Luxembourgeois"},
    {"mri",   "Maori"},
    {"mao",   "Maori"},
    {"mkd",   "Macédonien"},
    {"mac",   "Macédonien"},
    {"mlt",   "Maltais"},
    {"nau",   "Nauruan"},
    {"nob",   "Norvégien"},
    {"nde",   "Sindebele"},
    {"nep",   "Népalais"},
    {"ndo",   "Ndonga"},
    {"dut",   "Néerlandais"},
    {"nld",   "Néerlandais"},
    {"nno",   "Norvégien"},
    {"nor",   "Norvégien"},
    {"nbl",   "Nrebele"},
    {"nya",   "Chichewa"},
    {"oci",   "Occitan"},
    {"oji",   "Ojibwé"},
    {"orm",   "Oromo"},
    {"ori",   "Oriya"},
    {"oss",   "Ossète"},
    {"pan",   "Pendjabi"},
    {"pli",   "Pali"},
    {"pol",   "Polonais"},
    {"pus",   "Pachto"},
    {"por",   "Portugais"},
    {"que",   "Quechua"},
    {"roh",   "Romanche"},
    {"run",   "Kirundi"},
    {"rus",   "Russe"},
    {"kin",   "Kinyarwanda"},
    {"san",   "Sanskrit"},
    {"srd",   "Sarde"},
    {"snd",   "Sindhi"},
    {"sme",   "SameduNord"},
    {"sag",   "Sango"},
    {"sin",   "Cingalais"},
    {"slk",   "Slovaque"},
    {"slo",   "Slovaque"},
    {"slv",   "Slovène"},
    {"sqi",   "Albanais"},
    {"alb",   "Albanais"},
    {"srp",   "Serbe"},
    {"scc",   "Serbe"},
    {"ssw",   "Swati"},
    {"sot",   "SothoduSud"},
    {"sun",   "Soundanais"},
    {"swe",   "Suédois"},
    {"swa",   "Swahili"},
    {"tha",   "Thaï"},
    {"tir",   "Tigrigna"},
    {"tuk",   "Turkmène"},
    {"tgl",   "Tagalog"},
    {"tur",   "Turc"},
    {"tso",   "Tsonga"},
    {"tat",   "Tatar"},
    {"tah",   "Tahitien"},
    {"uig",   "Ouïghour"},
    {"ukr",   "Ukrainien"},
    {"urd",   "Ourdou"},
    {"vie",   "Vietnamien"},
    {"vol",   "Volapük"},
    {"wln",   "Wallon"},
    {"wol",   "Wolof"},
    {"xho",   "Xhosa"},
    {"zha",   "Zhuang"},
    {"zho",   "Chinois"},
    {"chi",   "Chinois"},
    {"zul",   "Zoulou"},
};



/**
 * @brief PlayerTrack::PlayerTrack: Copy constructor
 * @param rhs
 */
PlayerTrack::PlayerTrack(const PlayerTrack &rhs):
    trackType{rhs.trackType},
    id{rhs.id},
    stream_index{rhs.stream_index},
    codec{rhs.codec},
    extra{rhs.extra},
    filepath{rhs.filepath},
    langTriplet{rhs.langTriplet},
    langFull{rhs.langFull},
    title{rhs.title} {}

/**
 * @brief PlayerTrack::PlayerTrack
 * @param type
 * @param track
 */
PlayerTrack::PlayerTrack(PlayerTrack::TrackType type, const QVariantMap &track):
    trackType{type},
    id{_cvt<int>(track, "id")},
    stream_index{_cvt<int>(track, std::move("stream_index"))},
    codec{_cvt<QString>(track, std::move("codec"))},
    extra{_cvt<QString>(track, std::move("extra"))},
    filepath{_cvt<QString>(track, std::move("file"))},
    langTriplet{_cvt<QString>(track, std::move("language"))},
    title{_cvt<QString>(track, std::move("title"))}
{
    if(iso639LangMap.contains(langTriplet)){
        langFull = iso639LangMap[langTriplet].toStdString().c_str();
    }
    else {
        QTextStream ts{&langFull};
        ts << QT_TR_NOOP(((type== TrackType::audio) ?
                              "Piste Audio" :
                              "Piste Sous-titres"))
           << " " << id+1;
    }
}
