# player.py

from kval_app_binder import getBinder, getBinderMonitor
from PyQt5.QtDBus import QDBus, QDBusArgument, QDBusReply

__binder__ = getBinder()
__monitor__ = getBinderMonitor()

__all__ = ["Player", "PlayList"]


class Player:
    """
    Wrapper of kval player,
    so we could drive the play/stop/listing
    """

    def __init__(self, appid=None):
        self._isPlaying = False
        self.__appid = appid
        self.appbinder = __binder__.getComHdlr()
        if __monitor__:
            __monitor__.registerObj(self)

    def getAppId(self):
        return self.__appid

    def onPlayBackStarted(self):
        raise NotImplementedError()

    def onPlayBackStopped(self):
        raise NotImplementedError()

    def onPlayBackEnded(self):
        raise NotImplementedError()

    def onPlaybackFailed(self):
        raise NotImplementedError()

    def play(self, source, item=None, windowed=False, startpos=-1):
        """
         @brief Play an item.

         @param source              [opt] string - filename, url or playlist
         @param item                [opt] listitem - used with setInfo() to set
                                    different infolabels.
         @param windowed            [opt] bool - true=play video windowed,
                                    false=play users preference.(default)
         @param startpos            [opt] int - starting position when playing
                                    a playlist. Default = -1
         @note If  item  is  not  given then  the Player  will try  to play the
               current item in the current playlist.
               You can use the above as keywords for arguments and skip certain
               optional arguments
               Once  you use  a keyword, all following arguments require the
               keyword.
        -----------------------------------------------------------------------
         **Example:**
         ~~~~~~~~~~~~~{.py}
         item = Binder.ItemUi('Ironman')
         item.setInfo('video', {'Title': 'Ironman', 'Genre': 'Science Fiction'})
         Binder.Player().play(url, item)
         Binder.Player().play(playlist, item, startpos)
         ~~~~~~~~~~~~~
        """
        item_dict = {}

        if item:
            item_dict['label'] = item.label
            item_dict['subs'] = ','.join(item.subtitles)
            item_dict['IsPlayable'] = 'true' if item.IsPlayable else 'false'
            item_dict['isFolder'] = 'false'
            item_dict['path'] = item.path
            item_dict['url'] = item.path

            item_dict.update(item.artdict)
            item_dict.update(item.properties)
            item_dict.update(item.streamInfo)
            item_dict.update(item.metaInfo)
            item_dict.update(item.contextMenuItems)

        argument = QDBusArgument()
        argument.beginMap(QVariant.String, QVariant.String);
        for key, value in item_dict.iteritems():
            argument.beginMapEntry();
            argument.add(key, QVariant.String)
            strval = value if type(value) is str or type(value) is unicode else str(value)
            argument.add(strval, QVariant.String)
            argument.endMapEntry();
        argument.endMap();

        self.appbinder.call(QDBus.NoBlock,
                            'mediaplayer_play',
                            "playlist" if isinstance(source, PlayList) else source,
                            argument,
                            windowed,
                            startpos)

    def activatePlayList(self):
        """
        """
        self.appbinder.call(QDBus.NoBlock, 'mediaplayer_activateplaylist')

    def stop(self):
        """
        Stop player
        """
        self.appbinder.call(QDBus.NoBlock, 'mediaplayer_stop')

    def pause(self):
        """
        Pause player
        """
        self.appbinder.call(QDBus.NoBlock, 'mediaplayer_pause')

    def playnext(self):
        """
        Play next item in playlist.
        """
        self.appbinder.call(QDBus.NoBlock, 'mediaplayer_playnext')

    def playprevious(self):
        """
        Play previous item in playlist.
        """
        self.appbinder.call(QDBus.NoBlock, 'mediaplayer_playprevious')

    def playselected(self, selected):
        """
        Play selected item in playlist.
        """
        self.appbinder.call(QDBus.NoBlock, 'mediaplayer_playselected', selected)

    def isPlaying(self):
        """
        Is playing state
        """
        res = self.appbinder.call(QDBus.Block, 'mediaplayer_isplaying')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not create playlist")
            return Flase

        LOGGER.info("isPlaying: " + str(reply.value()))
        return reply.value()

    def isPlayingAudio(self):
        """
        Is playing Audio
        """
        res = self.appbinder.call(QDBus.Block, 'mediaplayer_isplayingaudio')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not create playlist")
            return Flase

        LOGGER.info("isPlayingAudio: " + str(reply.value()))
        return reply.value()

    def isPlayingVideo(self):
        """
        Is playing Video
        """
        res = self.appbinder.call(QDBus.Block, 'mediaplayer_isplayingvideo')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not create playlist")
            return Flase

        LOGGER.info("isPlayingVideo: " + str(reply.value()))
        return reply.value()

    def getPlayingFile(self):
        """
        Return the playing file Name
        """
        res = self.appbinder.call(QDBus.Block, 'mediaplayer_filename')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not create playlist")
            return None

        LOGGER.info("playing filename: " + str(reply.value()))
        return reply.value()

    def getPlayingTime(self):
        """
        Return the playing Time position
        """
        res = self.appbinder.call(QDBus.Block, 'mediaplayer_streamposition')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not create playlist")
            return -1

        LOGGER.info("Stream Position: " + str(reply.value()))
        return reply.value()

    def seekTime(self, seekTime):
        """
        Time to seek as fractional seconds
        """
        self.appbinder.call(QDBus.Block, 'mediaplayer_seek', seekTime)

    def setSubtitles(self, subtitleFile):
        """
        Set subtitle file
        """

        self.appbinder.call(QDBus.Block, 'mediaplayer_setsubsfile', seekTime)

    def getTotalTime(self):
        """
        Returns the total time of the current playing media in seconds.
        """

        res = self.appbinder.call(QDBus.Block, 'mediaplayer_streamlength')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not create playlist")
            return -1

        LOGGER.info("stream length: " + str(reply.value()))
        return reply.value()


