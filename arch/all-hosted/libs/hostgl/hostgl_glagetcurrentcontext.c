/*
    Copyright 2011-2015, The AROS Development Team. All rights reserved.
*/

#include "hostgl_ctx_manager.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      GLAContext glAGetCurrentContext(

/*  SYNOPSIS */
      )

/*  FUNCTION
        Returns the currently selected GL rendering context.
 
    INPUTS
 
    RESULT
        The GL rendering context which is currently active.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    return HostGL_GetCurrentContext();
}

