#include <utility>
#include <algorithm>
#include <typeinfo>
#include <cstdint>

#include "OMXOverlayText.h"
#include "SubtitleCodecElement.h"
#define LOG_ACTIVATED
#define LOG_SRC AMLSUBS
#include "KvalLogging.h"

using namespace std;

/**
 * @brief SubtitleQml_renderer::SubtitleQml_renderer
 */
SubtitleQml_renderer::SubtitleQml_renderer()
{
    INV();
    m_av_clock = nullptr;
    OUTV();
}

/**
 * @brief SubtitleQml_renderer::~SubtitleQml_renderer
 */
SubtitleQml_renderer::~SubtitleQml_renderer()
{
    INV();
    LOG_INFO(LOG_TAG,
             "+++++++++++++++++ ~SubtitleQml_renderer +++++++++++++++++");
    LOG_INFO(LOG_TAG,
             "m_subtitle_buffers.size(): %zu",
             m_subtitle_buffers.size());
    m_subtitle_buffers.clear();

    OUTV();
}


/**
 * @brief SubtitleQml_renderer::Open
 * @return
 */
bool SubtitleQml_renderer::Open(AmlVideoSinkClock* av_clock)
{
    INV();

    if(av_clock)
        m_av_clock = av_clock;

    SetName("SubtitleQml_renderer");
    Execute();
    WaitForExecutionState(ExecutionStateEnum::Idle);

    OUTV("true");
    return true;
}

/**
 * @brief SubtitleQml_renderer::setSubsBuffer
 * @param subtitle
 */
void SubtitleQml_renderer::setSubsBuffer(Subtitle subtitle)
{
    INV("subtitle: %p", &subtitle);
    subs_buffers_mtx.lock();
    auto stop = subtitle.stop;
    if (!m_subtitle_buffers.empty() &&
        subtitle.stop < m_subtitle_buffers.back().stop)
    {
        stop = m_subtitle_buffers.back().stop;
    }

    if(m_visible)
    {
        m_subtitle_buffers.push_back(Subtitle(subtitle.start,
                                              stop,
                                              subtitle.text_lines));
    }
    subs_buffers_mtx.unlock();
}

/**
 * @brief SubtitleQml_renderer::Initialize
 */
void SubtitleQml_renderer::Initialize()
{
    INV();
    OUTV();
}

/**
 * @brief SubtitleQml_renderer::DoWork
 */
void SubtitleQml_renderer::DoWork()
{
    INV();
    OUTV();
}


template <typename Iterator>
Iterator FindSubtitle(Iterator begin,
                      Iterator end,
                      int time)
{
  return upper_bound(begin, end, time,
    [](int a, const Subtitle& b) { return a < b.stop; });
}

/**
 * @brief SubtitleQml_renderer::State_Executing
 */
