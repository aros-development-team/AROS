/*
    Copyright 2009-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include "mesa3dgl_types.h"

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
    GET_CURRENT_CONTEXT(ctx);

    return (GLAContext)ctx;
}

