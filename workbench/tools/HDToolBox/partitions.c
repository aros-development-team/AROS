/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <stdio.h>
#include <strings.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/partition.h>

#include <aros/debug.h>
#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include "partitions.h"
#include "gadgets.h"
#include "hdtoolbox_support.h"
#include "partitiontypes.h"
#include "ptclass.h"

extern struct TagItem pcpartitiontags[], pcpaddpartitiontags[],
		pcpdeletepartitiontags[], pcpstartcyltags[], pcpendcyltags[],
		pcptotalcyltags[], pcpsizetags[], pcpnametags[], pcpfilesystemtags[],
		pcpbootabletags[], pcpbootpritags[], pcpeditarospartitiontags[];
extern struct creategadget pcpgadgets[];
struct List del_list;


void setPCPGadgetAttrs
	(
		struct Window *mainwin,
		struct PartitionTableNode *table
	)
{

	pcpaddpartitiontags[0].ti_Data = TRUE;
	pcpdeletepartitiontags[0].ti_Data = TRUE;
	pcpeditarospartitiontags[0].ti_Data = TRUE;
	pcpstartcyltags[0].ti_Data = TRUE;
	pcpendcyltags[0].ti_Data = TRUE;
	pcptotalcyltags[0].ti_Data = TRUE;
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_ADD_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpaddpartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_DELETE_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpdeletepartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_EDIT_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpeditarospartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_STARTCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpstartcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_ENDCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpendcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_TOTALCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcptotalcyltags
	);
}

void setPartitionName(struct PartitionNode *pnode) {

	if (pnode->pos != -1)
		sprintf(pnode->ln.ln_Name, "Partition %ld", pnode->pos);
	else
	{
		sprintf
		(
			pnode->ln.ln_Name,
			"%ld-%ld",
			pnode->de.de_LowCyl,
			pnode->de.de_HighCyl
		);
	}
}

struct PartitionNode *newPartition
	(
		struct Window *mainwin,
		struct PartitionTableNode *table,
		struct PartitionHandle *partition
	)
{
struct PartitionNode *pnode;
LONG flag;

	pnode = (struct PartitionNode *)AllocMem
		(sizeof(struct PartitionNode), MEMF_PUBLIC | MEMF_CLEAR);
	if (pnode)
	{
		pnode->ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
		if (pnode->ln.ln_Name)
		{
			GetPartitionAttrsA(partition,	PT_DOSENVEC,&pnode->de, TAG_DONE);
			if (existsAttr(table->pattrlist, PTA_TYPE))
				GetPartitionAttrsA(partition,	PT_TYPE, &pnode->type, TAG_DONE);
			else
			{
				pnode->type.id[0] = 0;
				pnode->type.id_len = 0;
			}
			if (existsAttr(table->pattrlist, PTA_POSITION))
				GetPartitionAttrsA(partition,	PT_POSITION, &pnode->pos, TAG_DONE);
			else
				pnode->pos = -1L;
			if (existsAttr(table->pattrlist, PTA_NAME))
				GetPartitionAttrsA(partition,	PT_NAME, pnode->ln.ln_Name, TAG_DONE);
			else
				setPartitionName(pnode);
			if (existsAttr(table->pattrlist, PTA_BOOTABLE))
			{
				GetPartitionAttrsA(partition, PT_BOOTABLE, &flag, TAG_DONE);
				if (flag)
					pnode->flags |= PNF_BOOTABLE;
			}
			if (existsAttr(table->pattrlist, PTA_AUTOMOUNT))
			{
				GetPartitionAttrsA(partition, PT_AUTOMOUNT, &flag, TAG_DONE);
				if (flag)
					pnode->flags |= PNF_AUTOMOUNT;
			}
			if (table->maxpartitions)
			{
				pnode->ln.ln_Pri = table->maxpartitions-1-pnode->pos;
				Enqueue(&table->pl, &pnode->ln);
			}
			else
				AddTail(&table->pl, &pnode->ln);
			pnode->ph = partition;
			pnode->root = table;
			SetGadgetAttrsA
			(
				pcpgadgets[ID_PCP_PARTITION-ID_PCP_FIRST_GADGET].gadget,
				mainwin, 0, pcpartitiontags
			);
			return pnode;
		}
		FreeMem(pnode, sizeof(struct PartitionNode));
	}
	return 0;
}

