#include <stdio.h>
#include <unistd.h>

#include "AMLplatform.h"
#include "AMLReader.h"
#include "KvalMiscUtils.h"
#include "KvalPlayerUtils.h"

#define LOG_ACTIVATED
#define LOG_SRC AMLMEDIAPLAYERELEMT
#include "KvalLogging.h"


QPointer<AMLReader> g_amlReaderRef;

static bool g_abort = false;
static int64_t timeout_start;
static int64_t timeout_default_duration;
static int64_t timeout_duration;

/**
 * @brief CurrentHostFrequency
 * @return
 */
static int64_t CurrentHostFrequency(void)
{
  return( (int64_t)1000000000L );
}

/**
 * @brief CurrentHostCounter
 * @return
 */
static int64_t CurrentHostCounter(void)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return( ((int64_t)now.tv_sec * 1000000000LL) + now.tv_nsec );
}

#define RESET_TIMEOUT(x) do { \
  timeout_start = CurrentHostCounter(); \
  timeout_duration = (x) * timeout_default_duration; \
} while (0)

/**
 * @brief interrupt_cb
 * @param unused
 * @return
 */
static int interrupt_cb(void *unused)
{
  (void)unused;
  int ret = 0;

  if (g_abort)
  {
    LOG_ERROR(LOG_TAG, "interrupt_cb - Told to abort");
    ret = 1;
  }
  else if (timeout_duration &&
           CurrentHostCounter() - timeout_start > timeout_duration)
  {
    LOG_ERROR(LOG_TAG, "interrupt_cb - Timed out");
    ret = 1;
  }
  return ret;
}

int64_t BitstreamStats::m_tmFreq;

/**
 * @brief BitstreamStats::BitstreamStats
 * @param nEstimatedBitrate
 */
BitstreamStats::BitstreamStats(unsigned int nEstimatedBitrate)
{
  m_dBitrate = 0.0;
  m_dMaxBitrate = 0.0;
  m_dMinBitrate = -1.0;

  m_nBitCount = 0;
  m_nEstimatedBitrate = nEstimatedBitrate;
  m_tmStart = 0LL;

  if (m_tmFreq == 0LL)
    m_tmFreq = CurrentHostFrequency();
}

/**
 * @brief BitstreamStats::~BitstreamStats
 */
BitstreamStats::~BitstreamStats()
{
}

/**
 * @brief BitstreamStats::AddSampleBytes
 * @param nBytes
 */
void BitstreamStats::AddSampleBytes(unsigned int nBytes)
{
  AddSampleBits(nBytes*8);
}

/**
 * @brief BitstreamStats::AddSampleBits
 * @param nBits
 */
void BitstreamStats::AddSampleBits(unsigned int nBits)
{
  m_nBitCount += nBits;
  if (m_nBitCount >= m_nEstimatedBitrate)
    CalculateBitrate();
}

/**
 * @brief BitstreamStats::Start
 */
void BitstreamStats::Start()
{
  m_nBitCount = 0;
  m_tmStart = CurrentHostCounter();
}

/**
 * @brief BitstreamStats::CalculateBitrate
 */
void BitstreamStats::CalculateBitrate()
{
  int64_t tmNow;
  tmNow = CurrentHostCounter();

  double elapsed = (double)(tmNow - m_tmStart) / (double)m_tmFreq;
  // only update once every 2 seconds
  if (elapsed >= 2)
  {
    m_dBitrate = (double)m_nBitCount / elapsed;

    if (m_dBitrate > m_dMaxBitrate)
      m_dMaxBitrate = m_dBitrate;

    if (m_dBitrate < m_dMinBitrate || m_dMinBitrate == -1)
      m_dMinBitrate = m_dBitrate;

    Start();
  }
}

/**
 * @brief AMLReader::outPin_BufferReturned
 * @param sender
 * @param args
 */
void AMLReader::onBufferReturned(BufferSPTR buffer)
{
    INV("buffer: %p", buffer.get());

    switch (buffer->Type())
    {
        case BufferTypeEnum::AVPacket:
        {
            // Free the memory allocated to the buffers by libav
            AVPacketBufferPTR avbuffer =
                    std::static_pointer_cast<AVPacketBuffer>(buffer);
            avbuffer->Reset();

            // Reuse the buffer
            availableBuffers.Push(buffer);
            break;
        }

        case BufferTypeEnum::Marker:
        default:
            LOG_INFO(LOG_TAG, "Not handled buffer type: %u", buffer->Type());
            break;
    }
    OUTV();
}

/**
 * @brief AMLReader::~AMLReader
 */
AMLReader::~AMLReader()
{
    INV();
    LOG_INFO(LOG_TAG, "+++++++++++++++ ~AMLReader +++++++++++++++");
    OUTV();
}

/**
 * @brief AudioCodecElement::Close
 */
void AMLReader::Close()
{
    INV();
    LOG_INFO(LOG_TAG, "Terminating m_aml_reader.");

    Terminate();
    WaitForExecutionState(ExecutionStateEnum::Terminated);

//    Q_EMIT finished();
    OUTV();
}

/**
 * @brief AMLReader::GetHints
 * @param type
 * @param hints
 * @return
 */
PinInfo * AMLReader::GetHints(MediaCategoryEnum type)
{
    INV("type: %u", type);

    switch (type)
    {
    case MediaCategoryEnum::Audio:
        if(m_audio_index != -1)
        {
            LOG_INFO(LOG_TAG, "audio StreamList[%d]: %p",
                 m_audio_index,
                 streamList[m_audio_index].get());
            return streamList[m_audio_index].get();
        }
        break;
    case MediaCategoryEnum::Video:
        if(m_video_index != -1)
        {
            LOG_INFO(LOG_TAG, "video StreamList[%d]: %p",
                 m_video_index,
                 streamList[m_video_index].get());
            return streamList[m_video_index].get();
        }
        break;
    case MediaCategoryEnum::Subtitle:
        if(m_subtitle_index != -1)
        {
            LOG_INFO(LOG_TAG, "Subs StreamList[%d]: %p",
                 m_subtitle_index,
                 streamList[m_subtitle_index].get());
            return streamList[m_subtitle_index].get();
        }
        break;
    default:
        break;
    }

    OUTV("nullptr");
    return nullptr;
}

/**
 * @brief AMLReader::GetHintsIndex
 * @param type
 * @param index
 * @return
 */
