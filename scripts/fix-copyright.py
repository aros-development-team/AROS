#!/usr/bin/env python3
# -*- coding: iso-8859-1 -*-

# Script for fixing copyright headers
# $Id$ and Lang: lines are removed
# Copyright sign unified to (C)
# Script starts with the current directory

import re, os

re_src_header = re.compile(r"^(/\*.*?Copyright.*?\*/)(.*)$", re.DOTALL)
re_src_id = re.compile(r"^ *?\$Id\$ *?\n", re.MULTILINE)
re_src_lang = re.compile(r"^ *?Lang:.*?\n", re.MULTILINE)
re_src_copyright = re.compile(r"^(.*Copyright\s+)(\S+?)(\s+\d{4}.*)$", re.MULTILINE)

re_mm_id = re.compile(r"^# *?\$Id\$\n", re.MULTILINE)
re_mm_copyright = re.compile(r"^(# *?Copyright\s+)(\S+?)(\s+\d{4}.*)$", re.MULTILINE)

def examine_src(filename):
    # handle *.c, *.h, *.cpp
    retval = 0
    file = open(filename, "r", encoding="iso-8859-1")
    content = file.read()
    file.close()

    match_header = re_src_header.match(content)
    if match_header:
        header = match_header.group(1)
        if "AROS Development Team" in header:
            oldheader = header
            source = match_header.group(2)

            # remove $Id$
            header = re_src_id.sub("", header)
            
            # remove Lang:
            header = re_src_lang.sub("", header)
            
            # fix copyright
            header = re_src_copyright.sub("\\1(C)\\3", header)

            # if header has changed write file back
            if header != oldheader:
                file = open(filename, "w", encoding="iso-8859-1")
                file.write(header)
                file.write(source)
                file.close()
                print("File {} written".format(filename))
                retval = 1
    return retval

def examine_mm(filename):
    # handle mmakefile*
    retval = 0
    file = open(filename, "r", encoding="iso-8859-1")
    content = file.read()
    file.close()
    oldcontent = content

    # remove $Id$
    content = re_mm_id.sub("", content)
    
    # fix copyright
    content = re_mm_copyright.sub("\\1(C)\\3", content)

    # if header has changed write file back
    if content != oldcontent:
        file = open(filename, "w", encoding="iso-8859-1")
        file.write(content)
        file.close()
        print("File {} written".format(filename))
        retval = 1

    return retval


count = 0
rootDir = os.path.abspath(".")
for dirName, subdirList, fileList in os.walk(rootDir):
    for fname in fileList:
        if \
                fname.endswith(".c") or \
                fname.endswith(".h") or \
                fname.endswith(".cpp"):
            count += examine_src(os.path.join(dirName, fname))

        if fname.startswith("mmakefile"):
            count += examine_mm(os.path.join(dirName, fname))
    
    for subdir in subdirList:
        if subdir.startswith(".git") or subdir == "external" or subdir == ".unmaintained":
            subdirList.remove(subdir)

print ("{} files changed".format(count))
