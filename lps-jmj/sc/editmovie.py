# File: editmovie.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

# http://www.openswf.org/spec/SWFfileformat.html

# Rather than munge the existing modularity, we just cache the
# deconstructed swf for its multiple uses
cacheFile = None
cacheTime = None
cacheAllBlocks = None
cacheActionBlocks = None
cacheFlashFile = None

def JGENreadProgramsInternal(infile):
    global cacheFile, cacheTime, cacheActionBlocks
    from os import path
    mt =  path.getmtime(infile)
    if cacheFile == infile:
        if cacheTime == mt:
            return cacheActionBlocks
    cacheFile = None
    f = FlashFile.parse(infile)
    import org.openlaszlo.sc.ActionBlockFinder as ActionBlockFinder
    actionBlocks = ActionBlockFinder.getActionBlocks(f);
    cacheFile = infile
    cacheTime = mt
    cacheActionBlocks = actionBlocks
    cacheFlashFile = f
    return actionBlocks

def readProgramsInternal(infile):
    global cacheFile, cacheTime, cacheAllBlocks, cacheActionBlocks
    from os import path
    mt =  path.getmtime(infile)
    if cacheFile == infile:
        if cacheTime == mt:
            return cacheActionBlocks, cacheAllBlocks
    cacheFile = None
    from struct import unpack
    f = open(infile, 'rb')
    bytes = f.read()
    f.close()
    assert bytes[:3] == 'FWS'
    actionBlocks = []
    allBlocks = []
    # SWF Header layout
    # 'F', 'W', 'S', byte version, int length, RECT size, short rate, short, count
    # RECT is uint:5 size, int:size xmin, int:size xmax, int:size ymin, int:size ymax
    rectSize = ord(bytes[8])>>3
    # rectBytes is the 5 size bits plus 4 bounds rounded to bytes
    rectBytes = int(((5 + (rectSize * 4)) + 7) / 8)
    # sizeof(signature + version + length + RECT + rate + count)
    start = 3 + 1 + 4 + rectBytes + 2 + 2 
    allBlocks.append(bytes[:start])
    while start < len(bytes):
        blockstart = start
        tag, = unpack('<H', bytes[start:start+2])
        start += 2
        tagid = int(tag >> 6)
        taglen = tag & 0x3f
        if taglen == 0x3f:
            taglen, = unpack('<L', bytes[start:start+4])
            start += 4
        if tagid == 0x0c:
            # extract just the program from action blocks
            block = bytes[start:start+taglen]
            allBlocks.append(block)
            actionBlocks.append(block)
        else:
            # keep the tag on non-action blocks
            allBlocks.append(bytes[blockstart:start+taglen])
        start += taglen
    cacheFile = infile
    cacheTime = mt
    cacheAllBlocks = allBlocks
    cacheActionBlocks = actionBlocks
    return actionBlocks, allBlocks

def readPrograms(infile):
    """Read infile:pathname, returning a list of the action blocks"""
    actionBlocks, _ = readProgramsInternal(infile)
    #actionBlocks = JGENreadProgramsInternal
    return actionBlocks

def actionBlockHeader(program):
    """Compute the DoAction tag for a program"""
    from struct import pack
    assert program[-1] == '\x00'
    newlen = len(program)
    if newlen < 0x3f:
        tag = (0x0c << 6) | newlen
        return pack('<H', tag)
    else:
        tag = (0x0c << 6) | 0x3f
        return pack('<H', tag) + pack('<L', newlen)

def setSWFFileSize(file):
    """Reset the SWF file size tag"""
    from struct import pack
    filesize = file.tell()
    file.seek(4)
    file.write(pack('<L', filesize))
    file.close()

def replaceFirstBlock(infile, ofile, program):
    """Copies infile:pathname to ofile:pathname, replacing the first
    action block by program:[byte]."""
    # TODO [2004-03-04 ptw] Why does a java byte[] get treated as signed by Jython?
    programString = ''.join(map(lambda b: chr(b&0xFF), program))
    actionBlocks, allBlocks = readProgramsInternal(infile)
    first = actionBlocks[0]
    file = open(ofile, 'wb')
    for block in allBlocks:
        if actionBlocks and block is actionBlocks[0]:
            actionBlocks = actionBlocks[1:]
            if block is first:
                file.write(actionBlockHeader(programString))
                file.write(programString)
            else:
                file.write(actionBlockHeader(block))
                file.write(block)
        else:
            file.write(block)
    setSWFFileSize(file)

def replaceProgramBlocks(infile, ofile, program, blockIndex=0, preservePreloader=0):
    """Copies infile:pathname to ofile:pathname, replacing the nth
    (0-based) action block by program:[byte] and eliding all other
    action blocks."""
    import string
    # TODO [2004-03-04 ptw] Why does a java byte[] get treated as signed by Jython?
    programString = ''.join(map(lambda b: chr(b&0xFF), program))
    deleteBlocks, allBlocks = readProgramsInternal(infile)
    # the block to replace
    replace = deleteBlocks[blockIndex]
    # In a swf with a preloader, preserve the first two as
    # blocks as they operate the splash.
    keepBlocks = []
    if preservePreloader:
        keepBlocks = deleteBlocks[:2]
    file = open(ofile, 'wb')
    for block in allBlocks:
        if deleteBlocks and block is deleteBlocks[0]:
            deleteBlocks = deleteBlocks[1:]
            if keepBlocks and block is keepBlocks[0]:
                assert not replace in keepBlocks
                keepBlocks = keepBlocks[1:]
                file.write(actionBlockHeader(block))
                file.write(block)
            elif block is replace:
                file.write(actionBlockHeader(programString))
                file.write(programString)
        else:
            file.write(block)
    setSWFFileSize(file)

def test():
    infile = '/Users/ptw/Laszlo/compiler/lps/WEB-INF/lps/lfc/laszlolibrary.swf'
    ofile = 'Newlaszlolibrary.swf'
    program = [0]
    replaceFirstBlock(infile, ofile, program)

#test()
