# File: jsdoc2xml.py
# TODO: [unknown] treat the constructor specially

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************


from __future__ import nested_scopes
import os
from Compiler import *
import xmldocutils
from xmldocutils import *

COMPILER_CONSTANTS = {'$debug': true,
                      '$krank': false,
                      '$profile': false,
                      '$swf5': false,
                      '$swf6': true,
                      '$swf7': false}

class DocumentationError(CompilerError):
    pass

# FIXME: [2003-05-08 ptw] Need a way to get the right number
SINCE = 2.2

# FIXME: [2003-05-08 ptw] Migrate these to the sources.
#  ['LzCloneDatapath.clonenumber',
#   'LzDatapath.text-nodes',
#   'LzDatapointer.text-nodes',
#   'LzDatapointer.getOtherNodeText',
#   'LzDataset.acceptencodings',
#   'LzDataset.sendheaders',
#   'LzDataset.cacheable',
#   'Debug.escapeText',
#   'LzBrowser.xmlEscape']

def carefulParseComment(str, validKeys):
    try:
        return parseComment(str, validKeys)
    except Exception, e:
        raise DocumentationError("malformed comment: %s" % e)

class CommentExtractor(Visitor):
    def __init__(self, handler):
        self.context = None #'.'
        self.handler = handler
        self.insideFunction = False
        self.errors = []

    def parse(self, str):
        p = Parser().parse(str)
        self.visit(p)

    def read(self, uname):
        import os
        fname = os.path.join(self.context or '.', uname)
        saved = self.context
        self.context = self.context or os.path.split(fname)[0]
        f = open(fname)
        try:
            try:
                self.parse(('#file "%s"\n#line 1\n' % uname) + f.read())
            finally:
                f.close()
        except xmldocutils.InputError, e:
            raise DocumentationError('%s: %s' % (fname, str(e)))
        except ParseException, e:
            raise CompilerError(("while in %s\n" % uname) + str(e))
        self.context = saved
    
    def visit(self, node):
        fn = self.getVisitor(node)
        children = node.children
        try:
            if fn:
                return fn(node, *children)
            # Sanity
            if node.comment and \
                      isDocComment(node.comment):
                if len(children) >= 1 and children[0].comment == node.comment:
                    # fall through to children loop
                    pass
                else:
                    raise DocumentationError("Unhandled doc comment: %s" % (node.comment), node)
        except DocumentationError, e:
            if not e.node:
                e.attachNode(node)
            self.errors.append(e)
        except CompilerError, e:
            if not e.node:
                e.attachNode(node)
            raise
        for c in children:
            try:
                self.visit(c)
            except DocumentationError, e:
                if not e.node:
                    e.attachNode(node)
                self.errors.append(e)
            except CompilerError, e:
                if not e.node:
                    e.attachNode(node)
                raise
                        
    def doClassConstructor(self, className, superclassName, constructor, comment):
        # permit 'param' because they will be consumed by
        # the addFunction of the constructor below
        k = self.handler.addClass(className, superclassName, comment, constructor and ['param'] or [])
        if constructor:
            # process constructor
            if 'private_constructor' in k.keywords:
                name = None
            else:
                name = className + '.constructor'
            self.doFunctionStatements(name, constructor, comment)

    def doFunctionStatements(self, name, function, comment):
        children = function.children
        if name:
            # In debug/profile mode, function expressions have names
            if len(children) == 2:
                args = [id.name for id in children[0].children]
            else:
                args = [id.name for id in children[1].children]
            self.handler.addFunction(name, args, comment)
        # process fields and events
        saved = self.insideFunction
        self.insideFunction = True
        self.visit(children[-1])
        self.insideFunction = saved

    def visitAssignmentExpression(self, node, *children):
        comment = node.comment
        comment = comment and extractComment(comment)
        name = ParseTreePrinter().visit(node[0]).replace('.prototype.', '.')
        value = node[2]
        if name.startswith('_root.'):
            name = name.split('.', 1)[1]
        path = name.split('.')
        lastname = path[-1]
        if value.class is FunctionExpression:
            if '.' not in name:
                if comment:
                    self.doClassConstructor(name, 'Object', value, comment)
                return
            elif not comment and not name.endswith('.dependencies') and \
                   lastname not in ('initialize', 'toString'):
                raise DocumentationError('%s: missing documentation' % name, node)
            if comment:
                self.doFunctionStatements(name, value, comment)
        elif '.' not in name:
            # name = new Object(...)
            # name = new LzTransformer(...)
            if value.class is NewExpression and \
                   value[0].class is CallExpression and \
                   value[0][0].class is Identifier and \
                   value[0][0].name in ['Object', 'LzTransformer']:
                self.handler.addClass(name, value[0][0].name, comment)
            # name = new Object;
            elif value.class is NewExpression and \
                     value[0].class is Identifier and \
                     value[0].name == 'Object':
                self.handler.addClass(name, 'Object', comment)
            # name = Object;
            elif value.class is Identifier and \
                     value.name == 'Object':
                self.handler.addClass(name, 'Object', comment)
            # name = Class("name", null, ...);
            # name = Class("name", Super, ...);            
            elif value.class is CallExpression and \
                 value[0].class is Identifier and \
                 value[0].name == 'Class' and \
                 value[1][0].value == name:
                if value[1][1].class is Identifier:
                    super = value[1][1].name
                else:
                    super = value[1][1]
                    assert super.class is Literal
                    assert not super.value
                    super = 'Object'
                if len(value[1]) > 2 and \
                   value[1][2].class is FunctionExpression:
                    constructor = value[1][2]
                else:
                    constructor = None
                self.doClassConstructor(name, super, constructor, comment)
            # name = LzTransformer();
            elif value.class is CallExpression and \
                 value[0].class is Identifier and \
                 value[0].name == 'LzTransformer':
                self.handler.addClass(name, 'Object', comment)
            #elif not self.insideFunction:
            #    print 'skipping', name
        elif len(path) > 2 and path[-2] == 'setters':
            c = carefulParseComment(comment, ['keywords', 'field', 'since', 'deprecated'])
            if 'private' in c.keywords:
                pass
            else:
                k = self.handler.internClass(path[0])
                # N.B.: fields will be handled in visitStatement
                since =  getattr(c, 'since', None)
                deprecated =  getattr(c, 'deprecated', None)
                if deprecated or (since and float(since) > SINCE):
                    pass
                else:
                    k.setters.append(lastname)
            if c.comment and isDocComment(node.comment):
                raise DocumentationError("Unhandled doc comment: %s" % (c.comment), node)

    def visitIfDirective(self, node, test, *branches):
        if test.class is Identifier and COMPILER_CONSTANTS.get(test.name) is not None:
            if COMPILER_CONSTANTS[test.name]:
                branches = branches[:1]
            else:
                branches = branches[1:]
        map(self.visit, branches)

    def visitStatement(self, node, *children):
        if node.comment:
            comment = node.comment.strip()
            if (comment.find('@field') != -1 \
                or comment.find('@event') != -1):
                lines = comment.split('\n')
                lines = [line[2:] for line in lines]
                if self.insideFunction:
                    tags = ['field', 'event', 'since', 'deprecated']
                else:
                    # Allow keywords param return if not in a function
                    tags = ['field', 'event', 'since', 'deprecated', 'keywords', 'param', 'return']
                c = carefulParseComment('\n'.join(lines), tags)
                if 'private' in c.keywords:
                    pass
                else:
                    since =  getattr(c, 'since', None)
                    deprecated =  getattr(c, 'deprecated', None)
                    if deprecated or (since and float(since) > SINCE):
                        def ign(name):
                            IGNORED.append(name)
                        map(ign, c.getFields('field'))
                        map(ign, c.getFields('event'))
                    else:
                        map(self.handler.addField, c.getFields('field'))
                        map(self.handler.addEvent, c.getFields('event'))
                    if self.insideFunction and \
                           len(children) == 1 and children[0].comment == node.comment:
                        # comment has been handled
                        return
            elif isDocComment(comment):
                if len(children) >= 1 and children[0].comment == node.comment:
                    # fall through to children loop
                    pass
                else:
                    raise DocumentationError("Unhandled doc comment: %s" % (node.comment), node)
        map(self.visit, children)

    def visitVariableStatement(self, node, decl):
        if len(decl.children) == 1:
            return
        if len(decl.children) != 2:
            return # TODO: [unknown]
        name, value = decl.children
        # var name = Class("name", null)
        # var name = Class("name", Super)
        if value.class is CallExpression and \
           value[0].class is Identifier and \
           value[0].name == 'Class' and \
           value[1][0].value == name.name:
            s = node.comment and extractComment(node.comment)
            if value[1][1].class is Identifier:
                super = value[1][1].name
            else:
                super = value[1][1]
                assert super.class is Literal
                assert not super.value
                super = 'Object'
            self.handler.addClass(name.name, super, s)
        # var name = new Object(...)
        if value.class is NewExpression and \
           value[0].class is CallExpression and \
           value[0][0].class is Identifier and \
           value[0][0].name == 'Object':
            s = node.comment and extractComment(node.comment)
            self.handler.addClass(name.name, 'Object', s)

    def visitPropertyIdentifierReference(self, node, *children):
        return self.visit(node[0])

    def visitIdentifier(self, node):
        return node.comment

    def visitFunctionDeclaration(self, node, *children):
        s = node.comment
        if s and s.find('DEFINE OBJECT') > 0:
            comment = extractComment(node.comment)
            self.doClassConstructor(node[0].name, 'Object', node, comment)

    def visitFunctionExpression(self, *children):
        pass

    def visitIncludeDirective(self, node, *children):
        self.read(node[0].value)

