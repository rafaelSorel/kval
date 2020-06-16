/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#define LOG_ACTIVATED
#include <QApplication>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <qqmlengine.h>
#include <qqmlcontext.h>
#include <qqml.h>
#include <QFile>
#include <QScreen>
#include <QQuickView>
#include <QFontDatabase>
#include <QObject>
#include <QSplashScreen>
#include <QTime>
#include <QProgressBar>
#include <QRect>
#include <QTranslator>

#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
#include <signal.h>
#include <execinfo.h>
#endif

#include "KvalGuiUtils.h"
#include "KvalConfigManager.h"
#include "KvalEpgEngine.h"
#include "KvalVodManager.h"
#include "appmanager/KvalAppManager.h"
#include "app/KvalAppElement.h"
#include "KvalResourceClientFetcher.h"
#include "KvalLocalBrowseManager.h"
#include "KvalSettingsManager.h"
#include "KvalLiveTvManager.h"
#include "player/KvalPlayerPlatform.h"
#define LOG_SRC MAINSTR
#include "KvalLogging.h"

#define ENABLE_QML_SAMPLE 1

/*------------------------------------------------------------------------------
|    Globals
+-----------------------------------------------------------------------------*/
QHash<Qt::HANDLE, QString> KvalThread::_m_threadHdles;
static void _kval_out();

/*------------------------------------------------------------------------------
|    Functions
+-----------------------------------------------------------------------------*/

#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
/**
 * @brief handlerSigsegv
 * @param sig
 */
void handlerSigsegv(int sig)
{
    Q_UNUSED(sig);
    LOG_ERROR(LOG_TAG, "segfault !!!");
    exit(0);
}

/**
 * @brief handlerSigTerm
 * @param sig
 */
void handlerSigTerm(int sig)
{
    Q_UNUSED(sig);
    return _kval_out();
}

/**
 * @brief handlerSigint
 * @param sig
 */
void handlerSigint(int sig)
{
    Q_UNUSED(sig);
//    log_stacktrace(LOG_TAG, LC_LOG_CRITICAL, 50);
    return _kval_out();
}
#endif //Q_OS_LINUX || Q_OS_OSX

/**
 * @brief _enable_remote_controle
 * @param view
 */
void _enable_remote_controle(QQuickView& view)
{
#ifdef ENABLE_LIRC_SYSTEM_CONTROLE
    QERemoteControleKeyEmitter * remoteEmitter =
            new QERemoteControleKeyEmitter(&view);
    remoteEmitter->start();
#else
    Q_UNUSED(view);
#endif //ENABLE_LIRC_SYSTEM_CONTROLE
}

/**
 * @brief _kval_out
 */
static void _kval_out()
{
    LOG_INFO(LOG_TAG, "Application Request STOP...");
    VM_VodPythonInvoker::vodStopScriptTask();
    SC_SubsClientPythonInvoker::clientStopScriptTask();
    KvalAppManagerBridge::appsStopScriptTask();

    LOG_INFO(LOG_TAG, "Bye Bye...");
    qApp->exit(0);
}

/**
 * @brief _register_signals
 */
void _register_signals(void)
{
#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
    signal(SIGSEGV, handlerSigsegv);
    signal(SIGINT, handlerSigint);
    signal(SIGABRT, handlerSigsegv);

    // SIGTERM handler
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = handlerSigTerm;
    sigaction(SIGTERM, &action, NULL);
#endif //Q_OS_LINUX || Q_OS_OSX
}

/**
 * @brief myMessageOutput
 * @param type
 * @param context
 * @param msg
 */
