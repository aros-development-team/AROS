/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/


#include "arosmesa_intern.h"

#include "glapi.h"

void **GETMESABASECTX(void)
{
    return &(REGMesaBase->mglb_CurrentContext);
}

struct _glapi_table **GETMESABASEDDISPATCH(void)
{
    return &(REGMesaBase->mglb_Dispatch);
}
