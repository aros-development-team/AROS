/*
    Copyright (C) 2009-2019, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

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
    D(bug("[MESA3DGL] %s()\n", __func__));

    /* The state tracker context current in this task names the GLAContext it
     * belongs to (glACreateContext sets frontend_context). Mesa's own current
     * context is another object and is not what glAMakeCurrent() takes. */
    struct st_context *current = st_api_get_current();

    return current ? (GLAContext)current->frontend_context : NULL;
}

