/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <proto/exec.h>

    AROS_LH1(VOID, DisposeCxMsg,

/*  SYNOPSIS */

	AROS_LHA(CxMsg *, cxm, A0),

/*  LOCATION */

	struct Library *, CxBase, 28, Commodities)

/*  FUNCTION

    Delete the commodity message 'cxm'. This function can be used to
    swallow all InputEvents by disposing every commodity message of type
    CXM_IEVENT.

    INPUTS

    cxm  -  the commodity message to delete (must NOT be NULL) 

    RESULT

    NOTES

    This function can only be used within the context of the input handler,
    and not from within a commodities' context; that is if you for instance
    get a CXM_IEVENT CxMsg from a commodity sender object, you must
    ReplyMsg() it instead of Disposing it.

    EXAMPLE

    BUGS

    SEE ALSO

    

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    Remove((struct Node *)cxm);
    FreeCxStructure(cxm, CX_MESSAGE, CxBase);

    AROS_LIBFUNC_EXIT
} /* DisposeCxMsg */
