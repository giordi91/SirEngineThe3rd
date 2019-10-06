"""
This module contains an action used to open a file in maya
"""
from actions import action
from maya import cmds


class OpenFileAction(action.FileAction):
    """
    This class implemnts an action used to open a file in maya
    """
    def execute(self):
        """
        This function execute the action and loads
        the given file in maya
        """
        cmds.file(self.path, f=1, open=1)

def get_action():
    """
    This function returns an instance of the action used during load
    time
    """
    return OpenFileAction()
