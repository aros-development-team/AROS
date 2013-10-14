# -*- coding: iso-8859-1 -*-
# Copyright © 2013, The AROS Development Team. All rights reserved.

# Copy directory 'src' recursively to 'dst' while ignoring
# all files given by 'ignore' parameter. Only files younger
# than those in 'dst' are copied.

# The files '.cvsignore', 'mmakefile.src' and the directories
# 'CVS', '.svn' are ignored.

# This is a support script for the %copy_dir_recursive macro
# in make.tmpl. Main purpose is to fix problem with file names
# which contain space characters.

import sys, os, shutil

def copy_tree(src, dst, ignore):
    names = os.listdir(src)

    if not os.path.exists(dst):
        os.makedirs(dst)

    for name in names:
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)

        if os.path.isdir(srcname):
            if name not in ("CVS", ".svn"):
                # print "Copying dir %s to %s" % (srcname, dstname)
                copy_tree(srcname, dstname, ignore)
        else:
            if (name not in (".cvsignore", "mmakefile.src")) and (srcname not in ignore):
                if not os.path.exists(dstname) or (os.path.getctime(srcname) > os.path.getctime(dstname)):
                    # print "Copying file %s to %s" % (srcname, dstname)
                    shutil.copy(srcname, dstname)

################################################################################

st_source = 1
st_dest = 2
st_exclude = 3
state = 0

sourcedir = "."
destdirs = []
ignore = []

for arg in sys.argv:
    if arg == "-s":
        state = st_source
    elif arg == "-d":
        state = st_dest
    elif arg == "-e":
        state = st_exclude
    else:
        if state == st_source:
            sourcedir = arg
        elif state == st_dest:
            destdirs.append(arg)
        elif state == st_exclude:
            ignore.append(arg)

for destdir in destdirs:
    print "Copying    directory '%s' to '%s', ignore '%s'" % (sourcedir, destdir, ignore)
    copy_tree(sourcedir, destdir, ignore)
