#!/usr/bin/env python

import xil
import re, string, cStringIO

class VarDB:
    def __init__ (self):
	self.locals = {}
	self.varstack = [self.locals]
	self.pool = {}

    def push (self, locals={}):
	self.locals = locals
	self.pool.update (locals)
	self.varstack.append (self.locals)

    def pop (self):
	locals = self.locals
	del self.varstack[-1]
	self.locals = self.varstack[-1]
	self.pool.clear ()
	for dict in self.varstack:
	    self.pool.update (dict)
	return locals

    def __setitem__ (self, key, item):
	key = string.upper (key)
	self.pool[key] = item

	if type (item) != type (''):
	    item = `item`

	i = len (self.varstack) - 1
	while i >= 0:
	    if self.varstack[i].has_key (key):
		self.varstack[i][key] = item
		return
	    i = i - 1

	self.locals[key] = item

    def __getitem__ (self, key):
	return self.pool[string.upper (key)]

    def has_key (self, key):
	return self.pool.has_key (string.upper (key))

    def len (self):
	return len (self.pool)

    def clear (self):
	self.locals.clear ()
	self.varstack = []
	self.pool.clear ()

    def update (self, dict):
	self.locals.update (dict)
	self.pool.update (dict)

    def keys (self):
	return self.pool.keys ()

    def setglobal (self, key, item):
	key = string.upper (key)

	self.pool.clear ()
	for dict in self.varstack:
	    self.pool.update (dict)

class HPPParser (xil.Parser):
    begin = '<'
    end = '>'

vardb = VarDB ()

class RemHandler (xil.EnvHandler):
    def __init__ (self):
	xil.EnvHandler.__init__ (self, 'rem', '/rem')

    def handle (self, parser, args, text):
	if parser.rest == '\n':
	    parser.rest = ''

token = re.compile ('\$')
escape = re.compile ('\%')
arg = re.compile ('[A-Z0-9_-]+', re.IGNORECASE)

def prepareBody (body):
    newBody = ''
    while body:
	match = token.search (body)
	if not match:
	    newBody = newBody + escape.sub ('%%', body)
	    break
	else:
	    match2 = arg.match (body, match.end (0))
	    name = string.upper (match2.group (0))
	    newBody = newBody \
		+ escape.sub ('%%', body[:match.start (0)]) \
		+ '%(' + name + ')s'
	    body = body[match2.end ():]

    return newBody

class MacroHandler (xil.CmdHandler):
    def __init__ (self, name, defaults, body):
	xil.CmdHandler.__init__ (self, name)

	self.defaults = defaults
	self.body = body

    def handle (self, parser, args):
	dict = {}
	dict.update (self.defaults)
	dict.update (args)
	vardb.push (dict)
	HPPParser (cStringIO.StringIO (self.body), parser.wf,
	    parser.handlers, startLine = parser.lineno).run ()
	vardb.pop ()

class DefHandler (xil.EnvHandler):
    def __init__ (self):
	xil.EnvHandler.__init__ (self, 'def', '/def')

    def handle (self, parser, args, text):
	"""Usage:

	    <DEF NAME=name optname[=default]...>
	    ...some text with $optname...
	    </DEF>

	    <NAME [optname=value]...>

	Inserts the text into the stream with all occurrencies of $(OPTNAME)s
	replaced with the values which were specified when the macro was
	called.

	Example:
	    <DEF NAME=test ARG1=hello ARG2="hello world">
	    arg1=$arg1 arg2=$arg2
	    </DEF>

	    <test>
	    <test arg1='test'>
	    <test arg2='test'>

	prints:

	    arg1=hello arg2=hello world
	    arg1=test arg2=hello world
	    arg1=hello arg2=test

	"""
	name = args['NAME']
	del args['NAME']

	macro = MacroHandler (name, args, text)
	parser.handlers[macro.name] = macro
	if parser.rest == '\n':
	    parser.rest = ''

class BlockHandler (xil.EnvHandler):
    def __init__ (self, name, defaults, body):
	xil.EnvHandler.__init__ (self, name, '/'+name)

	self.defaults = defaults
	self.body = body

    def handle (self, parser, args, body):
	dict = {}
	dict.update (self.defaults)
	dict.update (args)
	dict['BODY'] = body
	vardb.push (dict)
	HPPParser (cStringIO.StringIO (self.body), parser.wf,
	    parser.handlers, startLine = parser.lineno).run ()
	vardb.pop ()

class BDefHandler (xil.EnvHandler):
    def __init__ (self):
	xil.EnvHandler.__init__ (self, 'bdef', '/bdef')

    def handle (self, parser, args, text):
	"""Usage:

	    <BDEF NAME=name optname[=default]...>
	    ...some text with $optname...
	    </BDEF>

	    <NAME [optname=value]...>
	    body text
	    </NAME>

	Inserts the text into the stream with all occurrencies of $(OPTNAME)s
	replaced with the values which were specified when the macro was
	called.

	Example:
	    <DEF NAME=test ARG1=hello ARG2="hello world">
	    arg1=$arg1 arg2=$arg2 body=$body
	    </DEF>

	    <test>
	    body1
	    </test>
	    <test arg1='test'>
	    body2
	    </test>
	    <test arg2='test'>body3</test>

	prints:

	    arg1=hello arg2=hello world
	    arg1=test arg2=hello world
	    arg1=hello arg2=test

	"""
	name = args['NAME']
	del args['NAME']

	block = BlockHandler (name, args, text)
	parser.handlers[block.begin] = block
	if parser.rest == '\n':
	    parser.rest = ''

