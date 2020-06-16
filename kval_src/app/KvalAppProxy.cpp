#include <QMetaEnum>
#include "KvalAppProxy.h"


namespace KvalApplication {

QPointer<Proxy> _shproxy{nullptr};
/**
 * @brief Proxy::Proxy
 * @param parent
 */
Proxy::Proxy(QObject *parent) : QObject(parent)
{
    qInfo() << "Instantiate";
    //Create weak ref to share with builtin classes
    _shproxy = QPointer<Proxy>(this);

    // Register signal/slot meta types.
    qRegisterMetaType<Qt::HANDLE>("Qt::HANDLE");
    qRegisterMetaType<QObjWeakPtr>("QObjWeakPtr");
    qRegisterMetaType<QObjSharedPtr>("QObjSharedPtr");
    qRegisterMetaType<string>("string");
    qRegisterMetaType<ActivityClass const*>("ActivityClass const*");
}

Proxy::~Proxy()
{
    qInfo() << "Destruct";
    m_services.clear();
    m_activities.clear();
}
/**
 * @brief Proxy::registerActivity
 * @return
 */
void Proxy::registerActivity(
        shared_ptr<ExecStruct> _exec,
        QObjWeakPtr _wid,
        QObjWeakPtr handle)
{
    qInfo() << "In";
    if(getActivity(_exec->appid, _exec->service)) {
        qInfo() << "Already registred activity: " << _exec->appid.c_str();
    }
    else {
        QMutexLocker _locker(&m_mtx);
        qInfo("Register activity: (appid=%s, handle=%p)",
              _exec->appid.c_str(),
              handle.data());

        ActivityClass activity;
        activity.uid = _exec->appid;
        activity.wid = _wid;
        activity.handle = handle;
        activity.perm = _exec->permission;
        activity.enabled = false;
        activity.running = false;

        if(_exec->service){
            m_services.push_back(std::move(activity));
        }
        else {
            m_activities.push_back(std::move(activity));
        }
    }
}

/**
 * @brief Proxy::unregisterActivity
 * @param activity
 */
void Proxy::unregisterActivity(ActivityClass const* activity)
{
    qInfo() << "In";
    if(!activity) {
        qWarning() << "activity nullptr !!";
        return;
    }

    for(const ActivityClass& _act: m_activities) {
        if(_act.handle == activity->handle) {
            // TODO: Cleanup any callback regitration
            qInfo() << "Found activity";
            m_activities.remove(_act);
            return;
        }
    }

    for(const ActivityClass& _srv: m_services) {
        if(_srv.handle == activity->handle) {
            // TODO: Cleanup any callback regitration
            qInfo() << "Found Service";
            m_services.remove(_srv);
            return;
        }
    }
}
/**
 * @brief Proxy::checkActivity
 * @param appid
 * @return
 */
ActivityClass const* Proxy::getActivity(const string &appid, bool isService)
{
    list<ActivityClass>* _local{nullptr};
    if(isService) {
        _local = &m_services;
    }
    else {
        _local = &m_activities;
    }

    for(const ActivityClass& _act: *_local) {
        if(_act.uid == appid) {
            return &_act;
        }
    }

    return nullptr;
}

/**
 * @brief Proxy::getActivity
 * @param _handle
 * @return
 */
ActivityClass const* Proxy::getActivity(QObjWeakPtr _handle)
{
    for(const ActivityClass& _act: m_activities) {
        if(_act.handle == _handle) {
            return &_act;
        }
    }

    for(const ActivityClass& _srv: m_services) {
        if(_srv.handle == _handle) {
            return &_srv;
        }
    }

    return nullptr;
}


/**
 * @brief Proxy::onDisplayAlert
 */
void Proxy::onDisplayAlert(QObjSharedPtr _caller,
                           const QVariantMap& _alertMap)
{
    qInfo() << "In";

    auto _callerObj = unpackCaller<UserInterface::Alert>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _callerObj->thread();
    qInfo("caller=%p, handle=%p", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        return;
    }
    else {
        qInfo() << "found activity: " << _activity->uid.c_str();
        qInfo() << "_alertMap:" << _alertMap;

        Q_EMIT appDisplayAlert(_alertMap);
    }
}

/**
 * @brief Proxy::onDisplaySimpleDiag
 * @param _caller
 * @param _hdl
 */
void Proxy::onDisplaySimpleDiag(QObjSharedPtr _caller,
                                const QVariantMap& _dialog)
{
    qInfo() << "In";

    auto _callerObj = unpackCaller<UserInterface::SimpleDialog>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _caller.get()->thread();
    qInfo("onDisplaySimpleDiag: (%p, %p)", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        _callerObj->abort();
        return;
    }
    else {
        qInfo() << "Found activity: " << _activity->uid.c_str();
        qInfo() << "onDisplaySimpleDiag: " << _dialog;
        Q_EMIT appDisplaySimpleDiag(reinterpret_cast<quint64>(_hdl), _dialog);

        // TODO: Handle the interaction between the UI and the box owner
        QThread::sleep(3);
        if(_callerObj) {
            qInfo() << "Reply...";
            _callerObj->onReply(QVariant(0));
        }
    }
}

/**
 * @brief Proxy::onDisplayProgressDiag
 * @param _caller
 * @param _dialog
 */
void Proxy::onDisplayProgressDiag(QObjSharedPtr _caller,
                                  const QVariantMap& _dialog)
{
    qInfo() << "In";

    auto _callerObj = unpackCaller<UserInterface::ProgressDialog>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _callerObj->thread();
    qInfo("caller=%p, handle=%p", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        _callerObj->abort();
        return;
    }
    else {
        qInfo() << "Found activity: " << _activity->uid.c_str();
        qInfo() << "progress dialog: " << _dialog;
        Q_EMIT appDisplayProgressDiag(reinterpret_cast<quint64>(_hdl), _dialog);
    }
}


/**
 * @brief Proxy::onUpdateProgressDiag
 * @param _caller
 * @param _dialog
 */
void Proxy::onUpdateProgressDiag(QObjSharedPtr _caller,
                                 const QVariantMap& _dialog)
{
    auto _callerObj = unpackCaller<UserInterface::ProgressDialog>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _callerObj->thread();
    qInfo("caller=%p, handle=%p", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        _callerObj->abort();
        return;
    }
    else {
        qInfo() << "Found activity: " << _activity->uid.c_str();
        if(_callerObj->getTitle() == "Abort_Test" && _dialog["progress"] == 20)
        {
            qInfo() << "Abort Test !!!!";
            _callerObj->abort();
            return;
        }

        qInfo() << "progress dialog update: " << _dialog;
        qInfo() << "hadl: " << reinterpret_cast<quint64>(_hdl);
        Q_EMIT appUpdateProgressDiag(reinterpret_cast<quint64>(_hdl), _dialog);
    }
}

/**
 * @brief Proxy::onCloseProgressDiag
 * @param _caller
 */
void Proxy::onCloseProgressDiag(QObjSharedPtr _caller)
{
    auto _callerObj = unpackCaller<UserInterface::ProgressDialog>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _callerObj->thread();
    qInfo("caller=%p, handle=%p", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        _callerObj->abort();
        return;
    }
    else {
        qInfo() << "Found activity: " << _activity->uid.c_str();
        qInfo() << "Close progress dialog";

        m_boxSession.reset();
        Q_EMIT appCloseProgressDiag(reinterpret_cast<quint64>(_hdl));
    }
}

/**
 * @brief Proxy::onDisplayInputDiag
 * @param _caller
 */
void Proxy::onDisplayInputList(QObjSharedPtr _caller,
                               const QVariantMap &_entries)
{
    qInfo() << "In";

    auto _callerObj = unpackCaller<UserInterface::InputDialog>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _callerObj->thread();
    qInfo("caller=%p, handle=%p", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        _callerObj->abort();
        return;
    }
    else {
        qInfo() << "Found activity: " << _activity->uid.c_str();

        // Hold the shared obj as the caller my go out of scope or get killed,
        // This will avoid us manipulating null pointers...
        m_InputSession = _caller;
        QVariant _l_id;
        Q_FOREACH(const QVariant& _ipt, _entries["inputs"].toList()) {
            QVariantMap _input = _ipt.toMap();
            qInfo("Entry(id=%d, value=%s, icon=%s)",
                  _input["id"].toInt(),
                    qPrintable(_input["val"].toString()),
                    qPrintable(_input["icon"].toString()));
            _l_id = _input["id"];
        }

        QThread::sleep(2);
        qInfo() << "Reply:" << _l_id;
        _callerObj->onReply(_l_id);
        m_InputSession.reset();
    }
}

/**
 * @brief Proxy::onDisplayInputFileBrowse
 * @param _caller
 * @param _path
 * @param _filter
 */
void Proxy::onDisplayInputFileBrowse(QObjSharedPtr _caller,
                                     const QString& _path,
                                     const QString& _filter)
{
    qInfo() << "In";

    auto _callerObj = unpackCaller<UserInterface::InputDialog>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _callerObj->thread();
    qInfo("caller=%p, handle=%p", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        _callerObj->abort();
        return;
    }
    else {
        qInfo() << "Found activity: " << _activity->uid.c_str();

        // Hold the shared obj as the caller my go out of scope or get killed,
        // This will avoid us ending up with dangling pointers...
        m_InputSession = _caller;

        qInfo() << ", path: " << _path
                << ", filter: " << _filter;

        QThread::sleep(2);
        _callerObj->onReply(QVariant("myfile.txt"));
        m_InputSession.reset();
    }
}

/**
 * @brief Proxy::onDisplayInputKeyboard
 * @param _caller
 * @param _default
 */
void Proxy::onDisplayInputKeyboard(QObjSharedPtr _caller,
                                   const QString& _default)
{
    qInfo() << "In";

    auto _callerObj = unpackCaller<UserInterface::InputDialog>(_caller);
    if(!_callerObj) {
        return;
    }

    auto _hdl = _callerObj->thread();
    qInfo("caller=%p, handle=%p", _callerObj, _hdl);

    auto _activity = getActivity(_hdl);
    if(!_activity) {
        qCritical() << "Could not find activity !";
        _callerObj->abort();
        return;
    }
    else {
        qInfo() << "Found activity: " << _activity->uid.c_str();

        // Hold the shared obj as the caller my go out of scope or get killed,
        // This will avoid us manipulating null pointers...
        m_InputSession = _caller;

        qInfo() << ", path: " << _default;

        QThread::sleep(2);
        _callerObj->onReply(QVariant("Hello from User Interface"));
        m_InputSession.reset();
    }
}

template<typename T>
T * Proxy::unpackCaller(QObjSharedPtr _caller)
{
    if(!_caller) {
        qCritical() << "Null received QObjSharedPtr !!";
        return nullptr;
    }

    T* _callerObj = std::dynamic_pointer_cast<T>(_caller).get();

    if(!_callerObj) {
        qCritical() << "Null received _callerObj !!";
        return nullptr;
    }
    else {
        return _callerObj;
    }
}

} // namespace KvalApplication