void SubtitleQml_renderer::State_Executing()
{
    INV();
    LOG_INFO(LOG_TAG, "State_Executing Started... SubtitleQml_renderer_Thread in work !");

    int prev_now{};
    m_next_index = 0;
    bool have_next{};
    int current_stop = INT_MIN;
    bool showing{};

    auto TryPrepare = [&](int time)
    {
        for(; m_next_index != m_subtitle_buffers.size(); ++m_next_index)
        {
            if(m_subtitle_buffers[m_next_index].stop > time)
            {
                LOG_INFO(LOG_TAG, "TryPrepare renderer.prepare");
                subs_prepare(m_subtitle_buffers[m_next_index].text_lines);
                have_next = true;
                break;
            }
        }
    };

    auto Reset = [&](int time)
    {
        LOG_INFO(LOG_TAG, "Reset renderer.unprepare");
        subs_unprepare();
        current_stop = INT_MIN;

        auto it = FindSubtitle( m_subtitle_buffers.begin(),
                                m_subtitle_buffers.end(),
                                time);
        m_next_index = it - m_subtitle_buffers.begin();

        if(m_next_index != m_subtitle_buffers.size())
        {
            LOG_INFO(LOG_TAG, "m_next_index renderer.prepare");
            subs_prepare(m_subtitle_buffers[m_next_index].text_lines);
            have_next = true;
        }
        else
        {
            have_next = false;
        }
    };

    while(!m_stop_request)
    {
        if(!CheckCurrentState())
            break;

        subs_buffers_mtx.lock();
        if(!m_subtitle_buffers.size())
        {
            subs_buffers_mtx.unlock();
            usleep(10000);
            continue;
        }
        int timeout = INT_MAX;
        auto now = 0;

        if(m_av_clock)
            now = static_cast<int>(m_av_clock->Clock() * 1000);
        LOG_DEBUG(LOG_TAG, "now: %d...", now);
        LOG_DEBUG(LOG_TAG,
                 "m_subtitle_buffers[m_next_index].start: %d...",
                 m_subtitle_buffers[m_next_index].start);
        int till_stop = showing ? current_stop - now : INT_MAX;
        int till_next_start =   have_next ?
                                m_subtitle_buffers[m_next_index].start - now :
                                INT_MAX;

        timeout = min(min(till_stop, till_next_start), 1000);

        LOG_DEBUG(LOG_TAG, "timeout: %d", timeout);

        if (timeout > 0)
            usleep(timeout);

        if( (now < prev_now) ||
            (have_next && m_subtitle_buffers[m_next_index].stop <= now))
        {
            Reset(now);
        }
        else if(!have_next)
        {
            TryPrepare(now);
        }

        prev_now = now;

        if(current_stop <= now)
        {
            if(have_next && m_subtitle_buffers[m_next_index].start <= now)
            {
                LOG_INFO(LOG_TAG, "Show Subtitle Now");
                omxSubs_show_next();
                showing = true;
                current_stop = m_subtitle_buffers[m_next_index].stop;

                ++m_next_index;
                have_next = false;
                TryPrepare(now);
            }
            else if(showing)
            {
                LOG_INFO(LOG_TAG, "hide Subtitle");
                omxSubs_hide();
                showing = false;
            }
        }
        subs_buffers_mtx.unlock();

        if (ExecutionState() < ExecutionStateEnum::Idle)
            SetExecutionState(ExecutionStateEnum::Idle);

        usleep(1000);
    }

    m_mutexPending.lock();
    if (m_stop_request)
    {
        LOG_INFO(LOG_TAG, "Wake the Stop command.");
        m_stop_request = false;
        m_waitPendingCommand.wakeAll();
    }
    m_mutexPending.unlock();

    LOG_INFO(LOG_TAG, "Out State_Executing");

    OUTV();
}

/**
 * @brief SubtitleQml_renderer::Flush
 */
void SubtitleQml_renderer::Flush()
{
    INV();
    AMLComponent::Flush();
    subs_buffers_mtx.lock();
    m_subtitle_buffers.clear();
    m_next_index = 0;
    Q_EMIT hideSubs();
    subs_buffers_mtx.unlock();
    OUTV();
}

/**
 * @brief SubtitleQml_renderer::ChangeState
 * @param oldState
 * @param newState
 */
void SubtitleQml_renderer::ChangeState(MediaState oldState,
                                         MediaState newState)
{
    INV("oldState: %u, newState: %u",oldState, newState);
    QMutexLocker locker(&m_wait_state_change);
    AMLComponent::ChangeState(oldState, newState);
    switch (newState)
    {
        case Play:
        case Stop:
        {
            m_wait_state_cmd.wakeAll();
            break;
        }
        case Pause:
        default:
            break;
    }

    OUTV();
}

/**
 * @brief SubtitleQml_renderer::stopExec
 */
