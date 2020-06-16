#define LOG_ACTIVATED
#include <QDebug>
#include <QXmlStreamReader>

#include "KvalEpgEngine.h"
#define LOG_SRC EPGENGINESTR
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Local variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

/**
 * @brief EM_EpgEngine::EM_EpgEngine
 */
KvalEpgEngine::KvalEpgEngine():
    state(EPG_NOT_AVAILABLE_ST),
    m_abortParsing(false)
{
    LOG_INFO(LOG_TAG, "Instantiate EM_EpgEngine");
}


/**
 * @brief EM_EpgEngine::~EM_EpgEngine
 */
KvalEpgEngine::~KvalEpgEngine()
{
    LOG_INFO(LOG_TAG, "Delete EM_EpgEngine...");
}

/**
 * @brief checkEpgGuide
 */
bool KvalEpgEngine::Start(const char* epgfilepath)
{
    state.store(EPG_IN_PROGRESS_ST);
    try {
        process(epgfilepath);
        state.store(EPG_AVAILABLE_ST);
        qInfo() << "============ Epg Ready ===========";
    } catch (const int ex) {
        LOG_INFO(LOG_TAG, "an error occured while processing the epg file...");
        state.store(EPG_NOT_AVAILABLE_ST);
        return false;
    }

    return true;
}

/**
 * @brief EM_EpgEngine::stopParsing
 */
void KvalEpgEngine::Stop()
{
    m_abortParsing = true;
}

/**
 * @brief KvalEpgEngine::setTimeZoneOffset
 * @param off
 */
void KvalEpgEngine::setTimeZoneOffset(int off)
{
    m_localtimeZoneOffset = off;
    LOG_INFO(LOG_TAG, "m_localtimeZoneOffset: %d", m_localtimeZoneOffset);
}

/**
 * @brief KvalEpgEngine::processprg
 */
void KvalEpgEngine::process(const char* epgfilepath)
{
    QFile file(epgfilepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_ERROR(LOG_TAG, "Couldn't open xml file: %s", epgfilepath);
        throw -ENOENT;
    }

    QXmlStreamReader reader(&file);
    if (reader.readNextStartElement()) {
        if (reader.name() != "tv"){
            LOG_ERROR(LOG_TAG, "Not an epg xml file !");
            file.close();
            throw -EINVAL;
        }
        readPrograms(reader);
    }
    file.close();
}

/**
 * @brief KvalEpgEngine::readPrograms
 * @param reader
 */
void KvalEpgEngine::readPrograms(QXmlStreamReader& reader)
{
    m_channelsEpg.clear();
    while(reader.readNextStartElement()){
        if(reader.name() == "programme"){
            if(!reader.attributes().hasAttribute("channel")){
                reader.skipCurrentElement();
                continue;
            }

            auto chname = reader.attributes().value("channel").toString();

            if(m_channelsEpg.find(chname) == m_channelsEpg.end()) {
                std::pair<QString, ChannelEpg> _new{chname, ChannelEpg{chname}};
                m_channelsEpg.emplace(std::move(_new));
            }
            ChannelEpg &chepg =m_channelsEpg[chname];
            extractSingleProgram(reader, chepg);
        }
    }
}

/**
 * @brief extractSingleProgram
 * @param xml
 * @param chepg
 * @return
 */
