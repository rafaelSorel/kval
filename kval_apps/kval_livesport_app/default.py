#!/usr/bin/env python2.7
__author__ = 'kval team'

from livesports.runner import LiveSports

def startup():
    """
    Startup entry point
    """
    _livesports_ = LiveSports()
    _livesports_.run()
