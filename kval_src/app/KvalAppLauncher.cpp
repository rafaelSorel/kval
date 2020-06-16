#include <QDebug>
#include "KvalAppLauncher.h"
#include "KvalAppEmbedModules.h"

namespace py = pybind11;

namespace KvalApplication {

//-----------------------------------------------------------------------------
// Constants and macros
//-----------------------------------------------------------------------------
#define APPLAUNCHER_REGISTER_ACTIVITY(_ex, _w, _wth) \
    QMetaObject::invokeMethod(m_shdProxy.get(), \
                  "registerActivity", \
                  Qt::BlockingQueuedConnection, \
                  Q_ARG(shared_ptr<ExecStruct>, _ex), \
                  Q_ARG(QObjWeakPtr, QObjWeakPtr(dynamic_cast<QObject*>(_w))), \
                  Q_ARG(QObjWeakPtr, QObjWeakPtr(dynamic_cast<QObject*>(_wth))));

#define APPLAUNCHER_ENABLE_ACTIVITY(_w, _hdl) \
    QMetaObject::invokeMethod(m_shdProxy.get(), \
                  "registerActivity", \
                  Qt::BlockingQueuedConnection, \
                  Q_ARG(shared_ptr<ExecStruct>, _ex), \
                  Q_ARG(QObjWeakPtr, QObjWeakPtr(dynamic_cast<QObject*>(_w))));

#define APPLAUNCHER_UNREGISTER_ACTIVITY(_ac) \
    QMetaObject::invokeMethod(m_shdProxy.get(), \
                              "unregisterActivity", \
                              Qt::BlockingQueuedConnection, \
                              Q_ARG(ActivityClass const*, _ac));

#define APP_LAUNCHER_GET_ACTIVITY(_id, _srv, _ac) \
    QMetaObject::invokeMethod(m_shdProxy.get(), \
                  "getActivity", \
                  Qt::BlockingQueuedConnection, \
                  Q_RETURN_ARG(ActivityClass const*, _ac), \
                  Q_ARG(string, _id), \
                  Q_ARG(bool, _srv));

#define APP_LAUNCHER_GET_ACTIVITY_HDL(_hdl, _ac) \
    QMetaObject::invokeMethod(m_shdProxy.get(), \
                  "getActivity", \
                  Qt::BlockingQueuedConnection, \
                  Q_RETURN_ARG(ActivityClass const*, _ac), \
                  Q_ARG(QObjWeakPtr, _hdl));

//----------------------------------------------------------------------------
// Classes interface
//----------------------------------------------------------------------------

/**
 * @brief LauncherEngine::LauncherEngine
 * @param proxy
 * @param parent
 */
LauncherEngine::LauncherEngine(std::shared_ptr<Proxy> proxy,
                               QObject *parent) :
    QObject(parent),
    m_shdProxy(proxy)
{
    qRegisterMetaType<PyThreadState*>("PyThreadState*");
    qRegisterMetaType<shared_ptr<ExecStruct>>("shared_ptr<ExecStruct>");


    py::initialize_interpreter();
    PyEval_InitThreads();

    m_mainThSt = PyEval_SaveThread();
}

/**
 * @brief LauncherEngine::~LauncherEngine
 */
LauncherEngine::~LauncherEngine()
{
    qInfo() << "finalize LauncherEngine";
    for(const auto& _worker: m_workers) {
        get<1>(_worker)->abort();
        get<0>(_worker)->stop();
        delete get<0>(_worker);
        delete get<1>(_worker);
    }
    m_workers.clear();
    PyEval_RestoreThread(m_mainThSt);
    py::finalize_interpreter();
    PyEval_ReleaseLock(); // Deprecated, replace it by PyEval_ReleaseThread ?
    qInfo() << "After finalize interpreter...";
}

/**
 * @brief LauncherEngine::Start
 */
void LauncherEngine::Start()
{
}

/**
 * @brief LauncherEngine::syncExec
 * @param _exec
 */
void LauncherEngine::onStartApp(shared_ptr<ExecStruct> _exec)
{
    qInfo() << "sync exec: "
            << _exec->appid.c_str();

    ActivityClass const* activity{nullptr};
    APP_LAUNCHER_GET_ACTIVITY(_exec->appid, _exec->service, activity);

    if(!activity) {
        qInfo() << "Create new activity worker...";
        ActivityWorker* _worker = new ActivityWorker();
        KvalThread* _workerth = new KvalThread(_worker);
        connect(_worker, &ActivityWorker::execReport,
                this, &LauncherEngine::onExecReport,
                Qt::QueuedConnection);

        _workerth->start();

        tuple<QPointer<KvalThread>, QPointer<ActivityWorker>, string>
                _curWorker{_workerth, _worker, _exec->appid};
        m_workers.push_back(_curWorker);

        qInfo("_workerth: %p", _workerth);
        APPLAUNCHER_REGISTER_ACTIVITY(_exec, _worker, _workerth)

        QMetaObject::invokeMethod(_worker, "initialize",
                      Qt::QueuedConnection,
                      Q_ARG(PyThreadState*, m_mainThSt));

        qInfo() << "Start Activity: " << _worker;
        QMetaObject::invokeMethod(_worker, "exec",
                      Qt::QueuedConnection,
                      Q_ARG(shared_ptr<ExecStruct>, _exec));

    }
    else {
        qInfo() << "Found activity for: " << activity->uid.c_str();
        ActivityWorker* _worker =
                dynamic_cast<ActivityWorker*>(activity->wid.data());
        qInfo() << "Restart Activity: " << _worker;
        QMetaObject::invokeMethod( _worker, "exec",
                    Qt::QueuedConnection,
                    Q_ARG(shared_ptr<ExecStruct>, _exec));
    }
}

/**
 * @brief LauncherEngine::onStopApp
 * @param appid
 */
void LauncherEngine::onStopApp(const string &appid)
{
    qInfo() << "Stop activity...";

    ActivityClass const* activity{nullptr};
    APP_LAUNCHER_GET_ACTIVITY(appid, true, activity)

    if(!cleanupActivity(activity)) {
        qInfo() << "No registred activity found with name: " << appid.c_str();
    }
}

/**
 * @brief LauncherEngine::onExecReport
 * @param _st
 * @param appid
 * @param _hdl
 */
void LauncherEngine::onExecReport(
        int _st,
        const string& appid,
        QObjWeakPtr _hdl)
{
    ActivityClass const* activity{nullptr};
    APP_LAUNCHER_GET_ACTIVITY_HDL(_hdl, activity)

    switch(static_cast<ActivityWorker::EndStatus>(_st))
    {
        case ActivityWorker::EndStatus::Success:
            // Maintain the thread for future uses...
            // TODO: Inform the UI via proxy channel.
            break;
        case ActivityWorker::EndStatus::Abort:
            // This is caused by an abort call by the launcher itself
            // May be unregister from proxy guy...
            break;
        case ActivityWorker::EndStatus::Fail:
            // TODO: Inform the UI via the proxy channel

            // Failing has been observed, May be unregister and
            // shutdown the thread.
            if(!cleanupActivity(activity)) {
                qInfo() << "No registred activity found with name: "
                        << appid.c_str();
            }
            break;
        case ActivityWorker::EndStatus::Invalid:
        case ActivityWorker::EndStatus::Count:
        default:
            break;
    }
}

/**
 * @brief LauncherEngine::cleanupActivity
 * @param activity
 * @return
 */
bool LauncherEngine::cleanupActivity(ActivityClass const* activity)
{
    if (!activity || activity->wid.isNull()) {
        return false;
    }

    qInfo() << "Extract worker id ...";
    ActivityWorker* _wid = dynamic_cast<ActivityWorker*>(activity->wid.data());

    qInfo() << "unregister Activity ...";
    APPLAUNCHER_UNREGISTER_ACTIVITY(activity)

    if(!_wid) {
        qCritical() << "worker null !!";
        return false;
    }

    for(const auto& worker: m_workers) {
        if(get<1>(worker) == _wid) {
            get<1>(worker)->abort();
            get<0>(worker)->stop();
            delete get<0>(worker);
            delete get<1>(worker);
            m_workers.remove(worker);
            break;
        }
    }
    return true;
}

/**
 * @brief ActivityWorker::ActivityWorker
 * @param parent
 */
ActivityWorker::ActivityWorker(QObject *parent):
    QObject(parent)
{

}

/**
 * @brief ActivityWorker::~ActivityWorker
 */
ActivityWorker::~ActivityWorker()
{
    if(m_subInterp)
        delete m_subInterp;
}

/**
 * @brief ActivityWorker::initialize
 * @param _mth
 */
void ActivityWorker::initialize(PyThreadState *_mth)
{
    m_subInterp = new PySubInterpreter(_mth);
}

/**
 * @brief ActivityWorker::Abort
 */
void ActivityWorker::abort()
{
    if(m_state != State::Running) {
        qInfo() << "Abort Non running application...";
        return;
    }

    qInfo() << "Abort...";
    PySubInterpreter::ThStateScope guard(m_subInterp->ts());

    for (auto state = m_subInterp->interp()->tstate_head;
         state != NULL;
         state = state->next)
    {
      // Force python threads stopping
      auto reply = PyThreadState_SetAsyncExc(state->thread_id, PyExc_SystemExit);
      qInfo() << "async except reply: " << reply;
    }

    qInfo() << "Aborted...";
}

/**
 * @brief ActivityWorker::exec
 * @param _exec
 */
void ActivityWorker::exec(shared_ptr<ExecStruct> _exec)
{
    bool _excepted{false};
    bool _sysExit{false};
    {
        PySubInterpreter::ThStateScope guard(m_subInterp->ts());
        m_state = State::Running;

        // Add all deps paths...
        auto _syspath = py::module::import("sys").attr("path").cast<py::list>();
        for(const auto& dep: _exec->depPaths){
            _syspath.append(dep);
        }
        py::module::import("sys").attr("path") =_syspath;

        // Import builtin internals
        py::module::import("intkval");
        py::module::import("kvalui");

        // Exec ...
        try {
            if(_exec->syncall) {
                auto _modapp = py::module::import(_exec->module.c_str());
                py::object ret = _modapp.attr(_exec->attr.c_str())();
                _exec->_cb(_exec->_caller, ret);
            }
            else {
                qInfo() << "Execute application...";
                py::eval_file(_exec->execpath.c_str(), py::globals());
            }
        }
        catch(py::error_already_set& ex){
            qCritical() << "Raised error: " << ex.what();
            _sysExit = ex.matches(PyExc_SystemExit);
            _excepted = true;
        }
        catch (...) {
            qCritical() << "Raised error while exec application !";
            _excepted = true;
        }
    }
    m_state = State::Ended;
    auto _estatus{EndStatus::Invalid};
    if (!_excepted)
    {
        qInfo() << "Application successfully exit, cleanup dangling threads";
        this->cleanup();
        _estatus = EndStatus::Success;
    }
    else if(_sysExit)
    {
        qInfo() << "PyExc_SystemExit thrown";
        _estatus = EndStatus::Abort;
    }
    else {
        qInfo() << "Application simply failed !!";
        this->cleanup();
        _estatus = EndStatus::Fail;
    }

    Q_EMIT execReport(static_cast<int>(_estatus),
                      _exec->appid,
                      QObjWeakPtr(dynamic_cast<QObject*>(
                                    QThread::currentThread())));
    qInfo() << "Out";
}

/**
 * @brief ActivityWorker::cleanup
 */
void ActivityWorker::cleanup()
{
    qInfo() << "Stopped";
    PySubInterpreter::ThStateScope guard(m_subInterp->ts());

    for (PyThreadState* old = nullptr;
         m_subInterp->interp()->num_threads && m_subInterp->ts() != nullptr;)
    {
        PyThreadState* s = NULL;
        for (s = m_subInterp->interp()->tstate_head; s && s == m_subInterp->ts();) {
            s = s->next;
        }

        if (!s){
            break;
        }

        if (old != s)
        {
            qInfo() << "Waiting on thread...";
            old = s;
        }

        // TODO : send abortRequest to the thread.
        qInfo() << "Py_BEGIN_ALLOW_THREADS...";
        Py_BEGIN_ALLOW_THREADS
        QThread::msleep(100);
        Py_END_ALLOW_THREADS
        PyThreadState_Clear(s);
        PyThreadState_Delete(s);
    }
    qInfo() << "number threads: " << m_subInterp->interp()->num_threads;

}

} // namespace KvalApplication