import re
docCommentPattern = re.compile(r'''.*?^\s*//(\s*@|---|===).*''', re.MULTILINE)

def isDocComment(comment):
    return re.match(docCommentPattern, comment)

# Extract the comment between //==='s (top level style) or all the
# comment following the first @ (inline style)
def extractComment(str):
    if isDocComment(str):
        lines = [s[2:] for s in str.split('\n') if s.startswith('//')]
        i = len(lines)-1
        while i >= 0 and lines[i][:3] not in ('---', '==='):
            i -= 1
        if i > 0:
            lines = lines[:i]
            i = len(lines)-1
            while i >= 0 and lines[i][:3] not in ('---', '==='):
                i -= 1
            if i >= 0:
                lines = lines[i+1:]
            if lines and lines[0].strip().startswith('DEFINE OBJECT'):
                lines = lines[1:]
        else:
            while lines and not lines[0].strip().startswith('@'):
                lines = lines[1:]
        return '\n'.join(lines)

def getKeywords(comment):
    for line in comment.split('\n'):
        line = line.strip()
        if line.startswith('@keywords'):
            return line.split()[1:]
    return []

class Handler:
    def __init__(self):
        self.classes = {}
        self.lastClass = None

    def addClass(self, name, super, comment, additionalKeywords=[]):
        if self.classes.has_key(name):
            raise DocumentationError('duplicate definition for class %s' % name)
        additionalKeywords = additionalKeywords + (['field', 'event', 'keywords', 'since', 'deprecated'])
        c = carefulParseComment(comment, additionalKeywords)
        deprecated =  getattr(c, 'deprecated', None)
        since = getattr(c, 'since', None)
        if deprecated or (since and float(since) > SINCE):
            IGNORED.append(name)
            return
        # TODO: [unknown] copy params to constructor
        k = Class(name=name, extends=super, desc=c.comment, keywords=c.keywords)
        self.lastClass = k
        self.classes[name] = k
        map(self.addField, c.getFields('field'))
        map(self.addEvent, c.getFields('event'))
        return k

    def internClass(self, name):
        if not self.classes.has_key(name):
            if name != 'Object':
                raise DocumentationError('undefined class %s' % name)
            self.addClass(name, 'LzNode', None)
        return self.classes[name]

    def addEvent(self, event):
        if not self.lastClass:
            raise DocumentationError('no class to add this event to')
        self.lastClass.events.append(event)

    def addField(self, field):
        if not self.lastClass:
            raise DocumentationError('no class to add this field to')
        self.lastClass.attributes.append(field)

    def addFunction(self, name, args, comment):
        c = carefulParseComment(comment, ['param', 'return', 'keywords', 'since', 'deprecated'])
        deprecated =  getattr(c, 'deprecated', None)
        since = getattr(c, 'since', None)
        if deprecated or (since and float(since) > SINCE):
            IGNORED.append(name)
            return
        if '.' not in name:
            return # TODO: [unknown] create global function
        parts = name.split('.')
        if len(parts) != 2:
            return
        kn, mn = name.split('.')
        params = [Param(name=pn) for pn in args]
        for k, v in c.params.items():
            candidates = [p for p in params if p.name == k]
            if len(candidates) != 1:
                raise DocumentationError('%s: comment for nonexistent parameter %r' % (name, k))
                continue
            candidates[0].desc = v
            candidates[0].type = c.paramTypes.get(k)
        method = Method(name=mn, parameters=params, desc=c.comment)
        if hasattr(c, 'return'):
            rd = getattr(c, 'return')
            if rd.startswith(':'): rd = rd[1:]
            type = None
            if ':' in rd:
                type, rd = rd.split(':', 1)
                type = type.strip()
                rd = rd.strip()
            setattr(method, 'return', Return(desc=rd, type=type))
        if not 'private' in c.keywords:
            klass = self.internClass(kn)
            if [m for m in klass.methods if m.name == mn]:
                raise DocumentationError('duplicate method definition: %s.%s' % (klass.name, mn))
            klass.methods.append(method)

