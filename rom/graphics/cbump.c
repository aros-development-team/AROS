/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CBump()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/copper.h>
#include "graphics_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, CBump,

/*  SYNOPSIS */
	AROS_LHA(struct UCopList *, ucl, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 61, Graphics)

/*  FUNCTION
	Increment user copper list pointer. If the current user copper list
	is full a new one will be created and worked on.

    INPUTS
	ucl - pointer to a UCopList structure

    RESULT

    NOTES
	CWAIT() and CMOVE() automatically call this function!

    EXAMPLE

    BUGS

    SEE ALSO
	CINIT CWAIT CMOVE CEND graphics/copper.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)


#define NewCopListSize 10


  /* increment the instruction counter */
  ucl->CopList->Count++;

  /* is the current CopList full? */
  if (ucl->CopList->MaxCount == ucl->CopList->Count)
  {
    struct CopList * NextCopList;

    /* switch to the next CopList in the list, if it exists,
       otherwise alloc some memory for it  */
    if (NULL != ucl->CopList->Next)
      NextCopList = ucl->CopList->Next;
    else
    {
      NextCopList = (struct CopList *)AllocMem(sizeof(struct CopList), MEMF_CLEAR|MEMF_PUBLIC);
      if (NULL != NextCopList)
      {
        ucl->CopList->Next = NextCopList;
        /* this new one should hold 10 instructions */
        if (NULL==( NextCopList->CopIns =
                      AllocMem(NewCopListSize*sizeof(struct CopIns),
                               MEMF_CLEAR|MEMF_PUBLIC)))
          return; /* couldn't get memory */

        NextCopList->CopPtr = NextCopList->CopIns;
        NextCopList->MaxCount = NewCopListSize;
      }
      else /* couldn't get memory */
        return;
    }

    /* move the very last instruction from the old buffer to the new one... */
    NextCopList->CopPtr->OpCode = ucl->CopList->CopPtr->OpCode;
    NextCopList->CopPtr->u3.nxtlist = ucl->CopList->CopPtr->u3.nxtlist;

    /*... and leave a concatenation OpCode at that place */
    ucl->CopList->CopPtr->OpCode = CPRNXTBUF;
    ucl->CopList->CopPtr->u3.nxtlist = NextCopList;

    /* don't forget to increment the pointer and counter in the new list */
    NextCopList->CopPtr++;
    NextCopList->Count++;

    /* leave a pointer to the new list in the UCopList */
    ucl->CopList = NextCopList;
  }
  else /* current CopList is not full */
  {
    /* increment the pointer for the next instruction */
    ucl->CopList->CopPtr++;
  }

#undef NewCopListSize

  AROS_LIBFUNC_EXIT
} /* CBump */
