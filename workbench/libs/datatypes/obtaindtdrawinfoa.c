/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define USE_BOOPSI_STUBS
#include "datatypes_intern.h"
#include <proto/exec.h>
#include <proto/alib.h>
#include <utility/tagitem.h>
#include <datatypes/datatypesclass.h>
#include <intuition/classusr.h>
#include <clib/boopsistubs.h>

/*****************************************************************************

    NAME */

	AROS_LH2(APTR, ObtainDTDrawInfoA,

/*  SYNOPSIS */
	AROS_LHA(Object         *, o  ,   A0),
	AROS_LHA(struct TagItem *, attrs, A1),

/*  LOCATION */
	struct Library *, DataTypesBase, 20, DataTypes)

/*  FUNCTION

    Prepare a data type object for drawing into a RastPort; this function
    will send the DTM_OBTAINDRAWINFO method the object using an opSet
    message.

    INPUTS

    o      --  pointer to the data type object to obtain the drawinfo for;
               may be NULL in which case nothing is done
    attrs  --  additional attributes

    RESULT

    A private handle that must be passed to ReleaseDTDrawInfo when the
    application is done drawing the object, or NULL if failure.

    TAGS

        None defined so far.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    DrawDTObjectA(), ReleaseDTDrawInfo()

    INTERNALS

    HISTORY

    29.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct opSet ops;

    if(o == NULL)
	return NULL;

    ops.MethodID     = DTM_OBTAINDRAWINFO;
    ops.ops_AttrList = attrs;
    ops.ops_GInfo    = NULL;

    return (APTR)DoMethodA(o, (Msg)&ops);

    AROS_LIBFUNC_EXIT
} /* ObtainDTDrawInfoA */
