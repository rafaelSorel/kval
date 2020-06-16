
var _channelEpgData = {}

WorkerScript.onMessage = function(msg) 
{
    if (msg.action === 'addData')
    {
        msg.model.clear()
        _channelEpgData = {}
        var i;
        for(i = 0; i < msg.channels.length ; i++)
        {
            msg.model.insert(i,
            {
                'channelName': msg.channels[i]['channelName'],
                'channelId': msg.channels[i]['channelId'],
                'channelUrl': msg.channels[i]['channelUrl'],
                'channelProgress':0,
                'channelEpg': "",
            });
        }
        msg.model.sync();
        WorkerScript.sendMessage({ 'reply': 'ready'})
    }
    else if (msg.action === 'updateEpg')
    {
        for(i = 0; i < msg.model.count ; i++)
        {
            if (msg.epg['name'] === msg.model.get(i).channelName)
            {
                if(msg.viewlist)
                {
                    msg.model.set(i, {"channelProgress": (("progress" in msg.epg) ? msg.epg["progress"] : 0),
                                      "channelEpg": (("title" in msg.epg) ? msg.epg["title"] : "")})
                    msg.model.sync();   // updates the changes to the list
                }

                _channelEpgData[msg.model.get(i).channelName] = msg.epg
                break
            }
        }
    }
    else if (msg.action === 'getEpgFull')
    {
        if (msg.chName in _channelEpgData)
        {
            WorkerScript.sendMessage({ 'epgfull': _channelEpgData[msg.chName]})
        }
        else
        {
            WorkerScript.sendMessage({ 'epgfull': 'dummy' })
        }
    }
}
