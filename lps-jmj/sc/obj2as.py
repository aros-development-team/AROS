# File: obj2as.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

"""
Todo:
- only write each function once
- function def -> expr

Later:
- constant pool
"""

import os
False, True = 0, 1

import org.openlaszlo.sc.Instructions as Instructions
import org.openlaszlo.sc.Values as Values

# TODO [2004-03-09 ptw] Fix jython import * to import static members
NONE = Instructions.NONE
NextFrame = Instructions.NextFrame
PreviousFrame = Instructions.PreviousFrame
PLAY = Instructions.PLAY
STOP = Instructions.STOP
ToggleQuality = Instructions.ToggleQuality
StopSounds = Instructions.StopSounds
NumericAdd = Instructions.NumericAdd
SUBTRACT = Instructions.SUBTRACT
MULTIPLY = Instructions.MULTIPLY
DIVIDE = Instructions.DIVIDE
OldEquals = Instructions.OldEquals
OldLessThan = Instructions.OldLessThan
LogicalAnd = Instructions.LogicalAnd
LogicalOr = Instructions.LogicalOr
NOT = Instructions.NOT
StringEqual = Instructions.StringEqual
StringLength = Instructions.StringLength
SUBSTRING = Instructions.SUBSTRING
POP = Instructions.POP
INT = Instructions.INT
GetVariable = Instructions.GetVariable
SetVariable = Instructions.SetVariable
SetTargetExpression = Instructions.SetTargetExpression
StringConcat = Instructions.StringConcat
GetProperty = Instructions.GetProperty
SetProperty = Instructions.SetProperty
DuplicateMovieClip = Instructions.DuplicateMovieClip
RemoveClip = Instructions.RemoveClip
TRACE = Instructions.TRACE
StartDragMovie = Instructions.StartDragMovie
StopDragMovie = Instructions.StopDragMovie
StringLessThan = Instructions.StringLessThan
RANDOM = Instructions.RANDOM
MBLENGTH = Instructions.MBLENGTH
ORD = Instructions.ORD
CHR = Instructions.CHR
GetTimer = Instructions.GetTimer
MBSUBSTRING = Instructions.MBSUBSTRING
MBORD = Instructions.MBORD
MBCHR = Instructions.MBCHR
GotoFrame = Instructions.GotoFrame
GetUrl = Instructions.GetUrl
WaitForFrame = Instructions.WaitForFrame
SetTarget = Instructions.SetTarget
GotoLabel = Instructions.GotoLabel
WaitForFrameExpression = Instructions.WaitForFrameExpression
PUSH = Instructions.PUSH
BRANCH = Instructions.BRANCH
GetURL2 = Instructions.GetURL2
BranchIfTrue = Instructions.BranchIfTrue
CallFrame = Instructions.CallFrame
GotoExpression = Instructions.GotoExpression
DELETE = Instructions.DELETE
DELETE2 = Instructions.DELETE2
VarEquals = Instructions.VarEquals
CallFunction = Instructions.CallFunction
RETURN = Instructions.RETURN
MODULO = Instructions.MODULO
NEW = Instructions.NEW
VAR = Instructions.VAR
InitArray = Instructions.InitArray
InitObject = Instructions.InitObject
TypeOf = Instructions.TypeOf
TargetPath = Instructions.TargetPath
ENUMERATE = Instructions.ENUMERATE
ADD = Instructions.ADD
LessThan = Instructions.LessThan
EQUALS = Instructions.EQUALS
ObjectToNumber = Instructions.ObjectToNumber
ObjectToString = Instructions.ObjectToString
DUP = Instructions.DUP
SWAP = Instructions.SWAP
GetMember = Instructions.GetMember
SetMember = Instructions.SetMember
Increment = Instructions.Increment
Decrement = Instructions.Decrement
CallMethod = Instructions.CallMethod
NewMethod = Instructions.NewMethod
BitwiseAnd = Instructions.BitwiseAnd
BitwiseOr = Instructions.BitwiseOr
BitwiseXor = Instructions.BitwiseXor
ShiftLeft = Instructions.ShiftLeft
ShiftRight = Instructions.ShiftRight
UShiftRight = Instructions.UShiftRight
SetRegister = Instructions.SetRegister
CONSTANTS = Instructions.CONSTANTS
WITH = Instructions.WITH
DefineFunction = Instructions.DefineFunction
BranchIfFalse = Instructions.BranchIfFalse
LABEL = Instructions.LABEL
COMMENT = Instructions.COMMENT
CHECKPOINT = Instructions.CHECKPOINT
BLOB = Instructions.BLOB




