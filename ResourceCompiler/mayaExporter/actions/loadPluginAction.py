"""
This module contains an action used to load a plugin in maya
"""

from actions import action
from maya import cmds


class LoadPluginAction(action.FileAction):
    """
    This class implemnts an action used to load a plugin in maya
    """
    def execute(self):
        """
        This function execute the action and loads
        the given plugin in maya
        """
        cmds.loadPlugin(self.path)

def get_action():
    """
    This function returns an instance of the action used during load
    time
    """
    return LoadPluginAction()
