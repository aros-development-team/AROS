#! @PYTHON@
# -*- coding: iso-8859-1 -*-
# Copyright © 2003-2004, The AROS Development Team. All rights reserved.

import sys, re


if not len(sys.argv) in [2, 4] :
    print "Usage:",sys.argv[0],"tmplfile [inputfile outputfile]"
    print "Usage:",sys.argv[0],"tmplfile --listfile filename"

# A regular expression for the start of a template instantiation (ex. %build_module)
re_tmplinst = re.compile('%([a-zA-Z0-9][a-zA-Z0-9_]*)(?=(?:\s|$))')
# A regular expression for the argument specification during template instantiation
# (ex. cflags=$(CFLAGS) or uselibs="amiga arosc")
re_arg = re.compile('([a-zA-Z0-9][a-zA-Z0-9_]*)=([^\s"]+|".*?")?')

##################################
# Class and function definitions #
##################################

# Exception used throughout this program
class GenmfException:
    def __init__(self, s):
        self.s = s
    def __str__(self):
        return self.s


# Scan the given lines for template instantiations
# Input params:
# - lines: an array of strings
# - templates: an assosiative array of objects of class template
#
# Returns:
# - templrefs: an array of tuples with two elems. First element is the lineno
#   in the lines array, second is the results of the search for that line with
#   re_tmplinst. Only templates which name are present in the templates argument
#   will be added to this array.
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


# Write out the lines to a file with instantiating the templates present in the file
# Input params:
# - lines: The lines to write out
# - templrefs: the instantiated templates present in these lines
# - templates: the template definitions
# - outfile: the file to write to
#
# This function does not return anything but raises a GenmfException when there are
# problems
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
    

# arg is q class that stores the specification of an argument in the template header
# The of the arg is not stored in this class but this class is supposed to be
# stored in an assosiative array with the names of the arg as indices. E.g
# arg['cflags'] gives back an object of this class.
# It has the following members:
# - ismain: boolean to indicate if it has the /M definition
# - isneeded: boolean to indicate if it has the /A definition
# - used: boolean to indicate if a value is used in the body of a template
# - default: default value when the argument is not given during a template
#   instantiation
# - value: The value this argument has during a template instantiation is stored
#   here
class arg:
    # Specification can end with /A or /M
    re_mode = re.compile('/(A|M)')

    # You create this object with giving it the default value 
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


# template is a class to store the whole definition of a genmf template
# Members:
# - name: name of the template
# - args: an assosiative of the arguments of this template
# - body: an array of strings with the body of the template
# - mainarg: contains the arg with /M if it is present
# - used: Is this template already used; used to check for recursive calling
#   of a template
# - linerefs: an array to indicate the genmf variables used in the body of the
#   template. This is generated with the generate_linerefs method of this class
# - templrefs: an array to indicate the templates used in the body of the template.
#   This is generated with the generate_templrefs function of this class.
class template:
    re_arginst = re.compile('%\(([a-zA-Z0-9][a-zA-Z0-9_]*)\)')
    hascommon = 0

    # Generate a template
    # Input params:
    # - name: name of the template
    # - args: an assosiative array of the arguments defined for this template
    # - body: an array of the template with the bodylines of the template
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

    # Generate the references for the genmf variable used in this template
    # This function will return an assositive array of tuples with linerefs[lineno]
    # an array of tuples named argrefs with the tuple of the form (argbody, start, stop)
    # with argbody an object of class arg, start and stop the start and end of this variable
    # in the string of this line.
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


    # Write out the body of the template
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



# Read in the definition of the genmf templates from the given filename
# Return an assosiative array of the templates present in this file.
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


################
# Main program #
################

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
    # Read on input file and write out one outputfile
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

    # If %common was not present in the file write it out at the end of the file
    if not template.hascommon:
        outfile.write("\n")
        if templates.has_key("common"):
            templates["common"].write(outfile, "common", "", templates)
    
    if closeout:
        outfile.close()
else:
    # When a listfile is specified each line in this listfile is of the form
    # inputfile outputfile
    # and apply the instantiation of the templates to all these files listed there
    infile = open(listfile, "r")
    filelist = infile.readlines()
    infile.close()
    
    for fileno in range(len(filelist)):
        files = filelist[fileno].split()
        if len(files) <> 2:
            sys.exit('%s:%d: Syntax error: %s' % (listfile, fileno+1, filelist[fileno]))

        sys.stderr.write('Regenerating file %4d of %4d\r' % (fileno+1, len(filelist)))
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
