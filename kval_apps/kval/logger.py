# logger.py

import logging

__all__ = ["Logging"]


class Logging:
    """
    Logging system for applications
    """

    def __init__(self, appid="", level="ERROR"):
        self._logger = logging.getLogger(appid)
        logger_level = logging.ERROR
        if level is 'INFO':
            logger_level = logging.INFO
        elif level is 'DEBUG':
            logger_level = logging.DEBUG
        elif level is 'WARNING':
            logger_level = logging.WARNING
        elif level is 'NOTICE':
            logger_level = logging.NOTICE

        if not getattr(self._logger, 'handler_set', None):
            self._logger.setLevel(logger_level)
            # create console handler and set level to debug
            self.ch = logging.StreamHandler()
            self.ch.setLevel(logger_level)
            # create formatter
            self.formatter = \
                logging.Formatter('%(asctime)s #%(threadName)s [%(levelname)s] %(name)s::%(message)s')
            # add formatter to ch
            self.ch.setFormatter(self.formatter)
            # add ch to logger
            self._logger.addHandler(self.ch)
            self._logger.propagate = False
            self._logger.handler_set = True

    def log_warning(self, text):
        self._logger.warning(text)

    def log_error(self, text):
        self._logger.error(text)

    def log_notice(self, text):
        self._logger.debug(text)

    def log_debug(self, text):
        self._logger.debug(text)

    def log_info(self, text):
        self._logger.info(text)
