#ifndef PROXY_H
#define PROXY_H


#include <QObject>
#include <QDebug>
#include <QMutex>
#include "KvalAppShared.h"
#include "builtin/KvalAppUiPopUp.h"

using namespace std;

namespace KvalApplication {

using namespace UserInterface;

class Proxy;
extern QPointer<Proxy> _shproxy;
/**
 * @brief The Activity struct
 */
struct Activity: ActivityClass {
    // TODO: Handle Ui stuff
};

/**
 * @brief The Service struct
 */
struct Service: ActivityClass {
};

/**
 * @brief The Proxy class
 */
class Proxy : public QObject
{
    Q_OBJECT
public:
    explicit Proxy(QObject *parent = nullptr);
    static QPointer<Proxy> _proxyRef() {
        return _shproxy;
    }
    virtual ~Proxy();

Q_SIGNALS:
    void appDisplayAlert(const QVariantMap&);
    void appDisplaySimpleDiag(quint64, const QVariantMap&);
    void appDisplayProgressDiag(quint64, const QVariantMap&);
    void appUpdateProgressDiag(quint64, const QVariantMap&);
    void appCloseProgressDiag(quint64);

public Q_SLOTS:
    void registerActivity(shared_ptr<ExecStruct>, QObjWeakPtr, QObjWeakPtr);
    void unregisterActivity(ActivityClass const*);
    ActivityClass const* getActivity(const string&, bool);
    ActivityClass const* getActivity(const QObjWeakPtr);

    void onDisplayAlert(QObjSharedPtr, const QVariantMap&);
    void onDisplaySimpleDiag(QObjSharedPtr, const QVariantMap&);
    void onDisplayProgressDiag(QObjSharedPtr, const QVariantMap&);
    void onUpdateProgressDiag(QObjSharedPtr, const QVariantMap&);
    void onCloseProgressDiag(QObjSharedPtr);

    void onDisplayInputList(QObjSharedPtr, const QVariantMap&);
    void onDisplayInputFileBrowse(QObjSharedPtr, const QString&, const QString&);
    void onDisplayInputKeyboard(QObjSharedPtr, const QString&);


private:
    template<typename T>
    T * unpackCaller(QObjSharedPtr _caller);

private:
    QMutex m_mtx;
    QObjSharedPtr m_boxSession{nullptr};
    QObjSharedPtr m_InputSession{nullptr};
    list<ActivityClass> m_activities{};
    list<ActivityClass> m_services{};
};

} // namespace KvalApplication

#endif // PROXY_H
