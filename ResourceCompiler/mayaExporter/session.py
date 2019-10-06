"""
This module contains the session class declaration
"""
import os
from os.path import expanduser
import imp

from actions import action
import json_utils
import env_config


class ActionSession(object):
    """
    This class implements the session, which is a collection of different
    actions executed sequentially
    The user is able to create interactivly the session then save it load it
    and execute it
    """
    #constant used to exlcude folders in the parsing
    __foldersToExclude = []
    #constant used to exlcude files in the parsing
    __filesToExclude = ["__init__.py", "action.py"]

    def __init__(self):
        """
        This is the constructor
        """

        #Attribute holding all the actions created in the 
        #session
        self.actions = []
        #Internal attribute holding the available actions to be 
        #instantiaded , if needed that value you can access
        #it from available_actions attribute
        self.__available_actions = []
        #Dict mapping the module to its full path
        self.__actions_dict = {}

        #get the fresh list of available actions
        self.__get_available_actions()

    @property
    def available_actions(self):
        """
        This is the getter function for the attribute
        available_actions, this attribute holds all the possible
        actions that the user can instanciate
        """

        #private list holding the available actions
        self.__available_actions = []
        return self.__get_available_actions()

    def __get_available_actions(self):
        """
        This function returns a list of all availble actions
        """
        self.__check_path(env_config.ACTIONS_PATH)
        return self.__available_actions


    def add_action(self, my_action):
        """
        This method add an action to the current session
        @param my_action: Action, the action you wish to add to the session
        """
        self.actions.append(my_action)

    def add_action_from_str(self, action_name):
        """
        This method gets an input action name 
        and finds the correspoding modules, makes an istance
        and adds it to the session
        @param action_name: str , the name of the action to add
        """
        action_name = action_name.replace(".py","")
        my_action = self.__get_nstance_from_str(action_name)[0]
        self.add_action(my_action)

    def remove_action(self, my_action):
        """
        This function removes the given function from the session
        @param my_action : the action istance you wish to remove
        """

        if my_action in self.actions:
            self.actions.remove(my_action)
        else :
            print "action not found"

    def execute(self):
        """
        This function executes the whole session
        """
        for act in self.actions:
            act.execute()

    def save(self, path=None):
        """
        This function saves the current session
        @param path: str, where to save the session, if arg not given
                         a pop up will show up
        """
        data = []
        for act in self.actions:
            data.append(act.save())

        path = json_utils.save(data, path, expanduser("~"))
        return path.rsplit("/",1)[1]

    def load(self, path=None):
        """
        This method initialize a fresh session from a given json session
        @param path: str, the path to the json file in which the session
                         is stored
        """
        self.actions = []
        data = json_utils.load(path)
        for sub_data in data:
            my_action = self.__get_nstance_from_str(sub_data["action_type"])
            
            #we check if we got avalid action otherwise we just skip it
            if my_action :
                my_action = my_action[0]
            else :
                continue
            
            my_action.load(sub_data)
            self.add_action(my_action)


    def get_file_path_from_name(self, name):
        """
        This function retunrs the file path for the given action name
        @param name: str, the action you want the path of
        @return str
        """
        if name in self.__actions_dict:
            return self.__actions_dict[name]

    def __check_path(self, path):
        """
        This procedure checks a path for the py files and kicks the recursions
        @param path:  str, the path to check
        """

        res = os.listdir(path)
        to_return = []
        for sub_res in res:
            if sub_res not in self.__foldersToExclude and \
            os.path.isdir(path + sub_res) == 1:
                self.__check_path(path  + sub_res + "/")


            if sub_res.find("py") != -1 and sub_res.find(".pyc") == -1 \
            and sub_res not in self.__filesToExclude:
                if sub_res.find("reload") == -1:
                    to_return.append(sub_res)
                    self.__actions_dict[sub_res] = path +"/" + sub_res
        self.__available_actions += to_return



    def __get_action_from_string(self, action_name):
        """
        This procedure makes an istance of a module from its name
        @param moduleName: str ,the name of the module
        @return: module instance
        """

        #let s check if the action name is actually valid otherwise skip
        if not self.__fix_action_name(action_name) in self.__actions_dict:
            return

        my_action = self.__actions_dict[self.__fix_action_name(action_name)]
        my_action = imp.load_source(self.__fix_action_name(action_name, 0), \
                    my_action)
        return my_action

    def __fix_action_name(self, action_name, add_extension=1):
        """
        This procedure converts the given module name in a way that is suitable
        for the instancing
        @param moduleName: str ,the name of the module
        @param addExtension: bool , whether or not to add the ".py" at the end
        @return: str
        """

        val = action_name[0].lower() + action_name[1:]
        if add_extension == 1:
            val += ".py"

        return val

    def __get_nstance_from_str(self, action_name):
        """
        This procedure returns a class instance from the given string
        @param moduleName: str ,the name of the module
        """
        
        my_action = self.__get_action_from_string(action_name)
        
        #let s check if we actually got avalid action
        if not my_action:
            return
        my_action_instance = my_action.get_action()
        return my_action_instance, my_action

    def swap_action(self, action, direction="down"):
        """
        This function swap an action up and down
        """
        index = self.actions.index(action)
        if direction == "down":
            index2 = index +1
        else :
            index2 = index -1

        if index2 < 0 :
            index2 = len(self.actions) -1
        elif index2 > len(self.actions) -1 :
            index2 = 0

        self.actions[index], self.actions[index2] = \
        self.actions[index2], self.actions[index] 