/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <aros/libcall.h>
#include <exec/types.h>
//include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH1(ULONG, ReadJoyPort,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, port, D0),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 10, LowLevel)

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

  AROS_LIBFUNC_EXIT
} /* ReadJoyPort */
