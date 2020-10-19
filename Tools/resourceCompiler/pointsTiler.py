import json



def expandScatteredPoints(path,outputPath,tileSize,tilesPerEdge,visualizePoints):
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
            tileMax = [-99999999999999.0,-99999999999999.0,-99999999999999.0]
        
            pointIdx=0
            tileIdX = tileId%tilesPerEdge
            tileIdY = tileId/tilesPerEdge
            points = [None]*len(tile)
            print "Current tile: ",tileIdX,tileIdY
            for point in tile:
                posX = point[0]*tileSize + offsetX  +tileSize*tileIdX
                posY =0
                posZ = point[1]*tileSize + offsetZ  +tileSize*tileIdY
                tileMin[0] = min(tileMin[0],posX)
                tileMin[1] = min(tileMin[1],posY)
                tileMin[2] = min(tileMin[2],posZ)
                
                tileMax[0] = max(tileMax[0],posX)
                tileMax[1] = max(tileMax[1],posY)
                tileMax[2] = max(tileMax[2],posZ)
                
                points[pointIdx] = [posX,posY,posZ]
                
                pointIdx+=1
            processedData.append({"points": points,"aabb":[tileMin,tileMax]})
            tileId+=1
        
        #point visualizastion
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
        #outputting data
        #making sure AABB have a minimum size
        AABB_MIN_SIZE = 0.1
        for tile in processedData:
            aabbCenter = [ (tile['aabb'][1][subi] + tile['aabb'][0][subi])*0.5 for subi in range(0,3)]
            deltas = [ (tile['aabb'][1][subi] - tile['aabb'][0][subi]) for subi in range(0,3)]
            for i in range(0,3):
                if(deltas[i] < AABB_MIN_SIZE):
                    tile['aabb'][0][i] = aabbCenter[i] - (AABB_MIN_SIZE*0.5); 
                    tile['aabb'][1][i] = aabbCenter[i] + (AABB_MIN_SIZE*0.5); 
            
        jdata = json.dumps(processedData,indent=4,ensure_ascii=True)
        with open(outputPath,"w") as outFile:
            outFile.write(jdata)            


path = r"C:\WORK_IN_PROGRESS\C\directX\SirEngineThe3rd\build2019\bin\Debug\points.json"
outputPath  = r"C:\WORK_IN_PROGRESS\C\directX\SirEngineThe3rd\data\points.json"
tileSize =10.0
tilesPerEdge = 3
visualizePoints = False
expandScatteredPoints(path,outputPath,tileSize,tilesPerEdge,visualizePoints)
