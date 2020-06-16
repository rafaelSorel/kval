#include "../KvalAppProxy.h"
#include "KvalAppAbsBuiltin.h"


namespace KvalApplication {

/**
 * @brief BuiltinBaseClass::BuiltinBaseClass
 * @param parent
 */
BuiltinBaseClass::BuiltinBaseClass(QObject *parent) :
    QObject(parent),
    enable_shared_from_this<BuiltinBaseClass>(),
    m_proxy(Proxy::_proxyRef())
{
    qInfo() << "Instantiante...";
}

/**
 * @brief BuiltinBaseClass::~BuiltinBaseClass
 */
BuiltinBaseClass::~BuiltinBaseClass()
{
    qInfo() << "Destruct...";
}

} // namespace KvalApplication
