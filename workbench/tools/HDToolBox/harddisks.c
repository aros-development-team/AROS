/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/partition.h>
#include <devices/scsidisk.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <stdio.h>
#define DEBUG 1
#include "debug.h"

#include "harddisks.h"
#include "hdtoolbox_support.h"
#include "platform.h"

extern struct GUIGadgets gadgets;

void w2strcpy(STRPTR name, UWORD *wstr, ULONG len) {

	while (len)
	{
		*((UWORD *)name) = AROS_BE2WORD(*wstr);
		name += sizeof(UWORD);
		len -= 2;
		wstr++;
	}
	name -= 2;
	while ((*name==0) || (*name==' '))
		*name-- = 0;
}

BOOL identify(struct IOStdReq *ioreq, STRPTR name) {
struct SCSICmd scsicmd;
UWORD data[256];
UBYTE cmd=0xEC; /* identify */

	scsicmd.scsi_Data = data;
	scsicmd.scsi_Length = 512;
	scsicmd.scsi_Command = &cmd;
	scsicmd.scsi_CmdLength = 1;
	ioreq->io_Command = HD_SCSICMD;
	ioreq->io_Data = &scsicmd;
	ioreq->io_Length = sizeof(struct SCSICmd);
	if (DoIO((struct IORequest *)ioreq))
		return FALSE;
	w2strcpy(name, &data[27], 40);
	return TRUE;
}

void findHDs(struct ListNode *parent) {
struct IOStdReq *ioreq;
struct MsgPort *mp;
struct HDNode *node;
int i;

	mp = CreateMsgPort();
	if (mp)
	{
		ioreq = (struct IOStdReq *)CreateIORequest(mp, sizeof(struct IOStdReq));
		if (ioreq)
		{
			for (i=0;i<8;i++)
			{
				node = AllocMem(sizeof(struct HDNode), MEMF_PUBLIC | MEMF_CLEAR);
				if (node)
				{
					node->root_partition.listnode.ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
					if (node->root_partition.listnode.ln.ln_Name)
					{
						if (InitListNode(&node->root_partition.listnode, parent))
						{
							node->root_partition.listnode.ln.ln_Type = LNT_Harddisk;
							node->unit = i;
							if (OpenDevice(parent->ln.ln_Name, i, (struct IORequest *)ioreq, 0) == 0)
							{
								if (!identify(ioreq, node->root_partition.listnode.ln.ln_Name))
									sprintf(node->root_partition.listnode.ln.ln_Name, "Unit %d", i);
								CloseDevice((struct IORequest *)ioreq);
								node->root_partition.ph = OpenRootPartition(parent->ln.ln_Name, node->unit);
								if (node->root_partition.ph)
								{
									AddTail(&parent->list, &node->root_partition.listnode.ln);
									if (findPartitionTable(&node->root_partition))
									{
										findPartitions(&node->root_partition.listnode, &node->root_partition);
										node->root_partition.listnode.flags |= LNF_Listable;
									}
									GetPartitionAttrsA
									(
										node->root_partition.ph,
										PT_GEOMETRY, &node->root_partition.dg,
										PT_DOSENVEC, &node->root_partition.de,
										TAG_DONE
									);
									continue;
								}
							}
							UninitListNode(&node->root_partition.listnode);
						}
						FreeVec(node->root_partition.listnode.ln.ln_Name);
					}
					FreeMem(node, sizeof(struct HDNode));
				}
			}
			DeleteIORequest((struct IORequest *)ioreq);
		}
		DeleteMsgPort(mp);
	}
}

void freeHDList(struct List *list) {
struct HDNode *node;
struct HDNode *next;

	node = (struct HDNode *)list->lh_Head;
	while (node->root_partition.listnode.ln.ln_Succ)
	{
		next = (struct HDNode *)node->root_partition.listnode.ln.ln_Succ;
		if (node->root_partition.listnode.ln.ln_Type != LNT_Parent)
		{
			Remove(&node->root_partition.listnode.ln);
			if (node->root_partition.table)
			{
				freePartitionList(&node->root_partition.listnode.list);
				freePartitionTable(&node->root_partition);
			}
			CloseRootPartition(node->root_partition.ph);
			UninitListNode(&node->root_partition.listnode);
			FreeVec(node->root_partition.listnode.ln.ln_Name);
			FreeMem(node, sizeof(struct HDNode));
		}
		node = next;
	}
}

