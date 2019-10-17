"""
This module contains an action used to execute a python code
"""
import os

from maya import cmds

from actions import action
from workers import skeletonExporter 


class ExportSkeletonAction(action.FileAction):
    """
    This class implemnts an action used run python code
    action_type = "ExportSkeletonAction"
    """
    def __init__(self, path="", root=""):
        action.FileAction.__init__(self,path)
        self.root = root 

    def execute(self, basePath):
        """
        Exports the model using the exporter function
        """
        name = os.path.basename(self.path) 
        print (name)
        if not os.path.exists(basePath + os.path.dirname(self.path)):
            os.makedirs(basePath + os.path.dirname(self.path))
        skeletonExporter.save_skeleton(name,self.root, basePath + self.path + ".json")

    def initialize(self, rootNode):
        #calling base class for basic setup
        action.FileAction._createMetaNode(self,rootNode)

        #creating attributes needed
        cmds.addAttr(self._node, sn="rtt",ln="root",dt="string")

    def initializeFromNode(self, node):
        self._node = node
        action.FileAction.initializeFromNode(self,node)

        self.root= cmds.getAttr(self._node +".rtt")

    def save(self):
        """
        This method is used to back whatever data we have in the 
        variables into the node such that will persist in the scene
        """ 
        action.FileAction.save(self)
        cmds.setAttr(self._node + ".rtt", self.root, type="string")

def get_action():
    """
    This function returns an instance of the action used during load time
    """
    return ExportSkeletonAction()
