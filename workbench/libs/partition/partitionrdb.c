/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

*/

#define RDB_WRITE 1

#include <proto/exec.h>
#include <proto/partition.h>

#include <devices/hardblocks.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>

#include "partition_support.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include "debug.h"

#include <string.h>

struct RDBData {
    struct RigidDiskBlock rdb;
    UBYTE rdbblock; /* the block rdb was read from */
    struct List badblocklist;
    struct List fsheaderlist;
};

struct BadBlockNode {
    struct Node ln;
    struct BadBlockBlock bbb;
};

struct FileSysNode {
    struct Node ln;
    struct FileSysHeaderBlock fhb;
    struct LoadSegBlock *filesystem; /* the FS in LSEG blocks */
    ULONG fsblocks;                  /* nr of LSEG blocks for FS */
};

static ULONG calcChkSum(ULONG *ptr, ULONG size)
{
  ULONG i;
  ULONG sum=0;

  for (i=0;i<size;i++)
  {
    sum += AROS_BE2LONG(*ptr);
    ptr++;
  }
  return sum;
}

LONG PartitionRDBCheckPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
UBYTE i;
UBYTE space[root->de.de_SizeBlock<<2];
struct RigidDiskBlock *rdb = (struct RigidDiskBlock *)space;

    for (i=0;i<RDB_LOCATION_LIMIT; i++)
    {
        if (readBlock(PartitionBase, root, i, rdb) != 0)
            return 0;
        if (rdb->rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK))
            break;
    }
    if (i != RDB_LOCATION_LIMIT)
    {
        if (calcChkSum((ULONG *)rdb, AROS_BE2LONG(rdb->rdb_SummedLongs))==0)
            return 1;
    }
    return 0;
}

void CopyBE2HostDosEnvec(LONG *src, SIPTR *dst, ULONG size) {
ULONG count=0;

    while (count != size)
    {
        *dst++ = AROS_BE2LONG(*src);
        src++;
        count++;
    }
}

void CopyHost2BEDosEnvec(SIPTR *src, ULONG *dst, ULONG size) {

    ULONG count=0;

    while (count != size)
    {
        *dst++ = AROS_LONG2BE(*src);
        src++;
        count++;
    }

}

struct BadBlockNode *PartitionRDBNewBadBlock
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct BadBlockBlock *buffer
    )
{
struct BadBlockNode *bn;

    if (
            (AROS_BE2LONG(buffer->bbb_ID) == IDNAME_BADBLOCK) &&
            (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->bbb_SummedLongs))==0)
        )
    {
        bn = AllocMem(sizeof(struct BadBlockNode), MEMF_PUBLIC | MEMF_CLEAR);
        if (bn)
        {
            CopyMem(buffer, &bn->bbb, sizeof(struct BadBlockBlock));
            return bn;
        }
    }
    return NULL;
}

struct PartitionHandle *PartitionRDBNewHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct PartitionBlock *buffer
    )
{
struct PartitionBlock *pblock;
struct PartitionHandle *ph;

    if (
            (AROS_BE2LONG(buffer->pb_ID) == IDNAME_PARTITION) &&
            (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->pb_SummedLongs))==0)
        )
    {
        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph)
        {
            ph->ln.ln_Name = AllocVec(32, MEMF_PUBLIC | MEMF_CLEAR);
            if (ph->ln.ln_Name)
            {
                pblock = AllocMem(sizeof(struct PartitionBlock), MEMF_PUBLIC);
                if (pblock)
                {
                    CopyMem(buffer, pblock, sizeof(struct PartitionBlock));
                    ph->root = root;
                    ph->bd = root->bd;
                    ph->data = pblock;
                    CopyMem(pblock->pb_DriveName+1, ph->ln.ln_Name, pblock->pb_DriveName[0]);
                    ph->ln.ln_Name[pblock->pb_DriveName[0]]=0;
                    CopyBE2HostDosEnvec(pblock->pb_Environment, (SIPTR *)&ph->de, AROS_BE2LONG(((struct DosEnvec *)pblock->pb_Environment)->de_TableSize)+1);
                    ph->dg.dg_DeviceType = DG_DIRECT_ACCESS;
                    ph->dg.dg_SectorSize = ph->de.de_SizeBlock<<2;
                    ph->dg.dg_Heads = ph->de.de_Surfaces;
                    ph->dg.dg_TrackSectors = ph->de.de_BlocksPerTrack;
                    ph->dg.dg_Cylinders = ph->de.de_HighCyl - ph->de.de_LowCyl + 1;
                    ph->dg.dg_BufMemType = ph->de.de_BufMemType;
                    return ph;
                }
                FreeVec(ph->ln.ln_Name);
            }
            FreeMem(ph, sizeof(struct PartitionHandle));
        }
    }
    return NULL;
}

