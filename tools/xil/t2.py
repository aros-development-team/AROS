
import re, sys, string
import cStringIO

class XILParser:
    begin = '<!--**'
    end = '**--!>'

    EOF = 'EOF'
    BEGIN = 'BEGIN'
    END = 'END'
    TEXT = 'TEXT'

    def __init__ (self, rf, wf, handlers, startLine=1):
	self.beginRE = re.compile ('.*(' + re.escape (self.begin) + ')', re.IGNORECASE)
	self.endRE = re.compile ('.*(' + re.escape (self.end) + ')', re.IGNORECASE)
	self.rf, self.wf = rf, wf
	self.rest = ''
	self.handlers = {}
	self.lineno = startLine-1

	for key, item in handlers.items ():
	    self.handlers[string.lower (key)] = item

    def run (self):
	while 1:
	    text = self.readText ()
	    if text == self.EOF: break
	    if text:
		self.write (text)

	    cmd, args = self.readCmd ()
	    if not cmd: break

	    o = self.handlers.get (cmd, None)
	    if not o:
		self.error ('Unknown command %s in line %d' % (cmd, self.lineno))
	    else:
		if isinstance (o, CmdHandler):
		    o.handle (self, args)
		else:
		    body = self.readBody (o)
		    o.handle (self, args, body)

    def readText (self):
	text = cStringIO.StringIO ()
	while 1:
	    line = self.read ()
	    if not line: return self.EOF
	    match = self.beginRE.match (line)
	    if match:
		self.rest = line[match.end (1):]
		text.write (line[:match.start (1)])
		return text.getvalue ()
	    else:
		text.write (line)

    def readCmd (self):
	#print text
	text = self.read ()
	if not text: return None, None
	match = self.endRE.match (text)
	if match:
	    cmd = text[:match.start (1)]
	    self.rest = text[match.end (1):]
	else:
	    cmd = string.strip (text)
	    startline = self.lineno
	    while 1:
		text = self.read ()
		if not text:
		    self.error ('Missing end for command in line %d\n' % startline)
		    sys.exit (1)
		match = self.endRE.match (text)
		if match: break
		cmd = cmd + ' ' + string.strip (text)
	    cmd = cmd + ' ' + string.strip (text[:match.start (1)])
	    self.rest = text[match.end (1):]

	#print 'cmd=',cmd
	cmd, args = string.split (string.strip (cmd), None, 1)
	cmd = string.lower (cmd)

	return cmd, args

    def readBody (self, tag):
	text = cStringIO.StringIO ()
	startline = self.lineno
	level = 0
	while 1:
	    part = self.readText ()
	    if part == self.EOF:
		self.error ('Missing end of env in line %d\n' % startline)
	    text.write (part)
	    cmd, args = self.readCmd ()
	    if not cmd:
		self.error ('Missing end of env in line %d\n' % startline)
	    if cmd == tag.end:
		if not level:
		    break
		else:
		    level = level - 1
	    elif cmd == tag.begin:
		level = level + 1
	    text.write (self.begin)
	    text.write (cmd)
	    text.write (' ')
	    text.write (args)
	    text.write (self.end)

	return text.getvalue ()
	    
    def read (self):
	if self.rest:
	    line = self.rest
	    self.rest = ''
	else:
	    self.lineno = self.lineno + 1
	    line = self.rf.readline ()
	    
	return line

    def error (self, msg):
	sys.stderr.write (msg)

    def write (self, str):
	self.wf.write (str)

    def writeline (self, str):
	self.wf.write (str)
	self.wf.write ('\n')

class CmdHandler:
    def __init__ (self, name=None):
	self.name = name

    def handle (self, parser, argstr):
	pass

class EnvHandler:
    def __init__ (self, begin='env', end='/env'):
	self.begin, self.end = begin, end

    def handle (self, parser, argstr, text):
	pass

if __name__ == '__main__':
    input = """Das ist ein Test.
  <!--** 
    test arg1 arg2 arg3 
  **--!>'
<!--** test arg4 arg5 arg6 **--!>'
<!--**test arg4 arg5 arg6**--!>'
<!--** env arg7 arg8 arg9 ***--!>
text im env
<!--** /env ***--!>
<!--** env level0 ***--!>
<!--** env level1 ***--!>
text im env
<!--** /env ***--!>
text im env
<!--** /env ***--!>
"""
    rf = cStringIO.StringIO (input)
   
    class Test (CmdHandler):
        def handle (self, parser, argstr):
	    print 'test',argstr,
    
    class EnvTest (EnvHandler):
	def __init__ (self, begin='env', end='/env'):
	    EnvHandler.__init__ (self, begin, end)

        def handle (self, parser, argstr, text):
	    print 'env',argstr,'text=<<<%s>>>'%text
    
    parser = XILParser (rf, sys.stdout, 
	{
	    'test': Test(),
	    'env': EnvTest ('env', '/env'),
	}
    )
    parser.run ()
