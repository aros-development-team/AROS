# -*- coding: iso-8859-15 -*-

import logging, os, sys

import mmproject

mflags = []
targets = []
srcdir = builddir = currdir = os.getcwd()

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
        elif arg == "--help":
            print "%s [--srcdir=<directory>] [--builddir=<directory>] [--version] [-v,--verbose] [-q,--quiet] [--debug] [--help]" % (sys.argv[0])
            sys.exit(0)
        else:
            mflags.append(arg)
    else:
        targets.append(arg)

logging.basicConfig(level=loglevel)

logging.info("SRCDIR   '%s'" % (srcdir))
logging.info("BUILDDIR '%s'" % (builddir))

logging.debug("[MMAKE] mmake.py: parsed command line options")

myproject = mmproject.Project(srcdir, builddir, mflags)

logging.debug("[MMAKE] mmake.py: projects initialised")

for t in targets:
    projectname, _, targetname = t.partition(".")
    if targetname == "":
        targetname = projectname

    logging.debug("[MMAKE] mmake.py calling maketarget '%s'" % (targetname))
    myproject.maketarget(targetname)
