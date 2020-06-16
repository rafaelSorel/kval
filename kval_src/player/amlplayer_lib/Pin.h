#pragma once

#include <pthread.h>
#include <functional>
#include <memory>
#include "Event.h"
#include "EventArgs.h"

#include "AMLStreamInfo.h"
#include "Codec.h"
#include "Buffer.h"
#include <memory>

/**
 * @brief The PinDirectionEnum enum
 */
enum class PinDirectionEnum
{
    Out = 0,
    In
};

/**
 * @brief The MediaCategoryEnum enum
 */
enum class MediaCategoryEnum
{
    Unknown = 0,
    Audio,
    Video,
    Subtitle,
    Clock,
    Picture
};

/**
 * @brief The VideoFormatEnum enum
 */
enum class VideoFormatEnum
{
    Unknown = 0,
    Mpeg2,
    Mpeg4V3,
    Mpeg4,
    Avc,
    Avc4k2k,
    VC1,
    Hevc,
    VP9,
    Yuv420,
    NV12,
    NV21
};

///**
// * @brief The AML_SUPPORT_H264_4K2K enum
// */
//enum AML_SUPPORT_H264_4K2K
//{
//  AML_SUPPORT_H264_4K2K_UNINIT = -1,
//  AML_NO_H264_4K2K,
//  AML_HAS_H264_4K2K,
//  AML_HAS_H264_4K2K_SAME_PROFILE
//};

/**
 * @brief The AudioFormatEnum enum
 */
enum class AudioFormatEnum
{
    Unknown = 0,
    Pcm,
    MpegLayer2,
    Mpeg2Layer3,
    Ac3,
    Aac,
    Dts,
    WmaPro,
    DolbyTrueHD,
    EAc3,
    Opus,
    Vorbis,
    PcmDvd,
    Flac,
    PcmS24LE
};

/**
 * @brief The SubtitleFormatEnum enum
 */
enum class SubtitleFormatEnum
{
    Unknown = 0,
    Text,
    SubRip,
    Pgs, //Presentation Graphic Stream
    Dvb,
    DvbTeletext,
    Dvd
};

/**
 * @brief The PictureFormatEnum enum
 */
enum class PictureFormatEnum
{
    Unknown = 0,
    Image
};

/**
 * @brief The PinInfo class
 */
class PinInfo
{
    MediaCategoryEnum category;
    int stream_index;
    int id;

public:

    MediaCategoryEnum Category() const { return category; }
    int Index() const { return stream_index; }
    int Id() const { return id; }

    PinInfo(MediaCategoryEnum category, size_t index, int stream_id)
        : category(category),
          stream_index(index),
          id(stream_id)
    {
    }
    virtual ~PinInfo() {}
    virtual void Flush() {}
};
typedef std::shared_ptr<PinInfo> PinInfoSPTR;


typedef std::vector<unsigned char> ExtraData;
typedef std::shared_ptr<ExtraData> ExtraDataSPTR;

/**
 * @brief The VideoPinInfo class
 */
class VideoPinInfo : public PinInfo
{
public:
    VideoPinInfo(int index, int stream_id)
        : PinInfo(MediaCategoryEnum::Video, index, stream_id),
          stream_index(index),
          id(stream_id)
    {
    }

    VideoFormatEnum Format = VideoFormatEnum::Unknown;

    int stream_index = 0;
    int id = -1;
    int Width = 0;
    int Height = 0;
    double FrameRate = 0;
    ExtraDataSPTR ExtraData;
    bool HasEstimatedPts = false;
    CAMLStreamInfo hints;
};
typedef std::shared_ptr<VideoPinInfo> VideoPinInfoSPTR;

/**
 * @brief The AudioPinInfo class
 */
class AudioPinInfo : public PinInfo
{

public:
    AudioPinInfo(int index, int stream_id)
        : PinInfo(MediaCategoryEnum::Audio, index, stream_id),
          stream_index(index),
          id(stream_id)
    {
    }

