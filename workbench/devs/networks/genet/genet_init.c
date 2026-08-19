/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    BCM2711 GENET Ethernet — Device initialization
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/errors.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "genet.h"
#include LC_LIBDEFS_FILE

static int genet_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[genet] Init\n"));

    LIBBASE->gn_KernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->gn_KernelBase) {
        D(bug("[genet] Cannot open kernel.resource\n"));
        return FALSE;
    }

    /* Set global for proto/kernel.h macros */
    {
        extern APTR KernelBase;
        KernelBase = LIBBASE->gn_KernelBase;
    }

    LIBBASE->gn_UtilityBase = OpenLibrary("utility.library", 36);
    if (!LIBBASE->gn_UtilityBase) {
        D(bug("[genet] Cannot open utility.library\n"));
        return FALSE;
    }

    InitSemaphore(&LIBBASE->gn_Lock);

    /* Probe: check if GENET is present at the expected address */
    {
        IPTR regbase = GENET_BASE_DEFAULT;
        ULONG rev = AROS_LE2LONG(*(volatile ULONG *)regbase);
        ULONG major = (rev >> 24) & 0x0F;

        D(bug("[genet] GENET revision register: 0x%08lx (major=%ld)\n", rev, major));

        /* GENETv5 reports major=6 in the register (quirk from Broadcom) */
        if (major == 6 || major == 5) {
            struct GENETUnit *unit = genet_CreateUnit(LIBBASE, 0);
            if (unit) {
                LIBBASE->gn_Units[0] = unit;
                LIBBASE->gn_UnitCount = 1;
                D(bug("[genet] Unit 0 created at 0x%p\n", regbase));
            }
        } else {
            D(bug("[genet] No GENETv5 found (major=%ld)\n", major));
        }
    }

    return TRUE;
}

static int genet_Expunge(LIBBASETYPEPTR LIBBASE)
{
    ULONG i;

    for (i = 0; i < LIBBASE->gn_UnitCount; i++) {
        if (LIBBASE->gn_Units[i])
            genet_DeleteUnit(LIBBASE, LIBBASE->gn_Units[i]);
    }

    if (LIBBASE->gn_UtilityBase)
        CloseLibrary(LIBBASE->gn_UtilityBase);

    return TRUE;
}

ADD2INITLIB(genet_Init, 0)
ADD2EXPUNGELIB(genet_Expunge, 0)
