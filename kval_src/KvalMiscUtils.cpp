
#include <sys/types.h>
#include <unistd.h>
#include <QObject>
#include <QFile>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

#include "KvalMiscUtils.h"
#define LOG_ACTIVATED
#define LOG_SRC UTILS
#include "KvalLogging.h"

/**
 * @brief MU_MiscUtils::getDataBetween
 * @param begin
 * @param end
 * @param source
 * @return
 */
QString MU_MiscUtils::getDataBetween(QString begin, QString end, QString &source)
{
    int startIndex = 0;
    if(begin.isNull())
    {
        startIndex = 0;
    }
    else
    {
        startIndex = source.indexOf(begin)+begin.length();
    }
    if(startIndex < 0) return QString();
    int endIndex = source.indexOf(end, startIndex);
    if(endIndex <= 0) return QString();
    if(end.isNull())
    {
        return source.mid(startIndex);
    }
    return source.mid(startIndex, endIndex - startIndex);
}

/**
 * @brief MU_MiscUtils::Trim
 * @param str
 * @return
 */
std::string& MU_MiscUtils::Trim(std::string &str)
{
  TrimLeft(str);
  return TrimRight(str);
}

/**
 * @brief isspace_c: hack to check only first byte of UTF-8 character
 * @param c
 * @return
 */
static int isspace_c(char c)
{
  return (c & 0x80) == 0 && ::isspace(c);
}

/**
 * @brief MU_MiscUtils::TrimLeft
 * @param str
 * @return
 */
std::string& MU_MiscUtils::TrimLeft(std::string &str)
{
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun(isspace_c))));
  return str;
}

/**
 * @brief MU_MiscUtils::TrimRight
 * @param str
 * @return
 */
std::string& MU_MiscUtils::TrimRight(std::string &str)
{
  str.erase(std::find_if(str.rbegin(),
                         str.rend(),
                         std::not1(std::ptr_fun(isspace_c))).base(),
                         str.end());
  return str;
}

/**
 * @brief MU_MiscUtils::Tokenize
 * @param input
 * @param delimiters
 * @return
 */
std::vector<std::string> MU_MiscUtils::Tokenize(const std::string &input, const std::string &delimiters)
{
  std::vector<std::string> tokens;
  Tokenize(input, tokens, delimiters);
  return tokens;
}

/**
 * @brief MU_MiscUtils::Tokenize
 * @param input
 * @param tokens
 * @param delimiter
 */
void MU_MiscUtils::Tokenize(const std::string& input,
                     std::vector<std::string>& tokens,
                     const char delimiter)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiter);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find(delimiter, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiter, nextDelimPos);
  }
}

/**
 * @brief MU_MiscUtils::Tokenize
 * @param input
 * @param tokens
 * @param delimiters
 */
void MU_MiscUtils::Tokenize(const std::string& input,
                     std::vector<std::string>& tokens,
                     const std::string& delimiters)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiters);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find_first_of(delimiters, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiters, nextDelimPos);
  }
}

/**
 * @brief MU_MiscUtils::CompareNoCase
 * @param s1
 * @param s2
 * @return
 */
int MU_MiscUtils::CompareNoCase(const char *s1, const char *s2)
{
  char c2; // we need only one char outside the loop
  do
  {
    const char c1 = *s1++; // const local variable should help compiler to optimize
    c2 = *s2++;
    // This includes the possibility that one of the characters is the null-terminator,
    // which implies a string mismatch.
    if (c1 != c2 && ::tolower(c1) != ::tolower(c2))
      return ::tolower(c1) - ::tolower(c2);
  } while (c2 != '\0'); // At this point, we know c1 == c2, so there's no need to test them both.
  return 0;
}

/**
 * @brief MU_MiscUtils::GetString
 * @param path
 * @param valstr
 * @return
 */
int MU_MiscUtils::GetString(const std::string& path, std::string& valstr)
{
  int len;
  char buf[256] = {0};

  int fd = open(path.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    valstr.clear();
    while ((len = read(fd, buf, 256)) > 0)
      valstr.append(buf, len);
    close(fd);

    MU_MiscUtils::Trim(valstr);

    return 0;
  }

  LOG_ERROR(LOG_TAG, "Error reading %s", path.c_str());

  valstr = "fail";
  return -1;
}

/**
 * @brief MU_MiscUtils::SetString
 * @param path
 * @param valstr
 * @return
 */
