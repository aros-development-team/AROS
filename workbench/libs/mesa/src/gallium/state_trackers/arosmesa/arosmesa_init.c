/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include "state_tracker/st_gl_api.h"
#include "state_tracker/st_api.h"

/* This is a global GL API object */
/* TODO: Should be moved to LIBBASE */
struct st_api * glstapi;

LONG AROSMesaInit()
{
    glstapi = st_gl_api_create();
    if (glstapi)
        return 1;
    else
        return 0;
}


VOID AROSMesaExit()
{
    if (glstapi) glstapi->destroy(glstapi);
}

ADD2INIT(AROSMesaInit, 5);
ADD2EXIT(AROSMesaExit, 5);

