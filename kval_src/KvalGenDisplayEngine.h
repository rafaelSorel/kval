#ifndef KVALGENDISPLAYENGINE_H
#define KVALGENDISPLAYENGINE_H

#include <QString>
#include <QObject>

#include "KvalDisplayManager.h"

class GenDisplayEngine : public KvalDisplayPlatform::DisplayEngineIf
{
public:
    GenDisplayEngine() = default;
    virtual ~GenDisplayEngine() = default;

    virtual QString getCurrentResolution() final;
    virtual bool setResolution(const QString&) final;
    virtual QStringList getAvailableRes() final;

};
#endif // KVALGENDISPLAYENGINE_H
