/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: GPT partition table handler
*/

/*
#undef DEBUG
#define DEBUG 1
#define DEBUG_UUID
*/

#include <proto/exec.h>
#include <proto/partition.h>

#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>
#include <zlib.h>

#include "partition_support.h"
#include "partitiongpt.h"
#include "partitionmbr.h"
#include "platform.h"
#include "debug.h"

struct GPTPartitionHandle
{
    struct PartitionHandle ph;	/* Public part		      */
    ULONG entrySize;		/* Size of table entry	      */
    char name[36];		/* Name in ASCII	      */
    				/* Actual table entry follows */
};

static const uuid_t GPT_Type_Unused = MAKE_UUID(0x00000000, 0x0000, 0x0000, 0x0000, 0x000000000000);
static const uuid_t GPT_Type_AROS   = MAKE_UUID(0x00000000, 0xBB67, 0x46C5, 0xAA4A, 0xF502CA018E5E);

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

static inline void uuid_from_le(uuid_t *to, uuid_t *id)
{
    to->time_low            = AROS_LE2LONG(id->time_low);
    to->time_mid            = AROS_LE2WORD(id->time_mid);
    to->time_hi_and_version = AROS_LE2WORD(id->time_hi_and_version);

    /* Do not replace it with CopyMem(), gcc optimizes this nicely */
    memcpy(&to->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

static inline BOOL uuid_cmp_le(uuid_t *leid, uuid_t *id)
{
    if (AROS_LE2LONG(leid->time_low) != id->time_low)
    	return FALSE;
    if (AROS_LE2WORD(leid->time_mid) != id->time_mid)
    	return FALSE;
    if (AROS_LE2WORD(leid->time_hi_and_version) != id->time_hi_and_version)
    	return FALSE;

    return !memcmp(&leid->clock_seq_hi_and_reserved, &id->clock_seq_hi_and_reserved, 8);
}

/* For AROS we put DOS Type ID into first four bytes of UUID (time_low). So we mask them away. */
static inline BOOL is_aros_uuid_le(uuid_t *leid)
{
    if (AROS_LE2WORD(leid->time_mid) != GPT_Type_AROS.time_mid)
    	return 0;
    if (AROS_LE2WORD(leid->time_hi_and_version) != GPT_Type_AROS.time_hi_and_version)
    	return 0;

    return !memcmp(&leid->clock_seq_hi_and_reserved, &GPT_Type_AROS.clock_seq_hi_and_reserved, 8);
}

#ifdef DEBUG_UUID

static void PRINT_LE_UUID(uuid_t *id)
{
    unsigned int i;

    bug("[GPT] UUID: 0x%08X-%04X-%04X-%02X%02X-", AROS_LE2LONG(id->time_low), AROS_LE2WORD(id->time_mid), AROS_LE2WORD(id->time_hi_and_version),
    					          id->clock_seq_hi_and_reserved, id->clock_seq_low);

    for (i = 0; i < sizeof(id->node); i++)
    	bug("%02X", id->node[i]);

    RawPutChar('\n');
}

#else

#define PRINT_LE_UUID(id)

#endif

static LONG GPTCheckHeader(struct Library *PartitionBase, struct PartitionHandle *root, struct GPTHeader *hdr, UQUAD block)
{
    /* Load the GPT header */
    if (!readBlock(PartitionBase, root, block, hdr))
    {
    	ULONG hdrSize = AROS_LE2LONG(hdr->HeaderSize);

	D(bug("[GPT] Header size: specified %u, expected %u\n", hdrSize, GPT_MIN_HEADER_SIZE));

	/* Check signature, header size, and current block number */
    	if ((!memcmp(hdr->Signature, GPT_SIGNATURE, sizeof(hdr->Signature))) &&
    	    (hdrSize >= GPT_MIN_HEADER_SIZE) && (AROS_LE2LONG(hdr->CurrentBlock) == block))
    	{
    	    /*
    	     * Use zlib routine for CRC32.
    	     * CHECKME: is it correct on bigendian machines? It should, however who knows...
    	     */
    	    ULONG orig_crc = AROS_LE2LONG(hdr->HeaderCRC32);
    	    ULONG crc = crc32(0L, Z_NULL, 0);

	    hdr->HeaderCRC32 = 0;
    	    crc = crc32(crc, (const Bytef *)hdr, hdrSize);

	    D(bug("[GPT] Header CRC: calculated 0x%08X, expected 0x%08X\n", crc, orig_crc));
	    if (crc == orig_crc)
	    	return 1;
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
    	    /* TODO: handle backup header if something is wrong in the primary one */
    	}
    }

    FreeMem(blk, root->de.de_SizeBlock << 2);
    return res;
}

static LONG GPTReadPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root, struct GPTHeader *hdr, UQUAD block)
{
    LONG res;
    LONG err = ERROR_NOT_A_DOS_DISK;

    res = readBlock(PartitionBase, root, block, hdr);
    if (!res)
    {
	struct GPTPartition *table;
        ULONG cnt       = AROS_LE2LONG(hdr->NumEntries);
    	ULONG entrysize = AROS_LE2LONG(hdr->EntrySize);
	ULONG tablesize = AROS_ROUNDUP2(entrysize * cnt, root->de.de_SizeBlock << 2);

	D(bug("[GPT] %u entries per %u bytes, %u bytes total\n", cnt, entrysize, tablesize));

	table = AllocMem(tablesize, MEMF_ANY);
    	if (!table)
    	    return ERROR_NO_FREE_STORE;

	res = readDataFromBlock(root, AROS_LE2QUAD(hdr->StartBlock), tablesize, table);
	if (!res)
	{
	    struct GPTPartition *p = table;
	    ULONG i;

	    /* TODO: Check CRC of partition table, use backup if wrong */
	    D(bug("[GPT] Adding partitions...\n"));
	    err = 0;

	    for (i = 0; i < cnt; i++)
    	    {
	    	struct GPTPartitionHandle *gph;

		PRINT_LE_UUID(&p->TypeID);
		/*
		 * Skip unused entries.
		 * Normally GPT table has 128 preallocated entries, but only first of them are used.
		 * Just in case, we allow gaps between used entries. However (tested with MacOS X Disk Utility)
		 * partition editors seem to squeeze the table and do not leave empty entries when deleting
		 * partitions in the middle of the disk.
		 */
		if (!memcmp(&p->TypeID, &GPT_Type_Unused, sizeof(uuid_t)))
		    continue;

	    	gph = AllocVec(sizeof(struct GPTPartitionHandle) + entrysize, MEMF_CLEAR);
		if (gph)
		{
		    UQUAD startblk = AROS_LE2QUAD(p->StartBlock);
		    UQUAD endblk   = AROS_LE2QUAD(p->EndBlock);

		    initPartitionHandle(root, &gph->ph, startblk, endblk - startblk + 1);

		    /* Store the whole entry and convert name into ASCII form */
		    CopyMem(p, &gph[1], entrysize);
		    FromUTF16(gph->name, p->Name, 36);

		    gph->ph.ln.ln_Name = gph->name;
		    gph->entrySize     = entrysize;

		    ADDTAIL(&root->table->list, gph);
		    D(bug("[GPT] Added partition %u (%s), handle 0x%p\n", i, gph->name, gph));
		}
		else
		{
	    	    err = ERROR_NO_FREE_STORE;
	    	    break;
	    	}

		/* Jump to next entry, skip 'entrysize' bytes */
    	    	p = (APTR)p + entrysize;
    	    }
    	}

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

    /* Cleanup if reading failed */
    if (res)
    	PartitionGPTClosePartitionTable(PartitionBase, root);

    return res;
}

static LONG PartitionGPTGetPartitionAttr(struct Library *PartitionBase, struct PartitionHandle *ph, struct TagItem *tag)
{
    struct GPTPartition *part = (APTR)ph + sizeof(struct GPTPartitionHandle);

    switch (tag->ti_Tag)
    {
    case PT_TYPE:
    	uuid_from_le((uuid_t *)tag->ti_Data, &part->TypeID);
    	PTYPE(tag->ti_Data)->id_len = sizeof(uuid_t);
        return TRUE;

    case PT_BOOTABLE:
    	/* This extra flag is valid only for AROS partitions */
    	if (is_aros_uuid_le(&part->TypeID))
    	    *((ULONG *)tag->ti_Data) = (AROS_LE2LONG(part->Flags1) & GPT_PF1_AROS_BOOTABLE) ? TRUE : FALSE;
    	else
    	    *((ULONG *)tag->ti_Data) = FALSE;
        return TRUE;

    case PT_AUTOMOUNT:
	*((ULONG *)tag->ti_Data) = (AROS_LE2LONG(part->Flags1) & GPT_PF1_NOMOUNT) ? FALSE : TRUE;
        return TRUE;

    case PT_STARTBLOCK:
	*((ULONG *)tag->ti_Data) = AROS_LE2LONG(part->StartBlock);
    	return TRUE;

    case PT_ENDBLOCK:
	*((ULONG *)tag->ti_Data) = AROS_LE2LONG(part->EndBlock);
    	return TRUE;
    }

    return 0;
}

static const struct PartitionAttribute PartitionGPTPartitionTableAttrs[]=
{
    {PTTA_TYPE,           PLAM_READ},
    {PTTA_DONE,           0}
};

static const struct PartitionAttribute PartitionGPTPartitionAttrs[]=
{
    {PTA_GEOMETRY,  PLAM_READ},
    {PTA_TYPE,      PLAM_READ},
    {PTA_POSITION,  PLAM_READ},
    {PTA_NAME,      PLAM_READ},
    {PTA_BOOTABLE,  PLAM_READ},
    {PTA_AUTOMOUNT, PLAM_READ},
    {PT_STARTBLOCK, PLAM_READ},
    {PT_ENDBLOCK,   PLAM_READ},
    {PTA_DONE, 0}
};

const struct PTFunctionTable PartitionGPT =
{
    PHPTT_GPT,
    "GPT",
    PartitionGPTCheckPartitionTable,
    PartitionGPTOpenPartitionTable,
    PartitionGPTClosePartitionTable,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PartitionGPTGetPartitionAttr,
    NULL,
    PartitionGPTPartitionTableAttrs,
    PartitionGPTPartitionAttrs,
    NULL,
    NULL
};
