"""
This class holds the basic ui class for an action ui
"""

from PySide2 import QtWidgets, QtCore, QtWidgets
from functools import partial

import workFile.masterReload_rc

from ui.workFile import action_form


class ActionUi(QtWidgets.QWidget, action_form.Ui_Action_form):
    """
    @brief basic Actions Ui 
    This is a common base used for all the actions uis, 
    iplements the basic widget and buttons
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
        QtWidgets.QWidget.__init__(self,parent)
        #setupping the ui from the designer
        self.setupUi(self)
        #setting the title
        self.mainGB.setTitle( self.__class__.__name__)
        
        ##This attribute holds a pointer to 
        ##the current session class
        self.session = session_instance
        ##This attribute holds a pointer to the 
        ##action we are referring to
        self.internal_action = internal_action
        ##This attribute holds a pointer to the 
        ##sessionUi class that holds all the sub_ui
        self.session_ui = session_ui

        #connecting the close button signal
        self.closePB.clicked.connect(self.remove_widget)

        self.upPB.clicked.connect(partial(self.move_action,"up"))
        self.downPB.clicked.connect(partial(self.move_action,"down"))

    def remove_widget(self):
        """
        This functon removes this ui from te sessionUI
        instance
        """
        self.session.remove_action(self.internal_action)
        self.session_ui.update_ui()


    def exectue(self):
        """
        This function execute the action it refers
        to
        """
        self.internal_action.exectue()

    def move_action(self, direction):
        """
        This function moves the the action up and down
        which then will be reflected from the ui once the 
        data is updated
        @param direction: str, which direction to move our action,
                            accepted value : "up" , "down"
        """
        self.session_ui.swap_action(self.internal_action,direction)

    def save(self):
        """
        This function needs to be reimplemented and 
        defines how the data gets saved to the internal action
        """
        
        raise NotImplementedError()

    def load(self):
        """
        This function needs to be reimplemented and 
        defines how the data gets loaded from the internal action
        """
        raise NotImplementedError()



