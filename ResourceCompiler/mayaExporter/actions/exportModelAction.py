"""
This module contains an action used to execute a python code
"""
import os

from maya import cmds

from actions import action
from workers import meshExporter
from workers import skinclusterExporter 


class ExportModelAction(action.FileAction):
    """
    This class implemnts an action used run python code
    action_type = "ExportModelAction"
    """
    def __init__(self, path="", name= "", exportSkin=False, skinPath=""):
        action.FileAction.__init__(self,path)
        self.exportSkin = exportSkin
        self.skinPath = skinPath 
        self.meshName = name
        

    def execute(self, basePath):
        """
        Exports the model using the exporter function
        """

        if not os.path.exists(basePath + os.path.dirname(self.path)):
            os.makedirs(basePath + os.path.dirname(self.path))
        meshExporter.export_obj(self.meshName, basePath + self.path + ".obj")

        if self.exportSkin and self.skinPath:
            #find the skin cluster
            conns = cmds.listConnections(self.meshName + ".inMesh",s=1,d=0)
            skinNode = None
            for conn in conns:
                if cmds.nodeType(conn) == "skinCluster":
                    skinNode = conn
                    break
            if not skinNode:
                cmds.error("Could not find skin cluster on mesh {0}".format(self.meshName))
                return
            
            #we need to find the root, simple assumption will be the joint
            #which parent is not a joint
            #so we need to get all the joints
            matricesCount = cmds.getAttr(skinNode + ".matrix", size=1)
            skeletonRoot = None
            for i in range(0,matricesCount):
                conns = cmds.listConnections(skinNode + ".matrix[{0}]".format(i))
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
            
            if not os.path.exists(basePath + os.path.dirname(self.skinPath)):
                os.makedirs(basePath + os.path.dirname(self.skinPath))
            outSkinPath = basePath + self.skinPath + ".json"
            print("Saving skin with:\n root {root} \n skinNode {skin} \n skinPath {skinPath} ".format(root=skeletonRoot,
            skin=skinNode,skinPath=outSkinPath ))
            skinclusterExporter.export_skin(skeletonRoot, skinNode, 
                            outSkinPath, self.meshName, None, False)

    def initialize(self, rootNode):
        #calling base class for basic setup
        action.FileAction._createMetaNode(self,rootNode)

        #creating attributes needed
        cmds.addAttr(self._node, sn="exs",ln="exportSkin",at="bool")
        cmds.addAttr(self._node, sn="skp",ln="skinPath",dt="string")
        cmds.addAttr(self._node, sn="men",ln="meshName",dt="string")

    def initializeFromNode(self, node):
        self._node = node
        action.FileAction.initializeFromNode(self,node)

        self.exportSkin = cmds.getAttr(self._node +".exportSkin")
        self.skinPath = cmds.getAttr(self._node +".skinPath")
        self.meshName= cmds.getAttr(self._node +".meshName")

    def save(self):
        """
        This method is used to back whatever data we have in the 
        variables into the node such that will persist in the scene
        """ 
        action.FileAction.save(self)
        cmds.setAttr(self._node + ".exs", self.exportSkin)
        cmds.setAttr(self._node + ".skp", self.skinPath, type="string")
        cmds.setAttr(self._node + ".men", self.meshName, type="string")




def get_action():
    """
    This function returns an instance of the action used during load time
    """
    return ExportModelAction()
