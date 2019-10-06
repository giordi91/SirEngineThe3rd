"""
This module contains an action used to execute a python code
"""
from maya import cmds

from actions import action


class ExportModelAction(action.FileAction):
    """
    This class implemnts an action used run python code
    action_type = "ExportModelAction"
    """
    def __init__(self, path="", exportSkin=False):
        action.FileAction.__init__(self,path)
        self.exportSkin = exportSkin

    def execute(self):
        """
        Exports the model using the exporter function
        """
        

def get_action():
    """
    This function returns an instance of the action used during load time
    """
    return ExportModelAction()
