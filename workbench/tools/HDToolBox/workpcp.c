/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <strings.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <aros/debug.h>
#include <devices/trackdisk.h>
#include <exec/memory.h>
#include <utility/tagitem.h>

#include "workpcp.h"
#include "gadgets.h"
#include "listfunctions.h"
#include "pcpartitiontable.h"

extern struct List partition_list;
extern struct TagItem pcpartitiontags[], pcpdeletepartitiontags[],
		pcpeditarospartitiontags[], pcpstartcyltags[], pcpendcyltags[], pcptotalcyltags[],
		pcptypelvtags[], pcptypeintegertags[];
extern struct creategadget pcpgadgets[];

void setPCPGadgetAttrs(struct Window *mainwin) {
	pcpdeletepartitiontags[0].ti_Data = TRUE;
	pcpeditarospartitiontags[0].ti_Data = TRUE;
	pcpstartcyltags[0].ti_Data = TRUE;
	pcpendcyltags[0].ti_Data = TRUE;
	pcptotalcyltags[0].ti_Data = TRUE;
	pcptypelvtags[0].ti_Data = TRUE;
	pcptypelvtags[2].ti_Data = 0;
	pcptypelvtags[3].ti_Data = ~0;
	pcptypelvtags[4].ti_Data = 0;
	pcptypeintegertags[0].ti_Data = TRUE;
	pcptypeintegertags[1].ti_Data = 0;
	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_DELETE_PARTITION-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcpdeletepartitiontags
		);
/*	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_EDIT_PARTITION-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcpeditarospartitiontags
		);*/
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
			pcpgadgets[ID_PCP_TYPELV-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypelvtags
		);
	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_TYPEINTEGER-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypeintegertags
		);
}

void findPartitions(struct Window *mainwin, struct HDUnitNode *hdunit) {
struct PCPartitionTable *pcpt=(struct PCPartitionTable *)&hdunit->mbr_copy[0x1BE];
struct PartitionNode *pnode;
int i;

#warning "Check for RDB"
	CopyMem(hdunit->mbr, hdunit->mbr_copy, 512);
	pcpartitiontags[0].ti_Data = (STACKIPTR)&partition_list;
	for (i=0;i<4;i++)
	{
		if (pcpt[i].first_sector)
		{
			pnode = (struct PartitionNode *)AllocMem
				(
					sizeof(struct PartitionNode),
					MEMF_PUBLIC | MEMF_CLEAR
				);
			if (pnode)
			{
				pnode->ln.ln_Name = AllocMem(100,MEMF_PUBLIC | MEMF_CLEAR);
				if (pnode->ln.ln_Name)
				{
					sprintf
						(
							pnode->ln.ln_Name,
							"%s Primary-%d",
							pcpt[i].status == 0x80 ? "Boot" : "    ",
							i
						);
					pnode->ln.ln_Pri = -i;
					pnode->pt_entry = &pcpt[i];
					AddTail(&partition_list, &pnode->ln);
					SetGadgetAttrsA
						(
							pcpgadgets[ID_PCP_PARTITION-ID_PCP_FIRST_GADGET].gadget,
							mainwin, 0, pcpartitiontags
						);
					continue;
				}
				FreeMem(pnode, sizeof(struct PartitionNode));
			}
		}
	}
} 

void viewPartitionData
	(
		struct Window *mainwin,
		struct HDUnitNode *hdunit,
		struct PartitionNode *pn
	)
{
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox - Warning",
		"Partition is not cylinder aligned!!!\n"
		"I wouldn't change anything if this partition\n"
		"contains important data!",
		"Ok",
	};

	if (
			(pn->pt_entry->first_sector % hdunit->geometry.dg_CylSectors) ||
			(pn->pt_entry->count_sector % hdunit->geometry.dg_CylSectors)
		)
		EasyRequestArgs(0, &es, 0, 0);
	pcpdeletepartitiontags[0].ti_Data = FALSE;
