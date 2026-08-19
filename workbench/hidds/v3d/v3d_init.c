/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D) — HIDD initialization
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/kernel.h>

#include <hidd/gallium.h>

#include "v3d_intern.h"

#include LC_LIBDEFS_FILE

APTR KernelBase = NULL;

static int V3D_Init(LIBBASETYPEPTR LIBBASE)
{
    struct V3DData *sd = &LIBBASE->sd;

    D(bug("[V3D] Init\n"));

    KernelBase = OpenResource("kernel.resource");

    if (!v3d_hw_init(sd)) {
        D(bug("[V3D] Hardware init failed — V3D not available\n"));
        /* Not fatal: system works without 3D acceleration */
        return TRUE;
    }

    D(bug("[V3D] Hardware initialized successfully\n"));

    return TRUE;
}

static int V3D_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct V3DData *sd = &LIBBASE->sd;

    if (sd->powered)
        v3d_hw_shutdown(sd);

    return TRUE;
}

ADD2INITLIB(V3D_Init, 0)
ADD2EXPUNGELIB(V3D_Expunge, 0)
