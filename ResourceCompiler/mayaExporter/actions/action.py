"""
This module implementas the basic abstract type for actions
"""
from maya import cmds
import re

class Action(object):
    """
    This is the most abstract action, implements the basic needed method to load
    save and execute an action
    """
    _node = None
    __META_NODE_TYPE = "lightInfo"

    def __init__(self):
        """
        This is the constructor
        """
        pass

    def initialize(self, rootObject):
        """
        This is the method is used to setup the wiring
        of the action, whatever it might be 
        """
        self._createMetaNode(rootObject)

    def initializeFromNode(self, node):
        """
        reads the data stored in the node such data it can initialize the action
        this is an abstract method and should be reimplemented per action
        """
        raise NotImplementedError()

    def execute(self):
        """
        This is the abstract method used to execute the action
        """
        raise NotImplementedError()

    def remove(self,rootObject):
        """
        Unwires the action and patches it the others 
        """
        #need to find which index of the array the action is connected to
        plug = cmds.listConnections(self._node+".inRoot", s=1,d=0,p=1)[0]
        #extract the index
        #overkill regex, fight me!
        res = re.search("\[(\d+)\]",plug)
        if not res:
            raise ValueError()
        #this is the index we are connected to
        index = int(res.group(1))
        
        #we need to find how many connected objects we have
        total = len(cmds.getAttr(rootObject + ".actions")[0])

        #disconnecting the node
        cmds.disconnectAttr(rootObject + ".actions[" + str(index) + "]",self._node + ".inRoot")

        for i in range(index + 1,total):
            #first we need to check if index + 1 has connections
            conn = cmds.listConnections(rootObject+".actions[" + str(i) + "]",s=0,d=1,p=1)
            if(conn):
                cmds.connectAttr(rootObject + ".actions[" +str(i-1) + "]", conn[0], f=1)

        #deleting the node
        cmds.delete(self._node)

    def save(self):
        """
        This is the abstract method used to save the action
        """
        raise NotImplementedError()

    def _createMetaNode(self, rootObject):
        
        #not really sure what to do with the name of the node, does not really matter
        self._node = cmds.createNode(self.__META_NODE_TYPE, name = self.__class__.__name__) 
        cmds.addAttr(self._node,sn="atp", ln="actionType",dt="string")
        cmds.setAttr(self._node + ".atp", self.__class__.__name__, type="string")
        cmds.addAttr(self._node, sn = "irt", ln= "inRoot", at="message")
        index =  self._getFreeConnectionIndex(rootObject)
        cmds.connectAttr(rootObject + ".actions[" + str(index) + "]",self._node + ".irt" )

    def _getFreeConnectionIndex(self,rootObject):
        #here we are dealing with an attribute array due to ordering
        #we need to figure out what index we can get for connecting
        conn = cmds.getAttr(rootObject + ".actions", size=True )
        for i in range(0,conn):
            #check if is connected
            subConn = cmds.listConnections(rootObject + ".actions[{0}]".format(i),s=0,d=1)
            if subConn:
                continue
            return i
        
        return conn






class FileAction(Action):
    """
    This is an abstract implementation of a action dealing with a file ,
    it implements how to load and save the path
    """
    def __init__(self, path=""):
        """
        This is the constructor
        @param path: str, This is the path used by the action in the
                        execute method
        """
        Action.__init__(self)
        self.path = path


    def save(self):
        """
        This method is used to back whatever data we have in the 
        variables into the node such that will persist in the scene
        """ 
        print (self.path)
        cmds.setAttr(self._node + ".path", self.path,type="string")

    def _createMetaNode(self, rootObject):

        #calling the base class to get the basic configuration on a node
        #then adding extra attributes we have on top
        Action._createMetaNode(self,rootObject)
        cmds.addAttr(self._node,sn="pth",ln="path",dt="string")

    def initializeFromNode(self, node):
        """
        reads the data stored in the node such data it can initialize the action
        this is an abstract method and should be reimplemented per action
        """
        self.path = cmds.getAttr(node + ".path")