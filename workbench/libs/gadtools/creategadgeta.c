/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/intuition.h>
#include <intuition/gadgetclass.h>
#include <proto/utility.h>
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
	struct Library *, GadToolsBase, 5, GadTools)

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
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)

    struct Gadget *gad;
    struct TagItem stdgadtags[] = {
        {GA_Left, 0L},
	{GA_Top, 0L},
	{GA_Width, 0L},
	{GA_Height, 0L},
	{GA_Text, (IPTR)NULL},
	{GA_TextAttr, (IPTR)NULL},
	{GA_Previous, (IPTR)previous},
	{GA_ID, 0L},
	{GA_DrawInfo, (IPTR)NULL},
	{TAG_END, 0L}
    };

    if (previous == NULL || ng == NULL || ng->ng_VisualInfo == NULL)
	return NULL;

    stdgadtags[0].ti_Data = ng->ng_LeftEdge;
    stdgadtags[1].ti_Data = ng->ng_TopEdge;
    stdgadtags[2].ti_Data = ng->ng_Width;
    stdgadtags[3].ti_Data = ng->ng_Height;
    stdgadtags[4].ti_Data = (IPTR)ng->ng_GadgetText;
    stdgadtags[5].ti_Data = (IPTR)ng->ng_TextAttr;
    stdgadtags[6].ti_Data = (IPTR)previous;
    stdgadtags[7].ti_Data = ng->ng_GadgetID;
    stdgadtags[8].ti_Data = (IPTR)(((struct VisualInfo *)(ng->ng_VisualInfo))->vi_dri);

    switch(kind)
    {
    case BUTTON_KIND:
        gad = makebutton((struct GadToolsBase_intern *)GadToolsBase, 
            stdgadtags, (struct VisualInfo *)ng->ng_VisualInfo, taglist);
        break;
    case CHECKBOX_KIND:
        gad = makecheckbox((struct GadToolsBase_intern *)GadToolsBase,
            stdgadtags, (struct VisualInfo *)ng->ng_VisualInfo, taglist);
        break;
    default:
        return NULL;
    }

    if (gad)
	gad->GadgetType |= GTYP_GADTOOLS;

    return gad;
    AROS_LIBFUNC_EXIT
} /* CreateGadgetA */
