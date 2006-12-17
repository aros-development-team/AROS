/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <stdio.h>
#include <strings.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/partition.h>

#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include "partitions.h"
#include "hdtoolbox_support.h"
#include "platform.h"
#include "prefs.h"

#include "debug.h"

void setPartitionName(struct HDTBPartition *pnode) {

	if (pnode->pos != -1)
		sprintf(pnode->listnode.ln.ln_Name, "Partition %ld", pnode->pos);
	else
	{
		sprintf
		(
			pnode->listnode.ln.ln_Name,
			"%ld-%ld",
			pnode->de.de_LowCyl,
			pnode->de.de_HighCyl
		);
	}
}

struct HDTBPartition *newPartition
	(
		struct ListNode *parent,
		struct HDTBPartition *partition
	)
{
struct HDTBPartition *pn;

	pn = AllocMem(sizeof(struct HDTBPartition), MEMF_PUBLIC | MEMF_CLEAR);
	if (pn)
	{
		pn->listnode.ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
		if (pn->listnode.ln.ln_Name)
		{
			if (InitListNode(&pn->listnode, parent))
			{
				pn->listnode.ln.ln_Type = LNT_Partition;
				pn->root = partition;
				return pn;
			}
			FreeVec(pn->listnode.ln.ln_Name);
		}
		FreeMem(pn, sizeof(struct HDTBPartition));
	}
	return NULL;
}

void findPartitions(struct ListNode *parent, struct HDTBPartition *partition) {
struct PartitionHandle *ph;
struct HDTBPartition *pn;
LONG flag;

	for
		(
			ph = (struct PartitionHandle *)partition->ph->table->list.lh_Head;
			ph->ln.ln_Succ;
			ph = (struct PartitionHandle *)ph->ln.ln_Succ
		)
	{
		pn = newPartition(parent, partition);
		if (pn != NULL)
		{
			pn->ph = ph;
			GetPartitionAttrsA
			(
				pn->ph,
				PT_GEOMETRY, &pn->dg,
				PT_DOSENVEC, &pn->de,
				TAG_DONE
			);
			if (getAttrInfo(pn->root->table->pattrlist, PTA_TYPE) & PLAM_READ)
				GetPartitionAttrsA(pn->ph, PT_TYPE, &pn->type, TAG_DONE);
			else
			{
				pn->type.id[0] = 0;
				pn->type.id_len = 0;
			}
			if (getAttrInfo(pn->root->table->pattrlist, PTA_POSITION) & PLAM_READ)
				GetPartitionAttrsA(pn->ph, PT_POSITION, &pn->pos, TAG_DONE);
			else
				pn->pos = -1L;
			if (getAttrInfo(pn->root->table->pattrlist, PTA_NAME) & PLAM_READ)
				GetPartitionAttrsA(pn->ph, PT_NAME, pn->listnode.ln.ln_Name, TAG_DONE);
			else
				setPartitionName(pn);
			if (getAttrInfo(pn->root->table->pattrlist, PTA_BOOTABLE) & PLAM_READ)
			{
				GetPartitionAttrsA(pn->ph, PT_BOOTABLE, &flag, TAG_DONE);
				if (flag)
					pn->flags |= PNF_BOOTABLE;
			}
			if (getAttrInfo(pn->root->table->pattrlist, PTA_AUTOMOUNT) & PLAM_READ)
			{
				GetPartitionAttrsA(pn->ph, PT_AUTOMOUNT, &flag, TAG_DONE);
				if (flag)
					pn->flags |= PNF_AUTOMOUNT;
			}
			if (getAttrInfo(pn->root->table->pattrlist, PTA_ACTIVE) & PLAM_READ)
			{
				GetPartitionAttrsA(pn->ph, PT_ACTIVE, &flag, TAG_DONE);
				if (flag)
					pn->flags |= PNF_ACTIVE;
			}
			if (pn->root->table->max_partitions)
			{
				pn->listnode.ln.ln_Pri=pn->root->table->max_partitions-1-pn->pos;
				Enqueue(&parent->list, &pn->listnode.ln);
			}
			else
				AddTail(&parent->list, &pn->listnode.ln);
			if (findPartitionTable(pn))
			{
				findPartitions(&pn->listnode, pn);
				pn->listnode.flags |= LNF_Listable;
			}
		}
	}
}

