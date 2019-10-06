"""
This module holds the reload for this subsection of the tool
"""


from ui.helperWidgets import pathWidget
modules = [pathWidget]

def reload_it():
    """
    The method kicking the reload
    """
    
    print "------> Sarted reload of helperWidgets"
    for sub_module in modules:
        reload(sub_module)
        print "---> Reloading : " +str(sub_module.__name__)

    print "------> Ended reload of helperWidgets"
    