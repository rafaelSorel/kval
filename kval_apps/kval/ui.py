# ui.py

from kval_app_binder import getBinder, getBinderMonitor
from PyQt5.QtDBus import QDBus, QDBusArgument, QDBusReply

__binder__ = getBinder()
__monitor__ = getBinderMonitor()

__all__ = ["UiDisplayMode", "ItemUi", "DialogBox", "DialogProgress", "WindowSettingDiag"]


class UiDisplayMode:
    """
    Ui display mode when using
    the default UI template
    """
    UI_GRID_MODE = '1'
    UI_LIST_MODE = '2'
    UI_BOXWARE_MODE = '1'
    UI_RECTANGLEWARE_MODE = '2'
    UI_COVERWARE_MODE = '3'


class ItemUi:
    """
    Item property
    """

    def __init__(self, label=''):
        self.label = label
        self.uri = ''
        self.path = ''
        self.dualPath = {}
        self.artdict = {}
        self.properties = {}
        self.streamInfo = {}
        self.metaInfo = {}
        self.contextMenuItems = {}
        self.subtitles = []
        self.IsPlayable = False

    def setArt(self, artdict):
        """
        Sets the listitem's art

        @param values             dictionary - pairs of `{ label: value }`.
        - Some default art values (any string possible):
        | Label         | Type                                              |
        |:-------------:|:--------------------------------------------------|
        | thumb         | string - image filename
        | poster        | string - image filename
        | banner        | string - image filename
        | fanart        | string - image filename
        | clearart      | string - image filename
        | clearlogo     | string - image filename
        | landscape     | string - image filename
        | icon          | string - image filename
        """
        self.artdict = artdict

    def setProperty(self, key, value):
        """
        Sets an item property, similar to an infolabel.

        @param key            string - property name.
        @param value          string or unicode - value of property.

        @note Key is NOT case sensitive
        You can use the above as keywords for arguments
        and skip certain optional arguments.
        Once you use a keyword, all following arguments require the keyword.
        Some of these are treated internally by kval, such as the
        'StartOffset' property, which is the offset in seconds at which to
        start playback of an item.  Others may be used in the skin
        to add extra information, such as 'WatchedCount' for tvshow items
        -----------------------------------------------------------------------
        **Example:**
        ~~~~~~~~~~~~~
        item.setProperty('AspectRatio', '1.85 : 1')
        item.setProperty('StartOffset', '256.4')
        ~~~~~~~~~~~~~
        """
        self.properties[key] = value

    def setSubtitles(self, subs):
        """
        Sets subtitles for this listitem.

        @param subtitleFiles list with path to subtitle files
        **Example:**
        ~~~~~~~~~~~~~
        item.setSubtitles(['special://temp/example.srt',
                           'http://example.com/example.srt'])
        ~~~~~~~~~~~~~
        """
        self.subtitles = subs

    def addStreamInfo(self, type, values):
        """
        Add a stream with details.
        @param type              string - type of stream(video/audio/subtitle).
        @param values            dictionary - pairs of { label: value }.
        - Video Values:
        | Label         | Description                                     |
        |--------------:|:------------------------------------------------|
        | codec         | string (h264)
        | aspect        | float (1.78)
        | width         | integer (1280)
        | height        | integer (720)
        | duration      | integer (seconds)

        - Audio Values:
        | Label         | Description                                     |
        |--------------:|:------------------------------------------------|
        | codec         | string (dts)
        | language      | string (en)
        | channels      | integer (2)

        - Subtitle Values:
        | Label         | Description                                     |
        |--------------:|:------------------------------------------------|
        | language      | string (en)
        """
        self.streamInfo["type"] = type
        self.streamInfo.update(values)

    def setInfo(self, type, infoLabels=None):
        """
        Sets the listitem's infoLabels.

        @param type               string - type of
        @param infoLabels         dictionary - pairs of `{ label: value }`

        __Available types__
        | Command name | Description           |
        |:------------:|:----------------------|
        | video        | Video information
        | music        | Music information
        | pictures     | Pictures informantion

        @note To set pictures exif info, prepend `exif:` to the label. Exif values must be passed
              as strings, separate value pairs with a comma. <b>(eg. <c>{'exif:resolution': '720,480'}</c></b>
              See \ref kodi_pictures_infotag for valid strings.\n
              \n
              You can use the above as keywords for arguments and skip certain optional arguments.
              Once you use a keyword, all following arguments require the keyword.

        __General Values__ (that apply to all types):
        | Info label    | Description                                        |
        |--------------:|:---------------------------------------------------|
        | count         | integer (12) - can be used to store an id for later, or for sorting purposes
        | size          | long (1024) - size in bytes
        | date          | string (%d.%m.%Y / 01.01.2009) - file date

        __Video Values__:
        | Info label    | Description                                        |
        |--------------:|:---------------------------------------------------|
        | genre         | string (Comedy)
        | country       | string (Germany)
        | year          | integer (2009)
        | episode       | integer (4)
        | season        | integer (1)
        | top250        | integer (192)
        | setid         | integer (14)
        | tracknumber   | integer (3)
        | rating        | float (6.4) - range is 0..10
        | userrating    | integer (9) - range is 1..10 (0 to reset)
        | watched       | depreciated - use playcount instead
        | playcount     | integer (2) - number of times this item has been played
        | overlay       | integer (2) - range is `0..7`.  See \ref kodi_guilib_listitem_iconoverlay "Overlay icon types" for values
        | cast          | list (["Michal C. Hall","Jennifer Carpenter"]) - if provided a list of tuples cast will be interpreted as castandrole
        | castandrole   | list of tuples ([("Michael C. Hall","Dexter"),("Jennifer Carpenter","Debra")])
        | director      | string (Dagur Kari)
        | mpaa          | string (PG-13)
        | plot          | string (Long Description)
        | plotoutline   | string (Short Description)
        | title         | string (Big Fan)
        | originaltitle | string (Big Fan)
        | sorttitle     | string (Big Fan)
        | duration      | integer (245) - duration in seconds
        | studio        | string (Warner Bros.)
        | tagline       | string (An awesome movie) - short description of movie
        | writer        | string (Robert D. Siegel)
        | tvshowtitle   | string (Heroes)
        | premiered     | string (2005-03-04)
        | status        | string (Continuing) - status of a TVshow
        | set           | string (Batman Collection) - name of the collection
        | imdbnumber    | string (tt0110293) - IMDb code
        | code          | string (101) - Production code
        | aired         | string (2008-12-07)
        | credits       | string (Andy Kaufman) - writing credits
        | lastplayed    | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
        | album         | string (The Joshua Tree)
        | artist        | list (['U2'])
        | votes         | string (12345 votes)
        | path          | string (/home/user/movie.avi)
        | trailer       | string (/home/user/trailer.avi)
        | dateadded     | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
        | mediatype     | string - "video", "movie", "tvshow", "season", "episode" or "musicvideo"
        | dbid          | integer (23) - Only add this for items which are part of the local db. You also need to set the correct 'mediatype'!

        __Music Values__:
        | Info label               | Description                                        |
        |-------------------------:|:---------------------------------------------------|
        | tracknumber              | integer (8)
        | discnumber               | integer (2)
        | duration                 | integer (245) - duration in seconds
        | year                     | integer (1998)
        | genre                    | string (Rock)
        | album                    | string (Pulse)
        | artist                   | string (Muse)
        | title                    | string (American Pie)
        | rating                   | float - range is between 0 and 10
        | userrating               | integer - range is 1..10
        | lyrics                   | string (On a dark desert highway...)
        | playcount                | integer (2) - number of times this item has been played
        | lastplayed               | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
        | mediatype                | string - "music", "song", "album", "artist"
        | listeners                | integer (25614)
        | musicbrainztrackid       | string (cd1de9af-0b71-4503-9f96-9f5efe27923c)
        | musicbrainzartistid      | string (d87e52c5-bb8d-4da8-b941-9f4928627dc8)
        | musicbrainzalbumid       | string (24944755-2f68-3778-974e-f572a9e30108)
        | musicbrainzalbumartistid | string (d87e52c5-bb8d-4da8-b941-9f4928627dc8)
        | comment                  | string (This is a great song)

        __Picture Values__:
        | Info label    | Description                                        |
        |--------------:|:---------------------------------------------------|
        | title         | string (In the last summer-1)
        | picturepath   | string (`/home/username/pictures/img001.jpg`)
        | exif*         | string (See \ref kodi_pictures_infotag for valid strings)
        -----------------------------------------------------------------------
        **Example:**
        ~~~~~~~~~~~~~
        item.setInfo('video', { 'genre': 'Comedy' })
        ~~~~~~~~~~~~~
        """
        self.metaInfo['type'] = type
        if infoLabels:
            self.metaInfo.update(infoLabels)

    def addContextMenuItems(self, items, replaceItems=False):
        """
        Adds item(s) to the context menu for media lists.

        @param items               list - [(label, action,)*]
                                          A list of tuples consisting
                                          of label and action pairs.
          - label           string or unicode - item's label.
          - action          string or unicode - any built-in function to perform.

        @note You can use the above as keywords for arguments and skip certain
        optional arguments.
        Once you use a keyword, all following arguments require the keyword.
        """
        try:
            self.contextMenuItems = dict(('CtxMenu_' + key, value) for key, value in items)
        except:
            LOGGER.error("Something went wrong extracting contextMenu items !!")
            pass

    def setPath(self, path=''):
        """
        Sets the listitem's path.

        @param path           string or unicode - path,
                              activated when item is clicked.
        @note You can use the above as keywords for arguments.
        """
        self.path = path

    def setDualPath(self, dualPath={}):
        """
        Sets the listitem's dual stream path.

        dualPath           dict - path,
                           audioUri member: audio stream path
                           videoUri member: video stream path
        """
        self.dualPath = dualPath


