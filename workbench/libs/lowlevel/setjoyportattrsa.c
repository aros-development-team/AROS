/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: remvblankint.c 31639 2009-07-31 17:49:07Z stegerg $

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

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return FALSE;
    
    AROS_LIBFUNC_EXIT
    
} /* SetJoyPortAttrsA */
