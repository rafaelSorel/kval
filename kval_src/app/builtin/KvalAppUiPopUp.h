#ifndef ALERT_H
#define ALERT_H

#include <QMutex>
#include <QWaitCondition>
#include "KvalAppAbsBuiltin.h"

namespace KvalApplication {

namespace UserInterface {

/**
 * @brief The Alert class
 */
class Alert: public BuiltinBaseClass
{
    Q_OBJECT

public :
    explicit Alert();
    explicit Alert(AlertType _type,
                   const string& _header,
                   const string& _body,
                   const string& _icon = "",
                   int _timeout = 15);

    Alert(const Alert& rhs):
        BuiltinBaseClass(),
        type{rhs.type},
        header{rhs.header},
        body{rhs.body},
        icon{rhs.icon},
        timeout{rhs.timeout} {
        qInfo() << "copy constructor";
    }

    Alert(Alert&& rhs):
        BuiltinBaseClass(),
        type{rhs.type},
        header{std::move(rhs.header)},
        body{std::move(rhs.body)},
        icon{std::move(rhs.icon)},
        timeout{rhs.timeout} {
        qInfo() << "move constructor";
    }

    Alert& operator=(const Alert& rhs) {
        qInfo() << "Copy assignement";
        type = rhs.type;
        header = rhs.header;
        body = rhs.body;
        icon = rhs.icon;
        timeout = rhs.timeout;
        return *this;
    }
    Alert& operator=(Alert&& rhs) {
        qInfo() << "Move assignement";
        type = rhs.type;
        header = std::move(rhs.header);
        body = std::move(rhs.body);
        icon = std::move(rhs.icon);
        timeout = rhs.timeout;
        return *this;
    }

    virtual ~Alert();

    void display();
    void clear();


    friend QDebug operator<<(QDebug d, const Alert& rhs) {
        d.nospace();
        d << "type=" << static_cast<int>(rhs.type)
            << ", header='" << rhs.header.c_str()
            << "', body='" << rhs.body.c_str()
            << "', icon='" << rhs.icon.c_str()
            << "', timout=" << rhs.timeout;
        return d;
    }

    AlertType getType() { return type; }
    void setType(AlertType value)  { type = value; }

    string getHeader() const { return header; }
    void setHeader(const string &value) { header = value; }

    string getBody() const { return body; }
    void setBody(const string &value) { body = value; }

    string getIcon() const { return icon; }
    void setIcon(const string &value) { icon = value; }

    int getTimeout() const { return timeout; }
    void setTimeout(int value) { timeout = value; }

Q_SIGNALS:
    void displayAlertReq(QObjSharedPtr, const QVariantMap&);

private:
    AlertType type{AlertType::Invalid};
    string header{};
    string body{};
    string icon{};
    int timeout;
};

/**
 * @brief The ModalBaseDialog class
 */
class ModalBaseDialog: public BuiltinBaseClass
{
    Q_OBJECT

public:

    ModalBaseDialog();
    explicit ModalBaseDialog(const string&,
                const string&,
                const string&,
                const string& icon="");

    ModalBaseDialog(const ModalBaseDialog& rhs):
        BuiltinBaseClass(),
        title{rhs.title},
        header{rhs.header},
        body{rhs.body},
        icon{rhs.icon} {
        qInfo() << "copy constructor";
    }

    ModalBaseDialog(ModalBaseDialog&& rhs):
        BuiltinBaseClass(),
        title{std::move(rhs.title)},
        header{std::move(rhs.header)},
        body{std::move(rhs.body)},
        icon{std::move(rhs.icon)} {
        qInfo() << "move constructor";
    }

    ModalBaseDialog& operator=(const ModalBaseDialog& rhs) {
        qInfo() << "Copy assignement";
        if(&rhs == this) {
            return *this;
        }
        title = rhs.title;
        header = rhs.header;
        body = rhs.body;
        icon = rhs.icon;
        return *this;
    }
    ModalBaseDialog& operator=(ModalBaseDialog&& rhs) {
        qInfo() << "Move assignement";
        title = std::move(rhs.title);
        header = std::move(rhs.header);
        body = std::move(rhs.body);
        icon = std::move(rhs.icon);
        return *this;
    }

    virtual ~ModalBaseDialog();

    virtual int display();
    virtual void close();
    virtual void abort();

    QVariant& reply();
    bool isAborted();

    struct Entry {
        Entry() {};
        Entry(int id, const string& _val, const string _icon):
            id(id), value(_val), icon(_icon) {
            qDebug() << "Instantiate";
        }

        Entry(const Entry& rhs):
            id(rhs.id), value(rhs.value), icon(rhs.icon) {
            qDebug() << "copy constructor";
        };

        Entry(Entry&& rhs):
            id(rhs.id), value(std::move(rhs.value)), icon(std::move(rhs.icon)) {
            qDebug() << "move constructor";
        };

        Entry& operator=(const Entry& rhs) {
            qDebug() << "operator= copy";
            if (this == &rhs) {
                return *this;
            }
            id = rhs.id;
            value = rhs.value;
            icon = rhs.icon;
            return *this;
        }

        Entry& operator=(Entry&& rhs) {
            qDebug() << "operator= move";
            id = rhs.id;
            value = std::move(rhs.value);
            icon = std::move(rhs.icon);
            return *this;
        }

