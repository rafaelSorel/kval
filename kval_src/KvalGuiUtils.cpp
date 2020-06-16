/****************************************************************************
**
****************************************************************************/
#define LOG_ACTIVATED

#ifdef ENABLE_LIRC_SYSTEM_CONTROLE
#include <lirc/lirc_client.h>
#endif //ENABLE_LIRC_SYSTEM_CONTROLE

#include <errno.h>

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QQmlEngine>

#include "KvalGuiUtils.h"
#include "KvalConfigManager.h"
#define LOG_SRC MODELSTR
#include "KvalLogging.h"

#define EPG_TITLE_MAX_LEN_ON_DISPLAY     30
#define MAX_CH_FAV_LIST_LEN              100

QE_QmlCustomView * g_quick_view = nullptr;

#ifdef ENABLE_LIRC_SYSTEM_CONTROLE
static uint64_t epochMilli, epochMicro;
static void initialiseEpoch(void)
{
  struct timespec ts ;

  clock_gettime (CLOCK_MONOTONIC_RAW, &ts) ;
  epochMilli =  (uint64_t)ts.tv_sec * (uint64_t)1000    +
                (uint64_t)(ts.tv_nsec / 1000000L) ;
  epochMicro =  (uint64_t)ts.tv_sec * (uint64_t)1000000 +
                (uint64_t)(ts.tv_nsec /    1000L) ;
}


unsigned int millis(void)
{
  uint64_t now ;
  struct  timespec ts ;

  clock_gettime (CLOCK_MONOTONIC_RAW, &ts) ;
  now  =  (uint64_t)ts.tv_sec * (uint64_t)1000 +
          (uint64_t)(ts.tv_nsec / 1000000L) ;

  return (uint32_t)(now - epochMilli) ;
}
#endif

QE_QmlCustomView::QE_QmlCustomView()
{
    LOG_INFO(LOG_TAG, "Instantiate QE_QmlCustomView()");
    g_quick_view = this;
}

QE_QmlCustomView::~QE_QmlCustomView()
{
    LOG_INFO(LOG_TAG, "Delete QE_QmlCustomView...");
}

bool QE_QmlCustomView::event(QEvent *event)
{
    return QQuickView::event(event);
}

/**
 * @brief QE_QmlCustomView::processKey
 * @param keyCode
 */
void QE_QmlCustomView::processKey(unsigned int keyCode)
{
    QKeyEvent keyEvent(QEvent::KeyPress,
                       keyCode,
                       Qt::NoModifier);
    this->event(&keyEvent);

    QKeyEvent keyReleaseEvent(QEvent::KeyRelease, 
                              keyCode, 
                              Qt::NoModifier);
    this->event(&keyReleaseEvent);
}

#ifdef ENABLE_LIRC_SYSTEM_CONTROLE
/**
 * @brief QERemoteControleKeyEmitter::RemoteControleProcess
 */
