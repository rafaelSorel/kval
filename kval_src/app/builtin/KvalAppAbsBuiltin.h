#ifndef BUILTINABSCLASS_H
#define BUILTINABSCLASS_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include "KvalAppBuiltinCommon.h"

namespace KvalApplication {

class Proxy;

class BuiltinBaseClass : public QObject, public enable_shared_from_this<BuiltinBaseClass>
{
    Q_OBJECT
public:
    explicit BuiltinBaseClass(QObject *parent = nullptr);
    virtual ~BuiltinBaseClass();

Q_SIGNALS:

protected:
    struct BlockingCallScope
    {
        PyThreadState *_save;
        BlockingCallScope(): _save{nullptr} {
            //    Py_BEGIN_ALLOW_THREADS
            //    Py_END_ALLOW_THREADS
            _save = PyEval_SaveThread();
        }
        ~BlockingCallScope() {
            if(_save){
                PyEval_RestoreThread(_save);
            }
        }
    };

    QPointer<Proxy> _proxy() {
        return m_proxy;
    }
    QObjSharedPtr _shref() {
        return dynamic_pointer_cast<QObject>(shared_from_this());
    }

private:
    QPointer<Proxy> m_proxy;
};

} // namespace KvalApplication

#endif // BUILTINABSCLASS_H
