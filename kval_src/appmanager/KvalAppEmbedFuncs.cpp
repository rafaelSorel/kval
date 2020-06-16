#define LOG_ACTIVATED
#include "KvalMiscUtils.h"
#include "KvalAppEmbedFuncs.h"

#include "KvalConfigManager.h"
#include "KvalVodManager.h"
#include "KvalResourceClientFetcher.h"

#define LOG_SRC EMBEDFUNCS
#include "KvalLogging.h"

#include "KvalAppManager.h"

UI_Element * g_uiRef;

/**
 * @brief RunPlugin
 * @param params
 * @return
 */
static int RunPlugin(const std::vector<std::string>& params)
{
    if(!params.size())
        return 0;
    LOG_INFO(LOG_TAG, "RunPlugin called.");
    LOG_INFO(LOG_TAG, "params: %s", params[0].c_str());
    QMetaObject::invokeMethod(g_appEngineRef,
                            "runPlugin",
                            Q_ARG(QString, QString::fromStdString(params[0])));
    return 0;
}

/**
 * @brief RunScript
 * @param params
 * @return
 */
static int RunScript(const std::vector<std::string>& params)
{
    if(!params.size())
        return 0;

    LOG_INFO(LOG_TAG, "RunScript called TO BE IMPLEMENTED");
    LOG_INFO(LOG_TAG, "params: %s", params[0].c_str());

    return 0;
}

/**
 * @brief Reboot
 * @param params
 * @return
 */
static int Reboot(const std::vector<std::string>& params)
{
    (void)params;
    LOG_INFO(LOG_TAG, "Reboot ...");

    QMetaObject::invokeMethod(g_uiRef ,"systemReboot", Qt::BlockingQueuedConnection);
    QMetaObject::invokeMethod(g_appEngineRef, "tryReboot");

    return 0;
}

/**
 * @brief Exec
 * @param params
 * @return
 */
template<int Wait=0>
static int Exec(const std::vector<std::string>& params)
{
    if(!params.size())
    {
        return -1;
    }
    MU_MiscUtils::RunCommandLine(QString(params[0].c_str()), Wait);
    return 0;
}

/**
 * @brief InstallApp
 * @param params
 * @return
 */
static int InstallApp(const std::vector<std::string>& params)
{
    std::string appId = "";
    std::string appUrl = "";
    std::string appVersion = "";
    std::string appHash = "";

    if(!params.size() || params.size() < 2)
        return 0;

    else if(params.size() > 3 )
    {
        appId = params[0];
        appUrl = params[1];
        appVersion = params[2];
        appHash = params[3];
    }
    else
    {
        appId = params[0];
        appUrl = params[1];
    }

    QMetaObject::invokeMethod(g_appEngineRef,
                            "installApp",
                            Q_ARG(QString, QString::fromStdString(appId)),
                            Q_ARG(QString, QString::fromStdString(appUrl)),
                            Q_ARG(QString, QString::fromStdString(appVersion)),
                            Q_ARG(QString, QString::fromStdString(appHash)));

  return 0;
}

/**
 * @brief EF_CmdFuncs::GetOperations
 * @return
 */
EF_EmbedFuncs::CommandMap EF_CmdFuncs::GetOperations() const
{
    return {
        {"runplugin",                  {"Run the specified plugin", 1, RunPlugin}},
        {"runscript",                  {"Run the specified script", 1, RunScript}},
        {"installapp",                 {"Install the specified plugin/script", 1, InstallApp}},
        {"reboot",                     {"Reboot the system", 0, Reboot}},
        {"system.exec",                {"Execute shell commands", 1, Exec<0>}},
    };
}


/**
 * @brief Refresh
 * @param params
 * @return
 */
static int Refresh(const std::vector<std::string>& params)
{
    (void)params;
    LOG_INFO(LOG_TAG, "Refresh current window content");
    QMetaObject::invokeMethod(g_uiRef, "refresh");

    return 0;
}

/**
 * @brief Update UI window contents
 * @param params The parameters.
 *        params[0] = The URL to update listing at.
 *        params[1] = "replace" to reset history (optional).
 */
static int Update(const std::vector<std::string>& params)
{
    if(!params.size())
        return 0;

    bool replace = false;
    if (params.size() > 1 &&
        QString::fromStdString(params[1]).compare("replace",
                                                  Qt::CaseInsensitive))
    {
        // reset the history
        replace = true;
    }

    LOG_INFO(LOG_TAG, "Update current window content");
    QMetaObject::invokeMethod(g_uiRef,
                              "update",
                              Q_ARG(QString, QString::fromStdString(params[0])),
                              Q_ARG(bool, replace));

    return 0;
}

