/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/


#include <proto/exec.h>
#include <devices/newstyle.h>
#include <utility/tagitem.h>

#include "partition_intern.h"
#include "partition_support.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include "debug.h"

extern struct PTFunctionTable PartitionMBR;
extern struct PTFunctionTable PartitionRDB;

struct PTFunctionTable *PartitionSupport[]={&PartitionRDB, &PartitionMBR, 0};

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


/*
    read a block
    block is within partition ph
*/
LONG readBlock
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
    ioreq->iotd_Req.io_Command=ph->bd->cmdread;
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
