/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: DOS function LoadSeg()
    Lang: english
*/
#include <dos/dos.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#define DEBUG 1
#include <aros/debug.h>
#include "dos_intern.h"

BPTR LoadSeg_AOS(BPTR file);
BPTR LoadSeg_ELF(BPTR file);
BPTR LoadSeg_AOUT(BPTR file);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, LoadSeg,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),

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
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	UnLoadSeg()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    BPTR file, segs=0;

    /* Open the file */
    file=Open(name,MODE_OLDFILE);
    if(file)
    {
D(bug("Loading \"%s\"...\n", name));
	/* Then try to load the different file formats */
/*	  segs=LoadSeg_AOS(file); Not yet */
	if(!segs)
	    segs=LoadSeg_ELF(file);
	if(!segs)
	    segs=LoadSeg_AOUT(file);

	/* Clean up */
	Close(file);
    }
    /* And return */
    return segs;
    AROS_LIBFUNC_EXIT
} /* LoadSeg */
