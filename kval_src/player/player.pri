
HEADERS += \
    kval_src/player/KvalAppPlayerWrapper.h \
    kval_src/player/KvalHttpStreamManager.h \
    kval_src/player/KvalLiveMediaPvr.h \
    kval_src/player/KvalLiveStreamManager.h \
    kval_src/player/KvalPlayerManager.h \
    kval_src/player/KvalPlayerMetadata.h \
    kval_src/player/KvalPlayerPlatform.h \
    kval_src/player/KvalPlayerUtils.h \
    kval_src/player/quicksubtitle.h


unix:!macx{
    message(===== Add player unix Headers =====)
    HEADERS += kval_src/player/KvalRtmpStreamManager.h

    contains(DEFINES, AMLOGIC_TARGET){
        message(===== Add player AMLOGIC_TARGET Headers =====)

        HEADERS += \
            kval_src/player/KvalAmlPlayerCore.h \
            kval_src/player/KvalAmlPlayerElement.h

        HEADERS += \
            kval_src/player/amlplayer_lib/utils/BitstreamConverter.h \
            kval_src/player/amlplayer_lib/utils/BitstreamReader.h \
            kval_src/player/amlplayer_lib/utils/BitstreamWriter.h \
            kval_src/player/amlplayer_lib/Rectangle.h \
            kval_src/player/amlplayer_lib/AlsaAudioSink.h \
            kval_src/player/amlplayer_lib/AmlCodec.h \
            kval_src/player/amlplayer_lib/AmlVideoSink.h \
            kval_src/player/amlplayer_lib/AudioCodec.h \
            kval_src/player/amlplayer_lib/Buffer.h \
            kval_src/player/amlplayer_lib/Codec.h \
            kval_src/player/amlplayer_lib/AMLComponent.h \
            kval_src/player/amlplayer_lib/Event.h \
            kval_src/player/amlplayer_lib/EventArgs.h \
            kval_src/player/amlplayer_lib/EventListener.h \
            kval_src/player/amlplayer_lib/Exception.h \
            kval_src/player/amlplayer_lib/IClock.h \
            kval_src/player/amlplayer_lib/Image.h \
            kval_src/player/amlplayer_lib/LockedQueue.h \
            kval_src/player/amlplayer_lib/AMLReader.h \
            kval_src/player/amlplayer_lib/Mutex.h \
            kval_src/player/amlplayer_lib/NullSinkElement.h \
            kval_src/player/amlplayer_lib/Pin.h \
            kval_src/player/amlplayer_lib/AMLStreamInfo.h \
            kval_src/player/amlplayer_lib/SubtitleCodecElement.h \
            kval_src/player/amlplayer_lib/OMXOverlayCodec.h \
            kval_src/player/amlplayer_lib/OMXOverlayCodecText.h \
            kval_src/player/amlplayer_lib/OMXOverlay.h \
            kval_src/player/amlplayer_lib/OMXOverlayText.h \
            kval_src/player/amlplayer_lib/OMXSubtitleTagSami.h \
            kval_src/player/amlplayer_lib/PlatformDefs.h \
            kval_src/player/amlplayer_lib/system.h \
            kval_src/player/amlplayer_lib/RegExp.h \
            kval_src/player/amlplayer_lib/StdString.h \
            kval_src/player/amlplayer_lib/Timer.h \
            kval_src/player/amlplayer_lib/WaitCondition.h \
            kval_src/player/amlplayer_lib/AMLplatform.h

    } #AMLOGIC_TARGET
}

SOURCES += \
    kval_src/player/KvalAppPlayerWrapper.cpp \
    kval_src/player/KvalHttpStreamManager.cpp \
    kval_src/player/KvalLiveMediaPvr.cpp \
    kval_src/player/KvalLiveStreamManager.cpp \
    kval_src/player/KvalPlayerManager.cpp \
    kval_src/player/KvalPlayerMetadata.cpp \
    kval_src/player/KvalPlayerUtils.cpp \
    kval_src/player/quicksubtitle.cpp

unix:!macx{
    message(===== Add player unix Sources =====)

    SOURCES += kval_src/player/KvalRtmpStreamManager.cpp

    contains(DEFINES, AMLOGIC_TARGET){
        message(===== Add player AMLOGIC_TARGET Sources =====)
        SOURCES += \
            kval_src/player/KvalAmlPlayerCore.cpp \
            kval_src/player/KvalAmlPlayerElement.cpp

        SOURCES += \
            kval_src/player/amlplayer_lib/utils/BitstreamConverter.cpp \
            kval_src/player/amlplayer_lib/utils/BitstreamReader.cpp \
            kval_src/player/amlplayer_lib/utils/BitstreamWriter.cpp \
            kval_src/player/amlplayer_lib/AlsaAudioSink.cpp \
            kval_src/player/amlplayer_lib/AmlCodec.cpp \
            kval_src/player/amlplayer_lib/AmlVideoSink.cpp \
            kval_src/player/amlplayer_lib/AudioCodec.cpp \
            kval_src/player/amlplayer_lib/Buffer.cpp \
            kval_src/player/amlplayer_lib/Codec.cpp \
            kval_src/player/amlplayer_lib/AMLComponent.cpp \
            kval_src/player/amlplayer_lib/Exception.cpp \
            kval_src/player/amlplayer_lib/Image.cpp \
            kval_src/player/amlplayer_lib/LockedQueue.cpp \
            kval_src/player/amlplayer_lib/AMLReader.cpp \
            kval_src/player/amlplayer_lib/Mutex.cpp \
            kval_src/player/amlplayer_lib/Pin.cpp \
            kval_src/player/amlplayer_lib/AMLStreamInfo.cpp \
            kval_src/player/amlplayer_lib/SubtitleCodecElement.cpp \
            kval_src/player/amlplayer_lib/OMXOverlayCodecText.cpp \
            kval_src/player/amlplayer_lib/OMXSubtitleTagSami.cpp \
            kval_src/player/amlplayer_lib/RegExp.cpp \
            kval_src/player/amlplayer_lib/AMLplatform.cpp
    }
}