class DialogProgress:
    """
    Dialog progress dialog
    """

    def __init__(self):
        self.appbinder = __binder__.getComHdlr()
        self.header = ""
        self.text = ""
        self.abort = False

    def create(self, header, text):
        self.header = header
        self.text = text
        self.appbinder.call(QDBus.NoBlock,
                            'diag_progress_create',
                            header,
                            text)

    def update(self, position, text=''):
        """
        Update Diag box
        """
        if self.abort: return

        res = self.appbinder.call(QDBus.Block,
                                  'diag_progress_update',
                                  position,
                                  text)
        reply = QDBusReply(res)
        if not reply.isValid():
            LOGGER.error("Reply not valid from kval")
            return None

        LOGGER.debug("abort reply: " + str(reply.value()))
        if reply.value():
            self.abort = True

    def iscanceled(self):
        """
        Check User abort
        """
        return self.abort

    def close(self):
        """
        Update Diag box
        """
        self.appbinder.call(QDBus.NoBlock,
                            'diag_progress_close')


class DialogBox:
    """
    Dialog class for UI interaction
    """

    def __init__(self):
        self.appbinder = __binder__.getComHdlr()

    def yesNoBox(self, title, text, nolabel='No', yeslabel='Yes'):

        self.appbinder.call(QDBus.NoBlock,
                            'yesno_diag',
                            title,
                            text,
                            nolabel,
                            yeslabel)
        return self.check_yesno_reply()

    def okBox(self, title, text):
        """
        Ok box
        """
        self.appbinder.call(QDBus.NoBlock, 'ok_diag', title, text)

        return self.check_yesno_reply()

    def check_yesno_reply(self):
        """
        Check YES/NO reply
        """
        while True:
            res = self.appbinder.call(QDBus.Block, 'get_yesno_reply')

            reply = QDBusReply(res)
            if not reply.isValid():
                LOGGER.error("Reply not valid from kval")
                sleep(0.1)
                continue

            if reply.value() < 0:
                sleep(0.1)
                continue

            LOGGER.info("Reply: " + str(reply.value()))
            return True if reply.value() is 1 else False

    def input(self, title, default="", type=1, hidden=False):
        """
        Input Keyboard
        """
        self.appbinder.call(QDBus.NoBlock,
                            'input_keyboard',
                            title,
                            default,
                            hidden)

        while True:
            res = self.appbinder.call(QDBus.Block, 'input_has_reply')
            reply = QDBusReply(res)

            if not reply.isValid() or not reply.value():
                sleep(0.1)
                continue

            res = self.appbinder.call(QDBus.Block, 'get_input_keyboard_result')
            reply = QDBusReply(res)

            if not reply.isValid():
                LOGGER.error("Reply not valid from kval")
                sleep(0.1)
                continue

            LOGGER.info("Reply: " + str(reply.value()))
            return reply.value()

    def listInput(self, title, _items):
        """
        list Input
        """
        argument = QDBusArgument()
        argument.beginArray(QVariant.String);
        for item in _items:
            argument.add(item, QVariant.String)
        argument.endArray();

        self.appbinder.call(QDBus.NoBlock, 'input_list', title, argument)

        while True:
            res = self.appbinder.call(QDBus.Block, 'input_has_reply')
            reply = QDBusReply(res)

            if not reply.isValid() or not reply.value():
                sleep(0.1)
                continue

            res = self.appbinder.call(QDBus.Block, 'get_input_list_result')
            reply = QDBusReply(res)

            if not reply.isValid():
                LOGGER.error("Reply not valid from kval")
                sleep(0.1)
                continue

            LOGGER.info("Reply: " + str(reply.value()))
            return reply.value()