void SubtitleQml_renderer::stopExec()
{
    INV();
    switch (ExecutionState())
    {
        case ExecutionStateEnum::Idle:
        case ExecutionStateEnum::Executing:
            break;
        case ExecutionStateEnum::WaitingForExecute:
        case ExecutionStateEnum::Initializing:
        case ExecutionStateEnum::Terminating:
        case ExecutionStateEnum::Terminated:
        default:
            LOG_INFO(LOG_TAG, "Subs renderer not in action.");
            return;
    }

    LOG_INFO(LOG_TAG, "m_stop_request requested ....");
    if (State() == Pause)
    {
        m_wait_state_change.lock();
        m_wait_state_cmd.wakeAll();
        m_wait_state_change.unlock();
    }

    m_stop_request =true;
    m_mutexPending.lock();
    if(m_stop_request)
    {
        LOG_INFO(LOG_TAG, "Wait for the Stop command to finish.");
        m_waitPendingCommand.wait(&m_mutexPending);
    }
    m_mutexPending.unlock();

    OUTV();
}

/**
 * @brief SubtitleQml_renderer::Close
 */
void SubtitleQml_renderer::Close()
{
    stopExec();
    Terminate();
    WaitForExecutionState(ExecutionStateEnum::Terminated);

    Q_EMIT finished();
}

/**
 * @brief SubtitleQml_renderer::subs_prepare
 * @param text_lines
 */
void SubtitleQml_renderer::subs_prepare(QString text_lines)
{
    m_subsLine.clear();
    m_subsLine = text_lines;
    this->prepared_ = true;
}

/**
 * @brief SubtitleQml_renderer::omxSubs_draw
 */
void SubtitleQml_renderer::omxSubs_draw()
{
    LOG_INFO(LOG_TAG, "Q_EMIT showSubs()");
    QString showSubsList = m_subsLine;
    Q_EMIT showSubs(showSubsList);
}

/**
 * @brief SubtitleQml_renderer::omxSubs_clear
 */
void SubtitleQml_renderer::omxSubs_clear()
{
    Q_EMIT hideSubs();
}

/**
 * @brief SubtitleDecoderElement::SubtitleDecoderElement
 */
SubtitleDecoderElement::SubtitleDecoderElement()
{
    INV();
    m_subs_renderer = new SubtitleQml_renderer();

    m_subs_renderer_thread = new KvalThread("SubtitleQml_renderer");
    m_subs_renderer->moveToThread(m_subs_renderer_thread);
    m_subs_renderer_thread->setObjectName("SubtitleQml_renderer");
    connect(m_subs_renderer_thread, &QThread::finished, m_subs_renderer, &QObject::deleteLater);
    m_subs_renderer_thread->start();

    OUTV();
}

/**
 * @brief SubtitleDecoderElement::~SubtitleDecoderElement
 */
SubtitleDecoderElement::~SubtitleDecoderElement()
{
    INV();
    LOG_INFO(LOG_TAG,
             "+++++++++++++++++ ~SubtitleDecoderElement +++++++++++++++++");
    LOG_INFO(LOG_TAG, "Cleanup...");
    m_subs_renderer_thread->quit();
    m_subs_renderer_thread->wait();
    delete m_subs_renderer_thread;

    ClearInfosPins();
    OUTV();
}

/**
 * @brief SubtitleDecoderElement::Close
 */
void SubtitleDecoderElement::Close()
{
    INV();
    LOG_INFO(LOG_TAG, "Terminating SubtitleDecoderElement.");
    if(m_subs_renderer)
    {
        LOG_INFO(LOG_TAG, "Terminating subs_renderer.");
        m_subs_renderer->Close();
    }

    Terminate();
    WaitForExecutionState(ExecutionStateEnum::Terminated);

//    Q_EMIT finished();
    OUTV();
}

/**
 * @brief SubtitleDecoderElement::Open
 * @param aud_info
 * @param audio_codec
 * @param av_clock
 * @return
 */
