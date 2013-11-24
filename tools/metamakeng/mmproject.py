# -*- coding: iso-8859-15 -*-

import errno, logging, os, re, sys

import mmgenmf, mmglobal, mmtarget, mmvar


# regex for #MM... lines
mmregex = re.compile(r"""
^
(?P<spec>(\#MM-|\#MM[ \t]|\#MM))
\s*
(?P<targets>[\w\s$\(\)-\.]*)
(?P<colon>:?)
(?P<deps>[\w\s$\(\)-\.]*)
(?P<slash>\\?)
\s*
$
""", re.VERBOSE)

# regex for conf files
confregex = re.compile(r"""
^
\s*
(\w+)
\s*
[:=]+
(.*)
$
""", re.MULTILINE | re.VERBOSE)

class Project:
    def __init__(self, srcdir, builddir, mflags):
        self.name = "AROS"
        self.maketool = "$(HOST_MAKE) $(MKARGS) TOP=$(TOP) SRCDIR=$(SRCDIR) CURDIR=$(CURDIR) TARGET=$(TARGET)"
        self.defaulttarget = "AROS"
        self.genmakefilescript = "$(SRCDIR)/config/make.tmpl"
        self.genmakefiledeps = ["$(SRCDIR)/config/make.tmpl"]
        self.globalvarfiles = ("$(TOP)/bin/$(AROS_HOST_ARCH)-$(AROS_HOST_CPU)/gen/config/host.cfg",
            "$(TOP)/bin/$(AROS_TARGET_ARCH)-$(AROS_TARGET_CPU)/gen/config/target.cfg")
        self.genglobalvarfile = "sh $(TOP)/configure"
        self.ignoredirs = ("CVS", "bin", "intuition.morphos", ".svn", ".unmaintained", "distfiles")

        self.srctop = srcdir
        self.buildtop = builddir
        self.mflags = mflags

        self.vars = mmvar.VarList()
        self.targets = mmtarget.TargetList()


    def maketarget(self, targetname):
        logging.info("[MMAKE] Building %s.%s\n" % (self.name, targetname))

        os.chdir(self.srctop)
        self.readvars()

        genmakefilescript = self.vars.subst(self.genmakefilescript)
        for parent, dirs, files in os.walk("."):
            for igndir in self.ignoredirs:
                if igndir in dirs:
                    dirs.remove(igndir)
            if "mmakefile.src" in files:
                infilename = os.path.join(parent, "mmakefile.src") # relative to srctop
                outfilename = os.path.join(self.buildtop, parent, "mmakefile")
                if not os.path.exists(outfilename) or (os.path.getmtime(infilename) > os.path.getmtime(outfilename)):
                    try:
                        os.makedirs(os.path.join(self.buildtop, parent))
                    except os.error, e:
                        if e.errno != errno.EEXIST:
                            raise
                    mmgenmf.genmf(genmakefilescript, infilename, outfilename)
                self.parsemakefile(parent, True)
            elif "mmakefile" in files:
                self.parsemakefile(parent, False)


        #print "*** kernel *** %s" % (self.targets["kernel-linux-"])
        #print "*** targets *** %s" % (self.targets.search_targets("kernel-linux-"))
        os.chdir(self.buildtop)
        self.build_recursive(0, targetname)


    def readvars(self):
        self.vars["TOP"] = self.buildtop
        self.vars["SRCDIR"] = self.srctop
        self.vars["CURDIR"] = ""

        self.vars["AROS_TARGET_VARIANT"] = ""

        for varfile in self.globalvarfiles:
            filename = self.vars.subst(varfile)
            with open(filename, "r") as filehandle:
                content = filehandle.read()
                for match in confregex.finditer(content):
                    variable = match.group(1)
                    value = match.group(2).strip()
                    varline = self.vars.subst(value)
                    self.vars[variable] = varline
                    #print "variable %s value %s newvalue %s" % (variable, value, varline)

        for dep in range(len(self.genmakefiledeps)):
            self.genmakefiledeps[dep] = self.vars.subst(self.genmakefiledeps[dep])


    def parsemakefile(self, directory, generated):
        if generated:
            filename = os.path.join(self.buildtop, directory, "mmakefile")
        else:
            filename = os.path.join(self.srctop, directory, "mmakefile")

        with open(filename, "r") as filehandle:
            mmcont = False # True if line ends with \
            linenr = 0 # for error messages
            while True:
                line = filehandle.readline()
                if line == "":
                    break
                linenr = linenr + 1

                if line.startswith("#MM"):
                    #print "[MMAKE] parsing %s line %s nr %d" % (filename, line, linenr)
                    matches = mmregex.match(line)
                    if matches:
                        specification = matches.group("spec")
                        targets = self.vars.split_subst(matches.group("targets"))
                        colon = matches.group("colon")
                        dependencies = self.vars.split_subst(matches.group("deps"))
                        slash = matches.group("slash")
                        #print "spec '%s' target '%s' colon '%s' deps '%s' slash '%s'" %(specification, targets, colon, dependencies, slash)

                        if specification == "#MM " or specification == "#MM\t":
                            if not mmcont:
                                newtargets = [] # remember them for continuation lines
                                for targetname in targets:
                                    self.targets.add_target(targetname, directory, generated, dependencies)
                                    newtargets.append(targetname)
                            else:
                                # continuation
                                if colon != "":
                                    raise ValueError("[MMAKE] No colon allowed in continuation line")
                                for targetname in newtargets:
                                    # the deps are appearing in the 'targets' caption
                                    self.targets.add_dependencies(targetname, targets)
                        elif specification == "#MM-": # virtual
                            if not mmcont:
                                newtargets = [] # remember them for continuation lines
                                for targetname in targets:
                                    self.targets.add_target(targetname, None, False, dependencies)
                                    newtargets.append(targetname)
                            else:
                                raise ValueError("[MMAKE] #MM- not allowed after line continuation")
                        elif specification == "#MM":
                            if not mmcont:
                                line = filehandle.readline()
                                if line == "":
                                    raise ValueError("[MMAKE] Unexpected end while reading file")
                                newtargets = []
                                targets, _, _ = line.partition(":")
                                targets = self.vars.split_subst(targets)
                                for targetname in targets:
                                    self.targets.add_target(targetname, directory, generated, None)
                            else:
                                raise ValueError("[MMAKE] #MM not allowed after line continuation")
                        # if line ends with \ we enable continuation mode
                        if slash != "":
                            mmcont = True
                        else:
                            mmcont = False
                    else:
                        raise ValueError("[MMAKE] Regex didn't match in line %s nr %d" % (line, linenr))

                else:
                    # break continuation mode if line doesn't start with #MM
                    mmcont = False


    def build_recursive(self, level, targetname):
        #print "%s level %d target %s " % (" " * level * 4, level, targetname)

        if targetname in self.targets:
            target = self.targets[targetname]
            target.updated = True
            for dependency in target.dependencies:
                if dependency in self.targets:
                    subtarget = self.targets[dependency]
                    if not subtarget.updated:
                        #print "%s dep %s" % (" " * level * 4, dependency)
                        self.build_recursive(level + 1, dependency)
                else:
                    logging.warning("[MMAKE] nothing known about subtarget %s" % (dependency))
            for makefile in target.makefiles:
                self.callmake(targetname, makefile)
        else:
            logging.warning("[MMAKE] nothing known about target %s" % (targetname))


    def execute(self, command, infile, outfile, arguments):
        cmdstr = command + " "

        if infile != "-":
            cmdstr = cmdstr + "<" + infile + " "

        if outfile != "-":
            cmdstr = cmdstr + ">" + outfile + " "

        cmdstr = cmdstr + arguments
        cmdstr = self.vars.subst(cmdstr)

        logging.info("[MMAKE] Executing %s..." % (cmdstr))

        rc = os.system(cmdstr)

        if rc:
            logging.error("[MMAKE] %s failed: %d" % (cmdstr, rc))

        return not rc


    def callmake (self, targetname, makefile):
        path = makefile.directory

        if makefile.generated:
            os.chdir(self.buildtop)
        else:
            os.chdir(self.srctop)
        os.chdir(path)

        self.vars["CURDIR"] = path
        self.vars["TARGET"] = targetname

        buffer = ""

        for flag in self.mflags:
            buffer = buffer + flag + " "

        buffer = buffer + " --file=mmakefile %s" % (targetname)

        logging.info("[MMAKE] Making %s in %s" % (targetname, path))

        if not self.execute(self.maketool, "-", "-", buffer):
            raise ("[MMAKE] Error while running make in %s", path)
