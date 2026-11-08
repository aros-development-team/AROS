/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    
    Desc: GPT partition table handler
*/

/*
#undef DEBUG
#define DEBUG 1
#define DEBUG_UUID
*/

#define DREAD(x)

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

#include <string.h>

#include "partition_support.h"
#include "partition_types.h"
#include "partitiongpt.h"
#include "partitionmbr.h"
#include "platform.h"
#include "debug.h"

/* Some error code that collides with neither trackdisk.device not dos.library error codes */
#define ERROR_BAD_CRC 255

/*
 * UTF16-LE conversion.
 * Currently these are very basic routines which handle only Latin-1 character set.
 * If needed, conversion can be performed using codesets.library (but don't forget
 * that you can run early during system bootup and codesets.library won't be
 * available by that time).
 */

static void FromUTF16(char *to, char *from, ULONG len)
{
    ULONG i;
    
    for (i = 0; i < len; i++)
    {
        /* Currently we know only 7-bit ASCII characters */
        *to++ = from[0];

        if (!from[0])
            return;

        from += 2;
    }
}

static LONG GPTCheckHeader(struct Library *PartitionBase, struct PartitionHandle *root, struct GPTHeader *hdr, UQUAD block)
{
    /* Load the GPT header */
    if (!ReadPartitionDataQ(root, hdr, root->de.de_SizeBlock << 2, block))
    {
        ULONG hdrSize = AROS_LE2LONG(hdr->HeaderSize);
        UQUAD currentblk = gpt_le64(&(hdr->CurrentBlock));

        D(bug("[GPT] Header size: specified %u, expected %u\n", hdrSize, GPT_MIN_HEADER_SIZE));
        DREAD(KPrintF("[GPT] Read: Header block %llu, backup block %llu\n", currentblk, gpt_le64(&(hdr->BackupBlock))));

        /* Check signature, header size, and current block number */
        if ((!memcmp(hdr->Signature, GPT_SIGNATURE, sizeof(hdr->Signature))) &&
            (hdrSize >= GPT_MIN_HEADER_SIZE) &&
            (hdrSize <= (root->de.de_SizeBlock << 2)) && (currentblk == block))
        {
            /*
             * Use zlib routine for CRC32.
             * CHECKME: is it correct on bigendian machines? It should, however who knows...
             */
            ULONG orig_crc = AROS_LE2LONG(hdr->HeaderCRC32);
            ULONG crc;

            hdr->HeaderCRC32 = 0;
            crc = Crc32_ComputeBuf(0, hdr, hdrSize);

            D(bug("[GPT] Header CRC: calculated 0x%08X, expected 0x%08X\n", crc, orig_crc));

            return (crc == orig_crc) ? 1 : 2;
        }
    }
    return 0;
}

static LONG PartitionGPTCheckPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    APTR blk;
    LONG res = 0;

    /* GPT can be placed only in the root of the disk */
    if (root->root)
        return 0;

    blk = AllocMem(root->de.de_SizeBlock << 2, MEMF_ANY);

    if (!blk)
        return 0;

    /* First of all, we must have valid MBR stub */
    if (MBRCheckPartitionTable(PartitionBase, root, blk))
    {
        struct PCPartitionTable *pcpt = ((struct MBR *)blk)->pcpt;

        D(bug("[GPT] MBR check passed, first partition type 0x%02X, start block %u\n", pcpt[0].type, pcpt[0].first_sector));

        /* We must have partition 0 of type GPT starting at block 1 */
        if ((pcpt[0].type == MBRT_GPT) && (MBRGetLE32(&pcpt[0].first_sector) == 1))
        {
            res = GPTCheckHeader(PartitionBase, root, blk, 1);

            /* 2 is a special return code for "bad CRC" */
            if (res != 1)
            {
                /* Try to read backup header */
                UQUAD block = (UQUAD)root->dg.dg_TotalSectors - 1;

                res = GPTCheckHeader(PartitionBase, root, blk, block);

                /* There's no third backup :( */
                if (res != 1)
                    res = 0;
            }
        }
    }

    FreeMem(blk, root->de.de_SizeBlock << 2);
    return res;
}

