/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#define USE_BOOPSI_STUBS
#include <proto/intuition.h>
#include <datatypes/datatypesclass.h>
#include <intuition/intuition.h>
#include <clib/boopsistubs.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH4(IPTR, DoDTMethodA,

/*  SYNOPSIS */
	AROS_LHA(Object           *, o  , A0),
	AROS_LHA(struct Window    *, win, A1),
	AROS_LHA(struct Requester *, req, A2),
	AROS_LHA(Msg               , msg, A3),

/*  LOCATION */
	struct Library *, DataTypesBase, 15, DataTypes)

/*  FUNCTION

    Perform a specific datatypes methodl.

    INPUTS

    o    --  pointer to data type object
    win  --  window the object is attached to
    req  --  requester the object is attached to
    msg  --  the message to send to the object    

    RESULT

    The value returned by the specified method.
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    intuition.library/DoGadgetMethodA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR retval;
   
    switch(msg->MethodID)
    {
    case DTM_COPY:
    case DTM_PRINT:
    case DTM_WRITE:
	((struct dtGeneral *)msg)->dtg_GInfo = NULL;
	retval = DoMethodA(o, msg);
	break;
      
    default:
	retval = DoGadgetMethodA((struct Gadget *)o, win, req, msg);
	break;
   }
   
    return retval;

    AROS_LIBFUNC_EXIT
} /* DoDTMethodA */