struct FileSysNode *PartitionRDBNewFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysHeaderBlock *buffer
    )
{
struct FileSysNode *fn;

    if (
            (AROS_BE2LONG(buffer->fhb_ID) == IDNAME_FILESYSHEADER) &&
            (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->fhb_SummedLongs))==0)
        )
    {
        fn = AllocMem(sizeof(struct FileSysNode), MEMF_PUBLIC | MEMF_CLEAR);
        if (fn)
        {
            CopyMem(buffer, &fn->fhb, sizeof(struct FileSysHeaderBlock));
            return fn;
        }
    }
    return NULL;
}

ULONG PartitionRDBCalcFSSize
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        struct LoadSegBlock *buffer
    )
{
ULONG size;
ULONG block;

    size = 0;
    block = AROS_BE2LONG(fn->fhb.fhb_SegListBlocks);
    while (block != (ULONG)-1)
    {
        size++;
        if (readBlock(PartitionBase, root, block, buffer) !=0)
            return 0;
        if (
                (AROS_BE2LONG(buffer->lsb_ID) != IDNAME_LOADSEG) ||
                (calcChkSum((ULONG *)buffer, AROS_BE2LONG(buffer->lsb_SummedLongs)))
            )
            return 0;
        block = AROS_BE2LONG(buffer->lsb_Next);
    }
    return size;
}

void PartitionRDBReadFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        struct LoadSegBlock *buffer
    )
{
ULONG size;
ULONG block;

    size = PartitionRDBCalcFSSize(PartitionBase, root, fn, buffer);
    if (size)
    {
        fn->fsblocks = size;
        fn->filesystem = AllocVec(size*512, MEMF_PUBLIC);
        if (fn->filesystem)
        {
            size = 0;
            block = AROS_BE2LONG(fn->fhb.fhb_SegListBlocks);
            while (block != (ULONG)-1)
            {
                if (readBlock(PartitionBase, root, block, &fn->filesystem[size]) !=0)
                    return;
                block = AROS_BE2LONG(fn->filesystem[size].lsb_Next);
                size++;
            }
        }
    }
}

