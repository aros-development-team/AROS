/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/types.h>
//include <libraries/realtime.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, LockRealTime,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, lockType, D0),

/*  LOCATION */
      struct RealtimeBase *, RealtimeBase, 10, Realtime)

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

  return 0;

  AROS_LIBFUNC_EXIT
} /* LockRealTime */
