/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define AROS_ALMOST_COMPATIBLE

#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/partition.h>

#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <libraries/partition.h>

#include "partitiontables.h"
#include "gadgets.h"
#include "hdtoolbox_support.h"

#define DEBUG 1
#include <aros/debug.h>

struct List hd_list;
struct List pt_list;
extern struct TagItem hdtags[], cdttags[], mbbltags[], llftags[],
                      pdtags[], vdodtags[], sctdtags[];

extern struct creategadget maingadgets[];

void findHDs(char *device, int maxUnits) {
struct HDUnitNode *node;
int i,count;

	for (count=0;(device[count]!='.') && (device[count]!=0);count++);
	for (i=0;i<maxUnits;i++)
	{
		node =
			(struct HDUnitNode *)AllocMem
				(sizeof(struct HDUnitNode),	MEMF_PUBLIC | MEMF_CLEAR);
		if (node)
		{
			node->ln.ln_Name = AllocVec(count+1, MEMF_PUBLIC | MEMF_CLEAR);
			if (node->ln.ln_Name)
			{
				sprintf(node->ln.ln_Name, "%.*s", count, device);
				node->unit = i;
				node->root = OpenRootPartition(device, i);
				if (node->root)
				{
					AddTail(&hd_list, &node->ln);
					continue;
				}
				FreeVec(node->ln.ln_Name);
			}
			FreeMem(node, sizeof(struct HDUnitNode));
		}
	}
}

struct PartitionTableNode *findPTPH(struct PartitionHandle *ph) {
struct PartitionTableNode *table;

	table = (struct PartitionTableNode *)pt_list.lh_Head;
	while (table->ln.ln_Succ)
	{
		if (table->ph == ph)
			return table;
		table = (struct PartitionTableNode *)table->ln.ln_Succ;
	}
	return 0;
}

void addPartitionTable(struct PartitionNode *pn) {
struct PartitionTableNode *table;
struct PartitionTableNode *ptn;
struct PartitionNode *node;
UWORD count;
UWORD num;

	table = pn->root;
	num = getNodeNum(&pn->ln);
	/*
		where to add new table: right after "table"
		or is there another subtable in "table"?
		If so, add new table before/after it.
	*/
	ptn = (struct PartitionTableNode *)table->ln.ln_Succ;
	while (ptn->ln.ln_Succ)
	{
		count = 0;
		node = (struct PartitionNode *)table->pl.lh_Head;
		while (node->ln.ln_Succ)
		{
			if (node->ph == ptn->ph)
			{
				if (num == count)
					return; /* it was already in the list */
				if (num>count)
					break;  /* that's the one we want */
			}
			node = (struct PartitionNode *)node->ln.ln_Succ;
			count++;
		}
		if (node->ln.ln_Succ)
			break; /* count>num */
		ptn = (struct PartitionTableNode *)ptn->ln.ln_Succ;
	}
	if (ptn->ln.ln_Succ)
		table = ptn; /* after this we want to insert the new node */
	ptn=AllocMem(sizeof(struct PartitionTableNode), MEMF_PUBLIC | MEMF_CLEAR);
	if (ptn)
	{
		ptn->ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
		if (ptn->ln.ln_Name)
		{
			ptn->ph = pn->ph;
			count = 0;
			while (table->ln.ln_Name[count] == ' ')
				ptn->ln.ln_Name[count++]=' ';
			if (count == 0)
				while (count!=3)
					ptn->ln.ln_Name[count++]=' ';
			strcat(ptn->ln.ln_Name, "new subpartition");
			NEWLIST(&ptn->pl);
			Insert(&pt_list, &ptn->ln, &table->ln);
			SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_HARDDISK-ID_MAIN_FIRST_GADGET].gadget,
				0,
				0,
				hdtags
			);
			return;
		}
		FreeMem(ptn, sizeof(struct PartitionTableNode));
	}
}

