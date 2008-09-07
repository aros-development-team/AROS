/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH4(ULONG, SetDTAttrsA,

/*  SYNOPSIS */
	AROS_LHA(Object *          , o    , A0),
	AROS_LHA(struct Window    *, win  , A1),
	AROS_LHA(struct Requester *, req  , A2),
	AROS_LHA(struct TagItem   *, attrs, A3),

/*  LOCATION */
	struct Library *, DataTypesBase, 10, DataTypes)

/*  FUNCTION

    Set the attributes of a data type object.

    INPUTS

    o      --  pointer to the data type object the attributes of which to set
    win    --  window that the object has been added to
    attrs  --  attributes to set (terminated with TAG_DONE)
               tags are specified in <datatypes/datatypesclass.h>

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    GetDTAttrsA(), intuition.library/SetGadgetAttrsA(),
    <datatypes/datatypesclass.h>

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return SetGadgetAttrsA((struct Gadget *)o, win, req, attrs);

    AROS_LIBFUNC_EXIT
} /* SetDTAttrsA */