void loggerOutput(QtMsgType type,
                  const QMessageLogContext &context,
                  const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
//    const char *file = context.file ? context.file : "";

    QString func{};
    if(context.function){
        QRegularExpression re(
                "^.*?\\s*(?<class>\\w+)::(?<method>.*?)\\(.*?\\)\\s*$",
                QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(context.function);
        if (match.hasMatch()) {
            func = match.captured("class") + "::" + match.captured("method");
        }
    }
    switch (type) {
    case QtDebugMsg:
        log_debug("%s [%d] %s", qPrintable(func),context.line, localMsg.constData());
        break;
    case QtInfoMsg:
        log_info("%s [%d] %s", qPrintable(func) ,context.line,localMsg.constData());
        break;
    case QtWarningMsg:
        log_warn("%s [%d] %s", qPrintable(func),context.line,localMsg.constData());
        break;
    case QtCriticalMsg:
        log_err("%s [%d] %s", qPrintable(func),context.line,localMsg.constData());
        break;
    case QtFatalMsg:
        log_critical("%s [%d] %s", qPrintable(func),context.line,localMsg.constData());
        break;
    }
}

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    INV();
    qInstallMessageHandler(loggerOutput);

    QtAV::setLogLevel(LogOff);

    // For unix systems ...
    ::_register_signals();


    LOG_INFO(LOG_TAG, "Start Application ...");
    QApplication app(argc, argv);

#define TESTS 0
#if TESTS
    KvalApplication::Element element;
    app.exec();
    return 0;
#endif

    LOG_INFO(LOG_TAG, "System configuration ...");
    KvalConfigManager::Instance();
    KvalSettingManager::Instance()->initialize();

    // Well Some platform needs an activation of the audio system
    // @TODO Find a better place to configure the audio system...
    KvalMediaPlayerEngine::configureAudioSystem();

    //Disable Mouse cursor
    QPixmap nullCursor(16, 16);
    nullCursor.fill(Qt::transparent);
    app.setOverrideCursor(QCursor(nullCursor));
    app.processEvents();

#if 0
    LOG_INFO(LOG_TAG, "Create splash screen ...");
    QPixmap pixmap(CfgObj->get_path(CM_ConfigManager::KVAL_LOAD_SCREEN));
    QSplashScreen * splash = new QSplashScreen(pixmap);
    splash->show();
    app.processEvents();
#endif

    LOG_INFO(LOG_TAG, "Create Qquick view ...");
    QE_QmlCustomView view;

    // Set EGL Surface.
    QSurfaceFormat curSurface = view.format();
    curSurface.setRedBufferSize(8);
    curSurface.setGreenBufferSize(8);
    curSurface.setBlueBufferSize(8);
    curSurface.setAlphaBufferSize(8);
    view.setFormat(curSurface);
    view.setColor(QColor(Qt::transparent));
    view.setClearBeforeRendering(true);

    QScreen *sc = app.primaryScreen();
    qreal refHeight = 1080.;
    qreal refWidth = 1920.;
    QRect rect = sc->geometry();
    qreal height = rect.height();
    qreal width = rect.width();
    qreal dpi = sc->logicalDotsPerInch();
    auto m_ratio = qMin(height/refHeight, width/refWidth);
    LOG_INFO(LOG_TAG, "height: %f", height);
    LOG_INFO(LOG_TAG, "width: %f", width);
    LOG_INFO(LOG_TAG, "dpi: %f", dpi);
    LOG_INFO(LOG_TAG, "ratio: %f", m_ratio);
//    qDebug()<< "dpi phy: " << sc->physicalDotsPerInch()
//            << ", logical: " << sc->logicalDotsPerInch()
//            << ", dpr: " << sc->devicePixelRatio()
//            << "; vis rect:" << sc->virtualGeometry();
    qreal ratio = sc->physicalDotsPerInch()/sc->logicalDotsPerInch();
    LOG_INFO(LOG_TAG, "ratio: %f", ratio);
    view.rootContext()->setContextProperty(QStringLiteral("screenPixelDensity"), sc->physicalDotsPerInch()*sc->devicePixelRatio());
    view.rootContext()->setContextProperty(QStringLiteral("scaleRatio"), m_ratio);
    view.setX(0);
    view.setY(0);
    view.setWidth(refWidth*m_ratio);
    view.setHeight(refHeight*m_ratio);
//    view.setGeometry(sc->virtualGeometry());

    // Create QML objects
    qmlRegisterType<KvalConfigManagerElement>("kval.gui.qml", 1, 0, "KvalUiConfigManager");
    qmlRegisterType<KvalSettingManagerElement>("kval.gui.qml", 1, 0, "KvalUiSettingsManager");
    qmlRegisterType<KvalMediaPlayerElement>("kval.gui.qml", 1, 0, "UI_PlayerElement");
    qmlRegisterType<KvalGuiUtils>("kval.gui.qml", 1, 0, "KvalUiGuiUtils");
    qmlRegisterType<KvalApplication::Element>("kval.gui.qml", 1, 0, "KvalUiAppElement");
    qmlRegisterType<KvalAppManagerElement>("kval.gui.qml", 1, 0, "UI_AppsElement");
    qmlRegisterType<VM_VodElement>("kval.gui.qml", 1, 0, "UI_VodElement");
    qmlRegisterType<KvalLiveTvManagerElement>("kval.gui.qml", 1, 0, "KvalUiLiveTvManager");
    qmlRegisterType<SC_SubsClientElements>("kval.gui.qml", 1, 0, "UI_SubsClientElement");
    qmlRegisterType<KvalMediaBrowserElements>("kval.gui.qml",1, 0, "UI_MediaBrowserElement");
    qmlRegisterType<QuickSubtitle>("kval.gui.qml", 1, 0, "UI_Subs");
    qmlRegisterType<KvalLiveStreamManagerElement>("kval.gui.qml", 1, 0, "KvalUiLiveStreamManager");



    // QML Logger System
    QE_QMLLogger::registerObject(view.rootContext());

    //Create the view
#if defined(Q_OS_LINUX) || defined(Q_OS_OSX) || defined(Q_OS_WINDOWS)
#if defined(AMLOGIC_TARGET)
    view.setSource(QUrl("qrc:///kval_gui/KvalUI_AmlMainView.qml"));
#else
    view.setSource(QUrl("qrc:///kval_gui/KvalUI_GenMainView.qml"));
#endif // AMLOGIC_TARGET
#endif // Q_OS
    view.setResizeMode(view.SizeRootObjectToView);
    _enable_remote_controle(dynamic_cast<QQuickView&>(view));


#if 0
    splash->close();
    app.processEvents();
    delete splash;
#endif
    view.showFullScreen();

    int ret;
    try {
        ret = app.exec();
    } catch (const std::bad_alloc &) {
        _kval_out();
        return EXIT_FAILURE; // exit the application
    }

    return ret;
}
