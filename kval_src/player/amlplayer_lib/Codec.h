#pragma once

#include <pthread.h>
#include <queue>
#include <memory>

#include "Exception.h"
#include "LockedQueue.h"
#include "Buffer.h"

enum MediaState
{
    Pause = 0,
    Play,
    Stop
};

// AMSTREAM_IOC_SET_SCREEN_MODE
enum AspectRatio
{
    VIDEO_WIDEOPTION_NORMAL = 0,
    VIDEO_WIDEOPTION_FULL_STRETCH = 1,
    VIDEO_WIDEOPTION_4_3 = 2,
    VIDEO_WIDEOPTION_16_9 = 3,
    VIDEO_WIDEOPTION_NONLINEAR = 4,
    VIDEO_WIDEOPTION_NORMAL_NOSCALEUP = 5,
    VIDEO_WIDEOPTION_4_3_IGNORE = 6,
    VIDEO_WIDEOPTION_4_3_LETTER_BOX = 7,
    VIDEO_WIDEOPTION_4_3_PAN_SCAN = 8,
    VIDEO_WIDEOPTION_4_3_COMBINED = 9,
    VIDEO_WIDEOPTION_16_9_IGNORE = 10,
    VIDEO_WIDEOPTION_16_9_LETTER_BOX = 11,
    VIDEO_WIDEOPTION_16_9_PAN_SCAN = 12,
    VIDEO_WIDEOPTION_16_9_COMBINED = 13,
    VIDEO_WIDEOPTION_MAX = 14
};

// Forward Declarations
class AMLComponent;
typedef std::shared_ptr<AMLComponent> ElementSPTR;
typedef std::weak_ptr<AMLComponent> ElementWPTR;
typedef AMLComponent* ElementPTR;

class PinInfo;
typedef std::shared_ptr<PinInfo> PinInfoSPTR;

class Pin;
typedef std::shared_ptr<Pin> PinSPTR;

class InPin;
typedef std::shared_ptr<InPin> InPinSPTR;

class OutPin;
typedef std::shared_ptr<OutPin> OutPinSPTR;