LONG PartitionRDBOpenPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
UBYTE buffer[root->de.de_SizeBlock << 2];
struct RDBData *data;
UBYTE i;

    data = AllocMem(sizeof(struct RDBData), MEMF_PUBLIC);
    if (data)
    {
        for (i=0;i<RDB_LOCATION_LIMIT; i++)
        {
            if (readBlock(PartitionBase, root, i, buffer) != 0)
                return 1;
            CopyMem(buffer, &data->rdb, sizeof(struct RigidDiskBlock));
            if (data->rdb.rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK))
                break;
        }
        if (i != RDB_LOCATION_LIMIT)
        {
        ULONG block;

            data->rdbblock = i;
            NEWLIST(&root->table->list);
            NEWLIST(&data->badblocklist);
            NEWLIST(&data->fsheaderlist);
            root->table->data = data;
            /* take the values of the rdb instead of TD_GEOMETRY */
            root->dg.dg_SectorSize = AROS_BE2LONG(data->rdb.rdb_BlockBytes);
            root->dg.dg_Cylinders = AROS_BE2LONG(data->rdb.rdb_Cylinders);
            root->dg.dg_TrackSectors = AROS_BE2LONG(data->rdb.rdb_Sectors);
            root->dg.dg_Heads = AROS_BE2LONG(data->rdb.rdb_Heads);
            /* read bad blocks */
            block = AROS_BE2LONG(data->rdb.rdb_BadBlockList);
            while (block != (ULONG)-1)
            {
            struct BadBlockNode *bn;

                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    bn = PartitionRDBNewBadBlock(PartitionBase, root, (struct BadBlockBlock *)buffer);
                    if (bn != NULL)
                    {
                        AddTail(&data->badblocklist, &bn->ln);
                        block = AROS_BE2LONG(bn->bbb.bbb_Next);
                    }
                    else
                        break;
                }
                else
                    break;
            }
            /* read partition blocks */
            block = AROS_BE2LONG(data->rdb.rdb_PartitionList);
            while (block != (ULONG)-1)
            {
            struct PartitionHandle *ph;
                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    ph = PartitionRDBNewHandle(PartitionBase, root, (struct PartitionBlock *)buffer);
                    if (ph != NULL)
                    {
                        AddTail(&root->table->list, &ph->ln);
                        block = AROS_BE2LONG(((struct PartitionBlock *)ph->data)->pb_Next);
                    }
                    else
                        break;
                }
                else
                    break;
            }
            /* read filesystem blocks */
            block = AROS_BE2LONG(data->rdb.rdb_FileSysHeaderList);
            while (block != (ULONG)-1)
            {
            struct FileSysNode *fn;

                if (readBlock(PartitionBase, root, block, buffer)==0)
                {
                    fn = PartitionRDBNewFileSys(PartitionBase, root, (struct FileSysHeaderBlock *)buffer);
                    if (fn != NULL)
                    {
                        AddTail(&data->fsheaderlist, &fn->ln);
                        PartitionRDBReadFileSys(PartitionBase, root, fn, (struct LoadSegBlock *)buffer);
                        block = AROS_BE2LONG(fn->fhb.fhb_Next);
                    }
                    else
                        break;
                }
                else
                    break;
            }
            return 0;
        }
        FreeMem(data, sizeof(struct RDBData));
    }
    return 1;
}

void PartitionRDBFreeHandle
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
    ClosePartitionTable(ph);
    Remove(&ph->ln);
    FreeMem(ph->data, sizeof(struct PartitionBlock));
    FreeVec(ph->ln.ln_Name);
    FreeMem(ph, sizeof(struct PartitionHandle));
}

void PartitionRDBClosePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct PartitionHandle *ph;
struct BadBlockNode *bn;
struct FileSysNode *fn;
struct RDBData *data;

    while ((ph = (struct PartitionHandle *)RemTail(&root->table->list)))
        PartitionRDBFreeHandle(PartitionBase, ph);
    data = (struct RDBData *)root->table->data;
    while ((bn = (struct BadBlockNode *)RemTail(&data->badblocklist)))
        FreeMem(bn, sizeof(struct BadBlockNode));
    while ((fn = (struct FileSysNode *)RemTail(&data->fsheaderlist)))
    {
        if (fn->filesystem)
            FreeVec(fn->filesystem);
        FreeMem(fn, sizeof(struct FileSysNode));
    }
    FreeMem(data, sizeof(struct RDBData));
}

ULONG PartitionRDBWriteFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        ULONG block
    )
{
ULONG size;

    if (fn->filesystem)
    {
        size = 0;
        while (size != fn->fsblocks)
        {
            fn->filesystem[size].lsb_Next = (size+1) != fn->fsblocks ? AROS_LONG2BE(block+1) : (ULONG)-1;
            fn->filesystem[size].lsb_ChkSum = 0;
            fn->filesystem[size].lsb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&fn->filesystem[size], AROS_LONG2BE(fn->filesystem[size].lsb_SummedLongs)));
#if RDB_WRITE
            if (writeBlock(PartitionBase, root, block++, &fn->filesystem[size]) != 0)
                return block;
#else
            kprintf("RDB-write: block=%ld, type=LSEG\n", block);
            block++;
#endif
            size++;
        }
    }
    return block;
}

