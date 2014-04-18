/*
    Copyright 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/exec.h>

GLAContext glAGetCurrentContext();

/*****************************************************************************

    NAME */

      AROS_LH0(APTR, AROSMesaGetCurrentContext,

/*  SYNOPSIS */ 

/*  LOCATION */
      struct Library *, MesaBase, 10, Mesa)

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
    AROS_LIBFUNC_INIT

    return glAGetCurrentContext();

    AROS_LIBFUNC_EXIT
}


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

