# builtin_tests.py
import threading

import time
import kvalui
# import kval


class KvalStoreDaemon(threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self, daemon=True)
        self.stopped = False

    def stop(self):
        self.stopped = True

    def run(self):
        while not self.stopped:
            print('_service_::run', 'WAITING For stop', 1)
            time.sleep(.5)


def green(s):
    return '\033[1;32m%s\033[m' % s


def yellow(s):
    return '\033[1;33m%s\033[m' % s


def red(s):
    return '\033[1;31m%s\033[m' % s


def log_exit(*m):
    print(red("ERROR:"), *m)
    exit(1)


def test_wrapper(fn):

    def inner(*args, **kwargs):
        print("Start Tests:", fn.__name__)
        res_ = fn(*args, **kwargs)
        if not res_:
            print(red("FAIL"), fn.__name__)
        else:
            print(green("PASS"), fn.__name__)

        return res_

    return inner


@test_wrapper
def alert_test():
    """
    Test Alert builtin display
    """
    try:
        print("Start: 'Alert' display default constructor...")
        _alert = kvalui.Alert()
        _alert.type = kvalui.AlertType.Info
        _alert.header = "This is an Information alert"
        _alert.body = "This is my application body alert"
        _alert.display()
    except kvalui.KvalUiException as ex:
        print(red("FAIL"), "alert_test", ": Excepted")
        return False

    try:
        print("Start: 'Alert' display overloaded constructor...")
        _alert = kvalui.Alert(kvalui.AlertType.Error, "Error", "Error occured", timeout=10)
        _alert.display()
    except kvalui.KvalUiException as ex:
        print(red("FAIL"), "alert_test", ": Excepted")
        return False

    return True


@test_wrapper
def simple_dialog_test():
    """
    Test Box builtin display
    """
    try:
        print("Start: 'SimpleDialog' display default constructor...")
        _box = kvalui.SimpleDialog()
        _box.type = kvalui.BoxType.Singlebutton
        _box.title = "Test Application Ok Box Modal"
        _box.header = "Header Subject: "
        _box.body = "This is a test box modal with an only ok button..."
        button = kvalui.Button()
        button.id = 0
        button.value = "Ok"
        _box.add_button(button)
        print("Box Display...")
        reply_ = _box.display()
        if reply_ == -1 or reply_ is None:
            return False
    except kvalui.KvalUiException as ex:
        print(red("FAIL"), str(ex))
        return False
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    try:
        print("Start: 'SimpleDialog' display overloaded constructor...")
        _box = kvalui.SimpleDialog(type=kvalui.BoxType.Multibutton,
                                   title="Box Unit tests",
                                   header="Test box header:",
                                   body="This is a box body with two buttons, yes and no...")
        _box.set_buttons([kvalui.Button(0, "Yehhhh.."), kvalui.Button(1, "Nope !")])

        print("Box 2 Display...")
        reply_ = _box.display()
        if reply_ == -1 or reply_ is None:
            return False
    except kvalui.KvalUiException as ex:
        print(red("FAIL"), str(ex))
        return False
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    try:
        print("Start: 'SimpleDialog' display overloaded - Fail trigger...")
        _box = kvalui.SimpleDialog(type=kvalui.BoxType.Multibutton,
                                   title="Box Unit tests",
                                   header="Test box header:",
                                   body="This is a box body with two buttons, yes and no...")

        print("Box 3 Display, should fails here ...")
        reply_ = _box.display()
        if reply_ == -1 or reply_ is None:
            return False
    except kvalui.KvalUiException as ex:
        print(str(ex))
        return True
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    return True


