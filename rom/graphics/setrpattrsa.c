/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function SetRPAttrsA()
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/utility.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
#include <utility/tagitem.h>
#include <proto/graphics.h>

	AROS_LH2(void, SetRPAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct TagItem  *, tags, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 173, Graphics)

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
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct TagItem * tag, *tstate = tags;

    while ((tag = NextTagItem ((const struct TagItem **)&tstate)))
    {
	switch (tag->ti_Tag)
	{
	    case RPTAG_Font:
		SetFont (rp, (struct TextFont *)(tag->ti_Data));
		break;

	    case RPTAG_APen:
		SetAPen (rp, tag->ti_Data);
		break;

	    case RPTAG_BPen:
		SetBPen (rp, tag->ti_Data);
		break;

	    case RPTAG_DrMd:
		SetDrMd (rp, tag->ti_Data);
		break;

	    case RPTAG_OutlinePen:
		SetOutlinePen (rp, tag->ti_Data);
		break;

	    case RPTAG_WriteMask:
		SetWriteMask (rp, tag->ti_Data);
		break;

	    case RPTAG_MaxPen:
		break;

	    case RPTAG_DrawBounds:
		break;
	    
	} /* switch (tag) */
	
    } /* while (tag) */

    AROS_LIBFUNC_EXIT
} /* SetRPAttrsA */
