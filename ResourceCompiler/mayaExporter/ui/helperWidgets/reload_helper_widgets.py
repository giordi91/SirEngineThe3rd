"""
This module holds the reload for this subsection of the tool
"""


from ui.helperWidgets import pathWidget
from ui.helperWidgets import spinWidget 
modules = [pathWidget, spinWidget]

def reload_it():
    """
    The method kicking the reload
    """
    
    print "------> Sarted reload of helperWidgets"
    for sub_module in modules:
        print "---> Reloading : " +str(sub_module.__name__)
        reload(sub_module)

    print "------> Ended reload of helperWidgets"
    