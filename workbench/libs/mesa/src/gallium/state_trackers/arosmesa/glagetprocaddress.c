/*
    Copyright 2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_types.h"
#include <proto/exec.h>

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
    return glstapi->get_proc_address(glstapi, procname);
}
