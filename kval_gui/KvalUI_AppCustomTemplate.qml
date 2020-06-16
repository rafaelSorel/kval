import QtQuick 2.3
import QtGraphicalEffects 1.0
import iptv.box.qml 1.0

Item
{
    id: customUiTemplateView

    x: 0
    y: 0
    width: rootRectangle.width
    height: rootRectangle.height

    signal activeFocus()
    signal activateAppView()
    signal clearTemplate()
    signal activateMainScreenView()

    Rectangle {
        anchors.fill: parent
        color: "Red"
        opacity: 0.4
    }

    function activateTemplateView()
    {
        logger.info("customUiAppTemplate::activatedCustomView....")
        registerActiveView(customUiAppTemplate.item)
    }

    function showView()
    {
        activeFocus()
        activeFocusViewManager(customUiAppTemplate.item)
        opacity = 1
    }

    function endOfCategory(succeeded)
    {
        logger.info("customUiAppTemplate::endOfCategory succeeded: " + succeeded)
    }

    function appHasStopped()
    {
        logger.info("customUiAppTemplate::appHasStopped()")
        showView()
    }

    function appHasStarted()
    {
        logger.info("customUiAppTemplate::appHasStarted()")
    }

    function closeApplication()
    {
        appsView.abortRunningApp()

        enabled = false
        opacity = 0
        focus = false
        pyNotificationView.hideVodNotification();
    }

    function appViewBackPressed()
    {
        activateAppView()
        closeApplication()
        clearTemplate()
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_O) //Ok Button
        {
            logger.info("Pressed OK keys ...")
        }
        else if (event.key === Qt.Key_Backspace) //Back Button
        {
            appViewBackPressed();
        }
        else if (event.key === Qt.Key_H) //Home Button
        {
            closeApplication();
            activateMainScreenView();
            clearTemplate()
        }

        else {}
        event.accepted = true;
    }
}