CAMLStreamInfo * AMLReader::GetHintsIndex(MediaCategoryEnum type,
                                          unsigned int index)
{
    INV("type: %u", type);

    switch(type)
    {
        case MediaCategoryEnum::Audio:
            if((int)index > m_audio_count - 1)
                index = m_audio_count - 1;
            break;
        case MediaCategoryEnum::Video:
        {
            if((int)index > m_video_count - 1)
                index = m_video_count - 1;
            break;
        }
        case MediaCategoryEnum::Subtitle:
            if((int)index > m_subtitle_count)
                index = m_subtitle_count -1;
            break;
        default:
            break;
    }

    CAMLStreamInfo * hints = nullptr;
    for(size_t i = 0; i < streamList.size(); i++)
    {
        if( streamList[i]->Category() == type &&
            streamList[i]->Index() == (int)index)
        {
            switch(streamList[i]->Category())
            {
                case MediaCategoryEnum::Audio:
                {
                    LOG_INFO(LOG_TAG, "audioHints StreamList[%d]: %p", i, streamList[i].get());
                    AudioPinInfo * info = (AudioPinInfo *)streamList[i].get();
                    hints = &info->hints;
                    return hints;
                }
                case MediaCategoryEnum::Video:
                {
                    LOG_INFO(LOG_TAG, "VideoHints StreamList[%d]: %p", i, streamList[i].get());
                    VideoPinInfo * info = (VideoPinInfo *)streamList[i].get();
                    hints = &info->hints;
                    return hints;
                }

                case MediaCategoryEnum::Subtitle:
                {
                    LOG_INFO(LOG_TAG, "SubsHints StreamList[%d]: %p", i, streamList[i].get());
                    SubtitlePinInfo * info = (SubtitlePinInfo *)streamList[i].get();
                    hints = &info->hints;
                    return hints;
                }

                default:
                    break;
            }
        }
    }

    OUTV("hints: %p", hints);
    return hints;
}

/**
 * @brief AMLReader::SetActiveStream
 * @param type
 * @param index
 * @return
 */
bool AMLReader::SetActiveStream(MediaCategoryEnum type, unsigned int index)
{
    INV("type: %u, index: %u", type, index);
    QMutexLocker locker(&m_activeStreamMtx);
    bool ret = false;
    ret = SetActiveStreamInternal(type, index);
    OUTV("%u", ret);
    return ret;
}

/**
 * @brief AMLReader::SetActiveStreamInternal
 * @param type
 * @param index
 * @return
 */
bool AMLReader::SetActiveStreamInternal(MediaCategoryEnum type,
                                        unsigned int index)
{
    INV("type: %u, index: %u", type, index);
    bool ret = false;

    switch(type)
    {
        case MediaCategoryEnum::Audio:
            if((int)index > m_audio_count - 1)
                index = m_audio_count - 1;
            break;
        case MediaCategoryEnum::Video:
        {
            if((int)index > m_video_count - 1)
                index = m_video_count - 1;
            break;
        }
        case MediaCategoryEnum::Subtitle:
            if((int)index > m_subtitle_count)
                index = m_subtitle_count -1;
            break;
        default:
            break;
    }

    for(size_t i = 0; i < streamList.size(); i++)
    {
        if( streamList[i]->Category() == type &&
            streamList[i]->Index() == (int)index)
        {
            switch(streamList[i]->Category())
            {
                case MediaCategoryEnum::Audio:
                {
                    if(m_audio_index != -1)
                    {
                        AudioPinInfo * oldStreamInfo = (AudioPinInfo *)streamList[m_audio_index].get();
                        AudioPinInfo * newStreamInfo = (AudioPinInfo *)streamList[i].get();
                        if( oldStreamInfo &&
                            newStreamInfo &&
                           ( (oldStreamInfo->hints.codec != newStreamInfo->hints.codec) ||
                             (oldStreamInfo->hints.samplerate != newStreamInfo->hints.samplerate) ||
                             (oldStreamInfo->hints.channels != newStreamInfo->hints.channels) ) )
                        {
                            LOG_INFO(LOG_TAG,
                            "========== Audio Codec config has changed notify decoders ==========");
                            m_audio_codec_changed = true;
                        }
                    }
                    m_audio_index = i;
                    LOG_INFO(LOG_TAG, ">>>> audio StreamList[%d]: %p",
                         m_audio_index,
                         streamList[m_audio_index].get());
                    ret = true;
                    break;
                }
                case MediaCategoryEnum::Video:
                    m_video_index = i;
                    LOG_INFO(LOG_TAG, ">>>> video StreamList[%d]: %p",
                         m_video_index,
                         streamList[m_video_index].get());

                    ret = true;
                    break;
                case MediaCategoryEnum::Subtitle:
                    if(m_subtitle_index != -1)
                    {
                        SubtitlePinInfo * oldSubsStreamInfo =
                                (SubtitlePinInfo *)streamList[m_subtitle_index].get();
                        SubtitlePinInfo * newSubsStreamInfo =
                                (SubtitlePinInfo *)streamList[i].get();
                        if( oldSubsStreamInfo &&
                            newSubsStreamInfo &&
                            (oldSubsStreamInfo->hints.codec != newSubsStreamInfo->hints.codec) )
                        {
                            LOG_INFO(LOG_TAG, "Subs Codec has changed notify decoders ...");
                            m_subs_codec_changed = true;
                        }
                    }

                    m_subtitle_index = i;
                    LOG_INFO(LOG_TAG, ">>>> subs StreamList[%d]: %p",
                         m_subtitle_index,
                         streamList[m_subtitle_index].get());

                    ret = true;
                    break;
                default:
                    break;
            }
        }
    }

    if(!ret)
    {
        switch(type)
        {
            case MediaCategoryEnum::Audio:
                m_audio_index = -1;
                break;
            case MediaCategoryEnum::Video:
                m_video_index = -1;
                break;
            case MediaCategoryEnum::Subtitle:
                m_subtitle_index = -1;
                break;
            default:
                break;
        }
    }

    OUTV("ret: %u", ret);
    return ret;
}

/**
 * @brief AMLReader::IsActive
 * @param type
 * @param stream_index
 * @return
 */
bool AMLReader::IsActive(MediaCategoryEnum type, int stream_index)
{
    INV("type: %u, stream_index = %d", type, stream_index);
    switch(type)
    {
        case MediaCategoryEnum::Audio:
            if( (m_audio_index != -1) &&
                (streamList[m_audio_index]->Id() == stream_index ))
            {
                OUTV("true");
                return true;
            }
            break;
        case MediaCategoryEnum::Video:
            if( (m_video_index != -1) &&
                (streamList[m_video_index]->Id() == stream_index ))
            {
                OUTV("true");
                return true;
            }
            break;
        case MediaCategoryEnum::Subtitle:
            if( (m_subtitle_index != -1) &&
                (streamList[m_subtitle_index]->Id() == stream_index ))
            {
                OUTV("true");
                return true;
            }
            break;
        default:
            break;
    }

    OUTV("false");
    return false;
}