class PlayList(object):
    """
    Play List class
    To create and edit a playlist which can be handled by the player.
    """

    def __init__(self):
        self.appbinder = __binder__.getComHdlr()

    def add(self, url, item=None, index=-1):
        """
         Adds a new file to the playlist.

         @param url       string or unicode - filename or url to add.
         @param item      [opt] ItemUi - used with setInfo() to set different infolabels.
         @param index     [opt] integer - position to add playlist item. (default=end)

         @note You can use the above as keywords for arguments and skip certain optional arguments.
               Once you use a keyword, all following arguments require the keyword.
        """
        item_dict = {'url': url}

        if item:
            item_dict['label'] = item.label
            item_dict['subs'] = ','.join(item.subtitles)
            item_dict['IsPlayable'] = 'true' if item.IsPlayable else 'false'
            item_dict['isFolder'] = 'false'
            item_dict['path'] = item.path

            item_dict.update(item.artdict)
            item_dict.update(item.properties)
            item_dict.update(item.streamInfo)
            item_dict.update(item.metaInfo)
            item_dict.update(item.contextMenuItems)

        argument = QDBusArgument()
        argument.beginMap(QVariant.String, QVariant.String);
        for key, value in item_dict.iteritems():
            argument.beginMapEntry();
            argument.add(key, QVariant.String)
            strval = value if type(value) is str or type(value) is unicode else str(value)
            argument.add(strval, QVariant.String)
            argument.endMapEntry();
        argument.endMap();

        self.appbinder.call(QDBus.NoBlock,
                            'add_playlist_element',
                            argument,
                            index)
        return True

    def remove(self, index):
        """
        Remove item from play list
        """
        self.appbinder.call(QDBus.NoBlock,
                            'remove_playlist_element',
                            index)
        return True

    def clear(self):
        """
        Clear all items in the playlist.
        """
        self.appbinder.call(QDBus.NoBlock, 'clear_playlist')
        return True

    def size(self):
        """
        Returns the total number of PlayListItems in this playlist.
        """
        res = self.appbinder.call(QDBus.Block, 'get_playlist_size')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not get playlist size")
            return -1

        LOGGER.info("playlist size: " + str(reply.value()))
        return reply.value()

    def shuffle(self, index=-1):
        """
        Shuffle the playlist.
        """
        self.appbinder.call(QDBus.NoBlock, 'shuffle_playlist', index)

    def unshuffle(self):
        """
        Unshuffle the playlist.
        """
        self.appbinder.call(QDBus.NoBlock, 'unshuffle_playlist')

    def getPosition(self):
        """
        Returns the position of the current item in this playlist.
        """
        res = self.appbinder.call(QDBus.Block, 'get_playlist_position')
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Could not get playlist position")
            return -1

        LOGGER.info("playlist current position: " + str(reply.value()))
        return reply.value()

