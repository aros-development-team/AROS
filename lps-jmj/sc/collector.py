# file: collector.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

from editmovie import readPrograms
from disassembler import parsePushArgs
import os
False, True = 0, 1

class Program:
    def __init__(self, bytes):
        self.bytes = bytes
        self.index = 0

    def next(self):
        start = self.index
        size = 1
        opcode = self.bytes[start]
        if opcode & 0x80:
            size += 2 + self.bytes[start+1] + (self.bytes[start+2] << 8)
        self.index += size
        return Action(self.bytes[start:self.index])
        
class Action:
    def __init__(self, bytes):
        self.opcode = bytes[0]
        self.bytes = bytes

def collect(fname, trace=False):
    fname = fname or 'LFC.lzl'
    dict = {} # {fname: bytestring}
    dups = {}
    blocks = readPrograms(fname)
    for block in blocks:
        _collect(block, dict, dups, trace=trace)
    keys = dups.keys()
    keys.sort()
    for dup in keys:
        print 'duplicate definitions for %r' % dup
    return dict

def _collect(block, dict, dups, trace):
    program = Program(map(ord, block))
    constants = None
    while True:
        start = program.index
        block = program.next()
        op = block.opcode
        if op == 0:
            break
        if op == 0x88: # Constants
            constants = block.bytes
        if (op == 0x9b or op == 0x8e): # DefineFunction
            collect_function(block, start, program, constants, dict, dups, trace)
    return dict

def collect_function(block, start, program, constants, dict, dups, trace):
        if constants:
            raise "can't handle constant pools"
        j = 3
        while block.bytes[j]:
            j += 1
        name = ''.join(map(chr, block.bytes[3:j]))
        eof = program.index + block.bytes[-2] + (block.bytes[-1] << 8)
        s = program.index
        while s != eof:
            assert s < eof
            b = program.next()
            if (b.opcode == 0x9b or b.opcode == 0x8e):
                collect_function(b, s, program, constants, dict, dups, trace)
            s = program.index
        if not name:
            name = readNextName(program, constants)
        if name:
            if trace:
                print name
            if dict.has_key(name):
                dups[name] = True
            dict[name] = program.bytes[start:eof]
        else:
            print 'skipping unnamed function at %s' % hex(start)


def readNextName(program, constants):
    pos = program.index
    try:
        block = program.next()
        if block.opcode != 0x4c: return # DUP
        block = program.next()
        if block.opcode != 0x96: return # PUSH
        args = parsePushArgs(block, constants)
        if len(args) != 2: return
        if args[0] != 'name': return
        block = program.next()
        if block.opcode != 0x4f: return # SetMember
        return args[1]
    finally:
        program.index = pos

def tc(fname='LFC.lzl'):
    fname = os.path.join(os.getenv('LPS_HOME'), 'web-inf/lps/lfc/', fname)
    collect(fname)

#redact()
#collect(os.path.join(os.getenv('LPS_HOME'), 'web-inf/lps/lfc/LFC.lzl'), 0)