@test_wrapper
def progress_dialog_test():
    """
    Test progress Box builtin display
    """
    try:
        print("Start: 'Progress Box' display default constructor...")
        _progress_box = kvalui.ProgressDialog()
        _progress_box.type = kvalui.BoxType.Singlebutton
        _progress_box.title = "Test application progress box modal"
        _progress_box.header = "Header Test: "
        _progress_box.body = "This is a test progress box modal at 0 % ..."
        button = kvalui.Button()
        button.id = 0
        button.value = "Cancel"
        _progress_box.add_button(button)

        print("Box Display...")
        _progress_box.display()

        for update_step in range(1, 6):
            time.sleep(.5)
            if _progress_box.is_aborted():
                print("Aborted box, stop updating...")
                break

            print("ProgressDialog Update...")
            _progress_box.update(update_step*20, f"This is a test progress box modal at {update_step*20} % ...")

        _progress_box.close()

    except kvalui.KvalUiException as ex:
        print(red("FAIL"), str(ex))
        return False
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    try:
        print("Start: 'Box' display overloaded constructor...")
        _progress_box = kvalui.ProgressDialog(type=kvalui.BoxType.Singlebutton,
                                              title="Abort_Test",
                                              header="Progress Box 2:",
                                              body="Box 2 in progress 0 %...")
        _progress_box.add_button(kvalui.Button(0, "Yehhhh.."))

        print("Box 2 Display...")
        _progress_box.display()
        for update_step in range(1, 6):
            time.sleep(.5)
            if _progress_box.is_aborted():
                print("Aborted box, stop updating...")
                break

            print("ProgressDialog Update...")
            _progress_box.update(update_step*20, f"This is a test progress box modal at {update_step*20} % ...")

        _progress_box.close()

    except kvalui.KvalUiException as ex:
        print(red("FAIL"), str(ex))
        return False
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    return True


@test_wrapper
def input_dialog_test():
    """
    Test progress Box builtin display
    """
    try:
        print("Start: 'Input Dialog List' display default constructor...")
        _input_diag = kvalui.InputDialog()
        _input_diag.type = kvalui.InputType.Inputlist
        _input_diag.title = "Test application input list modal"
        _input_diag.header = "Choose one of the entries: "

        _lst = kvalui.EntryList()
        for i in range(1, 11):
            _lst.append(kvalui.Entry(i, f"choice {i}"))

        print("Call input List...")
        reply_ = _input_diag.input_list(_lst)
        if reply_ == -1 or reply_ is None:
            return False
        print("Got a Reply:", reply_)

    except kvalui.KvalUiException as ex:
        print(red("FAIL"), str(ex))
        return False
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    try:
        print("Start: 'Input Dialog File Browsing' display default constructor...")
        _input_diag = kvalui.InputDialog()
        _input_diag.type = kvalui.InputType.Inputfilebrowsing
        _input_diag.title = "Test application input filebrowsing modal"
        _input_diag.header = "Choose the file: "

        print("Call input FileBrowsing...")
        reply_ = _input_diag.input_filebrowser(path="/storage/.kval/userdata")
        if reply_ == -1 or reply_ is None:
            return False

        print("Got a Reply:", reply_)

    except kvalui.KvalUiException as ex:
        print(red("FAIL"), str(ex))
        return False
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    try:
        print("Start: 'Input Dialog Keyboard' display default constructor...")
        _input_diag = kvalui.InputDialog()
        _input_diag.type = kvalui.InputType.Inputkeyboard
        _input_diag.title = "Test application input keyboard modal"

        print("Call input Keyboard...")
        reply_ = _input_diag.input_keyboard("Please type something...")
        if reply_ == -1 or reply_ is None:
            return False

        print("Got a Reply:", reply_)

    except kvalui.KvalUiException as ex:
        print(red("FAIL"), str(ex))
        return False
    except Exception as ex:
        print(red("FAIL"), str(ex))
        return False

    return True


if __name__ == "__main__":
    daemon = KvalStoreDaemon()
    daemon.start()

    print("Start builtin tests...")

    alert_test()
    simple_dialog_test()
    progress_dialog_test()
    input_dialog_test()

    daemon.stop()
    daemon.join()

    print("End builtin tests...")