/**
 * @brief Set view mode
 * @param params The parameters.
 *        params[0] = ID of view mode.
 *        params[1] = ID of subview mode.
 * @return
 */
static int SetViewMode(const std::vector<std::string>& params)
{
    if(!params.size())
        return -1;

    QMetaObject::invokeMethod(g_uiRef,
                              "setViewMode",
                              Q_ARG(int, atoi(params[0].c_str())),
                              Q_ARG(int, atoi(params[1].c_str())));

    return 0;
}

/**
 * @brief EF_ContainerUiFuncs::GetOperations
 * @return
 */
EF_EmbedFuncs::CommandMap EF_ContainerUiFuncs::GetOperations() const
{
    return {
        {"container.refresh",            {"Refresh current listing", 0, Refresh}},
        {"container.update",             {"Update current listing. Send Container.Update(path,replace) to reset the path history", 1, Update}},
        {"container.setviewmode",        {"Move to the view with the given id", 1, SetViewMode}},
#if 0
        {"container.nextsortmethod",     {"Change to the next sort method", 0, ChangeSortMethod<1>}},
        {"container.nextviewmode",       {"Move to the next view type (and refresh the listing)", 0, ChangeViewMode<1>}},
        {"container.previoussortmethod", {"Change to the previous sort method", 0, ChangeSortMethod<-1>}},
        {"container.previousviewmode",   {"Move to the previous view type (and refresh the listing)", 0, ChangeViewMode<-1>}},
        {"container.setsortdirection",   {"Toggle the sort direction", 0, ToggleSortDirection}},
        {"container.setsortmethod",      {"Change to the specified sort method", 1, SetSortMethod}},
#endif
    };
}

/**
 * @brief EF_EmbedFuncs::EF_EmbedFuncs
 */
EF_EmbedFuncs::EF_EmbedFuncs()
{
    RegisterCommands<EF_CmdFuncs>();
    RegisterCommands<EF_ContainerUiFuncs>();
    RegisterCommands<EF_ControlUiFuncs>();
}

/**
 * @brief EF_EmbedFuncs::setAppsEngineRef
 * @param owner
 */
void EF_EmbedFuncs::setUiElementRef(QObject * owner)
{
    g_uiRef = (UI_Element*)owner;
}

/**
 * @brief EF_EmbedFuncs::~EF_EmbedFuncs
 */
EF_EmbedFuncs::~EF_EmbedFuncs()
{
}

/**
 * @brief EF_EmbedFuncs::GetInstance
 * @return
 */
EF_EmbedFuncs& EF_EmbedFuncs::GetInstance()
{
    static EF_EmbedFuncs sBuiltins;
    return sBuiltins;
}

/**
 * @brief EF_EmbedFuncs::HasCommand
 * @param execString
 * @return
 */
bool EF_EmbedFuncs::HasCommand(const QString& execString)
{
    QString function;
    std::vector<std::string> parameters;
    SplitExecFunction(execString, function, parameters);
    function = function.toLower();

    const auto& it = m_command.find(function.toStdString());
    if (it != m_command.end())
    {
      if (it->second.parameters == 0 || it->second.parameters <= parameters.size())
        return true;
    }

    return false;
}

/**
 * @brief EF_EmbedFuncs::IsSystemPowerdownCommand
 * @param execString
 * @return
 */
bool EF_EmbedFuncs::IsSystemPowerdownCommand(const QString& execString)
{
    (void)execString;
    return false;
}

/**
 * @brief EF_EmbedFuncs::GetHelp
 * @param help
 */
void EF_EmbedFuncs::GetHelp(std::string &help)
{
    help.clear();

    for (const auto& it : m_command)
    {
        help += it.first;
        help += "\t";
        help += it.second.description;
        help += "\n";
    }
}

/**
 * @brief EF_EmbedFuncs::Execute
 * @param execString
 * @return
 */
int EF_EmbedFuncs::Execute(const QString &execString)
{
    if (execString.isEmpty())
    {
        LOG_ERROR(LOG_TAG, "Empty exec function !");
        return -1;
    }
    LOG_INFO(LOG_TAG, "Execute function: %s",qPrintable(execString));
    QString execute;
    std::vector<std::string> params;
    SplitExecFunction(execString, execute, params);
    execute = execute.toLower();

    LOG_INFO(LOG_TAG,"extracted function: %s",qPrintable(execute));

    const auto& it = m_command.find(execute.toStdString());
    if (it != m_command.end())
    {
        if (it->second.parameters == 0 || params.size() >= it->second.parameters)
        {
            return it->second.Execute(params);
        }
        else
        {
            LOG_ERROR(LOG_TAG,
                      "%s called with invalid number of params (should be: %zd, is %zd)",
                      execute.toStdString().c_str(),
                      it->second.parameters,
                      params.size());
        }
    }
    return -1;
}

