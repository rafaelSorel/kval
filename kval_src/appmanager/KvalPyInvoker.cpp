#include <QDebug>
#include "KvalPyInvoker.h"
#include <pybind11/embed.h>

namespace py = pybind11;
using namespace std;

QVariant convertPyobject(PyObject* obj);

QVariant convertPyInt(PyObject* pVal)
{
    long val = PyLong_AsLong(pVal);
    return QVariant(static_cast<qlonglong>(val));
}

QVariant convertPyStr(PyObject* pVal)
{
    const char* val = PyUnicode_AsUTF8(pVal);
    return QVariant(val);
}

QVariant convertPyBytes(PyObject* pVal)
{
    const char* val = PyBytes_AsString(pVal);
    return QVariant(val);
}

QVariant convertPyDict(PyObject* dict) {
    if (!PyDict_Check(dict))
        throw "Not a dict !";

    QVariantMap native_dict;
    PyObject *key;
    PyObject *value;
    Py_ssize_t pos = 0;

    while (PyDict_Next(dict, &pos, &key, &value)) {
        native_dict.insert(
                    convertPyobject(key).toString(),
                    convertPyobject(value));
    }
    return native_dict;
}

QVariant convertPyIter(PyObject* iter) {

    QVariantList _list;
    PyObject *iterator = PyObject_GetIter(iter);
    PyObject *item;

    if (!iterator) {
        throw "It is not an iterator !";
    }

    while ((item = PyIter_Next(iterator))) {
        _list.append(convertPyobject(item));
        Py_XDECREF(item);
    }

    Py_XDECREF(iterator);

    if (PyErr_Occurred()) {
        throw "PyErr_Occurred !";
    }

    return _list;
}

QVariant convertPyobject(PyObject* obj)
{
    if(!obj){
        throw "Null object !";
    }
    if (PyDict_Check(obj))
        return convertPyDict(obj);
    if (PyLong_Check(obj))
        return convertPyInt(obj);
    if (PyUnicode_Check(obj))
        return convertPyStr(obj);
    if (PyBytes_Check(obj))
        return convertPyBytes(obj);
    if (PyList_Check(obj) || PyTuple_Check(obj) || PyIter_Check(obj))
        return convertPyIter(obj);

    throw "Unable to extract py type !";
}

//PYBIND11_EMBEDDED_MODULE(cppkval, m) {
//    py::class_<KvalPyApplication>(m, "Application")
//        .def(py::init<const string &, const string &>())
//        .def("get_id", &KvalPyApplication::get_id)
//        .def("add_dep", &KvalPyApplication::addDep)
//        .def("set_launcher", &KvalPyApplication::setLauncher)
//        .def("__repr__",
//            [](const KvalPyApplication &a) {
//                return "<cppkval.Application app id '" + a.get_id() + "'>";})
//            ;
//    py::class_<KvalPyApplication::Dependency>(m, "Dependency")
//            .def(py::init<const string&,const string&,const string&>())
//            .def_readwrite("id", &KvalPyApplication::Dependency::id)
//            .def_readwrite("version", &KvalPyApplication::Dependency::version)
//            .def_readwrite("uri", &KvalPyApplication::Dependency::uri)
//            ;
//    py::class_<KvalPyApplication::Launcher>(m, "Launcher")
//            .def(py::init<const string&,const string&,const string&,const string&>())
//            .def_readwrite("main", &KvalPyApplication::Launcher::main)
//            .def_readwrite("daemon", &KvalPyApplication::Launcher::daemon)
//            .def_readwrite("update", &KvalPyApplication::Launcher::update)
//            .def_readwrite("package", &KvalPyApplication::Launcher::package)
//            ;
//}

/**
 * @brief KvalPyInvoker::KvalPyInvoker
 */
KvalPyInvoker::KvalPyInvoker()
{
    py::initialize_interpreter();
    PyEval_InitThreads();
}

/**
 * @brief KvalPyInvoker::~KvalPyInvoker
 */
KvalPyInvoker::~KvalPyInvoker()
{
    py::finalize_interpreter();
}

/**
 * @brief KvalPyInvoker::extractInstalledApps
 */