LONG PartitionRDBWritePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
UBYTE buffer[root->de.de_SizeBlock << 2];
struct RDBData *data;
struct PartitionHandle *ph;
struct PartitionBlock *pblock;
struct BadBlockNode *bn;
struct FileSysNode *fn;
ULONG block;

    data = root->table->data;
    block = data->rdbblock+1; /* RDB will be written at the end */
    fillMem((UBYTE *)buffer, root->de.de_SizeBlock << 2, 0);

    /* write bad blocks */
    bn = (struct BadBlockNode *)data->badblocklist.lh_Head;
    if (bn->ln.ln_Succ)
        data->rdb.rdb_BadBlockList = block;
    else
        data->rdb.rdb_BadBlockList = (ULONG)-1;
    while (bn->ln.ln_Succ)
    {
        bn->bbb.bbb_Next = bn->ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block+1) : (ULONG)-1;
        bn->bbb.bbb_ChkSum = 0;
        bn->bbb.bbb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&bn->bbb, AROS_BE2LONG(bn->bbb.bbb_SummedLongs)));
        CopyMem(&bn->bbb, buffer, sizeof(struct BadBlockBlock));
#if RDB_WRITE
        writeBlock(PartitionBase, root, block++, buffer);
#else
        kprintf("RDB-write: block=%ld, type=BADB\n", block);
        block++;
#endif

        bn = (struct BadBlockNode *)bn->ln.ln_Succ;
    }

    /* write partition blocks */
    ph = (struct PartitionHandle *)root->table->list.lh_Head;
    if (ph->ln.ln_Succ)
        data->rdb.rdb_PartitionList = AROS_LONG2BE(block);
    else
        data->rdb.rdb_PartitionList = (ULONG)-1;
    while (ph->ln.ln_Succ)
    {
        pblock = (struct PartitionBlock *)ph->data;
        pblock->pb_Next = ph->ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block+1) : (ULONG)-1;
        pblock->pb_ChkSum = 0;
        pblock->pb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)pblock, AROS_BE2LONG(pblock->pb_SummedLongs)));
        CopyMem(pblock, buffer, sizeof(struct PartitionBlock));
#if RDB_WRITE
        writeBlock(PartitionBase, root, block++, buffer);
#else
        kprintf("RDB-write: block=%ld, type=PART\n", block);
        block++;
#endif
        ph = (struct PartitionHandle *)ph->ln.ln_Succ;
    }

    /* write filesystem blocks */
    fn = (struct FileSysNode *)data->fsheaderlist.lh_Head;
    if (fn->ln.ln_Succ)
        data->rdb.rdb_FileSysHeaderList = AROS_LONG2BE(block);
    else
        data->rdb.rdb_FileSysHeaderList = (ULONG)-1;
    while (fn->ln.ln_Succ)
    {
    ULONG fshblock;

        fshblock = block;
        block++; /* header block will be written later */
        fn->fhb.fhb_SegListBlocks = AROS_LONG2BE(block);
        /* write filesystem LSEG blocks */
        block = PartitionRDBWriteFileSys(PartitionBase, root, fn, block);
        fn->fhb.fhb_Next = fn->ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block) : (ULONG)-1;
        fn->fhb.fhb_ChkSum = 0;
        CopyMem(&fn->fhb, buffer, sizeof(struct FileSysHeaderBlock));
        ((struct FileSysHeaderBlock *)buffer)->fhb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)buffer, AROS_BE2LONG(fn->fhb.fhb_SummedLongs)));
#if RDB_WRITE
        writeBlock(PartitionBase, root, fshblock, buffer);
#else
        kprintf("RDB-write: block=%ld, type=FSHD\n", fshblock);
