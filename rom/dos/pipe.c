/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Creates a pair of filehandles connected to each other
    Lang: english
*/
#include <aros/debug.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, Pipe,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,      D1),
        AROS_LHA(BPTR *,       reader,    D2),
        AROS_LHA(BPTR *,       writer,    D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 160, Dos)

/*  FUNCTION
	Creates a pair of file handles connected to each other

    INPUTS
        name       - NULL-terminated name of the file
        reader     - Pointer to BPTR to store read handle in
        writer     - Pointer to BPTR to store write handle in

    RESULT
        DOSTRUE on success, DOSFALSE on failure. IoErr() gives additional
        information if the call failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR fhin, fhout;

    /* Create file for writing */
    fhout = Open(name, MODE_NEWFILE);
    if (fhout == BNULL)
    	    return DOSFALSE;

    fhin = Open(name, MODE_OLDFILE);
    if (fhin == BNULL) {
    	    DeleteFile(name);
    	    Close(fhout);
    	    return DOSFALSE;
    }

    *reader = fhin;
    *writer = fhout;

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* Pipe */
