# File: xmldocutils.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

from types import StringType, ListType, UnicodeType

False, True = 0, 1

def encode(str):
    if '&' in str:
        str = str.replace('&', '&amp;')
    if '<' in str:
        str = str.replace('<', '&lt;')
    if '>' in str:
        str = str.replace('>', '&gt;')
    return str

class InputError:
    def __init__(self, message):
        self.message = message

    def __repr__(self):
        return self.message

class Writer:
    def __init__(self, out):
        self.out = out
        self.stack = []
        self.prefix = ''
        self.openStartTag = False
        self.hasChildren = False

    def closeStartTag(self):
        if self.openStartTag:
            self.out.write('>')
        self.openStartTag = False

    def beginElement(self, name, attrs={}):
        self.closeStartTag()
        if self.prefix:
            self.out.write('\n')
        self.out.write(self.prefix + '<' + name)
        for k, v in attrs.items():
            if isinstance(v, UnicodeType):
                v = str(v)
            self.out.write(' %s=%r' % (k, encode(v)))
        self.openStartTag = True
        self.hasChildren = False
        self.prefix += '  '
        self.stack.append(name)

    def endElement(self):
        name = self.stack.pop()
        self.prefix = self.prefix[:-2]
        if self.openStartTag:
            self.out.write('/>')
        else:
            if self.hasChildren:
                self.out.write('\n' + self.prefix)
            self.out.write('</' + name + '>')
        self.openStartTag = False
        self.hasChildren = True

    def text(self, s):
        self.closeStartTag()
        self.out.write(encode(s))

class Serializable:
    AttrAttrs = []
    ContentAttrs = []
    TextAttrs = []
    
    def __init__(self, **attrs):
        self.__dict__.update(attrs)

    def toxml(self, handler):
        name = getattr(self, 'Name', self.__class__.__name__.lower())
        attrs = {}
        dict = self.__dict__
        for k in self.AttrAttrs:
            if dict.get(k) is not None:
            #if dict.has_key(k):
                attrs[k] = dict[k]
        handler.beginElement(name, attrs)
        for k in self.TextAttrs:
            if dict.get(k, None):
                handler.text(dict[k])
        for k in self.ContentAttrs:
            if dict.has_key(k):
                v = dict[k]
                if isinstance(v, StringType) or isinstance(v, UnicodeType):
                    handler.beginElement(k)
                    handler.text(v)
                    handler.endElement()
                elif isinstance(v, ListType):
                    if v:
                        handler.beginElement(k)
                        for i in v:
                            i.toxml(handler)
                        handler.endElement()
                elif v:
                    v.toxml(handler)
        handler.endElement()

class Comment:
    def __init__(self, str=None):
        self.comment = str
        self.params = {}
        self.attributes = {}
        self.events = {}
        self.paramTypes = {}
        self.fieldTypes = {}
        self.eventTypes = {}
        self.keywords = []

    def lookupDicts(self, key):
        if key in ('attr', 'attribute'):
            key = 'field'
        return {'param': (self.params, self.paramTypes, Param),
                'field': (self.attributes, self.fieldTypes, Attribute),
                'event': (self.events, self.eventTypes, Event)
                }.get(key)

    def getFields(self, key):
        dict, types, cons = self.lookupDicts(key)
        return [cons(name=k, desc=v, type=types.get(k))
                for k, v in dict.items()]

    def start(self, key):
        entry = self.lookupDicts(key)
        if entry is not None:
            self.cparam = None
            self.dict, self.types, _ = entry

    def append(self, key, value):
        if key in ('param','field','event'):
            if self.cparam:
                self.dict[self.cparam] += '\n' + value
            else:
                if ':' not in value:
                    raise Error('bad doc tag: %r' % value)
                    value = value + ':'
                key, value = value.split(':', 1)
                key = key.strip()
                value = value.strip()
                if ' ' in key:
                    type, key = key.split(' ', 1)
                    self.types[key] = type
                assert not self.dict.get(key), 'duplicate def for %r' % self.cparam
                self.dict[key] = value
                self.cparam = key
        elif key == 'keywords':
            self.keywords += value.split()
        else:
            if getattr(self, key, None):
                setattr(self, key, getattr(self, key) + '\n' + value)
            else:
                setattr(self, key, value)

def parseComment(str, validKeys):
    c = Comment()
    s = []
    w = None
    for line in (str or '').split('\n'):
        line = line.strip()
        if line.startswith('@') and len(line) > 2 and ' ' in line:
            w, line = line.split(' ', 1)
            w = w[1:]
            # handle '@return: ...'
            if w.endswith(':'):
                w = w[:-1]
            if w not in validKeys and w + 's' in validKeys:
                w += 's'
            if w not in validKeys and w[-1] == 's' and w[:-1] in validKeys:
                w = w[:-1]
            if w not in validKeys:
                if len(validKeys) > 1:
                    msg = 'Expected one of ' + ', '.join(validKeys)
                else:
                    msg = "Expected '%s'" % validKeys[0]
                raise InputError("Unknown doc tag '%s' in %s.  %s." % (w, line, msg))
            c.start(w)
        if w:
            c.append(w, line)
        else:
            s.append(line)
    if s:
        c.comment = '\n'.join(s).strip()
    return c

class Param(Serializable):
    AttrAttrs = ['name', 'required', 'type']
    ContentAttrs = ['desc']

class Attribute(Serializable):
    AttrAttrs = ['name', 'type', 'category', 'default', 'keywords']
    ContentAttrs = ['desc']

class Event(Serializable):
    AttrAttrs = ['name']
    ContentAttrs = ['desc']
