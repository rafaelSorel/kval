#ifndef KVAL_MEDIAPLAYERABS_H
#define KVAL_MEDIAPLAYERABS_H
#include <QObject>

#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
#ifdef AMLOGIC_TARGET
#include "KvalAMLPlayerElement.h"
#include "KvalAMLPlayerCore.h"
#include "amlplayer_lib/AMLReader.h"
#else
#include "KvalPlayerManager.h"
#endif
#elif defined(Q_OS_WINDOWS)
#include "kval_desktopmediaplayer.h"
#endif

#endif // KVAL_MEDIAPLAYERABS_H
