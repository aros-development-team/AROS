/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function AttachPalExtra()
    Lang: english
*/

#include <proto/exec.h>
#include <graphics/view.h>

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
	pe->pe_RefCnt    = AllocMem(cm->Count, MEMF_CLEAR);
	pe->pe_AllocList = AllocMem(cm->Count, MEMF_ANY);

	if (NULL != pe->pe_RefCnt && NULL != pe->pe_AllocList)
	{
	    /* initialize the AllocList BYTE-array */
	    ULONG i = 0;
	    while (i < cm->Count)
	    {
                pe->pe_AllocList[i] = (BYTE)i-1;
                i++;
	    }

	    /* connect the PaletteExtra structure to the ColorMap */
	    cm ->PalExtra = pe;

	    /* initialize the Palette Extra structure */
	    InitSemaphore(&pe->pe_Semaphore);
	    pe->pe_ViewPort = vp;

	    pe->pe_FirstFree   = cm->Count-1;
	    pe->pe_NFree       = cm->Count;
	    pe->pe_FirstShared = (UWORD)-1;
	    pe->pe_NShared     = 0;

	    /* set all entries in the color table to be shareable
               pe_SharableColors is not the number of colors but the last color index! */

	    pe->pe_SharableColors = cm->Count-1;
	    
	} /* if (NULL != pe->pe_RefCnt && NULL != pe->pe_AllocList) */
	else
	{
	    /* some memory allocation failed */
	    if (pe->pe_RefCnt)
                FreeMem(pe->pe_RefCnt, cm->Count);
	    if (pe->pe_AllocList) 
                FreeMem(pe->pe_AllocList, cm->Count);
	    FreeMem(pe, sizeof(struct PaletteExtra));
	    return 1;
	}
	
    } /* if (NULL != pe) */
    else
	return 1;

    return 0;

    AROS_LIBFUNC_EXIT
  
} /* AttachPalExtra */