void KvalPyInvoker::extractAvailableApps()
{
    auto _newpath = py::module::import("sys").attr("path").cast<py::list>();
    _newpath.append("/home/naloui/opt/kval/kval_apps");
    py::module::import("sys").attr("path") =_newpath;
    auto m = py::module::import("cppkval");

    auto _kvalapp = py::module::import("kvalapp");
    try {
        py::object installed = _kvalapp.attr("extract_available_apps")();
        py::list apps = installed.cast<py::list>();
        for(const auto& app: apps) {
            m_availableApps.push_back(app.cast<KvalPyApplication>());
            qInfo() << "append application id: "
                    << m_availableApps[0].get_id().c_str();
        }
    } catch (...) {
        qCritical() << "Error while extraction applications !";
        return;
    }
}

string KvalPyApplication::getId() const
{
    return m_id;
}

void KvalPyApplication::setId(const string &id)
{
    m_id = id;
}


#if 1
namespace py = pybind11;

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

    PyThreadState* getState() {
        return _state;
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
        qInfo() << "Create new thread state...";
        _ts = PyThreadState_New(interp);
        PyEval_RestoreThread(_ts);
    }

    ~thread_state()
    {
        PyThreadState_Clear(_ts);
        PyThreadState_DeleteCurrent();
        qInfo() << "delete current Thread state...";
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

    sub_interpreter(PyThreadState* mainth)
    {
//        restore_tstate_scope restore;
        PyEval_RestoreThread(mainth);
        _ts = Py_NewInterpreter();
        mainth = PyEval_SaveThread();
    }

    ~sub_interpreter()
    {
        if( _ts )
        {
            qInfo() << "End interpretor...";
//            swap_tstate_scope sts(_ts);
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

    static PyInterpreterState* current()
    {
        return thread_state::current()->interp;
    }

private:

    PyThreadState* _ts;
};
#define GC_SCRIPT \
  "import gc\n" \
  "gc.collect(2)\n"

void KvalPySystemMonitor::systemAbortNotify() {
    qInfo() << ">>>>>>>>>>>> Monitor Abort";
    m_aborted = true;
    onAbordRequest();
}


void KvalPySytemSupervisor::onAbortRequest() {
    QMutexLocker locker(&m_regMtx);
    for_each(begin(m_registredMonitor),
             end(m_registredMonitor),
             [&](MonitorPtr _m) {
        if(!_m.isNull())
            _m->systemAbortNotify();
    } );
}


PYBIND11_EMBEDDED_MODULE(cppkval, m) {
    py::class_<KvalPyApplication>(m, "Application")
        .def(py::init<const string &, const string &>())
        .def("get_id", &KvalPyApplication::get_id)
        .def("add_dep", &KvalPyApplication::addDep)
        .def("set_launcher", &KvalPyApplication::setLauncher)
        .def("__repr__",
            [](const KvalPyApplication &a) {
                return "<cppkval.Application app id '" + a.get_id() + "'>";})
            ;
    py::class_<KvalPyApplication::Dependency>(m, "Dependency")
            .def(py::init<const string&,const string&,const string&>())
            .def_readwrite("id", &KvalPyApplication::Dependency::id)
            .def_readwrite("version", &KvalPyApplication::Dependency::version)
            .def_readwrite("uri", &KvalPyApplication::Dependency::uri)
            ;
    py::class_<KvalPyApplication::Launcher>(m, "Launcher")
            .def(py::init<const string&,const string&,const string&,const string&>())
            .def_readwrite("main", &KvalPyApplication::Launcher::main)
            .def_readwrite("daemon", &KvalPyApplication::Launcher::daemon)
            .def_readwrite("update", &KvalPyApplication::Launcher::update)
            .def_readwrite("package", &KvalPyApplication::Launcher::package)
            ;
    py::class_<KvalPySystemMonitor, KvalPyMonitorWrapper>(m, "Monitor")
            .def(py::init<>())
            .def("aborted", &KvalPySystemMonitor::aborted)
            .def("onAbordRequest", &KvalPySystemMonitor::onAbordRequest)
            ;
}

class PyInvokerTest: public QThread
{
public:
    PyInvokerTest(const string& tname, PyThreadState* mainth):
        tname(tname),
        mainth(mainth)
    {
        qInfo() << "Construct thread: " << tname.c_str();
    }
    ~PyInvokerTest()
    {
        qInfo() << "Destructed thread...";
    }

    void run()
    {
        vector<KvalPyApplication> m_availableApps;
        qInfo() << "Create interpreter " << tname.c_str();
        sub_interpreter s1(mainth);

        qInfo() << "Starting thread scope " << tname.c_str();
        //sub_interpreter::thread_scope scope(s1.interp());

        PyEval_RestoreThread(s1.ts());
        auto _newpath = py::module::import("sys").attr("path").cast<py::list>();
        _newpath.append("/home/naloui/opt/kval/kval_apps");
        py::module::import("sys").attr("path") =_newpath;

        qInfo() << "Run tname" << tname.c_str();
        try {
            qCritical() << "before number threads: " << s1.interp()->num_threads;
            py::eval_file(tname, py::globals());
            PyRun_SimpleString(GC_SCRIPT);
            qCritical() << "after number threads: " << s1.interp()->num_threads;
        } catch (...) {
            qCritical() << "Error while extraction applications: " << tname.c_str();
        }

        // Main script has finished, sadely some python scripter may forget
        // To stop all the threads used in their scripts, so make the job for them
        // Note that this not a happy ending , if those threads needs to finalize
        // Correctly, this may be problematic for them..
        for (PyThreadState* old = nullptr;
             s1.interp()->num_threads && s1.ts() != nullptr;)
        {
            PyThreadState* s = NULL;
            for (s = s1.interp()->tstate_head; s && s == s1.ts();) {
                s = s->next;
            }

            if (!s){
                qInfo() << ">>>>>>>>>>>>>> NULL ...";
                break;
            }

            if (old != s)
            {
                qInfo() << "Waiting on thread: " << tname.c_str();
                old = s;
            }
            // TODO : before raising exception,
            //        send abortRequest to the thread.
            auto reply = PyThreadState_SetAsyncExc(s->thread_id,
                                                   PyExc_SystemExit);
            qInfo() << "async except: " << reply;
            qInfo() << "Py_BEGIN_ALLOW_THREADS: " << tname.c_str();
            Py_BEGIN_ALLOW_THREADS
            QThread::msleep(100);
            Py_END_ALLOW_THREADS
            PyThreadState_Delete(s);
        }

        PyEval_ReleaseLock();
        qCritical() << "number threads: " << s1.interp()->num_threads;
    }
private:
    string tname{};
    PyThreadState* mainth = NULL;
};

int main()
{
    qInstallMessageHandler(loggerOutput);
    py::initialize_interpreter();
    PyEval_InitThreads();


    qInfo() << "Main thread scope...";
    py::module::import("cppkval");
    PyThreadState* _mainthst = PyEval_SaveThread();

    PyInvokerTest t1{"/home/naloui/opt/kval/kval_apps/kvalapp.py", _mainthst};
    PyInvokerTest t2{"/home/naloui/opt/kval/kval_apps/kvalstore/daemon.py", _mainthst};
    PyInvokerTest t3{"/home/naloui/opt/kval/kval_apps/kvalstore/daemon.py", _mainthst};
    PyInvokerTest t4{"/home/naloui/opt/kval/kval_apps/kvalapp.py", _mainthst};
    PyInvokerTest t5{"/home/naloui/opt/kval/kval_apps/kvalstore/daemon.py", _mainthst};

    t1.start();
    t2.start();
    t3.start();
    t4.start();
    t5.start();

    QThread::sleep(5);

    // Should we take the GIL in here ?
    // It is a wise decision I think, since we block other threads from
    // Deleting some instance related by the forward request...
    PyEval_RestoreThread(_mainthst);
    KvalPySytemSupervisor::Instance()->onAbortRequest();
    _mainthst = PyEval_SaveThread();

    t1.wait();
    t2.wait();
    t3.wait();
    t4.wait();
    t5.wait();
    qInfo() << "thread finalized...";


    PyEval_RestoreThread(_mainthst);
    qInfo() << "finalize interpreter...";
    py::finalize_interpreter();
    qInfo() << "After finalize interpreter...";
    PyEval_ReleaseLock();

    qInfo() << "OUT...";
    return 0;
}
#endif
