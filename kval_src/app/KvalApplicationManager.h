#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "KvalAppDeployment.h"
#include "KvalAppProxy.h"
#include "KvalAppLauncher.h"
#include "KvalThreadUtils.h"

namespace KvalApplication {

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = nullptr);
    virtual ~Manager();

Q_SIGNALS:

public Q_SLOTS:
    void Start();
    void onAppsReady();

private:
    std::shared_ptr<Proxy> m_proxy{nullptr};
    std::shared_ptr<LauncherEngine> m_launcher{nullptr};
    std::unique_ptr<DeploymentEngine> m_deploy{nullptr};
    KvalThread *m_launcherth{nullptr};
    KvalThread *m_proxyth{nullptr};
};

} // namespace KvalApplication

#endif // MANAGER_H