//	pcpeditarospartitiontags[0].ti_Data = FALSE;
#warning "non cylinder aligned partitions!"
	pcpstartcyltags[0].ti_Data = FALSE;
	pcpstartcyltags[1].ti_Data =
		(pn->pt_entry->first_sector)/hdunit->geometry.dg_CylSectors;
	pcptotalcyltags[0].ti_Data = FALSE;
	pcptotalcyltags[1].ti_Data =
		pn->pt_entry->count_sector/hdunit->geometry.dg_CylSectors;
	pcpendcyltags[0].ti_Data = FALSE;
	pcpendcyltags[1].ti_Data = pcpstartcyltags[1].ti_Data+pcptotalcyltags[1].ti_Data-1;
	pcptypelvtags[0].ti_Data = FALSE;
	pcptypelvtags[2].ti_Data = pn->pt_entry->type;
	pcptypelvtags[3].ti_Data = pn->pt_entry->type;
	pcptypeintegertags[0].ti_Data = FALSE;
	pcptypeintegertags[1].ti_Data = pn->pt_entry->type;

	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_DELETE_PARTITION-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcpdeletepartitiontags
		);
/*	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_EDIT_PARTITION-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcpeditarospartitiontags
		);*/
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
			pcpgadgets[ID_PCP_TYPELV-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypelvtags
		);
	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_TYPEINTEGER-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypeintegertags
		);
}

struct PartitionNode *getPartition(struct Window *mainwin, struct HDUnitNode *hdunit, int num) {
struct PartitionNode *current_partition;

	current_partition = (struct PartitionNode *)getNumNode(&partition_list, num);
	if (current_partition)
	{
		viewPartitionData(mainwin, hdunit, current_partition);
	}
	return current_partition;
}

void freePartitionNode(struct PartitionNode *pn) {
	FreeMem(pn->ln.ln_Name, 100);
	FreeMem(pn, sizeof(struct PartitionNode));
}

void freePartitionList(struct List *partition_list) {
struct PartitionNode *pn;

	while ((pn = (struct PartitionNode *)RemHead(partition_list)))
	{
		freePartitionNode(pn);
	}
}

void pcp_Ok(struct List *hdlist) {
struct HDUnitNode *hdun;

	hdun = (struct HDUnitNode *)hdlist->lh_Head;
	while (hdun->ln.ln_Succ)
	{
		CopyMem(hdun->mbr_copy, hdun->mbr, 512);
		hdun = (struct HDUnitNode *)hdun->ln.ln_Succ;
	}
	freePartitionList(&partition_list);
}

void pcp_Cancel() {

	freePartitionList(&partition_list);
}

void setComponents
	(
		struct DriveGeometry *dg,
		ULONG sector,
		char *head,
		char *tracksector,
		char *cylinder
	)
{
ULONG val;

	val = sector / dg->dg_TrackSectors;
	*cylinder = val / dg->dg_Heads;
	*head = val % (dg->dg_Heads);
	*tracksector = (sector % dg->dg_TrackSectors) + 1;
}

BOOL validValue(struct List *partition_list, struct PartitionNode *current, ULONG value) {
struct PartitionNode *pn;

	pn = (struct PartitionNode *)partition_list->lh_Head;
	while (pn->ln.ln_Succ)
	{
		/* dont't check currently processed partition */
		if (current != pn)
		{
			if (
					(value>=pn->pt_entry->first_sector) &&
					(value<(pn->pt_entry->first_sector+pn->pt_entry->count_sector))
				)
				return FALSE;
		}
		pn = (struct PartitionNode *)pn->ln.ln_Succ;
	}
	return TRUE;
}

