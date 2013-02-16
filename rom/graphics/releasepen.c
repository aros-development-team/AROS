/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release a pen previously allocated.
    Lang: english
*/
#include <aros/debug.h>

#include "graphics_intern.h"
#include <graphics/view.h>

/*****************************************************************************

    NAME */
	#include <proto/graphics.h>

	AROS_LH2(void, ReleasePen,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm, A0),
	AROS_LHA(ULONG            , n , D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 158, Graphics)

/*  FUNCTION
        Release a pen that was previously allocated as an exclusive
        or shared pen by the application. Any other application can
        then obtain this pen and make changes to the color register
        entries.


    INPUTS
        cm - ColorMap structure where the pen was allocated
        n  - The number of the pen

    RESULT
        An exclusive pen is deallocated for other applications to use.
        A shared pen is only completely deallocated if no other
        application is using it anymore.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (NULL != cm && n < cm->Count)
    {
	struct PaletteExtra * pe = cm->PalExtra;
	PalExtra_AllocList_Type index;

	ObtainSemaphore(&pe->pe_Semaphore);
	/* First I check whether this pen is somewhere in the
	   free list already...
	*/
	
	index = pe->pe_FirstFree;
	while ((PalExtra_AllocList_Type)-1 != index)
	{
	    if (index == (PalExtra_AllocList_Type)n)
            	goto exit;
		
	    index = PALEXTRA_ALLOCLIST(pe, index);
	}

	/*
	** It is not in the free list.
	** If it is a shared pen, then I can recognize this 
	** by its value in the RefCnt
	*/

	if (0 != PALEXTRA_REFCNT(pe,n))
	{
	    /* 
	    ** A SHARED pen
	    */
	    PALEXTRA_REFCNT(pe, n)--;
	    if (0 == PALEXTRA_REFCNT(pe, n))
	    {
        	D(BOOL found = FALSE);
        	/* 
        	** I can take this out if the list of shared pens
        	** since this was the last application that used
        	** this pen.
        	*/
        	index = pe->pe_FirstShared;
        	if ((PalExtra_AllocList_Type)n == index)
        	{
        	    D(found = TRUE);
        	    /*
        	    ** it's the very first one.
        	    */
        	    /* 
        	    ** Take it out of the list of entries in
        	    ** the shared list...
        	    */
        	    if ((PalExtra_AllocList_Type)-1 == PALEXTRA_ALLOCLIST(pe,n))
        		pe->pe_FirstShared = (WORD)-1;
        	    else
        		pe->pe_FirstShared = (WORD)PALEXTRA_ALLOCLIST(pe,n);

        	    pe->pe_NShared--;

        	    /*
        	    ** ... and make it available in the list of free
        	    ** entries.
        	    */
        	    PALEXTRA_ALLOCLIST(pe,n) = (PalExtra_AllocList_Type)pe->pe_FirstFree;
        	    pe->pe_FirstFree = n;
        	    pe->pe_NFree++;
        	}
        	else
        	{
        	    do
        	    {
        		if ((PalExtra_AllocList_Type)n == PALEXTRA_ALLOCLIST(pe, index))
        		{
        		    D(found = TRUE);

        		    /*
        		    ** Take it out of the list of shared entries
        		    */
        		    PALEXTRA_ALLOCLIST(pe, index) = PALEXTRA_ALLOCLIST(pe, n);
        		    pe->pe_NShared--;

        		    /*
        		    ** ... and make it available in the list of free
        		    ** entries.
        		    */
        		    PALEXTRA_ALLOCLIST(pe, n) = (PalExtra_AllocList_Type)pe->pe_FirstFree;
        		    pe->pe_FirstFree = n;
        		    pe->pe_NFree++;
        		    break;
        		}
        		else
        		    index = PALEXTRA_ALLOCLIST(pe, index);
        	    }
        	    while ((PalExtra_AllocList_Type)-1 != index);
        	}

	    #if DEBUG
        	if (!found)
        	    D(bug("Error in ReleasePen() pen = %d!\n",n));
	    #endif

	    } /* if (no further app needs this pen) */
	  
	} /* if (0 != PALEXTRA_REFCNT(pe,n)) */
	else
	{
	    /* releasing an EXCLUSIVE pen */
	    D(bug("Releasing (exclusive) pen %d\n"));
	    
	    PALEXTRA_ALLOCLIST(pe, n) = pe->pe_FirstFree;
	    pe->pe_FirstFree = n;
	    pe->pe_NFree++;
	}
  exit:
	ReleaseSemaphore(&pe->pe_Semaphore);  

    } /* if (NULL != cm && n < cm->Count) */

    AROS_LIBFUNC_EXIT
    
} /* ReleasePen */
