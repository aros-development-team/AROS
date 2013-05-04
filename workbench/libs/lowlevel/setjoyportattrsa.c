/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/
#include "lowlevel_intern.h"

#include <libraries/lowlevel.h>

/*****************************************************************************

    NAME */

      AROS_LH2(BOOL, SetJoyPortAttrsA,

/*  SYNOPSIS */ 
      AROS_LHA(ULONG, portNumber, D0),
      AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 22, LowLevel)

/*  FUNCTION
 
    INPUTS
 
    RESULT
 
    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return FALSE;
    
    AROS_LIBFUNC_EXIT
    
} /* SetJoyPortAttrsA */