def isint(s):
    try:
        int(s)                          # for effect
        return True
    except ValueError:
        pass
    return False

# Strings representing integers are better pushed as ints if they are
# less than 256
def issmallint(s):
    try:
        if type(s) is type(0):
            return int(s) < 256
    except ValueError:
        pass
    return False

def toint(s):
    try:
        return int(s)                   # for effect
    except ValueError:
        return None

import org.openlaszlo.utils.LZHttpUtils as LzHttpUtils
import org.xml.sax.helpers.AttributesImpl as AttributesImpl

### url decode all the attribute values
def URLdecodeAttributes(attrs):
    nattrs = AttributesImpl()
    for idx in range(attrs.length):
      aval = attrs.getValue(idx)
      ### [hqm 2004-05-22] Need to use UTF-8 encoding on SWF6 files, windows-1252 for swf5
      nattrs.addAttribute(attrs.getURI(idx),
                          attrs.getLocalName(idx),
                          attrs.getQName(idx),"CDATA",LzHttpUtils.urldecode(aval, "windows-1252"))
    return nattrs

class SimpleOptimizer:
    def __init__(self, receiver):
        self.receiver = receiver
        self.instr = None
        self.bytes = getattr(receiver, 'bytes', [])

    def emit(self, instr):
        if self.instr and instr.IsPush:
            self.instr.args += instr.args
        else:
            if self.instr:
                self.receiver.emit(self.instr)
                self.instr = None
            if instr.IsPush:
                self.instr = instr
            else:
                self.receiver.emit(instr)

    def flush(self):
        if self.instr:
            self.receiver.emit(self.instr)
            self.instr = None

FIDS_TABLE_NAME = '$fids'
SIDS_TABLE_NAME = '$sids'
SIDS_TABLE_REGISTER = 0
SCRATCH_REGISTER = 1
ROOT_REGISTER = 2

import org.apache.xerces.parsers.SAXParser as SAXParser
import java.io.FileInputStream as FileInputStream
import java.io.InputStreamReader as InputStreamReader
import java.io.BufferedReader as BufferedReader
import org.xml.sax.InputSource as InputSource

ERROR = 'error'
TOP = 'top'
IGNORE = 'ignore'
INITING_OBJECT = 'makingObject'
INITING_ARRAY = 'makingArray'
EDITING_OBJECT = 'editingObject'
EDITING_ARRAY = 'editingArray'