/**
 * @brief EF_EmbedFuncs::SplitExecFunction
 * @param execString
 * @param function
 * @param parameters
 */
void EF_EmbedFuncs::SplitExecFunction(const QString &execString,
                                     QString &function,
                                     std::vector<std::string> &parameters)
{
    QString paramString;

    int iPos = execString.indexOf("(");
    int iPos2 = execString.indexOf(")");
    if(iPos<0 || iPos2<0)
    {
        function = execString;
    }
    else
    {
        paramString = execString.mid(iPos+1, iPos2 - iPos - 1);
        function = execString.mid(0, iPos);
    }
    function = function.trimmed();
    if(function.startsWith("kvalbinder.", Qt::CaseInsensitive))
    {
        function.remove(0, 12);
    }

    LOG_INFO(LOG_TAG, "function: %s", qPrintable(function));
    LOG_INFO(LOG_TAG, "split paramString: %s", qPrintable(paramString));
    SplitParams(paramString.toStdString(), parameters);
}

/**
 * @brief EF_EmbedFuncs::SplitParams
 * @param paramString
 * @param parameters
 */
void EF_EmbedFuncs::SplitParams(const std::string &paramString,
                               std::vector<std::string> &parameters)
{
    bool inQuotes = false;
    bool lastEscaped = false; // only every second character can be escaped
    int inFunction = 0;
    size_t whiteSpacePos = 0;
    std::string parameter;
    parameters.clear();
    for (size_t pos = 0; pos < paramString.size(); pos++)
    {
        char ch = paramString[pos];
        bool escaped = (pos > 0 && paramString[pos - 1] == '\\' && !lastEscaped);
        lastEscaped = escaped;
        if (inQuotes)
        { // if we're in a quote, we accept everything until the closing quote
            if (ch == '"' && !escaped)
            { // finished a quote - no need to add the end quote to our string
                inQuotes = false;
            }
        }
        else
        { // not in a quote, so check if we should be starting one
            if (ch == '"' && !escaped)
            { // start of quote - no need to add the quote to our string
                inQuotes = true;
            }
            if (inFunction && ch == ')')
            { // end of a function
                inFunction--;
            }
            if (ch == '(')
            { // start of function
                inFunction++;
            }
            if (!inFunction && ch == ',')
            { // not in a function, so a comma signfies the end of this parameter
                if (whiteSpacePos)
                    parameter = parameter.substr(0, whiteSpacePos);
            // trim off start and end quotes
            if (parameter.length() > 1 &&
                parameter[0] == '"' &&
                parameter[parameter.length() - 1] == '"')
            {
                parameter = parameter.substr(1, parameter.length() - 2);
            }

            else if (parameter.length() > 3 &&
                     parameter[parameter.length() - 1] == '"')
            {
                // check name="value" style param.
                size_t quotaPos = parameter.find('"');
                if (quotaPos > 1 &&
                    quotaPos < parameter.length() - 1 &&
                    parameter[quotaPos - 1] == '=')
                {
                    parameter.erase(parameter.length() - 1);
                    parameter.erase(quotaPos);
                }
            }
            parameters.push_back(parameter);
            parameter.clear();
            whiteSpacePos = 0;
            continue;
            }
        }
        if ((ch == '"' || ch == '\\') && escaped)
        { // escaped quote or backslash
            parameter[parameter.size()-1] = ch;
            continue;
        }
        // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
        if (ch == ' ' && !inQuotes)
        {
        if (parameter.empty()) // skip whitespace on left
            continue;
        if (!whiteSpacePos) // make a note of where whitespace starts on the right
            whiteSpacePos = parameter.size();
        }
        else
        {
            whiteSpacePos = 0;
        }
        parameter += ch;
    }
    if (inFunction || inQuotes)
    {
        LOG_WARNING(LOG_TAG,
                    "(%s) - end of string while searching for ) or \"",
                    paramString.c_str());
    }

    if (whiteSpacePos)
    {
        parameter.erase(whiteSpacePos);
    }
    // trim off start and end quotes
    if (parameter.size() > 1 && parameter[0] == '"' && parameter[parameter.size() - 1] == '"')
    {
        parameter = parameter.substr(1,parameter.size() - 2);
    }

    else if (parameter.size() > 3 && parameter[parameter.size() - 1] == '"')
    {
        // check name="value" style param.
        size_t quotaPos = parameter.find('"');
        if (quotaPos > 1 && quotaPos < parameter.length() - 1 && parameter[quotaPos - 1] == '=')
        {
          parameter.erase(parameter.length() - 1);
          parameter.erase(quotaPos);
        }
    }
    if (!parameter.empty() || parameters.size())
    {
        parameters.push_back(parameter);
    }
}
