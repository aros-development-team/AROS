/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>

#include "sdcard_base.h"
#include "sdcard_unit.h"

ULONG FNAME_SDCUNIT(MMCChangeFrequency)(struct sdcard_Unit *sdcUnit)
{
    return 0;
}
