/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/partition.h>

#include <devices/scsidisk.h>
#include <exec/io.h>
#include <exec/memory.h>

#include <stdio.h>

#define DEBUG 0
#include "debug.h"

#include "harddisks.h"
#include "hdtoolbox_support.h"
#include "platform.h"

extern struct GUIGadgets gadgets;

static void strcatlen(STRPTR dst, UBYTE *src, UBYTE maxlen)
{
    dst += strlen(dst);
    memcpy(dst, src, maxlen);
    while (maxlen-- > 0) {
        if (dst[maxlen] != ' ')
            break;
        dst[maxlen] = 0;
    }
}

static BOOL identify(struct IOStdReq *ioreq, STRPTR name)
{
    struct SCSICmd scsicmd = {0};
    UBYTE data[36 + 1];
    UBYTE cmd[6] = { 0x12, 0, 0, 0, 36, 0 }; /* inquiry */
    WORD i;

    D(bug("[HDToolBox] inquiry('%s')\n", name));

    scsicmd.scsi_Data = (UWORD*)data;
    scsicmd.scsi_Length = 36;
    scsicmd.scsi_Command = cmd;
    scsicmd.scsi_CmdLength = sizeof cmd;
    scsicmd.scsi_Flags = SCSIF_READ;
    ioreq->io_Command = HD_SCSICMD;
    ioreq->io_Data = &scsicmd;
    ioreq->io_Length = sizeof(struct SCSICmd);

    if (DoIO((struct IORequest *)ioreq))
        return FALSE;

    name[0] = 0;
    i = 4 + data[4];
    if (i >= 16)
        strcatlen(name, &data[8], 8);
    if (i >= 32) {
        strcat(name, " ");
        strcatlen(name, &data[16], 16);
    }
    if (i >= 36) {
        strcat(name, " ");
        strcatlen(name, &data[32], 4);
    }
    return TRUE;
}

void findHDs(struct HDTBDevice *parent)
{
    struct IOStdReq *ioreq;
    struct MsgPort *mp;
    struct HDNode *node;
    int i;

    D(bug("[HDToolBox] findHDs()\n"));

    mp = CreateMsgPort();
    if (mp)
    {
        ioreq = (struct IOStdReq *)CreateIORequest(mp, sizeof(struct IOStdReq));
        if (ioreq)
        {
	    int maxunits = 8;
	    if (parent->maxunits > maxunits)
		maxunits = parent->maxunits;

            for (i=0;i<maxunits;i++)
            {
                node = AllocMem(sizeof(struct HDNode), MEMF_PUBLIC | MEMF_CLEAR);
                if (node)
                {
                    node->root_partition.listnode.ln.ln_Name = AllocVec(100, MEMF_PUBLIC | MEMF_CLEAR);
                    if (node->root_partition.listnode.ln.ln_Name)
                    {
                        if (InitListNode(&node->root_partition.listnode, (struct ListNode *)parent))
                        {
                            node->root_partition.listnode.ln.ln_Type = LNT_Harddisk;
                            node->unit = i;
                            if (OpenDevice(parent->listnode.ln.ln_Name, i, (struct IORequest *)ioreq, 0) == 0)
                            {
                                D(bug("[HDToolBox] findHDs: Opened %s:%d\n",parent->listnode.ln.ln_Name, i));
                                if (!identify(ioreq, node->root_partition.listnode.ln.ln_Name))
                                    sprintf(node->root_partition.listnode.ln.ln_Name, "Unit %d", i);
                                CloseDevice((struct IORequest *)ioreq);
                                node->root_partition.ph = OpenRootPartition(parent->listnode.ln.ln_Name, node->unit);
                                if (node->root_partition.ph)
                                {
				    D(bug("[HDToolBox] - appending ROOT partition %p to list %p\n", &node->root_partition.listnode.ln, &parent->listnode.list));
				    D(bug("[HDToolBox] - first entry at %p\n", node->root_partition.listnode.list.lh_Head));
                                    AddTail(&parent->listnode.list, &node->root_partition.listnode.ln);
                                    if (findPartitionTable(&node->root_partition))
                                    {
					D(bug("[HDToolBox] - partition table found. searching for partitions\n"));
                                        findPartitions(&node->root_partition.listnode, &node->root_partition);
                                        node->root_partition.listnode.flags |= LNF_Listable;
                                    }
				    else
				    {
					D(bug("[HDToolBox] - partition table not found.\n"));
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
    D(bug("[HDToolBox] findHDs() successful\n"));
}

void freeHDList(struct List *list)
{
    struct HDNode *node;
    struct HDNode *next;

    D(bug("[HDToolBox] freeHDList(%p)\n", list));

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
