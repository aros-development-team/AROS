#! @PYTHON@

import sys, re


if not len(sys.argv) in [2, 4] :
    print "Usage:",sys.argv[0],"tmplfile [inputfile outputfile]"
    print "Usage:",sys.argv[0],"tmplfile --listfile filename"

re_tmplinst = re.compile('%([a-zA-Z0-9][a-zA-Z0-9_]*)(?=(?:\s|$))')
re_arg = re.compile('([a-zA-Z0-9][a-zA-Z0-9_]*)=([^\s"]+|".*?")?')

class GenmfException:
    def __init__(self, s):
	self.s = s
    def __str__(self):
	return self.s

def generate_templrefs(lines, templates):
    templrefs = []
    
    for lineno in range(len(lines)):
	line = lines[lineno]
	if len(line) == 0 or line[0] == "#":
	    continue
	
	m = re_tmplinst.search(line)
	if m and templates.has_key(m.group(1)) and not (m.start() > 0 and line[m.start()-1] == "#"):
	    templrefs.append((lineno, m))

    return templrefs

def writelines(lines, templrefs, templates, outfile):
    start = 0
    for lineno, m in templrefs:
	if start<lineno:
	    outfile.writelines(lines[start:lineno])

	start = lineno + 1
	line = lines[lineno]
	while line[len(line)-2] == "\\" and start < len(lines):
	    line = line[0:len(line)-2] + lines[start]
	    start = start + 1
	
	if m.group(1) == "common":
	    template.hascommon = 1

	try:
	    templates[m.group(1)].write(outfile, m.group(1), line[m.end():].lstrip(), templates)
	except GenmfException, ge:
	    raise GenmfException(("In instantiation of %s, line %d\n" % (m.group(1), lineno+1))+ge.s)
	
    if start < len(lines):
	outfile.writelines(lines[start:len(lines)])
    
class arg:
    re_mode = re.compile('/(A|M)')
    
    def __init__(self, default=None):
	self.ismain = 0
	self.isneeded = 0
	self.used = 0

	while default and len(default)>1:
	    m = arg.re_mode.match(default[len(default)-2:])
	    if not m:
		break
	    if m.group(1) == "M":
		self.ismain = 1
	    elif m.group(1) == "A":
		self.isneeded = 1
	    else:
		sys.exit('Internal error: Unknown match')

	    default = default[:len(default)-2]

	if default and default[0] == '"':
	    default = default[1:len(default)-1]
	
	self.default = default
	# The value field will get the value passed to the argument when the tmeplate is used
	self.value = None

class template:
    re_arginst = re.compile('%\(([a-zA-Z0-9][a-zA-Z0-9_]*)\)')
    hascommon = 0
    
    def __init__(self, name, args, body):
	self.name = name
	self.args = args
	self.body = body
	self.mainarg = None
	self.used = 0
	self.linerefs = None
	self.templrefs = None
	
	for argname, argbody in args.items():
	    if argbody.ismain:
		if self.mainarg:
		    sys.exit('A template can have only one main (/M) argument')
		self.mainarg = argbody

    def generate_linerefs(self):
	lineno = 0
	linerefs = {}
	while lineno < len(self.body):
	    argrefs = []
	    for m in template.re_arginst.finditer(self.body[lineno]):
		if self.args.has_key(m.group(1)):
		    argbody = self.args[m.group(1)]
		    argrefs.append((argbody, m.start(), m.end()))
		    argbody.used = 1

	    if len(argrefs) > 0:
		linerefs[lineno] = argrefs

	    lineno = lineno+1
	self.linerefs = linerefs
	
	for argname, argbody in self.args.items():
	    if not argbody.used:
		sys.stderr.write("Warning: template '%s': unused argument '%s'\n" % (self.name, argname))

    def write(self, outfile, name, line, templates):
	if self.used:
	    raise GenmfException("Template '%s' called recursively" % name)
	self.used = 1

	# Reading arguments of the template
	argno = 0
	while len(line) > 0:
	    m = re_arg.match(line)
	    if m and self.args.has_key(m.group(1)):
		value = m.group(2)
		if value == None:
		    #sys.stderr.write("Arg:"+m.group(1)+" Value: None Line:"+line+"\n")
		    self.args[m.group(1)].value = ''
		else:
		    #sys.stderr.write("Arg:"+m.group(1)+" Value:"+m.group(2)+" Line:"+line+"\n")
		    if len(value)>0 and value[0] == '"':
			value = value[1:len(value)-1]
		    self.args[m.group(1)].value = value
		line = line[m.end():].lstrip()
	    elif self.mainarg:
		self.mainarg.value = line[:len(line)-1]
		line = ''
	    else:
		raise GenmfException('Syntax error in arguments: '+line)

	if self.linerefs == None:
	    self.generate_linerefs()
	    self.templrefs = generate_templrefs(self.body, templates)

	for argname, argbody in self.args.items():
	    if argbody.isneeded and argbody.value == None:
		raise GenmfException('Arg "%s" not specified but should have been' % argname)
	
	text = self.body[:]

	for lineno, argrefs in self.linerefs.items():
	    line = text[lineno]

	    pos=0
	    lineout = ''
	    for argref in argrefs:
		if argref[1] > pos:
		    lineout = lineout + line[pos:argref[1]]
	    
		if not argref[0].value == None:
		    lineout = lineout + argref[0].value
		elif argref[0].default:
		    lineout = lineout + argref[0].default

		pos = argref[2]
		
	    if pos < len(line):
		lineout = lineout + line[pos:]
	    
	    text[lineno] = lineout

	writelines(text, self.templrefs, templates, outfile)
	#outfile.write('\n')
	
	for argname, argbody in self.args.items():
	    argbody.value = None
	self.used = 0

