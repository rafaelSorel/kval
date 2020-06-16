#include <QObject>
#include "KvalThreadUtils.h"


#include "AMLComponent.h"
#define LOG_ACTIVATED
#define LOG_SRC AMLCOMPONENT
#include "KvalLogging.h"

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Methods
//----------------------------------------------------------------------------

/**
 * @brief AMLComponent::CheckStopRequest
 * @return
 */
bool AMLComponent::CheckCurrentState()
{
    INV();
    QMutexLocker locker(&m_wait_state_change);
    bool status = true;
    switch (State())
    {
        case Stop:
            if (ExecutionState() != ExecutionStateEnum::Terminating &&
                ExecutionState() != ExecutionStateEnum::Terminated)
                SetExecutionState(ExecutionStateEnum::Terminating);

            status = false;
            goto return_status;

        case Pause:
            if (ExecutionState() != ExecutionStateEnum::Idle)
                SetExecutionState(ExecutionStateEnum::Idle);

            m_wait_state_cmd.wait(&m_wait_state_change);
            break;

        case Play:
        default:
            break;
    }
    if (ExecutionState() != ExecutionStateEnum::Executing &&
        ExecutionState() != ExecutionStateEnum::Terminating)
        SetExecutionState(ExecutionStateEnum::Executing);

return_status:
    OUTV("Status: %u", status);
    return status;

}

/**
 * @brief AMLComponent::SetExecutionState
 * @param newState
 */
void AMLComponent::SetExecutionState(ExecutionStateEnum newState)
{
    INV("newState: %u", newState);
    ExecutionStateEnum current = executionState;

    if (newState != current)
    {
        switch (current)
        {
            case ExecutionStateEnum::WaitingForExecute:
                if (newState != ExecutionStateEnum::Initializing && !abort_flag)
                {
                    LOG_ERROR(LOG_TAG, "newState != Initializing");
                    OUTV();
                    return;
                }
                break;

            case ExecutionStateEnum::Initializing:
                if (newState != ExecutionStateEnum::Idle && !abort_flag)
                {
                    LOG_ERROR(LOG_TAG, "newState != Idle");
                    OUTV();
                    return;
                }
                break;

            case ExecutionStateEnum::Executing:
                if (newState != ExecutionStateEnum::Idle &&
                    newState != ExecutionStateEnum::Terminating &&
                    !abort_flag)
                {
                    LOG_ERROR(LOG_TAG, "newState != Idle/Terminating");
                    OUTV();
                    return;
                }
                break;

            case ExecutionStateEnum::Idle:
                if (newState != ExecutionStateEnum::Executing &&
                    newState != ExecutionStateEnum::Terminating &&
                    !abort_flag)
                {
                    LOG_ERROR(LOG_TAG, "newState != Executing/Terminating");
                    OUTV();
                    return;
                }
                break;

            case ExecutionStateEnum::Terminating:
                if (newState != ExecutionStateEnum::Terminated && !abort_flag)
                {
                    LOG_ERROR(LOG_TAG, "newState != Terminated");
                    OUTV();
                    return;
                }
                break;
            case ExecutionStateEnum::Terminated:
                LOG_INFO(LOG_TAG, "newState == Terminated");
                break;
            default:
                break;
        }
    }

    m_execution_wait_mtx.lock();
        executionState = newState;
        m_execution_wait_cmd.wakeAll();
    m_execution_wait_mtx.unlock();

    LOG_DEBUG(LOG_TAG, "%s Set ExecutionState=%d", name(), (int)newState);
    OUTV();
}

/**
 * @brief AMLComponent::Initialize
 */
void AMLComponent::Initialize()
{
    INV();
    OUTV();
}

/**
 * @brief AMLComponent::DoWork
 */
void AMLComponent::DoWork()
{
    INV();
    LOG_INFO(LOG_TAG, "(%s) DoWork exited.", name());
    OUTV();
}

/**
 * @brief AMLComponent::Idling
 */
void AMLComponent::Idling()
{
    INV();
    OUTV();
}

/**
 * @brief AMLComponent::Idled
 */
void AMLComponent::Idled()
{
    INV();
    OUTV();
}

/**
 * @brief AMLComponent::Terminating
 */
void AMLComponent::Terminating()
{
    INV();
    OUTV();
}

/**
 * @brief AMLComponent::State_Executing
 */
void AMLComponent::State_Executing()
{
    INV();
    LOG_INFO(LOG_TAG, "State_Executing pchiit");
    OUTV();
}

/**
 * @brief AMLComponent::InternalWorkThread
 */
void AMLComponent::InternalWorkThread()
{
    INV();
    LOG_INFO(LOG_TAG, "(%s) InternalWorkThread entered.", name());

    SetExecutionState(ExecutionStateEnum::Initializing);
    Initialize();

    SetExecutionState(ExecutionStateEnum::Idle);
    State_Executing();

    OUTV();
}

/**
 * @brief AMLComponent::AddInfoPin
 * @param pin
 */
void AMLComponent::AddInfoPin(PinInfoSPTR pin)
{
    INV("pin: %p", pin.get());
    pininfos.Add(pin);
    OUTV();
}

/**
 * @brief AMLComponent::ClearInfosPins
 */
void AMLComponent::ClearInfosPins()
{
    INV();
    pininfos.Clear();
    OUTV();
}
/**
 * @brief AMLComponent::PinInfos
 * @return
 */
PinInfCollection* AMLComponent::PinInfos()
{
    INV();
    OUTV("pininfos: %p", &pininfos);
    return &pininfos;
}

/**
 * @brief AMLComponent::ExecutionState
 * @return
 */
