/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <proto/exec.h>
#include <proto/partition.h>

#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>

#include "partition_support.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include <aros/debug.h>

#warning "FIXME: we need AROS_LE2LONG(), AROS_LE2WORD() on big endian machines"

#define MBR_MAX_PARTITIONS (4)
#define MBRT_EXTENDED      (0x05)

struct PCPartitionTable {
   UBYTE status;
   UBYTE start_head;
   UBYTE start_sector;
   UBYTE start_cylinder;
   UBYTE type;
   UBYTE end_head;
   UBYTE end_sector;
   UBYTE end_cylinder;
   ULONG first_sector;
   ULONG count_sector;
};

struct MBR {
   BYTE boot_data[0x1BE];
   struct PCPartitionTable pcpt[4];
   UWORD magic;
} __attribute__((packed));

struct MBRData {
	struct PCPartitionTable *entry;
	UBYTE position;
};

LONG PartitionMBRCheckPartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root
	)
{
struct MBR mbr;
struct MBRData *data;

	if (readBlock(PartitionBase, root, 0, &mbr) == 0)
	{
		if (
				(mbr.magic == 0xAA55) &&
				(((mbr.pcpt[0].status & 0x0F)==0) || (mbr.pcpt[0].status & 0x80)) &&
				(((mbr.pcpt[1].status & 0x0F)==0) || (mbr.pcpt[1].status & 0x80)) &&
				(((mbr.pcpt[2].status & 0x0F)==0) || (mbr.pcpt[2].status & 0x80)) &&
				(((mbr.pcpt[3].status & 0x0F)==0) || (mbr.pcpt[3].status & 0x80))
			)
		{
			if (root->root)
			{
				data = root->data;
				if (
						(root->root->table->type == PHPTT_MBR) &&
						(data->entry->type != MBRT_EXTENDED)
					)
				{
					return 0;
				}
			}
			return 1;
#warning "TODO: some more checks"
		}
	}
	return 0;
}

struct PartitionHandle *PartitionMBRNewHandle
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root,
		UBYTE position,
		struct PCPartitionTable *entry
	)
{
struct PartitionHandle *ph;
ULONG cylsecs;

	if (entry->first_sector)
	{
		ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
		if (ph)
		{
		struct MBRData *data;

			data = AllocMem(sizeof(struct MBRData), MEMF_PUBLIC);
			if (data)
			{
				cylsecs = root->de.de_BlocksPerTrack*root->de.de_Surfaces;
				data->entry = entry;
				data->position = position;
				ph->root = root;
				ph->bd = root->bd;
				ph->data = data;
				CopyMem(&root->de, &ph->de, sizeof(struct DosEnvec));
				if (
						(data->entry->first_sector % cylsecs) ||
						(data->entry->count_sector % cylsecs)
					)
				{
				ULONG r = data->entry->first_sector % cylsecs;
#warning "This has only effect on one partition!"
#warning "Changing this partition causes setting it on cyl boundary (root->de)"
					ph->de.de_Surfaces = 1;
					if (data->entry->count_sector % r)
						ph->de.de_BlocksPerTrack = 1;
					else
						ph->de.de_BlocksPerTrack = r;
					cylsecs = ph->de.de_BlocksPerTrack*ph->de.de_Surfaces;
				}
				ph->de.de_LowCyl = data->entry->first_sector/cylsecs;
				ph->de.de_HighCyl =
					ph->de.de_LowCyl+(data->entry->count_sector/cylsecs)-1;
				ph->de.de_TableSize = 10; // only until de_HighCyl
				ph->ln.ln_Pri = MBR_MAX_PARTITIONS-1-position;
				return ph;
			}
			FreeMem(ph, sizeof(struct PartitionHandle));
		}
	}
	return 0;
}

LONG PartitionMBROpenPartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root
	)
{
struct PartitionHandle *ph;
struct MBR *mbr;
UBYTE i;

	mbr = AllocMem(root->de.de_SizeBlock<<2, MEMF_PUBLIC);
	if (mbr)
	{
		if (readBlock(PartitionBase, root, 0, mbr) == 0)
		{
			NEWLIST(&root->table->list);
			root->table->data = mbr;
			for (i=0;i<4;i++)
			{
				ph = PartitionMBRNewHandle(PartitionBase, root, i, &mbr->pcpt[i]);
				if (ph)
					Enqueue(&root->table->list, &ph->ln);
			}
			return 0;
		}
		FreeMem(mbr, root->de.de_SizeBlock<<2);
	}
	return 1;
}

void PartitionMBRFreeHandle
	(
		struct Library *PartitionBase,
		struct PartitionHandle *ph
	)
{
	ClosePartitionTable(ph);
	Remove(&ph->ln);
	FreeMem(ph->data, sizeof(struct MBRData));
	FreeMem(ph, sizeof(struct PartitionHandle));
}

void PartitionMBRClosePartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root
	)
{
struct PartitionHandle *ph;

	while ((ph = (struct PartitionHandle *)RemTail(&root->table->list)))
		PartitionMBRFreeHandle(PartitionBase, ph);
	FreeMem(root->table->data, root->de.de_SizeBlock<<2);
}

LONG PartitionMBRWritePartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root
	)
{
#warning "FIXME: readBlock(0) and synchronize data"

	/* root->data = mbr is up to date */
	if (writeBlock(PartitionBase, root, 0, root->table->data))
		return 1;
	return 0;
}

LONG PartitionMBRCreatePartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *ph
	)
{
struct MBR *mbr;
UBYTE i;

	mbr = AllocMem(ph->de.de_SizeBlock<<2, MEMF_PUBLIC);
	if (mbr)
	{
		if (readBlock(PartitionBase, ph, 0, mbr) == 0)
		{
			ph->table->data = mbr;
			for (i=0;i<4;i++)
			{
				mbr->pcpt[i].status = 0;
				mbr->pcpt[i].first_sector = 0;
				mbr->pcpt[i].count_sector = 0;
			}
			mbr->magic = AROS_WORD2BE(0x55AA);
			NEWLIST(&ph->table->list);
			return 0;
		}
		FreeMem(mbr, ph->de.de_SizeBlock<<2);
	}
	return 1;
}

void PartitionMBRSetDosEnvec
	(
		struct PartitionHandle *root,
		struct PCPartitionTable *entry,
		struct DosEnvec *de
	)
{
ULONG end;
ULONG track;
ULONG cyl;

	entry->first_sector =
		de->de_LowCyl*de->de_Surfaces*de->de_BlocksPerTrack;
	entry->count_sector =
		(de->de_HighCyl-de->de_LowCyl+1)*
		de->de_Surfaces*
		de->de_BlocksPerTrack;
	track = entry->first_sector/root->de.de_BlocksPerTrack;
	cyl = track/root->de.de_Surfaces;
	if (cyl<255)
	{
		entry->start_head = track % root->de.de_Surfaces;
		entry->start_sector=(entry->first_sector % root->de.de_BlocksPerTrack)+1;
		entry->start_cylinder = cyl;
	}
	else
	{
		entry->start_head = 0xFF;
		entry->start_sector = 0xFF;
		entry->start_cylinder = 0xFF;
	}
	end = entry->first_sector+entry->count_sector;
	track = end/root->de.de_BlocksPerTrack;
	cyl = track/root->de.de_Surfaces-1;
	if (cyl<255)
	{
		entry->end_head = (track-1) % root->de.de_Surfaces;
		entry->end_sector = end - ((track-1)*root->de.de_BlocksPerTrack);
		entry->end_cylinder = cyl;
	}
	else
	{
		entry->end_head = 0xFF;
		entry->end_sector = 0xFF;
		entry->end_cylinder = 0xFF;
	}
}


struct PartitionHandle *PartitionMBRAddPartition
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root,
		struct TagItem *taglist
	)
{
struct TagItem *tag;

	tag = findTagItem(PT_DOSENVEC, taglist);
	if (tag)
	{
	struct PCPartitionTable *entry;
	struct PartitionHandle *ph;
	struct DosEnvec *de;
	UBYTE pos;

		de =  (struct DosEnvec *)tag->ti_Data;
		tag = findTagItem(PT_POSITION, taglist);
		if (tag)
		{
			pos = tag->ti_Data;
			entry = &((struct MBR *)root->table->data)->pcpt[pos];
			tag = findTagItem(PT_ACTIVE, taglist);
			if (tag)
				entry->status = tag->ti_Data ? 0x80 : 0;
			else
				entry->status = 0;
			tag = findTagItem(PT_TYPE, taglist);
			entry->type = tag ? tag->ti_Data : 0;
			PartitionMBRSetDosEnvec(root, entry, de);
			ph = PartitionMBRNewHandle(PartitionBase,	root, pos, entry);
			if (ph)
				Enqueue(&root->table->list, &ph->ln);
			else
				fillMem((BYTE *)entry, sizeof(struct PCPartitionTable), 0);
			return ph;
		}
	}
	return 0;
}

void PartitionMBRDeletePartition
	(
		struct Library *PartitionBase,
		struct PartitionHandle *ph
	)
{
struct MBRData *data;

	data = (struct MBRData *)ph->data;
	fillMem((BYTE *)data->entry, sizeof(struct PCPartitionTable *), 0);
	PartitionMBRFreeHandle(PartitionBase, ph);
}

LONG PartitionMBRGetPartitionTableAttrs
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
		case PTT_DOSENVEC:
			{
				struct DosEnvec *de = (struct DosEnvec *)taglist[0].ti_Data;
				CopyMem(&root->de, de, sizeof(struct DosEnvec));
				de->de_HighCyl -= de->de_LowCyl;
				de->de_LowCyl = 0;
			}
			break;
		case PTT_TYPE:
			*((LONG *)taglist[0].ti_Data) = root->table->type;
			break;
		case PTT_RESERVED:
			*((LONG *)taglist[0].ti_Data) =
				root->de.de_Surfaces*root->de.de_BlocksPerTrack; /* one cylinder */
			break;
		case PTT_MAX_PARTITIONS:
			*((LONG *)taglist[0].ti_Data) = MBR_MAX_PARTITIONS;
		}
		taglist++;
	}
	return 0;
}

