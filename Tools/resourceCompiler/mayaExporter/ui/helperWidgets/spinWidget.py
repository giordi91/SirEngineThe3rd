"""
This class holds a pathPicker widget
"""

from PySide2 import QtWidgets, QtCore

from ui.workFile import spin_widget_form 


class SpinWidget(QtWidgets.QWidget, spin_widget_form.Ui_path_form):
    """
    This class define a widget with a lineedit and a button 
    to load a path in the line edit
    """
    #custom signal used when the text is changed
    valueChanged = QtCore.Signal()
    __isDouble = False

    def __init__(self, isDouble =False, parent=None):
        """
        This is the constructor
        @param parent: the parent of the widget
        """
        QtWidgets.QWidget.__init__(self,parent)
        self.setupUi(self)

        self.__isDouble = isDouble
        if self.__isDouble:
            self.spin = QtWidgets.QDoubleSpinBox()
        else :
            self.spin = QtWidgets.QSpinBox()
        
        self.horizontalLayout_2.addWidget(self.spin)
        self.spin.valueChanged.connect(self.emit_value_changed)

    def set_label_text(self, text):
        """
        This function set the label text
        """
        self.label.setText(text)

    def set_precision(self, precisionCount):
        """
        This function set the label text
        """
        if self.__isDouble:
            self.spin.setDecimals(precisionCount)

    def set_min_max(self, minValue, maxValue):

        self.spin.setMinimum(minValue)
        self.spin.setMaximum(maxValue)

    @property
    def value(self):
        return self.spin.value()

    @value.setter
    def value(self, value):
        self.spin.setValue(value)
        
    def emit_value_changed(self, *args):
        """
        Function used to trigget the textChanged signal,
        this function exists in order to be able to 
        automatically bypass all the input arguments we dont 
        need for the signal
        """
        self.valueChanged.emit()


