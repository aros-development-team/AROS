/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "lowlevel_intern.h"

static int Lowlevel_amiga_InitLib(struct LowLevelBase *LowLevelBase)
{
    LowLevelBase->ll_PotgoBase = OpenResource("potgo.resource");
    if (LowLevelBase->ll_PotgoBase == NULL)
        return 0;

    LowLevelBase->ll_PortType[0] = 0;   /* Autosense */
    LowLevelBase->ll_PortType[1] = 0;   /* Autosense */
    return 1;
}

ADD2INITLIB(Lowlevel_amiga_InitLib, 40)
