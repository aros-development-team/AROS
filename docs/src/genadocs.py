#!/usr/bin/python
#
# Small script to check the dates of all sources and compare them to
# the ones of the HTML pages. If the sources are newer, then their
# HTML pages are regenerated.
#

import os, string, sys, time, re

# Parse arguments
srcs = []
for arg in sys.argv[1:]:
    if arg[:8] == 'DESTDIR=':
	DESTDIR = arg[8:]
    elif arg[:10] == 'DOCGENDIR=':
	DOCGENDIR = arg[10:]
    else:
	srcs.append (arg)

special_item = {
    'NEWLIST()': '<A HREF="/cgi-bin/source/include/exec/lists.h">NEWLIST()</A>',
}

SRC2NAME = {
    'clib': 'ANSI C link library',
    'alib': 'Amiga.lib',
    'devs': 'Devices',
    'aros': 'AROS',
    'arossupport': 'AROS Support',
    'intuition': 'Intuition',
    'exec': 'Exec',
    'boopsi': 'BOOPSI',
    'mathffp': 'Math FFP/Basic Functions',
    'mathtrans': 'Math FFP/Transient Functions',
    'mathieeesingbas': 'Math IEEE/Single Precision/Basic Functions',
    'mathieeedoubbas': 'Math IEEE/Double Precision/Basic Functions',
    'mathieeesingtrans': 'Math IEEE/Single Precision/Transient Functions',
    'mathieeedoubtrans': 'Math IEEE/Double Precision/Transient Functions',
}
sections = ('NAME', 'LOCATION', 'SYNOPSIS',
    'FUNCTION', 'INPUTS', 'RESULT', 'NOTES', 'EXAMPLE',
    'BUGS', 'SEE ALSO', 'INTERNALS', 'HISTORY'
)

eol2par = re.compile (r"[ \t\r]*\n\n\s*");

alllibs = {}
allfuncs = {}
links = {}

def filterSrc (src):
    sys.stdout.write ("Regenerating %s..." % src)
    sys.stdout.flush ()
    mode = 'searchheader'
    out = None

    if src[-5:] != '.arch':
	raise 'This only supports .arch files for now'

    libname = SRC2NAME[os.path.basename (src[:-5])]
    alllibs[libname] = funcs = []

    # Where does this function belong to ? The name of the part of AROS is
    # the name of the directory in which the file is.
    lib = os.path.basename (os.path.dirname (src))
    
    fh = open (src, 'r')
    lineno = 0
    line = None
    while 1:
	lastline = line
	line = fh.readline ()
	if not line: break
	lineno = lineno + 1

	if mode == 'searchheader':
	    if line[:10] == '#Function ':
		dummy, dummy, rettype, funcname = string.split (line)
		parameters = []
	    elif line[:11] == '#Parameter ':
		words = string.split (line)
		type, name = words[1], words[2]
		if len (words) == 5:
		    reg = words[3] + '/' + words[4]
		else:
		    reg = words[3]
		parameters.append ((type, name, reg))
	    elif line[:11] == '#LibOffset':
		dummy, offset = string.split (line)
	    elif line[:8] == '#AutoDoc':
		written = 1 # Something has been written to the file

		if out:
		    raise 'Missing end of #AutoDoc in line %d' % lineno

		list = allfuncs.get (funcname, None)
		if not list:
		    list = allfuncs[funcname] = []
		    id = ''
		else:
		    id = '%d' + len (list)
		
		filename = string.lower (funcname) + id + '.html'
		link = '/autodocs/' + filename
		htmlfile = os.path.join (DESTDIR, filename)

		links[libname+":"+funcname] = link
		funcs.append (funcname)
	        list.append ((libname, link))

		sys.stdout.write ('.');
		sys.stdout.flush ();
		out = open (htmlfile, 'w')
		out.write ("""<HTML><HEAD>
    <TITLE>AROS - The Amiga Research OS - AutoDocs</TITLE>
</HEAD>
<BODY BACKGROUND="/pics/background.gif" BGCOLOR="#C0C0C0"
   TEXT="#000011" LINK="#3300DD" ALINK="#FF5566" VLINK="#550055">
<CENTER>(C) 1998-2000 AROS - The Amiga Research OS</CENTER><P>
<HR><P>
""")
		# Now read the data in the header
		mode = 'autodoc'
		section = {}
		for s in sections:
		    section[s] = ''
		sectionname = ''

		text = ''
		for inc in includes:
		    text = text + inc + '<BR>\n'
		text = text + '<BR>\n%s %s (' % (rettype, funcname)
		synopsis = ''
		for type, name, reg in parameters:
		    text = text + name + ', '
		    synopsis = synopsis + '<TT>%s %s</TT><BR>\n' % (type, name)
		text = text[:-2] + ')'
		section['NAME'] = text
		section['SYNOPSIS'] = synopsis
	    elif line[:7] == '#Header':
		mode = 'readinc'
		includes = []
	elif mode == 'readinc':
	    if line[:8] == '#/Header' or line[:6] == '#local':
		mode = 'searchheader'
	    elif line[:8] == '#include':
		pos = string.find (line, '<')
		if pos > 0:
		    pos2 = string.find (line, '>', pos)
		    file = line[pos+1:pos2]
		    includes.append ('#include &lt;<A HREF="/include/%s">%s</A>&gt;' % (
			file, file
		    ))
	elif mode == 'autodoc':
	    if line[:9] == '#/AutoDoc':
		mode = 'searchheader'
		if out:
		    out.write ('<DL>\n')
		    for s in sections:
			text = eol2par.sub ('<P>\n\n', string.strip (section[s]));
			out.write ('<DT>%s\n<DD>%s<P>\n\n' % (s, text))
		    out.write ('</DL>\n')
		    out.write ('</BODY></HTML>')
		    out.close ()
		    out = None

		continue

	    line = string.strip (line)
	    words = string.split (line)

	    if len (words) == 1 and section.has_key (words[0]):
		sectionname = words[0]
	    elif len (words) == 2 and words == ['SEE', 'ALSO']:
		sectionname = 'SEE ALSO'
	    else:
		assert (sectionname)
		section[sectionname] = section[sectionname] + line + "\n"
	    
    fh.close ()
    print

