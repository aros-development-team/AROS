/*
    Copyright 2011-2015, The AROS Development Team. All rights reserved.
*/

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
    /* This function cannot be implemented in HostGL as it is Gallium specific */
    return NULL;
}
