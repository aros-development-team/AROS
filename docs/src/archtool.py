'''Read in all archive files and take them apart.

An archive has the following format:

    ... any amount of text which is ignored ...
    #Archive
    contents
    #/Archive

The contents of an archive are one Header node or any number of Function
nodes. A header node looks like this:

    #Header
    ... text ...
    #Local
    ... local text ...
    #/Local
    ... more text ...
    #/Header

The contents of the header is prepended to all generated source files.
It usually contains include statements. The text in the local node
are also prepended to all generated sources but they are internal
to this archive. For example, they will not be shown in the documentation.

The function nodes contain information about the functions which make
up the library. They have this format:

    #Function
    #Options options...
    #Parameter type name register
	Explanation for this parameter
    #/Parameter
    #Parameter...
    #/Parameter
    #LibOffset offset
    #AutoDoc
	FUNCTION
	    text
	
	RESULT
	    text

	NOTES
	    text
	
	EXAMPLE
	    code

	BUGS
	    text
	
	SEE ALSO
	    function and library names, cross references, URLs
	
	INTERNALS
	    text
	
	HISTORY
	    text
    #/AutoDoc
    #Code
	code of the function
    #/Code
    #/Function

options can be

    I - This function is independend of the library base (ie. it doesn't
	need it).
    QUAD - This function returns its result in the registers D0/D1.

type is any valid C type (for example, struct Node *), name is the
name of the parameter and register is the m68k register spec which
tells the compiler in which register this parameter will be passed.
If the parameter needs more than one register, you must use reg1/reg2
syntax (ie. D2/D3).	

To use, import this module and run archtool.Archive() with a list
of archive file names.
'''

import os.path, string, re, cStringIO
from abbreviations import abbreviations

class Data:
    def __init__ (self, filename):
	self.filename = filename
	self.fh = open (filename, 'r')
	self.lineno = 0
	self.__line = None

    def unreadline (self, line):
	self.__line = line

    def readline (self):
	if self.__line:
	    line = self.__line
	    self.__line = None
	    #print line
	    return line

	line = self.fh.readline ()
	if line:
	    self.lineno = self.lineno + 1
	#print line
	return line

includePattern = re.compile (r'\s*#\s*include\s+(?P<file>(<[^>]*>)|"[^"]*"|[^<"]\S*)\s+(?P<all>\/\*#ALL\*\/)?')

headerPattern = re.compile (
    r'^\s*(\/\*)?\s+(?P<part>NAME|SYNOPSIS|LOCATION|FUNCTION|RESULT|NOTES|EXAMPLE|BUGS|SEE ALSO|INTERNALS|HISTORY)\s+(\*\/)?$'
)

beginCommentPattern = re.compile (r'\/\*')
endCommentPattern = re.compile (r'\*\/')

