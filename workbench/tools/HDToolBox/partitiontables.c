/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/partition.h>

#include <devices/bootblock.h>
#include <devices/scsidisk.h>
#include <devices/trackdisk.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <libraries/expansion.h>
#include <libraries/partition.h>

#include "partitiontables.h"
#include "gadgets.h"
#include "hdtoolbox_support.h"
#include "partitiontypes.h"
#include "platform.h"

#define DEBUG 1
#include "debug.h"

struct List hd_list;
struct List pt_list;
extern struct TagItem hdtags[], cdttags[], mbbltags[], llftags[],
                      pdtags[], vdodtags[], sctdtags[];

extern struct creategadget maingadgets[];

void w2strcpy(STRPTR name, UWORD *wstr, ULONG len) {

	while (len)
	{
		*((UWORD *)name)++ = AROS_BE2WORD(*wstr);
		len -= 2;
		wstr++;
	}
	name -= 2;
	while ((*name==0) || (*name==' '))
		*name-- = 0;
}

BOOL identify(struct PartitionHandle *ph, STRPTR name) {
struct SCSICmd scsicmd;
UWORD data[256];
UBYTE cmd=0xEC; /* identify */
struct IOExtTD *ioreq = ph->bd->ioreq;

	scsicmd.scsi_Data = data;
	scsicmd.scsi_Length = 512;
	scsicmd.scsi_Command = &cmd;
	scsicmd.scsi_CmdLength = 1;
	ioreq->iotd_Req.io_Command = HD_SCSICMD;
	ioreq->iotd_Req.io_Data = &scsicmd;
	ioreq->iotd_Req.io_Length = sizeof(struct SCSICmd);
	if (DoIO((struct IORequest *)ioreq))
		return FALSE;
	w2strcpy(name, &data[27], 40);
	return TRUE;
}

void findHDs(char *device, ULONG maxUnits) {
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
			node->ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
			if (node->ln.ln_Name)
			{
				node->root = OpenRootPartition(device, i);
				if (node->root)
				{
					strcpy(node->devname, device);
					node->unit = i;
					if (!identify(node->root, node->ln.ln_Name))
						sprintf(node->ln.ln_Name, "%.*s", count, device);
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
			ptn->hd = pn->root->hd;
			count = 0;
			while (table->ln.ln_Name[count] == ' ')
				ptn->ln.ln_Name[count++]=' ';
			if (count == 0)
				while (count!=3)
					ptn->ln.ln_Name[count++]=' ';
			strcat(ptn->ln.ln_Name, "new subpartition");
			NEWLIST(&ptn->pl);
			Insert(&pt_list, &ptn->ln, &table->ln);
			GT_SetGadgetAttrsA
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
		struct HDUnitNode *hd,
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
			ptn->hd = hd;
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
			setPartitionType(ptn);
			sprintf
			(
				ptn->ln.ln_Name, "%s   %04d%04d    0",
				name,
				(UWORD)(unit>>16),
				(UWORD)(unit & 0xFFFF)
			);
			NEWLIST(&ptn->pl);
			AddTail(&pt_list, &ptn->ln);
			GT_SetGadgetAttrsA
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
		struct HDUnitNode *hd,
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
		newPartitionTable(mainwin, hd, ph, name, unit);
		node = (struct PartitionHandle *)ph->table->list.lh_Head;
		i = 0;
		while (node->ln.ln_Succ)
		{
			strcpy(str, "   subtable");
			findPT(mainwin, hd, node,str,(i<<16)+unit);
			i++;
			node = (struct PartitionHandle *)node->ln.ln_Succ;
		}
		return 1;
	}
	return 0;
}

void findPartitionTables(struct Window *mainwin, struct List *hdlist) {
struct HDUnitNode *hd;

	hdtags[0].ti_Data = (STACKIPTR)&pt_list;
	hd = (struct HDUnitNode *)hdlist->lh_Head;
	while (hd->ln.ln_Succ)
	{
		if (findPT(mainwin, hd, hd->root, hd->ln.ln_Name, hd->unit)==0)
			newPartitionTable(mainwin, hd, hd->root,hd->ln.ln_Name,hd->unit);
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
			setPartitionType(table);
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
/*		GT_SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_CHANGE_DRIVE_TYPE-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,cdttags
			);
		GT_SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_MODIFY_BBL-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,mbbltags
			);
		GT_SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_LL_FORMAT-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,llftags
			);*/
		GT_SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_PARTITION_DRIVE-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,pdtags
			);
/*		GT_SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_VERIFY_DD-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,vdodtags
			);*/
		GT_SetGadgetAttrsA
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
	ClosePartitionTable(ptn->ph);
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
		GT_SetGadgetAttrsA
			(
				maingadgets[ID_MAIN_SAVE_CHANGES-ID_MAIN_FIRST_GADGET].gadget,
				mainwin,0,sctdtags
			);
	}
}

