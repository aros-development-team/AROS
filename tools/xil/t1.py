
import re, sys, string

class XILParser:
    begin = '<!--**'
    end = '**--!>'

    EOF = 'EOF'
    BEGIN = 'BEGIN'
    END = 'END'
    TEXT = 'TEXT'

    def __init__ (self, rf, wf, handlers):
	self.beginRE = re.compile ('.*(' + re.escape (self.begin) + ')', re.IGNORECASE)
	self.endRE = re.compile ('.*(' + re.escape (self.end) + ')', re.IGNORECASE)
	self.rf, self.wf = rf, wf
	self.rest = ''
	self.handlers = {}
	self.lineno = 0
	self.handlerdict = {}

	for key, item in handlers.items ():
	    self.handlers[string.lower (key)] = item

    def run (self):
	while 1:
	    text = self.read ()
	    if not text: break
	    match = self.beginRE.match (text)
	    if not match:
		self.wf.write (text)
		continue

	    prefix = text[:match.start (1)]
	    text = text[match.end (1):]
	    if prefix:
		self.wf.write (prefix)

	    #print text
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
	    if not self.handlers.has_key (cmd):
		self.error ('Unknown command %s\n' % cmd)
	    else:
		self.handlers[cmd].handle (self, string.strip (args))
		
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

class Handler:
    def handle (self, parser, argstr):
	pass

if __name__ == '__main__':
    input = """Das ist ein Test.
  <!--** 
    test arg1 arg2 arg3 
  **--!>'
<!--** test arg4 arg5 arg6 **--!>'
"""
    import cStringIO
    rf = cStringIO.StringIO (input)
   
    class Test (Handler):
        def handle (self, parser, argstr):
	    print 'test',argstr,
    
    parser = XILParser (rf, sys.stdout, {'test': Test()})
    parser.run ()
