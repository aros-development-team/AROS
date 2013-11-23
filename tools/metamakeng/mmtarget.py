# -*- coding: iso-8859-15 -*-

class Makefile:
    def __init__(self, directory, generated):
        if directory == None:
            raise ValueError("[MMAKE] 'directory' must not be None")

        self.directory = directory      # relative to BUILDTOP
        self.generated = generated      # True under BUILDTOP, FALSE under SRCTOP

    def __str__(self):
        return "directory '%s' generated '%s'" % (self.directory, self.generated)


class Target():
    def __init__(self):
        self.makefiles = []     # Makefile() objects
        self.dependencies = []  # list of dependency target names
        self.updated = False    # protection against recursion


    def add_makefile(self, makefile):
        if isinstance(makefile, Makefile):
            self.makefiles.append(makefile)
        else:
            raise TypeError("[MMAKE] 'makefile' is not a Makefile object")


    def add_dependencies(self, dependencies):
        if type(dependencies) == list:
            for dependency in dependencies:
                #ensure unique values
                if dependency not in self.dependencies:
                    self.dependencies.append(dependency)
        elif dependencies == None:
            pass
        else:
            raise TypeError("[MMAKE] 'dependencies' must be a list")


    def __str__(self):
        res = ""
        for makefile in self.makefiles:
            res = res + "\tMakefile %s\n" % (makefile)
        for dependency in self.dependencies:
            res = res + "\tDependency '%s'\n" % (dependency)
        return res


class TargetList(dict):
    def __setitem__(self, key, value):
        raise KeyError("[MMAKE] setting is not allowed")

    def add_target(self, targetname, directory, generated, dependencies):
        if targetname == None or targetname == "":
            raise ValueError("[MMAKE] 'targetname' not defined")

        if targetname in self:
            oldtarget = dict.__getitem__(self, targetname)
            if directory:
                newmakefile = Makefile(directory, generated)
                oldtarget.add_makefile(newmakefile)
            oldtarget.add_dependencies(dependencies)
        else:
            newtarget = Target()
            if directory:
                newmakefile = Makefile(directory, generated)
                newtarget.add_makefile(newmakefile)
            newtarget.add_dependencies(dependencies)
            dict.__setitem__(self, targetname, newtarget)


    def add_dependencies(self, targetname, dependencies):
        if targetname in self:
            oldtarget = dict.__getitem__(self, targetname)
            oldtarget.add_dependencies(dependencies)
        else:
            raise KeyError("[MMAKE] targetname '%s' doesn't exist" % (targetname))


    def search_targets(self, dependency):
        targetnames = []
        for targetname, target in self.iteritems():
            if dependency in target.dependencies:
                targetnames.append(targetname)
        return targetnames


    def __str__(self):
        res = ""
        for key, value in self.iteritems():
            res = res + "Target '%s'\n%s\n" % (key, value)
        return res

            
if __name__ == "__main__":
    tl = TargetList()

    tl.add_target("foo", "rom/exec", True, ["aa", "bb"])
    tl.add_target("bar", "rom/graphics", False, ["cc", "dd"])
    tl.add_target("foo", "rom/dos", False, ["ee", "ff"])
    tl.add_target("eee", None, False, ["qq", "rr"])

    tl.add_dependencies("bar", ["zz", "yy"])

    print tl