void findPartitions(struct Window *mainwin, struct PartitionTableNode *table) {
struct PartitionHandle *partition;
struct PartitionNode *pnode;

	NEWLIST(&del_list);
	pcpartitiontags[0].ti_Data = (STACKIPTR)&table->pl;
	for
	(
		partition = (struct PartitionHandle *)table->ph->table->list.lh_Head;
		partition->ln.ln_Succ;
		partition = (struct PartitionHandle *)partition->ln.ln_Succ
	)
	{
		pnode = newPartition(mainwin, table, partition);
		if (pnode)
			if (findPTPH(pnode->ph))
				pnode->flags |= PNF_TABLE;
	}
}

extern struct Gadget *ptgad;

void par_Init(struct Window *win, struct PartitionTableNode *table) {

	findPartitions(win, table);
	setPCPGadgetAttrs(win, table);
	viewPartitionData(win, table, 0);
	SetGadgetAttrs(ptgad, win, NULL, PTCT_PartitionTable, table, TAG_DONE);
}

void viewDosEnvecData
	(
		struct Window *mainwin,
		struct PartitionTableNode *table,
		struct DosEnvec *de
	)
{
UBYTE str[16];
ULONG size;

	pcpaddpartitiontags[0].ti_Data = FALSE;
	pcpdeletepartitiontags[0].ti_Data = TRUE;
	pcpeditarospartitiontags[0].ti_Data = TRUE;
	pcpstartcyltags[0].ti_Data = TRUE;
	pcpstartcyltags[1].ti_Data = de->de_LowCyl;
	pcptotalcyltags[0].ti_Data = TRUE;
	pcptotalcyltags[1].ti_Data = de->de_HighCyl-de->de_LowCyl+1;
	pcpendcyltags[0].ti_Data = TRUE;
	pcpendcyltags[1].ti_Data = de->de_HighCyl;
	size =
		(
			pcptotalcyltags[1].ti_Data*
			de->de_Surfaces*
			de->de_BlocksPerTrack
		)/1024*(de->de_SizeBlock<<2);
	getSizeStr(str, size);
	pcpnametags[0].ti_Data = TRUE;
	pcpnametags[1].ti_Data = (STACKIPTR)"";
	pcpsizetags[1].ti_Data = (STACKIPTR)str;
	pcpbootabletags[0].ti_Data = TRUE;
	pcpbootpritags[0].ti_Data = TRUE;
	pcpfilesystemtags[1].ti_Data = (STACKIPTR)"";
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_ADD_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpaddpartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_DELETE_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpdeletepartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_EDIT_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpeditarospartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_STARTCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpstartcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_ENDCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpendcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_TOTALCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcptotalcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_SIZE-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpsizetags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_NAME-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpnametags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_BOOTABLE-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpbootabletags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_BOOTPRI-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpbootpritags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_FILESYSTEM-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpfilesystemtags
	);
	SetGadgetAttrs(ptgad, mainwin, NULL, PTCT_Selected, de, TAG_DONE);
}