QERemoteControleKeyEmitter::QERemoteControleKeyEmitter(QObject * receiver)
{
    LOG_INFO(LOG_TAG, "Instantiate QERemoteControleKeyEmitter()");
    initialiseEpoch();

    m_mainQuickWindow = receiver;
    //Initiate LIRC. Exit on failure
    if(lirc_init((char*)"lirc",1) == -1)
    {
        LOG_ERROR(LOG_TAG, "Could not init lirc !!");
        delete this;
    }
    m_lircQtKeys["KEY_LIST"] = Qt::Key_Return;
    m_lircQtKeys["KEY_SCROLLUP"] = Qt::Key_Up;
    m_lircQtKeys["KEY_SCROLLDOWN"] = Qt::Key_Down;
    m_lircQtKeys["KEY_SELECT"] = Qt::Key_O;
    m_lircQtKeys["KEY_EXIT"] = Qt::Key_Escape;
    m_lircQtKeys["KEY_RED"] = Qt::Key_R;
    m_lircQtKeys["KEY_GREEN"] = Qt::Key_G;
    m_lircQtKeys["KEY_YELLOW"] = Qt::Key_Y;
    m_lircQtKeys["KEY_BLUE"] = Qt::Key_B;
    m_lircQtKeys["KEY_BACK"] = Qt::Key_Backspace;
    m_lircQtKeys["KEY_PAUSE"] = Qt::Key_Space;
    m_lircQtKeys["KEY_PLAY"] = Qt::Key_P;
    m_lircQtKeys["KEY_STOP"] = Qt::Key_S;
    m_lircQtKeys["KEY_FASTFORWARD"] = Qt::Key_F;
    m_lircQtKeys["KEY_FASTREWIND"] = Qt::Key_W;
    m_lircQtKeys["KEY_FORWARD"] = Qt::Key_F;
    m_lircQtKeys["KEY_REWIND"] = Qt::Key_W;
    m_lircQtKeys["BTN_LEFT"] = Qt::Key_Left;
    m_lircQtKeys["BTN_RIGHT"] = Qt::Key_Right;
    m_lircQtKeys["KEY_MENU"] = Qt::Key_M;
    m_lircQtKeys["KEY_HOME"] = Qt::Key_H;
    m_lircQtKeys["KEY_VOLUMEUP"] = Qt::Key_U;
    m_lircQtKeys["KEY_VOLUMEDOWN"] = Qt::Key_D;
    m_lircQtKeys["KEY_MUTE"] = Qt::Key_N;
    m_lircQtKeys["KEY_CHANNELUP"] = Qt::Key_Plus;
    m_lircQtKeys["KEY_CHANNELDOWN"] = Qt::Key_Minus;
    m_lircQtKeys["KEY_BTN0"] = Qt::Key_0;
    m_lircQtKeys["KEY_BTN1"] = Qt::Key_1;
    m_lircQtKeys["KEY_BTN2"] = Qt::Key_2;
    m_lircQtKeys["KEY_BTN3"] = Qt::Key_3;
    m_lircQtKeys["KEY_BTN4"] = Qt::Key_4;
    m_lircQtKeys["KEY_BTN5"] = Qt::Key_5;
    m_lircQtKeys["KEY_BTN6"] = Qt::Key_6;
    m_lircQtKeys["KEY_BTN7"] = Qt::Key_7;
    m_lircQtKeys["KEY_BTN8"] = Qt::Key_8;
    m_lircQtKeys["KEY_BTN9"] = Qt::Key_9;
    m_lircQtKeys["KEY_INFO"] = Qt::Key_I;

    connect(this, SIGNAL(sendKey(unsigned int)), 
            m_mainQuickWindow, SLOT(processKey(unsigned int)));
}

/**
 * @brief QERemoteControleKeyEmitter::~QERemoteControleKeyEmitter
 */
QERemoteControleKeyEmitter::~QERemoteControleKeyEmitter()
{
    LOG_INFO(LOG_TAG, "Delete QERemoteControleKeyEmitter()");

    //closes the connection to lircd and does some internal clean-up stuff.
    lirc_deinit();
    QThread::currentThread()->terminate();
}

/**
 * @brief QERemoteControleKeyEmitter::run
 */
void QERemoteControleKeyEmitter::run()
{
    LOG_INFO(LOG_TAG, "QERemoteControleKeyEmitter Thread start ");
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);
    struct lirc_config *config;
    int gap = 125;
    int retries = 0;
    //Timer for buttons
    int buttonTimer = millis();
    char * code;
    if(lirc_readconfig("/dev/null",&config,NULL)==0)
    {
        while(lirc_nextcode(&code) == 0)
        {
            LOG_DEBUG(LOG_TAG, "lirc_nextcode ...");
            if(code == NULL) continue;
            //Make sure there is a 125ms gap before detecting button presses.
            if((millis() - buttonTimer) < 125)
            {
                if (retries > 10)
                    gap = 50;
            }
            else
            {
                gap = 125;
                if (retries > 5) retries = 0;
            }
            if ((millis() - buttonTimer) > gap)
            {
                QHashIterator<const char*, unsigned int> iterator(m_lircQtKeys);
                while (iterator.hasNext())
                {
                    iterator.next();
                    if(strstr (code, iterator.key()))
                    {
#if 0 /*Screen Shot Mode*/
                        if(strstr(code, (char*)"00000000000016b1 00 KEY_PAUSE shot"))
                        {
                            LOG_INFO(LOG_TAG, "Smile we are screening :)");
                            QE_QmlCustomView * mainQuickWindow = (QE_QmlCustomView*)m_mainQuickWindow;
                            QImage img = mainQuickWindow->grabWindow();
                            img.save("/storage/screen_1.png");
                        }
#endif
                        this->sendKey(iterator.value());
                        buttonTimer = millis();
                        retries++;
                    }
                }
            }
            else
            {
                retries++;
            }
        }
    }
    free(code);
    //Frees the data structures associated with config.
    lirc_freeconfig(config);
}
#endif //ENABLE_LIRC_SYSTEM_CONTROLE

