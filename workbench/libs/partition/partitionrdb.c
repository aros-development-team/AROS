/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

*/

#define RDB_WRITE 0

#include <proto/exec.h>
#include <proto/partition.h>

#include <aros/macros.h>
#include <devices/hardblocks.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>

#include "partition_support.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include <aros/debug.h>

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

ULONG calcChkSum(ULONG *ptr, ULONG size) {
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

void CopyBEDosEnvec(ULONG *src, ULONG *dst, ULONG size) {
ULONG count=0;

	while (count != size)
	{
		if (count == DE_DOSTYPE)
			*dst++ = *src++;
		else
		{
			*dst++ = AROS_BE2LONG(*src);
			src++;
		}
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
	return 0;
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
			ph->ln.ln_Name = AllocMem(32, MEMF_PUBLIC | MEMF_CLEAR);
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
					CopyBEDosEnvec(pblock->pb_Environment, (ULONG *)&ph->de, 17);
					return ph;
				}
				FreeMem(ph->ln.ln_Name, 32);
			}
			FreeMem(ph, sizeof(struct PartitionHandle));
		}
	}
	return 0;
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
	return 0;
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
UBYTE buffer[512];
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
			if (root->de.de_HighCyl != (AROS_BE2LONG(data->rdb.rdb_Cylinders)-1))
				root->de.de_HighCyl = AROS_BE2LONG(data->rdb.rdb_Cylinders)-1;
			if (root->de.de_BlocksPerTrack != AROS_BE2LONG(data->rdb.rdb_Sectors))
				root->de.de_BlocksPerTrack = AROS_BE2LONG(data->rdb.rdb_Sectors);
			if (root->de.de_Surfaces != AROS_BE2LONG(data->rdb.rdb_Heads))
				root->de.de_Surfaces = AROS_BE2LONG(data->rdb.rdb_Heads);
			/* read bad blocks */
			block = AROS_BE2LONG(data->rdb.rdb_BadBlockList);
			while (block != (ULONG)-1)
			{
			struct BadBlockNode *bn;

				if (readBlock(PartitionBase, root, block, buffer)==0)
				{
					bn = PartitionRDBNewBadBlock(PartitionBase, root, (struct BadBlockBlock *)buffer);
					if (bn)
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
					if (ph)
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
					if (fn)
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
	FreeMem(ph->ln.ln_Name, 32);
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
				return;
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
UBYTE buffer[512];
struct RDBData *data;
struct PartitionHandle *ph;
struct PartitionBlock *pblock;
struct BadBlockNode *bn;
struct FileSysNode *fn;
ULONG block;

	data = root->table->data;
	block = data->rdbblock+1; /* RDB will be written at the end */
	fillMem((UBYTE *)buffer, 512, 0);

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
//		writeBlock(PartitionBase, root, block++, 512);
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
		writeBlock(PartitionBase, root, block++, 512);
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
		fn->fhb.fhb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&fn->fhb, AROS_BE2LONG(fn->fhb.fhb_SummedLongs)));
		CopyMem(&fn->fhb, buffer, sizeof(struct FileSysHeaderBlock));
#if RDB_WRITE
//		writeBlock(PartitionBase, root, fshblock, 512);
#else
		kprintf("RDB-write: block=%ld, type=FSHD\n", block);
		block++;
#endif
		fn = (struct FileSysNode *)fn->ln.ln_Succ;
	}
	data->rdb.rdb_HighRDSKBlock = AROS_LONG2BE(block-1);
	data->rdb.rdb_ChkSum = 0;
	data->rdb.rdb_ChkSum = AROS_LONG2BE(0-calcChkSum((ULONG *)&data->rdb, AROS_BE2LONG(data->rdb.rdb_SummedLongs)));
	CopyMem(&data->rdb, buffer, sizeof(struct RigidDiskBlock));
#if RDB_WRITE
//	writeBlock(PartitionBase, root, data->rdbblock, 512);
#else
	kprintf("RDB-write: block=%ld, type=RDSK\n", data->rdbblock);
#endif
	return 1;
}

LONG PartitionRDBCreatePartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *ph
	)
{
struct RDBData *data;

	data = AllocMem(sizeof(struct RDBData), MEMF_PUBLIC | MEMF_CLEAR);
	if (data)
	{
		ph->table->data = data;
		data->rdb.rdb_ID = AROS_LONG2BE(IDNAME_RIGIDDISK);
		data->rdb.rdb_SummedLongs = AROS_LONG2BE(sizeof(struct RigidDiskBlock)/4);
		data->rdb.rdb_BlockBytes = AROS_LONG2BE(ph->de.de_SizeBlock<<2);
		data->rdb.rdb_BadBlockList = (ULONG)-1;
		data->rdb.rdb_PartitionList = (ULONG)-1;
		data->rdb.rdb_FileSysHeaderList = (ULONG)-1;
		data->rdb.rdb_Cylinders = AROS_LONG2BE(ph->de.de_HighCyl+1);
		data->rdb.rdb_Sectors = AROS_LONG2BE(ph->de.de_BlocksPerTrack);
		data->rdb.rdb_Heads = AROS_LONG2BE(ph->de.de_Surfaces);
		data->rdb.rdb_RDBBlocksLo = AROS_LONG2BE(1); /* leave a block for PC */
#warning "reserved >= 2??? blocks"
		data->rdb.rdb_RDBBlocksHi = AROS_LONG2BE(ph->de.de_Surfaces*ph->de.de_BlocksPerTrack*2); /* two cylinders */
		data->rdb.rdb_LoCylinder = AROS_LONG2BE(2);
		data->rdb.rdb_HiCylinder = AROS_LONG2BE(ph->de.de_HighCyl);
		data->rdb.rdb_CylBlocks = AROS_LONG2BE(ph->de.de_Surfaces*ph->de.de_BlocksPerTrack);
		data->rdbblock = 1;
		NEWLIST(&data->badblocklist);
		NEWLIST(&data->fsheaderlist);
		NEWLIST(&ph->table->list);
		return 0;
	}
	return 1;
}