class Class(Serializable):
    Name = 'api'
    AttrAttrs = ['name', 'extends']
    ContentAttrs = ['desc', 'attributes', 'fields', 'events', 'methods']

    def __init__(self, **initargs):
        Serializable.__init__(self, **initargs)
        self.methods = []
        self.attributes = []
        self.events = []
        self.setters = []

    def finalize(self):
        def hasSetter(attr):
            return attr.name in self.setters
        self.fields = [Field(name=a.name, type=a.type, desc=a.desc)
                       for a in self.attributes if not hasSetter(a)]
        self.attributes = [a for a in self.attributes if hasSetter(a)]
        for name in self.setters:
            if not [a for a in self.attributes if a.name == name]:
                self.attributes.append(Attribute(name=name))

class Method(Serializable):
    AttrAttrs = ['name', 'keywords']
    ContentAttrs = ['desc', 'parameters', 'return']

class Return(Serializable):
    AttrAttrs = ['type']
    ContentAttrs = ['desc']

class Field(Serializable):
    AttrAttrs = ['name', 'type']
    ContentAttrs = ['desc']

# used for debugging
def showComments(n):
    if n.comment:
        print n, n.comment
    map(showComments, n.children)

def process(fname='LaszloLibrary.as'):
    import os
    try: os.makedirs('jsdocs')
    except: pass
    global IGNORED
    IGNORED = []
    h = Handler()
    ce = CommentExtractor(h)
    ce.read(os.path.join(os.getenv('LPS_HOME'), 'WEB-INF/lps/lfc/', fname))
    errors = ce.errors
    for k in h.classes.values():
        if 'private' in k.keywords:
            continue
        k.finalize()
        fname = os.path.join('jsdocs', 'api-' + k.name.lower() + '.xml')
        f = open(fname, 'w')
        try:
            k.toxml(Writer(f))
        finally:
            f.close()
        import org.apache.xerces.parsers.SAXParser as SAXParser
        import java.io.FileReader as FileReader
        import org.xml.sax.InputSource as InputSource
        import org.xml.sax.helpers.DefaultHandler as DefaultHandler
        import org.xml.sax.SAXParseException as SAXParseException
        handler = DefaultHandler()
        p = SAXParser()
        p.contentHandler = handler
        p.errorHandler = handler
        r = FileReader(fname)
        try:
            p.parse(InputSource(r))
        except SAXParseException, e:
            errors.append(DocumentationError('%s: %s' % (fname, e.message)))
    #print 'ignored', IGNORED
    if errors:
        for e in errors:
            print e
        print "Mistakes were made"
        return 1
    else:
        return 0

test = process

def main(args):
    assert not args
    status = process()
    import sys
    sys.exit(status)
    

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])
else:
    #test('core/LzNode.as')
    test()
