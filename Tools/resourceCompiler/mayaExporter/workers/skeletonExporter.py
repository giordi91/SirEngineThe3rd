from maya import cmds
from maya.api import OpenMaya
import json





def get_skeleton_data(root):

    toProcess = cmds.listRelatives(root)
    children = []
    if toProcess:
        children = [c for c in toProcess  if cmds.nodeType(c) == "joint"]

    security = 2000
    counter = 0
    joints = [root]
    while( children):
        
        child = children[0]
        joints.append(child)

        next_children  =(cmds.listRelatives(child))
        if next_children:
            for nc in next_children:
                if cmds.nodeType(nc) == "joint":
                    children.append( nc)
        children = children[1:]

        counter +=1

        if counter>security:
            break

    data = []
    for i,j in enumerate(joints):

        #skipping the root
        if i !=0:
            par =cmds.listRelatives(j,p=1)
            idx = joints.index(par[0])
            #print j,par,idx
        else:
            idx =-1
        mat = [ float(v) for v in cmds.getAttr(j + ".worldInverseMatrix")]
        j_data = { "name" : j ,
                   "idx"  : idx,
                   "mat"  : mat 
                }

        data.append(j_data)
    
    return data, joints

def save_skeleton(name,root, path):

    data,joints = get_skeleton_data(root)
    #data["department"] = "amimation"
    #data["type"] = "skeleton"
    full = {"department" : "animation", 
            "type": "skeleton",
            "data": data,
            "name": name}
    to_save = json.dumps(full)

    f = open( path, 'w')
    f.write(to_save)
    f.close()
    print "saved to", path

"""
if __name__ == "__main__" or __name__ == "__builtin__":
    root = "root"
    path =  r"E:\WORK_IN_PROGRESS\C\platfoorm\engine\misc\exporters\temp_data\skeleton_mannequin.json"
    save_skeleton("dogSkeleton",root, path)
"""


