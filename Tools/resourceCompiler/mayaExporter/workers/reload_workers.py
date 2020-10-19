from workers import animationExporter 
from workers import meshExporter 
from workers import skeletonExporter 
from workers import skinclusterExporter 


modules = [animationExporter, meshExporter,skeletonExporter,skinclusterExporter]

def reload_it():
    print "------> Sarted reload of workers"
    for sub_module in modules:
        reload(sub_module)
        print "---> Reloading : " +str(sub_module.__name__)

    print "------> Ended reload of workers"
    