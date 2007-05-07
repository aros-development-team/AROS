/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include "datatypes_intern.h"	/* Must be after <intuition/intuition.h> */

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

        AROS_LH4(LONG, AddDTObject,

/*  SYNOPSIS */
	AROS_LHA(struct Window    *, win, A0),
	AROS_LHA(struct Requester *, req, A1),
	AROS_LHA(Object           *, obj, A2),
	AROS_LHA(LONG              , pos, D0),

/*  LOCATION */
	struct Library *, DataTypesBase, 12, DataTypes)

/*  FUNCTION

    Add an object to the window 'win' or requester 'req' at the position
    in the gadget list specified by the 'pos' argument.

    INPUTS
    
    win  --  window the object should be added to; may be NULL in which case
             nothing is done
    req  --  requester the object should be added to
    obj  --  the object to add; may be NULL in which case nothing is done
    pos  --  the position of the object in the list


    RESULT

    The position where the object was added (may be different from what
    you asked for).

    NOTES

    The object will receice a GM_LAYOUT message with the gpl_Initial field
    set to 1 when the object is added.

    EXAMPLE

    BUGS

    SEE ALSO

    RemoveDTObject(), intuition.library/AddGList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(obj == NULL || win == NULL)
	return -1;

    return AddGList(win, (struct Gadget *)obj, pos, 1, req);

    AROS_LIBFUNC_EXIT
} /* AddDTObject */