BOOL changeStartCyl
	(
		struct Window *mainwin,
		struct HDUnitNode *hdunit,
		struct PartitionNode *pn,
		ULONG value
	)
{

	value *= hdunit->geometry.dg_CylSectors;
	if (value != pn->pt_entry->first_sector)
	{
		if (value>=hdunit->geometry.dg_CylSectors)
		{
			if (validValue(&partition_list, pn, value))
			{
				/* Do we add space to partition ? */
				if (value<pn->pt_entry->first_sector)
				{
					/* if so add it to total cylinders */
					pn->pt_entry->count_sector = 
						pn->pt_entry->count_sector+(pn->pt_entry->first_sector-value);
				}
				else
				{
					/* if not sub it from total cylinders */
					pn->pt_entry->count_sector = 
						pn->pt_entry->count_sector-(value-pn->pt_entry->first_sector);
				}
				/* make changes visible */
				pcptotalcyltags[1].ti_Data =
					pn->pt_entry->count_sector/hdunit->geometry.dg_CylSectors;
				SetGadgetAttrsA
					(
						pcpgadgets[ID_PCP_TOTALCYL-ID_PCP_FIRST_GADGET].gadget,
						mainwin,0,pcptotalcyltags
					);
				/* set new starting sector */
				pn->pt_entry->first_sector = value;
				setComponents
					(
						&hdunit->geometry,
						pn->pt_entry->first_sector,
						&pn->pt_entry->start_head,
						&pn->pt_entry->start_sector,
						&pn->pt_entry->start_cylinder
					);
				return TRUE;
			}
			else
			{
				/* Input wasn't valid */
				pcpstartcyltags[1].ti_Data =
					pn->pt_entry->first_sector/hdunit->geometry.dg_CylSectors;
				SetGadgetAttrsA
					(
						pcpgadgets[ID_PCP_STARTCYL-ID_PCP_FIRST_GADGET].gadget,
						mainwin,0,pcpstartcyltags
					);
			}
		}
		else
		{
			pcpstartcyltags[1].ti_Data =
				pn->pt_entry->first_sector/hdunit->geometry.dg_CylSectors;
			SetGadgetAttrsA
				(
					pcpgadgets[ID_PCP_STARTCYL-ID_PCP_FIRST_GADGET].gadget,
					mainwin,0,pcpstartcyltags
				);
		}
	}
	return FALSE;
}

BOOL changeEndCyl
	(
		struct Window *mainwin,
		struct HDUnitNode *hdunit,
		struct PartitionNode *pn,
		ULONG value
	)
{

	value *= hdunit->geometry.dg_CylSectors;
	if (value>pn->pt_entry->first_sector)
	{
		if (value !=
				(
					pn->pt_entry->count_sector+
					pn->pt_entry->first_sector-
					hdunit->geometry.dg_CylSectors
				)
			)
		{
			if (validValue(&partition_list, pn, value))
			{
				pn->pt_entry->count_sector =
					value-pn->pt_entry->first_sector+hdunit->geometry.dg_CylSectors;
				setComponents
					(
						&hdunit->geometry,
						value,
						&pn->pt_entry->end_head,
						&pn->pt_entry->end_sector,
						&pn->pt_entry->end_cylinder
					);
				pcptotalcyltags[1].ti_Data =
					pn->pt_entry->count_sector/
					hdunit->geometry.dg_CylSectors;
				/* make changes visible */
				SetGadgetAttrsA
					(
						pcpgadgets[ID_PCP_TOTALCYL-ID_PCP_FIRST_GADGET].gadget,
						mainwin,0,pcptotalcyltags
					);
				return TRUE;
			}
			else
			{
				/* invalid entry */
				pcpendcyltags[1].ti_Data =
						(pn->pt_entry->first_sector+pn->pt_entry->count_sector-1)/
						hdunit->geometry.dg_CylSectors;
				SetGadgetAttrsA
					(
						pcpgadgets[ID_PCP_ENDCYL-ID_PCP_FIRST_GADGET].gadget,
						mainwin,0,pcpendcyltags
					);
			}
		}
	}
	else
	{
		/* value is smaller than first sector */
		pcpendcyltags[1].ti_Data =
				(pn->pt_entry->first_sector+pn->pt_entry->count_sector-1)/
				hdunit->geometry.dg_CylSectors;
		SetGadgetAttrsA
			(
				pcpgadgets[ID_PCP_ENDCYL-ID_PCP_FIRST_GADGET].gadget,
				mainwin,0,pcpendcyltags
			);
	}
	return FALSE;
}

