/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/alib.h>
#include <intuition/cghooks.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/intuition.h>
#include <proto/intuition.h>

	AROS_LH4(ULONG, SetGadgetAttrsA,

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
	This function sends OM_SET to the object.

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), DisposeObject(), SetAttrsA(), GetAttr(), MakeClass(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    IPTR result;
    struct opSet ops;
    struct GadgetInfo gi;

    gi.gi_Screen = window->WScreen;
    gi.gi_Window = window;
    gi.gi_Requester = requester;
    gi.gi_RastPort = window->RPort;
    if (requester != NULL)
    {
	gi.gi_Layer = requester->ReqLayer;
	gi.gi_Domain.Left = requester->RelLeft;
	gi.gi_Domain.Top = requester->RelTop;
	gi.gi_Domain.Width = requester->Width;
	gi.gi_Domain.Height = requester->Height;
    }
    else
    {
	gi.gi_Layer = window->WLayer;
	if ((gadget->GadgetType & GTYP_GZZGADGET) == GTYP_GZZGADGET)
	{
	    gi.gi_Domain.Left = window->BorderLeft;
	    gi.gi_Domain.Top = window->BorderTop;
	}
	else
	{
	    gi.gi_Domain.Left = 0;
	    gi.gi_Domain.Top = 0;
	}
	gi.gi_Domain.Width = window->Width;
	gi.gi_Domain.Height = window->Height;
    }
    gi.gi_Pens.DetailPen = window->DetailPen;
    gi.gi_Pens.BlockPen = window->BlockPen;
    gi.gi_DrInfo = GetScreenDrawInfo(window->WScreen);

    ops.MethodID     = OM_SET;
    ops.ops_AttrList = tagList;
    ops.ops_GInfo    = &gi;

    result = DoMethodA ((Object *)gadget, (Msg)&ops);
    FreeScreenDrawInfo(window->WScreen, gi.gi_DrInfo);

    return(result);
    AROS_LIBFUNC_EXIT
} /* SetGadgetAttrsA */
