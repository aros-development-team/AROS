/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: GPT partition table handler
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
    	if ((pcpt[0].type == MBRT_GPT) && (pcpt[0].first_sector == 1))
    	{
	    /* Load the GPT header */
    	    if (!readBlock(PartitionBase, root, 1, blk))
    	    {
    	    	struct GPTHeader *hdr = blk;

		D(bug("[GPT] Header size: specified %u, expected %u\n", hdr->HeaderSize, GPT_HEADER_SIZE));

		/* Check signature and header size */
    	    	if ((!memcmp(hdr->Signature, GPT_SIGNATURE, sizeof(hdr->Signature))) && (hdr->HeaderSize >= GPT_HEADER_SIZE))
    	    	{
    	    	    /* Use zlib routine for CRC32 */
    	    	    ULONG orig_crc = hdr->HeaderCRC32;
    	    	    ULONG crc = crc32(0L, Z_NULL, 0);

		    hdr->HeaderCRC32 = 0;
    	    	    crc = crc32(crc, blk, hdr->HeaderSize);

		    D(bug("[GPT] Header CRC: calculated 0x%08X, expected 0x%08X\n", crc, orig_crc));

		    if (crc == orig_crc)
		    {
    	    	    	res = 1;
    	    	    }
    	    	}
    	    }
    	}
    }

    FreeMem(blk, root->de.de_SizeBlock << 2);
    return res;
}

static LONG PartitionGPTOpenPartitionTable(struct Library *PartitionBase, struct PartitionHandle *root)
{
    /* TODO */
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
    NULL,
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