void freePartitionNode(struct HDTBPartition *node) {

	if (node->table)
	{
		freePartitionList(&node->listnode.list);
		freePartitionTable(node);
	}
	UninitListNode(&node->listnode);
	FreeVec(node->listnode.ln.ln_Name);
	FreeMem(node, sizeof(struct HDTBPartition));
}

void freePartitionList(struct List *list) {
struct HDTBPartition *node;
struct HDTBPartition *next;

	node = (struct HDTBPartition *)list->lh_Head;
	while (node->listnode.ln.ln_Succ)
	{
		next = (struct HDTBPartition *)node->listnode.ln.ln_Succ;
		if (node->listnode.ln.ln_Type != LNT_Parent)
		{
			Remove(&node->listnode.ln);
			freePartitionNode(node);
		}
		node = next;
	}
}

BOOL validValue
	(
		struct HDTBPartition *table,
		struct HDTBPartition *current,
		ULONG value
	)
{
struct HDTBPartition *pn;
ULONG spc;

	if (value<table->table->reserved)
		return FALSE;
	spc = table->dg.dg_Heads*table->dg.dg_TrackSectors;
	if (value>=(table->dg.dg_Cylinders*spc))
		return FALSE;
	pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
	while (pn->listnode.ln.ln_Succ)
	{
		/* dont't check currently processed partition */
		if (current != pn)
		{
			spc = pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
			if (
					(value >= (pn->de.de_LowCyl*spc)) &&
					(value <  (((pn->de.de_HighCyl+1)*spc)-1))
				)
				return FALSE;
		}
		pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
	}
	return TRUE;
}

struct PartitionHandle *AddPartitionA
	(
		struct PartitionHandle *root,
		LONG tag,
		...
	)
{

	return AddPartition(root, (struct TagItem *)&tag);
}

struct HDTBPartition *addPartition
	(
		struct HDTBPartition *table,
		struct DosEnvec *de
	)
{
struct HDTBPartition *partition;
struct HDTBPartition *pn;
struct TableTypeNode *ttn;

	partition = newPartition(&table->listnode, table);
	if (partition)
	{
		if (table->table->max_partitions)
		{
			/* find first free position */
			ULONG count = 0;
			pn = (struct HDTBPartition *)table->listnode.list.lh_Head;
			while (pn->listnode.ln.ln_Succ)
			{
				if (pn->listnode.ln.ln_Type != LNT_Parent)
				{
					if (count!=pn->pos)
					{
						partition->pos = count;
						break;
					}
					count++;
				}
				pn = (struct HDTBPartition *)pn->listnode.ln.ln_Succ;
			}
			if (pn->listnode.ln.ln_Succ == NULL)
				partition->pos = count;
			partition->listnode.ln.ln_Pri = table->table->max_partitions-1-partition->pos;
			Enqueue(&table->listnode.list, &partition->listnode.ln);
		}
		else
			AddTail(&table->listnode.list, &partition->listnode.ln);
		de->de_TableSize = DE_DOSTYPE;
		de->de_MaxTransfer = 0xFFFFFF;
		de->de_Mask = 0xFFFFFFFE;
		de->de_Reserved = 2;
		de->de_BufMemType = MEMF_ANY;
		CopyMem(de, &partition->de, sizeof(struct DosEnvec));
		ttn = findTableTypeNode(table->table->type);
		CopyMem(&ttn->defaulttype, &partition->type, sizeof(struct PartitionType));
		if (getAttrInfo(table->table->pattrlist, PTA_NAME) & PLAM_WRITE)
			strcpy(partition->listnode.ln.ln_Name, "DH0");
		else
			setPartitionName(partition);
		partition->ph = AddPartitionA
		(
			table->ph,
			PT_DOSENVEC, de,
			PT_TYPE, &partition->type,
			PT_NAME, partition->listnode.ln.ln_Name,
			PT_POSITION, partition->pos,
			TAG_DONE
		);
		if (partition->ph)
		{
			/* we did not set GEOMETRY so partitionlib did it */
			GetPartitionAttrsA
			(
				partition->ph,
				PT_GEOMETRY, &partition->dg,
				TAG_DONE
			);
			/* Update DOS type in local DOSEnvec */
			GetPartitionAttrsA(partition->ph, PT_DOSENVEC, &partition->de, TAG_DONE);
			return partition;
		}
		freePartitionNode(partition);
	}
	return NULL;
}