class Archive:
    '''Read a set of archives and merge them. If the archives contain
    informations about different libraries, then self.libs will contain
    all libraries found.
    '''
    def __init__ (self, archives, postFunctionCB=None):
	'''Read in all archives and build a new list of known libraries.

	    postFunctionCB - This is called when a new function has been
		    read. The function will be called with a reference
		    to the archive object, the library and the function.
	    dirs - A list of directories to examine.
	'''

	if not postFunctionCB:
	    postFunctionCB = lambda x, y, z: None
	self.postFunctionCB = postFunctionCB

	self.libs = {}
	self.seeAlso = {}

	for filename in archives:
	    self.processArchive (filename)

    sections = (
	'FUNCTION', 'RESULT', 'NOTES', 'EXAMPLE',
	'BUGS', 'SEE ALSO', 'INTERNALS', 'HISTORY'
    )

    def processCArchive (self, filename):
	data = Data (filename)
	data.lib = None
	data.func = None
	self.mode = self.searching
	data.includes = []
	line = None
	while self.mode:
	    line = data.readline ()
	    if not line: break

	    self.mode (data, line)
	
    def searching (self, data, line):
	match = includePattern.match (line)
	if match:
	    dict = match.groupdict ()
	    #print dict
	    if dict['all']:
		#print '#include',dict['file']
		data.includes.append (dict['file'])
	else:
	    if line[:10] == '/*#AROS_LH':
		self.mode = self.readFuncHeader
    
    def readFuncHeader (self, data, line):
	if line[0] == '{':
	    self.processFunction (data)
	    return

	match = headerPattern.match (line)
	if match:
	    #print match.groupdict ()
	    part = match.group ('part')
	    #print 'Part:',part
	    if part == 'NAME':
		self.mode = self.readNAME
		data.extraIncludes = []
	    elif part == 'SYNOPSIS':
		self.mode = self.readSYNOPSIS
	    else:
		text = self.readPart (data)

		if part == 'LOCATION':
		    self.processLocation (data, text)
		elif part == 'SEE ALSO':
		    self.processSeeAlso (data, text)
		    data.func.section[part] = text
		else:
		    data.func.section[part] = text
   
    def processSeeAlso (self, data, text):
	text = string.strip (text)
	if not text:
	    return
	for item in string.split (text, ','):
	    item = string.strip (item)
	    if item[0] == '@':
		list = self.seeAlso.get (item, None)
		if not list:
		    list = []
		    self.seeAlso[item] = list
		list.append ((data.lib, data.func))
	
    def readNAME (self, data, line):
	#print 'readName', line
	match = includePattern.match (line)
	if match:
	    #print '#include',match.group ('file')
	    data.extraIncludes.append (match.group ('file'))
	    return
	
	pos = string.rfind (line, '(')
	if pos != -1:
	    if data.func:
		raise 'Error %s:%d: Two function declarations ? (%s and %s)' % (
		    data.filename, data.lineno, data.func.name, line)

	    line = line[:pos]
	    words = string.split (string.strip (line))

	    rettype = string.join (words[0:-1])
	    funcname = words[-1]
	    data.func = Function (rettype, funcname)
	    # Clean autodoc section
	    for s in self.sections:
		data.func.section[s] = ''
	    for inc in data.includes:
		data.func.header.append ((0, '#include %s' % inc))
	    for inc in data.extraIncludes:
		data.func.header.append ((0, '#include %s' % inc))
	    self.mode = self.readFuncHeader

	    #print rettype, funcname, '()'

    def readSYNOPSIS (self, data, line):
	#print 'readSYNOPSIS', line
	if headerPattern.match (line):
	    data.unreadline (line)
	    self.mode = self.readFuncHeader
	    return

	match = beginCommentPattern.search (line)
	if match:
	    pos = match.start ()
	    paramString = string.strip (line[:pos])
	    pos = string.find (line, '#', pos)
	    register = string.strip (line[pos+1:])
	    if endCommentPattern.search (line[pos:]):
		comment = ''
	    else:
		comment = self.readComment (data)

	    if paramString[-1] in (',', ')'):
		paramString = paramString[:-1]
	    if paramString[-1] == ')':
		raise 'Error %s:%d: Function pointers not yet supported' % (
		    data.filename, data.lineno)
	    words = string.split (paramString)
	    type, name = string.join (words[:-1]), words[-1]
	    parameter = Parameter (type, name, register)
	    parameter.explanation = comment
	    #print '   Parameter',type,name,register,comment
	    data.func.parameters.append (parameter)

    def readComment (self, data):
	comment = ''
	while 1:
	    line = data.readline ()
	    if not line:
		raise 'Error %s:%d: Unexpected EOF' % (
		    data.filename, data.lineno)

	    match = endCommentPattern.search (line)
	    if match:
		line = line[:match.start ()]
		line = string.strip (line)
		if not line:
		    break
	    
	    line = string.strip (line)
	    comment = comment + line + '\n'
	    
	    if match:
		break

	return comment

    def readPart (self, data):
	part = ''
	while 1:
	    line = data.readline ()
	    if not line:
		raise 'Error %s:%d: Unexpected EOF' % (
		    data.filename, data.lineno)

	    match = endCommentPattern.search (line)
	    if not match:
		match = headerPattern.match (line)
	    if match:
		data.unreadline (line)
		break

	    part = part + line
	
	part = string.expandtabs (part)
	
	# De-indent part
	pos, N = 0, len (part)
	start = 0
	while pos < N:
	    if not part[pos] in string.whitespace:
		break

	    if part[pos] in '\n\r':
		start = pos+1

	    pos = pos + 1
	
	firstIndent = part[start:pos]
	part = part[start:]
	skip = len (firstIndent)

	fh = cStringIO.StringIO (part)
	part = ''
	while 1:
	    line = fh.readline ()
	    if not line:
		break

	    n = self.countws (line)

	    if n > skip:
		n = n - skip
	    
	    part = part + string.rstrip (line[n:]) + '\n'
	
	#print `part`
	return part

    def readCode (self, data):
	code = ''
	while 1:
	    line = data.readline ()
	    if not line:
		raise 'Error %s:%d: Unexpected EOF' % (
		    data.filename, data.lineno)

	    # {
	    if line[0] == '}':
		break
	
	    code = code + line

	data.func.code = code
	
    def countws (self, line):
	i, n = 0, len (line)
	while i < n:
	    if line[i] != ' ':
		break
	    i = i + 1
	
	return i

    def processLocation (self, data, text):
	items = string.split (text, ',')
	libname, offset = string.split (string.strip (items[0]))
	offset = offset[1:-1] # Remove "()"
	lib = findLibrary (libname)
	
	if self.libs.has_key (lib.name):
	    lib = self.libs[lib.name]
	else:
	    self.libs[lib.name] = lib

	data.lib = lib
	data.func.offset = int (offset)
	for item in items[1:]:
	    item = string.strip (item)
	    data.func.flag[item] = 1
	
    def processFunction (self, data):
	data.func.code = self.readCode (data)

	# This may overwrite an existing function (merging)
	data.lib.functions[data.func.name] = data.func

	#print self.postFunctionCB
	self.postFunctionCB (self, data.lib, data.func)
	data.func = None

	self.mode = self.searching

    def processArchive (self, filename):
	print 'Processing', filename

	if filename[-2:] == '.c':
	    self.processCArchive (filename)
	    return

	dir = os.path.dirname (filename)
	lib = Library (os.path.join (dir))

	if self.libs.has_key (lib.name):
	    lib = self.libs[lib.name]
	else:
	    self.libs[lib.name] = lib

	header = []
	func = None

	mode = 'searchheader'
	fh = open (filename, 'r')
	lineno = 0
	line = None
	while 1:
	    line = fh.readline ()
	    if not line: break
	    lineno = lineno + 1

	    if mode == 'searchheader':
		if line[:10] == '#Function ':
		    words = string.split (line)
		    rettype = string.join (words[1:-1])
		    funcname = words[-1]
		    func = Function (rettype, funcname)
		    func.header = header
		    lib.functions[func.name] = func
		elif line[:9] == '#Options':
		    words = string.split (line)
		    for word in words[1:]:
			func[flags] = None
		elif line[:10] == '#/Function':
		    class Storage: pass
		    Storage.func = func
		    Storage.lib = lib
		    self.processSeeAlso (Storage, func.section['SEE ALSO'])	
		    self.postFunctionCB (self, lib, func)
		    func = None
		elif line[:11] == '#Parameter ':
		    words = string.split (line)
		    type, name = string.join (words[1:-2]), words[-2]
		    reg = words[-1]
		    parameter = Parameter (type, name, reg)
		    func.parameters.append (parameter)
		    mode = 'readparameter'
		elif line[:11] == '#LibOffset':
		    dummy, func.offset = string.split (line)
		elif line[:8] == '#AutoDoc':
		    # Now read the data in the header
		    mode = 'autodoc'
		    # Clean autodoc section
		    for s in self.sections:
			func.section[s] = ''
		    currentsection = ''
		elif line[:7] == '#Header':
		    mode = 'readinc'
		    isLocal = 0
		    header = []
		elif line[:5] == '#Code':
		    mode = 'readcode'
		    func.code = ''
	    elif mode == 'readparameter':
		if line[:11] == '#/Parameter':
		    mode = 'searchheader'
		else:
		    line = string.strip (line)
		    parameter.explanation = parameter.explanation + line + '\n'
	    elif mode == 'readcode':
		if line[:6] == '#/Code':
		    mode = 'searchheader'
		else:
		    func.code = func.code + line + '\n'
	    elif mode == 'readinc':
		if line[:8] == '#/Header':
		    mode = 'searchheader'
		elif line[:6] == '#Local':
		    isLocal = 1
		elif line[:7] == '#/Local':
		    isLocal = 0
		else:
		    line = string.rstrip (line)
		    lib.header.append ((isLocal, line))
		    header.append ((isLocal, line))
	    elif mode == 'autodoc':
		if line[:9] == '#/AutoDoc':
		    mode = 'searchheader'
		    continue

		line = string.strip (line)
		words = string.split (line)

		if len (words) == 1 and func.section.has_key (words[0]):
		    currentsection = words[0]
		elif len (words) == 2 and words == ['SEE', 'ALSO']:
		    currentsection = 'SEE ALSO'
		else:
		    assert (currentsection)
		    func.section[currentsection] = func.section[currentsection] + line + "\n"
		
	fh.close ()

    def dump (self):
	print self.libs
	for name, lib in self.libs.items ():
	    print 'Library',name

	    for func in lib.functions.values ():
		print '    %s %s (' % (func.result.type, func.name)
		for param in func.parameters:
		    print '\t%s' % param
		print '    );'

