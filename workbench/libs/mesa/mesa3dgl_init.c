/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <aros/symbolsets.h>

#include <stdint.h>
#include <stdbool.h>

#include "mesa3dgl_types.h"

LONG MESA3DGLInit()
{
    D(bug("[MESA3DGL] %s()\n", __func__));

    /* nothing to set up: Mesa >= 22 is driven through st_api_*() directly */
    return TRUE;
}

VOID MESA3DGLExit()
{
    D(bug("[MESA3DGL] %s()\n", __func__));
}

ADD2INIT(MESA3DGLInit, 5);
ADD2EXIT(MESA3DGLExit, 5);