def read_templates(filename):
    try:
	infile = open(filename)
    except:
	print "Error reading template file: "+filename

    re_name = re.compile('[a-zA-Z0-9][a-zA-Z0-9_]*(?=(?:\s|$))')
    re_openstring = re.compile('[^\s"]*"[^"]*$')
    re_define = re.compile('%define(?=\s)')
    
    lines = infile.readlines()
    lineno = 0
    templates = {}
    while lineno < len(lines):
	line = lines[lineno]
	if re_define.match(line):
	    while line[len(line)-2] == "\\" and lineno < len(lines):
		lineno = lineno + 1
		line = line[0:len(line)-2] + lines[lineno]

	    line = line[7:].strip()
	    
	    m = re_name.match(line)
	    if not m:
		sys.exit("%s:%d:Error in syntax of template name" % (filename, lineno+1))
	    tmplname = m.group(0)
	    line = line[m.end():].lstrip()
	    
	    args = {}
	    while len(line) > 0:
		m = re_arg.match(line)
		if not m:
		    sys.exit("%s:%d:Error in syntax of argument %d Line: %s" % (filename, lineno+1, len(args)+1, line))
		args[m.group(1)] = arg(m.group(2))
		
		line = line[m.end():].lstrip()

	    #print "Line: %d Template: %s" % (lineno+1, tmplname)

	    lineno = lineno+1
	    line = lines[lineno]
	    bodystart = lineno
	    while lineno < len(lines) and line[0:4] <> "%end":
		lineno = lineno+1
		line = lines[lineno]

	    if lineno == len(lines):
		sys.exit('%s:End of file reached in a template definition' % filename)

	    templates[tmplname] = template(tmplname, args, lines[bodystart:lineno])
	    
	lineno = lineno+1

    return templates


argv = []
i = 0
listfile = None
while i < len(sys.argv):
    if sys.argv[i] == "--listfile":
	listfile = sys.argv[i+1]
	i = i + 2
    else:
	argv.append(sys.argv[i])
	i = i + 1
    
#sys.stderr.write("Reading templates\n")
templates = read_templates(argv[1])
#sys.stderr.write("Read %d templates\n" % len(templates))

if listfile == None:
    if len(sys.argv) == 2:
	lines = sys.stdin.readlines()
    else:
	infile = open(sys.argv[2], "r")
	lines = infile.readlines()
	infile.close()

    if len(sys.argv) == 2:
	outfile = sys.stdout
	closeout = 0
    else:
	outfile = open(sys.argv[3], "w")
	closeout = 1

    try:
	writelines(lines, generate_templrefs(lines, templates), templates, outfile)
    except GenmfException, ge:
	s = ge.s
	if len(sys.argv) == 4:
	    s = sys.argv[3]+":"+s
	    sys.exit(s+"\n")

    if not template.hascommon:
	outfile.write("\n")
	if templates.has_key("common"):
	    templates["common"].write(outfile, "common", "", templates)
    
    if closeout:
	outfile.close()
else:
    infile = open(listfile, "r")
    filelist = infile.readlines()
    infile.close()
    
    for fileno in range(len(filelist)):
	files = filelist[fileno].split()
	if len(files) <> 2:
	    sys.exit('%s:%d: Syntax error: %s' % (listfile, fileno+1, filelist[fileno]))

	sys.stderr.write('Regenerate file %4d of %4d\r' % (fileno+1, len(filelist)))
	sys.stderr.flush()
	
	infile = open(files[0], "r")
	lines = infile.readlines()
	infile.close()

	outfile = open(files[1], "w")
	template.hascommon = 0

	try:
	    writelines(lines, generate_templrefs(lines, templates), templates, outfile)
	except GenmfException, ge:
	    s = ge.s
	    if len(sys.argv) == 4:
		s = files[0]+":"+s
		sys.exit(s+"\n")

	if not template.hascommon:
	    outfile.write("\n")
	    if templates.has_key("common"):
		templates["common"].write(outfile, "common", "", templates)
    
	outfile.close()
	
    sys.stderr.write('\n')
