/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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

      AROS_LH3(VOID, StartTimerInt,

/*  SYNOPSIS */ 
      AROS_LHA(APTR , intHandle, A1),
      AROS_LHA(ULONG, timeInterval, D0),
      AROS_LHA(BOOL , continuous, D1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 21, LowLevel)

/*  NAME
 
    FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct LowLevelBase *, LowLevelBase)

#warning TODO: Write lowlevel/StartTimerInt()
    aros_print_not_implemented ("lowlevel/StartTimerInt");

    return;

  AROS_LIBFUNC_EXIT
} /* StartTimerInt */
