#define LOG_ACTIVATED

#if defined(AML_TARGET)
#include "KvalAmlDisplayEngine.h"
#else
#include "KvalGenDisplayEngine.h"
#endif

#include "KvalDisplayManager.h"

#define LOG_SRC SETMANAGER
#include "KvalLogging.h"

#define KVAL_DISPLAYMANAGER_MAX_TIMEOUT     20U

namespace KvalDisplayPlatform {

/**
 * @brief ResolutionInfo::ResolutionInfo
 * @param width
 * @param height
 * @param aspect
 * @param mode
 */
ResolutionInfo::ResolutionInfo(
        int width,
        int height,
        float aspect,
        const std::string &mode) :
  strMode(mode)
{
    iWidth = width;
    iHeight = height;
    iScreenWidth = width;
    iScreenHeight = height;
    fPixelRatio = aspect ? ((float)width)/height / aspect : 1.0f;
    fRefreshRate = 0;
    dwFlags = 0;
}

/**
 * @brief CM_ResolutionInfo::printRes
 * @param res
 */
void ResolutionInfo::printRes(const ResolutionInfo& res)
{
    LOG_INFO(LOG_TAG, "iWidth: %d", res.iWidth);
    LOG_INFO(LOG_TAG, "iHeight: %d", res.iHeight);
    LOG_INFO(LOG_TAG, "iScreenWidth: %d", res.iScreenWidth);
    LOG_INFO(LOG_TAG, "iScreenHeight: %d", res.iScreenHeight);
    LOG_INFO(LOG_TAG, "dwFlags: %u", res.dwFlags);
    LOG_INFO(LOG_TAG, "fPixelRatio: %f", res.fPixelRatio);
    LOG_INFO(LOG_TAG, "fRefreshRate: %f", res.fRefreshRate);
    LOG_INFO(LOG_TAG, "strMode: %s", res.strMode.c_str());
    LOG_INFO(LOG_TAG, "strOutput: %s", res.strOutput.c_str());
    LOG_INFO(LOG_TAG, "strId: %s", res.strId.c_str());
}

/**
 * @brief CM_ResolutionInfo::DisplayRatio
 * @return
 */
float ResolutionInfo::DisplayRatio() const
{
  return iWidth * fPixelRatio / iHeight;
}

/**
 * @brief DisplayManager::DisplayManager
 * @param parent
 */
DisplayManager::DisplayManager()
{
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()), Qt::QueuedConnection);
    connect(this, SIGNAL(stoptimer()), &m_timer, SLOT(stop()), Qt::QueuedConnection);
    connect(this, SIGNAL(starttimer(int)), &m_timer, SLOT(start(int)), Qt::QueuedConnection);
    m_timer.setSingleShot(false);

    registerDisplayEngines();
    if(m_displayEngine.isNull()){
        LOG_INFO(LOG_TAG, "Display not handled.");
    }
}

/**
 * @brief DisplayManager::resolutions
 * @return
 */
QStringList DisplayManager::resolutions()
{
    QStringList rvs{};
    if(m_displayEngine.isNull()){
        LOG_ERROR(LOG_TAG, "display not handled.");
        return rvs;
    }

    try {
        return m_displayEngine->getAvailableRes();
    } catch (const char* exception) {
        LOG_WARNING(LOG_TAG, "%s", exception);
    }

    return rvs;
}

/**
 * @brief DisplayManager::getCurrentResolution
 * @return
 */
QString DisplayManager::getCurrentResolution()
{
    try {
        return m_displayEngine->getCurrentResolution();
    } catch (const char* exception) {
        LOG_WARNING(LOG_TAG, "%s", exception);
        return "";
    }
}
/**
 * @brief DisplayManager::setResolution
 * @param res
 * @return
 */
bool DisplayManager::setResolution(const QString& res)
{
    try {
        return m_displayEngine->setResolution(res);
    } catch (const char* exception) {
        LOG_WARNING(LOG_TAG, "%s", exception);
    }
    return false;
}

/**
 * @brief DisplayManager::registerDisplayEngines
 */
bool DisplayManager::updateScreenRes(const QString& res)
{
    if(m_displayEngine.isNull()){
        LOG_ERROR(LOG_TAG, "Display not handled.");
        return false;
    }

    try {
        m_currentRes = m_displayEngine->getCurrentResolution();
    } catch (const char* exception) {
        LOG_WARNING(LOG_TAG, "%s", exception);
        return false;
    }


    bool status = setResolution(res);
    if(!status) {
        Q_EMIT displayMsg("Error" , tr("Résolution non supporté !"));
    }
    else {
        m_newRes = res;
        m_countdown = 0;
        LOG_INFO(LOG_TAG, "Start timer");
        Q_EMIT starttimer(1000);
        Q_EMIT yesNoDiag(tr("Does the display look OK ?"),
           tr("The display will be reset to its previous "
              "configuration<br>in %n seconds.<br>Keep this configuration ?", "",
              KVAL_DISPLAYMANAGER_MAX_TIMEOUT),
           tr("No"),
           tr("Yes"));
    }
    return status;
}

/**
 * @brief DisplayManager::onTimeout
 */
void DisplayManager::onTimeout()
{
    qDebug() << "m_countdown: " << m_countdown;
    if(++m_countdown >= KVAL_DISPLAYMANAGER_MAX_TIMEOUT) {
        Q_EMIT stoptimer();
        Q_EMIT yesNoDiagClose();
        m_countdown = 0;
        m_newRes.clear();
        try {
            m_displayEngine->setResolution(m_currentRes);
        } catch (const char* exception) {
            LOG_WARNING(LOG_TAG, "%s", exception);
        }
    }
    else {
        Q_EMIT yesNoDiagUpdate(tr("Does the display look OK ?"),
           tr("The display will be reset to its previous configuration"
              "<br>in %n seconds.<br>Keep this configuration ?", "",
              KVAL_DISPLAYMANAGER_MAX_TIMEOUT-m_countdown));
    }
}

/**
 * @brief DisplayManager::userReply
 * @param reply
 */
void DisplayManager::onUserReply(
        const QString& group,
        const QString& key,
        const QVariant& reply)
{
    qInfo() << ">>>>>>>>>>> onUserReply";
    if(key != KVALSETTING_KEY_SCREENRES)
    {
        LOG_INFO(LOG_TAG, "This reply is not for me !");
        return;
    }

    if(m_timer.isActive()) {
        Q_EMIT stoptimer();
        m_countdown = 0;
    }

    if(!reply.toBool()) {
        LOG_INFO(LOG_TAG, "Restore the old screen mode");
        m_newRes.clear();
        try {
            m_displayEngine->setResolution(m_currentRes);
        } catch (const char* exception) {
            LOG_WARNING(LOG_TAG, "%s", exception);
        }
    }
    else {
        LOG_INFO(LOG_TAG, "Retain the new display mode");

        m_currentRes = m_newRes;
        Q_EMIT displayValueChanged(group, key, QVariant(m_newRes));
    }
}

/**
 * @brief DisplayManager::registerDisplayEngines
 */
void DisplayManager::registerDisplayEngines()
{
    LOG_ERROR(LOG_TAG, "registerDisplayEngines...");

#if defined (Q_OS_LINUX)
#ifdef AMLOGIC_TARGET
    m_displayEngine.reset(new AmlDisplayEngine());
#endif //AMLOGIC_TARGET
    m_displayEngine.reset(new GenDisplayEngine());

#elif defined (Q_OS_OSX)
#elif defined (Q_OS_WINDOWS)
#else
#endif// Q_OS_LINUX

}

} // namespace KvalDisplayPlatorm
