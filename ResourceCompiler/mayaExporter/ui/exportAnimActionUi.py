"""
This module holds a run py file action Ui
"""
from PySide2 import QtWidgets

from ui import fileActionUi
from ui.helperWidgets import pathWidget
from ui.helperWidgets import spinWidget 


class ExportAnimActionUi(fileActionUi.FileActionUi):
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
        self.pathWidget.set_force_end_slash(False)
        self.root= pathWidget.PathWidget()
        self.root.set_label_text("root: ")
        self.root.pickPB.hide()
        self.verticalLayout_2.addWidget(self.root)
        self.skeletonName= pathWidget.PathWidget()
        self.skeletonName.set_label_text("Skeleton name: ")
        self.skeletonName.pickPB.hide()
        self.verticalLayout_2.addWidget(self.skeletonName)
        self.frameRateSB = spinWidget.SpinWidget(True) 
        self.frameRateSB.value = 1.0/33.0
        self.frameRateSB.set_precision( 6)
        self.frameRateSB.set_label_text("framerate: ")
        self.verticalLayout_2.addWidget(self.frameRateSB)
        self.startFrameSB =spinWidget.SpinWidget() 
        self.startFrameSB.set_label_text("start frame: ")
        self.startFrameSB.set_precision(6)
        self.startFrameSB.set_min_max(-1000,1000)
        self.verticalLayout_2.addWidget(self.startFrameSB)
        self.endFrameSB= spinWidget.SpinWidget()
        self.endFrameSB.set_label_text("end frame: ")
        self.endFrameSB.set_min_max(-1000,1000)
        self.verticalLayout_2.addWidget(self.endFrameSB)


    def save(self):
        fileActionUi.FileActionUi.save(self)
        self.internal_action.root= self.root.path
        self.internal_action.frameRate = self.frameRateSB.value
        self.internal_action.skeletonName = self.skeletonName.path
        self.internal_action.startFrame = self.startFrameSB.value
        self.internal_action.endFrame = self.endFrameSB.value
        self.internal_action.save()

    def load(self):
        """
        This function loads the data from the action to 
        the pathWidget
        """
        fileActionUi.FileActionUi.load(self)
        self.root.path = self.internal_action.root
        self.skeletonName.path = self.internal_action.skeletonName
        self.frameRateSB.value = self.internal_action.frameRate
        self.startFrameSB.value = self.internal_action.startFrame
        self.endFrameSB.value = self.internal_action.endFrame

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
    return ExportAnimActionUi(session_instance, internal_action, session_ui, parent)