# -*- coding: iso-8859-15 -*-

import mmfunction

class BuildEnv:
    def __init__(self, project):
        self.project = project


    def add_virtual_metatarget(self, targetname, dependencies):
        self.project.targets.add_target(targetname, None, False, dependencies)


    def get_variable(self, name):
        return self.project.vars[name]


    def substitute(self, str):
        return self.project.vars.subst(str)


    def add_function(self, targetname, function):
        self.add_virtual_metatarget(targetname, [])             # metatarget must exit before we can add functions
        self.project.targets.add_function(targetname, function) # add the function object


    def output(self, mmake, text):
        mmake = self.substitute(mmake)                  # substitute variables like $(TOP)
        function = mmfunction.Output(self, text)        # create a function object
        self.add_function(mmake, function)              # add it to the function list of the metatarget
