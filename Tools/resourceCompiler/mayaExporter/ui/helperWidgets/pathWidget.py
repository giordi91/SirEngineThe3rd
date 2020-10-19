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
    #custom signal used when the text is changed
    textChanged = QtCore.Signal()
    __shouldFileExists = True 
    __isFolder = False
    __force_end_slash = False

    def __init__(self, parent=None):
        """
        This is the constructor
        @param parent: the parent of the widget
        """
        QtWidgets.QWidget.__init__(self,parent)
        self.setupUi(self)
        #Holds the type of extension we want to filter for
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

    def set_should_file_exists(self, exits):
        self.__shouldFileExists = exits

    def set_is_directory(self, isFolder):
        print ("setting ",isFolder)
        self.__isFolder = isFolder 
    def set_force_end_slash(self, value):
        self.__force_end_slash = value

    def pick_file(self):
        """
        This fucntion fires up the browser for picikng the file
        """
        if not self.__isFolder:
            if self.__shouldFileExists :
                fileName = QtWidgets.QFileDialog.getOpenFileName(self,
                            "Open", __file__, 
                            "Files ({x})".format( \
                                x = self.file_extension))
            else:
                fileName = QtWidgets.QFileDialog.getSaveFileName(self,
                            "Save Session", __file__, 
                            "Files ({x})".format( \
                                x = self.file_extension))

            self.pathLE.setText(fileName[0])
        else :
            if self.__shouldFileExists :
                fileName = QtWidgets.QFileDialog.getExistingDirectory(self,
                            "Open", __file__ 
                            )
                self.pathLE.setText(fileName)
            else:
                raise ValueError("Cannot pick a not existing path")

        if(self.__force_end_slash):
            if not self.pathLE.text().endswith("/"):
                self.pathLE.setText(self.pathLE.text() + "/")
        self.textChanged.emit()

    @property
    def path(self):
        """
        Decorator getter returning the current path of the widget
        by extracting it from the line edit widget
        """
        if(self.__force_end_slash):
            if not self.pathLE.text().endswith("/"):
                self.pathLE.setText(self.pathLE.text() + "/")
        return str(self.pathLE.text())

    @path.setter
    def path(self, value):
        """
        Decorator setter for the attribute path, sets the given path
        in the line edit widget
        """
        if(self.__force_end_slash):
            if not self.pathLE.text().endswith("/"):
                self.pathLE.setText(self.pathLE.text() + "/")
        self.pathLE.setText(value)
        
    def emit_text_changed(self, *args):
        """
        Function used to trigget the textChanged signal,
        this function exists in order to be able to 
        automatically bypass all the input arguments we dont 
        need for the signal
        """
        self.textChanged.emit()


