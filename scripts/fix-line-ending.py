#!/usr/bin/env python3
# -*- coding: iso-8859-1 -*-

# Script for fixing line ending from \r and \r\n to \n.
# Script starts with the current directory.

import os, sys

def examine(filename):
    infile = open(filename, "rb")
    content = infile.read()
    infile.close()

    newcontent = content.replace(b"\r\n", b"\n") # Windows
    newcontent = newcontent.replace(b"\r", b"\n") # Mac
    if content != newcontent:
        outfile = open(filename, "wb")
        outfile.write(newcontent)
        outfile.close()


rootDir = os.path.abspath(".")
for dirName, subdirList, fileList in os.walk(rootDir):
    for fname in fileList:
        if \
                fname.endswith(".c") or \
                fname.endswith(".h") or \
                fname.endswith(".cpp") or \
                fname.endswith(".py") or \
                fname.endswith(".conf") or \
                fname.startswith("mmakefile"):
            examine(os.path.join(dirName, fname))
    
    for subdir in subdirList:
        if subdir.startswith(".git"):
            subdirList.remove(subdir)
