#ifndef KVALOLDPYWRAPPER_H
#define KVALOLDPYWRAPPER_H

#include <pybind11/embed.h>

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QMutex>
#include <QDebug>

class KvalPySystemMonitor;


class KvalPySytemSupervisor: public QObject {

    Q_OBJECT
    Q_DISABLE_COPY(KvalPySytemSupervisor)
    using MonitorPtr = QPointer<KvalPySystemMonitor>;

public:

    static KvalPySytemSupervisor* Instance() {
        static KvalPySytemSupervisor _supervisor;
        return &_supervisor;
    }

    bool registerMonitor(MonitorPtr monitor) {
        QMutexLocker locker(&m_regMtx);
        m_registredMonitor.insert(monitor);
        return true;
    }

    bool unRegisterMonitor(MonitorPtr monitor) {
        QMutexLocker locker(&m_regMtx);
        m_registredMonitor.remove(monitor);
        return true;
    }

public Q_SLOTS:
    void onAbortRequest();

private:
    KvalPySytemSupervisor() = default;
    ~KvalPySytemSupervisor() = default;

private:
    QMutex m_regMtx;
    QSet<MonitorPtr> m_registredMonitor;
};

/**
 * @brief The KvalPySystemMonitor class
 */
class KvalPySystemMonitor: public QObject {

    Q_OBJECT

public:
    KvalPySystemMonitor() {
        qInfo() << ">>>>>>> Construct KvalPySystemMonitor";
        KvalPySytemSupervisor::Instance()->registerMonitor(
                    QPointer<KvalPySystemMonitor>(this));
    }
    virtual ~KvalPySystemMonitor(){
        qInfo() << ">>>>>>> Destruct KvalPySystemMonitor";
        KvalPySytemSupervisor::Instance()->unRegisterMonitor(
                    QPointer<KvalPySystemMonitor>(this));
    }

    bool aborted(){
        qInfo() << ">>>>>>>>>>>> aborted: " << m_aborted;
        return m_aborted;
    }

    void systemAbortNotify();
    virtual void onAbordRequest() {}

private:
    bool m_aborted{false};
};

class KvalPyMonitorWrapper : public KvalPySystemMonitor {
public:
    using KvalPySystemMonitor::KvalPySystemMonitor; /* Inherit constructors */

    void onAbordRequest() override {
        /* Generate wrapping code that enables native function overloading */
        PYBIND11_OVERLOAD(
            void,         /* Return type */
            KvalPySystemMonitor, /* Parent class */
            onAbordRequest, /* Name of function */
        );
    }

};
#if 0
/**
 * @brief CPyInstance::extractDefaultPath
 */
void CPyInstance::extractDefaultPath()
{
    qInfo() << "Set Default path...";
    PyObject* sysMod(PyImport_ImportModule("sys"));
    PyObject* sysModDict(PyModule_GetDict(sysMod)); // borrowed ref
    PyObject* pathObj(PyDict_GetItemString(sysModDict, "path")); // borrowed ref

    if (pathObj != NULL && PyList_Check(pathObj))
    {
        for (int i = 0; i < PyList_Size(pathObj); i++)
        {
            PyObject* e = PyList_GetItem(pathObj, i); // borrowed ref
            if (e != NULL && PyUnicode_Check(e))
                addPath(PyUnicode_AsUTF8(e)); // returns internal data, don't delete or modify
        }
    }
    else
    {
        std::wstring wideString = Py_GetPath();
        QString _pypath = QString::fromWCharArray( wideString.c_str() );
        addPath(_pypath);
    }

    Py_XDECREF(sysMod);
    qInfo() << "Default path: " << m_pypath;
}

/**
 * @brief CPyInstance::setPath
 */
void CPyInstance::setPath()
{
    qInfo() << "Modified path: " << m_pypath;
    std::wstring pypath = m_pypath.toStdWString();
    PySys_SetPath(pypath.c_str());
}
#endif
#endif // KVALOLDPYWRAPPER_H