void viewPartitionData
	(
		struct Window *mainwin,
		struct PartitionTableNode *table,
		struct PartitionNode *pn
	)
{
UBYTE str[16];
ULONG size;
ULONG disabled = pn ? FALSE : TRUE;

#warning "info if table->de_Surfaces/de_BlocksPerTrack != pn->de_Surfaces/..."
	pcpartitiontags[1].ti_Data = pn ? getNodeNum(&pn->ln) : ~0;
	pcpartitiontags[2].ti_Data = pcpartitiontags[1].ti_Data;
	pcpaddpartitiontags[0].ti_Data = TRUE;
	pcpdeletepartitiontags[0].ti_Data = disabled;
	pcpeditarospartitiontags[0].ti_Data = disabled;
	pcpstartcyltags[0].ti_Data = disabled;
	pcpstartcyltags[1].ti_Data = pn ? pn->de.de_LowCyl : 0;
	pcptotalcyltags[0].ti_Data = disabled;
	pcptotalcyltags[1].ti_Data = pn ? pn->de.de_HighCyl-pn->de.de_LowCyl+1 : 0;
	pcpendcyltags[0].ti_Data = disabled;
	pcpendcyltags[1].ti_Data = pn ? pn->de.de_HighCyl : 0;
	if (pn)
	{
		size =
			(
				pcptotalcyltags[1].ti_Data*
				pn->de.de_Surfaces*
				pn->de.de_BlocksPerTrack
			)/1024*(pn->de.de_SizeBlock<<2);
		getSizeStr(str, size);
	}
	else
		*str = 0;
	pcpsizetags[1].ti_Data = (STACKIPTR)str;
	pcpnametags[0].ti_Data = (existsAttr(table->pattrlist, PTA_NAME) && pn) ? FALSE: TRUE;
	pcpnametags[1].ti_Data = (STACKIPTR)(pn ? pn->ln.ln_Name : str);
	if (existsAttr(table->pattrlist, PTA_BOOTABLE))
	{

		pcpbootabletags[0].ti_Data = pn ? FALSE : TRUE;
		pcpbootabletags[1].ti_Data = (pn && (pn->flags & PNF_BOOTABLE)) ? TRUE : FALSE;
		pcpbootpritags[0].ti_Data = !pcpbootabletags[1].ti_Data;
		pcpbootpritags[1].ti_Data = pn ? pn->de.de_BootPri : 0;
	}
	else
	{
		pcpbootabletags[0].ti_Data = TRUE;
		pcpbootpritags[0].ti_Data = TRUE;
	}
	if (pn)
	{
	struct PartitionTypeNode *ptypenode;

		ptypenode = getPartitionTypeNode(table, &pn->type);
		pcpfilesystemtags[1].ti_Data =  (STACKIPTR)ptypenode->ln.ln_Name;
	}
	else
		pcpfilesystemtags[1].ti_Data =  (STACKIPTR)str;
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_ADD_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpaddpartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_DELETE_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpdeletepartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_EDIT_PARTITION-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpeditarospartitiontags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_STARTCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpstartcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_ENDCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpendcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_TOTALCYL-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcptotalcyltags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_SIZE-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpsizetags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_NAME-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpnametags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_BOOTABLE-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpbootabletags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_BOOTPRI-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpbootpritags
	);
	SetGadgetAttrsA
	(
		pcpgadgets[ID_PCP_FILESYSTEM-ID_PCP_FIRST_GADGET].gadget,
		mainwin,0,pcpfilesystemtags
	);
	SetGadgetAttrs(ptgad, mainwin, NULL, PTCT_Selected, pn, TAG_DONE);
}

void freePartitionNode(struct PartitionNode *pn) {

	FreeVec(pn->ln.ln_Name);
	FreeMem(pn, sizeof(struct PartitionNode));
}

void freePartitionList(struct List *partition_list) {
struct PartitionNode *pn;

	while ((pn = (struct PartitionNode *)RemHead(partition_list)))
	{
		freePartitionNode(pn);
	}
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

BOOL pcp_Ok(struct PartitionTableNode *table) {
BOOL changed = FALSE;
struct PartitionNode *pn;

	while ((pn = (struct PartitionNode *)RemHead(&del_list)))
	{
		DeletePartition(pn->ph);
		freePartitionNode(pn);
		changed = TRUE;
	}
	pn = (struct PartitionNode *)table->pl.lh_Head;
	while (pn->ln.ln_Succ)
	{
		if (pn->flags & PNF_CHANGED)
		{
			if (pn->flags & PNF_ADDED)
			{
				pn->ph = AddPartitionA
					(
						table->ph,
						PT_DOSENVEC, &pn->de,
						PT_TYPE, &pn->type,
						PT_POSITION, pn->pos,
						PT_NAME, pn->ln.ln_Name,
						TAG_DONE
					);
				changed = TRUE;
			}
			else
			{
				if (pn->flags & PNF_NEW_TABLE)
				{
					addPartitionTable(pn);
				}
				else if (pn->flags & PNF_DEL_TABLE)
				{
				struct PartitionTableNode *table;

					table = findPTPH(pn->ph);
					if (table)
					{
						Remove(&table->ln);
						freePartitionTable(table);
					}
				}
				else
				{
					SetPartitionAttrsA
					(
						pn->ph,
						PT_TYPE, &pn->type,
						PT_NAME, pn->ln.ln_Name,
						TAG_DONE
					);
					changed = TRUE;
				}
			}
			if (pn->flags & PNF_FLAGS_CHANGED)
			{
			LONG flag;

				flag = (pn->flags & PNF_BOOTABLE) ? TRUE : FALSE;
				SetPartitionAttrsA(pn->ph, PT_BOOTABLE, flag, TAG_DONE);
				flag = (pn->flags & PNF_AUTOMOUNT) ? TRUE : FALSE;
				SetPartitionAttrsA(pn->ph, PT_AUTOMOUNT, flag, TAG_DONE);
				changed = TRUE;
			}
			if (pn->flags & PNF_DE_CHANGED)
			{
				SetPartitionAttrsA(pn->ph, PT_DOSENVEC, &pn->de, TAG_DONE);
				changed = TRUE;
			}
		}
		pn = (struct PartitionNode *)pn->ln.ln_Succ;
	}
	freePartitionList(&table->pl);
	return changed;
}

void pcp_Cancel(struct PartitionTableNode *table) {

	freePartitionList(&del_list);
	freePartitionList(&table->pl);
}

BOOL validValue
	(
		struct PartitionTableNode *table,
		struct PartitionNode *current,
		ULONG value
	)
{
struct PartitionNode *pn;
ULONG spc;

	if (value<table->reserved)
		return FALSE;
	spc = table->de.de_Surfaces*table->de.de_BlocksPerTrack;
	if (value>=((table->de.de_HighCyl+1)*spc))
		return FALSE;
	pn = (struct PartitionNode *)table->pl.lh_Head;
	while (pn->ln.ln_Succ)
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
		pn = (struct PartitionNode *)pn->ln.ln_Succ;
	}
	return TRUE;
}

