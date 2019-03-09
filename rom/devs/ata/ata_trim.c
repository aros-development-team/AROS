/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <hidd/storage.h>
#include <hidd/ata.h>
#include <oop/oop.h>

#include <devices/ata.h>

#include "ata.h"

void ata_TRIMCmd(struct IOStdReq *io)
{
#if (0)
    ata_CommandBlock acb =
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    };
#endif
    D(bug("[ATA%02ld] %s()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, __func__));
}
