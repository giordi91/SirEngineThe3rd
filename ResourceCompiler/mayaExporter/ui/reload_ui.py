from ui.helperWidgets import reload_helper_widgets
from ui.workFile import reload_work_file
from ui import actionUi
from ui import sessionUi
from ui import fileActionUi
from ui import runSystemActionUi
from ui import runPyActionUi
from ui import runPyFileActionUi
from ui import exportModelActionUi 



modules = [reload_helper_widgets, reload_work_file, actionUi,
            sessionUi, fileActionUi ,exportModelActionUi]

reload_it_modules = [reload_helper_widgets, reload_work_file]
def reload_it():
    print "------> Sarted reload of ui"
    for sub_module in modules:
        reload(sub_module)
        print "---> Reloading : " +str(sub_module.__name__)

    for sub_module in reload_it_modules:
        sub_module.reload_it()

    print "------> Ended reload of ui"
