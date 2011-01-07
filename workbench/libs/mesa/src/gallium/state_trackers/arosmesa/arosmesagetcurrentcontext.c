/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      AROS_LH0(AROSMesaContext, AROSMesaGetCurrentContext,

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

    SAVE_REG
    
    PUT_MESABASE_IN_REG

    GET_CURRENT_CONTEXT(ctx);

    RESTORE_REG;
    
    return (AROSMesaContext)ctx;

    AROS_LIBFUNC_EXIT
}