ExecutionStateEnum AMLComponent::ExecutionState() const
{
    INV();
    ExecutionStateEnum result;
    result = executionState;
    OUTV("result: %u", result);
    return result;
}

/**
 * @brief AMLComponent::IsExecuting
 * @return
 */
bool AMLComponent::IsExecuting() const
{
    INV();
    bool state = State() == Play &&
            ExecutionState() == ExecutionStateEnum::Executing;
    OUTV("state: %u", state);
    return state;
}

/**
 * @brief AMLComponent::State
 * @return
 */
MediaState AMLComponent::State() const
{
    INV();
    OUTV("state: %u", (MediaState)state);
    return state;
}

/**
 * @brief AMLComponent::SetState
 * @param value
 */
void AMLComponent::SetState(MediaState value)
{
    INV("value: %u", value);
    if(state == Stop && value < Stop)
    {
        LOG_WARNING(LOG_TAG, "Stop has already been set , ignore any Play/Pause request !");
        return;
    }
    if (state != value)
    {
        ChangeState(state, value);
    }
    OUTV();
}

/**
 * @brief AMLComponent::returnAllBuffers
 */
void AMLComponent::returnAllBuffers()
{
    INV();
    if(state == Stop)
    {
        OUTV();
        return;
    }
    m_returnAllBuffersMtx.lock();
    m_returnAllBuffers = true;
    LOG_INFO(LOG_TAG, "Return all buffers ...");
    SetState(Play);
    LOG_INFO(LOG_TAG, "Send marker buffer ...");
    MarkerBufferSPTR disconMarker =
            std::make_shared<MarkerBuffer>(nullptr, MarkerEnum::Discontinue);

    QMetaObject::invokeMethod(this,
                              "onBufferAvailable",
                              Q_ARG(BufferSPTR, disconMarker));


    m_returnAllBuffersCmd.wait(&m_returnAllBuffersMtx);
    m_returnAllBuffersMtx.unlock();
    SetState(Pause);
    LOG_INFO(LOG_TAG, "All buffers returned...");
    OUTV();
}


/**
 * @brief AMLComponent::Execute
 */
void AMLComponent::Execute()
{
    INV();
    if(executionState != ExecutionStateEnum::WaitingForExecute)
    {
        LOG_ERROR(LOG_TAG, "executionState != WaitingForExecute");
        OUTV();
        return;
    }
    QMetaObject::invokeMethod(this, "InternalWorkThread");
    LOG_INFO(LOG_TAG, "(%s) Execute.", name());
    OUTV();
}

/**
 * @brief AMLComponent::Terminate
 */
void AMLComponent::Terminate()
{
    INV();
    if (executionState == ExecutionStateEnum::WaitingForExecute)
    {
        LOG_ERROR(LOG_TAG, "ABORT ...");
        abort_flag = true;
        QMetaObject::invokeMethod(this, "cleanup");
        OUTV();
        return;
    }

    LOG_INFO(LOG_TAG, "(%s) Set Stop State", name());
    SetState(Stop);

    if (ExecutionState() != ExecutionStateEnum::Terminating)
    {
        LOG_INFO(LOG_TAG, "(%s) Set Terminating execState", name());
        SetExecutionState(ExecutionStateEnum::Terminating);
    }

    WaitForExecutionState(ExecutionStateEnum::Terminating);

    LOG_INFO(LOG_TAG, "(%s) Call Flush", name());
    Flush();

    LOG_INFO(LOG_TAG, "(%s) State - Terminating", name());
    SetExecutionState(ExecutionStateEnum::Terminating);
    Terminating();

    WaitForExecutionState(ExecutionStateEnum::Terminating);

    QMetaObject::invokeMethod(this, "cleanup");
    LOG_INFO(LOG_TAG, "(%s) Terminate.", name());
    OUTV();
}

/**
 * @brief AMLComponent::ChangeState
 * @param oldState
 * @param newState
 */
void AMLComponent::ChangeState(MediaState oldState, MediaState newState)
{
    INV("oldState: %u, newState: %u", oldState, newState);
    state = newState;

    LOG_INFO(LOG_TAG,
             "(%s) ChangeState oldState=%d newState=%d.",
             name(),
             (int)oldState,
             (int)newState);
    OUTV();
}

/**
 * @brief AMLComponent::WaitForExecutionState
 * @param state
 */
void AMLComponent::WaitForExecutionState(ExecutionStateEnum state)
{
    INV("state: %u", state);

    m_execution_wait_mtx.lock();
    while (executionState != state)
    {
        m_execution_wait_cmd.wait(&m_execution_wait_mtx);
    }
    m_execution_wait_mtx.unlock();

    OUTV();
}

/**
 * @brief AMLComponent::Flush
 */
void AMLComponent::Flush()
{
    INV();
    if (State() == Play)
    {
        LOG_ERROR(LOG_TAG, "State != Pause && State != Stop");
        OUTV();
        return;
    }
    else if (State() == Pause)
    {
        LOG_INFO(LOG_TAG, "(%s) Wait for Idle", name());
        LOG_INFO(LOG_TAG, "(%s)  current executionState: %u" , name(), executionState);

        WaitForExecutionState(ExecutionStateEnum::Idle);
    }
    else if(State() == Stop)
    {
        WaitForExecutionState(ExecutionStateEnum::Terminating);
    }

    pininfos.Flush();

    LOG_INFO(LOG_TAG, "(%s) Flush exited.", name());
    OUTV();
}

/**
 * @brief AMLComponent::cleanup
 */
void AMLComponent::cleanup()
{
    INV();
    LOG_INFO(LOG_TAG, "(%s) cleanup called.", name());
    SetExecutionState(ExecutionStateEnum::Terminated);

    OUTV();
}

