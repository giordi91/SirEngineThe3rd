"""
This module holds a load plugin action ui
"""

from ui import fileActionUi

class RunPyFileActonUi(fileActionUi.FileActionUi):
    """
    @brief Actions for running a python script
    This class is used to execute a script in python, which is a string to be 
    evaluated
    """
    def __init__(self, session_instance, internal_action, session_ui, parent=None ):
        """
        This is the constructor
        @param session_instance: this is the session we want to display the data of
        @param internal_action: this is the acton class we want to work on with this
                                UI
        @param session_ui: this is is the instance of the sessionUi class
        @param parent: this is the parent widget holding the sub_ui
        """
        fileActionUi.FileActionUi.__init__(self, session_instance, internal_action, session_ui, parent)

        #Setupping the pathWidget
        self.pathWidget.set_label_text("py path:")
        self.pathWidget.file_extension = "*.py"

def get_ui(session_instance, internal_action, session_ui, parent):
    """
    This is a generic method that returns an istance 
    of the Ui in this module

    @param session_instance: this is the session we want to display the data of
    @param internal_action: this is the acton class we want to work on with this
                            UI
    @param session_ui: this is is the instance of the sessionUi class
    @param parent: this is the parent widget holding the sub_ui
    @return instance
    """
    return RunPyFileActonUi(session_instance, internal_action, session_ui, parent)