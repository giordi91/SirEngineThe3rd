"""
This module contains an action used to execute a python code
"""
import os

from maya import cmds

from actions import action
from workers import skinclusterExporter 


class ExportSkinAction(action.FileAction):
    """
    This class implemnts an action used run python code
    action_type = "ExportSkinAction"
    """
    def __init__(self, path="", skinNode=""):
        action.FileAction.__init__(self,path)
        self.skinNode = skinNode

    def execute(self, basePath):
        """
        Exports the model using the exporter function
        """

        #we need to find the root, simple assumption will be the joint
        #which parent is not a joint
        #so we need to get all the joints
        matricesCount = cmds.getAttr(self.skinNode + ".matrix", size=1)
        skeletonRoot = None
        for i in range(0,matricesCount):
            conns = cmds.listConnections(self.skinNode + ".matrix[{0}]".format(i))
            if not conns:
                continue
            nodeType = cmds.nodeType(conns[0])
            if nodeType != "joint":
                continue

            parentNode = cmds.listRelatives(conns[0], p=1)
            #no parent we consider it a root
            if not parentNode:
                skeletonRoot = conns[0]
                break
            #has a parent but is not a joint, considered a root
            if cmds.nodeType(parentNode) !=  "joint":
                skeletonRoot = conns[0]
                break

        if not skeletonRoot:
            cmds.error("Could not find skeleton root")
            return 
        
        #find mesh name
        conns = cmds.listConnections(self.skinNode+ ".outputGeometry[0]",s=0,d=1)
        if not conns:
            cmds.error("No mesh associated with skin cluster node: {0}".format(self.skinNode))
            return
        
        meshName = conns[0]
        
        outSkinPath = basePath + self.path+ ".json"
        if not os.path.exists(basePath + os.path.dirname(self.path)):
            os.makedirs(basePath + os.path.dirname(self.path))
        print("Saving skin with:\n root {root} \n skinNode {skin} \n skinPath {skinPath} ".format(root=skeletonRoot,
        skin=self.skinNode,skinPath=outSkinPath ))
        skinclusterExporter.export_skin(skeletonRoot, self.skinNode, 
                        outSkinPath, meshName, None, False)

    def initialize(self, rootNode):
        #calling base class for basic setup
        action.FileAction._createMetaNode(self,rootNode)

        #creating attributes needed
        cmds.addAttr(self._node, sn="skn",ln="skinNode",dt="string")

    def initializeFromNode(self, node):
        self._node = node
        action.FileAction.initializeFromNode(self,node)

        self.skinNode= cmds.getAttr(self._node +".skinNode")

    def save(self):
        """
        This method is used to back whatever data we have in the 
        variables into the node such that will persist in the scene
        """ 
        action.FileAction.save(self)
        cmds.setAttr(self._node + ".skn", self.skinNode, type="string")

def get_action():
    """
    This function returns an instance of the action used during load time
    """
    return ExportSkinAction()