struct PartitionTableNode *newPartitionTable
	(
		struct Window *mainwin,
		struct PartitionHandle *ph,
		char *name,
		ULONG unit
	)
{
struct PartitionTableNode *ptn;

	ptn=AllocMem(sizeof(struct PartitionTableNode), MEMF_PUBLIC | MEMF_CLEAR);
	if (ptn)
	{
		ptn->ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
		if (ptn->ln.ln_Name)
		{
			ptn->ph = ph;
			ptn->tattrlist = QueryPartitionTableAttrs(ph);
			ptn->pattrlist = QueryPartitionAttrs(ph);
			GetPartitionTableAttrsA
			(
				ph,
				PTT_DOSENVEC, &ptn->de,
				PTT_TYPE, &ptn->type,
				PTT_RESERVED, &ptn->reserved,
				PTT_MAX_PARTITIONS, &ptn->maxpartitions,
				TAG_DONE
			);
			sprintf
			(
				ptn->ln.ln_Name, "%s   %04d%04d    0",
				name,
				(UWORD)(unit>>16),
				(UWORD)(unit & 0xFFFF)
			);
			NEWLIST(&ptn->pl);
			AddTail(&pt_list, &ptn->ln);
			SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_HARDDISK-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,
				0,
				hdtags
			);
			return ptn;
		}
		FreeMem(ptn, sizeof(struct PartitionTableNode));
	}
	return 0;
}

LONG findPT
	(
		struct Window *mainwin,
		struct PartitionHandle *ph,
		char *name,
		ULONG unit
	)
{
struct PartitionHandle *node;
char str[100];
ULONG i;

	if (OpenPartitionTable(ph)==0)
	{
		newPartitionTable(mainwin,ph,name,unit);
		node = (struct PartitionHandle *)ph->table->list.lh_Head;
		i = 0;
		while (node->ln.ln_Succ)
		{
			strcpy(str, "   subtable");
			findPT(mainwin, node,str,(i<<16)+unit);
			i++;
			node = (struct PartitionHandle *)node->ln.ln_Succ;
		}
		return 1;
	}
	return 0;
}

void findPartitionTables(struct Window *mainwin, char *device, int maxUnits) {
struct HDUnitNode *hd;

	hdtags[0].ti_Data = (STACKIPTR)&pt_list;
	findHDs(device, maxUnits);
	hd = (struct HDUnitNode *)hd_list.lh_Head;
	while (hd->ln.ln_Succ)
	{
		if (findPT(mainwin, hd->root, hd->ln.ln_Name, hd->unit)==0)
			newPartitionTable(mainwin,hd->root,hd->ln.ln_Name,hd->unit);
		hd = (struct HDUnitNode *)hd->ln.ln_Succ;
	}
}

BOOL makePartitionTable(struct PartitionTableNode *table) {
struct List list;
struct Node *node;
ULONG i=0;
BOOL retval=FALSE;

	NEWLIST(&list);
	while (PartitionBase->tables[i])
	{
		node = AllocMem(sizeof(struct Node), MEMF_PUBLIC);
		if (node)
		{
			node->ln_Name = PartitionBase->tables[i]->pti_Name;
			node->ln_Type = PartitionBase->tables[i]->pti_Type;
			AddTail(&list, node);
		}
		i++;
	}
	if (RequestList(&list, &i))
	{
		node = getNumNode(&list, i);
		i = node->ln_Type;
		if (CreatePartitionTable(table->ph, i) == 0)
		{
			table->tattrlist = QueryPartitionTableAttrs(table->ph);
			table->pattrlist = QueryPartitionAttrs(table->ph);
			GetPartitionTableAttrsA
			(
				table->ph,
				PTT_DOSENVEC, &table->de,
				PTT_TYPE, &table->type,
				PTT_RESERVED, &table->reserved,
				PTT_MAX_PARTITIONS, &table->maxpartitions,
				TAG_DONE
			);
			retval = TRUE;
		}
	}
	while ((node = RemTail(&list)))
		FreeMem(node, sizeof(struct Node));
	return retval;
};

