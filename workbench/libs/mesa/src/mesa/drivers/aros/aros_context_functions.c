/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_context_functions.c 31737 2009-08-23 13:34:05Z deadwood $
*/

#include "aros_context_functions.h"

#include <aros/debug.h>
#include "context.h"

void _aros_destroy_context(AROSMesaContext aros_ctx)
{
    if (aros_ctx)
    {
        _mesa_free_context_data(GET_GL_CTX_PTR(aros_ctx));
        FreeVec(aros_ctx);
    }
}