bool SubtitleDecoderElement::Open(size_t stream_count,
                                  SubtitlePinInfo * subs_info,
                                  AmlVideoSinkClock* av_clock,
                                  AMLReader *aml_reader)
{
    INV("subs_info: %p, av_clock: %p, aml_reader: %p, stream_count: %zu",
        subs_info,
        av_clock,
        aml_reader,
        stream_count);

    if (subs_info->Category() != MediaCategoryEnum::Subtitle)
    {
        LOG_ERROR(LOG_TAG, "Not connected to a subs pin.");
        OUTV("false");
        return false;
    }


    if(!av_clock)
    {
        LOG_ERROR(LOG_TAG, "No Clock specified for subs.");
        OUTV("false");
        return false;
    }

    LOG_INFO(LOG_TAG, "Create Connections.");

    m_aml_reader    = aml_reader;
    m_subsInfo      = subs_info;

    m_av_clock = av_clock;

    connect(m_aml_reader,
            SIGNAL(bufferAvailable(BufferSPTR)),
            this,
            SLOT(onBufferAvailable(BufferSPTR)));
    connect(m_aml_reader,
            SIGNAL(bufferAvailable_S(BufferSPTR)),
            this,
            SLOT(onBufferAvailable(BufferSPTR)));
    connect(this,
            SIGNAL(sendBackBuffer(BufferSPTR)),
            m_aml_reader,
            SLOT(onBufferReturned(BufferSPTR)),
            Qt::DirectConnection);


    if(!SetupCodec())
    {
        LOG_WARNING(LOG_TAG, "Unhandled codec subs !");
    }
    isFirstData = false;

    SetName("AMLSubtitles");
    Execute();
    WaitForExecutionState(ExecutionStateEnum::Idle);

    LOG_INFO(LOG_TAG, "m_subs_renderer Open.");
    if(!m_subs_renderer->Open(m_av_clock))
    {
        LOG_ERROR(LOG_TAG, "Could not open subs_renderer device !");
        OUTV("false");
        return false;
    }

    OUTV("true");
    return true;
}

/**
 * @brief SubtitleDecoderElement::Initialize
 */
void SubtitleDecoderElement::Initialize()
{
    INV();
    OUTV();
}

/**
 * @brief SubtitleDecoderElement::SetupCodec
 * @return
 */
bool SubtitleDecoderElement::SetupCodec()
{
    INV();
    switch (m_subsInfo->Format)
    {
        case SubtitleFormatEnum::SubRip:
            break;

        case SubtitleFormatEnum::Pgs:
            break;

        case SubtitleFormatEnum::Dvb:
            break;

        case SubtitleFormatEnum::DvbTeletext:
            break;

        default:
            LOG_ERROR(LOG_TAG,
                      "Subtitle format %d is not supported.",
                      (int)m_subsInfo->Format);
            OUTV("false");
            return false;
    }

    OUTV("true");
    return true;
}

/**
 * @brief SubtitleDecoderElement::ProcessBuffer
 * @param buffer
 */
void SubtitleDecoderElement::ProcessBuffer(AVPacketBufferSPTR buffer)
{
    INV("buffer: %p", buffer.get());
    AVPacket* pkt = buffer->GetAVPacket();

    auto start = static_cast<int>(pkt->pts);
    auto stop = start + static_cast<int>(pkt->duration);

    m_subtitle_codec.Open(m_subsInfo->hints);

    int result = m_subtitle_codec.Decode(pkt->data, pkt->size, 0, 0);
    if(result != OC_OVERLAY)
    {
        LOG_ERROR(LOG_TAG, "an error occured, result: %d", result);
        OUTV();
        return;
    }

    auto overlay = m_subtitle_codec.GetOverlay();
    if(!overlay)
    {
        LOG_ERROR(LOG_TAG, "NULL overlay");
        OUTV();
        return;
    }

    auto e = ((COMXOverlayText*) overlay)->m_pHead;
    if(e && e->IsElementType(COMXOverlayText::ELEMENT_TYPE_TEXT))
    {
        Subtitle subtitle = Subtitle(start,
                                     stop,
                        QString(((COMXOverlayText::CElementText*) e)->m_text));
        m_subs_renderer->setSubsBuffer(subtitle);
    }

    OUTV();
}