/**
 * @brief QE_QmlBinder::QE_QmlBinder
 */
KvalGuiUtils::KvalGuiUtils():
    m_liveCustomMainZoneInfo{},
    m_liveCustomFirstBanZoneInfo{},
    m_liveCustomSecondBanZoneInfo{},
    m_liveCustomSecondBanAnimZoneImg{},
    m_liveCustomSecondBanAnimZoneTxt{},
    m_liveCustomThirdBanZoneInfo{},
    m_liveCustomThirdBanAnimZoneImg{},
    m_liveCustomThirdBanAnimZoneTxt{},
    m_vodCustomTopLeftZoneInfo{},
    m_vodCustomBottomLeftZoneInfo{},
    m_vodCustombottomMiddleZoneInfo{},
    m_vodCustomTopRightZoneInfo{},
    m_vodCustomMiddleRightZoneInfo{},
    m_vodCustomBottomRightZoneInfo{},
    m_channelstree{},
    m_favoriteChannelsIdx{}
{
    LOG_INFO(LOG_TAG, "Instantiate QE_QmlBinder");
    analyseXmlLiveChFile();
    analyseXmlLiveCusomFile();
    analyseXmlVODCusomFile();

}

/**
 * @brief QE_QmlBinder::~QE_QmlBinder
 */
KvalGuiUtils::~KvalGuiUtils()
{
    LOG_INFO(LOG_TAG, "Delete QE_kvalUiGuiUtils...");
}

/**
 * @brief QE_QmlBinder::analyseXmlLiveChFile
 */
void KvalGuiUtils::analyseXmlLiveChFile()
{
    INV();

    QDomDocument doc;
    QDomElement docElem;
    QDomNodeList nodeList;
    QList<int> loc_channelsIndexes;
    LOG_DEBUG(LOG_TAG, "analyseXmlLiveChFile called");

    QString _fpath= CfgObj->get_path(KvalConfigManager::KVAL_XML_LIVE_TV_FILE);
    QFile file(_fpath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "Couldn't open xml file: "
                    << CfgObj->get_path(KvalConfigManager::KVAL_XML_LIVE_TV_FILE);
        goto return_cleanup;
    }

    m_channelstree.clear();
    doc.setPrefix("mydocument");
    if (!doc.setContent(&file))
    {
        LOG_ERROR(LOG_TAG, "Couldn't set content !!");
        goto return_cleanup;
    }

    docElem = doc.documentElement();
    nodeList = docElem.elementsByTagName("liveTvFavorite");

    for(int i = 0; i < nodeList.count(); i++)
    {
        QDomElement currentElement = nodeList.at(i).toElement();
        QDomNodeList childNodeList = currentElement.childNodes();
        loc_channelsIndexes.clear();
        for(int j = 0; j < childNodeList.count(); j++)
        {
            QDomElement currentChildElement = childNodeList.at(j).toElement();
            loc_channelsIndexes.append(currentChildElement.attribute("id").toInt());
        }
        m_channelstree.append(loc_channelsIndexes);
    }

return_cleanup:
    file.close();
    OUTV();
}

/**
 * @brief QE_QmlBinder::analyseXmlVODCusomFile
 */
