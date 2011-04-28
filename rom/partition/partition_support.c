/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <proto/exec.h>
#include <devices/newstyle.h>
#include <utility/tagitem.h>

#include "partition_intern.h"
#include "partition_support.h"
#include "debug.h"

const struct PTFunctionTable * const PartitionSupport[] =
{
    &PartitionRDB,
    &PartitionGPT, /* The order of these two is important, GPT must be checked before MBR */
    &PartitionMBR,
    &PartitionEBR,
    NULL
};

/* get geometry */
LONG PartitionGetGeometry
   (
      struct Library *PartitionBase,
      struct IOExtTD *ioreq,
      struct DriveGeometry *dg
   )
{

   ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
   ioreq->iotd_Req.io_Data = dg;
   ioreq->iotd_Req.io_Length = sizeof(struct DriveGeometry);
   return DoIO((struct IORequest *)ioreq);
}

/* query NSD commands */
void PartitionNsdCheck
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct NSDeviceQueryResult nsdq;
struct IOExtTD *ioreq = root->bd->ioreq;
UWORD *cmdcheck;

   if (
         (
                root->de.de_HighCyl*
                root->de.de_Surfaces*
                root->de.de_BlocksPerTrack*
                ((root->de.de_SizeBlock<<2)/512)
         )>8388608)
   {
      nsdq.SizeAvailable=0;
      nsdq.DevQueryFormat=0;
      ioreq->iotd_Req.io_Command=NSCMD_DEVICEQUERY;
      ioreq->iotd_Req.io_Data=&nsdq;
      ioreq->iotd_Req.io_Length=sizeof(struct NSDeviceQueryResult);
      if (DoIO((struct IORequest *)ioreq)==0)
      {
         if (
               (ioreq->iotd_Req.io_Actual<=sizeof(struct NSDeviceQueryResult)) &&
               (ioreq->iotd_Req.io_Actual!=0) &&
               (ioreq->iotd_Req.io_Actual==nsdq.SizeAvailable)
            )
         {
            if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
               D(bug("partition: NSDcheck: WARNING no trackdisk type\n"));
            for (cmdcheck=nsdq.SupportedCommands;*cmdcheck;cmdcheck++)
            {
               if (*cmdcheck == NSCMD_TD_READ64)
                  root->bd->cmdread = NSCMD_TD_READ64;
               if (*cmdcheck == NSCMD_TD_WRITE64);
                  root->bd->cmdwrite = NSCMD_TD_WRITE64;
            }
         }
            else
            D(bug("partition: NSDcheck: WARNING wrong io_Actual using NSD\n"));
      }
   }
}

/* get real first block of partition ph */
ULONG getStartBlock(struct PartitionHandle *ph) {
ULONG start = 0;

    while (ph)
    {
        start += ph->de.de_LowCyl*ph->de.de_BlocksPerTrack*ph->de.de_Surfaces;
        ph = ph->root;
    }
    return start;
}

/* read a single block within partition ph */
LONG readBlock(struct Library *PartitionBase, struct PartitionHandle *ph, ULONG block, void *mem)
{
    return readData(ph, block, ph->de.de_SizeBlock<<2, mem);
}

/* read 'size' bytes starting from 'block' within partition ph */
LONG readData(struct PartitionHandle *ph, UQUAD block, ULONG size, void *mem)
{
    UQUAD offset = (getStartBlock(ph) + block) * (ph->de.de_SizeBlock<<2);
    struct IOExtTD *ioreq;

    ioreq = ph->bd->ioreq;
    ioreq->iotd_Req.io_Command = ph->bd->cmdread;
    ioreq->iotd_Req.io_Length  = size;
    ioreq->iotd_Req.io_Data    = mem;
    ioreq->iotd_Req.io_Offset  = offset;
    ioreq->iotd_Req.io_Actual  = offset >> 32;

    return DoIO((struct IORequest *)&ioreq->iotd_Req);
}

