#include "KvalAppElement.h"

namespace KvalApplication {

/**
 * @brief Element::Element
 * @param parent
 */
Element::Element(QObject *parent) :
    QObject(parent),
    m_managerth(new KvalThread(&m_manager))
{
    m_managerth->start();

    qInfo() << "Start Manager...";
    QMetaObject::invokeMethod(&m_manager, "Start");

    // TODO: Connect proxy UI signals here ...
    connect(Proxy::_proxyRef().data(), &Proxy::appDisplayAlert,
            this, &Element::appDisplayAlert,
            Qt::QueuedConnection);
    connect(Proxy::_proxyRef().data(), &Proxy::appDisplaySimpleDiag,
            this, &Element::appDisplaySimpleDiag,
            Qt::QueuedConnection);

}

/**
 * @brief Element::~Element
 */
Element::~Element()
{
    m_managerth->stop();
    delete m_managerth;
}

} // namespace KvalApplication