BOOL changeTotalCyl
	(
		struct Window *mainwin,
		struct HDUnitNode *hdunit,
		struct PartitionNode *pn,
		ULONG value
	)
{
	value *= hdunit->geometry.dg_CylSectors;
	if (value != pn->pt_entry->count_sector)
	{
		if (validValue(&partition_list, pn, value+pn->pt_entry->first_sector-1))
		{
			pn->pt_entry->count_sector = value;
			setComponents
				(
					&hdunit->geometry,
					pn->pt_entry->first_sector+pn->pt_entry->count_sector-1,
					&pn->pt_entry->end_head,
					&pn->pt_entry->end_sector,
					&pn->pt_entry->end_cylinder
				);
			/* make changes visible */
			pcpendcyltags[1].ti_Data =
					(pn->pt_entry->first_sector+pn->pt_entry->count_sector-1)/
					hdunit->geometry.dg_CylSectors;
			SetGadgetAttrsA
				(
					pcpgadgets[ID_PCP_ENDCYL-ID_PCP_FIRST_GADGET].gadget,
					mainwin,0,pcpendcyltags
				);
			return TRUE;
		}
		else
		{
			/* invalid Value */
			pcptotalcyltags[1].ti_Data =
				pn->pt_entry->count_sector/hdunit->geometry.dg_CylSectors;
			SetGadgetAttrsA
				(
					pcpgadgets[ID_PCP_TOTALCYL-ID_PCP_FIRST_GADGET].gadget,
					mainwin,0,pcptotalcyltags
				);
		}
	}
	return FALSE;
}

BOOL changeType
	(
		struct Window *mainwin,
		struct PartitionNode *pn,
		ULONG value
	)
{
BOOL retval = FALSE;

	if ((value>0) && (value<=255))
	{
		pn->pt_entry->type = (char)value;
		retval = TRUE;
	}
	pcptypelvtags[2].ti_Data = pn->pt_entry->type;
	pcptypelvtags[3].ti_Data = pn->pt_entry->type;
	pcptypeintegertags[1].ti_Data = pn->pt_entry->type;
	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_TYPELV-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypelvtags
		);
	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_TYPEINTEGER-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypeintegertags
		);
	return retval;
}

void deletePartition(struct Window *mainwin, struct PartitionNode *pn) {

	pn->pt_entry->status=0;
	pn->pt_entry->start_head=0;
	pn->pt_entry->start_sector=0;
	pn->pt_entry->start_cylinder=0;
	pn->pt_entry->type=0;
	pn->pt_entry->end_head=0;
	pn->pt_entry->end_sector=0;
	pn->pt_entry->end_cylinder=0;
	pn->pt_entry->first_sector=0;
	pn->pt_entry->count_sector=0;
	Remove(&pn->ln);
	freePartitionNode(pn);
	pcpdeletepartitiontags[0].ti_Data = TRUE;
	pcpstartcyltags[0].ti_Data = TRUE;
	pcpendcyltags[0].ti_Data = TRUE;
	pcptotalcyltags[0].ti_Data = TRUE;
	pcpeditarospartitiontags[0].ti_Data = TRUE;
	pcptypelvtags[0].ti_Data = TRUE;
	pcptypeintegertags[0].ti_Data = TRUE;
	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_PARTITION-ID_PCP_FIRST_GADGET].gadget,
			mainwin,	0, pcpartitiontags
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
			pcpgadgets[ID_PCP_TYPELV-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypelvtags
		);
	SetGadgetAttrsA
		(
			pcpgadgets[ID_PCP_TYPEINTEGER-ID_PCP_FIRST_GADGET].gadget,
			mainwin,0,pcptypeintegertags
		);
}

struct SpaceFind {
	ULONG start;
	ULONG size;
};

