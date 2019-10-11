

from maya import cmds
from maya.api import OpenMaya
import skeletonExporter
import json





def get_anim_data(root, start,end):
    
    data,joints = skeletonExporter.get_skeleton_data(root)
    anim_data= []
    for frame in range(start, end+1):
        cmds.currentTime(frame)
        pose_data = []
        for j in joints:
            pose_data.append(joint_to_QT(j))
        
        anim_data.append(pose_data)

    return anim_data

def joint_to_QT(joint): 

    #m = cmds.getAttr(joint + '.wm')
    #m =  cmds.xform(joint,q=1,ws=1,m=1 )
    m =  cmds.getAttr(joint + ".wm")
    p = cmds.listRelatives(joint, p =1)
    #not really sure it is necessary
    if p and cmds.nodeType(p) == "joint":
        mp =  cmds.xform(p[0],q=1,ws=1,m=1 )
        mm =  OpenMaya.MMatrix(m)
        mmp =  OpenMaya.MMatrix(mp)
        m = mm * mmp.inverse()
    #else:
    #    print "no parent found for ", joint, p

    pos = cmds.getAttr(joint +'.t')[0]

    mmat =  OpenMaya.MMatrix(m)
    mfn = OpenMaya.MTransformationMatrix(mmat)
    rot = mfn.rotation(asQuaternion =True)

    return {"pos":pos, "quat": [rot.x,rot.y,rot.z,rot.w]}


def save_anim(root,anim_name,skeleton_name,looping,start, end , path, frame_rate):
    """
    function that export the animation ready to be processed by the engine
    @param root:str, the name of the root bone
    @param anim_name: str, the name to which to store the aniomaton
    @param skeleton_name: str, the skeleton on which the anim depends on
    @param looping: bool , wheter or not the animation should be looping
    @param start: int, staring frame of the animation
    @param end: int, end frame of the animation
    @param path: str, where to store the animation on disk
    @param framerare: float the rate at which frames should be plated, example 1/24
    """
    
    data = get_anim_data(root, start, end)
    full = {"poses":data,
            "start": start,
            "end" : end,
            "looping":looping,
            "department": "animation",
            "type" : "clip",
            "name" : anim_name,
            "skeleton_name": skeleton_name,
            "frame_rate":frame_rate,
			"bonesPerPose": len(data[0])}

    to_save = json.dumps(full)
    f = open( path, 'w')
    f.write(to_save)
    f.close()
    print ("saved to", path)


"""
if __name__ == "__main__" or __name__ == "__builtin__":
    print "exporting anim"
    
    root = "root"
    path =  r"E:\WORK_IN_PROGRESS\C\platfoorm\engine\misc\exporters\temp_data\mannequin_idle.json"
    save_anim(root,"mannequinIdle","mannequinSkeleton",True, 0,78, path, 1.0/33.0)
"""


