'''Read in all archive files and takes them apart.

An archive has the following format:

    ... any amount of text which is ignored ...
    #Archvie
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

import os.path, string
from abbreviations import abbreviations

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
	    postFunctionCB = lambda x, y: None
	self.postFunctionCB = postFunctionCB

	self.libs = {}

	for filename in archives:
	    self.processArchive (filename)

    sections = (
	'FUNCTION', 'RESULT', 'NOTES', 'EXAMPLE',
	'BUGS', 'SEE ALSO', 'INTERNALS', 'HISTORY'
    )

    def processArchive (self, filename):
	print 'Processing', filename

	dir = os.path.dirname (filename)
	lib = Library (os.path.join (dir))
	header = []

	if self.libs.has_key (lib.name):
	    lib = self.libs[lib.name]

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
		    func.section = {}
		    # Clean autodoc section
		    for s in self.sections:
			func.section[s] = ''
		    currentsection = ''
		elif line[:7] == '#Header':
		    mode = 'readinc'
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
		isLocal = 0
		if line[:8] == '#/Header':
		    mode = 'searchheader'
		elif line[:6] == '#Local':
		    isLocal = 1
		elif line[:7] == '#/Local':
		    isLocal = 0
		else:
		    line = string.rstrip (line)
		    lib.header.append (isLocal, line)
		    header.append (isLocal, line)
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

class Function:
    def __init__ (self, rettype, name):
	self.result = Parameter (rettype, '', '')
	self.name = name
	self.parameters = []
	self.section = {}

    def __cmp__ (self, other):
	if not other:
	    return 1
	
	return cmp (self.name, other.name)

class Parameter:
    def __init__ (self, type, name, registers):
	self.type, self.name = type, name
	self.registers = string.split (registers, '/')
	self.explanation = ''
