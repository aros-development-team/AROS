/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release a pen previously allocated.
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/view.h>
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

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
        An exclusive pen is deallocated for other appilcations to use.
        A shared pen is only completely deallocated if no other
        application is not using it anymore.

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

  if (NULL != cm && n < cm->Count)
  {
    struct PaletteExtra * pe = cm->PalExtra;
    ULONG index;

    ObtainSemaphore(&pe->pe_Semaphore);
    /* First I check whether this pen is somewhere in the
       free list already...
    */
    index = pe->pe_FirstFree;
    while (-1 != (BYTE)index)
    {
      if (index == n)
        goto exit;
      index = pe->pe_AllocList[index];
    }
    
    /*
    ** It is not in the free list.
    ** If it is a shared pen, then I can recognize this 
    ** by its value in the RefCnt
    */
    
    if (0 != pe->pe_RefCnt[n])
    {
      /* 
      ** A SHARED pen
      */
      pe->pe_RefCnt[n]--;
      if (0 == pe->pe_RefCnt[n])
      {
        BOOL found = FALSE;
        /* 
        ** I can take this out if the list of shared pens
        ** since this was the last application that used
        ** this pen.
        */
        index = pe->pe_FirstShared;
        if (n == index)
        {
          found = TRUE;
          /*
          ** it's the very first one.
          */
          /* 
          ** Take it out of the list of entries in
          ** the shared list...
          */
          if ((UBYTE)-1 == pe->pe_AllocList[n])
            pe->pe_FirstShared = (WORD)-1;
          else
            pe->pe_FirstShared = (WORD)pe->pe_AllocList[n];
          
          pe->pe_NShared--;
          
          /*
          ** ... and make it available in the list of free
          ** entries.
          */
          pe->pe_AllocList[n] = (UBYTE)pe->pe_FirstFree;
          pe->pe_FirstFree = n;
          pe->pe_NFree++;
        }
        else
        {
          do
          {
            if ((UBYTE)n == pe->pe_AllocList[index])
            {
              found = TRUE;
              
              /*
              ** Take it out of the list of shared entries
              */
              pe->pe_AllocList[index] = pe->pe_AllocList[n];
              pe->pe_NShared--;
              
              /*
              ** ... and make it available in the list of free
              ** entries.
              */
              pe->pe_AllocList[n] = (UBYTE)pe->pe_FirstFree;
              pe->pe_FirstFree = n;
              pe->pe_NFree++;
              break;
            }
            else
              index = pe->pe_AllocList[index];
          }
          while (-1 != (BYTE)index);
        }
        
#if DEBUG
        if (FALSE==found)
          D(bug("Error in RealsePen() pen = %d!\n",n));
#endif

      } /* if (no further app needs this pen) */
    }
    else
    {
      /* releasing an EXCLUSIVE pen */
      D(bug("Releasing (exclusive) pen %d\n"));
      pe->pe_AllocList[n] = pe->pe_FirstFree;
      pe->pe_FirstFree    = n;
      pe->pe_NFree++;
    }
exit:
    ReleaseSemaphore(&pe->pe_Semaphore);  
  
  }

  AROS_LIBFUNC_EXIT
} /* ReleasePen */
