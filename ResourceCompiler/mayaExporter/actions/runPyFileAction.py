"""
This module contains an action used run a python file
"""
from actions import action
from maya import cmds


class RunPyFileAction(action.FileAction):
    """
    This class implemnts an action used to run a python file
    """
    def execute(self):
        """
        This function execute the action runs
        the given python file
        """
        execfile(self.path, locals(), locals())
        

def get_action():
    """
    This function returns an instance of the action used during load
    time
    """
    return RunPyFileAction()
