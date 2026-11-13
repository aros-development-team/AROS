#include "hostgl_types.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      GLAProc glAGetProcAddress(

/*  SYNOPSIS */
      const GLubyte * procname)

/*  FUNCTION
        Look up an OpenGL entry point in the host GLX implementation.
 
    INPUTS
        procname - NUL-terminated name of the OpenGL entry point.
 
    RESULT
        Function pointer returned by glXGetProcAddress(), or NULL when
        no entry point is available.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    return GLXCALL(glXGetProcAddress, procname);
}