#endif
        fn = (struct FileSysNode *)fn->ln.ln_Succ;
    }
    data->rdb.rdb_HighRDSKBlock = AROS_LONG2BE(block-1);
    data->rdb.rdb_ChkSum = 0;
    data->rdb.rdb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&data->rdb, AROS_BE2LONG(data->rdb.rdb_SummedLongs)));
    CopyMem(&data->rdb, buffer, sizeof(struct RigidDiskBlock));
#if RDB_WRITE
    writeBlock(PartitionBase, root, data->rdbblock, buffer);
#else
    kprintf("RDB-write: block=%ld, type=RDSK\n", data->rdbblock);
#endif
    return 0;
}

LONG PartitionRDBCreatePartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{
struct RDBData *data;
ULONG i;

    data = AllocMem(sizeof(struct RDBData), MEMF_PUBLIC | MEMF_CLEAR);
    if (data)
    {
        ph->table->data = data;
        data->rdb.rdb_ID = AROS_LONG2BE(IDNAME_RIGIDDISK);
        data->rdb.rdb_SummedLongs = AROS_LONG2BE(sizeof(struct RigidDiskBlock)/4);
        data->rdb.rdb_BlockBytes = AROS_LONG2BE(ph->dg.dg_SectorSize);
        data->rdb.rdb_BadBlockList = (ULONG)-1;
        data->rdb.rdb_PartitionList = (ULONG)-1;
        data->rdb.rdb_FileSysHeaderList = (ULONG)-1;
        data->rdb.rdb_DriveInit = (ULONG)-1;
        for (i=0;i<6;i++)
            data->rdb.rdb_Reserved1[i] = (ULONG)-1;
        data->rdb.rdb_Cylinders = AROS_LONG2BE(ph->dg.dg_Cylinders);
        data->rdb.rdb_Sectors = AROS_LONG2BE(ph->dg.dg_TrackSectors);
        data->rdb.rdb_Heads = AROS_LONG2BE(ph->dg.dg_Heads);

        data->rdb.rdb_Park = data->rdb.rdb_Cylinders;
        data->rdb.rdb_WritePreComp = data->rdb.rdb_Cylinders;
        data->rdb.rdb_ReducedWrite = data->rdb.rdb_Cylinders;
        /* StepRate */

        data->rdb.rdb_RDBBlocksLo = AROS_LONG2BE(1); /* leave a block for PC */
        data->rdb.rdb_RDBBlocksHi = AROS_LONG2BE((ph->dg.dg_Heads*ph->dg.dg_TrackSectors*2)-1); /* two cylinders */
        data->rdb.rdb_LoCylinder = AROS_LONG2BE(2);
        data->rdb.rdb_HiCylinder = AROS_LONG2BE(ph->dg.dg_Cylinders-1);
        data->rdb.rdb_CylBlocks = AROS_LONG2BE(ph->dg.dg_Heads*ph->dg.dg_TrackSectors);
        /* AutoParkSeconds */
        /* DiskVendor */
        /* DiskProduct */
        /* DiskRevision */
        /* ControllerVendor */
        /* ControllerProduct */
        /* ControllerRevision */

        data->rdbblock = 1;
        NEWLIST(&data->badblocklist);
        NEWLIST(&data->fsheaderlist);
        NEWLIST(&ph->table->list);
        return 0;
    }
    return 1;
}

LONG PartitionRDBGetPartitionTableAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {

        switch (taglist[0].ti_Tag)
        {
        case PTT_TYPE:
            *((LONG *)taglist[0].ti_Data) = root->table->type;
            break;
        case PTT_RESERVED:
            *((LONG *)taglist[0].ti_Data) =
                root->de.de_Surfaces*root->de.de_BlocksPerTrack*2; /* 2 cylinders */
            break;
        }
        taglist++;
    }
    return 0;
}

LONG PartitionRDBSetPartitionTableAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {

        switch (taglist[0].ti_Tag)
        {
        }
        taglist++;
    }
    return 0;
}

