

#from maya import cmds
import os
from maya import cmds
import json
from maya.api import OpenMaya

def get_dag_node( xform ):
    selectionList = OpenMaya.MSelectionList()
    try:
        selectionList.add( xform )
    except:
        return None
    dagPath = OpenMaya.MDagPath()
    dagPath = selectionList.getDagPath( 0 )

    return dagPath

def get_tans(shape):
    dag = get_dag_node(shape)

    fnMesh = OpenMaya.MFnMesh(dag)
    tcount, tvtx = fnMesh.getTriangles() 
    tangents = fnMesh.getTangents()
    fit = OpenMaya.MItMeshPolygon(dag)
    #vtx_count = fnMesh.numVertices 
    f_count = fit.count()
    tan_dict={}
    for i in range(f_count):

        fit.setIndex(i)
        f_tr_p, f_tr_id = fit.getTriangles()
        for t in range(0,len(f_tr_p),3):
            for t_idx in range(3):
                v_idx = f_tr_id[t+ t_idx]
                if v_idx in tan_dict:
                    continue

                idx = fit.tangentIndex(t_idx)
                c_tan = tangents[idx] 
                c_tan_pod = [v for v in c_tan]
                tan_dict[v_idx] = c_tan_pod
    tan_list = []
    vcount = fnMesh.numVertices
    for i in range(vcount):
        v = tan_dict[i]
        tan_list.append(v[0])
        tan_list.append(v[1])
        tan_list.append(v[2])
    
    #return tan_dict
    return tan_list


def get_connectivity(shape):
    dag = get_dag_node(shape)
    vit = OpenMaya.MItMeshVertex(dag)

    conn_dict={}
    for i in range (vit.count()):

        vit.setIndex(i)
        idx = vit.getConnectedVertices()
        conn_dict[i] =  [v for v in idx]      

    return conn_dict


def get_normals(shape):
    dag = get_dag_node(shape)
    fit = OpenMaya.MItMeshVertex(dag)
    normals = []
    for i in range(fit.count()):
        fit.setIndex(i)
        n = fit.getNormal()
        n = [v for v in n]
        normals.append(n[0])
        normals.append(n[1])
        normals.append(n[2])
    
    return normals


def debug_tangents(mesh_name,tans_dict):
    for idx,t in tans_dict.iteritems():
        #obj  = cmds.ls(sl=1)[0]
        pos = cmds.xform(mesh_name +".vtx["+str(idx) + "]" , ws=1,q=1,t=1)
        #cmds.setAttr(obj +'.t', *pos)
        loc = cmds.spaceLocator()[0]
        cmds.setAttr(loc +'.t' , *t)
        g = cmds.group(empty=1)
        cmds.parent(loc,g)
        cmds.setAttr(g +'.t' , *pos)
        cmds.setAttr(g + '.s', 0.23,0.23,0.23)
        cmds.setAttr(loc + '.localScale', 0.08,0.08,0.08)




def export_obj(mesh,path):
    cmds.select(mesh)
    cmds.file(path ,pr=1,typ="OBJexport",es=1, f=1,
    op="groups=0; ptgroups=0; materials=0; smoothing=0; normals=1")

    shape = cmds.listRelatives(mesh, shapes=1)[0]
    print "shape found was:", shape

    tans_dict = get_tans(shape)
    t_path = path.replace(".obj",".tangents") 
    data_j = json.dumps(tans_dict, indent=4, separators=(',', ': '))
    f = open(t_path, "w")
    f.write(data_j)
    f.close()
    print "saved tangents to", t_path 



#export_obj("pPlane1","C:/Users/marco/Desktop/WORK_IN_PROGRESS/SirEngineThe3rd/data/meshes")
def exportMaterial(obj,outPath):
    shape = cmds.listRelatives(obj, shapes=1)[0]
    #lets try to find the material shall we?
    SG = cmds.listConnections(shape + ".instObjGroups")[0]
    print SG
    shader = cmds.listConnections(SG + ".surfaceShader")[0]
    print shader
    colorTex = cmds.listConnections(shader +".color")[0]
    print colorTex
    colorPath = cmds.getAttr(colorTex + ".fileTextureName")
    print colorPath


def exportSelected(outPath):
    objs = cmds.ls(selection=1)
    
    for obj in objs:
        export_obj(obj, outPath)
    
    

    
    
    

