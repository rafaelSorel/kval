#define LOG_ACTIVATED
#include <QProcess>
#include "KvalTimeZoneManager.h"
#define LOG_SRC SETMANAGER
#include "KvalLogging.h"

namespace KvalTimeZonePlatform {
/**
 * @brief TimeZoneManager::TimeZone
 */
TimeZoneManager::TimeZoneManager() :
    m_counties{},
    m_countryByCode{},
    m_countryByName{},
    m_timezonesByCountryCode{},
    m_countriesByTimezoneName{}
{
    loadTimeZone();
    loadCounties();
    loadtzfile();
}

/**
 * @brief TimeZoneManager::loadTimeZone
 * @return
 */
bool TimeZoneManager::loadTimeZone()
{
    QString countryCode{};
    QString timeZone{};

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
    // Load timezones
    QFile fp("/usr/share/zoneinfo/zone.tab");
    if(!fp.open(QIODevice::ReadOnly))
    {
        LOG_ERROR(LOG_TAG, "Unable to open file path !!");
        return false;
    }
    QTextStream fileStream(&fp);

    while(!fileStream.atEnd()) {
        QString line = fileStream.readLine();
        line= line.trimmed();
        if(line.isEmpty() || line[0] == '#')
            continue;
        QStringList tokens = line.split(" \t", QString::SkipEmptyParts);
        if (tokens.size() < 3)
           continue;

        countryCode = tokens[0];
        timeZone = tokens[2];
        if(m_timezonesByCountryCode.contains(countryCode)) {
            m_timezonesByCountryCode[countryCode].append(timeZone);
        }
        else {
            m_timezonesByCountryCode.insert(countryCode, QStringList{timeZone});
        }

        m_countriesByTimezoneName.insert(timeZone, countryCode);

    }
    fp.close();
#else
    // @TODO: for windows...
#endif
    return true;
}

/**
 * @brief TimeZoneManager::loadCounties
 * @return
 */
bool TimeZoneManager::loadCounties()
{
    QString countryCode{};
    QString countryName{};

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
    QFile fp;
    QMap<QString, bool> countryCodeFiles{
        {"/usr/share/zoneinfo/iso3166.tab", false},
        {"/usr/share/misc/iso3166", true}
    };

   // Load countries
    Q_FOREACH (const QString& filepath, countryCodeFiles.keys()) {
        fp.setFileName(filepath);
        if(fp.open(QIODevice::ReadOnly))
            break;
    }

    if(!fp.isReadable()){
        LOG_ERROR(LOG_TAG, "Unable to open country file !!");
        return false;
    }
    QTextStream countryCodeFstream(&fp);

    while(!countryCodeFstream.atEnd()) {
        QString line = countryCodeFstream.readLine();
        line= line.trimmed();
        if(line.isEmpty() || line[0] == '#')
            continue;
        QStringList tokens = line.split(" \t", QString::SkipEmptyParts);
        if (tokens.size() != 2)
           continue;

        countryCode = tokens[0];
        countryName = tokens[1];
        if(countryCode.size() > 3)
            continue;

        m_countryByCode[countryCode] = countryName;
        m_countryByName[countryName] = countryCode;
        m_counties.append(countryName);
    }

    fp.close();
    std::sort(m_counties.begin(), m_counties.end());
#else
    // @TODO: for windows...
#endif

    return true;
}
/**
 * @brief TimeZoneManager::GetCounties
 * @return
 */
const QStringList &TimeZoneManager::GetCounties()
{
    return m_counties;
}

/**
 * @brief TimeZoneManager::GetTimezonesByCountry
 * @param country
 * @return
 */
const QStringList &TimeZoneManager::GetTimezonesByCountry(
        const QString& country)
{
    if( m_countryByName.contains(country) &&
        m_timezonesByCountryCode.contains(m_countryByName[country]))
        return m_timezonesByCountryCode[m_countryByName[country]];

    return std::move<QStringList>(QStringList());
}

/**
 * @brief TimeZoneManager::GetCountryByTimezone
 * @param timezone
 * @return
 */
QString TimeZoneManager::GetCountryByTimezone(
        const QString& timezone)
{
    if (m_countriesByTimezoneName.contains(timezone) &&
        m_countryByCode.contains(m_countriesByTimezoneName[timezone]))
        return m_countryByCode[m_countriesByTimezoneName[timezone]];

    return "";
}

/**
 * @brief TimeZoneManager::init
 * @return
 */
void TimeZoneManager::loadtzfile()
{
#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)

#ifdef AMLOGIC_TARGET
    QFile timezonefile("/storage/.cache/timezone");
    if(timezonefile.exists())
    {
        LOG_INFO(LOG_TAG, "Time zone file exists.");
        return;
    }

    //Create the timezone file.
    timezonefile.open(QIODevice::WriteOnly);
    QTextStream tzStreamFile(&timezonefile);
    tzStreamFile << "TIMEZONE=Europe/Paris" << endl;
    tzStreamFile.flush();
    timezonefile.close();
    system("systemctl restart tz-data.service");
#endif //AMLOGIC_TARGET

#else // defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#endif // defined (Q_OS_LINUX) || defined (Q_OS_OSX)
}

/**
 * @brief TimeZoneManager::set
 * @param timezone
 */
void TimeZoneManager::set(const QString& timezone)
{

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)

#ifdef AMLOGIC_TARGET
    QFile file("/storage/.cache/timezone");
    if (!file.open(QIODevice::Truncate))
    {
        LOG_ERROR(LOG_TAG, "Unable to open timezone file for update");
        return;
    }
    QTextStream tzStreamFile(&file);
    tzStreamFile << "TIMEZONE=" << timezone << endl;
    tzStreamFile.flush();
    file.close();

