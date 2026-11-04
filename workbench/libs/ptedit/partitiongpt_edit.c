/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: GPT partition table handler, write support
*/

/*
#undef DEBUG
#define DEBUG 1
#define DEBUG_UUID
*/

#define DWRITE(x)

/*
 * Note that we use KPrintF() for debugging in some places.
 * KPrintF() uses RawDoFmt() for formatting, which (if patched by locale.library)
 * correctly supports %llu, unlike kprintf().
 * This will change when i386 port gets kernel.resource. After this kprintf()
 * will be moved to libdebug.a and rewritten to simply call KrnBug().
 */

#include <exec/memory.h>
#include <libraries/partition.h>
#include <proto/debug.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <proto/utility.h>
#include <proto/uuid.h>

#include <string.h>

#include "ptedit_intern.h"
#include "partition_types.h"
#include "partitiongpt.h"
#include "partitionmbr.h"
#include "platform.h"
#include "debug.h"

static void __attribute__((noinline)) gpt_put64(UQUAD *q, UQUAD v)
{
    ULONG *l = (ULONG *)q;
    ULONG lo = (ULONG)v, hi = (ULONG)(v >> 32);
    l[0] = AROS_LONG2LE(lo);
    l[1] = AROS_LONG2LE(hi);
}

/*
 * UTF16-LE conversion.
 * Currently these are very basic routines which handle only Latin-1 character set.
 * If needed, conversion can be performed using codesets.library (but don't forget
 * that you can run early during system bootup and codesets.library won't be
 * available by that time).
 */

static void ToUTF16(char *to, char *from, ULONG len)
{
    ULONG i;
    
    for (i = 0; i < len; i++)
    {
        /* Currently we know only 7-bit ASCII characters */
        *to++ = *from;
        *to++ = 0;

        if (!*from++)
            return;
    }
}

#ifdef NO_WRITE
#undef PartitionWriteData
#define PartitionWriteData(root, table, tablesize, blk) TDERR_WriteProt
#define PartitionWriteBlock(base, root, blk, mem) TDERR_WriteProt
#endif
#ifdef SIM_WRITE
#undef PartitionWriteData
#define PartitionWriteData(root, table, tablesize, blk) 0
#define PartitionWriteBlock(base, root, blk, mem) 0
#endif

/* The backup header location, seen from whichever header was loaded */
static UQUAD GPTBackupBlock(struct GPTHeader *hdr)
{
    UQUAD cur = gpt_le64(&hdr->CurrentBlock);

    return cur == 1 ? gpt_le64(&hdr->BackupBlock) : cur;
}

static LONG GPTWriteTable(struct Library *PartitionBase, struct PartitionHandle *root, struct GPTHeader *hdr, struct GPTPartition *table,
                           UQUAD headerblk, UQUAD backupblk, UQUAD startblk, ULONG tablesize)
{
    LONG res;
    ULONG crc;

    gpt_put64(&hdr->CurrentBlock, headerblk);
    gpt_put64(&hdr->BackupBlock, backupblk);
    gpt_put64(&hdr->StartBlock, startblk);
    hdr->HeaderCRC32  = 0;

    /* We modify the header, so we have to recalculate its CRC */
    crc = Crc32_ComputeBuf(0, hdr, AROS_LE2LONG(hdr->HeaderSize));
    hdr->HeaderCRC32 = AROS_LONG2LE(crc);
    DWRITE(bug("[GPT] New header CRC 0x%08X\n", crc));

    DWRITE(KPrintF("[GPT] Write data: start block %llu\n", startblk));
    res = PartitionWriteData(root, table, tablesize, startblk);

    DWRITE(bug("[GPT] Write result: %u\n", res));
    if (res == 0)
    {
        DWRITE(KPrintF("[GPT] Write header: start block %llu\n", headerblk));
        res = PartitionWriteData(root, hdr, root->de.de_SizeBlock << 2, headerblk);

        DWRITE(bug("[GPT] Write result: %u\n", res));
    }

    return deviceError(res);
}

/* Publish a protective MBR after both GPT copies have been written. Retain
 * boot code and an existing protective/hybrid MBR when updating a disk. */