    int stream_index = 0;
    int id = -1;
    AudioFormatEnum Format = AudioFormatEnum::Unknown;
    int Channels = 0;
    int SampleRate = 0;
    ExtraDataSPTR ExtraData;
    CAMLStreamInfo hints;
};
typedef std::shared_ptr<AudioPinInfo> AudioPinInfoSPTR;

/**
 * @brief The SubtitlePinInfo class
 */
class SubtitlePinInfo : public PinInfo
{
public:
    SubtitlePinInfo(int index, int stream_id)
        : PinInfo(MediaCategoryEnum::Subtitle, index, stream_id),
          stream_index(index),
          id(stream_id)
    {
    }

    int stream_index = 0;
    int id = -1;
    CAMLStreamInfo hints;
    SubtitleFormatEnum Format = SubtitleFormatEnum::Unknown;
};
typedef std::shared_ptr<SubtitlePinInfo> SubtitlePinInfoSPTR;

/**
 * @brief The PicturePinInfo class
 */
class PicturePinInfo : public PinInfo
{
public:
    PicturePinInfo(int index, int stream_id)
        : PinInfo(MediaCategoryEnum::Picture, index, stream_id)
    {
    }
    PictureFormatEnum Format = PictureFormatEnum::Unknown;
};
typedef std::shared_ptr<PicturePinInfo> PicturePinInfoSPTR;


/**
 * @brief The Pin class
 */
class Pin : public std::enable_shared_from_this<Pin>
{
    PinDirectionEnum direction;
    ElementWPTR owner;
    PinInfoSPTR info;
    std::string name;

protected:
    Pin(PinDirectionEnum direction, ElementWPTR owner, PinInfoSPTR info)
        : Pin(direction, owner, info, "Pin")
    {
    }
    Pin(PinDirectionEnum direction, ElementWPTR owner, PinInfoSPTR info, std::string name)
        : direction(direction), owner(owner), info(info), name(name)
    {
        if (owner.expired())
            throw ArgumentNullException();

        if (!info)
            throw ArgumentNullException();
    }


public:

    PinDirectionEnum Direction() const
    {
        return direction;
    }

    ElementWPTR Owner() const
    {
        return owner;
    }

    PinInfoSPTR Info() const
    {
        return info;
    }

    std::string Name() const
    {
        return name;
    }
    void SetName(std::string value)
    {
        name = value;
    }

    ~Pin() { }

    virtual void Flush()
    {
    }
};


template <typename T> // where T : PinSPTR
/**
 * @brief The PinCollection class
 */
class PinCollection
{
    friend class AMLComponent;
    std::vector<T> pins;


protected:

    void Add(T value)
    {
        if (!value)
            throw ArgumentNullException();

        pins.push_back(value);
    }

    void Clear()
    {
        pins.clear();
    }


public:

    PinCollection()
    {
    }


    int Count() const
    {
        return pins.size();
    }

    T Item(int index)
    {
        if (index < 0 || index >= (int)pins.size())
            throw ArgumentOutOfRangeException();

        return pins[index];
    }

    void Flush()
    {
        for (auto pin : pins)
        {
            pin->Flush();
        }
    }

    T FindFirst(MediaCategoryEnum category)
    {
        for (auto item : pins)
        {
            if (item->Category() == category)
            {
                return item;
            }
        }

        return nullptr;
    }

    T Find(MediaCategoryEnum category, int index)
    {
        // TODO: Disable this check for now until
        // a method to ignore a stream is added.
        // Until then, index=-1 is used.

        //if (index < 0)
        // throw ArgumentOutOfRangeException();

        int count = 0;
        for (auto item : pins)
        {
            if (item->Category() == category)
            {
                if (count == index)
                {
                    return item;
                }
                else
                {
                    ++count;
                }
            }
        }

        return nullptr;
    }
};

/**
 * @brief The OutPinCollection class
 */
class PinInfCollection : public PinCollection<PinInfoSPTR>
{
public:
    PinInfCollection() : PinCollection()
    {
    }
};