class WindowSettingDiag(object):
    """
    Wrapper of kval settings Diag app,
    """

    def __init__(self, appid=None):
        self.properties = {}
        self.__appid = appid
        self.appbinder = __binder__.getComHdlr()
        __monitor__.registerObj(self)

        self._registerWindow()

    def getAppId(self):
        return self.__appid

    def _registerWindow(self):
        self.appbinder.call(QDBus.NoBlock,
                            'register_window_setting_diag',
                            self.__appid)

    def endOfEntries(self, succeeded=True):
        LOGGER.info("endOfEntries...." + str(succeeded))
        self.appbinder.call(QDBus.NoBlock,
                            'end_of_entries',
                            self.__appid,
                            succeeded)

    def addMenuItemSetting(self, dictProperties):
        """
        """
        item_dict = {}
        item_dict.update(dictProperties)

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
                            'add_menu_item_setting',
                            self.__appid,
                            argument)

    def addConfigItemSetting(self, men, item, count):
        """
        add config Item Setting
        """
        LOGGER.debug("addItem List....")

        item_dict = {}
        item_dict.update(item)

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
                            'add_config_item_setting',
                            self.__appid,
                            men,
                            argument,
                            count)

    def setProperty(self, key, value):
        self.properties[key] = value

    def getProperty(self, key):
        self.properties.get(key, "")

    def onInit(self):
        raise NotImplementedError()

    def onAction(self, action):
        raise NotImplementedError()

    def onClick(self, controlID):
        raise NotImplementedError()

    def onUnload(self):
        raise NotImplementedError()

    def onFocus(self, controlID):
        raise NotImplementedError()
