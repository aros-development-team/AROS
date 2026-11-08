/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: RDB partition table handler, write support
*/

#define RDB_WRITE 1

#include <aros/config.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <proto/utility.h>

#include <devices/hardblocks.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>
#include <resources/filesysres.h>

#include "ptedit_intern.h"
#include "partitionrdb.h"
#include "partition_types.h"
#include "platform.h"

#ifndef DEBUG
#define DEBUG 0
#endif
#include "debug.h"

#include <string.h>

#if AROS_BIG_ENDIAN && (__WORDSIZE == 32)
/* Same layout and byte order: a plain copy */
#define CopyHost2BEDosEnvec(src, dst, size) CopyMem((src), (dst), (size) * sizeof(ULONG))
#else
static void CopyHost2BEDosEnvec(SIPTR *src, ULONG *dst, ULONG size) {

    ULONG count=0;

    while (count != size)
    {
        *dst++ = AROS_LONG2BE(*src);
        src++;
        count++;
    }

}
#endif

/* All retained records occupy a complete device sector. Checksum and write
 * that buffer directly instead of copying each record through root->buffer. */
static LONG PartitionRDBWriteRecord(struct Library *PartitionBase,
    struct PartitionHandle *root, ULONG block, void *record)
{
    ULONG *words = record;

    words[2] = 0;
    words[2] = AROS_LONG2BE(0 - calcChkSum(words, AROS_BE2LONG(words[1])));
    return writeBlock(PartitionBase, root, block, record);
}

LONG PartitionRDBWriteFileSys
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root,
        struct FileSysNode *fn,
        ULONG *nextblock,
        ULONG first
    )
{
ULONG size;
ULONG block = *nextblock;
LONG error;

    if (fn->filesystem)
    {
        size = first;
        while (size != fn->fsblocks)
        {
            struct LoadSegBlock *lsb = PartitionRDBFileSysBlock(fn, size);

            lsb->lsb_Next = (size+1) != fn->fsblocks ? AROS_LONG2BE(block+1) : (ULONG)-1;
#if RDB_WRITE
            error = PartitionRDBWriteRecord(PartitionBase, root, block++, lsb);
            if (error != 0)
                return error;
#else
            kprintf("RDB-write: block=%ld, type=LSEG\n", block);
            block++;
#endif
            size++;
        }
    }
    *nextblock = block;
    return 0;
}

