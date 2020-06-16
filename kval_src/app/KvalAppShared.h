#ifndef KVALAPPSHARED_H
#define KVALAPPSHARED_H

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>


#include <QObject>
#include <QDebug>
#include <QPointer>

using namespace std;

namespace KvalApplication {

using QObjWeakPtr = QPointer<QObject>;
using QObjSharedPtr = shared_ptr<QObject>;
/**
 * @brief The Permission enum
 */
enum Permission {
    Invalid,
    System,
    Generic,
    Count
};

/**
 * @brief The ExecStruct struct
 */
struct __attribute__ ((visibility("hidden"))) ExecStruct {
    string appid;
    string execpath;
    string module;
    string attr;
    Permission permission;
    vector<string> depPaths{};
    vector<string> args{};
    bool service{false};
    bool syncall{false};
    QObject* _caller;
    function<void(QObject*, const pybind11::object&)> _cb;
};


/**
 * @brief The ActivityClass struct
 */
struct ActivityClass {
    bool operator==(const ActivityClass& rhs) {
        return (uid == rhs.uid && handle == rhs.handle);
    }
    string uid;
    QObjWeakPtr wid;
    QObjWeakPtr handle;
    Permission perm;
    bool enabled;
    bool running;
};


} // namespace KvalApplication

#endif // KVALAPPSHARED_H