void KvalGuiUtils::analyseXmlVODCusomFile()
{
    INV();

    QDomDocument doc;
    QDomElement docElem;
    QDomNodeList nodeList;
    QString _fpath = CfgObj->get_path(KvalConfigManager::KVAL_XML_VOD_CUSTOM_FILE);
    QFile file(_fpath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "Couldn't open xml file: " << _fpath;
        return;
    }

    doc.setPrefix("mydocument");
    if (!doc.setContent(&file))
    {
        LOG_ERROR(LOG_TAG, "Couldn't set content !!");
        file.close();
        return;
    }

    m_vodCustomTopLeftZoneInfo.clear();
    m_vodCustomBottomLeftZoneInfo.clear();
    m_vodCustombottomMiddleZoneInfo.clear();
    m_vodCustomTopRightZoneInfo.clear();
    m_vodCustomMiddleRightZoneInfo.clear();
    m_vodCustomBottomRightZoneInfo.clear();

    docElem = doc.documentElement();

    nodeList = docElem.elementsByTagName("topLeftZone");
    QDomElement currentElement = nodeList.at(0).toElement();
    QDomNodeList childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        QDomNodeList currentChildNodeList = currentChildElement.childNodes();
        for(int k = 0; k < currentChildNodeList.count(); k++)
        {
            QDomElement deepChildElement =
                    currentChildNodeList.at(k).toElement();
            m_vodCustomTopLeftZoneInfo.append(
                    CfgObj->xmlGetStr(deepChildElement.text()));
        }
    }

    nodeList = docElem.elementsByTagName("bottomLeftZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        QDomNodeList currentChildNodeList = currentChildElement.childNodes();
        for(int k = 0; k < currentChildNodeList.count(); k++)
        {
            QDomElement deepChildElement =
                    currentChildNodeList.at(k).toElement();
            m_vodCustomBottomLeftZoneInfo.append(
                        CfgObj->xmlGetStr(deepChildElement.text()));
        }
    }

    nodeList = docElem.elementsByTagName("bottomMiddleZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        QDomNodeList currentChildNodeList = currentChildElement.childNodes();
        for(int k = 0; k < currentChildNodeList.count(); k++)
        {
            QDomElement deepChildElement =
                    currentChildNodeList.at(k).toElement();
            m_vodCustombottomMiddleZoneInfo.append(
                        CfgObj->xmlGetStr(deepChildElement.text()));
        }
    }

    nodeList = docElem.elementsByTagName("topRightZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        if( currentChildElement.tagName() == "bottomRightTxt" ||
            currentChildElement.tagName() == "bottomLeftTxt")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_vodCustomTopRightZoneInfo.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else
        {m_vodCustomTopRightZoneInfo.append(
                        CfgObj->xmlGetStr(currentChildElement.text()));
        }
    }

    nodeList = docElem.elementsByTagName("middleRightZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        if( currentChildElement.tagName() == "bottomRightTxt" ||
            currentChildElement.tagName() == "bottomLeftTxt" )
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_vodCustomMiddleRightZoneInfo.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else
        {m_vodCustomMiddleRightZoneInfo.append(
                        CfgObj->xmlGetStr(currentChildElement.text()));
        }
    }

    nodeList = docElem.elementsByTagName("bottomRightZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        if( currentChildElement.tagName() == "bottomRightTxt" ||
            currentChildElement.tagName() == "bottomLeftTxt"||
            currentChildElement.tagName() == "iconImageTxt")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_vodCustomBottomRightZoneInfo.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else
        {m_vodCustomBottomRightZoneInfo.append(
                        CfgObj->xmlGetStr(currentChildElement.text()));
        }
    }

    file.close();
    OUTV();
}

/**
 * @brief QE_QmlBinder::analyseXmlLiveCusomFile
 */
