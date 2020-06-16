#ifndef DEPLOYMENTENGINE_H
#define DEPLOYMENTENGINE_H

#include <QObject>
#include <QDebug>
#include "KvalAppLauncher.h"

using namespace std;

namespace KvalApplication {

class KApplication;

/**
 * @brief The DeploymentEngine class
 */
class DeploymentEngine : public QObject
{
    Q_OBJECT
public:
    explicit DeploymentEngine(shared_ptr<LauncherEngine> launcher,
                              QObject *parent = nullptr);
    virtual ~DeploymentEngine();

    void extractApps();
    const vector<KApplication> &getApps();
    bool installApp(const QString &appid);
    bool unInstallApp(const QString &appid);
    bool updateApp(const QString &appid);
    bool addToFavorite(const QString &appid);
    void availableApps(vector<KApplication>&);

Q_SIGNALS:
    void appsReady(void);

private:
    shared_ptr<LauncherEngine> m_shdLauncher;
    vector<KApplication> m_apps;
};


/**
 * @brief The KApplication class
 */
class KApplication {

public:
    KApplication() = default;
    explicit KApplication(const string& id, const string& version) :
        m_id(id),
        m_version(version)
    {
        qInfo() << ">>>>>>>>>>>>>> Construct";
    }
    KApplication(const KApplication& rhs) = default;
    KApplication(KApplication&& rhs)
    {
        qInfo() << "move constructor";
        m_id = move(rhs.m_id);
        m_version = move(rhs.m_version);
        m_launcher = move(rhs.m_launcher);
        m_dependencies = move(rhs.m_dependencies);
    }
    KApplication& operator=(const KApplication& rhs) = delete;

    virtual ~KApplication() {
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

    struct Meta{
        struct Desc{
            string lang;
            string payload;
        };
        string icon{};
        string back{};
        string fanart{};
        vector<Desc> summaries;
        vector<Desc> descriptions;
        vector<Desc> disclaimers;
        string platform;
        qint64 size;
    };

    string get_id() const {
        qInfo() << "get me the id: " << m_id.c_str();
        return m_id;
    }

    void addDep(const Dependency& dep)
    {
        m_dependencies.push_back(move(dep));
    }
    void setMeta(Meta&& meta)
    {
        m_meta = move(meta);
    }
    void setLauncher(const Launcher& launcher)
    {
        m_launcher = move(launcher);
    }

private:
    string m_id{};
    string m_version{};
    Launcher m_launcher{};
    Meta m_meta{};
    vector<Dependency> m_dependencies{};
};

} // namespace KvalApplication

#endif // DEPLOYMENTENGINE_H