import org.xml.sax.helpers.DefaultHandler as DefaultHandler
class Compiler(DefaultHandler):
    def __init__(self, instrs, functions, FIDcounts, SIDcounts, recursive, runtime):
        self.hasPreloader = False
        self.locator = None
        from Compiler import InstructionCollector
        self.collector = InstructionCollector(False, True)
        self.emit = self.collector.emit
        assert runtime in ("swf5", "swf6", "swf7"), 'uknown runtime %r' % runtime
        Instructions.setRuntime(runtime)
        
        self.functions = functions # {name: [byte]}
        self.globalFunctions = {}
        self.FIDcounts = FIDcounts
        self.SIDcounts = SIDcounts
        self.recursive = recursive
        self.sids = {} # {sid: varname | None}
        self.stack = [[ERROR]]
        self.addInstructions(instrs)
        self.push('_root')
        self.emit(GetVariable)
        self.emit(SetRegister(ROOT_REGISTER))
        self.emit(POP)
        self.addDefinitions(functions)
        self.push(SIDS_TABLE_NAME)
        self.push(0)
        self.emit(InitObject)
        self.emit(DUP)
        self.emit(SetRegister(SIDS_TABLE_REGISTER))
        self.emit(SetVariable)

    def addInstructions(self, instrs):
        map(self.emit, instrs)

    def addDefinitions(self, functions):
        for k, v in functions.items():
            name = k
            pname = name
            isexpr = v[3] == 0
            if not self.FIDcounts.has_key(name):
                continue
            if self.FIDcounts[name] == 1 and isexpr:
                continue
            self.globalFunctions[name] = True
            if isexpr:
                pname = '/*%s*/' % pname
                self.push(Values.Register(ROOT_REGISTER))
                self.pushkey(name)
            self.emit(BLOB('function %s (...) {...}' % pname, v))
            if isexpr:
                self.emit(SetMember)

    def push(self, *args):
        self.emit(PUSH(args))

    def pushkey(self, key):
        # Try to conserve space by using ints rather than strings,
        # but not if the int would take more space than a constant
        # reference
        if issmallint(key):
            self.push(int(key))
        else:
            self.push(key)

    def saveregs(self):
        self.push(Values.Register(ROOT_REGISTER),
                  Values.Register(SIDS_TABLE_REGISTER))

    def restoreregs(self):
        self.emit(SetRegister(SIDS_TABLE_REGISTER))
        self.emit(POP)
        self.emit(SetRegister(ROOT_REGISTER))
        self.emit(POP)

    def compile(self, fname):
        p = SAXParser()
        p.contentHandler = self
        p.errorHandler = self
        fis = FileInputStream(fname)
        r = BufferedReader(InputStreamReader(fis, "UTF-8"))
        try:
            p.parse(InputSource(r))
        except:
            if self.locator:
                print '#line', self.locator.lineNumber
            raise

    def visitBoolean(self, v):
        value = Values.False
        if v == 'true':
            value = Values.True
        self.push(value)

    def visitNull(self, v):
        self.push(Values.Null)

    def visitUndefined(self, v):
        self.push(Values.Undefined)

    def visitNumber(self, v):
        if v == 'Infinity':
            self.push('infinity')
            self.emit(GetVariable)
            return
        if v == '-Infinity':
            self.push('infinity')
            self.emit(GetVariable)
            self.push(-1)
            self.emit(MULTIPLY)
            return
        if v == 'NaN':
            self.push('NaN')
            self.emit(GetVariable)
            return
        # optimization to int handled by peep-hole optimizer
        self.push(float(v))

    def visitString(self, v):
        self.push(v)

    def compileColor(self, attrs):
        mc = attrs.getValue('mc')
        self.eval(mc)                   # mc
        self.push(1, 'Color')           # mc 1 'Color'
        self.emit(NEW)                  # c
        if attrs.getValue('rgb'):
            rgb = attrs.getValue('rgb')
            self.emit(SetRegister(SCRATCH_REGISTER)) # c
            self.saveregs()             # c savedregs
            self.push(rgb, 1,
                      Values.Register(SCRATCH_REGISTER),
                      'setRGB')         # c savedregs rgb 1 c setRGB
            self.emit(CallMethod)       # c savedregs c.setRGB(rgb)
            self.emit(POP)              # c savedregs
            self.restoreregs()          # c


    # Recreate a textfield by calling mc.createTextField, on the parent.
    def compileFlash6Text(self, attrs):
        mc = attrs.getValue('mc')
        # We need to create and retrieve the new TextField. 
        # It will be a new child named "$LzText" on mc.
        #  TextField's depth is always 1
        #                           name    depth, x, y, width, height
        # call mc.createTextField( $LzText, 1,     0, 0, 100,   12 );

        #self.push(attrs.getValue('height'), attrs.getValue('width'),0,0,1,'$LzText',6);
        self.push(20,100,0,0,1,'$LzText',6);
        self.eval(mc);
        self.push('createTextField');
        self.emit(CallMethod)       # undefined return value
        self.emit(POP);
        # get the newly created TextField and leave it on the stack
        self.eval(mc+".$LzText") # mc.$LzText
        return

    def compileSound(self, attrs):
        if attrs.getValue('mc'):
            self.eval(attrs.getValue('mc')) # mc
            self.push(1)                # mc 1
        else:
            self.push(0)                # 0
        self.push('Sound')              # (mc 1)|0 'Sound'
        self.emit(NEW)                  # s
        if attrs.getValue('vol'):
            vol = attrs.getValue('vol')
            self.emit(SetRegister(SCRATCH_REGISTER)) # s
            self.saveregs()             # s savedregs
            self.push(vol, 1,
                      Values.Register(SCRATCH_REGISTER),
                      'setVolume')      # s savedregs vol 1 c setVolume
            self.emit(CallMethod)       # s savedregs c.setVolume(vol)
            self.emit(POP)              # s savedregs
            self.restoreregs()          # s
        if attrs.getValue('pan'):
            pan = attrs.getValue('pan')
            self.emit(SetRegister(SCRATCH_REGISTER)) # s
            self.saveregs()             # s savedregs
            self.push(pan, 1,
                      Values.Register(SCRATCH_REGISTER),
                      'setPan')         # s savedregs pan 1 c setPan
            self.emit(CallMethod)       # s savedregs c.setPan(pan)
            self.emit(POP)              # s savedregs
            self.restoreregs()          # s

    def visitObject(self, attrs, mode):
        assert attrs.getValue('v') != 'Array.prototype'
        # implicit object type: m => movie, fid => function
        m = attrs.getValue('m')
        fid = attrs.getValue('FID')
        sid = attrs.getValue('sid')
        constructor = attrs.getValue('constructor')
        # Builtin object that properties have been attached to
        if attrs.getValue('isbuiltin'):
            # Get the object so it can be edited
            self.eval(attrs.getValue('n'))
            mode = EDITING_OBJECT
        elif fid:
            name = fid
            if name.endswith('()'):
                name = name[:-2]
                assert self.globalFunctions.has_key(name), "undefined fid %r" % name
                self.saveregs()         # savedregs
                self.push(0)            # savedregs 0
                self.push(Values.Register(ROOT_REGISTER)) # savedregs 0 _root
                self.pushkey(name)      # savedregs 0 _root name
                self.emit(CallMethod)   # savedregs _root.name()
                self.emit(SetRegister(SCRATCH_REGISTER))
                self.emit(POP)          # savedregs
                self.restoreregs()      #
                self.push(Values.Register(SCRATCH_REGISTER)) # _root.name()
            else:
                assert not name.endswith(')'), "can't compile %r" % name
                assert not sid, "sid not handled for %r" % name
                if not self.functions.has_key(name):
                    self.warn('undefined fid %r', name)
                    # Nothing to emit
                    self.functions[name] = []
                if self.globalFunctions.has_key(name):
                    self.push(Values.Register(ROOT_REGISTER))
                    self.pushkey(name)
                    self.emit(GetMember)
                else:
                    pname = '/*%s*/' % name
                    bytes = self.functions[name]
                    self.emit(BLOB('function %s (...) {...}' % pname, bytes))
            mode = EDITING_OBJECT
        # handle custom constructors, sid or no.  These should be rare
        # so we don't try to optimize INIT vs. EDIT
        elif constructor in ('Boolean', 'Date', 'Number', 'String'):
            if constructor in ('Date', 'Number'):
                self.visitNumber(attrs.getValue('args'))
            elif constructor == 'Boolean':
                self.visitBoolean(attrs.getValue('args'))
            else:
                self.visitString(attrs.getValue('args'))
            self.push(1);
            self.push(constructor)
            self.emit(NEW)
            mode = EDITING_OBJECT
        elif constructor == 'TextField':
            self.compileFlash6Text(attrs)
            mode = EDITING_OBJECT
        elif constructor == 'Color':
            self.compileColor(attrs)
            mode = EDITING_OBJECT
        elif constructor == 'Sound':
            self.compileSound(attrs)
            mode = EDITING_OBJECT
        elif constructor == 'Array':
            # TODO: [2003-09-17 ptw] Teach the serializer to serialize
            # Arrays in order so that we could optimize this to
            # INIT_ARRAY when there are no recursive references (but
            # worry about arrays with non-indexed properties)
            self.push(0)
            self.emit(InitArray)
            mode = EDITING_ARRAY
        # TODO: [2003-10-29 ptw] Could this be serialized as a constructor?
        elif m:
            self.eval(m)
            mode = EDITING_OBJECT
        # only need to 'edit' if SID is used recursively
        elif sid and self.recursive.has_key(sid):
            self.push(0)
            assert not constructor, "constructor %r not handled for %r" % (constructor, mode)
            self.emit(InitObject)
            mode = EDITING_OBJECT
        # SID is not used recursively, so be more efficient and 'init'
        # in endElement instead
        # N.B. custom constructors not handled in INIT mode
        if mode == INITING_OBJECT:
            assert not constructor, "constructor %r not handled for %r" % (constructor, mode)
        # If the object has an SID, and it has been created already,
        # set the sid-table entry for it
        if sid and mode in (EDITING_OBJECT, EDITING_ARRAY):
            self.setSid(sid)
        # Note mode, # elements, SID and if there is a resolve method
        # to be run
        self.stack.append([mode, 0, sid, attrs.getValue('x')])

    MAP = {
        'b': visitBoolean,
        'l': visitNull,
        'n': visitNumber,
        'o': visitObject,
        's': visitString,
        'u': visitUndefined
        }

    def startElement(self, uri, name, qName, attrs):
        len0 = len(self.stack)
        state = self.stack[-1]
        mode = state[0]

	attrs = URLdecodeAttributes(attrs)

        if attrs.getValue('v') == 'Array.prototype':
            assert name == 'o'
            assert mode in (INITING_ARRAY, EDITING_ARRAY), 'Array.prototype is child of %s' % mode
            self.stack.append([IGNORE])
            return
