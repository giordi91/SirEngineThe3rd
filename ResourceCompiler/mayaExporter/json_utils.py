"""
This modules contains useful function to deal with
json files
"""

import json
from PySide2 import QtGui
from maya import cmds


# @todo : replace maya dialog with qt dialog
def save(stuff_to_save=None, path=None, start_path=None):
    '''
    This procedure saves given data in a json file
    @param[in] stuff_to_save : this is the data you want to save ,
                                be sure it s json serializable
    @param path : where you want to save the file
    '''
    if not start_path:
        start_path = __file__
    if not path:
        path  = QtGui.QFileDialog.getSaveFileName(None,
    "Open Session", start_path, "Session Files *.session")[0]
              
    if path == "":
        return
    to_be_saved = json.dumps(stuff_to_save, sort_keys=True, \
                            ensure_ascii=True, indent=2)
    current_file = open(path, 'w')
    current_file.write(to_be_saved)
    current_file.close()

    print "------> file correctly saved here : ", path
    return path

def load(path=None, start_path=None):
    '''
    This procedure loads and returns the content of a json file
    @param path:  what file you want to load
    @return : the content of the file
    '''
    if not start_path:
        start_path = __file__
    if not path:
        path  = QtGui.QFileDialog.getOpenFileName(None,
    "Open Session", start_path, "Session Files *.session")[0]

    if path == "":
        return

    current_file = open(path)
    data_file = json.load(current_file)

    return data_file
