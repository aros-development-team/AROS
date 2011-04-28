/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: GPT partition table handler
*/

/*
#undef DEBUG
#define DEBUG 1
#define DEBUG_UID
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

static const uuid_t GPT_Type_Unused = {0x00000000, 0x0000, 0x0000, 0x0000, 0x000000000000};

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

#ifdef DEBUG_UID

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

	res = readData(root, AROS_LE2QUAD(hdr->StartBlock), tablesize, table);
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
		 * Stop at the first unused entry.
		 * Normally GPT table has 128 preallocated entries, but only first of them are used.
		 * We assume that there are no gaps between used entries, and the first empty entry
		 * is a terminator.
		 * CHECKME: Is it correct ?
		 */
		if (!memcmp(&p->TypeID, &GPT_Type_Unused, sizeof(uuid_t)))
		    break;

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
		    D(bug("[GPT] Added partition %u (%s)\n", i, gph->name));
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

static const struct PartitionAttribute PartitionGPTPartitionTableAttrs[]=
{
    {PTTA_TYPE,           PLAM_READ},
    {PTTA_DONE,           0}
};

static const struct PartitionAttribute PartitionGPTPartitionAttrs[]=
{
    {PTA_GEOMETRY,  PLAM_READ},
    {PTA_TYPE,      PLAM_READ},
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
    NULL,
    NULL,
    PartitionGPTPartitionTableAttrs,
    PartitionGPTPartitionAttrs,
    NULL,
    NULL
};