#         self.emit(COMMENT('<%s %s>' %
#                           (name,
#                            ' '.join(['%s=%r' % (attrs.getLocalName(i), attrs.getValue(i))
#                                      for i in range(attrs.length)]))))
        # ignore movies and comments
        if name == 'm' or name == 'c' or name == 't':
            return
        # reference (explicit for editing, implicit for initing)
        if mode in (EDITING_OBJECT, EDITING_ARRAY):
            self.emit(DUP)
        elif mode == INITING_OBJECT:
            state[1] += 1
        # initArray takes only a list of elements in index order, with
        # no gaps
        elif mode == INITING_ARRAY:
            an = attrs.getValue('n')
            try:
                ai = int(an)
            except ValueError:
                assert 0, "non-integral array index: %s" % an
            if state[2] == -1:
                state[2] = ai
            else:
                assert ai == (state[2] - 1), 'non-monotonic array index: %s' % an
            state[1] += 1
            state[2] -= 1
        # slot name
        if mode in (INITING_OBJECT, INITING_ARRAY, EDITING_OBJECT, EDITING_ARRAY):
            self.pushkey(attrs.getValue('n'))
        # slot value
        if attrs.getValue('r'):
            r = attrs.getValue('r')
            if not self.sids.has_key(r):
                self.warn('undefined reference to %r', r)
                # Hush further warnings
                self.sids[r] = None
            if self.sids[r]:
                self.eval(self.sids[r])
            else:
                self.push(Values.Register(SIDS_TABLE_REGISTER))
                self.pushkey(r)
                self.emit(GetMember)
            if name == 'o':
                self.stack.append([EDITING_OBJECT, 0])
        elif name == 'o':
            self.visitObject(attrs, INITING_OBJECT)
        elif name == '_top':
            self.hasPreloader = (attrs.getValue('preloader') == 'true')
            self.visitObject(attrs, TOP)
        else:
            self.MAP[name](self, attrs.getValue('v'))
            if mode in (EDITING_OBJECT, EDITING_ARRAY):
                self.emit(SetMember)
        assert len(self.stack) == len0 + (name == '_top') + (name == 'o')

    def endElement(self, uri, name, qName):
        if name != 'o':
            return
