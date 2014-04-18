/*
    Copyright 2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "state_tracker/st_gl_api.h"
#include <proto/exec.h>

APTR GetOpenGLStateTrackerApi();

/*****************************************************************************

    NAME */

      AROS_LH0(APTR, AROSMesaGetOpenGLStateTrackerApi,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct Library *, MesaBase, 11, Mesa)

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
    AROS_LIBFUNC_INIT

    return GetOpenGLStateTrackerApi();

    AROS_LIBFUNC_EXIT
}

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
