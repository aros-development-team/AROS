
import string, sys

class Token:
    def __init__ (self, text):
	self.text = text

    def dump (self, level):
	sys.stdout.write ('    '*level)
	print self.__class__.__name__

    def __repr__ (self):
	return self.__class__.__name__

class Entity (Token):
    def dump (self, level):
	sys.stdout.write ('    '*level)
	print '&%s;' % self.text

class Text (Token):
    pass

class Comment (Token):
    pass

class ProcInst (Token):
    pass

class Tag (Token):
    def __init__ (self, text):
	# Split tag into name and a dictionary of attribute=value.
	words = string.split (text, None, 1)
	self.name = words[0]
	self.attr = {}
	self.content = []

	if len (words) > 1:
	    rest = words[1]
	    while rest:
		attr, rest = string.split (rest, '=', 1)
		pos = string.find (rest, '"', 1)
		value = rest[1:pos]
		self.attr[attr] = value
		rest = string.lstrip (rest[pos+1:])

    def setContents (self, content):
	self.content = content
	#print self,'setContents',`self.content`

    def dump (self, level):
	sys.stdout.write ('    '*level)
	print '<%s' % self.name
	for key in self.attr.keys ():
	    sys.stdout.write ('    '*(level+1))
	    print '%s="%s"' % (key, self.attr[key])
	sys.stdout.write ('    '*level)
	print '>'
	#print `self.content`
	for item in self.content:
	    item.dump (level+1)
	sys.stdout.write ('    '*level)
	print '</%s>' % self.name
	
class Emptytag (Tag):
    def setContents (self, content):
	raise '%s: An empty tag cannot have contents' % self.name

    def dump (self, level):
	sys.stdout.write ('    '*level)
	print '<%s' % self.name
	for key in self.attr.keys ():
	    sys.stdout.write ('    '*(level+1))
	    print '%s="%s"' % (key, self.attr[key])
	sys.stdout.write ('    '*level)
	print '/>'

class Endtag (Token):
    def __init__ (self, text):
	self.name = text

    def dump (self, level):
	sys.stdout.write ('    '*level)
	print '</%s>' % self.name

class EOF: pass

class Reader:
    def __init__ (self, string):
	self.string, self.pos = string, 0
	self.lineno = 1
	self.tokenContext = ''
	self.filename = ''

    def read (self):
	if self.pos >= len (self.string):
	    return EOF

	self.updateContext ()

	if self.string[self.pos] == '<':
	    if self.string[self.pos+1] == '?':
		tagEnd = string.find (self.string, '?>', self.pos)
		assert tagEnd != -1
		tagEnd = tagEnd + 2
		text = self.string[self.pos:tagEnd]
		self.adjustLineCount (text)
		self.pos = tagEnd
		return ProcInst (text)
	    elif self.string[self.pos:self.pos+4] == '<!--':
		tagEnd = string.find (self.string, '-->', self.pos)
		assert tagEnd != -1
		tagEnd = tagEnd + 2
		text = self.string[self.pos:tagEnd]
		self.adjustLineCount (text)
		self.pos = tagEnd
		return Comment (text)
	    else:
		tagEnd = string.find (self.string, '>', self.pos)
		assert tagEnd != -1
		text = self.string[self.pos+1:tagEnd]
		empty = 0
		if text[-1] == '/':
		    #print text
		    empty = 1
		    text = text[:-1]
		self.adjustLineCount (text)
		self.pos = tagEnd + 1
		text = string.strip (text)
		if text[0] == '/':
		    text = string.strip (text[1:])
		    return Endtag (text)
		elif empty:
		    #print text
		    return Emptytag (text)
		else:
		    return Tag (text)
	elif self.string[self.pos] == '&':
	    pos = string.find (self.string, ';', self.pos)
	    assert pos != -1
	    text = self.string[self.pos+1:pos]
	    self.pos = pos+1
	    return Entity (text)
	else:
	    nextTagStart = string.find (self.string, '<', self.pos)
	    if nextTagStart == -1:
		nextTagStart = len (self.string)

	    nextEntityStart = string.find (self.string, '&', self.pos)
	    if nextEntityStart == -1:
		nextEntityStart = len (self.string)

	    maxLen = nextTagStart
	    if nextEntityStart < maxLen:
		maxLen = nextEntityStart

	    text = self.string[self.pos:maxLen]
	    self.adjustLineCount (text)
	    self.pos = maxLen

	    return Text (text)

    def adjustLineCount (self, text):
	self.lineno = self.lineno + string.count (text, '\n')

    def updateContext (self):
	pos = string.find (self.string, '\n', self.pos)
	if pos > 79:
	    pos = 79
	self.tokenContext = '%s:%d: %s' % (self.filename, self.lineno, self.string[self.pos:pos])

