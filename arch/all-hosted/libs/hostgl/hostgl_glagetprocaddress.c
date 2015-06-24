#include "hostgl_types.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      GLAProc glAGetProcAddress(

/*  SYNOPSIS */
      const GLubyte * procname)

/*  FUNCTION
        validate if the requested function is actually supported by
        current configuration of 3D subsystem. 
 
    INPUTS
 
    RESULT
        The return value is defined as APTR. If it is not NULL, the requested
        GL function is supported by current configuration of 3D subsystem.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    return GLXCALL(glXGetProcAddress, procname);
}
