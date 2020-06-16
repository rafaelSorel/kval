#!/usr/bin/env python2.7

__author__ = 'Kval Team'

from os import uname
from distutils.version import LooseVersion
from hashlib import md5, sha1
import json
import urllib
from traceback import print_exc

from kval_app_binder import Binder
from kval_app_binder import DialogBox as Dialog
from kval_app_binder import Logging
from livesports import net

DEFAULT_STORE_ADDRESS = ""

UPDATE_STRINGS_MAP = {'app.update': 40001,
                      'app.update.problem': 40002,
                      'app.no.update': 40003,}
def startup():
    """
    Perform the update mechanism
    """
    logging = Logging(appid='kval.livesport.app', level='INFO')
    appbinder = Binder(appid=u'kval.livesport.app')
    _headers = {"Content-type": "application/x-www-form-urlencoded", 
                "Accept": "text/plain", 
                "User-Agent": 'kvaltvbox 1.0/4.0'}

    server_addr = appbinder.getSetting('kvalstore.address')

    params = urllib.urlencode({ 'name': sha1(md5(uname()[1]).hexdigest()).hexdigest(),
                                'action': 'GET_APP_INSTALL_INFO',
                                'appid': appbinder.getAppInfo('id')})

    url = 'https://' + server_addr + '/server?' + params
    logging.log_info('url: ' + str(url))
    try :
        #get response from server
        rsp = net.Net().http_GET(url, headers=_headers).content
        logging.log_info('rsp: ' + str(rsp))
        jsonData = json.loads(rsp)
    except:
        logging.log_error('Problem connection occured')
        print_exc()
        return

    #process server response and data
    app_info = None
    if jsonData['responseCode'] == 200:
        try :
            #parse data received in here
            app_info = jsonData['info']
        except:
            logging.log_error('Could not parse received data')
            dialog = Dialog()
            dialog.okBox("Update...",
                         "No update point available for <b>%s</b> !" % appbinder.getAppInfo('name'))
            return

    else :
        logging.log_error("Response code: " + jsonData['responseCode'])
        dialog = Dialog()
        dialog.okBox("Update...",
                     "No update point available for <b>%s</b> !" % appbinder.getAppInfo('name'))
        return


    if  LooseVersion(appbinder.getAppInfo('version')) <= LooseVersion(app_info['version']):
        logging.log_info("No update available !")
        dialog = Dialog()
        dialog.okBox("Update...",
                     "You have already the latest <b>%s</b> version %s." % \
                     (appbinder.getAppInfo('name'),
                      appbinder.getAppInfo('version')) )
        return

    appbinder.executebuiltin("InstallApp(%s, %s, %s)" % \
                (app_info["id"],
                 app_info["url"],
                 app_info["hash"] if "hash" in app_info else ""))

    return

