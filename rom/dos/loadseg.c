/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:54  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/dos.h>
#include <clib/dos_protos.h>

BPTR LoadSeg_AOS(BPTR file);
BPTR LoadSeg_ELF(BPTR file);

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(BPTR, LoadSeg,

/*  SYNOPSIS */
	__AROS_LA(STRPTR, name, D1),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    BPTR file, segs=0;

    /* Open the file */    
    file=Open(name,MODE_OLDFILE);
    if(file)
    {
        /* Then try to load the different file formats */
/*        segs=LoadSeg_AOS(file); Not yet */
        if(!segs)
            segs=LoadSeg_ELF(file);
            
        /* Clean up */
        Close(file);
    }
    /* And return */
    return segs;
    __AROS_FUNC_EXIT
} /* LoadSeg */