/* Check the complete rewrite before touching any metadata or volume data. */
static LONG PartitionRDBCheckWriteSize(struct PartitionHandle *root)
{
    struct RDBData *data = root->table->data;
    struct BadBlockNode *bn;
    struct PartitionHandle *ph;
    struct FileSysNode *fn;
    struct FileSysNode *code[2] = {data->driveinit, data->bootcode};
    ULONG i;
    UQUAD end = (UQUAD)data->rdbblock + 1;
    UQUAD blocks = PartitionMulQ(root->de.de_HighCyl - root->de.de_LowCyl + 1,
        root->de.de_Surfaces, root->de.de_BlocksPerTrack);
    UQUAD first = PartitionMulQ(AROS_BE2LONG(data->rdb.rdb_LoCylinder),
        AROS_BE2LONG(data->rdb.rdb_Heads), AROS_BE2LONG(data->rdb.rdb_Sectors));
    UQUAD limit = (UQUAD)AROS_BE2LONG(data->rdb.rdb_RDBBlocksHi) + 1;

    if (data->readerror)
        return data->readerror;
    if (data->rdbblock < AROS_BE2LONG(data->rdb.rdb_RDBBlocksLo))
        return ERROR_NOT_A_DOS_DISK;
    if (limit > blocks)
        limit = blocks;
    if (limit > first)
        limit = first;

    for (bn = (struct BadBlockNode *)data->badblocklist.lh_Head;
         bn->ln.ln_Succ; bn = (struct BadBlockNode *)bn->ln.ln_Succ)
        end++;
    for (ph = (struct PartitionHandle *)root->table->list.lh_Head;
         ph->ln.ln_Succ; ph = (struct PartitionHandle *)ph->ln.ln_Succ)
    {
        UQUAD start = PartitionMulQ(ph->de.de_LowCyl, ph->de.de_Surfaces, ph->de.de_BlocksPerTrack);
        if (limit > start)
            limit = start;
        end++;
    }
    for (fn = (struct FileSysNode *)data->fsheaderlist.lh_Head;
         fn->h.ln.ln_Succ; fn = (struct FileSysNode *)fn->h.ln.ln_Succ)
    {
        if (fn->blockbytes != root->de.de_SizeBlock * 4 ||
            ((!fn->filesystem || !fn->fsblocks) && fn->fhb.fhb_SegListBlocks != (ULONG)-1))
            return ERROR_NOT_A_DOS_DISK;
        end += (UQUAD)fn->fsblocks + 1;
    }
    for (i = 0; i < 2; i++)
    {
        if (code[i])
        {
            if (!code[i]->filesystem || !code[i]->fsblocks)
                return ERROR_NOT_A_DOS_DISK;
            end += code[i]->fsblocks;
            if (i == 1 && AROS_BE2LONG(data->rdb.rdb_Reserved1[0]) < data->rdbblock)
                end--; /* its head remains before the RDB header */
        }
    }
    /* Replacement sectors contain live disk data, not free metadata space. */
    for (bn = (struct BadBlockNode *)data->badblocklist.lh_Head;
         bn->ln.ln_Succ; bn = (struct BadBlockNode *)bn->ln.ln_Succ)
    {
        ULONG *words = (ULONG *)&bn->bbb;
        ULONG count = AROS_BE2LONG(bn->bbb.bbb_SummedLongs);

        for (i = 7; i < count; i += 2)
        {
            ULONG good = AROS_BE2LONG(words[i]);
            if ((good || words[i - 1]) && good >= data->rdbblock && good < end)
                return ERROR_DISK_FULL;
        }
    }
    return end > limit ? ERROR_DISK_FULL : 0;
}

/* Write a list of single-block records as a chain; Next is longword 4. */
static LONG PartitionRDBWriteChain(struct Library *PartitionBase, struct PartitionHandle *root,
    struct List *list, ULONG *head, ULONG *block, BOOL indirect)
{
    struct Node *n = list->lh_Head;
    LONG error;

    *head = n->ln_Succ ? AROS_LONG2BE(*block) : (ULONG)-1;
    for (; n->ln_Succ; n = n->ln_Succ)
    {
        ULONG *rec = indirect ? ((struct PartitionHandle *)n)->data : (ULONG *)&((struct BadBlockNode *)n)->bbb;

        rec[4] = n->ln_Succ->ln_Succ ? AROS_LONG2BE(*block + 1) : (ULONG)-1;
        error = PartitionRDBWriteRecord(PartitionBase, root, (*block)++, rec);
        if (error != 0)
            return error;
    }
    return 0;
}

