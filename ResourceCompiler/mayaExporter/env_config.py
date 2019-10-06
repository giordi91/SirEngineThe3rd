"""
This module implements some env settings
used throught the whole programm
"""

import os
import inspect

##Constant holding the root path of the tool
ROOT_PATH = os.path.dirname(os.path.abspath( \
    inspect.getfile(inspect.currentframe())))

##Constant holding the action path of the tool
ACTIONS_PATH = ROOT_PATH + "/actions"
##Constant holding the ui path of the tool
UI_PATH = ROOT_PATH +'/ui'