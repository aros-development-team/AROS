/*
    Copyright 2010-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include "state_tracker/st_gl_api.h"
#include "state_tracker/st_api.h"

/* This is a global GL API object */
/* TODO: Should be moved to LIBBASE */
struct st_api * glstapi;

LONG MESA3DGLInit()
{
    glstapi = st_gl_api_create();
    if (glstapi)
        return 1;
    else
        return 0;
}


VOID MESA3DGLExit()
{
    if (glstapi) glstapi->destroy(glstapi);
}

ADD2INIT(MESA3DGLInit, 5);
ADD2EXIT(MESA3DGLExit, 5);

