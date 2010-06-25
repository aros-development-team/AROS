/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "state_tracker/st_public.h"
#include <proto/exec.h>

AROS_LH0(AROSMesaContext, AROSMesaGetCurrentContext,
    struct Library *, MesaBase, 10, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG
    
    PUT_MESABASE_IN_REG

    GET_CURRENT_CONTEXT(ctx);

    RESTORE_REG;
    
    return (AROSMesaContext)ctx;

    AROS_LIBFUNC_EXIT
}

