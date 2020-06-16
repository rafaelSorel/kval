#include <QObject>
#include <QDebug>
#include <memory>
#include "KvalApplicationManager.h"

namespace KvalApplication {

/**
 * @brief Manager::Manager
 * @param parent
 */
Manager::Manager(QObject *parent) :
    QObject(parent),
    m_proxy{std::make_shared<Proxy>()},
    m_launcher{std::make_shared<LauncherEngine>(m_proxy)},
    m_deploy{new DeploymentEngine(m_launcher)}
{
    qInfo() << "Instantiate Manager";
    m_proxyth = new KvalThread(m_proxy.get());
    m_launcherth = new KvalThread(m_launcher.get());

    m_launcherth->start();
    m_proxyth->start();
    QMetaObject::invokeMethod(m_launcher.get(), "Start");
}

/**
 * @brief Manager::~Manager
 */
Manager::~Manager()
{
    if(m_launcherth){
        m_launcherth->stop();
        delete m_launcherth;
    }

    if(m_proxyth){
        m_proxyth->stop();
        delete m_proxyth;
    }

}
/**
 * @brief Manager::Start
 */
void Manager::Start()
{
    qInfo() << "In";
    connect(m_deploy.get(), SIGNAL(appsReady()),
            this, SLOT(onAppsReady()), Qt::QueuedConnection);

    m_deploy->extractApps();
}

/**
 * @brief Manager::onAppsReady
 */
void Manager::onAppsReady()
{
    qInfo() << "applications list available...";
    const vector<KApplication>& apps = m_deploy->getApps();

    if(apps.size()>0)
        qInfo() << "App name: " << apps[0].get_id().c_str();
}

} // namespace KvalApplication