struct PartitionHandle *PartitionRDBAddPartition
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
	struct PartitionBlock *pblock;
	struct PartitionHandle *ph;
	struct PartitionHandle *oph;
	struct DosEnvec *de;

		de = (struct DosEnvec *)tag->ti_Data;
		ph = AllocMem(sizeof(struct PartitionHandle), MEMF_PUBLIC | MEMF_CLEAR);
		if (ph)
		{
			pblock = AllocMem(sizeof(struct PartitionBlock), MEMF_PUBLIC | MEMF_CLEAR);
			if (pblock)
			{
				ph->data = pblock;
				CopyMem(de, &ph->de, sizeof(struct DosEnvec *));
				CopyBEDosEnvec((ULONG *)de, pblock->pb_Environment, 17);
				pblock->pb_ID = AROS_LONG2BE(IDNAME_PARTITION);
				pblock->pb_SummedLongs = AROS_LONG2BE(sizeof(struct PartitionBlock)/4);
				tag = findTagItem(PT_NAME, taglist);
				if (tag)
				{
				STRPTR name = (STRPTR)tag->ti_Data;
				ULONG len = strlen(name);

					CopyMem(name, ph->ln.ln_Name, len+1);
					CopyMem(name, pblock->pb_DriveName, len);
					pblock->pb_DriveName[len] = 0;
					pblock->pb_DriveName[0] = len;
				}
				oph = (struct PartitionHandle *)root->table->list.lh_Head;
				while (oph->ln.ln_Succ)
				{
					if (de->de_LowCyl<oph->de.de_LowCyl)
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
				return ph;
			}
			FreeMem(ph, sizeof(struct PartitionHandle));
		}
	}
	return 0;
}

void PartitionRDBDeletePartition
	(
		struct Library *PartitionBase,
		struct PartitionHandle *ph
	)
{

	PartitionRDBFreeHandle(PartitionBase, ph);
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
#warning "reserved >= 2??? blocks"
			*((LONG *)taglist[0].ti_Data) =
				root->de.de_Surfaces*root->de.de_BlocksPerTrack*2; /* two cylinders */
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
		case PT_DOSENVEC:
			CopyMem(&ph->de, (struct DosEnvec *)taglist[0].ti_Data, sizeof(struct DosEnvec));
			break;
		case PT_TYPE:
			{
			struct PartitionType *ptype = taglist[0].ti_Data;

				CopyMem(&ph->de.de_DosType, ptype->id, 4);
				ptype->id_len = 4;
			}
			break;
		case PT_NAME:
			CopyMem(ph->ln.ln_Name, (UBYTE *)taglist[0].ti_Data, 32);
			break;
		case PT_BOOTABLE:
			taglist[0].ti_Data = (AROS_BE2LONG(data->pb_Flags) & PBFF_BOOTABLE) ? TRUE : FALSE;
			break;
		case PT_AUTOMOUNT:
			taglist[0].ti_Data = (AROS_BE2LONG(data->pb_Flags) & PBFF_NOMOUNT) ? FALSE : TRUE;
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
			struct DosEnvec *de;
				de = (struct DosEnvec *)taglist[0].ti_Data;
				CopyMem(de, &ph->de, sizeof(struct DosEnvec));
				CopyBEDosEnvec((ULONG *)&ph->de, data->pb_Environment, 17);
			}
			break;
		case PT_TYPE:
			{
			struct PartitionType *ptype = taglist[0].ti_Data;

				CopyMem(ptype->id, &ph->de.de_DosType, 4);
				((struct DosEnvec *)&data->pb_Environment)->de_DosType = ph->de.de_DosType;
			}
			break;
		case PT_NAME:
			{
			STRPTR name = (STRPTR)taglist[0].ti_Data;
			ULONG len = strlen(name);

				CopyMem(name, ph->ln.ln_Name, len+1);
				CopyMem(name, data->pb_DriveName, len);
				data->pb_DriveName[len] = 0;
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

ULONG PartitionRDBPartitionTableAttrs[]=
{
	PTTA_GEOMETRY,
	PTTA_RESERVED,
	NULL
};

ULONG *PartitionRDBQueryPartitionTableAttrs(struct Library *PartitionBase)
{
	return PartitionRDBPartitionTableAttrs;
}

ULONG PartitionRDBPartitionAttrs[]=
{
	PTA_DOSENVEC,
	PTA_TYPE,
	PTA_NAME,
	PTA_BOOTABLE,
	PTA_AUTOMOUNT,
	NULL
};

ULONG *PartitionRDBQueryPartitionAttrs(struct Library *PartitionBase)
{
	return PartitionRDBPartitionAttrs;
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
	PartitionRDBQueryPartitionAttrs
};

