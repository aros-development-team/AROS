/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/intuition.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>

        AROS_LH4(struct Gadget *, CreateGadgetA,

/*  SYNOPSIS */
	AROS_LHA(ULONG, kind, D0),
	AROS_LHA(struct Gadget *, previous, A0),
	AROS_LHA(struct NewGadget *, ng, A1),
	AROS_LHA(struct TagItem *, taglist, A2),

/*  LOCATION */
	struct Library *, GadtoolsBase, 5, Gadtools)

/*  FUNCTION
        Creates a gadtools gadget.

    INPUTS

        kind -     kind of gadget. See <libraries/gadtools.h> for a list of
                   all kinds
	previous - previous gadget
        ng -       pointer to struct NewGadget
        taglist -  additional tags

    RESULT
        A point to a gadget or NULL to indicate an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadtoolsBase *,GadtoolsBase)

    BOOL error = TRUE;
    struct Gadget *gad;
    STRPTR classname;

    return NULL;

    if (previous == NULL || ng == NULL || ng->ng_VisualInfo == NULL)
	return NULL;

    switch(kind)
    {
    case BUTTON_KIND:
	classname = BUTTONGCLASS;
	error = FALSE;
	break;
    }

    if (error)
	return NULL;

    gad = (struct Gadget *)NewObject(NULL, classname,
	GA_Left, ng->ng_LeftEdge,
	GA_Top, ng->ng_TopEdge,
	GA_Width, ng->ng_Width,
	GA_Height, ng->ng_Height,
	GA_Text, ng->ng_GadgetText,
	GA_TextAttr, ng->ng_TextAttr,
	GA_Previous, previous,
	TAG_END);

    if (gad)
	gad->GadgetType |= GTYP_GADTOOLS;

    return gad;
    AROS_LIBFUNC_EXIT
} /* CreateGadgetA */