ULONG getOffset(struct PartitionHandle *ph) {
struct DosEnvec de;
ULONG offset=0;

	while (ph->root)
	{
		GetPartitionAttrsA(ph, PT_DOSENVEC, de, TAG_DONE);
		offset += de.de_LowCyl;
		ph = ph->root;
	}
	return offset;
}

/* 
 Output: 0 - no mount (no partition change)
         1 - mount that device
         2 - reboot not really neccessary
             (FS not so important things changed like de_Mask)
         3 - reboot neccessary
             (FS important things changed like de_LowCyl)
*/
WORD checkMount
	(
		struct PartitionTableNode *table,
		STRPTR name,
		struct DosEnvec *de
	)
{
WORD retval = 1;
struct DosList *dl;
struct DeviceNode *entry;
ULONG i;

	dl = LockDosList(LDF_READ | LDF_DEVICES);
	if (dl)
	{
		entry = (struct DeviceNode *)FindDosEntry(dl, name, LDF_DEVICES);
		if (entry)
		{
		struct FileSysStartupMsg *fssm;
		struct DosEnvec *d_de;
		STRPTR devname;

			fssm = (struct FileSysStartupMsg *)BADDR(entry->dn_Startup);
			devname = AROS_BSTR_ADDR(fssm->fssm_Device);
			if (
					(fssm->fssm_Unit != table->hd->unit) ||
					(strcmp(devname, table->hd->devname))
				)
			{
				retval = 3; /* better do a reboot */
			}
			else
			{	
				d_de = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
				i = getOffset(table->ph);
				if (
						(d_de->de_SizeBlock != de->de_SizeBlock) ||
						(d_de->de_Reserved  != de->de_Reserved) ||
						(d_de->de_PreAlloc  != de->de_PreAlloc) ||
						(d_de->de_LowCyl    != (de->de_LowCyl+i)) ||
						(d_de->de_HighCyl   != (de->de_HighCyl+i)) ||
						(d_de->de_DosType   != de->de_DosType) ||
						(
							(
								/* at least one has de_BootBocks */
								(d_de->de_TableSize>=DE_BOOTBLOCKS) ||
								(de->de_TableSize>=DE_BOOTBLOCKS)
							) &&
							(
								/* if one has no de_BootBlock assume de_BootBlock change */
								(d_de->de_TableSize<DE_BOOTBLOCKS) ||
								(de->de_TableSize<DE_BOOTBLOCKS)
							)
						) ||
						(
							/* both have de_BootBlocks */
							(d_de->de_TableSize>=DE_BOOTBLOCKS) &&
							(de->de_TableSize>=DE_BOOTBLOCKS) &&
							(d_de->de_BootBlocks != de->de_BootBlocks)
						)
					)
				{
					retval = 3;
				}
				else if
					(
						(d_de->de_NumBuffers  != de->de_NumBuffers) ||
						(d_de->de_BufMemType  != de->de_BufMemType) ||
						(d_de->de_MaxTransfer != de->de_MaxTransfer) ||
						(d_de->de_Mask        != de->de_Mask) ||
						(d_de->de_BootPri     != de->de_BootPri)
					)
				{
					retval = 2;
				}
				else
					retval = 0;
			}
		}
		UnLockDosList(LDF_READ | LDF_DEVICES);
	}
	return retval;
}