#         self.emit(COMMENT('/' + name))
        state = self.stack.pop()
        mode = state[0]
        if mode in (EDITING_OBJECT, EDITING_ARRAY):
            self.emitResolveMethod(state)
        elif mode == INITING_OBJECT:
            self.push(state[1])
            self.emit(InitObject)
            self.setSid(state[2])
            self.emitResolveMethod(state)
        elif mode == INITING_ARRAY:
            self.push(state[1])
            self.emit(InitArray)
            self.setSid(state[2])
            self.emitResolveMethod(state)
        elif mode in (TOP, IGNORE):
            pass
        else:
            raise "unknown mode: %s" % mode
        # Now look at the parent mode to see if we are just collecting
        # this object on the stack or if we need to store it
        parentMode = self.stack[-1][0]
        if parentMode in (EDITING_OBJECT, EDITING_ARRAY):
            self.emit(SetMember)
        elif parentMode == TOP:
            # Pop the object, we're done with it
            self.emit(POP)

    def emitResolveMethod(self, state):
        '''Expects object on TOS, leaves it there'''
        # If object has a resolve method, call it
        if len(state) > 3 and state[3]:
            self.emit(SetRegister(SCRATCH_REGISTER)) # o
            self.saveregs()             # o savedregs
            self.push(0,
                      Values.Register(SCRATCH_REGISTER),
                      '$SID_RESOLVE_OBJECT'
                      )      # o savedregs 0 o $SID_RESOLVE_OBJECT
            self.emit(CallMethod)       # o savedregs o.$SID_RESOLVE_OBJECT()
            self.emit(POP)              # o savedregs
            self.restoreregs()          # o
        # TODO: [2003-09-08 ptw] delete $SID_RESOLVE_OBJECT

    def setDocumentLocator(self, locator):
        self.locator = locator

    def warn(self, fmtstr, *args):
        pre = 'warning: '
        if self.locator:
            pre = 'line %d: ' % self.locator.lineNumber
        print pre + fmtstr % args

    def eval(self, name):
        parts = name.split('.')
        self.push(parts[0])
        self.emit(GetVariable)
        for part in parts[1:]:
            self.push(part)
            self.emit(GetMember)

    def setSid(self, id):
        "expects value on top of stack (and leaves it)"
        if self.sids.has_key(id):
            self.warn('duplicate definition for SID %r', id)
        # Only emit for SID's that were referenced
        if self.SIDcounts.has_key(id):
            self.emit(SetRegister(SCRATCH_REGISTER)) # v
            self.push(Values.Register(SIDS_TABLE_REGISTER)) # v $sids
            self.pushkey(id)            # v $sids id
            self.push(Values.Register(SCRATCH_REGISTER)) # v $sids id v
            self.emit(SetMember)        # v
            # object is now in external table
            self.sids[id] = None

