/**
 *   @file       Buffer.h
 *   @brief      All Used buffer in aml player
 */

#pragma once
#include "Exception.h"


extern "C"
{
#include <libavformat/avformat.h>
}

#include <memory>
#include <vector>

enum class BufferTypeEnum
{
    Unknown = 0,
    Marker,
    ClockData,
    PcmData,
    AVPacket,
    AVFrame,
    Image,
    ImageList
};


class AMLComponent;
typedef std::shared_ptr<AMLComponent> ElementSPTR;

/**
 * @brief The Buffer abstract class
 */
class Buffer
{
public:
    BufferTypeEnum Type() { return type; }
    ElementSPTR Owner() const { return owner; }

    virtual ~Buffer() { }
    // TODO: Remove DataPtr/DataLength
    //      since they only make sense if
    //      the buffer payload is understood
    virtual void* DataPtr() = 0;
    virtual int DataLength() = 0;
    virtual double TimeStamp() = 0;

protected:
    Buffer(BufferTypeEnum type, ElementSPTR owner)
        : type(type), owner(owner){}

private:
    BufferTypeEnum type;
    ElementSPTR owner;
};

typedef std::shared_ptr<Buffer> BufferSPTR;
typedef std::unique_ptr<Buffer> BufferUPTR;
typedef std::weak_ptr<Buffer> BufferWPTR;


/**
 * @brief The MarkerEnum enum
 */
enum class MarkerEnum
{
    Unknown = 0,
    EndOfStream,
    Discontinue,
    Flush,
    Count
};

/**
 * @brief The MarkerBuffer class
 */
class MarkerBuffer: public Buffer
{
public:

    MarkerEnum Marker() const
    {
        return status;
    }

    virtual ~MarkerBuffer() { }
    MarkerBuffer(ElementSPTR owner, MarkerEnum status)
        : Buffer(BufferTypeEnum::Marker, owner),
        status(status)
    {}

    virtual void* DataPtr() override
    {
        return nullptr;
    }

    virtual int DataLength() override
    {
        return 0;
    }

    virtual double TimeStamp() override
    {
        // Time stamps are not significant
        // since the buffer is temporally located
        // in the stream
        return 0;
    }

private:
    MarkerEnum status;
};

typedef std::shared_ptr<MarkerBuffer> MarkerBufferSPTR;


/**
 * @brief The GenericBuffer class
 */
template<class T>
class GenericBuffer : public Buffer
{
public:
    virtual ~GenericBuffer() {}
    virtual T Payload()
    {
        return payload;
    }

protected:
    GenericBuffer(BufferTypeEnum type, ElementSPTR owner, T payload)
        : Buffer(type, owner), payload(payload) { }

private:
    T payload;
};

/**
 * @brief The ClockData class
 */
class ClockData
{
public:
    double TimeStamp = -1;
};

/**
 * @brief The ClockDataBuffer class
 */
class ClockDataBuffer final: public GenericBuffer<ClockData*>
{
public:
    ClockDataBuffer(ElementSPTR owner)
        : GenericBuffer(BufferTypeEnum::ClockData, owner, &clockData)
    {}
    virtual ~ClockDataBuffer() { }

    virtual void* DataPtr() override
    {
        return Payload();
    }
    virtual int DataLength() override
    {
        return sizeof(*Payload());
    }
    virtual double TimeStamp() override
    {
        return clockData.TimeStamp;
    }
    void SetTimeStamp(double value)
    {
        clockData.TimeStamp = value;
    }

private:
    ClockData clockData;
};
typedef std::shared_ptr<ClockDataBuffer> ClockDataBufferSPTR;

/**
 * @brief The PcmFormat enum
 */
enum class PcmFormat
{
    Unknown = 0,
    Int16,
    Int16Planes,
    Int32,
    Int32Planes,
    Float32,
    Float32Planes
};

/**
 * @brief The PcmData struct
 */
struct PcmData
{
    static const int MAX_CHANNELS = 8;
    int Channels = 0;
    PcmFormat Format = PcmFormat::Unknown;
    int Samples = 0;
    void* Channel[MAX_CHANNELS] = { 0 };
    int ChannelSize = 0;
};

