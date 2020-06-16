
#include <fcntl.h>
#include <QStringList>
#include <QObject>

/**
 * @brief The MU_MiscUtils class
 */
class MU_MiscUtils
{

public:

    MU_MiscUtils() {}
    ~MU_MiscUtils() {}

    /**=============== Strings utils ========================*/
    static QString getDataBetween(QString begin, QString end, QString &source);
    static std::string& TrimRight(std::string &str);
    static std::string& TrimLeft(std::string &str);
    static std::string& Trim(std::string &str);
    static int  CompareNoCase(const char *s1, const char *s2);
    static std::vector<std::string> Tokenize(const std::string &input,
                                             const std::string &delimiters);
    static void Tokenize(const std::string& input,
                         std::vector<std::string>& tokens,
                         const std::string& delimiters);
    static void Tokenize(const std::string& input,
                         std::vector<std::string>& tokens,
                         const char delimiter);

    /**=============== SysFs utils ========================*/
    static int GetString(const std::string& path, std::string& valstr);
    static int SetString(const std::string& path, const std::string& valstr);
    static int GetInt(const std::string& path, int& val);
    static int SetInt(const std::string& path, const int val);
    static bool Has(const std::string &path);
    static bool HasRW(const std::string &path);

    static bool Command(const QStringList& arrArgs, bool waitExit);
    static bool RunCommandLine(const QString& cmdLine, bool waitExit);

};

struct sortstringbyname
{
  bool operator()(const std::string& strItem1, const std::string& strItem2)
  {
    return MU_MiscUtils::CompareNoCase(strItem1.c_str(), strItem1.c_str()) < 0;
  }
};

