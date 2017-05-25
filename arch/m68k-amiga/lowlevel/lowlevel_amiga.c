/*
    Copyright © 2013-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include <aros/symbolsets.h>
#include <graphics/gfxbase.h>

#include "lowlevel_intern.h"

static int Lowlevel_amiga_InitLib(struct LowLevelBase *LowLevelBase)
{
    struct GfxBase *GfxBase;

    GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    LowLevelBase->ll_EClockMult = (GfxBase->DisplayFlags & REALLY_PAL) ? 23245 : 23459;
    CloseLibrary((struct Library*)GfxBase);

    LowLevelBase->ll_CIA.llciat_iCRBit = -1;

    LowLevelBase->ll_PotgoBase = OpenResource("potgo.resource");
    if (LowLevelBase->ll_PotgoBase == NULL)
        return 0;

    LowLevelBase->ll_PortType[0] = 0;   /* Autosense */
    LowLevelBase->ll_PortType[1] = 0;   /* Autosense */

    return 1;
}

ADD2INITLIB(Lowlevel_amiga_InitLib, 40)
