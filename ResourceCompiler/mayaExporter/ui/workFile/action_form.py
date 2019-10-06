# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'actionUi.ui'
#
# Created: Sun Aug 31 23:38:15 2014
#      by: pyside-uic 0.2.15 running on PySide 1.2.2
#
# WARNING! All changes made in this file will be lost!

from PySide2 import QtCore, QtGui

class Ui_Action_form(object):
    def setupUi(self, Action_form):
        Action_form.setObjectName("Action_form")
        Action_form.resize(655, 130)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(Action_form.sizePolicy().hasHeightForWidth())
        Action_form.setSizePolicy(sizePolicy)
        Action_form.setMinimumSize(QtCore.QSize(0, 130))
        self.verticalLayout = QtGui.QVBoxLayout(Action_form)
        self.verticalLayout.setSpacing(2)
        self.verticalLayout.setContentsMargins(1, 1, 1, 1)
        self.verticalLayout.setObjectName("verticalLayout")
        self.mainGB = QtGui.QGroupBox(Action_form)
        self.mainGB.setMinimumSize(QtCore.QSize(0, 0))
        self.mainGB.setStyleSheet("")
        self.mainGB.setObjectName("mainGB")
        self.gridLayout = QtGui.QGridLayout(self.mainGB)
        self.gridLayout.setContentsMargins(2, 2, 2, 2)
        self.gridLayout.setSpacing(2)
        self.gridLayout.setObjectName("gridLayout")
        self.cntsF = QtGui.QFrame(self.mainGB)
        self.cntsF.setFrameShape(QtGui.QFrame.StyledPanel)
        self.cntsF.setFrameShadow(QtGui.QFrame.Raised)
        self.cntsF.setObjectName("cntsF")
        self.horizontalLayout_2 = QtGui.QHBoxLayout(self.cntsF)
        self.horizontalLayout_2.setSpacing(2)
        self.horizontalLayout_2.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.downPB = QtGui.QPushButton(self.cntsF)
        self.downPB.setMinimumSize(QtCore.QSize(24, 24))
        self.downPB.setMaximumSize(QtCore.QSize(24, 24))
        self.downPB.setStyleSheet("background-image: url(:/images/down.png);")
        self.downPB.setText("")
        self.downPB.setObjectName("downPB")
        self.horizontalLayout_2.addWidget(self.downPB)
        self.upPB = QtGui.QPushButton(self.cntsF)
        self.upPB.setMinimumSize(QtCore.QSize(24, 24))
        self.upPB.setMaximumSize(QtCore.QSize(24, 24))
        self.upPB.setStyleSheet("background-image: url(:/images/up.png);")
        self.upPB.setText("")
        self.upPB.setObjectName("upPB")
        self.horizontalLayout_2.addWidget(self.upPB)
        self.closePB = QtGui.QPushButton(self.cntsF)
        self.closePB.setMinimumSize(QtCore.QSize(24, 24))
        self.closePB.setMaximumSize(QtCore.QSize(24, 24))
        self.closePB.setStyleSheet("background-image: url(:/images/x.png);")
        self.closePB.setText("")
        self.closePB.setObjectName("closePB")
        self.horizontalLayout_2.addWidget(self.closePB)
        self.gridLayout.addWidget(self.cntsF, 0, 1, 1, 1)
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 0, 0, 1, 1)
        self.customF = QtGui.QFrame(self.mainGB)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.customF.sizePolicy().hasHeightForWidth())
        self.customF.setSizePolicy(sizePolicy)
        self.customF.setFrameShape(QtGui.QFrame.StyledPanel)
        self.customF.setFrameShadow(QtGui.QFrame.Raised)
        self.customF.setObjectName("customF")
        self.verticalLayout_2 = QtGui.QVBoxLayout(self.customF)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.gridLayout.addWidget(self.customF, 1, 0, 1, 2)
        self.verticalLayout.addWidget(self.mainGB)

        self.retranslateUi(Action_form)
        QtCore.QMetaObject.connectSlotsByName(Action_form)

    def retranslateUi(self, Action_form):
        Action_form.setWindowTitle(QtGui.QApplication.translate("Action_form", "Form", None, QtGui.QApplication.UnicodeUTF8))
        self.mainGB.setTitle(QtGui.QApplication.translate("Action_form", "XXX", None, QtGui.QApplication.UnicodeUTF8))

import masterReload_rc