struct PartitionTableNode *getPartitionTable(struct Window *mainwin, int num) {
struct PartitionTableNode *current_pt;

	current_pt=(struct PartitionTableNode *)getNumNode(&pt_list, num);
	if (current_pt)
	{
	BOOL part=TRUE;
		if (current_pt->type == PHPTT_UNKNOWN)
		{
		struct EasyStruct es =
		{
			sizeof(struct EasyStruct), 0,
			"HDToolBox",
			"This medium doesn't seem to contain a partition table.\n"
			"Do you want to initialize it?",
			"Yes|No"
		};
			if (EasyRequestArgs(0, &es, 0, 0))
			{
				if (makePartitionTable(current_pt))
					current_pt->flags |= PNF_TABLE_CHANGED;
				else
					part = FALSE;
			}
			else
				part = FALSE;
		}
/*		cdttags[0].ti_Data = FALSE;
		mbbltags[0].ti_Data = FALSE;
		llftags[0].ti_Data = FALSE;*/
		pdtags[0].ti_Data = part ? FALSE : TRUE;
//		vdodtags[0].ti_Data = FALSE;
		sctdtags[0].ti_Data = current_pt->flags & PNF_TABLE_CHANGED ? FALSE : TRUE;
/*		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_CHANGE_DRIVE_TYPE-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,cdttags
			);
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_MODIFY_BBL-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,mbbltags
			);
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_LL_FORMAT-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,llftags
			);*/
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_PARTITION_DRIVE-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,pdtags
			);
/*		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_VERIFY_DD-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,vdodtags
			);*/
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_SAVE_CHANGES-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,sctdtags
			);
	}
	return current_pt;
}

BOOL reallyExit(struct List *ptlist) {
struct PartitionTableNode *pt;
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox",
		"Some partitions have been changed.\n"
		"Are you sure to quit?",
		"Yes|No"
	};

	pt = (struct PartitionTableNode *)ptlist->lh_Head;
	while (pt->ln.ln_Succ)
	{
		if (pt->flags & PNF_TABLE_CHANGED)
			return EasyRequestArgs(0, &es, 0, 0);
		pt = (struct PartitionTableNode *)pt->ln.ln_Succ;
	}
	return TRUE;
}

void freePartitionTable(struct PartitionTableNode *ptn) {
kprintf("close\n");
	ClosePartitionTable(ptn->ph);
kprintf("close done\n");
	FreeVec(ptn->ln.ln_Name);
	FreeMem(ptn, sizeof(struct PartitionTableNode));
}

void freePartitionTableList(struct List *ptlist) {
struct PartitionTableNode *ptn;

	while ((ptn = (struct PartitionTableNode *)RemHead(ptlist)))
		freePartitionTable(ptn);
	freeHDList(&hd_list);
}

void freeHDList(struct List *hdlist) {
struct HDUnitNode *hdunit;

	while ((hdunit = (struct HDUnitNode *)RemHead(hdlist)))
	{
		CloseRootPartition(hdunit->root);
		FreeVec(hdunit->ln.ln_Name);
		FreeMem(hdunit, sizeof(struct HDUnitNode));
	}
}

void saveChanges(struct Window *mainwin, struct PartitionTableNode *table) {
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox",
		"\nAll data on modified partitions will be destroyed!\n"
		"Are you sure to write changes to disk?",
		"Yes|No"
	};
ULONG args[40];

	if (EasyRequestArgs(0, &es, 0, args))
	{
		WritePartitionTable(table->ph);
		table->flags &= ~PNF_TABLE_CHANGED;
		sctdtags[0].ti_Data = TRUE;
		SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_SAVE_CHANGES-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,sctdtags
			);
	}
}

