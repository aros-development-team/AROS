# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************
"""
File: parseinstructions.py
Author: Oliver Steele, P T Withington
Copyright 2002-2004 by Laszlo Systems, Inc.  All rights reserved.
Description: SWF instruction printer and parser
"""

#
# Imports
#

from __future__ import nested_scopes
from types import *
import org.openlaszlo.sc.Actions as Actions
import org.openlaszlo.sc.Emitter as Emitter
import org.openlaszlo.sc.Values as Values
import org.openlaszlo.sc.Instructions as Instructions

# TODO [2004-03-05 ptw] Why does
# from org.openlaszlo.sc.Instructions import *
# not work?
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

import re
false, true = 0, 1


#
# Instruction Printer
#

class InstructionPrinter(Emitter):
    def __init__(self, prefix='  ', writer=None):
        self.linePrefix = prefix
        self.labelStack = []
        self.nextLabel = 1
        self.writer = writer

    def assemble(self, instrs):
        map(self.emit, instrs)
        return None

    def emit(self, instr):
        labelStack = self.labelStack
        if instr.getIsLabel():
            label = instr.name
            if labelStack and label == labelStack[-1][1]:
                while labelStack and label == labelStack[-1][1]:
                    sourceType = labelStack[-1][0]
                    del labelStack[-1:]
                    self.linePrefix = self.linePrefix[:-2]
                    comment = {DefineFunction: 'of function\n'}.get(sourceType, '')
                    if comment:
                        comment = ' // ' + comment
                    print >> self.writer, self.linePrefix + 'end' + comment
            else:
                print >> self.writer, '%s:' % label
            return
        op = instr.op
        args = instr.args
        if op is Actions.DefineFunction or op is Actions.DefineFunction2:
            target = args[0]
            labelStack.append((op, target))
            print >> self.writer, self.linePrefix + '%s' % instr
            self.linePrefix += '  '
        elif op is Actions.WITH:
            labelStack.append((op, args[0]))
            print >> self.writer, self.linePrefix + 'with'
            self.linePrefix += '  '
        else:
            if instr.getHasTarget():
                instr = instr.replaceTarget(instr.getTarget())
            elif instr.getIsLabel():
                instr = Label(instr.name)
            print >> self.writer, self.linePrefix + `instr`


# Instruction Parser
#

# Parses Flash instructions from a string, returns a list of
# instruction objects.
# Multi-line (/* ... */) and single-line (// ...) comments are
# stripped, the string is split at newlines, and each line is
# tokenized.  Tokens are separated by whitespace and/or commas.


def parseInstruction(str):
    # String -> Instruction | None
    # strip single-line comments
    slc = re.compile(r"""//.*$|("[^"]*"|'[^']*'|[^'"/]*)""")
    t = re.sub(slc, lambda m: m.groups("")[0], str)
    # split into atoms (finds quoted strings and comma and/or
    # whitespace-separated strings)
    atom = re.compile(r"""("[^"]*"|'[^']*'|[^'"\s,]+)\s*,?\s*""")
    t = re.findall(atom, t)
    if len(t) == 0:
        return
    # handle labels
    if len(t) == 1 and t[0][-1] == ":":
        return LABEL(t[0][:-1])
    instruction = Instructions.NameInstruction.get(t[0])
    if len(t) > 1:
        if not instruction.op.args:
            raise "%s cannot take an argument in %r" % (instruction, str)
        # Canonicalize the arguments
        def canon(arg):
            if arg.find("r:") == 0:
                arg = int(arg[2:])
                if instruction is PUSH:
                    return Values.Register(arg)
            else:
                map = {'NULL': Values.Null,
                       'UNDEF': Values.Undefined,
                       'FALSE': Values.False,
                       'TRUE': Values.True
                       }
                v = map.get(arg.upper())
                try:
                    if v:
                        arg = v
                    else:
                        arg = eval(arg)
                except NameError:
                    # Convert unrecognized tokens to
                    # string literals
                    pass
                    # Convert unrecognized tokens to
                    # string literals
                if type(arg) is TupleType:
                    if instruction is PUSH:
                        return arg
                    else:
                        return arg[1]
                if type(arg) is IntType:
                    arg = long(arg)
                if instruction is PUSH:
                    return arg
            return arg
        args = [ canon(x) for x in t[1:] ]
        instruction = instruction(args)
    else:
        if hasattr(instruction, '__call__'):
            #raise "%s requires an argument in %r" % (instruction, str)
            instruction = instruction()
    return instruction

def parseInstructions(v):
    # String -> [Instruction]
    #
    # TODO: [2002-12-18 ptw] Perhaps it would be better to parse...
    mlc = re.compile(r"""/\*[^*]*\*+([^/*][^*]*\*+)*/|("[^"]*"|'[^']*'|[^"'/]*)""") # make emacs happy: "
    instrs = []
    # strip multi-line comments
    v = re.sub(mlc, lambda m: m.groups("")[1], v)
    # allow multi-line strings
    for t in v.split("\n"):
        instruction = parseInstruction(t)
        if instruction:
            instrs.append(instruction)
    return instrs
