#ifndef KVAL_THREADUTILS_H
#define KVAL_THREADUTILS_H

#include <QDebug>
#include <QObject>
#include <QString>
#include <QMap>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QMutexLocker>

/**
 * @brief The TU_QThread class
 */
class KvalThread : public QThread
{
public:

    explicit KvalThread(const QString& name,
                        Priority pr = QThread::NormalPriority,
                        QObject* parent = 0) :
        QThread(parent),
        m_name{name},
        _pr{pr}
    {
            qDebug() << "Create _Th: " << m_name;
            this->setObjectName(name);
    }

    explicit KvalThread(QObject* obj,
                        Priority pr = QThread::NormalPriority,
                        QObject* parent = 0) :
        QThread(parent),
        m_name{obj->metaObject()->className()},
        _pr{pr}
    {
            qDebug() << "Create _Th: " << m_name;
            this->setObjectName(m_name);
            obj->moveToThread(this);
    }

    virtual ~KvalThread() {
        qDebug() << "Erase: " << _m_threadHdles[m_threadId];
        auto thread_id = _m_threadHdles.find(m_threadId);
        if(thread_id != _m_threadHdles.end())
            _m_threadHdles.erase(thread_id);
    }

    static void display_threads_status(void)
    {
        for(const auto &it: _m_threadHdles){
            qDebug() << "Remaining threads: " << it;
        }
    }
    inline Qt::HANDLE getThreadId() {
        return currentThreadId();
    }

    void stop() {
        quit();
        wait();
    }

protected:
    void run() {
        qDebug() << "Started _Th: " << m_name;
        m_threadId = QThread::currentThreadId();
        _m_threadHdles.insert(m_threadId, m_name);
        setPriority(_pr);
        QThread::run();
    }

private:
    static QHash<Qt::HANDLE, QString> _m_threadHdles;
    Qt::HANDLE m_threadId{nullptr};
    QString m_name{};
    Priority _pr{QThread::NormalPriority};
};

#endif // KVAL_THREADUTILS_H
