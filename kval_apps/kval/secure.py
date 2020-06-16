# secure.py

from PyQt5.QtDBus import QDBus, QDBusReply
from kval_app_binder import getBinder

__binder__ = getBinder()

__all__ = ["Crypt"]


class Crypt:
    def __init__(self):
        self.appbinder = __binder__.getComHdlr()
        self.__aborted = False
        self.wait_time = 0

    def decrypt(self, sig, key, src, dst, wait_time=0):
        """
        Decrypt the provided file
            wait_time in millisec: must be greater than 100ms
        """
        self.wait_time = wait_time if (wait_time > 100) else 0
        self.appbinder.call(QDBus.NoBlock, 'decrypt_cmd', sig, key, src, dst)

        return self.check_reply()

    def check_reply(self):
        """
        Check decrypt reply
        """
        wait_time_max = (self.wait_time // 100) if (self.wait_time > 100) else 0
        wait_time_counter = 0
        while True:
            res = self.appbinder.call(QDBus.Block, 'get_decrypt_reply')
            wait_time_counter >= wait_time_max
            if (wait_time_max > 0):
                wait_time_counter = wait_time_counter + 1

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