/**
 * @brief AMLReader::GetStreamLength
 * @return
 */
int AMLReader::GetStreamLength()
{
    INV();
  if (!m_pFormatContext)
    return 0;

  return (int)(m_pFormatContext->duration / (AV_TIME_BASE / 1000));
}

/**
 * @brief AMLReader::CanSeek
 * @return
 */
bool AMLReader::CanSeek()
{
    INV();
    if(!m_pFormatContext || !m_pFormatContext->pb)
    {
        OUTV("false");
        return false;
    }
    if(m_pFormatContext->pb->seekable == AVIO_SEEKABLE_NORMAL)
    {
        OUTV("true");
        return true;
    }
    OUTV("false");
    return false;
}

/**
 * @brief AMLReader::PrintDictionary
 * @param dictionary
 */
void AMLReader::PrintDictionary(const AVDictionary* dictionary) const
{
    INV("dictionary: %p", dictionary);
    int count = av_dict_count(dictionary);

    AVDictionaryEntry* prevEntry = nullptr;

    for (int i = 0; i < count; ++i)
    {
        AVDictionaryEntry* entry = av_dict_get(dictionary,
                                               "",
                                               prevEntry,
                                               AV_DICT_IGNORE_SUFFIX);
        if (entry != nullptr)
        {
            LOG_INFO(LOG_TAG,"\tkey=%s, value=%s", entry->key, entry->value);
        }
        prevEntry = entry;
    }
    OUTV();
}

/**
 * @brief AMLReader::setInternalHints
 * @param stream
 * @param hints
 * @return
 */
bool AMLReader::setInternalHints(AVStream *stream, CAMLStreamInfo *hints)
{
    INV("stream: %p, hints: %p", stream, hints);
    if(!hints || !stream)
    {
        OUTV("false");
        return false;
    }

    hints->codec_name = GetStreamCodecName(stream);
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,83,0)
    AVDictionaryEntry *langTag = av_dict_get(stream->metadata,
                                           "language",
                                           NULL,
                                           0);
    memset(hints->language, 0x0, sizeof(hints->language));
    if (langTag)
    {
        strncpy(hints->language, langTag->value, 3);
    }
#else
    strcpy(hints->language, stream->language );
#endif

    memset(hints->languageStr, 0x0, sizeof(hints->languageStr));
    if(iso639LangMap.contains(hints->language)){
        strncpy(hints->languageStr,
                iso639LangMap[hints->language].toStdString().c_str(),
                iso639LangMap[hints->language].size());
    }

    hints->codec         = stream->codec->codec_id;
    hints->extradata     = stream->codec->extradata;
    hints->extrasize     = stream->codec->extradata_size;
    hints->channels      = stream->codec->channels;
    hints->samplerate    = stream->codec->sample_rate;
    hints->blockalign    = stream->codec->block_align;
    hints->bitrate       = stream->codec->bit_rate;
    hints->bitspersample = stream->codec->bits_per_coded_sample;
    if(hints->bitspersample == 0)
        hints->bitspersample = 16;

    hints->width         = stream->codec->width;
    hints->height        = stream->codec->height;
    hints->profile       = stream->codec->profile;
    hints->orientation   = 0;

    if(stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        hints->fpsrate       = stream->r_frame_rate.num;
        hints->fpsscale      = stream->r_frame_rate.den;

        if( m_bMatroska &&
            stream->avg_frame_rate.den &&
            stream->avg_frame_rate.num)
        {
          hints->fpsrate      = stream->avg_frame_rate.num;
          hints->fpsscale     = stream->avg_frame_rate.den;
        }
        else if(stream->r_frame_rate.num && stream->r_frame_rate.den)
        {
          hints->fpsrate      = stream->r_frame_rate.num;
          hints->fpsscale     = stream->r_frame_rate.den;
        }
        else
        {
          hints->fpsscale     = 0;
          hints->fpsrate      = 0;
        }

        if (stream->sample_aspect_ratio.num != 0)
        {
            hints->aspect = av_q2d(stream->sample_aspect_ratio) *
                                    stream->codec->width /
                                    stream->codec->height;
        }
        else if (stream->codec->sample_aspect_ratio.num != 0)
        {
            hints->aspect = av_q2d(stream->codec->sample_aspect_ratio) *
                                    stream->codec->width /
                                    stream->codec->height;
        }
        else
        {
            hints->aspect = 0.0f;
        }
        if (m_bAVI && stream->codec->codec_id == AV_CODEC_ID_H264)
        {
            hints->ptsinvalid = true;
        }
        AVDictionaryEntry *rtag =
                av_dict_get(stream->metadata, "rotate", NULL, 0);
        if (rtag)
        {
            hints->orientation = atoi(rtag->value);
        }
    }

  return true;
}

/**
 * @brief AMLReader::GetStreamCodecName
 * @param stream
 * @return
 */
std::string AMLReader::GetStreamCodecName(AVStream *stream)
{
  std::string strStreamName = "";

  if(!stream)
    return strStreamName;

  unsigned int in = stream->codec->codec_tag;
  // FourCC codes are only valid on video streams, audio codecs in AVI/WAV
  // are 2 bytes and audio codecs in transport streams have subtle variation
  // e.g AC-3 instead of ac3
  if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO && in != 0)
  {
    char fourcc[5];
    memcpy(fourcc, &in, 4);
    fourcc[4] = 0;
    // fourccs have to be 4 characters
    if (strlen(fourcc) == 4)
    {
      strStreamName = fourcc;
      return strStreamName;
    }
  }

#ifdef FF_PROFILE_DTS_HD_MA
  /* use profile to determine the DTS type */
  if (stream->codec->codec_id == AV_CODEC_ID_DTS)
  {
    if (stream->codec->profile == FF_PROFILE_DTS_HD_MA)
      strStreamName = "dtshd_ma";
    else if (stream->codec->profile == FF_PROFILE_DTS_HD_HRA)
      strStreamName = "dtshd_hra";
    else
      strStreamName = "dca";
    return strStreamName;
  }
#endif

  AVCodec *codec = avcodec_find_decoder(stream->codec->codec_id);

  if (codec)
    strStreamName = codec->name;

  return strStreamName;
}

/**
 * @brief AMLReader::GetStreamLanguage
 * @param type
 * @param stream_index
 * @return
 */
