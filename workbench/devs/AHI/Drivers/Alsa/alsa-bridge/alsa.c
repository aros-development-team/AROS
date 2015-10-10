/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "alsa.h"
#include "alsa_hostlib.h"

BOOL ALSA_Init()
{
    return ALSA_HostLib_Init();
}

VOID ALSA_Cleanup()
{
    ALSA_HostLib_Cleanup();
}