BOOL findSpace(struct List *plist, struct HDUnitNode *hdunit, ULONG *first, ULONG *count) {
struct PartitionNode *pn;
struct SpaceFind space[5]={{0,0},{0,0},{0,0},{0,0},{0,0}};
int i,j,last=0;

	*first=hdunit->geometry.dg_CylSectors;
#warning "When partioning visible let user select space"
	pn = (struct PartitionNode *)plist->lh_Head;
	while (pn->ln.ln_Succ)
	{
		if (last)
		{
			for (i=0;i<last;i++)
			{
				if (pn->pt_entry->first_sector<space[i].start)
				{
					for (j=last;j>0;j--)
					{
						space[j].start = space[j-1].start;
						space[j].size = space[j-1].size;
					}
					i = 0;
					break;
				}
			}
			space[i].start = pn->pt_entry->first_sector;
			space[i].size = pn->pt_entry->count_sector;
			last++;
		}
		else
		{
			space[0].start = pn->pt_entry->first_sector;
			space[0].size = pn->pt_entry->count_sector;
			last = 1;
		}
		pn = (struct PartitionNode *)pn->ln.ln_Succ;
	}
	if (last)
	{
		for (i=0;i<last;i++)
		{
			if (*first<space[i].start)
			{
				/* allocate between two partitions */
				*count = space[i].start-*first;
				return TRUE;
			}
			else
				*first = space[i].start+space[i].size;
		}
		/* space at the end of a partition */
		if (*first>hdunit->geometry.dg_TotalSectors)
			return FALSE;
		*count =	hdunit->geometry.dg_TotalSectors-*first;
	}
	else
	{
		/* whole hd is free */
		if (*first>hdunit->geometry.dg_TotalSectors)
			return FALSE;
		*count = hdunit->geometry.dg_TotalSectors-*first;
	}
	return TRUE;
}

struct PartitionNode *addPartition(struct Window *mainwin, struct HDUnitNode *hdunit) {
struct EasyStruct es =
	{
		sizeof(struct EasyStruct), 0,
		"HDToolBox",
		"There is no more space\n"
		"to add a new partition."
		"Cancel",
	};

struct PCPartitionTable *pcpt=(struct PCPartitionTable *)&hdunit->mbr_copy[0x1BE];
struct PartitionNode *pn;
int i,j;
char flags;
char choose[20];

	flags = 0;
	j = 0;
	for (i=0;i<4;i++)
	{
		if (pcpt[i].first_sector == 0)
		{
			choose[j++]=i+0x30;
			choose[j++]='|';
			choose[j]=0;
			flags |= 1<<i;
		}
	}
	if (flags)
	{
		strcat(choose, "Cancel");
		pn = AllocMem(sizeof(struct PartitionNode), MEMF_PUBLIC | MEMF_CLEAR);
		if (pn)
		{
			pn->ln.ln_Name = AllocMem(100, MEMF_PUBLIC | MEMF_CLEAR);
			if (pn->ln.ln_Name)
			{
				es.es_TextFormat = "Choose partition number";
				es.es_GadgetFormat = choose;
				flags = EasyRequestArgs(0, &es, 0, 0);
				if (flags)
				{
					flags = choose[(flags-1)*2]-0x30;
					sprintf(pn->ln.ln_Name, "     Primary-%d", flags);
					pn->ln.ln_Pri = -flags;
					pn->pt_entry = &pcpt[flags];
					if (findSpace
							(
								&partition_list,
								hdunit,
								&pn->pt_entry->first_sector,
								&pn->pt_entry->count_sector
							)
						)
					{
						setComponents
							(
								&hdunit->geometry,
								pn->pt_entry->first_sector,
								&pn->pt_entry->start_head,
								&pn->pt_entry->start_sector,
								&pn->pt_entry->start_cylinder
							);
						setComponents
							(
								&hdunit->geometry,
								pn->pt_entry->first_sector+pn->pt_entry->count_sector-1,
								&pn->pt_entry->end_head,
								&pn->pt_entry->end_sector,
								&pn->pt_entry->end_cylinder
							);
						pn->pt_entry->status = 0;
						pn->pt_entry->type = 0x30;
						Enqueue(&partition_list, &pn->ln);
						SetGadgetAttrsA
							(
								pcpgadgets[ID_PCP_PARTITION-ID_PCP_FIRST_GADGET].gadget,
								mainwin, 0, pcpartitiontags
							);
						viewPartitionData(mainwin, hdunit, pn);
						return pn;
					}
					else
					{
						es.es_TextFormat = 
							"There is no more space\n"
							"to add a new partition.";
						es.es_GadgetFormat = "Cancel";
						EasyRequestArgs(0, &es, 0, 0);
					}
				}
				FreeMem(pn->ln.ln_Name, 100);
			}
			FreeMem(pn, sizeof(struct PartitionNode));
		}
	}
	else
		EasyRequestArgs(0, &es, 0, 0);
	return 0;
}
