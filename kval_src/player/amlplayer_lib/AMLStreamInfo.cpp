#include "AMLStreamInfo.h"

/**
 * @brief CAMLStreamInfo::CAMLStreamInfo
 */
CAMLStreamInfo::CAMLStreamInfo()
{ 
  extradata = NULL; 
  Clear(); 
}

/**
 * @brief CAMLStreamInfo::~CAMLStreamInfo
 */
CAMLStreamInfo::~CAMLStreamInfo()
{
  //if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;
}

/**
 * @brief CAMLStreamInfo::Clear
 */
void CAMLStreamInfo::Clear()
{
  codec = AV_CODEC_ID_NONE;
  software = false;
  codec_tag  = 0;

  //if( extradata && extrasize ) free(extradata);

  extradata = NULL;
  extrasize = 0;

  fpsscale = 0;
  fpsrate  = 0;
  height   = 0;
  width    = 0;
  aspect   = 0.0;
  vfr      = false;
  stills   = false;
  level    = 0;
  profile  = 0;
  ptsinvalid = false;

  channels   = 0;
  samplerate = 0;
  blockalign = 0;
  bitrate    = 0;
  bitspersample = 0;

  identifier = 0;

  framesize  = 0;
  syncword   = 0;
}