    system("systemctl restart tz-data.service");
    static char env_var[255];
    sprintf(env_var, "TZ=:%s", timezone.toStdString().c_str());
    putenv(env_var);
    tzset();
    refreshDateAndTime();
#else
    Q_UNUSED(timezone)
#endif//AMLOGIC_TARGET

#else// defined (Q_OS_LINUX) || defined (Q_OS_OSX)
    Q_UNUSED(timezone)
#endif// defined (Q_OS_LINUX) || defined (Q_OS_OSX)

}

/**
 * @brief TimeZoneManager::refreshDateAndTime
 * @return
 */
bool TimeZoneManager::refreshDateAndTime()
{
    bool status{false};

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)

#ifdef AMLOGIC_TARGET
    QProcess refresh_time;
    refresh_time.start("service ntp stop");
    refresh_time.waitForFinished();

    refresh_time.start("ntpdate -s time.nist.gov");
    refresh_time.waitForFinished();

    refresh_time.start("service ntp start");
    refresh_time.waitForFinished();

    QString stderrr = refresh_time.readAllStandardError();
    if(stderrr.isEmpty())
    {
        LOG_DEBUG(LOG_TAG, "Success fetching date and time");
        status=true;
    }
    else
    {
        LOG_WARNING(LOG_TAG, "Could not extract date and time");
        status=false;
    }
#else
#endif//AMLOGIC_TARGET

#else// defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#endif// defined (Q_OS_LINUX) || defined (Q_OS_OSX)

    return status;
}

/**
 * @brief TimeZoneManager::getCurrent
 * @return
 */
QString TimeZoneManager::getCurrent()
{

#if defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#ifdef AMLOGIC_TARGET
    QFile timezonefile("/storage/.cache/timezone");
    if (!timezonefile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_INFO(LOG_TAG, "Time zone file does not exists.");
        return "Europe/Paris";
    }

    QTextStream in(&timezonefile);
    QString line = in.readLine();
    int index = line.indexOf("TIMEZONE=");
    if(index > 0){
        return line.mid(index).simplified();
    }
#endif //AMLOGIC_TARGET
#else //defined (Q_OS_LINUX) || defined (Q_OS_OSX)
#endif // defined (Q_OS_LINUX) || defined (Q_OS_OSX)

    return "";
}

} //namespace KvalTimeZonePlatform