# Collects counts of FID's and SID's that are used so we only include
# functions that are used and only generate SIDs for objects that have
# multiple references
class IDCollector(DefaultHandler):
    def __init__(self):
        self.FIDcounts = {}
        self.SIDcounts = {}
        self.SIDstack = []
        self.recursive = {}

    def parse(self, fname):
        p = SAXParser()
        p.contentHandler = self
        p.errorHandler = self
        fis = FileInputStream(fname)
        r = BufferedReader(InputStreamReader(fis, "UTF-8"))
        p.parse(InputSource(r))
        return self.FIDcounts, self.SIDcounts, self.recursive

    def startElement(self, uri, name, qName, attrs):
        if name == 'o':
            attrs = URLdecodeAttributes(attrs)
            fid = attrs.getValue('FID')
            sid = attrs.getValue('sid')
            self.SIDstack.append(sid)
            # yes 'r', we are counting references to SIDs
            r = attrs.getValue('r')
            if fid:
                inc = 1
                if fid.endswith('()'):
                    fid = fid[:-2]
                    inc = 2
                self.FIDcounts[fid] = self.FIDcounts.get(fid, 0) + inc
            if r:
                self.SIDcounts[r] = self.SIDcounts.get(r, 0) + 1
                if r in self.SIDstack:
                    self.recursive[r] = self.recursive.get(r, 0) + 1

    def endElement(self, uri, name, qName):
        if name == 'o':
            self.SIDstack.pop()