std::string AMLReader::GetStreamLanguage(MediaCategoryEnum type,
                                         int stream_index)
{
  std::string language = "";

  switch(type)
  {
      case MediaCategoryEnum::Audio:
          if( (m_audio_index != -1) &&
              (streamList[m_audio_index]->Id() == stream_index ))
          {
              AudioPinInfo * info =
                      (AudioPinInfo *)GetHints(MediaCategoryEnum::Audio);
              language = info->hints.language;
              break;
          }
          break;
      case MediaCategoryEnum::Video:
          if( (m_video_index != -1) &&
              (streamList[m_video_index]->Id() == stream_index ))
          {
              VideoPinInfo * info =
                      (VideoPinInfo *)GetHints(MediaCategoryEnum::Video);
              language = info->hints.language;
              break;
          }
          break;
      case MediaCategoryEnum::Subtitle:
          if( (m_subtitle_index != -1) &&
              (streamList[m_subtitle_index]->Id() == stream_index ))
          {
              SubtitlePinInfo * info =
                      (SubtitlePinInfo *)GetHints(MediaCategoryEnum::Subtitle);
              language = info->hints.language;
              break;
          }
          break;
      default:
          break;
  }

  return language;
}

/**
 * @brief AMLReader::SetupPins
 */
void AMLReader::SetupPins()
{
    INV();

    for (size_t i = 0; i < streamList.size(); ++i)
    {
        AVStream* streamPtr = m_pFormatContext->streams[i];
        AVCodecContext* codecCtxPtr = streamPtr->codec;
        AVMediaType mediaType = codecCtxPtr->codec_type;
        AVCodecID codec_id = codecCtxPtr->codec_id;
        ExtraDataSPTR ext = std::make_shared<ExtraData>();

        // Copy codec extra data
        unsigned char* src = codecCtxPtr->extradata;
        int size = codecCtxPtr->extradata_size;

        for (int j = 0; j < size; ++j)
        {
            ext->push_back(src[j]);
        }

        switch (mediaType)
        {
            case AVMEDIA_TYPE_VIDEO:
            {
                VideoPinInfoSPTR video_info =
                        std::make_shared<VideoPinInfo>(m_video_count, i);
                video_info->id = i;
                video_info->FrameRate = av_q2d(streamPtr->avg_frame_rate);
                video_info->Width = codecCtxPtr->width;
                video_info->Height = codecCtxPtr->height;
                video_info->ExtraData = ext;
                if (m_url.compare(m_url.size() - 4, 4, ".avi") == 0)
                {
                    video_info->HasEstimatedPts = true;
                    LOG_INFO(LOG_TAG,
                             "Avi Format info->HasEstimatedPts = true");
                }
                setInternalHints(streamPtr, &video_info->hints);
                streamList[i] = video_info;
                AddInfoPin(video_info);
                m_video_count++;

                switch (codec_id)
                {
                    case AV_CODEC_ID_MPEG2VIDEO:
                        LOG_INFO(LOG_TAG,"stream #%d - VIDEO/MPEG2", i);
                        if (video_info)
                            video_info->Format = VideoFormatEnum::Mpeg2;
                        break;

                    case AV_CODEC_ID_MSMPEG4V3:
                        LOG_INFO(LOG_TAG,"stream #%d - VIDEO/MPEG4V3", i);
                        if (video_info)
                            video_info->Format = VideoFormatEnum::Mpeg4V3;
                        break;

                    case AV_CODEC_ID_MPEG4:
                        LOG_INFO(LOG_TAG,"stream #%d - VIDEO/MPEG4", i);
                        if (video_info)
                            video_info->Format = VideoFormatEnum::Mpeg4;
                        break;

                    case AV_CODEC_ID_H264:
                        LOG_INFO(LOG_TAG,"stream #%d - VIDEO/H264", i);
                        if (video_info)
                            video_info->Format = VideoFormatEnum::Avc;
                        break;

                    case AV_CODEC_ID_HEVC:
                        LOG_INFO(LOG_TAG,"stream #%d - VIDEO/HEVC", i);
                        if (video_info)
                            video_info->Format = VideoFormatEnum::Hevc;
                        break;

                    case AV_CODEC_ID_VC1:
                        LOG_INFO(LOG_TAG,"stream #%d - VIDEO/VC1", i);
                        if (video_info)
                            video_info->Format = VideoFormatEnum::VC1;
                        break;
                    case AV_CODEC_ID_VP9:
                        if(aml_support_vp9())
                        {
                            LOG_INFO(LOG_TAG,"stream #%d - VIDEO/VP9", i);
                            if (video_info)
                                video_info->Format = VideoFormatEnum::VP9;
                            break;
                        }
                    default:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - VIDEO/UNKNOWN(%x)",
                                 i,
                                 codec_id);
                        if (video_info)
                            video_info->Format = VideoFormatEnum::Unknown;
                        break;
                }

                LOG_INFO(LOG_TAG,
                         "w: %d h: %d ",
                         video_info->Width,
                         video_info->Height);

                LOG_INFO(LOG_TAG,
                         "fps: %f(%d/%d) ",
                         video_info->FrameRate,
                         streamPtr->avg_frame_rate.num,
                         streamPtr->avg_frame_rate.den);

                LOG_INFO(LOG_TAG,"SAR=(%d/%d) ",
                    streamPtr->sample_aspect_ratio.num,
                    streamPtr->sample_aspect_ratio.den);
                // TODO: DAR
            }
            break;

            case AVMEDIA_TYPE_AUDIO:
            {
                AudioPinInfoSPTR info =
                        std::make_shared<AudioPinInfo>(m_audio_count, i);
                info->id = i;
                info->Channels = codecCtxPtr->channels;
                info->SampleRate = codecCtxPtr->sample_rate;
                info->Format = AudioFormatEnum::Unknown;
                info->ExtraData = ext;
                LOG_INFO(LOG_TAG,"Audio SampleRate=%d", info->SampleRate);
                setInternalHints(streamPtr, &info->hints);
                streamList[i] = info;
                AddInfoPin(info);
                m_audio_count++;

                switch (codec_id)
                {
                    case AV_CODEC_ID_MP2:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/MP2", i);
                        if (info) info->Format = AudioFormatEnum::MpegLayer2;
                        break;

                    case AV_CODEC_ID_MP3:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/MP3", i);
                        if (info) info->Format = AudioFormatEnum::Mpeg2Layer3;
                        break;

                    case AV_CODEC_ID_AAC:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/AAC", i);
                        if (info) info->Format = AudioFormatEnum::Aac;
                        break;

                    case AV_CODEC_ID_AC3:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/AC3", i);
                        if (info) info->Format = AudioFormatEnum::Ac3;
                        break;

                    case AV_CODEC_ID_DTS:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/DTS", i);
                        if (info) info->Format = AudioFormatEnum::Dts;
                        break;

                    case AV_CODEC_ID_WMAPRO:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/WMAPRO", i);
                        if (info) info->Format = AudioFormatEnum::WmaPro;
                        break;

                    case AV_CODEC_ID_TRUEHD:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/TRUEHD", i);
                        if (info) info->Format = AudioFormatEnum::DolbyTrueHD;
                        break;

                    case AV_CODEC_ID_EAC3:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/EAC3", i);
                        if (info) info->Format = AudioFormatEnum::EAc3;
                        break;

                    case AV_CODEC_ID_OPUS:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/OPUS", i);
                        if (info) info->Format = AudioFormatEnum::Opus;
                        break;

                    case AV_CODEC_ID_VORBIS:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/VORBIS", i);
                        if (info) info->Format = AudioFormatEnum::Vorbis;
                        break;

                    case AV_CODEC_ID_PCM_DVD:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/PCM_DVD", i);
                        if (info) info->Format = AudioFormatEnum::PcmDvd;
                        break;

                    case AV_CODEC_ID_FLAC:
                        LOG_INFO(LOG_TAG,"stream #%d - AUDIO/FLAC", i);
                        if (info) info->Format = AudioFormatEnum::Flac;
                        break;

                    case AV_CODEC_ID_PCM_S24LE:
                        LOG_INFO(LOG_TAG, "stream #%d - AUDIO/PCM_S24LE", i);
                        if (info) info->Format = AudioFormatEnum::PcmS24LE;
                        break;

                    default:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - AUDIO/UNKNOWN (0x%x)",
                                 i,
                                 codec_id);
                        break;
                }
            }
            break;

            case AVMEDIA_TYPE_SUBTITLE:
            {
                SubtitlePinInfoSPTR info =
                        std::make_shared<SubtitlePinInfo>(m_subtitle_count, i);
                setInternalHints(streamPtr, &info->hints);
                info->id = i;
                streamList[i] = info;
                AddInfoPin(info);
                m_subtitle_count++;

                switch (codec_id)
                {
                    case AV_CODEC_ID_SUBRIP:
                        LOG_INFO(LOG_TAG,"stream #%d - SUBTITLE/SUBRIP", i);
                        info->Format = SubtitleFormatEnum::SubRip;
                        break;

                    case  AV_CODEC_ID_HDMV_PGS_SUBTITLE:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - SUBTITLE/HDMV_PGS_SUBTITLE",
                                 i);
                        info->Format = SubtitleFormatEnum::Pgs;
                        break;

                    case  AV_CODEC_ID_DVB_SUBTITLE:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - SUBTITLE/DVB_SUBTITLE",
                                 i);
                        info->Format = SubtitleFormatEnum::Dvb;
                        break;

                    case  AV_CODEC_ID_TEXT:
                        LOG_INFO(LOG_TAG,"stream #%d - SUBTITLE/TEXT", i);
                        info->Format = SubtitleFormatEnum::Text;
                        break;

                    case  AV_CODEC_ID_XSUB:
                        LOG_INFO(LOG_TAG,"stream #%d - TODO SUBTITLE/XSUB", i);
                        break;

                    case  AV_CODEC_ID_SSA:
                        LOG_INFO(LOG_TAG,"stream #%d - TODO SUBTITLE/SSA", i);
                        break;

                    case  AV_CODEC_ID_MOV_TEXT:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - TODO SUBTITLE/MOV_TEXT",
                                 i);
                        break;
                    case  AV_CODEC_ID_DVB_TELETEXT:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - SUBTITLE/DVB_TELETEXT",
                                 i);
                        info->Format = SubtitleFormatEnum::DvbTeletext;
                        break;
                    case  AV_CODEC_ID_SRT:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - TODO SUBTITLE/SRT",
                                 i);
                        break;

                    case  AV_CODEC_ID_DVD_SUBTITLE:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - SUBTITLE/DVD_SUBTITLE\n",
                                 i);
                        info->Format = SubtitleFormatEnum::Dvd;
                        break;

                    default:
                        LOG_INFO(LOG_TAG,
                                 "stream #%d - SUBTITLE/UNKNOWN (0x%x)",
                                 i,
                                 codec_id);
                        break;
                }
            }
            break;

            case AVMEDIA_TYPE_DATA:
            default:
            {
                LOG_INFO(LOG_TAG,
                         "stream #%d - Unknown mediaType (%x)" ,i, mediaType);
                PinInfoSPTR info =
                        std::make_shared<PinInfo>(MediaCategoryEnum::Unknown,
                                                      m_unknow_media_count,
                                                      i);
                streamList[i] = info;
                AddInfoPin(info);
                m_unknow_media_count++;
                break;
            }
        }
    }

    OUTV();
}

