# -*- coding: iso-8859-15 -*-

import glob

import mmfunction

class BuildEnv:
    def __init__(self, project):
        self.project = project

    ###############################################################################################

    # support functions

    def get_variable(self, name):
        """Return value of a variable.

        Metamake keeps a dictionary of variables which are read
        from config files. If the variable doesn' exist it is
        read from the system environment. If that fails an exception
        is raised.

        Keyword arguments:
        name -- the name of the variable whose value should be returned

        """
        return self.project.vars[name]


    def substitute(self, str):
        """Substitute patterns in a string.

        Substitute $(VARIBLE) entries in a string by the value
        of that variable. Returns the changed string.

        Keyword arguments:
        str -- the string whose templates should be substituted

        """
        return self.project.vars.subst(str)


    def add_function(self, targetname, function):
        """Bind a builder function to a metatarget.

        Keyword arguments:
        targetname -- the metatarget name
        function -- the builder function object

        """
        # metatarget must exist before we can add functions
        self.add_virtual_metatarget(targetname)
         # add the function object
        self.project.targets.add_function(targetname, function)


    def split(self, str, subst=False):
        """Split a string to a list.

        Separation is done by whitespace.

        Keyword arguments:
        str -- the string to be splitted
        subst -- if True then templates like $(VARIABLE) are substituted.

        """
        if subst:
            list = self.project.vars.split_subst(str)
        else:
            list = str.split()
        return list


    def glob(self, path):
        """Do pattern matching.

        Example: cfiles = buildenv.glob("*.c")

        Keyword arguments:
        path -- the path to

        """
        return glob.glob(path)

    ###############################################################################################

    # metatarget handling

    def add_virtual_metatarget(self, targetname, dependencies=[]):
        """Create a virtual metatarget.

        If the metatarget already exists the dependencies will be added.

        Keyword arguments:
        targetname -- metatarget
        dependencies -- list of names of dependencies

        """
        self.project.targets.add_target(targetname, None, False, dependencies)

    ###############################################################################################

    # builders

    def output(self, mmake, text):
        """Builder which outputs some text.

        This builder exists mainly for testing purposes and as an example
        how a builder must be written.

        Keyword arguments:
        text -- the string which should be printed

        """
        # substitute variables like $(TOP)
        mmake = self.substitute(mmake)
        # create a function object
        function = mmfunction.Output(self, text)
        # add it to the function list of the metatarget
        self.add_function(mmake, function)


    def mkdirs(self, mmake, dirs):
        """Builder which creates directories.

        Keyword arguments:
        dirs -- list of directory names

        """
        mmake = self.substitute(mmake)
        function = mmfunction.MkDirs(self, dirs)
        self.add_function(mmake, function)


    def copy_files(self, mmake, files, src, dst):
        """Builder which copies files.

        Keyword arguments:
        files -- list of file names
        src -- source directory
        dst -- destination directory

        """
        mmake = self.substitute(mmake)
        function = mmfunction.CopyFiles(self, files, src, dst)
        self.add_function(mmake, function)
