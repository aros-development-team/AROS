/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
#include "gui.h"
#include "hdtoolbox_support.h"
#include "platform.h"

#define DEBUG 1
#include "debug.h"

extern struct GUIGadgets gadgets;

void getPartitionInfo(struct PartitionTable *table, struct PartitionHandle *ph) {

	table->tattrlist = QueryPartitionTableAttrs(ph);
	table->pattrlist = QueryPartitionAttrs(ph);
	GetPartitionTableAttrsA
	(
		ph,
		PTT_TYPE, &table->type,
		PTT_RESERVED, &table->reserved,
		PTT_MAX_PARTITIONS, &table->max_partitions,
		TAG_DONE
	);
kprintf("type=%ld\n", table->type);
}

struct PartitionTable *newPartitionTable(struct PartitionHandle *ph) {
struct PartitionTable *table;

	table = AllocMem(sizeof(struct PartitionTable), MEMF_PUBLIC | MEMF_CLEAR);
	if (table)
		getPartitionInfo(table, ph);
	return table;
}

BOOL findPartitionTable(struct HDTBPartition *partition) {

	if (OpenPartitionTable(partition->ph) == 0)
	{
		partition->table = newPartitionTable(partition->ph);
		if (partition->table != NULL)
			return TRUE;
		ClosePartitionTable(partition->ph);
	}
	return FALSE;
}

void freePartitionTable(struct HDTBPartition *partition) {

	ClosePartitionTable(partition->ph);
	FreeMem(partition->table, sizeof(struct PartitionTable));
}

BOOL makePartitionTable(struct HDTBPartition *table, ULONG type) {

	if (table->table)
	{
		/* if there is already a partition table then free it */
		freePartitionList(&table->listnode.list);
		DestroyPartitionTable(table->ph);
	}
	else
	{
		table->table = AllocMem(sizeof(struct PartitionTable), MEMF_PUBLIC | MEMF_CLEAR);
	}
	if (table->table)
	{
		if (CreatePartitionTable(table->ph, type) == 0)
		{
			getPartitionInfo(table->table, table->ph);
			return TRUE;
		}
	}
	return FALSE;
};

/**************************************************************************/

#if 0
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
		struct HDTBPartition *table,
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
		struct HDTBPartition *table,
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

#error TODO: pass DOS device name in params[0] and set handler name manually
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
					dn->dn_Name = MKBADDR(AllocVec(AROS_BSTR_MEMSIZE4LEN(strlen(name)), MEMF_PUBLIC));
					dn->dn_Ext.dn_AROS.dn_DevName = AROS_BSTR_ADDR(dn->dn_Name);

					i = 0;
					do
					{
						AROS_BSTR_putchar(dn->dn_Name, i, name[i]);
					} while (name[i++]);
					AROS_BSTR_setstrlen(dn->dn_Name, i-1);
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
#endif
