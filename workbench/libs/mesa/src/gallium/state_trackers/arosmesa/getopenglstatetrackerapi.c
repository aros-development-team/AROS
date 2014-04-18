/*
    Copyright 2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "state_tracker/st_gl_api.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      APTR GetOpenGLStateTrackerApi(

/*  SYNOPSIS */
      )

/*  FUNCTION
        This is a PRIVATE function used by egl.library to receive pointer to
        api structure of OpenGL. Do not use this function in your application.

    INPUTS

    RESULT

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    return (APTR)st_gl_api_create();
}