static LONG GPTReadPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root, struct GPTHeader *hdr, UQUAD block)
{
    LONG res;
    LONG err = ERROR_NOT_A_DOS_DISK;

    DREAD(KPrintF("[GPT] Read: header block %llu\n", block));
    res = GPTCheckHeader(PartitionBase, root, hdr, block);
    if (res == 2)
        return ERROR_BAD_CRC;

    if (res == 1)
    {
        struct GPTPartition *table;
        ULONG cnt       = AROS_LE2LONG(hdr->NumEntries);
        ULONG entrysize = AROS_LE2LONG(hdr->EntrySize);
        ULONG blocksize = root->de.de_SizeBlock << 2;
        ULONG tablesize;
        UQUAD startblk, endblk;
        UQUAD dstart = gpt_le64(&hdr->DataStart), dend = gpt_le64(&hdr->DataEnd);

        if (!cnt || entrysize < sizeof(struct GPTPartition) ||
            (entrysize & (entrysize - 1)) ||
            cnt > (0xffffffffUL - (blocksize - 1)) / entrysize)
            return ERROR_NOT_A_DOS_DISK;
        tablesize = AROS_ROUNDUP2(entrysize * cnt, blocksize);
        startblk = gpt_le64(&(hdr->StartBlock));
        /* Geometry can be truncated or saturated. Validate against the
         * CRC-checked GPT layout instead of its 32-bit sector count. */
        UQUAD backup = block == 1 ? gpt_le64(&(hdr->BackupBlock)) : block;
        UQUAD arrayblocks = tablesize / blocksize;
        if (backup <= arrayblocks + 2 ||
            dstart < 2 + arrayblocks ||
            dstart > dend ||
            dend >= backup - arrayblocks ||
            startblk < 2 || startblk >= backup || arrayblocks > backup - startblk ||
            (block == 1 && startblk + arrayblocks > dstart) ||
            (block != 1 && (gpt_le64(&(hdr->BackupBlock)) != 1 ||
                startblk <= dend)))
            return ERROR_NOT_A_DOS_DISK;

        DREAD(bug("[GPT] Read: %u entries per %u bytes, %u bytes total\n", cnt, entrysize, tablesize));

        table = AllocMem(tablesize, MEMF_ANY);
        if (!table)
            return ERROR_NO_FREE_STORE;

        DREAD(KPrintF("[GPT] Read: start block %llu\n", startblk));
        res = ReadPartitionDataQ(root, table, tablesize, startblk);
        if (!res)
        {
            ULONG orig_crc = AROS_LE2LONG(hdr->PartCRC32);
            ULONG crc = Crc32_ComputeBuf(0, table, entrysize * cnt);

            D(bug("[GPT] Data CRC: calculated 0x%08X, expected 0x%08X\n", crc, orig_crc));

            if (crc == orig_crc)
            {
                struct GPTPartition *p = table;
                ULONG i;

                DREAD(bug("[GPT] Adding partitions...\n"));
                err = 0;

                for (i = 0; i < cnt; i++, p = (APTR)p + entrysize)
                {
                    struct GPTPartitionHandle *gph;

                    startblk = gpt_le64(&(p->StartBlock));
                    endblk   = gpt_le64(&(p->EndBlock));

                    /*
                     * Skip unused entries. NumEntries in the header holds total number of preallocated entries,
                     * not the number of used ones.
                     * Normally GPT table has 128 preallocated entries, but only first of them are used.
                     * Just in case, we allow gaps between used entries. However (tested with MacOS X Disk Utility)
                     * partition editors seem to squeeze the table and do not leave empty entries when deleting
                     * partitions in the middle of the disk.
                     */
                    if (!memcmp(&p->TypeID, &GPT_Type_Unused, sizeof(uuid_t)))
                        continue;

                    DREAD(PRINT_LE_UUID("Type     ", &p->TypeID));
                    DREAD(PRINT_LE_UUID("Partition", &p->PartitionID));
                    DREAD(KPrintF("[GPT] Blocks    %llu - %llu\n", startblk, endblk));
                    DREAD(KPrintF("[GPT] Flags     0x%08lX 0x%08lX\n", AROS_LE2LONG(p->Flags0), AROS_LE2LONG(p->Flags1)));
                    DREAD(KPrintF("[GPT] Offset    0x%p\n", (APTR)p - (APTR)table));

                    gph = AllocVec(sizeof(struct GPTPartitionHandle) + entrysize, MEMF_CLEAR);
                    if (gph)
                    {
                        /* Preserve entries that the 32-bit DosEnvec interface
                         * cannot represent, without probing or mounting an
                         * accidentally truncated location. */
                        if (GPTEntryValid(hdr, p, TRUE))
                        {
                            initPartitionHandle(root, &gph->ph, startblk, endblk - startblk + 1);
                            GPT_PatchDosEnvec(PartitionBase, &gph->ph, p);
                        }
                        else
                        {
                            initPartitionHandle(root, &gph->ph, 0, 1);
                            gph->ph.de.de_DosType = 0;
                        }

                        /* Store the whole entry and convert name into ASCII form */
                        CopyMem(p, &gph[1], entrysize);
                        FromUTF16(gph->name, p->Name, 36);

                        gph->ph.ln.ln_Name = gph->name;
                        gph->entrySize     = entrysize;
                        gph->position      = i;

                        ADDTAIL(&root->table->list, gph);
                        DREAD(bug("[GPT] Added partition %u (%s), handle 0x%p\n", i, gph->name, gph));
                    }
                    else
                    {
                        err = ERROR_NO_FREE_STORE;
                        break;
                    }


                }
            }
            else
                err = ERROR_BAD_CRC;
        }

        if (res)
            err = deviceError(res);
        FreeMem(table, tablesize);
    }

    return err;
}

