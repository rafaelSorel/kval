#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <QStringList>
#include "AMLReader.h"
#include "AlsaAudioSink.h"
#include "AmlVideoSink.h"
#include "Codec.h"
#include "AMLComponent.h"
#include "OMXOverlayCodecText.h"
#include "Pin.h"

#include "Timer.h"
#include "IClock.h"

/**
 * @brief The Subtitle struct
 */
struct Subtitle
{
    Subtitle(int start, int stop, QString text_lines)
    :   start(start),
        stop(stop),
        text_lines(text_lines)
    {}

    int start;
    int stop;
    QString text_lines;
};


/**
 * @brief The SubtitleQml_renderer class
 */
class SubtitleQml_renderer : public AMLComponent
{
    Q_OBJECT

public:
    SubtitleQml_renderer();
    virtual ~SubtitleQml_renderer();

    bool Open(AmlVideoSinkClock* av_clock);
    void stopExec();

    void setSubsBuffer(Subtitle);
    virtual void Initialize() override;
    virtual void DoWork() override;
    virtual void State_Executing() override;
    virtual void ChangeState(MediaState oldState, MediaState newState) override;
    virtual void Flush() override;
    virtual void Close() override;

    void subs_prepare(QString text_lines);
    void subs_unprepare()
    {prepared_ = false;
    }
    void omxSubs_show_next()
    {
        if (prepared_)
            omxSubs_draw();
    }
    void omxSubs_hide()
    {
        omxSubs_clear();
    }

Q_SIGNALS:
    void showSubs(QString);
    void hideSubs();

private:
    AmlVideoSinkClock* m_av_clock = nullptr;
    void omxSubs_clear();
    void omxSubs_draw();
    bool prepared_;
    QString m_subsLine;
    QMutex subs_buffers_mtx;
    QMutex m_mutexPending;
    QWaitCondition m_waitPendingCommand;
    std::vector<Subtitle> m_subtitle_buffers;
    bool m_visible = true;
    bool m_stop_request = false;
    size_t m_next_index = 0;
};

/**
 * @brief The SubtitleDecoderElement class
 */
class SubtitleDecoderElement : public AMLComponent
{
    Q_OBJECT

public:

    SubtitleDecoderElement();
    virtual ~SubtitleDecoderElement();
    bool Open(size_t,
              SubtitlePinInfo*,
              AmlVideoSinkClock*,
              AMLReader*);
    void Close();

    virtual void Initialize() override;
    virtual void DoWork() override;
    virtual void State_Executing() override;
    virtual void ChangeState(MediaState oldState, MediaState newState) override;
    virtual void Flush() override;
    SubtitleQml_renderer * SubsRenderer()
    {
        return m_subs_renderer;
    }

public Q_SLOTS:
    void onBufferAvailable(BufferSPTR);

Q_SIGNALS:
    void bufferAvailable(BufferSPTR);
    void sendBackBuffer(BufferSPTR);

private:
    AmlVideoSinkClock * m_av_clock = nullptr;
    AMLReader * m_aml_reader = nullptr;
    SubtitlePinInfo * m_subsInfo = nullptr;
    SubtitleQml_renderer * m_subs_renderer = nullptr;
    KvalThread * m_subs_renderer_thread;
    SubtitlePinInfoSPTR inInfo;
    std::vector<Subtitle> m_subtitle_buffers;

    COMXOverlayCodecText m_subtitle_codec;

    AVSubtitle* avSubtitle = nullptr;
    bool isFirstData = true;

    bool isSaaHeaderSent = false;

    bool SetupCodec();
    void ProcessBuffer(AVPacketBufferSPTR buffer);
};
typedef std::shared_ptr<SubtitleDecoderElement> SubtitleDecoderElementSPTR;
