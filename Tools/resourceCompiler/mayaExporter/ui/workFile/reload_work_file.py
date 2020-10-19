from ui.workFile import action_form
from ui.workFile import path_form
from ui.workFile import session_form


modules = [action_form, path_form, session_form]

def reload_it():
    print "------> Sarted reload of workFile"
    for sub_module in modules:
        reload(sub_module)
        print "---> Reloading : " +str(sub_module.__name__)

    print "------> Ended reload of workFile"
    