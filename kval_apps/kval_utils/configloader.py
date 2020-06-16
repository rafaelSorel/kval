# configloader.py

import json
import os
from collections import namedtuple
from kval_exceptions import AppConfigException

__all__ = ["Dependency", "AppLauncher", "AppMeta", "AppConfig", "format_config"]

Dependency = namedtuple("Dependencies", "id version uri")
Dependency.__new__.__defaults__ = (None,) * len(Dependency._fields)

AppLauncher = namedtuple("AppLauncher", "main daemon update package")
AppLauncher.__new__.__defaults__ = (None,) * len(AppLauncher._fields)

AppMeta = namedtuple("AppMeta", "assets summary description disclaimer platform size")
AppMeta.__new__.__defaults__ = (None,) * len(AppMeta._fields)

AppConfig = namedtuple("AppConfig", "id name version category provider ui_type dependencies launcher meta")
AppConfig.__new__.__defaults__ = (None,) * len(AppConfig._fields)


def format_config(cfg_file):
    """
    Args:
        cfg_file: json app config file abs path
    Returns:
        config un-mutable object, based on namedtuple
    """
    if not os.path.exists(cfg_file):
        raise FileNotFoundError(f"Unable to find {cfg_file} !!")

    with open(cfg_file) as fp:
        json_dict = json.load(fp)

    _dependencies = json_dict.get("dependencies", [])
    _dependencies = [Dependency(**_dict) for _dict in _dependencies]
    json_dict["dependencies"] = _dependencies

    exec_ifs = json_dict.get("launcher", None)
    if not exec_ifs:
        raise AppConfigException(f"Unable to fetch {cfg_file} exec information!!")
    try:
        _launcher = AppLauncher(**exec_ifs)
    except TypeError as ex:
        raise AppConfigException(f"'launcher' malformation in {cfg_file}: {str(ex)}")

    json_dict["launcher"] = _launcher

    _meta_data = json_dict.get("meta", None)
    if not _meta_data:
        raise AppConfigException(f"Unable to fetch {cfg_file} meta information!!")
    try:
        _meta = AppMeta(**_meta_data)
    except TypeError as ex:
        raise AppConfigException(f"'meta' malformation in {cfg_file}: {str(ex)}")

    json_dict["meta"] = _meta

    try:
        app_cfg = AppConfig(**json_dict)
    except TypeError as ex:
        raise AppConfigException(f"malformation in {cfg_file}: {str(ex)}")

    return app_cfg


def _run_config_loader_test():
    """
    # Unit test
    """
    try:
        app_config = format_config("../kvalstore/appconfig.json")
        print("app_config: ", repr(app_config))
    except AppConfigException as ex:
        print("load json config except:", ex.message)
        pass


if __name__ == "__main__":
    _run_config_loader_test()
