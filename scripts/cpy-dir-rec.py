# -*- coding: iso-8859-1 -*-
# Copyright © 2013, The AROS Development Team. All rights reserved.

# Copy directory 'src' recursively to 'dst' while ignoring
# all files given by 'ignore' parameter. Only files younger
# than those in 'dst' are copied. You can specify multiple
# 'dst' directories.

# The files '.cvsignore', 'mmakefile.src' and the directories
# 'CVS', '.svn' are ignored.

# This is a support script for the %copy_dir_recursive macro
# in make.tmpl. Main purpose is to fix problem with file names
# which contain space characters.

import sys, os, shutil


def in_ignore_list(name, ignore):
    # check if rightmost part of name is in ignore list
    for ign in ignore:
        if len(name) >= len(ign):
            if name[-len(ign):] == ign:
                # print "%s found in ignore list" % name
                return True
    return False


def copy_tree(src, dst, ignore):
    # Conversion to Unicode is needed in order to yield Unicode file names.
    # This can be important on Windows. Without this the script fails to access
    # directories like Locale/Help/Español on non-western systems, where locale
    # is different from Latin-1 (e. g. russian).
    # See http://docs.python.org/2/howto/unicode.html#unicode-filenames
    names = os.listdir(unicode(src))

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
            if (name not in (".cvsignore", "mmakefile.src")) and not in_ignore_list(srcname, ignore):
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
    elif arg == "-h":
        print "Usage: python cpy-dir-rec.py -s <souredir> -d <target directories> [-e <files to exclude>]"
    elif arg[0] == "-":
        print "cpy-dir-rec: unknown argument %s" % arg
        sys.exit(1)
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
