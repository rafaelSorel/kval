#include <QDebug>
#include "KvalConfigManager.h"
#include "KvalAppDeployment.h"

namespace KvalApplication {

/**
 * @brief DeploymentEngine::DeploymentEngine
 * @param parent
 */
DeploymentEngine::DeploymentEngine(shared_ptr<LauncherEngine> launcher,
                                   QObject *parent) :
    QObject(parent),
    m_shdLauncher(launcher)
{

}

/**
 * @brief DeploymentEngine::~DeploymentEngine
 */
DeploymentEngine::~DeploymentEngine()
{
    qInfo() << "In";
}

/**
 * @brief DeploymentEngine::availableApps
 *        Only the caller knows how to deal with data provided by the
 *        attribute return and hence this is the only place that it knows
 *        how to convert pyobj to internal data.
 *        The idea of exposing pyobj outside of the launcher is not the best one
 *        it will remain so until a better idea shows up.
 * @param obj
 */
void _cbDepApps(QObject* _caller, const pybind11::object& obj)
{
    vector<KApplication> m_apps;
    pybind11::list apps = obj.cast<pybind11::list>();
    for(const auto& app: apps) {
        m_apps.push_back(app.cast<KApplication>());
        qInfo() << "append application id: "
                << m_apps[0].get_id().c_str();
    }

    dynamic_cast<DeploymentEngine*>(_caller)->availableApps(m_apps);
}

/**
 * @brief availableApps
 * @param apps
 */
void DeploymentEngine::availableApps(vector<KApplication>& apps)
{
    m_apps = std::move(apps);
    Q_EMIT appsReady();
}
/**
 * @brief DeploymentEngine::extractApps
 * @return
 */
void DeploymentEngine::extractApps()
{
    qInfo() << "Extract installed applications...";

#if 0
    shared_ptr<ExecStruct> _exec = make_shared<ExecStruct>();
    _exec->appid = "kval_app";
    _exec->service = false;
    _exec->syncall = true;
    _exec->depPaths.push_back("$HOME/opt/kval/kval_apps");
    _exec->module = "kvalapp";
    _exec->attr = "extract_available_apps";
    _exec->_caller = this;
    _exec->_cb = &_cbDepApps;

    QMetaObject::invokeMethod(m_shdLauncher.get(),
                              "onStartApp",
                              Qt::QueuedConnection,
                              Q_ARG(shared_ptr<ExecStruct>, _exec));


    QThread::sleep(1);
#endif
    shared_ptr<ExecStruct> _execpath = make_shared<ExecStruct>();
    _execpath->appid = "kaval.app.utests.activity";
    _execpath->service = false;
    _execpath->syncall = false;
    _execpath->execpath = "$HOME/opt/kval/kval_apps/tests/builtin_tests.py";
    QMetaObject::invokeMethod(m_shdLauncher.get(),
                              "onStartApp",
                              Qt::QueuedConnection,
                              Q_ARG(shared_ptr<ExecStruct>, _execpath));

//    QThread::sleep(3);
//    QMetaObject::invokeMethod(m_shdLauncher.get(),
//                              "onStopApp",
//                              Qt::QueuedConnection,
//                              Q_ARG(string, "kaval.app.daemon"));
}

/**
 * @brief DeploymentEngine::getApps
 * @return
 */
const vector<KApplication> &DeploymentEngine::getApps()
{
    return  m_apps;
}
/**
 * @brief DeploymentEngine::installApp
 * @param appid
 * @return
 */
bool DeploymentEngine::installApp(const QString &appid)
{
    Q_UNUSED(appid)

    return false;
}

/**
 * @brief DeploymentEngine::unInstallApp
 * @param appid
 * @return
 */
bool DeploymentEngine::unInstallApp(const QString &appid)
{
    Q_UNUSED(appid)

    return false;

}

/**
 * @brief DeploymentEngine::updateApp
 * @param appid
 * @return
 */
bool DeploymentEngine::updateApp(const QString &appid)
{
    Q_UNUSED(appid)

    return false;
}

/**
 * @brief DeploymentEngine::addToFavorite
 * @param appid
 * @return
 */
bool DeploymentEngine::addToFavorite(const QString &appid)
{
    Q_UNUSED(appid)

    return false;
}

} // namespace KvalApplication
