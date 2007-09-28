/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain a certain pen
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/view.h>

/*****************************************************************************

    NAME */
	#include <proto/graphics.h>

	AROS_LH6(LONG, ObtainPen,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm   , A0),
	AROS_LHA(ULONG            , n    , D0),
	AROS_LHA(ULONG            , r    , D1),
	AROS_LHA(ULONG            , g    , D2),
	AROS_LHA(ULONG            , b    , D3),
	AROS_LHA(ULONG            , flags, D4),

/*  LOCATION */
	struct GfxBase *, GfxBase, 159, Graphics)

/*  FUNCTION
        Attempt to allocate an entry in the colormap for exclusive
        or shared use by the application. To deallocate the pen
        ReleasePen() must be called.
        
    INPUTS
        cm    - A pointer to a color map structure
        n     - index of the entry in the color map; if any entry is fine
                pass -1
        r     - red value (left justified 32 bit fraction)
        g     - green value (left justified 32 bit fraction)
        b     - blue value (left justified 32 bit fraction)
        flags - PEN_EXCLUSIVE - for exclusive access to a color register;
                              default is shared access
                              
                PEN_NO_SETCOLOR - will not change the RGB values
                                  for the selected pen.

    RESULT
        n  = allocated pen number, -1 for failure

    NOTES
        Shared palette entries should not be changed (via SetRGB??())
        since other applications might use the same color.
        A PaletteExtra structure must have been attached to the 
        ColorMap prior to calling this function (AttachPalExtra()). 

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG retval = -1;
    PalExtra_AllocList_Type index;
    BOOL was_shared = FALSE;
    /*
       Change the calculation of the color if the entries
       in the colortable attached to the colormap structure
       are changed
    */
    struct PaletteExtra * pe = cm->PalExtra;

    ObtainSemaphore(&pe->pe_Semaphore);

    /* pe_SharableColors is not the number of colors but the last color index! */

    if (NULL != pe && (n <= pe->pe_SharableColors || -1 == n ))
    {
	if (-1 != n)
	{
	    /* A specific color map entry is requested */
	    /* Does the user want shared or exclusive access ? */
	    
	    if (0 != (flags & PENF_EXCLUSIVE))
	    {
        	/* EXCLUSIVE ACCESS to pen */
        	if (pe->pe_FirstFree == n)
        	{
        	    /* it is the very first one of available pens */
        	    retval = n;

        	    pe->pe_NFree--;

        	    if ((PalExtra_AllocList_Type)-1 == PALEXTRA_ALLOCLIST(pe, n))
        		pe->pe_FirstFree = (UWORD)-1;
        	    else
        		pe->pe_FirstFree = PALEXTRA_ALLOCLIST(pe, n);

        	    /* mark that entry as used */
        	    PALEXTRA_ALLOCLIST(pe, n) = (PalExtra_AllocList_Type)-1;

        	} /* if (pe->pe_FirstFree == n) */
        	else
        	{
        	    /*
        	       walk through the list of free entries and see whether 
        	       the requested one is still available. 
        	    */
        	    index = (PalExtra_AllocList_Type)pe->pe_FirstFree;

        	    while ((PalExtra_AllocList_Type)-1 != index)
        	    {
        		if (n == PALEXTRA_ALLOCLIST(pe, index))
        		{
        		    /* it's still free! So I allocate it */
        		    retval = n;
        		    PALEXTRA_ALLOCLIST(pe, index) = PALEXTRA_ALLOCLIST(pe, n);
        		    PALEXTRA_ALLOCLIST(pe, n) = (PalExtra_AllocList_Type)-1;
        		    pe->pe_NFree--;
        		    break;
        		}
        		else
        		    index = PALEXTRA_ALLOCLIST(pe, index);

        	    } /* while */
		  
        	} /* (pe->pe_FirstFree != n) */
		
	    } /* if (EXCLUSIVE access) */
	    else
	    {
        	/* SHARED ACCESS to pen */
        	/*
        	   the pen could already be shared or it can still be in
        	   the free list.
        	   I recognize that a pen is already shared by its entry
        	   in pe_RefCnt being != 0.
        	*/
        	if (PALEXTRA_REFCNT(pe, n) != 0)
        	{
        	    /* 
        	       this one is already in shared mode, so test
        	       whether the color is the same.
        	       ??? Is this necessary for a shared pen?
        	    */
        	    if (color_equal(cm,r,g,b,n))
        	    {
        		/* increase the RefCnt */
        		PALEXTRA_REFCNT(pe, n)++;
			was_shared = TRUE;
        		retval = n;
        	    }
        	} /* if PALEXTRA_REFCNT(pe, n) != 0) */
        	else
        	{
       	    	    /* 
        	    ** The RefCnt is 0, so the pen is probably still in the
        	    ** free list unless it is an exclusive pen.
        	    */
        	    if (pe->pe_FirstFree == n)
        	    {
        		/* it is the very first one of available pens */
        		retval = n;

        		if ((PalExtra_AllocList_Type)-1 == PALEXTRA_ALLOCLIST(pe, n))
        		    pe->pe_FirstFree = (UWORD)-1;
        		else
        		    pe->pe_FirstFree = (UWORD)PALEXTRA_ALLOCLIST(pe, n);
        		/* mark that entry as shared */

        	    } /* if (pe->pe_FirstFree == n) */
        	    else
        	    {
        		/*
        		   walk through the list of free entries and see whether 
        		   the requested one is still available 
        		*/
        		index = (PalExtra_AllocList_Type)pe->pe_FirstFree;

        		while ((PalExtra_AllocList_Type)-1 != index)
        		{
        		    if ((PalExtra_AllocList_Type)n == PALEXTRA_ALLOCLIST(pe, index))
        		    {
                		/* it's still free! So I allocate it */
                		retval = n;
                		PALEXTRA_ALLOCLIST(pe, index) = PALEXTRA_ALLOCLIST(pe, n);

                		break;
        		    }
        		    else
                	        index = PALEXTRA_ALLOCLIST(pe, index);
			    
        		} /* while */
			
        	    } /* (pe->pe_FirstFree != n) */

        	    if (-1 != retval)
        	    {
        		PALEXTRA_ALLOCLIST(pe, n) = (PalExtra_AllocList_Type)pe->pe_FirstShared;
        		pe->pe_FirstShared = n;
        		pe->pe_NFree--;
        		pe->pe_NShared++;
        		PALEXTRA_REFCNT(pe, n) = 1;
        	    } 
		  
        	} /* (PALEXTRA_REFCNT(pe, n) == 0) */
	      
	    } /* shared access */
	  
	} /* n != -1 */
	else
	{
	    /* Any entry in the color table is fine */

	    /* Does the user want shared or exclusive access ? */
	    if (0 != (flags & PENF_EXCLUSIVE))
	    {
        	/* EXCLUSIVE ACCESS to pen */
        	/*
        	** Search for the very first entry that I can
        	** give exclusive access to, if there are still
        	** entries free
        	*/
        	if (0 != pe->pe_NFree)
        	{
        	    retval = pe->pe_FirstFree;
        	    pe->pe_NFree--;
        	    if (0 == pe->pe_NFree)
        	        pe->pe_FirstFree = (UWORD)-1;
        	    else
        	        pe->pe_FirstFree = (UWORD)PALEXTRA_ALLOCLIST(pe, retval);
        	    PALEXTRA_ALLOCLIST(pe, retval) = (PalExtra_AllocList_Type)-1;
        	    PALEXTRA_REFCNT(pe, retval) = 0;
        	}
		
	    } /* if (0 != (flags & PENF_EXCLUSIVE)) */
	    else
	    {
        	/* SHARED ACCESS */
        	/*
        	** Search for the very first entry that I can give
        	** shared access to. First search the list of shared
        	** colors and look for matching colors and if nothing can
        	** be found there then take an entry out of the
        	** free list.
        	*/
        	index = (PalExtra_AllocList_Type)pe->pe_FirstShared;
        	while ((PalExtra_AllocList_Type)-1 != index)
        	{
        	    if (color_equal(cm,r,g,b,index))
        	    {
        		/* That's a good one */
        		retval = index;
        		PALEXTRA_REFCNT(pe, retval)++;
			was_shared = TRUE;
        		break;
        	    }
        	    index = PALEXTRA_ALLOCLIST(pe, index);
        	}

        	/* 
        	** If nothing was found take an entry from the free list 
        	*/
        	if (-1 == retval && 0 != pe->pe_NFree)
        	{
        	    retval = pe->pe_FirstFree;
        	    pe->pe_NFree--;
        	    if (0 == pe->pe_NFree)
        		pe->pe_FirstFree = (UWORD)-1;
        	    else
        		pe->pe_FirstFree = (UWORD)PALEXTRA_ALLOCLIST(pe, retval);

        	    PALEXTRA_ALLOCLIST(pe, retval) = (PalExtra_AllocList_Type)pe->pe_FirstShared;
        	    pe->pe_FirstShared = retval;
        	    PALEXTRA_REFCNT(pe, retval) = 1;
		    pe->pe_NShared++;
        	}
	      
	    } /* shared access */ 
	       
	} /* n = -1 */
	
    } /* if (NULL != pe && (n <= pe->pe_SharableColors || -1 == n )) */

    if (-1 != retval && 0 == (flags & PENF_NO_SETCOLOR) && !was_shared)
    {
        /* Change the rgb values for the selected pen */
	
	if (pe->pe_ViewPort)
	{
	    SetRGB32(pe->pe_ViewPort, retval, r, g, b);
	} else {
            SetRGB32CM(cm, retval, r, g, b);
	}
    }

    ReleaseSemaphore(&pe->pe_Semaphore);

    return retval;

    AROS_LIBFUNC_EXIT
} /* ObtainPen */