/**
 * @brief The PcmDataBuffer class
 */
class PcmDataBuffer final: public GenericBuffer<PcmData*>
{
public:
    PcmDataBuffer(ElementSPTR owner, PcmFormat format, int channels, int samples)
        : GenericBuffer(BufferTypeEnum::PcmData, owner, &pcmData)

    {
        pcmData.Format = (format);
        pcmData.Channels = channels;
        pcmData.Samples = (samples);

        if (IsInterleaved(format))
        {
            int size = samples * GetSampleSize(format) * channels;
            pcmData.Channel[0] = malloc(size);
            pcmData.ChannelSize = size;
        }
        else
        {
            int size = samples * GetSampleSize(format);
            pcmData.ChannelSize = size;
            for (int i = 0; i < channels; ++i)
            {
                pcmData.Channel[i] = malloc(size);
            }
        }
    }
    virtual ~PcmDataBuffer()
    {
        for (int i = 0; i < PcmData::MAX_CHANNELS; ++i)
        {
            free(pcmData.Channel[i]);
        }
    }


    virtual void* DataPtr() override
    {
        return pcmData.Channel;
    }

    virtual int DataLength() override
    {
        return pcmData.Channels * sizeof(void*);
    }

    virtual double TimeStamp() override
    {
        return timeStamp;
    }
    void SetTimeStamp(double value)
    {
        timeStamp = value;
    }

    PcmData* GetPcmData()
    {
        return Payload();
    }

private:
    PcmData pcmData;
    double timeStamp = -1;

    static int GetSampleSize(PcmFormat format)
    {
        int sampleSize{};
        switch (format)
        {
            case PcmFormat::Int16:
            case PcmFormat::Int16Planes:
                sampleSize = 2;
                break;
            case PcmFormat::Int32:
            case PcmFormat::Int32Planes:
            case PcmFormat::Float32:
            case PcmFormat::Float32Planes:
                sampleSize = 4;
                break;
            default:
                throw NotSupportedException();
        }

        return sampleSize;
    }

    static bool IsInterleaved(PcmFormat format)
    {
        int result;

        switch (format)
        {
        case PcmFormat::Int16:
        case PcmFormat::Int32:
        case PcmFormat::Float32:
            result = true;
            break;

        case PcmFormat::Int16Planes:
        case PcmFormat::Int32Planes:
        case PcmFormat::Float32Planes:
            result = false;
            break;

        default:
            throw NotSupportedException();
            break;
        }
        return result;
    }
};
typedef std::shared_ptr<PcmDataBuffer> PcmDataBufferPtr;
typedef std::shared_ptr<PcmDataBuffer> PcmDataBufferSPTR;

/**
 * @brief The AVPacketBuffer class
 */
class AVPacketBuffer final: public GenericBuffer<AVPacket*>
{
    double timeStamp = -1;
    AVRational time_base;

    static AVPacket* CreatePayload()
    {
        AVPacket* avpkt;
        avpkt = (AVPacket*)calloc(1, sizeof(*avpkt));
        av_init_packet(avpkt);
        return avpkt;
    }

public:
    AVPacketBuffer(ElementSPTR owner)
        : GenericBuffer(BufferTypeEnum::AVPacket, owner, CreatePayload())
    {
        time_base.num = 1;
        time_base.den = 1;
    }

    virtual ~AVPacketBuffer()
    {
        AVPacket* avpkt = Payload();
        av_packet_unref(avpkt);
        free(avpkt);
    }

    virtual void* DataPtr() override
    {
        return Payload()->data;
    }
    virtual int DataLength() override
    {
        return Payload()->size;
    }
    virtual double TimeStamp() override
    {
        return timeStamp;
    }
    void SetTimeStamp(double value)
    {
        timeStamp = value;
    }


    // Named GetAVPacket instead of AVPacket
    // to avoid compiler name collisions
    AVPacket* GetAVPacket()
    {
        return Payload();
    }
    void Reset()
    {
        AVPacket* avpkt = Payload();
        av_packet_unref(avpkt);
        av_init_packet(avpkt);
        avpkt->data = NULL;
        avpkt->size = 0;
        time_base.num = 1;
        time_base.den = 1;
        timeStamp = -1;
    }
    AVRational TimeBase()
    {
        return time_base;
    }
    void SetTimeBase(AVRational value)
    {
        time_base = value;
    }
};