void changeStartCyl
	(
		struct Window *mainwin,
		struct PartitionTableNode *table,
		struct PartitionNode *pn,
		ULONG value
	)
{
ULONG block;

	block = value*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack;
	if (validValue(table, pn, block))
	{
		/* set new starting sector */
		pn->de.de_LowCyl = value;
		pn->flags |= PNF_CHANGED | PNF_DE_CHANGED;
	}
	viewPartitionData(mainwin, table, pn);
}

void changeEndCyl
	(
		struct Window *mainwin,
		struct PartitionTableNode *table,
		struct PartitionNode *pn,
		ULONG value
	)
{
ULONG block;

	if (value>=pn->de.de_LowCyl)
	{
		if (value != pn->de.de_HighCyl)
		{
			block = ((value+1)*pn->de.de_Surfaces*pn->de.de_BlocksPerTrack)-1;
			if (validValue(table, pn, block))
			{
				pn->de.de_HighCyl = value;
				pn->flags |= PNF_CHANGED | PNF_DE_CHANGED;
			}
		}
	}
	viewPartitionData(mainwin, table, pn);
}

void changeTotalCyl
	(
		struct Window *mainwin,
		struct PartitionTableNode *table,
		struct PartitionNode *pn,
		ULONG value
	)
{
ULONG block;

	if (value != (pn->de.de_HighCyl-pn->de.de_LowCyl+1))
	{
		block =
			(
				(pn->de.de_LowCyl+value)*
				pn->de.de_Surfaces*pn->de.de_BlocksPerTrack
			)-1;
		if (validValue(table, pn, block))
		{
			pn->de.de_HighCyl = pn->de.de_LowCyl+value-1;
			pn->flags |= PNF_CHANGED | PNF_DE_CHANGED;
		}
	}
	viewPartitionData(mainwin, table, pn);
}

void deletePartition(struct Window *mainwin, struct PartitionNode *pn) {
struct PartitionTableNode *table;

	table = pn->root;
	Remove(&pn->ln);
	if (pn->flags & PNF_ADDED)
		freePartitionNode(pn);
	else
		Enqueue(&del_list, &pn->ln);
	viewPartitionData(mainwin, table, 0);
}