static void PartitionGPTClosePartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    struct PartitionHandle *ph, *ph2;

    /* Free all partition entries */
    ForeachNodeSafe(&root->table->list, ph, ph2)
    {
        ClosePartitionTable(ph);
        FreeVec(ph);
    }

    FreeMem(root->table->data, root->de.de_SizeBlock<<2);
}

static LONG PartitionGPTOpenPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    LONG res;

    /*
     * The header is attached to partition table handle.
     * This allows us to write back the complete header and keep
     * data we don't know about in future GPT revisions.
     */
    root->table->data = AllocMem(root->de.de_SizeBlock << 2, MEMF_ANY);
    if (!root->table->data)
        return ERROR_NO_FREE_STORE;

    /* Read primary GPT table */
    res = GPTReadPartitionTable(PartitionBase, root, root->table->data, 1);

    if (res != 0)
    {
        /* Try the backup even if the primary header could not be read. */
        struct GPTHeader *hdr = root->table->data;
        UQUAD block = (UQUAD)root->dg.dg_TotalSectors - 1;
        if (GPTCheckHeader(PartitionBase, root, hdr, 1) == 1)
            block = gpt_le64(&(hdr->BackupBlock));

        struct PartitionHandle *ph, *next;
        ForeachNodeSafe(&root->table->list, ph, next)
        {
            Remove(&ph->ln);
            FreeVec(ph);
        }
        res = GPTReadPartitionTable(PartitionBase, root, hdr, block);

        /* There's no third backup... */
        if (res == ERROR_BAD_CRC)
            res = ERROR_NOT_A_DOS_DISK;
    }

    /* Cleanup if reading failed */
    if (res)
        PartitionGPTClosePartitionTable(PartitionBase, root);

    return res;
}

static LONG PartitionGPTGetPartitionAttr(struct Library *PartitionBase,
    struct PartitionHandle *ph, const struct TagItem *tag)
{
    struct GPTPartition *part = (APTR)ph + sizeof(struct GPTPartitionHandle);

