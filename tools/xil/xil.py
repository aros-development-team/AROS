
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

    EOF = 'EOF'
    BEGIN = 'BEGIN'
    END = 'END'
    TEXT = 'TEXT'

    def __init__ (self, rf, wf, handlers, startLine=1):
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
	while 1:
	    text, eof = self.readText ()
	    if text:
		#print '<<<TEXT<<<',
		self.write (text)
		#print '>>>TEXT>>>',
	    if eof: break

	    cmd, args = self.readCmd ()
	    #print 'cmd=',cmd,args
	    if not cmd: break

	    o = self.handlers.get (cmd, None)
	    if not o:
		self.error ('Unknown command %s in line %d\n' % (cmd, self.lineno))
	    else:
		if isinstance (o, CmdHandler):
		    o.handle (self, args)
		else:
		    body = self.readBody (o)
		    o.handle (self, args, body)

    def readText (self):
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
	pos = start
	count = 0
	while 1:
	    match = self.newLinePattern (text, pos, end)
	    if not match: break
	    self.lineno = self.lineno + 1
	    pos = match.end (0)

	return count

    def readCmd (self):
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

	if match.group (0) == self.end:
	    self.yytext = text[0:pos]
	    self.rest = text[pos:]
	    return cmd, {}

	args = {}
	while 1:
	    pos = self.optSpacePattern.match (text, pos).end (0)

	    #print '1',`string.strip (text[pos:pos+10])`
	    match = self.argNameEndPattern.search (text, pos)
	    if not match:
		self.calcNewLines (text, 0, pos)
		error ('Unterminated tag in line %d\n' % self.lineno)

	    argname = string.upper (text[pos:match.start (0)])
	    pos = match.end (0)
	    #print 'argname=',argname
	    if not argname:
		if match.group (0) == '=':
		    self.calcNewLines (text, 0, pos)
		    error ('Missing argument name in line %d\n' % self.lineno)
		break
	    
	    if match.group (0) == '=':
		pos = self.optSpacePattern.match (text, pos).end (0)

		if self.endRE.match (text, pos):
		    self.calcNewLines (text, 0, pos)
		    self.error ('Missing value for argument %s in line %d\n', (argname, self.lineno))

		match = self.strDelimPattern.match (text, pos)
		if not match:
		    m2 = self.endRE.search (text, pos)
		    match = self.nonSpacePattern.match (text, pos)
		    if m2 and m2.start (0) < match.end (0):
			end = m2.start (0)
		    else:
			end = match.end (0)
		    value = text[pos:end]
		    pos = end
		else:
		    delim = match.group (0)
		    pos = strstart = match.end (0)
		    #print 'Found string delim=',delim,'value=',value
		    epat = re.compile (re.escape (delim))
		    match = epat.search (text, pos)
		    if not match:
			self.calcNewLines (text, 0, pos)
			self.error ('Missing end for string starting in line %d\n', self.lineno)
		    value = text[pos:match.start (0)]
		    pos = match.end (0)
	    else:
		value = None

	    #print 'found %s="%s"'%(argname,value)
	    args[argname] = value

	self.yytext = text[0:pos]
	self.rest = text[pos:]
	#print 'yytext=',self.yytext
	return cmd, args

    def skipWhiteSpace (self):
	text = self.read ()
	pos = self.optSpacePattern.match (text).end (0)
	self.rest = text[pos:]

    def readBody (self, tag):
	text = cStringIO.StringIO ()
	startline = self.lineno
	level = 0
	while 1:
	    part, eof = self.readText ()
	    if eof:
		self.error ('Missing end of env %s in line %d\n' % (tag.begin, startline))
	    text.write (part)
	    cmd, args = self.readCmd ()
	    #print cmd
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
	    text.write (self.yytext)

	return text.getvalue ()
	    
    def read (self):
	if self.rest:
	    line = self.rest
	    self.rest = ''
	else:
	    line = self.rf.read ()
	    
	#print 'read=',line
	return line

    def error (self, msg):
	raise ParseError, msg

    def write (self, str):
	self.wf.write (str)

    def writeline (self, str):
	self.wf.write (str)
	self.wf.write ('\n')

class CmdHandler:
    def __init__ (self, name='dummy'):
	self.name = string.upper (name)

    def handle (self, parser, args):
	print self.name,`args`,

class EnvHandler:
    def __init__ (self, begin='env', end='/env'):
	self.begin, self.end = string.upper (begin), string.upper (end)

    def handle (self, parser, args, text):
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
