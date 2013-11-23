# -*- coding: iso-8859-15 -*-

import sys, os

import mmproject, mmglobal

mflags = []
targets = []
srcdir = builddir = currdir = os.getcwd()

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
        elif arg == "--verbose" or arg == "-v":
            mmglobal.verbose = True
        elif arg == "--quiet" or arg == "-q":
            mmglobal.quiet = True
        elif arg == "--debug":
            mmglobal.debug = True
        elif arg == "--help":
            print "%s [--srcdir=<directory>] [--builddir=<directory>] [--version] [-v,--verbose] [-q,--quiet] [--debug] [--help]" % (sys.argv[0])
            sys.exit(0)
        else:
            mflags.append(arg)
    else:
        targets.append(arg)

    if mmglobal.verbose:
        mmglobal.quiet = False
        print "SRCDIR   '%s'" % (srcdir)
        print "BUILDDIR '%s'" % (builddir)

    if mmglobal.debug:
        mmglobal.quiet = False

    mmglobal.debugout("MMAKE:mmake.py: parsed command line options")

    myproject = mmproject.Project(srcdir, builddir, mflags)

    mmglobal.debugout("MMAKE:mmake.py: projects initialised")

    for t in targets:
        projectname, _, targetname = t.partition(".")
        if targetname == "":
            targetname = projectname

        mmglobal.debugout("MMAKE:mmake.py calling maketarget '%s'" % (targetname))
        myproject.maketarget(targetname)