/* Returns a raw device error */
static LONG PartitionRDBWriteAll(struct Library *PartitionBase, struct PartitionHandle *root)
{
    struct RDBData *data = root->table->data;
    struct FileSysNode *fn;
    ULONG block;
    LONG error;

    block = data->rdbblock+1; /* RDB will be written at the end */

    /* Firmware may scan the early sectors rather than follow BootBlockList.
     * Pack BOOT before other metadata, retaining an earlier head in place. */
    if (data->bootcode)
    {
        ULONG head = AROS_BE2LONG(data->rdb.rdb_Reserved1[0]);

        if (head < data->rdbblock)
        {
            struct LoadSegBlock *lsb = PartitionRDBFileSysBlock(data->bootcode, 0);
            ULONG next = block;

            error = PartitionRDBWriteFileSys(PartitionBase, root, data->bootcode, &block, 1);
            if (error != 0)
                return error;
            lsb->lsb_Next = data->bootcode->fsblocks > 1 ? AROS_LONG2BE(next) : (ULONG)-1;
            error = PartitionRDBWriteRecord(PartitionBase, root, head, lsb);
        }
        else
        {
            data->rdb.rdb_Reserved1[0] = AROS_LONG2BE(block);
            error = PartitionRDBWriteFileSys(PartitionBase, root, data->bootcode, &block, 0);
        }
        if (error != 0)
            return error;
    }

    /* write bad blocks and partition blocks */
    error = PartitionRDBWriteChain(PartitionBase, root, &data->badblocklist, &data->rdb.rdb_BadBlockList, &block, FALSE);
    if (error != 0)
        return error;
    error = PartitionRDBWriteChain(PartitionBase, root, &root->table->list, &data->rdb.rdb_PartitionList, &block, TRUE);
    if (error != 0)
        return error;

    /* write filesystem blocks */
    fn = (struct FileSysNode *)data->fsheaderlist.lh_Head;
    data->rdb.rdb_FileSysHeaderList = fn->h.ln.ln_Succ ? AROS_LONG2BE(block) : (ULONG)-1;
    while (fn->h.ln.ln_Succ)
    {
    ULONG fshblock;

        fshblock = block;
        block++; /* header block will be written later */
        fn->fhb.fhb_SegListBlocks = fn->fsblocks ? AROS_LONG2BE(block) : (ULONG)-1;
        /* write filesystem LSEG blocks */
        error = PartitionRDBWriteFileSys(PartitionBase, root, fn, &block, 0);
        if (error != 0)
            return error;
        fn->fhb.fhb_Next = fn->h.ln.ln_Succ->ln_Succ ? AROS_LONG2BE(block) : (ULONG)-1;
        error = PartitionRDBWriteRecord(PartitionBase, root, fshblock, &fn->fhb);
        if (error != 0)
            return error;
        fn = (struct FileSysNode *)fn->h.ln.ln_Succ;
    }
    if (data->driveinit)
    {
        data->rdb.rdb_DriveInit = AROS_LONG2BE(block);
        error = PartitionRDBWriteFileSys(PartitionBase, root, data->driveinit, &block, 0);
        if (error != 0)
            return error;
    }
    data->rdb.rdb_HighRDSKBlock = AROS_LONG2BE(block-1);
    error = PartitionRDBWriteRecord(PartitionBase, root, data->rdbblock, &data->rdb);
    if (error != 0)
        return error;
    /* A newly created header at block 1 must not be hidden by an old one at
     * block 0. Preserve boot code/MBRs and do not clear the other probe blocks:
     * they already contain the new table's PART/FSHD/LSEG records. */
    if (data->newtable)
    {
        struct RigidDiskBlock *old = (struct RigidDiskBlock *)root->buffer;

        error = readBlock(PartitionBase, root, 0, root->buffer);
        if (error != 0)
            return error;
        if (AROS_BE2LONG(old->rdb_ID) == IDNAME_RIGIDDISK)
        {
            old->rdb_ID = 0;
            error = writeBlock(PartitionBase, root, 0, root->buffer);
            if (error != 0)
                return error;
        }
        data->newtable = FALSE;
    }
    return 0;
}

static LONG PartitionRDBWritePartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    LONG error;

    if (root->de.de_SizeBlock < 128 ||
        root->de.de_SizeBlock > sizeof(root->buffer) / 4)
        return ERROR_BAD_NUMBER;

    error = PartitionRDBCheckWriteSize(root);
    if (error != 0)
        return error;

    return deviceError(PartitionRDBWriteAll(PartitionBase, root));
}

