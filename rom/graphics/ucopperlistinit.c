/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function UCopperListInit()
    Lang: english
*/
#include <graphics/copper.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(struct CopList *, UCopperListInit,

/*  SYNOPSIS */
        AROS_LHA(struct UCopList *, ucl, A0),
        AROS_LHA(WORD             , n  , D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 99, Graphics)

/*  FUNCTION
        Allocates and initializes copperlist structures and buffers
        internal to UCopList structure.

    INPUTS
        ucl - pointer to a UCopList structure. Must not be NULL!
        n   - number of instructions the buffer must be able to hold

    RESULT
        cl - pointer to a buffer that will accept n intermediate
             copper instructions

        NOTE: this is a pointer to UCopList->FirstCopList!

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CINIT CMOVE CWAIT CEND graphics/copper.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  /* has this structure previously been initialized? */

  if (ucl->FirstCopList != NULL &&
      ucl->FirstCopList->MaxCount != 0 &&
      ucl->FirstCopList->CopIns != NULL )
  {
    ucl->FirstCopList->Count  = ucl->FirstCopList->MaxCount;
    ucl->FirstCopList->CopPtr = ucl->FirstCopList->CopIns;
   return ucl->FirstCopList;
  }


  if (NULL != (ucl->FirstCopList =
                 (struct CopList *)AllocMem(sizeof(struct CopList),
                                            MEMF_CLEAR|MEMF_PUBLIC)))
  {
  /* if we were successful with the memory allocation then let's get
   * the buffer for the instructions
   */
    ucl->CopList = ucl->FirstCopList;
    /* further init the coplist structure */
    ucl->FirstCopList->MaxCount = n;

    ucl->FirstCopList->CopIns = (struct CopIns *)AllocMem(n*sizeof(struct CopIns), MEMF_CLEAR|MEMF_PUBLIC);
    ucl->FirstCopList->CopPtr = ucl->FirstCopList->CopIns;

    /* did we get the memory? */
    if (NULL == ucl->FirstCopList->CopIns)
      return (NULL);
  }
  return (ucl->FirstCopList);
  AROS_LIBFUNC_EXIT
} /* UCopperListInit */
