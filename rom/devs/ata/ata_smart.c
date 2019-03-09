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

/* perform the S.M.A.R.T operation specified in io_Reserved1 */
void ata_SMARTCmd(struct IOStdReq *io)
{
#if (0)
    ata_CommandBlock acb =
    {
        ATA_SMART,
        IOStdReq(io)->io_Reserved1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        CM_NoData,
        CT_NoBlock
    };
#endif
    D(bug("[ATA%02ld] %s()\n", ((struct ata_Unit*)io->io_Unit)->au_UnitNum, __func__));
#if (0)
	if (IOStdReq(io)->io_Reserved1 == SMARTC_READ_VALUES || IOStdReq(io)->io_Reserved1 == SMARTC_READ_THRESHOLDS) {
		acb.buffer = IOStdReq(io)->io_Data;
		acb.length = IOStdReq(io)->io_Length;
	}
#endif
}
