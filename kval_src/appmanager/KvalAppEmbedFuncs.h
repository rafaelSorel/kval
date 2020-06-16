#ifndef KVAL_EMBEDFUNCS_H
#define KVAL_EMBEDFUNCS_H

#include <map>
#include <string>
#include <vector>

#include <atomic>
#include <QObject>
#include <QMap>

/**
 * @brief The EF_EmbedFuncs class
 */
class EF_EmbedFuncs
{
public:
    //! \brief Struct representing a command from handler classes.
    struct BUILT_IN_STRUCT
    {
        std::string description; //!< Description of command (help string)
        size_t parameters;       //!< Number of required parameters (can be 0)
        int (*Execute)(const std::vector<std::string>& params); //!< Function to handle command
    };

    //! \brief A map of commands
    typedef std::map<std::string, EF_EmbedFuncs::BUILT_IN_STRUCT> CommandMap;

    void setUiElementRef(QObject *owner);
    static EF_EmbedFuncs& GetInstance();
    bool HasCommand(const QString& execString);
    bool IsSystemPowerdownCommand(const QString& execString);
    void GetHelp(std::string &help);
    int Execute(const QString &execString);


    EF_EmbedFuncs();
    EF_EmbedFuncs(const EF_EmbedFuncs&);
    const EF_EmbedFuncs& operator=(const EF_EmbedFuncs&);
    ~EF_EmbedFuncs();

private:
    CommandMap m_command; //!< Map of registered commands

    //! \brief Convenience template used to register commands from providers
    template<class T>
    void RegisterCommands()
    {
        T t;
        CommandMap map = t.GetOperations();
        m_command.insert(map.begin(), map.end());
    }
    void SplitExecFunction(const QString &execString,
                           QString &function,
                           std::vector<std::string> &parameters);
    void SplitParams(const std::string &paramString,
                     std::vector<std::string> &parameters);

};

/**
 * @brief The Class providing Apps container related built-in commands.
 */
class EF_CmdFuncs
{
public:
    EF_EmbedFuncs::CommandMap GetOperations() const;
};

/**
 * @brief The Class providing UI container related built-in commands.
 */
class EF_ContainerUiFuncs
{
public:
    EF_EmbedFuncs::CommandMap GetOperations() const;
};

/**
 * @brief Class providing UI control related built-in commands.
 */
class EF_ControlUiFuncs
{
public:
    /**
     * @brief Returns the UI control map of operations.
     * @return
     */
    EF_EmbedFuncs::CommandMap GetOperations() const
    {
        return {
#if 0
        {"control.message",  {"Send a given message to a control within a given window", 2, SendMessage}},
        {"control.move",     {"Tells the specified control to 'move' to another entry specified by offset", 2, ControlMove}},
        {"control.setfocus", {"Change current focus to a different control id", 1, SetFocus}},
        {"pagedown",         {"Send a page down event to the pagecontrol with given id", 1, ShiftPage<GUI_MSG_PAGE_DOWN>}},
        {"pageup",           {"Send a page up event to the pagecontrol with given id", 1, ShiftPage<GUI_MSG_PAGE_UP>}},
        {"sendclick",        {"Send a click message from the given control to the given window", 1, SendClick}},
        {"setfocus",         {"Change current focus to a different control id", 1, SetFocus}},
#endif
        };
    }
};

#endif // KVAL_EMBEDFUNCS_H