    switch (tag->ti_Tag)
    {
    case PT_POSITION:
        *((ULONG *)tag->ti_Data) = GPTH(ph)->position;
        return TRUE;

    case PT_TYPE:
    {
        uuid_t typeid;
        uuid_from_le(&typeid, &part->TypeID);
        CopyMem(&typeid, PTYPE(tag->ti_Data)->id, sizeof(typeid));
        PTYPE(tag->ti_Data)->id_len = sizeof(uuid_t);
        return TRUE;
    }

    case PT_BOOTABLE:
        /* This extra flag is valid only for AROS partitions */
        if (is_aros_uuid_le(&part->TypeID))
            *((ULONG *)tag->ti_Data) = (AROS_LE2LONG(part->Flags1) & GPT_PF1_AROS_BOOTABLE) ? TRUE : FALSE;
        else
            *((ULONG *)tag->ti_Data) = FALSE;
        return TRUE;

    case PT_AUTOMOUNT:
        if (gpt_le64(&(part->EndBlock)) >= 0xffffffffULL ||
            ph->de.de_DosType == 0)
        {
            *((ULONG *)tag->ti_Data) = FALSE;
            return TRUE;
        }
        *((ULONG *)tag->ti_Data) = (AROS_LE2LONG(part->Flags1) & GPT_PF1_NOMOUNT) ? FALSE : TRUE;
        return TRUE;

    case PT_STARTBLOCK:
        *((UQUAD *)tag->ti_Data) = gpt_le64(&(part->StartBlock));
        return TRUE;

    case PT_ENDBLOCK:
        *((UQUAD *)tag->ti_Data) = gpt_le64(&(part->EndBlock));
        return TRUE;
    }

    return 0;
}

static LONG PartitionGPTGetPartitionTableAttr(struct Library *PartitionBase,
    struct PartitionHandle *root, const struct TagItem *tag)
{
    struct GPTHeader *hdr = root->table->data;
    switch (tag->ti_Tag)
    {
    case PTT_RESERVED:
        *((ULONG *)tag->ti_Data) = gpt_le64(&(hdr->DataStart));
        return TRUE;
    case PTT_MAX_PARTITIONS:
        *((ULONG *)tag->ti_Data) = AROS_LE2LONG(hdr->NumEntries);
        return TRUE;
    case PTT_FIRSTUSABLE:
        *((UQUAD *)tag->ti_Data) = gpt_le64(&(hdr->DataStart));
        return TRUE;
    case PTT_LASTUSABLE:
        *((UQUAD *)tag->ti_Data) = gpt_le64(&(hdr->DataEnd));
        return TRUE;
    }
    return FALSE;
}

static const struct PartitionAttribute PartitionGPTPartitionTableAttrs[]=
{
    {PTT_TYPE,           PLAM_READ},
    {PTT_RESERVED,       PLAM_READ},
    {PTT_MAX_PARTITIONS, PLAM_READ},
    {PTT_FIRSTUSABLE,    PLAM_READ},
    {PTT_LASTUSABLE,     PLAM_READ},
    {TAG_DONE,           0}
};

static const struct PartitionAttribute PartitionGPTPartitionAttrs[]=
{
    {PT_GEOMETRY,  PLAM_READ},
    {PT_DOSENVEC,  PLAM_READ|PLAM_WRITE},
    {PT_TYPE,      PLAM_READ|PLAM_WRITE},
    {PT_POSITION,  PLAM_READ},
    {PT_NAME,      PLAM_READ|PLAM_WRITE},
    {PT_BOOTABLE,  PLAM_READ|PLAM_WRITE},
    {PT_AUTOMOUNT, PLAM_READ|PLAM_WRITE},
    {PT_STARTBLOCK, PLAM_READ},
    {PT_ENDBLOCK,   PLAM_READ},
    {TAG_DONE, 0}
};

const struct PTFunctionTable PartitionGPT =
{
    PHPTT_GPT,
    "GPT",
    PartitionGPTCheckPartitionTable,
    PartitionGPTOpenPartitionTable,
    PartitionGPTClosePartitionTable,
    PartitionGPTGetPartitionTableAttr,
    PartitionGPTGetPartitionAttr,
    PartitionGPTPartitionTableAttrs,
    PartitionGPTPartitionAttrs,
    NULL
};
