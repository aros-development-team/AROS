/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <exec/memory.h>
#include <hidd/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <stdio.h>

#include "cybergraphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <clib/cybergraphics_protos.h>

	AROS_LH1(struct List *, AllocCModeListTagList,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 12, Cybergraphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    cybergraphics_lib.fd and clib/cybergraphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Implement Display mode attributes */

    const struct TagItem *tstate;
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
    
    while ((mode = NextDisplayInfo(mode)) != INVALID_ID) {	    
	struct CyberModeNode *cmnode;
	UWORD *cyberpixfmts;
	IPTR width, height, depth;
	OOP_Object *sync, *pf;
	struct VecInfo info;
	
	if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_VEC, mode) != sizeof(info)) {
	    /* This should never happen because NextDisplayInfo() should
	       only return valid modes
	    */
	    D(bug("!!! UNABLE TO GET HIDD MODE INFO IN AllocCModeListTagList() !!!\n"));
	    D(bug("!!! THIS SHOULD *NEVER* HAPPEN !!!\n"));
	    goto failexit;
	}

	sync = (OOP_Object *)info.reserved[0];
	pf   = (OOP_Object *)info.reserved[1];
	OOP_GetAttr(sync, aHidd_Sync_HDisp,  &width);
	OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
	
	if (	width < minwidth
	     || width > maxwidth
	     || height < minheight
	     || height > maxheight) {
	     
	     continue;
	}
	    
	/* Get the pxifmt info */
	OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    
	if (depth < mindepth || depth > maxdepth)
	    continue;
		
	/* Check whether the gfxmode is the correct pixel format */
	if (NULL != cmodelarray) {
	    HIDDT_StdPixFmt stdpf;
	    UWORD cyberpf;
	    BOOL found = FALSE;
	    
	    /* Get the gfxmode pixelf format */
	    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdpf);
		
	    cyberpf = hidd2cyber_pixfmt(stdpf);
	    if (cyberpf == (UWORD)-1)
		continue;	/* Unknown format */
			
	    for (cyberpixfmts = cmodelarray; *cyberpixfmts; cyberpixfmts ++) {
		/* See if the stdpixfmt is present in the array */
		if (*cyberpixfmts == cyberpf) {
		    found = TRUE;
		    break;
		}
	    } /* for (each supplied pixelformat in the cmodelarray) */
		    
	    if (!found)
		continue; /* PixFmt not wanted, just continue with next node */

	} /* if (cmodelarray supplied in the taglist) */
		
	/* Allocate a cybergfx modeinfo struct */
	cmnode = AllocMem(sizeof (struct CyberModeNode), MEMF_CLEAR);
	if (NULL == cmnode)
	    goto failexit;
		
	cmnode->Width	= width;
	cmnode->Height	= height;
	cmnode->Depth	= depth;
	cmnode->DisplayTagList = NULL;
	    
	snprintf( cmnode->ModeText
		, DISPLAYNAMELEN
		, "AROS: %ldx%ldx%ld"
		, width, height, depth
	);
		
	/* Keep track of the node */
	AddTail(cybermlist, (struct Node *)cmnode);
	
    } /* for (modeids returned from the HIDD) */

    return cybermlist;

failexit:
    if (NULL != cybermlist)
     	FreeCModeList(cybermlist);	

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AllocCModeListTagList */
