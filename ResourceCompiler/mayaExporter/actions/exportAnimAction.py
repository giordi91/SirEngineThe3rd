"""
This module contains an action used to execute a python code
"""
import os

from maya import cmds

from actions import action
from workers import animationExporter 


class ExportAnimAction(action.FileAction):
    """
    This class implemnts an action used run python code
    action_type = "ExportAnimAction"
    """
    def __init__(self, path="", root="", skeletonName="",frameRate= 1.0/33.0):
        action.FileAction.__init__(self,path)
        self.root = root 
        self.skeletonName = skeletonName
        self.frameRate = frameRate
        self.startFrame = 0
        self.endFrame = 100

    def execute(self, basePath):
        """
        Exports the model using the exporter function
        """
        name = os.path.basename(self.path[:-1]) 
        if not os.path.exists(basePath + os.path.dirname(self.path)):
            os.makedirs(basePath + os.path.dirname(self.path))

        path = basePath + self.path + ".json" 
        animationExporter.save_anim(self.root,name,self.skeletonName,True, 
        self.startFrame,self.endFrame, path, self.frameRate)

    def initialize(self, rootNode):
        #calling base class for basic setup
        action.FileAction._createMetaNode(self,rootNode)

        #creating attributes needed
        cmds.addAttr(self._node, sn="rtt",ln="root",dt="string")
        cmds.addAttr(self._node, sn="skn",ln="skeletonName",dt="string")
        cmds.addAttr(self._node, sn="frr",ln="frameRate",at="double")
        cmds.addAttr(self._node, sn="sfr",ln="startFrame",at="long")
        cmds.addAttr(self._node, sn="efr",ln="endFrame",at="long")

    def initializeFromNode(self, node):
        self._node = node
        action.FileAction.initializeFromNode(self,node)

        self.root= cmds.getAttr(self._node +".rtt")
        self.skeletonName = cmds.getAttr(self._node + ".skn")
        self.frameRate= cmds.getAttr(self._node + ".frr")
        self.startFrame = cmds.getAttr(self._node + ".sfr")
        self.endFrame= cmds.getAttr(self._node + ".efr")

    def save(self):
        """
        This method is used to back whatever data we have in the 
        variables into the node such that will persist in the scene
        """ 
        action.FileAction.save(self)
        cmds.setAttr(self._node + ".rtt", self.root, type="string")
        cmds.setAttr(self._node + ".skn", self.skeletonName, type="string")
        cmds.setAttr(self._node + ".frr", self.frameRate)
        cmds.setAttr(self._node + ".sfr", self.startFrame)
        cmds.setAttr(self._node + ".efr", self.endFrame)

def get_action():
    """
    This function returns an instance of the action used during load time
    """
    return ExportAnimAction()
