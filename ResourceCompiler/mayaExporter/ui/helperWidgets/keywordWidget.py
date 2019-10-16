
#--coding: utf-8 --

from PySide2 import QtWidgets,QtCore,QtGui

from ui import fileActionUi
from ui.helperWidgets import pathWidget
from ui.helperWidgets import spinWidget


class KeywordWidget(QtWidgets.QWidget):
    __header_labels = ["key", "value"]
    def __init__(self, parent=None):
        QtWidgets.QWidget.__init__(self, parent)

        m_mainLayout = QtWidgets.QHBoxLayout()
        m_mainLayout.setAlignment(QtCore.Qt.AlignTop)

        self.m_view = QtWidgets.QTableView()
        self.m_model = QtGui.QStandardItemModel()
        self.m_view.setModel(self.m_model)

        self.m_actions=[]
        self.create_toolbar()

        m_mainLayout.addWidget(self.m_toolbar)
        m_mainLayout.addWidget(self.m_view)
        m_mainLayout.setMargin(0)
        m_mainLayout.setSpacing(0)
        self.setLayout(m_mainLayout)
        self.m_model.setHorizontalHeaderLabels(self.__header_labels)



    def create_toolbar(self):
        self.m_toolbar = QtWidgets.QToolBar()
        self.m_toolbar.setIconSize(QtCore.QSize(30,30))
        self.m_toolbar.setOrientation(QtCore.Qt.Vertical)

        #Add delete - first in array of actions, index is later used to setVisible
        action = QtWidgets.QAction("❌",self)
        action.setToolTip("Delete selected")
        action.setVisible(True)
        self.m_toolbar.addAction(action)
        self.m_actions.append(action)
        action.triggered.connect(self.onRemoveSelected)

        action = QtWidgets.QAction("➕",self)
        action.setToolTip("Add")
        action.setVisible(True)
        self.m_toolbar.addAction(action)
        self.m_actions.append(action)
        action.triggered.connect(self.onAdd)



        #connect(action, &QAction::triggered, this, &Test_widget::on_remove_selected)
    
    def onRemoveSelected(self):
        index = self.m_view.currentIndex().row()
        self.m_model.removeRow(index)

    def onAdd(self):
    
        key= QtGui.QStandardItem("")
        value= QtGui.QStandardItem("")
        self.m_model.appendRow([key,value])

    def addItem(self, k,v):
        key= QtGui.QStandardItem(k)
        value= QtGui.QStandardItem(v)
        self.m_model.appendRow([key,value])
