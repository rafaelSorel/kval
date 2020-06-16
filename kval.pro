TARGET = kval

VERSION = 1.0.0

QT += core core-private gui gui-private opengl quick quick-private quickcontrols2 xml dbus network widgets
QT += av

CONFIG += no_keywords
HOME = $$system(echo $HOME)

QMAKE_CFLAGS_ISYSTEM += -I
QMAKE_CFLAGS_WARN_ON += -Wno-class-memaccess
QMAKE_CXXFLAGS_WARN_ON += -Wno-class-memaccess
QMAKE_CFLAGS += -Wno-class-memaccess
QMAKE_CXXFLAGS += -Wno-class-memaccess
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Configure the use of amlogic or desktop version
#DEFINES += AMLOGIC_TARGET
DEFINES += ENABLE_LOGS
# Macro definitions logging level

contains(DEFINES, ENABLE_LOGS) {
#DEFINES += BUILD_LOG_LEVEL_DEBUG
DEFINES += BUILD_LOG_LEVEL_INFORMATION
#DEFINES += BUILD_LOG_LEVEL_ERROR
#DEFINES += BUILD_LOG_LEVEL_CRITICAL
}

contains(DEFINES, AMLOGIC_TARGET) {
QMAKE_CXXFLAGS += -std=c++11 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS             \
   -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE               \
   -D_FILE_OFFSET_BITS=64 -DHAVE_CMAKE_CONFIG                                           \
   -Wno-deprecated-declarations -Wno-missing-field-initializers -Wno-ignored-qualifiers \
   -Wno-psabi -Wno-unused-parameter

QMAKE_LFLAGS += -O -s -flto

#Active for debugging symbols and gdb sigv debugging
#Pleace note that you need to deactivate all the -s -O -Os that strips symbols
#from main application
#QMAKE_LFLAGS += -g

DEFINES += AML_HARDWARE_DECODER
# This enable use of remote controle throught lirc driver interface.
DEFINES += ENABLE_LIRC_SYSTEM_CONTROLE

LIBS += -llirc_client
DEFINES += HAS_CONNMAN
INCLUDEPATH += $$[QT_SYSROOT]/include
INCLUDEPATH += $$[QT_SYSROOT]/usr/local/qtmxq/include/connman-qt5

#For secure
LIBS += $$[QT_SYSROOT]/usr/lib/libcryptopp.a
#LIBS += -lcryptopp
INCLUDEPATH += $$[QT_SYSROOT]/usr/include/cryptopp

# External
LIBS += -lrt -lEGL -lGLESv2 -lpcre
LIBS += -lrtmp -ludev -liw
LIBS += -lavformat -lavcodec -lavutil -lpthread -lasound -lswresample
LIBS += -lconnman-qt5 -lpthread
}
else {
QMAKE_CXXFLAGS += -std=c++11 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS             \
   -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE               \
   -D_FILE_OFFSET_BITS=64 -DHAVE_CMAKE_CONFIG                                           \
   -Wno-deprecated-declarations -Wno-missing-field-initializers -Wno-ignored-qualifiers \
   -Wno-psabi -Wno-unused-parameter

INCLUDEPATH += /usr/include
INCLUDEPATH += $$_PRO_FILE_PWD_/3rdparty/ffmpeg_3.2_x86-64/include
INCLUDEPATH += $$_PRO_FILE_PWD_/kval_src

#For secure
#LIBS += /usr/local/lib/libcryptopp.a
#LIBS += -lcryptopp
#INCLUDEPATH += $$[QT_SYSROOT]/usr/local/include/cryptopp

#QMAKE_LFLAGS += -O -s
QMAKE_LFLAGS += -g -O0 -da -Q

LIBS += -L$$_PRO_FILE_PWD_/3rdparty/ffmpeg_3.2_x86-64/lib -lavformat -lavcodec -lavutil  -lswresample
LIBS += -lpthread -lasound
}

unix:!macx{
message(===== Add unix dynalibs =====)
LIBS += -lrtmp -ludev -liw -lpcre
LIBS += -lrt -lfdk-aac
LIBS += -L$$HOME/Qt5.14.0/5.14.0/gcc_64/lib -lQtAV

# Python Integrated interpretor
LIBS += -L/usr/lib/python3.7/config-3.7m-x86_64-linux-gnu -lpython3.7
INCLUDEPATH += $$_PRO_FILE_PWD_/3rdparty/
}

HEADERS = \
    kval_src/KvalConfigManager.h \
    kval_src/KvalDesktopNetEngine.h \
    kval_src/KvalDisplayManager.h \
    kval_src/KvalEpgEngine.h \
    kval_src/KvalGenDisplayEngine.h \
    kval_src/KvalGuiUtils.h \
    kval_src/KvalLiveTvManager.h \
    kval_src/KvalLocalBrowseManager.h \
    kval_src/KvalLogging.h \
    kval_src/KvalMiscUtils.h \
    kval_src/KvalNetworkManager.h \
    kval_src/KvalResourceClientFetcher.h \
    kval_src/KvalSettingsKeys.h \
    kval_src/KvalSettingsManager.h \
    kval_src/KvalSettingsPlatform.h \
    kval_src/KvalThreadUtils.h \
    kval_src/KvalTimeZoneManager.h \
    kval_src/KvalVodManager.h


