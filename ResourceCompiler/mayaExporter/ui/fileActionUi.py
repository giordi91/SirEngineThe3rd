"""
This module contains the ui for the loadPluginAction
"""

from ui import actionUi
from ui.helperWidgets import pathWidget


class FileActionUi(actionUi.ActionUi):
    """
    @brief basic ui based on a path to a file
    This function works as a base for many UIs that works
    with a path file
    """

    def __init__(self,session_instance, internal_action, session_ui, parent=None ):
        """
        This is the constructor
        @param session_instance: this is the session we want to display the data of
        @param internal_action: this is the acton class we want to work on with this
                                UI
        @param session_ui: this is is the instance of the sessionUi class
        @param parent: this is the parent widget holding the sub_ui
        """
        actionUi.ActionUi.__init__(self, session_instance, internal_action, session_ui, parent)

        ##Widget used to pick a path from disk
        self.pathWidget = pathWidget.PathWidget()
        self.verticalLayout_2.addWidget(self.pathWidget)

        #connect the signal of the widget to the save function
        self.pathWidget.textChanged.connect(self.save)
    
    def save(self):
        """
        This function stores the path of the file in the action
        """
    	self.internal_action.path = self.pathWidget.path

    def load(self):
        """
        This function loads the data from the action to 
        the pathWidget
        """
    	self.pathWidget.path = self.internal_action.path


