#include <QString>
#include <QVector>
#include <QMetaEnum>

#include "../KvalAppProxy.h"
#include "KvalAppUiPopUp.h"

namespace KvalApplication {

namespace UserInterface {

/**
 * @brief Alert::Alert
 */
Alert::Alert() :
    Alert(AlertType::Invalid,
          "", "", "")
{
    qInfo() << "Default constructor";
}

/**
 * @brief Alert::Alert
 * @param type
 * @param header
 * @param body
 * @param icon
 * @param timeout
 */
Alert::Alert(AlertType _type,
             const string& _header,
             const string& _body,
             const string& _icon,
             int _timeout) :
    BuiltinBaseClass(),
    type{_type},
    header{_header},
    body{_body},
    icon{_icon},
    timeout{_timeout}

{
    qInfo() << "Extented constructor";
    connect(this, &Alert::displayAlertReq,
            _proxy().data(), &Proxy::onDisplayAlert,
            Qt::QueuedConnection);
}

/**
 * @brief Alert::display
 */
void Alert::display()
{
    qInfo() << *this;

    // Translate Ui msg:
    QMetaEnum _enum = QMetaEnum::fromType<UserInterface::AlertType>();
    QString _stype{_enum.valueToKey((int)type)};

    // This will typically be the only copy made toward the whole chain to UI,
    // Thanks to QT implicit sharing, no copy will be performed from this point.
    QVariantMap _alertMap {
    { "type", _stype },
    { "head", QString::fromStdString(header) },
    { "body", QString::fromStdString(body) },
    { "icon", QString::fromStdString(icon) },
    { "time", timeout }
    };

    _alertMap.setSharable(true);
    Q_EMIT displayAlertReq(_shref(), _alertMap);
}

/**
 * @brief Alert::clear
 */
void Alert::clear()
{

}

/**
 * @brief Alert::~Alert
 */
Alert::~Alert()
{
    qInfo() << "Destruct Alert";
}

/**
 * @brief ModalBaseDialog::ModalBaseDialog
 */
ModalBaseDialog::ModalBaseDialog():
    ModalBaseDialog("","","")
{

}

/**
 * @brief ModalBaseDialog::ModalBaseDialog
 * @param title
 * @param header
 * @param body
 * @param icon
 */
ModalBaseDialog::ModalBaseDialog(
        const string& title,
        const string& header,
        const string& body,
        const string& icon):
    BuiltinBaseClass(),
    title{title},
    header{header},
    body{body},
    icon{icon}
{
    qInfo() << "Instantiate";
}

/**
 * @brief ModalBaseDialog::~ModalBaseDialog
 */
ModalBaseDialog::~ModalBaseDialog()
{
    qInfo() << "Destroy";
}

/**
 * @brief ModalBaseDialog::display
 */
int ModalBaseDialog::display()
{
    return -1;
}

/**
 * @brief ModalBaseDialog::reply
 * @param _rep
 */
void ModalBaseDialog::onReply(const QVariant& _rep)
{
    qInfo() << "reply: " << _rep;
    QMutexLocker _lckr(&m_replyMtx);

    m_reply = _rep;
    m_waitReply.wakeAll();

}

/**
 * @brief ModalBaseDialog::userReply
 */
QVariant& ModalBaseDialog::reply()
{
    BlockingCallScope guard{};

    QMutexLocker _lckr(&m_replyMtx);
    // Wait for reply
    if(!m_reply.isValid())
        m_waitReply.wait(&m_replyMtx);

    return m_reply;
}

/**
 * @brief ModalBaseDialog::close
 */
void ModalBaseDialog::close()
{

}

/**
 * @brief ModalBaseDialog::abort
 */
void ModalBaseDialog::abort()
{
    m_aborted.store(true);
    QMutexLocker _lckr(&m_replyMtx);
    m_waitReply.wakeAll();
}

/**
 * @brief ModalBaseDialog::isAborted
 * @return
 */
bool ModalBaseDialog::isAborted()
{
    return m_aborted.load();
}

/**
 * @brief SimpleDialog::SimpleDialog
 */
SimpleDialog::SimpleDialog():
    SimpleDialog(BoxType::Invalid, "", "", "")
{

}

/**
 * @brief SimpleDialog::SimpleDialog
 * @param type
 * @param title
 * @param header
 * @param body
 * @param icon
 */
SimpleDialog::SimpleDialog(BoxType type, const string& title, const string& header,
         const string& body, const string& icon):
    ModalBaseDialog(title, header, body, icon),
    type(type)
{
    qInfo() << "Instantiate";

    connect(this, &SimpleDialog::displaySimpleDiag,
            _proxy().data(), &Proxy::onDisplaySimpleDiag,
            Qt::QueuedConnection);
}

/**
 * @brief SimpleDialog::~SimpleDialog
 */
SimpleDialog::~SimpleDialog()
{
    qInfo() << "Destroy";
}

/**
 * @brief SimpleDialog::display
 */
int SimpleDialog::display()
{
    if(buttons.empty()){
        throw KvalUiException("You need to add user interaction sets of buttons !");
    }

    QMetaEnum _enum = QMetaEnum::fromType<UserInterface::BoxType>();
    QString _stype{_enum.valueToKey((int)type)};

    // This will typically be the only copy made toward the whole chain to UI,
    // Thanks to QT implicit sharing, no copy will be performed from this point.
    QVariantList _buttons;
    for_each(begin(buttons), end(buttons), [&_buttons](const Button& _but) {
        QVariantMap _button {
            {"id", _but.id},
            {"val", QString::fromStdString(_but.value)},
            {"icon", QString::fromStdString(_but.icon)},
            {"rgb", QString::fromStdString(_but.rgb)}
        };
        _buttons.append(_button);
    });

    QVariantMap _dialogMap {
    { "type", _stype },
    { "title", QString::fromStdString(title) },
    { "head", QString::fromStdString(header) },
    { "body", QString::fromStdString(body) },
    { "icon", QString::fromStdString(icon) },
    { "buttons", _buttons }
    };

    Q_EMIT displaySimpleDiag(_shref(), _dialogMap);

    QVariant _rep = reply();
    if(_rep.isValid())
        return _rep.toInt();
    else
        return -1;
}


/**
 * @brief SimpleDialog::SimpleDialog
 */
ProgressDialog::ProgressDialog():
    ProgressDialog(BoxType::Invalid, "", "", "")
{

}

/**
 * @brief SimpleDialog::SimpleDialog
 * @param type
 * @param title
 * @param header
 * @param body
 * @param icon
 */
ProgressDialog::ProgressDialog(BoxType type, const string& title,
                         const string& header, const string& body,
                         const string& icon):
    SimpleDialog(type, title, header, body, icon)
{
    qInfo() << "Instantiate";

    connect(this, &ProgressDialog::displayProgressDiag,
            _proxy().data(), &Proxy::onDisplayProgressDiag,
            Qt::QueuedConnection);
    connect(this, &ProgressDialog::updateProgressDiag,
            _proxy().data(), &Proxy::onUpdateProgressDiag,
            Qt::QueuedConnection);
    connect(this, &ProgressDialog::closeProgressDiag,
            _proxy().data(), &Proxy::onCloseProgressDiag,
            Qt::QueuedConnection);
}

/**
 * @brief SimpleDialog::~SimpleDialog
 */
ProgressDialog::~ProgressDialog()
{
    qInfo() << "Destroy";
}

/**
 * @brief SimpleDialog::display
 */
int ProgressDialog::display()
{
    if(buttons.empty()){
        throw KvalUiException("You need to add user interaction sets of buttons !");
    }
    QMetaEnum _enum = QMetaEnum::fromType<UserInterface::BoxType>();
    QString _stype{_enum.valueToKey((int)type)};

    // This will typically be the only copy made toward the whole chain to UI,
    // Thanks to QT implicit sharing, no copy will be performed from this point.
    QVariantList _buttons;
    for_each(begin(buttons), end(buttons), [&_buttons](const Button& _but) {
        QVariantMap _button {
            {"id", _but.id},
            {"val", QString::fromStdString(_but.value)},
            {"icon", QString::fromStdString(_but.icon)},
            {"rgb", QString::fromStdString(_but.rgb)}
        };
        _buttons.append(_button);
    });

    QVariantMap _dialogMap {
    { "type", _stype },
    { "title", QString::fromStdString(title) },
    { "head", QString::fromStdString(header) },
    { "body", QString::fromStdString(body) },
    { "icon", QString::fromStdString(icon) },
    { "progress", progress },
    { "buttons", _buttons }
    };

    Q_EMIT displayProgressDiag(_shref(), _dialogMap);

    return true;
}

/**
 * @brief ProgressDialog::update
 * @return
 */
void ProgressDialog::update(int value, const string& _body)
{
    setProgress(value);
    QVariantMap _dialogMap {
    { "progress", value },
    { "body", QString::fromStdString(_body) }
    };

    Q_EMIT updateProgressDiag(_shref(), _dialogMap);

}

/**
 * @brief ProgressDialog::close
 */
void ProgressDialog::close()
{
    qInfo() << "close progress box";
    Q_EMIT closeProgressDiag(_shref());
}

/**
 * @brief ProgressDialog::abort
 */
void ProgressDialog::abort()
{
    qInfo() << "ProgressDialog requests Abort !";
    m_aborted.store(true);
}

/**
 * @brief InputDialog::InputDialog
 */
InputDialog::InputDialog(): InputDialog(InputType::Invalid, "","","","")
{

}

/**
 * @brief InputDialog::InputDialog
 * @param _type
 * @param _title
 * @param _header
 * @param _body
 * @param _icon
 */
InputDialog::InputDialog(InputType _type,
                         const string& _title,
                         const string& _header,
                         const string& _body,
                         const string& _icon):
    ModalBaseDialog(_title, _header, _body, _icon),
    type(_type)
{
    qInfo() << "Instantiate";
    qRegisterMetaType<vector<ModalBaseDialog::Entry>>
            ("vector<ModalBaseDialog::Entry>");
    connect(this, &InputDialog::displayInputList,
            _proxy().data(), &Proxy::onDisplayInputList,
            Qt::QueuedConnection);
    connect(this, &InputDialog::displayInputFileBrowse,
            _proxy().data(), &Proxy::onDisplayInputFileBrowse,
            Qt::QueuedConnection);
    connect(this, &InputDialog::displayInputKeyboard,
            _proxy().data(), &Proxy::onDisplayInputKeyboard,
            Qt::QueuedConnection);
}

/**
 * @brief InputDialog::~InputDialog
 */
InputDialog::~InputDialog()
{
    qInfo() << "Destroy";
}

/**
 * @brief InputDialog::inputList
 * @param _entries
 * @return
 */
int InputDialog::inputList(vector<Entry>& entries)
{
    qInfo() << "In: " << entries.size();
    if(entries.empty()){
        throw KvalUiException("Empty entry list !");
    }

    QMetaEnum _enum = QMetaEnum::fromType<UserInterface::InputType>();
    QString _stype{_enum.valueToKey((int)type)};

    // This will typically be the only copy made toward the whole chain to UI,
    // Thanks to QT implicit sharing, no copy will be performed from this point.
    QVariantList _inputs;
    for_each(begin(entries), end(entries), [&_inputs](const Entry& _ent) {
        QVariantMap _input {
            {"id", _ent.id},
            {"val", QString::fromStdString(_ent.value)},
            {"icon", QString::fromStdString(_ent.icon)}
        };
        _inputs.append(_input);
    });

    QVariantMap _inputMap {
    { "type", _stype },
    { "title", QString::fromStdString(title) },
    { "head", QString::fromStdString(header) },
    { "body", QString::fromStdString(body) },
    { "icon", QString::fromStdString(icon) },
    { "inputs", _inputs }
    };

    Q_EMIT displayInputList(_shref(), _inputMap);

    QVariant _rep = reply();
    if(_rep.isValid())
        return _rep.toInt();
    else
        return -1;
}

/**
 * @brief InputDialog::inputFileBrowser
 * @param _path
 * @param _extFilter
 * @return
 */
string InputDialog::inputFileBrowser(const string &path,
                                     const string &filter)
{
    if(path.empty()){
        throw KvalUiException("Empty path !");
    }

    Q_EMIT displayInputFileBrowse(_shref(),
                                  QString::fromStdString(path),
                                  QString::fromStdString(filter));

    QVariant _rep = reply();
    if(_rep.isValid())
        return _rep.toString().toStdString();
    else
        return "";

}

/**
 * @brief InputDialog::inputKeyBoard
 * @param _default
 * @return
 */
string InputDialog::inputKeyBoard(const string &_default)
{
    Q_EMIT displayInputKeyboard(_shref(), QString::fromStdString(_default));

    QVariant _rep = reply();
    if(_rep.isValid())
        return _rep.toString().toStdString();
    else
        return "";
}

} //namespace UserInterface

} // namespace KvalApplication
