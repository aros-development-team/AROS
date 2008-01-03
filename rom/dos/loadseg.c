/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS function LoadSeg()
    Lang: english
*/
#ifdef __x86_64__
#define DEBUG 1
#else
#define DEBUG 0
#endif

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(BPTR, LoadSeg,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 25, Dos)

/*  FUNCTION
        Loads an executable file into memory. Each hunk of the loadfile
        is loaded into its own memory section and a handle on all of them
        is returned. The segments can be freed with UnLoadSeg().

    INPUTS
        name - NUL terminated name of the file.

    RESULT
        Handle to the loaded executable or NULL if the load failed.
        IoErr() gives additional information in that case.

    NOTES
        This function is built on top of InternalLoadSeg()

    EXAMPLE

    BUGS

    SEE ALSO
        UnLoadSeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    void (* FunctionArray[3])();
    BPTR file, segs=0;

    FunctionArray[0] = __AROS_GETVECADDR(DOSBase,7);  /* Read() */
    FunctionArray[1] = __AROS_GETVECADDR(SysBase,33); /* AllocMem() */
    FunctionArray[2] = __AROS_GETVECADDR(SysBase,35); /* FreeMem() */

    /* Open the file */
    D(bug("[LoadSeg] Opening '%s'...\n", name));
    file = Open (name, FMF_READ);

    if (file)
    {
	D(bug("[LoadSeg] Loading '%s'...\n", name));

	segs = InternalLoadSeg(file, NULL, (void *)FunctionArray, NULL);

	if (segs)
            SetIoErr(0);
	else
 	    bug("[LoadSeg] Failed to load '%s'\n", name);
        Close(file);
    }
  D(else
        bug("[LoadSeg] Failed to open '%s'\n", name));
  

    /* And return */
    return segs;

    AROS_LIBFUNC_EXIT
} /* LoadSeg */
