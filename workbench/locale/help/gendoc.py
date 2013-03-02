# -*- coding: iso-8859-1 -*-
# Copyright (C) 2013, The AROS Development Team. All rights reserved.
# $Id$

"""Documentation to guide.

Extract documentation from C source files and create guide files

Usage: python gendoc.py <sourcedir> <targetdir>

<sourcedir> will be scanned recursively for C source files.
<targetdir> where to place the guide. Directory will be created
            if it doesn't exist.
"""

import re
import os
import sys
import datetime

# Regex for whole autodoc block (without surrounding comments)
ad_regx = re.compile(r"""
^/\*{6,}
(
.*?
^\s{4,4}NAME
.*?
)
^\*{7,}/
""", re.VERBOSE | re.MULTILINE | re.DOTALL)

# Regex for a title
titles_regx = re.compile(r"""
^\s{4,4}
(NAME|FORMAT|SYNOPSIS|LOCATION|FUNCTION|INPUTS|TAGS|RESULT|EXAMPLE|NOTES|BUGS|SEE\ ALSO|INTERNALS|HISTORY|TEMPLATE)$
""", re.VERBOSE)


def parsedoc(filename, targetdir):
    # print "reading " + filename
    blocks = {}
    filehandle = open(filename)
    content = filehandle.read()
    doc = ad_regx.search(content)
    current_title = None
    if doc:
        for line in doc.group(1).splitlines():
            match = titles_regx.match(line)
            if match:
                current_title = match.group(1)
                blocks[current_title] = ""
            elif current_title:
                blocks[current_title] += line.expandtabs()[4:] + "\n"
        
        # check for empty chapters, because we don't want to print them
        for title, content in blocks.iteritems():
            if content.strip() == "":
                blocks[title] = ""

    filehandle.close()

    if blocks.has_key("NAME"):
        # get docname
        docname = blocks["NAME"].split()[0]
        if docname == "":
            raise ValueError("docname is empty")

        docfilename = docname + ".guide"
        today = datetime.date.today()
        filehandle = open(os.path.join(targetdir, docfilename), "w")

        # The titles we want to printed
        shell_titles = ("Name","Format","Template","Synopsis","Location","Function",
            "Inputs","Tags","Result","Example","Notes","Bugs","See also")  

        filehandle.write("@DATABASE %s\n\n" % (docfilename))
        filehandle.write("@$VER: %s 1.0 (%d.%d.%d)\n" % (docfilename, today.day, today.month, today.year))
        filehandle.write("@(C) Copyright (C) %d, The AROS Development Team. All rights reserved.\n" % (today.year))
        filehandle.write("@MASTER %s\n\n" %(filename))
        filehandle.write("@NODE MAIN \"%s\"\n\n" % (docname))

        for title in shell_titles:
            title_key = title.upper()
            if blocks.has_key(title_key) and blocks[title_key] != "":
                filehandle.write("@{B}" + title + "@{UB}\n")
                filehandle.write(blocks[title_key])
                filehandle.write("\n")

        filehandle.write('@TOC "HELP:English/Index.guide/MAIN"\n')
        filehandle.write("@ENDNODE\n")

        filehandle.close()

###############################################################################

sourcedir = sys.argv[1]
targetdir = sys.argv[2]

print "gendoc sourcedir " + sourcedir + " targetdir " + targetdir

if not os.path.exists(targetdir):
    os.mkdir(targetdir)

for root, dirs, files in os.walk(sourcedir):
    for filename in files:
        if len(filename) > 2 and filename[-2:] == ".c":
            parsedoc(os.path.join(root, filename), targetdir)
    if '.svn' in dirs:
        dirs.remove('.svn')
