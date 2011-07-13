/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS function LoadSegment()
    Lang: english
*/


#define DEBUG 0

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "loadseg_intern.h"

/*****************************************************************************

    NAME */
#include <loadseg/loadseg.h>

        BPTR LoadSegment(

/*  SYNOPSIS */
        BPTR        fh,
        BPTR        table,
        SIPTR     * funcarray,
        LONG *      stack,
        SIPTR     * error,
        struct Library *lib)

/*  LOCATION
        loadseg.lib       

    FUNCTION
        Loads from fh. Although specified as a BPTR, 'fh' is passed
        to the funcarray routines unchanged, and is never inspected
        nor modified by LoadSegment()

        funcarray is a pointer to an array of functions. See below.

        This function really tries to load the AOS HUNK and AROS ELF
        file formats.

    INPUTS
        fh            : Filehandle to load from
        table         : BPTR to BPTR that will point to an array
                        of BPTRs to segments (AOS HUNK format only)
        funcarray : array of functions to be used for read, seek, alloc and free
           FuncTable[0] -> bytes  = ReadFunc(readhandle, buffer, length) lib
                           D0                D1          A0      D0       A6
           FuncTable[1] -> Memory = AllocFunc(size,flags), ExecBase
                           D0                 D0   D1      A6
           FuncTable[2] -> FreeFunc(memory, size), ExecBase
                                    A1       D0    A6
           FuncTable[3] -> pos    = SeekFunc(readhandle, pos, mode) lib
                           D0                D0          D1   D2    A6
        stack         : pointer to storage (LONG) for stacksize.
                        (currently ignored)
        error         : DOS IoError() style error code return
        lib           : Library base to pass to FuncTable[0] and [3]

    RESULT
        seglist  - pointer to loaded Seglist or NULL in case of failure.

    NOTES
        FuncTable[3] is not used for Amiga HUNK format files, but is required
        for ELF.

        The returned segment should be deallocatable by Dos/UnloadSeg(),
        so ensure that your AllocFunc() routine does *not* allocate
        pooled memory!

    EXAMPLE

    BUGS
       Use of table and stack are not implemented, yet!

    SEE ALSO
        UnLoadSeg()

    INTERNALS

*****************************************************************************/
{
    typedef struct _segfunc_t
    {
    	ULONG id;
        BPTR (*func)(BPTR, BPTR, SIPTR *, LONG *, SIPTR *, struct Library *);
        D(CONST_STRPTR format;)
    } segfunc_t;

    #define SEGFUNC(id, format) {id, LoadSegment_##format D(, (STRPTR)#format)}
    
    static const segfunc_t funcs[] = 
    {
        SEGFUNC(0x7f454c46, ELF),
        SEGFUNC(0x000003f3, AOS)
    };
  
    BPTR segs = 0;

    if (fh)
    {
        UBYTE i;
	const UBYTE num_funcs = sizeof(funcs) / sizeof(funcs[0]);
    	ULONG id;
	LONG len;

	*error = 0;
    	len = ilsRead(fh, &id, sizeof(id));
	if (len == sizeof(id)) {
	    id = AROS_BE2LONG(id);
	    for (i = 0; i < num_funcs; i++) {
		if (funcs[i].id == id) {
		    segs = (*funcs[i].func)(fh, table, (SIPTR *)funcarray,
			stack, error, lib);
		    D(bug("[LoadSegment] %s loading %p as an %s object.\n",
			segs ? "Succeeded" : "FAILED", fh, funcs[i].format));
		    return segs;
 		}
 	    }
 	}
    }

    *error = ERROR_NOT_EXECUTABLE;
    return BNULL;

} /* LoadSegment */

APTR _ilsAllocVec(SIPTR *funcarray, ULONG size, ULONG req)
{
    UBYTE *p = ilsAllocMem(size, req);
    if (!p)
    	return NULL;
    D(bug("allocmem %p %d\n", p, size));
    *((ULONG*)p) = (ULONG)size;
    return p + sizeof(ULONG);       
}

void _ilsFreeVec(SIPTR *funcarray, void *buf)
{
    UBYTE *p = (UBYTE*)buf;
    ULONG size;
    if (!buf)
    	return;
    p -= sizeof(ULONG);
    size = ((ULONG*)p)[0];
    D(bug("freemem %p %d\n", p, size));
    if (!size)
    	return;

    ilsFreeMem(p, size);
}