static LONG GPTWriteProtectiveMBR(struct Library *PartitionBase, struct PartitionHandle *root)
{
    ULONG size = root->de.de_SizeBlock << 2;
    struct MBR *mbr = AllocMem(size, MEMF_CLEAR);
    LONG error;

    if (!mbr)
        return ERROR_NO_FREE_STORE;
    error = ReadPartitionDataQ(root, mbr, size, 0);
    if (!error && !(mbr->magic == AROS_WORD2LE(MBR_MAGIC) &&
        mbr->pcpt[0].type == MBRT_GPT && mbr->pcpt[0].first_sector == AROS_LONG2LE(1)))
    {
        memset(mbr->pcpt, 0, sizeof(mbr->pcpt));
        mbr->magic = AROS_WORD2LE(MBR_MAGIC);
        mbr->pcpt[0].type = MBRT_GPT;
        mbr->pcpt[0].start_sector = 2;
        mbr->pcpt[0].end_head = 0xff;
        mbr->pcpt[0].end_sector = 0xff;
        mbr->pcpt[0].end_cylinder = 0xff;
        MBRPutLE32(&mbr->pcpt[0].first_sector, 1);
        struct GPTHeader *hdr = root->table->data;
        UQUAD last = GPTBackupBlock(hdr);
        MBRPutLE32(&mbr->pcpt[0].count_sector, last > 0xffffffffULL ? 0xffffffffUL : (ULONG)last);
        error = PartitionWriteData(root, mbr, size, 0);
    }
    FreeMem(mbr, size);
    return deviceError(error);
}

static LONG PartitionGPTWritePartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    struct GPTHeader *hdr = root->table->data;
    ULONG cnt       = AROS_LE2LONG(hdr->NumEntries);
    ULONG entrysize = AROS_LE2LONG(hdr->EntrySize);
    ULONG tablesize = AROS_ROUNDUP2(entrysize * cnt, root->de.de_SizeBlock << 2);
    struct GPTPartition *table;

    DWRITE(bug("[GPT] Write: %u entries per %u bytes, %u bytes total\n", cnt, entrysize, tablesize));

    /* Allocate buffer for the whole table */
    table = AllocMem(tablesize, MEMF_CLEAR);
    if (table)
    {
        ULONG crc;
        struct GPTPartition *p = table;
        struct GPTPartitionHandle *gph;
        UQUAD backup;
        LONG res;

        /*
         * Collect our entries and build up the whole table.
         * At this point we are guaranteed to have no more entries than
         * can fit into reserved space. It's AddPartition()'s job to ensure this.
         */
        ForeachNode(&root->table->list, gph)
        {
            DWRITE(bug("[GPT] Writing partition %s, handle 0x%p\n", gph->name, gph));

            /*
             * Put our entry into the buffer.
             * Use entry's own length, because if this entry is created by AddPartition(),
             * it can be shorter than on-disk one (if someone uses extended length we don't know about).
             */
            if (gph->position >= cnt || gph->entrySize > entrysize)
            {
                FreeMem(table, tablesize);
                return ERROR_NOT_A_DOS_DISK;
            }
            p = (APTR)table + gph->position * entrysize;
            CopyMem(&gph[1], p, gph->entrySize);
            /* Nonstandard records are retained for inspection, but must not
             * let a rewrite place metadata over an existing partition. */
            if (!GPTEntryValid(hdr, p, FALSE))
            {
                FreeMem(table, tablesize);
                return ERROR_NOT_A_DOS_DISK;
            }

            DWRITE(PRINT_LE_UUID("Type     ", &p->TypeID));
            DWRITE(PRINT_LE_UUID("Partition", &p->PartitionID));
            DWRITE(KPrintF("[GPT] Blocks    %llu - %llu\n", gpt_le64(&(p->StartBlock)), gpt_le64(&(p->EndBlock))));
            DWRITE(KPrintF("[GPT] Flags     0x%08lX 0x%08lX\n", AROS_LE2LONG(p->Flags0), AROS_LE2LONG(p->Flags1)));
            DWRITE(KPrintF("[GPT] Offset    0x%p\n", (APTR)p - (APTR)table));

            /* Jump to next entry */
            p = (APTR)p + entrysize;
        }

        crc = Crc32_ComputeBuf(0, table, entrysize * cnt);
        hdr->PartCRC32 = AROS_LONG2LE(crc);
        DWRITE(bug("[GPT] New data CRC 0x%08X\n", crc));

        /* First we attempt to write a backup table. It's placed in the end. */
        backup = GPTBackupBlock(hdr);
        if (backup <= tablesize / (root->de.de_SizeBlock << 2) ||
            gpt_le64(&(hdr->DataEnd)) >= backup - tablesize / (root->de.de_SizeBlock << 2))
        {
            FreeMem(table, tablesize);
            return ERROR_NOT_A_DOS_DISK;
        }
        res = GPTWriteTable(PartitionBase, root, hdr, table, backup, 1, backup - tablesize / (root->de.de_SizeBlock << 2), tablesize);

        if (!res)
        {
            /*
             * And only if succeeded, write a primary one.
             * This gives us a chance to discard writing if something goes wrong with disk/device/whatever.
             */
            res = GPTWriteTable(PartitionBase, root, hdr, table, 1, backup, 2, tablesize);
        }

        if (!res)
            res = GPTWriteProtectiveMBR(PartitionBase, root);
        FreeMem(table, tablesize);

        return res;
    }

    return ERROR_NO_FREE_STORE;
}

