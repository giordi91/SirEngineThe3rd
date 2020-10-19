from ui.helperWidgets import reload_helper_widgets
from ui.workFile import reload_work_file
from ui import actionUi
from ui import fileActionUi
from ui import exportModelActionUi 
from ui import exportSkinActionUi
from ui import exportSkeletonActionUi
from ui import exportAnimActionUi
from ui import sessionUi



modules = [ actionUi, fileActionUi, exportModelActionUi, exportSkinActionUi,
            exportSkeletonActionUi, exportAnimActionUi, sessionUi]

reload_it_modules = [reload_helper_widgets, reload_work_file]
def reload_it():
    print "------> Sarted reload of ui"
    for sub_module in reload_it_modules:
        reload(sub_module)
        sub_module.reload_it()

    for sub_module in modules:
        reload(sub_module)
        print "---> Reloading : " +str(sub_module.__name__)


    print "------> Ended reload of ui"