def tohtml (line):
    return string.replace (string.replace (string.replace (
	string.replace (line, '&', '&amp;'), '<', '&lt;'),
	'>', '&gt;'), '"', '&quot;'
    )


updatelist = []

for src in srcs:
    filterSrc (src)

out = open (os.path.join (DESTDIR, 'index.html'), 'w')

fh = open ('page_header.html', 'r')
header = string.replace (fh.read (), "\\thischapter", "AutoDocs")
fh.close ()
out.write (header)

fh = open ('adoc_header.html', 'r')
header = fh.read ()
fh.close ()
out.write (header)

# Create references for by-library
out.write ("""
<H2><A NAME="#bylib">Functions sorted by Library</A></H2>

<UL>
""")

libs = alllibs.keys ()
libs.sort ()
idxcnt = 0
for lib in libs:
    idx = 'lib%d' % idxcnt
    idxcnt = idxcnt + 1
    
    out.write ('<FONT SIZE=-1><A HREF="#%s">%s</A></FONT><BR>\n' % (
	idx, lib
    ))
out.write ('</UL>\n\n')

idxcnt = 0
for lib in libs:
    idx = 'lib%d' % idxcnt
    idxcnt = idxcnt + 1
    
    out.write ('\n<H2><A NAME="%s">%s</H2>\n\n<UL>' % (
	idx, lib
    ))

    funcs = alllibs[lib]
    n = 0
    out.write ("""<TABLE WIDTH="100%" BORDER="0" CELLSPACING="0" CALLPADDING="0">
<TR>
<TH WIDTH="33%" ALIGN="LEFT"></TH>
<TH WIDTH="33%" ALIGN="LEFT"></TH>
<TH WIDTH="33%" ALIGN="LEFT"></TH>
</TR>
""")
    for func in funcs:
	htmlfile = links[lib+":"+func]

	if n == 3:
	    out.write ('</TR>')
	    n = 0
	if n == 0:
	    out.write ('<TR>')
    
	out.write ('<TD><FONT SIZE=-1><A HREF="%s">%s</A></FONT><BR></TD>\n' % (
	    htmlfile, func
	))
	n = n + 1
    out.write ('</TABLE>\n')

    out.write ('</UL>\n\n')

out.write ("""
<H2><A NAME="#byname">Functions sorted by Name</H2>

""")

funcs = allfuncs.keys ()
funcs.sort ()
letters = {}
for func in funcs:
    letters[string.upper (func[0])] = 1;

out.write ('<UL>')
list = letters.keys ()
list.sort ()
for item in list:
    out.write ('<FONT SIZE=-1><A HREF="#%s">%s</A></FONT>\n' % (
	item, item
    ))
out.write ('</UL>\n')

letter = ''
for func in funcs:
    fl = string.upper (func[0])
    if fl != letter:
	if letter:
	    out.write ('</UL>\n\n')
	letter = fl
	out.write ('<H3><A NAME="%s">%s</A></H3>\n\n' % (fl, fl))
	out.write ('<UL>\n')

    list = allfuncs[func]

    for lib, file in list:
	out.write ('<FONT SIZE=-1><A HREF="%s">%s (%s)</A></FONT><BR>\n' % (
	    htmlfile, func, lib
	))

if letter:
    out.write ('</UL>\n\n')

fh = open ('adoc_footer.html', 'r')
header = string.replace (fh.read (), '\\today', 
    time.strftime ('%d. %b %Y', time.localtime (time.clock ())))
fh.close ()
out.write (header)

