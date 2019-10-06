"""
This class holds a pathPicker widget
"""

from PySide2 import QtWidgets, QtCore

from ui.workFile import path_form


class PathWidget(QtWidgets.QWidget, path_form.Ui_path_form):
    """
    This class define a widget with a lineedit and a button 
    to load a path in the line edit
    """
    ##custom signal used when the text is changed
    textChanged = QtCore.Signal()

    def __init__(self, parent=None):
        """
        This is the constructor
        @param parent: the parent of the widget
        """
        QtWidgets.QWidget.__init__(self,parent)
        self.setupUi(self)
        ##Holds the type of extension we want to filter for
        self.file_extension = "*.*"

        #attach the file browser event
        self.pickPB.clicked.connect(self.pick_file)
        #emit text changed everytime it happens
        self.pathLE.textChanged.connect(self.emit_text_changed)

    def set_label_text(self, text):
        """
        This function set the label text
        """

        self.label.setText(text)

    def pick_file(self):
        """
        This fucntion fires up the browser for picikng the file
        """
        fileName = QtWidgets.QFileDialog.getOpenFileName(self,
                    "Open Session", __file__, 
                    "Session Files ({x})".format( \
                        x = self.file_extension))
        
        self.pathLE.setText(fileName[0])
        self.textChanged.emit()

    @property
    def path(self):
        """
        Decorator getter returning the current path of the widget
        by extracting it from the line edit widget
        """
        return str(self.pathLE.text())

    @path.setter
    def path(self, value):
        """
        Decorator setter for the attribute path, sets the given path
        in the line edit widget
        """
        self.pathLE.setText(value)
        
    def emit_text_changed(self, *args):
        """
        Function used to trigget the textChanged signal,
        this function exists in order to be able to 
        automatically bypass all the input arguments we dont 
        need for the signal
        """
        self.textChanged.emit()


