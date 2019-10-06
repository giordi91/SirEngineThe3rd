"""
This module holds a run py file action Ui
"""

from ui import fileActionUi
from PySide2 import QtWidgets

class ExportModelActionUi(fileActionUi.FileActionUi):
    """
    @brief Actions for running a .py file
    This class is used to execute a stand-alone py file
    """
    def __init__(self, session_instance, internal_action, session_ui, parent=None ):
        """
        This is the constructor
        @param session_instance: this is the session we want to display the data of
        @param internal_action: this is the action class we want to work on with this
                                UI
        @param session_ui: this is is the instance of the sessionUi class
        @param parent: this is the parent widget holding the sub_ui
        """
        fileActionUi.FileActionUi.__init__(self, session_instance, internal_action, session_ui, parent)

        #Setupping the pathWidget
        self.pathWidget.set_label_text("exportPath:")
        self.pathWidget.set_should_file_exists(False)
        self.skinCB = QtWidgets.QCheckBox("Export Skin")
        self.skinCB.stateChanged.connect(self.save)
        self.verticalLayout_2.addWidget(self.skinCB)

    def save(self):
        """
        This function stores the path of the file in the action
        """
        fileActionUi.FileActionUi.save(self)
        self.internal_action.exportSkin= self.skinCB.isChecked()

    def load(self):
        """
        This function loads the data from the action to 
        the pathWidget
        """
        fileActionUi.FileActionUi.load(self)
        self.skinCB.setChecked(self.internal_action.exportSkin)

def get_ui(session_instance, internal_action, session_ui, parent):
    """
    This is a generic method that returns an istance 
    of the Ui in this module

    @param session_instance: this is the session we want to display the data of
    @param internal_action: this is the acton class we want to work on with this
                            UI
    @param session_ui: this is is the instance of the sessionUi class
    @param parent: this is the parent widget holding the sub_ui
    """
    return ExportModelActionUi(session_instance, internal_action, session_ui, parent)