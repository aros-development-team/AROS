/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AttachPalExtra()
    Lang: english
*/

#include <proto/exec.h>
#include <graphics/view.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(LONG, AttachPalExtra,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm, A0),
	AROS_LHA(struct ViewPort *, vp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 139, Graphics)

/*  FUNCTION
        Allocates a PalExtra structure and attaches it to the
        given ColorMap. This function must be called prior to palette
        sharing. The PalExtra structure will be freed bt FreeColorMap().

    INPUTS
        cm  - Pointer to a color map structure
        vp  - Pointer to the viewport associated with the ColorMap
  
    RESULT
        0 - success
        1 - out of memory

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct PaletteExtra * pe;

    if (NULL != cm->PalExtra)
        return 0;

    pe = AllocMem(sizeof(struct PaletteExtra), MEMF_CLEAR|MEMF_PUBLIC);
    if (NULL != pe)
    {
	/* 
	** if you change the number of byte allocated here then you
	** must also make chnages to FreeColorMap()!
	*/
	pe->pe_RefCnt    = AllocMem(cm->Count * sizeof(PalExtra_RefCnt_Type), MEMF_CLEAR);
	pe->pe_AllocList = AllocMem(cm->Count * sizeof(PalExtra_AllocList_Type), MEMF_ANY);

	if (NULL != pe->pe_RefCnt && NULL != pe->pe_AllocList)
	{
	    UWORD sharablecolors, bmdepth;
	    
	    sharablecolors = cm->Count;

    	    /* cm->Count may contain more entries than 2 ^ bitmapdepth,
	       for pointer sprite colors, etc. Sharablecolors OTOH is
	       limited to 2 ^ bitmapdepth */
	         
	    bmdepth = GetBitMapAttr(vp->RasInfo->BitMap, BMA_DEPTH);
	    if (bmdepth < 8)
	    {
	    	if ((1L << bmdepth) < sharablecolors)
		{
		    sharablecolors = 1L << bmdepth;
		}
	    }
	    	    
	    /* initialize the AllocList BYTE-array */
	    ULONG i = 0;

	    /* CHECKME: Should probably say "i < sharablecolors", but might
	       not actually cause anything bad either, even if it doesn't. */	       
    	    while (i < cm->Count) 
	    {
                PALEXTRA_ALLOCLIST(pe, i) = (PalExtra_AllocList_Type)i-1;
                i++;
	    }

	    /* connect the PaletteExtra structure to the ColorMap */
	    cm ->PalExtra = pe;

	    /* initialize the Palette Extra structure */
	    InitSemaphore(&pe->pe_Semaphore);
	    pe->pe_ViewPort = vp;

	    pe->pe_FirstFree   = sharablecolors-1;
	    pe->pe_NFree       = sharablecolors;
	    pe->pe_FirstShared = (UWORD)-1;
	    pe->pe_NShared     = 0;

	    /* set all entries in the color table to be shareable
               pe_SharableColors is not the number of colors but the last color index! */

	    pe->pe_SharableColors = sharablecolors;
	    
	} /* if (NULL != pe->pe_RefCnt && NULL != pe->pe_AllocList) */
	else
	{
	    /* some memory allocation failed */
	    if (pe->pe_RefCnt)
                FreeMem(pe->pe_RefCnt, cm->Count * sizeof(PalExtra_RefCnt_Type));
	    if (pe->pe_AllocList) 
                FreeMem(pe->pe_AllocList, cm->Count* sizeof(PalExtra_AllocList_Type));
	    FreeMem(pe, sizeof(struct PaletteExtra));
	    return 1;
	}
	
    } /* if (NULL != pe) */
    else
	return 1;

    return 0;

    AROS_LIBFUNC_EXIT
  
} /* AttachPalExtra */
