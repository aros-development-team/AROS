/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: gadtools.library function CreateGadgetA()
    Lang: english
*/
#include <proto/exec.h>
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

        kind -     Kind of gadget. See <libraries/gadtools.h> for a list of
                   all possible kinds.
	previous - Pointer to the previous gadget in gadget-list. Create the
	           first "gadget" with CreateContext(). This may be NULL, in
		   which case CreateGadgetA() fails.
        ng -       Pointer to struct NewGadget. See <libraries/gadtools.h>.
        taglist -  Additional tags. See <libraries/gadtools.h>.

    RESULT
        A pointer to a gadget or NULL to indicate an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CreateContext(), FreeGadgets(), <libraries/gadtools.h>

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Gadget 	*gad = NULL;
    struct TagItem 	stdgadtags[] =
    {
        {GA_Left	, 0L			},
	{GA_Top		, 0L			},
	{GA_Width	, 0L			},
	{GA_Height	, 0L			},
	{GA_IntuiText	, (IPTR)NULL		},
        {GA_LabelPlace	, (IPTR)GV_LabelPlace_In},
	{GA_Previous	, (IPTR)NULL		},
	{GA_ID		, 0L			},
	{GA_DrawInfo	, (IPTR)NULL		},
	{GA_UserData	, (IPTR)NULL		},
	{TAG_DONE				}
    };

    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: Kind %d Prev 0x%lx NewGadget 0x%lx TagList 0x%lx\n",
			    kind, previous, ng, taglist));

    if (previous == NULL || ng == NULL || ng->ng_VisualInfo == NULL)
	return (NULL);

    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: previous->NextGadget 0x%lx\n",
                            previous->NextGadget));

    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: Left %d Top %d Width %d Height %d Text 0x%lx <%s>\n",
                            ng->ng_LeftEdge, ng->ng_TopEdge,
                            ng->ng_Width, ng->ng_Height,
                            ng->ng_GadgetText, ng->ng_GadgetText ? ng->ng_GadgetText : "NULL"));
    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: TextAttr 0x%lx ID %d Flags 0x%08lx VisualInfo 0x%x UserData 0x%x\n",
                            ng->ng_TextAttr, ng->ng_GadgetID,
                            ng->ng_Flags, ng->ng_VisualInfo, ng->ng_UserData));

    DEBUG_CREATEGADGETA(if (taglist)
	    {
		struct TagItem *tag;
		APTR state = taglist;
		while (tag = NextTagItem(&state))
		{
		    dprintf("\t%08lx %08lx\n", tag->ti_Tag,tag->ti_Data);
		}
	    });

    /* Emm: The makeXXX() functions expect this, and with 'normal' object creation,
     * this is always true. However, with Triton previous->NextGadget is trashed
     * after a window resize. What Triton does is somewhat questionable, but
     * setting this pointer doesn't hurt, so...
     */

    previous->NextGadget = NULL;

    /* Set the default label place according to the gadget type. */

    if (kind == LISTVIEW_KIND)
	stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Above;
    else if (kind != BUTTON_KIND)
	stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Left;
    
    stdgadtags[TAG_Left].ti_Data = ng->ng_LeftEdge;
    stdgadtags[TAG_Top].ti_Data = ng->ng_TopEdge;
    stdgadtags[TAG_Width].ti_Data = ng->ng_Width;
    stdgadtags[TAG_Height].ti_Data = ng->ng_Height;

    if (ng->ng_GadgetText)
    {
        ULONG old_ng_flags = ng->ng_Flags;
	
	if (kind == BUTTON_KIND) ng->ng_Flags &= ~NG_HIGHLABEL;	
    	stdgadtags[TAG_IText].ti_Data = (IPTR)makeitext(GTB(GadToolsBase), ng, taglist);
	if (kind == BUTTON_KIND) ng->ng_Flags = old_ng_flags;
	
    	if (!stdgadtags[TAG_IText].ti_Data)
	{
	    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: can't make label\n"));
            return (NULL);
	}
    } else {
    	stdgadtags[TAG_IText].ti_Tag = TAG_IGNORE;
    }

    stdgadtags[TAG_Previous].ti_Data = (IPTR)previous;
    stdgadtags[TAG_ID].ti_Data = ng->ng_GadgetID;
    stdgadtags[TAG_DrawInfo].ti_Data = (IPTR)(VI(ng->ng_VisualInfo)->vi_dri);
    stdgadtags[TAG_UserData].ti_Data = (IPTR)ng->ng_UserData;

    /* Calculate label placement.*/
    if ((ng->ng_Flags & PLACETEXT_LEFT))
        stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Left;
    else if ((ng->ng_Flags & PLACETEXT_RIGHT))
        stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Right;
    else if ((ng->ng_Flags & PLACETEXT_ABOVE))
        stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Above;
    else if ((ng->ng_Flags & PLACETEXT_BELOW))
        stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Below;

    switch(kind)
    {
	case BUTTON_KIND:
            gad = makebutton(GTB(GadToolsBase), 
                             stdgadtags,
                             VI(ng->ng_VisualInfo),
                             taglist);
            break;

	case CHECKBOX_KIND:
            gad = makecheckbox(GTB(GadToolsBase),
                               stdgadtags,
                               VI(ng->ng_VisualInfo),
                               taglist);
            break;

	case CYCLE_KIND:
            gad = makecycle(GTB(GadToolsBase),
                            stdgadtags,
                            VI(ng->ng_VisualInfo),
			    ng->ng_TextAttr,
                            taglist);
            break;

	case MX_KIND:
            gad = makemx(GTB(GadToolsBase),
                	 stdgadtags,
                	 VI(ng->ng_VisualInfo),
			 ng->ng_TextAttr,
                	 taglist);
            break;

	case PALETTE_KIND:
            gad = makepalette(GTB(GadToolsBase),
                              stdgadtags,
                              VI(ng->ng_VisualInfo),
                              taglist);
            break;

	case TEXT_KIND:
            gad = maketext(GTB(GadToolsBase),
                	   stdgadtags,
                	   VI(ng->ng_VisualInfo),
                	   ng->ng_TextAttr,
                	   taglist);
            break;

	case NUMBER_KIND:
            gad = makenumber(GTB(GadToolsBase),
                             stdgadtags,
                             VI(ng->ng_VisualInfo),
                             ng->ng_TextAttr,
                             taglist);
            break;

	case SLIDER_KIND:
            gad = makeslider(GTB(GadToolsBase),
                             stdgadtags,
                             VI(ng->ng_VisualInfo),
                             ng->ng_TextAttr,
                             taglist);

            break;

	case SCROLLER_KIND:
            gad = makescroller(GTB(GadToolsBase),
                               stdgadtags,
                               VI(ng->ng_VisualInfo),
                               taglist);
	    break;

	case STRING_KIND:
            gad = makestring(GTB(GadToolsBase),
                             stdgadtags,
                             VI(ng->ng_VisualInfo),
                             ng->ng_TextAttr,
                             taglist);
	    break;

	case INTEGER_KIND:
            gad = makeinteger(GTB(GadToolsBase),
                              stdgadtags,
                              VI(ng->ng_VisualInfo),
                              ng->ng_TextAttr,
                              taglist);
	    break;

	case LISTVIEW_KIND:
            gad = makelistview(GTB(GadToolsBase),
                               stdgadtags,
                               VI(ng->ng_VisualInfo),
                               ng->ng_TextAttr,
                               taglist);
	    break;
	
	case GENERIC_KIND:
	    gad = makegeneric(GTB(GadToolsBase),
	    		      stdgadtags,
			      VI(ng->ng_VisualInfo),
			      ng->ng_TextAttr,
			      taglist);
	    break;
	    
    } /* switch(kind) */

    if (gad)
    {
#if 0
        struct Gadget *gad2 = gad;
	while (gad2)
	{
	    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: created gadget 0x%lx\n", gad2));
	    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: mark GTYPE_GADTOOLS, GadgetID %ld UserData 0x%lx\n",gad->GadgetID,gad->UserData));
	    gad2->GadgetType |= GTYP_GADTOOLS;
	    gad2 = gad2->NextGadget;
	}
#endif

	/*
	 * Scan through all childgadgets and the return the last one, so we return
	 * a ptr with no NextGadget which would collide with our triton fix above
	 */
	while (gad->NextGadget)
	{
	    gad = gad->NextGadget;
	}
	DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: return gadget 0x%lx\n", gad));
    }
    else
    {
    	previous->NextGadget = NULL;
        FreeVec((APTR)stdgadtags[TAG_IText].ti_Data);
    }

    DEBUG_CREATEGADGETA(dprintf("CreateGadgetA: Return 0x%x\n", gad));
    
    return (gad);
    AROS_LIBFUNC_EXIT
    
} /* CreateGadgetA */