int MU_MiscUtils::SetString(const std::string& path, const std::string& valstr)
{
  int fd = open(path.c_str(), O_RDWR, 0644);
  int ret = 0;
  if (fd >= 0)
  {
    if (write(fd, valstr.c_str(), valstr.size()) < 0)
      ret = -1;
    close(fd);
  }
  if (ret)
    LOG_ERROR(LOG_TAG, "Error writing %s", path.c_str());

  return ret;
}

/**
 * @brief MU_MiscUtils::SetInt
 * @param path
 * @param val
 * @return
 */
int MU_MiscUtils::SetInt(const std::string& path, const int val)
{
  int fd = open(path.c_str(), O_RDWR, 0644);
  int ret = 0;
  if (fd >= 0)
  {
    char bcmd[16];
    sprintf(bcmd, "%d", val);
    if (write(fd, bcmd, strlen(bcmd)) < 0)
      ret = -1;
    close(fd);
  }
  if (ret)
    LOG_ERROR(LOG_TAG,"%s: error writing %s",__FUNCTION__, path.c_str());

  return ret;
}

/**
 * @brief MU_MiscUtils::GetInt
 * @param path
 * @param val
 * @return
 */
int MU_MiscUtils::GetInt(const std::string& path, int& val)
{
  int fd = open(path.c_str(), O_RDONLY);
  int ret = 0;
  if (fd >= 0)
  {
    char bcmd[16];
    if (read(fd, bcmd, sizeof(bcmd)) < 0)
      ret = -1;
    else
      val = strtol(bcmd, NULL, 16);

    close(fd);
  }
  if (ret)
    LOG_ERROR(LOG_TAG,"Error reading %s", path.c_str());

  return ret;
}
/**
 * @brief SysfsUtils::Has
 * @param path
 * @return
 */
bool MU_MiscUtils::Has(const std::string &path)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd >= 0)
  {
    close(fd);
    return true;
  }
  return false;
}

/**
 * @brief SysfsUtils::HasRW
 * @param path
 * @return
 */
bool MU_MiscUtils::HasRW(const std::string &path)
{
  int fd = open(path.c_str(), O_RDWR);
  if (fd >= 0)
  {
    close(fd);
    return true;
  }
  return false;
}

/**
 * @brief Util::RunCommandLine
 * @param cmdLine
 * @param waitExit
 * @return
 */
bool MU_MiscUtils::RunCommandLine(const QString& cmdLine, bool waitExit)
{
    QStringList args = cmdLine.split(",");

    // Strip quotes and whitespace around the arguments, or exec will fail.
    // This allows the python invocation to be written more naturally with any amount of whitespace around the args.
    // But it's still limited, for example quotes inside the strings are not expanded, etc.
    QStringList cleanedArgs;
    Q_FOREACH (QString arg, args)
    {
        arg = arg.trimmed();
        cleanedArgs.append(arg);
    }

    return Command(args, waitExit);
}

/**
 * @brief MU_MiscUtils::Command
 * @param arrArgs
 * @param waitExit
 * @return
 */
bool MU_MiscUtils::Command(const QStringList& arrArgs, bool waitExit)
{
    LOG_INFO(LOG_TAG,"Command %s", arrArgs[0].toStdString().c_str());
    QProcess command;
    command.setProgram(arrArgs[0]);
    command.setArguments(QStringList(arrArgs.begin()+1, arrArgs.end()));
    if (!waitExit)
    {
        LOG_INFO(LOG_TAG, "Process will be executed as a daemon");
        if (command.startDetached() && command.waitForStarted(3000U)){
            switch (command.state()) {
                case QProcess::Starting:
                case QProcess::Running:
                    return true;
                case QProcess::NotRunning:
                    return false;
            }
        }
        LOG_ERROR(LOG_TAG, "Unable to start daemon process");
        command.kill();
        return false;
    }

    command.start();
    if(command.waitForStarted(3000U))
    {
        LOG_INFO(LOG_TAG,
                 "[%s] started ,Wait 10s for proc to finish...",
                 qPrintable(arrArgs[0]));
        command.waitForFinished(10000U);
        if (command.state() == QProcess::NotRunning) {
            switch(command.exitStatus()) {
                case QProcess::NormalExit:
                    return true;
                case QProcess::CrashExit:
                default:
                    return false;
            }
        }
    }

    LOG_ERROR(LOG_TAG,
              "Unable to start [%s]",
              qPrintable(arrArgs[0]));
    command.kill();
    return false;
}

