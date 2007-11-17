#!/usr/bin/python

# $Id$

# Converts config file for shared libraries to a C file
# with function prototypes.
# A *.conf file is searched in the current directory.
# Result is print to STDOUT.

import re
import glob
import sys

lvo = 5                      # 1st functions has always LVO of 5
libtype = "struct Library *" # default
libvar = "library"           # default 
mode = ""
tab = "    "                 # tabulator for identation

# regex for splitting line into rettype, retval, args, regs
linepatt = re.compile('(.+?)\s*(\w*)\s*\((.*?)\)\s*\((.*?)\)')

# regex for splitting arg into rettype and argname
argpatt = re.compile('\s*(.*?)\s*(\w+)\s*$')

# regex for splitting line into two parts
splitpatt = re.compile('([#\w]*?)\s+(.*)\s*$')

infiles = glob.glob("*.conf")
if len(infiles) != 1:
    sys.stderr.write("There must be one *.conf in current directory")
    sys.exit(1)

libname = infiles[0].split(".")
libname = libname[0].capitalize()

infile = open(infiles[0], "r")

for line in infile:
    parts = splitpatt.match(line)
    if parts and parts.group(1) == "##begin":
        mode = parts.group(2)
    elif parts and parts.group(1) == "##end":
        mode = ""
    elif mode == "config":
        if parts and parts.group(1) == "libbasetype":
            libtype = parts.group(2)
    elif mode == "functionlist":
        res = linepatt.match(line)
        if res:
            rettype = res.group(1)
            funcname = res.group(2)
            args = res.group(3).split(",")
            regs = res.group(4).split(",")
            argcnt = len(args)
            print "#ifdef __AROS__"
            print "AROS_LH%d(%s, %s, " %(argcnt, rettype, funcname)
            for i in range(argcnt):
                argres = argpatt.match(args[i])
                print "%sAROS_LHA(%s, %s, %s)," %(tab, argres.group(1), argres.group(2), regs[i])
            print "%s%s, %s, %d, %s" %(tab, libtype, libvar, lvo, libname)
            print ")\n{"
            print "%sAROS_LIBFUNC_INIT" %(tab)
            print "#else"
            print "#endif\n"
            print "#ifdef __AROS__"
            print "%sAROS_LIBFUNC_EXIT" %(tab)
            print "#endif"
            print "}\n"
  
        # even empty line increase LVO
        lvo = lvo + 1

 
infile.close()
