/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <proto/timer.h>

#include "lowlevel_intern.h"

#include <aros/libcall.h>
#include <exec/types.h>
#include <devices/timer.h>
#include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, ElapsedTime,

/*  SYNOPSIS */ 
      AROS_LHA(struct EClockVal *, context, A0),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 17, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS
        This function is unimplemented.

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

    struct Library *TimerBase = LowLevelBase->ll_TimerBase;
    struct timeval *tlast = (struct timeval *)context;
    struct timeval a, b;

    GetSysTime(&a);
    b = a;
    SubTime(&b, tlast);
    *tlast = a;

    return b.tv_secs*1000 + b.tv_micro/1000;

    AROS_LIBFUNC_EXIT
} /* ElapsedTime */
