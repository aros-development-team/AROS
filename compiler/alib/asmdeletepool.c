/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Simply call the exec function 
    Lang: english
*/
#define AROS_TAGRETURNTYPE APTR

#include "alib_intern.h"

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/exec.h>

	AROS_UFH2(void, AsmDeletePool,

/*  SYNOPSIS */
	AROS_UFHA(APTR, poolHeader, A0),
	AROS_UFHA(struct ExecBase *, SysBase, A6)) 

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_USERFUNC_INIT
    DeletePool(poolHeader);
    AROS_USERFUNC_EXIT
} /* AsmDeletePool */
