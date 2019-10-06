#This module holds the reload for the entire tool

import env_config
import json_utils
import session

from actions import reload_actions
from ui import reload_ui

modules = [env_config, json_utils, session, reload_actions,
            reload_ui]

reload_it_modules = [reload_ui, reload_actions]

def reload_it():
    """
    The method that kick the "recursive" reload
    of the tool
    """
    
    print("---------> Sarted reload of master reload")
    for sub_module in modules:
        reload(sub_module)
        print ("---> Reloading : " + str(sub_module.__name__))

    for sub_module in reload_it_modules:
        sub_module.reload_it()

    print ("---------> Ended reload of master reload")
