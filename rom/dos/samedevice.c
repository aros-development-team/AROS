/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, SameDevice,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock1, D1),
	AROS_LHA(BPTR, lock2, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 164, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    struct FileHandle *fh1, *fh2;
    
    if (lock1 == NULL || lock2 == NULL)
    	return DOSFALSE;
	
    fh1 = (struct FileHandle *)BADDR(lock1);
    fh2 = (struct FileHandle *)BADDR(lock2);

#warning Is this check good enough ?
    if (fh1->fh_Device == fh2->fh_Device)
    	return DOSTRUE;
    

    return DOSFALSE;
    AROS_LIBFUNC_EXIT
} /* SameDevice */
