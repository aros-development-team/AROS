
import re, sys, string
import cStringIO

ParseError = 'ParseError'

class Parser:
    begin = '<!--**'
    end = '**--!>'

    """The long versions must precede the short versions.
    Otherwise, the scanner will stop when the short version
    is found (ie. ''' will be read as the empty string and
    the beginning of another string)."""
    stringdelimiters = ["'''", '"""', "'", '"']

    def __init__ (self, rf, wf, handlers, startLine=1):
	"""Initialize a new instance of a XIL parser. When you run() the
	new parser, it will read from rf and write the result to wf.
	The handlers will be called for each tag in the input stream.
	The startLine is used for error reports when a new parser is
	create to parse the middle of a file."""
	self.beginRE = re.compile ('(' + re.escape (self.begin) + ')', re.IGNORECASE)
	self.endRE = re.compile ('(' + re.escape (self.end) + ')', re.IGNORECASE)
	self.rf, self.wf = rf, wf
	self.rest = ''
	self.lineno = startLine-1

	self.handlers = {}
	for key, item in handlers.items ():
	    self.handlers[string.upper (key)] = item

	self.strDelimPattern = re.compile ('(' 
	    + string.join (self.stringdelimiters, '|')
	    + ')'
	)
	self.wordPattern = re.compile ('[A-Z0-9-_/]+', re.IGNORECASE)
	self.spacePattern = re.compile ('\s+')
	self.optSpacePattern = re.compile ('\s*')
	self.nonSpacePattern = re.compile ('\S+')
	self.newLinePattern = re.compile ('\n|\r|\n\r|\r\n')

	self.argNameEndPattern = re.compile ('\s+|' + re.escape (self.end) + '|=')

    def run (self):
	"""Read the stream and execute for commands in it."""
	while 1:
	    # Read the text upto the next command or EOF
	    text, eof = self.readText ()
	    if text:
		#print '<<<TEXT<<<',
		self.write (text)
		#print '>>>TEXT>>>',
	    if eof: break

	    # We *must* have a command here. Read it.
	    cmd, args = self.readCmd ()
	    #print 'cmd=',cmd,args

	    # Find the handler for this command
	    o = self.handlers.get (cmd, None)
	    if not o:
		self.error ('Unknown command %s in line %d\n' % (cmd, self.lineno))
	    else:
		# Check if this is a command or an environment handler.
		if isinstance (o, CmdHandler):
		    o.handle (self, args)
		else:
		    body = self.readBody (o)
		    o.handle (self, args, body)

    def readText (self):
	""" This method must read all the text upto the next command. The
	start delimiter for the command must be left in the input stream."""

	text = self.read ()
	if not text:
	    eof = 1
	else:
	    match = self.beginRE.search (text)
	    if not match:
		eof = 1
	    else:
		self.rest = text[match.start (0):]
		text = text[:match.start (0)]
		eof = 0

	pos = 0
	while 1:
	    match = self.newLinePattern.search (text, pos)
	    if not match: break
	    self.lineno = self.lineno + 1
	    pos = match.end (0)

	return text, eof

    def calcNewLines (str, start, end):
	"""For return the number of lines between two positions in the
	string."""

	pos = start
	count = 0
	while 1:
	    match = self.newLinePattern (text, pos, end)
	    if not match: break
	    self.lineno = self.lineno + 1
	    pos = match.end (0)

	return count

    def readCmd (self):
	"""Parse a command. This method must return a string (the name of the
	command) and the arguments. This implementation returns a dictionary
	for the arguments. The key is the name of the argument and the value
	is the value of the argument. But that's not neccessary. The 
	arguments are passed on to the handlers which must be able to
	process them. No other parts of Xil look at the arguments."""

	# Read the command
	text = self.read ()
	#print 'text=|%s|'%`text[:20]`
	if not text:
	    self.error ('Missing command in line %d\n' % self.lineno)

	match = self.beginRE.match (text)
	if not match:
	    self.error ('Missing command in line %d\n' % self.lineno)

	pos = self.optSpacePattern.match (text, match.end (0)).end (0)
	if pos >= len (text):
	    self.error ('Missing command in line %d\n' % self.lineno)
	match = self.argNameEndPattern.search (text, pos)

	startpos = pos;
	pos = match.end (0)
	startline = self.lineno
	cmd = string.upper (text[startpos:match.start (0)])
	#print 'cmd=',cmd
	
	if cmd == self.end:
	    self.error ('Missing command name in line %d\n' % self.lineno)

	# Is this the end of the tag ?
	if match.group (0) == self.end:
	    self.yytext = text[0:pos]
	    self.rest = text[pos:]
	    return cmd, {}

	# Read the arguments
	args = {}
	while 1:
	    # Skip all whilespace
	    pos = self.optSpacePattern.match (text, pos).end (0)

	    # The name of the argument is terminated by a) whitespace,
	    # b) the end delimiter or c) a '='
	    #print '1',`string.strip (text[pos:pos+10])`
	    match = self.argNameEndPattern.search (text, pos)
	    if not match:
		self.lineno = self.lineno + self.calcNewLines (text, 0, pos)
		error ('Unterminated tag in line %d\n' % self.lineno)

	    argname = string.upper (text[pos:match.start (0)])
	    pos = match.end (0)
	    #print 'argname=',argname
	    # If the argument name is empty, we must have hit something else.
	    # It can't be a space, because spaces have already been skipped
	    # by the first operation in this loop
	    if not argname:
		if match.group (0) == '=':
		    self.lineno = self.lineno + self.calcNewLines (text, 0, pos)
		    error ('Missing argument name in line %d\n' % self.lineno)
		break
	    
	    # If a '=' follows, then there must be some value
	    if match.group (0) == '=':
		# Skip whitespace
		pos = self.optSpacePattern.match (text, pos).end (0)

		# Is the end delimiter the next thing in the input ?
		if self.endRE.match (text, pos):
		    self.lineno = self.lineno + self.calcNewLines (text, 0, pos)
		    self.error ('Missing value for argument %s in line %d\n', (argname, self.lineno))

		# Is there a string delimiter ?
		match = self.strDelimPattern.match (text, pos)
		if not match:
		    # If not, then the value of the argument is 
		    # terminated by whitespace or the end delimiter.
		    m2 = self.endRE.search (text, pos)
		    match = self.nonSpacePattern.match (text, pos)
		    if m2 and m2.start (0) < match.end (0):
			end = m2.start (0)
		    else:
			end = match.end (0)
		    value = text[pos:end]
		    pos = end
		else:
		    # It's a normal string. Check which delimiter is used
		    # and then search for it.
		    delim = match.group (0)
		    pos = strstart = match.end (0)
		    epat = re.compile (re.escape (delim))
		    match = epat.search (text, pos)
		    if not match:
			self.lineno = self.lineno + self.calcNewLines (text, 0, pos)
			self.error ('Missing end for string starting in line %d\n', self.lineno)
		    value = text[pos:match.start (0)]
		    pos = match.end (0)
	    else:
		# No value
		value = None

	    #print 'found %s="%s"'%(argname,value)
	    args[argname] = value

	# self.yytext contains the complete command *unprocessed*.
	self.yytext = text[0:pos]
	self.rest = text[pos:]
	#print 'yytext=',self.yytext

	# Update the line number
	self.lineno = self.lineno + self.calcNewLines (text, 0, pos)
	return cmd, args

    def skipWhiteSpace (self):
	"""This method can be called from the handlers to skip
	obsolete white space"""
	text = self.read ()
	pos = self.optSpacePattern.match (text).end (0)
	self.rest = text[pos:]

    def readBody (self, tag):
	"""Read the body of an environment. Notice that an environment
	must contain valid text (it must conform to the normal XIL
	syntax)."""
	text = cStringIO.StringIO ()
	startline = self.lineno
	level = 0
	while 1:
	    # Read the plain text
	    part, eof = self.readText ()
	    if eof:
		self.error ('Missing end of env %s in line %d\n' % (tag.begin, startline))
	    text.write (part)

	    # Read the command.
	    cmd, args = self.readCmd ()
	    #print cmd

	    # Check if the env nests. If yes, update a counter. The
	    # code exist when the end of an end is read and the counter
	    # is 0.
	    if not cmd:
		self.error ('Missing end of env %s in line %d\n' % (tag.begin, startline))
	    if cmd == tag.end:
		if not level:
		    break
		else:
		    level = level - 1
	    elif cmd == tag.begin:
		level = level + 1
	    #print 'yytext=',self.yytext ()

	    # yytext contains the unprocessed tag which was read by
	    # readCmd()
	    text.write (self.yytext)

	return text.getvalue ()
	    
    def read (self):
	"""This is called if more input is needed. In this implementation,
	all available data is read at once to speed up the parsing.
	When some other method needs data, it calls this and puts all
	data it didn't process into self.rest."""
	if self.rest:
	    line = self.rest
	    self.rest = ''
	else:
	    line = self.rf.read ()
	    
	#print 'read=',line
	return line

    def error (self, msg):
	"""Call this in case of an error"""
	raise ParseError, msg

    def write (self, str):
	"Write something to the output stream"""
	self.wf.write (str)

    def writeline (self, str):
	"Write a line to the output stream. This also writes an EOL to the
	stream."""
	self.wf.write (str)
	self.wf.write ('\n')

class CmdHandler:
    """This is a handler for commands. Commands only take arguments, no
    text."""
    def __init__ (self, name='dummy'):
	"""Initialize the command. This must set self.name to some 
	reasonable value."""
	self.name = string.upper (name)

    def handle (self, parser, args):
	"""Handle the command. parser is the parser instance which
	has read the command. The command is invoked immediately
	and the input stream points to the first character after the
	command. You may read from the stream and you can invoke
	methods of the parser. args contains the arguments which were
	found in the tag. The type of this depends on the parser which
	is used."""
	print self.name,`args`,

class EnvHandler:
    def __init__ (self, begin='env', end='/env'):
	"""Initialize the environment. This must set self.begin and self.end
	to some reasonable value. self.end is used in the parser to
	find the end of the environment."""
	self.begin, self.end = string.upper (begin), string.upper (end)

    def handle (self, parser, args, text):
	"""This is called when the environment has been found and the
	text in it has been read. The parser doesn't process the text
	in any way except that it searches for the end of the environment
	in it. Since environments can nest, it's no good idea to read from
	the input stream in this method. args contains the arguments
	which were found by the parser and text is the unprocessed text
	in the environment. If you want to process the text, you
	*must* create a new parser instance. Use the StringIO modules
	to convert the text into a stream."""
	print self.begin,`args`,'text=<<<%s>>>'%text

if __name__ == '__main__':
    input = """Das ist ein Test.
  <!--** 
    test arg1='hallo' arg2
    = '''hallo,
welt''' arg3
	=
	"test"
  **--!>'
<!--** test arg4="<!--**" arg5="**--!>" arg6 **--!>'
<!--**test arg4 arg5 arg6**--!>'
<!--** env arg7 arg8 arg9 **--!>
text im env
<!--** /env **--!>
<!--** env level0 **--!>
<!--** env level1 **--!>
text im env
<!--** /env **--!>
text im env
<!--** /env **--!>
"""
    rf = cStringIO.StringIO (input)
   
    parser = Parser (rf, sys.stdout, 
	{
	    'test': CmdHandler ('test'),
	    'env': EnvHandler ('env', '/env'),
	}
    )
    parser.run ()