/* The public DosEnvec geometry uses 32-bit sector positions. */
static BOOL GPTPartitionBounds(struct PartitionHandle *root,
    struct PartitionHandle *skip, const struct DosEnvec *de, UQUAD *first, UQUAD *end)
{
    struct GPTHeader *hdr = root->table->data;
    struct GPTPartitionHandle *other;
    UQUAD cylsize;

    if (!de || de->de_SizeBlock != root->de.de_SizeBlock ||
        !de->de_Surfaces || !de->de_BlocksPerTrack || de->de_HighCyl < de->de_LowCyl)
        return FALSE;
    cylsize = (UQUAD)de->de_Surfaces * de->de_BlocksPerTrack;
    if (de->de_HighCyl >= 0xffffffffULL ||
        cylsize > 0xffffffffULL / (de->de_HighCyl + 1))
        return FALSE;
    *first = cylsize * de->de_LowCyl;
    *end = cylsize * (de->de_HighCyl + 1) - 1;
    if (*first < gpt_le64(&(hdr->DataStart)) || *end > gpt_le64(&(hdr->DataEnd)))
        return FALSE;
    ForeachNode(&root->table->list, other)
    {
        struct GPTPartition *p = (APTR)&other[1];
        if (&other->ph != skip && *first <= gpt_le64(&(p->EndBlock)) &&
            *end >= gpt_le64(&(p->StartBlock)))
            return FALSE;
    }
    return TRUE;
}

