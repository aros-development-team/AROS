/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free arguments structure from ReadArgs()
    Lang: english
*/
#include <proto/exec.h>
#include <dos/rdargs.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_NTLH1(void, FreeArgs,

/*  SYNOPSIS */
	AROS_LHA(struct RDArgs *, args, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 143, Dos)

/*  FUNCTION
	FreeArgs() will clean up after a call to ReadArgs(). If the
	RDArgs structure was allocated by the system in a call to 
	ReadArgs(), then it will be freed. If however, you allocated
	the RDArgs structure with AllocDosObject(), then you will
	have to free it yourself with FreeDosObject().

    INPUTS
	args		- The data used by ReadArgs(). May be NULL,
			  in which case, FreeArgs() does nothing.

    RESULT
	Some memory will have been returned to the system.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ReadArgs()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    if(!args)
	return;

    /* ReadArgs() failed. Clean everything up. */
    if (args->RDA_DAList)
    {
	FreeVec(((struct DAList *)args->RDA_DAList)->ArgBuf);
	FreeVec(((struct DAList *)args->RDA_DAList)->StrBuf);
	FreeVec(((struct DAList *)args->RDA_DAList)->MultVec);
	FreeVec((struct DAList *)args->RDA_DAList);

    	args->RDA_DAList = 0;
	
#if 0
	/*
	    Why do I put this here. Unless the user has been bad and
	    set RDA_DAList to something other than NULL, then this
	    RDArgs structure was allocated by ReadArgs(), so we can
	    free it. Otherwise the RDArgs was allocated by
	    AllocDosObject(), so we are not allowed to free it.

	    See the original AmigaOS autodoc if you don't believe me
	*/

    	FreeVec(args);
#endif
    }

    if (args->RDA_Flags & RDAF_ALLOCATED_BY_READARGS)
	FreeVec(args);

    AROS_LIBFUNC_EXIT
    
} /* FreeArgs */
