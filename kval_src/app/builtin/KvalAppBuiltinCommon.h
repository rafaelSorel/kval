#ifndef KVALAPPBUILTINCOMMON_H
#define KVALAPPBUILTINCOMMON_H

#include <memory>
#include <QObject>
#include "../KvalAppShared.h"

namespace KvalApplication {

namespace UserInterface {

Q_NAMESPACE
enum class AlertType {
    Invalid = 0,
    Info,
    Success,
    Warning,
    Error,
    Count
};
Q_ENUM_NS(AlertType);

enum class BoxType {
    Invalid = 0,
    Singlebutton,
    Multibutton,
};
Q_ENUM_NS(BoxType);

enum class InputType {
    Invalid=0,
    Inputlist,
    Inputkeyboard,
    Inputfilebrowsing
};
Q_ENUM_NS(InputType);

/**
 * @brief The KvalUiException class
 */
class KvalUiException : public std::exception {
public:
    explicit KvalUiException(const char * m) : message{m} {}
    virtual const char * what() const noexcept override {return message.c_str();}
private:
    std::string message = "";
};

} // namespace UserInterface


} // namespace KvalApplication
#endif // KVALAPPBUILTINCOMMON_H