static LONG PartitionGPTSetPartitionAttrs(struct Library *PartitionBase, struct PartitionHandle *ph, const struct TagItem *taglist)
{
    struct GPTPartition *part = (APTR)ph + sizeof(struct GPTPartitionHandle);
    struct TagItem *tag;
    struct TagItem *bootable = NULL;
    struct DosEnvec *de = (APTR)GetTagData(PT_DOSENVEC, 0, taglist);
    UQUAD first, end;
    struct GPTHeader *hdr = ph->root->table->data;

    /* Check geometry before changing any other attributes. */
    if (de && (!GPTEntryValid(hdr, part, TRUE) ||
        !GPTPartitionBounds(ph->root, ph, de, &first, &end)))
        return ERROR_BAD_NUMBER;
    if (de)
    {
        gpt_put64(&part->StartBlock, first);
        gpt_put64(&part->EndBlock, end);
        CopyMem(de, &ph->de, sizeof(*de));
        if (is_aros_uuid_le(&part->TypeID))
            part->Flags1 = (part->Flags1 & ~AROS_LONG2LE(GPT_PF1_AROS_BOOTPRI)) |
                AROS_LONG2LE((UBYTE)de->de_BootPri);
        GPT_PatchDosEnvec(PartitionBase, ph, part);
    }

    while ((tag = NextTagItem((struct TagItem **)&taglist)))
    {
        switch (tag->ti_Tag)
        {
        case PT_NAME:
            strncpy(GPTH(ph)->name, (char *)tag->ti_Data, 36);
            GPTH(ph)->name[36] = 0;
            memset(part->Name, 0, sizeof(part->Name));
            ToUTF16(part->Name, GPTH(ph)->name, 36);
            break;

        case PT_TYPE:
            /* Foolproof check */
            if (PTYPE(tag->ti_Data)->id_len == sizeof(uuid_t))
            {
                uuid_t typeid;
                CopyMem(PTYPE(tag->ti_Data)->id, &typeid, sizeof(typeid));
                uuid_to_le(&part->TypeID, &typeid);
                /* Update DOSType according to a new type ID */
                GPT_PatchDosEnvec(PartitionBase, ph, part);
            }
            break;

        case PT_BOOTABLE:
            bootable = tag;
            break;

        case PT_AUTOMOUNT:
            D(bug("[GPT] Setting automount flag to %ld\n", tag->ti_Data));
            D(bug("[GPT] Partition handle 0x%p, flags 0x%08X\n", ph, part->Flags1));

            if (tag->ti_Data)
                part->Flags1 &= ~AROS_LONG2LE(GPT_PF1_NOMOUNT);
            else
                part->Flags1 |= AROS_LONG2LE(GPT_PF1_NOMOUNT);

            D(bug("[GPT] New flags: 0x%08X\n", part->Flags1));

            break;

        /* Geometry updates are supplied through PT_DOSENVEC above. */
        }
    }

    /*
     * Now check bootable attribute.
     * It is applicable only to AROS partitions, so we check it here,
     * after possible type change.
     */
    if (bootable && is_aros_uuid_le(&part->TypeID))
    {
        if (bootable->ti_Data)
            part->Flags1 |= AROS_LONG2LE(GPT_PF1_AROS_BOOTABLE);
        else
            part->Flags1 &= ~AROS_LONG2LE(GPT_PF1_AROS_BOOTABLE);
    }

    return 0;
}

/* Creation is only used after DOS has started; boot-time probing does not
 * acquire uuid.library. Keep UUID byte order separate from the disk format. */
static BOOL GPTGenerateID(uuid_t *id)
{
    struct Library *UUIDBase = OpenLibrary("uuid.library", 0);
    uuid_t generated;

    if (!UUIDBase)
        return FALSE;
    UUID_Generate(UUID_TYPE_DCE_RANDOM, &generated);
    CloseLibrary(UUIDBase);
    uuid_to_le(id, &generated);
    return TRUE;
}

static LONG PartitionGPTCreatePartitionTable(struct Library *PartitionBase,
    struct PartitionHandle *root)
{
    ULONG blocksize = root->de.de_SizeBlock << 2;
    ULONG entries = 128, entrysize = sizeof(struct GPTPartition);
    ULONG arrayblocks;
    struct GPTHeader *hdr;
    UQUAD blocks = root->dg.dg_TotalSectors;

    /* The device geometry API reports a 32-bit sector count. Do not create
     * a backup at a guessed last sector if that count has saturated. */
    if (root->root || blocksize < 512 || (blocksize & (blocksize - 1)) ||
        !blocks || blocks == 0xffffffffULL)
        return ERROR_BAD_NUMBER;
    arrayblocks = (entries * entrysize + blocksize - 1) / blocksize;
    if (blocks <= 2 * arrayblocks + 3)
        return ERROR_DISK_FULL;
    hdr = AllocMem(blocksize, MEMF_CLEAR);
    if (!hdr)
        return ERROR_NO_FREE_STORE;
    if (!GPTGenerateID(&hdr->DiskID))
    {
        FreeMem(hdr, blocksize);
        return ERROR_INVALID_RESIDENT_LIBRARY;
    }
    CopyMem(GPT_SIGNATURE, hdr->Signature, sizeof(hdr->Signature));
    hdr->Revision = AROS_LONG2LE(0x00010000);
    hdr->HeaderSize = AROS_LONG2LE(GPT_MIN_HEADER_SIZE);
    gpt_put64(&hdr->CurrentBlock, 1);
    gpt_put64(&hdr->BackupBlock, blocks - 1);
    gpt_put64(&hdr->DataStart, 2 + arrayblocks);
    gpt_put64(&hdr->DataEnd, blocks - arrayblocks - 2);
    gpt_put64(&hdr->StartBlock, 2);
    hdr->NumEntries = AROS_LONG2LE(entries);
    hdr->EntrySize = AROS_LONG2LE(entrysize);
    root->table->data = hdr;
    NEWLIST(&root->table->list);
    return 0;
}