LONG PartitionMBRSetPartitionTableAttrs
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

LONG PartitionMBRGetPartitionAttrs
	(
		struct Library *PartitionBase,
		struct PartitionHandle *ph,
		struct TagItem *taglist
	)
{

	while (taglist[0].ti_Tag != TAG_DONE)
	{
	struct MBRData *data = (struct MBRData *)ph->data;

		switch (taglist[0].ti_Tag)
		{
		case PT_DOSENVEC:
			CopyMem(&ph->de, (struct DosEnvec *)taglist[0].ti_Data, sizeof(struct DosEnvec));
			break;
		case PT_TYPE:
			*((LONG *)taglist[0].ti_Data) = (LONG)data->entry->type;
			break;
		case PT_POSITION:
			*((LONG *)taglist[0].ti_Data) = (LONG)data->position;
			break;
		case PT_ACTIVE:
			*((LONG *)taglist[0].ti_Data) = data->entry->type & 0x80 ? 1 : 0;
			break;
		}
		taglist++;
	}
	return 0;
}

LONG PartitionMBRSetPartitionAttrs
	(
		struct Library *PartitionBase,
		struct PartitionHandle *ph,
		struct TagItem *taglist
	)
{

	while (taglist[0].ti_Tag != TAG_DONE)
	{
	struct MBRData *data = (struct MBRData *)ph->data;

		switch (taglist[0].ti_Tag)
		{
		case PT_DOSENVEC:
			{
			struct DosEnvec *de;
				de = (struct DosEnvec *)taglist[0].ti_Data;
				CopyMem(de, &ph->de, sizeof(struct DosEnvec));
				PartitionMBRSetDosEnvec(ph->root, data->entry, de);
			}
			break;
		case PT_TYPE:
			data->entry->type = (UBYTE)taglist[0].ti_Data;
			break;
		case PT_POSITION:
			if (taglist[0].ti_Data != data->position)
			{
			struct PartitionHandle *node;
			struct PCPartitionTable *entry;

				node = (struct PartitionHandle *)ph->root->table->list.lh_Head;
				while (node->ln.ln_Succ)
				{
					if (taglist[0].ti_Data == ((struct MBRData *)node->data)->position)
						goto posbreak;
					node = (struct PartitionHandle *)node->ln.ln_Succ;
				}
				data->position = taglist[0].ti_Data;
				entry = &((struct MBR *)ph->root->table->data)->pcpt[data->position];
				CopyMem(data->entry, entry, sizeof(struct PCPartitionTable));
				fillMem((BYTE *)data->entry, sizeof(struct PCPartitionTable), 0);
				data->entry = entry;
				ph->ln.ln_Pri = MBR_MAX_PARTITIONS-1-data->position;
				Remove(&ph->ln);
				Enqueue(&ph->root->table->list, &ph->ln);
posbreak:
			}
			break;
		case PT_ACTIVE:
			if (taglist[0].ti_Data)
				data->entry->type |= 0x80;
			else
				data->entry->type &= ~0x80;
			break;
		}
		taglist++;
	}
	return 0;
}

ULONG PartitionMBRPartitionTableAttrs[]=
{
	PTTA_GEOMETRY,
	PTTA_TYPE,
	PTTA_RESERVED,
	PTTA_MAX_PARTITIONS,
	NULL
};

ULONG *PartitionMBRQueryPartitionTableAttrs(struct Library *PartitionBase)
{
	return PartitionMBRPartitionTableAttrs;
}

ULONG PartitionMBRPartitionAttrs[]=
{
	PTA_GEOMETRY,
	PTA_TYPE,
	PTA_POSITION,
	PTA_ACTIVE,
	NULL
};

ULONG *PartitionMBRQueryPartitionAttrs(struct Library *PartitionBase)
{
	return PartitionMBRPartitionAttrs;
}

struct PTFunctionTable PartitionMBR =
{
	PHPTT_MBR,
	"PC-MBR",
	PartitionMBRCheckPartitionTable,
	PartitionMBROpenPartitionTable,
	PartitionMBRClosePartitionTable,
	PartitionMBRWritePartitionTable,
	PartitionMBRCreatePartitionTable,
	PartitionMBRAddPartition,
	PartitionMBRDeletePartition,
	PartitionMBRGetPartitionTableAttrs,
	PartitionMBRSetPartitionTableAttrs,
	PartitionMBRGetPartitionAttrs,
	PartitionMBRSetPartitionAttrs,
	PartitionMBRQueryPartitionTableAttrs,
	PartitionMBRQueryPartitionAttrs
};

