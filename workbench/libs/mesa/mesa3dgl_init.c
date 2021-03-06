/*
    Copyright (C) 2010-2020, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <aros/symbolsets.h>

#include <stdint.h>
#include <stdbool.h>

#include "mesa3dgl_types.h"

/* This is a global GL API object */
/* TODO: Should be moved to LIBBASE */
struct st_api * glstapi;

LONG MESA3DGLInit()
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    glstapi = st_gl_api_create();

    D(bug("[MESA3DGL] %s: st_api @ 0x%p\n", __func__, glstapi));

    return (glstapi != 0);
}

VOID MESA3DGLExit()
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    if (glstapi)
        glstapi->destroy(glstapi);
}

ADD2INIT(MESA3DGLInit, 5);
ADD2EXIT(MESA3DGLExit, 5);

