/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/memory.h>
#include <hidd/gfx.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <stdio.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH1(struct List *, AllocCModeListTagList,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 12, Cybergraphics)

/*  FUNCTION
        Retrieves a list of RTG screenmodes that match the criteria specified
        in a taglist. The supported tags are as follows:
            CYBRMREQ_MinWidth (ULONG) - the minimum acceptable display width
                (defaults to 320).
            CYBRMREQ_MaxWidth (ULONG) - the maximum acceptable display width.
                (defaults to 1600).
            CYBRMREQ_MinHeight (ULONG) - the minimum acceptable display
                height (defaults to 240).
            CYBRMREQ_MaxHeight (ULONG) - the maximum acceptable display
                height (defaults to 1200).
            CYBRMREQ_MinDepth (UWORD) - the minimum acceptable display depth
                (defaults to 8).
            CYBRMREQ_MaxDepth (UWORD) - the minimum acceptable display depth
                (defaults to 32).
            CYBRMREQ_CModelArray (UWORD *) - array of permitted pixel formats.
                Any of the PIXFMT_#? constants may be specified (see
                LockBitMapTagList()), and the array must be terminated by ~0.
                By default, all pixel formats are acceptable.

    INPUTS
        tags - mode selection criteria (may be NULL).

    RESULT
        result - a list of matching screenmodes, or NULL if there are none.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        FreeCModeList()

    INTERNALS
        The function relies on pixelformat object being passed in
        DimensionInfo.reserved[1] by graphics.library/GetDisplayInfoData().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Implement Display mode attributes */

    struct TagItem *tstate;
    struct TagItem *tag;

    ULONG minwidth = 320;
    ULONG maxwidth = 1600;
    ULONG minheight = 240;
    ULONG maxheight = 1200;
    ULONG mindepth = 8;
    ULONG maxdepth = 32;
    
    struct List *cybermlist = NULL;

    UWORD *cmodelarray = NULL;
    ULONG mode = INVALID_ID;

    for (tstate = tags; (tag = NextTagItem(&tstate)); ) {
	switch (tag->ti_Tag) {
	    case CYBRMREQ_MinWidth:
	    	minwidth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MaxWidth:
	     	maxwidth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MinHeight:
	    	minheight = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MaxHeight:
	    	maxheight = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MinDepth:
	    	mindepth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_MaxDepth:
	    	maxdepth = (ULONG)tag->ti_Data;
		break;
		
	    case CYBRMREQ_CModelArray:
	    	cmodelarray = (UWORD *)tag->ti_Data;
		break;
		
	    default:
	    	D(bug("!!! UNKNOWN TAG PASSED TO AllocCModeListTagList\n"));
		break;
	} 	
    }

    /* Allocate the exec list */
    cybermlist = AllocMem(sizeof (struct List), MEMF_CLEAR);
    if (NULL == cybermlist)
    	return NULL;

    NEWLIST(cybermlist);

    while ((mode = NextDisplayInfo(mode)) != INVALID_ID)
    {
	struct CyberModeNode *cmnode;
	UWORD *cyberpixfmts;
	ULONG width, height;
	struct DimensionInfo info;
	struct NameInfo name;
	IPTR cyberpf;
	OOP_Object *pf;

	D(bug("[cgfx.library] %s: trying %08x\n", __func__, mode);)

	if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_DIMS, mode) != sizeof(info)) {
	    /* This should never happen because NextDisplayInfo() should
	       only return valid modes
	    */
	    bug("!!! UNABLE TO GET MODE INFO IN AllocCModeListTagList() !!!\n");
	    bug("!!! THIS SHOULD *NEVER* HAPPEN !!!\n");
	    goto failexit;
	}
	
	if (GetDisplayInfoData(NULL, (UBYTE *)&name, sizeof(name), DTAG_NAME, mode) != sizeof(name)) {
	    bug("!!! UNABLE TO GET MODE NAME IN AllocCModeListTagList() !!!\n");
	    bug("!!! THIS SHOULD *NEVER* HAPPEN !!!\n");
	    goto failexit;
	}

	width  = info.Nominal.MaxX - info.Nominal.MinX + 1;
	height = info.Nominal.MaxY - info.Nominal.MinY + 1;

	D(
		bug("[cgfx.library] %s:      width %u\n", __func__, width);
		bug("[cgfx.library] %s:      height %u\n", __func__, height);
		bug("[cgfx.library] %s:      depth %u\n", __func__, info.MaxDepth);
	)
	
	if (	width < minwidth
	     || width > maxwidth
	     || height < minheight
	     || height > maxheight) {
	     
	     continue;
	}   
	if (info.MaxDepth < mindepth || info.MaxDepth > maxdepth)
	    continue;

	/* Get the gfxmode pixelf format */
	pf = (OOP_Object *)info.reserved[1];
	OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &cyberpf);
	/* ignore non-CGX modes */
	if (cyberpf == -1)
	    continue;

	D(bug("[cgfx.library] %s:   checking pixfmt...\n", __func__);)

	/* Check whether the gfxmode is the correct pixel format */
	if (NULL != cmodelarray)
        {
	    BOOL found = FALSE;

	    for (cyberpixfmts = cmodelarray; *cyberpixfmts != (UWORD)~0; cyberpixfmts ++) {
		/* See if the stdpixfmt is present in the array */
		D(bug("[cgfx.library] %s:   pixfmt %04x, cyberpf %04x\n", __func__, *cyberpixfmts, (UWORD)cyberpf);)

		if (*cyberpixfmts == (UWORD)cyberpf) {
		    found = TRUE;
		    break;
		}
	    } /* for (each supplied pixelformat in the cmodelarray) */

	    if (!found)
		continue; /* PixFmt not wanted, just continue with next node */

	} /* if (cmodelarray supplied in the taglist) */

	D(bug("[cgfx.library] %s:  allocating modeinfo...\n", __func__);)

	/* Allocate a cybergfx modeinfo struct */
	cmnode = AllocMem(sizeof (struct CyberModeNode), MEMF_CLEAR);
	if (NULL == cmnode)
	    goto failexit;

	D(bug("[cgfx.library] %s:  modeinfo @ 0x%p\n", __func__, cmnode);)

	cmnode->Width	= width;
	cmnode->Height	= height;
	cmnode->Depth	= info.MaxDepth;
	cmnode->DisplayID = mode;
	cmnode->DisplayTagList = NULL;
	CopyMem("CGFX:", cmnode->ModeText, 5);
	CopyMem(name.Name, cmnode->ModeText + 5, DISPLAYNAMELEN - 5);

	/* Keep track of the node */
	AddTail(cybermlist, (struct Node *)cmnode);
	
    } /* for (modeids returned from the HIDD) */

    D(bug("[cgfx.library] %s:  returning list @ 0x%p\n", __func__, cybermlist);)

    return cybermlist;

failexit:
    D(bug("[cgfx.library] %s: failing...\n", __func__);)

    if (NULL != cybermlist)
     	FreeCModeList(cybermlist);	

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AllocCModeListTagList */
