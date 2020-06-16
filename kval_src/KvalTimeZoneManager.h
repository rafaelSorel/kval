#ifndef KVALTIMEZONEMANAGER_H
#define KVALTIMEZONEMANAGER_H

#include <QObject>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QScopedPointer>
#include <QPointer>
#include <QSettings>

namespace KvalTimeZonePlatform {
/**
 * @brief The Timezone class
 */
class TimeZoneManager : public QObject
{
public:
   TimeZoneManager();
   virtual ~TimeZoneManager() = default;

   virtual const QStringList &GetCounties();
   virtual const QStringList &GetTimezonesByCountry(const QString&);
   virtual QString GetCountryByTimezone(const QString&);
   virtual QString getCurrent();
   virtual bool refreshDateAndTime();
   virtual void set(const QString&);

private:
   void loadtzfile();
   bool loadTimeZone();
   bool loadCounties();

private:
   QStringList m_counties{};
   QMap<QString, QString> m_countryByCode{};
   QMap<QString, QString> m_countryByName{};
   QMap<QString, QStringList> m_timezonesByCountryCode{};
   QMap<QString, QString> m_countriesByTimezoneName{};
};

} //namespace KvalTimeZonePlatform

#endif // KVALTIMEZONEMANAGER_H