import org.openlaszlo.sc.Assembler as Assembler
import org.openlaszlo.sc.Optimizer as Optimizer

def optimize(fname, props, includeObjectHierarchy=True):
    basename = fname
    swfname = basename + '__.swf'
    objxmlname = basename + "obj__.xml"
    mcxmlname = basename + "obj__.xml"

    runtime = props.getProperty("runtime")

    print 'reading %r...' % swfname
    from collector import collect
    functions = collect(swfname)

    print 'compiling %r...' % mcxmlname
    import attachMCs
    (mcinstrs, fxtextnames) = attachMCs.mc2flasm(mcxmlname)

    # fxtextnames now contains a list of fixed-size test resources , will look like
    # ['lzinputtext/lztahoe8/8/plain/398/257', 'lzinputtext/lztahoe8/8/plain/400/250', ...]
    FIDcounts, SIDcounts, recursive = IDCollector().parse(objxmlname)
    c = Compiler(mcinstrs, includeObjectHierarchy and functions or {}, FIDcounts, SIDcounts, recursive, runtime)

    if includeObjectHierarchy:
        print 'compiling %r...' % objxmlname
        c.compile(objxmlname)

    c.collector.emit(STOP)
    instrs = c.collector.getInstructions(True)
    # end marker
    instrs.add(NONE)
    assembler = Optimizer(Assembler())
    program = assembler.assemble(instrs)

    # if there are fixed-size text fields to add, do it now
    if (runtime == 'swf5' and len(fxtextnames) > 0):
        print "checking for fixedsize text fields: %s..." % swfname
        import org.openlaszlo.sc.AddInputText as addinputtext
        inputswf = basename + '__.swf'
        swfname = basename + '_ftxt.swf'
        # convert the fxtextnames table into a String[] for Java
        print "fixed-size text fields found, adding to swf file: %s " % fxtextnames.keys()
        # this is how we pass a String[] to a java method
        from jarray import array
        from java.lang import String
        fxnames = array(fxtextnames.keys(), String)
        addinputtext.addtext(inputswf, swfname, fxnames)

    if (runtime != 'swf5'):
        # If flash 6 or greater, output lzo to a tmpfile, and then flash6 compress it.
        ofile = basename + '.lzo.tmp'
    else:
        ofile = basename + '.lzo'

    print 'writing %r...' % ofile
    import editmovie
    editmovie.replaceProgramBlocks(swfname, ofile, program, -1, c.hasPreloader)

    # If our runtime version >= Flash6, do flash 6 compression on file
    if (runtime != 'swf5'):
        import org.openlaszlo.sc.CompressFlashFile as CompressFlashFile
        print 'doing flash6-compression on lzo %r...' % ofile
        compressed_ofile = basename + '.lzo'
        CompressFlashFile.compressFile(ofile, compressed_ofile)

    print 'done'

def c(fname='simple/simple', includeObjectHierarchy=True):
    fname = os.path.join(os.getenv('LPS_HOME'), 'demos', fname)
    optimize(fname, includeObjectHierarchy)

def t(fname):
    if '/' not in fname:
        fname = os.path.join('tests', fname)
    c = Compiler([], {}, {})
    from instructions import InstructionPrinter
    c.assembler = InstructionPrinter()
    c.emit = c.assembler.emit
    c.compile(fname)

################

import java.util.Properties as Properties

def t6():
    props6 = Properties()
    props6.setProperty('runtime', 'swf6')
    optimize("c:\\sandboxes\\intl2\\test\\hello2", props6)


def t5():
    props = Properties()
    props.setProperty('runtime', 'swf5')
    optimize("c:\\sandboxes\\intl2\\test\\hello5", props)

    


#c()
#c('contacts/contacts')