LONG PartitionRDBGetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct PartitionBlock *data = (struct PartitionBlock *)ph->data;

        switch (taglist[0].ti_Tag)
        {
        case PT_GEOMETRY:
            {
                struct DriveGeometry *dg = (struct DriveGeometry *)taglist[0].ti_Data;
                CopyMem(&ph->dg, dg, sizeof(struct DriveGeometry));
            }
            break;
        case PT_DOSENVEC:
            CopyMem(&ph->de, (struct DosEnvec *)taglist[0].ti_Data, sizeof(struct DosEnvec));
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)taglist[0].ti_Data;
            ULONG dt = AROS_LONG2BE(ph->de.de_DosType);

                CopyMem(&dt, ptype->id, 4);
                ptype->id_len = 4;
            }
            break;
        case PT_NAME:
            CopyMem(ph->ln.ln_Name, (UBYTE *)taglist[0].ti_Data, 32);
            break;
        case PT_BOOTABLE:
            *((LONG *)taglist[0].ti_Data) = (AROS_BE2LONG(data->pb_Flags) & PBFF_BOOTABLE) ? TRUE : FALSE;
            break;
        case PT_AUTOMOUNT:
            *((LONG *)taglist[0].ti_Data) = (AROS_BE2LONG(data->pb_Flags) & PBFF_NOMOUNT) ? FALSE : TRUE;
            break;
        }
        taglist++;
    }
    return 0;
}

LONG PartitionRDBSetPartitionAttrs
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph,
        struct TagItem *taglist
    )
{

    while (taglist[0].ti_Tag != TAG_DONE)
    {
    struct PartitionBlock *data = (struct PartitionBlock *)ph->data;

        switch (taglist[0].ti_Tag)
        {
        case PT_DOSENVEC:
            {
            struct DosEnvec *de = (struct DosEnvec *)taglist[0].ti_Data;

                CopyMem(de, &ph->de, (de->de_TableSize+1)*sizeof(IPTR));
                CopyHost2BEDosEnvec((SIPTR *)de, data->pb_Environment, de->de_TableSize+1);
            }
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)taglist[0].ti_Data;
            ULONG dt;

                CopyMem(ptype->id, &dt, 4);
                ph->de.de_DosType = AROS_BE2LONG(dt);
                data->pb_Environment[DE_DOSTYPE] = dt;
            }
            break;
        case PT_NAME:
            {
            STRPTR name = (STRPTR)taglist[0].ti_Data;
            ULONG len = strlen(name);

                CopyMem(name, ph->ln.ln_Name, len+1);
                CopyMem(name, data->pb_DriveName+1, len);
                data->pb_DriveName[len+1] = 0;
                data->pb_DriveName[0] = len;
            }
            break;
        case PT_BOOTABLE:
            if (taglist[0].ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_BOOTABLE);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_BOOTABLE);
            break;
        case PT_AUTOMOUNT:
            if (taglist[0].ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_NOMOUNT);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_NOMOUNT);
            break;
        }
        taglist++;
    }
    return 0;
}

