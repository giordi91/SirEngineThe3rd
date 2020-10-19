# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'actionUi.ui',
# licensing of 'actionUi.ui' applies.
#
# Created: Wed Oct 16 15:35:09 2019
#      by: pyside2-uic  running on PySide2 5.13.1
#
# WARNING! All changes made in this file will be lost!

from PySide2 import QtCore, QtGui, QtWidgets

class Ui_Action_form(object):
    def setupUi(self, Action_form):
        Action_form.setObjectName("Action_form")
        Action_form.resize(655, 69)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(Action_form.sizePolicy().hasHeightForWidth())
        Action_form.setSizePolicy(sizePolicy)
        Action_form.setMinimumSize(QtCore.QSize(0, 0))
        self.verticalLayout = QtWidgets.QVBoxLayout(Action_form)
        self.verticalLayout.setSpacing(0)
        self.verticalLayout.setContentsMargins(1, 1, 1, 1)
        self.verticalLayout.setObjectName("verticalLayout")
        self.mainGB = QtWidgets.QGroupBox(Action_form)
        self.mainGB.setMinimumSize(QtCore.QSize(0, 0))
        self.mainGB.setStyleSheet("")
        self.mainGB.setObjectName("mainGB")
        self.gridLayout = QtWidgets.QGridLayout(self.mainGB)
        self.gridLayout.setContentsMargins(1, 1, 1, 1)
        self.gridLayout.setHorizontalSpacing(1)
        self.gridLayout.setVerticalSpacing(0)
        self.gridLayout.setObjectName("gridLayout")
        self.cntsF = QtWidgets.QFrame(self.mainGB)
        self.cntsF.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.cntsF.setFrameShadow(QtWidgets.QFrame.Raised)
        self.cntsF.setObjectName("cntsF")
        self.horizontalLayout_2 = QtWidgets.QHBoxLayout(self.cntsF)
        self.horizontalLayout_2.setSpacing(0)
        self.horizontalLayout_2.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.downPB = QtWidgets.QPushButton(self.cntsF)
        self.downPB.setMinimumSize(QtCore.QSize(24, 24))
        self.downPB.setMaximumSize(QtCore.QSize(24, 24))
        self.downPB.setObjectName("downPB")
        self.horizontalLayout_2.addWidget(self.downPB)
        self.upPB = QtWidgets.QPushButton(self.cntsF)
        self.upPB.setMinimumSize(QtCore.QSize(24, 24))
        self.upPB.setMaximumSize(QtCore.QSize(24, 24))
        self.upPB.setObjectName("upPB")
        self.horizontalLayout_2.addWidget(self.upPB)
        self.closePB = QtWidgets.QPushButton(self.cntsF)
        self.closePB.setMinimumSize(QtCore.QSize(24, 24))
        self.closePB.setMaximumSize(QtCore.QSize(24, 24))
        self.closePB.setStyleSheet("")
        self.closePB.setObjectName("closePB")
        self.horizontalLayout_2.addWidget(self.closePB)
        self.gridLayout.addWidget(self.cntsF, 0, 1, 1, 1)
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.gridLayout.addItem(spacerItem, 0, 0, 1, 1)
        self.customF = QtWidgets.QFrame(self.mainGB)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.MinimumExpanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.customF.sizePolicy().hasHeightForWidth())
        self.customF.setSizePolicy(sizePolicy)
        self.customF.setFrameShape(QtWidgets.QFrame.StyledPanel)
        self.customF.setFrameShadow(QtWidgets.QFrame.Raised)
        self.customF.setObjectName("customF")
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(self.customF)
        self.verticalLayout_2.setSpacing(0)
        self.verticalLayout_2.setSizeConstraint(QtWidgets.QLayout.SetDefaultConstraint)
        self.verticalLayout_2.setContentsMargins(0, 0, 0, 0)
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        self.gridLayout.addWidget(self.customF, 1, 0, 1, 2)
        self.verticalLayout.addWidget(self.mainGB)

        self.retranslateUi(Action_form)
        QtCore.QMetaObject.connectSlotsByName(Action_form)

    def retranslateUi(self, Action_form):
        Action_form.setWindowTitle(QtWidgets.QApplication.translate("Action_form", "Form", None, -1))
        self.mainGB.setTitle(QtWidgets.QApplication.translate("Action_form", "XXX", None, -1))
        self.downPB.setText(QtWidgets.QApplication.translate("Action_form", "⏷", None, -1))
        self.upPB.setText(QtWidgets.QApplication.translate("Action_form", "⏶", None, -1))
        self.closePB.setText(QtWidgets.QApplication.translate("Action_form", "✖", None, -1))

