"""
This module contains an action used to run a system code , like bash or dos
"""
import subprocess
from actions import action



class RunSystemAction(action.FileAction):
    """
    This class implemnts an action used to run dos or bash commands etc
    """
    def execute(self):
        """
        This function execute the action and opens a new maya instance
        """
        subprocess.Popen([self.path])

def get_action():
    """
    This function returns an instance of the action used during load
    time
    """
    return RunSystemAction()
