/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#define USE_BOOPSI_STUBS
#include <proto/utility.h>
#include <proto/intuition.h>
#include <clib/boopsistubs.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

        AROS_LH2(ULONG, GetDTAttrsA,

/*  SYNOPSIS */
	AROS_LHA(Object         *, o    , A0),
	AROS_LHA(struct TagItem *, attrs, A2),

/*  LOCATION */
	struct Library *, DataTypesBase, 11, DataTypes)

/*  FUNCTION

    Get the attributes of a specific data type object.

    INPUTS

    o      --  pointer to a data type object; may be NULL
    attrs  --  the attributes to get terminated with TAG_DONE; each Tag's
               data element should contain the address of the respective
	       storage element; may be NULL

    RESULT

    The number of attributes obtained.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    SetDTAttrsA(), intuition.library/GetAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG                 result = 0;
    const struct TagItem *tstate = attrs;
    struct TagItem       *tag;
    struct opGet          opGet;

    if(o == NULL || attrs == NULL)
	return 0;
   
    opGet.MethodID = OM_GET;
   
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
	opGet.opg_AttrID  = tag->ti_Tag;
	opGet.opg_Storage = (ULONG *)tag->ti_Data;
	
	if(DoMethodA(o, (Msg)&opGet))
	    result++;
	else
	    *(opGet.opg_Storage) = 0;
    }

    return(result);

    AROS_LIBFUNC_EXIT
} /* GetDTAttrsA */
