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

#include <linklibs/fatbpb.h>
#include "partition_support.h"
#include "partition_types.h"
#include "partitiongpt.h"
#include "partitionmbr.h"
#include "platform.h"
#include "debug.h"

/* Some error code that collides with neither trackdisk.device not dos.library error codes */
#define ERROR_BAD_CRC 255

struct GPTPartitionHandle
{
    struct PartitionHandle ph;  /* Public part                */
    ULONG entrySize;            /* Size of table entry        */
    ULONG position;            /* Stable index in the on-disk entry array */
    char name[37];              /* Name in ASCII              */
                                /* Actual table entry follows */
};

#define GPTH(ph) ((struct GPTPartitionHandle *)ph)

static const uuid_t GPT_Type_Unused = MAKE_UUID(0x00000000, 0x0000, 0x0000, 0x0000, 0x000000000000ULL);
static const uuid_t GPT_Type_MSBasicData = MAKE_UUID(0xEBD0A0A2, 0xB9E5, 0x4433, 0x87C0, 0x68B6B72699C7ULL);
/*
 * This is a bit special.
 * The first four bytes (time_low) hold DOS Type ID (for simple mapping),
 * so we set them to zero here. We ignore it during comparison.
 * I hope this won't create any significant problems. Even if some ID ever collides, it will
 * unlikely collide with existing DOSTypes being used, so it can be blacklisted then.
 */
static const uuid_t GPT_Type_AROS   = MAKE_UUID(0x00000000, 0xBB67, 0x46C5, 0xAA4A, 0xF502CA018E5EULL);


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

/*
 * Little-endian UUID conversion and comparison.
 * We can't use uuid.library here because it's not available during
 * system bootup. However, we are going to use it for generation.
 */
