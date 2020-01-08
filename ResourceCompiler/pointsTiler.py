import json



def expandScatteredPoints(path,tileSize,tilesPerEdge,visualizePoints):
    with open(path) as json_file:
        data = json.load(json_file)
        counter =0
        print "tiles count found in file:" ,len(data['tiles'])
        tileId = 0
        pointIdx =0
        
        #compute corner offset
        isEven = tilesPerEdge%2 == 0
        offsetX = 0
        offsetZ = 0
        half = tilesPerEdge/2.0
        offsetX = -half*tileSize
        offsetZ = -half*tileSize
             
        processedData =[]
        
        for tile in data['tiles']:
            tileMin = [99999999999999.0,99999999999999.0,99999999999999.0]
            tileMax = [-99999999999999.-0,99999999999999.-0,99999999999999.0]
        
            pointIdx=0
            tileIdX = tileId%tilesPerEdge
            tileIdY = tileId/tilesPerEdge
            points = [None]*len(tile)
            print tileIdX,tileIdY
            for point in tile:
                posX = point[0]*tileSize + offsetX  +tileSize*tileIdX
                posY =0
                posZ = point[1]*tileSize + offsetZ  +tileSize*tileIdY
                tileMin[0] = min(tileMin[0],posX)
                tileMin[1] = min(tileMin[1],posY)
                tileMin[2] = min(tileMin[2],posZ)
                
                tileMax[0] = min(tileMax[0],posX)
                tileMax[1] = min(tileMax[1],posY)
                tileMax[2] = min(tileMax[2],posZ)
                
                points[pointIdx] = [posX,posY,posZ]
                
                pointIdx+=1
            processedData.append({"points": points,"aabb":[tileMin,tileMax]})
            tileId+=1
        
        if(visualizePoints):
            ll = cmds.ls("TILE_*")
            if(ll):
                cmds.delete(ll)
            tileId=0
            for tile in processedData:
                pointIdx =0
                print tile['points']
                for point in tile['points']:
                    loc = cmds.spaceLocator(name="TILE_" + str(tileId)+"_" + str(pointIdx))[0]
                    cmds.setAttr(loc + '.t', point[0],point[1],point[2])
                    cmds.setAttr(loc + '.localScale',0.2,0.2,0.2)
                    pointIdx+=1

                tileId+=1



path = r"C:\WORK_IN_PROGRESS\C\directX\SirEngineThe3rd\build2019\bin\Debug\points.json"
tileSize =10.0
tilesPerEdge = 3
visualizePoints = True
expandScatteredPoints(path,tileSize,tilesPerEdge,visualizePoints)