void mount
	(
		struct PartitionTableNode *table,
		struct PartitionHandle *ph,
		STRPTR name,
		struct DosEnvec *de
	)
{
struct ExpansionBase *ExpansionBase;
struct DeviceNode *dn;
struct DosEnvec *nde;
IPTR *params;
ULONG i;

#warning "TODO: get filesystem"
	if ((de->de_DosType & 0xFFFFFF00) == BBNAME_DOS)
	{
		ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",41);
		if (ExpansionBase)
		{
			params = (IPTR *)AllocVec(sizeof(struct DosEnvec)+sizeof(IPTR)*4, MEMF_PUBLIC | MEMF_CLEAR);
			if (params)
			{
				nde = (struct DosEnvec *)&params[4];
				CopyMem(de, nde, sizeof(struct DosEnvec));
				params[0] = (IPTR)"afs.handler";
				params[1] = (IPTR)table->hd->devname;
				params[2] = (IPTR)table->hd->unit;
				params[3] = 0;
				i = getOffset(ph->root);
				nde->de_LowCyl += i;
				nde->de_HighCyl += i;
				dn = MakeDosNode(params);
				if (dn)
				{
					dn->dn_OldName = MKBADDR(AllocVec(strlen(name)+2, MEMF_PUBLIC));
#ifndef __AMIGAOS__
					dn->dn_NewName = AROS_BSTR_ADDR(dn->dn_OldName);
#endif
					i = 0;
					do
					{
						AROS_BSTR_putchar(dn->dn_OldName, i, name[i]);
					} while (name[i++]);
					AROS_BSTR_setstrlen(dn->dn_OldName, i-1);
					AddDosNode(nde->de_BootPri, ADNF_STARTPROC, dn);
				}
				else
					FreeVec(params);
			}
			CloseLibrary((struct Library *)ExpansionBase);
		}
	}
	else
		kprintf("ignored %s: unknown FS (0x%lx)\n", name, de->de_DosType);
}

void mountPartitions(struct List *ptlist) {
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox",
		0,
		"Yes|No"
	};
struct PartitionTableNode *table;
struct PartitionHandle *ph;
WORD cm;
WORD reboot=0;

	table = (struct PartitionTableNode *)ptlist->lh_Head;
	while (table->ln.ln_Succ)
	{
		if (table->type != PHPTT_UNKNOWN)
		{
			ph = (struct PartitionHandle *)table->ph->table->list.lh_Head;
			while (ph->ln.ln_Succ)
			{
				if (existsAttr(table->pattrlist, PTA_AUTOMOUNT))
				{
				LONG flag;

					GetPartitionAttrsA(ph, PT_AUTOMOUNT, &flag, TAG_DONE);
					if (flag)
					{
						if (existsAttr(table->pattrlist, PTA_NAME))
						{
						UBYTE name[32];
						struct DosEnvec de;
	
							GetPartitionAttrsA(ph, PT_NAME, name, PT_DOSENVEC, &de, TAG_DONE);
							cm = checkMount(table, name, &de);
							if (cm == 1)
								mount(table, ph, name, &de);
							else if (cm == 2)
								kprintf("may reboot\n");
							else if (cm == 3)
								kprintf("have to reboot\n");
							else
								kprintf("mount %s not needed\n", name);
							if (reboot<cm)
								reboot = cm;
						}
						else
							kprintf("Partition with no name is automountable\n");
					}
				}
				ph = (struct PartitionHandle *)ph->ln.ln_Succ;
			}
		}
		table = (struct PartitionTableNode *)table->ln.ln_Succ;
	}
	if (reboot>1)
	{
		if (reboot == 2)
		{
			es.es_TextFormat =
				"A reboot is not necessary because the changes do not\n"
				"affect the work of any running filesystem.\n"
				"Do you want to reboot anyway?";
		}
		else
		{
			es.es_TextFormat =
				"A reboot is required because the changes affect\n"
				"the work of at least one running filesystem.\n"
				"Do you want to reboot now?";
		}
		if (EasyRequestArgs(0, &es, 0, 0))
			ColdReboot();
	}
}

