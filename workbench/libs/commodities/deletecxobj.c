/*
    (C) 1997-98 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*****************************************************************************

    NAME */

#include "cxintern.h"
#include <proto/commodities.h>

    AROS_LH1(VOID, DeleteCxObj,

/*  SYNOPSIS */

	AROS_LHA(CxObj *, co, A0),

/*  LOCATION */

	struct Library *, CxBase, 8, Commodities)

/*  FUNCTION

    Delete the commodity object 'co'. By deleting, it's meant that the
    memory used for the object is freed and if the object was in the
    commodity hierarchy, it's removed.

    INPUTS

    co  --  the object to be deleted (may be NULL)

    RESULT

    NOTES

    After deleting the commodity object, the handle 'co' is no longer valid.
    Deleteing an object that has other objects attached to it is (that
    should be deleted too) is easiest accomplished by using the
    DeleteCxObjAll() function.

    EXAMPLE

    BUGS

    SEE ALSO

    DeleteCxObjAll(), 

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (co == NULL)
    {
	return;
    }

    RemoveCxObj(co);
    FreeCxStructure(co, CX_OBJECT, CxBase);

    AROS_LIBFUNC_EXIT
} /* DeleteCxObj */