/**
 * @brief AMLReader::Chapters
 * @return
 */
const ChapterListSPTR AMLReader::Chapters() const
{
    INV();
    OUTV("chapters: %p", chapters.get());
    return chapters;
}

/**
 * @brief AMLReader::AMLReader
 */
AMLReader::AMLReader() :
    m_pFormatContext(nullptr)
{
    INV();
    m_video_index = -1;
    m_audio_index = -1;
    m_subtitle_index = -1;
    m_video_count = 0;
    m_audio_count = 0;
    m_subtitle_count = 0;
    m_bMatroska = false;
    m_bAVI = false;
    m_stop_request = false;
    m_loopMotionGraphics = false;
    m_audio_codec_changed = false;
    m_subs_codec_changed = false;

    g_amlReaderRef = this;
    OUTV();
}

/**
 * @brief AMLReader::Open
 * @param url
 * @param avOptions
 * @return
 */
bool AMLReader::Open(std::string url, std::string avOptions)
{
    INV("url: %s, avOptions: %s", url.c_str(), avOptions.c_str());
    m_url = url;
    g_abort = false;

    av_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_QUIET);

    //Interreption timeout
    float timeout = 1.0f;
    timeout_default_duration = (int64_t) (timeout * 1e9);

    const AVIOInterruptCB int_cb = { interrupt_cb, this };
    RESET_TIMEOUT(40);

    AVDictionary* options_dict = NULL;

    m_pFormatContext = avformat_alloc_context();
    /*
    Set probing size in bytes, i.e. the size of the data to analyze to get
    stream information. A higher value will enable detecting more information
    in case it is dispersed into the stream, but will increase latency. Must
    be an integer not lesser than 32. It is 5000000 by default.
    */
    //av_dict_set(&options_dict, "probesize", "10000000", 0);

    /*
    Specify how many microseconds are analyzed to probe the input. A higher
    value will enable detecting more accurate information, but will increase
    latency. It defaults to 5,000,000 microseconds = 5 seconds.
    */
    //av_dict_set(&options_dict, "analyzeduration", "50000", 0);

    /*Reconnect on remote stream*/
    av_dict_set(&options_dict, "reconnect", "1", 0);

    if (av_dict_parse_string(&options_dict, avOptions.c_str(), ":", ",", 0))
    {
        LOG_ERROR(LOG_TAG,"Invalid AVDictionary options.");
        av_dict_free(&options_dict);
        Terminating();
        return false;
    }

    // set the interrupt callback, appeared in libavformat 53.15.0
    m_pFormatContext->interrupt_callback = int_cb;

    m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;

    int ret = avformat_open_input(&m_pFormatContext,
                                  m_url.c_str(),
                                  NULL,
                                  &options_dict);
    if (ret < 0)
    {
        LOG_INFO(LOG_TAG,"avformat_open_input failed");
        av_dict_free(&options_dict);
        Terminating();
        return false;
    }

    av_dict_free(&options_dict);

    LOG_INFO(LOG_TAG,"Source Metadata:");
    PrintDictionary(m_pFormatContext->metadata);

    // Chapters
    int chapterCount = m_pFormatContext->nb_chapters;
    LOG_INFO(LOG_TAG,"Chapters (count=%d):", chapterCount);

    AVChapter** avChapters = m_pFormatContext->chapters;
    for (int i = 0; i < chapterCount; ++i)
    {
        AVChapter* avChapter = avChapters[i];

        //int index = i + 1;
        double start =  avChapter->start * avChapter->time_base.num /
                        (double)avChapter->time_base.den;


        Chapter chapter;
        chapter.TimeStamp = start;
        AVDictionary* metadata = avChapter->metadata;
        AVDictionaryEntry* titleEntry = av_dict_get(
            metadata,
            "title",
            NULL,
            0);

        if (titleEntry)
        {
            chapter.Title = std::string(titleEntry->value);
        }

        chapters->push_back(chapter);
    }

    m_bMatroska = strncmp(m_pFormatContext->iformat->name, "matroska", 8) == 0;
    m_bAVI = strcmp(m_pFormatContext->iformat->name, "avi") == 0;

    if(/*m_bAVI || */m_bMatroska)
    {
        m_pFormatContext->max_analyze_duration = 0;
    }

    ret = avformat_find_stream_info(m_pFormatContext, NULL);
    if (ret < 0)
    {
        LOG_ERROR(LOG_TAG, "Could not find stream info !!");
        av_dict_free(&options_dict);
        Terminating();
        OUTV("false");
        return false;
    }

    duration = (int)(m_pFormatContext->duration / (AV_TIME_BASE / 1000));
    LOG_INFO(LOG_TAG, "Duration: %f", duration);

    int streamCount = m_pFormatContext->nb_streams;
    if (streamCount < 1)
    {
        LOG_ERROR(LOG_TAG, "streamCount: %d !!", streamCount);
        av_dict_free(&options_dict);
        Terminating();
        OUTV("false");
        return false;
    }

    for (int i = 0; i < streamCount; ++i)
    {
        streamList.push_back(nullptr);
    }

    // Seems to be a bug in ffmpeg, hls jumps back to start after a couple of seconds
    // this fixes the issue
    if (m_pFormatContext->iformat && strcmp(m_pFormatContext->iformat->name, "hls,applehttp") == 0)
    {
        int flags = AVFMT_SEEK_TO_PTS;
        int64_t seekPts = 0;
        flags |= AVSEEK_FLAG_BACKWARD;

        int ret = av_seek_frame(m_pFormatContext, -1, seekPts, flags);
        if (ret < 0)
        {
            LOG_INFO(LOG_TAG,
                     "av_seek_frame (%f) failed, Seeking is unavailable",
                     0);
        }
    }


    LOG_INFO(LOG_TAG,"Streams (count=%d):", streamCount);

    SetName("AMLReader");
    Execute();
    WaitForExecutionState(ExecutionStateEnum::Idle);

    OUTV("true");
    return true;
}

