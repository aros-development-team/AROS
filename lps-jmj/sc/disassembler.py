# File: disassembler.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

import org.openlaszlo.sc.Values as Values
PushTypes = Values.PushTypes

def parsePushArgs(block, constants):
    args = []
    bytes = block.bytes
    i = 3
    while i < len(bytes):
        t = bytes[i]
        i += 1
        if t == PushTypes.String:
            start = i
            while bytes[i]:
                i += 1
            v = ''.join(map(chr, bytes[start:i]))
            i += 1
        elif t == PushTypes.CONSTANT_INDEX8:
            n = bytes[i]
            i += 1
            v = lookupConstant(constants, n)
        elif t == PushTypes.CONSTANT_INDEX16:
            n = bytes[i] + (bytes[i+1] << 8)
            i += 2
            v = lookupConstant(constants, n)
        elif t == PushTypes.Integer:
            v = bytes[i] + (bytes[i+1] << 8) + (bytes[i+2] << 16) + (bytes[i+3] << 24)
            i += 4
        elif t == PushTypes.Boolean:
            v = bytes[i]
            i += 1
            if v:
                v = Values.True
            else:
                v = Values.False
        elif t == PushTypes.Register:
            n = bytes[i]
            i += 1
            v = Values.Register(n)
        elif t == PushTypes.Null:
            v = Values.Null
        elif t == PushTypes.Undefined:
            v = Values.Undefined
        elif t == PushTypes.Double:
            import struct
            v, = struct.unpack("<d", ''.join(map(chr, bytes[i:i+4])))
            i += 4
        else:
            raise 'unknown push type %s' % hex(t)
        args.append(v)
    return args

def lookupConstant(constants, n):
    bytes = constants
    i = 5
    while True:
        start = i
        while bytes[i]:
            i += 1
        # land on the 0
        if not n:
            return ''.join(map(chr, bytes[start:i]))
        n -= 1
        i += 1 # skip over the 0

def disassemble(bytes):
    import org.openlaszlo.sc.Actions as Actions, com.laszlosytems.sc.Instructions as Instructions
    if bytes[0] == Actions.DefineFunction.opcode:
        i = 3
        while bytes[i]:
            i += 1
        name = ''.join(map(chr, bytes[3:i]))
        i += 1
        nargs = bytes[i] + (bytes[i+1] << 8)
        i += 2
        args = []
        while nargs:
            start = i
            while bytes[i]:
                i += 1
            args.append(''.join(map(chr, bytes[start:i])))
            i += 1
            nargs -= 1
        target = bytes[-2] + (bytes[-1] << 8)
        return Instructions.DefineFunction(target, name, args)
    else:
        assert len(bytes) == 5, 'bad length %s' % map(hex, bytes)
        target = bytes[-2] + (bytes[-1] << 8)
        for a, b in [(Actions.BRANCH, Instructions.BRANCH),
                     (Actions.BranchIfTrue, Instructions.BranchIfTrue),
                     (Actions.WITH, Instructions.WITH)]:
            if bytes[0] == a.opcode:
                return b(target)
        raise 'unknown opcode %s' % hex(bytes[0])