typedef std::shared_ptr<AVPacketBuffer> AVPacketBufferPtr;
typedef std::shared_ptr<AVPacketBuffer> AVPacketBufferPTR;
typedef std::shared_ptr<AVPacketBuffer> AVPacketBufferSPTR;

/**
 * @brief The AVFrameBuffer class
 */
class AVFrameBuffer final: public GenericBuffer<AVFrame*>
{
    double timeStamp = -1;
    static AVFrame* CreatePayload()
    {
        AVFrame* frame = av_frame_alloc();
        if (!frame)
        {
            throw Exception("av_frame_alloc failed.\n");
        }

        return frame;
    }

public:
    AVFrameBuffer(ElementSPTR owner)
        : GenericBuffer(BufferTypeEnum::AVFrame, owner, CreatePayload())
    {
    }

    virtual ~AVFrameBuffer()
    {
        AVFrame* frame = Payload();
        av_frame_free(&frame);
    }

    virtual void* DataPtr() override
    {
        return Payload()->data;
    }

    virtual int DataLength() override
    {
        // DataPtr is an array of pointers.
        // DataLength is the bytelength of that array
        // not the actual data elements.
        return AV_NUM_DATA_POINTERS * sizeof(void*);
    }

    virtual double TimeStamp() override
    {
        return timeStamp;
    }
    void SetTimeStamp(double value)
    {
        timeStamp = value;
    }

    // Named GetAVFrame() instead of AVFrame()
    // to avoid compiler name collisions
    AVFrame* GetAVFrame()
    {
        return Payload();
    }
};
typedef std::shared_ptr<AVFrameBuffer> AVFrameBufferPtr;
typedef std::shared_ptr<AVFrameBuffer> AVFrameBufferSPTR;


class Image;
typedef std::shared_ptr<Image> ImageSPTR;

/**
 * @brief The ImageBuffer class
 */
class ImageBuffer final: public GenericBuffer<ImageSPTR>
{
    double timeStamp = -1;
    int x = 0;
    int y = 0;
    double duration = 0;

public:
    int X() const { return x; }
    void SetX(int value) { x = value; }
    int Y() const { return y; }
    void SetY(int value) { y = value; }
    double Duration() const { return duration; }
    void SetDuration(double value){duration = value; }

    ImageBuffer(ElementSPTR owner, ImageSPTR image)
        : GenericBuffer(BufferTypeEnum::Image, owner, image)
    {
        if (!image)
            throw ArgumentNullException();
    }
    virtual ~ImageBuffer() { }

    virtual void* DataPtr() override
    {
        // TODO: Remote this API point
        return nullptr;
    }

    virtual int DataLength() override
    {
        // TODO: Remote this API point
        return 0;
    }

    virtual double TimeStamp() override
    {
        return timeStamp;
    }
    void SetTimeStamp(double value)
    {
        timeStamp = value;
    }
};
typedef std::shared_ptr<ImageBuffer> ImageBufferSPTR;
typedef std::vector<ImageBufferSPTR> ImageList;
typedef std::shared_ptr<ImageList> ImageListSPTR;

/**
 * @brief The ImageListBuffer class
 */
class ImageListBuffer final: public GenericBuffer<ImageListSPTR>
{
    double timeStamp = -1;

public:
    ImageListBuffer(ElementSPTR owner, ImageListSPTR imageList)
        : GenericBuffer(BufferTypeEnum::ImageList, owner, imageList),
          timeStamp{-1}
    {
        if (!imageList)
            throw ArgumentNullException();
    }
    virtual ~ImageListBuffer() { }

    virtual void* DataPtr() override
    {
        // TODO: Remote this API point
        return nullptr;
    }

    virtual int DataLength() override
    {
        // TODO: Remote this API point
        return 0;
    }

    virtual double TimeStamp() override
    {
        return timeStamp;
    }
    void SetTimeStamp(double value)
    {
        timeStamp = value;
    }
};
typedef std::shared_ptr<ImageListBuffer> ImageListBufferSPTR;