void KvalGuiUtils::analyseXmlLiveCusomFile()
{
    INV();

    QDomDocument doc;
    QDomElement docElem;
    QDomNodeList nodeList;
    QString _fpath =
            CfgObj->get_path(KvalConfigManager::KVAL_XML_LIVE_TV_CUSTOM_FILE);
    QFile file(_fpath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "Couldn't open xml file: " << _fpath;
        OUTV();
        return;
    }

    doc.setPrefix("mydocument");
    if (!doc.setContent(&file))
    {
        LOG_ERROR(LOG_TAG, "Couldn't set content !!");
        file.close();
        OUTV();
        return;
    }

    m_liveCustomMainZoneInfo.clear();
    m_liveCustomFirstBanZoneInfo.clear();
    m_liveCustomSecondBanZoneInfo.clear();
    m_liveCustomSecondBanAnimZoneImg.clear();
    m_liveCustomSecondBanAnimZoneTxt.clear();
    m_liveCustomThirdBanZoneInfo.clear();
    m_liveCustomThirdBanAnimZoneImg.clear();
    m_liveCustomThirdBanAnimZoneTxt.clear();

    docElem = doc.documentElement();

    nodeList = docElem.elementsByTagName("mainZone");
    QDomElement currentElement = nodeList.at(0).toElement();
    QDomNodeList childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        if( currentChildElement.tagName() == "descriptionText" ||
            currentChildElement.tagName() == "overlayText")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_liveCustomMainZoneInfo.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else
        {m_liveCustomMainZoneInfo.append(currentChildElement.text());
        }
    }

    nodeList = docElem.elementsByTagName("firstBannerZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        if( currentChildElement.tagName() == "firstBannerLabel" ||
            currentChildElement.tagName() == "firstBannerEticketTxt")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_liveCustomFirstBanZoneInfo.append(
                        CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else
        {
            m_liveCustomFirstBanZoneInfo.append(
                        CfgObj->xmlGetStr(currentChildElement.text()));
        }
    }

    nodeList = docElem.elementsByTagName("secondBannerZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        if( currentChildElement.tagName() == "topLeftMiniImageTxt" ||
            currentChildElement.tagName() == "secondBannerLabel")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                LOG_DEBUG(LOG_TAG,
                          "deepChildElement.text(): %s",
                          deepChildElement.text().toStdString().c_str());
                m_liveCustomSecondBanZoneInfo.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else if( currentChildElement.tagName() == "bottomRightTxt")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_liveCustomSecondBanAnimZoneTxt.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else if( currentChildElement.tagName() == "animatedImages")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_liveCustomSecondBanAnimZoneImg.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else
        {m_liveCustomSecondBanZoneInfo.append(
                        CfgObj->xmlGetStr(currentChildElement.text()));
        }
    }

    nodeList = docElem.elementsByTagName("thirdBannerZone");
    currentElement = nodeList.at(0).toElement();
    childNodeList = currentElement.childNodes();
    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        if( currentChildElement.tagName() == "topLeftMiniImageTxt" ||
            currentChildElement.tagName() == "thirdBannerLabel")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_liveCustomThirdBanZoneInfo.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else if( currentChildElement.tagName() == "bottomRightTxt")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_liveCustomThirdBanAnimZoneTxt.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else if( currentChildElement.tagName() == "animatedImages")
        {
            QDomNodeList currentChildNodeList =
                    currentChildElement.childNodes();
            for(int k = 0; k < currentChildNodeList.count(); k++)
            {
                QDomElement deepChildElement =
                        currentChildNodeList.at(k).toElement();
                m_liveCustomThirdBanAnimZoneImg.append(
                            CfgObj->xmlGetStr(deepChildElement.text()));
            }
        }
        else
        {m_liveCustomThirdBanZoneInfo.append(
                        CfgObj->xmlGetStr(currentChildElement.text()));
        }
    }
    file.close();
    OUTV();
}

/**
 * @brief QE_QmlBinder::removeSelectedChannelFromFav
 * @param chIndex
 */
void KvalGuiUtils::removeSelectedChannelFromFav(const QString &chIndex)
{
    INV("chIndex = %s", qPrintable(chIndex));
    if(delChannelFromFavXml(chIndex))
    {
        LOG_ERROR(LOG_TAG, "Could not remove channel from favorite !");
        displayMsg("Error", "Une erreur c'est produite" );
        OUTV("");
        return;
    }
    displayMsg("Info", "Chaines supprimées des favoris" );
    OUTV("");
}

/**
 * @brief QE_QmlBinder::clearChannelFavoriteList
 */
void KvalGuiUtils::clearChannelFavoriteList()
{
    if(this->clearFavList())
    {
        LOG_ERROR(LOG_TAG, "Could not clear favorite list");
        displayMsg("Error", "Une erreur c'est produite" );
        OUTV("");
        return;
    }
    displayMsg("Info", "Liste favoris vide." );
    OUTV("");
}

/**
 * @brief QE_QmlBinder::clearFavList
 * @return 
 */
int KvalGuiUtils::clearFavList()
{
    INV("");
    QString _fpath =
        CfgObj->get_path(KvalConfigManager::KVAL_XML_LIVE_TV_FAVCHANNEL_FILE);
    QFile file(_fpath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "Couldn't open file: " << _fpath;;
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }

    QDomDocument doc("mydocument");
    if (!doc.setContent(&file))
    {
        LOG_ERROR(LOG_TAG, "Couldn't set content !!"); 
        file.close();
        OUTV("%d", -EIO);
        return -EIO;
    }

    QDomElement docElem = doc.documentElement();
    QDomNodeList nodeList = docElem.elementsByTagName("favorite");

    // get the current one as QDomElement
    QDomElement currentElement = nodeList.at(0).toElement();
    QDomNode pEntries = currentElement.firstChild();
    while(!pEntries.isNull())
    {
        currentElement.removeChild(pEntries);
        pEntries = currentElement.firstChild();
    }

    file.close();
    if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly))
    {
        LOG_ERROR(LOG_TAG, "Basically Now we have lost the file !!!");
        OUTV("%d", -EIO);
        return -EIO;
    }
    QByteArray xml = doc.toByteArray();
    file.write(xml);
    file.close();

    m_favoriteChannelsIdx.clear();
    OUTV("%d", ENOERR);
    return ENOERR;
}

