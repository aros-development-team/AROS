/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: DOS function LoadSeg()
    Lang: english
*/

#define DEBUG 0

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include "dos_intern.h"

#if AROS_MODULES_DEBUG
#include <exec/nodes.h>
#include <exec/lists.h>
#include <string.h>

struct MinList debug_seglist, free_debug_segnodes;

#endif

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
        is loaded into his own memory section and a handle on all of them
        is returned. The segments can be freed with UnLoadSeg().

    INPUTS
        name - NUL terminated name of the file.

    RESULT
        Handle to the loaded executable or 0 if the load failed.
q        IoErr() gives additional information in that case.

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
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    void (* FunctionArray[3])();
    BPTR file, segs=0;

    FunctionArray[0] = __AROS_GETVECADDR(DOSBase,7);
    FunctionArray[1] = __AROS_GETVECADDR(SysBase,33);
    FunctionArray[2] = __AROS_GETVECADDR(SysBase,35);

    /* Open the file */
    D(bug("[LoadSeg] Opening '%s'...\n", name));
    file = Open (name, FMF_READ);

    if (file)
    {
	D(bug("[LoadSeg] Loading '%s'...\n", name));

	segs = InternalLoadSeg (file, NULL, (void *)FunctionArray, NULL);

	if (segs)
        {
#if AROS_MODULES_DEBUG
            struct debug_segnode *segnode;

	    Forbid();
            segnode = (struct debug_segnode *)REMHEAD(&free_debug_segnodes);
            Permit();

	    if (segnode)
            {
                NameFromFH(file, segnode->name, sizeof(segnode->name));

                segnode->seglist = segs;

		Forbid();
                ADDTAIL(&debug_seglist, segnode);
		Permit();
            }
#endif

            SetIoErr(0);
        }
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
