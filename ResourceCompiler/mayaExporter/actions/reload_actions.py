"""
This module is specialized for reloading a subset of modules
"""

from actions import action
from actions import runSystemAction
from actions import runPyAction
from actions import runPyFileAction
from actions import exportModelAction 



modules = [action, runSystemAction, runPyAction, runPyFileAction]

def reload_it():
    """
    This function reloads the modules stored in the modules array of 
    this module
    """
    print "------> Sarted reload of actions"
    for sub_module in modules:
        reload(sub_module)
        print "---> Reloading : " +str(sub_module.__name__)

    print "------> Ended reload of actions"
    