        ~Entry() {
            qDebug() << "Destroy";
        }
        int id;
        string value;
        string icon;
    };

    string getTitle() const { return title; };
    void setTitle(const string &value) { title = value; };

    string getHeader() const { return header; }
    void setHeader(const string &value) { header = value; };

    string getBody() const { return body; }
    void setBody(const string &value) { body = value; }

    string getIcon() const { return icon; };
    void setIcon(const string &value) { icon = value; }

public Q_SLOTS:
    void onReply(const QVariant&);

protected:
    atomic_bool m_aborted{false};
    string title{};
    string header{};
    string body{};
    string icon{};

private:
    QVariant m_reply{};
    QMutex m_replyMtx;
    QWaitCondition m_waitReply;

};

/**
 * @brief The MessageBox class
 */
class SimpleDialog: public ModalBaseDialog
{
    Q_OBJECT

public:
    struct Button: ModalBaseDialog::Entry
    {
        using ModalBaseDialog::Entry::Entry;
        Button() {};
        Button(int id, const string& _val, const string _icon, const string& _rgb) :
            ModalBaseDialog::Entry(id, _val, _icon),
            rgb(_rgb) {}

        string rgb;
    };

    explicit SimpleDialog();
    explicit SimpleDialog(BoxType, const string&, const string&,
                 const string&, const string& icon="");

    SimpleDialog(const SimpleDialog& rhs):
        ModalBaseDialog(rhs),
        buttons(rhs.buttons),
        type(rhs.type) {
        qInfo() << "copy constructor";
    }

    SimpleDialog(SimpleDialog&& rhs):
        ModalBaseDialog(std::move(rhs)),
        buttons(std::move(rhs.buttons)),
        type(rhs.type) {
        qInfo() << "move constructor";
    }

    SimpleDialog& operator=(const SimpleDialog& rhs) {
        qInfo() << "Copy assignement";
        if(&rhs == this) {
            return *this;
        }
        ModalBaseDialog::operator=(rhs);
        type = rhs.type;
        buttons = rhs.buttons;
        return *this;
    }

    SimpleDialog& operator=(SimpleDialog&& rhs) {
        qInfo() << "Move assignement";
        ModalBaseDialog::operator=(std::move(rhs));
        type = rhs.type;
        buttons = std::move(rhs.buttons);
        return *this;
    }

    virtual ~SimpleDialog();

    virtual int display() override;

    void setType(BoxType value) { type = value; }
    BoxType getType() const { return type; }

    void addButton(const Button& _but) { buttons.push_back(_but); }
    void setButtons(const vector<Button>& _but) { buttons = _but; }
    vector<Button> getButtons() const { return buttons; }

Q_SIGNALS:
    void displaySimpleDiag(QObjSharedPtr, const QVariantMap&);

protected:
    vector<Button> buttons;
    BoxType type;
};

/**
 * @brief The ProgressDialog class
 */
class ProgressDialog: public SimpleDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog();
    explicit ProgressDialog(BoxType, const string&, const string&,
                 const string&, const string& icon="");

    ProgressDialog(const ProgressDialog& rhs):
        SimpleDialog(rhs) {
        qInfo() << "copy constructor";
    }

    ProgressDialog(ProgressDialog&& rhs):
        SimpleDialog(std::move(rhs)) {
        qInfo() << "move constructor";
    }

    ProgressDialog& operator=(const ProgressDialog& rhs) {
        qInfo() << "Copy assignement";
        if(&rhs == this) {
            return *this;
        }
        SimpleDialog::operator=(rhs);
        return *this;
    }

    ProgressDialog& operator=(ProgressDialog&& rhs) {
        qInfo() << "Move assignement";
        SimpleDialog::operator=(std::move(rhs));
        return *this;
    }

    virtual ~ProgressDialog();

    virtual int display() override;
    virtual void close() override;
    virtual void abort() override;

    void update(int, const string& _body="");

    void setProgress(int value){ progress = value; }
    int getProgress() const { return progress; }

Q_SIGNALS:
    void displayProgressDiag(QObjSharedPtr, const QVariantMap&);
    void updateProgressDiag(QObjSharedPtr, const QVariantMap&);
    void closeProgressDiag(QObjSharedPtr);

private:
    int progress{0};
};

/**
 * @brief The MessageInput class
 */
class InputDialog: public ModalBaseDialog
{
    Q_OBJECT

public:
    explicit InputDialog();
    explicit InputDialog(InputType, const string&, const string&,
                       const string&, const string& icon="");

    virtual ~InputDialog();

    void setType(InputType value) { type = value; }
    InputType getType() const { return type; }

    int inputList(vector<Entry>& _entries);
    string inputFileBrowser(const string& path, const string& filter="");
    string inputKeyBoard(const string& _default="");

Q_SIGNALS:
    void displayInputList(QObjSharedPtr, const QVariantMap&);
    void displayInputFileBrowse(QObjSharedPtr, const QString&, const QString&);
    void displayInputKeyboard(QObjSharedPtr, const QString&);

private:
    InputType type;
};

} //namespace UserInterface

} // namespace KvalApplication
#endif // ALERT_H
