#! /usr/bin/env python

# This program reads the output of cxref:
# http://www.gedanken.demon.co.uk/cxref/
# and produces input for Graphviz:
# http://www.research.att.com/sw/tools/graphviz/
# and of course you'll need Python:
# http://www.python.org/
# 10/12/99, Dr. Tom Holroyd, tomh@po.crl.go.jp

# Notes:
# * You can give a dummy file to cxref (or edit cxref.function) and use -k
# to include some specific functions you're interested in such as malloc(),
# etc., without including all the standard libraries.
# * If the graph is big, the node labels may be small.  Try editing the .dot
# file and adding 'node [fontsize = 24]' or similar, or set 'ratio = auto'
# and let it output multiple pages.  Also, see the dot user's guide.

import sys
import getopt
import re
import string

__usage = """[-k] [-n] [-t a4|a4r|us|usr] filename
Parse the cxref.function file produced by "cxref -xref-func <file>...", and
produce .dot file output suitable for use with graphviz programs like 'dot'
or 'neato' that will create a postscript version of the call graph.  If -k
is specified, only nodes that are 'known', in the sense of being defined
within the group of files sent to cxref, are output.  Otherwise all called
functions are included, e.g., stdio functions, etc.  If -n is specified, the
node is labeled with the file where the function is defined, if known.  -t
sets the paper size and orientation (a4r default).  Send the output of this
script to, e.g., dot -Tps > xref.ps"""

__scriptname = sys.argv[0]
def printusage():
    sys.stderr.write("usage: %s %s\n" % (__scriptname, __usage))

nodeflag = 0
knownflag = 0

# Various paper sizes with .5 inch margins.  Note: I used inches here because
# dot uses inches.  Complain to AT&T.

A4paper = {
    'page' : "8.26, 11.69", 'rotate' : "", 'size' : "7.2, 10.6"
}
A4Rpaper = {
    'page' : "8.26, 11.69", 'rotate' : "rotate = 90;", 'size' : "10.6, 7.2"
}
USpaper = {
    'page' : "8.5, 11", 'rotate' : "", 'size' : "7.5, 10"
}
USRpaper = {
    'page' : "8.5, 11", 'rotate' : "rotate = 90;", 'size' : "10, 7.5"
}
papertypes = { 'a4' : A4paper, 'a4r' : A4Rpaper,
    'us' : USpaper, 'usr' : USRpaper }

paperdef = A4Rpaper

try:
    optlist, args = getopt.getopt(sys.argv[1:], "nkt:")
    for opt, arg in optlist:
	if opt == '-n':
	    nodeflag = 1
	if opt == '-k':
	    knownflag = 1
	if opt == '-t':
	    if papertypes.has_key(arg):
		paperdef = papertypes[arg]
	    else:
		raise getopt.error, "unknown paper type '%s'" % arg

except getopt.error, msg:
    sys.stderr.write("%s: %s\n" % (__scriptname, msg))
    printusage()
    sys.exit(1)

if len(args) != 1:
    printusage()
    sys.exit(1)

filename = args[0]

profile = open(filename).readlines()

# Build the call tree.

nodelist = []   # list of known nodes
calls = {}      # key: node, value: call list (includes unknown nodes)
filename = {}   # key: node, value: filename

sp = re.compile(r'\s%|\s')
for line in profile:
    l = sp.split(string.strip(line))
    node = l[1]
    nodelist.append(node)
    if filename.has_key(node) or calls.has_key(node):
	sys.stderr.write("duplicate function '%s', ignoring previous definition\n" % node)
    filename[node] = l[0]
    calls[node] = []
    for i in range(3, len(l)):
	calls[node].append(l[i])

# Output the graph.

print 'digraph call {'
print 'page = "%(page)s"; %(rotate)s size = "%(size)s"' % paperdef
print 'ratio = fill; center = 1'

for node in nodelist:
    if nodeflag:
	label = '%s\\n%s' % (node, filename[node])
	print '%s [label = "%s"]' % (node, label)
    for n in calls[node]:
	if not knownflag or n in nodelist:
	    print node, '->', n

print '}'