def findArchives (*dirs):
    '''Small utility function to search a set of directories for
    archives.'''
    def visit (list, path, entries):
	for entry in entries:
	    if entry[-5:] == '.arch':
		list.append (os.path.join (path, entry))

    list = []
    for dir in dirs:
	os.path.walk (dir, visit, list)
    
    return list

class Library:
    def __init__ (self, dir):
	filename = os.path.join (dir, 'lib.conf')
	fh = open (filename, 'r')
	# Copy all settings into attributes of this object
	while 1:
	    line = fh.readline ()
	    if not line:
		break

	    words = string.split (string.rstrip (line), None, 1)
	    if words[0] == 'options':
		words[1] = string.split (words[1])

	    setattr (self, words[0], words[1])
	fh.close ()

	self.longName = abbreviations[self.name]
	self.functions = {}
	self.header = []

    def __cmp__ (self, other):
	if not other:
	    return 1
	
	return cmp (self.longName, other.longName)

arosdir = os.path.abspath (os.path.expandvars ("$AROSDIR"))
def setArosdir (dir):
    global arosdir
    arosdir = dir

libraryDirs = {
    'exec': 'rom/exec',
}

def addLibraryPath (name, path):
    libraryDirs (string.tolower (name), path)

libraries = {}
def findLibrary (name):
    name = string.lower (name)

    lib = libraries.get (name)
    if lib:
	return lib
    
    dir = libraryDirs.get (name, None)
    if not dir:
	raise 'Unknown library: %s' % name
    if dir[0] != '/':
	dir = os.path.join (arosdir, dir)
    
    lib = Library (dir)
    libraries[name] = lib

    return lib

class Function:
    def __init__ (self, rettype, name):
	self.result = Parameter (rettype, '', '')
	self.name = name
	self.parameters, self.header = [], []
	self.section, self.flag = {}, {}

    def __cmp__ (self, other):
	if not other:
	    return 1
	
	return cmp (self.name, other.name)

class Parameter:
    def __init__ (self, type, name, registers):
	self.type, self.name = type, name
	self.registers = string.split (registers, '/')
	self.explanation = ''

    def __str__ (self):
	return '%s %s /*#%s %s */' % (
	    self.type, self.name, string.join (self.registers, '/'),
	    self.explanation
	)

if __name__ == '__main__':
    import sys
    
    archive = Archive (sys.argv[1:])
    archive.dump ()