bool KvalEpgEngine::extractSingleProgram(
        QXmlStreamReader& reader,
        ChannelEpg& chepg)
{
    Program prg{};

    auto addPrgDt = [&](const QXmlStreamAttribute& at, Program& prg){
        QString xmlEpgTime = at.value().toString();
        int timeZoneOffset = xmlEpgTime.right(5).left(3).toInt();
        QDateTime dt = QDateTime::fromString(
                                    xmlEpgTime.left(14),
                                    "yyyyMMddhhmmss");
        if (timeZoneOffset != m_localtimeZoneOffset)
        {
            qDebug() << "timeZoneOffset: " << timeZoneOffset;
            qDebug() << "original dt: " << dt;
            timeZoneOffset = timeZoneOffset - m_localtimeZoneOffset;
            dt = dt.addSecs((-timeZoneOffset) * 3600);
            qDebug() << "converted startTime: " << dt;
        }
        if(at.name().toString() == "start")
            prg.start = dt;
        else if(at.name().toString() == "stop")
            prg.stop = dt;
    };

    QMap<QString,
        std::function<void(const QXmlStreamAttribute& at, Program& prg)>>
        _fmap { {"start", addPrgDt}, {"stop", addPrgDt} };

    std::for_each(
        std::begin(reader.attributes()),
        std::end(reader.attributes()),
        [&](const QXmlStreamAttribute& at){
            auto _key = qPrintable(at.name().toString());
            if(_fmap.contains(_key)) _fmap[_key](at, prg);
        });

    reader.readNext();
    auto end = [](const QXmlStreamReader& rd){
        return (rd.tokenType() == QXmlStreamReader::EndElement && rd.name() == "programme");
    };

    while(!end(reader))
    {
        if(m_abortParsing) break;
        if(reader.tokenType() == QXmlStreamReader::StartElement)
        {
            if(reader.name() == "title")
            {
                reader.readNext();
                if(reader.tokenType() == QXmlStreamReader::Characters) {
                    prg.title = reader.text().toString();
                }
            }
            if(reader.name() == "desc")
            {
                reader.readNext();
                if(reader.tokenType() == QXmlStreamReader::Characters) {
                    prg.desc = reader.text().toString();
                }
            }
            if(reader.name() == "actor")
            {
                reader.readNext();
                if(reader.tokenType() == QXmlStreamReader::Characters) {
                    prg.actors.append(reader.text().toString());
                }
            }
            if(reader.name() == "category")
            {
                reader.readNext();
                if(reader.tokenType() == QXmlStreamReader::Characters) {
                    prg.categories.append(reader.text().toString());
                }
            }
            if(reader.name() == "date")
            {
                reader.readNext();
                if(reader.tokenType() == QXmlStreamReader::Characters) {
                    prg.date = reader.text().toString();
                }
            }
            if(reader.name() == "uid")
            {
                reader.readNext();
                if(reader.tokenType() == QXmlStreamReader::Characters) {
                    prg.date = reader.text().toString();
                }
            }
        }
        /* ...and next... */
        reader.readNext();
    }

    chepg.prgs.push_back(std::move(prg));
    return false;
}

/**
 * @brief EM_EpgEngine::epgUpdateInProgress
 */
void KvalEpgEngine::epgUpdateInProgress()
{
    state.store(EPG_IN_PROGRESS_ST);
}

/**
 * @brief KvalEpgEngine::generateEpgChList
 * @param chList
 */
void KvalEpgEngine::generateEpgChList(const QStringList& chList)
{
    std::for_each(std::begin(chList), std::end(chList),
                  [&](const QString& ch){
                    QVariantMap epg{};
                    if(generateUiChEpg(ch, epg))
                        Q_EMIT qmlEpgReady(epg); } );

    Q_EMIT qmlEpgEnds();
}

/**
 * @brief EM_EpgEngine::quickGetChannelEpg
 * @param chName
 * @return
 */
bool KvalEpgEngine::generateUiChEpg(
        const QString& chname,
        QVariantMap &epg)
{
    if(m_channelsEpg.find(chname) == m_channelsEpg.end()){
        return false;
    }

    const QDateTime currentdt = QDateTime::currentDateTime();
    qInfo() << "currentdt: " << currentdt;
    // @use QPair for prg and next
    auto formatprg = [&](const Program* _prg, const Program* nextprg) {
        qreal prgDuration = _prg->start.secsTo(_prg->stop)/60;
        qreal prgRemains = currentdt.secsTo(_prg->stop)/60;
        qreal prgProgress = (prgDuration > 0)
                             ? (((prgDuration-prgRemains)/prgDuration)*100)
                             : 0;

        qDebug() << "prg title: " << _prg->title;
        epg["title"] = _prg->title;
        epg["start"] = _prg->start.toString("HH:mm");
        epg["end"] = _prg->stop.toString("HH:mm");
        epg["progress"] = prgProgress;
        epg["reamains"] = prgRemains;
        epg["desc"] = _prg->categories.join(", ")+'\n'+_prg->desc;
        epg["hasnext"] = (!nextprg) ? false: true;
        if(nextprg) {
            epg["nextTitle"] = nextprg->title;
            epg["nextStart"] = nextprg->start.toString("HH:mm");
            epg["nextEnd"] = nextprg->stop.toString("HH:mm");
            epg["nextDuration"] = _prg->start.secsTo(_prg->stop)/60;;
        }
    };

    const ChannelEpg &chepg= m_channelsEpg[chname];

    for(auto it = chepg.prgs.begin(); it!=chepg.prgs.end(); ++it){
        if(currentdt > it->start && currentdt < it->stop) {
            LOG_DEBUG(LOG_TAG, "--> prg Found <--");
            auto next = std::next(it);
            formatprg(&(*it), (next!=chepg.prgs.end()) ? &(*next) : nullptr);
            epg["name"] = chepg.name;
            return true;
        }
    }
    return false;
}
