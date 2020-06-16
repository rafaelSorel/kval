#pragma once

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#include <string>
extern "C"
{
#include "libavcodec/avcodec.h"
}
typedef struct _iso639Lang {
    char isoCode[4];
    char langStr[40];
}iso639Lang_t;

class CDemuxStream;

class CAMLStreamInfo
{
public:
  CAMLStreamInfo();

  ~CAMLStreamInfo();

  void Clear(); // clears current information

  enum AVCodecID codec;
  bool software;  //force software decoding


  // VIDEO
  int fpsscale; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int fpsrate;
  int height; // height of the stream reported by the demuxer
  int width; // width of the stream reported by the demuxer
  float aspect; // display aspect as reported by demuxer
  bool vfr; // variable framerate
  bool stills; // there may be odd still frames in video
  int level; // encoder level of the stream reported by the decoder. used to qualify hw decoders.
  int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
  bool ptsinvalid;  // pts cannot be trusted (avi's).
  int orientation; // video orientation in clockwise degrees

  // AUDIO
  int channels;
  int samplerate;
  int bitrate;
  int blockalign;
  int bitspersample;

  // SUBTITLE
  int identifier;

  // CODEC EXTRADATA
  void*        extradata; // extra data for codec to use
  unsigned int extrasize; // size of extra data
  unsigned int codec_tag; // extra identifier hints for decoding

  /* ac3/dts indof */
  unsigned int framesize;
  uint32_t     syncword;

  // Common
  char language[4];
  char languageStr[40];
  std::string name;
  std::string codec_name;
};
