# kval_exceptions.py

__author__ = 'Kval Team'


class KvalException(Exception):
    def __init__(self, msg):
        Exception.__init__(self, msg)
        self._message = msg

    def __str__(self):
        return self._message

    @property
    def message(self):
        return self._message

    def get_message(self):
        return self._message


class InstallerException(KvalException):
    pass


class AppConfigException(KvalException):
    pass


class DatabaseException(KvalException):
    pass


class CancelException(KvalException):
    pass


class LauncherException(KvalException):
    pass
