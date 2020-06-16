HEADERS += \
    $$PWD/KvalAppDeployment.h \
    $$PWD/KvalAppElement.h \
    $$PWD/KvalAppEmbedModules.h \
    $$PWD/KvalAppLauncher.h \
    $$PWD/KvalAppProxy.h \
    $$PWD/KvalAppShared.h \
    $$PWD/KvalApplicationManager.h

SOURCES += \
    $$PWD/KvalAppDeployment.cpp \
    $$PWD/KvalAppElement.cpp \
    $$PWD/KvalAppEmbedModules.cpp \
    $$PWD/KvalAppLauncher.cpp \
    $$PWD/KvalAppProxy.cpp \
    $$PWD/KvalApplicationManager.cpp


!include($$PWD/builtin/builtin.pri): error("could not find builtin.pri")
