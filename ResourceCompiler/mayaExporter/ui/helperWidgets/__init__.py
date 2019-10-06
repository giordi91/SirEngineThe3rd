import inspect 
import os
import importlib

currPath=  inspect.getfile(inspect.currentframe())
filename = currPath.split("/")[-1]
currPath= os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))


files  = os.listdir(currPath)
to_import = [ f for f in files if (f.find("Action.py") != -1 and f.find(".pyc") == -1)]
for f in to_import:
	importlib.import_module("actions." + f.replace(".py",""))

