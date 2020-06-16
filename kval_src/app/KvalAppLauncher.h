#ifndef LAUNCHERENGINE_H
#define LAUNCHERENGINE_H

#include <memory>

#include <QObject>
#include <QDebug>
#include <QPointer>

#include "KvalThreadUtils.h"
#include "KvalAppProxy.h"
#include "KvalAppShared.h"

using namespace std;

namespace KvalApplication {

class PySubInterpreter;
class ActivityWorker;

/**
 * @brief The LauncherEngine class
 */
class LauncherEngine : public QObject
{
    Q_OBJECT
public:
    explicit LauncherEngine(shared_ptr<Proxy> proxy,
                            QObject *parent = nullptr);
    virtual ~LauncherEngine();

Q_SIGNALS:

public Q_SLOTS:
    void Start();
    void onStartApp(shared_ptr<ExecStruct> _exec);
    void onStopApp(const string& appid);
    void onExecReport(int, const string&, QObjWeakPtr);

private:
    bool cleanupActivity(ActivityClass const*);

private:
    shared_ptr<Proxy> m_shdProxy;
    list<tuple<QPointer<KvalThread>, QPointer<ActivityWorker>, string>> m_workers;
    PyThreadState* m_mainThSt{nullptr};
};


/**
 * @brief The ActivityWorker class
 */
class ActivityWorker : public QObject
{
    Q_OBJECT
public:
    explicit ActivityWorker(QObject *parent = nullptr);
    virtual ~ActivityWorker();
    enum class State { Invalid = 0, Running, Ended, Count };
    enum class EndStatus { Invalid = 0, Success, Abort, Fail, Count };
    Q_ENUM(EndStatus)

Q_SIGNALS:
    void execReport(int, const string&, QObjWeakPtr);

public Q_SLOTS:
    void initialize(PyThreadState* _mth);
    void exec(shared_ptr<ExecStruct> _exec);
    void abort();

private:
    void cleanup();

private:
    PySubInterpreter* m_subInterp{nullptr};
    State m_state{State::Invalid};
};

/**
 * @brief The PySubInterpreter class
 *        It represents python subinterpretor.
 */
class PySubInterpreter
{

public:

    struct ThStateScope
    {
        ThStateScope(PyThreadState* _ts) {
            m_ts = _ts;
            PyEval_RestoreThread(m_ts);
        }

        ~ThStateScope() {
            m_ts = PyEval_SaveThread();
        }
        PyThreadState* m_ts;
    };

    // perform the necessary setup and cleanup
    // for a new thread running using a specific
    // interpreter
    explicit PySubInterpreter(PyThreadState* mainth)
    {
        PyEval_RestoreThread(mainth);
        _ts = Py_NewInterpreter();
        mainth = PyEval_SaveThread();
    }
    PySubInterpreter(const PySubInterpreter&) = delete;
    PySubInterpreter& operator=(const PySubInterpreter&) = delete;

    ~PySubInterpreter()
    {
        if( _ts )
        {
            qInfo() << "End interpretor...";
            PyEval_RestoreThread(_ts);
            Py_EndInterpreter(_ts);
            PyEval_ReleaseLock();
            qInfo() << "OUT End interpretor...";
        }
    }

    PyInterpreterState* interp()
    {
        return _ts->interp;
    }

    PyThreadState* ts()
    {
        return _ts;
    }

private:
    PyThreadState* _ts;
};

} // namespace KvalApplication

#endif // LAUNCHERENGINE_H
