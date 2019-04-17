/*
    Copyright © 2014-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "mesa3dgl_types.h"

/*****************************************************************************

    NAME */

      GLAProc glAGetProcAddress(

/*  SYNOPSIS */
      const GLubyte * procname)

/*  FUNCTION

    INPUTS

    RESULT
      Pointer to procname function or NULL if function is not supported


    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    return _glapi_get_proc_address(procname);
}
