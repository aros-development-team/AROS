# -*- coding: iso-8859-1 -*-
# Copyright (C) 2013, The AROS Development Team. All rights reserved.
# $Id$

"""Generate Index.guide.

Create index page from the help guide documents.

Usage: python genindex.py <helpdir>

<helpdir> Index.guide will be placed in that directory. Sub-directories will be scanned
for help guides.
"""

import os
import sys
import datetime

def write_section(helpdir, filehandle, section):
    files = os.listdir(os.path.join(helpdir, section))
    files.sort()
    filehandle.write('@NODE %s %s\n' % (section, section))
    filehandle.write('\n\n@{B}Section "%s"@{UB}\n\n' % (section))
    for file in files:
        if file[-6:] == ".guide":
            name = file[:-6]
            filehandle.write('    @{"%s" LINK "HELP:English/%s/%s/MAIN"}\n' % (name, section, file))
    filehandle.write('@ENDNODE\n\n')

###############################################################################

helpdir = sys.argv[1]
print "genindex helpdir " + helpdir

sections = ("Commands", "System")
today = datetime.date.today()

filehandle = open(os.path.join(helpdir, "Index.guide"), "w")
filehandle.write("@DATABASE Index.guide\n\n")
filehandle.write("@$VER: Index.guide 1.0 (%d.%d.%d)\n" % (today.day, today.month, today.year))
filehandle.write("@(C) Copyright (C) %d, The AROS Development Team. All rights reserved.\n\n" % (today.year))

filehandle.write('@NODE MAIN "Help sections\n')
filehandle.write('\n\n@{B}Sections@{UB}\n\n')
for section in sections:
    filehandle.write('    @{"%s" LINK "%s"}\n' % (section, section))
filehandle.write('@ENDNODE\n\n')

for section in sections:
    write_section(helpdir, filehandle, section)

filehandle.close()
