/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS function InternalLoadSeg()
    Lang: english
*/


#define DEBUG 0

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <loadseg/loadseg.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH4(BPTR, InternalLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(BPTR       , fh           , D0),
        AROS_LHA(BPTR       , table        , A0),
        AROS_LHA(LONG_FUNC *, funcarray    , A1),
        AROS_LHA(LONG *     , stack        , A2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 126, Dos)

/*  FUNCTION
        Loads from fh.
        Functionarray is a pointer to an array of functions. See below.

        This function really only tries to load the different file
        formats aos, elf and aout.

    INPUTS
        fh            : Filehandle to load from
        table         : ignored
        funcarray : array of functions to be used for read, seek, alloc and free
           FuncTable[0] -> bytes  = ReadFunc(readhandle, buffer, length), DOSBase
                           D0                D1          A0      D0       A6
           FuncTable[1] -> Memory = AllocFunc(size,flags), ExecBase
                           D0                 D0   D1      A6
           FuncTable[2] -> FreeFunc(memory, size), ExecBase
                                    A1       D0    A6
           FuncTable[3] -> pos    = SeekFunc(readhandle, pos, mode), DOSBase
                           D0                D0          D1   D2
        stack         : pointer to storage (LONG) for stacksize.
                        (currently ignored)

    RESULT
        seglist  - pointer to loaded Seglist or NULL in case of failure.

    NOTES
        FuncTable[3] is not used for Amiga HUNK format files, but is required
                     for ELF.

    EXAMPLE

    BUGS
       Use of table and stack are not implemented, yet!

    SEE ALSO
        UnLoadSeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR seg;
    SIPTR error;

    seg = LoadSegment(fh, table, (SIPTR *)funcarray, stack, &error, (struct Library *)DOSBase);

    SetIoErr(error);
    return seg;
  
    AROS_LIBFUNC_EXIT
} /* InternalLoadSeg */
