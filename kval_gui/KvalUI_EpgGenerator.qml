import QtQuick 2.0
import QtQuick.XmlListModel 2.0
import "KvalUI_constant.js" as IPTV_Constants
import kval.gui.qml 1.0

Item
{
    id: kvalBoxEpgGenerator
    enabled: true

    property string     epgGenCurrentTitle      : "";
    property string     epgGenCurrentStart      : "";
    property string     epgGenCurrentEnd        : "";
    property string     epgGenCurrentRemains    : "";
    property string     epgGenNextTitle         : "";
    property string     epgGenNextStart         : "";
    property string     epgGenNextEnd           : "";
    property string     epgGenNextDuration      : "";
    property string     epgGenNextPicon         : "";
    property int        epgGenCurrentDuration   : 0 ;
    property string     epgGenCurrentSummary    : "";

    function clearEpgInfo()
    {
        updateEpgGlobal(true)
    }

    function initLocalEpgValues()
    {
        epgGenCurrentTitle = "";
        epgGenCurrentStart = "";
        epgGenCurrentEnd = "";
        epgGenCurrentRemains = "";
        epgGenCurrentDuration = 0;
        epgGenCurrentSummary= "";
        epgGenNextTitle = "";
        epgGenNextStart= "";
        epgGenNextEnd= "";
        epgGenNextDuration= "";
        epgGenNextPicon= "";
    }

    function setLocalEpgValues(epg)
    {
        if (epg === 'dummy')
        {
            updateEpgGlobal(true)
            return
        }

        epgGenCurrentTitle = epg["title"];
        epgGenCurrentStart = epg["start"];
        epgGenCurrentEnd = epg["end"];
        epgGenCurrentDuration = epg["progress"]
        epgGenCurrentRemains = epg["reamains"].toString()
        epgGenCurrentSummary = epg["desc"]

        if(!epg["hasnext"])
        {
            epgGenNextTitle = "";
            epgGenNextStart= "";
            epgGenNextEnd= "";
            epgGenNextDuration= "";
        }
        else
        {
            epgGenNextTitle = epg["nextTitle"];
            epgGenNextStart= epg["nextStart"];
            epgGenNextEnd= epg["nextEnd"];
            epgGenNextDuration= epg["nextDuration"];
        }
        updateEpgGlobal()
    }

    function updateEpgGlobal(clear)
    {
        if(clear)
        {initLocalEpgValues();
        }
        updateEpgSelectionView();
    }

    function updateEpgSelectionView()
    {
        channelSelectionListView.epgCurrentTitle = epgGenCurrentTitle;
        channelSelectionListView.epgCurrentStart = epgGenCurrentStart;
        channelSelectionListView.epgCurrentEnd = epgGenCurrentEnd;
        channelSelectionListView.epgCurrentDuration = epgGenCurrentDuration;
        //Current Summary
        channelSelectionListView.epgCurrentSummary = epgGenCurrentSummary;
        //Next epg Start Time
        if(epgGenNextStart === "")
        {
            channelSelectionListView.epgNextProgram = "";
            return;
        }
        else
        {
            channelSelectionListView.epgNextProgram =
                    kvalUiConfigManager.retranslate +
                    qsTr("Next: ") +
                    epgGenNextStart +
                    " - " +
                    epgGenNextDuration +
                    " Min"+ "\n" + epgGenNextTitle;
        }
    }

    function updateEpgInfoView(epg)
    {
        if('title' in epg)
        {
            channelInfoView.currentEpgTitle = epg["title"];
            channelInfoView.currentEpgTiming = (epg["start"] === "") ? '' :
                                               (epg["start"] + " - "+
                                                epg["end"] + "\n" + "+" +
                                                epg["reamains"].toString() + " min")
            channelInfoView.epgCurrentDuration = (epg["start"] === "") ? 0 : epg["progress"]

            if(!epg["hasnext"])
            {
                channelInfoView.nextEpgTitle = "";
                channelInfoView.nextEpgStartEnd = "";
                channelInfoView.nextEpgDuration = "";
                channelInfoView.epgCurrentDuration = 0;
            }
            else
            {
                channelInfoView.nextEpgTitle = epg["nextTitle"];
                channelInfoView.nextEpgStartEnd = epg["nextStart"] + " - " +
                                                epg["nextEnd"];
                channelInfoView.nextEpgDuration = epg["nextDuration"] + " min";
            }
        }
        else
        {
            channelInfoView.currentEpgTitle = ""
            channelInfoView.currentEpgTiming = ""
            channelInfoView.epgCurrentDuration = 0
            channelInfoView.nextEpgTitle = "";
            channelInfoView.nextEpgStartEnd = "";
            channelInfoView.nextEpgDuration = "";
            channelInfoView.epgCurrentDuration = 0;
        }
    }
}
