__author__ = 'kval team'

import random
import urllib
import urlparse
import sys
import os
import time
import subprocess

from kval_app_binder import Binder
from kval_app_binder import DialogBox as Dialog
from kval_app_binder import ItemUi as Item
from kval_app_binder import Logging


class LiveSports():
    def __init__(self):
        self._appbinder = Binder(appid=u'kval.livesport.app')
        self._logging = Logging(appid='kval.livesport.app', level='INFO')

    def run(self):
        self._logging.log_info("name: " + self._appbinder.getAppInfo('name'))
        self._appbinder.displayMessageUI("Application Version",
                                         "%s : version %s" % (
                                             self._appbinder.getAppInfo('name'),
                                             self._appbinder.getAppInfo('version')))