static struct PartitionHandle *PartitionGPTAddPartition(struct Library *PartitionBase,
    struct PartitionHandle *root, const struct TagItem *tags)
{
    struct GPTHeader *hdr = root->table->data;
    struct DosEnvec *de = (APTR)GetTagData(PT_DOSENVEC, 0, tags);
    struct PartitionType *type = (APTR)GetTagData(PT_TYPE, 0, tags);
    struct GPTPartitionHandle *gph, *other;
    struct GPTPartition *part;
    ULONG count = AROS_LE2LONG(hdr->NumEntries), position;
    IPTR requested = GetTagData(PT_POSITION, (IPTR)-1, tags);
    UQUAD first, end;
    uuid_t typeid;

    if (!type || type->id_len != sizeof(uuid_t) ||
        !GPTPartitionBounds(root, NULL, de, &first, &end))
        return NULL;
    for (position = 0; position < count; position++)
    {
        BOOL used = FALSE;
        if (requested != (IPTR)-1 && position != requested)
            continue;
        ForeachNode(&root->table->list, other)
            if (other->position == position)
                used = TRUE;
        if (!used)
            break;
    }
    if (position == count)
        return NULL;
    gph = AllocVec(sizeof(*gph) + sizeof(*part), MEMF_CLEAR);
    if (!gph)
        return NULL;
    part = (APTR)&gph[1];
    CopyMem(type->id, &typeid, sizeof(typeid));
    uuid_to_le(&part->TypeID, &typeid);
    if (!memcmp(&part->TypeID, &GPT_Type_Unused, sizeof(uuid_t)) ||
        !GPTGenerateID(&part->PartitionID))
    {
        FreeVec(gph);
        return NULL;
    }
    gpt_put64(&part->StartBlock, first);
    gpt_put64(&part->EndBlock, end);
    initPartitionHandle(root, &gph->ph, first, end - first + 1);
    gph->entrySize = sizeof(*part);
    gph->position = position;
    gph->ph.ln.ln_Name = gph->name;
    GPT_PatchDosEnvec(PartitionBase, &gph->ph, part);
    PartitionGPTSetPartitionAttrs(PartitionBase, &gph->ph, tags);
    ADDTAIL(&root->table->list, gph);
    return &gph->ph;
}

static void PartitionGPTDeletePartition(struct Library *PartitionBase,
    struct PartitionHandle *ph)
{
    Remove(&ph->ln);
    ClosePartitionTable(ph);
    FreeVec(ph);
}

static ULONG PartitionGPTDestroyPartitionTable(struct Library *PartitionBase,
    struct PartitionHandle *root)
{
    ULONG size = root->de.de_SizeBlock << 2;
    APTR zero = AllocMem(size, MEMF_CLEAR);
    LONG error;

    if (!zero)
        return ERROR_NO_FREE_STORE;
    /* GPT's table->data is a GPT header, not an MBR. Invalidate both
     * headers as well as the protective MBR so reopening cannot recover
     * the deliberately deleted table from its backup. */
    struct GPTHeader *hdr = root->table->data;
    UQUAD backup = GPTBackupBlock(hdr);
    error = PartitionWriteData(root, zero, size, backup);
    if (!error)
        error = PartitionWriteData(root, zero, size, 1);
    if (!error)
        error = PartitionWriteData(root, zero, size, 0);
    FreeMem(zero, size);
    return deviceError(error);
}

const struct PTEditFunctionTable PtEditGPT =
{
    PHPTT_GPT,
    PartitionGPTWritePartitionTable,
    PartitionGPTCreatePartitionTable,
    PartitionGPTAddPartition,
    PartitionGPTDeletePartition,
    NULL,
    PartitionGPTSetPartitionAttrs,
    PartitionGPTDestroyPartitionTable
};