static inline void uuid_from_le(uuid_t *to, uuid_t *id)
{
    to->time_low            = AROS_LE2LONG(id->time_low);
    to->time_mid            = AROS_LE2WORD(id->time_mid);
    to->time_hi_and_version = AROS_LE2WORD(id->time_hi_and_version);

    /* Do not replace it with CopyMem(), gcc optimizes this nicely */
    memcpy(&to->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

static inline void uuid_to_le(uuid_t *to, uuid_t *id)
{
    to->time_low            = AROS_LONG2LE(id->time_low);
    to->time_mid            = AROS_WORD2LE(id->time_mid);
    to->time_hi_and_version = AROS_WORD2LE(id->time_hi_and_version);

    /* Do not replace it with CopyMem(), gcc optimizes this nicely */
    memcpy(&to->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

static inline BOOL uuid_cmp_le(uuid_t *leid, const uuid_t *id)
{
    if (AROS_LE2LONG(leid->time_low) != id->time_low)
        return FALSE;
    if (AROS_LE2WORD(leid->time_mid) != id->time_mid)
        return FALSE;
    if (AROS_LE2WORD(leid->time_hi_and_version) != id->time_hi_and_version)
        return FALSE;

    return !memcmp(&leid->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

/* For AROS we put DOS Type ID into first four bytes of UUID (time_low), so we ignore them. */
static inline BOOL is_aros_uuid_le(uuid_t *leid)
{
    if (AROS_LE2WORD(leid->time_mid) != GPT_Type_AROS.time_mid)
        return 0;
    if (AROS_LE2WORD(leid->time_hi_and_version) != GPT_Type_AROS.time_hi_and_version)
        return 0;

    return !memcmp(&leid->clock_seq_hi_and_reserved, &GPT_Type_AROS.clock_seq_hi_and_reserved, 8);
}

#ifdef DEBUG_UUID

static void PRINT_LE_UUID(char *s, uuid_t *id)
{
    unsigned int i;

    bug("[GPT] %s UUID: 0x%08X-%04X-%04X-%02X%02X-", s,
        AROS_LE2LONG(id->time_low), AROS_LE2WORD(id->time_mid), AROS_LE2WORD(id->time_hi_and_version),
        id->clock_seq_hi_and_reserved, id->clock_seq_low);

    for (i = 0; i < sizeof(id->node); i++)
        bug("%02X", id->node[i]);

    RawPutChar('\n');
}

#else

#define PRINT_LE_UUID(s, id)

#endif

#ifdef NO_WRITE
#undef WritePartitionDataQ
#define WritePartitionDataQ(root, table, tablesize, blk) TDERR_WriteProt
#define PartitionWriteBlock(base, root, blk, mem) TDERR_WriteProt
#endif
#ifdef SIM_WRITE
#undef WritePartitionDataQ
#define WritePartitionDataQ(root, table, tablesize, blk) 0
#define PartitionWriteBlock(base, root, blk, mem) 0
#endif

static ULONG GPT_ProbeFAT(struct Library *PartitionBase,
    struct PartitionHandle *ph, struct GPTPartition *p)
{
    UQUAD start = AROS_LE2QUAD(p->StartBlock);
    UQUAD end = AROS_LE2QUAD(p->EndBlock);
    UQUAD blocks;
    ULONG block_size = ph->de.de_SizeBlock << 2;
    ULONG type = 0;
    UBYTE *block;

    if (block_size < 512 || end < start || end == ~0ULL)
        return 0;

    blocks = end - start + 1;
    if (blocks > ~0ULL / block_size)
        return 0;

    block = AllocMem(block_size, MEMF_ANY);
    if (block)
    {
        struct FATBPBInfo bpb;

        if (ReadPartitionDataQ(ph, block, block_size, 0) == 0 &&
            FAT_ParseBPB(block, block_size, blocks * block_size, &bpb))
            type = bpb.dos_type;

        FreeMem(block, block_size);
    }

    return type;
}

static void GPT_PatchDosEnvec(struct Library *PartitionBase,
    struct PartitionHandle *ph, struct GPTPartition *p)
{
    ULONG type   = 0;
    LONG bootpri = 0;

    if (is_aros_uuid_le(&p->TypeID))
    {
        type    = AROS_LE2LONG(p->TypeID.time_low);
        /* This casting is needed for proper sign expansion */
        bootpri = (BYTE)(AROS_LE2LONG(p->Flags1) & GPT_PF1_AROS_BOOTPRI);
    }
    else
    {
        const struct TypeMapping *m;

        for (m = PartTypes; m->DOSType; m++)
        {
            if (m->uuid && uuid_cmp_le(&p->TypeID, m->uuid))
            {
                type = m->DOSType;
                break;
            }
        }

        if (type == 0 && uuid_cmp_le(&p->TypeID, &GPT_Type_MSBasicData))
            type = GPT_ProbeFAT(PartitionBase, ph, p);
    }
    
    setDosType(&ph->de, type);
    ph->de.de_BootPri = bootpri;
}

static LONG GPTCheckHeader(struct Library *PartitionBase, struct PartitionHandle *root, struct GPTHeader *hdr, UQUAD block)
{
    /* Load the GPT header */
    if (!ReadPartitionDataQ(root, hdr, root->de.de_SizeBlock << 2, block))
    {
        ULONG hdrSize = AROS_LE2LONG(hdr->HeaderSize);
        UQUAD currentblk = AROS_LE2QUAD(hdr->CurrentBlock);

        D(bug("[GPT] Header size: specified %u, expected %u\n", hdrSize, GPT_MIN_HEADER_SIZE));
        DREAD(KPrintF("[GPT] Read: Header block %llu, backup block %llu\n", currentblk, AROS_LE2QUAD(hdr->BackupBlock)));

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
        if ((pcpt[0].type == MBRT_GPT) && (AROS_LE2LONG(pcpt[0].first_sector) == 1))
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

        if (!cnt || entrysize < sizeof(struct GPTPartition) ||
            (entrysize & (entrysize - 1)) ||
            cnt > (0xffffffffUL - (blocksize - 1)) / entrysize)
            return ERROR_NOT_A_DOS_DISK;
        tablesize = AROS_ROUNDUP2(entrysize * cnt, blocksize);
        startblk = AROS_LE2QUAD(hdr->StartBlock);
        /* Geometry can be truncated or saturated. Validate against the
         * CRC-checked GPT layout instead of its 32-bit sector count. */
        UQUAD backup = block == 1 ? AROS_LE2QUAD(hdr->BackupBlock) : block;
        UQUAD arrayblocks = tablesize / blocksize;
        if (backup <= arrayblocks + 2 ||
            AROS_LE2QUAD(hdr->DataStart) < 2 + arrayblocks ||
            AROS_LE2QUAD(hdr->DataStart) > AROS_LE2QUAD(hdr->DataEnd) ||
            AROS_LE2QUAD(hdr->DataEnd) >= backup - arrayblocks ||
            startblk < 2 || startblk >= backup || arrayblocks > backup - startblk ||
            (block == 1 && startblk + arrayblocks > AROS_LE2QUAD(hdr->DataStart)) ||
            (block != 1 && (AROS_LE2QUAD(hdr->BackupBlock) != 1 ||
                startblk <= AROS_LE2QUAD(hdr->DataEnd))))
            return ERROR_NOT_A_DOS_DISK;

        DREAD(bug("[GPT] Read: %u entries per %u bytes, %u bytes total\n", cnt, entrysize, tablesize));

        table = AllocMem(tablesize, MEMF_ANY);
        if (!table)
            return ERROR_NO_FREE_STORE;

        startblk = AROS_LE2QUAD(hdr->StartBlock);

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

                    startblk = AROS_LE2QUAD(p->StartBlock);
                    endblk   = AROS_LE2QUAD(p->EndBlock);

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
                        if (endblk < 0xffffffffULL && endblk >= startblk &&
                            startblk >= AROS_LE2QUAD(hdr->DataStart) &&
                            endblk <= AROS_LE2QUAD(hdr->DataEnd))
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
        FreeVec(ph);

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
            block = AROS_LE2QUAD(hdr->BackupBlock);

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

static LONG GPTWriteTable(struct Library *PartitionBase, struct PartitionHandle *root, struct GPTHeader *hdr, struct GPTPartition *table,
                           UQUAD headerblk, UQUAD backupblk, UQUAD startblk, ULONG tablesize)
{
    LONG res;
    ULONG crc;

    hdr->CurrentBlock = AROS_QUAD2LE(headerblk);
    hdr->BackupBlock  = AROS_QUAD2LE(backupblk);
    hdr->StartBlock   = AROS_QUAD2LE(startblk);
    hdr->HeaderCRC32  = 0;

    /* We modify the header, so we have to recalculate its CRC */
    crc = Crc32_ComputeBuf(0, hdr, AROS_LE2LONG(hdr->HeaderSize));
    hdr->HeaderCRC32 = AROS_LONG2LE(crc);
    DWRITE(bug("[GPT] New header CRC 0x%08X\n", crc));

    DWRITE(KPrintF("[GPT] Write data: start block %llu\n", startblk));
    res = WritePartitionDataQ(root, table, tablesize, startblk);

    DWRITE(bug("[GPT] Write result: %u\n", res));
    if (res == 0)
    {
        DWRITE(KPrintF("[GPT] Write header: start block %llu\n", headerblk));
        res = WritePartitionDataQ(root, hdr, root->de.de_SizeBlock << 2, headerblk);

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
        mbr->pcpt[0].first_sector = AROS_LONG2LE(1);
        struct GPTHeader *hdr = root->table->data;
        UQUAD last = AROS_LE2QUAD(hdr->CurrentBlock) == 1 ?
            AROS_LE2QUAD(hdr->BackupBlock) : AROS_LE2QUAD(hdr->CurrentBlock);
        mbr->pcpt[0].count_sector = AROS_LONG2LE(last > 0xffffffffULL ? 0xffffffffUL : (ULONG)last);
        error = WritePartitionDataQ(root, mbr, size, 0);
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
            if (AROS_LE2QUAD(p->StartBlock) < AROS_LE2QUAD(hdr->DataStart) ||
                AROS_LE2QUAD(p->EndBlock) > AROS_LE2QUAD(hdr->DataEnd) ||
                AROS_LE2QUAD(p->EndBlock) < AROS_LE2QUAD(p->StartBlock))
            {
                FreeMem(table, tablesize);
                return ERROR_NOT_A_DOS_DISK;
            }

            DWRITE(PRINT_LE_UUID("Type     ", &p->TypeID));
            DWRITE(PRINT_LE_UUID("Partition", &p->PartitionID));
            DWRITE(KPrintF("[GPT] Blocks    %llu - %llu\n", AROS_LE2QUAD(p->StartBlock), AROS_LE2QUAD(p->EndBlock)));
            DWRITE(KPrintF("[GPT] Flags     0x%08lX 0x%08lX\n", AROS_LE2LONG(p->Flags0), AROS_LE2LONG(p->Flags1)));
            DWRITE(KPrintF("[GPT] Offset    0x%p\n", (APTR)p - (APTR)table));

            /* Jump to next entry */
            p = (APTR)p + entrysize;
        }

        crc = Crc32_ComputeBuf(0, table, entrysize * cnt);
        hdr->PartCRC32 = AROS_LONG2LE(crc);
        DWRITE(bug("[GPT] New data CRC 0x%08X\n", crc));

        /* First we attempt to write a backup table. It's placed in the end. */
        backup = AROS_LE2QUAD(hdr->CurrentBlock) == 1 ?
            AROS_LE2QUAD(hdr->BackupBlock) : AROS_LE2QUAD(hdr->CurrentBlock);
        if (backup <= tablesize / (root->de.de_SizeBlock << 2) ||
            AROS_LE2QUAD(hdr->DataEnd) >= backup - tablesize / (root->de.de_SizeBlock << 2))
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
        if (AROS_LE2QUAD(part->EndBlock) >= 0xffffffffULL ||
            ph->de.de_DosType == 0)
        {
            *((ULONG *)tag->ti_Data) = FALSE;
            return TRUE;
        }
        *((ULONG *)tag->ti_Data) = (AROS_LE2LONG(part->Flags1) & GPT_PF1_NOMOUNT) ? FALSE : TRUE;
        return TRUE;

    case PT_STARTBLOCK:
        *((UQUAD *)tag->ti_Data) = AROS_LE2QUAD(part->StartBlock);
        return TRUE;

    case PT_ENDBLOCK:
        *((UQUAD *)tag->ti_Data) = AROS_LE2QUAD(part->EndBlock);
        return TRUE;
    }

    return 0;
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
    if (*first < AROS_LE2QUAD(hdr->DataStart) || *end > AROS_LE2QUAD(hdr->DataEnd))
        return FALSE;
    ForeachNode(&root->table->list, other)
    {
        struct GPTPartition *p = (APTR)&other[1];
        if (&other->ph != skip && *first <= AROS_LE2QUAD(p->EndBlock) &&
            *end >= AROS_LE2QUAD(p->StartBlock))
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
    if (de && (AROS_LE2QUAD(part->StartBlock) < AROS_LE2QUAD(hdr->DataStart) ||
        AROS_LE2QUAD(part->EndBlock) > AROS_LE2QUAD(hdr->DataEnd) ||
        AROS_LE2QUAD(part->EndBlock) >= 0xffffffffULL ||
        AROS_LE2QUAD(part->EndBlock) < AROS_LE2QUAD(part->StartBlock) ||
        !GPTPartitionBounds(ph->root, ph, de, &first, &end)))
        return ERROR_BAD_NUMBER;
    if (de)
    {
        part->StartBlock = AROS_QUAD2LE(first);
        part->EndBlock = AROS_QUAD2LE(end);
        initPartitionHandle(ph->root, ph, first, end - first + 1);
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
    hdr->CurrentBlock = AROS_QUAD2LE(1);
    hdr->BackupBlock = AROS_QUAD2LE(blocks - 1);
    hdr->DataStart = AROS_QUAD2LE(2 + arrayblocks);
    hdr->DataEnd = AROS_QUAD2LE(blocks - arrayblocks - 2);
    hdr->StartBlock = AROS_QUAD2LE(2);
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
    part->StartBlock = AROS_QUAD2LE(first);
    part->EndBlock = AROS_QUAD2LE(end);
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
    FreeVec(ph);
}

static LONG PartitionGPTGetPartitionTableAttr(struct Library *PartitionBase,
    struct PartitionHandle *root, const struct TagItem *tag)
{
    struct GPTHeader *hdr = root->table->data;
    switch (tag->ti_Tag)
    {
    case PTT_RESERVED:
        *((ULONG *)tag->ti_Data) = AROS_LE2QUAD(hdr->DataStart);
        return TRUE;
    case PTT_MAX_PARTITIONS:
        *((ULONG *)tag->ti_Data) = AROS_LE2LONG(hdr->NumEntries);
        return TRUE;
    case PTT_FIRSTUSABLE:
        *((UQUAD *)tag->ti_Data) = AROS_LE2QUAD(hdr->DataStart);
        return TRUE;
    case PTT_LASTUSABLE:
        *((UQUAD *)tag->ti_Data) = AROS_LE2QUAD(hdr->DataEnd);
        return TRUE;
    }
    return FALSE;
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
    UQUAD backup = AROS_LE2QUAD(hdr->CurrentBlock) == 1 ?
        AROS_LE2QUAD(hdr->BackupBlock) : AROS_LE2QUAD(hdr->CurrentBlock);
    error = WritePartitionDataQ(root, zero, size, backup);
    if (!error)
        error = WritePartitionDataQ(root, zero, size, 1);
    if (!error)
        error = WritePartitionDataQ(root, zero, size, 0);
    FreeMem(zero, size);
    return deviceError(error);
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
    PartitionGPTWritePartitionTable,
    PartitionGPTCreatePartitionTable,
    PartitionGPTAddPartition,
    PartitionGPTDeletePartition,
    PartitionGPTGetPartitionTableAttr,
    NULL,
    PartitionGPTGetPartitionAttr,
    PartitionGPTSetPartitionAttrs,
    PartitionGPTPartitionTableAttrs,
    PartitionGPTPartitionAttrs,
    PartitionGPTDestroyPartitionTable,
    NULL
};
