#ifndef KVAL_EPGMANAGER_H
#define KVAL_EPGMANAGER_H
#include <QDebug>
#include <QFile>
#include <QDate>
#include <unordered_map>
#include <QXmlStreamReader>

struct Program {
    explicit Program() = default;
    ~Program() = default;

    // Avoid any copy as epg files may contains heavy data...
    Program(const Program& other) = delete;
    Program& operator=(const Program& other) = delete;

    Program(Program&& other):
        start(std::move(other.start)),
        stop(std::move(other.stop)),
        title(std::move(other.title)),
        categories(std::move(other.categories)),
        actors(std::move(other.actors)),
        date(std::move(other.date)),
        desc(std::move(other.desc)),
        uid(std::move(other.uid)) {}

    Program& operator=(Program&& other) {
        if(this == &other) {
            return *this;
        }
        start = std::move(other.start);
        stop = std::move(other.stop);
        title = std::move(other.title);
        categories = std::move(other.categories);
        actors = std::move(other.actors);
        date = std::move(other.date);
        desc =std::move(other.desc);
        uid = std::move(other.uid);
        return *this;
    }

    QDateTime start{};
    QDateTime stop{};
    QString title{};
    QStringList categories{};
    QStringList actors{};
    QString date{};
    QString desc{};
    QString uid{};
};

struct ChannelEpg {
    explicit ChannelEpg() = default;
    ~ChannelEpg() = default;

    explicit ChannelEpg(QString name): name(name){
        prgs.reserve(100);
    }

    // Avoid any copy as epg files may contains heavy data...
    ChannelEpg(const ChannelEpg& other) = delete;
    ChannelEpg& operator&(const ChannelEpg& other) = delete;
    ChannelEpg(ChannelEpg&& other):
        name(std::move(other.name)),
        prgs(std::move(other.prgs))
    {}
    ChannelEpg& operator=(ChannelEpg&& other)
    {
        if(this == &other)
            return *this;
        name = std::move(other.name);
        prgs = std::move(other.prgs);
        return *this;
    }

    QString name{};
    // Use explicitly std::vector as QVector needs a copy constructor, for its
    // internal cook.
    std::vector<Program> prgs{};
};

/**
 * @brief The EM_EpgEngine class
 */
class KvalEpgEngine : public QObject
{
    Q_OBJECT
public:
    enum EpgStatus
    {
        EPG_AVAILABLE_ST = 0,
        EPG_IN_PROGRESS_ST,
        EPG_NOT_AVAILABLE_ST
    };

    KvalEpgEngine();
    ~KvalEpgEngine();

    bool Start(const char*);
    void Stop();
    EpgStatus status() { return state.load(); };
    bool generateUiChEpg(const QString&, QVariantMap&);
    void generateEpgChList(const QStringList& chList);
    void setTimeZoneOffset(int);

Q_SIGNALS:
    void qmlEpgReady(const QVariantMap&);
    void qmlEpgEnds();
    void startEpgWebGrabber();

public Q_SLOTS:
    void epgUpdateInProgress();

private:
    void process(const char*);
    void readPrograms(QXmlStreamReader&);
    bool extractSingleProgram(QXmlStreamReader&, ChannelEpg&);

private:
    //Local Attributes
    std::atomic<KvalEpgEngine::EpgStatus> state;
    // Use explicitly std::unordered_map as QHash needs a copy constructor.
    // for subscript operator, it adds a default entry if none found...
    std::unordered_map<QString, ChannelEpg> m_channelsEpg;

    int m_localtimeZoneOffset;
    bool m_abortParsing;
};

#endif // KVAL_EPGMANAGER_H
