#ifndef ELEMENT_H
#define ELEMENT_H

#include <QObject>
#include "KvalThreadUtils.h"
#include "KvalApplicationManager.h"

namespace KvalApplication {

class Element : public QObject
{
    Q_OBJECT

public:
    explicit Element(QObject *parent = nullptr);
    virtual ~Element();

public Q_SLOTS:

Q_SIGNALS:
    void appDisplayAlert(const QVariantMap&);
    void appDisplaySimpleDiag(quint64, const QVariantMap&);


private:
    QString m_test{"this is my test text"};
    Manager m_manager;
    KvalThread * m_managerth;
};

} // namespace KvalApplication

#endif // ELEMENT_H