ULONG getPartitionPos(struct PartitionTableNode *table, struct List *plist) {
struct PartitionNode *pn;
struct Node *node;
struct List list;
ULONG pos;
ULONG retval;

	NEWLIST(&list);
	pn = (struct PartitionNode *)plist->lh_Head;
	for (pos=0;pos<table->maxpartitions;pos++)
	{
		if ((pn->ln.ln_Succ) && (pos == pn->pos))
		{
			pn = (struct PartitionNode *)pn->ln.ln_Succ;
		}
		else
		{
			node = AllocMem(sizeof(struct Node), MEMF_PUBLIC | MEMF_CLEAR);
			if (node)
			{
				node->ln_Name = AllocMem(10, MEMF_PUBLIC | MEMF_CLEAR);
				if (node->ln_Name)
				{
					sprintf(node->ln_Name, "%ld", pos);
					node->ln_Type = pos;
					AddTail(&list, node);
					continue;
				}
				FreeMem(node, sizeof(struct Node));
			}
		}
	}
	retval = RequestList(&list, &pos);
	if (retval)
	{
		node = getNumNode(&list, pos);
		pos = node->ln_Type;
	}
	while ((node = RemTail(&list)))
	{
		FreeMem(node->ln_Name, 10);
		FreeMem(node, sizeof(struct Node));
	}
	if (retval)
		return pos;
	else if (retval == 0)
		return -1;
	else
		return -2;
}

struct PartitionNode *addPartition
	(
		struct Window *mainwin,
		struct PartitionTableNode *table,
		struct DosEnvec *de
	)
{
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox",
		0,
		"Cancel"
	};

struct PartitionNode *pn;
ULONG pos;

	if (table->maxpartitions)
	{
	ULONG count;

		count = countNodes(&table->pl);
		if ((count+1) == table->maxpartitions)
		{
			count = 0;
			pn = (struct PartitionNode *)table->pl.lh_Head;
			while (pn->ln.ln_Succ)
			{
				if (count!=pn->pos)
				{
					pos = count;
					break;
				}
				count++;
				pn = (struct PartitionNode *)pn->ln.ln_Succ;
			}
			if (pn->ln.ln_Succ == 0)
				pos = count;
		}
		else if (count<table->maxpartitions)
		{
			pos = getPartitionPos(table, &table->pl);
			if (pos == -2)
			{
				es.es_TextFormat = "Error opening Requester";
				EasyRequestArgs(0, &es, 0, 0);
				return 0;
			}
			else if (pos == -1)
				return 0;
		}
		else
		{
			es.es_TextFormat = "Sorry, partition table full";
			EasyRequestArgs(0, &es, 0, 0);
			return 0;
		}
	}
	else
		pos = 0;


	pn = AllocMem(sizeof(struct PartitionNode), MEMF_PUBLIC | MEMF_CLEAR);
	if (pn)
	{
		pn->ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
		if (pn->ln.ln_Name)
		{
			pn->pos = pos;
			pn->ln.ln_Pri = table->maxpartitions-1-pos;
			CopyMem(de, &pn->de, sizeof(struct DosEnvec));
			pn->de.de_TableSize = DE_DOSTYPE;
			pn->de.de_MaxTransfer = 0xFFFFFF;
			pn->de.de_Mask = 0xFFFFFFFE;
			pn->de.de_Reserved = 2;
			pn->de.de_BufMemType = MEMF_ANY;
			if (existsAttr(table->pattrlist, PTA_NAME))
				strcpy(pn->ln.ln_Name, "DH0");
			else
				setPartitionName(pn);
			Enqueue(&table->pl, &pn->ln);
			SetGadgetAttrsA
			(
				pcpgadgets[ID_PCP_PARTITION-ID_PCP_FIRST_GADGET].gadget,
				mainwin, 0, pcpartitiontags
			);
			viewPartitionData(mainwin, table, pn);
			pn->flags |= PNF_CHANGED | PNF_ADDED;
			pn->root = table;
			return pn;
			FreeVec(pn->ln.ln_Name);
		}
		FreeMem(pn, sizeof(struct PartitionNode));
	}
	return 0;
}

void changeBootPri(struct Window *win, struct PartitionNode *pn, LONG val) {

	if ((val<=127) && (val>=-128))
	{
		pn->de.de_BootPri = val;
		pn->flags |= PNF_CHANGED | PNF_DE_CHANGED;
	}
	viewPartitionData(win, pn->root, pn);
}

void changeName(struct Window *win, struct PartitionNode *pn, STRPTR name) {

	strcpy(pn->ln.ln_Name, name);
	pn->flags |= PNF_CHANGED;
	viewPartitionData(win, pn->root, pn);
}