/**
 * @brief QE_QmlBinder::addSelectedChannelToFav
 * @param chIndex
 */
void KvalGuiUtils::addSelectedChannelToFav( const QString &chIndex,
                                            const QString &chName, 
                                            const QString &chUri)
{
    INV("chIndex = %s, chName = %s, chUri = %s", qPrintable(chIndex),
                                                qPrintable(chName),
                                                qPrintable(chUri));
    int rc = checkFavList(chIndex.toInt());
    if(rc)
    {
        LOG_ERROR(LOG_TAG, "Could not add channel to favorite !");
        if(rc == -EINVAL)
        {displayMsg("Error", "Existe déja dans les favoris" );
        }
        else
        {displayMsg("Error", "Liste des Favoris pleine." );
        }
        return;
    }
    if(addChannelToFavXml(chIndex, chName, chUri))
    {
        LOG_ERROR(LOG_TAG, "Could not add channel to favorite !");
        displayMsg("Error", "Une erreur c'est produite" );
    }
    displayMsg("Info", "Ajout avec succés au favoris" );
    OUTV("");
}

/**
 * @brief QE_QmlBinder::checkFavList
 * @param index
 * @return 
 */
int KvalGuiUtils::checkFavList(int index)
{
    INV("index = %d", index);
    if(m_favoriteChannelsIdx.count() > MAX_CH_FAV_LIST_LEN)
        return -ENOENT;

    for(int i = 0; i<m_favoriteChannelsIdx.count(); i++)
    {
        if(m_favoriteChannelsIdx.at(i) == index)
        {
            OUTV("EINVAL");
            return -EINVAL;
        }
    }
    OUTV("ENOERR");
    return ENOERR;
}

/**
 * @brief QE_QmlBinder::delChannelFromFavXml
 * @param chIndex
 * @return 
 */
int KvalGuiUtils::delChannelFromFavXml(const QString &chIndex)
{
    INV("chIndex = %s", qPrintable(chIndex));
    QString _fpath =
        CfgObj->get_path(KvalConfigManager::KVAL_XML_LIVE_TV_FAVCHANNEL_FILE);
    QFile file(_fpath);
    bool isDeleted = false;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "Couldn't open the file: " << _fpath;
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }

    QDomDocument doc("mydocument");
    if (!doc.setContent(&file))
    {
        LOG_ERROR(LOG_TAG, "Couldn't set content !!"); 
        file.close();
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }

    QDomElement docElem = doc.documentElement();
    QDomNodeList nodeList = docElem.elementsByTagName("favorite");

    // get the current one as QDomElement
    QDomElement currentElement = nodeList.at(0).toElement();
    QDomNodeList childNodeList = currentElement.childNodes();

    for(int j = 0; j < childNodeList.count(); j++)
    {
        QDomElement currentChildElement = childNodeList.at(j).toElement();
        LOG_INFO(LOG_TAG, "chIndex %s", qPrintable(chIndex));
        LOG_INFO(LOG_TAG, "attribute %s", qPrintable(currentChildElement.attribute("id")));
        if(!currentChildElement.attribute("id").compare(chIndex))
        {
            LOG_INFO(LOG_TAG, "Remove Entery channel %s", qPrintable(chIndex));
            currentElement.removeChild(currentChildElement);
            isDeleted = true;
            break;
        }
    }

    //Check if we have deleted the file
    if(!isDeleted)
    {
        file.close();
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }

    file.close();
    if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly))
    {
        LOG_ERROR(LOG_TAG, "Basically Now we have lost the file !!!");
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }
    QByteArray xml = doc.toByteArray();
    file.write(xml);
    file.close();

    for(int i = 0 ; i < m_favoriteChannelsIdx.count(); i++)
    {
        if(m_favoriteChannelsIdx.at(i) == chIndex.toInt())
        {
            m_favoriteChannelsIdx.removeAt(i);
            break;
        }
    }

    OUTV("%d", ENOERR);
    return ENOERR;
}

