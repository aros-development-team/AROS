/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/
#include "lowlevel_intern.h"

#include <aros/libcall.h>
#include <exec/types.h>
#include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH1(VOID, RemKBInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, intHandle, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 11, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS
        This function is unimplemented.

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    /* TODO: Write lowlevel/RemKBInt() */
    aros_print_not_implemented ("lowlevel/RemKBInt");

    if (intHandle)
    {
    }

    return;

  AROS_LIBFUNC_EXIT
} /* RemKBInt */