class XmlFile:
    def __init__ (self, filename):
	self.filename = filename
	fh = open (self.filename)
	data = fh.read ()
	fh.close ()

	self.reader = Reader (data)
	self.reader.filename = self.filename
	self.entities = {
	    'lt': '&lt;',
	    'gt': '&gt;',
	    'quot': '&quot;',
	    'amp': '&amp;',
	}

    def addEntity (self, name, replacement):
	self.entities[name] = replacement

    def parseEntity (self, token, tree, level):
	str = '&%s;' % token.text
	# Disable recursive invokations. This is a crude workaround for the
	# fact that we don't have encodings.
	replacement = self.entities.get (token.text, None)
	if not replacement:
	    print 'Warning: &%s; not defined' % token.text
	    replacement = '&%s;' % token.text
	if str == replacement:
	    tree.append (Text (str))
	    return
	
	reader = self.reader
	self.reader = Reader (replacement)
	entityTree = self.parse (level+1)
	tree = tree + entityTree
	#print entityTree
	self.reader = reader
	return
	
    def parse (self, level=0):
	tree = []
	
	while 1:
	    token = self.reader.read ()
	    if token == EOF:
		break

	    #print token
	    #token.dump (level)

	    if isinstance (token, Entity):
		self.parseEntity (token, tree, level)
		continue

	    tree.append (token)
	    if isinstance (token, Emptytag):
		continue
	    elif isinstance (token, Tag):
		#print 'Reading contents for',token.name
		context = self.reader.tokenContext
		subtree = self.parse (level+1)
		if not isinstance (subtree[-1], Endtag):
		    print context
		    if isinstance (subtree[-1], Text):
			print subtree[-1].text
		    
		    raise 'Expected </%s>, got %s' % (token.name, subtree[-1])
		if subtree[-1].name != token.name:
		    print context
		    raise 'Expected </%s> and found </%s>' % (
			token.name, subtree[-1].name
		    )
		#print 'Contents for',token.name,'=',subtree[:-1]
		token.setContents (subtree[:-1])
	    elif isinstance (token, Endtag):
		break

	#print '    '*level,tree
	if level == 0:
	    self.tree = tree
	return tree

    def dump (self):
	for item in self.tree:
	    item.dump (0)

    def process (self, p):
	self.processRecursive (p, self.tree)

    def processRecursive (self, p, list):
	for item in list:
	    if isinstance (item, Tag):
		p.call (item.name, self, item)
	    else:
		try:
		    p.call (item.__class__.__name__, self, item)
		except:
		    print `item`
		    raise

class Processor:
    def __init__ (self):
	self.processors = {}
    
    def default (self, *args):
	pass

    def setDefault (self, func):
	self.default = func

    def add (self, name, func):
	self.processors[name] = func

    def call (self, name, *args):
	func = self.processors.get (name, self.default)
	apply (func, (self,)+args)

if __name__ == '__main__':
    xmlfile = XmlFile (sys.argv[1])
    xmlfile.parse ()
    xmlfile.dump ()