/**
 * @brief QE_QmlBinder::addChannelToFavXml
 * @param chIndex
 * @param chName
 * @param chUri
 * @return 
 */
int KvalGuiUtils::addChannelToFavXml(const QString &chIndex,
                                    const QString &chName, 
                                    const QString &chUri)
{
    INV("chIndex = %s, chName = %s, chUri = %s", qPrintable(chIndex),
                                                qPrintable(chName),
                                                qPrintable(chUri));
    QString _fpath = CfgObj->get_path(
                        KvalConfigManager::KVAL_XML_LIVE_TV_FAVCHANNEL_FILE);
    QFile file(_fpath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCritical() << "Couldn't open file: " << _fpath;
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }

    QDomDocument doc("mydocument");
    if (!doc.setContent(&file))
    {
        LOG_ERROR(LOG_TAG, "Couldn't set content !!"); 
        file.close();
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }

    QDomElement docElem = doc.documentElement();
    QDomNodeList nodeList = docElem.elementsByTagName("favorite");

    // get the current one as QDomElement
    QDomElement currentElement = nodeList.at(0).toElement();
    QDomElement channel = doc.createElement("channel");
    channel.setAttribute( "id", chIndex);
    channel.setAttribute( "title", chName);
    channel.setAttribute( "url", chUri);
    currentElement.appendChild(channel);

    file.close();
    if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly))
    {
        LOG_ERROR(LOG_TAG, "Basically Now we have lost the file !!!");
        OUTV("%d", -EINVAL);
        return -EINVAL;
    }
    QByteArray xml = doc.toByteArray();
    file.write(xml);
    file.close();

    m_favoriteChannelsIdx.append(chIndex.toInt());
    OUTV("%d", ENOERR);
    return ENOERR;
}

/**
 * @brief QE_QmlBinder::reloadChannelsList
 */
void KvalGuiUtils::reloadChannelsList()
{
    INV("");
    analyseXmlLiveChFile();
    OUTV("");
}

/**
 * @brief QE_QmlBinder::updateCustomsList
 */
void KvalGuiUtils::updateCustomsList()
{
    INV("");
    analyseXmlLiveCusomFile();
    analyseXmlVODCusomFile();
    Q_EMIT cutomLiveTvParsed();
    OUTV("");
}

/**
 * @brief QE_QmlBinder::stopWelcomeScreenProc
 */
void KvalGuiUtils::stopWelcomeScreenProc()
{
    INV();
    const char WELCOME_SCREEN_TASKS_TERM[] =  "killall -15 kvalWelcomScreen";
    (void)WELCOME_SCREEN_TASKS_TERM;
    OUTV();
}

/**
 * @brief QE_QmlBinder::trimCache
 */
void KvalGuiUtils::trimCache()
{
    LOG_INFO(LOG_TAG, "QML Engine trimComponentCache");
    QQmlEngine* engine = g_quick_view->engine();
    engine->trimComponentCache();
}

/**
 * @brief QE_QmlBinder::fileExists
 * @param fileName
 * @return 
 */
QList<int> KvalGuiUtils::getChannelPosition(int channelNumber)
{
    INV("channelNumber = %d", channelNumber);
    QList<int> position;
    for(int i = 0; i < m_channelstree.count(); i++)
    {
        for(int j = 0; j < m_channelstree.at(i).count(); j++)
        {
            if (m_channelstree.at(i).at(j) == channelNumber)
            {
                position.append(i);
                position.append(j);
                return position;
            }
        }
    }
    position.append(-1);
    position.append(-1);
    return position;
}

void QE_QMLLogger::debug(QString s) const {
    log_debug(qPrintable("[QML] "+s));
}

bool QE_QMLLogger::verbose(QString s) const {
    return log_verbose(qPrintable("[QML] "+s));
}

bool QE_QMLLogger::info(QString s) const {
    return log_info(qPrintable("[QML] "+s));
}

bool QE_QMLLogger::warn(QString s) const {
    return log_warn(qPrintable("[QML] "+s));
}

bool QE_QMLLogger::error(QString s) const {
    return log_err(qPrintable("[QML] "+s));
}

bool QE_QMLLogger::critical(QString s) const {
    return log_critical(qPrintable("[QML] "+s));
}

void QE_QMLLogger::registerObject(QQmlContext *context) {
    context->setContextProperty(QStringLiteral("logger"), &(instance()));
}