class ExpandHandler (xil.EnvHandler):
    def __init__ (self):
	xil.EnvHandler.__init__ (self, 'expand', '/expand')

    def handle (self, parser, args, body):
	dict = {}
	dict.update (args)
	vardb.push (dict)
	HPPParser (cStringIO.StringIO (prepareBody (body) % vardb), parser.wf,
	    parser.handlers, startLine = parser.lineno).run ()
	vardb.pop ()

class EnvHandler (xil.EnvHandler):
    def __init__ (self, name, args):
	xil.EnvHandler.__init__ (self, name, '/'+name)

	self.prefix = args['BEGIN']
	self.postfix = args['END']

    def handle (self, parser, args, body):
	HPPParser (cStringIO.StringIO (self.prefix + body + self.postfix),
	    parser.wf,
	    parser.handlers, startLine = parser.lineno).run ()

class EDefHandler (xil.CmdHandler):
    def __init__ (self):
	xil.CmdHandler.__init__ (self, 'edef')

    def handle (self, parser, args):
	"""Usage:

	    <EDEF NAME=name BEGIN=... END=...>

	    <NAME>text</NAME>

	Inserts the text into the stream with all occurrencies of $(OPTNAME)s
	replaced with the values which were specified when the macro was
	called.

	Example:
	    <EDEF NAME=btt BEGIN="<B><TT>" END="</B></TT>">

	    <BTT>test</BTT>

	prints:

	    <B><TT>test</B></TT>

	"""
	name = args['NAME']
	del args['NAME']

	env = EnvHandler (name, args)
	parser.handlers[env.begin] = env
	if parser.rest == '\n':
	    parser.rest = ''

class HTMLEnvHandler (xil.EnvHandler):
    def __init__ (self, tag):
	xil.EnvHandler.__init__ (self, tag, '/'+tag)

    def handle (self, parser, args, body):
	parser.write ('<%s' % self.begin)
	for argname, value in args.items ():
	    if value:
		parser.write (' %s' % argname)
	    else:
		parser.write (' %s="%s"' % (argname, value))
	parser.write ('>')
	HPPParser (cStringIO.StringIO (body),
	    parser.wf,
	    parser.handlers, startLine = parser.lineno).run ()
	parser.write ('<%s>' % self.end)

class SetHandler (xil.CmdHandler):
    def __init__ (self):
	xil.CmdHandler.__init__ (self, 'set')

    def handle (self, parser, args):
	vardb.update (args)
	if parser.rest == '\n':
	    parser.rest = ''

class VerbHandler (xil.CmdHandler):
    def __init__ (self):
	xil.CmdHandler.__init__ (self, 'verb')

    def handle (self, parser, args):
	parser.write (args['TEXT'])

class VerbatimHandler (xil.EnvHandler):
    def __init__ (self):
	xil.EnvHandler.__init__ (self, 'verbatim', '/verbatim')

    def handle (self, parser, args, body):
	parser.write (body)

class PythonHandler (xil.CmdHandler):
    def __init__ (self):
	xil.CmdHandler.__init__ (self, 'python')

    def handle (self, parser, args):
	localvars = {'parser': parser, 'args': args}
	exec args['CODE'] in globals(), localvars

class WriteHandler (xil.CmdHandler):
    def __init__ (self):
	xil.CmdHandler.__init__ (self, 'python')

    def handle (self, parser, args):
	filename = args['FILENAME']
	text = args['TEXT']

	fh = open (filename, 'a')
	fh.write (prepareBody (text) % vardb)
	if not args.has_key ('NOLINE'):
	    fh.write ('\n')
	fh.close ()

	if parser.rest == '\n':
	    parser.rest = ''

class SkipHandler (xil.CmdHandler):
    def __init__ (self):
	xil.CmdHandler.__init__ (self, 'skip')

    def handle (self, parser, args):
	parser.skipWhiteSpace ()

if __name__ == '__main__':
    import sys

    rf = open (sys.argv[1])

    HPPParser (rf, sys.stdout,
	{
	    'def': DefHandler (),
	    'bdef': BDefHandler (),
	    'edef': EDefHandler (),
	    'rem': RemHandler (),
	    'expand': ExpandHandler (),
	    'set': SetHandler (),
	    'verb': VerbHandler (),
	    'verbatim': VerbatimHandler (),
	    'python': PythonHandler (),
	    'b': HTMLEnvHandler ('b'),
	    'tt': HTMLEnvHandler ('tt'),
	    'sws': SkipHandler (),
	}
    ).run ()
