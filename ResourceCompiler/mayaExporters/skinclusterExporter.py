import sys 
sys.path.append( "E:\\WORK_IN_PROGRESS\\C\\platfoorm\\engine\\misc\\exporters")


from maya import cmds
from maya import OpenMaya
from maya import OpenMayaAnim
import skeletonExporter
reload(skeletonExporter)
import json
MAX_INFLUENCE = 6;

def map_shadow_to_skeleton(root):

    data,joints = skeletonExporter.get_skeleton_data(root)

    shadow_to_skele = {}
    skele_to_shadow={}
    #for each joints we need to follow the constraint to find the driver and build 
    #a map with that data
    for j in joints:
        const = cmds.listConnections(j + '.tx', d=0,s=1)[0]
        driver = cmds.listConnections(const + '.target[0].targetTranslate',s=1,d=0)

        shadow_to_skele[j] = driver[0]
        skele_to_shadow[driver[0]] = j

    return shadow_to_skele, skele_to_shadow


def getWeightsData (mesh,skinNode, skele_to_shadow, joints):
    '''
    This procedure let you create a dictionary holding all the needed information to rebuild
    a skinCluster map
    '''
    sknN = skinNode 
    cmds.undoInfo(openChunk = 1)
    
        

    infls = cmds.skinCluster(skinNode, q=True, inf=True)
      
    weightMap = []
    
    # get the dag path of the shape node
    
    sel = OpenMaya.MSelectionList()
    cmds.select(skinNode)
    OpenMaya.MGlobal.getActiveSelectionList(sel)
    skinClusterObject = OpenMaya.MObject()
    sel.getDependNode(0,skinClusterObject )
    skinClusterFn = OpenMayaAnim.MFnSkinCluster(skinClusterObject)
    
    
    cmds.select(mesh)
    sel = OpenMaya.MSelectionList()
    OpenMaya.MGlobal.getActiveSelectionList(sel)
    shapeDag = OpenMaya.MDagPath()
    sel.getDagPath(0, shapeDag)
    # create the geometry iterator
    geoIter = OpenMaya.MItGeometry(shapeDag)
    
    # create a pointer object for the influence count of the MFnSkinCluster
    infCount = OpenMaya.MScriptUtil()
    infCountPtr = infCount.asUintPtr()
    OpenMaya.MScriptUtil.setUint(infCountPtr, 0)
    
    value = OpenMaya.MDoubleArray()
    weightMap = []

    infls= OpenMaya.MDagPathArray()
    skinClusterFn.influenceObjects(infls)
    while geoIter.isDone() == False:
        skinClusterFn.getWeights(shapeDag, geoIter.currentItem(), value, infCountPtr)
        vtx_data ={"idx": geoIter.index(),
                   "j":[],
                   "w":[]} 
        for j in range(0, infls.length()):

            if value[j] > 0:
                if skele_to_shadow:
                    jnt_idx = joints.index(skele_to_shadow[infls[j]])
                else:

                    #node = cmds.listConnections(skinN + ".matrix[" + str(j) + "]",s=1,d=0)[0]
                    #jnt_idx = joints.index(node)
                    node = infls[j].fullPathName().rsplit("|",1)[1]
                    #print node
                    jnt_idx = joints.index(node)
                    #jnt_idx = j
                weight= value[j]

                vtx_data["j"].append(int(jnt_idx))
                vtx_data["w"].append(float(weight))

        currL = len(vtx_data["j"])
        if currL>MAX_INFLUENCE:
            print "vertex",vtx_data["idx"], "joints got more than "+str(MAX_INFLUENCE) + " infs"  
            return;

        if currL!= MAX_INFLUENCE:
            #lets format the data to have always 4 elemets
            deltaSize = MAX_INFLUENCE - currL
            vtx_data['j'].extend([int(0)]*deltaSize)
            vtx_data['w'].extend([0.0]*deltaSize)

            if len(vtx_data["j"]) != MAX_INFLUENCE:
                print "vertex",vtx_data["idx"], "wrong formatting after correction"  
            if len(vtx_data["w"]) != MAX_INFLUENCE:
                print "vertex",vtx_data["idx"], "wrong formatting after correction"  
            
        weightMap.append(vtx_data)
        geoIter.next()

    cmds.undoInfo(closeChunk = 1)
    print "------> WeightMap has been saved!"
    return weightMap

def export_skin(root, skin_name, path, mesh , tootle_path=None, is_shadow=True):


    data,joints = skeletonExporter.get_skeleton_data(root)
    #print joints.index("L_EyeAim0")
    if is_shadow:
        print "----> Remapping to shadow skeleton"
        shadow_to_skele, skele_to_shadow = map_shadow_to_skeleton(root)
        data = getWeightsData(mesh,skin_name,skele_to_shadow, joints)
    else :
        data =  getWeightsData(mesh,skin_name,None, joints)

    full = {"type":"skinCluster",
            "data":data,
            "skeleton": "dogSkeleton"
            }

    if tootle_path != None:
        #read in the tootle
        print "---> remapping skin using tootle data"
        t = open(tootle_path, 'r')
        tootle_map = json.load(t)
            
        newData = [0]*len(full["data"])
        for i,d in enumerate(full["data"]):
            new = tootle_map[str(i)]
            newData[new] = d

        full["data"] = newData
    else:
        print "skippping tootle"

    to_save = json.dumps(full)
    f = open( path, 'w')
    f.write(to_save)
    f.close()
    print "saved to", path

if __name__ == "__main__" or __name__ == "__builtin__":
    print "exporting skin"
     
    root = "root"
    skin = "skinCluster1"
    path =  r"E:\WORK_IN_PROGRESS\C\platfoorm\engine\misc\exporters\temp_data\mannequin_skin.json"
    mesh = "mannequin"
    tootle_path = r"E:\WORK_IN_PROGRESS\C\platfoorm\engine\misc\exporters\temp_data\mannequin.tootle"
    tootle_path=None
    export_skin(root, skin, path, mesh, tootle_path, False)
    """
    data,joints = skeleton_exporter.get_skeleton_data(root)
    shadow_to_skele, skele_to_shadow = map_shadow_to_skeleton(root)
    data = getWeightsData(mesh,skin,skele_to_shadow, joints)

    full = {"type":"skinCluster",
            "data":data,
            "skeleton": "dogSkeleton"
            }

    to_save = json.dumps(full)
    f = open( path, 'w')
    f.write(to_save)
    f.close()
    print "saved to", path
    """