/*
    write a block
    block is within partition ph
*/
LONG PartitionWriteBlock
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        ULONG block,
        void *mem
    )
{
struct IOExtTD *ioreq;
#ifdef __AMIGAOS__
ULONG lo, hi;
#else
UQUAD offset;
#endif

    ioreq = ph->bd->ioreq;
    ioreq->iotd_Req.io_Command=ph->bd->cmdwrite;
    ioreq->iotd_Req.io_Length=ph->de.de_SizeBlock<<2;
    ioreq->iotd_Req.io_Data=mem;
#ifdef __AMIGAOS__
    lo = getStartBlock(ph)+block;
    hi = lo>>16;  /* high 16 bits into "hi" */
    lo &= 0xFFFF; /* low 16 bits stay in "lo" */
    lo *= (ph->de.de_SizeBlock<<2); /* multiply lo part */
    hi *= (ph->de.de_SizeBlock<<2); /* multiply hi part */
    ioreq->iotd_Req.io_Offset = lo+(hi<<16); /* compose first low 32 bit */
    ioreq->iotd_Req.io_Actual = hi>>16; /* high 32 bits (at least until bit 48 */
#else
    offset=(UQUAD)(getStartBlock(ph)+block)*(ph->de.de_SizeBlock<<2);
    ioreq->iotd_Req.io_Offset=0xFFFFFFFF & offset;
    ioreq->iotd_Req.io_Actual=offset>>32;
#endif
    return DoIO((struct IORequest *)&ioreq->iotd_Req);
}

/*
 * Initialize partition handle based on parent's data.
 * Geometry will be inherited from parent and adjusted if needed
 * for given start block and length in blocks to fit in.
 */
void initPartitionHandle(struct PartitionHandle *root, struct PartitionHandle *ph, ULONG first_sector, ULONG count_sector)
{
    ULONG cylsecs = root->de.de_BlocksPerTrack * root->de.de_Surfaces;

    /* Attach parent */
    ph->root = root;
    ph->bd   = root->bd;

    /* initialize DosEnvec */
    CopyMemQuick(&root->de, &ph->de, sizeof(struct DosEnvec));

    /* Check if partition starts and ends on a cylinder boundary */
    if ((first_sector % cylsecs != 0) || (count_sector % cylsecs != 0))
    {
        /* Treat each track as a cylinder if possible */
        ph->de.de_Surfaces = 1;
        cylsecs = ph->de.de_BlocksPerTrack;

        if ((first_sector % cylsecs != 0) || (count_sector % cylsecs != 0))
        {
            /*
             * We can't. We could find the highest common factor of
             * first_sector and count_sector here, but currently we
             * simply use one block per cylinder (flat LBA)
             */
            ph->de.de_BlocksPerTrack = 1;
            cylsecs = 1;
        }
    }

    /* initialize DriveGeometry */
    ph->dg.dg_DeviceType   = DG_DIRECT_ACCESS;
    ph->dg.dg_SectorSize   = ph->de.de_SizeBlock<<2;
    ph->dg.dg_Heads        = ph->de.de_Surfaces;
    ph->dg.dg_TrackSectors = ph->de.de_BlocksPerTrack;
    ph->dg.dg_Cylinders    = count_sector / cylsecs;
    ph->dg.dg_BufMemType   = ph->de.de_BufMemType;

    /* Set start/end cylinder in DosEnvec */
    ph->de.de_TableSize = 10;	/* only fields up to de_HighCyl are valid (CHECKME: is it correct ?) */
    ph->de.de_LowCyl    = first_sector / cylsecs;
    ph->de.de_HighCyl   = ph->de.de_LowCyl + ph->dg.dg_Cylinders - 1;
}

struct TagItem *findTagItem(ULONG tag, struct TagItem *taglist) {

    while (taglist[0].ti_Tag != TAG_DONE)
    {
        if (taglist[0].ti_Tag == tag)
            return &taglist[0];
        taglist++;
    }
    return 0;
}

void fillMem(BYTE *mem, LONG size, BYTE fillbyte) {

    while (size--)
        mem[size]=fillbyte;
}