LONG PartitionRDBCreatePartitionTable(struct Library *PartitionBase, struct PartitionHandle *ph)
{
    struct RDBData *data;
    ULONG i;
    ULONG rdbsize;

    if (!ph->dg.dg_CylSectors || ph->dg.dg_CylSectors > 0x7fffffff ||
        PartitionMulQ(ph->dg.dg_Heads, ph->dg.dg_TrackSectors, 1) != ph->dg.dg_CylSectors ||
        !ph->dg.dg_Cylinders || ph->de.de_SizeBlock < 128 ||
        ph->de.de_SizeBlock > sizeof(ph->buffer) / 4 ||
        ph->dg.dg_SectorSize != ph->de.de_SizeBlock * 4)
        return ERROR_BAD_NUMBER;

    /* Reserve two cylinders, increasing in whole-cylinder steps to at least
     * 256 blocks for flat LBA containers (Heads == TrackSectors == 1). */
    rdbsize = ph->dg.dg_CylSectors << 1;
    while (rdbsize < 256)
        rdbsize <<= 1;
    if (rdbsize >= PartitionMulQ(ph->dg.dg_Cylinders, ph->dg.dg_CylSectors, 1))
        return ERROR_DISK_FULL;

    data = AllocMem(sizeof(struct RDBData) + (ph->de.de_SizeBlock << 2) - sizeof(struct RigidDiskBlock), MEMF_PUBLIC | MEMF_CLEAR);
    if (data)
    {
        ph->table->data = data;

        data->rdb.rdb_ID                = AROS_LONG2BE(IDNAME_RIGIDDISK);
        data->rdb.rdb_SummedLongs       = AROS_LONG2BE(sizeof(struct RigidDiskBlock)/4);
        data->rdb.rdb_BlockBytes        = AROS_LONG2BE(ph->dg.dg_SectorSize);
        data->rdb.rdb_BadBlockList      = (ULONG)-1;
        data->rdb.rdb_PartitionList     = (ULONG)-1;
        data->rdb.rdb_FileSysHeaderList = (ULONG)-1;
        data->rdb.rdb_DriveInit         = (ULONG)-1;
        for (i = 0; i < 6; i++)
            data->rdb.rdb_Reserved1[i]  = (ULONG)-1;
        data->rdb.rdb_Cylinders         = AROS_LONG2BE(ph->dg.dg_Cylinders);
        data->rdb.rdb_Sectors           = AROS_LONG2BE(ph->dg.dg_TrackSectors);
        data->rdb.rdb_Heads             = AROS_LONG2BE(ph->dg.dg_Heads);

        data->rdb.rdb_Park              = data->rdb.rdb_Cylinders;
        data->rdb.rdb_WritePreComp      = data->rdb.rdb_Cylinders;
        data->rdb.rdb_ReducedWrite      = data->rdb.rdb_Cylinders;
        /* StepRate */
        data->rdb.rdb_RDBBlocksLo       = AROS_LONG2BE(1); /* leave a block for PC */
        data->rdb.rdb_RDBBlocksHi       = AROS_LONG2BE(rdbsize - 1);
        data->rdb.rdb_LoCylinder        = AROS_LONG2BE(rdbsize / ph->dg.dg_CylSectors);
        data->rdb.rdb_HiCylinder        = AROS_LONG2BE(ph->dg.dg_Cylinders-1);
        data->rdb.rdb_CylBlocks         = AROS_LONG2BE(ph->dg.dg_CylSectors);
        /* AutoParkSeconds */
        /* DiskVendor */
        /* DiskProduct */
        /* DiskRevision */
        /* ControllerVendor */
        /* ControllerProduct */
        /* ControllerRevision */

        data->rdbblock = 1;
        data->newtable = TRUE;
        NEWLIST(&data->badblocklist);
        NEWLIST(&data->fsheaderlist);
        NEWLIST(&ph->table->list);
        return 0;
    }
    return 1;
}

