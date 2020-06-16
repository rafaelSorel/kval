#ifndef KVALPYINVOKER_H
#define KVALPYINVOKER_H

#include "Python.h"
#include <QObject>
#include <string>
#include <vector>
#include <QString>

#include "KvalThreadUtils.h"

// KvalAppManagerElement -> UI main thread
// KvalAppManager -> Threaded
//            Handle the load of available applications
//            Install/Uninstall applications
//            Update applicaion,

// KvalPySystemSupervisor: Handle all applications
// that have monitoring thread system, call python callback functions

using namespace std;

class KvalPyMonitor {

public:
    KvalPyMonitor(const string& id) : id(id) {
        QString _id(id.c_str());
        qInfo() << ">>>>>>>>>>>>>> Construct";
    }
    virtual ~KvalPyMonitor(){
        qInfo() << "<<<<<<<<<<<<<< Destruct";
    };

private:
    string id{};
};

/**
 * @brief The KvalPyApplication class
 */
class KvalPyApplication {

public:
    KvalPyApplication(const string& id, const string& version) :
        m_id(id),
        m_version(version)
    {
        qInfo() << ">>>>>>>>>>>>>> Construct";
    }
    KvalPyApplication(const KvalPyApplication& rhs) = default;
    KvalPyApplication(KvalPyApplication&& rhs)
    {
        qInfo() << ">>>>>>>>>>>>>> move constructor";
        m_id = move(rhs.m_id);
        m_version = move(rhs.m_version);
        m_launcher = move(rhs.m_launcher);
        m_dependencies = move(rhs.m_dependencies);
    }
    KvalPyApplication& operator=(const KvalPyApplication& rhs) = delete;

    virtual ~KvalPyApplication() {
        qInfo() << "<<<<<<<<<<<<<< Destruct";
    };
    struct Dependency {
        Dependency() = default;
        Dependency(const string& id,
                   const string& version,
                   const string& uri):
            id(id),
            version(version),
            uri(uri) {}

        string id{};
        string version{};
        string uri{};
    };
    struct Launcher {
        Launcher() = default;
        Launcher(const string& main,
                 const string& daemon,
                 const string& update,
                 const string& pack):
            main(main),
            daemon(daemon),
            update(update),
            package(pack) {}

        string main{};
        string daemon{};
        string update{};
        string package{};
    };
//    struct Meta{
//        struct Desc{
//            string lang;
//            string payload;
//        };
//        string icon{};
//        string back{};
//        string fanart{};
//        vector<Desc> summaries;
//        vector<Desc> descriptions;
//        vector<Desc> disclaimers;
//        string platform;
//        qint64 size;
//    };

    string get_id() const {
        qInfo() << "get me the id: " << m_id.c_str();
        return m_id;
    }

    void addDep(const Dependency& dep)
    {
        m_dependencies.push_back(move(dep));
    }
//    void setMeta(Meta&& meta)
//    {
//        m_meta = move(meta);
//    }
    void setLauncher(const Launcher& launcher)
    {
        m_launcher = move(launcher);
    }

private:
    string m_id{};
    string m_version{};
    Launcher m_launcher{};
//    Meta m_meta{};
    vector<Dependency> m_dependencies{};
};

class KvalPyAppWorker
{
public:
    KvalPyAppWorker(const QString& appid): m_appid(appid) {

    };

private:
    QString m_appid{};
};

/**
 * @brief The KvalPyInvoker class
 *        Creates the main Global python interpretor,
 *
 *        it runs applications using the subinterpretors spawn on their
 *        own threads
 *
 */
class KvalPyInvoker: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(KvalPyInvoker)

public:
    KvalPyInvoker();
    virtual ~KvalPyInvoker();
    void extractAvailableApps();

private:
    vector<KvalPyApplication> m_availableApps;
    vector<KvalPyAppWorker*> m_pyAppWorkers;
    vector<KvalThread*> m_pyWorkersThs;
};

// initialize and clean up python
struct initialize
{
    initialize()
    {
        Py_InitializeEx(1);
        PyEval_InitThreads();
    }

    ~initialize()
    {
        Py_Finalize();
    }
};

// allow other threads to run
class enable_threads_scope
{
public:
    enable_threads_scope()
    {
        _state = PyEval_SaveThread();
    }

    ~enable_threads_scope()
    {
        PyEval_RestoreThread(_state);
    }

private:

    PyThreadState* _state;
};

// restore the thread state when the object goes out of scope
class restore_tstate_scope
{
public:

    restore_tstate_scope()
    {
        _ts = PyThreadState_Get();
    }

    ~restore_tstate_scope()
    {
        PyThreadState_Swap(_ts);
    }

private:

    PyThreadState* _ts;
};

// swap the current thread state with ts, restore when the object goes out of scope
class swap_tstate_scope
{
public:

    swap_tstate_scope(PyThreadState* ts)
    {
        _ts = PyThreadState_Swap(ts);
    }

    ~swap_tstate_scope()
    {
        PyThreadState_Swap(_ts);
    }

private:

    PyThreadState* _ts;
};

// create new thread state for interpreter interp, make it current, and clean up on destruction
class thread_state
{
public:

    thread_state(PyInterpreterState* interp)
    {
        _ts = PyThreadState_New(interp);
        PyEval_RestoreThread(_ts);
    }

    ~thread_state()
    {
        PyThreadState_Clear(_ts);
        PyThreadState_DeleteCurrent();
    }

    operator PyThreadState*()
    {
        return _ts;
    }

    static PyThreadState* current()
    {
        return PyThreadState_Get();
    }

private:

    PyThreadState* _ts;
};

// represent a sub interpreter
class sub_interpreter
{
public:

    // perform the necessary setup and cleanup for a new thread running using a specific interpreter
    struct thread_scope
    {
        thread_state _state;
        swap_tstate_scope _swap{ _state };

        thread_scope(PyInterpreterState* interp) :
            _state(interp)
        {
        }
    };

    sub_interpreter()
    {
        restore_tstate_scope restore;

        _ts = Py_NewInterpreter();
    }

    ~sub_interpreter()
    {
        if( _ts )
        {
            swap_tstate_scope sts(_ts);

            Py_EndInterpreter(_ts);
        }
    }

    PyInterpreterState* interp()
    {
        return _ts->interp;
    }

    static PyInterpreterState* current()
    {
        return thread_state::current()->interp;
    }

private:

    PyThreadState* _ts;
};


#endif // KVALPYINVOKER_H
