/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <proto/exec.h>
#include <exec/memory.h>

#include "partition_intern.h"
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
   AROS_LH2(struct PartitionHandle *, OpenRootPartition,

/*  SYNOPSIS */
   AROS_LHA(STRPTR, Device, A1),
   AROS_LHA(LONG,     Unit, D1),

/*  LOCATION */
   struct Library *, PartitionBase, 5, Partition)

/*  FUNCTION
    create a root handle, open device/unit

    INPUTS
    Device - name of the block device
    Unit - unit of the block device

    RESULT
    handle to the device

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
    21-02-02    first version

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PartitionHandle *ph;

    ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
    if (ph)
    {
        ph->bd = AllocMem(sizeof(struct PartitionBlockDevice), MEMF_PUBLIC);
        if (ph->bd)
        {
            ph->bd->cmdread = CMD_READ;
            ph->bd->cmdwrite = CMD_WRITE;
            ph->bd->port = CreateMsgPort();
            if (ph->bd->port)
            {
                ph->bd->ioreq = (struct IOExtTD *)CreateIORequest(ph->bd->port, sizeof(struct IOExtTD));
                if (ph->bd->ioreq)
                {
                    if (OpenDevice(Device, Unit, (struct IORequest *)ph->bd->ioreq, 0)==0)
                    {
                        if (getGeometry(PartitionBase, ph->bd->ioreq, &ph->dg)==0)
                        {
                            if (ph->dg.dg_DeviceType != DG_CDROM)
                            {
                                ph->de.de_SizeBlock = ph->dg.dg_SectorSize>>2;
                                ph->de.de_Surfaces = ph->dg.dg_Heads;
                                ph->de.de_SectorPerBlock = 1;
                                ph->de.de_BlocksPerTrack = ph->dg.dg_TrackSectors;
                                ph->de.de_HighCyl = ph->dg.dg_Cylinders-1;
                                ph->de.de_NumBuffers = 20;
                                ph->de.de_BufMemType = ph->dg.dg_BufMemType;
                                PartitionNsdCheck(PartitionBase, ph);
                                return ph;
                            }
                        }
                    }
                    DeleteIORequest((struct IORequest *)ph->bd->ioreq);
                }
                DeleteMsgPort(ph->bd->port);
            }
            FreeMem(ph->bd, sizeof(struct PartitionBlockDevice));
        }
        FreeMem(ph, sizeof(struct PartitionHandle));
        ph = 0;
    }
    return 0;
    AROS_LIBFUNC_EXIT
}