/**
 * @brief AMLReader::signalDisconnected
 * @param category
 */
void AMLReader::signalDisconnected(MediaCategoryEnum category)
{
    INV("category: %u", category);
    switch(category)
    {
        case MediaCategoryEnum::Video:
            LOG_INFO(LOG_TAG, "Send Video receiver will be disconnected...");
            m_video_discon=true;
            break;
        case MediaCategoryEnum::Audio:
            LOG_INFO(LOG_TAG, "Send audio receiver will be disconnected...");
            m_audio_discon=true;
            break;
        default:
            break;
    }
    OUTV();
}

/**
 * @brief AMLReader::Initialize
 */
void AMLReader::Initialize()
{
    INV();
    SetupPins();

    for (size_t i = 0; i < streamList.size(); ++i)
    {
        streamNextPts.push_back(0);
    }

    // Chapters
    int index = 0;
    for (auto& chapter : *chapters)
    {
        LOG_INFO(LOG_TAG,
                 "Chapter[%02d] = '%s' : %f",
                 index,
                 chapter.Title.c_str(),
                 chapter.TimeStamp);
        ++index;
    }

    // Create buffers
    for (auto i = 0; i < BUFFER_COUNT; ++i)
    {
        AVPacketBufferPtr buffer = std::make_shared<AVPacketBuffer>(nullptr);
        availableBuffers.Push(buffer);
    }
    OUTV();
}

/**
 * @brief AMLReader::ChangeState
 * @param oldState
 * @param newState
 */
void AMLReader::ChangeState(MediaState oldState, MediaState newState)
{
    INV("oldState: %u, newState: %u", oldState, newState);
    QMutexLocker locker(&m_wait_state_change);
    AMLComponent::ChangeState(oldState, newState);
    switch (State())
    {
        case Play:
        case Stop:
            m_wait_state_cmd.wakeAll();
            break;
        case Pause:
        default:
            break;
    }

    OUTV();
}

/**
 * @brief AMLReader::stopExec
 */
void AMLReader::stopExec()
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
            LOG_INFO(LOG_TAG, "Reader not in action %u", ExecutionState());
            return;
    }

    LOG_INFO(LOG_TAG, "m_stop_request requested ....");
    g_abort = true;
    m_stop_request = true;

    if (State() == Pause)
    {
        m_wait_state_change.lock();
        m_wait_state_cmd.wakeAll();
        m_wait_state_change.unlock();
    }

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
 * @brief AMLReader::Terminating
 */