/**
 * @brief SubtitleDecoderElement::DoWork
 */
void SubtitleDecoderElement::DoWork()
{
    INV();
    OUTV();
}

/**
 * @brief SubtitleDecoderElement::State_Executing
 */
void SubtitleDecoderElement::State_Executing()
{
    INV();
    LOG_INFO(LOG_TAG,
        "State_Executing Started... SubtitleDecoderElement_Thread in work !");
    OUTV();
}

/**
 * @brief SubtitleDecoderElement::onBufferAvailable
 * @param buffer
 */
void SubtitleDecoderElement::onBufferAvailable(BufferSPTR buffer)
{
    INV("buffer: %p", buffer.get());

    if(!CheckCurrentState())
    {
        LOG_DEBUG(LOG_TAG, "Send back the subs buffer...");
        Q_EMIT sendBackBuffer(buffer);
        return;
    }

    switch (buffer->Type())
    {
        case BufferTypeEnum::Marker:
        {
            MarkerBufferSPTR markerBuffer =
                    std::static_pointer_cast<MarkerBuffer>(buffer);
            switch (markerBuffer->Marker())
            {
                case MarkerEnum::EndOfStream:
                {
                    if(!m_aml_reader->isSeekRequest())
                    {
                        MarkerBufferSPTR eosBuffer =
                                std::make_shared<MarkerBuffer>(
                                    nullptr,
                                    MarkerEnum::EndOfStream);
                        Q_EMIT bufferAvailable(eosBuffer);
                        SetState(Pause);
                    }
                    break;
                }
                case MarkerEnum::Discontinue:
                {
                    m_returnAllBuffersMtx.lock();
                    if(m_returnAllBuffers)
                        m_returnAllBuffers = false;
                    m_returnAllBuffersCmd.wakeAll();
                    m_returnAllBuffersMtx.unlock();
                    break;
                }
                default:
                    // ignore unknown
                    break;
            }
            break;
        }
        case BufferTypeEnum::AVPacket:
        {
            AVPacketBufferSPTR avPacketBuffer =
                    std::static_pointer_cast<AVPacketBuffer>(buffer);
            if (!m_returnAllBuffers &&
                m_aml_reader->IsActive(MediaCategoryEnum::Subtitle,
                                avPacketBuffer->GetAVPacket()->stream_index))
            {
                ProcessBuffer(avPacketBuffer);
            }
            break;
        }
        default:
            // Ignore
            break;
    }
    LOG_DEBUG(LOG_TAG, "Send back the subs buffer...");
    Q_EMIT sendBackBuffer(buffer);
    if (ExecutionState() < ExecutionStateEnum::Idle)
        SetExecutionState(ExecutionStateEnum::Idle);
    OUTV();
}

/**
 * @brief SubtitleDecoderElement::ChangeState
 * @param oldState
 * @param newState
 */
void SubtitleDecoderElement::ChangeState(MediaState oldState,
                                         MediaState newState)
{
    INV("oldState: %u, newState: %u",oldState, newState);
    QMutexLocker locker(&m_wait_state_change);
    AMLComponent::ChangeState(oldState, newState);
    switch (newState)
    {
        case Play:
        case Stop:
        {
            m_wait_state_cmd.wakeAll();
            break;
        }
        case Pause:
        default:
            break;
    }

    OUTV();
}

/**
 * @brief SubtitleDecoderElement::Flush
 */
void SubtitleDecoderElement::Flush()
{
    INV();
    AMLComponent::Flush();
    returnAllBuffers();
    OUTV();
}