static LONG PartitionRDBSetPartitionAttrs(struct Library *PartitionBase, struct PartitionHandle *ph, const struct TagItem *taglist)
{
    struct TagItem *tag;
    struct PartitionBlock *data = (struct PartitionBlock *)ph->data;

    /* Validate first so an invalid tag does not leave a partially edited entry. */
    const struct TagItem *check = taglist;

    while ((tag = NextTagItem((struct TagItem **)&check)))
    {
        if (tag->ti_Tag == PT_DOSENVEC)
        {
            struct DosEnvec *de = (struct DosEnvec *)tag->ti_Data;

            if (de->de_TableSize < DE_NUMBUFFERS || de->de_TableSize >= sizeof(*de) / sizeof(IPTR))
                return ERROR_BAD_NUMBER;
        }
        if (tag->ti_Tag == PT_NAME && strlen((STRPTR)tag->ti_Data) >= sizeof(data->pb_DriveName))
            return ERROR_BAD_NUMBER;
    }

    while ((tag = NextTagItem((struct TagItem **)&taglist)))
    {
        switch (tag->ti_Tag)
        {
        case PT_DOSENVEC:
            {
            struct DosEnvec *de = (struct DosEnvec *)tag->ti_Data;

                CopyMem(de, &ph->de, (de->de_TableSize+1)*sizeof(IPTR));
                CopyHost2BEDosEnvec((SIPTR *)de, data->pb_Environment, de->de_TableSize+1);
                PartitionRDBUpdateGeometry(ph);
            }
            break;
        case PT_TYPE:
            {
            struct PartitionType *ptype=(struct PartitionType *)tag->ti_Data;
            ULONG dt;

                CopyMem(ptype->id, &dt, 4);
                ph->de.de_DosType = AROS_BE2LONG(dt);
                data->pb_Environment[DE_DOSTYPE] = dt;
            }
            break;
        case PT_NAME:
            {
            STRPTR name = (STRPTR)tag->ti_Data;
            ULONG len = strlen(name);

                CopyMem(name, ph->ln.ln_Name, len+1);
                CopyMem(name, data->pb_DriveName+1, len);
                data->pb_DriveName[0] = len;
            }
            break;
        case PT_BOOTABLE:
            if (tag->ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_BOOTABLE);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_BOOTABLE);
            break;
        case PT_AUTOMOUNT:
            if (tag->ti_Data)
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) & ~PBFF_NOMOUNT);
            else
                data->pb_Flags = AROS_LONG2BE(AROS_BE2LONG(data->pb_Flags) | PBFF_NOMOUNT);
            break;
        }
    }
    return 0;
}

struct PartitionHandle *PartitionRDBAddPartition(struct Library *PartitionBase, struct PartitionHandle *root, const struct TagItem *taglist)
{
    if (FindTagItem(PT_DOSENVEC, taglist) != NULL)
    {
        struct PartitionBlock *pblock;
        struct PartitionHandle *ph;
        struct PartitionHandle *oph;

        ph = PartitionRDBAllocHandle(root);
        if (ph)
        {
                    pblock = ph->data;
                    pblock->pb_ID = AROS_LONG2BE(IDNAME_PARTITION);
                    pblock->pb_SummedLongs = AROS_LONG2BE(sizeof(struct PartitionBlock)/4);
                    if (PartitionRDBSetPartitionAttrs(PartitionBase, ph, taglist) != 0)
                    {
                        PartitionRDBFreeHandleMem(ph);
                        return NULL;
                    }
                    oph = (struct PartitionHandle *)root->table->list.lh_Head;
                    while (oph->ln.ln_Succ)
                    {
                        if (ph->de.de_LowCyl<oph->de.de_LowCyl)
                            break;
                        oph = (struct PartitionHandle *)oph->ln.ln_Succ;
                    }
                    /* Insert behind the predecessor (head/tail sentinels included) */
                    Insert(&root->table->list, &ph->ln, oph->ln.ln_Pred);
                    return ph;
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

ULONG PartitionRDBDestroyPartitionTable
    (
        struct Library *PartitionBase,
        struct PartitionHandle *root
    )
{
struct RDBData *data;
struct RigidDiskBlock *rdb;

    if (root->de.de_SizeBlock < 128 ||
        root->de.de_SizeBlock > sizeof(root->buffer) / 4)
            return ERROR_BAD_NUMBER;

    data = root->table->data;
    CopyMem(&data->rdb, root->buffer, root->de.de_SizeBlock << 2);
    rdb = (struct RigidDiskBlock *)root->buffer;
    rdb->rdb_ID = 0;
    return deviceError(writeBlock(PartitionBase, root, data->rdbblock, root->buffer));
}

const struct PTEditFunctionTable PtEditRDB =
{
    PHPTT_RDB,
    PartitionRDBWritePartitionTable,
    PartitionRDBCreatePartitionTable,
    PartitionRDBAddPartition,
    PartitionRDBDeletePartition,
    NULL,
    PartitionRDBSetPartitionAttrs,
    PartitionRDBDestroyPartitionTable
};