void AMLReader::Terminating()
{
    INV();
    LOG_INFO(LOG_TAG,
             "======= AMLReader (%s) availableBuffers count=%d =======",
             name(),
             availableBuffers.Count());

    for (size_t i = 0; i < (size_t)BUFFER_COUNT; i++)
    {
        BufferSPTR freeBuffer;
        if (availableBuffers.TryPop(&freeBuffer))
        {
            AVPacketBufferPTR buffer =
                    std::static_pointer_cast<AVPacketBuffer>(freeBuffer);
            buffer->Reset();
        }
    }
    availableBuffers.Clear();
    streamList.clear();
    streamNextPts.clear();

    if (m_pFormatContext)
    {
        for (size_t i = 0; i < m_pFormatContext->nb_streams; i++)
        {
            avcodec_close(m_pFormatContext->streams[i]->codec);
        }
        avformat_close_input(&m_pFormatContext);
    }

    avformat_network_deinit();

    m_pFormatContext = nullptr;
    OUTV();
}

/**
 * @brief AMLReader::getCurrentPts
 * @return
 */
int64_t AMLReader::getCurrentPts()
{
    return m_player_pts.load();
}

/**
 * @brief AMLReader::setPvrCb
 * @param cb
 */
void AMLReader::setPvrCb(void* cb)
{
    LOG_INFO(LOG_TAG, "set pvrcb ...");
    cbFlushed = reinterpret_cast<CallBackFlushed>(cb);
}

/**
 * @brief AMLReader::activatePvrTimeout
 */
void AMLReader::activatePvrTimeout()
{
    m_isPvrTimeout = true;
}

/**
 * @brief AMLReader::State_Executing
 */
void AMLReader::State_Executing()
{
    INV();
    LOG_INFO(LOG_TAG,
             "State_Executing Started... AMLReader_Thread in work !");

    bool has_emit_eos = false;
    m_videoStats.Start();
    int buffer_count = 0;
    int buffer_size = 0;
    while(!m_stop_request)
    {
        if(!CheckCurrentState()) break;

        BufferSPTR freeBuffer;
        LOG_DEBUG(LOG_TAG,
                 "MediaElement (%s) DoWork availableBuffers count=%d.",
                 name(),
                 availableBuffers.Count());

        // Process
        if (!availableBuffers.TryPop(&freeBuffer))
        {
            LOG_DEBUG(LOG_TAG, "Wait for Buffers to reuse.");
            usleep(10);
            continue;
        }

        LOG_DEBUG(LOG_TAG, "freeBuffer poped...");
        AVPacketBufferPTR buffer = std::static_pointer_cast<AVPacketBuffer>(freeBuffer);

        RESET_TIMEOUT(10);
        if(m_isPvrTimeout) RESET_TIMEOUT(40);
        int ret = av_read_frame(m_pFormatContext, buffer->GetAVPacket());
        if (ret < 0)
        {
            // Free the memory allocated to the buffers by libav
            buffer->Reset();
            availableBuffers.Push(buffer);

            // End of file
            if (ret == (int)AVERROR_EOF || ret == (int)AVERROR_EXIT)
            {
                if(m_loopMotionGraphics && ret == (int)AVERROR_EOF)
                {
                    ret = av_seek_frame(m_pFormatContext,
                                        -1,
                                        0,
                                        AVFMT_SEEK_TO_PTS | AVSEEK_FLAG_BACKWARD);
                    if (ret >= 0)
                    {
                        LOG_DEBUG(LOG_TAG, "Loop to the beginning");
                        continue;
                    }
                }
                if(!has_emit_eos)
                {
                    LOG_INFO(LOG_TAG, "AVERROR_EOF: %d ...", (int)AVERROR_EOF);
                    LOG_INFO(LOG_TAG, "AVERROR_EXIT: %d ...", (int)AVERROR_EXIT);
                    LOG_INFO(LOG_TAG, "End of stream detected: %d ...", ret);
                    MarkerBufferSPTR eosBuffer =
                        std::make_shared<MarkerBuffer>(nullptr, MarkerEnum::EndOfStream);
                    Q_EMIT bufferAvailable(eosBuffer);
                }
                has_emit_eos = true;
            }
            else if (ret == AVERROR(EINTR) || ret == AVERROR(EAGAIN))
            {
                // timeout, probably no real error, empty packet
                LOG_ERROR(LOG_TAG, "timeout, probably no real error, empty packet: %d", ret);
            }
            else
            {
                LOG_ERROR(LOG_TAG, "Error av_read_frame: %d", ret);

                if (m_pFormatContext)
                {
                    avformat_flush(m_pFormatContext);
                }
            }
        }
        else
        {
            has_emit_eos = false;
            LOG_DEBUG(LOG_TAG, "frame Read...");
            AVPacket* pkt = buffer->GetAVPacket();

            LOG_DEBUG(LOG_TAG, "PLAYER ----- pkt.pts: %ld", pkt->pts);
            AVStream* streamPtr = m_pFormatContext->streams[pkt->stream_index];
            buffer->SetTimeBase(streamPtr->time_base);

            if (pkt->pts != AV_NOPTS_VALUE)
            {
                buffer->SetTimeStamp(av_q2d(streamPtr->time_base) * pkt->pts/*streamPtr->cur_dts*/);
                LOG_DEBUG(LOG_TAG, "PLAYER ----- buffer TS: %f", buffer->TimeStamp());
                streamNextPts[pkt->stream_index] = pkt->pts + pkt->duration;
                // keep the pts for seek flags
                lastPts = pkt->pts;
            }
            else
            {
                buffer->SetTimeStamp(av_q2d(streamPtr->time_base) *
                                            streamNextPts[pkt->stream_index]);
                streamNextPts[pkt->stream_index] += pkt->duration;
            }
            PinInfoSPTR pin_info = streamList[pkt->stream_index];

            LOG_DEBUG(LOG_TAG,
                     "pkt->stream_index: %d, pin_info->id: %d",
                     pkt->stream_index,
                     pin_info->Id());

            if (!pin_info)
            {
                // Free the memory allocated to the buffers by libav
                LOG_DEBUG(LOG_TAG,
                         "Free the memory allocated to the buffers by libav...");
                buffer->Reset();
                availableBuffers.Push(buffer);
                continue;
            }
            switch(pin_info->Category())
            {
                case MediaCategoryEnum::Video:
                    if(!m_video_discon && IsActive(MediaCategoryEnum::Video,
                                               pin_info->Id()))
                    {
                        buffer_count = buffer_count+1;
                        buffer_size = buffer->DataLength();
                        LOG_DEBUG(LOG_TAG,
                                  "PLAYER ----- Video timebase: %f",
                                  av_q2d(streamPtr->time_base));
                        LOG_DEBUG(LOG_TAG, "send video buffer...");
                        LOG_DEBUG(LOG_TAG,
                                 "V::count [%d], size[%d], available buffer [%d]",
                                 buffer_count,
                                 buffer_size,
                                 availableBuffers.Count());
                        Q_EMIT bufferAvailable_V(buffer);
                    }
                    else
                    {
                        LOG_DEBUG(LOG_TAG, "discon dont send video buffer...");
                        buffer->Reset();
                        availableBuffers.Push(buffer);
                    }
                    break;
                case MediaCategoryEnum::Audio:
                    if(!m_audio_discon)
                    {
                        LOG_DEBUG(LOG_TAG, "send audio buffer...");
                        Q_EMIT bufferAvailable_A(buffer);
                    }
                    else
                    {
                        LOG_DEBUG(LOG_TAG, "discon dont send audio buffer...");
                        buffer->Reset();
                        availableBuffers.Push(buffer);
                    }
                    break;
                case MediaCategoryEnum::Subtitle:
                    LOG_DEBUG(LOG_TAG, "send subtitle buffer...");
                    Q_EMIT bufferAvailable_S(buffer);
                    break;

                default:
                    LOG_DEBUG(LOG_TAG, "buffer reset...");
                    buffer->Reset();
                    availableBuffers.Push(buffer);
                    break;
            }
            m_videoStats.AddSampleBytes(pkt->size);
        }
        if (ExecutionState() < ExecutionStateEnum::Idle)
            SetExecutionState(ExecutionStateEnum::Idle);
    }

    SetExecutionState(ExecutionStateEnum::Terminating);

    m_mutexPending.lock();
     LOG_INFO(LOG_TAG, "Wake the Stop command.");
     m_stop_request = false;
     m_waitPendingCommand.wakeAll();
    m_mutexPending.unlock();

    LOG_INFO(LOG_TAG, "End of Loop AMLReader");
    OUTV();
}

