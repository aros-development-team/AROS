/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/alib.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <intuition/cghooks.h>
#include <intuition/classusr.h>
#include <intuition/classes.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <utility/tagitem.h>

	AROS_LH4(IPTR, SetGadgetAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct Gadget *,    gadget,    A0),
	AROS_LHA(struct Window *,    window,    A1),
	AROS_LHA(struct Requester *, requester, A2),
	AROS_LHA(struct TagItem *,   tagList,   A3),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 110, Intuition)

/*  FUNCTION
	Sets some tags and provides gadget specific data. Prefer this to
	SetAttrsA(), if you are manipulating gadgets.

    INPUTS
	gadget - Change the attributes of this gadget
	window - The window of the gadget
	requester - The requester of the gadget (or NULL)
	tagList - This is a list of attribute/value-pairs

    RESULT
	Depends in the class. For gadgets, this value is non-zero if
	they need redrawing after the values have changed. Other classes
	will define other return values.

    NOTES
	This function sends OM_SET to the gadget object.

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrsA(), GetAttr(), DoGadgetMethodA(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

    INTERNALS
	SetGadgetAttrsA(gad, win, req, tags) ist just a replacement for
	DoGadgetMethod(gad, win, req, OM_SET, tags, NULL).

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    
    IPTR result;
    
    if (window || requester)
    {
	struct opSet ops;

	ops.MethodID     = OM_SET;
	ops.ops_AttrList = tagList;
	ops.ops_GInfo	 = 0; /* Not really necessary as it will be filled in by DoGadgetMethodA */
	
	result = DoGadgetMethodA(gadget, window, requester, (Msg)&ops);
    }
    else
    {
        result = SetAttrsA((Object *)gadget, tagList);
    }
   
    return result;
    
    AROS_LIBFUNC_EXIT
    
} /* SetGadgetAttrsA */