SOURCES = \
    kval_src/KvalConfigManager.cpp \
    kval_src/KvalDesktopNetEngine.cpp \
    kval_src/KvalDisplayManager.cpp \
    kval_src/KvalEpgEngine.cpp \
    kval_src/KvalGenDisplayEngine.cpp \
    kval_src/KvalGuiUtils.cpp \
    kval_src/KvalLiveTvManager.cpp \
    kval_src/KvalLocalBrowseManager.cpp \
    kval_src/KvalMiscUtils.cpp \
    kval_src/KvalNetworkManager.cpp \
    kval_src/KvalResourceClientFetcher.cpp \
    kval_src/KvalSettingsManager.cpp \
    kval_src/KvalTimeZoneManager.cpp \
    kval_src/KvalVodManager.cpp \
    kval_src/kval.cpp


unix:!macx{
message(===== Add unix Headers =====)
contains(DEFINES, AMLOGIC_TARGET){
message(===== Add AMLOGIC_TARGET Headers =====)
HEADERS += kval_src/KvalConnManNetEngine.h \
           kval_src/KvalAmlDisplayEngine.h

SOURCES += kval_src/KvalConnManNetEngine.cpp \
           kval_src/KvalAmlDisplayEngine.cpp
} #AMLOGIC_TARGET
}

RESOURCES += kval.qrc


target.path = $$[QT_INSTALL_EXAMPLES]/quick/models/abstractitemmodel
INSTALLS += target


target.path = /home/pi

TARGET = kval

for(deploymentfolder, DEPLOYMENTFOLDERS) {
    item = item$${deploymentfolder}
    itemsources = $${item}.sources
    $$itemsources = $$eval($${deploymentfolder}.source)
    itempath = $${item}.path
    $$itempath= $$eval($${deploymentfolder}.target)
    export($$itemsources)
    export($$itempath)
    DEPLOYMENT += $$item
}

installPrefix = /home/pi/$${TARGET}

for(deploymentfolder, DEPLOYMENTFOLDERS) {
    item = item$${deploymentfolder}
    itemfiles = $${item}.files
    $$itemfiles = $$eval($${deploymentfolder}.source)
    itempath = $${item}.path
    $$itempath = $${installPrefix}/$$eval($${deploymentfolder}.target)
    export($$itemfiles)
    export($$itempath)
    INSTALLS += $$item
}

TRANSLATIONS = languages/kval_fr-FR.ts \
               languages/kval_en-US.ts

DISTFILES += \
       kval_gui/KvalUI_AmlMainView.qml \
       kval_gui/KvalUI_GenMainView.qml \
       kval_gui/KvalUI_platform.qml \
       kval_gui/KvalUI_EpgGenerator.qml \
       kval_gui/KvalUI_MainScreenView.qml \
       kval_gui/KvalUI_ChannelSelectListView.qml \
       kval_gui/KvalUI_MessagePopUp.qml \
       kval_gui/KvalUI_channelInfoView.qml \
       kval_gui/KvalUI_StreamInfoMetaData.qml \
       kval_gui/KvalUI_PyNotificationView.qml \
       kval_gui/KvalUI_VodView.qml \
       kval_gui/KvalUI_VodLaunchView.qml \
       kval_gui/KvalUI_MediaPlayerInfoView.qml \
       kval_gui/KvalUI_SubsSurfaceView.qml \
       kval_gui/KvalUI_VirtualKeyBoard.qml \
       kval_gui/KvalUI_BootScreen.qml \
       kval_gui/KvalUI_ServerNotificationCenter.qml \
       kval_gui/KvalUI_MailBoxView.qml \
       kval_gui/KvalUI_AppsView.qml \
       kval_gui/KvalUI_SettingsMainView.qml \
       kval_gui/KvalUI_FileMediaBrowser.qml \
       kval_gui/mainscreenitems/AppsTemplate.qml \
       kval_gui/mainscreenitems/DevicesTemplate.qml \
       kval_gui/mainscreenitems/LiveTvTemplate.qml \
       kval_gui/mainscreenitems/MessagesTemplate.qml \
       kval_gui/mainscreenitems/SearchTemplate.qml \
       kval_gui/mainscreenitems/SettingsTemplate.qml \
       kval_gui/mainscreenitems/VodTemplate.qml \
       kval_gui/appstemplates/VideoTemplate.qml \
       kval_gui/appstemplates/ListMode.qml \
       kval_gui/appstemplates/GridMode.qml \
       kval_gui/appstemplates/navigation_history.js \
       kval_gui/KvalUI_backgroundImagesDb.js \
       kval_gui/KvalUI_constant.js \
       kval_gui/KvalUI_Utils.js \
       kval_gui/KvalUI_dataloader.js

!include($$_PRO_FILE_PWD_/kval_src/player/player.pri): error("could not find player.pri")
!include($$_PRO_FILE_PWD_/kval_src/appmanager/appmanager.pri): error("could not find appmanager.pri")
!include($$_PRO_FILE_PWD_/kval_src/app/app.pri): error("could not find app.pri")
