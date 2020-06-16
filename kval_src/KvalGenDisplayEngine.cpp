#define LOG_ACTIVATED

#include "KvalGenDisplayEngine.h"
#include <QScreen>
#include <QGuiApplication>
#define LOG_SRC SETMANAGER
#include "KvalLogging.h"

/**
 * @brief AmlDisplayEngine::setResolution
 * @param resolution
 */
bool GenDisplayEngine::setResolution(const QString& resolution)
{
    Q_UNUSED(resolution)
    LOG_ERROR(LOG_TAG, "Not Handled in desktop environement");
    return true;
    throw "Not Handled in desktop environement";
}

/**
 * @brief GenDisplayEngine::getCurrentResolution
 * @return
 */
QString GenDisplayEngine::getCurrentResolution()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QString resolution;
    QTextStream _sr(&resolution);
    _sr << screen->geometry().width()
        << "x" << screen->geometry().height() << " " << screen->refreshRate();
    return resolution;
}

/**
 * @brief AmlDisplayEngine::getAvailableRes
 * @return
 */
QStringList GenDisplayEngine::getAvailableRes()
{
    LOG_ERROR(LOG_TAG, "Not Handled in desktop environement");
    return QStringList{"1280x720 60.00", "1920x1080 60.00", "3860x2160 60.00"};
    throw "Not Handled in desktop environement";
}