struct PartitionHandle *PartitionRDBAddPartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct TagItem *taglist
    )
{

    if (findTagItem(PT_DOSENVEC, taglist) != NULL)
    {
    struct PartitionBlock *pblock;
    struct PartitionHandle *ph;
    struct PartitionHandle *oph;

        ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
        if (ph)
        {
            ph->ln.ln_Name = AllocVec(32, MEMF_PUBLIC | MEMF_CLEAR);
            if (ph->ln.ln_Name)
            {
                pblock = AllocMem(sizeof(struct PartitionBlock), MEMF_PUBLIC | MEMF_CLEAR);
                if (pblock)
                {
                    ph->root = root;
                    ph->bd = root->bd;
                    ph->data = pblock;
                    pblock->pb_ID = AROS_LONG2BE(IDNAME_PARTITION);
                    pblock->pb_SummedLongs = AROS_LONG2BE(sizeof(struct PartitionBlock)/4);
                    PartitionRDBSetPartitionAttrs(PartitionBase, ph, taglist);
                    oph = (struct PartitionHandle *)root->table->list.lh_Head;
                    while (oph->ln.ln_Succ)
                    {
                        if (ph->de.de_LowCyl<oph->de.de_LowCyl)
                            break;
                        oph = (struct PartitionHandle *)oph->ln.ln_Succ;
                    }
                    if (oph->ln.ln_Succ)
                    {
                        oph = (struct PartitionHandle *)oph->ln.ln_Pred;
                        if (oph->ln.ln_Pred)
                        {
                            Insert(&root->table->list, &ph->ln, &oph->ln);
                        }
                        else
                            AddHead(&root->table->list, &ph->ln);
                    }
                    else
                        AddTail(&root->table->list, &ph->ln);
                    if (findTagItem(PT_DOSENVEC, taglist) == NULL)
                    {
                        ph->dg.dg_DeviceType = DG_DIRECT_ACCESS;
                        ph->dg.dg_SectorSize = ph->de.de_SizeBlock<<2;
                        ph->dg.dg_Heads = ph->de.de_Surfaces;
                        ph->dg.dg_TrackSectors = ph->de.de_BlocksPerTrack;
                        ph->dg.dg_Cylinders = ph->de.de_HighCyl - ph->de.de_LowCyl + 1;
                        ph->dg.dg_BufMemType = ph->de.de_BufMemType;
                    }
                    return ph;
                }
                FreeVec(ph->ln.ln_Name);
            }
            FreeMem(ph, sizeof(struct PartitionHandle));
        }
    }
    return NULL;
}

void PartitionRDBDeletePartition
    (
        struct Library *PartitionBase,
        struct PartitionHandle *ph
    )
{

    PartitionRDBFreeHandle(PartitionBase, ph);
}

struct PartitionAttribute PartitionRDBPartitionTableAttrs[]=
{
    {PTTA_TYPE,     PLAM_READ},
    {PTTA_RESERVED, PLAM_READ},
    {PTTA_DONE,     0}
};

struct PartitionAttribute *PartitionRDBQueryPartitionTableAttrs(struct Library *PartitionBase)
{
    return PartitionRDBPartitionTableAttrs;
}

struct PartitionAttribute PartitionRDBPartitionAttrs[]=
{
#warning "TODO: implement write"
    {PTA_GEOMETRY,  PLAM_READ},
    {PTA_DOSENVEC,  PLAM_READ | PLAM_WRITE},
    {PTA_TYPE,      PLAM_READ | PLAM_WRITE},
    {PTA_NAME,      PLAM_READ | PLAM_WRITE},
    {PTA_BOOTABLE,  PLAM_READ | PLAM_WRITE},
    {PTA_AUTOMOUNT, PLAM_READ | PLAM_WRITE},
    {PTA_DONE,      0}
};

struct PartitionAttribute *PartitionRDBQueryPartitionAttrs(struct Library *PartitionBase)
{
    return PartitionRDBPartitionAttrs;
}

ULONG PartitionRDBDestroyPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct RDBData *data;
UBYTE buffer[root->de.de_SizeBlock << 2];

    data = root->table->data;
    CopyMem(&data->rdb, buffer, sizeof(struct RigidDiskBlock));
    ((struct RigidDiskBlock *)buffer)->rdb_ID = 0;
    if (writeBlock(PartitionBase, root, data->rdbblock, buffer))
        return 1;
    return 0;
}

struct PTFunctionTable PartitionRDB =
{
    PHPTT_RDB,
    "RDB",
    PartitionRDBCheckPartitionTable,
    PartitionRDBOpenPartitionTable,
    PartitionRDBClosePartitionTable,
    PartitionRDBWritePartitionTable,
    PartitionRDBCreatePartitionTable,
    PartitionRDBAddPartition,
    PartitionRDBDeletePartition,
    PartitionRDBGetPartitionTableAttrs,
    PartitionRDBSetPartitionTableAttrs,
    PartitionRDBGetPartitionAttrs,
    PartitionRDBSetPartitionAttrs,
    PartitionRDBQueryPartitionTableAttrs,
    PartitionRDBQueryPartitionAttrs,
    PartitionRDBDestroyPartitionTable
};

