/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/12/10 14:00:06  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 14:11:39  digulla
    Open and close screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <clib/utility_protos.h>

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <intuition/screens.h>
#include <clib/intuition_protos.h>

	AROS_LH2(struct Screen *, OpenScreenTagList,

/*  SYNOPSIS */
	AROS_LHA(struct NewScreen *, newScreen, A0),
	AROS_LHA(struct TagItem   *, tagList, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 102, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    struct TagItem * tag;
    struct NewScreen ns =
    {
	0, 0, 640, 200, 1, /* left, top, width, height, depth */
	0, 1, /* DetailPen, BlockPen */
	HIRES | LACE, /* ViewModes */
	CUSTOMSCREEN, /* Type */
	NULL, /* Font */
	NULL, /* DefaultTitle */
	NULL, /* Gadgets */
	NULL, /* CustomBitMap */
    };

    while ((tag = NextTagItem (&tagList)))
    {
	switch (tag->ti_Tag)
	{
	case SA_Left:	    ns.LeftEdge  = tag->ti_Data; break;
	case SA_Top:	    ns.TopEdge	 = tag->ti_Data; break;
	case SA_Width:	    ns.Width	 = tag->ti_Data; break;
	case SA_Height:     ns.Height	 = tag->ti_Data; break;
	case SA_Depth:	    ns.Depth	 = tag->ti_Data; break;
	case SA_DetailPen:  ns.DetailPen = tag->ti_Data; break;
	case SA_BlockPen:   ns.BlockPen  = tag->ti_Data; break;
	case SA_Type:	    ns.Type	 = tag->ti_Data; break;

	case SA_Title:
	    ns.DefaultTitle = (UBYTE *)tag->ti_Data;
	    break;

	case SA_Font:
	    ns.Font = (struct TextAttr *)tag->ti_Data;
	    break;

	case SA_Colors:
	case SA_ErrorCode:
	case SA_SysFont:
	case SA_BitMap:
	case SA_PubName:
	case SA_PubSig:
	case SA_PubTask:
	case SA_DisplayID:
	case SA_DClip:
	case SA_Overscan:
	case SA_ShowTitle:
	case SA_Behind:
	case SA_Quiet:
	case SA_AutoScroll:
	case SA_Pens:
	case SA_FullPalette:
	case SA_ColorMapEntries:
	case SA_Parent:
	case SA_Draggable:
	case SA_Exclusive:
	case SA_SharePens:
	case SA_BackFill:
	case SA_Interleaved:
	case SA_Colors32:
	case SA_VideoControl:
	case SA_FrontChild:
	case SA_BackChild:
	case SA_LikeWorkbench:
	case SA_MinimizeISG:
	    /* TODO */
	    break;
	}
    }

    return OpenScreen (&ns);
    AROS_LIBFUNC_EXIT
} /* OpenScreenTagList */
