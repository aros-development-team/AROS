# -*- coding: iso-8859-15 -*-

import logging, os, sys

import mmproject

mflags = []
targets = []
srcdir = builddir = currdir = os.getcwd()
dryrun = False
loglevel = logging.WARNING

for idx in range(1, len(sys.argv)):
    arg = sys.argv[idx]
    if arg[0] == "-":
        if arg == "--version":
            print "MetaMakeNG 1.0"
            sys.exit(0)
        elif arg.startswith("--srcdir"):
            srcdir = arg[9:]
        elif arg.startswith("--builddir"):
            builddir = arg[11:]
        elif arg == "--quiet" or arg == "-q":
            loglevel = logging.ERROR
        elif arg == "--verbose" or arg == "-v":
            loglevel = logging.INFO
        elif arg == "--debug":
            loglevel = logging.DEBUG
        elif arg == "--dryrun":
            dryrun = True
        elif arg == "--help":
            print "%s [--srcdir=<directory>] [--builddir=<directory>] [--version] [-v,--verbose] [-q,--quiet] [--debug] [--help]" % (sys.argv[0])
            sys.exit(0)
        else:
            mflags.append(arg)
    else:
        targets.append(arg)

logging.basicConfig(level=loglevel)

logging.info("[MMAKE] SRCDIR   '%s'" % (srcdir))
logging.info("[MMAKE] BUILDDIR '%s'" % (builddir))

logging.debug("[MMAKE] mmake.py: parsed command line options")

projectlist = {}

with open("mmake.config", "r") as filehandle:
    newproject = mmproject.Project(srcdir, builddir, mflags, dryrun)    # create default project
    projectlist[newproject.name] = newproject
    for line in filehandle:
        if line.startswith("#") or line.startswith("\n"):
            continue
        elif line.startswith("["):
            end = line.find("]", 2)
            if end != -1:
                name = line[1:end]
                newproject = mmproject.Project(srcdir, builddir, mflags, dryrun)
                newproject.name = name
                projectlist[newproject.name] = newproject
        else:
            space = line.find(" ")
            if space != -1:
                cmd = line[0 : space]
                value = line[space + 1 : ].strip()
                if cmd == "add":
                    newproject.extramakefiles.append(value)
                elif cmd == "ignoredir":
                    newproject.ignoredirs.append(value)
                elif cmd == "defaultmakefilename":
                    newproject.defaultmakefilename = value
                elif cmd == "top":
                    newproject.srctop = value
                elif cmd == "defaulttarget":
                    newproject.defaulttarget = value
                elif cmd == "genmakefilescript":
                    newproject.genmakefilescript = value
                elif cmd == "genmakefiledeps":
                    deps = value.split()
                    newproject.genmakefiledeps = newproject.genmakefiledeps + deps
                elif cmd == "globalvarfile":
                    newproject.globalvarfiles.append(value)
                elif cmd == "genglobalvarfile":
                    newproject.genglobalvarfile = value
                elif cmd == "maketool":
                    newproject.maketool = value
                else:
                    newproject.vars[cmd] = value

logging.debug("[MMAKE] mmake.py: projects initialised")

for t in targets:
    projectname, _, targetname = t.partition(".")
    if targetname == "":
        targetname = projectname

    if projectname not in projectlist:
        logging.error("[MMAKE] Nothing known about project %s" % (projectname))
        sys.exit(20)

    logging.debug("[MMAKE] mmake.py calling maketarget '%s'" % (targetname))
    myproject = projectlist[projectname]
    myproject.maketarget(targetname)
