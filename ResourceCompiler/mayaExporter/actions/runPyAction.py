"""
This module contains an action used to execute a python code
"""
from maya import cmds

from actions import action


class RunPyAction(action.FileAction):
    """
    This class implemnts an action used run python code
    action_type = "RunPyAction"
    """
    def execute(self):
        """
        This function execute the action and exec
        the provided python code
        """
        exec self.path
        

def get_action():
    """
    This function returns an instance of the action used during load
    time
    """
    return RunPyAction()