/**
 * @brief AMLReader::GetVideoBitrate
 * @return
 */
int AMLReader::GetVideoBitrate()
{
  return (int)m_videoStats.GetBitrate();
}

/**
 * @brief AMLReader::Reset
 * @return
 */
bool AMLReader::Reset()
{
    stopExec();

    Close();

    SetExecutionState(ExecutionStateEnum::WaitingForExecute);
    ChangeState(Stop, Pause);
    g_abort = false;
    m_video_index = -1;
    m_audio_index = -1;
    m_subtitle_index = -1;
    m_video_count = 0;
    m_audio_count = 0;
    m_subtitle_count = 0;
    m_bMatroska = false;
    m_bAVI = false;
    m_stop_request = false;
    m_loopMotionGraphics = false;
    m_audio_codec_changed = false;
    m_subs_codec_changed = false;
    streamList.clear();
    streamNextPts.clear();
    lastPts = 0;
    duration = -1;

    //Interreption timeout
    float timeout = 1.0f;
    timeout_default_duration = (int64_t) (timeout * 1e9);

    av_register_all();
    avformat_network_init();

    const AVIOInterruptCB int_cb = { interrupt_cb, this };
    RESET_TIMEOUT(40);
    AVDictionary* options_dict = NULL;
    m_pFormatContext = avformat_alloc_context();
    av_dict_set(&options_dict, "reconnect", "1", 0);
    // set the interrupt callback, appeared in libavformat 53.15.0
    m_pFormatContext->interrupt_callback = int_cb;
    m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;
    int ret = avformat_open_input(&m_pFormatContext,
                                  m_url.c_str(),
                                  NULL,
                                  &options_dict);
    if (ret < 0)
    {
        LOG_INFO(LOG_TAG,"avformat_open_input failed");
        av_dict_free(&options_dict);
        Terminating();
        return false;
    }

    av_dict_free(&options_dict);

    LOG_INFO(LOG_TAG,"Source Metadata:");
    PrintDictionary(m_pFormatContext->metadata);

    ret = avformat_find_stream_info(m_pFormatContext, NULL);
    if (ret < 0)
    {
        LOG_ERROR(LOG_TAG, "Could not find stream info !!");
        av_dict_free(&options_dict);
        Terminating();
        OUTV("false");
        return false;
    }

    cbFlushed();
    duration = (int)(m_pFormatContext->duration / (AV_TIME_BASE / 1000));
    LOG_INFO(LOG_TAG, "Duration: %f", duration);

    int streamCount = m_pFormatContext->nb_streams;
    if (streamCount < 1)
    {
        LOG_ERROR(LOG_TAG, "streamCount: %d !!", streamCount);
        av_dict_free(&options_dict);
        Terminating();
        OUTV("false");
        return false;
    }

    for (int i = 0; i < streamCount; ++i)
    {
        streamList.push_back(nullptr);
    }


    this->Execute();
    this->WaitForExecutionState(ExecutionStateEnum::Idle);

    SetActiveStream(MediaCategoryEnum::Video, 0);
    SetActiveStream(MediaCategoryEnum::Audio, 0);
    LOG_INFO(LOG_TAG,"Streams (count=%d):", streamCount);
    return true;
}

/**
 * @brief AMLReader::Seek
 * @param timeStamp
 */
void AMLReader::Seek(double timeStamp)
{
    INV("timeStamp: %f", timeStamp);
    if (ExecutionState() != ExecutionStateEnum::Idle)
    {
        LOG_WARNING(LOG_TAG,"Idle State could not seek...");
        return;
    }

    int flags = AVFMT_SEEK_TO_PTS; //AVFMT_SEEK_TO_PTS; //AVSEEK_FLAG_ANY;
    int64_t seekPts = (int64_t)(timeStamp * AV_TIME_BASE);

    flags |= AVSEEK_FLAG_BACKWARD;

    LOG_INFO(LOG_TAG, "Seeeeeekkkkkk");
    RESET_TIMEOUT(4);
    int ret = av_seek_frame(m_pFormatContext, -1, seekPts, flags);
    if (ret < 0)
    {
        LOG_INFO(LOG_TAG,
                 "av_seek_frame (%f) failed, Seeking is unavailable",
                 timeStamp);
        return;
    }

    if(m_audio_codec_changed)
    {
        m_audio_codec_changed = false;
        Q_EMIT audioStreamCodecChanged();
    }

    if(m_subs_codec_changed)
    {
        m_subs_codec_changed = false;
    }

    // Send all Output Pins a Discontinue marker
    MarkerBufferSPTR disconMarker =
            std::make_shared<MarkerBuffer>(nullptr,
                                           MarkerEnum::Discontinue);
    Q_EMIT bufferAvailable(disconMarker);

}
