/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH4(void, RefreshDTObjectA,

/*  SYNOPSIS */
	AROS_LHA(Object           *, object, A0),
	AROS_LHA(struct Window    *, window, A1),
	AROS_LHA(struct Requester *, req   , A2),
	AROS_LHA(struct TagItem   *, attrs , A3),

/*  LOCATION */
	struct Library *, DataTypesBase, 13, DataTypes)

/*  FUNCTION

    Refresh a specified object sending the GM_RENDER message to the object.

    INPUTS

    object   --  pointer to the data type object to refresh; may be NULL
    window   --  pointer to the window; may be NULL
    req      --  must be NULL
    attrs    --  additional attributes (currently none defined)

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    AddDTObject(), RemoveDTObject(), intuition.library/RefreshGList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(object == NULL || window == NULL)
	return;

    RefreshGList((struct Gadget *)object, window, req, 1);

    AROS_LIBFUNC_EXIT
} /* RefreshDTObjectA */
