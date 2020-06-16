#pragma once

#include <pthread.h>
#include <atomic>
#include <sys/time.h>
#include <QObject>
#include "KvalThreadUtils.h"
#include <QMutex>
#include <QWaitCondition>

#include "Codec.h"
#include "Pin.h"

/**
==== Execution Flow: ====

     WaitingForExecute
            |
            V
        Initializing
            |
            |<---------------------------------------------
            V                                             |
          Idle  --> [Play] -->  Executing --> [Pause] -----
            |                       |
            V                       |
        Terminating <----------------
            |
            V
        Terminated

==== ==== ==== ==== ====
*/

/**
 * @brief The ExecutionStateEnum enum
 */
enum class ExecutionStateEnum
{
    WaitingForExecute = 0,
    Initializing,
    Executing,
    Idle,
    Terminating,
    Terminated
};

/**
 * @brief The AMLComponent class
 */
class AMLComponent : public QObject,
                     public std::enable_shared_from_this<AMLComponent>
{
    Q_OBJECT

public:

    Q_INVOKABLE void InternalWorkThread();
    Q_INVOKABLE void cleanup();

    virtual ~AMLComponent() {}

    virtual MediaState State() const;
    virtual void SetState(MediaState value);
    virtual void Execute();
    virtual void Terminate();
    virtual void ChangeState(MediaState oldState, MediaState newState);
    virtual void Flush();
    virtual void Close() = 0;

    void returnAllBuffers();
    PinInfCollection* PinInfos();
    ExecutionStateEnum ExecutionState() const;
    bool IsExecuting() const;
    const char * name() const { return (const char *)m_name.c_str(); }
    void SetName(std::string name) { m_name = name; }
    void WaitForExecutionState(ExecutionStateEnum state);

Q_SIGNALS:
    void finished();

protected:
    virtual void Initialize();
    virtual void DoWork();
    virtual void Idling();
    virtual void Idled();
    virtual void Terminating();
    virtual void State_Executing();

    void SetExecutionState(ExecutionStateEnum newState);
    void AddInfoPin(PinInfoSPTR pin);
    void ClearInfosPins();
    bool CheckCurrentState();

    QMutex m_state_mutex;
    //State Change wait condition
    QMutex m_wait_state_change;
    QWaitCondition m_wait_state_cmd;
    //Execution Change wait condition
    QWaitCondition m_execution_wait_cmd;
    QMutex m_execution_wait_mtx;
    pthread_cond_t executionWaitCondition;
    pthread_mutex_t executionWaitMutex;
    std::atomic<MediaState> state {Pause};
    QMutex m_returnAllBuffersMtx;
    QWaitCondition m_returnAllBuffersCmd;
    bool m_returnAllBuffers = false;
    bool m_seekRequested = false;

private:
    std::atomic_bool abort_flag{false};
    PinInfCollection pininfos;
    ExecutionStateEnum executionState{ExecutionStateEnum::WaitingForExecute};

    bool canSleep = true;
    std::string m_name{"AMLComponent"};
};
typedef std::shared_ptr<AMLComponent> ElementSPTR;
typedef std::weak_ptr<AMLComponent> ElementWPTR;
