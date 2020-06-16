# monitor.py

import os
from time import sleep
from kval_app_binder import getBinderMonitor

__monitor__ = getBinderMonitor()

__all__ = ["Monitor"]


HOME = os.environ['HOME']
HOME_SCRIPTS = os.environ['HOME_SCRIPTS']
APPS_PATH = os.path.join(HOME, 'apps')
APPS_SYSTEM_PATH = os.path.join(HOME_SCRIPTS, 'apps')
APPS_PATH_EXEC = os.path.join(HOME_SCRIPTS, 'apps')


class Monitor:
    """
    Monitoring of kval main proc
    """

    def __init__(self, appid):
        self.__aborted = False
        self.__appid = appid
        __monitor__.registerObj(self)

    def onAbortRequest(self):
        self.__aborted = True

    def abortRequested(self):
        """
        abort requested
        """
        return self.__aborted

    def getAppId(self):
        return self.__appid

    def waitForAbort(self, timeout=10):
        """
        wait for abort request
        timeout: in seconds
        """
        chunkedTimeout = 0.1
        chunk_number = timeout / chunkedTimeout
        while chunk_number > 0:
            if self.abortRequested():
                return True
            else:
                sleep(chunkedTimeout)
                chunk_number = chunk_number - 1

        return False
